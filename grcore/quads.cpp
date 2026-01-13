// 
// grcore/quads.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "quads.h"
#include "vertexdecl.h"

#if RSG_ORBIS && ENABLE_LCUE
#include "im.h"
#endif

namespace rage {

__THREAD int g_QuadsInBegin = 0;
__THREAD grcQuadVertex *g_QuadPtr = NULL;

static grcVertexDeclaration *s_QuadDecl;

void grcBeginQuadsInternal(int quadCount) 
{
	Assert(!g_QuadPtr);
	GRCDEVICE.SetVertexDeclaration(s_QuadDecl); 
	if (!grcEffect::IsInDraw())
		GRCDEVICE.SetDefaultEffect(false,false); 
	g_QuadPtr = (grcQuadVertex*) GRCDEVICE.BeginVertices(grcQuadPrimitive, quadCount * grcVerticesPerQuad, sizeof(grcQuadVertex)); 
	Assert(g_QuadPtr);
}

void grcInitQuads()
{
	Assert(!s_QuadDecl);
	static grcVertexElement elem[] = {
		grcVertexElement(0, grcVertexElement::grcvetPosition,	0, 12, grcFvf::grcdsFloat3),
#if __D3D11
		grcVertexElement(0, grcVertexElement::grcvetNormal,		0, 12, grcFvf::grcdsFloat3),
#endif // __D3D11
		grcVertexElement(0, grcVertexElement::grcvetColor,		0, 4, grcFvf::grcdsColor),
		grcVertexElement(0, grcVertexElement::grcvetTexture,	0, 8, grcFvf::grcdsFloat2)
	};
	s_QuadDecl = GRCDEVICE.CreateVertexDeclaration(elem,sizeof(elem)/sizeof(grcVertexElement));
}

void grcShutdownQuads()
{
	Assert(s_QuadDecl);
	s_QuadDecl->Release();
}

#if RSG_ORBIS && ENABLE_LCUE

void grcDrawSingleQuadf(float x1, float y1, float x2, float y2, float zVal, float u1, float v1, float u2,float v2, const Color32 & color32)
{
	Assert(!g_QuadPtr);		// catch incorrectly nested legacy grcBeginQuads
	grcBegin(drawRects,3);
	grcVertex(x1, y1, zVal, 0,0,0, color32, u1,v1);
	grcVertex(x2, y1, zVal, 0,0,0, color32, u2,v1);
	grcVertex(x1, y2, zVal, 0,0,0, color32, u1,v2);
	grcEnd();
}

#else

void grcDrawSingleQuadf(float x1, float y1, float x2, float y2, float zVal, float u1, float v1, float u2,float v2, const Color32 & color32)
{
	grcBeginQuads(1);
	grcDrawQuadf(x1, y1, x2, y2, zVal, u1, v1, u2, v2, color32);
	grcEndQuads();
}

#endif

}		// namespace rage
