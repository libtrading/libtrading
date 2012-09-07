#include "trading/array.h"
#include "trading/die.h"

#include "trading/fix_message.h"
#include "trading/fix_session.h"

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

struct protocol_info {
	const char		*name;
	void			(*session_accept)(int incoming_fd);
};

static void fix_session_accept(int incoming_fd)
{
	struct fix_message logon_msg;
	struct fix_session *session;
	struct fix_message *msg;

	session		= fix_session_new(incoming_fd, FIX_4_4, "BUYSIDE", "SELLSIDE");

	msg = fix_message_recv(incoming_fd, 0);

	logon_msg	= (struct fix_message) {
		.msg_type	= FIX_MSG_LOGON,
	};
	fix_session_send(session, &logon_msg, 0);

	fix_message_free(msg);

	for (;;) {
		struct fix_message logout_msg;

		msg = fix_message_recv(incoming_fd, 0);
		if (!msg || !fix_message_type_is(msg, FIX_MSG_LOGOUT)) {
			fix_message_free(msg);
			continue;
		}
		fix_message_free(msg);

		logout_msg	= (struct fix_message) {
			.msg_type	= FIX_MSG_LOGOUT,
		};
		fix_session_send(session, &logout_msg, 0);
		break;
	}

	fix_session_free(session);
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
	printf("\n  usage: trade server -p [port] -c [protocol]\n\n");
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

int main(int argc, char *argv[])
{
	const struct protocol_info *proto_info;
	const char *proto = NULL;
	struct sockaddr_in sa;
	int port = 0;
	int sockfd;
	int opt;

	while ((opt = getopt(argc, argv, "p:c:")) != -1) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			break;
		case 'c':
			proto = optarg;
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (!port || !proto) {
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

	printf("Server is listening to port %d using '%s' protocol...\n", port, proto);

	if (listen(sockfd, 10) < 0)
		die("listen failed");

	for (;;) {
		int incoming_fd;

		incoming_fd = accept(sockfd, NULL, NULL);
		if (incoming_fd < 0)
			die("accept failed");

		if (socket_setopt(incoming_fd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
			die("cannot set socket option TCP_NODELAY");

		proto_info->session_accept(incoming_fd);

		shutdown(incoming_fd, SHUT_RDWR);

		close(incoming_fd);
	}

	close(sockfd);

	return 0;
}
