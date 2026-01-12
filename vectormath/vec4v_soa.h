#ifndef VECTORMATH_VEC4V_SOA_H
#define VECTORMATH_VEC4V_SOA_H

#include "data/struct.h"
#include "vectormath.h"
#include "scalarv_soa.h"
#include "quatv_soa.h"
#include "vec3v_soa.h"
#include "vecbool4v_soa.h"

namespace rage
{
	class SoA_Vec4V;

	//================================================
	// TYPES
	//================================================

	typedef SoA_Vec4V			SoA_Vec4V_Val;
	typedef SoA_Vec4V*			SoA_Vec4V_Ptr;
	typedef SoA_Vec4V&			SoA_Vec4V_Ref;
	typedef const SoA_Vec4V&	SoA_Vec4V_ConstRef;

	typedef SoA_Vec4V_ConstRef	SoA_Vec4V_In;
	typedef const SoA_Vec4V_Val	SoA_Vec4V_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef SoA_Vec4V_Ref		SoA_Vec4V_InOut;

#define VEC4V_SOA_DECL(m)			rage::Vec::Vector_4V_In m##_x,rage::Vec::Vector_4V_In m##_y,rage::Vec::Vector_4V_In m##_z,rage::Vec::Vector_4V_In_After3Args m##_w
#define VEC4V_SOA_DECL2(m)			rage::Vec::Vector_4V_In_After3Args m##_x,rage::Vec::Vector_4V_In_After3Args m##_y,rage::Vec::Vector_4V_In_After3Args m##_z,rage::Vec::Vector_4V_In_After3Args m##_w
#define VEC4V_SOA_DECL3(m)			rage::Vec::Vector_4V_In m##_x,rage::Vec::Vector_4V_In m##_y,rage::Vec::Vector_4V_In_After3Args m##_z,rage::Vec::Vector_4V_In_After3Args m##_w
#define VEC4V_SOA_ARG(m)			(m).GetXIntrin128(),(m).GetYIntrin128(),(m).GetZIntrin128(),(m).GetWIntrin128()
#define VEC4V_SOA_ARG_GET(m)		SoA_Vec4V( m##_x,m##_y,m##_z,m##_w )

	//================================================
	// VEC4V
	//================================================

	class SoA_Vec4V
	{
		friend class SoA_Mat33V;
		friend class SoA_Mat34V;
		friend class SoA_Mat44V;

	public:
		enum eZEROInitializer			{	ZERO = 0,	};
		enum eONEInitializer			{	ONE = 0,	};
		enum eMASKXYZWInitializer		{	MASKXYZW = 0,	};
		enum eFLT_LARGE_8Initializer	{	FLT_LARGE_8 = 0,	};
		enum eFLT_EPSInitializer		{	FLT_EPS = 0,	};
		enum eFLTMAXInitializer			{	FLTMAX = 0,	};

	public:
		// vector constant generation syntax: SoA_Vec4V( SoA_Vec4V::ZERO )
		explicit SoA_Vec4V(eZEROInitializer);
		explicit SoA_Vec4V(eONEInitializer);
		explicit SoA_Vec4V(eMASKXYZWInitializer);
		explicit SoA_Vec4V(eFLT_LARGE_8Initializer);
		explicit SoA_Vec4V(eFLT_EPSInitializer);
		explicit SoA_Vec4V(eFLTMAXInitializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		SoA_Vec4V(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(SoA_Vec4V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(SoA_Vec4V);
			STRUCT_FIELD( x );
			STRUCT_FIELD( y );
			STRUCT_FIELD( z );
			STRUCT_FIELD( w );
			STRUCT_END();
		}
#endif

		SoA_Vec4V();
		// Note: This copy constructor must be defined in order to avoid a million unnecessary lvx/stvx calls when returning matrices by value!
		// https://ps3.scedev.net/forums/thread/24368
		SoA_Vec4V(SoA_Vec4V_ConstRef);
		SoA_Vec4V_Ref operator= (SoA_Vec4V_ConstRef);

		explicit SoA_Vec4V(const float& x,const float& y,const float& z,const float& w);
		explicit SoA_Vec4V(Vec::Vector_4V_In scalar);
		explicit SoA_Vec4V(Vec::Vector_4V_In x,Vec::Vector_4V_In y,Vec::Vector_4V_In z,Vec::Vector_4V_In_After3Args w);
		explicit SoA_Vec4V(SoA_ScalarV_In,SoA_ScalarV_In,SoA_ScalarV_In,SoA_ScalarV_In);
		explicit SoA_Vec4V(SoA_ScalarV_In); // Splats the input
		explicit SoA_Vec4V(SoA_QuatV_In);

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

		SoA_Vec3V_Out GetXYZ() const;

		SoA_QuatV_Out AsQuatV() const;

		void ZeroComponents();

		//============================================================================
		// Output

		void Print(bool newline=true) const;
		void PrintHex(bool newline=true) const;

		//============================================================================
		// Operators

		// Logical
		SoA_VecBool4V_Out	operator==	(SoA_Vec4V_In b) const;
		SoA_VecBool4V_Out	operator!=	(SoA_Vec4V_In b) const;
		SoA_VecBool4V_Out	operator<	(SoA_Vec4V_In bigVector) const;
		SoA_VecBool4V_Out	operator<=	(SoA_Vec4V_In bigVector) const;
		SoA_VecBool4V_Out	operator>	(SoA_Vec4V_In smallVector) const;
		SoA_VecBool4V_Out	operator>=	(SoA_Vec4V_In smallVector) const;

		// Arithmetic
		SoA_Vec4V_Out		operator*	(SoA_Vec4V_In b) const; // per-element multiply!
		SoA_Vec4V_Out		operator*	(SoA_ScalarV_In b) const;
		friend SoA_Vec4V_Out operator*	(SoA_ScalarV_In a, SoA_Vec4V_In b);
		SoA_Vec4V_Out		operator/	(SoA_Vec4V_In b) const; // per-element divide!
		SoA_Vec4V_Out		operator/	(SoA_ScalarV_In b) const;
		SoA_Vec4V_Out		operator+	(SoA_Vec4V_In b) const;
		SoA_Vec4V_Out		operator-	(SoA_Vec4V_In b) const;
		void			operator*=	(SoA_Vec4V_In b);
		void			operator*=	(SoA_ScalarV_In b);
		void			operator/=	(SoA_Vec4V_In b);
		void			operator/=	(SoA_ScalarV_In b);
		void			operator+=	(SoA_Vec4V_In b);
		void			operator-=	(SoA_Vec4V_In b);
		SoA_Vec4V_Out	operator-	() const;

		// Bitwise
		SoA_Vec4V_Out		operator|	(SoA_Vec4V_In b) const;
		SoA_Vec4V_Out		operator&	(SoA_Vec4V_In b) const;
		SoA_Vec4V_Out		operator^	(SoA_Vec4V_In b) const;
		void				operator|=	(SoA_Vec4V_In b);
		void				operator&=	(SoA_Vec4V_In b);
		void				operator^=	(SoA_Vec4V_In b);

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

#include "vec4v_soa.inl"


#endif // VECTORMATH_VEC4V_SOA_H
