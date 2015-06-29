#ifndef LIBTRADING_BYTE_ORDER_H
#define LIBTRADING_BYTE_ORDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <endian.h>

/*
 * Little Endian
 */

static inline le16 cpu_to_le16(u16 value)
{
	return htole16(value);
}

static inline le32 cpu_to_le32(u32 value)
{
	return htole32(value);
}

static inline le64 cpu_to_le64(u64 value)
{
	return htole64(value);
}

static inline u16 le16_to_cpu(le16 value)
{
	return le16toh(value);
}

static inline u32 le32_to_cpu(le32 value)
{
	return le32toh(value);
}

static inline u64 le64_to_cpu(le64 value)
{
	return le64toh(value);
}

/*
 * Big Endian
 */

static inline be16 cpu_to_be16(u16 value)
{
	return htobe16(value);
}

static inline be32 cpu_to_be32(u32 value)
{
	return htobe32(value);
}

static inline be64 cpu_to_be64(u64 value)
{
	return htobe64(value);
}

static inline u16 be16_to_cpu(be16 value)
{
	return be16toh(value);
}

static inline u32 be32_to_cpu(be32 value)
{
	return be32toh(value);
}

static inline u64 be64_to_cpu(be64 value)
{
	return be64toh(value);
}

#ifdef __cplusplus
}
#endif

#endif
