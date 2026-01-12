#ifndef VECTORMATH_VEC2V_SOA_H
#define VECTORMATH_VEC2V_SOA_H

#include "data/struct.h"
#include "vectormath.h"
#include "scalarv_soa.h"

namespace rage
{
	class SoA_Vec2V;

	//================================================
	// TYPES
	//================================================

	typedef SoA_Vec2V			SoA_Vec2V_Val;
	typedef SoA_Vec2V*			SoA_Vec2V_Ptr;
	typedef SoA_Vec2V&			SoA_Vec2V_Ref;
	typedef const SoA_Vec2V&	SoA_Vec2V_ConstRef;

	typedef SoA_Vec2V_ConstRef	SoA_Vec2V_In;
	typedef const SoA_Vec2V_Val	SoA_Vec2V_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef SoA_Vec2V_Ref		SoA_Vec2V_InOut;

#define VEC2V_SOA_DECL(m)			rage::Vec::Vector_4V_In m##_x,rage::Vec::Vector_4V_In m##_y
#define VEC2V_SOA_DECL2(m)			rage::Vec::Vector_4V_In m##_x,rage::Vec::Vector_4V_In_After3Args m##_y
#define VEC2V_SOA_DECL3(m)			rage::Vec::Vector_4V_In_After3Args m##_x,rage::Vec::Vector_4V_In_After3Args m##_y
#define VEC2V_SOA_ARG(m)			(m).GetXIntrin128(),(m).GetYIntrin128()
#define VEC2V_SOA_ARG_GET(m)		SoA_Vec2V( m##_x,m##_y )

	//================================================
	// VEC2V
	//================================================

	class SoA_Vec2V
	{
		friend class Vec3V;
		friend class Vec4V;
		friend class QuatV;
		friend class Mat33V;
		friend class Mat34V;
		friend class Mat44V;

	public:
		enum eZEROInitializer			{	ZERO = 0,	};
		enum eONEInitializer			{	ONE = 0,	};
		enum eMASKXYInitializer			{	MASKXY = 0,	};
		enum eFLT_LARGE_8Initializer	{	FLT_LARGE_8 = 0,	};
		enum eFLT_EPSInitializer		{	FLT_EPS = 0,	};
		enum eFLTMAXInitializer			{	FLTMAX = 0,	};

	public:
		// vector constant generation syntax: SoA_Vec2V( SoA_Vec2V::ZERO )
		explicit SoA_Vec2V(eZEROInitializer);
		explicit SoA_Vec2V(eONEInitializer);
		explicit SoA_Vec2V(eMASKXYInitializer);
		explicit SoA_Vec2V(eFLT_LARGE_8Initializer);
		explicit SoA_Vec2V(eFLT_EPSInitializer);
		explicit SoA_Vec2V(eFLTMAXInitializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		SoA_Vec2V(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(SoA_Vec2V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(SoA_Vec2V);
			STRUCT_FIELD( x );
			STRUCT_FIELD( y );
			STRUCT_END();
		}
#endif

		SoA_Vec2V();
		// Note: This copy constructor must be defined in order to avoid a million unnecessary lvx/stvx calls when returning matrices by value!
		// https://ps3.scedev.net/forums/thread/24368
		SoA_Vec2V(SoA_Vec2V_ConstRef);
		SoA_Vec2V_Ref operator= (SoA_Vec2V_ConstRef);

		explicit SoA_Vec2V(const float& x, const float& y);
		explicit SoA_Vec2V(SoA_ScalarV_In scalar);
		explicit SoA_Vec2V(SoA_ScalarV_In x, SoA_ScalarV_In y);
		explicit SoA_Vec2V(Vec::Vector_4V_In scalar);
		explicit SoA_Vec2V(Vec::Vector_4V_In x, Vec::Vector_4V_In y);

		void SetXIntrin128(Vec::Vector_4V_In v);
		void SetYIntrin128(Vec::Vector_4V_In v);
		Vec::Vector_4V_Out GetXIntrin128() const;
		Vec::Vector_4V_Ref GetXIntrin128Ref();
		Vec::Vector_4V_Out GetYIntrin128() const;
		Vec::Vector_4V_Ref GetYIntrin128Ref();

		SoA_ScalarV_Out GetX() const;
		SoA_ScalarV_Out GetY() const;
		void SetX( SoA_ScalarV_In newX );
		void SetY( SoA_ScalarV_In newY );

		void ZeroComponents();

		//============================================================================
		// Output

		void Print(bool newline=true) const;
		void PrintHex(bool newline=true) const;

		//============================================================================
		// Operators

		// Logical
		//SoA_VecBool2V_Out	operator==	(SoA_Vec2V_In b) const;
		//SoA_VecBool2V_Out	operator!=	(SoA_Vec2V_In b) const;
		//SoA_VecBool2V_Out	operator<	(SoA_Vec2V_In bigVector) const;
		//SoA_VecBool2V_Out	operator<=	(SoA_Vec2V_In bigVector) const;
		//SoA_VecBool2V_Out	operator>	(SoA_Vec2V_In smallVector) const;
		//SoA_VecBool2V_Out	operator>=	(SoA_Vec2V_In smallVector) const;

		// Arithmetic
		SoA_Vec2V_Out		operator*	(SoA_Vec2V_In b) const; // per-element multiply!
		SoA_Vec2V_Out		operator*	(SoA_ScalarV_In b) const;
		friend SoA_Vec2V_Out operator*	(SoA_ScalarV_In a, SoA_Vec2V_In b);
		SoA_Vec2V_Out		operator/	(SoA_Vec2V_In b) const; // per-element divide!
		SoA_Vec2V_Out		operator/	(SoA_ScalarV_In b) const;
		SoA_Vec2V_Out		operator+	(SoA_Vec2V_In b) const;
		SoA_Vec2V_Out		operator-	(SoA_Vec2V_In b) const;
		void				operator*=	(SoA_Vec2V_In b);
		void				operator*=	(SoA_ScalarV_In b);
		void				operator/=	(SoA_Vec2V_In b);
		void				operator/=	(SoA_ScalarV_In b);
		void				operator+=	(SoA_Vec2V_In b);
		void				operator-=	(SoA_Vec2V_In b);
		SoA_Vec2V_Out		operator-	() const;

		// Bitwise
		SoA_Vec2V_Out	operator|	(SoA_Vec2V_In b) const;
		SoA_Vec2V_Out	operator&	(SoA_Vec2V_In b) const;
		SoA_Vec2V_Out	operator^	(SoA_Vec2V_In b) const;
		void			operator|=	(SoA_Vec2V_In b);
		void			operator&=	(SoA_Vec2V_In b);
		void			operator^=	(SoA_Vec2V_In b);

		// Element access.
		Vec::Vector_4V_ConstRef	operator[]	(u32 elem) const;
		Vec::Vector_4V_Ref		operator[]	(u32 elem);

	private:
		Vec::Vector_4V x;
		Vec::Vector_4V y;
	};

} // namespace rage

#include "vec2v_soa.inl"


#endif // VECTORMATH_VEC2V_SOA_H
