#include "trading/builtins.h"

#include "trading/core.h"
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
#include <errno.h>
#include <netdb.h>
#include <stdio.h>

struct protocol_info {
	const char		*name;
	int			(*session_initiate)(int incoming_fd);
};

static int fix_session_initiate(int sockfd)
{
	struct fix_session *session;
	int retval;

	session	= fix_session_new(sockfd, FIX_4_4, "BUYSIDE", "SELLSIDE");
	if (!session)
		die("unable to allocate fix session");

	if (fix_session_logon(session)) {
		printf("Logon OK\n");
		retval = 0;
	} else {
		printf("Logon FAILED\n");
		retval = 1;
	}

	if (fix_session_logout(session)) {
		printf("Logout OK\n");
		retval = 0;
	} else {
		printf("Logout FAILED\n");
		retval = 1;
	}

	fix_session_free(session);

	return retval;
}

static const struct protocol_info protocols[] = {
	{ "fix",		fix_session_initiate },
	{ NULL,			NULL }
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
	printf("\n  usage: fix client [hostname] [port] [protocol]\n\n");
	exit(EXIT_FAILURE);
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

int cmd_client(int argc, char *argv[])
{
	const struct protocol_info *proto_info;
	struct sockaddr_in sa;
	int saved_errno = 0;
	struct hostent *he;
	const char *proto;
	const char *host;
	int sockfd = -1;
	int retval;
	char **ap;
	int port;

	if (argc != 5)
		usage();

	host	= argv[2];
	port	= atoi(argv[3]);
	proto	= argv[4];

	proto_info = lookup_protocol_info(proto);
	if (!proto_info) {
		printf("Unsupported protocol '%s'\n", proto);
		exit(EXIT_FAILURE);
	}

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

	if (socket_setopt(sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
		die("cannot set socket option TCP_NODELAY");

	retval = proto_info->session_initiate(sockfd);

	shutdown(sockfd, SHUT_RDWR);

	if (close(sockfd) < 0)
		die("close");

	return retval;
}
