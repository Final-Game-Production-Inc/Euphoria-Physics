//
// grcore/im_gnm.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//
#include "im.h"
#include "device.h"
#include "light.h"
#include "profile/page.h"
#include "profile/group.h"
#include "profile/element.h"
#include "viewport.h"
#if __GNM

namespace rage {

extern __THREAD u8 g_VertexShaderImmediateMode;

bool g_TallyGrcCalls;

namespace GraphicsStats {
	PF_PAGE(GfxPage,"Graphics Stats");
	PF_GROUP(GfxGeneric);
	PF_LINK(GfxPage,GfxGeneric);

	PF_COUNTER(grmGeometry_Draw_Calls,GfxGeneric);
	PF_COUNTER(grmGeometry_Draw_Vertices,GfxGeneric);
	PF_COUNTER(grmGeometry_Draw_Indices,GfxGeneric);
	PF_COUNTER(grmGeometry_Draw_Primitives,GfxGeneric);
	PF_COUNTER(grmShaderFx_Bind_Calls,GfxGeneric);
	PF_COUNTER(grcViewport_SetWorldMtx_Calls,GfxGeneric);
	PF_COUNTER(grcBegin_Calls,GfxGeneric);
	PF_COUNTER(grcBegin_Vertices,GfxGeneric);
}
using namespace GraphicsStats;

__THREAD float *s_Pos; __THREAD u32 s_PosStride;
__THREAD float *s_Nrm; __THREAD u32 s_NrmStride;
__THREAD u32 *s_Cpv; __THREAD u32 s_CpvStride;
__THREAD float *s_TexCoord; __THREAD u32 s_TexCoordStride;

grcVertexDeclaration *s_Decls[8];

void grcInit()
{
	for (int i=0; i<8; i++) {
		grcVertexElement elems[4];
		elems[0] = grcVertexElement(0, grcVertexElement::grcvetPosition,	0, 12, grcFvf::grcdsFloat3);
		int elemCt = 1;
		if (i & 1)
			elems[elemCt++] = grcVertexElement(0, grcVertexElement::grcvetNormal,		0, 12, grcFvf::grcdsFloat3);
		if (i & 2)
			elems[elemCt++] = grcVertexElement(0, grcVertexElement::grcvetColor,		0, 4, grcFvf::grcdsColor);
		if (i & 4)
			elems[elemCt++] = grcVertexElement(0, grcVertexElement::grcvetTexture,	0, 8, grcFvf::grcdsFloat2);
		s_Decls[i] = GRCDEVICE.CreateVertexDeclaration(elems, elemCt);
	}
}

void grcBegin(grcDrawMode dm,int vertCount)
{
	AssertMsg(vertCount <= grcBeginMax, "Too many verts in this begin draw call");
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->Begin(dm,vertCount);
	else
#endif
	{
#if __STATS
		if (g_TallyGrcCalls) {
			PF_INCREMENT(grcBegin_Calls);
			PF_INCREMENTBY(grcBegin_Vertices,vertCount);
		}
#endif

		if (!grcEffect::IsInDraw())
			GRCDEVICE.SetDefaultEffect(grcLightState::IsEnabled(),false); 
		int vsim = g_VertexShaderImmediateMode;
		GRCDEVICE.SetVertexDeclaration(s_Decls[vsim]);

		AssertMsg(!s_Pos,"grcBegin called again without grcEnd");
		static u8 strides[8] = { 3, 3+3, 3+1, 3+3+1,    3+2, 3+3+2, 3+1+2, 3+3+1+2 };
		/* static u8 nrmStrides[8] = { 0, 3+3, 0, 3+3+1,   0, 3+3+2, 0, 3+3+1+2 };
		static u8 cpvStrides[8] = { 0, 0, 3+1, 0,       0, 0, 3+1+2, 3+3+1+2 };
		static u8 texStrides[8] = { 0, 0, 0, 0,         3+2, 3+3+2, 3+1+2, 3+3+1+2 }; */

		float *rover = (float*) GRCDEVICE.BeginVertices(dm,vertCount,strides[vsim] * 4);
		static float nrmDummy[3], texCoordDummy[2];
		static u32 cpvDummy[1];

		int stride = strides[vsim];
		s_Pos = rover, s_PosStride = stride; rover += 3;

		if (vsim & 1)
			s_Nrm = rover, s_NrmStride = stride, rover += 3;
		else
			s_Nrm = nrmDummy, s_NrmStride = 0;

		if (vsim & 2)
			s_Cpv = (u32*)rover, s_CpvStride = stride, rover += 1;
		else
			s_Cpv = cpvDummy, s_CpvStride = 0;

		if (vsim & 4)
			s_TexCoord = rover, s_TexCoordStride = stride, rover += 2;
		else 
			s_TexCoord = texCoordDummy, s_TexCoordStride = 0;
	}
}

void grcVertex(float x,float y,float z,float nx,float ny,float nz,Color32 c,float s,float t)
{
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->Vertex(x,y,z,nx,ny,nz,c,s,t);
	else
#endif
	{
		float *xyz = s_Pos; s_Pos += s_PosStride;
		float *nrm = s_Nrm; s_Nrm += s_NrmStride;
		u32 *cpv = s_Cpv; s_Cpv += s_CpvStride;
		float *st = s_TexCoord; s_TexCoord += s_TexCoordStride;

		xyz[0] = x; xyz[1] = y; xyz[2] = z;
		nrm[0] = nx; nrm[1] = ny; nrm[2] = nz;
		cpv[0] = c.GetDeviceColor();
		st[0] = s; st[1] = t;
	}
}

#if RAGE_INSTANCED_TECH
void grcInstEnd()
{
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->End();
	else 
#endif
	{
		Assertf(grcViewport::GetInstancing() == true, "must be in instancing render phase.\n");
		GRCDEVICE.EndInstancedVertices(s_Pos,grcViewport::GetNumInstVP());
		s_Pos = NULL;
	}
}
#endif

void grcEnd()
{
#if RAGE_INSTANCED_TECH
	if (grcViewport::GetInstancing()) { return grcInstEnd(); }
#endif

#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->End();
	else 
#endif
	{
		GRCDEVICE.EndVertices(s_Pos);
		s_Pos = NULL;
	}
}

}	// namespace rage

#endif	// __PSP2