#include "libtrading/proto/fast_message.h"
#include "libtrading/proto/fast_session.h"

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
	int			(*session_initiate)(const struct protocol_info *, int, const char *, const char *);
};

static int fast_session_initiate(const struct protocol_info *proto, int sockfd,
						const char *xml, const char *script)
{
	struct fcontainer *container = NULL;
	struct fast_session *session = NULL;
	struct felem *expected_elem;
	struct fast_message *msg;
	FILE *stream;
	int ret = 1;

	stream = fopen(script, "r");
	if (!stream) {
		fprintf(stderr, "Opening %s failed: %s\n", script, strerror(errno));
		goto exit;
	}

	session = fast_session_new(sockfd);
	if (!session) {
		fprintf(stderr, "FAST session cannot be created\n");
		goto exit;
	}

	container = fcontainer_new();
	if (!container) {
		fprintf(stderr, "Cannot allocate container\n");
		goto exit;
	}

	if (fast_suite_template(session, xml)) {
		fprintf(stderr, "Cannot read template xml file\n");
		goto exit;
	}

	fcontainer_init(container, session->rx_messages);

	if (script_read(stream, container)) {
		fprintf(stderr, "Invalid script: %s\n", script);
		goto exit;
	}

	expected_elem = next_elem(container);

	while (expected_elem) {
		msg = fast_session_recv(session, 0);

		if (!msg)
			continue;

		fprintf(stdout, "< ");
		fprintmsg(stdout, msg);

		if (fmsgcmp(&expected_elem->msg, msg)) {
			fprintf(stderr, "Client: messages differ\n");
			fprintmsg(stderr, &expected_elem->msg);
			fprintmsg(stderr, msg);
			goto exit;
		}

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
	{ "fast",		fast_session_initiate },
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
	printf("\n  usage: trade client -t [template] -f [filename] -h [hostname] -p [port] -c [protocol]\n\n");
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
	const char *xml = NULL;
	struct sockaddr_in sa;
	int saved_errno = 0;
	struct hostent *he;
	int sockfd = -1;
	int port = 0;
	int retval;
	char **ap;
	int opt;

	while ((opt = getopt(argc, argv, "f:h:p:c:t:")) != -1) {
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
		case 't':
			xml = optarg;
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (!port || !proto || !filename || !host || !xml) {
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

	retval = proto_info->session_initiate(proto_info, sockfd, xml, filename);

	shutdown(sockfd, SHUT_RDWR);

	if (close(sockfd) < 0)
		die("close");

	return retval;
}
