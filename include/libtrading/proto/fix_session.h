#ifndef LIBTRADING_FIX_SESSION_H
#define LIBTRADING_FIX_SESSION_H

#include "libtrading/proto/fix_message.h"

#include "libtrading/buffer.h"

#include <stdbool.h>

#define RECV_BUFFER_SIZE	4096UL

struct fix_message;

enum fix_version {
	FIXT_1_1,
	FIX_4_4,
	FIX_4_3,
	FIX_4_2,
	FIX_4_1,
	FIX_4_0,
};

struct fix_session {
	int				sockfd;
	const char			*begin_string;
	const char			*sender_comp_id;
	const char			*target_comp_id;

	unsigned long			out_msg_seq_num;

	struct buffer			*rx_buffer;
};

struct fix_session *fix_session_new(int sockfd, enum fix_version, const char *sender_comp_id, const char *target_comp_id);
void fix_session_free(struct fix_session *self);
int fix_session_send(struct fix_session *self, struct fix_message *msg, int flags);
struct fix_message *fix_session_recv(struct fix_session *self, int flags);
bool fix_session_logon(struct fix_session *session);
bool fix_session_logout(struct fix_session *session);
bool fix_session_heartbeat(struct fix_session *session, bool request_response);

static inline bool fix_session_buffer_full(struct fix_session *session)
{
	return (buffer_remaining(session->rx_buffer) <= MAX_MESSAGE_SIZE);
}

#endif
