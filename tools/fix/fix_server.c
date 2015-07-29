#include "libtrading/proto/fix_message.h"
#include "libtrading/proto/fix_session.h"

#include "libtrading/array.h"
#include "libtrading/die.h"

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "fix_server.h"
#include "test.h"

static const char *program;

static struct fix_server_function fix_server_functions[] = {
	[FIX_SERVER_SCRIPT] = {
		.fix_session_accept	= fix_server_script,
		.mode			= FIX_SERVER_SCRIPT,
	},
	[FIX_SERVER_SESSION] = {
		.fix_session_accept	= fix_server_session,
		.mode			= FIX_SERVER_SESSION,
	},
};

static bool fix_server_logon(struct fix_session *session)
{
	struct fix_message logon_msg;
	struct fix_message *msg;

retry:
	if (fix_session_recv(session, &msg, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (!fix_message_type_is(msg, FIX_MSG_TYPE_LOGON))
		return false;

	logon_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_LOGON,
	};

	fix_session_send(session, &logon_msg, 0);
	session->active = true;

	return true;
}

static bool fix_server_logout(struct fix_session *session)
{
	struct fix_message logout_msg;
	struct fix_message *msg;
	bool ret = true;

retry:
	if (!session->active)
		goto exit;

	if (fix_session_recv(session, &msg, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (!fix_message_type_is(msg, FIX_MSG_TYPE_LOGOUT))
		ret = false;

	logout_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_LOGOUT,
	};

	fix_session_send(session, &logout_msg, 0);

exit:
	return ret;
}

static int fix_server_script(struct fix_session_cfg *cfg, struct fix_server_arg *arg)
{
	struct fcontainer *s_container = NULL;
	struct fcontainer *c_container = NULL;
	struct fix_session *session = NULL;
	struct felem *expected_elem;
	struct fix_message *msg;
	FILE *stream = NULL;
	int ret = -1;

	if (!arg->script) {
		fprintf(stderr, "No script is specified\n");
		goto exit;
	}

	stream = fopen(arg->script, "r");
	if (!stream) {
		fprintf(stderr, "Opening %s failed: %s\n",
				arg->script, strerror(errno));
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
		fprintf(stderr, "Invalid script: %s\n", arg->script);
		goto exit;
	}

	if (fix_server_logon(session))
		fprintf(stdout, "Server Logon OK\n");
	else {
		fprintf(stderr, "Server Logon FAILED\n");
		goto exit;
	}

	expected_elem = cur_elem(c_container);

	while (expected_elem) {
		if (fix_session_recv(session, &msg, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
			continue;

		fprintf(stdout, "> ");
		fprintmsg_iov(stdout, msg);

		if (fmsgcmp(&expected_elem->msg, msg)) {
			fprintf(stderr, "Server: messages differ\n");
			fprintmsg(stderr, &expected_elem->msg);
			fprintmsg(stderr, msg);
			break;
		}

		if (fix_session_admin(session, msg))
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
		fprintf(stdout, "Server Logout OK\n");
	else {
		fprintf(stderr, "Server Logout FAILED\n");
		goto exit;
	}

	ret = 0;

exit:
	fcontainer_free(c_container);
	fcontainer_free(s_container);
	fix_session_free(session);

	if (stream)
		fclose(stream);

	return ret;
}

static unsigned long fix_execution_report_fields(struct fix_field *fields)
{
	unsigned long nr = 0;

	fields[nr++] = FIX_STRING_FIELD(OrderID, "OrderID");
	fields[nr++] = FIX_STRING_FIELD(Symbol, "Symbol");
	fields[nr++] = FIX_STRING_FIELD(ExecID, "ExecID");
	fields[nr++] = FIX_STRING_FIELD(OrdStatus, "2");
	fields[nr++] = FIX_STRING_FIELD(ExecType, "0");
	fields[nr++] = FIX_FLOAT_FIELD(LeavesQty, 0);
	fields[nr++] = FIX_FLOAT_FIELD(CumQty, 100);
	fields[nr++] = FIX_FLOAT_FIELD(AvgPx, 100);
	fields[nr++] = FIX_STRING_FIELD(Side, "1");

	return nr;

}

static int fix_server_session(struct fix_session_cfg *cfg, struct fix_server_arg *arg)
{
	struct fix_session *session = NULL;
	struct fix_field *fields = NULL;
	struct fix_message logon_msg;
	struct fix_message *msg;
	unsigned long nr;
	int ret = -1;

	session = fix_session_new(cfg);
	if (!session)
		goto exit;

	fix_session_recv(session, &msg, FIX_RECV_FLAG_MSG_DONTWAIT);

	logon_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_LOGON,
	};

	fix_session_send(session, &logon_msg, 0);

	fields = calloc(FIX_MAX_FIELD_NUMBER, sizeof(struct fix_field));
	if (!fields)
		goto exit;

	nr = fix_execution_report_fields(fields);

	for (;;) {
		struct fix_message logout_msg;

		if (fix_session_recv(session, &msg, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
			continue;
		else if (fix_message_type_is(msg, FIX_MSG_TYPE_NEW_ORDER_SINGLE)) {
			fix_session_execution_report(session, fields, nr);
			continue;
		} else if (!fix_message_type_is(msg, FIX_MSG_TYPE_LOGOUT))
			continue;

		logout_msg	= (struct fix_message) {
			.type		= FIX_MSG_TYPE_LOGOUT,
		};
		fix_session_send(session, &logout_msg, 0);
		break;
	}

	ret = 0;

exit:
	fix_session_free(session);
	free(fields);

	return ret;
}

static void usage(void)
{
	printf("\n usage: %s [-m mode] [-d dialect] [-f filename] [-s sender-comp-id] [-t target-comp-id] -p port\n\n", program);

	exit(EXIT_FAILURE);
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

static enum fix_server_mode strservermode(const char *mode)
{
	enum fix_server_mode m;

	if (!strcmp(mode, "session"))
		return FIX_SERVER_SESSION;
	else if (!strcmp(mode, "script"))
		return FIX_SERVER_SCRIPT;

	if (sscanf(mode, "%u", &m) != 1)
		return FIX_SERVER_SCRIPT;

	switch (m) {
		case FIX_SERVER_SESSION:
		case FIX_SERVER_SCRIPT:
			return m;
		default:
			break;
	}

	return FIX_SERVER_SCRIPT;
}

static enum fix_version strversion(const char *dialect)
{
	if (!strcmp(dialect, "fix42"))
		return FIX_4_2;
	else if (!strcmp(dialect, "fix43"))
		return FIX_4_3;
	else if (!strcmp(dialect, "fix44"))
		return FIX_4_4;

	return FIX_4_4;
}

int main(int argc, char *argv[])
{
	enum fix_server_mode mode = FIX_SERVER_SCRIPT;
	enum fix_version version = FIX_4_4;
	const char *target_comp_id = NULL;
	const char *sender_comp_id = NULL;
	struct fix_server_arg arg = {0};
	struct fix_session_cfg cfg;
	struct sockaddr_in sa;
	int port = 0;
	int sockfd;
	int opt;
	int ret;

	program = basename(argv[0]);

	while ((opt = getopt(argc, argv, "f:p:d:s:t:m:")) != -1) {
		switch (opt) {
		case 'd':
			version = strversion(optarg);
			break;
		case 's':
			sender_comp_id = optarg;
			break;
		case 't':
			target_comp_id = optarg;
			break;
		case 'm':
			mode = strservermode(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'f':
			arg.script = optarg;
			break;
		default: /* '?' */
			usage();
		}
	}

	if (!port)
		usage();

	fix_session_cfg_init(&cfg);

	cfg.dialect	= &fix_dialects[version];

	if (!sender_comp_id) {
		strncpy(cfg.sender_comp_id, "SELLSIDE", ARRAY_SIZE(cfg.sender_comp_id));
	} else {
		strncpy(cfg.sender_comp_id, sender_comp_id, ARRAY_SIZE(cfg.sender_comp_id));
	}

	if (!target_comp_id) {
		strncpy(cfg.target_comp_id, "BUYSIDE", ARRAY_SIZE(cfg.target_comp_id));
	} else {
		strncpy(cfg.target_comp_id, target_comp_id, ARRAY_SIZE(cfg.target_comp_id));
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

	fprintf(stderr, "Server is listening to port %d...\n", port);

	if (listen(sockfd, 10) < 0)
		die("listen failed");

	cfg.sockfd = accept(sockfd, NULL, NULL);
	if (cfg.sockfd < 0)
		die("accept failed");

	if (socket_setopt(cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
		die("cannot set socket option TCP_NODELAY");

	switch (mode) {
	case FIX_SERVER_SCRIPT:
		ret = fix_server_functions[mode].fix_session_accept(&cfg, &arg);
		break;
	case FIX_SERVER_SESSION:
		ret = fix_server_functions[mode].fix_session_accept(&cfg, &arg);
		break;
	default:
		error("Invalid mode");
	}

	shutdown(cfg.sockfd, SHUT_RDWR);

	close(cfg.sockfd);

	close(sockfd);

	return ret;
}
