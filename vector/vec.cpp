//
// vector/vec.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "vec.h"
#include "math/amath.h"

using namespace rage;

void Mtx_xyz::FromEulersXZY(const Vec_xyz &e) {
#if 1
	float ex = ((float*)&e)[0];
	float ey = ((float*)&e)[1];
	float ez = ((float*)&e)[2];
	float sx,sy,sz,cx,cy,cz;
	if(ex==0.0f) {sx=0.0f; cx=1.0f;}
	else {cos_and_sin(cx,sx,ex);}
	if(ey==0.0f) {sy=0.0f; cy=1.0f;}
	else {cos_and_sin(cy,sy,ey);}
	if(ez==0.0f) {sz=0.0f; cz=1.0f;}
	else {cos_and_sin(cz,sz,ez);}

#else
	float cosT[4], sinT[4];
	cos_and_sin4(cosT,sinT,(float*)&e);		//lint !e740 unusual pointer cast

#define cx cosT[0]
#define cy cosT[1]
#define cz cosT[2]
#define sx sinT[0]
#define sy sinT[1]
#define sz sinT[2]
#endif
	a.Set(  cz*cy,             sz,    -cz*sy);
	b.Set( -cx*sz*cy + sx*sy,  cx*cz,  cx*sz*sy + sx*cy);
	c.Set(  sx*sz*cy + cx*sy, -sx*cz, -sx*sz*sy + cx*cy);
}
