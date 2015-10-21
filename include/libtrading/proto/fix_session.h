#ifndef LIBTRADING_FIX_SESSION_H
#define LIBTRADING_FIX_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/proto/fix_message.h"

#include "libtrading/buffer.h"

#include <stdbool.h>

#define RECV_BUFFER_SIZE	4096UL
#define FIX_TX_HEAD_BUFFER_SIZE	FIX_MAX_HEAD_LEN
#define FIX_TX_BODY_BUFFER_SIZE	FIX_MAX_BODY_LEN

struct fix_message;

enum fix_version {
	FIX_4_0,
	FIX_4_1,
	FIX_4_2,
	FIX_4_3,
	FIX_4_4,
	FIX_5_0,
	FIXT_1_1,
};

struct fix_dialect {
	enum fix_version	version;
	enum fix_type		(*tag_type)(int tag);
};

extern struct fix_dialect	fix_dialects[];

struct fix_session_cfg {
	char			sender_comp_id[32];
	char			target_comp_id[32];
	char			password[32];
	int			heartbtint;
	struct fix_dialect	*dialect;
	int			sockfd;
	unsigned long		in_msg_seq_num;
	unsigned long		out_msg_seq_num;
	
	void			*user_data;
};

enum fix_failure_reason {
	FIX_SUCCESS		= 0,
	FIX_FAILURE_CONN_CLOSED = 1,
	FIX_FAILURE_RECV_ZERO_B = 2,
	FIX_FAILURE_SYSTEM	= 3,	// see errno
	FIX_FAILURE_GARBLED	= 4
};

struct fix_session {
	struct fix_dialect		*dialect;
	int				sockfd;
	bool				active;
	const char			*password;
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

	struct timespec			now;
	char				str_now[64];

	struct timespec			rx_timestamp;
	struct timespec			tx_timestamp;

	char				testreqid[64];
	struct timespec			tr_timestamp;
	int				tr_pending;

	enum fix_failure_reason		failure_reason;

	void				*user_data;
};

static inline bool fix_msg_expected(struct fix_session *session, struct fix_message *msg)
{
	return msg->msg_seq_num == session->in_msg_seq_num || fix_message_type_is(msg, FIX_MSG_TYPE_SEQUENCE_RESET);
}

void fix_session_cfg_init(struct fix_session_cfg *cfg);
struct fix_session_cfg *fix_session_cfg_new(const char *sender_comp_id, const char *target_comp_id, int heartbtint, const char *dialect, int sockfd);
struct fix_session *fix_session_new(struct fix_session_cfg *cfg);
void fix_session_free(struct fix_session *self);
int fix_session_time_update_monotonic(struct fix_session *self, struct timespec *monotonic);
int fix_session_time_update_realtime(struct fix_session *self, struct timespec *realtime);
int fix_session_time_update(struct fix_session *self);
int fix_session_send(struct fix_session *self, struct fix_message *msg, unsigned long flags);
int fix_session_recv(struct fix_session *self, struct fix_message **msg, unsigned long flags);

enum fix_send_flag {
	FIX_SEND_FLAG_PRESERVE_MSG_NUM = 1UL << 0, // lower 16 bits
	FIX_SEND_FLAG_PRESERVE_BUFFER  = 1UL << 1,
};

enum fix_recv_flag {
	FIX_RECV_FLAG_MSG_DONTWAIT = 1UL << 16, // upper 16 bits
	FIX_RECV_KEEP_IN_MSGSEQNUM = 1UL << 17
};

#ifdef __cplusplus
}
#endif

#endif
