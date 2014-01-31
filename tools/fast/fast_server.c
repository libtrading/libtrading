#include "libtrading/proto/fast_message.h"
#include "libtrading/proto/fast_session.h"

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

#include "fast_server.h"
#include "test.h"

static const char *program;

static struct fast_server_function fast_server_functions[] = {
	[FAST_SERVER_SCRIPT] = {
		.fast_session_accept	= fast_server_script,
		.mode			= FAST_SERVER_SCRIPT,
	},
	[FAST_SERVER_PONG] = {
		.fast_session_accept	= fast_server_pong,
		.mode			= FAST_SERVER_PONG,
	},
};

static void fast_send_prepare(struct fast_message *msg, struct felem *elem)
{
	struct fast_message *elem_msg;
	struct fast_field *elem_field;
	struct fast_field *field;
	int i;

	if (!elem || !msg)
		goto exit;

	elem_msg = &elem->msg;

	for (i = 0; i < msg->nr_fields; i++) {
		elem_field = elem_msg->fields + i;
		field = msg->fields + i;

		field->state = elem_field->state;

		switch (elem_field->type) {
		case FAST_TYPE_INT:
			field->int_value = elem_field->int_value;
			break;
		case FAST_TYPE_UINT:
			field->uint_value = elem_field->uint_value;
			break;
		case FAST_TYPE_STRING:
			memcpy(field->string_value, elem_field->string_value, strlen(elem_field->string_value) + 1);
			break;
		case FAST_TYPE_VECTOR:
			break;
		case FAST_TYPE_DECIMAL:
			field->decimal_value.exp = elem_field->decimal_value.exp;
			field->decimal_value.mnt = elem_field->decimal_value.mnt;
			break;
		case FAST_TYPE_SEQUENCE:
			break;
		default:
			break;
		}
	}

exit:
	return;
}

static int fast_server_script(struct fast_session_cfg *cfg, struct fast_server_arg *arg)
{
	struct fcontainer *container = NULL;
	struct fast_session *session = NULL;
	struct felem *expected_elem;
	struct fast_message *msg;
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

	session = fast_session_new(cfg);
	if (!session) {
		fprintf(stderr, "FAST session cannot be created\n");
		goto exit;
	}

	container = fcontainer_new();
	if (!container) {
		fprintf(stderr, "Cannot allocate container\n");
		goto exit;
	}

	if (fast_parse_template(session, arg->xml)) {
		fprintf(stderr, "Cannot read template xml file\n");
		goto exit;
	}

	fcontainer_init(container, session->rx_messages);

	if (script_read(stream, container)) {
		fprintf(stderr, "Invalid script: %s\n", arg->script);
		goto exit;
	}

	expected_elem = cur_elem(container);
	msg = &expected_elem->msg;

	expected_elem = next_elem(container);

	while (expected_elem) {
		fast_send_prepare(msg, expected_elem);

		if (fast_session_send(session, msg, 0))
			goto exit;

		expected_elem = next_elem(container);
	}

	ret = 0;

exit:
	fcontainer_free(container);
	fast_session_free(session);
	fclose(stream);

	return ret;
}

static int fast_pong_prepare(struct fast_message *tx_msg)
{
	struct fast_field *tx_field;
	int ret = -1;
	int i;

	if (!tx_msg)
		goto exit;

	for (i = 0; i < tx_msg->nr_fields; i++) {
		tx_field = tx_msg->fields + i;

		tx_field->state = FAST_STATE_ASSIGNED;
		if (tx_field->op == FAST_OP_CONSTANT)
			continue;

		switch (tx_field->type) {
		case FAST_TYPE_STRING:
			strcpy(tx_field->string_value, "Forty two");
			break;
		case FAST_TYPE_DECIMAL:
			tx_field->decimal_value.mnt = -42;
			tx_field->decimal_value.exp = 42;
			break;
		case FAST_TYPE_UINT:
			tx_field->uint_value = 42;
			break;
		case FAST_TYPE_INT:
			tx_field->int_value = -42;
			break;
		case FAST_TYPE_VECTOR:
			break;
		case FAST_TYPE_SEQUENCE:
			break;
		default:
			break;
		}
	}

	ret = 0;

exit:
	return ret;
}

static int fast_server_pong(struct fast_session_cfg *cfg, struct fast_server_arg *arg)
{
	struct fast_session *session = NULL;
	struct fast_message *tx_msg = NULL;
	struct fast_message *rx_msg = NULL;
	struct fast_session *aux = NULL;
	int ret = -1;
	int i;

	session = fast_session_new(cfg);
	if (!session) {
		fprintf(stderr, "FAST session cannot be created\n");
		goto exit;
	}

	if (fast_parse_template(session, arg->xml)) {
		fprintf(stderr, "Cannot read template xml file\n");
		goto exit;
	}

	aux = fast_session_new(cfg);
	if (!aux) {
		fprintf(stderr, "FAST session cannot be created\n");
		goto exit;
	}

	if (fast_parse_template(aux, arg->xml)) {
		fprintf(stderr, "Cannot read template xml file\n");
		goto exit;
	}

	rx_msg = session->rx_messages;
	if (!rx_msg) {
		fprintf(stderr, "Message cannot be found\n");
		goto exit;
	}

	tx_msg = aux->rx_messages;
	if (!tx_msg) {
		fprintf(stderr, "Message cannot be found\n");
		goto exit;
	}

	if (fast_pong_prepare(tx_msg)) {
		fprintf(stderr, "Cannot initialize tx_msg\n");
		goto exit;
	}

	for (i = 0; i < arg->pongs; i++) {
retry:
		rx_msg = fast_session_recv(session, MSG_DONTWAIT);

		if (!rx_msg)
			goto retry;

		if (fast_session_send(session, tx_msg, 0))
			goto exit;
	}

	ret = 0;

exit:
	fast_session_free(session);
	fast_session_free(aux);

	return ret;
}

static void usage(void)
{
	printf("\n usage: %s [-m mode] [-f filename] [-n pongs] -p port -t template\n\n", program);

	exit(EXIT_FAILURE);
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

static enum fast_server_mode strservermode(const char *mode)
{
	enum fast_server_mode m;

	if (!strcmp(mode, "script"))
		return FAST_SERVER_SCRIPT;
	else if (!strcmp(mode, "pong"))
		return FAST_SERVER_PONG;

	if (sscanf(mode, "%u", &m) != 1)
		return FAST_SERVER_SCRIPT;

	switch (m) {
	case FAST_SERVER_SCRIPT:
	case FAST_SERVER_PONG:
		return m;
	default:
		break;
	}

	return FAST_SERVER_SCRIPT;
}

int main(int argc, char *argv[])
{
	enum fast_server_mode mode = FAST_SERVER_SCRIPT;
	struct fast_server_arg arg = {0};
	struct fast_session_cfg cfg;
	struct sockaddr_in sa;
	int port = 0;
	int sockfd;
	int opt;
	int ret;

	program = basename(argv[0]);

	while ((opt = getopt(argc, argv, "p:f:t:m:n:")) != -1) {
		switch (opt) {
		case 'm':
			mode = strservermode(optarg);
			break;
		case 'n':
			arg.pongs = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'f':
			arg.script = optarg;
			break;
		case 't':
			arg.xml = optarg;
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (!port || !arg.xml)
		usage();

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

	fprintf(stderr, "FAST server is listening to port %d...\n", port);

	if (listen(sockfd, 10) < 0)
		die("listen failed");

	cfg.sockfd = accept(sockfd, NULL, NULL);
	if (cfg.sockfd < 0)
		die("accept failed");

	if (socket_setopt(cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
		die("cannot set socket option TCP_NODELAY");

	cfg.preamble_bytes = 0;
	cfg.reset = false;

	switch (mode) {
	case FAST_SERVER_SCRIPT:
	case FAST_SERVER_PONG:
		ret = fast_server_functions[mode].fast_session_accept(&cfg, &arg);
		break;
	default:
		error("Invalid mode");
	}

	shutdown(cfg.sockfd, SHUT_RDWR);

	close(cfg.sockfd);

	close(sockfd);

	return ret;
}
