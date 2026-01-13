// 
// grcore/vertexbuffer_d3d.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_XENON_RESOURCE_H
#define GRCORE_XENON_RESOURCE_H

#include "xenon_sdk.h"

CompileTimeAssert(_XDK_VER >= 2638);

// Stolen from XeDK headers.

#define D3DCOMMON_TYPE_VERTEXBUFFER             (1)
#define D3DCOMMON_TYPE_INDEXBUFFER              (2)
#define D3DCOMMON_TYPE_TEXTURE                  (3)
#define D3DCOMMON_TYPE_SURFACE                  (4)
#define D3DCOMMON_TYPE_VERTEXDECLARATION        (5)
#define D3DCOMMON_TYPE_VERTEXSHADER             (6)
#define D3DCOMMON_TYPE_PIXELSHADER              (7)
#define D3DCOMMON_TYPE_CONSTANTBUFFER           (8)
#define D3DCOMMON_TYPE_COMMANDBUFFER            (9)

// When accessing this resource via the CPU, the CPU uses a cached memory
// view.  D3D ensures coherency with the GPU by flushing the modified
// range at Unlock time.
//
#define D3DCOMMON_CPU_CACHED_MEMORY         0x00200000

// If set in the 'Common' field, this bit indicates that the indices are
// 32-bit values instead of 16-bit:
#define D3DINDEXBUFFER_INDEX32				0x80000000

#define D3DFLUSH_INITIAL_VALUE              0xffff0000

struct D3DResource {
    DWORD Common;           // Flags common to all resources
    DWORD ReferenceCount;   // External reference count
    DWORD Fence;            // This is the fence number of the last ring buffer
                            //   reference to this resource.  (This field was
                            //   known as 'Lock' on the original Xbox.)
                            //   Initialize it to zero.
    DWORD ReadFence;        // This is used to determine when it's safe for the
                            //   CPU to read a resource that was written to
                            //   by the GPU.  Initialize it to zero.
    DWORD Identifier;       // Game-supplied data that identifies the resource
    DWORD BaseFlush;        // Encodes the memory range to be flushed by D3D
                            //   via 'dcbf' at 'Unlock' time.  Initialize it
                            //   to D3DFLUSH_INITIAL_VALUE.
};

struct D3DIndexBuffer: public D3DResource {
	DWORD Address;
	DWORD Size;
};

typedef struct {
	/* struct {
		// DWORD 0:

		DWORD Type                      : 2;    // GPUCONSTANTTYPE
		DWORD BaseAddress               : 30;   // DWORD

		// DWORD 1:

		DWORD Endian                    : 2;    // GPUENDIAN
		DWORD Size                      : 24;   // DWORD
		DWORD AddressClamp              : 1;    // GPUADDRESSCLAMP
		DWORD                           : 1;
		DWORD RequestSize               : 2;    // GPUREQUESTSIZE
		DWORD ClampDisable              : 2;    // BOOL
	}; */
	DWORD dword[2];
} GPUVERTEX_FETCH_CONSTANT;

struct D3DVertexBuffer: public D3DResource {
	GPUVERTEX_FETCH_CONSTANT Format;
};

#endif	// GRCORE_XENON_RESOURCE_H
