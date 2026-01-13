// 
// grcore/quads.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef GRCORE_QUADS_H
#define GRCORE_QUADS_H

#include "system/tls.h"
#include "grcore/drawmode.h"
#include "grcore/device.h"

namespace rage {

class Color32;

struct grcQuadVertex {
	float x, y, z;
#if __D3D11
	float nx, ny, nz;
#endif // __D3D11
	unsigned cpv;
	float u, v;
	void Set(float x_,float y_,float z_,unsigned cpv_,float u_,float v_) XENON_ONLY(volatile)
	{
		x = x_; y = y_; z = z_; cpv = cpv_; u = u_; v = v_;
#if __D3D11
		nx = ny = nz = 0.0f;
#endif // __D3D11
	}
};

extern __THREAD int g_QuadsInBegin;

extern __THREAD grcQuadVertex *g_QuadPtr;

void grcInitQuads();

void grcShutdownQuads();

void grcBeginQuadsInternal(int quadCount);


const int grcVerticesPerQuad = RSG_PS3? 4 : RSG_XENON || RSG_ORBIS? 3 : 6;
const grcDrawMode grcQuadPrimitive = RSG_PS3? drawQuads : RSG_ORBIS || RSG_XENON? drawRects : drawTris;

inline void grcBeginQuads(int quadCount) { if (++g_QuadsInBegin == 1) { grcBeginQuadsInternal(quadCount); } }

inline void grcEndQuads() { if (--g_QuadsInBegin == 0) { GRCDEVICE.EndVertices(g_QuadPtr); g_QuadPtr = NULL; } }

inline bool grcInQuads() { return g_QuadsInBegin != 0; }

// PS3 has 8160 byte fifo segment limit.  Each vertex takes up to 24 bytes, and each quad is 4 vertices.
const int grcBeginQuadsMax = RSG_PS3? 84 : RSG_XENON || RSG_ORBIS? 1024/3 : 1024/6;

inline void grcDrawQuadf(float x1, float y1, float x2, float y2, float zVal, float u1, float v1, float u2,float v2, const Color32 & color32)
{
	// TODO: Should this be volatile on 360 to avoid compiler reordering to write-combined memory?
	XENON_ONLY(volatile) grcQuadVertex *v = g_QuadPtr;
	unsigned color = color32.GetDeviceColor();

#if RSG_PS3
	v[0].Set(x1, y1, zVal,color,u1,v1);
	v[1].Set(x1, y2, zVal,color,u1,v2);
	v[2].Set(x2, y2, zVal,color,u2,v2);
	v[3].Set(x2, y1, zVal,color,u2,v1);
#elif RSG_XENON || RSG_ORBIS
	v[0].Set(x1, y1, zVal,color,u1,v1);
	v[1].Set(x2, y1, zVal,color,u2,v1);
	v[2].Set(x1, y2, zVal,color,u1,v2);
#else
	// TODO: Switch to GRCDEVICE.BeginIndexedVertices
	v[0].Set(x1, y1, zVal,color,u1,v1);
	v[1].Set(x1, y2, zVal,color,u1,v2);
	v[2].Set(x2, y1, zVal,color,u2,v1);

	v[3].Set(x1, y2, zVal,color,u1,v2);
	v[4].Set(x2, y2, zVal,color,u2,v2);
	v[5].Set(x2, y1, zVal,color,u2,v1);
#endif
	g_QuadPtr += grcVerticesPerQuad;
}

// This is simpler when you're only drawing one quad in relative isolation, and behaves differently under the hood in some platforms.
extern void grcDrawSingleQuadf(float x1, float y1, float x2, float y2, float zVal, float u1, float v1, float u2,float v2, const Color32 & color32);

};		// namespace rage

#endif	// GRCORE_QUADS_H
