// 
// grcore/effect_config.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_EFFECT_CONFIG_H 
#define GRCORE_EFFECT_CONFIG_H 

#include "grcore/config.h"
#include "paging/handle.h"

// Please make sure this is only included from .cpp files, not .h files.  Yeah, that promise was broken long ago.

// 1=basic fifo management and EDGE job submission
// 2=grcEffect class
// 3=rmcDrawable class (note, this breaks blendshapes on sample_mt_creature)
#define SPU_GCM_FIFO			            (3)
#define GRCORE_ON_SPU			            (__PPU? SPU_GCM_FIFO : 0)
#define PATCH_ON_SPU			            (!__SPU && !GRCORE_ON_SPU && !SPU_GCM_FIFO)
#define USE_EDGE				            (__PS3 && 1)

// PGHANDLE_REF_COUNT debug code doesn't currently catch as many errors when
// USE_PACKED_GCMTEX is enabled, so don't enable them both.
#define USE_PACKED_GCMTEX		            (__PS3 && !PGHANDLE_REF_COUNT)

// Enable fragment ucode branch stripping on PS3.
// This is disabled in beta builds just to save code space in drawablespu.
#define ENABLE_FRAG_UCODE_BRANCH_STRIPPING  (__PS3 && !__DEV)

#if USE_EDGE
	#define EDGE_ONLY(x) x
#else
	#define EDGE_ONLY(x)
#endif

#define GRCDBG_IMPLEMENTED              ((RSG_DURANGO || RSG_ORBIS || RSG_PS3 || RSG_PC) && !__FINAL && !RSG_TOOL && !__RESOURCECOMPILER)
#if GRCDBG_IMPLEMENTED
#	define GRCDBG_IMPLEMENTED_ONLY(...) __VA_ARGS__
#else
#	define GRCDBG_IMPLEMENTED_ONLY(...)
#	define GRCDBG_PUSH(name)            (void)0
#	define GRCDBG_POP()                 (void)0
#endif

#define HACK_GTA4_MODELINFOIDX_ON_SPU	(1 && HACK_GTA4 && __PS3 && __BANK)
#define TINT_PALETTE_RESOURCES			(HACK_GTA4 && !__64BIT)				// supported only on ps3 and 360 atm

#if USE_EDGE
enum eEdgeShadowType
{
	EDGE_SHADOWTYPE_NONE                = 0,
	EDGE_SHADOWTYPE_CASCADE_SHADOWS     = 1, // directional
	EDGE_SHADOWTYPE_PARABOLOID_SHADOWS  = 2, // point
	EDGE_TYPE_PARABOLOID_REFLECTION     = 3,
};
#endif // USE_EDGE

#endif // GRCORE_EFFECT_CONFIG_H 
