#ifndef CONFIG_SWITCHES_H
#define CONFIG_SWITCHES_H

// Synthesize __D3D9 and __D3D11 defines when compiling shaders.
#if defined(__SHADERMODEL)

#if RSG_PC 

#if (__SHADERMODEL == 30)
#define __D3D9		1
#define __D3D11		0
#elif (__SHADERMODEL == 40)
#define __D3D9		0
#define __D3D11		1
#elif (__SHADERMODEL == 50)
#define __D3D9		0
#define __D3D11		1
#else
#define __D3D9		0
#define __D3D11		0
#endif

#elif RSG_DURANGO
#define __D3D9		0
#define __D3D11		1
#else
#define __D3D9		0
#define __D3D11		0
#endif

#else // defined(__SHADERMODEL)

//For cpp builds, to keep from having errors, set shader defines to 0. We should hopefully only need to turn off the functionality
//below for shaders compiled with instancing support.
#ifndef INSTANCED
#	define INSTANCED 0
#endif

#ifndef BATCH_INSTANCING
#	define BATCH_INSTANCING 0
#endif

#endif // defined(__SHADERMODEL)

#define RAGE_SUPPORT_TESSELLATION_TECHNIQUES		((1 && (RSG_PC && __D3D11)) || (1 && RSG_DURANGO) || (1 && RSG_ORBIS))

// TEMP!!!!
#define TEMP_RUNTIME_CALCULATE_TERRAIN_OPEN_EDGE_INFO RAGE_SUPPORT_TESSELLATION_TECHNIQUES && ((0 && (RSG_PC && __D3D11)) || (0 && RSG_DURANGO))

// NOTE:- RE-COMPILE SHADERS AFTER ALTERING THESE SWITCHES!
#define RAGE_INSTANCED_TECH				(((1 && (__D3D11 && RSG_PC)) || (1 && RSG_DURANGO) || (1 && RSG_ORBIS)) && !INSTANCED && !BATCH_INSTANCING)
#define GS_INSTANCED_SHADOWS			((RAGE_INSTANCED_TECH && ((1 && (__D3D11 && RSG_PC)) || (1 && RSG_DURANGO)) || (1 && RSG_ORBIS)) && !INSTANCED && !BATCH_INSTANCING)
#define GS_INSTANCED_CUBEMAP			((RAGE_INSTANCED_TECH && ((0 && (__D3D11 && RSG_PC)) || (0 && RSG_DURANGO)) || (0 && RSG_ORBIS)) && !INSTANCED && !BATCH_INSTANCING)

// To check/debug print render targets etc at start and end of drawlists.
#define DEBUG_SEALING_OF_DRAWLISTS ((0 && RSG_PC) || (0 && RSG_ORBIS) || (0 && RSG_DURANGO))

#ifndef __LOW_QUALITY 
	#define __LOW_QUALITY 0
#endif

#define SUPPORT_INVERTED_PROJECTION		((1 && RSG_PC) || (1 && RSG_ORBIS) || (1 && RSG_DURANGO))
#define SUPPORT_INVERTED_VIEWPORT		(((0 && RSG_PC) || (0 && RSG_ORBIS) || (0 && RSG_DURANGO)) && SUPPORT_INVERTED_PROJECTION)

#define USE_INVERTED_PROJECTION_ONLY	(SUPPORT_INVERTED_PROJECTION && !SUPPORT_INVERTED_VIEWPORT)

#endif // CONFIG_SWITCHES_H
