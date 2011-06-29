#ifndef FIX_DIE_H
#define FIX_DIE_H

#define die(format, args...) do_die("%s: " format, __func__, ## args)

void do_die(const char *format, ...) __attribute__ ((noreturn))
    __attribute__ ((format (printf, 1, 2)));

#endif /* FIX_DIE_H */
