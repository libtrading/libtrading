#include <string.h>

#include "engine.h"

#define	MAX_TRADERS	100

struct trader {
	struct fix_session *session;
	unsigned long id;
	char name[32];
	bool active;
};

struct market {
	struct trader traders[MAX_TRADERS];
	unsigned long traders_num;
	struct order_book book;
};

static inline struct trader *trader_by_sock(struct market *market, int sockfd)
{
	struct trader *traders = market->traders;
	struct trader *trader;
	int i;

	for (i = 0; i < market->traders_num; i++) {
		trader = traders + i;

		if (trader->session && trader->session->sockfd == sockfd)
			return trader;
	}

	return NULL;
}

static inline struct trader *trader_by_name(struct market *market, char *name)
{
	struct trader *traders = market->traders;
	struct trader *trader;
	int i;

	for (i = 0; i < market->traders_num; i++) {
		trader = traders + i;

		if (!strcmp(trader->name, name))
			return trader;
	}

	return NULL;
}

static inline struct trader *trader_by_id(struct market *market, unsigned long id)
{
	struct trader *traders = market->traders;
	struct trader *trader;
	int i;

	for (i = 0; i < market->traders_num; i++) {
		trader = traders + i;

		if (trader->id == id)
			return trader;
	}

	return NULL;
}

static inline struct trader *trader_new(struct market *market)
{
	struct trader *traders = market->traders;
	struct trader *trader;

	if (market->traders_num >= MAX_TRADERS)
		return NULL;

	trader = traders + market->traders_num;
	trader->id = market->traders_num++;
	trader->active = false;

	return trader;
}

static inline void trader_free(struct trader *trader)
{
	if (!trader)
		goto done;

	if (trader->session)
		fix_session_free(trader->session);

	trader->session = NULL;
	trader->active = false;

done:
	return;
}
