//
// system/tmcommands.h
//
// Copyright (C) 2013-2015 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_TMCOMMANDS_H
#define SYSTEM_TMCOMMANDS_H

// Generally want to enable the sysTmCmdXXXs in all but the final shipping build
// (ie, !__FINAL and !__NO_OUTPUT).
//
// But also need to disable on any Microsoft builds that have LTCG or PGO enabled,
// because of the evil hackery in tmcommands_ms.cpp.
//
#if RSG_DURANGO || RSG_PC
#    define SYSTMCMD_ENABLE     (!__FINAL)
#else
#    define SYSTMCMD_ENABLE     (!__FINAL || !__NO_OUTPUT)
#endif

// Disable the tm commands if building for a PC target, but not the game.
#if RSG_PC && (__RESOURCECOMPILER || __GAMETOOL || __TOOL)
#	undef  SYSTMCMD_ENABLE
#	define SYSTMCMD_ENABLE  (0)
#endif

#if SYSTMCMD_ENABLE
#	define SYSTMCMD_ENABLE_ONLY(...)    __VA_ARGS__
#else
#	define SYSTMCMD_ENABLE_ONLY(...)
#endif

// So that we can include this header in tmcommands_durango.cpp, disable the
// rest of the header file if JUST_DEFINE_SYSTMCMD_ENABLE is defined.
#ifndef JUST_DEFINE_SYSTMCMD_ENABLE

#if SYSTMCMD_ENABLE

#include "param.h"

namespace rage
{
	XPARAM(rockstartargetmanager);

	DURANGO_ONLY(extern "C" {)

	void sysTmCmdNop();
	void sysTmCmdPrintAllThreadStacks();
	void sysTmCmdStopNoErrorReport();
	void sysTmCmdQuitf(const char *msg);
	void sysTmCmdGpuHang(DURANGO_ONLY(const wchar_t *xhitFilename));
	void sysTmCmdCpuHang();
	void sysTmCmdConfig(const char *config);
	void sysTmCmdCreateDump();
	void sysTmCmdExceptionHandlerBegin();
	void sysTmCmdExceptionHandlerEnd(const char *coredump, const void *bugstarConfig);

#	if RSG_DURANGO
		void sysTmCmdThreadBegin();
		void sysTmCmdThreadEnd();
		void sysTmCmdReportGpuHangHack(); // ask R*TM to patch bug in July 2014 XDK ID3D11DeviceX::ReportGpuHang

		// Evil evil hackery
		struct GpuCmdBufAccess
		{
			size_t offsTinyDevice;                                  // offsetof(CD3DContext, m_TinyDevice)
			size_t offsDeBufferPos;                                 // offsetof(CD3DContext, m_TinyDevice.m_DeBuffer.m_pPos)
			size_t offsDeBufferLimit;                               // offsetof(CD3DContext, m_TinyDevice.m_DeBuffer.m_pLimit)
			size_t offsDeBufferActiveDwords;                        // offsetof(CD3DContext, m_TinyDevice.m_DeBuffer.m_ActiveDwords)
			size_t offsCeBufferPos;                                 // offsetof(CD3DContext, m_TinyDevice.m_DeBuffer.m_pPos)
			size_t offsCeBufferLimit;                               // offsetof(CD3DContext, m_TinyDevice.m_DeBuffer.m_pLimit)
			size_t offsCeBufferActiveDwords;                        // offsetof(CD3DContext, m_TinyDevice.m_DeBuffer.m_ActiveDwords)
			void (*StartNewSegment)(void *tinyDevice, u32 x, u32 y);     // TinyD3D::Device::StartNewSegment
		};
		bool sysTmCmdGetGpuCommandBufferAccess(GpuCmdBufAccess *access);

#	elif RSG_ORBIS
		void sysTmCmdSubmitDoneExceptionEnabled(bool enabled);
#	endif

	DURANGO_ONLY(})

	// timeoutMs of 0 means infinite/no-timeout
	void sysTmInit(bool waitConnect, u32 timeoutMs=0);

#	if RSG_DURANGO
		const char *sysTmGetPdb();
		const char *sysTmGetFullPackageName();
#	endif
}
// namespace rage

#endif // SYSTMCMD_ENABLE

#endif // JUST_DEFINE_SYSTMCMD_ENABLE

#endif // SYSTEM_TMCOMMANDS_H
