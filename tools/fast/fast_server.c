#include "libtrading/proto/fast_message.h"
#include "libtrading/proto/fast_session.h"

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
	int			(*session_accept)(int incoming_fd, const char *xml, const char *script);
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

static int fast_session_accept(int incoming_fd, const char *xml, const char *script)
{
	struct fcontainer *container = NULL;
	struct fast_session *session = NULL;
	struct felem *expected_elem;
	struct fast_message *msg;
	FILE *stream;
	int ret = -1;

	stream = fopen(script, "r");
	if (!stream) {
		fprintf(stderr, "Opening %s failed: %s\n", script, strerror(errno));
		goto exit;
	}

	session = fast_session_new(incoming_fd);
	if (!session) {
		fprintf(stderr, "FAST session cannot be created\n");
		goto exit;
	}

	container = fcontainer_new();
	if (!container) {
		fprintf(stderr, "Cannot allocate container\n");
		goto exit;
	}

	if (fast_parse_template(session, xml)) {
		fprintf(stderr, "Cannot read template xml file\n");
		goto exit;
	}

	fcontainer_init(container, session->rx_messages);

	if (script_read(stream, container)) {
		fprintf(stderr, "Invalid script: %s\n", script);
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

static const struct protocol_info protocols[] = {
	{ "fast",		fast_session_accept },
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
	printf("\n  usage: trade server -p [port] -c [protocol] -t [template] -f [filename]\n\n");
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

int main(int argc, char *argv[])
{
	const struct protocol_info *proto_info;
	const char *filename = NULL;
	const char *proto = NULL;
	const char *xml = NULL;
	struct sockaddr_in sa;
	int incoming_fd;
	int port = 0;
	int sockfd;
	int opt;
	int ret;

	while ((opt = getopt(argc, argv, "p:c:f:t:")) != -1) {
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
		case 't':
			xml = optarg;
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (!port || !proto || !filename || !xml) {
		usage();
		exit(EXIT_FAILURE);
	}

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

	incoming_fd = accept(sockfd, NULL, NULL);
	if (incoming_fd < 0)
		die("accept failed");

	if (socket_setopt(incoming_fd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
		die("cannot set socket option TCP_NODELAY");

	ret = proto_info->session_accept(incoming_fd, xml, filename);

	shutdown(incoming_fd, SHUT_RDWR);

	close(incoming_fd);

	close(sockfd);

	return ret;
}
