#ifndef VECTORMATH_VEC1V_SOA_H
#define VECTORMATH_VEC1V_SOA_H

#include "data/struct.h"
#include "vectormath.h"
#include "vecbool1v_soa.h"

// This vector always has 4 components of the same value.

namespace rage
{
	class SoA_ScalarV;

	//================================================
	// TYPES
	//================================================

	typedef SoA_ScalarV					SoA_ScalarV_Val;
	typedef SoA_ScalarV*				SoA_ScalarV_Ptr;
	typedef SoA_ScalarV&				SoA_ScalarV_Ref;
	typedef const SoA_ScalarV&			SoA_ScalarV_ConstRef;

#if __WIN32PC
	typedef SoA_ScalarV_ConstRef		SoA_ScalarV_In;
#else
	typedef SoA_ScalarV_Val				SoA_ScalarV_In;
#endif
	typedef const SoA_ScalarV_Val		SoA_ScalarV_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef SoA_ScalarV_Ref				SoA_ScalarV_InOut;

	//================================================
	// VEC1V
	//================================================

#if __XENON && UNIQUE_VECTORIZED_TYPE
	__declspec(passinreg)
#endif
	class SoA_ScalarV
	{
		friend class SoA_Vec2V;
		friend class SoA_Vec3V;
		friend class SoA_Vec4V;
		friend class SoA_QuatV;
		friend class SoA_Mat33V;
		friend class SoA_Mat34V;
		friend class SoA_Mat44V;

	public:
		enum eZEROInitializer			{	ZERO = 0,	};
		enum eONEInitializer			{	ONE = 0,	};
		enum eMASKXInitializer			{	MASKX = 0,	};
		enum eFLT_LARGE_8Initializer	{	FLT_LARGE_8 = 0,	};
		enum eFLT_EPSInitializer		{	FLT_EPS = 0,	};
		enum eFLTMAXInitializer			{	FLTMAX = 0,	};

	public:
		// vector constant generation syntax: SoA_ScalarV( SoA_ScalarV::ZERO )
		explicit SoA_ScalarV(eZEROInitializer);
		explicit SoA_ScalarV(eONEInitializer);
		explicit SoA_ScalarV(eMASKXInitializer);
		explicit SoA_ScalarV(eFLT_LARGE_8Initializer);
		explicit SoA_ScalarV(eFLT_EPSInitializer);
		explicit SoA_ScalarV(eFLTMAXInitializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		SoA_ScalarV(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(SoA_ScalarV);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(SoA_ScalarV);
			STRUCT_FIELD( x );
			STRUCT_END();
		}
#endif

		SoA_ScalarV();
		explicit SoA_ScalarV(Vec::Vector_4V_In scalar);

#if __WIN32PC
		SoA_ScalarV(SoA_ScalarV_ConstRef);
#endif

		// operator= cripples pass-by-register on Xenon as of XDK 7776.0!
#if !__XENON
		SoA_ScalarV_ConstRef operator= (SoA_ScalarV_ConstRef);
#endif		

		void SetIntrin128(Vec::Vector_4V_In v);
		Vec::Vector_4V_Out GetIntrin128() const;
		Vec::Vector_4V_Ref GetIntrin128Ref();

		void ZeroComponents();

		//============================================================================
		// Output

		void Print(bool newline=true) const;
		void PrintHex(bool newline=true) const;

		//============================================================================
		// Operators

		// Logical
		SoA_VecBool1V_Out	operator==	(SoA_ScalarV_In b) const;
		SoA_VecBool1V_Out	operator!=	(SoA_ScalarV_In b) const;
		SoA_VecBool1V_Out	operator<	(SoA_ScalarV_In bigVector) const;
		SoA_VecBool1V_Out	operator<=	(SoA_ScalarV_In bigVector) const;
		SoA_VecBool1V_Out	operator>	(SoA_ScalarV_In smallVector) const;
		SoA_VecBool1V_Out	operator>=	(SoA_ScalarV_In smallVector) const;

		// Arithmetic
		SoA_ScalarV_Out		operator*	(SoA_ScalarV_In b) const;
		SoA_ScalarV_Out		operator/	(SoA_ScalarV_In b) const;
		SoA_ScalarV_Out		operator+	(SoA_ScalarV_In b) const;
		SoA_ScalarV_Out		operator-	(SoA_ScalarV_In b) const;
		void				operator*=	(SoA_ScalarV_In b);
		void				operator/=	(SoA_ScalarV_In b);
		void				operator+=	(SoA_ScalarV_In b);
		void				operator-=	(SoA_ScalarV_In b);
		SoA_ScalarV_Out		operator-	() const;

		// Bitwise
		SoA_ScalarV_Out		operator|	(SoA_ScalarV_In b) const;
		SoA_ScalarV_Out		operator&	(SoA_ScalarV_In b) const;
		SoA_ScalarV_Out		operator^	(SoA_ScalarV_In b) const;
		void				operator|=	(SoA_ScalarV_In b);
		void				operator&=	(SoA_ScalarV_In b);
		void				operator^=	(SoA_ScalarV_In b);

	private:
		Vec::Vector_4V x;
	}
#if __PPU
	__attribute__((vecreturn))
#endif
	;

} // namespace rage

#include "scalarv_soa.inl"


#endif // VECTORMATH_VEC1V_SOA_H
