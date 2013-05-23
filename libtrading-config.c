#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>

static const char *program;

static const char *usage_fmt =
"Usage: %s <option>\n"
"\n"
"Options:\n"
"  --cflags   C compiler flags for files that include libtrading headers.\n"
"  --ldflags  Linker flags.\n"
"  --libs     Libraries needed to link against libtrading.\n";

static void usage(void)
{
	fprintf(stderr, usage_fmt, program);
	exit(1);
}

int main(int argc, char *argv[])
{
	bool ldflags = false;
	bool cflags = false;
	bool libs = false;
	int i;

	program = basename(argv[0]);

	if (argc == 1)
		usage();

	for (i = 1; i < argc; i++) {
		const char *opt = argv[i];

		if (!strcmp(opt, "--cflags"))
			cflags = true;
		else if (!strcmp(opt, "--ldflags"))
			ldflags = true;
		else if (!strcmp(opt, "--libs"))
			libs = true;
		else
			usage();
	}

	if (cflags)
		printf("-I" PREFIX "/include ");

	if (ldflags)
		printf("-L" PREFIX "/lib ");

	if (libs)
		printf("-ltrading ");

	printf("\n");

	return 0;
}
