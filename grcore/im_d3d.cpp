//
// grcore/im_d3d.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "im.h"

#if __D3D

#include "device.h"
#include "effect.h"
#include "light.h"
#include "viewport.h"
#include "profile/page.h"
#include "profile/group.h"
#include "profile/element.h"
#include "system/namedpipe.h"
#include "system/pipepacket.h"
#include "system/xtl.h"
#include "system/d3d9.h"

namespace rage {

bool g_TallyGrcCalls = true;

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

class grmShader;

struct VTX {
	float x, y, z;
	float nx, ny, nz;
	u32 c;
	float s, t;
};


static DECLARE_MTR_THREAD struct VTX *s_Current = NULL;
static DECLARE_MTR_THREAD grcDrawMode s_DrawMode;
static DECLARE_MTR_THREAD int s_Offset = 0;
static DECLARE_MTR_THREAD int s_Count = 0;

#if __WIN32PC
static DECLARE_MTR_THREAD u16* s_CurrentIndex;
static DECLARE_MTR_THREAD int s_IndexOffset;
static DECLARE_MTR_THREAD int s_IndexCount;
static DECLARE_MTR_THREAD bool s_bIndexed;
#endif // __WIN32PC


void grcBegin(grcDrawMode dm,int count) {
	AssertMsg(count > 0,"Non-positive count passed to grcBegin!");
	Assert("Too many verts in this begin draw call" && count <= grcBeginMax);
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->Begin(dm,count);
	else
#endif
	{
		WIN32PC_ONLY(AssertMsg( dm != drawQuads, "drawQuads not supported on PC")); 
		AssertMsg(!s_Count,"Nested grcBegin call!");

#if __STATS
		if (g_TallyGrcCalls) {
			PF_INCREMENT(grcBegin_Calls);
			PF_INCREMENTBY(grcBegin_Vertices,count);
		}
#endif

		
		if (!grcEffect::IsInDraw())
			GRCDEVICE.SetDefaultEffect(grcLightState::IsEnabled(),false); 
		GRCDEVICE.SetVertexDeclaration(GRCDEVICE.GetImmediateModeDecl());

		s_Current = (VTX*) GRCDEVICE.BeginVertices(dm,count,sizeof(VTX));
		Assert(s_Current != NULL);
		s_DrawMode = dm;
		s_Offset = 0;
		s_Count = count;
	}
}

void grcVertex(float x,float y,float z,float nx,float ny,float nz,Color32 c,float s,float t) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->Vertex(x,y,z,nx,ny,nz,c,s,t);
	else
#endif
	{
		if (s_Count && !s_Current) {	// grcBegin failed, silently exit
			++s_Offset;
			return;
		}
		Assert(s_Current && "grcVertex outside of begin/end");
		volatile VTX &cv = *s_Current++;
		++s_Offset;
		cv.x = x;
		cv.y = y;
		cv.z = z;
		cv.nx = nx;
		cv.ny = ny;
		cv.nz = nz;
		cv.c = c.GetDeviceColor();
		cv.s = s;
		cv.t = t;
		Assert(s_Offset <= s_Count);
	}
}

#if RAGE_INSTANCED_TECH
void grcInstEnd() {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->End();
	else 
#endif
	{
#if RAGE_INSTANCED_TECH
		Assertf(grcViewport::GetInstancing() == true, "must be in instancing render phase.\n");
#endif
		Assertf(s_Offset == s_Count,"Got %d verts, wanted %d",s_Offset,s_Count);
		if (s_Current) {
			GRCDEVICE.EndInstancedVertices(s_Current);
			s_Current = NULL;
		}
		s_Count = 0;
	}
}
#endif


void grcEnd() {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->End();
	else 
#endif
	{
#if RAGE_INSTANCED_TECH
		if (grcViewport::GetInstancing()) { return grcInstEnd(); }
#endif
		Assertf(s_Offset == s_Count,"Got %d verts, wanted %d",s_Offset,s_Count);
		if (s_Current) {
			GRCDEVICE.EndVertices(s_Current);
			s_Current = NULL;
		}
		s_Count = 0;
	}
}

#if __D3D11
void grcEnd(u32 numVertices)
{
#if RAGE_INSTANCED_TECH
	if (grcViewport::GetInstancing()) { return grcInstEnd(); }
#endif
	Assertf((s32)numVertices < s_Count, "numverts greater than count");
	if (s_Current) {
		GRCDEVICE.EndVertices(numVertices);
		s_Current = NULL;
	}
	s_Count = 0;
}
#endif


#if __WIN32PC
void grcBeginIndexed(grcDrawMode dm, int vertexCount, int indexCount)
{
	AssertMsg(vertexCount > 0,"Non-positive vertex count passed to grcBeginIndexed!");
	AssertMsg(indexCount > 0,"Non-positive index count passed to grcBeginIndexed");
	Assert("Too many verts in this begin draw call" && vertexCount <= grcBeginMax);
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->Begin(dm,vertexCount);
	else
#endif
	{
		AssertMsg(!s_Count,"Nested grcBegin call!");

#if __STATS
		if (g_TallyGrcCalls) {
			PF_INCREMENT(grcBegin_Calls);
			PF_INCREMENTBY(grcBegin_Vertices,vertexCount);
		}
#endif

		if (!grcEffect::IsInDraw())
			GRCDEVICE.SetDefaultEffect(grcLightState::IsEnabled(),false); 
		GRCDEVICE.SetVertexDeclaration(GRCDEVICE.GetImmediateModeDecl());

		GRCDEVICE.BeginIndexedVertices(dm,vertexCount,sizeof(VTX),indexCount,(void**)&s_Current,(void**)&s_CurrentIndex);
		Assert(s_Current != NULL);
		Assert(s_CurrentIndex != NULL);
		s_DrawMode = dm;
		s_Offset = 0;
		s_IndexOffset = 0;
		s_Count = vertexCount;
		s_IndexCount = indexCount;
		s_bIndexed = true;
	}
}

void grcIndex(u16 index)
{
	AssertMsg(s_bIndexed, "grcIndex was called without a call to grcBeginIndexed");

	if (s_IndexCount && !s_CurrentIndex) {	// grcBegin failed, silently exit
		++s_IndexOffset;
		return;
	}
	Assert(s_CurrentIndex && "grcIndex outside of BeginIndexed/EndIndexed");

	volatile u16 &ci = *s_CurrentIndex++;
	++s_IndexOffset;
	ci = index;

	Assertf(s_IndexOffset <= s_IndexCount, "Offset: %d Count: %d", s_IndexOffset, s_IndexCount);
}

void grcEndIndexed(int indexCount, int vertexCount)
{
	Assertf(!grcBatcher::GetCurrent(), "Currently grcEndIndexed is not supported for grcBatcher.");
	AssertMsg(s_bIndexed, "grcBeginIndexed was not called but calling grcEndIndexed.");

	if (s_Current && s_CurrentIndex)
	{
		GRCDEVICE.EndIndexedVertices(indexCount, vertexCount);
		s_Current = NULL;
		s_CurrentIndex = NULL;
	}

	s_Count = 0;
	s_bIndexed = false;
}
#endif // __WIN32PC

}	// namespace rage

#endif // __D3D
