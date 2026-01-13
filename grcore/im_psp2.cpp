//
// grcore/im_psp2.cpp
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//
#include "im.h"
#include "device.h"
#include "profile/page.h"
#include "profile/group.h"
#include "profile/element.h"

#if __PSP2

namespace rage {


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

struct Vtx {
	float x, y, z;
	float nx, ny, nz;
	Color32 c;
	float s, t;
};

Vtx *s_Begin;

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

		// normal, color, tex, pos
		AssertMsg(s_Begin,"grcBegin called again without grcEnd");
		s_Begin = (Vtx*) GRCDEVICE.BeginVertices(dm,vertCount,12 + 12 + 4 + 8);
	}
}

void grcVertex(float x,float y,float z,float nx,float ny,float nz,Color32 c,float s,float t)
{
	AssertMsg(s_Begin,"grcVertex called outside of grcBegin/End");
	Vtx *v = s_Begin++;
	v->x = x; v->y = y; v->z = z; 
	v->nx = nx; v->ny = ny; v->nz = nz; 
	v->c = c; v->s = s; v->t = t;
}

void grcEnd()
{
	GRCDEVICE.EndVertices();
}

}	// namespace rage

#endif	// __PSP2