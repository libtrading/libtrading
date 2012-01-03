#ifndef LIBTRADING_DIE_H
#define LIBTRADING_DIE_H

#define die(format, args...) do_die("%s: " format, __func__, ## args)

void do_die(const char *format, ...) __attribute__ ((noreturn))
    __attribute__ ((format (printf, 1, 2)));

void error(const char *format, ...) __attribute__ ((noreturn))
    __attribute__ ((format (printf, 1, 2)));

#endif /* LIBTRADING_DIE_H */
