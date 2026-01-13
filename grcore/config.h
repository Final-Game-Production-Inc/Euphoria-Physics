//
// grcore/config.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_CONFIG_H
#define GRCORE_CONFIG_H

#include "system/tls.h"

#if __PS3
	#define __D3D		0
	#define __D3D9		0
	#define __D3D11		0
	#define __D3D11_1	0
	#define __GCM		1
	#define __GNM		0
	#define __OPENGL	0
#elif __OSX
	#define __D3D		0
	#define __D3D9		0
	#define __D3D11		0
	#define __D3D11_1	0
	#define __GCM		0
	#define __GNM		0
	#define __OPENGL	1
#elif __PSP2
	#define __D3D		0
	#define __D3D9		0
	#define __D3D11		0
	#define __D3D11_1	0
	#define __GCM		0
	#define __GNM		0
	#define __OPENGL	0
#elif __XENON
	#define __D3D		0x900
	#define __D3D9		1
	#define __D3D11		0
	#define __D3D11_1	0
	#define __GCM		0
	#define __GNM		0
	#define __OPENGL	0
#elif RSG_ORBIS
	#define __D3D		0
	#define __D3D9		0
	#define __D3D11		0
	#define __D3D11_1	0
	#define __GCM		0
	#define __GNM		1
	#define __OPENGL	0
	#define ENABLE_LCUE	0
#elif RSG_DURANGO
	#define __D3D		0xB00
	#define __D3D9		0
	#define __D3D11		1
	#define __D3D11_1	1
	#define __GCM		0
	#define __GNM		0
	#define __OPENGL	0
#elif __WIN32PC && !defined(__D3D) && !defined(__D3D9) && !defined(__D3D11)
  #if __64BIT && !__TOOL && !__RESOURCECOMPILER
	#define __D3D		0xB00
	#define __D3D9		0
	#define __D3D11		1
	#define __D3D11_1	0
  #else
	#define __D3D		0x900
	#define __D3D9		1
	#define __D3D11		0
	#define __D3D11_1	0
  #endif
	#define __GCM		0
	#define __GNM		0
	#define __OPENGL	0
#endif

#if __D3D
#define D3D_ONLY(...)       __VA_ARGS__
#else
#define D3D_ONLY(...)
#endif // __D3D

#if __D3D9
#define D3D9_ONLY(...)      __VA_ARGS__
#else
#define D3D9_ONLY(...)
#endif // __D3D9

#if __D3D11
#define D3D11_ONLY(...)     __VA_ARGS__
#else
#define D3D11_ONLY(...)
#endif // __D3D11

#if __D3D11_1
#define D3D11_1_ONLY(...)     __VA_ARGS__
#else
#define D3D11_1_ONLY(...)
#endif // __D3D11_1

#if __GCM
#define GCM_ONLY(...)       __VA_ARGS__
#else
#define GCM_ONLY(...)
#endif // __GCM

#if __GNM
#define GNM_ONLY(...)       __VA_ARGS__
#else
#define GNM_ONLY(...)
#endif // __GNM

#if __D3D11 || __GNM
#define D3D11_OR_ORBIS_ONLY(...) __VA_ARGS__
#else
#define D3D11_OR_ORBIS_ONLY(...)
#endif

#if __OPENGL
#define OPENGL_ONLY(...)    __VA_ARGS__
#else
#define OPENGL_ONLY(...)
#endif // __OPENGL

#define EFFECT_CONTAINER_SIZE					((__64BIT || RSG_PC) ? (175*1024) : 32768)

#if RSG_PC
	#define DECLARE_MTR_THREAD
#else
	#define DECLARE_MTR_THREAD __THREAD
#endif // RSG_PC

#if __D3D11 || RSG_ORBIS
#define RSG_SM_50		1
#else
#define RSG_SM_50		0
#endif 

#define __D3D11_MONO_DRIVER (1 && RSG_DURANGO)
// Temp hacks to get the preview monolithic driver to work
#define __D3D11_MONO_DRIVER_HACK ( 1 && __D3D11_MONO_DRIVER )

#include "config_switches.h"

#endif // GRCORE_CONFIG_H
