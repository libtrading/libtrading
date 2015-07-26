#include "fix/fix_common.h"

#include <string.h>
#include <stdio.h>

bool fix_session_keepalive(struct fix_session *session, struct timespec *now)
{
	int diff;

	if (!session->tr_pending) {
		diff = now->tv_sec - session->rx_timestamp.tv_sec;

		if (diff > 1.2 * session->heartbtint)
			fix_session_test_request(session);
	} else {
		diff = now->tv_sec - session->tr_timestamp.tv_sec;

		if (diff > 0.5 * session->heartbtint)
			return false;
	}

	diff = now->tv_sec - session->tx_timestamp.tv_sec;
	if (diff > session->heartbtint)
		fix_session_heartbeat(session, NULL);

	return true;
}

static int fix_do_unexpected(struct fix_session *session, struct fix_message *msg)
{
	char text[128];

	if (msg->msg_seq_num > session->in_msg_seq_num) {
		unsigned long end_seq_no;

		if (session->dialect->version <= FIX_4_1) {
			end_seq_no = 999999;
		} else {
			end_seq_no = 0;
		}
		fix_session_resend_request(session, session->in_msg_seq_num, end_seq_no);

		session->in_msg_seq_num--;
	} else if (msg->msg_seq_num < session->in_msg_seq_num) {
		snprintf(text, sizeof(text),
			"MsgSeqNum too low, expecting %lu received %lu",
				session->in_msg_seq_num, msg->msg_seq_num);

		session->in_msg_seq_num--;

		if (!fix_get_field(msg, PossDupFlag)) {
			fix_session_logout(session, text);
			return 1;
		}
	}

	return 0;
}

/*
 * Return values:
 * - true means that the function was able to handle a message, a user should
 *	not process the message further
 * - false means that the function wasn't able to handle a message, a user
 *	should decide on what to do further
 */
bool fix_session_admin(struct fix_session *session, struct fix_message *msg)
{
	struct fix_field *field;

	if (!fix_msg_expected(session, msg)) {
		fix_do_unexpected(session, msg);

		goto done;
	}

	switch (msg->type) {
	case FIX_MSG_TYPE_HEARTBEAT: {
		field = fix_get_field(msg, TestReqID);

		if (field && !strncmp(field->string_value,
				session->testreqid, strlen(session->testreqid)))
			session->tr_pending = 0;

		goto done;
	}
	case FIX_MSG_TYPE_TEST_REQUEST: {
		char id[128] = "TestReqID";

		field = fix_get_field(msg, TestReqID);

		if (field)
			fix_get_string(field, id, sizeof(id));

		fix_session_heartbeat(session, id);

		goto done;
	}
	case FIX_MSG_TYPE_RESEND_REQUEST: {
		unsigned long begin_seq_num;
		unsigned long end_seq_num;

		field = fix_get_field(msg, BeginSeqNo);
		if (!field)
			goto fail;

		begin_seq_num = field->int_value;

		field = fix_get_field(msg, EndSeqNo);
		if (!field)
			goto fail;

		end_seq_num = field->int_value;

		fix_session_sequence_reset(session, begin_seq_num, end_seq_num + 1, true);

		goto done;
	}
	case FIX_MSG_TYPE_SEQUENCE_RESET: {
		unsigned long exp_seq_num;
		unsigned long new_seq_num;
		unsigned long msg_seq_num;
		char text[128];

		field = fix_get_field(msg, GapFillFlag);

		if (field && !strncmp(field->string_value, "Y", 1)) {
			field = fix_get_field(msg, NewSeqNo);

			if (!field)
				goto done;

			exp_seq_num = session->in_msg_seq_num;
			new_seq_num = field->int_value;
			msg_seq_num = msg->msg_seq_num;

			if (msg_seq_num > exp_seq_num) {
				fix_session_resend_request(session,
						exp_seq_num, msg_seq_num);

				session->in_msg_seq_num--;

				goto done;
			} else if (msg_seq_num < exp_seq_num) {
				snprintf(text, sizeof(text),
					"MsgSeqNum too low, expecting %lu received %lu",
								exp_seq_num, msg_seq_num);

				session->in_msg_seq_num--;

				if (!fix_get_field(msg, PossDupFlag))
					fix_session_logout(session, text);

				goto done;
			}

			if (new_seq_num > msg_seq_num) {
				session->in_msg_seq_num = new_seq_num - 1;
			} else {
				snprintf(text, sizeof(text),
					"Attempt to lower sequence number, invalid value NewSeqNum = %lu", new_seq_num);

				fix_session_reject(session, msg_seq_num, text);
			}
		} else {
			field = fix_get_field(msg, NewSeqNo);

			if (!field)
				goto done;

			exp_seq_num = session->in_msg_seq_num;
			new_seq_num = field->int_value;

			if (new_seq_num > exp_seq_num) {
				session->in_msg_seq_num = new_seq_num - 1;
			} else if (new_seq_num < exp_seq_num) {
				snprintf(text, sizeof(text),
					"Value is incorrect (too low) %lu", new_seq_num);

				session->in_msg_seq_num--;

				fix_session_reject(session, exp_seq_num, text);
			}
		}

		goto done;
	}
	default:
		break;
	}

fail:
	return false;

done:
	return true;
}

int fix_session_logon(struct fix_session *session)
{
	struct fix_message *response;
	struct fix_message logon_msg;
	struct fix_field fields[] = {
		FIX_INT_FIELD(EncryptMethod, 0),
		FIX_STRING_FIELD(ResetSeqNumFlag, "Y"),
		FIX_INT_FIELD(HeartBtInt, session->heartbtint),
		FIX_STRING_FIELD(Password, session->password),
	};

	logon_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_LOGON,
		.nr_fields	= ARRAY_SIZE(fields),
		.fields		= fields,
	};

	if (!session->password || !strlen(session->password))
		logon_msg.nr_fields--;

	fix_session_send(session, &logon_msg, 0);
	session->active = true;

retry:
	if (fix_session_recv(session, &response, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (!fix_msg_expected(session, response)) {
		if (fix_do_unexpected(session, response))
			return -1;

		goto retry;
	}

	if (!fix_message_type_is(response, FIX_MSG_TYPE_LOGON)) {
		fix_session_logout(session, "First message not a logon");

		return -1;
	}

	return 0;
}

int fix_session_logout(struct fix_session *session, const char *text)
{
	struct fix_field fields[] = {
		FIX_STRING_FIELD(Text, text),
	};
	long nr_fields = ARRAY_SIZE(fields);
	struct fix_message logout_msg;
	struct fix_message *response;
	struct timespec start, end;

	if (!text)
		nr_fields--;

	logout_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_LOGOUT,
		.nr_fields	= nr_fields,
		.fields		= fields,
	};

	fix_session_send(session, &logout_msg, 0);

	clock_gettime(CLOCK_MONOTONIC, &start);
	session->active = false;

retry:
	clock_gettime(CLOCK_MONOTONIC, &end);
	/* Grace period 2 seconds */
	if (end.tv_sec - start.tv_sec > 2)
		return 0;

	if (fix_session_recv(session, &response, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0)
		goto retry;

	if (fix_session_admin(session, response))
		goto retry;

	if (fix_message_type_is(response, FIX_MSG_TYPE_LOGOUT))
		return 0;

	return -1;
}

int fix_session_heartbeat(struct fix_session *session, const char *test_req_id)
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

	return fix_session_send(session, &heartbeat_msg, 0);
}

int fix_session_test_request(struct fix_session *session)
{
	struct fix_message test_req_msg;
	struct fix_field fields[] = {
		FIX_STRING_FIELD(TestReqID, session->str_now),
	};

	strncpy(session->testreqid, session->str_now,
				sizeof(session->testreqid));

	test_req_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_TEST_REQUEST,
		.nr_fields	= ARRAY_SIZE(fields),
		.fields		= fields,
	};

	session->tr_timestamp = session->now;
	session->tr_pending = 1;

	return fix_session_send(session, &test_req_msg, 0);
}

int fix_session_resend_request(struct fix_session *session,
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

	return fix_session_send(session, &resend_request_msg, 0);
}

int fix_session_reject(struct fix_session *session, unsigned long refseqnum, char *text)
{
	struct fix_message reject_msg;
	struct fix_field fields[] = {
		FIX_INT_FIELD(RefSeqNum, refseqnum),
		FIX_STRING_FIELD(Text, text),
	};
	long nr_fields = ARRAY_SIZE(fields);

	if (!text)
		nr_fields--;

	reject_msg		= (struct fix_message) {
		.type		= FIX_MSG_TYPE_REJECT,
		.nr_fields	= nr_fields,
		.fields		= fields,
	};

	return fix_session_send(session, &reject_msg, 0);
}

int fix_session_sequence_reset(struct fix_session *session, unsigned long msg_seq_num,
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

	return fix_session_send(session, &sequence_reset_msg, FIX_SEND_FLAG_PRESERVE_MSG_NUM);
}

int fix_session_new_order_single(struct fix_session *session,
					struct fix_field *fields, long nr_fields)
{
	struct fix_message new_order_single_msg;

	new_order_single_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_NEW_ORDER_SINGLE,
		.nr_fields	= nr_fields,
		.fields		= fields,
	};

	return fix_session_send(session, &new_order_single_msg, 0);
}

int fix_session_order_cancel_request(struct fix_session *session,
					struct fix_field *fields, long nr_fields)
{
	struct fix_message order_cancel_request;

	order_cancel_request	= (struct fix_message) {
		.type		= FIX_MSG_ORDER_CANCEL_REQUEST,
		.nr_fields	= nr_fields,
		.fields		= fields,
	};

	return fix_session_send(session, &order_cancel_request, 0);
}

int fix_session_order_cancel_replace(struct fix_session *session,
					struct fix_field *fields, long nr_fields)
{
	struct fix_message order_cancel_replace;

	order_cancel_replace	= (struct fix_message) {
		.type		= FIX_MSG_ORDER_CANCEL_REPLACE,
		.nr_fields	= nr_fields,
		.fields		= fields,
	};

	return fix_session_send(session, &order_cancel_replace, 0);
}

int fix_session_execution_report(struct fix_session *session,
					struct fix_field *fields, long nr_fields)
{
	struct fix_message new_order_single_msg;

	new_order_single_msg	= (struct fix_message) {
		.type		= FIX_MSG_TYPE_EXECUTION_REPORT,
		.nr_fields	= nr_fields,
		.fields		= fields,
	};

	return fix_session_send(session, &new_order_single_msg, 0);
}

