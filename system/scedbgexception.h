/* SCE CONFIDENTIAL
 * PlayStation(R)4 Programmer Tool Runtime Library Release 02.000.071
 * Copyright (C) 2013 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#ifndef _SCE_EXCEPTION_H_
#define _SCE_EXCEPTION_H_

#include <kernel.h>

__BEGIN_DECLS

#ifndef _SYS_UCONTEXT_H_

typedef struct __mcontext {
	__register_t	mc_onstack;
	__register_t	mc_rdi;
	__register_t	mc_rsi;
	__register_t	mc_rdx;
	__register_t	mc_rcx;
	__register_t	mc_r8;
	__register_t	mc_r9;
	__register_t	mc_rax;
	__register_t	mc_rbx;
	__register_t	mc_rbp;
	__register_t	mc_r10;
	__register_t	mc_r11;
	__register_t	mc_r12;
	__register_t	mc_r13;
	__register_t	mc_r14;
	__register_t	mc_r15;
	__uint32_t	mc_trapno;
	__uint16_t	mc_fs;
	__uint16_t	mc_gs;
	__register_t	mc_addr;
	__uint32_t	mc_flags;
	__uint16_t	mc_es;
	__uint16_t	mc_ds;
	__register_t	mc_err;
	__register_t	mc_rip;
	__register_t	mc_cs;
	__register_t	mc_rflags;
	__register_t	mc_rsp;
	__register_t	mc_ss;

	long	mc_len;
	long	mc_fpformat;
	long	mc_ownedfp;
	long	mc_fpstate[104] __aligned(64);

	__register_t	mc_fsbase;
	__register_t	mc_gsbase;

	long	mc_spare[6];
} mcontext_t;


typedef struct __ucontext {
	sigset_t	uc_sigmask;
	mcontext_t	uc_mcontext;
	struct __ucontext *uc_link;
	stack_t		uc_stack;
	int		uc_flags;
	int		__spare__[4];
} ucontext_t;

#endif

#define SCE_DBG_EXCEPTION_GPU     1
#define SCE_DBG_EXCEPTION_ILL     4
#define SCE_DBG_EXCEPTION_FPE     8
#define SCE_DBG_EXCEPTION_BUS    10
#define SCE_DBG_EXCEPTION_SEGV   11

typedef ucontext_t SceDbgUcontext;
typedef void (*SceDbgExceptionHandler)(int, SceDbgUcontext *);
int sceDbgInstallExceptionHandler(int en, SceDbgExceptionHandler handler);
int sceDbgRemoveExceptionHandler(int en);

/* GPU page fault detected */
#define SCE_DBG_EXCEPTION_GPU_FAULT_PAGE_FAULT_ASYNC                    0xa0d0c005

/* GPU encountered illegal instruction (obsolete) */
#define SCE_DBG_EXCEPTION_GPU_FAULT_BAD_OP_CODE_ASYNC                   0xa0d0c006

/* GPU encountered illegal command */
#define SCE_DBG_EXCEPTION_GPU_FAULT_BAD_COMMAND_ASYNC                   0xa0d0c006

/*
 * System software forcibly shutdown the process because of submitDone timeout.
 * GPU was in busy state.
 */
#define SCE_DBG_EXCEPTION_GPU_FAULT_SUBMITDONE_TIMEOUT_IN_RUN_ASYNC     0xa0d0c007

/*
 * System software failed to suspend the process because of submitDone timeout.
 * GPU was in busy state.
 */
#define SCE_DBG_EXCEPTION_GPU_FAULT_SUBMITDONE_TIMEOUT_IN_SUSPEND_ASYNC  0xa0d0c008

/*
 * System software forcibly shutdown the process because of submitDone timeout.
 * GPU was in idle state.
*/
#define SCE_DBG_EXCEPTION_CPU_FAULT_SUBMITDONE_TIMEOUT_IN_RUN_ASYNC      0xa0d0c009

/*
 * System software failed to suspend the process because of submitDone timeout.
 * GPU was in idle state.
 */
#define SCE_DBG_EXCEPTION_CPU_FAULT_SUBMITDONE_TIMEOUT_IN_SUSPEND_ASYNC  0xa0d0c00a

#define SCE_DBG_EVFILT_GPU_EXCEPTION  EVFILT_GPU_EXCEPTION

int sceDbgAddGpuExceptionEvent(SceKernelEqueue eq, void *udata);
int sceDbgDeleteGpuExceptionEvent(SceKernelEqueue eq);

__END_DECLS

#endif
