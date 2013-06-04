#include "libtrading/proto/fix_message.h"
#include "libtrading/proto/fast_book.h"

#include <stdlib.h>

static int price_align(struct fast_decimal *price, struct fast_decimal *tick)
{
	if (price->exp == tick->exp)
		return 0;

	if (price->exp > tick->exp) {
		while (price->exp > tick->exp) {
			price->mnt *= 10;
			price->exp--;
		}
	} else {
		while (price->exp < tick->exp) {
			if (price->mnt % 10)
				return -1;

			price->mnt /= 10;
			price->exp++;
		}
	}

	return 0;
}

static bool fast_book_is_valid(struct fast_book *book)
{
	if (book_has_flags(book, FAST_BOOK_EMPTY))
		return false;

	return order_book_is_valid(&book->ob);
}

static int md_increment(struct fast_book *book, struct fast_message *msg)
{
	struct price_level *levels;
	struct fast_decimal price;
	struct fast_field *field;
	struct price_level level;
	unsigned long *map;
	char type;
	i64 size;
	u32 ind;

	map = book->md_map->inc_map;

	field = map_field(msg, map, MDEntryTypeInc);
	if (!field || field_state_empty(field))
		goto fail;

	type = field->string_value[0];
	if (type == '0') {
		levels = book->ob.bids;
	} else if (type == '1') {
		levels = book->ob.asks;
	} else if (type == 'J') {
		book_add_flags(book, FAST_BOOK_EMPTY);
		goto exit;
	} else {
		goto fail;
	}

	field = map_field(msg, map, MDPriceLevelInc);
	if (!field || field_state_empty(field))
		goto fail;

	ind = field->uint_value - 1;

	field = map_field(msg, map, MDEntrySizeInc);
	if (!field || field_state_empty(field))
		goto fail;

	size = field->int_value;

	field = map_field(msg, map, MDEntryPxInc);
	if (!field || field_state_empty(field))
		goto fail;

	price = field->decimal_value;
	if (price_align(&price, &book->tick))
		goto fail;

	level.price = price.mnt;
	level.size = size;

	field = map_field(msg, map, MDUpdateActionInc);
	if (!field || field_state_empty(field))
		goto fail;

	if (field->uint_value == 0) {
		if (new_price_level(levels, book->ob.depth, &level, ind))
			goto fail;
	} else if (field->uint_value == 1) {
		if (change_price_level(levels, book->ob.depth, &level, ind))
			goto fail;
	} else if (field->uint_value == 2) {
		if (delete_price_level(levels, book->ob.depth, &level, ind))
			goto fail;
	} else {
		goto fail;
	}

exit:
	return 0;

fail:
	return -1;
}

static int md_snapshot(struct fast_book *book, struct fast_message *msg)
{
	struct price_level *levels;
	struct fast_decimal price;
	struct fast_field *field;
	struct price_level level;
	unsigned long *map;
	char type;
	i64 size;
	u32 ind;

	map = book->md_map->snp_map;

	field = map_field(msg, map, MDEntryTypeSnp);
	if (!field || field_state_empty(field))
		goto fail;

	type = field->string_value[0];
	if (type == '0') {
		levels = book->ob.bids;
	} else if (type == '1') {
		levels = book->ob.asks;
	} else if (type == 'J') {
		book_add_flags(book, FAST_BOOK_EMPTY);
		goto exit;
	} else {
		goto fail;
	}

	field = map_field(msg, map, MDPriceLevelSnp);
	if (!field || field_state_empty(field))
		goto fail;

	ind = field->uint_value - 1;

	field = map_field(msg, map, MDEntrySizeSnp);
	if (!field || field_state_empty(field))
		goto fail;

	size = field->int_value;

	field = map_field(msg, map, MDEntryPxSnp);
	if (!field || field_state_empty(field))
		goto fail;

	price = field->decimal_value;
	if (price_align(&price, &book->tick))
		goto fail;

	level.price = price.mnt;
	level.size = size;

	if (set_price_level(levels, book->ob.depth, &level, ind))
		goto fail;

exit:
	return 0;

fail:
	return -1;
}

static int apply_increment(struct fast_book_set *set, struct fast_book *dst, struct fast_message *msg)
{
	struct fast_sequence *seq;
	struct fast_field *field;
	struct fast_message *md;
	struct fast_book *book;
	unsigned long *map;
	int i;

	map = set->msg_map.inc_map;

	field = map_field(msg, map, MDEntriesInc);
	if (!field || field_state_empty(field))
		goto fail;

	seq = field->ptr_value;
	if (field_state_empty(&seq->length))
		goto fail;

	map = set->md_map.inc_map;

	for (i = 1; i <= seq->length.uint_value; i++) {
		md = seq->elements + i;

		field = map_field(md, map, SecurityIDInc);
		if (!field || field_state_empty(field))
			goto fail;

		book = fast_book_find(set, field->uint_value);
		if (!book || !book_has_flags(book, FAST_BOOK_ACTIVE))
			continue;

		if (dst && dst->secid != book->secid)
			continue;

		field = map_field(md, map, RptSeqInc);
		if (!field || field_state_empty(field))
			goto fail;

		book->rptseq++;
		if (book->rptseq != field->uint_value)
			goto fail;

		book_clear_flags(book, FAST_BOOK_EMPTY);
		book_add_mask(set, book);

		if (md_increment(book, md))
			goto fail;
	}

	return 0;

fail:
	return -1;
}

static int apply_snapshot(struct fast_book_set *set, struct fast_book *dst, struct fast_message *msg)
{
	struct fast_sequence *seq;
	struct fast_field *field;
	struct fast_message *md;
	struct fast_book *book;
	unsigned long *map;
	int i;

	map = set->msg_map.snp_map;

	field = map_field(msg, map, SecurityIDSnp);
	if (!field || field_state_empty(field))
		goto fail;

	book = fast_book_find(set, field->uint_value);
	if (!book || book_has_flags(book, FAST_BOOK_ACTIVE))
		goto done;

	if (dst && dst->secid != book->secid)
		goto done;

	field = map_field(msg, map, MDEntriesSnp);
	if (!field || field_state_empty(field))
		goto fail;

	seq = field->ptr_value;
	if (field_state_empty(&seq->length))
		goto fail;

	field = map_field(msg, map, RptSeqSnp);
	if (!field || field_state_empty(field))
		goto fail;

	book->rptseq = field->uint_value;

	book_clear_flags(book, FAST_BOOK_EMPTY);
	book_add_mask(set, book);

	for (i = 1; i <= seq->length.uint_value; i++) {
		md = seq->elements + i;

		if (md_snapshot(book, md))
			goto fail;
	}

	if (!fast_book_is_valid(book)) {
		if (!book_has_flags(book, FAST_BOOK_EMPTY))
			goto fail;
	} else {
		book_add_flags(book, FAST_BOOK_ACTIVE);
	}

done:
	return 0;

fail:
	return -1;
}

static int recv_increment(struct fast_book_set *set, struct fast_feed *feed, struct fast_message **next)
{
	struct fast_message *msg;
	struct fast_field *field;
	enum fix_msg_type type;
	unsigned long *map;
	u64 msg_num;

	map = set->msg_map.inc_map;
	*next = NULL;

	if (feed->recv_num > set->inc_msg_num)
		goto done;

	msg = fast_feed_recv(feed, 0);

	if (!msg) {
		goto done;
	} else if (fast_msg_has_flags(msg, FAST_MSG_FLAGS_RESET)) {
		fast_session_reset(feed->session);
		goto done;
	}

	field = map_field(msg, map, MsgSeqNumInc);
	if (!field || field_state_empty(field))
		goto fail;

	msg_num = field->uint_value;
	feed->recv_num = msg_num;

	if (!set->inc_msg_num) {
		set->inc_msg_num = msg_num;
	} else if (set->inc_msg_num + 1 == msg_num) {
		set->inc_msg_num++;

		if (set->inc_gap_mode)
			set->inc_gap_mode = false;
	} else if (set->inc_msg_num < msg_num) {
		set->inc_gap_mode = true;

		goto done;
	} else
		goto done;

	field = map_field(msg, map, MessageTypeInc);
	if (!field || field_state_empty(field))
		goto fail;

	type = fix_msg_type_parse(field->string_value, 0x00);

	switch (type) {
	case FIX_MSG_TYPE_SESSION_STATUS:
		break;
	case FIX_MSG_TYPE_SEQUENCE_RESET:
		break;
	case FIX_MSG_TYPE_HEARTBEAT:
		break;
	case FIX_MSG_TYPE_INCREMENT_REFRESH:
		*next = msg;

		break;
	default:
		goto fail;
	}

done:
	return 0;

fail:
	return -1;
}

static int recv_snapshot(struct fast_book_set *set, struct fast_feed *feed, struct fast_message **next)
{
	struct fast_message *msg;
	struct fast_field *field;
	enum fix_msg_type type;
	unsigned long *map;

	map = set->msg_map.snp_map;
	*next = NULL;

	msg = fast_feed_recv(feed, 0);

	if (!msg) {
		goto done;
	} else if (fast_msg_has_flags(msg, FAST_MSG_FLAGS_RESET)) {
		fast_session_reset(feed->session);
		goto done;
	}

	field = map_field(msg, map, MessageTypeSnp);
	if (!field || field_state_empty(field))
		goto fail;

	type = fix_msg_type_parse(field->string_value, 0x00);

	switch (type) {
	case FIX_MSG_TYPE_SESSION_STATUS:
		break;
	case FIX_MSG_TYPE_SEQUENCE_RESET:
		break;
	case FIX_MSG_TYPE_HEARTBEAT:
		break;
	case FIX_MSG_TYPE_SNAPSHOT_REFRESH:
		*next = msg;

		break;
	default:
		goto fail;
	}

done:
	return 0;

fail:
	return -1;
}

static int next_increment(struct fast_book_set *set, struct fast_message **next)
{
	struct fast_message *msg;
	struct fast_feed *feed;
	u64 min_msg_num;
	int i;

	*next = NULL;

	for (i = 0; i < set->inc_feeds_num; i++) {
		feed = set->inc_feeds + i;

		if (recv_increment(set, feed, &msg))
			goto fail;

		if (!msg || *next)
			continue;

		*next = msg;
	}

	if (set->inc_gap_mode) {
		min_msg_num = set->inc_msg_num + 1;

		for (i = 0; i < set->inc_feeds_num; i++) {
			feed = set->inc_feeds + i;

			if (feed->recv_num < min_msg_num)
				min_msg_num = feed->recv_num;
		}

		if (min_msg_num > set->inc_msg_num)
			goto fail;
	}

	return 0;

fail:
	return -1;
}

static int next_snapshot(struct fast_book_set *set, struct fast_message **next)
{
	struct fast_message *msg;
	struct fast_feed *feed;
	int i;

	*next = NULL;

	for (i = 0; i < set->snp_feeds_num; i++) {
		feed = set->snp_feeds + i;

		if (recv_snapshot(set, feed, &msg))
			goto fail;

		if (!msg || *next)
			continue;

		*next = msg;
	}

	return 0;

fail:
	return -1;
}

static int fast_books_join(struct fast_book_set *set, struct fast_book *book)
{
	struct fast_message *inc_buf = NULL;
	struct fast_message *tmp_buf;
	struct fast_field *field;
	struct fast_message *msg;
	unsigned long size = 32;
	unsigned long pos = 0;
	u64 last_msg_seq_num;
	u64 msg_seq_num = 0;
	unsigned long *map;
	unsigned long i;

	if (fast_feed_open(set->snp_feeds))
		goto fail;

	inc_buf = calloc(size, sizeof(struct fast_message));
	if (!inc_buf)
		goto fail;

	book_clear_flags(book, FAST_BOOK_ACTIVE);

	while (!book_has_flags(book, FAST_BOOK_ACTIVE)) {
		map = set->msg_map.inc_map;

		if (next_increment(set, &msg))
			goto fail;

		if (!msg)
			continue;

		if (apply_increment(set, NULL, msg))
			goto fail;

		if (pos >= size) {
			size *= 2;

			tmp_buf = realloc(inc_buf, size * sizeof(struct fast_message));
			if (!tmp_buf)
				goto fail;

			inc_buf = tmp_buf;
		}

		if (fast_message_copy(inc_buf + pos, msg)) {
			goto fail;
		} else
			pos++;

		if (!msg_seq_num) {
			field = map_field(msg, map, MsgSeqNumInc);

			if (!field || field_state_empty(field))
				goto fail;

			msg_seq_num = field->uint_value;
		}

		map = set->msg_map.snp_map;

		if (next_snapshot(set, &msg))
			goto fail;

		if (!msg)
			continue;

		field = map_field(msg, map, LastMsgSeqNumProcessedSnp);
		if (!field || field_state_empty(field))
			goto fail;

		last_msg_seq_num = field->uint_value;

		if (last_msg_seq_num < msg_seq_num)
			continue;

		if (apply_snapshot(set, book, msg))
			goto fail;
	}

	if (!pos)
		goto fail;

	map = set->msg_map.inc_map;

	for (i = 0; i < pos; i++) {
		msg = inc_buf + i;

		field = map_field(msg, map, MsgSeqNumInc);
		if (!field || field_state_empty(field))
			goto fail;

		msg_seq_num = field->uint_value;

		if (msg_seq_num <= last_msg_seq_num)
			continue;

		if (apply_increment(set, book, msg))
			goto fail;
	}

	if (fast_feed_close(set->snp_feeds))
		goto fail;

	fast_message_free(inc_buf, pos);

	return 0;

fail:
	fast_feed_close(set->snp_feeds);
	fast_message_free(inc_buf, pos);

	return -1;
}

static int fast_books_recover(struct fast_book_set *set)
{
	struct fast_book *book;
	struct fast_feed *feed;
	int i;

retry:
	set->inc_gap_mode = false;
	set->inc_msg_num = 0;

	for (i = 0; i < set->inc_feeds_num; i++) {
		feed = set->inc_feeds + i;
		feed->recv_num = 0;
	}

	for (i = 0; i < set->books_num; i++) {
		book = set->books + i;

		book_clear_flags(book, FAST_BOOK_ACTIVE);
	}

	for (i = 0; i < set->books_num; i++) {
		book = set->books + i;

		if (!book_has_flags(book, FAST_BOOK_SUBSCRIBED))
			continue;

		if (fast_books_join(set, book))
			goto retry;
	}

	return 0;
}

int fast_books_subscribe(struct fast_book_set *set, struct fast_book *book)
{
	if (fast_books_join(set, book))
		goto fail;

	book_add_flags(book, FAST_BOOK_SUBSCRIBED);

	return 0;

fail:
	return -1;
}

int fast_books_update(struct fast_book_set *set)
{
	struct fast_message *msg;

	memset(set->books_mask, 0, sizeof(set->books_mask));

	if (next_increment(set, &msg)) {
		if (fast_books_recover(set))
			goto fail;

		goto done;
	}

	if (!msg)
		goto done;

	if (apply_increment(set, NULL, msg))
		goto fail;

done:
	return 0;

fail:
	return -1;
}

static inline struct fast_message *fast_msg_by_type(struct fast_session *session, enum fix_msg_type type)
{
	struct fast_message *msg;
	struct fast_field *field;
	int i;

	for (i = 0; i < FAST_TEMPLATE_MAX_NUMBER; i++) {
		msg = session->rx_messages + i;

		field = fast_get_field(msg, MsgType);
		if (!field)
			continue;

		if (fix_msg_type_parse(field->string_value, 0x00) == type)
			return msg;
	}

	return NULL;
}

static int map_init_increment(struct fast_book_set *set, struct fast_message *msg)
{
	struct fast_sequence *seq;
	struct fast_field *field;
	struct fast_message *md;
	unsigned long *msg_map;
	unsigned long *md_map;
	int msg_init_num = 0;
	int md_init_num = 0;
	unsigned long i, j;
	int ret = -1;

	msg_map = set->msg_map.inc_map;
	md_map = set->md_map.inc_map;

	for (i = 0; i < msg->nr_fields; i++) {
		field = msg->fields + i;

		if (!strncmp(field->name, "MessageType", 12)) {
			if (field->id != MsgType)
				goto done;

			msg_map[MessageTypeInc] = i;
			msg_init_num++;
		} else if (!strncmp(field->name, "MsgSeqNum", 10)) {
			if (field->id != MsgSeqNum)
				goto done;

			msg_map[MsgSeqNumInc] = i;
			msg_init_num++;
		} else if (!strncmp(field->name, "MDEntries", 10)) {
			if (field->type != FAST_TYPE_SEQUENCE)
				goto done;

			msg_map[MDEntriesInc] = i;
			msg_init_num++;

			seq = field->ptr_value;

			md = seq->elements;
			for (j = 0; j < md->nr_fields; j++) {
				field = md->fields + j;

				if (!strncmp(field->name, "MDUpdateAction", 15)) {
					if (field->id != MDUpdateAction)
						goto done;

					md_map[MDUpdateActionInc] = j;
					md_init_num++;
				} else if (!strncmp(field->name, "MDEntryType", 12)) {
					if (field->id != MDEntryType)
						goto done;

					md_map[MDEntryTypeInc] = j;
					md_init_num++;
				} else if (!strncmp(field->name, "SecurityID", 11)) {
					if (field->id != SecurityID)
						goto done;

					md_map[SecurityIDInc] = j;
					md_init_num++;
				} else if (!strncmp(field->name, "RptSeq", 7)) {
					if (field->id != RptSeq)
						goto done;

					md_map[RptSeqInc] = j;
					md_init_num++;
				} else if (!strncmp(field->name, "MDPriceLevel", 13)) {
					if (field->id != MDPriceLevel)
						goto done;

					md_map[MDPriceLevelInc] = j;
					md_init_num++;
				} else if (!strncmp(field->name, "MDEntryPx", 10)) {
					if (field->id != MDEntryPx)
						goto done;

					md_map[MDEntryPxInc] = j;
					md_init_num++;
				} else if (!strncmp(field->name, "MDEntrySize", 12)) {
					if (field->id != MDEntrySize)
						goto done;

					md_map[MDEntrySizeInc] = j;
					md_init_num++;
				}
			}

			break;
		}
	}

	if (msg_init_num < MsgMapMaxInc)
		goto done;

	if (md_init_num < MDMapMaxInc)
		goto done;

	ret = 0;

done:
	return ret;
}

static int map_init_snapshot(struct fast_book_set *set, struct fast_message *msg)
{
	struct fast_sequence *seq;
	struct fast_field *field;
	struct fast_message *md;
	unsigned long *msg_map;
	unsigned long *md_map;
	int msg_init_num = 0;
	int md_init_num = 0;
	unsigned long i, j;
	int ret = -1;

	msg_map = set->msg_map.snp_map;
	md_map = set->md_map.snp_map;

	for (i = 0; i < msg->nr_fields; i++) {
		field = msg->fields + i;

		if (!strncmp(field->name, "MessageType", 12)) {
			if (field->id != MsgType)
				goto done;

			msg_map[MessageTypeSnp] = i;
			msg_init_num++;
		} else if (!strncmp(field->name, "MsgSeqNum", 10)) {
			if (field->id != MsgSeqNum)
				goto done;

			msg_map[MsgSeqNumSnp] = i;
			msg_init_num++;
		} else if (!strncmp(field->name, "RptSeq", 7)) {
			if (field->id != RptSeq)
				goto done;

			msg_map[RptSeqSnp] = i;
			msg_init_num++;
		} else if (!strncmp(field->name, "LastMsgSeqNumProcessed", 23)) {
			if (field->id != LastMsgSeqNumProcessed)
				goto done;

			msg_map[LastMsgSeqNumProcessedSnp] = i;
			msg_init_num++;
		} else if (!strncmp(field->name, "SecurityID", 11)) {
			if (field->id != SecurityID)
				goto done;

			msg_map[SecurityIDSnp] = i;
			msg_init_num++;
		} else if (!strncmp(field->name, "MDEntries", 10)) {
			if (field->type != FAST_TYPE_SEQUENCE)
				goto done;

			msg_map[MDEntriesSnp] = i;
			msg_init_num++;

			seq = field->ptr_value;

			md = seq->elements;
			for (j = 0; j < md->nr_fields; j++) {
				field = md->fields + j;

				if (!strncmp(field->name, "MDEntryType", 12)) {
					if (field->id != MDEntryType)
						goto done;

					md_map[MDEntryTypeSnp] = j;
					md_init_num++;
				} else if (!strncmp(field->name, "MDEntryPx", 10)) {
					if (field->id != MDEntryPx)
						goto done;

					md_map[MDEntryPxSnp] = j;
					md_init_num++;
				} else if (!strncmp(field->name, "MDEntrySize", 12)) {
					if (field->id != MDEntrySize)
						goto done;

					md_map[MDEntrySizeSnp] = j;
					md_init_num++;
				} else if (!strncmp(field->name, "MDPriceLevel", 13)) {
					if (field->id != MDPriceLevel)
						goto done;

					md_map[MDPriceLevelSnp] = j;
					md_init_num++;
				}
			}

			break;
		}
	}

	if (msg_init_num < MsgMapMaxSnp)
		goto done;

	if (md_init_num < MDMapMaxSnp)
		goto done;

	ret = 0;

done:
	return ret;
}

int fast_books_init(struct fast_book_set *set)
{
	struct fast_message *msg;
	int i;

	if (!set)
		goto fail;

	if (!set->inc_feeds_num)
		goto fail;

	if (!set->snp_feeds_num)
		goto fail;

	for (i = 0; i < set->inc_feeds_num; i++) {
		if (fast_feed_open(set->inc_feeds + i))
			goto fail;
	}

	if (fast_feed_open(set->snp_feeds))
		goto fail;

	msg = fast_msg_by_type(set->inc_feeds->session, FIX_MSG_TYPE_INCREMENT_REFRESH);
	if (!msg)
		goto fail;

	if (map_init_increment(set, msg))
		goto fail;

	msg = fast_msg_by_type(set->inc_feeds->session, FIX_MSG_TYPE_SNAPSHOT_REFRESH);
	if (!msg)
		goto fail;

	if (map_init_snapshot(set, msg))
		goto fail;

	if (fast_feed_close(set->snp_feeds))
		goto fail;

	set->inc_gap_mode = false;
	set->inc_msg_num = 0;

	return 0;

fail:
	fast_books_fini(set);

	return -1;
}

int fast_books_fini(struct fast_book_set *set)
{
	int i;

	for (i = 0; i < set->inc_feeds_num; i++) {
		if (fast_feed_close(set->inc_feeds + i))
			goto fail;
	}

	for (i = 0; i < set->snp_feeds_num; i++) {
		if (fast_feed_close(set->snp_feeds + i))
			goto fail;
	}

	return 0;

fail:
	return -1;
}
