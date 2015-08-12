#include "libtrading/proto/fix_template.h"
#include "libtrading/read-write.h"
#include "libtrading/buffer.h"
#include "libtrading/array.h"
#include "libtrading/trace.h"
#include "libtrading/itoa.h"

#include "modp_numtoa.h"

#include <stdlib.h>
#include <assert.h>

char *fix_field_unparse_zpad(struct fix_field *self, int zpad, struct buffer *buffer)
{
	char *marker = NULL;

	buffer->end += uitoa(self->tag, buffer_end(buffer));

	buffer_put(buffer, '=');
	marker = buffer_end(buffer);

	switch (self->type) {
	case FIX_TYPE_STRING: {
		const char *p = self->string_value;

		while (*p) {
			buffer_put(buffer, *p++);
		}
		break;
	}
	case FIX_TYPE_STRING_8: {
		for (int i = 0; i < sizeof(self->string_8_value) && self->string_8_value[i]; ++i) {
			buffer_put(buffer, self->string_8_value[i]);
		}
		break;
	}
	case FIX_TYPE_CHAR: {
		buffer_put(buffer, self->char_value);
		break;
	}
	case FIX_TYPE_FLOAT: {
		// dtoa2 do not print leading zeros or .0, 7 digits needed sometimes
		buffer->end += modp_dtoa2(self->float_value, buffer_end(buffer), 7); // @TODO: zero pad
		break;
	}
	case FIX_TYPE_INT: {
		buffer->end += modp_litoa10_zpad(self->int_value, zpad, buffer_end(buffer));
		break;
	}
	case FIX_TYPE_CHECKSUM: {
		buffer->end += checksumtoa(self->int_value, buffer_end(buffer));
		break;
	}
	default:
		break;
	};

	buffer_put(buffer, 0x01);

	return marker;
}

struct fix_template *fix_template_new(void)
{
	struct fix_template *self = calloc(1, sizeof *self);

	if (!self)
		return NULL;

	self->marker_transact_time = NULL;

	self->buf.start = 0;
	self->buf.end = 0;
	self->buf.capacity = FIX_MAX_TEMPLATE_BUFFER_SIZE;
	self->buf.data = &self->tx_data[0];

	self->csum_field = FIX_CHECKSUM_FIELD(CheckSum, 0);

	return self;
}

void fix_template_free(struct fix_template *self)
{
	if (!self)
		return;

	free(self);
}

void fix_template_prepare(struct fix_template *self, struct fix_template_cfg *cfg)
{
	int i;

	// head
	fix_field_unparse(&FIX_STRING_FIELD(BeginString, cfg->begin_string), &self->buf);
	self->marker_body_length = fix_field_unparse_zpad(&FIX_INT_FIELD(BodyLength, 0), FIX_TEMPLATE_BODY_LEN_ZPAD, &self->buf);
	fix_field_unparse(&FIX_STRING_FIELD(MsgType, fix_msg_types[cfg->msg_type]), &self->buf);
	self->marker_msg_seq_num = fix_field_unparse_zpad(&FIX_INT_FIELD(MsgSeqNum, 0), FIX_TEMPLATE_MSG_SEQ_NUM_ZPAD, &self->buf);
	self->marker_sender_comp_id = fix_field_unparse_zpad(&FIX_STRING_FIELD(SenderCompID, cfg->sender_comp_id), 0, &self->buf); // assume all sender_comp_id are of the same size
	self->sender_comp_id_len = strlen(cfg->sender_comp_id);
	self->marker_sending_time = fix_field_unparse_zpad(&FIX_STRING_FIELD(SendingTime, "YYYYMMDD-HH:MM:SS.sss"), 0, &self->buf);
	fix_field_unparse(&FIX_STRING_FIELD(TargetCompID, cfg->target_comp_id), &self->buf); // assume target_comp_id is static

	if (cfg->manage_transact_time)
		self->marker_transact_time = fix_field_unparse_zpad(&FIX_STRING_FIELD(TransactTime, "YYYYMMDD-HH:MM:SS.sss"), 0, &self->buf);

	self->marker_const_start = buffer_end(&self->buf);

	for (i = 0; i < cfg->nr_const_fields; i++)
		fix_field_unparse(&cfg->const_fields[i], &self->buf);

	self->marker_const_end = buffer_end(&self->buf);
	self->const_csum =  buffer_sum_range(self->marker_const_start, self->marker_const_end) +
			    buffer_sum_range(self->buf.data, self->marker_body_length);

	assert(self->marker_body_length != NULL);
	assert(self->marker_msg_seq_num != NULL);
	assert(self->marker_sender_comp_id != NULL);
	assert(self->marker_sending_time != NULL);
	assert(self->marker_const_start != NULL);
	assert(self->marker_const_end != NULL);
}

void fix_template_update_time(struct fix_template *self, const char *str_now)
{
	memcpy(self->marker_sending_time, str_now, 21); // sizeof("YYYYMMDD-HH:MM:SS.sss")
	if (self->marker_transact_time)
		memcpy(self->marker_transact_time, str_now, 21); // sizeof("YYYYMMDD-HH:MM:SS.sss")
}

void fix_template_unparse(struct fix_template *self, struct fix_session *session)
{
	int i;

	self->buf.start = 0;
	self->buf.end = (self->marker_const_end - self->buf.data);

	for (i = 0; i < self->nr_fields; i++)
		fix_field_unparse(&self->fields[i], &self->buf);

	memcpy(self->marker_sender_comp_id, session->sender_comp_id, self->sender_comp_id_len);
	modp_litoa10_zpad(session->out_msg_seq_num, FIX_TEMPLATE_MSG_SEQ_NUM_ZPAD, self->marker_msg_seq_num);
	modp_litoa10_zpad(self->buf.end - (self->marker_body_length - self->buf.data + FIX_TEMPLATE_BODY_LEN_ZPAD + 1),
			  FIX_TEMPLATE_BODY_LEN_ZPAD, self->marker_body_length);

	self->csum_field.int_value = (buffer_sum_range(self->marker_body_length, self->marker_const_start) +
				      self->const_csum +
				      buffer_sum_range(self->marker_const_end, buffer_end(&self->buf))) % 256;
	fix_field_unparse(&self->csum_field, &self->buf);

	self->iov[0].iov_base	= &self->tx_data[0]; // prepare iovec for sending
	self->iov[0].iov_len	= self->buf.end;
}

int fix_template_send(struct fix_template *self, int sockfd, int flags)
{
	int ret = 0;

	TRACE(LIBTRADING_FIX_MESSAGE_SEND(self, sockfd, flags));

	if ((ret = io_sendmsg(sockfd, self->iov, ARRAY_SIZE(self->iov), flags)) < 0) {
		return ret;
	}

	TRACE(LIBTRADING_FIX_MESSAGE_SEND_RET());

	return iov_byte_length(self->iov, ARRAY_SIZE(self->iov)) - ret;
}
