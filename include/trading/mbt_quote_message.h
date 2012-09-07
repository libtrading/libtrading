#ifndef LIBTRADING_MBT_QUOTE_MESSAGE_H
#define LIBTRADING_MBT_QUOTE_MESSAGE_H

struct buffer;

/*
 * Message types:
 */
enum {
	MBT_QUOTE_LOGGING_ON			= 'L',
	MBT_QUOTE_SUBSCRIPTION			= 'S',
	MBT_QUOTE_UNSUBSCRIPTION		= 'U',
	MBT_QUOTE_FUNDAMENTAL			= 'H',
	MBT_QUOTE_PING_HEARTBEAT		= '9',
	MBT_QUOTE_LOGON_ACCEPTED		= 'G',
	MBT_QUOTE_LOGON_DENIED			= 'D',
	MBT_QUOTE_LEVEL_1_UPDATE		= '1',
	MBT_QUOTE_LEVEL_2_UPDATE		= '2',
	MBT_QUOTE_TIME_AND_SALES_UPDATE		= '3',
	MBT_QUOTE_FUNDAMENTAL_DATA_RESPONSE	= 'N',
	MBT_QUOTE_OPTIONS_CHAINS_UPDATE		= '4',
};

enum {
	MBT_QUOTE_USER_NAME			= 100,
	MBT_QUOTE_PASSWORD			= 101,
};

struct mbt_quote_message {
	char			Type;
};

struct mbt_quote_logging_on {
	struct mbt_quote_message	msg;
	char			*UserName;
	char			*Password;
};

void mbt_quote_message_delete(struct mbt_quote_message *msg);

struct mbt_quote_message *mbt_quote_message_decode(struct buffer *buf);

static inline void *mbt_quote_message_payload(struct mbt_quote_message *msg)
{
	return (void *) msg + sizeof(struct mbt_quote_message);
}

#endif
