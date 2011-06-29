#include "fix/builtins.h"
#include "fix/message.h"
#include "fix/session.h"
#include "fix/die.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>

static void usage(void)
{
	printf("\n  usage: fix client [hostname] [port]\n\n");
	exit(EXIT_FAILURE);
}

int cmd_client(int argc, char *argv[])
{
	struct fix_message *response;
	struct fix_message logon_msg;
	struct fix_session *session;
	struct sockaddr_in sa;
	int saved_errno = 0;
	struct hostent *he;
	const char *host;
	int sockfd = -1;
	int retval;
	char **ap;
	int port;

	if (argc != 4)
		usage();

	host = argv[2];
	port = atoi(argv[3]);

	he = gethostbyname(argv[2]);
	if (!he)
		error("Unable to look up %s (%s)", host, hstrerror(h_errno));

	for (ap = he->h_addr_list; *ap; ap++) {
		sockfd = socket(he->h_addrtype, SOCK_STREAM, IPPROTO_TCP);
		if (sockfd < 0) {
			saved_errno = errno;
			continue;
		}

		sa = (struct sockaddr_in) {
			.sin_family		= he->h_addrtype,
			.sin_port		= htons(port),
		};
		memcpy(&sa.sin_addr, *ap, he->h_length);

		if (connect(sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
			saved_errno = errno;
			close(sockfd);
			sockfd = -1;
			continue;
		}
		break;
	}

	if (sockfd < 0)
		error("Unable to connect to a socket (%s)", strerror(saved_errno));

	session		= fix_session_new(sockfd, FIX_4_4, "BUYSIDE", "SELLSIDE");

	logon_msg	= (struct fix_message) {
		.msg_type	= Logon,
	};
	fix_session_send(session, &logon_msg, 0);

	response	= fix_message_recv(sockfd, 0);
	if (response && fix_message_type_is(response, Logon)) {
		printf("Logon OK\n");
		retval = 0;
	} else {
		printf("Logon FAILED\n");
		retval = 1;
	}

	if (response)
		fix_message_free(response);

	fix_session_free(session);

	shutdown(sockfd, SHUT_RDWR);

	if (close(sockfd) < 0)
		die("close");

	return retval;
}
