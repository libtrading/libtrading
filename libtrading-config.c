#include <stdbool.h>
#include <getopt.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *program;

static const char *usage_fmt =
"Usage: %s <option>\n"
"\n"
"Options:\n"
"  --version  Print libtrading version.\n"
"  --cflags   C compiler flags for files that include libtrading headers.\n"
"  --ldflags  Linker flags.\n"
"  --libs     Libraries needed to link against libtrading.\n";

static void usage(void)
{
	fprintf(stderr, usage_fmt, program);
	exit(1);
}

static const struct option options[] = {
	{ "version",	no_argument, NULL, 'v' },
	{ "ldflags",	no_argument, NULL, 'd' },
	{ "cflags",	no_argument, NULL, 'c' },
	{ "libs",	no_argument, NULL, 'l' },
};

static bool version;
static bool ldflags;
static bool cflags;
static bool libs;

static void parse_args(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt_long(argc, argv, "cdlv", options, NULL)) != -1) {
		switch (opt) {
		case 'v':
			version	= true;
			break;
		case 'd':
			ldflags	= true;
			break;
		case 'c':
			cflags	= true;
			break;
		case 'l':
			libs	= true;
			break;
		default:
			usage();
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	program = basename(argv[0]);

	if (argc == 1)
		usage();

	parse_args(argc, argv);

	if (version) {
		printf("%s\n", LIBTRADING_VERSION);
		return 0;
	}

	if (cflags)
		printf("-I" PREFIX "/include ");

	if (ldflags)
		printf("-L" PREFIX "/lib ");

	if (libs)
		printf("-ltrading -lz ");

	printf("\n");

	return 0;
}
