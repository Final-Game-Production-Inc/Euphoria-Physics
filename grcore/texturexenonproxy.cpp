// 
// grcore/texturexenonproxy.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "texturexenonproxy.h"

namespace rage {

#if __DECLARESTRUCT

void grcTextureXenonProxy::DeclareStruct(datTypeStruct&)
{}


#endif

grcTextureXenonProxy::grcTextureXenonProxy()
{ 
}

grcTextureXenonProxy::~grcTextureXenonProxy()
{ 
}


grcTextureXenonProxy::grcTextureXenonProxy(datResource &rsc) : grcTexture(rsc)
{
}

#if !__RESOURCECOMPILER 
void grcTextureXenonProxy::SetTextureSwizzle(eTextureSwizzle , eTextureSwizzle , eTextureSwizzle , eTextureSwizzle , bool)
{
}

void grcTextureXenonProxy::GetTextureSwizzle( eTextureSwizzle& ,  eTextureSwizzle& ,  eTextureSwizzle&, eTextureSwizzle& ) const
{
}
#endif

}	// namespace rage
