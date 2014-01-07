#ifndef LIBTRADING_FAST_SESSION_H
#define LIBTRADING_FAST_SESSION_H

#include <libtrading/proto/fast_message.h>

#include <libtrading/buffer.h>

#include <string.h>

#define	FAST_RECV_BUFFER_SIZE	(2 * FAST_MESSAGE_MAX_SIZE)
#define	FAST_TX_BUFFER_SIZE	(2 * FAST_MESSAGE_MAX_SIZE)

struct fast_message;

struct fast_session_cfg {
	int		preamble_bytes;
	int		sockfd;
	bool		reset;
};

struct fast_session {
	u64			last_tid;
	int			sockfd;

	struct buffer		*rx_buffer;
	struct buffer		*tx_pmap_buffer;
	struct buffer		*tx_message_buffer;

	int			nr_messages;
	struct fast_message	*rx_message;
	struct fast_message	*rx_messages;

	struct fast_preamble	preamble;
	struct fast_pmap	pmap;

	bool			reset;

	ssize_t			(*recv)(struct buffer*, int, size_t);
	ssize_t			(*send)(int, struct msghdr *, int);
};

static inline struct fast_message *fast_msg_by_name(struct fast_session *session, const char *name)
{
	struct fast_message *msg;
	int i;

	for (i = 0; i < FAST_TEMPLATE_MAX_NUMBER; i++) {
		msg = session->rx_messages + i;

		if (!strcmp(msg->name, name))
			return msg;
	}

	return NULL;
}

int fast_session_send(struct fast_session *self, struct fast_message *msg, int flags);
struct fast_message *fast_session_recv(struct fast_session *self, int flags);
int fast_parse_template(struct fast_session *self, const char *xml);
struct fast_session *fast_session_new(struct fast_session_cfg *cfg);
void fast_session_free(struct fast_session *self);
void fast_session_reset(struct fast_session *self);

#endif
