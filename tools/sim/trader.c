#include "fix/fix_common.h"

#include "libtrading/array.h"
#include "libtrading/die.h"

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>

#define	FIX_FIELDS_NUM	32

static const char *program;

static void usage(void)
{
	fprintf(stderr, "\n usage: %s -h host -p port\n\n", program);

	exit(EXIT_FAILURE);
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

static int do_trader(struct fix_session_cfg *cfg)
{
	struct fix_session *session = NULL;
	struct fix_field *fields = NULL;
	double price, qty;
	int ret = -1;
	int nr = 0;
	int side;

	session = fix_session_new(cfg);
	if (!session) {
		fprintf(stderr, "FIX session cannot be created\n");
		goto exit;
	}

	ret = fix_session_logon(session);
	if (ret) {
		fprintf(stderr, "Trader Logon failed\n");
		goto exit;
	}

	fprintf(stdout, "Trader Logged on\n");

	fields = calloc(FIX_FIELDS_NUM, sizeof(struct fix_field));
	if (!fields) {
		fprintf(stderr, "Cannot allocate memory\n");
		goto exit;
	}

	while (true) {
		nr = 0;

		fprintf(stdout, "Buy/Sell/Quit (1/2/0): ");
		if (scanf("%d", &side) != 1)
			continue;

		if (side == 1)
			fields[nr++] = FIX_STRING_FIELD(Side, "1");
		else if (side == 2)
			fields[nr++] = FIX_STRING_FIELD(Side, "2");
		else if (side == 0)
			break;
		else
			continue;

		fprintf(stdout, "Price: ");
		if (scanf("%lf", &price) != 1)
			continue;

		if (price <= 0)
			continue;

		fields[nr++] = FIX_FLOAT_FIELD(Price, price);

		fprintf(stdout, "Quantity: ");
		if (scanf("%lf", &qty) != 1)
			continue;

		if (qty <= 0)
			continue;

		fields[nr++] = FIX_FLOAT_FIELD(OrderQty, qty);

		fix_session_new_order_single(session, fields, nr);
	}

	ret = fix_session_logout(session, NULL);
	if (!ret)
		fprintf(stdout, "Trader logged out\n");
	else
		fprintf(stderr, "Trader logout failed\n");

exit:
	fix_session_free(session);
	free(fields);

	return ret;
}

int main(int argc, char *argv[])
{
	struct fix_session_cfg cfg;
	const char *host = NULL;
	struct sockaddr_in sa;
	int saved_errno = 0;
	struct hostent *he;
	int port = 0;
	int ret = 0;
	char **ap;
	int opt;

	program = basename(argv[0]);

	while ((opt = getopt(argc, argv, "h:p:")) != -1) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			break;
		case 'h':
			host = optarg;
			break;
		default: /* '?' */
			usage();
		}
	}

	if (!port || !host)
		usage();

	fix_session_cfg_init(&cfg);

	strncpy(cfg.target_comp_id, "SELLSIDE", ARRAY_SIZE(cfg.target_comp_id));
	strncpy(cfg.sender_comp_id, "BUYSIDE", ARRAY_SIZE(cfg.sender_comp_id));
	cfg.dialect	= &fix_dialects[FIX_4_4];

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

	ret = do_trader(&cfg);

	shutdown(cfg.sockfd, SHUT_RDWR);

	if (close(cfg.sockfd) < 0)
		die("close");

	return ret;
}
