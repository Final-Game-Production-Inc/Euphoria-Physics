// 
// vector/MATRIX33_xenon.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_MATRIX33_XENON_H
#define VECTOR_MATRIX33_XENON_H

// Default Matrix33 Implementations

namespace rage
{

	//=============================================================================
	// Implementations

#ifndef MATRIX33_IDENTITY
#define MATRIX33_IDENTITY
inline void Matrix33::Identity()
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
#endif	// MATRIX33_IDENTITY


#ifndef MATRIX33_ZERO
#define MATRIX33_ZERO
	__forceinline void Matrix33::Zero()
	{
		a = b = c = _vzerofp;
	}
#endif // MATRIX33_ZERO3X3

#ifndef MATRIX33_SET34
#define MATRIX33_SET34
	__forceinline void Matrix33::Set(const Matrix34 &m)
	{
		a = m.a;
		b = m.b;
		c = m.c;
	}
#endif // MATRIX33_SET3X3

#ifndef MATRIX33_SET_M
#define MATRIX33_SET_M
	__forceinline void Matrix33::Set(const Matrix33 &m)
	{
		a = m.a;
		b = m.b;
		c = m.c;
	}
#endif // MATRIX33_SET_M

#ifndef MATRIX33_SET_V
#define MATRIX33_SET_V
	__forceinline void Matrix33::Set(const Vector3& newA, const Vector3& newB, const Vector3& newC)
	{
		a = newA;
		b = newB;
		c = newC;
	}
#endif // MATRIX33_SET_V

#ifndef MATRIX33_SET_DIAGONAL
#define MATRIX33_SET_DIAGONAL
	__forceinline void Matrix33::SetDiagonal( Vector3::Vector3Param d )
	{
		Vector3 dd = d;
		dd.And( VEC3_ANDW );
		a.Permute<VEC_PERM_X, VEC_PERM_W, VEC_PERM_W, VEC_PERM_W>(dd);
		b.Permute<VEC_PERM_W, VEC_PERM_Y, VEC_PERM_W, VEC_PERM_W>(dd);
		c.Permute<VEC_PERM_W, VEC_PERM_W, VEC_PERM_Z, VEC_PERM_W>(dd);
		
	}
#endif

#ifndef MATRIX33_DOT
#define MATRIX33_DOT
	__forceinline void Matrix33::Dot(const Matrix33 &m)
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
#endif // MATRIX33_DOT

#ifndef MATRIX33_DOTFROMLEFT
#define MATRIX33_DOTFROMLEFT
	__forceinline void Matrix33::DotFromLeft(const Matrix33& m)
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
#endif // MATRIX33_DOTFROMLEFT

#ifndef MATRIX33_DOT_M
#define MATRIX33_DOT_M
	__forceinline void Matrix33::Dot(const Matrix33 &m, const Matrix33 &n)
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
#endif // MATRIX33_DOT_M

#ifndef MATRIX33_DOTTRANSPOSE
#define MATRIX33_DOTTRANSPOSE
	__forceinline void Matrix33::DotTranspose( const Matrix33 &n )
	{
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
	}
#endif // MATRIX33_DOTTRANSPOSE

#ifndef MATRIX33_DOTTRANSPOSE_M
#define MATRIX33_DOTTRANSPOSE_M
	__forceinline void Matrix33::DotTranspose( const Matrix33 &m, const Matrix33 &n )
	{
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
	}
#endif // MATRIX33_DOTTRANSPOSE_M

#ifndef MATRIX33_ISEQUAL
#define MATRIX33_ISEQUAL
	__forceinline bool Matrix33::IsEqual(const Matrix33& m) const
	{
		_uvector4 t1 = __vcmpeqfp(VEC3_CLEAR_W(a), VEC3_CLEAR_W(m.a));
		_uvector4 t2 = __vcmpeqfp(VEC3_CLEAR_W(b), VEC3_CLEAR_W(m.b));
		_uvector4 t3 = __vcmpeqfp(VEC3_CLEAR_W(c), VEC3_CLEAR_W(m.c));
		return _vequal(__vand(__vand(t1, t2), t3), _vall1) != 0;
	}
#endif // MATRIX33_ISEQUAL

#ifndef MATRIX33_ISNOTEQUAL
#define MATRIX33_ISNOTEQUAL
	__forceinline bool Matrix33::IsNotEqual(const Matrix33& m) const
	{
		return !IsEqual(m);
	}
#endif // MATRIX33_ISNOTEQUAL

#ifndef MATRIX33_CROSSPRODUCT
#define MATRIX33_CROSSPRODUCT
	__forceinline void Matrix33::CrossProduct(Vector3::Vector3Param r)
	{
		// this matrix is row major. but representing a column major thing.  Ack all goes back to the Dot operator on a Matrix being backwards.
		//a.Set(0.0f,-r.z,r.y);
		//b.Set(r.z,0.0f,-r.x);
		//c.Set(-r.y,r.x,0.0f);
		// switch to row major format with Negates
		a.Cross( XAXIS, r );
		b.Cross( YAXIS, r );
		c.Cross( ZAXIS, r );

		Assert( a.x == 0.0f && a.y == -Vector3(r).z && a.z == Vector3(r).y );

	}
#endif

#ifndef MATRIX33_OUTERPRODUCT
#define MATRIX33_OUTERPRODUCT
	__forceinline void Matrix33::OuterProduct(const Vector3& u, const Vector3& v)
	{
		__vector4 ux = __vspltw(u, 0);
		__vector4 uy = __vspltw(u, 1);
		__vector4 uz = __vspltw(u, 2);

		a.xyzw = __vmulfp(ux, v);
		b.xyzw = __vmulfp(uy, v);
		c.xyzw = __vmulfp(uz, v);
	}
#endif // MATRIX33_OUTERPRODUCT

#ifndef MATRIX33_TRANSFORM
#define MATRIX33_TRANSFORM
	__forceinline void Matrix33::Transform(Vector3::Vector3Ref inAndOut) const 
	{
		Transform(inAndOut,inAndOut);
	}
#endif // MATRIX33_TRANSFORM

#ifndef MATRIX33_TRANSFORM_V
#define MATRIX33_TRANSFORM_V
	__forceinline void Matrix33::Transform(Vector3::Param in,Vector3::Vector3Ref out) const 
	{
#if __PS3
		__vector4 xxxx = __vspltw(in, 0);
		__vector4 yyyy = __vspltw(in, 1);
		__vector4 zzzz = __vspltw(in, 2);
		__vector4 result = __vmaddfp(xxxx, a.xyzw, __vmaddfp(yyyy, b.xyzw, __vmulfp(zzzz, c.xyzw)));
		out = result;
#else
		__vector4 _zero = _vzerofp;
		__vector4 temp0 = __vmrghw(a, c);
		__vector4 temp1 = __vmrghw(b, _zero);
		__vector4 temp2 = __vmrglw(a, c);
		__vector4 temp3 = __vmrglw(b, _zero);

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
#endif // MATRIX33_TRANSFORM_V

#ifndef MATRIX33_UNTRANSFORM_V
#define MATRIX33_UNTRANSFORM_V
	__forceinline void Matrix33::UnTransform(Vector3::Param in,Vector3::Ref out) const
	{
#if __PS3
		__vector4 temp0 = __vmrghw(a, c);
		__vector4 temp1 = __vmrghw(b, b);
		__vector4 temp2 = __vmrglw(a, c);
		__vector4 temp3 = __vmrglw(b, b);
		__vector4 va = __vmrghw(temp0, temp1);
		__vector4 vb = __vmrglw(temp0, temp1);
		__vector4 vc = __vmrghw(temp2, temp3);
		__vector4 xxxx = __vspltw(in, 0);
		__vector4 yyyy = __vspltw(in, 1);
		__vector4 zzzz = __vspltw(in, 2);
		__vector4 result = __vmaddfp(xxxx, va, __vmaddfp(yyyy, vb, __vmulfp(zzzz, vc)));
		out = result;
#else
		__vector4 ox = __vdot3fp(a, in);
		__vector4 oy = __vdot3fp(b, in);
		__vector4 oz = __vdot3fp(c, in);
		ox = __vmrghw(ox, oz);
		out = __vmrghw(ox, oy);
#endif
	}
#endif // MATRIX33_UNTRANSFORM_V

#ifndef MATRIX33_UNTRANSFORM
#define MATRIX33_UNTRANSFORM
	__forceinline void Matrix33::UnTransform(Vector3::Ref inAndOut) const 
	{
		UnTransform(inAndOut, inAndOut);
	}
#endif // MATRIX33_UNTRANSFORM

#ifndef MATRIX33_SCALE
#define MATRIX33_SCALE
	__forceinline void Matrix33::Scale(float s) 
	{
		__vector4 scale = _vsplatf(s);
		a.xyzw = __vmulfp(a, scale);
		b.xyzw = __vmulfp(b, scale);
		c.xyzw = __vmulfp(c, scale);
	}
#endif // MATRIX33_SCALE

#ifndef MATRIX33_SCALE_F
#define MATRIX33_SCALE_F
	__forceinline void Matrix33::Scale(float x,float y,float z) 
	{
		__vector4 scale = { x, y, z, 1.0f };
		a.xyzw = __vmulfp(a, scale);
		b.xyzw = __vmulfp(b, scale);
		c.xyzw = __vmulfp(c, scale);
	}
#endif // MATRIX33_SCALE_F

#ifndef MATRIX33_SCALE_V
#define MATRIX33_SCALE_V
	__forceinline void Matrix33::Scale(const Vector3& v)
	{
		a.xyzw = __vmulfp(a, v);
		b.xyzw = __vmulfp(b, v);
		c.xyzw = __vmulfp(c, v);
	}
#endif // MATRIX33_SCALE_V


#ifndef MATRIX33_TRANSPOSE
#define MATRIX33_TRANSPOSE
	__forceinline void Matrix33::Transpose() 
	{
		__vector4 _zero = _vzerofp;
		__vector4 temp0 = __vmrghw(a, c);
		__vector4 temp1 = __vmrghw(b, _zero);
		__vector4 temp2 = __vmrglw(a, c);
		__vector4 temp3 = __vmrglw(b, _zero);

		a = __vmrghw(temp0, temp1);
		b = __vmrglw(temp0, temp1);
		c = __vmrghw(temp2, temp3);
	}
#endif // MATRIX33_TRANSPOSE

#ifndef MATRIX33_TRANSPOSE_M
#define MATRIX33_TRANSPOSE_M
	__forceinline void Matrix33::Transpose(const Matrix33 &m) 
	{
		__vector4 _zero = _vzerofp;
		__vector4 temp0 = __vmrghw(m.a, m.c);
		__vector4 temp1 = __vmrghw(m.b, _zero);
		__vector4 temp2 = __vmrglw(m.a, m.c);
		__vector4 temp3 = __vmrglw(m.b, _zero);

		a = __vmrghw(temp0, temp1);
		b = __vmrglw(temp0, temp1);
		c = __vmrghw(temp2, temp3);
	}
#endif // MATRIX33_TRANSPOSE_M

} // namespace rage

#endif // VECTOR_MATRIX33_XENON_H
