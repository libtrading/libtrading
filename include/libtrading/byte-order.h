#ifndef LIBTRADING_BYTE_ORDER_H
#define LIBTRADING_BYTE_ORDER_H

#include "libtrading/types.h"

#include <arpa/inet.h>

static inline be16 cpu_to_be16(u16 value)
{
	return htons(value);
}

static inline be32 cpu_to_be32(u32 value)
{
	return htonl(value);
}

static inline u16 be16_to_cpu(be16 value)
{
	return ntohs(value);
}

static inline u32 be32_to_cpu(be32 value)
{
	return ntohl(value);
}

#endif
