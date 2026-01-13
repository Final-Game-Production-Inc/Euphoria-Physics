// 
// grcore/im_ogl.cpp 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#include "config.h"

#if __OPENGL
#include "im.h"


#include "device.h"
#include "grcore/opengl.h"
#include "profile/page.h"
#include "profile/group.h"
#include "profile/element.h"

#if __WIN32
#pragma comment(lib,"opengl32.lib")
#endif

namespace rage {

bool g_TallyGrcCalls;

namespace GraphicsStats {
	PF_PAGE(GfxPage,"Graphics Stats");
	PF_GROUP(GfxGeneric);
	PF_LINK(GfxPage,GfxGeneric);

	PF_COUNTER(grmGeometry_Draw_Calls,GfxGeneric);
	PF_COUNTER(grmGeometry_Draw_Vertices,GfxGeneric);
	PF_COUNTER(grmGeometry_Draw_Indices,GfxGeneric);
	PF_COUNTER(grmShaderFx_Bind_Calls,GfxGeneric);
	PF_COUNTER(grcViewport_SetWorldMtx_Calls,GfxGeneric);
	PF_COUNTER(grcBegin_Calls,GfxGeneric);
	PF_COUNTER(grcBegin_Vertices,GfxGeneric);
}
using namespace GraphicsStats;

Color32 grcCurrentColor;
float grcCurrentS, grcCurrentT;
float grcCurrentNX, grcCurrentNY, grcCurrentNZ;

static float s_TexCoords[grcBeginMax][2] ALIGNED(128);
static u8 s_Colors[grcBeginMax][4] ALIGNED(128);
static float s_Normals[grcBeginMax][3] ALIGNED(128);
static float s_Positions[grcBeginMax][3] ALIGNED(128);

static grcDrawMode s_DrawMode;
static int s_Base, s_Offset, s_Count;

void grcBegin(grcDrawMode dm,int count) {
	Assert("Too many verts in this begin draw call" && count <= grcBeginMax);
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
		if (s_Base + count > grcBeginMax)
			s_Base = 0;
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
		Assert(s_Count && "grcVertex outside of begin/end");
		int i = s_Base + s_Offset++;
		s_Positions[i][0] = x;
		s_Positions[i][1] = y;
		s_Positions[i][2] = z;
		s_Normals[i][0] = nx;
		s_Normals[i][1] = ny;
		s_Normals[i][2] = nz;
		s_Colors[i][0] = c.GetRed();
		s_Colors[i][1] = c.GetGreen();
		s_Colors[i][2] = c.GetBlue();
		s_Colors[i][3] = c.GetAlpha();
		s_TexCoords[i][0] = s;
		s_TexCoords[i][1] = t;
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
		// GRCDEVICE.SetDefaultVertexShader(grcState::GetLightingMode() != 0,false);
		Assert(s_Offset == s_Count);

		GLenum mode = 0;

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glVertexPointer(3,GL_FLOAT,0,s_Positions);
		glNormalPointer(GL_FLOAT,0,s_Normals);
		glColorPointer(4,GL_UNSIGNED_BYTE,0,s_Colors);
		glTexCoordPointer(2,GL_FLOAT,0,s_TexCoords);

		switch (s_DrawMode) {
			case drawPoints: mode = GL_POINTS; break;
			case drawLines: mode = GL_LINES; break;
			case drawLineStrip: mode = GL_LINE_STRIP; break;
			case drawTris: mode = GL_TRIANGLES; break;
			case drawTriStrip: mode = GL_TRIANGLE_STRIP; break;
			case drawTriFan: mode = GL_TRIANGLE_FAN; break;
			case drawQuads: mode = GL_QUADS; break;
			default:
				Assert(0 && "unhandled prim type");
		}
		glDrawArrays(mode, s_Base, s_Count);
		s_Base += s_Count;
	}
}


#if __DEV
bool grcReceivePackets(sysNamedPipe&) {
	// TODO!
	return false;
}
#endif

}	// namespace rage

#endif
