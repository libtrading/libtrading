#include "libtrading/proto/itch41_message.h"

#include "libtrading/buffer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

static char *program;

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
	printf("usage: %s [-v] <itch-file>\n", basename(program));

	exit(EXIT_FAILURE);
}

#define BUFFER_SIZE (1ULL << 20) /* 1 MB */

static void print_progress(int fd, struct stat *st)
{
	off_t offset;

	offset = lseek(fd, 0, SEEK_CUR);

	fprintf(stderr, "Decoding messages: %3u%%\r", (unsigned int)(offset * 100 / st->st_size));
	fflush(stderr);
}

int main(int argc, char *argv[])
{
	struct buffer *buffer;
	const char *filename;
	bool show_progress;
	struct stat st;
	bool verbose;
	int opt;
	int fd;

	program = argv[0];

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
		char tmp[128];

		msg = (void *) tmp;

retry_size:
		if (buffer_size(buffer) < sizeof(uint16_t)) {
			ssize_t nr;

			buffer_compact(buffer);

			nr = buffer_read(buffer, fd);
			if (nr <= 0)
				break;

			if (show_progress)
				print_progress(fd, &st);

			goto retry_size;
		}

		buffer_advance(buffer, sizeof(uint16_t));

retry_message:
		if (itch41_message_decode(buffer, msg) < 0) {
			ssize_t nr;

			buffer_compact(buffer);

			nr = buffer_read(buffer, fd);
			if (nr <= 0)
				break;

			if (show_progress)
				print_progress(fd, &st);

			goto retry_message;
		}

		if (verbose)
			printf("%c", msg->MessageType);
	}

	buffer_delete(buffer);

	if (close(fd) < 0)
		die("%s: %s: %s\n", program, filename, strerror(errno));

	return 0;
}
