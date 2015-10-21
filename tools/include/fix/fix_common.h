#ifndef TOOLS_FIX_SESSION_H
#define TOOLS_FIX_SESSION_H

#include "libtrading/proto/fix_session.h"
#include "libtrading/array.h"

int fix_session_sequence_reset(struct fix_session *session, unsigned long msg_seq_num, unsigned long new_seq_num, bool gap_fill);
int fix_session_order_cancel_request(struct fix_session *session, struct fix_field *fields, long nr_fields);
int fix_session_order_cancel_replace(struct fix_session *session, struct fix_field *fields, long nr_fields);
int fix_session_execution_report(struct fix_session *session, struct fix_field *fields, long nr_fields);
int fix_session_new_order_single(struct fix_session *session, struct fix_field* fields, long nr_fields);
int fix_session_resend_request(struct fix_session *session, unsigned long bgn, unsigned long end);
int fix_session_reject(struct fix_session *session, unsigned long refseqnum, char *text);
int fix_session_heartbeat(struct fix_session *session, const char *test_req_id);
bool fix_session_keepalive(struct fix_session *session, struct timespec *now);
bool fix_session_admin(struct fix_session *session, struct fix_message *msg);
int fix_session_logout(struct fix_session *session, const char *text);
int fix_session_test_request(struct fix_session *session);
int fix_session_logon(struct fix_session *session);

#endif
