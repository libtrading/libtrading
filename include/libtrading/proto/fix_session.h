#ifndef LIBTRADING_FIX_SESSION_H
#define LIBTRADING_FIX_SESSION_H

#include "libtrading/proto/fix_message.h"

#include "libtrading/buffer.h"

#include <stdbool.h>

#define RECV_BUFFER_SIZE	4096UL
#define FIX_TX_HEAD_BUFFER_SIZE	FIX_MAX_HEAD_LEN
#define FIX_TX_BODY_BUFFER_SIZE	FIX_MAX_BODY_LEN

struct fix_message;

enum fix_version {
	FIXT_1_1,
	FIX_4_4,
	FIX_4_3,
	FIX_4_2,
	FIX_4_1,
	FIX_4_0,
};

struct fix_session_cfg {
	char			sender_comp_id[32];
	char			target_comp_id[32];
	int			heartbtint;
	enum fix_version	version;
	int			sockfd;
};

struct fix_session {
	int				sockfd;
	const char			*begin_string;
	const char			*sender_comp_id;
	const char			*target_comp_id;

	unsigned long			in_msg_seq_num;
	unsigned long			out_msg_seq_num;

	struct buffer			*rx_buffer;
	struct buffer			*tx_head_buffer;
	struct buffer			*tx_body_buffer;

	struct fix_message		*rx_message;

	int				heartbtint;
	struct timespec			rx_timestamp;
	struct timespec			tx_timestamp;

	char				testreqid[64];
	struct timespec			tr_timestamp;
	int				tr_pending;
};

static inline void fix_session_set_in_msg_seq_num(struct fix_session *session, unsigned long new_msg_seq_num)
{
	session->in_msg_seq_num = new_msg_seq_num;
}

struct fix_session *fix_session_new(struct fix_session_cfg *cfg);
void fix_session_free(struct fix_session *self);
int fix_session_send(struct fix_session *self, struct fix_message *msg, int flags);
struct fix_message *fix_session_recv(struct fix_session *self, int flags);
int fix_session_keepalive(struct fix_session *session, struct timespec *now);
int fix_session_admin(struct fix_session *session, struct fix_message *msg);
bool fix_session_logon(struct fix_session *session);
bool fix_session_logout(struct fix_session *session);
bool fix_session_heartbeat(struct fix_session *session, const char *test_req_id);
bool fix_session_test_request(struct fix_session *session);
bool fix_session_resend_request(struct fix_session *session, unsigned long bgn, unsigned long end);
bool fix_session_sequence_reset(struct fix_session *session, unsigned long msg_seq_num, unsigned long new_seq_num, bool gap_fill);
bool fix_session_new_order_single(struct fix_session *session, struct fix_field* fields, long nr_fields);
bool fix_session_execution_report(struct fix_session *session, struct fix_field *fields, long nr_fields);

#define	FIX_FLAG_PRESERVE_MSG_NUM	0x01

#endif
