#include "libtrading/proto/fix_template.h"
#include "libtrading/read-write.h"
#include "libtrading/buffer.h"
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

	self->head_buf.start = 0;
	self->head_buf.end = 0;
	self->head_buf.capacity = FIX_MAX_HEAD_BUFFER_SIZE;
	self->head_buf.data = &self->tx_data[0];

	self->const_buf.start = 0;
	self->const_buf.end = 0;
	self->const_buf.capacity = FIX_MAX_TEMPLATE_TX_BUFFER_SIZE;
	self->const_buf.data = self->head_buf.data + self->head_buf.capacity;

	self->sys_buf.start = 0;
	self->sys_buf.end = 0;
	self->sys_buf.capacity = FIX_MAX_SYS_BUFFER_SIZE;
	self->sys_buf.data = self->const_buf.data + self->const_buf.capacity;

	self->body_buf.start = 0;
	self->body_buf.end = 0;
	self->body_buf.capacity = FIX_MAX_TEMPLATE_TX_BUFFER_SIZE;
	self->body_buf.data = self->sys_buf.data + self->sys_buf.capacity;

	self->csum_buf.start = 0;
	self->csum_buf.end = 0;
	self->csum_buf.capacity = FIX_MAX_CSUM_BUFFER_SIZE;
	self->csum_buf.data = self->body_buf.data + self->body_buf.capacity;

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

	/* head */
	fix_field_unparse(&FIX_STRING_FIELD(BeginString, cfg->begin_string), &self->head_buf);
	self->marker_body_length = fix_field_unparse_zpad(&FIX_INT_FIELD(BodyLength, 0), FIX_TEMPLATE_BODY_LEN_ZPAD, &self->head_buf);
	fix_field_unparse(&FIX_STRING_FIELD(MsgType, fix_msg_types[cfg->msg_type]), &self->const_buf);
	self->marker_msg_seq_num = fix_field_unparse_zpad(&FIX_INT_FIELD(MsgSeqNum, 0), FIX_TEMPLATE_MSG_SEQ_NUM_ZPAD, &self->sys_buf);
	self->marker_sender_comp_id = fix_field_unparse_zpad(&FIX_STRING_FIELD(SenderCompID, cfg->sender_comp_id), 0, &self->sys_buf); // assume all sender_comp_id are of the same size
	self->sender_comp_id_len = strlen(cfg->sender_comp_id);
	self->marker_sending_time = fix_field_unparse_zpad(&FIX_STRING_FIELD(SendingTime, "YYYYMMDD-HH:MM:SS.sss"), 0, &self->sys_buf);
	fix_field_unparse(&FIX_STRING_FIELD(TargetCompID, cfg->target_comp_id), &self->const_buf); // assume target_comp_id is static

	if (cfg->manage_transact_time)
		self->marker_transact_time = fix_field_unparse_zpad(&FIX_STRING_FIELD(TransactTime, "YYYYMMDD-HH:MM:SS.sss"), 0, &self->sys_buf);

	for (i = 0; i < cfg->nr_const_fields; i++)
		fix_field_unparse(&cfg->const_fields[i], &self->const_buf);

	self->const_csum = buffer_sum(&self->const_buf);
	self->marker_check_sum = fix_field_unparse_zpad(&FIX_CHECKSUM_FIELD(CheckSum, 0), 3, &self->csum_buf); // checksum is always three digit

	assert(self->marker_body_length != NULL);
	assert(self->marker_msg_seq_num != NULL);
	assert(self->marker_sender_comp_id != NULL);
	assert(self->marker_sending_time != NULL);
	assert(self->marker_check_sum != NULL);
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

	buffer_reset(&self->body_buf);

	for (i = 0; i < self->nr_fields; i++)
		fix_field_unparse(&self->fields[i], &self->body_buf);

	modp_litoa10_zpad(buffer_size(&self->body_buf) + buffer_size(&self->sys_buf) + buffer_size(&self->const_buf), FIX_TEMPLATE_BODY_LEN_ZPAD, self->marker_body_length);
	modp_litoa10_zpad(session->out_msg_seq_num, FIX_TEMPLATE_MSG_SEQ_NUM_ZPAD, self->marker_msg_seq_num);
	memcpy(self->marker_sender_comp_id, session->sender_comp_id, self->sender_comp_id_len);
	checksumtoa((buffer_sum(&self->head_buf) + buffer_sum(&self->sys_buf) + self->const_csum + buffer_sum(&self->body_buf)) % 256, self->marker_check_sum);
}

static size_t iov_length(struct iovec *iov, size_t iov_len)
{
	size_t len = 0;
	size_t i;

	for (i = 0; i < iov_len; ++i)
		len += iov[i].iov_len;

	return len;
}

int fix_template_send(struct fix_template *self, int sockfd, int flags)
{
	int ret = 0;

	TRACE(LIBTRADING_FIX_MESSAGE_SEND(self, sockfd, flags));

	self->iov[0].iov_base	= &self->tx_data[0];
	self->iov[0].iov_len	= self->head_buf.capacity + self->const_buf.end; // head and const buffers are continuous
	buffer_to_iovec(&self->sys_buf,	 &self->iov[1]);
	buffer_to_iovec(&self->body_buf, &self->iov[2]);
	buffer_to_iovec(&self->csum_buf, &self->iov[3]);

	if ((ret = io_sendmsg(sockfd, self->iov, 4, flags)) < 0) {
		return ret;
	}

	TRACE(LIBTRADING_FIX_MESSAGE_SEND_RET());

	return iov_length(self->iov, 4) - ret;
}