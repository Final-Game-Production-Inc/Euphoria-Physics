#ifndef VECTORMATH_VECBOOL2V_SOA_H
#define VECTORMATH_VECBOOL2V_SOA_H

#include "data/struct.h"
#include "vectormath.h"

namespace rage
{

	class SoA_VecBool2V;

	//================================================
	// TYPES
	//================================================

	typedef SoA_VecBool2V			SoA_VecBool2V_Val;
	typedef SoA_VecBool2V*			SoA_VecBool2V_Ptr;
	typedef SoA_VecBool2V&			SoA_VecBool2V_Ref;
	typedef const SoA_VecBool2V&	SoA_VecBool2V_ConstRef;

	typedef SoA_VecBool2V_ConstRef	SoA_VecBool2V_In;
	typedef const SoA_VecBool2V_Val	SoA_VecBool2V_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef SoA_VecBool2V_Ref		SoA_VecBool2V_InOut;

#define VECBOOL2V_SOA_DECL(m)			rage::Vec::Vector_4V_In m##_x,rage::Vec::Vector_4V_In m##_y
#define VECBOOL2V_SOA_DECL2(m)			rage::Vec::Vector_4V_In m##_x,rage::Vec::Vector_4V_In_After3Args m##_y
#define VECBOOL2V_SOA_ARG(m)			(m).GetXIntrin128(),(m).GetYIntrin128()
#define VECBOOL2V_SOA_ARG_GET(m)		SoA_VecBool2V( m##_x,m##_y )

	//================================================
	// VECBOOL2V
	//================================================

	class SoA_VecBool2V
	{
	public:

		enum eF_FInitializer {	F_F = 0,	};
		enum eF_TInitializer {	F_T = 0,	};
		enum eT_FInitializer {	T_F = 0,	};
		enum eT_TInitializer {	T_T = 0,	};

	public:
		SoA_VecBool2V();
		// Note: This copy constructor must be defined in order to avoid a million unnecessary lvx/stvx calls when returning matrices by value!
		// https://ps3.scedev.net/forums/thread/24368
		SoA_VecBool2V(SoA_VecBool2V_ConstRef);
		SoA_VecBool2V_Ref operator= (SoA_VecBool2V_ConstRef);

		explicit SoA_VecBool2V(eF_FInitializer);
		explicit SoA_VecBool2V(eF_TInitializer);
		explicit SoA_VecBool2V(eT_FInitializer);
		explicit SoA_VecBool2V(eT_TInitializer);

		explicit SoA_VecBool2V(Vec::Vector_4V_In v);
		explicit SoA_VecBool2V(Vec::Vector_4V_In x,Vec::Vector_4V_In y);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		SoA_VecBool2V(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(SoA_VecBool2V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(SoA_VecBool2V);
			STRUCT_FIELD( x );
			STRUCT_FIELD( y );
			STRUCT_END();
		}
#endif

		//============================================================================
		// Operators

		// Logical
		SoA_VecBool2V_Out operator==	(SoA_VecBool2V_In b) const;
		SoA_VecBool2V_Out operator!=	(SoA_VecBool2V_In b) const;
		SoA_VecBool2V_Out operator! () const;

		// Bitwise
		SoA_VecBool2V_Out	operator|	(SoA_VecBool2V_In b) const;
		SoA_VecBool2V_Out	operator&	(SoA_VecBool2V_In b) const;
		SoA_VecBool2V_Out	operator^	(SoA_VecBool2V_In b) const;
		void				operator|=	(SoA_VecBool2V_In b);
		void				operator&=	(SoA_VecBool2V_In b);
		void				operator^=	(SoA_VecBool2V_In b);

		void SetXIntrin128(Vec::Vector_4V_In x);
		void SetYIntrin128(Vec::Vector_4V_In y);
		Vec::Vector_4V_Out GetXIntrin128() const;
		Vec::Vector_4V_Out GetYIntrin128() const;

		void Print(bool newline=true);

	private:
		Vec::Vector_4V x;
		Vec::Vector_4V y;
	};

} // namespace rage

#include "vecbool2v_soa.inl"


#endif // VECTORMATH_VECBOOL2V_SOA_H
