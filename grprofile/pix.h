//
// grprofile/pix.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
//

#ifndef GRPROFILE_PIX_H
#define GRPROFILE_PIX_H

#include "diag/stats.h"
#include "system/pix.h"

#if __XENON && ENABLE_PIX_TAGGING
#define PROFILE_BUILD		// insure that PROFILE_BUILD is defined for system/xtl.h in bankrelease builds
#endif

	#include "grcore/config.h"

	#if (__XENON || __WIN32PC || RSG_DURANGO) && ENABLE_PIX_TAGGING
		void	PIXBeginC	(unsigned int filter, unsigned int c, const char *event);
		void	PIXBegin	(unsigned int filter, const char *event);
		void	PIXBeginCN	(unsigned int filter, unsigned int c, const char *event, ...);
		void	PIXBeginN	(unsigned int filter, const char *event, ...);
		void	PIXEnd		();
		inline void	PIXEndE		(const char* /*event*/) { PIXEnd(); }

		bool	PIXSaveGPUCapture(char* filename);
		bool	PIXIsGPUCapturing();

		// Caches results of PIXIsGPUCapturing() and only updates once per-frame
		inline bool PIXIsGPUCapturingCached() { extern bool g_CachedPIXIsGPUCapturing; return g_CachedPIXIsGPUCapturing; }
		inline void PIXUpdateGPUCapturingCache() { extern bool g_CachedPIXIsGPUCapturing; g_CachedPIXIsGPUCapturing = PIXIsGPUCapturing(); }

		void PIXInit();
		#define PIXShutdown()
		#define PIXBeginFrame() PIXUpdateGPUCapturingCache()
		#define PIXEndFrame()
		#define PIXPoll()

	#elif __GCM && ENABLE_PIX_TAGGING && !__SPU
		extern unsigned int g_EnablePixAnnotation;

		void	PIXBeginC	(unsigned int filter, unsigned int c, const char *event);
		void	PIXBegin	(unsigned int filter, const char *event);
		void	PIXBeginCN	(unsigned int filter, unsigned int c, const char *event, ...);
		void	PIXBeginN	(unsigned int filter, const char *event, ...);
		void	PIXEnd		();
		void	PIXEndE		(const char *event);

		bool	PIXSaveGPUCapture(char* filename);
		bool	PIXIsGPUCapturing();

		// Caches results of PIXIsGPUCapturing() and only updates once per-frame
		inline bool PIXIsGPUCapturingCached() { extern bool g_CachedPIXIsGPUCapturing; return g_CachedPIXIsGPUCapturing; }
		inline void PIXUpdateGPUCapturingCache() { extern bool g_CachedPIXIsGPUCapturing; g_CachedPIXIsGPUCapturing = PIXIsGPUCapturing(); }

		void PIXInit();
		#define PIXShutdown()
		#define PIXBeginFrame() PIXUpdateGPUCapturingCache()
		#define PIXEndFrame()
		#define PIXPoll()

	#elif RSG_ORBIS && ENABLE_PIX_TAGGING
		extern unsigned int g_EnablePixAnnotation;

		void	PIXBeginC	(unsigned int filter, unsigned int c, const char *event);
		void	PIXBegin	(unsigned int filter, const char *event);
		void	PIXBeginCN	(unsigned int filter, unsigned int c, const char *event, ...);
		void	PIXBeginN	(unsigned int filter, const char *event, ...);
		void	PIXEnd		();
		void	PIXEndE		(const char *event);

		bool	PIXSaveGPUCapture(char* filename);
		bool	PIXIsGPUCapturing();

		// Caches results of PIXIsGPUCapturing() and only updates once per-frame
		inline bool PIXIsGPUCapturingCached() { extern bool g_CachedPIXIsGPUCapturing; return g_CachedPIXIsGPUCapturing; }
		inline void PIXUpdateGPUCapturingCache() { extern bool g_CachedPIXIsGPUCapturing; g_CachedPIXIsGPUCapturing = PIXIsGPUCapturing(); }

		void PIXInit();
		#define PIXShutdown()
		#define PIXBeginFrame() PIXUpdateGPUCapturingCache()
		#define PIXEndFrame()
		#define PIXPoll()

	#else // either not PC or not D3D
		inline void PIXBegin(unsigned int,const char * /*event*/) { }
		inline void PIXBeginC(unsigned int /*filter*/,unsigned /*c*/, const char * /*event*/) { }
		inline void PIXBeginCN(unsigned int /*filter*/,unsigned /*c*/, const char * /*event*/,...) { }
		inline void PIXBeginN(unsigned int /*filter*/,const char * /*event*/,...) { }
		inline void PIXEnd() { }
		inline void PIXEndE(const char* /*event*/) { }

		inline bool	PIXSaveGPUCapture(char* /*filename*/) { return true; }
		inline bool	PIXIsGPUCapturing() { return false; }

		inline bool PIXIsGPUCapturingCached() { return false; }
		inline void PIXUpdateGPUCapturingCache() {}
	
		// These are currently only used on the PS3, but include them as empty macros for completeness
		void PIXInit();
		#define PIXShutdown()
		#define PIXBeginFrame()
		#define PIXEndFrame()
		#define PIXPoll()
	#endif // (__WIN32 && (!defined(__OPENGL) || !__OPENGL))

#define GCM_HUD (__BANK && __OPTIMIZED && __GCM && ENABLE_PIX_TAGGING && 0)

#endif  // GRPROFILE_PIX_H
