#include <string.h>
#include <stdio.h>

#include "engine.h"

void order_book_init(struct order_book *book)
{
	memset(book, 0, sizeof(struct order_book));

	book->best_ask = MAX_LEVEL + 1;
	book->best_bid = 0;
}

static void order_insert(struct level *level, struct order *order)
{
	if (level->head != NULL) {
		level->tail->next = order;
		order->prev = level->tail;
	} else {
		level->head = order;
		order->prev = NULL;
	}

	level->tail = order;
	order->next = NULL;

	return;
}

static void order_remove(struct level *level, struct order *order)
{
	struct order *prev = order->prev;
	struct order *next = order->next;

	if (prev == NULL)
		level->head = next;
	else
		prev->next = next;

	if (next == NULL)
		level->tail = prev;
	else
		next->prev = prev;

	return;
}

static int do_trade(unsigned long buyer, unsigned long seller, unsigned long price, size_t size)
{
	fprintf(stdout, "Buyer %lu Seller %lu, price %lu, size %zu\n", buyer, seller, price, size);

	return 0;
}

static int match(struct order_book *book, struct order *order)
{
	struct level *level;
	struct order *head;

	if (order->side == 0) {
		level = book->levels + order->level;

		while (order->level >= book->best_ask) {
			level = book->levels + book->best_ask;
			head = level->head;

			while (head != NULL) {
				if (head->size < order->size) {
					do_trade(order->trader, head->trader, book->best_ask, head->size);

					order->size -= head->size;
					head->size = 0;
				} else {
					do_trade(order->trader, head->trader, book->best_ask, order->size);

					head->size -= order->size;
					order->size = 0;
				}

				if (!head->size) {
					order_remove(level, head);
					head = level->head;
				}

				if (!order->size)
					goto done;
			}

			book->best_ask++;
		}

		if (order->level > book->best_bid)
			book->best_bid = order->level;
	} else {
		level = book->levels + order->level;

		while (order->level <= book->best_bid) {
			level = book->levels + book->best_bid;
			head = level->head;

			while (head != NULL) {
				if (head->size < order->size) {
					do_trade(head->trader, order->trader, book->best_bid, head->size);

					order->size -= head->size;
					head->size = 0;
				} else {
					do_trade(head->trader, order->trader, book->best_bid, order->size);

					head->size -= order->size;
					order->size = 0;
				}

				if (!head->size) {
					order_remove(level, head);
					head = level->head;
				}

				if (!order->size)
					goto done;
			}

			book->best_bid--;
		}

		if (order->level < book->best_ask)
			book->best_ask = order->level;
	}

	order_insert(level, order);

done:
	return 0;
}

int do_limit(struct order_book *book, struct order *limit)
{
	struct order *order;

	if (book->orders_num >= MAX_ORDERS)
		goto reject;

	if (limit->level > MAX_LEVEL || limit->level < 1)
		goto reject;

	if (!limit->size)
		goto reject;

	order = book->orders + book->orders_num;
	memcpy(order, limit, sizeof(struct order));
	book->orders_num++;

	return match(book, order);

reject:
	return -1;
}

int do_cancel(struct order_book *book, unsigned long id)
{
	struct order *order = book->orders + id;
	struct level *level;

	if (!book->orders_num || id >= book->orders_num)
		goto reject;

	if (!order->size)
		goto reject;

	level = book->levels + order->level;
	order_remove(level, order);

	order->size = 0;

	return 0;

reject:
	return -1;
}
