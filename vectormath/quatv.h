#ifndef VECTORMATH_QUATV_H
#define VECTORMATH_QUATV_H

#include "data/struct.h"
#include "vectormath.h"
#include "vec3v.h"
#include "vec2v.h"
#include "scalarv.h"

namespace rage
{
	class QuatV;

	//================================================
	// TYPES
	//================================================

	typedef QuatV			QuatV_Val;
	typedef QuatV*			QuatV_Ptr;
	typedef const QuatV*	QuatV_ConstPtr;
	typedef QuatV&			QuatV_Ref;
	typedef const QuatV&	QuatV_ConstRef;

#if __WIN32PC
	typedef QuatV_ConstRef	QuatV_In;
#else
	typedef QuatV_Val		QuatV_In;
#endif
	typedef const QuatV_Val	QuatV_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef QuatV_Ref		QuatV_InOut;

	// Use this to pass the quat as four floats into a function, e.g. Displayf("<%f, %f, %f, %f>, VEC4V_ARGS(vec));
#define QUATV_ARGS(v) (v).GetXf(),(v).GetYf(),(v).GetZf(),(v).GetWf()

	//================================================
	// QUATV
	//================================================

	// Quaternion of the form: q = q3 + i*q0 + j*q1 + k*q2
	// Represented as: <x,y,z,w> = <q0,q1,q2,q3>

	// Unit quaternions represent rotations. The first three elements (x, y and z) are the unit axis of
	// rotation scaled by the sine of half the angle of rotation, and the last element (w) is the cosine
	// of half the angle of rotation.

#if __XENON && UNIQUE_VECTORIZED_TYPE
	__declspec(passinreg)
#endif
	class QuatV
	{
		friend class Vec4V;

	public:
		// quat constant generation syntax: QuatV(V_ZERO)
		explicit QuatV(eZEROInitializer);
		explicit QuatV(eFLT_LARGE_8Initializer);
		explicit QuatV(eFLT_EPSILONInitializer);
		explicit QuatV(eFLT_SMALL_6Initializer);
		explicit QuatV(eFLT_SMALL_5Initializer);
		explicit QuatV(eIDENTITYInitializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		QuatV(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(QuatV);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(QuatV);
			STRUCT_FIELD( v );
			STRUCT_END();
		}
#endif

		QuatV();
		explicit QuatV(const float&,const float&,const float&,const float&);
		explicit QuatV(Vec::Vector_4V_In);
		explicit QuatV(ScalarV_In);
		explicit QuatV(ScalarV_In,ScalarV_In,ScalarV_In,ScalarV_In);
		explicit QuatV(ScalarV_In,ScalarV_In,Vec2V_In);
		explicit QuatV(ScalarV_In,Vec2V_In,ScalarV_In);
		explicit QuatV(Vec2V_In,ScalarV_In,ScalarV_In);
		explicit QuatV(ScalarV_In,Vec3V_In);
		explicit QuatV(Vec2V_In,Vec2V_In);
		explicit QuatV(Vec3V_In,ScalarV_In);
		explicit QuatV(Vec3V_In); // Leaves w undefined.
	private:
		explicit QuatV(int,int,int,int); // DO NOT USE
	public:

#if __WIN32PC
		QuatV(QuatV_ConstRef _v);
#endif

		// operator= cripples pass-by-register on Xenon as of XDK 7776.0!
#if !__XENON
		QuatV_ConstRef operator=(QuatV_ConstRef _v);
#endif

		void SetIntrin128(Vec::Vector_4V_In v);
		Vec::Vector_4V_Val GetIntrin128() const;
		Vec::Vector_4V_Ref GetIntrin128Ref();
		Vec::Vector_4V_ConstRef GetIntrin128ConstRef() const;

		ScalarV_Out GetX() const;
		ScalarV_Out GetY() const;
		ScalarV_Out GetZ() const;
		ScalarV_Out GetW() const;
		Vec3V_Out GetXYZ() const;
		void SetX( ScalarV_In newX );
		void SetY( ScalarV_In newY );
		void SetZ( ScalarV_In newZ );
		void SetW( ScalarV_In newW );
		void SetXInMemory( ScalarV_In newX );
		void SetYInMemory( ScalarV_In newY );
		void SetZInMemory( ScalarV_In newZ );
		void SetWInMemory( ScalarV_In newW );
		void SetXYZ( Vec3V_In newXYZ );
		void SetX( const float& floatVal );
		void SetY( const float& floatVal );
		void SetZ( const float& floatVal );
		void SetW( const float& floatVal );

		float GetXf() const;
		float GetYf() const;
		float GetZf() const;
		float GetWf() const;
		float GetElemf( unsigned elem ) const;
		void SetElemf( unsigned elem, float fVal );
		void SetXf( float floatVal );
		void SetYf( float floatVal );
		void SetZf( float floatVal );
		void SetWf( float floatVal );

		void ZeroComponents();
		void SetWZero();

		//============================================================================
		// Output

#if !__NO_OUTPUT
		void Print(bool newline=true) const;
		void PrintHex(bool newline=true) const;
#endif

		//============================================================================
		// Operators

		// Logical
		VecBoolV_Out	operator==	(QuatV_In b) const;
		VecBoolV_Out	operator!=	(QuatV_In b) const;

		// Element access. Warning: Expensive operations.
		const float&	operator[]	(unsigned elem) const;
		float&			operator[]	(unsigned elem);	

	private:
		Vec::Vector_4V v;
	}
#if __PPU
	__attribute__((vecreturn))
#endif
	;

} // namespace rage

#include "quatv.inl"

#endif // VECTORMATH_QUATV_H
