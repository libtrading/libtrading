#include "fix/session.h"

#include "fix/message.h"

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

	return fix_message_send(msg, self->sockfd, flags);
}
