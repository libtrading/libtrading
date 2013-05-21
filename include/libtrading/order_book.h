#ifndef	LIBTRADING_ORDER_BOOK_H
#define	LIBTRADING_ORDER_BOOK_H

#include <libtrading/types.h>

#include <stdbool.h>

#define	MAX_BOOK_DEPTH	50

struct price_level {
	u64	price;
	u64	size;
};

struct order_book {
	struct price_level	bids[MAX_BOOK_DEPTH];
	struct price_level	asks[MAX_BOOK_DEPTH];

	int		depth;
};

bool order_book_is_valid(struct order_book *ob);
int new_price_level(struct price_level *levels, int depth, struct price_level *level, int ind);
int set_price_level(struct price_level *levels, int depth, struct price_level *level, int ind);
int change_price_level(struct price_level *levels, int depth, struct price_level *level, int ind);
int delete_price_level(struct price_level *levels, int depth, struct price_level *level, int ind);

#endif	/* LIBTRADING_ORDER_BOOK_H */
