#include "libtrading/proto/fix_message.h"
#include "libtrading/proto/fix_session.h"

#include "libtrading/array.h"
#include "libtrading/die.h"

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>

#include "test.h"

struct protocol_info {
	const char		*name;
	int			(*session_initiate)(const struct protocol_info *, int, const char *);
};

static int fix_session_initiate(const struct protocol_info *proto, int sockfd, const char *script)
{
	struct fcontainer *s_container = NULL;
	struct fcontainer *c_container = NULL;
	struct fix_session *session = NULL;
	struct felem *expected_elem;
	struct felem *tosend_elem;
	enum fix_version version;
	struct fix_message *msg;
	FILE *stream;
	int ret = 1;

	version = FIX_4_4;

	if (!strcmp(proto->name, "fix42"))
		version = FIX_4_2;
	else if (!strcmp(proto->name, "fix43"))
		version = FIX_4_3;
	else if (!strcmp(proto->name, "fix44"))
		version = FIX_4_4;

	stream = fopen(script, "r");
	if (!stream) {
		fprintf(stderr, "Opening %s failed: %s\n", script, strerror(errno));
		goto exit;
	}

	session = fix_session_new(sockfd, version, "BUYSIDE", "SELLSIDE");
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

	if (fix_session_logon(session))
		fprintf(stderr, "Client Logon OK\n");
	else {
		fprintf(stderr, "Client Logon FAILED\n");
		goto exit;
	}

	expected_elem = cur_elem(s_container);
	tosend_elem = cur_elem(c_container);

	while (tosend_elem) {
		fix_session_send(session, &tosend_elem->msg, 0);

		msg = fix_session_recv(session, 0);

		fprintf(stdout, "< ");
		fprintmsg(stdout, msg);

		if (!msg) {
			fprintf(stderr, "Client: nothing received\n");
			break;
		} else if (fmsgcmp(&expected_elem->msg, msg)) {
			fprintf(stderr, "Client: messages differ\n");
			fprintmsg(stderr, &expected_elem->msg);
			fprintmsg(stderr, msg);
			break;
		}

		expected_elem = next_elem(s_container);
		tosend_elem = next_elem(c_container);
	}

	if (tosend_elem)
		goto exit;

	if (fix_session_logout(session))
		fprintf(stderr, "Client Logout OK\n");
	else {
		fprintf(stderr, "Client Logout FAILED\n");
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
	{ "fix",		fix_session_initiate },
	{ "fix42",		fix_session_initiate },
	{ "fix43",		fix_session_initiate },
	{ "fix44",		fix_session_initiate },
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
	printf("\n  usage: trade client -f [filename] -h [hostname] -p [port] -c [protocol]\n\n");
	exit(EXIT_FAILURE);
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
	const char *host = NULL;
	struct sockaddr_in sa;
	int saved_errno = 0;
	struct hostent *he;
	int sockfd = -1;
	int port = 0;
	int retval;
	char **ap;
	int opt;

	while ((opt = getopt(argc, argv, "f:h:p:c:")) != -1) {
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
		case 'h':
			host = optarg;
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (!port || !proto || !filename || !host) {
		usage();
		exit(EXIT_FAILURE);
	}

	proto_info = lookup_protocol_info(proto);
	if (!proto_info) {
		printf("Unsupported protocol '%s'\n", proto);
		exit(EXIT_FAILURE);
	}

	he = gethostbyname(host);
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

	retval = proto_info->session_initiate(proto_info, sockfd, filename);

	shutdown(sockfd, SHUT_RDWR);

	if (close(sockfd) < 0)
		die("close");

	return retval;
}
