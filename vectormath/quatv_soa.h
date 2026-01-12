#ifndef VECTORMATH_QUATV_SOA_H
#define VECTORMATH_QUATV_SOA_H

#include "data/struct.h"
#include "vectormath.h"
#include "scalarv_soa.h"
#include "vecbool4v_soa.h"

namespace rage
{
	class SoA_QuatV;

	//================================================
	// TYPES
	//================================================

	typedef SoA_QuatV			SoA_QuatV_Val;
	typedef SoA_QuatV*			SoA_QuatV_Ptr;
	typedef SoA_QuatV&			SoA_QuatV_Ref;
	typedef const SoA_QuatV&	SoA_QuatV_ConstRef;

	typedef SoA_QuatV_ConstRef	SoA_QuatV_In;
	typedef const SoA_QuatV_Val	SoA_QuatV_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef SoA_QuatV_Ref		SoA_QuatV_InOut;

#define QUATV_SOA_DECL(m)			rage::Vec::Vector_4V_In m##_x,rage::Vec::Vector_4V_In m##_y,rage::Vec::Vector_4V_In m##_z,rage::Vec::Vector_4V_In_After3Args m##_w
#define QUATV_SOA_DECL2(m)			rage::Vec::Vector_4V_In_After3Args m##_x,rage::Vec::Vector_4V_In_After3Args m##_y,rage::Vec::Vector_4V_In_After3Args m##_z,rage::Vec::Vector_4V_In_After3Args m##_w
#define QUATV_SOA_ARG(m)			(m).GetXIntrin128(),(m).GetYIntrin128(),(m).GetZIntrin128(),(m).GetWIntrin128()
#define QUATV_SOA_ARG_GET(m)		SoA_QuatV( m##_x,m##_y,m##_z,m##_w )

	//================================================
	// QUATV
	//================================================

	// Quaternion of the form: q = q3 + i*q0 + j*q1 + k*q2
	// Represented as: <x,y,z,w> = <q0,q1,q2,q3>

	// Unit quaternions represent rotations. The first three elements (x, y and z) are the unit axis of
	// rotation scaled by the sine of half the angle of rotation, and the last element (w) is the cosine
	// of half the angle of rotation.

	class SoA_QuatV
	{
		friend class SoA_Vec4V;
		friend class SoA_Mat33V;
		friend class SoA_Mat34V;
		friend class SoA_Mat44V;

	public:
		enum eZEROInitializer			{	ZERO = 0,	};
		enum eIDENTITYInitializer		{	IDENTITY = 0,	};
		enum eFLT_LARGE_8Initializer	{	FLT_LARGE_8 = 0,	};
		enum eFLT_EPSInitializer		{	FLT_EPS = 0,	};

	public:
		// vector constant generation syntax: SoA_QuatV( SoA_QuatV::ZERO )
		explicit SoA_QuatV(eZEROInitializer);
		explicit SoA_QuatV(eIDENTITYInitializer);
		explicit SoA_QuatV(eFLT_LARGE_8Initializer);
		explicit SoA_QuatV(eFLT_EPSInitializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		SoA_QuatV(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(SoA_QuatV);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(SoA_QuatV);
			STRUCT_FIELD( x );
			STRUCT_FIELD( y );
			STRUCT_FIELD( z );
			STRUCT_FIELD( w );
			STRUCT_END();
		}
#endif

		SoA_QuatV();
		// Note: This copy constructor must be defined in order to avoid a million unnecessary lvx/stvx calls when returning matrices by value!
		// https://ps3.scedev.net/forums/thread/24368
		SoA_QuatV(SoA_QuatV_ConstRef);
		SoA_QuatV_Ref operator= (SoA_QuatV_ConstRef);

		explicit SoA_QuatV(const float& x,const float& y,const float& z,const float& w);
		explicit SoA_QuatV(Vec::Vector_4V_In scalar);
		explicit SoA_QuatV(Vec::Vector_4V_In x,Vec::Vector_4V_In y,Vec::Vector_4V_In z,Vec::Vector_4V_In_After3Args w);

		void SetXIntrin128(Vec::Vector_4V_In x);
		void SetYIntrin128(Vec::Vector_4V_In y);
		void SetZIntrin128(Vec::Vector_4V_In z);
		void SetWIntrin128(Vec::Vector_4V_In w);
		Vec::Vector_4V_Val GetXIntrin128() const;
		Vec::Vector_4V_Val GetYIntrin128() const;
		Vec::Vector_4V_Val GetZIntrin128() const;
		Vec::Vector_4V_Val GetWIntrin128() const;
		Vec::Vector_4V_Ref GetXIntrin128Ref();
		Vec::Vector_4V_Ref GetYIntrin128Ref();
		Vec::Vector_4V_Ref GetZIntrin128Ref();
		Vec::Vector_4V_Ref GetWIntrin128Ref();

		SoA_ScalarV_Out GetX() const;
		SoA_ScalarV_Out GetY() const;
		SoA_ScalarV_Out GetZ() const;
		SoA_ScalarV_Out GetW() const;
		void SetX( SoA_ScalarV_In newX );
		void SetY( SoA_ScalarV_In newY );
		void SetZ( SoA_ScalarV_In newZ );
		void SetW( SoA_ScalarV_In newW );

		void ZeroComponents();

		//============================================================================
		// Output

		void Print(bool newline=true) const;
		void PrintHex(bool newline=true) const;

		//============================================================================
		// Operators

		// Logical
		SoA_VecBool4V_Out	operator==	(SoA_QuatV_In b) const;
		SoA_VecBool4V_Out	operator!=	(SoA_QuatV_In b) const;

		// Element access.
		Vec::Vector_4V_ConstRef	operator[]	(u32 elem) const;
		Vec::Vector_4V_Ref		operator[]	(u32 elem);

	private:
		Vec::Vector_4V x;
		Vec::Vector_4V y;
		Vec::Vector_4V z;
		Vec::Vector_4V w;
	};

} // namespace rage

#include "quatv_soa.inl"

#endif // VECTORMATH_QUATV_SOA_H
