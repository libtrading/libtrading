#include "trading/die.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

void do_die(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);

	printf("\n");

	exit(EXIT_FAILURE);
}

void error(const char *format, ...)
{
	va_list ap;

	printf("error: ");
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);

	printf("\n");

	exit(EXIT_FAILURE);
}
