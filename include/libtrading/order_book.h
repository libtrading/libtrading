#ifndef	LIBTRADING_ORDER_BOOK_H
#define	LIBTRADING_ORDER_BOOK_H

#include <libtrading/types.h>

#include <stdbool.h>
#include <glib.h>

struct ob_level {
	unsigned long	seq_num;
	unsigned long	price;
	unsigned long	size;
};

struct ob_order {
	unsigned long	seq_num;
	unsigned long	price;
	unsigned long	size;
	bool		buy;
};

struct order_book {
	GHashTable	*ghbids;
	GHashTable	*ghasks;

	GTree		*gtbids;
	GTree		*gtasks;

	GList		*glbids;
	GList		*glasks;
};

static inline gint g_uint_compare(gconstpointer pa, gconstpointer pb)
{
	unsigned int a = GPOINTER_TO_UINT(pa);
	unsigned int b = GPOINTER_TO_UINT(pb);
	gint ret = 0;

	if (a < b)
		ret = -1;
	else if (a > b)
		ret  = 1;

	return ret;
}

static inline gint g_level_compare(gconstpointer pa, gconstpointer pb)
{
	const struct ob_level *la = pa;
	const struct ob_level *lb = pb;
	gint ret = 0;

	if (la->price < lb->price)
		ret = -1;
	else if (la->price > lb->price)
		ret = 1;

	return ret;
}

int ob_init(struct order_book *ob);
void ob_fini(struct order_book *ob);
int ob_clear(struct order_book *ob);
int ob_level_modify(struct order_book *ob, struct ob_order *order);
int ob_level_delete(struct order_book *ob, struct ob_order *order);
struct ob_level *ob_level_lookup(struct order_book *ob, struct ob_order *order);

#endif	/* LIBTRADING_ORDER_BOOK_H */
