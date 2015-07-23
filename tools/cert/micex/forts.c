#include "libtrading/proto/fix_message.h"
#include "libtrading/proto/fix_session.h"
#include "libtrading/proto/fast_book.h"

#include "libtrading/compat.h"
#include "libtrading/array.h"
#include "libtrading/die.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <inttypes.h>
#include <libgen.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <float.h>
#include <netdb.h>
#include <stdio.h>

static const char *program;
static sig_atomic_t stop;

static void signal_handler(int signum)
{
	if (signum == SIGINT)
		stop = 1;
}

static void usage(void)
{
	fprintf(stdout, "\n usage: %s -m mode -x template -c config -s sender-comp-id -t target-comp-id -h hostname -p port\n\n", program);

	exit(EXIT_FAILURE);
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

			if (!xmlStrcmp(ptr->name, (const xmlChar *)"port")) {
				feed->port = atoi((const char *)prop);
			} else if (!xmlStrcmp(ptr->name, (const xmlChar *)"ip")) {
				strncpy(feed->ip, (const char *)prop, sizeof(feed->ip));
			} else if (!xmlStrcmp(ptr->name, (const xmlChar *)"lip")) {
				strncpy(feed->lip, (const char *)prop, sizeof(feed->lip));
			} else if (!xmlStrcmp(ptr->name, (const xmlChar *)"sip")) {
				strncpy(feed->sip, (const char *)prop, sizeof(feed->sip));
			} else if (!xmlStrcmp(ptr->name, (const xmlChar *)"file")) {
				strncpy(feed->file, (const char *)prop, sizeof(feed->file));
			} else if (!xmlStrcmp(ptr->name, (const xmlChar *)"reset")) {
				feed->cfg.reset = true;
			} else if (!xmlStrcmp(ptr->name, (const xmlChar *)"preamble")) {
				feed->cfg.preamble_bytes = atoi((const char *)prop);
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
			else if (!xmlStrcmp(ptr->name, (const xmlChar *)"tick_mnt"))
				book->tick.mnt = atoi((const char *)prop);
			else if (!xmlStrcmp(ptr->name, (const xmlChar *)"tick_exp"))
				book->tick.exp = atoi((const char *)prop);
			else if (!xmlStrcmp(ptr->name, (const xmlChar *)"session"))
				strncpy(book->session, (const char *)prop, sizeof(book->session));

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

static struct fast_book_set *market_data_init(const char *config, const char *template)
{
	struct fast_book_set *book_set = NULL;
	struct fast_book *book;
	int i;

	book_set = calloc(1, sizeof(struct fast_book_set));
	if (!book_set)
		goto fail;

	if (parse_config(book_set, config, template))
		goto fail;

	if (fast_books_init(book_set))
		goto fail;

	for (i = 0; i < book_set->books_num; i++) {
		book = book_set->books + i;

		if (fast_books_subscribe(book_set, book))
			goto fail;
	}

	return book_set;

fail:
	free(book_set);
	return NULL;
}

static int market_data_fini(struct fast_book_set *book_set)
{
	int ret;

	ret = fast_books_fini(book_set);
	free(book_set);

	return ret;
}

static int do_limit(struct fix_session *session, struct fast_book_set *set)
{
	struct fix_field fields[12];
	struct fast_book *book;
	struct ob_level *level;
	unsigned long nr = 0;
	GList *list;

	if (set->books_num < 1)
		goto fail;

	book = set->books + 0;

	list = g_list_last(book->ob.glbids);
	level = g_list_nth_data(list, 0);

	fields[nr++] = FIX_STRING_FIELD(TransactTime, session->str_now);
	fields[nr++] = FIX_STRING_FIELD(ClOrdID, session->str_now);
	fields[nr++] = FIX_STRING_FIELD(Symbol, book->symbol);
	fields[nr++] = FIX_FLOAT_FIELD(Price, level->price);
	fields[nr++] = FIX_STRING_FIELD(Account, "B16");
	fields[nr++] = FIX_STRING_FIELD(OrdType, "2");
	fields[nr++] = FIX_FLOAT_FIELD(OrderQty, 5);
	fields[nr++] = FIX_STRING_FIELD(Side, "1");

	fix_session_new_order_single(session, fields, nr);

	return 0;

fail:
	return -1;
}

static int do_exec(struct fix_message *msg)
{
	struct fix_field *field;
	char exec_type = 0;
	double order_qty;
	char status = 0;
	double cum_qty;

	field = fix_field_by_tag(msg, OrdStatus);
	if (!field)
		goto fail;
	status = field->string_value[0];

	field = fix_field_by_tag(msg, ExecType);
	if (!field)
		goto fail;
	exec_type = field->string_value[0];

	field = fix_field_by_tag(msg, OrderQty);
	if (!field)
		goto fail;
	order_qty = field->float_value;

	field = fix_field_by_tag(msg, CumQty);
	if (!field)
		goto fail;
	cum_qty = field->float_value;

	switch (status) {
	case '0':
		fprintf(stdout, "New order placed\n");
		break;
	case '1':
		fprintf(stdout, "Partially filled %.1lf/%.1lf\n",
							cum_qty, order_qty);
		break;
	case '2':
		fprintf(stdout, "Filled %.1lf/%.1lf\n", cum_qty, order_qty);
		break;
	case 'E':
		switch (exec_type) {
		case 'E':
			fprintf(stdout, "Pending replace\n");
			break;
		case 'F':
			fprintf(stdout, "Pending replace trade %.1lf/%.1lf\n",
							cum_qty, order_qty);
			break;
		default:
			fprintf(stdout, "Pending replace: unknown status");
			break;
		}

		break;
	default:
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static void fstrcpy(char *dest, const char *src)
{
	while (*src && *src != 0x01)
		*dest++ = *src++;

	*dest = 0;

	return;
}

static int do_replace(struct fix_session *session, struct fast_book_set *set, struct fix_message *msg)
{
	struct fix_field fields[12];
	struct fix_field *field;
	struct fast_book *book;
	struct ob_level *level;
	unsigned long nr = 0;
	char clordid[32];
	char side[32];
	GList *list;

	if (set->books_num < 1)
		goto fail;

	book = set->books + 0;

	list = g_list_first(book->ob.glasks);
	level = g_list_nth_data(list, 1);

	fields[nr++] = FIX_STRING_FIELD(TransactTime, session->str_now);
	fields[nr++] = FIX_STRING_FIELD(ClOrdID, session->str_now);

	field = fix_field_by_tag(msg, ClOrdID);
	if (!field)
		goto fail;
	fstrcpy(clordid, field->string_value);

	fields[nr++] = FIX_STRING_FIELD(OrigClOrdID, clordid);
	fields[nr++] = FIX_STRING_FIELD(Symbol, book->symbol);
	fields[nr++] = FIX_FLOAT_FIELD(Price, level->price);
	fields[nr++] = FIX_STRING_FIELD(Account, "B16");
	fields[nr++] = FIX_STRING_FIELD(OrdType, "2");
	fields[nr++] = FIX_FLOAT_FIELD(OrderQty, 3);

	field = fix_field_by_tag(msg, Side);
	if (!field)
		goto fail;
	fstrcpy(side, field->string_value);

	fields[nr++] = FIX_STRING_FIELD(Side, side);

	fix_session_order_cancel_replace(session, fields, nr);

	fprintf(stdout, "New price %lu Trying to replace...\n", level->price);

	return 0;

fail:
	return -1;
}

static int do_logic(struct fix_session_cfg *cfg, const char *config, const char *template, int mode)
{
	struct fix_session *session = NULL;
	struct fast_book_set *book_set;
	struct timespec cur, prev;
	struct fix_message *msg;
	bool replaced = false;
	bool done = false;
	int ret = -1;
	int diff;

	book_set = market_data_init(config, template);
	if (!book_set) {
		fprintf(stderr, "Unable to initialize market data\n");
		goto exit;
	}

	if (signal(SIGINT, signal_handler) == SIG_ERR) {
		fprintf(stderr, "Unable to register a signal handler\n");
		goto exit;
	}

	session	= fix_session_new(cfg);
	if (!session) {
		fprintf(stderr, "FIX session cannot be created\n");
		goto exit;
	}

	ret = fix_session_logon(session);
	if (ret) {
		fprintf(stderr, "Client Logon FAILED\n");
		goto exit;
	}

	fprintf(stdout, "Client Logon OK\n");

	clock_gettime(CLOCK_MONOTONIC, &prev);

	while (!stop && session->active) {
		if (fast_books_update(book_set))
			break;

		clock_gettime(CLOCK_MONOTONIC, &cur);
		diff = cur.tv_sec - prev.tv_sec;

		if (diff > 0.1 * session->heartbtint) {
			prev = cur;

			if (!fix_session_keepalive(session, &cur))
				break;
		}

		if (fix_session_time_update(session))
			break;

		if (fix_session_recv(session, &msg, 0) > 0) {
			if (fix_session_admin(session, msg))
				continue;

			switch (msg->type) {
			case FIX_MSG_TYPE_EXECUTION_REPORT:
				if (do_exec(msg))
					stop = 1;

				if (mode > 1 && !replaced) {
					if (do_replace(session, book_set, msg))
						stop = 1;

					replaced = true;
				}
				break;
			case FIX_MSG_ORDER_CANCEL_REJECT:
				fprintf(stdout, "Cancel reject\n");
				break;
			case FIX_MSG_TYPE_LOGOUT:
				stop = 1;
				break;
			default:
				stop = 1;
				break;
			}
		}

		if (done)
			continue;

		if (do_limit(session, book_set))
			break;

		done = true;
	}

	if (session->active) {
		ret = fix_session_logout(session, NULL);
		if (ret) {
			fprintf(stderr, "Client Logout FAILED\n");
			goto exit;
		}
	}

	fprintf(stdout, "Client Logout OK\n");

	market_data_fini(book_set);

exit:
	fix_session_free(session);

	return ret;
}

int main(int argc, char *argv[])
{
	const char *target_comp_id = NULL;
	const char *sender_comp_id = NULL;
	const char *template = NULL;
	struct fix_session_cfg cfg;
	const char *config = NULL;
	const char *host = NULL;
	struct sockaddr_in sa;
	int saved_errno = 0;
	struct hostent *he;
	int mode = 0;
	int port = 0;
	char **ap;
	int opt;
	int ret;

	program = basename(argv[0]);

	while ((opt = getopt(argc, argv, "s:t:h:p:x:c:m:")) != -1) {
		switch (opt) {
		case 's':
			sender_comp_id = optarg;
			break;
		case 't':
			target_comp_id = optarg;
			break;
		case 'm':
			mode = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'x':
			template = optarg;
			break;
		case 'c':
			config = optarg;
			break;
		case 'h':
			host = optarg;
			break;
		default: /* '?' */
			usage();
		}
	}

	if (!mode || !sender_comp_id || !target_comp_id || !host || !port || !template || !config)
		usage();

	strncpy(cfg.target_comp_id, target_comp_id, ARRAY_SIZE(cfg.target_comp_id));
	strncpy(cfg.sender_comp_id, sender_comp_id, ARRAY_SIZE(cfg.sender_comp_id));
	cfg.dialect = &fix_dialects[FIX_4_4];

	he = gethostbyname(host);
	if (!he)
		error("Unable to look up %s (%s)", host, hstrerror(h_errno));

	for (ap = he->h_addr_list; *ap; ap++) {
		cfg.sockfd = socket(he->h_addrtype, SOCK_STREAM, IPPROTO_TCP);
		if (cfg.sockfd < 0) {
			saved_errno = errno;
			continue;
		}

		sa = (struct sockaddr_in) {
			.sin_family		= he->h_addrtype,
			.sin_port		= htons(port),
		};

		memcpy(&sa.sin_addr, *ap, he->h_length);

		if (connect(cfg.sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
			saved_errno = errno;
			close(cfg.sockfd);
			cfg.sockfd = -1;
			continue;
		}

		break;
	}

	if (cfg.sockfd < 0)
		error("Unable to connect to a socket (%s)", strerror(saved_errno));

	if (socket_setopt(cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
		die("Cannot set socket option TCP_NODELAY");

	cfg.heartbtint = 10;
	ret = do_logic(&cfg, config, template, mode);

	shutdown(cfg.sockfd, SHUT_RDWR);

	if (close(cfg.sockfd) > 0)
		die("close");

	return ret;
}
