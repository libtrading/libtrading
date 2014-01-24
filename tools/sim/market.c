#include "libtrading/proto/fix_message.h"
#include "libtrading/proto/fix_session.h"
#include "libtrading/array.h"
#include "libtrading/die.h"
#include "market.h"

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define	EPOLL_MAXEVENTS	100

static const char *program;

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

static void usage(void)
{
	fprintf(stderr, "\n usage: %s -p port\n\n", program);

	exit(EXIT_FAILURE);
}

static void market_init(struct market *market)
{
	memset(market, 0, sizeof(struct market));

	order_book_init(&market->book);

	return;
}

static inline struct fix_field *fix_field_by_tag(struct fix_message *msg, enum fix_tag tag)
{
	struct fix_field *field = NULL;
	int i;

	for (i = 0; i < msg->nr_fields; i++) {
		field = msg->fields + i;

		if (field->tag == tag)
			return field;
	}

	return NULL;
}

static int do_income(struct market *market, struct fix_session_cfg *cfg)
{
	struct fix_message *recv_msg;
	struct fix_message send_msg;
	struct fix_field *field;
	struct trader *trader;
	struct order order;

	trader = trader_by_sock(market, cfg->sockfd);
	if (!trader) {
		trader = trader_new(market);
		if (!trader)
			goto fail;

		trader->session = fix_session_new(cfg);
		if (!trader->session)
			goto fail;
	}

	recv_msg = fix_session_recv(trader->session, 0);
	if (!recv_msg)
		goto done;

	send_msg.nr_fields = 0;

	if (!trader->active) {
		if (!fix_message_type_is(recv_msg, FIX_MSG_TYPE_LOGON))
			goto logout;

		field = fix_field_by_tag(recv_msg, SenderCompID);
		if (!field)
			goto logout;

		strncpy(trader->name, field->string_value, 32);

		trader->active = true;

		send_msg.type = FIX_MSG_TYPE_LOGON;
		fix_session_send(trader->session, &send_msg, 0);

		goto done;
	}

	if (fix_session_admin(trader->session, recv_msg))
		goto done;

	if (fix_message_type_is(recv_msg, FIX_MSG_TYPE_LOGOUT)) {
		goto logout;
	} else if (fix_message_type_is(recv_msg, FIX_MSG_TYPE_NEW_ORDER_SINGLE)) {
		order.trader = trader->id;

		field = fix_field_by_tag(recv_msg, Side);
		if (!field)
			goto done;

		if (field->string_value[0] == '1')
			order.side = 0;
		else if (field->string_value[0] == '2')
			order.side = 1;
		else
			goto done;

		field = fix_field_by_tag(recv_msg, Price);
		if (!field)
			goto done;

		order.level = round(field->float_value);

		field = fix_field_by_tag(recv_msg, OrderQty);
		if (!field)
			goto done;

		order.size = round(field->float_value);

		if (do_limit(&market->book, &order))
			goto done;
	}

done:
	return 0;

fail:
	return -1;

logout:
	send_msg.type = FIX_MSG_TYPE_LOGOUT;
	fix_session_send(trader->session, &send_msg, 0);

	trader_free(trader);
	close(cfg->sockfd);

	return 0;
}

int main(int argc, char *argv[])
{
	struct epoll_event *events = NULL;
	struct market *market = NULL;
	struct fix_session_cfg cfg;
	struct epoll_event event;
	struct sockaddr_in sa;
	int sockfd = -1;
	int port = 0;
	int efd = -1;
	int ret = -1;
	int num;
	int opt;
	int i;

	program = basename(argv[0]);

	while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			break;
		default:
			usage();
		}
	}

	if (!port)
		usage();

	market = calloc(1, sizeof(struct market));
	if (!market)
		die("couldn't create a market");

	market_init(market);
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0)
		die("cannot create a socket");

	if (socket_setopt(sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
		die("cannot set a socket option: TCP_NODELAY");

	if (socket_setopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 1) < 0)
		die("cannot set a socket option: SO_REUSEADDR");

	sa = (struct sockaddr_in) {
		.sin_family		= AF_INET,
		.sin_port		= htons(port),
		.sin_addr		= (struct in_addr) {
			.s_addr			= INADDR_ANY,
		},
	};

	if (bind(sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
		die("bind failed");

	fprintf(stdout, "Market is running on port %d...\n", port);

	if (listen(sockfd, 10) < 0)
		die("listen failed");

	efd = epoll_create1(0);
	if (efd < 0)
		die("epoll_create1 failed");

	events = calloc(EPOLL_MAXEVENTS, sizeof(struct epoll_event));
	if (!events)
		die("calloc failed");

	memset(&event, 0, sizeof(struct epoll_event));

	event.data.fd = sockfd;
	event.events = EPOLLIN | EPOLLRDHUP;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &event)) {
		fprintf(stderr, "epoll_ctl failed\n");
		goto exit;
	}

	strncpy(cfg.sender_comp_id, "SELLSIDE", ARRAY_SIZE(cfg.sender_comp_id));
	strncpy(cfg.target_comp_id, "BUYSIDE", ARRAY_SIZE(cfg.target_comp_id));
	cfg.dialect = &fix_dialects[FIX_4_4];

	while (true) {
		num = epoll_wait(efd, events, EPOLL_MAXEVENTS, -1);
		if (num < 0) {
			fprintf(stderr, "epoll_wait failed\n");
			goto exit;
		}

		for (i = 0; i < num; i++) {
			if (events[i].data.fd == sockfd) {
				if (events[i].events & EPOLLERR ||
					events[i].events & EPOLLHUP ||
						events[i].events & EPOLLRDHUP) {
					fprintf(stderr, "listening socket error\n");
					goto exit;
				}

				cfg.sockfd = accept(sockfd, NULL, NULL);
				if (cfg.sockfd < 0) {
					fprintf(stderr, "accept failed\n");
					goto exit;
				}

				event.data.fd = cfg.sockfd;
				event.events = EPOLLIN | EPOLLRDHUP;
				if (epoll_ctl(efd, EPOLL_CTL_ADD, cfg.sockfd, &event) < 0) {
					fprintf(stderr, "epoll_ctl failed\n");
					goto exit;
				}
			} else {
				cfg.sockfd = events[i].data.fd;

				if (events[i].events & EPOLLERR ||
					events[i].events & EPOLLHUP ||
						events[i].events & EPOLLRDHUP) {
					trader_free(trader_by_sock(market, cfg.sockfd));
					close(cfg.sockfd);

					continue;
				}

				if (do_income(market, &cfg))
					close(cfg.sockfd);
			}
		}
	}

exit:
	close(sockfd);
	free(events);
	free(market);

	return ret;
}
