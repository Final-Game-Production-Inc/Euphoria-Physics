#ifndef VECTORMATH_VECBOOL1V_SOA_H
#define VECTORMATH_VECBOOL1V_SOA_H

#include "data/struct.h"
#include "vectormath.h"

namespace rage
{
	class SoA_VecBool1V;

	//================================================
	// TYPES
	//================================================

	typedef SoA_VecBool1V			SoA_VecBool1V_Val;
	typedef SoA_VecBool1V*			SoA_VecBool1V_Ptr;
	typedef SoA_VecBool1V&			SoA_VecBool1V_Ref;
	typedef const SoA_VecBool1V&	SoA_VecBool1V_ConstRef;

	typedef SoA_VecBool1V_ConstRef	SoA_VecBool1V_In;
	typedef const SoA_VecBool1V_Val	SoA_VecBool1V_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef SoA_VecBool1V_Ref		SoA_VecBool1V_InOut;


	//================================================
	// VECBOOL1V
	//================================================

#if __XENON && UNIQUE_VECTORIZED_TYPE
	__declspec(passinreg)
#endif
	class SoA_VecBool1V
	{
	public:

		enum eFInitializer {	F = 0,	};
		enum eTInitializer {	T = 0,	};

	public:
		explicit SoA_VecBool1V(eFInitializer);
		explicit SoA_VecBool1V(eTInitializer);

		SoA_VecBool1V();
		explicit SoA_VecBool1V(Vec::Vector_4V_In x);

#if __WIN32PC
		SoA_VecBool1V(SoA_VecBool1V_ConstRef);
#endif

		// operator= cripples pass-by-register on Xenon as of XDK 7776.0!
#if !__XENON
		SoA_VecBool1V_ConstRef operator= (SoA_VecBool1V_ConstRef);
#endif

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		SoA_VecBool1V(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(SoA_VecBool1V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(SoA_VecBool1V);
			STRUCT_FIELD( x );
			STRUCT_END();
		}
#endif

		void SetIntrin128(Vec::Vector_4V_In x);
		Vec::Vector_4V_Out GetIntrin128() const;

		//============================================================================
		// Operators

		// Logical
		SoA_VecBool1V_Out operator==	(SoA_VecBool1V_In b) const;
		SoA_VecBool1V_Out operator!=	(SoA_VecBool1V_In b) const;
		SoA_VecBool1V_Out operator! () const;

		// Bitwise
		SoA_VecBool1V_Out	operator|	(SoA_VecBool1V_In b) const;
		SoA_VecBool1V_Out	operator&	(SoA_VecBool1V_In b) const;
		SoA_VecBool1V_Out	operator^	(SoA_VecBool1V_In b) const;
		void				operator|=	(SoA_VecBool1V_In b);
		void				operator&=	(SoA_VecBool1V_In b);
		void				operator^=	(SoA_VecBool1V_In b);

		void Print(bool newline=true);

	private:
		Vec::Vector_4V x;
	}
#if __PPU
	__attribute__((vecreturn))
#endif
	;

} // namespace rage

#include "vecbool1v_soa.inl"


#endif // VECTORMATH_VECBOOL1V_SOA_H
