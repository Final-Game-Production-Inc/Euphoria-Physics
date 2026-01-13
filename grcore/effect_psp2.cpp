//
// grcore/effect_psp2.cpp
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//
#if __PSP2

#include "effect.h"
#include "effect_internal.h"

namespace rage {

void SamplerState::Set(u32,u32)
{
}

void grcVertexProgram::SetParameter(int,const float*,int)
{
}

void grcFragmentProgram::SetParameter(int,const float*,int)
{
}

void grcEffect::SetRenderState(grceRenderState,int)
{
}

}	// namespace rage

#endif	// __PSP2