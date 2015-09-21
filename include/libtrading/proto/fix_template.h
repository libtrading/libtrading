#ifndef LIBTRADING_FIX_TEMPLATE_H
#define LIBTRADING_FIX_TEMPLATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fix_message.h"
#include "fix_session.h"

#define FIX_TEMPLATE_BODY_LEN_ZPAD 4UL
#define FIX_TEMPLATE_MSG_SEQ_NUM_ZPAD 6UL
#define FIX_MAX_TEMPLATE_BUFFER_SIZE 1024UL

struct fix_template {
	char			*marker_body_length;
	char			*marker_msg_seq_num;
	char			*marker_sender_comp_id;
	char			*marker_sending_time;
	char			*marker_transact_time;
	char			*marker_const_start;
	char			*marker_const_end;

	unsigned long		sender_comp_id_len;
	uint8_t			const_csum;

	struct buffer		buf;	 // first two fields
	char			tx_data[FIX_MAX_TEMPLATE_BUFFER_SIZE];

	unsigned long		nr_fields;	 // number of variable length fields to be serialized each time
	struct fix_field	fields[FIX_MAX_FIELD_NUMBER]; // variable fields array
	struct fix_field	csum_field;

	struct iovec		iov[1];
};

struct fix_template_cfg {
	const char		*begin_string;	 // protocol begin string i.e. FIX4.2
	enum fix_msg_type	msg_type;	 // message type i.e. NEW_ORDER (D)

	const char		*sender_comp_id; // SenderCompID - initial value, assumed to be constant length for all target sessions
	const char		*target_comp_id; // TargetCompID - constant value, unchanged on each unparse cycle

	bool			manage_transact_time; // true to print / manage transact time

	unsigned long		nr_const_fields; // number of constant fields to be serialized during initialization only
	struct fix_field	const_fields[FIX_MAX_FIELD_NUMBER];	 // constant fields array
};

struct fix_template *fix_template_new(void);
void fix_template_free(struct fix_template *self);
char *fix_field_unparse_zpad(struct fix_field *self, int zpad, struct buffer *buffer);
void fix_template_prepare(struct fix_template *self, struct fix_template_cfg *cfg);
void fix_template_update_time(struct fix_template *self, const char *str_now);
void fix_template_unparse(struct fix_template *self, struct fix_session *session);
int fix_template_send(struct fix_template *self, int sockfd, int flags);

#ifdef __cplusplus
}
#endif

#endif
