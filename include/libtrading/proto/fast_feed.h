#ifndef	LIBTRADING_FAST_FEED_H
#define	LIBTRADING_FAST_FEED_H

#include "libtrading/proto/fast_session.h"

#include <sys/socket.h>
#include <sys/types.h>

struct fast_feed {
	struct fast_session	*session;

	u64			recv_num;
	char			xml[128];
	char			lip[32];
	char			ip[32];
	bool			active;
	int			port;
};

static inline int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

static inline struct fast_message *fast_feed_recv(struct fast_feed *feed, int flags)
{
	return fast_session_recv(feed->session, flags);
}

int fast_feed_close(struct fast_feed *feed);
int fast_feed_open(struct fast_feed *feed);

#endif	/* LIBTRADING_FAST_FEED_H */
