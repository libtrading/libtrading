#ifndef	LIBTRADING_TRACE_H
#define	LIBTRADING_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef	CONFIG_PROBES
#include "libtrading/probes.h"

#define	TRACE(probe)	probe
#else
#define	TRACE(probe)
#endif

#ifdef __cplusplus
}
#endif

#endif
