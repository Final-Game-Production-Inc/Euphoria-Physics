#ifndef VECTORMATH_LEGACYCONVERT_H
#define VECTORMATH_LEGACYCONVERT_H

#include "classes.h"
#include "vector/quaternion.h"
#include "vector/vector3.h"
#include "vector/vector4.h"
#include "vector/matrix33.h"
#include "vector/matrix34.h"
#include "vector/matrix44.h"

// Conversion routines for:
//
//	rage::__vector4		<-->	rage::Vec3V
//	rage::__vector4		<-->	rage::Vec4V
//	rage::__vector4		<-->	rage::Vector3
//	rage::__vector4		<-->	rage::Vector4
//	rage::Vector4		<-->	rage::Vec4V
//	rage::Vector3		<-->	rage::Vec3V
//	rage::Vector2		<-- 	rage::Vec2V
//	rage::Matrix34		<-->	rage::Mat34V
//  ...etc...
//
// Helpful for integration of the new vector library. reinterpret_cast-ing is the way to go!
//

// For the conversion macros below, there are two styles. The reinterpret_cast-ing ones are preferred,
// and used for vectors when loading them from memory. They look like this: RCC_VEC3V, RC_SCALARV, etc.
// The type name is the destination type, and the code will fail to compile if that cast isn't allowed.
//
// RCC_ is for reinterpret_cast (const), i.e. reinterpret_cast<const Vec3V&>(...) 
// RC_ is for reinterpret_cast (non-const), i.e. reinterpret_cast<ScalarV&>(...)
// 
// If you need to convert temporary values (values in registers, ones without addresses) use the
// VECTOR3_TO_VEC3V style macros at the bottom of the file.


#if LEGACYCONVERT_REINTERPRETCAST_DEBUG

namespace rage {

template <class TO, class FROM>
const TO& RccCast(const FROM& x)
{
	return InvalidConversion(x);
}

template <class TO, class FROM>
TO& RcCast(FROM& x)
{
	return InvalidConversion(x);
}

#define ALLOW_CAST(TO, FROM)							\
template <>												\
__forceinline const TO& RccCast<TO, FROM>(const FROM& x)\
{														\
	return (*(reinterpret_cast<const TO*>(&(x))));		\
};														\
														\
template <>												\
__forceinline TO& RcCast<TO, FROM>(FROM& x)				\
{														\
	return (*(reinterpret_cast<TO*>(&(x))));			\
};														


ALLOW_CAST(Vector3, Vec3f)
ALLOW_CAST(Vec3f, Vector3)

ALLOW_CAST(Quaternion, QuatV)
ALLOW_CAST(QuatV, Quaternion)

ALLOW_CAST(Vec3V, Vec::Vector_4V)
ALLOW_CAST(Vec::Vector_4V, Vec3V)

ALLOW_CAST(Vec4V, Vec::Vector_4V)
ALLOW_CAST(Vec::Vector_4V, Vec4V)

ALLOW_CAST(Vector3, Vec::Vector_4V)
ALLOW_CAST(Vec::Vector_4V, Vector3)

ALLOW_CAST(Vector3, Vec3V)
ALLOW_CAST(Vec3V, Vector3)

ALLOW_CAST(Vector4, Vec4V)
ALLOW_CAST(Vec4V, Vector4)

ALLOW_CAST(Vector4, Vec3V)
ALLOW_CAST(Vec3V, Vector4)

ALLOW_CAST(Vector3, Vec4V)
ALLOW_CAST(Vec4V, Vector3)

ALLOW_CAST(Vector2, Vec2V)
// Inverse conversion isn't allowed, because the alignments are different

ALLOW_CAST(Matrix44, Mat44V)
ALLOW_CAST(Mat44V, Matrix44)

ALLOW_CAST(Matrix34, Mat34V)
ALLOW_CAST(Mat34V, Matrix34)

ALLOW_CAST(Matrix33, Mat33V)
ALLOW_CAST(Mat33V, Matrix33)

ALLOW_CAST(ScalarV, Vec::Vector_4V)
ALLOW_CAST(Vec::Vector_4V, ScalarV)

ALLOW_CAST(ScalarV, Vector3)
ALLOW_CAST(Vector3, ScalarV)

ALLOW_CAST(ScalarV, Vector4)
ALLOW_CAST(Vector4, ScalarV)

ALLOW_CAST(Vector2, Vec2f);
ALLOW_CAST(Vec2f, Vector2);

} // namespace rage

#define RCC_SCALARV(x)	::rage::RccCast< ::rage::ScalarV >(*(&(x)))
#define RCC_VEC2F(x)	::rage::RccCast< ::rage::Vec2f >(*(&(x)))
#define RCC_VEC3V(x)	::rage::RccCast< ::rage::Vec3V >(*(&(x)))
#define RCC_VEC3F(x)	::rage::RccCast< ::rage::Vec3f >(*(&(x)))
#define RCC_VEC4V(x)	::rage::RccCast< ::rage::Vec4V >(*(&(x)))
#define RCC_QUATV(x)	::rage::RccCast< ::rage::QuatV >(*(&(x)))
#define RCC_MAT34V(x)	::rage::RccCast< ::rage::Mat34V >(*(&(x)))
#define RCC_MAT44V(x)	::rage::RccCast< ::rage::Mat44V >(*(&(x)))
#define RCC_MAT33V(x)	::rage::RccCast< ::rage::Mat33V >(*(&(x)))

#define RCC_VECTOR2(x)	::rage::RccCast< ::rage::Vector2 >(*(&(x)))
#define RCC_VECTOR3(x)	::rage::RccCast< ::rage::Vector3 >(*(&(x)))
#define RCC_VECTOR4(x)	::rage::RccCast< ::rage::Vector4 >(*(&(x)))
#define RCC_QUATERNION(x) 	::rage::RccCast< ::rage::Quaternion >(*(&(x)))

#define RCC_MATRIX33(x)	::rage::RccCast< ::rage::Matrix33 >(*(&(x)))
#define RCC_MATRIX34(x)	::rage::RccCast< ::rage::Matrix34 >(*(&(x)))
#define RCC_MATRIX44(x)	::rage::RccCast< ::rage::Matrix44 >(*(&(x)))

#define RC_SCALARV(x)	::rage::RcCast< ::rage::ScalarV >(*(&(x)))
#define RC_VEC2F(x)		::rage::RcCast< ::rage::Vec2f >(*(&(x)))
#define RC_VEC3V(x)		::rage::RcCast< ::rage::Vec3V >(*(&(x)))
#define RC_VEC3F(x)		::rage::RcCast< ::rage::Vec3f >(*(&(x)))
#define RC_VEC4V(x)		::rage::RcCast< ::rage::Vec4V >(*(&(x)))
#define RC_QUATV(x)		::rage::RcCast< ::rage::QuatV >(*(&(x)))
#define RC_MAT34V(x)	::rage::RcCast< ::rage::Mat34V >(*(&(x)))
#define RC_MAT44V(x)	::rage::RcCast< ::rage::Mat44V >(*(&(x)))
#define RC_MAT33V(x)	::rage::RcCast< ::rage::Mat33V >(*(&(x)))

#define RC_VECTOR2(x)	::rage::RcCast< ::rage::Vector2 >(*(&(x)))
#define RC_VECTOR3(x)	::rage::RcCast< ::rage::Vector3 >(*(&(x)))
#define RC_VECTOR4(x)	::rage::RcCast< ::rage::Vector4 >(*(&(x))) 
#define RC_QUATERNION(x) ::rage::RcCast< ::rage::Quaternion >(*(&(x)))

#define RC_MATRIX33(x)	::rage::RcCast< ::rage::Matrix33 >(*(&(x)))
#define RC_MATRIX34(x)	::rage::RcCast< ::rage::Matrix34 >(*(&(x)))
#define RC_MATRIX44(x)	::rage::RcCast< ::rage::Matrix44 >(*(&(x)))

#else // LEGACYCONVERT_REINTERPRETCAST_DEBUG

#define RCC_SCALARV(x)	(*(reinterpret_cast< const ::rage::ScalarV* >(&(x))))
#define RCC_VEC2F(x)	(*(reinterpret_cast< const ::rage::Vec2f* >(&(x))))
#define RCC_VEC3V(x)	(*(reinterpret_cast< const ::rage::Vec3V* >(&(x))))
#define RCC_VEC3F(x)		(*(reinterpret_cast< const ::rage::Vec3f* >(&(x))))
#define RCC_VEC4V(x)	(*(reinterpret_cast< const ::rage::Vec4V* >(&(x))))
#define RCC_QUATV(x)	(*(reinterpret_cast< const ::rage::QuatV* >(&(x))))
#define RCC_MAT34V(x)	(*(reinterpret_cast< const ::rage::Mat34V* >(&(x))))
#define RCC_MAT44V(x)	(*(reinterpret_cast< const ::rage::Mat44V* >(&(x))))
#define RCC_MAT33V(x)	(*(reinterpret_cast< const ::rage::Mat33V* >(&(x))))

#define RCC_VECTOR2(x)	(*(reinterpret_cast< const ::rage::Vector2* >(&(x))))
#define RCC_VECTOR3(x)	(*(reinterpret_cast< const ::rage::Vector3* >(&(x))))
#define RCC_VECTOR4(x)	(*(reinterpret_cast< const ::rage::Vector4* >(&(x))))
#define RCC_QUATERNION(x)	(*(reinterpret_cast< const ::rage::Quaternion* >(&(x))))

#define RCC_MATRIX33(x)	(*(reinterpret_cast< const ::rage::Matrix33* >(&(x))))
#define RCC_MATRIX34(x)	(*(reinterpret_cast< const ::rage::Matrix34* >(&(x))))
#define RCC_MATRIX44(x)	(*(reinterpret_cast< const ::rage::Matrix44* >(&(x))))

#define RC_SCALARV(x)	(*(reinterpret_cast< ::rage::ScalarV* >(&(x))))
#define RC_VEC2F(x)		(*(reinterpret_cast< ::rage::Vec2f* >(&(x))))
#define RC_VEC3V(x)		(*(reinterpret_cast< ::rage::Vec3V* >(&(x))))
#define RC_VEC3F(x)		(*(reinterpret_cast< ::rage::Vec3f* >(&(x))))
#define RC_VEC4V(x)		(*(reinterpret_cast< ::rage::Vec4V* >(&(x))))
#define RC_QUATV(x)		(*(reinterpret_cast< ::rage::QuatV* >(&(x))))
#define RC_MAT34V(x)	(*(reinterpret_cast< ::rage::Mat34V* >(&(x))))
#define RC_MAT44V(x)	(*(reinterpret_cast< ::rage::Mat44V* >(&(x))))
#define RC_MAT33V(x)	(*(reinterpret_cast< ::rage::Mat33V* >(&(x))))

#define RC_VECTOR2(x)	(*(reinterpret_cast< ::rage::Vector2* >(&(x))))
#define RC_VECTOR3(x)	(*(reinterpret_cast< ::rage::Vector3* >(&(x))))
#define RC_VECTOR4(x)	(*(reinterpret_cast< ::rage::Vector4* >(&(x))))
#define RC_QUATERNION(x) (*(reinterpret_cast< ::rage::Quaternion* >(&(x))))

#define RC_MATRIX33(x)	(*(reinterpret_cast< ::rage::Matrix33* >(&(x))))
#define RC_MATRIX34(x)	(*(reinterpret_cast< ::rage::Matrix34* >(&(x))))
#define RC_MATRIX44(x)	(*(reinterpret_cast< ::rage::Matrix44* >(&(x))))

#endif // LEGACYCONVERT_REINTERPRETCAST_DEBUG

//
// Copying around. These are good for creation. They avoid pointers (reinterpret_cast'ing), so aliasing performance shouldn't be an issue.
// Compiler optimizes away extra copies. ( Except on Win32 :-( )
//

#define	INTRIN_TO_VECTOR3( __v4 )		(::rage::Vector3(__v4))
#define	INTRIN_TO_VECTOR4( __v4 )		(::rage::Vector4(__v4))
#define	VECTOR3_TO_INTRIN( v3 )			((v3).xyzw)
#define	VECTOR4_TO_INTRIN( v4 )			((v4).xyzw)

#define INTRIN_TO_VEC3V( __v4 )			(::rage::Vec3V(__v4))
#define INTRIN_TO_VEC4V( __v4 )			(::rage::Vec4V(__v4))
#define SCALARV_TO_INTRIN( sv )			(sv).GetIntrin128()
#define INTRIN_TO_SCALARV( __v4 )		(::rage::ScalarV(__v4))
#define VEC3V_TO_INTRIN( v3v )			(v3v).GetIntrin128()
#define VEC4V_TO_INTRIN( v4v )			(v4v).GetIntrin128()

#define QUATV_TO_INTRIN( qv )			(qv).GetIntrin128()
#define QUATERNION_TO_INTRIN( q )		(q).xyzw
#define INTRIN_TO_QUATV( __v4 )			(::rage::QuatV(__v4))
#define INTRIN_TO_QUATERNION( __v4 )	(::rage::Quaternion(__v4))

#define QUATV_TO_QUATERNION( qv )		( INTRIN_TO_QUATERNION( QUATV_TO_INTRIN(qv) ) )
#define QUATERNION_TO_QUATV( q )		( INTRIN_TO_QUATV( QUATERNION_TO_INTRIN(q) ) )

#define VEC3V_TO_VECTOR3( v3v )			( INTRIN_TO_VECTOR3( VEC3V_TO_INTRIN(v3v) ) )
#define VECTOR3_TO_VEC3V( v3 )			( INTRIN_TO_VEC3V( VECTOR3_TO_INTRIN(v3) ) )
#define VEC4V_TO_VECTOR4( v4v )			( INTRIN_TO_VECTOR4( VEC4V_TO_INTRIN(v4v) ) )
#define VECTOR4_TO_VEC4V( v4 )			( INTRIN_TO_VEC4V( VECTOR4_TO_INTRIN(v4) ) )

#define VEC3V_TO_VECTOR4( v3v )			( INTRIN_TO_VECTOR4( VEC3V_TO_INTRIN(v3v) ) )
#define VECTOR4_TO_VEC3V( v4 )			( INTRIN_TO_VEC3V( VECTOR4_TO_INTRIN(v4) ) )
#define VEC4V_TO_VECTOR3( v4v )			( INTRIN_TO_VECTOR3( VEC4V_TO_INTRIN(v4v) ) )
#define VECTOR3_TO_VEC4V( v3 )			( INTRIN_TO_VEC4V( VECTOR3_TO_INTRIN(v3) ) )

// These are functions, not #defines, so that we don't evaluate the argument more than once (in case we're converting the return value of a function call) 
#if __WIN32PC || RSG_DURANGO || RSG_ORBIS
__forceinline ::rage::Matrix33		MAT33V_TO_MATRIX33( ::rage::Mat33V_In m33v )		{ return ::rage::Matrix33( (m33v).GetCol0Intrin128(), (m33v).GetCol1Intrin128(), (m33v).GetCol2Intrin128() ); }
__forceinline ::rage::Mat33V_Out	MATRIX33_TO_MAT33V( const ::rage::Matrix33& m33 )	{ return ::rage::Mat33V( (m33).a, (m33).b, (m33).c ); }
__forceinline ::rage::Matrix34		MAT34V_TO_MATRIX34( ::rage::Mat34V_In m34v )		{ return ::rage::Matrix34( (m34v).GetCol0Intrin128(), (m34v).GetCol1Intrin128(), (m34v).GetCol2Intrin128(), (m34v).GetCol3Intrin128() ); }
__forceinline ::rage::Mat34V_Out	MATRIX34_TO_MAT34V( const ::rage::Matrix34& m34 )	{ return ::rage::Mat34V( (m34).a, (m34).b, (m34).c, (m34).d ); }
__forceinline ::rage::Matrix44		MAT44V_TO_MATRIX44( ::rage::Mat44V_In m44v )		{ return ::rage::Matrix44( (m44v).GetCol0Intrin128(), (m44v).GetCol1Intrin128(), (m44v).GetCol2Intrin128(), (m44v).GetCol3Intrin128() ); }
__forceinline ::rage::Mat44V_Out	MATRIX44_TO_MAT44V( const ::rage::Matrix44& m44 )	{ return ::rage::Mat44V( (m44).a, (m44).b, (m44).c, (m44).d ); }
#else
__forceinline ::rage::Matrix33		MAT33V_TO_MATRIX33( ::rage::Mat33V_Val m33v )	{ return ::rage::Matrix33( (m33v).GetCol0Intrin128(), (m33v).GetCol1Intrin128(), (m33v).GetCol2Intrin128() ); }
__forceinline ::rage::Mat33V_Out	MATRIX33_TO_MAT33V( ::rage::Matrix33 m33 )		{ return ::rage::Mat33V( (m33).a, (m33).b, (m33).c ); }
__forceinline ::rage::Matrix34		MAT34V_TO_MATRIX34( ::rage::Mat34V_Val m34v )	{ return ::rage::Matrix34( (m34v).GetCol0Intrin128(), (m34v).GetCol1Intrin128(), (m34v).GetCol2Intrin128(), (m34v).GetCol3Intrin128() ); }
__forceinline ::rage::Mat34V_Out	MATRIX34_TO_MAT34V( ::rage::Matrix34 m34 )		{ return ::rage::Mat34V( (m34).a, (m34).b, (m34).c, (m34).d ); }
__forceinline ::rage::Matrix44		MAT44V_TO_MATRIX44( ::rage::Mat44V_Val m44v )	{ return ::rage::Matrix44( (m44v).GetCol0Intrin128(), (m44v).GetCol1Intrin128(), (m44v).GetCol2Intrin128(), (m44v).GetCol3Intrin128() ); }
__forceinline ::rage::Mat44V_Out	MATRIX44_TO_MAT44V( ::rage::Matrix44 m44 )		{ return ::rage::Mat44V( (m44).a, (m44).b, (m44).c, (m44).d ); }
#endif

#define	SCALARV_TO_VECTOR3( sv )		(::rage::Vector3(SCALARV_TO_INTRIN(sv)))
#define	VECTOR3_TO_SCALARV( v3 )		(::rage::ScalarV(VECTOR3_TO_INTRIN(v3)))

#define	SCALARV_TO_VECTOR4( sv )		(::rage::Vector4(SCALARV_TO_INTRIN(sv)))
#define	VECTOR4_TO_SCALARV( v4 )		(::rage::ScalarV(VECTOR4_TO_INTRIN(v4)))

#define VECTOR3_TO_VEC3F(v3)			(::rage::Vec3f((v3).x, (v3).y, (v3).z))

#define VECTOR2_TO_VEC2F(v2)			(::rage::Vec2f((v2).x, (v2).y))
#define VEC2F_TO_VECTOR2(v2)			(::rage::Vector2((v2).GetX(), (v2).GetY())


#endif // VECTORMATH_LEGACYCONVERT_H
