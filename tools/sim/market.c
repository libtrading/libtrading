#include "libtrading/proto/fix_message.h"
#include "libtrading/proto/fix_session.h"
#include "libtrading/array.h"
#include "libtrading/die.h"
#include "market.h"

#include <event2/listener.h>
#include <event2/event.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <libgen.h>
#include <stdlib.h>
#include <event.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>

#define	EPOLL_MAXEVENTS	100

static const char *program;

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

static int do_income(struct market *market, int sockfd)
{
	struct fix_message *recv_msg;
	struct fix_message send_msg;
	struct fix_session_cfg cfg;
	struct fix_field *field;
	struct trader *trader;
	struct order order;

	fix_session_cfg_init(&cfg);

	trader = trader_by_sock(market, sockfd);
	if (!trader) {
		trader = trader_new(market);
		if (!trader)
			goto fail;

		strncpy(cfg.sender_comp_id, "SELLSIDE",
				ARRAY_SIZE(cfg.sender_comp_id));
		strncpy(cfg.target_comp_id, "BUYSIDE",
				ARRAY_SIZE(cfg.target_comp_id));
		cfg.dialect = &fix_dialects[FIX_4_4];
		cfg.sockfd = sockfd;

		trader->session = fix_session_new(&cfg);
		if (!trader->session)
			goto fail;
	}

	if (fix_session_recv(trader->session, &recv_msg, 0) <= 0)
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
	close(sockfd);

	return 0;
}

static void ev_cb(evutil_socket_t sockfd, short events, void *arg)
{
	if (do_income(arg, sockfd))
		close(sockfd);

	return;
}

static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd,
				struct sockaddr *address, int socklen, void *arg)
{
	struct event_base *base = evconnlistener_get_base(listener);
	struct event *ev = event_new(base, fd, EV_READ | EV_PERSIST, ev_cb, arg);

	event_add(ev, NULL);

	return;
}

static void accept_error_cb(struct evconnlistener *listener, void *arg)
{
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();

	fprintf(stderr, "Listener error (%d) %s\n",
			err, evutil_socket_error_to_string(err));

	event_base_loopexit(base, NULL);

	return;
}

int main(int argc, char *argv[])
{
	struct evconnlistener *listener;
	struct market *market = NULL;
	struct event_base *base;
	struct sockaddr_in sa;
	int sockfd = -1;
	int port = 0;
	int ret = EXIT_FAILURE;
	int opt;

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

	memset(&sa, 0, sizeof(sa));

	sa = (struct sockaddr_in) {
		.sin_family		= AF_INET,
		.sin_port		= htons(port),
		.sin_addr		= (struct in_addr) {
			.s_addr			= INADDR_ANY,
		},
	};

	fprintf(stdout, "Market is running on port %d...\n", port);

	base = event_base_new();
	if (!base) {
		perror("Couldn't create a base");

		goto exit;
	}

	listener = evconnlistener_new_bind(base, accept_conn_cb, market,
			LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
			-1, (struct sockaddr *)&sa, sizeof(sa));

	if (!listener) {
		perror("Couldn't create a listener");

		goto exit;
	}
	evconnlistener_set_error_cb(listener, accept_error_cb);

	event_base_dispatch(base);

	ret = EXIT_SUCCESS;

exit:
	close(sockfd);
	free(market);

	return ret;
}
