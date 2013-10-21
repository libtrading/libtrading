#ifndef	LIBTRADING_TRACE_H
#define	LIBTRADING_TRACE_H

#ifdef	CONFIG_PROBES
#include "libtrading/probes.h"

#define	TRACE(probe)	probe
#else
#define	TRACE(probe)
#endif

#endif
