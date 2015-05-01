#include <libtrading/proto/fix_message.h>
#include <libtrading/proto/fix_session.h>
#include <libtrading/buffer.h>
#include <libtrading/compat.h>
#include <libtrading/time.h>

#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static unsigned long fix_new_order_single_fields(struct fix_field *fields)
{
	unsigned long nr = 0;

	fields[nr++] = FIX_STRING_FIELD(TransactTime, "20121227-11:20:43.000");
	fields[nr++] = FIX_STRING_FIELD(ClOrdID, "ClOrdID");
	fields[nr++] = FIX_STRING_FIELD(Symbol, "Symbol");
	fields[nr++] = FIX_INT_FIXED_FIELD(OrderQty, 100, 4);
	fields[nr++] = FIX_STRING_FIELD(OrdType, "2");
	fields[nr++] = FIX_STRING_FIELD(Side, "1");
	fields[nr++] = FIX_FLOAT_FIXED_FIELD(Price, 100.2, 7);

	return nr;
}

static struct fix_message *new_order_single_message(void)
{
	struct fix_field *fields = NULL;
	struct fix_message *msg;
	size_t nr;

	fields = calloc(FIX_MAX_FIELD_NUMBER, sizeof(struct fix_field));

	nr = fix_new_order_single_fields(fields);

	msg = fix_message_new();

	msg->begin_string	= "FIX.4.2";
	msg->type		= FIX_MSG_TYPE_NEW_ORDER_SINGLE;
	msg->sender_comp_id	= "DLD_TEX";
	msg->target_comp_id	= "TEX_DLD";
	msg->msg_seq_num	= 9650;
	msg->str_now		= (char *)"20150101-00:00:00.000";
	msg->fields		= fields;
	msg->nr_fields		= nr;

	return msg;
}

int main(int argc, char *argv[])
{
	struct fix_message_unparse_context unparse_ctx;
	struct buffer *head_buf, *body_buf;
	struct timespec start, end;
	struct fix_message *rx_msg;
	struct fix_message *msg;
	struct buffer *rx_buf;
	uint64_t elapsed_nsec;
	int count;
	int i;

	if (argc != 2) {
		fprintf(stderr, "usage: %s [message count]\n", basename(argv[0]));
		return EXIT_FAILURE;
	}

	count = strtol(argv[1], NULL, 10);

	head_buf = buffer_new(4096);
	body_buf = buffer_new(4096);
	rx_buf = buffer_new(4096);

	msg = new_order_single_message();

	msg->head_buf		= head_buf;
	msg->body_buf		= body_buf;

	buffer_reset(msg->head_buf);
	buffer_reset(msg->body_buf);
	fix_message_pre_unparse_fixed(msg, &unparse_ctx);

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0; i < count; i++) {
		unparse_ctx.msg_seq_num.int_value++;
		msg->fields[3].int_value++;
		msg->fields[6].float_value += 0.1;
		fix_message_replace_fixed(msg, &unparse_ctx);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed_nsec = timespec_delta(&start, &end);

	printf("%-10s %d %f µs/message\n", "format/replace", count, (double)elapsed_nsec/(double)count/1000.0);
	// printf("%.*s%.*s\n", buffer_size(head_buf), buffer_start(head_buf), buffer_size(body_buf), buffer_start(body_buf));

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0; i < count; i++) {
		buffer_reset(msg->head_buf);
		buffer_reset(msg->body_buf);
		fix_message_unparse(msg);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed_nsec = timespec_delta(&start, &end);

	printf("%-10s %d %f µs/message\n", "format", count, (double)elapsed_nsec/(double)count/1000.0);

	rx_msg = fix_message_new();

	buffer_append(rx_buf, head_buf);
	buffer_append(rx_buf, body_buf);

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0; i < count; i++) {
		rx_buf->start = 0;
		fix_message_parse(rx_msg, &fix_dialects[FIX_4_2], rx_buf);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed_nsec = timespec_delta(&start, &end);

	printf("%-10s %d %f µs/message\n", "parse", count, (double)elapsed_nsec/(double)count/1000.0);

	fix_message_free(msg);
	buffer_delete(head_buf);
	buffer_delete(body_buf);

	return EXIT_SUCCESS;
}
