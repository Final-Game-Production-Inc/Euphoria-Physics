
#ifndef SPU_PRINTF_HELPER_H
#define SPU_PRINTF_HELPER_H

#include <spu_printf.h>

// Automatically append job names to spu_printf's
#define INSERT_JOB_NAME		(1)

#if INSERT_JOB_NAME
	#define RAGE_SPU_PRINTF	spu_printf_with_jobname
#else
	#define RAGE_SPU_PRINTF	spu_printf
#endif

// String should concatenate automatically
#define spu_printf_with_jobname(... )	spu_printf("["DEBUG_SPU_JOB_NAME"] " __VA_ARGS__)

#endif // SPU_PRINTF_HELPER_H
