#include "builtin-cmds.h"

#include "libtrading/array.h"

#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char *program;

typedef int (*builtin_cmd_fn)(int argc, char *argv[]);

struct builtin_cmd {
	const char		*name;
	builtin_cmd_fn		cmd_fn;
};

#define DEFINE_BUILTIN(n, c) { .name = n, .cmd_fn = c }

static struct builtin_cmd builtins[] = {
	DEFINE_BUILTIN("check",		cmd_check),
};

static struct builtin_cmd *parse_builtin_cmd(int argc, char *argv[])
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(builtins); i++) {
		struct builtin_cmd *cmd = &builtins[i];

		if (strcmp(argv[1], cmd->name) == 0)
			return cmd;
	}

	return NULL;
}

static void usage(void)
{
#define FMT								\
"\n usage: %s COMMAND [ARGS]\n"						\
"\n The commands are:\n"						\
"   check     Check market data file\n"					\
"\n"
	fprintf(stderr, FMT, program);

	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	struct builtin_cmd *cmd;

	program = basename(argv[0]);

	if (argc < 2)
		usage();

	cmd = parse_builtin_cmd(argc, argv);
	if (!cmd)
		usage();

	return cmd->cmd_fn(argc - 1, argv + 1);
}
