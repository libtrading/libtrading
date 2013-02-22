#include "libtrading/proto/fast_message.h"
#include "libtrading/proto/fast_session.h"

#include "libtrading/array.h"
#include "libtrading/die.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "test.h"

int reset_id;

struct protocol_info {
	const char		*name;
	int			(*session_initiate)(const struct protocol_info *, int, const char *);
};

static int fast_session_initiate(const struct protocol_info *proto, int sockfd, const char *xml)
{
	struct fast_session *session = NULL;
	struct fast_message *msg;
	int ret = 1;

	session = fast_session_new(sockfd);
	if (!session) {
		fprintf(stderr, "Parser: FAST session cannot be created\n");
		goto exit;
	}

	if (fast_micex_template(session, xml)) {
		fprintf(stderr, "Parser: Cannot read template xml file\n");
		goto exit;
	}

	while (true) {
		msg = fast_session_recv(session, 0);

		if (!msg) {
			fprintf(stderr, "Parser: Nothing received\n");
			goto exit;
		} else if (msg->tid == reset_id) {
			if (msg->nr_fields != 0) {
				fprintf(stderr, "Parser: Reset msg must be empty\n");
				goto exit;
			} else {
				fprintf(stdout, "< tid = %lu: reset\n", msg->tid);
				fast_session_reset(session);
			}
		} else {
			fprintf(stdout, "< tid = %lu:\n", msg->tid);
			fprintmsg(stdout, msg);
		}
	}

	ret = 0;

exit:
	fast_session_free(session);

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
	printf("\n  usage: trade client -t [template] -h [hostname] -p [port] -l [listen ip] -c [protocol] -r [reset id]\n\n");
	exit(EXIT_FAILURE);
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

int main(int argc, char *argv[])
{
	const struct protocol_info *proto_info;
	const char *proto = NULL;
	const char *host = NULL;
	const char *xml = NULL;
	const char *ip = NULL;
	struct sockaddr_in sa;
	struct ip_mreq group;
	int sockfd = -1;
	int port = 0;
	int retval;
	int opt;

	while ((opt = getopt(argc, argv, "h:p:c:t:r:l:")) != -1) {
		switch (opt) {
		case 'r':
			reset_id = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
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
		case 'l':
			ip = optarg;
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (!port || !proto || !host || !xml || !ip || !reset_id) {
		usage();
		exit(EXIT_FAILURE);
	}

	proto_info = lookup_protocol_info(proto);
	if (!proto_info)
		error("Parser: Unsupported protocol '%s'\n", proto);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd < 0)
		error("Parser: Unable to open a socket (%s)\n", strerror(errno));

	if (socket_setopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 1) < 0)
		error("Parser: Unable to set opt (%s)\n", strerror(errno));

	sa = (struct sockaddr_in) {
		.sin_family		= AF_INET,
		.sin_port		= htons(port),
		.sin_addr		= (struct in_addr) {
			.s_addr			= INADDR_ANY,
		},
	};

	if (bind(sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
		error("Parser: Unable to bind (%s)\n", strerror(errno));

	group.imr_multiaddr.s_addr = inet_addr(host);
	group.imr_interface.s_addr = inet_addr(ip);

	if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
		error("Parser: Unable to join a group (%s)\n", strerror(errno));

	retval = proto_info->session_initiate(proto_info, sockfd, xml);

	shutdown(sockfd, SHUT_RDWR);

	if (close(sockfd) < 0)
		die("close");

	return retval;
}
