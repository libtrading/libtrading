#ifndef LIBTRADING_TYPES_H
#define LIBTRADING_TYPES_H

#include <stdint.h>

#ifdef __CHECKER__
#define bitwise __attribute__((bitwise))
#define force	__attribute__((force))
#else
#define bitwise
#define force
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint16_t bitwise le16;
typedef uint32_t bitwise le32;
typedef uint64_t bitwise le64;

typedef uint16_t bitwise be16;
typedef uint32_t bitwise be32;
typedef uint64_t bitwise be64;

#endif /* LIBTRADING_TYPES_H */
