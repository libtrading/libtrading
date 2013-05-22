#include "libtrading/proto/fix_message.h"
#include "libtrading/proto/fix_session.h"

#include "libtrading/compat.h"
#include "libtrading/array.h"
#include "libtrading/die.h"

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <libgen.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

static const char	*program;

static bool		stop;
static int		test;

static void signal_handler(int signum)
{
	if (signum == SIGINT)
		stop = true;
}

static unsigned long fix_new_order_single_fields(struct fix_field *fields)
{
	unsigned long nr = 0;
	char buf[64];

	fix_timestamp_now(buf, sizeof buf);

	fields[nr++] = FIX_STRING_FIELD(ClOrdID, "ClOrdID");
	fields[nr++] = FIX_STRING_FIELD(TransactTime, buf);
	fields[nr++] = FIX_STRING_FIELD(Symbol, "Symbol");
	fields[nr++] = FIX_FLOAT_FIELD(OrderQty, 100);
	fields[nr++] = FIX_STRING_FIELD(OrdType, "2");
	fields[nr++] = FIX_STRING_FIELD(Side, "1");
	fields[nr++] = FIX_FLOAT_FIELD(Price, 100);

	return nr;
}

static void fix_session_test(struct fix_session *session)
{
	struct fix_field *fields;
	struct fix_message *msg;
	double average_time;
	unsigned long nr;
	int i;

	fields = calloc(FIX_MAX_FIELD_NUMBER, sizeof(struct fix_field));
	if (!fields)
		return;

	nr = fix_new_order_single_fields(fields);

	average_time = 0.0;

	for (i = 0; i < test; i++) {
		struct timespec before, after;

		clock_gettime(CLOCK_MONOTONIC, &before);

		fix_session_new_order_single(session, fields, nr);

retry:
		msg = fix_session_recv(session, 0);
		if (!msg)
			goto retry;

		if (!fix_message_type_is(msg, FIX_MSG_TYPE_EXECUTION_REPORT))
			goto retry;

		clock_gettime(CLOCK_MONOTONIC, &after);

		average_time += 1000000 * (after.tv_sec - before.tv_sec) +
						(after.tv_nsec - before.tv_nsec) / 1000;
	}

	printf("Messages sent: %d, average latency: %.1lf\n", test, average_time / test);

	free(fields);
}

static void fix_session_normal(struct fix_session *session)
{
	struct timespec cur, prev;
	struct fix_message *msg;
	int diff;

	clock_gettime(CLOCK_MONOTONIC, &prev);

	while (!stop && session->active) {
		clock_gettime(CLOCK_MONOTONIC, &cur);
		diff = cur.tv_sec - prev.tv_sec;

		if (diff > 0.1 * session->heartbtint) {
			prev = cur;

			if (!fix_session_keepalive(session, &cur)) {
				stop = true;
				break;
			}
		}

		msg = fix_session_recv(session, 0);
		if (msg) {
			if (fix_session_admin(session, msg))
				continue;

			switch (msg->type) {
			case FIX_MSG_TYPE_LOGOUT:
				stop = true;
				break;
			default:
				stop = true;
				break;
			}
		}
	}
}

static int fix_session_initiate(struct fix_session_cfg *cfg)
{
	struct fix_session *session;
	int retval;

	if (signal(SIGINT, signal_handler) == SIG_ERR)
		die("unable to register signal handler");

	session	= fix_session_new(cfg);
	if (!session)
		die("unable to allocate memory for session");

	retval = fix_session_logon(session);
	if (!retval) {
		printf("Logon OK\n");
	} else {
		printf("Logon FAILED\n");
		goto exit;
	}

	if (!test)
		fix_session_normal(session);
	else
		fix_session_test(session);

	if (session->active) {
		retval = fix_session_logout(session, NULL);
		if (!retval) {
			printf("Logout OK\n");
		} else {
			printf("Logout FAILED\n");
		}
	} else
		printf("Logout OK\n");

	fix_session_free(session);

exit:
	return retval;
}

static void usage(void)
{
	printf("\n  usage: %s [hostname] [port] [version] [sender-comp-id] [target-comp-id]\n\n", program);

	exit(EXIT_FAILURE);
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

static enum fix_version strversion(const char *name)
{
	if (!strcmp(name, "fix42"))
		return FIX_4_2;
	else if (!strcmp(name, "fix43"))
		return FIX_4_3;
	else if (!strcmp(name, "fix44"))
		return FIX_4_4;

	return FIX_4_4;
}

int main(int argc, char *argv[])
{
	struct fix_session_cfg cfg;
	struct sockaddr_in sa;
	int saved_errno = 0;
	struct hostent *he;
	const char *host;
	int retval;
	char **ap;
	int port;
	int opt;

	program	= basename(argv[0]);

	while ((opt = getopt(argc, argv, "i:t:")) != -1) {
		switch (opt) {
		case 'i':
			stop = true;
			break;
		case 't':
			test = atoi(optarg);
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (argc < 6)
		usage();

	host		= argv[optind];
	port		= atoi(argv[optind + 1]);
	cfg.version	= strversion(argv[optind + 2]);
	strncpy(cfg.sender_comp_id, argv[optind + 3], ARRAY_SIZE(cfg.sender_comp_id));
	strncpy(cfg.target_comp_id, argv[optind + 4], ARRAY_SIZE(cfg.target_comp_id));

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

	cfg.heartbtint = 15;

	retval = fix_session_initiate(&cfg);

	shutdown(cfg.sockfd, SHUT_RDWR);

	if (close(cfg.sockfd) < 0)
		die("close");

	return retval;
}
