#include "libtrading/proto/fix_session.h"

#include "libtrading/compat.h"
#include "libtrading/trace.h"

#include <sys/socket.h>
#include <sys/time.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *begin_strings[] = {
	[FIXT_1_1]	= "FIXT.1.1",
	[FIX_4_4]	= "FIX.4.4",
	[FIX_4_3]	= "FIX.4.3",
	[FIX_4_2]	= "FIX.4.2",
	[FIX_4_1]	= "FIX.4.1",
	[FIX_4_0]	= "FIX.4.0",
};

void fix_session_cfg_init(struct fix_session_cfg *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
}

struct fix_session_cfg *fix_session_cfg_new(
	const char *sender_comp_id,
	const char *target_comp_id,
	int heartbtint,
	const char *dialect,
	int sockfd
) {
	struct fix_session_cfg *cfg;
	enum fix_version version;

	cfg = calloc(1, sizeof(struct fix_session_cfg));

	strncpy(cfg->sender_comp_id, sender_comp_id, sizeof(cfg->sender_comp_id));
	strncpy(cfg->target_comp_id, target_comp_id, sizeof(cfg->target_comp_id));

	cfg->heartbtint = heartbtint;

	version =
		!strcmp(dialect, "fixt-1.1") ? FIXT_1_1 :
		!strcmp(dialect, "fix-4.0")  ? FIX_4_0 :
		!strcmp(dialect, "fix-4.1")  ? FIX_4_1 :
		!strcmp(dialect, "fix-4.2")  ? FIX_4_2 :
		!strcmp(dialect, "fix-4.3")  ? FIX_4_3 : FIX_4_4;
	cfg->dialect = &fix_dialects[version];

	cfg->sockfd = sockfd;

	return cfg;
}

struct fix_session *fix_session_new(struct fix_session_cfg *cfg)
{
	struct fix_session *self = calloc(1, sizeof *self);

	if (!self)
		return NULL;

	self->dialect		= cfg->dialect;

	self->rx_buffer		= buffer_new(RECV_BUFFER_SIZE);
	if (!self->rx_buffer) {
		fix_session_free(self);
		return NULL;
	}

	self->tx_head_buffer	= buffer_new(FIX_TX_HEAD_BUFFER_SIZE);
	if (!self->tx_head_buffer) {
		fix_session_free(self);
		return NULL;
	}

	self->tx_body_buffer	= buffer_new(FIX_TX_BODY_BUFFER_SIZE);
	if (!self->tx_body_buffer) {
		fix_session_free(self);
		return NULL;
	}

	self->rx_message	= fix_message_new();
	if (!self->rx_message) {
		fix_session_free(self);
		return NULL;
	}

	if (fix_session_time_update(self)) {
		fix_session_free(self);
		return NULL;
	}

	self->rx_timestamp = self->now;
	self->tx_timestamp = self->now;

	self->begin_string	= begin_strings[cfg->dialect->version];
	self->sender_comp_id	= cfg->sender_comp_id;
	self->target_comp_id	= cfg->target_comp_id;
	self->heartbtint	= cfg->heartbtint;
	self->password		= cfg->password;
	self->sockfd		= cfg->sockfd;
	self->tr_pending	= 0;
	self->in_msg_seq_num	= cfg->in_msg_seq_num  > 0 ? cfg->in_msg_seq_num  : 0;
	self->out_msg_seq_num	= cfg->out_msg_seq_num > 1 ? cfg->out_msg_seq_num : 1;

	return self;
}

void fix_session_free(struct fix_session *self)
{
	if (!self)
		return;

	buffer_delete(self->rx_buffer);
	buffer_delete(self->tx_head_buffer);
	buffer_delete(self->tx_body_buffer);
	fix_message_free(self->rx_message);
	free(self);
}

int fix_session_time_update_monotonic(struct fix_session *self, struct timespec *monotonic)
{
	self->now = *monotonic;
	return 0;
}

int fix_session_time_update_realtime(struct fix_session *self, struct timespec *realtime)
{
	struct timeval tv;
	struct tm *tm;
	char fmt[64];

	TIMESPEC_TO_TIMEVAL(&tv, realtime);

	tm = gmtime(&tv.tv_sec);
	if (!tm)
		goto fail;

	strftime(fmt, sizeof(fmt), "%Y%m%d-%H:%M:%S", tm);

	snprintf(self->str_now, sizeof(self->str_now), "%s.%03ld", fmt, (long)tv.tv_usec / 1000);

	return 0;
fail:
	return -1;
}

int fix_session_time_update(struct fix_session *self)
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts))
		goto fail;

	if (fix_session_time_update_monotonic(self, &ts))
		goto fail;

	if (clock_gettime(CLOCK_REALTIME, &ts))
		goto fail;

	return fix_session_time_update_realtime(self, &ts);
fail:
	return -1;
}

int fix_session_send(struct fix_session *self, struct fix_message *msg, unsigned long flags)
{
	msg->begin_string	= self->begin_string;
	msg->sender_comp_id	= self->sender_comp_id;
	msg->target_comp_id	= self->target_comp_id;

	if (!(flags && FIX_SEND_FLAG_PRESERVE_MSG_NUM))
		msg->msg_seq_num	= self->out_msg_seq_num++;

	msg->head_buf = self->tx_head_buffer;
	buffer_reset(msg->head_buf);
	msg->body_buf = self->tx_body_buffer;
	buffer_reset(msg->body_buf);

	self->tx_timestamp = self->now;
	msg->str_now = self->str_now;

	return fix_message_send(msg, self->sockfd, flags);
}

static inline bool fix_session_buffer_full(struct fix_session *session)
{
	return buffer_remaining(session->rx_buffer) <= FIX_MAX_MESSAGE_SIZE;
}

static int translate_recv_flags(unsigned long flags)
{
	return flags & FIX_RECV_FLAG_MSG_DONTWAIT ? MSG_DONTWAIT : 0;
}

int fix_session_recv(struct fix_session *self, struct fix_message **res, unsigned long flags)
{
	struct fix_message *msg = self->rx_message;
	struct buffer *buffer = self->rx_buffer;

	self->failure_reason = FIX_SUCCESS;

	size_t size;

	TRACE(LIBTRADING_FIX_MESSAGE_RECV(msg, flags));

	if (!fix_message_parse(msg, self->dialect, buffer, flags)) {
		self->rx_timestamp = self->now;
		if (!(flags & FIX_RECV_KEEP_IN_MSGSEQNUM)) self->in_msg_seq_num++;
		goto parsed;
	}

	if (fix_session_buffer_full(self))
		buffer_compact(buffer);

	size = buffer_remaining(buffer);
	if (size > FIX_MAX_MESSAGE_SIZE) {
		ssize_t nr;

		size -= FIX_MAX_MESSAGE_SIZE;

		nr = buffer_recv(buffer, self->sockfd, size, translate_recv_flags(flags));

		if (nr <= 0) {
			self->failure_reason = nr == 0 ? FIX_FAILURE_CONN_CLOSED : FIX_FAILURE_SYSTEM;
			return -1;
		}
	}

	if (!fix_message_parse(msg, self->dialect, buffer, flags)) {
		self->rx_timestamp = self->now;
		if (!(flags & FIX_RECV_KEEP_IN_MSGSEQNUM)) self->in_msg_seq_num++;
		goto parsed;
	}

	TRACE(LIBTRADING_FIX_MESSAGE_RECV_ERR());

	return 0;

parsed:
	TRACE(LIBTRADING_FIX_MESSAGE_RECV_RET());

	*res = msg;
	return 1;
}
