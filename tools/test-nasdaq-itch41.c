#include "libtrading/proto/nasdaq_itch41_message.h"

#include "libtrading/read-write.h"
#include "libtrading/buffer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <libgen.h>
#include <locale.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <zlib.h>

static char *program;

static uint64_t stats[26];

static void die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}

static void usage(void)
{
	printf("usage: %s [-v] <itch-file>\n", program);

	exit(EXIT_FAILURE);
}

#define BUFFER_SIZE	(1ULL << 20) /* 1 MB */

static void print_stat(const char *name, u8 msg_type)
{
	fprintf(stdout, "%'14.0f  %s\n", (double) stats[msg_type - 'A'], name);
}

static void print_stats(const char *filename)
{
	printf(" Message type stats for '%s':\n\n", filename);

	print_stat("Timestamp - Seconds", ITCH41_MSG_TIMESTAMP_SECONDS);
        print_stat("System Event", ITCH41_MSG_SYSTEM_EVENT);
        print_stat("Stock Directory", ITCH41_MSG_STOCK_DIRECTORY);
        print_stat("Stock Trading Action", ITCH41_MSG_STOCK_TRADING_ACTION);
        print_stat("REG SHO Restriction", ITCH41_MSG_REG_SHO_RESTRICTION);
        print_stat("Market Participant Position", ITCH41_MSG_MARKET_PARTICIPANT_POS);
        print_stat("Add Order", ITCH41_MSG_ADD_ORDER);
        print_stat("Add Order - MPID Attribution", ITCH41_MSG_ADD_ORDER_MPID);
        print_stat("Order Executed", ITCH41_MSG_ORDER_EXECUTED);
        print_stat("Order Executed With Price", ITCH41_MSG_ORDER_EXECUTED_WITH_PRICE);
        print_stat("Order Cancel", ITCH41_MSG_ORDER_CANCEL);
        print_stat("Order Delete", ITCH41_MSG_ORDER_DELETE);
        print_stat("Order Replace", ITCH41_MSG_ORDER_REPLACE);
        print_stat("Trade (non-cross)", ITCH41_MSG_TRADE);
        print_stat("Cross Trade", ITCH41_MSG_CROSS_TRADE);
        print_stat("Broken Trade", ITCH41_MSG_BROKEN_TRADE);
        print_stat("NOII", ITCH41_MSG_NOII);

	printf("\n");
}

static void print_progress(int fd, struct stat *st)
{
	off_t offset;

	offset = lseek(fd, 0, SEEK_CUR);

	fprintf(stderr, "Decoding messages: %3u%%\r", (unsigned int)(offset * 100 / st->st_size));
	fflush(stderr);
}

static void init_stream(z_stream *stream)
{
	memset(stream, 0, sizeof(*stream));

	if (inflateInit2(stream, 15 + 32) != Z_OK)
		die("%s: unable to initialize zlib\n", program);
}

static void release_stream(z_stream *stream)
{
	inflateEnd(stream);
}

int main(int argc, char *argv[])
{
	struct buffer *buffer;
	const char *filename;
	bool show_progress;
	z_stream stream;
	struct stat st;
	bool verbose;
	int opt;
	int fd;

	setlocale(LC_ALL, "");

	program = basename(argv[0]);

	if (argc < 2)
		usage();

	show_progress	= true;
	verbose		= false;

	while ((opt = getopt(argc, argv, "v")) != -1) {
		switch (opt) {
		case 'v':
			verbose		= true;
			show_progress	= false;
			break;
		default:
			usage();
			break;
		}
	}

	init_stream(&stream);

	filename = argv[optind];

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		die("%s: %s: %s\n", program, filename, strerror(errno));

	if (fstat(fd, &st) < 0)
		die("%s: %s: %s\n", program, filename, strerror(errno));

	buffer = buffer_new(BUFFER_SIZE);
	if (!buffer)
		die("%s: %s\n", program, strerror(errno));

	for (;;) {
		struct itch41_message *msg;

retry_size:
		if (buffer_size(buffer) < sizeof(u16)) {
			ssize_t nr;

			buffer_compact(buffer);

			nr = buffer_inflate(buffer, fd, &stream);
			if (nr < 0)
				die("%s: zlib error\n", program);

			if (!nr)
				break;

			if (show_progress)
				print_progress(fd, &st);

			goto retry_size;
		}

		buffer_advance(buffer, sizeof(u16));

retry_message:
		msg = itch41_message_decode(buffer);
		if (!msg) {
			ssize_t nr;

			buffer_compact(buffer);

			nr = buffer_inflate(buffer, fd, &stream);
			if (nr < 0)
				die("%s: zlib error\n", program);

			if (!nr)
				break;

			if (show_progress)
				print_progress(fd, &st);

			goto retry_message;
		}

		if (verbose)
			printf("%c", msg->MessageType);

		stats[msg->MessageType - 'A']++;
	}

	printf("\n");

	buffer_delete(buffer);

	if (close(fd) < 0)
		die("%s: %s: %s\n", program, filename, strerror(errno));

	release_stream(&stream);

	print_stats(filename);

	return 0;
}
