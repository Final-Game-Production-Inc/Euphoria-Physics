//
// system/pix.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
//

#ifndef SYSTEM_PIX_H
#define SYSTEM_PIX_H

#include "diag/stats.h"
#include "system/pix.h"

#if RSG_DURANGO
#define ENABLE_PIX_TAGGING	(!__FINAL&&!__TOOL&&!__RESOURCECOMPILER&&!__GAMETOOL&&!__RGSC_DLL)
#else
#define ENABLE_PIX_TAGGING	((__BANK||__DEV||__PROFILE||__STATS)&&!__TOOL&&!__RESOURCECOMPILER&&!__GAMETOOL&&!__RGSC_DLL)
#endif

#if ENABLE_PIX_TAGGING
#define PIX_TAGGING_ONLY(...)	__VA_ARGS__
#else
#define PIX_TAGGING_ONLY(...)
#endif

#if ENABLE_PIX_TAGGING
namespace rage {

	// These are all callback functions for manipulating markers. RageCore callbacks are all no-ops, but RageGraphics will install the "real" versions of the functions
	namespace pfPixMarkerFuncs {
		typedef void (*PixBeginCb)			(unsigned int filter, const char *event);
		typedef void (*PixBeginCCb)			(unsigned int filter, unsigned int c, const char *event);
		typedef void (*PixBeginCNCb)		(unsigned int filter, unsigned int c, const char *event, ...);
		typedef void (*PixBeginNCb)			(unsigned int filter, const char *event, ...);
		typedef void (*PixEndCb)			();
		typedef void (*PixEndECb)			(const char *event);

		extern PixBeginCb				g_PixBeginCb;
		extern PixBeginCCb				g_PixBeginCCb;
		extern PixBeginCNCb				g_PixBeginCNCb;
		extern PixBeginNCb				g_PixBeginNCb;
		extern PixEndCb					g_PixEndCb;
		extern PixEndECb				g_PixEndECb;
	} // namespace pfPixMarkerFuncs

} // namespace rage

#define PF_PUSH_MARKER(x)		::rage::pfPixMarkerFuncs::g_PixBeginCb(1, x)
#define PF_PUSH_MARKERC(c,x)	::rage::pfPixMarkerFuncs::g_PixBeginCCb(1,c,x)
#define PF_PUSH_MARKERCN(c,...)	::rage::pfPixMarkerFuncs::g_PixBeginCNCb(1,c,##__VA_ARGS__)
#define PF_POP_MARKER()			::rage::pfPixMarkerFuncs::g_PixEndCb()
struct PIXAutoTag {
	PIXAutoTag(unsigned int filter,const char *event) { ::rage::pfPixMarkerFuncs::g_PixBeginCb(filter,event); }
	PIXAutoTag(unsigned int filter,const char *event,int n) { ::rage::pfPixMarkerFuncs::g_PixBeginNCb(filter,event,n); }
	PIXAutoTag(unsigned int filter,const char *event,float n) { ::rage::pfPixMarkerFuncs::g_PixBeginNCb(filter,event,n); }
	PIXAutoTag(unsigned int filter,const char *event,const char *n) { ::rage::pfPixMarkerFuncs::g_PixBeginNCb(filter,event,n); }
	~PIXAutoTag() { ::rage::pfPixMarkerFuncs::g_PixEndCb(); }
};
#define PIX_AUTO_TAG(filter,x)		PIXAutoTag AutoTag_##__LINE__(filter,x)
#define PIX_AUTO_TAGN(filter,x,n)	PIXAutoTag AutoTag_##__LINE__(filter,x,(n))
#else
#define PIX_AUTO_TAG(filter,x)
#define PIX_AUTO_TAGN(filter,x,n)
#define PF_PUSH_MARKER(x) 
#define PF_PUSH_MARKERC(c,x)
#define PF_PUSH_MARKERCN(c,...)
#define PF_POP_MARKER() 
#endif

#endif  // SYSTEM_PIX_H
