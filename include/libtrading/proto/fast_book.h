#ifndef	LIBTRADING_FAST_BOOK_H
#define	LIBTRADING_FAST_BOOK_H

#include "libtrading/proto/fast_message.h"
#include "libtrading/proto/fast_feed.h"
#include "libtrading/order_book.h"

#define	FAST_FEED_NUM		2
#define	FAST_BOOK_NUM		10

#define	FAST_BOOK_SUBSCRIBED	0x00000001
#define	FAST_BOOK_ACTIVE	0x00000002
#define	FAST_BOOK_EMPTY		0x00000004

#define	FAST_BOOK_MASK_SIZE	(1 + ((FAST_BOOK_NUM) >> 6))

enum increment_msg_map {
	MessageTypeInc			= 0,
	MsgSeqNumInc			= 1,
	MDEntriesInc			= 2,
	MsgMapMaxInc,
};

enum snapshot_msg_map {
	MessageTypeSnp			= 0,
	MsgSeqNumSnp			= 1,
	RptSeqSnp			= 2,
	LastMsgSeqNumProcessedSnp	= 3,
	SecurityIDSnp			= 4,
	MDEntriesSnp			= 5,
	MsgMapMaxSnp,
};

enum increment_md_map {
	MDUpdateActionInc		= 0,
	MDEntryTypeInc			= 1,
	SecurityIDInc			= 2,
	RptSeqInc			= 3,
	MDPriceLevelInc			= 4,
	MDEntryPxInc			= 5,
	MDEntrySizeInc			= 6,
	MDMapMaxInc,
};

enum snapshot_md_map {
	MDEntryTypeSnp			= 0,
	MDEntryPxSnp			= 1,
	MDEntrySizeSnp			= 2,
	MDPriceLevelSnp			= 3,
	MDMapMaxSnp,
};

struct msg_map {
	unsigned long inc_map[MsgMapMaxInc];
	unsigned long snp_map[MsgMapMaxSnp];
};

struct md_map {
	unsigned long inc_map[MDMapMaxInc];
	unsigned long snp_map[MDMapMaxSnp];
};

struct fast_book {
	struct fast_decimal	tick;
	struct order_book	ob;

	struct msg_map	*msg_map;
	struct md_map	*md_map;

	char		symbol[8];
	u64		rptseq;
	int		flags;
	u64		secid;
	int		num;
};

static inline void book_set_flags(struct fast_book *book, int flags)
{
	book->flags = flags;
}

static inline void book_add_flags(struct fast_book *book, int flags)
{
	book->flags |= flags;
}

static inline void book_clear_flags(struct fast_book *book, int flags)
{
	book->flags &= ~flags;
}

static inline int book_has_flags(struct fast_book *book, int flags)
{
	return book->flags & flags;
}

struct fast_book_set {
	struct fast_feed	inc_feeds[FAST_FEED_NUM];
	struct fast_feed	snp_feeds[FAST_FEED_NUM];

	unsigned long		inc_feeds_num;
	unsigned long		snp_feeds_num;

	u64			books_mask[FAST_BOOK_MASK_SIZE];
	struct fast_book	books[FAST_BOOK_NUM];
	unsigned long		books_num;

	struct msg_map		msg_map;
	struct md_map		md_map;

	bool			inc_gap_mode;
	u64			inc_msg_num;
};

static inline void book_add_mask(struct fast_book_set *set, struct fast_book *book)
{
	set->books_mask[book->num / 64] |= (1 << book->num % 64);
}

static inline void book_clear_mask(struct fast_book_set *set, struct fast_book *book)
{
	set->books_mask[book->num / 64] &= ~(1 << book->num % 64);
}

static inline u64 book_has_mask(struct fast_book_set *set, struct fast_book *book)
{
	return set->books_mask[book->num / 64] & (1 << book->num % 64);
}

static inline struct fast_book *fast_book_add(struct fast_book_set *set)
{
	struct fast_book *book;

	if (set->books_num >= FAST_BOOK_NUM)
		goto fail;

	book = set->books + set->books_num;

	memset(book, 0, sizeof(struct fast_book));
	book->msg_map = &set->msg_map;
	book->md_map = &set->md_map;
	book->num = set->books_num;
	set->books_num++;

	return book;

fail:
	return NULL;
}

static inline struct fast_book *fast_book_find(struct fast_book_set *set, u64 secid)
{
	int i;

	for (i = 0; i < set->books_num; i++) {
		if (set->books[i].secid == secid)
			return set->books + i;
	}

	return NULL;
}

static inline struct fast_feed *inc_feed_add(struct fast_book_set *set)
{
	struct fast_feed *feed;

	if (set->inc_feeds_num >= FAST_FEED_NUM)
		goto fail;

	feed = set->inc_feeds + set->inc_feeds_num;

	memset(feed, 0, sizeof(struct fast_feed));
	set->inc_feeds_num++;

	return feed;

fail:
	return NULL;
}

static inline struct fast_feed *snp_feed_add(struct fast_book_set *set)
{
	struct fast_feed *feed;

	if (set->snp_feeds_num >= FAST_FEED_NUM)
		goto fail;

	feed = set->snp_feeds + set->snp_feeds_num;

	memset(feed, 0, sizeof(struct fast_feed));
	set->snp_feeds_num++;

	return feed;

fail:
	return NULL;
}

static inline struct fast_field *map_field(struct fast_message *msg, unsigned long *map, int ind)
{
	unsigned long i = map[ind];

	if (i < msg->nr_fields)
		return msg->fields + i;
	else
		return NULL;
}

int fast_books_subscribe(struct fast_book_set *set, struct fast_book *book);
int fast_books_update(struct fast_book_set *set);
int fast_books_init(struct fast_book_set *set);
int fast_books_fini(struct fast_book_set *set);

#endif	/* LIBTRADING_FAST_BOOK_H */
