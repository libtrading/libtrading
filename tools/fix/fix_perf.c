#include <libtrading/proto/fix_template.h>
#include <libtrading/proto/fix_message.h>
#include <libtrading/proto/fix_session.h>
#include <libtrading/buffer.h>
#include <libtrading/compat.h>
#include <libtrading/time.h>

#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

static unsigned long fix_new_order_single_fields(struct fix_field *fields, const char *now)
{
	unsigned long nr = 0;

	fields[nr++] = FIX_STRING_FIELD(50, "AAA");
	fields[nr++] = FIX_STRING_FIELD(57, "CME");
	fields[nr++] = FIX_STRING_FIELD(142, "CH");
	fields[nr++] = FIX_STRING_FIELD(Account, "1234567");
	fields[nr++] = FIX_CHAR_FIELD(21, 'Y');
	fields[nr++] = FIX_CHAR_FIELD(OrdType, '2');
	fields[nr++] = FIX_CHAR_FIELD(1028, 'Y');
	fields[nr++] = FIX_CHAR_FIELD(1091, 'N');
	fields[nr++] = FIX_STRING_FIELD(167, "FUT");
	fields[nr++] = FIX_CHAR_FIELD(204, '0');
	fields[nr++] = FIX_CHAR_FIELD(9702, '4');

	fields[nr++] = FIX_CHAR_FIELD(Side, '1');
	fields[nr++] = FIX_STRING_FIELD(Symbol, "ES");
	fields[nr++] = FIX_STRING_FIELD(107, "ESM5");
	fields[nr++] = FIX_STRING_FIELD(TransactTime, now);
	fields[nr++] = FIX_STRING_FIELD(ClOrdID, "ClOrdID");
	fields[nr++] = FIX_STRING_FIELD(9717, "ClOrdID");
	fields[nr++] = FIX_FLOAT_FIELD(OrderQty, 1);
	fields[nr++] = FIX_FLOAT_FIELD(Price, 10000);

	return nr;
}

static struct fix_message *new_order_single_message(void)
{
	struct fix_field *fields = NULL;
	struct fix_message *msg;
	char now[64];
	size_t nr;

	snprintf(now, 64, "20121227-11:20:43.000");

	fields = calloc(FIX_MAX_FIELD_NUMBER, sizeof(struct fix_field));

	nr = fix_new_order_single_fields(fields, now);

	msg = fix_message_new();

	msg->begin_string	= "FIX.4.2";
	msg->type		= FIX_MSG_TYPE_NEW_ORDER_SINGLE;
	msg->sender_comp_id	= "DLD_TEX";
	msg->target_comp_id	= "TEX_DLD";
	msg->msg_seq_num	= 499650;
	msg->str_now		= now;
	msg->fields		= fields;
	msg->nr_fields		= nr;

	return msg;
}

static unsigned long fix_new_order_single_const_fields(struct fix_field *fields)
{
	unsigned long nr = 0;

	fields[nr++] = FIX_STRING_FIELD(50, "AAA");
	fields[nr++] = FIX_STRING_FIELD(57, "CME");
	fields[nr++] = FIX_STRING_FIELD(142, "CH");
	fields[nr++] = FIX_STRING_FIELD(Account, "1234567");
	fields[nr++] = FIX_CHAR_FIELD(21, 'Y');
	fields[nr++] = FIX_CHAR_FIELD(OrdType, '2');
	fields[nr++] = FIX_CHAR_FIELD(1028, 'Y');
	fields[nr++] = FIX_CHAR_FIELD(1091, 'N');
	fields[nr++] = FIX_STRING_FIELD(167, "FUT");
	fields[nr++] = FIX_CHAR_FIELD(204, '0');
	fields[nr++] = FIX_CHAR_FIELD(9702, '4');

	return nr;
}

static unsigned long fix_new_order_single_variable_fields(struct fix_field *fields)
{
	unsigned long nr = 0;

	fields[nr++] = FIX_CHAR_FIELD(Side, '1');
	fields[nr++] = FIX_STRING_FIELD(Symbol, "ES");
	fields[nr++] = FIX_STRING_FIELD(107, "ESM5");
	fields[nr++] = FIX_STRING_FIELD(ClOrdID, "ClOrdID");
	fields[nr++] = FIX_STRING_FIELD(9717, "ClOrdID");
	fields[nr++] = FIX_FLOAT_FIELD(OrderQty, 1);
	fields[nr++] = FIX_FLOAT_FIELD(Price, 10000);

	return nr;
}

static struct fix_template *new_order_single_template(char *now)
{
	struct fix_template_cfg cfg;
	struct fix_template *template;

	template = fix_template_new();
	cfg.begin_string = "FIX.4.2";
	cfg.msg_type = FIX_MSG_TYPE_NEW_ORDER_SINGLE;
	cfg.sender_comp_id = "DLD_TEX";
	cfg.target_comp_id = "TEX_DLD";
	cfg.nr_const_fields = fix_new_order_single_const_fields(&cfg.const_fields[0]);
	fix_template_prepare(template, &cfg);

	template->nr_fields = fix_new_order_single_variable_fields(&template->fields[0]);

	return template;
}

static void fix_message_unparse_benchmark(const int count, struct buffer *head_buf, struct buffer *body_buf)
{
	struct timespec start, end;
	struct fix_message *msg;
	uint64_t elapsed_nsec;
	int i;

	msg = new_order_single_message();

	msg->head_buf = head_buf;
	msg->body_buf = body_buf;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0; i < count; i++) {
		buffer_reset(msg->head_buf);
		buffer_reset(msg->body_buf);
		fix_message_unparse(msg);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed_nsec = timespec_delta(&start, &end);
	printf("%-10s %d %f µs/message\n", "format      ", count, (double)elapsed_nsec/(double)count/1000.0);

	fix_message_free(msg);
}

static void fix_template_unparse_benchmark(const int count, struct buffer *rx_buf, struct fix_message *rx_msg)
{
	struct fix_template *template;
	struct fix_session session;
	struct timespec start, end;
	uint64_t elapsed_nsec;
	int templ_parse;
	int i;

	template = new_order_single_template(session.str_now);
	strncpy(session.str_now, "20121227-11:20:43.000", 64);
	fix_template_update_time(template, session.str_now);
	session.out_msg_seq_num = 499650;
	session.sender_comp_id = "ABC_DEF";

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0; i < count; i++) {
		session.out_msg_seq_num++;
		fix_template_unparse(template, &session);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed_nsec = timespec_delta(&start, &end);
	printf("%-10s %d %f µs/message\n", "format/templ", count, (double)elapsed_nsec/(double)count/1000.0);
	buffer_printf(rx_buf, "%.*s", (int)buffer_size(&template->buf), buffer_start(&template->buf));
	// printf("%.*s\n", (int)buffer_size(&template->buf), buffer_start(&template->buf));

	templ_parse = fix_message_parse(rx_msg, &fix_dialects[FIX_4_2], rx_buf, 0);
	printf("template unparse status: %i\n", templ_parse);

	fix_template_free(template);
}

static void fix_message_parse_benchmark(const int count, struct buffer *rx_buf, struct fix_message *rx_msg, int flags) 
{
	struct timespec start, end;
	uint64_t elapsed_nsec;
	int i;

	rx_buf->start = 0;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0; i < count; i++) {
		rx_buf->start = 0;
		fix_message_parse(rx_msg, &fix_dialects[FIX_4_2], rx_buf, flags);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed_nsec = timespec_delta(&start, &end);

	printf("%-10s %d %f µs/message\n", (flags & FIX_PARSE_FLAG_NO_CSUM ? "parse/fast  " : "parse       "), count, (double)elapsed_nsec/(double)count/1000.0);
}

int main(int argc, char *argv[])
{
	struct buffer *head_buf, *body_buf;
	struct fix_message *rx_msg;
	struct buffer *rx_buf;
	int count;

	if (argc != 2) {
		fprintf(stderr, "usage: %s [message count]\n", basename(argv[0]));
		return EXIT_FAILURE;
	}

	count = strtol(argv[1], NULL, 10);

	head_buf = buffer_new(4096);
	body_buf = buffer_new(4096);
	rx_buf = buffer_new(4096);
	rx_msg = fix_message_new();

	fix_message_unparse_benchmark(count, head_buf, body_buf);
	fix_template_unparse_benchmark(count, rx_buf, rx_msg);
	fix_message_parse_benchmark(count, rx_buf, rx_msg, FIX_PARSE_FLAG_NO_CSUM);
	fix_message_parse_benchmark(count, rx_buf, rx_msg, 0);

	fix_message_free(rx_msg);
	buffer_delete(rx_buf);
	buffer_delete(head_buf);
	buffer_delete(body_buf);

	return EXIT_SUCCESS;
}