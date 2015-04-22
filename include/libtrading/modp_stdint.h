/* vi: set ft=c expandtab shiftwidth=4 tabstop=4: */
#ifndef MODP_STDINT_H_
#define MODP_STDINT_H_

/**
 * \file modp_stdint.h
 * \brief An attempt to make stringencoders compile under windows
 *
 * This attempts to define various integral types that are normally
 * defined in stdint.h and stdbool.h which oddly don't exit on
 * windows.
 *
 * Please file bugs or patches if it doesn't work!
 */

#include <string.h>

#ifndef _WIN32
#  include <stdint.h>
#  include <stdbool.h>
#else
/* win64 is llp64 so these are the same for 32/64bit
   so no check for _WIN64 is required.
 */
  typedef unsigned char uint8_t;
  typedef signed char int8_t;
  typedef unsigned short uint16_t;
  typedef signed short int16_t;
  typedef unsigned int uint32_t;
  typedef signed int int32_t;
  typedef unsigned __int64 uint64_t;
  typedef signed __int64 int64_t;

/* windows doesn't do C99 and stdbool */

#ifndef __cplusplus
typedef unsigned char bool;
#define true 1
#define false 0
#endif

#endif /* _WIN32 */
#endif /* MODP_STDINT_H_ */
