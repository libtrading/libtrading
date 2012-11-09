#include "libtrading/proto/soupbin3_session.h"
#include "libtrading/proto/itch41_message.h"
#include "libtrading/proto/fix_message.h"
#include "libtrading/proto/fix_session.h"

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

static bool fix_in_seq_num_process(struct fix_session *session, struct fix_message *msg)
{
	if (msg->msg_seq_num < session->in_msg_seq_num && !fix_get_field(msg, PossDupFlag))
		return false;

	return true;
}

static unsigned long fix_new_order_single_fields(struct fix_field *fields)
{
	char fmt[64], buf[64];
	unsigned long nr = 0;
	struct timeval tv;
	struct tm *tm;

	gettimeofday(&tv, NULL);
	tm = gmtime(&tv.tv_sec);
	strftime(fmt, sizeof fmt, "%Y%m%d-%H:%M:%S", tm);
	snprintf(buf, sizeof buf, "%s.%03ld", fmt, (long)tv.tv_usec / 1000);

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

	return;
}

static void fix_session_normal(struct fix_session *session)
{
	struct fix_message *msg;

	while (!stop) {
		msg = fix_session_recv(session, 0);
		if (msg) {
			msg = fix_session_process(session, msg);
			if (!msg)
				continue;

			if (!fix_in_seq_num_process(session, msg))
				stop = true;
			else if (fix_message_type_is(msg, FIX_MSG_TYPE_LOGOUT))
				stop = true;
			else if (fix_message_type_is(msg, FIX_MSG_TYPE_HEARTBEAT))
				fix_session_heartbeat(session, NULL);
		}
	}

	return;
}

static int fix_session_initiate(int sockfd, const char *fix_version,
				const char *sender_comp_id, const char *target_comp_id)
{
	struct fix_session *session;
	enum fix_version version;
	int retval;

	version = FIX_4_4;

	if (!strcmp(fix_version, "fix42"))
		version = FIX_4_2;
	else if (!strcmp(fix_version, "fix43"))
		version = FIX_4_3;
	else if (!strcmp(fix_version, "fix44"))
		version = FIX_4_4;

	if (signal(SIGINT, signal_handler) == SIG_ERR)
		die("unable to register signal handler");

	session	= fix_session_new(sockfd, version, sender_comp_id, target_comp_id);
	if (!session)
		die("unable to allocate memory for session");

	if (fix_session_logon(session)) {
		printf("Logon OK\n");
		retval = 0;
	} else {
		printf("Logon FAILED\n");
		retval = 1;
	}

	if (!test)
		fix_session_normal(session);
	else
		fix_session_test(session);

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

static void usage(void)
{
	printf("\n  usage: %s [hostname] [port] [version] [sender-comp-id] [target-comp-id]\n\n", program);

	exit(EXIT_FAILURE);
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

int main(int argc, char *argv[])
{
	const char *sender_comp_id;
	const char *target_comp_id;
	struct sockaddr_in sa;
	int saved_errno = 0;
	const char *version;
	struct hostent *he;
	const char *host;
	int sockfd = -1;
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
	version		= argv[optind + 2];
	sender_comp_id	= argv[optind + 3];
	target_comp_id	= argv[optind + 4];

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

	retval = fix_session_initiate(sockfd, version, sender_comp_id, target_comp_id);

	shutdown(sockfd, SHUT_RDWR);

	if (close(sockfd) < 0)
		die("close");

	return retval;
}
