#include "fix/builtins.h"
#include "fix/message.h"
#include "fix/session.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

static void usage(void)
{
	printf("\n  usage: fix server -p [port]\n\n");
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

int cmd_server(int argc, char *argv[])
{
	struct sockaddr_in sa;
	int port = 0;
	int sockfd;
	int opt;

	while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (!port) {
		usage();
		exit(EXIT_FAILURE);
	}

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0) {
		perror("can not create socket");
		exit(EXIT_FAILURE);
	}

	if (socket_setopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 1) < 0) {
		perror("can not set socket option SO_REUSEADDR");
		exit(EXIT_FAILURE);
	}

	sa = (struct sockaddr_in) {
		.sin_family		= AF_INET,
		.sin_port		= htons(port),
		.sin_addr		= (struct in_addr) {
			.s_addr			= INADDR_ANY,
		},
	};

	if (bind(sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
		perror("error bind failed");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	printf("FIX server is listening to port %d...\n", port);

	if (listen(sockfd, 10) < 0) {
		perror("error listen failed");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	for (;;) {
		struct fix_message logon_msg;
		struct fix_session *session;
		struct fix_message *msg;
		int incoming_fd;
		
		incoming_fd = accept(sockfd, NULL, NULL);
		if (incoming_fd < 0) {
			perror("error accept failed");
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		session		= fix_session_new(incoming_fd, FIX_4_4, "BUYSIDE", "SELLSIDE");

		msg = fix_message_recv(incoming_fd, 0);

		logon_msg	= (struct fix_message) {
			.msg_type	= Logon,
		};
		fix_session_send(session, &logon_msg, 0);

		fix_message_free(msg);

		fix_session_free(session);

		shutdown(incoming_fd, SHUT_RDWR);

		close(incoming_fd);
	}

	close(sockfd);
	return 0;
}
