#include "fix/builtins.h"
#include "fix/message.h"
#include "fix/session.h"
#include "fix/die.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

int cmd_client(int argc, char *argv[])
{
	struct fix_message *response;
	struct fix_message logon_msg;
	struct fix_session *session;
	struct sockaddr_in sa;
	int sockfd;
	int retval;
	int err;
	
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0)
		die("can not create socket");

	sa = (struct sockaddr_in) {
		.sin_family		= AF_INET,
		.sin_port		= htons(9000),
	};

	err = inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);
	if (err <= 0)
		die("inet_pton failed");

	if (connect(sockfd, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
		die("connect failed");

	session		= fix_session_new(sockfd, FIX_4_4, "BUYSIDE", "SELLSIDE");

	logon_msg	= (struct fix_message) {
		.msg_type	= Logon,
	};
	fix_session_send(session, &logon_msg, 0);

	response	= fix_message_recv(sockfd, 0);
	if (response && fix_message_type_is(response, Logon)) {
		printf("Logon OK\n");
		retval = 0;
	} else {
		printf("Logon FAILED\n");
		retval = 1;
	}

	if (response)
		fix_message_free(response);

	fix_session_free(session);

	shutdown(sockfd, SHUT_RDWR);

	if (close(sockfd) < 0)
		die("close");

	return retval;
}
