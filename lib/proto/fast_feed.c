#include "libtrading/proto/fast_feed.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

static int fast_feed_socket(struct fast_feed *feed)
{
	struct ip_mreq_source group_src;
	struct ip_mreq group;

	struct sockaddr_in sa;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd < 0)
		goto fail;

	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK) < 0)
		goto fail;

	if (socket_setopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 1) < 0)
		goto fail;

	sa = (struct sockaddr_in) {
		.sin_family		= AF_INET,
		.sin_port		= htons(feed->port),
		.sin_addr		= (struct in_addr) {
			.s_addr			= INADDR_ANY,
		},
	};

	if (bind(sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
		goto fail;

	if (inet_addr(feed->sip) == INADDR_NONE) {
		group.imr_multiaddr.s_addr = inet_addr(feed->ip);
		group.imr_interface.s_addr = inet_addr(feed->lip);

		if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					(char *)&group, sizeof(group)) < 0)
			goto fail;
	} else {
		group_src.imr_multiaddr.s_addr = inet_addr(feed->ip);
		group_src.imr_sourceaddr.s_addr = inet_addr(feed->sip);
		group_src.imr_interface.s_addr = inet_addr(feed->lip);

		if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP,
					(char *)&group_src, sizeof(group_src)) < 0)
			goto fail;
	}

	return sockfd;

fail:
	return -1;
}

int fast_feed_open(struct fast_feed *feed)
{
	int sockfd;

	if (feed->active)
		goto fail;

	feed->session = NULL;

	if (strlen(feed->file)) {
		sockfd = open(feed->file, O_RDONLY);
	} else {
		sockfd = fast_feed_socket(feed);
	}

	if (sockfd < 0)
		goto fail;

	feed->cfg.sockfd = sockfd;

	feed->session = fast_session_new(&feed->cfg);
	if (!feed->session)
		goto fail;

	if (fast_parse_template(feed->session, feed->xml))
		goto fail;

	feed->active = true;

	return 0;

fail:
	fast_session_free(feed->session);

	return -1;
}

int fast_feed_close(struct fast_feed *feed)
{
	struct fast_session *session;
	int sockfd;

	session = feed->session;

	if (!feed->active)
		goto done;

	if (!session)
		goto done;

	sockfd = session->sockfd;

	fast_session_free(session);

	shutdown(sockfd, SHUT_RDWR);

	if (close(sockfd) < 0)
		goto fail;

	feed->session = NULL;
	feed->active = false;

done:
	return 0;

fail:
	return -1;
}
