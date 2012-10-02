#include "libtrading/proto/fix_session.h"
#include "libtrading/proto/fix_message.h"

#include "libtrading/array.h"

#include <stdlib.h>

static const char *begin_strings[] = {
	[FIXT_1_1]	= "FIXT.1.1",
	[FIX_4_4]	= "FIX.4.4",
	[FIX_4_3]	= "FIX.4.3",
	[FIX_4_2]	= "FIX.4.2",
	[FIX_4_1]	= "FIX.4.1",
	[FIX_4_0]	= "FIX.4.0",
};

struct fix_session *fix_session_new(int sockfd, enum fix_version fix_version, const char *sender_comp_id, const char *target_comp_id)
{
	struct fix_session *self = calloc(1, sizeof *self);

	if (!self)
		return NULL;

	self->sockfd		= sockfd;
	self->begin_string	= begin_strings[fix_version];
	self->sender_comp_id	= sender_comp_id;
	self->target_comp_id	= target_comp_id;
	self->out_msg_seq_num	= 1;

	return self;
}

void fix_session_free(struct fix_session *self)
{
	free(self);
}

int fix_session_send(struct fix_session *self, struct fix_message *msg, int flags)
{
	msg->begin_string	= self->begin_string;
	msg->sender_comp_id	= self->sender_comp_id;
	msg->target_comp_id	= self->target_comp_id;
	msg->msg_seq_num	= self->out_msg_seq_num++;

	return fix_message_send(msg, self->sockfd, flags);
}

bool fix_session_logon(struct fix_session *session)
{
	struct fix_message *response;
	struct fix_message logon_msg;
	struct fix_field fields[] = {
		FIX_INT_FIELD(EncryptMethod, 0),
		FIX_INT_FIELD(HeartBtInt, 15),
		FIX_STRING_FIELD(ResetSeqNumFlag, "Y"),
	};
	bool ret;

	logon_msg	= (struct fix_message) {
		.msg_type	= FIX_MSG_LOGON,
		.nr_fields	= ARRAY_SIZE(fields),
		.fields		= fields,
	};

	fix_session_send(session, &logon_msg, 0);

	response = fix_message_recv(session->sockfd, 0);
	if (!response)
		return false;

	ret = fix_message_type_is(response, FIX_MSG_LOGON);

	fix_message_free(response);

	return ret;
}

bool fix_session_logout(struct fix_session *session)
{
	struct fix_message *response;
	struct fix_message logout_msg;
	bool ret;

	logout_msg	= (struct fix_message) {
		.msg_type	= FIX_MSG_LOGOUT,
	};
	fix_session_send(session, &logout_msg, 0);

retry:
	/* TODO: Logout should be forced after grace period elapsed */

	response = fix_message_recv(session->sockfd, 0);
	if (!response)
		return false;

	if (fix_message_type_is(response, FIX_MSG_TEST_REQUEST)) {
		fix_session_heartbeat(session, true);
		fix_message_free(response);
		goto retry;
	}

	ret = fix_message_type_is(response, FIX_MSG_LOGOUT);

	fix_message_free(response);

	return ret;
}

bool fix_session_heartbeat(struct fix_session *session, bool request_response)
{
	struct fix_message heartbeat_msg;
	/* Any string can be used as the TestReqID */
	struct fix_field fields[] = {
		FIX_STRING_FIELD(TestReqID, "TestReqID"),
	};
	long nr_fields = ARRAY_SIZE(fields);

	/* Hearbeat is not the result of Test Request */
	/* Do not include TestReqID tag in heartbeat */
	if (!request_response)
		nr_fields--;

	heartbeat_msg	= (struct fix_message) {
		.msg_type	= FIX_MSG_HEARTBEAT,
		.nr_fields	= nr_fields,
		.fields		= fields,
	};

	fix_session_send(session, &heartbeat_msg, 0);

	return true;
}
