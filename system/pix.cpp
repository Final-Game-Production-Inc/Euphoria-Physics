//
// system/pix.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
//

#include "pix.h"

#if ENABLE_PIX_TAGGING
namespace rage
{
	namespace pfPixMarkerFuncs {
		void PixBeginCb_NoOp			(unsigned int , const char *) {}
		void PixBeginCCb_NoOp			(unsigned int , unsigned int , const char *) {}
		void PixBeginCNCb_NoOp			(unsigned int , unsigned int , const char *, ...) {}
		void PixBeginNCb_NoOp			(unsigned int , const char *, ...) {}
		void PixEndCb_NoOp				() {}
		void PixEndECb_NoOp				(const char *) {}

		PixBeginCb			g_PixBeginCb	= PixBeginCb_NoOp;
		PixBeginCCb			g_PixBeginCCb	= PixBeginCCb_NoOp;
		PixBeginCNCb		g_PixBeginCNCb	= PixBeginCNCb_NoOp;
		PixBeginNCb			g_PixBeginNCb	= PixBeginNCb_NoOp;
		PixEndCb			g_PixEndCb		= PixEndCb_NoOp;
		PixEndECb			g_PixEndECb		= PixEndECb_NoOp;


	} // namespace pfPixMarkerFuncs
} // namespace rage
#endif // ENABLE_PIX_TAGGINGn