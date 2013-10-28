#include "libtrading/itoa.h"

#define	ITOA_TAB_SIZE	100UL
#define	ITOA_TAB_LOG	2UL

static const char itoa_tab[ITOA_TAB_SIZE * ITOA_TAB_LOG + 1] = {
	"0001020304050607080910111213141516171819"
	"2021222324252627282930313233343536373839"
	"4041424344454647484950515253545556575859"
	"6061626364656667686970717273747576777879"
	"8081828384858687888990919293949596979899"
};

static inline unsigned int uilog_10(unsigned int n)
{
	switch (n) {
	case         0 ...         9:	return 1;
	case        10 ...        99:	return 2;
	case       100 ...       999:	return 3;
	case      1000 ...      9999:	return 4;
	case     10000 ...     99999:	return 5;
	case    100000 ...    999999:	return 6;
	case   1000000 ...   9999999:	return 7;
	case  10000000 ...  99999999:	return 8;
	case 100000000 ... 999999999:	return 9;
	default:			return 10;
	}
}

static int uitoa_general(unsigned int n, char *s)
{
	unsigned log = uilog_10(n);
	char *p = s + log;
	const char *src;

	while (n >= ITOA_TAB_SIZE) {
		src = itoa_tab + ITOA_TAB_LOG * (n % ITOA_TAB_SIZE);
		p -= ITOA_TAB_LOG; n /= ITOA_TAB_SIZE;
		memcpy(p, src, ITOA_TAB_LOG);
	}

	src = itoa_tab + (n + 1) * ITOA_TAB_LOG - 1; p--;
	switch (n) {
	default:
		*p-- = *src--;
	case 0 ... 9:
		*p-- = *src--;
	}

	return log;
}

int uitoa(unsigned int n, char *s)
{
	const char *src = itoa_tab + ITOA_TAB_LOG * n;
	char *p = s;

	switch (n) {
	case 10 ... 99:
		*p++ = src[0];
	case 0  ...  9:
		*p++ = src[1];
		return p - s;
	default:
		break;
	}

	return uitoa_general(n, p);
}

int itoa(int n, char *s)
{
	unsigned int un = n;
	char *p = s;

	if (n < 0) {
		*p++ = '-';
		un = -n;
	}

	return uitoa(un, p) + p - s;
}

int i64toa(int64_t n, char *s)
{
	if (n > INT_MAX || n < INT_MIN)
		return sprintf(s, "%" PRId64, n);
	else
		return itoa(n, s);
}

int checksumtoa(int n, char *s)
{
	const char *src = itoa_tab + ITOA_TAB_LOG * n;
	char *p = s;

	switch (n) {
	case 0 ... 99:
		*p++ = '0';
		*p++ = *src++;
		*p++ = *src++;
		break;
	default:
		return uitoa_general(n, s);
	}

	return p - s;
}
