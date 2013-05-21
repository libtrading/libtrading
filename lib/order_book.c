#include "libtrading/order_book.h"

#include <string.h>

int new_price_level(struct price_level *levels, int depth, struct price_level *level, int ind)
{
	struct price_level *ptr;
	int diff;

	if (depth <= ind)
		goto fail;

	diff = depth - ind - 1;
	ptr = levels + ind;

	if (diff)
		memmove(ptr + 1, ptr, diff * sizeof(struct price_level));

	ptr->price = level->price;
	ptr->size = level->size;

	return 0;

fail:
	return -1;
}

int change_price_level(struct price_level *levels, int depth, struct price_level *level, int ind)
{
	struct price_level *ptr;

	if (depth <= ind)
		goto fail;

	ptr = levels + ind;

	if (ptr->price != level->price)
		goto fail;

	ptr->size = level->size;

	return 0;
fail:
	return -1;
}

int delete_price_level(struct price_level *levels, int depth, struct price_level *level, int ind)
{
	struct price_level *ptr;
	int diff;

	if (depth <= ind)
		goto fail;

	ptr = levels + ind;

	if (ptr->price != level->price)
		goto fail;

	diff = depth - ind - 1;
	ptr->size = 0;

	if (diff)
		memmove(ptr, ptr + 1, diff * sizeof(struct price_level));

	return 0;
fail:
	return -1;
}

int set_price_level(struct price_level *levels, int depth, struct price_level *level, int ind)
{
	struct price_level *ptr;

	if (depth <= ind)
		goto fail;

	ptr = levels + ind;

	ptr->price = level->price;
	ptr->size = level->size;

	return 0;
fail:
	return -1;
}

bool order_book_is_valid(struct order_book *ob)
{
	struct price_level *asks = ob->asks;
	struct price_level *bids = ob->bids;
	int i;

	for (i = 0; i < ob->depth - 1; i++) {
		if (asks[i].price >= asks[i + 1].price)
			goto fail;

		if (bids[i].price <= bids[i + 1].price)
			goto fail;
	}

	return true;

fail:
	return false;
}
