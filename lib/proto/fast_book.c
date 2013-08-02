#include "libtrading/proto/fix_message.h"
#include "libtrading/proto/fast_book.h"

#include <stdlib.h>

static int decimal_to_int(struct fast_decimal *decimal, i64 *out)
{
	i64 exp = decimal->exp;

	if (exp < 0)
		return -1;

	*out = decimal->mnt;
	while (exp--)
		*out *= 10;

	return 0;
}

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

static int md_increment(struct fast_book *book, struct fast_message *msg)
{
	struct fast_decimal price;
	struct fast_field *field;
	struct ob_order order;
	char type;
	i64 size;

	field = fast_get_field(msg, "MDEntryType");
	if (!field || field_state_empty(field))
		goto fail;

	type = field->string_value[0];
	if (type == '0') {
		order.buy = true;
	} else if (type == '1') {
		order.buy = false;
	} else if (type == 'J') {
		book_add_flags(book, FAST_BOOK_EMPTY);
		goto exit;
	} else {
		goto fail;
	}

	field = fast_get_field(msg, "MDEntrySize");
	if (!field || field_state_empty(field))
		goto fail;

	switch (field->type) {
	case FAST_TYPE_INT:
		size = field->int_value;
		break;
	case FAST_TYPE_DECIMAL:
		if (decimal_to_int(&field->decimal_value, &size))
			goto fail;
		break;
	case FAST_TYPE_UINT:
	case FAST_TYPE_STRING:
	case FAST_TYPE_VECTOR:
	case FAST_TYPE_SEQUENCE:
	default:
		goto fail;
	}

	field = fast_get_field(msg, "MDEntryPx");
	if (!field || field_state_empty(field))
		goto fail;

	price = field->decimal_value;
	if (price_align(&price, &book->tick))
		goto fail;

	order.price = price.mnt;
	order.size = size;

	field = fast_get_field(msg, "MDUpdateAction");
	if (!field || field_state_empty(field))
		goto fail;

	if (field->uint_value == 0) {
		if (ob_level_modify(&book->ob, &order))
			goto fail;
	} else if (field->uint_value == 1) {
		if (ob_level_modify(&book->ob, &order))
			goto fail;
	} else if (field->uint_value == 2) {
		if (ob_level_delete(&book->ob, &order))
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
	struct fast_decimal price;
	struct fast_field *field;
	struct ob_order order;
	char type;
	i64 size;

	field = fast_get_field(msg, "MDEntryType");
	if (!field || field_state_empty(field))
		goto fail;

	type = field->string_value[0];
	if (type == '0') {
		order.buy = true;
	} else if (type == '1') {
		order.buy = false;
	} else if (type == 'J') {
		book_add_flags(book, FAST_BOOK_EMPTY);
		goto exit;
	} else {
		goto fail;
	}

	field = fast_get_field(msg, "MDEntrySize");
	if (!field || field_state_empty(field))
		goto fail;

	switch (field->type) {
	case FAST_TYPE_INT:
		size = field->int_value;
		break;
	case FAST_TYPE_DECIMAL:
		if (decimal_to_int(&field->decimal_value, &size))
			goto fail;
		break;
	case FAST_TYPE_UINT:
	case FAST_TYPE_STRING:
	case FAST_TYPE_VECTOR:
	case FAST_TYPE_SEQUENCE:
	default:
		goto fail;
	}

	field = fast_get_field(msg, "MDEntryPx");
	if (!field || field_state_empty(field))
		goto fail;

	price = field->decimal_value;
	if (price_align(&price, &book->tick))
		goto fail;

	order.price = price.mnt;
	order.size = size;

	if (ob_level_modify(&book->ob, &order))
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
	int i;

	field = fast_get_field(msg, "MDEntries");
	if (!field) {
		field = fast_get_field(msg, "GroupMDEntries");
	}

	if (!field || field_state_empty(field))
		goto fail;

	seq = field->ptr_value;
	if (field_state_empty(&seq->length))
		goto fail;

	for (i = 1; i <= seq->length.uint_value; i++) {
		md = seq->elements + i;

		field = fast_get_field(md, "SecurityID");
		if (field) {
			if (field_state_empty(field))
				goto fail;

			book = fast_book_by_id(set, field->uint_value);
			if (!book || !book_has_flags(book, FAST_BOOK_ACTIVE))
				continue;

			if (dst && dst->secid != book->secid)
				continue;
		} else {
			field = fast_get_field(md, "Symbol");
			if (!field || field_state_empty(field))
				goto fail;

			book = fast_book_by_symbol(set, field->string_value);
			if (!book || !book_has_flags(book, FAST_BOOK_ACTIVE))
				continue;

			if (dst && strncmp(book->symbol, dst->symbol,
							strlen(dst->symbol)))
				continue;
		}

		field = fast_get_field(md, "TradingSessionID");
		if (field) {
			if (field_state_empty(field))
				goto fail;

			if (strncmp(book->session, field->string_value,
						strlen(book->session)))
				continue;
		}

		field = fast_get_field(md, "RptSeq");
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
	int i;

	field = fast_get_field(msg, "SecurityID");
	if (field) {
		if (field_state_empty(field))
			goto fail;

		book = fast_book_by_id(set, field->uint_value);
		if (!book || book_has_flags(book, FAST_BOOK_ACTIVE))
			goto done;

		if (dst && dst->secid != book->secid)
			goto done;
	} else {
		field = fast_get_field(msg, "Symbol");
		if (!field || field_state_empty(field))
			goto fail;

		book = fast_book_by_symbol(set, field->string_value);
		if (!book || book_has_flags(book, FAST_BOOK_ACTIVE))
			goto done;

		if (dst && strncmp(book->symbol, dst->symbol,
						strlen(dst->symbol)))
			goto done;
	}

	field = fast_get_field(msg, "MDEntries");
	if (!field) {
		field = fast_get_field(msg, "GroupMDEntries");
	}

	if (!field || field_state_empty(field))
		goto fail;

	seq = field->ptr_value;
	if (field_state_empty(&seq->length))
		goto fail;

	if (!seq->length.uint_value)
		goto done;

	md = seq->elements;

	field = fast_get_field(md, "TradingSessionID");
	if (field) {
		if (field_state_empty(field))
			goto fail;

		if (strncmp(book->session, field->string_value,
					strlen(book->session)))
			goto done;
	}

	field = fast_get_field(msg, "RptSeq");
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

	if (!book_has_flags(book, FAST_BOOK_EMPTY))
		book_add_flags(book, FAST_BOOK_ACTIVE);

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
	u64 msg_num;

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

	field = fast_get_field(msg, "MsgSeqNum");
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

	field = fast_get_field(msg, "MessageType");
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

	*next = NULL;

	msg = fast_feed_recv(feed, 0);

	if (!msg) {
		goto done;
	} else if (fast_msg_has_flags(msg, FAST_MSG_FLAGS_RESET)) {
		fast_session_reset(feed->session);
		goto done;
	}

	field = fast_get_field(msg, "MessageType");
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
	unsigned long i;

	if (fast_feed_open(set->snp_feeds))
		goto fail;

	inc_buf = calloc(size, sizeof(struct fast_message));
	if (!inc_buf)
		goto fail;

	book_clear_flags(book, FAST_BOOK_ACTIVE);

	while (!book_has_flags(book, FAST_BOOK_ACTIVE)) {
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
			field = fast_get_field(msg, "MsgSeqNum");

			if (!field || field_state_empty(field))
				goto fail;

			msg_seq_num = field->uint_value;
		}

		if (next_snapshot(set, &msg))
			goto fail;

		if (!msg)
			continue;

		field = fast_get_field(msg, "LastMsgSeqNumProcessed");
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

	for (i = 0; i < pos; i++) {
		msg = inc_buf + i;

		field = fast_get_field(msg, "MsgSeqNum");
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

int fast_books_init(struct fast_book_set *set)
{
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
