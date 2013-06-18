#include "libtrading/proto/fast_message.h"
#include "libtrading/proto/fast_session.h"

#include "libtrading/array.h"
#include "libtrading/die.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "test.h"

static bool	stop;

static void signal_handler(int signum)
{
	if (signum == SIGINT)
		stop = true;
}

struct protocol_info {
	const char		*name;
	int			(*session_initiate)(struct fast_session_cfg *, const char *);
};

static int raw_session_initiate(struct fast_session_cfg *cfg, const char *out)
{
	unsigned char buf[FAST_MESSAGE_MAX_SIZE];
	int ret = 1;
	int outfd;

	outfd = open(out, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (outfd < 0) {
		fprintf(stderr, "Parser: Cannot open a file (%s)\n", strerror(errno));
		goto exit;
	}

	ret = read(cfg->sockfd, buf, FAST_MESSAGE_MAX_SIZE);
	if (ret < 0) {
		fprintf(stderr, "Parser: Cannot read data (%s)\n", strerror(errno));
		goto exit;
	}

	ret = write(outfd, buf, ret);
	if (ret < 0) {
		fprintf(stderr, "Parser: Cannot write data (%s)\n", strerror(errno));
		goto exit;
	}

	close(outfd);

exit:
	return ret;
}

static int fast_session_initiate(struct fast_session_cfg *cfg, const char *xml)
{
	struct fast_session *session = NULL;
	struct fast_message *msg;
	struct sigaction sa;
	int ret = 1;

	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) == -1)
		die("unable to register signal handler");

	session = fast_session_new(cfg);
	if (!session) {
		fprintf(stderr, "Parser: FAST session cannot be created\n");
		goto exit;
	}

	if (fast_parse_template(session, xml)) {
		fprintf(stderr, "Parser: Cannot read template xml file\n");
		goto exit;
	}

	while (!stop) {
		msg = fast_session_recv(session, 0);

		if (!msg) {
			continue;
		} else if (fast_msg_has_flags(msg, FAST_MSG_FLAGS_RESET)) {
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

			if (session->reset)
				fast_session_reset(session);
		}
	}

	ret = 0;

exit:
	fast_session_free(session);

	return ret;
}

static const struct protocol_info protocols[] = {
	{ "raw",		raw_session_initiate},
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
	const char format[] = "  %-30s:   %-30s\n";

	fprintf(stdout, "\n  The program works in 3 possible modes:");
	fprintf(stdout, "\n   * Capture raw data from a socket and put them into a file without any modification");
	fprintf(stdout, "\n   * Capture raw data from a file and put decoded FAST messages to the standard output");
	fprintf(stdout, "\n   * Capture raw data from a socket and put decoded FAST messages to the standard output");

	fprintf(stdout, "\n\n  Here is a short summary of the options available:\n");
	fprintf(stdout, format, "-p, --protocol [fast | raw]", "protocol to be used");
	fprintf(stdout, format, "-m, --multicast ip:port", "multicast data input");
	fprintf(stdout, format, "-s, --source ip", "source specific multicast");
	fprintf(stdout, format, "-l, --listen ip", "multicast listen address");
	fprintf(stdout, format, "-t, --template file", "template file to be used");
	fprintf(stdout, format, "-b, --preamble bytes", "number of bytes in preamble");
	fprintf(stdout, format, "-f, --file file", "ordinary file data input");
	fprintf(stdout, format, "-o, --out file", "ordinary file data output");
	fprintf(stdout, format, "-r, --reset", "implicit reset (0xCO 0xF8)");

	fprintf(stdout, "\n\n  Usage examples:");
	fprintf(stdout, "\n  # fast_parser -p fast -t template -m ip:port -l ip");
	fprintf(stdout, "\n  # fast_parser -p fast -t template -f file");
	fprintf(stdout, "\n  # fast_parser -p raw -m ip:port -l ip -o file");
	fprintf(stdout, "\n\n");

	exit(EXIT_FAILURE);
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

static int msocket(const char *ip, const char *lip, const char *sip, int port)
{
	struct ip_mreq_source group_src;
	struct sockaddr_in sa;
	struct ip_mreq group;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd < 0) {
		fprintf(stderr, "Parser: Unable to open a socket (%s)\n", strerror(errno));
		goto fail;
	}

	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK) < 0) {
		fprintf(stderr, "Parser: Unable to make a socket nonblocking (%s)\n", strerror(errno));
		goto fail;
	}

	if (socket_setopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 1) < 0) {
		fprintf(stderr, "Parser: Unable to set opt (%s)\n", strerror(errno));
		goto fail;
	}

	sa = (struct sockaddr_in) {
		.sin_family		= AF_INET,
		.sin_port		= htons(port),
		.sin_addr		= (struct in_addr) {
			.s_addr			= INADDR_ANY,
		},
	};

	if (bind(sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "Parser: Unable to bind (%s)\n", strerror(errno));
		goto fail;
	}

	if (!sip) {
		group.imr_multiaddr.s_addr = inet_addr(ip);
		group.imr_interface.s_addr = inet_addr(lip);

		if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) {
			fprintf(stderr, "Parser: Unable to join a group (%s)\n", strerror(errno));
			goto fail;
		}
	} else {
		group_src.imr_multiaddr.s_addr = inet_addr(ip);
		group_src.imr_interface.s_addr = inet_addr(lip);
		group_src.imr_sourceaddr.s_addr = inet_addr(sip);

		if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, (char *)&group_src, sizeof(group_src)) < 0) {
			fprintf(stderr, "Parser: Unable to join a group (%s)\n", strerror(errno));
			goto fail;
		}
	}

	return sockfd;

fail:
	return -1;
}

int main(int argc, char *argv[])
{
	const struct protocol_info *proto_info = NULL;
	struct fast_session_cfg cfg;
	const char *output = NULL;
	const char *input = NULL;
	const char *proto = NULL;
	const char *tmp = NULL;
	const char *xml = NULL;
	const char *lip = NULL;
	const char *sip = NULL;
	const char *ip = NULL;
	bool reset = false;
	int opt_index = 0;
	int preamble = 0;
	int port = 0;
	int fd = -1;
	int opt;
	int ret;

	const char *short_opt = "f:l:m:o:p:t:b:s:r";
	const struct option long_opt[] = {
		{"multicast", required_argument, NULL, 'm'},
		{"protocol", required_argument, NULL, 'p'},
		{"preamble", required_argument, NULL, 'b'},
		{"template", required_argument, NULL, 't'},
		{"listen", required_argument, NULL, 'l'},
		{"source", required_argument, NULL, 's'},
		{"file", required_argument, NULL, 'h'},
		{"out", required_argument, NULL, 'o'},
		{"reset", no_argument, NULL, 'r'},
		{NULL, 0, NULL, 0}
	};

	while ((opt = getopt_long(argc, argv, short_opt, long_opt, &opt_index)) != -1) {
		switch (opt) {
		case 'm':
			ip = optarg;
			port = atoi(strchr(ip, ':') + 1);
			*strchr(ip, ':') = 0;
			break;
		case 'b':
			preamble = atoi(optarg);
			break;
		case 'p':
			proto = optarg;
			break;
		case 't':
			xml = optarg;
			break;
		case 'l':
			lip = optarg;
			break;
		case 's':
			sip = optarg;
			break;
		case 'f':
			input = optarg;
			break;
		case 'o':
			output = optarg;
			break;
		case 'r':
			reset = true;
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (!proto)
		usage();

	if (!strncmp(proto, "raw", 3)) {
		if (!ip || !port || !lip || !output)
			usage();

		proto_info = lookup_protocol_info(proto);
		if (!proto_info)
			error("Parser: Unsupported protocol '%s'\n", proto);

		fd = msocket(ip, lip, sip, port);
		if (fd < 0)
			error("Parser: Multicast socket creation failed\n");

		tmp = output;
	} else if (!strncmp(proto, "fast", 4)) {
		if ((!ip || !port || !lip || !xml) && !input)
			usage();

		if (ip && port && lip && xml && input)
			usage();

		proto_info = lookup_protocol_info(proto);
		if (!proto_info)
			error("Parser: Unsupported protocol '%s'\n", proto);

		if (input) {
			fd = open(input, O_RDONLY);
			if (fd < 0)
				error("Parser: Unable to open a file (%s)\n", strerror(errno));
		} else {
			fd = msocket(ip, lip, sip, port);
			if (fd < 0)
				error("Parser: Multicast socket creation failed\n");
		}

		tmp = xml;
	} else
		usage();

	cfg.preamble_bytes = preamble;
	cfg.reset = reset;
	cfg.sockfd = fd;

	ret = proto_info->session_initiate(&cfg, tmp);

	if (ip || port)
		shutdown(fd, SHUT_RDWR);

	if (close(fd) < 0)
		die("close");

	return ret;
}
