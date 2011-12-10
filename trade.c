#include "trading/builtins.h"
#include "trading/core.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef int (*builtin_cmd_fn)(int argc, char *argv[]);

struct builtin_cmd {
	const char		*name;
	builtin_cmd_fn		cmd_fn;
};

#define DEFINE_BUILTIN(n, c) { .name = n, .cmd_fn = c }

static struct builtin_cmd builtins[] = {
	DEFINE_BUILTIN("client",	cmd_client),
	DEFINE_BUILTIN("server",	cmd_server),
};

static struct builtin_cmd *parse_builtin_cmd(int argc, char *argv[])
{
	int i;

	for (i = 0; i < ARRAY_SIZE(builtins); i++) {
		struct builtin_cmd *cmd = &builtins[i];

		if (strcmp(argv[1], cmd->name) == 0)
			return cmd;
	}
	return NULL;
}

static void usage(void)
{
	printf("\n usage: fix COMMAND [ARGS]\n");
	printf("\n The commands are:\n");
	printf("   client    FIX client\n");
	printf("   server    FIX server\n");
	printf("\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	struct builtin_cmd *cmd;

	if (argc < 2)
		usage();

	cmd = parse_builtin_cmd(argc, argv);
	if (!cmd)
		usage();

	return cmd->cmd_fn(argc, argv);
}
