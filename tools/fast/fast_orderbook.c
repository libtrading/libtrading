#include "libtrading/proto/fast_book.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <inttypes.h>
#include <ncurses.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

bool stop;

static void signal_handler(int signum)
{
	if (signum == SIGINT)
		stop = true;
}

static void fast_books_print(struct fast_book_set *set)
{
	struct fast_book *book;
	int row, col;
	u64 mask = 0;
	int depth;
	int i, j;

	for (i = 0; i < set->books_num; i++) {
		book = set->books + i;

		mask |= book_has_mask(set, book);
	}

	if (!mask)
		return;

	clear();

	for (i = 0; i < set->books_num; i++) {
		book = set->books + i;

		col = 15 * i;
		row = 0;

		mvprintw(row++, col, "%s\n", book->symbol);

		attron(COLOR_PAIR(1));
		depth = book->ob.depth;
		for (j = 0; j < depth; j++) {
			mvprintw(row++, col, "%6"PRIi64" %6"PRIi64"\n",
					book->ob.asks[depth - 1 - j].price,
						book->ob.asks[depth - 1 - j].size);
		}
		attroff(COLOR_PAIR(1));

		attron(COLOR_PAIR(2));
		for (j = 0; j < depth; j++) {
			mvprintw(row++, col, "%6"PRIi64" %6"PRIi64"\n",
					book->ob.bids[j].price,
						book->ob.bids[j].size);
		}
		attroff(COLOR_PAIR(2));
	}

	refresh();

	return;
}

static int parse_feeds(xmlNodePtr node, struct fast_book_set *set, const char *template)
{
	struct fast_feed *feed;
	xmlNodePtr ptr;
	xmlChar *prop;

	node = node->xmlChildrenNode;
	while (node != NULL) {
		if (node->type != XML_ELEMENT_NODE) {
			node = node->next;
			continue;
		}

		if (xmlStrcmp(node->name, (const xmlChar *)"feed"))
			goto fail;

		prop = xmlGetProp(node, (const xmlChar *)"type");
		if (!prop)
			goto fail;

		if (!xmlStrcmp(prop, (const xmlChar *)"increment")) {
			feed = inc_feed_add(set);
		} else if (!xmlStrcmp(prop, (const xmlChar *)"snapshot")) {
			feed = snp_feed_add(set);
		} else {
			feed = NULL;
		}

		if (!feed) {
			fprintf(stderr, "Cannot add a feed\n");
			goto fail;
		}

		strncpy(feed->xml, template, sizeof(feed->xml));

		ptr = node->xmlChildrenNode;
		while (ptr != NULL) {
			if (ptr->type != XML_ELEMENT_NODE) {
				ptr = ptr->next;
				continue;
			}

			prop = xmlGetProp(ptr, (const xmlChar *)"value");
			if (!prop)
				goto fail;

			if (!xmlStrcmp(ptr->name, (const xmlChar *)"port")) {
				feed->port = atoi((const char *)prop);
			} else if (!xmlStrcmp(ptr->name, (const xmlChar *)"ip")) {
				strncpy(feed->ip, (const char *)prop, sizeof(feed->ip));
			} else if (!xmlStrcmp(ptr->name, (const xmlChar *)"lip")) {
				strncpy(feed->lip, (const char *)prop, sizeof(feed->lip));
			}

			ptr = ptr->next;
		}

		node = node->next;
	}

	return 0;

fail:
	return -1;
}

static int parse_books(xmlNodePtr node, struct fast_book_set *set)
{
	struct fast_book *book;
	xmlNodePtr ptr;
	xmlChar *prop;

	node = node->xmlChildrenNode;
	while (node != NULL) {
		if (node->type != XML_ELEMENT_NODE) {
			node = node->next;
			continue;
		}

		if (xmlStrcmp(node->name, (const xmlChar *)"book"))
			goto fail;

		book = fast_book_add(set);
		if (!book) {
			fprintf(stderr, "Cannot add a book\n");
			goto fail;
		}

		prop = xmlGetProp(node, (const xmlChar *)"symbol");
		if (!prop)
			goto fail;

		strncpy(book->symbol, (const char *)prop, sizeof(book->symbol));

		ptr = node->xmlChildrenNode;
		while (ptr != NULL) {
			if (ptr->type != XML_ELEMENT_NODE) {
				ptr = ptr->next;
				continue;
			}

			prop = xmlGetProp(ptr, (const xmlChar *)"value");
			if (!prop)
				goto fail;

			if (!xmlStrcmp(ptr->name, (const xmlChar *)"id"))
				book->secid = atol((const char *)prop);
			else if (!xmlStrcmp(ptr->name, (const xmlChar *)"depth"))
				book->ob.depth = atoi((const char *)prop);
			else if (!xmlStrcmp(ptr->name, (const xmlChar *)"tick_mnt"))
				book->tick.mnt = atoi((const char *)prop);
			else if (!xmlStrcmp(ptr->name, (const xmlChar *)"tick_exp"))
				book->tick.exp = atoi((const char *)prop);

			ptr = ptr->next;
		}

		node = node->next;
	}

	return 0;

fail:
	return -1;
}

static int parse_config(struct fast_book_set *set, const char *config, const char *template)
{
	xmlNodePtr node;
	xmlDocPtr doc;
	int ret = -1;

	doc = xmlParseFile(config);
	if (doc == NULL)
		goto exit;

	node = xmlDocGetRootElement(doc);
	if (node == NULL)
		goto exit;

	if (xmlStrcmp(node->name, (const xmlChar *)"config"))
		goto free;

	node = node->xmlChildrenNode;
	while (node != NULL) {
		if (node->type != XML_ELEMENT_NODE) {
			node = node->next;
			continue;
		}

		if (!xmlStrcmp(node->name, (const xmlChar *)"feeds"))
			ret  = parse_feeds(node, set, template);
		else if (!xmlStrcmp(node->name, (const xmlChar *)"books"))
			ret = parse_books(node, set);

		if (ret)
			goto free;

		node = node->next;
	}

free:
	xmlFreeDoc(doc);

exit:
	return ret;
}

static void usage(void)
{
	fprintf(stderr, "\n usage: orderbook -t, --template template -c, --config config\n");
	return;
}

int main(int argc, char *argv[])
{
	struct fast_book_set *book_set;
	const char *template = NULL;
	const char *config = NULL;
	struct fast_book *book;
	struct sigaction sa;
	int opt_index = 0;
	int opt;
	int i;

	const char *short_opt = "t:c:";
	const struct option long_opt[] = {
		{"template", required_argument, NULL, 't'},
		{"config", required_argument, NULL, 'c'},
		{NULL, 0, NULL, 0}
	};

	while ((opt = getopt_long(argc, argv, short_opt, long_opt, &opt_index)) != -1) {
		switch (opt) {
		case 't':
			template = optarg;
			break;
		case 'c':
			config = optarg;
			break;
		default:
			usage();
			goto fail;
		}
	}

	if (!config || !template) {
		usage();
		goto fail;
	}

	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		fprintf(stderr, "Unable to register a signal handler\n");
		goto fail;
	}

	book_set = calloc(1, sizeof(struct fast_book_set));
	if (!book_set) {
		fprintf(stderr, "Unable to allocate book set memory\n");
		goto fail;
	}

	if (parse_config(book_set, config, template))
		goto fail;

	if (fast_books_init(book_set)) {
		fprintf(stderr, "Cannot initialize a book set\n");
		goto fail;
	}

	for (i = 0; i < book_set->books_num; i++) {
		book = book_set->books + i;

		if (fast_books_subscribe(book_set, book)) {
			fprintf(stderr, "Cannot subscribe to the book %s\n", book->symbol);
			goto fail;
		}
	}

	initscr();
	cbreak();
	noecho();

	start_color();
	init_pair(1, COLOR_WHITE, COLOR_RED);
	init_pair(2, COLOR_WHITE, COLOR_GREEN);

	while (!stop) {
		if (fast_books_update(book_set)) {
			fprintf(stderr, "Books update failed\n");
			goto fail;
		}

		fast_books_print(book_set);
	}

	if (fast_books_fini(book_set)) {
		fprintf(stderr, "Books are not finalized\n");
		goto fail;
	}

	endwin();

	return 0;

fail:
	return -1;
}
