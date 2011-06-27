#ifndef FIX__SESSION_H
#define FIX__SESSION_H

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
};

struct fix_session *fix_session_new(int sockfd, enum fix_version, const char *sender_comp_id, const char *target_comp_id);
void fix_session_free(struct fix_session *self);
int fix_session_send(struct fix_session *self, struct fix_message *msg, int flags);

#endif /* FIX__SESSION_H */
