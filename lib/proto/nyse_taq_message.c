#include "libtrading/proto/nyse_taq_message.h"

#include "libtrading/buffer.h"

static void *decode(struct buffer *buf, size_t size)
{
	void *msg;

	if (buffer_size(buf) < size)
		return NULL;

	msg = buffer_start(buf);

	buffer_advance(buf, size);

	return msg;
}

struct nyse_taq_msg_daily_quote *nyse_taq_msg_daily_quote_decode(struct buffer *buf)
{
	return decode(buf, sizeof(struct nyse_taq_msg_daily_quote));
}

struct nyse_taq_msg_daily_trade *nyse_taq_msg_daily_trade_decode(struct buffer *buf)
{
	return decode(buf, sizeof(struct nyse_taq_msg_daily_trade));
}

struct nyse_taq_msg_daily_nbbo *nyse_taq_msg_daily_nbbo_decode(struct buffer *buf)
{
	return decode(buf, sizeof(struct nyse_taq_msg_daily_nbbo));
}
