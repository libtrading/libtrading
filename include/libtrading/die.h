#ifndef LIBTRADING_DIE_H
#define LIBTRADING_DIE_H

#ifdef __cplusplus
extern "C" {
#endif

#define die(format, args...) do_die("%s: " format, __func__, ## args)

void do_die(const char *format, ...) __attribute__ ((noreturn))
    __attribute__ ((format (printf, 1, 2)));

void error(const char *format, ...) __attribute__ ((noreturn))
    __attribute__ ((format (printf, 1, 2)));

#ifdef __cplusplus
}
#endif

#endif
