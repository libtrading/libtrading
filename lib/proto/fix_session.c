#include "libtrading/proto/fix_session.h"

#include "libtrading/array.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static const char *begin_strings[] = {
	[FIXT_1_1]	= "FIXT.1.1",
	[FIX_4_4]	= "FIX.4.4",
	[FIX_4_3]	= "FIX.4.3",
	[FIX_4_2]	= "FIX.4.2",
	[FIX_4_1]	= "FIX.4.1",
	[FIX_4_0]	= "FIX.4.0",
};

struct fix_session *fix_session_new(struct fix_session_cfg *cfg)
{
	struct fix_session *self = calloc(1, sizeof *self);

	if (!self)
		return NULL;

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

	if (clock_gettime(CLOCK_MONOTONIC, &self->rx_timestamp)) {
		fix_session_free(self);
		return NULL;
	}

	if (clock_gettime(CLOCK_MONOTONIC, &self->tx_timestamp)) {
		fix_session_free(self);
		return NULL;
	}

	self->begin_string	= begin_strings[cfg->version];
	self->sender_comp_id	= cfg->sender_comp_id;
	self->target_comp_id	= cfg->target_comp_id;
	self->heartbtint	= cfg->heartbtint;
	self->sockfd		= cfg->sockfd;
	self->in_msg_seq_num	= 0;
	self->out_msg_seq_num	= 1;

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

int fix_session_send(struct fix_session *self, struct fix_message *msg, int flags)
{
	msg->begin_string	= self->begin_string;
	msg->sender_comp_id	= self->sender_comp_id;
	msg->target_comp_id	= self->target_comp_id;

	if (!(flags && FIX_FLAG_PRESERVE_MSG_NUM))
		msg->msg_seq_num	= self->out_msg_seq_num++;

	msg->head_buf = self->tx_head_buffer;
	buffer_reset(msg->head_buf);
	msg->body_buf = self->tx_body_buffer;
	buffer_reset(msg->body_buf);

	clock_gettime(CLOCK_MONOTONIC, &self->tx_timestamp);

	return fix_message_send(msg, self->sockfd, flags);
}

static inline bool fix_session_buffer_full(struct fix_session *session)
{
	return buffer_remaining(session->rx_buffer) <= FIX_MAX_MESSAGE_SIZE;
}

struct fix_message *fix_session_recv(struct fix_session *self, int flags)
{
	struct fix_message *msg = self->rx_message;
	struct buffer *buffer = self->rx_buffer;
	const char *start_prev;
	size_t size;
	ssize_t nr;
	long shift;

	start_prev = buffer_start(buffer);

	if (!fix_message_parse(msg, buffer)) {
		clock_gettime(CLOCK_MONOTONIC, &self->rx_timestamp);
		self->in_msg_seq_num++;
		return msg;
	}

	shift = start_prev - buffer_start(buffer);

	buffer_advance(buffer, shift);

	if (fix_session_buffer_full(self))
		buffer_compact(buffer);

	size = buffer_remaining(buffer);
	if (size > FIX_MAX_MESSAGE_SIZE) {
		size -= FIX_MAX_MESSAGE_SIZE;

		nr = buffer_nread(buffer, self->sockfd, size);
		if (nr < 0)
			return NULL;
	}

	if (!buffer_size(buffer))
		return NULL;

	if (!fix_message_parse(msg, buffer)) {
		clock_gettime(CLOCK_MONOTONIC, &self->rx_timestamp);
		self->in_msg_seq_num++;
		return msg;
	} else
		return NULL;
}

struct fix_message *fix_session_process(struct fix_session *session, struct fix_message *msg)
{
	struct fix_field *field;

	if (msg->msg_seq_num > session->in_msg_seq_num) {
		fix_session_resend_request(session, session->in_msg_seq_num, msg->msg_seq_num);
		fix_session_set_in_msg_seq_num(session, session->in_msg_seq_num - 1);

		return NULL;
	}

	switch (msg->type) {
	case FIX_MSG_TYPE_TEST_REQUEST: {
		char id[128] = "TestReqID";

		field = fix_get_field(msg, TestReqID);

		if (field)
			fix_get_string(field, id, sizeof(id));

		fix_session_heartbeat(session, id);

		return NULL;
	}
	case FIX_MSG_TYPE_RESEND_REQUEST: {
		unsigned long begin_seq_num;
		unsigned long end_seq_num;

		field = fix_get_field(msg, BeginSeqNo);
		if (!field)
			return NULL;

		begin_seq_num = field->int_value;

		field = fix_get_field(msg, EndSeqNo);
		if (!field)
			return NULL;

		end_seq_num = field->int_value;

		fix_session_sequence_reset(session, begin_seq_num, end_seq_num + 1, true);

		return NULL;
	}
	default:
		break;
	}

	return msg;
}

bool fix_session_logon(struct fix_session *session)
{
	struct fix_message *response;
	struct fix_message logon_msg;
	struct fix_field fields[] = {
		FIX_INT_FIELD(EncryptMethod, 0),
		FIX_STRING_FIELD(ResetSeqNumFlag, "Y"),
		FIX_INT_FIELD(HeartBtInt, session->heartbtint),
	};
	bool ret;

	logon_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_LOGON,
		.nr_fields	= ARRAY_SIZE(fields),
		.fields		= fields,
	};

	fix_session_send(session, &logon_msg, 0);

	response = fix_session_recv(session, 0);
	if (!response)
		return false;

	ret = fix_message_type_is(response, FIX_MSG_TYPE_LOGON);

	return ret;
}

bool fix_session_logout(struct fix_session *session)
{
	struct fix_message *response;
	struct fix_message logout_msg;
	bool ret;

	logout_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_LOGOUT,
	};
	fix_session_send(session, &logout_msg, 0);

retry:
	/* TODO: Logout should be forced after grace period elapsed */

	response = fix_session_recv(session, 0);
	if (!response)
		return false;

	if (!fix_session_process(session, response))
		goto retry;

	ret = fix_message_type_is(response, FIX_MSG_TYPE_LOGOUT);

	return ret;
}

bool fix_session_heartbeat(struct fix_session *session, const char *test_req_id)
{
	struct fix_message heartbeat_msg;
	struct fix_field fields[1];
	int nr_fields = 0;

	if (test_req_id)
		fields[nr_fields++] = FIX_STRING_FIELD(TestReqID, test_req_id);

	heartbeat_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_HEARTBEAT,
		.nr_fields	= nr_fields,
		.fields		= fields,
	};

	fix_session_send(session, &heartbeat_msg, 0);

	return true;
}

bool fix_session_test_request(struct fix_session *session)
{
	struct fix_message test_req_msg;
	/* Any string can be used as the TestReqID */
	struct fix_field fields[] = {
		FIX_STRING_FIELD(TestReqID, "TestReqID"),
	};

	test_req_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_TEST_REQUEST,
		.nr_fields	= ARRAY_SIZE(fields),
		.fields		= fields,
	};

	fix_session_send(session, &test_req_msg, 0);

	return true;
}

bool fix_session_resend_request(struct fix_session *session,
					unsigned long bgn, unsigned long end)
{
	struct fix_message resend_request_msg;
	struct fix_field fields[] = {
		FIX_INT_FIELD(BeginSeqNo, bgn),
		FIX_INT_FIELD(EndSeqNo, end),
	};

	resend_request_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_RESEND_REQUEST,
		.nr_fields	= ARRAY_SIZE(fields),
		.fields		= fields,
	};

	fix_session_send(session, &resend_request_msg, 0);

	return true;
}

bool fix_session_sequence_reset(struct fix_session *session, unsigned long msg_seq_num,
							unsigned long new_seq_num, bool gap_fill)
{
	struct fix_message sequence_reset_msg;
	struct fix_field fields[] = {
		FIX_INT_FIELD(NewSeqNo, new_seq_num),
		FIX_STRING_FIELD(GapFillFlag, "Y"),
	};
	long nr_fields = ARRAY_SIZE(fields);

	if (!gap_fill)
		nr_fields--;

	sequence_reset_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_SEQUENCE_RESET,
		.msg_seq_num	= msg_seq_num,
		.nr_fields	= nr_fields,
		.fields		= fields,
	};

	fix_session_send(session, &sequence_reset_msg, FIX_FLAG_PRESERVE_MSG_NUM);
	return true;
}

bool fix_session_new_order_single(struct fix_session *session,
					struct fix_field *fields, long nr_fields)
{
	struct fix_message new_order_single_msg;

	new_order_single_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_NEW_ORDER_SINGLE,
		.nr_fields	= nr_fields,
		.fields		= fields,
	};

	fix_session_send(session, &new_order_single_msg, 0);

	return true;
}

bool fix_session_execution_report(struct fix_session *session,
					struct fix_field *fields, long nr_fields)
{
	struct fix_message new_order_single_msg;

	new_order_single_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_EXECUTION_REPORT,
		.nr_fields	= nr_fields,
		.fields		= fields,
	};

	fix_session_send(session, &new_order_single_msg, 0);

	return true;
}
