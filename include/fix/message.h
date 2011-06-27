#ifndef FIX__FIX_H
#define FIX__FIX_H

#include "fix/buffer.h"
#include "fix/field.h"

#include <stdbool.h>
#include <stdint.h>

#define	Heartbeat		"0"
#define TestRequest		"1"
#define ResendRequest		"2"
#define Reject			"3"
#define SequenceReset		"4"
#define Logout			"5"
#define Logon			"A"

struct fix_message {
	/*
	 * These are required fields.
	 */
	const char			*begin_string;
	/* XXX: BodyLength */
	const char			*msg_type;
	const char			*sender_comp_id;
	const char			*target_comp_id;
	/* XXX: MsgSeqNum */
	/* XXX: SendingTime */

	/*
	 * This buffer is used for wire protocol parsing and unparsing.
	 */
	struct buffer			*buffer;
};

struct fix_message *fix_message_new(void);
void fix_message_free(struct fix_message *self);

struct fix_message *fix_message_parse(struct buffer *buffer);
void fix_message_validate(struct fix_message *self);
struct fix_message *fix_message_recv(int sockfd, int flags);
int fix_message_send(struct fix_message *self, int sockfd, int flags);

bool fix_message_type_is(struct fix_message *self, const char *type);

#endif /* FIX__FIX_H */
