#define	MAX_ORDERS	10000
#define	MAX_LEVEL	100

struct order {
	unsigned long trader;
	unsigned long level;
	struct order *next;
	struct order *prev;
	unsigned long side;
	size_t size;
};

struct level {
	struct order *head;
	struct order *tail;
};

struct order_book {
	struct level levels[MAX_LEVEL + 2];
	struct order orders[MAX_ORDERS];
	unsigned long orders_num;
	unsigned long best_ask;
	unsigned long best_bid;
	unsigned long book_id;
	char code[32];
};

int do_cancel(struct order_book *book, unsigned long order_id);
int do_limit(struct order_book *book, struct order *order);
void order_book_init(struct order_book *book);
