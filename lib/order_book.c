#include "libtrading/order_book.h"

#include <string.h>
#include <stdlib.h>

int ob_init(struct order_book *ob)
{
	if (!ob)
		goto fail;

	ob->ghbids = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);
	if (!ob->ghbids)
		goto fail;

	ob->ghasks = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);
	if (!ob->ghasks)
		goto fail;

	ob->gtasks = g_tree_new(g_uint_compare);
	if (!ob->gtasks)
		goto fail;

	ob->gtbids = g_tree_new(g_uint_compare);
	if (!ob->gtbids)
		goto fail;

	ob->glasks = NULL;
	ob->glbids = NULL;

	return 0;

fail:
	ob_fini(ob);
	return -1;
}

void ob_fini(struct order_book *ob)
{
	if (!ob)
		return;

	if (ob->ghbids)
		g_hash_table_destroy(ob->ghbids);

	if (ob->ghasks)
		g_hash_table_destroy(ob->ghasks);

	if (ob->gtbids)
		g_tree_destroy(ob->gtbids);

	if (ob->gtasks)
		g_tree_destroy(ob->gtasks);

	if (ob->glbids)
		g_list_free(ob->glbids);

	if (ob->glasks)
		g_list_free(ob->glasks);
}

int ob_clear(struct order_book *ob)
{
	ob_fini(ob);

	return ob_init(ob);
}

struct ob_level *ob_level_lookup(struct order_book *ob, struct ob_order *order)
{
	GHashTable *ghtable;

	if (order->buy) {
		ghtable = ob->ghbids;
	} else {
		ghtable = ob->ghasks;
	}

	return g_hash_table_lookup(ghtable, GUINT_TO_POINTER(order->price));
}

int ob_level_modify(struct order_book *ob, struct ob_order *order)
{
	struct ob_level *level;
	GHashTable *ghtable;
	GList **glist;
	GTree *gtree;

	if (!ob)
		goto fail;

	if (order->buy) {
		ghtable = ob->ghbids;
		glist = &ob->glbids;
		gtree = ob->gtbids;
	} else {
		ghtable = ob->ghasks;
		glist = &ob->glasks;
		gtree = ob->gtasks;
	}

	level = g_hash_table_lookup(ghtable, GUINT_TO_POINTER(order->price));

	if (!level) {
		level = calloc(1, sizeof(struct ob_level));
		if (!level)
			goto fail;

		level->seq_num = order->seq_num;
		level->price = order->price;
		level->size = order->size;

		g_hash_table_insert(ghtable, GUINT_TO_POINTER(level->price), level);
		*glist = g_list_insert_sorted(*glist, level, g_level_compare);
		g_tree_insert(gtree, GUINT_TO_POINTER(level->price), level);
	} else {
		if (level->seq_num >= order->seq_num)
			goto fail;

		level->seq_num = order->seq_num;
		level->size = order->size;
	}

	return 0;

fail:
	return -1;
}

int ob_level_delete(struct order_book *ob, struct ob_order *order)
{
	struct ob_level *level;
	GHashTable *ghtable;
	GList **glist;
	GTree *gtree;

	if (!ob)
		goto fail;

	if (order->buy) {
		ghtable = ob->ghbids;
		glist = &ob->glbids;
		gtree = ob->gtbids;
	} else {
		ghtable = ob->ghasks;
		glist = &ob->glasks;
		gtree = ob->gtasks;
	}

	level = g_hash_table_lookup(ghtable, GUINT_TO_POINTER(order->price));

	if (level) {
		if (!g_hash_table_remove(ghtable, GUINT_TO_POINTER(order->price)))
			goto fail;

		if (!g_tree_remove(gtree, GUINT_TO_POINTER(order->price)))
			goto fail;

		*glist = g_list_remove(*glist, level);
	}

	return 0;

fail:
	return -1;
}
