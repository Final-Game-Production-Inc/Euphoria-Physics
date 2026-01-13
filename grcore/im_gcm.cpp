// 
// grcore/im_ogl.cpp 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#if __PPU
#include "im.h"
#include "device.h"
#include "effect.h"
#include "light.h"

#include "wrapper_gcm.h"
#include "profile/page.h"
#include "profile/group.h"
#include "profile/element.h"
#include "math/float16.h"

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

	PF_COUNTER(GCLK_PRIMITIVECOUNT,GfxGeneric);
	PF_COUNTER(GCLK_SETUP_TRIANGLES,GfxGeneric);
	PF_COUNTER(GCLK_VAB_CMD_LOAD,GfxGeneric);
	PF_COUNTER(GCLK_VAB_CMD_TRI,GfxGeneric);

	/* PF_GROUP(EffectCache);
	PF_LINK(GfxPage, EffectCache);

	PF_COUNTER(EffectCacheHits, EffectCache);
	PF_COUNTER(EffectCacheMisses, EffectCache);
	PF_VALUE_FLOAT(EffectCacheHitRatio, EffectCache);
	PF_COUNTER(EffectCacheFlushes, EffectCache);
	PF_COUNTER(EffectCacheNumFlushed, EffectCache);
	PF_COUNTER(EffectCacheSizeAccumulator, EffectCache);
	PF_VALUE_FLOAT(EffectCacheUtilization, EffectCache);

	PF_GROUP(LookAhead);
	PF_LINK(GfxPage, LookAhead);
	PF_COUNTER(LookAhead1, LookAhead);
	PF_COUNTER(LookAhead2, LookAhead);
	PF_COUNTER(LookAhead3, LookAhead);
	PF_COUNTER(LookAhead4, LookAhead);
	PF_COUNTER(LookAhead5, LookAhead);
	PF_COUNTER(LookAhead6, LookAhead); */

}
using namespace GraphicsStats;

extern u16 g_VertexShaderInputs;

/* struct Vtx {
	float Position[3];
	float Normal[3];
	u8 Color[4];
	float TexCoord[2];
}; */

// Must be on game heap in order to be accessible from libgcm.
static uint32_t *s_WritePtr;
static grcDrawMode s_DrawMode;
static int s_Offset, s_Count;
static u32 s_LastVertexShaderInputs;
static int s_VtxSizeWords;		// in words

const int PerPacket = 32;
const uint32_t PacketShift = 5;

// PARAM(immspu,"safdsadf");

void grcBegin(grcDrawMode dm,int count) {
	BANK_ONLY(AssertMsg(g_AreRenderTargetsBound, "Attempting to draw with no render targets bound");)
	AssertMsg(count <= grcBeginMax, "Too many verts in this begin draw call");
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->Begin(dm,count);
	else
#endif
	{
#if __STATS
		if (g_TallyGrcCalls) {
			PF_INCREMENT(grcBegin_Calls);
			PF_INCREMENTBY(grcBegin_Vertices,count);
		}
#endif

		
		if (!grcEffect::IsInDraw())
			GRCDEVICE.SetDefaultEffect(grcLightState::IsEnabled(),false); 
		s_DrawMode = dm;
		s_Offset = 0;
		s_Count = count;
		s_VtxSizeWords = 12/4;
		int normalCount = CELL_GCM_VERTEX_F, colorCount = CELL_GCM_VERTEX_UB, texCount = CELL_GCM_VERTEX_F;
		if (g_VertexShaderInputs & (1<<2)) {
			s_VtxSizeWords += 12/4;		// Normal
			normalCount |= 3 << 4;
		}
		if (g_VertexShaderInputs & (1<<3)) {
			s_VtxSizeWords += 4/4;			// Color
			colorCount |= 4 << 4;
		}
		if (g_VertexShaderInputs & (1<<8)) {
			s_VtxSizeWords += 8/4;			// TexCoord
			texCount |= 2 << 4;
		}
		int stride = s_VtxSizeWords << 2;

		grcStateBlock::FlushThrough(false);
		SPU_COMMAND(grcDevice__grcBegin,normalCount);
		cmd->common = stride << 8;		// u16
		cmd->colorCount = colorCount;	// u8
		cmd->texCount = texCount;		// u8
		s_LastVertexShaderInputs = g_VertexShaderInputs;

		uint32_t mode = 0;
		switch (s_DrawMode) {
			case drawPoints: mode = CELL_GCM_PRIMITIVE_POINTS; break;
			case drawLines: mode = CELL_GCM_PRIMITIVE_LINES; break;
			case drawLineStrip: mode = CELL_GCM_PRIMITIVE_LINE_STRIP; break;
			case drawTris: mode = CELL_GCM_PRIMITIVE_TRIANGLES; break;
			case drawTriStrip: mode = CELL_GCM_PRIMITIVE_TRIANGLE_STRIP; break;
			case drawTriFan: mode = CELL_GCM_PRIMITIVE_TRIANGLE_FAN; break;
			case drawQuads: mode = CELL_GCM_PRIMITIVE_QUADS; break;
			default:
				AssertMsg(0 , "unhandled prim type");
		}

		// BEGIN GCM HUD BUG WORKAROUND
		uint32_t packetCount = (count+(PerPacket-1)) >> PacketShift;
		uint32_t vtxSize = count * s_VtxSizeWords;
		uint32_t wordCount = 6 + packetCount + vtxSize + 2;
		// END GCM HUD BUG WORKAROUND

		if (GCM_CONTEXT->current + wordCount >= GCM_CONTEXT->end)
			GCM_CONTEXT->callback(GCM_CONTEXT, wordCount);

		GCM_DEBUG(GCM::cellGcmSetDrawBegin(GCM_CONTEXT,mode));
	}
}

/* static inline u32 PackNormalComponent(float value,u32 size,u32 shift) {
	float scale = (float)((1 << (size-1)) - 1);
	return (u32(value * scale) & ((1 << size)-1)) << shift;
}

static inline u32 PackNormal(float x,float y,float z) {
	return PackNormalComponent(x,11,0) | PackNormalComponent(y,11,11) | PackNormalComponent(z,10,22);
} */

void grcVertex(float x,float y,float z,float nx,float ny,float nz,Color32 c,float s,float t) {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->Vertex(x,y,z,nx,ny,nz,c,s,t);
	else
#endif
	{
		if ((s_Offset & (PerPacket-1)) == 0) {
			int count = s_Count - s_Offset;
			if (count > PerPacket)
				count = PerPacket;
			GCM_DEBUG(GCM::cellGcmSetDrawInlineArrayPointer(GCM_CONTEXT,count * s_VtxSizeWords,(void**)&s_WritePtr));
		}
		AssertMsg(s_Count , "grcVertex outside of begin/end");
		u8 *v = (u8*) s_WritePtr;
		s_WritePtr += s_VtxSizeWords;
		s_Offset++;
		float *p = (float*) v;
		v += 12;
		p[0] = x;
		p[1] = y;
		p[2] = z;

		if (s_LastVertexShaderInputs & (1<<2)) {
			float *n = (float*) v;
			v += 12;
			n[0] = nx;
			n[1] = ny;
			n[2] = nz;
		}

		if (s_LastVertexShaderInputs & (1<<3)) {
			u8 *c_ = (u8*) v;
			v += 4;
			c_[0] = c.GetRed();
			c_[1] = c.GetGreen();
			c_[2] = c.GetBlue();
			c_[3] = c.GetAlpha();
		}

		if (s_LastVertexShaderInputs & (1<<8)) {
			float *tc = (float*) v;
			tc[0] = s;
			tc[1] = t;
			// no reason to increment v here.
		}
		Assert(s_Offset <= s_Count);
	}
}


void grcEnd() {
#if __IM_BATCHER
	if (grcBatcher::GetCurrent())
		grcBatcher::GetCurrent()->End();
	else
#endif
	{
		GCM_DEBUG(GCM::cellGcmSetDrawEnd(GCM_CONTEXT));
		InvalidateSpuGcmState(CachedStates.VertexFormats, ~0);
		s_WritePtr = NULL;
	}
}

#if __DEV
bool grcReceivePackets(sysNamedPipe&) {
	// TODO!
	return false;
}
#endif

}	// namespace rage

#endif	// __PPU
