// 
// vector/matrix34_xenon.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_MATRIX34_XENON_H
#define VECTOR_MATRIX34_XENON_H

// Default Matrix34 Implementations

namespace rage
{

	//=============================================================================
	// Implementations

#ifndef MATRIX34_IDENTITY3X3
#define MATRIX34_IDENTITY3X3
inline void Matrix34::Identity3x3()
{
#if __XENON // take advantage of __vupkd3d
	__vector4 zeroInZ_oneInW = __vupkd3d( _vzerofp, VPACK_NORMSHORT2 );

	a = Vec4VectorSwizzle( zeroInZ_oneInW, VEC_PERM_W, VEC_PERM_Z, VEC_PERM_Z, VEC_PERM_Z );
	b = Vec4VectorSwizzle( zeroInZ_oneInW, VEC_PERM_Z, VEC_PERM_W, VEC_PERM_Z, VEC_PERM_Z );
	c = Vec4VectorSwizzle( zeroInZ_oneInW, VEC_PERM_Z, VEC_PERM_Z, VEC_PERM_W, VEC_PERM_Z );
#elif __PS3
	__vector4 _zero = _vzerofp;
	__vector4 _1000 = (__vector4)vec_sld( vec_ctf( _vfour1, 0 ), _zero, 0xC );
	__vector4 _0100 = __vmrghw( _zero, _1000 );
	__vector4 _0010 = __vmrghw( _0100, _zero );
	a = _1000;
	b = _0100;
	c = _0010;
#else
	a.Set(1.0f,0.0f,0.0f);
	b.Set(0.0f,1.0f,0.0f);
	c.Set(0.0f,0.0f,1.0f);
#endif
}
#endif	// MATRIX34_IDENTITY3X3

#ifndef MATRIX34_ZERO3X3
#define MATRIX34_ZERO3X3
__forceinline void Matrix34::Zero3x3()
{
	a = b = c = _vzerofp;
}
#endif // MATRIX34_ZERO3X3

#ifndef MATRIX34_ZERO
#define MATRIX34_ZERO
__forceinline void Matrix34::Zero()
{
	a = b = c = d = _vzerofp;
}
#endif // MATRIX34_ZERO

#ifndef MATRIX34_SET3X3
#define MATRIX34_SET3X3
__forceinline void Matrix34::Set3x3(const Matrix34 &m)
{
	a = m.a;
	b = m.b;
	c = m.c;
}
#endif // MATRIX34_SET3X3

#ifndef MATRIX34_SET_M
#define MATRIX34_SET_M
__forceinline void Matrix34::Set(const Matrix34 &m)
{
	a = m.a;
	b = m.b;
	c = m.c;
	d = m.d;
}
#endif // MATRIX34_SET_M

#ifndef MATRIX34_SET_V
#define MATRIX34_SET_V
__forceinline void Matrix34::Set(const Vector3& newA, const Vector3& newB, const Vector3& newC, const Vector3& newD)
{
	a = newA;
	b = newB;
	c = newC;
	d = newD;
}
#endif // MATRIX34_SET_V

#ifndef MATRIX34_SET_DIAGONAL
#define MATRIX34_SET_DIAGONAL
__forceinline void Matrix34::SetDiagonal( Vector3::Vector3Param diag )
{
	Vector3 dd = diag;
	dd.And( VEC3_ANDW );
	a.Permute<VEC_PERM_X, VEC_PERM_W, VEC_PERM_W, VEC_PERM_W>(dd);
	b.Permute<VEC_PERM_W, VEC_PERM_Y, VEC_PERM_W, VEC_PERM_W>(dd);
	c.Permute<VEC_PERM_W, VEC_PERM_W, VEC_PERM_Z, VEC_PERM_W>(dd);
	
}
#endif

#ifndef MATRIX34_DOT
#define MATRIX34_DOT
__forceinline void Matrix34::Dot(const Matrix34 &m) 
{
	// Load the input matrices into registers. /FF
	const __vector4 q_a = m.a.xyzw;
	const __vector4 q_b = m.b.xyzw;
	const __vector4 q_c = m.c.xyzw;
	const __vector4 q_d = m.d.xyzw;
	const __vector4 p_a = a.xyzw;
	const __vector4 p_b = b.xyzw;
	const __vector4 p_c = c.xyzw;
	const __vector4 p_d = d.xyzw;

#if !__XENON
	// Xenon doesn't need a zero vector since it's got the vmulfp instruction. /FF
	const __vector4 zero = _vzerofp;
#endif

	// Splat the elements of the p matrix. /FF
	const __vector4 splat_pax = __vspltw(p_a, 0);
	const __vector4 splat_pay = __vspltw(p_a, 1);
	const __vector4 splat_paz = __vspltw(p_a, 2);
	const __vector4 splat_pbx = __vspltw(p_b, 0);
	const __vector4 splat_pby = __vspltw(p_b, 1);
	const __vector4 splat_pbz = __vspltw(p_b, 2);
	const __vector4 splat_pcx = __vspltw(p_c, 0);
	const __vector4 splat_pcy = __vspltw(p_c, 1);
	const __vector4 splat_pcz = __vspltw(p_c, 2);
	const __vector4 splat_pdx = __vspltw(p_d, 0);
	const __vector4 splat_pdy = __vspltw(p_d, 1);
	const __vector4 splat_pdz = __vspltw(p_d, 2);

	// Do the multiplications and additions. /FF

#if __XENON
	const __vector4 da1 = __vmulfp(q_a, splat_pax);
	const __vector4 db1 = __vmulfp(q_a, splat_pbx);
	const __vector4 dc1 = __vmulfp(q_a, splat_pcx);
#else
	const __vector4 da1 = __vmaddfp(q_a, splat_pax, zero);
	const __vector4 db1 = __vmaddfp(q_a, splat_pbx, zero);
	const __vector4 dc1 = __vmaddfp(q_a, splat_pcx, zero);
#endif
	const __vector4 dd1 = __vmaddfp(q_a, splat_pdx, q_d);

	const __vector4 da2 = __vmaddfp(q_b, splat_pay, da1);
	const __vector4 db2 = __vmaddfp(q_b, splat_pby, db1);
	const __vector4 dc2 = __vmaddfp(q_b, splat_pcy, dc1);
	const __vector4 dd2 = __vmaddfp(q_b, splat_pdy, dd1);

	const __vector4 da = __vmaddfp(q_c, splat_paz, da2);
	const __vector4 db = __vmaddfp(q_c, splat_pbz, db2);
	const __vector4 dc = __vmaddfp(q_c, splat_pcz, dc2);
	const __vector4 dd = __vmaddfp(q_c, splat_pdz, dd2);

	// Store the output. /FF

	a.xyzw = da;
	b.xyzw = db;
	c.xyzw = dc;
	d.xyzw = dd;
}
#endif // MATRIX34_DOT

#ifndef MATRIX34_DOTFROMLEFT
#define MATRIX34_DOTFROMLEFT
__forceinline void Matrix34::DotFromLeft(const Matrix34& m)
{
	// Load the input matrices into registers. /FF
	const __vector4 q_a = a.xyzw;
	const __vector4 q_b = b.xyzw;
	const __vector4 q_c = c.xyzw;
	const __vector4 q_d = d.xyzw;
	const __vector4 p_a = m.a.xyzw;
	const __vector4 p_b = m.b.xyzw;
	const __vector4 p_c = m.c.xyzw;
	const __vector4 p_d = m.d.xyzw;

#if !__XENON
	// Xenon doesn't need a zero vector since it's got the vmulfp instruction. /FF
	const __vector4 zero = _vzerofp;
#endif

	// Splat the elements of the p matrix. /FF
	const __vector4 splat_pax = __vspltw(p_a, 0);
	const __vector4 splat_pay = __vspltw(p_a, 1);
	const __vector4 splat_paz = __vspltw(p_a, 2);
	const __vector4 splat_pbx = __vspltw(p_b, 0);
	const __vector4 splat_pby = __vspltw(p_b, 1);
	const __vector4 splat_pbz = __vspltw(p_b, 2);
	const __vector4 splat_pcx = __vspltw(p_c, 0);
	const __vector4 splat_pcy = __vspltw(p_c, 1);
	const __vector4 splat_pcz = __vspltw(p_c, 2);
	const __vector4 splat_pdx = __vspltw(p_d, 0);
	const __vector4 splat_pdy = __vspltw(p_d, 1);
	const __vector4 splat_pdz = __vspltw(p_d, 2);

	// Do the multiplications and additions. /FF

#if __XENON
	const __vector4 da1 = __vmulfp(q_a, splat_pax);
	const __vector4 db1 = __vmulfp(q_a, splat_pbx);
	const __vector4 dc1 = __vmulfp(q_a, splat_pcx);
#else
	const __vector4 da1 = __vmaddfp(q_a, splat_pax, zero);
	const __vector4 db1 = __vmaddfp(q_a, splat_pbx, zero);
	const __vector4 dc1 = __vmaddfp(q_a, splat_pcx, zero);
#endif
	const __vector4 dd1 = __vmaddfp(q_a, splat_pdx, q_d);

	const __vector4 da2 = __vmaddfp(q_b, splat_pay, da1);
	const __vector4 db2 = __vmaddfp(q_b, splat_pby, db1);
	const __vector4 dc2 = __vmaddfp(q_b, splat_pcy, dc1);
	const __vector4 dd2 = __vmaddfp(q_b, splat_pdy, dd1);

	const __vector4 da = __vmaddfp(q_c, splat_paz, da2);
	const __vector4 db = __vmaddfp(q_c, splat_pbz, db2);
	const __vector4 dc = __vmaddfp(q_c, splat_pcz, dc2);
	const __vector4 dd = __vmaddfp(q_c, splat_pdz, dd2);

	// Store the output. /FF

	a.xyzw = da;
	b.xyzw = db;
	c.xyzw = dc;
	d.xyzw = dd;
}
#endif // MATRIX34_DOTFROMLEFT

#ifndef MATRIX34_DOT_M
#define MATRIX34_DOT_M
__forceinline void Matrix34::Dot(const Matrix34 &m, const Matrix34 &n)
{
	// Load the input matrices into registers. /FF
	const __vector4 q_a = n.a.xyzw;
	const __vector4 q_b = n.b.xyzw;
	const __vector4 q_c = n.c.xyzw;
	const __vector4 q_d = n.d.xyzw;
	const __vector4 p_a = m.a.xyzw;
	const __vector4 p_b = m.b.xyzw;
	const __vector4 p_c = m.c.xyzw;
	const __vector4 p_d = m.d.xyzw;

#if !__XENON
	// Xenon doesn't need a zero vector since it's got the vmulfp instruction. /FF
	const __vector4 zero = _vzerofp;
#endif

	// Splat the elements of the p matrix. /FF
	const __vector4 splat_pax = __vspltw(p_a, 0);
	const __vector4 splat_pay = __vspltw(p_a, 1);
	const __vector4 splat_paz = __vspltw(p_a, 2);
	const __vector4 splat_pbx = __vspltw(p_b, 0);
	const __vector4 splat_pby = __vspltw(p_b, 1);
	const __vector4 splat_pbz = __vspltw(p_b, 2);
	const __vector4 splat_pcx = __vspltw(p_c, 0);
	const __vector4 splat_pcy = __vspltw(p_c, 1);
	const __vector4 splat_pcz = __vspltw(p_c, 2);
	const __vector4 splat_pdx = __vspltw(p_d, 0);
	const __vector4 splat_pdy = __vspltw(p_d, 1);
	const __vector4 splat_pdz = __vspltw(p_d, 2);

	// Do the multiplications and additions. /FF

#if __XENON
	const __vector4 da1 = __vmulfp(q_a, splat_pax);
	const __vector4 db1 = __vmulfp(q_a, splat_pbx);
	const __vector4 dc1 = __vmulfp(q_a, splat_pcx);
#else
	const __vector4 da1 = __vmaddfp(q_a, splat_pax, zero);
	const __vector4 db1 = __vmaddfp(q_a, splat_pbx, zero);
	const __vector4 dc1 = __vmaddfp(q_a, splat_pcx, zero);
#endif
	const __vector4 dd1 = __vmaddfp(q_a, splat_pdx, q_d);

	const __vector4 da2 = __vmaddfp(q_b, splat_pay, da1);
	const __vector4 db2 = __vmaddfp(q_b, splat_pby, db1);
	const __vector4 dc2 = __vmaddfp(q_b, splat_pcy, dc1);
	const __vector4 dd2 = __vmaddfp(q_b, splat_pdy, dd1);

	const __vector4 da = __vmaddfp(q_c, splat_paz, da2);
	const __vector4 db = __vmaddfp(q_c, splat_pbz, db2);
	const __vector4 dc = __vmaddfp(q_c, splat_pcz, dc2);
	const __vector4 dd = __vmaddfp(q_c, splat_pdz, dd2);

	// Store the output. /FF

	a.xyzw = da;
	b.xyzw = db;
	c.xyzw = dc;
	d.xyzw = dd;


	// Here is the previous version of this function, based on __vdot[3/4]fp.
	// It's provided here for future reference in case somebody wants to give
	// it a try again (my tests showed the new function to be significantly
	// faster on Xenon). Note that this old version for some reason *loads*
	// from the destination matrix while merging, which seems unnecessary,
	// but even without that I found the new version to be faster. /FF
	//
	//// Set m.d.w = 1.0f
	//__vector4 vd = __vor(__vand(m.d.xyzw, VEC3_ANDW), VEC3_ONEW);
	//
	//// Transpose n
	//__vector4 temp0 = __vmrghw(n.a, n.c);
	//__vector4 temp1 = __vmrghw(n.b, n.d);
	//__vector4 temp2 = __vmrglw(n.a, n.c);
	//__vector4 temp3 = __vmrglw(n.b, n.d);
	//
	//__vector4 transposed0 = __vmrghw(temp0, temp1);
	//__vector4 transposed1 = __vmrglw(temp0, temp1);
	//__vector4 transposed2 = __vmrghw(temp2, temp3);
	//
	//// Multiply
	//__vector4 ax = __vdot3fp(transposed0, m.a);
	//__vector4 ay = __vdot3fp(transposed1, m.a);
	//__vector4 az = __vdot3fp(transposed2, m.a);
	//
	//__vector4 bx = __vdot3fp(transposed0, m.b);
	//__vector4 by = __vdot3fp(transposed1, m.b);
	//__vector4 bz = __vdot3fp(transposed2, m.b);
	//
	//__vector4 cx = __vdot3fp(transposed0, m.c);
	//__vector4 cy = __vdot3fp(transposed1, m.c);
	//__vector4 cz = __vdot3fp(transposed2, m.c);
	//
	//__vector4 dx = __vdot4fp(transposed0, vd);
	//__vector4 dy = __vdot4fp(transposed1, vd);
	//__vector4 dz = __vdot4fp(transposed2, vd);
	//
	//__vector4 axaz = __vmrghw(ax, az);
	//__vector4 aynn = __vmrghw(ay, a);
	//
	//__vector4 bxbz = __vmrghw(bx, bz);
	//__vector4 bynn = __vmrghw(by, b);
	// 
	//__vector4 cxcz = __vmrghw(cx, cz);
	//__vector4 cynn = __vmrghw(cy, c);
	//
	//__vector4 dxdz = __vmrghw(dx, dz);
	//__vector4 dynn = __vmrghw(dy, d);
	//
	//a.xyzw = __vmrghw(axaz, aynn);
	//b.xyzw = __vmrghw(bxbz, bynn);
	//c.xyzw = __vmrghw(cxcz, cynn);
	//d.xyzw = __vmrghw(dxdz, dynn);
}
#endif // MATRIX34_DOT_M

#ifndef MATRIX34_DOTTRANSPOSE
#define MATRIX34_DOTTRANSPOSE
__forceinline void Matrix34::DotTranspose(const Matrix34 &n)
{
	__vector4 vd = __vsubfp(d, n.d);
#if __XENON
	__vector4 ax = __vdot3fp(a, n.a);
	__vector4 ay = __vdot3fp(a, n.b);
	__vector4 az = __vdot3fp(a, n.c);

	__vector4 bx = __vdot3fp(b, n.a);
	__vector4 by = __vdot3fp(b, n.b);
	__vector4 bz = __vdot3fp(b, n.c);

	__vector4 cx = __vdot3fp(c, n.a);
	__vector4 cy = __vdot3fp(c, n.b);
	__vector4 cz = __vdot3fp(c, n.c);

	__vector4 dx = __vdot3fp(vd, n.a);
	__vector4 dy = __vdot3fp(vd, n.b);
	__vector4 dz = __vdot3fp(vd, n.c);

	__vector4 axaz = __vmrghw(ax, az);
	__vector4 aynn = __vmrghw(ay, a);

	__vector4 bxbz = __vmrghw(bx, bz);
	__vector4 bynn = __vmrghw(by, b);
	 
	__vector4 cxcz = __vmrghw(cx, cz);
	__vector4 cynn = __vmrghw(cy, c);

	__vector4 dxdz = __vmrghw(dx, dz);
	__vector4 dynn = __vmrghw(dy, d);
	
	a.xyzw = __vmrghw(axaz, aynn);
	b.xyzw = __vmrghw(bxbz, bynn);
	c.xyzw = __vmrghw(cxcz, cynn);
	d.xyzw = __vmrghw(dxdz, dynn);
#else
	__vector4 temp0 = __vmrghw(n.a, n.c);
	__vector4 temp1 = __vmrghw(n.b, n.c);
	__vector4 temp2 = __vmrglw(n.a, n.c);
	__vector4 temp3 = __vmrglw(n.b, n.c);
	__vector4 ta = __vmrghw(temp0, temp1);
	__vector4 tb = __vmrglw(temp0, temp1);
	__vector4 tc = __vmrghw(temp2, temp3);	
	__vector4 va = a;
	__vector4 vb = b;
	__vector4 vc = c;
	a = __vmaddfp(__vspltw(va, 0), ta, __vmaddfp(__vspltw(va, 1), tb, __vmulfp(__vspltw(va, 2), tc)));
	b = __vmaddfp(__vspltw(vb, 0), ta, __vmaddfp(__vspltw(vb, 1), tb, __vmulfp(__vspltw(vb, 2), tc)));
	c = __vmaddfp(__vspltw(vc, 0), ta, __vmaddfp(__vspltw(vc, 1), tb, __vmulfp(__vspltw(vc, 2), tc)));
	d = __vmaddfp(__vspltw(vd, 0), ta, __vmaddfp(__vspltw(vd, 1), tb, __vmulfp(__vspltw(vd, 2), tc)));
#endif
}
#endif // MATRIX34_DOTTRANSPOSE

#ifndef MATRIX34_DOTTRANSPOSE_M
#define MATRIX34_DOTTRANSPOSE_M
__forceinline void Matrix34::DotTranspose(const Matrix34 &m, const Matrix34 &n)
{
	__vector4 vd = __vsubfp(m.d, n.d);
#if __XENON
	__vector4 ax = __vdot3fp(m.a, n.a);
	__vector4 ay = __vdot3fp(m.a, n.b);
	__vector4 az = __vdot3fp(m.a, n.c);

	__vector4 bx = __vdot3fp(m.b, n.a);
	__vector4 by = __vdot3fp(m.b, n.b);
	__vector4 bz = __vdot3fp(m.b, n.c);

	__vector4 cx = __vdot3fp(m.c, n.a);
	__vector4 cy = __vdot3fp(m.c, n.b);
	__vector4 cz = __vdot3fp(m.c, n.c);

	__vector4 dx = __vdot3fp(vd, n.a);
	__vector4 dy = __vdot3fp(vd, n.b);
	__vector4 dz = __vdot3fp(vd, n.c);

	__vector4 axaz = __vmrghw(ax, az);
	__vector4 aynn = __vmrghw(ay, a);

	__vector4 bxbz = __vmrghw(bx, bz);
	__vector4 bynn = __vmrghw(by, b);
	 
	__vector4 cxcz = __vmrghw(cx, cz);
	__vector4 cynn = __vmrghw(cy, c);

	__vector4 dxdz = __vmrghw(dx, dz);
	__vector4 dynn = __vmrghw(dy, d);
	
	a.xyzw = __vmrghw(axaz, aynn);
	b.xyzw = __vmrghw(bxbz, bynn);
	c.xyzw = __vmrghw(cxcz, cynn);
	d.xyzw = __vmrghw(dxdz, dynn);
#else
	__vector4 temp0 = __vmrghw(n.a, n.c);
	__vector4 temp1 = __vmrghw(n.b, n.c);
	__vector4 temp2 = __vmrglw(n.a, n.c);
	__vector4 temp3 = __vmrglw(n.b, n.c);
	__vector4 ta = __vmrghw(temp0, temp1);
	__vector4 tb = __vmrglw(temp0, temp1);
	__vector4 tc = __vmrghw(temp2, temp3);	
	__vector4 va = m.a;
	__vector4 vb = m.b;
	__vector4 vc = m.c;
	a = __vmaddfp(__vspltw(va, 0), ta, __vmaddfp(__vspltw(va, 1), tb, __vmulfp(__vspltw(va, 2), tc)));
	b = __vmaddfp(__vspltw(vb, 0), ta, __vmaddfp(__vspltw(vb, 1), tb, __vmulfp(__vspltw(vb, 2), tc)));
	c = __vmaddfp(__vspltw(vc, 0), ta, __vmaddfp(__vspltw(vc, 1), tb, __vmulfp(__vspltw(vc, 2), tc)));
	d = __vmaddfp(__vspltw(vd, 0), ta, __vmaddfp(__vspltw(vd, 1), tb, __vmulfp(__vspltw(vd, 2), tc)));
#endif
}
#endif // MATRIX34_DOTTRANSPOSE_M

#ifndef MATRIX34_DOT3X3
#define MATRIX34_DOT3X3
__forceinline void Matrix34::Dot3x3(const Matrix34 &m)
{
	// Load the input matrices into registers. /FF
	const __vector4 q_a = m.a.xyzw;
	const __vector4 q_b = m.b.xyzw;
	const __vector4 q_c = m.c.xyzw;
	const __vector4 p_a = a.xyzw;
	const __vector4 p_b = b.xyzw;
	const __vector4 p_c = c.xyzw;

#if !__XENON
	// Xenon doesn't need a zero vector since it's got the vmulfp instruction. /FF
	const __vector4 zero = _vzerofp;
#endif

	// Splat the elements of the p matrix. /FF
	const __vector4 splat_pax = __vspltw(p_a, 0);
	const __vector4 splat_pay = __vspltw(p_a, 1);
	const __vector4 splat_paz = __vspltw(p_a, 2);
	const __vector4 splat_pbx = __vspltw(p_b, 0);
	const __vector4 splat_pby = __vspltw(p_b, 1);
	const __vector4 splat_pbz = __vspltw(p_b, 2);
	const __vector4 splat_pcx = __vspltw(p_c, 0);
	const __vector4 splat_pcy = __vspltw(p_c, 1);
	const __vector4 splat_pcz = __vspltw(p_c, 2);

	// Do the multiplications and additions. /FF

#if __XENON
	const __vector4 da1 = __vmulfp(q_a, splat_pax);
	const __vector4 db1 = __vmulfp(q_a, splat_pbx);
	const __vector4 dc1 = __vmulfp(q_a, splat_pcx);
#else
	const __vector4 da1 = __vmaddfp(q_a, splat_pax, zero);
	const __vector4 db1 = __vmaddfp(q_a, splat_pbx, zero);
	const __vector4 dc1 = __vmaddfp(q_a, splat_pcx, zero);
#endif

	const __vector4 da2 = __vmaddfp(q_b, splat_pay, da1);
	const __vector4 db2 = __vmaddfp(q_b, splat_pby, db1);
	const __vector4 dc2 = __vmaddfp(q_b, splat_pcy, dc1);

	const __vector4 da = __vmaddfp(q_c, splat_paz, da2);
	const __vector4 db = __vmaddfp(q_c, splat_pbz, db2);
	const __vector4 dc = __vmaddfp(q_c, splat_pcz, dc2);

	// Store the output. /FF

	a.xyzw = da;
	b.xyzw = db;
	c.xyzw = dc;

	// Here is the previous version of this function, based on __vdot3fp.
	// It's provided here for future reference in case somebody wants to give
	// it a try again (my tests showed the new function to be significantly
	// faster on Xenon). Note that this old version for some reason *loads*
	// from the destination matrix while merging, which seems unnecessary. /FF
	//
	//// Transpose m
	//__vector4 temp0 = __vmrghw(m.a, m.c);
	//__vector4 temp1 = __vmrghw(m.b, m.d);
	//__vector4 temp2 = __vmrglw(m.a, m.c);
	//__vector4 temp3 = __vmrglw(m.b, m.d);
	//
	//__vector4 transposed0 = __vmrghw(temp0, temp1);
	//__vector4 transposed1 = __vmrglw(temp0, temp1);
	//__vector4 transposed2 = __vmrghw(temp2, temp3);
	//
	//// Multiply
	//__vector4 ax = __vdot3fp(transposed0, a);
	//__vector4 ay = __vdot3fp(transposed1, a);
	//__vector4 az = __vdot3fp(transposed2, a);
	//
	//__vector4 bx = __vdot3fp(transposed0, b);
	//__vector4 by = __vdot3fp(transposed1, b);
	//__vector4 bz = __vdot3fp(transposed2, b);
	//
	//__vector4 cx = __vdot3fp(transposed0, c);
	//__vector4 cy = __vdot3fp(transposed1, c);
	//__vector4 cz = __vdot3fp(transposed2, c);
	//
	//__vector4 axaz = __vmrghw(ax, az);
	//__vector4 aynn = __vmrghw(ay, a);
	//
	//__vector4 bxbz = __vmrghw(bx, bz);
	//__vector4 bynn = __vmrghw(by, b);
	// 
	//__vector4 cxcz = __vmrghw(cx, cz);
	//__vector4 cynn = __vmrghw(cy, c);
	//
	//a.xyzw = __vmrghw(axaz, aynn);
	//b.xyzw = __vmrghw(bxbz, bynn);
	//c.xyzw = __vmrghw(cxcz, cynn);

}
#endif // MATRIX34_DOT3X3

#ifndef MATRIX34_DOT3X3FROMLEFT
#define MATRIX34_DOT3X3FROMLEFT
__forceinline void Matrix34::Dot3x3FromLeft(const Matrix34& m)
{
	// Load the input matrices into registers. /FF
	const __vector4 q_a = a.xyzw;
	const __vector4 q_b = b.xyzw;
	const __vector4 q_c = c.xyzw;
	const __vector4 p_a = m.a.xyzw;
	const __vector4 p_b = m.b.xyzw;
	const __vector4 p_c = m.c.xyzw;

#if !__XENON
	// Xenon doesn't need a zero vector since it's got the vmulfp instruction. /FF
	const __vector4 zero = _vzerofp;
#endif

	// Splat the elements of the p matrix. /FF
	const __vector4 splat_pax = __vspltw(p_a, 0);
	const __vector4 splat_pay = __vspltw(p_a, 1);
	const __vector4 splat_paz = __vspltw(p_a, 2);
	const __vector4 splat_pbx = __vspltw(p_b, 0);
	const __vector4 splat_pby = __vspltw(p_b, 1);
	const __vector4 splat_pbz = __vspltw(p_b, 2);
	const __vector4 splat_pcx = __vspltw(p_c, 0);
	const __vector4 splat_pcy = __vspltw(p_c, 1);
	const __vector4 splat_pcz = __vspltw(p_c, 2);

	// Do the multiplications and additions. /FF

#if __XENON
	const __vector4 da1 = __vmulfp(q_a, splat_pax);
	const __vector4 db1 = __vmulfp(q_a, splat_pbx);
	const __vector4 dc1 = __vmulfp(q_a, splat_pcx);
#else
	const __vector4 da1 = __vmaddfp(q_a, splat_pax, zero);
	const __vector4 db1 = __vmaddfp(q_a, splat_pbx, zero);
	const __vector4 dc1 = __vmaddfp(q_a, splat_pcx, zero);
#endif

	const __vector4 da2 = __vmaddfp(q_b, splat_pay, da1);
	const __vector4 db2 = __vmaddfp(q_b, splat_pby, db1);
	const __vector4 dc2 = __vmaddfp(q_b, splat_pcy, dc1);

	const __vector4 da = __vmaddfp(q_c, splat_paz, da2);
	const __vector4 db = __vmaddfp(q_c, splat_pbz, db2);
	const __vector4 dc = __vmaddfp(q_c, splat_pcz, dc2);

	// Store the output. /FF

	a.xyzw = da;
	b.xyzw = db;
	c.xyzw = dc;
}
#endif // MATRIX34_DOT3X3FROMLEFT

#ifndef MATRIX34_DOT3X3_M
#define MATRIX34_DOT3X3_M
__forceinline void Matrix34::Dot3x3(const Matrix34 &m, const Matrix34 &n)
{
	// Load the input matrices into registers. /FF
	const __vector4 q_a = n.a.xyzw;
	const __vector4 q_b = n.b.xyzw;
	const __vector4 q_c = n.c.xyzw;
	const __vector4 p_a = m.a.xyzw;
	const __vector4 p_b = m.b.xyzw;
	const __vector4 p_c = m.c.xyzw;

#if !__XENON
	// Xenon doesn't need a zero vector since it's got the vmulfp instruction. /FF
	const __vector4 zero = _vzerofp;
#endif

	// Splat the elements of the p matrix. /FF
	const __vector4 splat_pax = __vspltw(p_a, 0);
	const __vector4 splat_pay = __vspltw(p_a, 1);
	const __vector4 splat_paz = __vspltw(p_a, 2);
	const __vector4 splat_pbx = __vspltw(p_b, 0);
	const __vector4 splat_pby = __vspltw(p_b, 1);
	const __vector4 splat_pbz = __vspltw(p_b, 2);
	const __vector4 splat_pcx = __vspltw(p_c, 0);
	const __vector4 splat_pcy = __vspltw(p_c, 1);
	const __vector4 splat_pcz = __vspltw(p_c, 2);

	// Do the multiplications and additions. /FF

#if __XENON
	const __vector4 da1 = __vmulfp(q_a, splat_pax);
	const __vector4 db1 = __vmulfp(q_a, splat_pbx);
	const __vector4 dc1 = __vmulfp(q_a, splat_pcx);
#else
	const __vector4 da1 = __vmaddfp(q_a, splat_pax, zero);
	const __vector4 db1 = __vmaddfp(q_a, splat_pbx, zero);
	const __vector4 dc1 = __vmaddfp(q_a, splat_pcx, zero);
#endif

	const __vector4 da2 = __vmaddfp(q_b, splat_pay, da1);
	const __vector4 db2 = __vmaddfp(q_b, splat_pby, db1);
	const __vector4 dc2 = __vmaddfp(q_b, splat_pcy, dc1);

	const __vector4 da = __vmaddfp(q_c, splat_paz, da2);
	const __vector4 db = __vmaddfp(q_c, splat_pbz, db2);
	const __vector4 dc = __vmaddfp(q_c, splat_pcz, dc2);

	// Store the output. /FF

	a.xyzw = da;
	b.xyzw = db;
	c.xyzw = dc;
}
#endif // MATRIX34_DOT3X3_M

#ifndef MATRIX34_DOT3X3TRANSPOSE
#define MATRIX34_DOT3X3TRANSPOSE
__forceinline void Matrix34::Dot3x3Transpose( const Matrix34 &n )
{
#if __XENON
	__vector4 ax = __vdot3fp(a, n.a);
	__vector4 ay = __vdot3fp(a, n.b);
	__vector4 az = __vdot3fp(a, n.c);

	__vector4 bx = __vdot3fp(b, n.a);
	__vector4 by = __vdot3fp(b, n.b);
	__vector4 bz = __vdot3fp(b, n.c);

	__vector4 cx = __vdot3fp(c, n.a);
	__vector4 cy = __vdot3fp(c, n.b);
	__vector4 cz = __vdot3fp(c, n.c);

	__vector4 axaz = __vmrghw(ax, az);
	__vector4 aynn = __vmrghw(ay, a);

	__vector4 bxbz = __vmrghw(bx, bz);
	__vector4 bynn = __vmrghw(by, b);
	 
	__vector4 cxcz = __vmrghw(cx, cz);
	__vector4 cynn = __vmrghw(cy, c);
	
	a.xyzw = __vmrghw(axaz, aynn);
	b.xyzw = __vmrghw(bxbz, bynn);
	c.xyzw = __vmrghw(cxcz, cynn);
#else
	__vector4 temp0 = __vmrghw(n.a, n.c);
	__vector4 temp1 = __vmrghw(n.b, n.c);
	__vector4 temp2 = __vmrglw(n.a, n.c);
	__vector4 temp3 = __vmrglw(n.b, n.c);
	__vector4 ta = __vmrghw(temp0, temp1);
	__vector4 tb = __vmrglw(temp0, temp1);
	__vector4 tc = __vmrghw(temp2, temp3);	
	__vector4 va = a;
	__vector4 vb = b;
	__vector4 vc = c;
	a = __vmaddfp(__vspltw(va, 0), ta, __vmaddfp(__vspltw(va, 1), tb, __vmulfp(__vspltw(va, 2), tc)));
	b = __vmaddfp(__vspltw(vb, 0), ta, __vmaddfp(__vspltw(vb, 1), tb, __vmulfp(__vspltw(vb, 2), tc)));
	c = __vmaddfp(__vspltw(vc, 0), ta, __vmaddfp(__vspltw(vc, 1), tb, __vmulfp(__vspltw(vc, 2), tc)));	
#endif
}
#endif // MATRIX34_DOT3X3TRANSPOSE

#ifndef MATRIX34_DOT3X3TRANSPOSE_M
#define MATRIX34_DOT3X3TRANSPOSE_M
__forceinline void Matrix34::Dot3x3Transpose( const Matrix34 &m, const Matrix34 &n )
{
#if __XENON
	__vector4 ax = __vdot3fp(m.a, n.a);
	__vector4 ay = __vdot3fp(m.a, n.b);
	__vector4 az = __vdot3fp(m.a, n.c);

	__vector4 bx = __vdot3fp(m.b, n.a);
	__vector4 by = __vdot3fp(m.b, n.b);
	__vector4 bz = __vdot3fp(m.b, n.c);

	__vector4 cx = __vdot3fp(m.c, n.a);
	__vector4 cy = __vdot3fp(m.c, n.b);
	__vector4 cz = __vdot3fp(m.c, n.c);

	__vector4 axaz = __vmrghw(ax, az);
	__vector4 aynn = __vmrghw(ay, a);

	__vector4 bxbz = __vmrghw(bx, bz);
	__vector4 bynn = __vmrghw(by, b);
	 
	__vector4 cxcz = __vmrghw(cx, cz);
	__vector4 cynn = __vmrghw(cy, c);
	
	a.xyzw = __vmrghw(axaz, aynn);
	b.xyzw = __vmrghw(bxbz, bynn);
	c.xyzw = __vmrghw(cxcz, cynn);
#else
	__vector4 temp0 = __vmrghw(n.a, n.c);
	__vector4 temp1 = __vmrghw(n.b, n.c);
	__vector4 temp2 = __vmrglw(n.a, n.c);
	__vector4 temp3 = __vmrglw(n.b, n.c);
	__vector4 ta = __vmrghw(temp0, temp1);
	__vector4 tb = __vmrglw(temp0, temp1);
	__vector4 tc = __vmrghw(temp2, temp3);	
	__vector4 va = m.a;
	__vector4 vb = m.b;
	__vector4 vc = m.c;
	a = __vmaddfp(__vspltw(va, 0), ta, __vmaddfp(__vspltw(va, 1), tb, __vmulfp(__vspltw(va, 2), tc)));
	b = __vmaddfp(__vspltw(vb, 0), ta, __vmaddfp(__vspltw(vb, 1), tb, __vmulfp(__vspltw(vb, 2), tc)));
	c = __vmaddfp(__vspltw(vc, 0), ta, __vmaddfp(__vspltw(vc, 1), tb, __vmulfp(__vspltw(vc, 2), tc)));	
#endif
}
#endif // MATRIX34_DOT3X3TRANSPOSE_M

#ifndef MATRIX34_ISEQUAL
#define MATRIX34_ISEQUAL
__forceinline bool Matrix34::IsEqual(const Matrix34& m) const
{
	_uvector4 t1 = __vcmpeqfp(VEC3_CLEAR_W(a), VEC3_CLEAR_W(m.a));
	_uvector4 t2 = __vcmpeqfp(VEC3_CLEAR_W(b), VEC3_CLEAR_W(m.b));
	_uvector4 t3 = __vcmpeqfp(VEC3_CLEAR_W(c), VEC3_CLEAR_W(m.c));
	_uvector4 t4 = __vcmpeqfp(VEC3_CLEAR_W(d), VEC3_CLEAR_W(m.d));
	return _vequal(__vand(__vand(t1, t2), __vand(t3,t4)), _vall1) != 0;
}
#endif // MATRIX34_ISEQUAL

#ifndef MATRIX34_ISNOTEQUAL
#define MATRIX34_ISNOTEQUAL
__forceinline bool Matrix34::IsNotEqual(const Matrix34& m) const
{
	return !IsEqual(m);
}
#endif // MATRIX34_ISNOTEQUAL

#ifndef MATRIX34_CROSSPRODUCT
#define MATRIX34_CROSSPRODUCT
__forceinline void Matrix34::CrossProduct(Vector3::Vector3Param r)
{
	// this matrix is row major. but representing a column major thing.  Ack all goes back to the Dot operator on a Matrix being backwards.
	//a.Set(0.0f,-r.z,r.y);
	//b.Set(r.z,0.0f,-r.x);
	//c.Set(-r.y,r.x,0.0f);
	// switch to row major format with Negates
	a.Cross( XAXIS, r );
	b.Cross( YAXIS, r );
	c.Cross( ZAXIS, r );
	
	ASSERT_ONLY(Vector3 vr(r));
	FastAssert( a.x == 0.0f && a.y == -vr.z && a.z == vr.y );

}
#endif

#ifndef MATRIX34_OUTERPRODUCT
#define MATRIX34_OUTERPRODUCT
__forceinline void Matrix34::OuterProduct(const Vector3& u, const Vector3& v)
{
	__vector4 ux = __vspltw(u, 0);
	__vector4 uy = __vspltw(u, 1);
	__vector4 uz = __vspltw(u, 2);

	a.xyzw = __vmulfp(ux, v);
	b.xyzw = __vmulfp(uy, v);
	c.xyzw = __vmulfp(uz, v);
}
#endif // MATRIX34_OUTERPRODUCT

#ifndef MATRIX34_TRANSFORM_V
#define MATRIX34_TRANSFORM_V
__forceinline void Matrix34::Transform(Vector3::Param in,Vector3::Vector3Ref out) const 
{
#if __PS3
	__vector4 zzzz = __vspltw(in, 2);
	__vector4 yyyy = __vspltw(in, 1);
	__vector4 xxxx = __vspltw(in, 0);
	__vector4 result = __vmaddfp(xxxx, a.xyzw, __vmaddfp(yyyy, b.xyzw, __vmaddfp(zzzz, c.xyzw, d.xyzw)));
	out = result;			   
#else
	__vector4 vd = __vor(VEC3_ONEW, __vand(in, VEC3_ANDW));		// vd = in.xyz0 || 0001

	__vector4 temp0 = __vmrghw(a, c);							// temp0 = a.x, c.x, a.y, c.y
    __vector4 temp1 = __vmrghw(b, d);							// temp1 = b.x, d.x, b.y, d.y
    __vector4 temp2 = __vmrglw(a, c);							// temp2 = a.z, c.z, ???, ???
    __vector4 temp3 = __vmrglw(b, d);							// temp3 = b.z, d.z, ???, ???

    __vector4 transposed0 = __vmrghw(temp0, temp1);				// transposed0 = a.x, b.x, c.x, d.x
    __vector4 transposed1 = __vmrglw(temp0, temp1);				// transposed1 = a.y, b.y, c.y, d.y
    __vector4 transposed2 = __vmrghw(temp2, temp3);				// transposed2 = a.z, b.z, c.z, d.z

	__vector4 ox = __vdot4fp(vd, transposed0);					// ox = (in.x * a.x) + (in.y * b.x) + (in.z * c.x) + (1 * d.x)
	__vector4 oy = __vdot4fp(vd, transposed1);					// oy = (in.x * a.y) + (in.y * b.y) + (in.z * c.y) + (1 * d.y)
	__vector4 oz = __vdot4fp(vd, transposed2);					// oz = (in.x * a.z) + (in.y * b.z) + (in.z * c.z) + (1 * d.z)

	ox = __vmrghw(ox, oz);										// ox = ox, oz, ox oz
	oy = __vmrghw(oy, vd);										// oy = oy, 01, oy, 01
	out = __vmrghw(ox, oy);										// out.xyzw = ox, oy, oz, 01
#endif
}
#endif // MATRIX34_TRANSFORM_V

#ifndef MATRIX34_TRANSFORM
#define MATRIX34_TRANSFORM
__forceinline void Matrix34::Transform(Vector3::Vector3Ref inAndOut) const 
{
	Transform(inAndOut,inAndOut);
}
#endif // MATRIX34_TRANSFORM

#ifndef MATRIX34_TRANSFORM3X3_V
#define MATRIX34_TRANSFORM3X3_V
__forceinline void Matrix34::Transform3x3(Vector3::Param in,Vector3::Vector3Ref out) const 
{
#if __PS3
	__vector4 xxxx = __vspltw(in, 0);
	__vector4 yyyy = __vspltw(in, 1);
	__vector4 zzzz = __vspltw(in, 2);
	__vector4 result = __vmaddfp(xxxx, a.xyzw, __vmaddfp(yyyy, b.xyzw, __vmulfp(zzzz, c.xyzw)));
	out = result;
#else
	__vector4 temp0 = __vmrghw(a, c);
	__vector4 temp1 = __vmrghw(b, d);
	__vector4 temp2 = __vmrglw(a, c);
	__vector4 temp3 = __vmrglw(b, d);

	__vector4 transposed0 = __vmrghw(temp0, temp1);
	__vector4 transposed1 = __vmrglw(temp0, temp1);
	__vector4 transposed2 = __vmrghw(temp2, temp3);

	__vector4 ox = __vdot3fp(in, transposed0);
	__vector4 oy = __vdot3fp(in, transposed1);
	__vector4 oz = __vdot3fp(in, transposed2);

	ox = __vmrghw(ox, oz);
	out = __vmrghw(ox, oy);
#endif
}
#endif // MATRIX34_TRANSFORM3X3_V

#ifndef MATRIX34_TRANSFORM3X3
#define MATRIX34_TRANSFORM3X3
__forceinline void Matrix34::Transform3x3(Vector3::Vector3Ref inAndOut) const
{
	Transform3x3(inAndOut,inAndOut);
}
#endif // MATRIX34_TRANSFORM3X3

#ifndef MATRIX34_UNTRANSFORM_V
#define MATRIX34_UNTRANSFORM_V
__forceinline void Matrix34::UnTransform(Vector3::Param in,Vector3::Ref out) const
{
	__vector4 temp = __vsubfp(in, d);
	UnTransform3x3(temp, out);
}
#endif // MATRIX34_UNTRANSFORM_V

#ifndef MATRIX34_UNTRANSFORM
#define MATRIX34_UNTRANSFORM
__forceinline void Matrix34::UnTransform(Vector3::Ref inAndOut) const 
{
	UnTransform(inAndOut, inAndOut);
}
#endif // MATRIX34_UNTRANSFORM

#ifndef MATRIX34_UNTRANSFORM3X3_V
#define MATRIX34_UNTRANSFORM3X3_V
__forceinline void Matrix34::UnTransform3x3(Vector3::Param in,Vector3::Ref out) const
{
#if __PS3
	__vector4 temp0 = __vmrghw(a, c);
	__vector4 temp1 = __vmrghw(b, c);
	__vector4 temp2 = __vmrglw(a, c);
	__vector4 temp3 = __vmrglw(b, c);
	__vector4 ta = __vmrghw(temp0, temp1);
	__vector4 tb = __vmrglw(temp0, temp1);
	__vector4 tc = __vmrghw(temp2, temp3);
	__vector4 xxxx = __vspltw(in, 0);
	__vector4 yyyy = __vspltw(in, 1);
	__vector4 zzzz = __vspltw(in, 2);
	out = __vmaddfp(xxxx, ta, __vmaddfp(yyyy, tb, __vmulfp(zzzz, tc)));
#else
	__vector4 ox = __vdot3fp(a, in);
	__vector4 oy = __vdot3fp(b, in);
	__vector4 oz = __vdot3fp(c, in);
	ox = __vmrghw(ox, oz);
	out = __vmrghw(ox, oy);
#endif
}
#endif // MATRIX34_UNTRANSFORM3X3_V

#ifndef MATRIX34_UNTRANSFORM3X3
#define MATRIX34_UNTRANSFORM3X3
__forceinline void Matrix34::UnTransform3x3(Vector3::Ref inAndOut) const 
{
	UnTransform3x3(inAndOut, inAndOut);
}
#endif // MATRIX34_UNTRANSFORM3X3

#ifndef MATRIX34_SCALE
#define MATRIX34_SCALE
__forceinline void Matrix34::Scale(float s) 
{
	__vector4 scale = _vsplatf(s);
	a.xyzw = __vmulfp(a, scale);
	b.xyzw = __vmulfp(b, scale);
	c.xyzw = __vmulfp(c, scale);
}
#endif // MATRIX34_SCALE

#ifndef MATRIX34_SCALE_F
#define MATRIX34_SCALE_F
__forceinline void Matrix34::Scale(float x,float y,float z) 
{
	__vector4 scale = { x, y, z, 1.0f };
	a.xyzw = __vmulfp(a, scale);
	b.xyzw = __vmulfp(b, scale);
	c.xyzw = __vmulfp(c, scale);
}
#endif // MATRIX34_SCALE_F

#ifndef MATRIX34_SCALE_V
#define MATRIX34_SCALE_V
__forceinline void Matrix34::Scale(const Vector3& v)
{
	a.xyzw = __vmulfp(a, v);
	b.xyzw = __vmulfp(b, v);
	c.xyzw = __vmulfp(c, v);
}
#endif // MATRIX34_SCALE_V

#ifndef MATRIX34_SCALE_V_NATIVE
#define MATRIX34_SCALE_V_NATIVE
__forceinline void Matrix34::Scale(Vector3::Param v)
{
	a.xyzw = __vmulfp(a, v);
	b.xyzw = __vmulfp(b, v);
	c.xyzw = __vmulfp(c, v);
}
#endif // MATRIX34_SCALE_V_NATIVE

#ifndef MATRIX34_SCALEFULL
#define MATRIX34_SCALEFULL
__forceinline void Matrix34::ScaleFull(float s) 
{
	__vector4 scale = _vsplatf(s);
	a.xyzw = __vmulfp(a, scale);
	b.xyzw = __vmulfp(b, scale);
	c.xyzw = __vmulfp(c, scale);
	d.xyzw = __vmulfp(d, scale);
}
#endif // MATRIX34_SCALEFULL

#ifndef MATRIX34_SCALLEFULL_F
#define MATRIX34_SCALLEFULL_F
__forceinline void Matrix34::ScaleFull(float x,float y,float z) 
{
	__vector4 scale = { x,y,z,1.0f };
	a.xyzw = __vmulfp(a, scale);
	b.xyzw = __vmulfp(b, scale);
	c.xyzw = __vmulfp(c, scale);
	d.xyzw = __vmulfp(d, scale);
}
#endif // MATRIX34_SCALEFULL_F

#ifndef MATRIX34_SCALLEFULL_V
#define MATRIX34_SCALLEFULL_V
__forceinline void Matrix34::ScaleFull(const Vector3& v)
{
	a.xyzw = __vmulfp(a, v);
	b.xyzw = __vmulfp(b, v);
	c.xyzw = __vmulfp(c, v);
	d.xyzw = __vmulfp(d, v);
}
#endif // MATRIX34_SCALEFULL_V

#ifndef MATRIX34_FASTINVERSE_M
#define MATRIX34_FASTINVERSE_M
inline void Matrix34::FastInverse(const Matrix34 &m) 
{
	Transpose(m);

	m.UnTransform3x3(m.d, d);
	d.Negate();
}
#endif // MATRIX34_FASTINVERSE_M

#ifndef MATRIX34_TRANSPOSE
#define MATRIX34_TRANSPOSE
__forceinline void Matrix34::Transpose() 
{
	__vector4 temp0 = __vmrghw(a, c);
    __vector4 temp1 = __vmrghw(b, d);
    __vector4 temp2 = __vmrglw(a, c);
    __vector4 temp3 = __vmrglw(b, d);

    a = __vmrghw(temp0, temp1);
    b = __vmrglw(temp0, temp1);
    c = __vmrghw(temp2, temp3);
}
#endif // MATRIX34_TRANSPOSE

#ifndef MATRIX34_TRANSPOSE_M
#define MATRIX34_TRANSPOSE_M
__forceinline void Matrix34::Transpose(const Matrix34 &m) 
{
	__vector4 temp0 = __vmrghw(m.a, m.c);
    __vector4 temp1 = __vmrghw(m.b, m.d);
    __vector4 temp2 = __vmrglw(m.a, m.c);
    __vector4 temp3 = __vmrglw(m.b, m.d);

    a = __vmrghw(temp0, temp1);
    b = __vmrglw(temp0, temp1);
    c = __vmrghw(temp2, temp3);
}
#endif // MATRIX34_TRANSPOSE_M

} // namespace rage

#endif // VECTOR_MATRIX34_XENON_H
