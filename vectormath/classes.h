#ifndef VECTORMATH_CLASSES_H
#define VECTORMATH_CLASSES_H

//=======================================================================================================================================
//
//
// INCLUDES
//
//
//=======================================================================================================================================

#include "vectormath.h"


//================================================
// AoS
//================================================

#include "boolv.h"
#include "vecboolv.h"
#include "scalarv.h"
#include "vec2v.h"
#include "vec3v.h"
#include "vec4v.h"
#include "quatv.h"
#include "mat44v.h"
#include "mat33v.h"
#include "mat34v.h"
#include "transformv.h"
#include "vec2f.h"
#include "vec3f.h"
#include "vec4f.h"
#include "quatf.h"




//=======================================================================================================================================
//
//
// FREE FUNCTIONS (class arguments)
//
//
//=======================================================================================================================================

// Scalar Vec* free functions.
#include "classfreefuncsf.h"

// Vectorized Vec*V free functions.
#include "classfreefuncsv.h"

namespace rage
{
	// V4 AoS type conversions (Vec4V <--> Vec3V <--> Vec2V <--> ScalarV <--> VecBoolV <--> QuatV).
	template <typename _4tupleTo, typename _4tupleFrom>
	void CastV( _4tupleTo& _to, const _4tupleFrom& from );

	// Use these to use either float or vec load/store instructions, regardless of the data type.
	// If just converting between local variables, use Load*(). This will incur one LHS.
	void LoadAsScalar( Quatf_Ref a, QuatV_ConstRef loc );
	void LoadAsVec( QuatV_Ref a, Quatf_ConstRef loc );
	void StoreAsScalar( QuatV_Ref loc, Quatf_ConstRef a );
	void StoreAsVec( Quatf_Ref loc, QuatV_ConstRef a );

	void LoadAsScalar( Vec4f_Ref a, Vec4V_ConstRef loc );
	void LoadAsVec( Vec4V_Ref a, Vec4f_ConstRef loc );
	void StoreAsScalar( Vec4V_Ref loc, Vec4f_ConstRef a );
	void StoreAsVec( Vec4f_Ref loc, Vec4V_ConstRef a );

	void LoadAsScalar( Vec3f_Ref a, Vec3V_ConstRef loc );
	void LoadAsVec( Vec3V_Ref a, Vec3f_ConstRef loc );
	void StoreAsScalar( Vec3V_Ref loc, Vec3f_ConstRef a );
	//void StoreAsVec( Vec3f_Ref loc, Vec3V_ConstRef a );

	void LoadAsScalar( Vec2f_Ref a, Vec2V_ConstRef loc );
	void LoadAsVec( Vec2V_Ref a, Vec2f_ConstRef loc );
	void StoreAsScalar( Vec2V_Ref loc, Vec2f_ConstRef a );
	//void StoreAsVec( Vec2f_Ref loc, Vec2V_ConstRef a );
}


//=======================================================================================================================================
//
//
// FREE FUNCTION IMPLEMENTATIONS
//
//
//=======================================================================================================================================

// Scalar Vec* free functions.
#include "classfreefuncsf.inl"

// Vectorized Vec*V free functions.
#include "classfreefuncsv.inl"

namespace rage
{
	template <typename _To, typename _From>
	__forceinline void CastV( _To& _to, const _From& from )
	{
		_to.SetIntrin128(from.GetIntrin128());
	}

	__forceinline void LoadAsScalar( Quatf_Ref a, QuatV_ConstRef loc )
	{
		Vec::V4LoadAsScalar( a.GetIntrinRef(), loc.GetIntrin128ConstRef() );
	}

	__forceinline void LoadAsVec( QuatV_Ref a, Quatf_ConstRef loc )
	{
		Vec::V4LoadAsVec( a.GetIntrin128Ref(), loc.GetIntrinConstRef() );
	}

	__forceinline void StoreAsScalar( QuatV_Ref loc, Quatf_ConstRef a )
	{
		Vec::V4StoreAsScalar( loc.GetIntrin128Ref(), a.GetIntrinConstRef() );
	}

	__forceinline void StoreAsVec( Quatf_Ref loc, QuatV_ConstRef a )
	{
		Vec::V4StoreAsVec( loc.GetIntrinRef(), a.GetIntrin128ConstRef() );
	}

	__forceinline void LoadAsScalar( Vec4f_Ref a, Vec4V_ConstRef loc )
	{
		Vec::V4LoadAsScalar( a.GetIntrinRef(), loc.GetIntrin128ConstRef() );
	}

	__forceinline void LoadAsVec( Vec4V_Ref a, Vec4f_ConstRef loc )
	{
		Vec::V4LoadAsVec( a.GetIntrin128Ref(), loc.GetIntrinConstRef() );
	}

	__forceinline void StoreAsScalar( Vec4V_Ref loc, Vec4f_ConstRef a )
	{
		Vec::V4StoreAsScalar( loc.GetIntrin128Ref(), a.GetIntrinConstRef() );
	}

	__forceinline void StoreAsVec( Vec4f_Ref loc, Vec4V_ConstRef a )
	{
		Vec::V4StoreAsVec( loc.GetIntrinRef(), a.GetIntrin128ConstRef() );
	}

	__forceinline void LoadAsScalar( Vec3f_Ref a, Vec3V_ConstRef loc )
	{
		Vec::V3LoadAsScalar( a.GetIntrinRef(), loc.GetIntrin128ConstRef() );
	}

	__forceinline void LoadAsVec( Vec3V_Ref a, Vec3f_ConstRef loc )
	{
		Vec::V3LoadAsVec( a.GetIntrin128Ref(), loc.GetIntrinConstRef() );
	}

	__forceinline void StoreAsScalar( Vec3V_Ref loc, Vec3f_ConstRef a )
	{
		Vec::V3StoreAsScalar( loc.GetIntrin128Ref(), a.GetIntrinConstRef() );
	}

	__forceinline void LoadAsScalar( Vec2f_Ref a, Vec2V_ConstRef loc )
	{
		Vec::V2LoadAsScalar( a.GetIntrinRef(), loc.GetIntrin128ConstRef() );
	}

	__forceinline void LoadAsVec( Vec2V_Ref a, Vec2f_ConstRef loc )
	{
		Vec::V2LoadAsVec( a.GetIntrin128Ref(), loc.GetIntrinConstRef() );
	}

	__forceinline void StoreAsScalar( Vec2V_Ref loc, Vec2f_ConstRef a )
	{
		Vec::V2StoreAsScalar( loc.GetIntrin128Ref(), a.GetIntrinConstRef() );
	}
}

#include "data/serialize.h"

namespace rage
{
	inline datSerialize & operator<< (datSerialize &s, Vec2V_InOut v) {
		s << v[0] << v[1];
		return s;
	}

	inline datSerialize & operator<< (datSerialize &s, Vec3V_InOut v) {
		s << v[0] << v[1] << v[2];
		return s;
	}

	inline datSerialize & operator<< (datSerialize &s, Vec4V_InOut v) {
		s << v[0] << v[1] << v[2] << v[3];
		return s;
	}

	inline datSerialize & operator<< (datSerialize &s, QuatV_InOut q) {
		s << q[0] << q[1] << q[2] << q[3];
		return s;
	}

	inline datSerialize & operator<< (datSerialize &s, ScalarV_InOut v) {
		float f = v.Getf();
		s << f;
		v.Setf(f);
		return s;
	}

	inline datSerialize & operator<< (datSerialize &s, Mat34V_InOut m) {
		s << m.GetCol0Ref() << m.GetCol1Ref() << m.GetCol2Ref() << m.GetCol3Ref();
		return s;
	}

	inline datSerialize & operator<< (datSerialize &s, TransformV_InOut t) {
		s << t.GetRotationRef() << t.GetPositionRef();
		return s;
	}
}

#endif // VECTORMATH_CLASSES_H
