# Introduction

**Libtrading** is an open source API that implements network protocols used for
communicating with exchanges, dark pools, and other trading venues. For a list
of supported protocols, see the *Supported Protocols* section.

## Features

* No memory allocation on the RX and TX paths
* Single-threaded, non-blocking I/O philosophy
* SystemTap/DTrace probes

# Supported Protocols

* [FAST][]
* [FIX][]

## FIX

It is pretty easy to start working with FIX. One just need to create and
initialize a FIX session properly. Once session is initialized a user is
immediately able to send and receive messages. Therefore, a typical FIX
work cycle consists of three principal stages: create a session, do basic
logic, destroy a session.

To run a new session one just need to invoke a function

```c
struct fix_session *fix_session_new(struct fix_session_cfg *cfg);
```

which accepts a desirable configuration as an input argument and returns
a new session in case of success. Configuration structure is just a set
of particular parameters applicable to the newly created session. Among
these are socket descriptor used for communication, SenderCompID and
TargetCompID, Heartbeat interval etc

The heart of FIX protocol is a FIX message. FIX message is a key instrument
used for communication. No wonder that libtrading introduces a special entity
to represent a message.

```c
struct fix_message;
```

FIX messages consist of FIX fields which fully determine the type of the
messages and are used to transfer data between counterparties. There are
a few mandatory fields that must be in every transferred FIX message.
Such fields can be directly addressed within *struct fix_message*. All
the remaining fields are currently held in a simple linear array.

```c
struct fix_message {
	/*
	 * These are required fields.
	 */
	const char			*begin_string;
	unsigned long			body_length;
	const char			*msg_type;
	const char			*sender_comp_id;
	const char			*target_comp_id;
	unsigned long			msg_seq_num;

	unsigned long			nr_fields;
	struct fix_field		*fields;
};
```

The code snippet above demonstrates that BeginString, BodyLength, MsgType,
SenderCompID, TargetCompID and MsgSeqNum occupy separate fields within a
structure while all the remaining fields and their values are stored in a
*fields* array.

There are two principal actions one can perform with messages: transmit and
receive. To receive a message *fix_session_recv()* function is to be used

```c
struct fix_message *fix_session_recv(struct fix_session *self, int flags)
```

Its input arguments speak for themselves: an active FIX session returned
by *fix_session_new()* and *flags* which specify input options. A pointer to
the received message is returned upon success and NULL in case of failure.

A general function to send a message looks like the following

```c
int fix_session_send(struct fix_session *self, struct fix_message *msg, int flags);
```

Its input arguments include an active FIX session returned by *fix_session_new()*,
a pointer to the message to be sent and input options as a third argument.
Libtrading provides a bunch of functions to send messages of different types.
These functions are essentially wrappers around general *fix_session_send()*.

```c
int fix_session_logon(struct fix_session *session);
int fix_session_test_request(struct fix_session *session);
int fix_session_logout(struct fix_session *session, const char *text);
int fix_session_heartbeat(struct fix_session *session, const char *test_req_id);
```

The functions above may be used to send Logon and Logout, HeartBeat and TestRequest messages.

### Dialects

FIX field is just a pair of Tag and Value which appears in the message as
"Tag=Value" followed by the FIX standard trailer. In order to parse a Value
properly we should know what type it belongs to. It could be one of the datatypes
defined in the FIX specification i.e. char, int, float etc Once datatype is known
the whole FIX field could be parsed.

We would say that a particular Tag is known if we can parse the corresponding
Value. A set of knows Tags is called a *dialect*. In other words, a dialect is
the set of FIX fields we could parse properly.

We need dialects to support different flavours of FIX that are used in real life.
Trading venues could run various versions of FIX protocol. Moreover custom fields
could also be used. Libtrading provides the way to handle this smoothly.

```c
	struct fix_dialect {
		enum fix_version	version;
		enum fix_type		(*tag_type)(int tag);
	};
```

The structure above represents a dialect. Version is an indetification field e.g.
FIX_4_4 is a FIX version 4.4 dialect. We have already discussed that in order to
parse a FIX field we should know what datatype this field is. The function tag_type
is intended to match the input Tag argument with the type of the FIX field i.e.
the type of the Value. A user is free to develop its own dialect or use one of
the already existed

```c
	struct fix_dialect fix_dialects[];
```

The choosen dialect is stored within a session structure and passed through
configuration parameters. Each time a FIX message is being received a tag_type
function is invoked. Known fields are parsed in accordance with their datatypes,
unknow fields are stored as strings.

### FIX client example

In this section we will outline key fragments of a simple FIX-client implementation.
The full code is available under the *tools/fix/* subdirectory.

The full code may be broken into the following parts:

* Prepare and initialize configuration parameters

```c
struct fix_session_cfg cfg;

fix_session_cfg_init(&cfg);

cfg.dialect	= &fix_dialects[version];

if (!sender_comp_id) {
	strncpy(cfg.sender_comp_id, "BUYSIDE", ARRAY_SIZE(cfg.sender_comp_id));
} else {
	strncpy(cfg.sender_comp_id, sender_comp_id, ARRAY_SIZE(cfg.sender_comp_id));
}

if (!target_comp_id) {
	strncpy(cfg.target_comp_id, "SELLSIDE", ARRAY_SIZE(cfg.target_comp_id));
} else {
	strncpy(cfg.target_comp_id, target_comp_id, ARRAY_SIZE(cfg.target_comp_id));
}

cfg.sockfd = socket(he->h_addrtype, SOCK_STREAM, IPPROTO_TCP);
cfg.heartbtint = 15;
```

* Create and configure a FIX session

```c
session	= fix_session_new(cfg);
if (!session) {
	fprintf(stderr, "FIX session cannot be created\\n");
	goto exit;
}
```

* Log on the server and start receiving the messages

```c
ret = fix_session_logon(session);
if (ret) {
	fprintf(stderr, "Client Logon FAILED\\n");
	goto exit;
}

fprintf(stdout, "Client Logon OK\\n");
```

* Receive and process the messages in accordance with FIX specification

```c
while (!stop && session->active) {
	msg = fix_session_recv(session, 0);
	if (msg) {
		if (fix_session_admin(session, msg))
			continue;
	}
}
```

* Perform log out safely and destroy a session

```c
if (session->active) {
	ret = fix_session_logout(session, NULL);
	if (ret) {
		fprintf(stderr, "Client Logout FAILED\\n");
		goto exit;
	}
}

fix_session_free(session);
```

## FAST

The are only subtle differences between the way one can deal with FAST and
the way one can work with FIX protocol. Libtrading provides a single approach
for both. In order to start working with FAST one should create and initialize
a FAST session and parse a template file. Therefore, a typical FAST work cycle
consists of four principal stages: create a session, parse a template file,
do basic logic, destroy a session.

To run a new session one just need to invoke a function

```c
struct fast_session *fast_session_new(struct fast_session_cfg *cfg)
```

which accepts a desirable configuration as an input argument and returns
a new session in case of success. Configuration structure is just a set
of particular parameters applicable to the newly created session.

To parse a template file one should use a function

```c
int fast_parse_template(struct fast_session *self, const char *xml);
```

which accepts a session to which a template file is applicable and a path to
the xml template file.

Libtrading provides a particular structure to represent a FAST message.

```c
struct fast_message;
```

There are two ways to access a field within a message. Hash table can be used
to access a field given the name and linear array to search by index.

```c
struct fast_message {
	unsigned long		nr_fields;
	struct fast_field	*fields;
	GHashTable		*ghtab;
};
```

To receive and transmit messages *fast_session_recv()* and *fast_session_send()*
are to be used

```c
struct fast_message *fast_session_recv(struct fast_session *self, int flags);
int fast_session_send(struct fast_session *self, struct fast_message *msg, int flags);
```

### FAST parser

In this section we will outline key fragments of a simple FAST parser implementation.
The full code is available under the *tools/fast/* subdirectory.

The full code may be broken into the following parts:

* Prepare and initialize configuration parameters

```c
struct fast_session_cfg cfg;

cfg.preamble_bytes = preamble;
cfg.reset = reset;
cfg.sockfd = fd;
```

* Create and configure a FAST session

```c
session = fast_session_new(cfg);
if (!session) {
	fprintf(stderr, "Parser: FAST session cannot be created\\n");
	goto exit;
}
```

* Parse a template file

```c
if (fast_parse_template(session, xml)) {
	fprintf(stderr, "Parser: Cannot read template xml file\\n");
	goto exit;
}
```

* Listen for input messages and print them out

```c
while (!stop) {
	msg = fast_session_recv(session, 0);
	if (!msg) {
		continue;
	} else {
		fprintf(stdout, "< tid = %lu:\\n", msg->tid);
		fprintmsg(stdout, msg);

		if (session->reset)
			fast_session_reset(session);
	}
}
```

* Destroy a session once finished

```c
fast_session_free(session);
```

# Contacts

Should you have any questions, feel free to contact Marat Stanichenko <mstanichenko@gmail.com>

[FAST]: http://fixprotocol.org/fastspec/
[FIX]: http://fixprotocol.org/specifications/
