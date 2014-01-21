#include <libtrading/proto/fix_message.h>
#include <libtrading/buffer.h>
#include <libtrading/compat.h>
#include <libtrading/time.h>

#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[])
{
	struct buffer *head_buf, *body_buf;
	struct timespec start, end;
	struct fix_message *msg;
	uint64_t elapsed_nsec;
	char now[64];
	int count;
	int i;

	if (argc != 2) {
		fprintf(stderr, "usage: %s [message count]\n", basename(argv[0]));
		return EXIT_FAILURE;
	}

	count = strtol(argv[1], NULL, 10);

	snprintf(now, 64, "20121227-11:20:43");
	head_buf = buffer_new(4096);
	body_buf = buffer_new(4096);

	msg = fix_message_new();

	msg->begin_string	= "FIX.4.2";
	msg->type		= FIX_MSG_TYPE_NEW_ORDER_SINGLE;
	msg->sender_comp_id	= "DLD_TEX";
	msg->target_comp_id	= "TEX_DLD";
	msg->msg_seq_num	= 499650;
	msg->head_buf		= head_buf;
	msg->body_buf		= body_buf;
	msg->str_now		= now;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0; i < count; i++) {
		buffer_reset(msg->head_buf);
		buffer_reset(msg->body_buf);
		fix_message_unparse(msg);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed_nsec = timespec_delta(&start, &end);

	printf("%-10s %d %f µs/message\n", "encode", count, (double)elapsed_nsec/(double)count/1000.0);

	fix_message_free(msg);
	buffer_delete(head_buf);
	buffer_delete(body_buf);

	return EXIT_SUCCESS;
}
