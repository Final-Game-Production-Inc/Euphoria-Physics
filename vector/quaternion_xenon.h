// 
// vector/quaternion_xenon.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

namespace rage
{

#ifndef QUAT_SET_Q
#define QUAT_SET_Q
inline void Quaternion::Set(const Quaternion& q)
{
	xyzw = q.xyzw;
}
#endif // QUAT_SET_Q

#ifndef QUAT_NEGATE
#define QUAT_NEGATE
inline void Quaternion::Negate()
{
	xyzw = __vsubfp(__vxor(xyzw, xyzw), xyzw);
}
#endif // QUAT_NEGATE

#ifndef QUAT_NEGATE_Q
#define QUAT_NEGATE_Q
inline void Quaternion::Negate(const Quaternion &q)
{
	xyzw = __vsubfp(__vxor(q.xyzw, q.xyzw), q.xyzw);
}
#endif // QUAT_NEGATE_Q

#ifndef QUAT_NORMALIZE
#define QUAT_NORMALIZE
inline void Quaternion::Normalize()
{
	__vector4 dot = __vdot4fp(xyzw, xyzw);
	__vector4 rsqrt = NewtonRaphsonRsqrt(dot);
	xyzw = __vmulfp(xyzw, rsqrt);
}
#endif // QUAT_NORMALIZE

#ifndef QUAT_NORMALIZE_Q
#define QUAT_NORMALIZE_Q
inline void Quaternion::Normalize(const Quaternion& q)
{
	__vector4 dot = __vdot4fp(q.xyzw, q.xyzw);
	__vector4 rsqrt = NewtonRaphsonRsqrt(dot);
	xyzw = __vmulfp(q.xyzw, rsqrt);
}
#endif // QUAT_NORMALIZE_Q


#ifndef QUAT_ISEQUAL
#define QUAT_ISEQUAL
inline bool Quaternion::IsEqual (const Quaternion& q) const
{
	u32 cr;
	__vcmpeqfpR(q.xyzw, xyzw, &cr);
	return ((cr & VEC3_CMP_VAL) != 0);
}
#endif // QUAT_ISEQUAL

} // namespace rage
