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

#include "fast_client.h"
#include "test.h"

static const char *program;

static struct fast_client_function fast_client_functions[] = {
	[FAST_CLIENT_SCRIPT] = {
		.fast_session_initiate	= fast_client_script,
		.mode			= FAST_CLIENT_SCRIPT,
	},
};

static int fast_client_script(struct fast_session_cfg *cfg, struct fast_client_arg *arg)
{
	struct fcontainer *container = NULL;
	struct fast_session *session = NULL;
	struct felem *expected_elem;
	struct fast_message *msg;
	FILE *stream = NULL;
	int ret = 1;

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

static void usage(void)
{
	printf("\n usage: %s [-m mode] [-f filename] -t template -h hostname -p port\n\n", program);

	exit(EXIT_FAILURE);
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

static enum fast_client_mode strclientmode(const char *mode)
{
	enum fast_client_mode m;

	if (!strcmp(mode, "script"))
		return FAST_CLIENT_SCRIPT;

	if (sscanf(mode, "%u", &m) != 1)
		return FAST_CLIENT_SCRIPT;

	switch (m) {
	case FAST_CLIENT_SCRIPT:
		return m;
	default:
		break;
	}

	return FAST_CLIENT_SCRIPT;
}

int main(int argc, char *argv[])
{
	enum fast_client_mode mode = FAST_CLIENT_SCRIPT;
	struct fast_client_arg arg = {0};
	struct fast_session_cfg cfg;
	const char *host = NULL;
	struct sockaddr_in sa;
	int saved_errno = 0;
	struct hostent *he;
	int port = 0;
	char **ap;
	int opt;
	int ret;

	program = basename(argv[0]);

	while ((opt = getopt(argc, argv, "f:h:p:t:")) != -1) {
		switch (opt) {
		case 'm':
			mode = strclientmode(optarg);
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
		case 'h':
			host = optarg;
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (!port || !host || !arg.xml)
		usage();

	he = gethostbyname(host);
	if (!he)
		error("Unable to look up %s (%s)", host, hstrerror(h_errno));

	for (ap = he->h_addr_list; *ap; ap++) {
		cfg.sockfd = socket(he->h_addrtype, SOCK_STREAM, IPPROTO_TCP);
		if (cfg.sockfd < 0) {
			saved_errno = errno;
			continue;
		}

		sa = (struct sockaddr_in) {
			.sin_family		= he->h_addrtype,
			.sin_port		= htons(port),
		};
		memcpy(&sa.sin_addr, *ap, he->h_length);

		if (connect(cfg.sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
			saved_errno = errno;
			close(cfg.sockfd);
			cfg.sockfd = -1;
			continue;
		}
		break;
	}

	if (cfg.sockfd < 0)
		error("Unable to connect to a socket (%s)", strerror(saved_errno));

	if (socket_setopt(cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
		die("cannot set socket option TCP_NODELAY");

	cfg.preamble_bytes = 0;
	cfg.reset = false;

	switch (mode) {
	case FAST_CLIENT_SCRIPT:
		ret = fast_client_functions[mode].fast_session_initiate(&cfg, &arg);
		break;
	default:
		error("Invalid mode");
	}

	shutdown(cfg.sockfd, SHUT_RDWR);

	if (close(cfg.sockfd) < 0)
		die("close");

	return ret;
}
