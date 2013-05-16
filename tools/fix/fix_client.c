#include "libtrading/proto/fix_message.h"
#include "libtrading/proto/fix_session.h"

#include "libtrading/array.h"
#include "libtrading/die.h"

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>

#include "test.h"

struct protocol_info {
	const char		*name;
	int			(*session_initiate)(struct fix_session_cfg *, const char *);
};

static int fix_session_initiate(struct fix_session_cfg *cfg, const char *script)
{
	struct fcontainer *s_container = NULL;
	struct fcontainer *c_container = NULL;
	struct fix_session *session = NULL;
	struct felem *expected_elem;
	struct felem *tosend_elem;
	struct fix_message *msg;
	FILE *stream;
	int ret = -1;

	stream = fopen(script, "r");
	if (!stream) {
		fprintf(stderr, "Opening %s failed: %s\n", script, strerror(errno));
		goto exit;
	}

	session = fix_session_new(cfg);
	if (!session) {
		fprintf(stderr, "FIX session cannot be created\n");
		goto exit;
	}

	s_container = fcontainer_new();
	if (!s_container) {
		fprintf(stderr, "Cannot allocate container\n");
		goto exit;
	}

	c_container = fcontainer_new();
	if (!c_container) {
		fprintf(stderr, "Cannot allocate container\n");
		goto exit;
	}

	if (script_read(stream, s_container, c_container)) {
		fprintf(stderr, "Invalid script: %s\n", script);
		goto exit;
	}

	ret = fix_session_logon(session);
	if (ret) {
		fprintf(stderr, "Client Logon FAILED\n");
		goto exit;
	}

	fprintf(stderr, "Client Logon OK\n");

	expected_elem = cur_elem(s_container);
	tosend_elem = cur_elem(c_container);

	while (tosend_elem) {
		if (tosend_elem->msg.msg_seq_num)
			fix_session_send(session, &tosend_elem->msg, FIX_FLAG_PRESERVE_MSG_NUM);
		else
			fix_session_send(session, &tosend_elem->msg, 0);

		if (!expected_elem)
			goto next;

retry:
		msg = fix_session_recv(session, 0);

		if (!msg)
			goto retry;

		fprintf(stdout, "< ");
		fprintmsg(stdout, msg);

		if (fmsgcmp(&expected_elem->msg, msg)) {
			fprintf(stderr, "Client: messages differ\n");
			fprintmsg(stderr, &expected_elem->msg);
			fprintmsg(stderr, msg);
			break;
		}

next:
		expected_elem = next_elem(s_container);
		tosend_elem = next_elem(c_container);
	}

	if (tosend_elem)
		goto exit;

	ret = fix_session_logout(session, NULL);
	if (ret) {
		fprintf(stderr, "Client Logout FAILED\n");
		goto exit;
	}

	fprintf(stderr, "Client Logout OK\n");

exit:
	fcontainer_free(c_container);
	fcontainer_free(s_container);
	fix_session_free(session);
	fclose(stream);

	return ret;
}

static const struct protocol_info protocols[] = {
	{ "fix",		fix_session_initiate },
	{ "fix42",		fix_session_initiate },
	{ "fix43",		fix_session_initiate },
	{ "fix44",		fix_session_initiate },
};

static const struct protocol_info *lookup_protocol_info(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(protocols); i++) {
		const struct protocol_info *proto_info = &protocols[i];

		if (!strcmp(proto_info->name, name))
			return proto_info;
	}
	return NULL;
}

static void usage(void)
{
	printf("\n  usage: trade client -f [filename] -h [hostname] -p [port] -c [protocol]\n\n");
	exit(EXIT_FAILURE);
}

static int socket_setopt(int sockfd, int level, int optname, int optval)
{
	return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

static enum fix_version strversion(const char *name)
{
	if (!strcmp(name, "fix42"))
		return FIX_4_2;
	else if (!strcmp(name, "fix43"))
		return FIX_4_3;
	else if (!strcmp(name, "fix44"))
		return FIX_4_4;

	return FIX_4_4;
}

int main(int argc, char *argv[])
{
	const struct protocol_info *proto_info;
	const char *filename = NULL;
	struct fix_session_cfg cfg;
	const char *proto = NULL;
	const char *host = NULL;
	struct sockaddr_in sa;
	int saved_errno = 0;
	struct hostent *he;
	int port = 0;
	int retval;
	char **ap;
	int opt;

	while ((opt = getopt(argc, argv, "f:h:p:c:")) != -1) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			break;
		case 'f':
			filename = optarg;
			break;
		case 'c':
			proto = optarg;
			break;
		case 'h':
			host = optarg;
			break;
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (!port || !proto || !filename || !host)
		usage();

	cfg.version	= strversion(proto);
	strncpy(cfg.sender_comp_id, "BUYSIDE", ARRAY_SIZE(cfg.sender_comp_id));
	strncpy(cfg.target_comp_id, "SELLSIDE", ARRAY_SIZE(cfg.target_comp_id));

	proto_info = lookup_protocol_info(proto);
	if (!proto_info) {
		printf("Unsupported protocol '%s'\n", proto);
		exit(EXIT_FAILURE);
	}

	he = gethostbyname(host);
	if (!he)
		error("Unable to look up %s (%s)", host, hstrerror(h_errno));

	for (ap = he->h_addr_list; *ap; ap++) {
		cfg.sockfd = socket(he->h_addrtype, SOCK_STREAM, IPPROTO_TCP);
		if (cfg.sockfd < 0) {
			saved_errno = errno;
			continue;
		}

		sa = (struct sockaddr_in) {
			.sin_family		= he->h_addrtype,
			.sin_port		= htons(port),
		};
		memcpy(&sa.sin_addr, *ap, he->h_length);

		if (connect(cfg.sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
			saved_errno = errno;
			close(cfg.sockfd);
			cfg.sockfd = -1;
			continue;
		}
		break;
	}

	if (cfg.sockfd < 0)
		error("Unable to connect to a socket (%s)", strerror(saved_errno));

	if (socket_setopt(cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
		die("cannot set socket option TCP_NODELAY");

	retval = proto_info->session_initiate(&cfg, filename);

	shutdown(cfg.sockfd, SHUT_RDWR);

	if (close(cfg.sockfd) < 0)
		die("close");

	return retval;
}
