// 
// grcore/state_ogl.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "config.h"
#if __OPENGL

#include "opengl.h"
#include "effect.h"

namespace rage {

static u32 s_StateShadow[grcsCount];

void grcState::SetState(grcStateType type,u32 value) {
	static GLenum TranslateFunc[] = { GL_NEVER, GL_ALWAYS, GL_LESS,
		GL_LEQUAL, GL_EQUAL, GL_GEQUAL, GL_GREATER, GL_NOTEQUAL };

	s_StateShadow[type] = value;

	switch (type) {
		case grcsCullMode:
			{
				switch (value) {
				case grccmNone:
					glDisable(GL_CULL_FACE);
					break;
				case grccmFront:
					glEnable(GL_CULL_FACE);
					glCullFace(GL_FRONT);
					break;
				case grccmBack:
					glEnable(GL_CULL_FACE);
					glCullFace(GL_BACK);
					break;
				}
				break;
			}
		case grcsColorWrite:
			{
				glColorMask((value & grccwRed) != 0,(value & grccwGreen) != 0,
					(value & grccwBlue) != 0, (value & grccwAlpha) != 0);
				break;
			}
		case grcsAlphaBlend:
			{
				if (value) 
					glEnable(GL_BLEND); 
				else 
					glDisable(GL_BLEND);
				break;
			}
		case grcsAlphaTest:
			{
				if (value)
					glEnable(GL_ALPHA_TEST);
				else
					glDisable(GL_ALPHA_TEST);
				break;
			}
		case grcsBlendSet:
			{
				sm_BlendSet = (grcBlendSet) value;
				switch (value) 
				{
					case grcbsNormal:    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
					case grcbsAdd:       glBlendFunc(GL_ONE, GL_ONE); break;
					case grcbsOverwrite: glBlendFunc(GL_ONE, GL_ZERO); break;
					case grcbsAlphaAdd:	 glBlendFunc(GL_SRC_ALPHA, GL_ONE); break; // src*srcAlpha + dest
					default:             grcErrorf("OGL: BlendSet Unimplemented [%d]", value);
				}
				break;
			}
		case grcsAlphaFunc:
			{
				glAlphaFunc(TranslateFunc[value],s_StateShadow[grcsAlphaRef] / 255.0f);
				break;
			}
		case grcsAlphaRef:
			{
				glAlphaFunc(TranslateFunc[s_StateShadow[grcsAlphaFunc]],value / 255.0f);
				break;
			}
		case grcsFillMode:
			{
				static GLenum TranslateFill[] = { GL_POINT, GL_LINE, GL_FILL };
				glPolygonMode(GL_FRONT_AND_BACK, TranslateFill[value]);
				break;
			}
		case grcsDepthFunc:
			{
				glDepthFunc(TranslateFunc[value]);
				break;
			}
		case grcsDepthTest:
			{
				if (value)
					glEnable(GL_DEPTH_TEST);
				else
					glDisable(GL_DEPTH_TEST);
				break;
			}
		case grcsDepthWrite:
			{
				glDepthMask(value != 0);
				break;
			}
		case grcsLighting:
			{
				sm_Lighting = (grcLightingMode) value;
				break;
			}
		case grcsDepthBias:
		case grcsSlopeScaleDepthBias:
			{
				break;
			}
		case grcsCount:
		default:
			AssertMsg(0,"Invalid grcstate");
	}
}


u32 grcState::GetState(grcStateType type) {
	return s_StateShadow[type];
}


}	// namepsace rage
#endif	// __OPENGL
