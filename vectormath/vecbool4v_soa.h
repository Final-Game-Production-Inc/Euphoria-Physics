#ifndef VECTORMATH_VECBOOL4V_SOA_H
#define VECTORMATH_VECBOOL4V_SOA_H

#include "data/struct.h"
#include "vectormath.h"

namespace rage
{

	class SoA_VecBool4V;

	//================================================
	// TYPES
	//================================================

	typedef SoA_VecBool4V			SoA_VecBool4V_Val;
	typedef SoA_VecBool4V*			SoA_VecBool4V_Ptr;
	typedef SoA_VecBool4V&			SoA_VecBool4V_Ref;
	typedef const SoA_VecBool4V&	SoA_VecBool4V_ConstRef;

	typedef SoA_VecBool4V_ConstRef	SoA_VecBool4V_In;
	typedef const SoA_VecBool4V_Val	SoA_VecBool4V_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef SoA_VecBool4V_Ref		SoA_VecBool4V_InOut;

#define VECBOOL4V_SOA_DECL(m)			rage::Vec::Vector_4V_In m##_x,rage::Vec::Vector_4V_In m##_y,rage::Vec::Vector_4V_In m##_z,rage::Vec::Vector_4V_In_After3Args m##_w
#define VECBOOL4V_SOA_DECL2(m)			rage::Vec::Vector_4V_In_After3Args m##_x,rage::Vec::Vector_4V_In_After3Args m##_y,rage::Vec::Vector_4V_In_After3Args m##_z,rage::Vec::Vector_4V_In_After3Args m##_w
#define VECBOOL4V_SOA_ARG(m)			(m).GetXIntrin128(),(m).GetYIntrin128(),(m).GetZIntrin128(),(m).GetWIntrin128()
#define VECBOOL4V_SOA_ARG_GET(m)		SoA_VecBool4V( m##_x,m##_y,m##_z,m##_w )

	//================================================
	// VECBOOL4V
	//================================================

	class SoA_VecBool4V
	{
	public:

		enum eF_F_F_FInitializer {	F_F_F_F,	};
		enum eF_F_F_TInitializer {	F_F_F_T,	};
		enum eF_F_T_FInitializer {	F_F_T_F,	};
		enum eF_F_T_TInitializer {	F_F_T_T,	};
		enum eF_T_F_FInitializer {	F_T_F_F,	};
		enum eF_T_F_TInitializer {	F_T_F_T,	};
		enum eF_T_T_FInitializer {	F_T_T_F,	};
		enum eF_T_T_TInitializer {	F_T_T_T,	};
		enum eT_F_F_FInitializer {	T_F_F_F,	};
		enum eT_F_F_TInitializer {	T_F_F_T,	};
		enum eT_F_T_FInitializer {	T_F_T_F,	};
		enum eT_F_T_TInitializer {	T_F_T_T,	};
		enum eT_T_F_FInitializer {	T_T_F_F,	};
		enum eT_T_F_TInitializer {	T_T_F_T,	};
		enum eT_T_T_FInitializer {	T_T_T_F,	};
		enum eT_T_T_TInitializer {	T_T_T_T,	};

	public:
		SoA_VecBool4V();
		// Note: This copy constructor must be defined in order to avoid a million unnecessary lvx/stvx calls when returning matrices by value!
		// https://ps3.scedev.net/forums/thread/24368
		SoA_VecBool4V(SoA_VecBool4V_ConstRef);
		SoA_VecBool4V_Ref operator= (SoA_VecBool4V_ConstRef);

		explicit SoA_VecBool4V(eF_F_F_FInitializer);
		explicit SoA_VecBool4V(eF_F_F_TInitializer);
		explicit SoA_VecBool4V(eF_F_T_FInitializer);
		explicit SoA_VecBool4V(eF_F_T_TInitializer);
		explicit SoA_VecBool4V(eF_T_F_FInitializer);
		explicit SoA_VecBool4V(eF_T_F_TInitializer);
		explicit SoA_VecBool4V(eF_T_T_FInitializer);
		explicit SoA_VecBool4V(eF_T_T_TInitializer);
		explicit SoA_VecBool4V(eT_F_F_FInitializer);
		explicit SoA_VecBool4V(eT_F_F_TInitializer);
		explicit SoA_VecBool4V(eT_F_T_FInitializer);
		explicit SoA_VecBool4V(eT_F_T_TInitializer);
		explicit SoA_VecBool4V(eT_T_F_FInitializer);
		explicit SoA_VecBool4V(eT_T_F_TInitializer);
		explicit SoA_VecBool4V(eT_T_T_FInitializer);
		explicit SoA_VecBool4V(eT_T_T_TInitializer);

		explicit SoA_VecBool4V(Vec::Vector_4V_In);
		explicit SoA_VecBool4V(Vec::Vector_4V_In,Vec::Vector_4V_In,Vec::Vector_4V_In,Vec::Vector_4V_In_After3Args);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		SoA_VecBool4V(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(SoA_VecBool4V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(SoA_VecBool4V);
			STRUCT_FIELD( x );
			STRUCT_FIELD( y );
			STRUCT_FIELD( z );
			STRUCT_FIELD( w );
			STRUCT_END();
		}
#endif

		//============================================================================
		// Operators

		// Logical
		SoA_VecBool4V_Out operator==	(SoA_VecBool4V_In b) const;
		SoA_VecBool4V_Out operator!=	(SoA_VecBool4V_In b) const;
		SoA_VecBool4V_Out operator! () const;

		// Bitwise
		SoA_VecBool4V_Out	operator|	(SoA_VecBool4V_In b) const;
		SoA_VecBool4V_Out	operator&	(SoA_VecBool4V_In b) const;
		SoA_VecBool4V_Out	operator^	(SoA_VecBool4V_In b) const;
		void				operator|=	(SoA_VecBool4V_In b);
		void				operator&=	(SoA_VecBool4V_In b);
		void				operator^=	(SoA_VecBool4V_In b);

		void SetXIntrin128(Vec::Vector_4V_In x);
		void SetYIntrin128(Vec::Vector_4V_In y);
		void SetZIntrin128(Vec::Vector_4V_In z);
		void SetWIntrin128(Vec::Vector_4V_In w);
		Vec::Vector_4V_Out GetXIntrin128() const;
		Vec::Vector_4V_Out GetYIntrin128() const;
		Vec::Vector_4V_Out GetZIntrin128() const;
		Vec::Vector_4V_Out GetWIntrin128() const;

		void Print(bool newline=true);

	private:
		Vec::Vector_4V x;
		Vec::Vector_4V y;
		Vec::Vector_4V z;
		Vec::Vector_4V w;
	};

} // namespace rage

#include "vecbool4v_soa.inl"


#endif // VECTORMATH_VECBOOL4V_SOA_H
