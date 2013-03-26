#include "libtrading/proto/fix_message.h"
#include "libtrading/proto/fix_session.h"

#include "libtrading/array.h"
#include "libtrading/die.h"

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "test.h"

struct protocol_info {
	const char		*name;
	int			(*session_accept)(struct fix_session_cfg *cfg, const char *script);
};

static bool fix_server_logon(struct fix_session *session)
{
	struct fix_message logon_msg;
	struct fix_message *msg;

	msg = fix_session_recv(session, 0);
	if (!fix_message_type_is(msg, FIX_MSG_TYPE_LOGON))
		return false;

	logon_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_LOGON,
	};

	fix_session_send(session, &logon_msg, 0);

	return true;
}

static bool fix_server_logout(struct fix_session *session)
{
	struct fix_message logout_msg;
	struct fix_message *msg;
	bool ret = true;

	msg = fix_session_recv(session, 0);
	if (!fix_message_type_is(msg, FIX_MSG_TYPE_LOGOUT))
		ret = false;

	logout_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_LOGOUT,
	};

	fix_session_send(session, &logout_msg, 0);

	return ret;
}

static int fix_session_accept(struct fix_session_cfg *cfg, const char *script)
{
	struct fcontainer *s_container = NULL;
	struct fcontainer *c_container = NULL;
	struct fix_session *session = NULL;
	struct felem *expected_elem;
	struct fix_message *msg;
	FILE *stream;
	int ret = -1;

	stream = fopen(script, "r");
	if (!stream) {
		fprintf(stderr, "Opening %s failed: %s\n", script, strerror(errno));
		goto exit;
	}

	session = fix_session_new(cfg);
	if (!session) {
		fprintf(stderr, "FIX session cannot be created\n");
		goto exit;
	}

	s_container = fcontainer_new();
	if (!s_container) {
		fprintf(stderr, "Cannot allocate container\n");
		goto exit;
	}

	c_container = fcontainer_new();
	if (!c_container) {
		fprintf(stderr, "Cannot allocate container\n");
		goto exit;
	}

	if (script_read(stream, s_container, c_container)) {
		fprintf(stderr, "Invalid script: %s\n", script);
		goto exit;
	}

	if (fix_server_logon(session))
		fprintf(stderr, "Server Logon OK\n");
	else {
		fprintf(stderr, "Server Logon FAILED\n");
		goto exit;
	}

	expected_elem = cur_elem(c_container);

	while (expected_elem) {
		msg = fix_session_recv(session, 0);

		fprintf(stdout, "> ");
		fprintmsg(stdout, msg);

		if (!msg) {
			fprintf(stderr, "Server: nothing received\n");
			break;
		} else if (fmsgcmp(&expected_elem->msg, msg)) {
			fprintf(stderr, "Server: messages differ\n");
			fprintmsg(stderr, &expected_elem->msg);
			fprintmsg(stderr, msg);
			break;
		}

		if (!fix_session_admin(session, msg))
			goto next;

		if (fix_message_type_is(msg, FIX_MSG_TYPE_LOGON)) {
			fprintf(stderr, "Server: repeated Logon\n");
			break;
		} else if (fix_message_type_is(msg, FIX_MSG_TYPE_LOGOUT)) {
			fprintf(stderr, "Server: premature Logout\n");
			break;
		}

next:
		expected_elem = next_elem(c_container);
	}

	if (expected_elem)
		goto exit;

	if (fix_server_logout(session))
		fprintf(stderr, "Server Logout OK\n");
	else {
		fprintf(stderr, "Server Logout FAILED\n");
		goto exit;
	}

	ret = 0;

exit:
	fcontainer_free(c_container);
	fcontainer_free(s_container);
	fix_session_free(session);
	fclose(stream);

	return ret;
}

static const struct protocol_info protocols[] = {
	{ "fix",		fix_session_accept },
};

static const struct protocol_info *lookup_protocol_info(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(protocols); i++) {
		const struct protocol_info *proto_info = &protocols[i];

		if (!strcmp(proto_info->name, name))
			return proto_info;
	}
	return NULL;
}

static void usage(void)
{
	printf("\n  usage: trade server -p [port] -c [protocol] -f [filename]\n\n");
	exit(EXIT_FAILURE);
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

static enum fix_version strversion(const char *name)
{
	if (!strcmp(name, "fix42"))
		return FIX_4_2;
	else if (!strcmp(name, "fix43"))
		return FIX_4_3;
	else if (!strcmp(name, "fix44"))
		return FIX_4_4;

	return FIX_4_4;
}

int main(int argc, char *argv[])
{
	const struct protocol_info *proto_info;
	const char *filename = NULL;
	struct fix_session_cfg cfg;
	const char *proto = NULL;
	struct sockaddr_in sa;
	int port = 0;
	int sockfd;
	int opt;
	int ret;

	while ((opt = getopt(argc, argv, "p:c:f:")) != -1) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			break;
		case 'f':
			filename = optarg;
			break;
		case 'c':
			proto = optarg;
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (!port || !proto || !filename)
		usage();

	cfg.version	= strversion(proto);
	strncpy(cfg.sender_comp_id, "BUYSIDE", ARRAY_SIZE(cfg.sender_comp_id));
	strncpy(cfg.target_comp_id, "SELLSIDE", ARRAY_SIZE(cfg.target_comp_id));

	proto_info = lookup_protocol_info(proto);
	if (!proto_info) {
		printf("Unsupported protocol '%s'\n", proto);
		exit(EXIT_FAILURE);
	}

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0)
		die("cannot create socket");

	if (socket_setopt(sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
		die("cannot set socket option TCP_NODELAY");

	if (socket_setopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 1) < 0)
		die("cannot set socket option SO_REUSEADDR");

	sa = (struct sockaddr_in) {
		.sin_family		= AF_INET,
		.sin_port		= htons(port),
		.sin_addr		= (struct in_addr) {
			.s_addr			= INADDR_ANY,
		},
	};

	if (bind(sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
		die("bind failed");

	fprintf(stderr, "Server is listening to port %d using '%s' protocol...\n", port, proto);

	if (listen(sockfd, 10) < 0)
		die("listen failed");

	cfg.sockfd = accept(sockfd, NULL, NULL);
	if (cfg.sockfd < 0)
		die("accept failed");

	if (socket_setopt(cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
		die("cannot set socket option TCP_NODELAY");

	ret = proto_info->session_accept(&cfg, filename);

	shutdown(cfg.sockfd, SHUT_RDWR);

	close(cfg.sockfd);

	close(sockfd);

	return ret;
}
