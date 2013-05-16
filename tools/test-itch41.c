#include "libtrading/proto/itch41_message.h"

#include "libtrading/read-write.h"
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
#include <zlib.h>

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

#define BUFFER_SIZE	(1ULL << 20) /* 1 MB */
#define INFLATE_SIZE	(1ULL << 18) /* 256 KB */

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

static size_t buffer_inflate(struct buffer *buffer, int fd, z_stream *stream, const char *filename)
{
	unsigned char in[INFLATE_SIZE];
	ssize_t nr;
	int ret;

	nr = xread(fd, in, INFLATE_SIZE);
	if (nr < 0)
		return -1;

	if (!nr)
		return 0;

	stream->avail_in	= nr;
	stream->next_in		= in;
	stream->avail_out	= buffer_remaining(buffer);
	stream->next_out	= (void *) buffer_end(buffer);

	ret = inflate(stream, Z_NO_FLUSH);
	switch (ret) {
	case Z_STREAM_ERROR:
	case Z_DATA_ERROR:
	case Z_BUF_ERROR:
	case Z_MEM_ERROR:
	case Z_NEED_DICT:
		die("%s: %s: zlib error: %s\n", program, filename, stream->msg);
	default:
		break;
	}

	nr = buffer_remaining(buffer) - stream->avail_out;

	buffer->end += nr;

	return nr;
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
		char tmp[128];

		msg = (void *) tmp;

retry_size:
		if (buffer_size(buffer) < sizeof(u16)) {
			ssize_t nr;

			buffer_compact(buffer);

			nr = buffer_inflate(buffer, fd, &stream, filename);
			if (nr <= 0)
				break;

			if (show_progress)
				print_progress(fd, &st);

			goto retry_size;
		}

		buffer_advance(buffer, sizeof(u16));

retry_message:
		if (itch41_message_decode(buffer, msg) < 0) {
			ssize_t nr;

			buffer_compact(buffer);

			nr = buffer_inflate(buffer, fd, &stream, filename);
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

	release_stream(&stream);

	return 0;
}
