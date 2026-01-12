// 
// vector/Color64.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "color64.h"
#include "vector3.h"
#include "vector4.h"

using namespace rage;

namespace Color64_util {

static __forceinline float Clamp_0_1(float f)
{
	return f<0? 0 : f>1? 1 : f;
}

static __forceinline u64 Clamp_0_65535(int f)
{
	return f<0? 0 : f>66535? 66535 : u64(f);
}

} // namespace Color64_util

// Constructor
Color64::Color64(const Vector3& color)
{
	Setf(Color64_util::Clamp_0_1(color.x), Color64_util::Clamp_0_1(color.y), Color64_util::Clamp_0_1(color.z));
}

// Constructor
Color64::Color64(const Vector4& color)
{
	Setf(Color64_util::Clamp_0_1(color.x), Color64_util::Clamp_0_1(color.y), Color64_util::Clamp_0_1(color.z), Color64_util::Clamp_0_1(color.w));
}

Color64 Color64::operator*(const Color64 that) const
{
	return Color64(
		(GetRed  () * that.GetRed  ()) / 65535,
		(GetGreen() * that.GetGreen()) / 65535,
		(GetBlue () * that.GetBlue ()) / 65535,
		(GetAlpha() * that.GetAlpha()) / 65535
	);
}

void Color64::Set(int r,int g,int b,int a)
{
	m_Color = Color64_util::Clamp_0_65535(b) | (Color64_util::Clamp_0_65535(g) << 16) | (Color64_util::Clamp_0_65535(r) << 32) | (Color64_util::Clamp_0_65535(a) << 48);
}
