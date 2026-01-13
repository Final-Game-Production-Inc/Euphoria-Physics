// 
// grcore/effect_values.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "effect_values.h"

#if __WIN32PC || __XENON
#include "system/xtl.h"
#include "system/d3d9.h"
#include "system/d3d11.h"

using namespace rage::grcRSV;

#define Check(x) typedef int __check__##x[x==D3D##x? 1 : -1]

Check(FILL_POINT);
Check(FILL_WIREFRAME);
Check(FILL_SOLID);

Check(BLEND_ZERO);
Check(BLEND_ONE);
Check(BLEND_SRCCOLOR);
Check(BLEND_INVSRCCOLOR);
Check(BLEND_SRCALPHA);
Check(BLEND_INVSRCALPHA);
Check(BLEND_DESTCOLOR);
Check(BLEND_INVDESTCOLOR);
Check(BLEND_DESTALPHA);
Check(BLEND_INVDESTALPHA);
Check(BLEND_BLENDFACTOR);
Check(BLEND_INVBLENDFACTOR);
#if __XENON
Check(BLEND_CONSTANTALPHA);
Check(BLEND_INVCONSTANTALPHA);
#endif
Check(BLEND_SRCALPHASAT);

#if !__RESOURCECOMPILER && (RSG_PC || RSG_DURANGO)
// Front faces are CCW on all NG platforms. D3D11 says to cull front, or back faces rather than CW/CCW faces.
CompileTimeAssert(CULL_NONE == D3D11_CULL_NONE);
CompileTimeAssert(CULL_CW == D3D11_CULL_BACK);
CompileTimeAssert(CULL_CCW == D3D11_CULL_FRONT);
#else // __D3D11
Check(CULL_NONE);
Check(CULL_CW);
Check(CULL_CCW);
#endif // __D3D11

Check(CMP_NEVER);
Check(CMP_LESS);
Check(CMP_EQUAL);
Check(CMP_LESSEQUAL);
Check(CMP_GREATER);
Check(CMP_NOTEQUAL);
Check(CMP_GREATEREQUAL);
Check(CMP_ALWAYS);

Check(STENCILOP_KEEP);
Check(STENCILOP_ZERO);
Check(STENCILOP_REPLACE);
Check(STENCILOP_INCRSAT);
Check(STENCILOP_DECRSAT);
Check(STENCILOP_INVERT);
typedef int __STENCILOP_INCR[STENCILOP_INCRWRAP==D3DSTENCILOP_INCR? 1 : -1];
typedef int __STENCILOP_DECR[STENCILOP_DECRWRAP==D3DSTENCILOP_DECR? 1 : -1];

Check(COLORWRITEENABLE_RED);
Check(COLORWRITEENABLE_GREEN);
Check(COLORWRITEENABLE_BLUE);
Check(COLORWRITEENABLE_ALPHA);

Check(BLENDOP_ADD);
Check(BLENDOP_SUBTRACT);
Check(BLENDOP_MIN);
Check(BLENDOP_MAX);
Check(BLENDOP_REVSUBTRACT);

using namespace rage::grcSSV;

Check(TADDRESS_WRAP);
Check(TADDRESS_CLAMP);
Check(TADDRESS_MIRROR);
Check(TADDRESS_MIRRORONCE);
Check(TADDRESS_BORDER);
#if __XENON
Check(TADDRESS_BORDER_HALF);
Check(TADDRESS_MIRRORONCE_BORDER_HALF);
Check(TADDRESS_MIRRORONCE_BORDER);
#endif

Check(TEXF_NONE);
Check(TEXF_POINT);
Check(TEXF_LINEAR);
Check(TEXF_ANISOTROPIC);
#if __WIN32PC
Check(TEXF_PYRAMIDALQUAD);
Check(TEXF_GAUSSIANQUAD);
#endif

#undef Check

#endif // __WIN32
