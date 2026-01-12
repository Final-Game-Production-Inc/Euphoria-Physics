#ifndef VECTORMATH_VECBOOL3V_SOA_H
#define VECTORMATH_VECBOOL3V_SOA_H

#include "data/struct.h"
#include "vectormath.h"

namespace rage
{

	class SoA_VecBool3V;

	//================================================
	// TYPES
	//================================================

	typedef SoA_VecBool3V			SoA_VecBool3V_Val;
	typedef SoA_VecBool3V*			SoA_VecBool3V_Ptr;
	typedef SoA_VecBool3V&			SoA_VecBool3V_Ref;
	typedef const SoA_VecBool3V&	SoA_VecBool3V_ConstRef;

	typedef SoA_VecBool3V_ConstRef	SoA_VecBool3V_In;
	typedef const SoA_VecBool3V_Val	SoA_VecBool3V_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef SoA_VecBool3V_Ref		SoA_VecBool3V_InOut;

#define VECBOOL3V_SOA_DECL(m)			rage::Vec::Vector_4V_In m##_x,rage::Vec::Vector_4V_In m##_y,rage::Vec::Vector_4V_In m##_z
#define VECBOOL3V_SOA_DECL2(m)			rage::Vec::Vector_4V_In_After3Args m##_x,rage::Vec::Vector_4V_In_After3Args m##_y,rage::Vec::Vector_4V_In_After3Args m##_z
#define VECBOOL3V_SOA_ARG(m)			(m).GetXIntrin128(),(m).GetYIntrin128(),(m).GetZIntrin128()
#define VECBOOL3V_SOA_ARG_GET(m)		SoA_VecBool3V( m##_x,m##_y,m##_z )

	//================================================
	// VECBOOL3V
	//================================================

	class SoA_VecBool3V
	{
	public:

		enum eF_F_FInitializer {	F_F_F = 0,	};
		enum eF_F_TInitializer {	F_F_T = 0,	};
		enum eF_T_FInitializer {	F_T_F = 0,	};
		enum eF_T_TInitializer {	F_T_T = 0,	};
		enum eT_F_FInitializer {	T_F_F = 0,	};
		enum eT_F_TInitializer {	T_F_T = 0,	};
		enum eT_T_FInitializer {	T_T_F = 0,	};
		enum eT_T_TInitializer {	T_T_T = 0,	};

	public:
		SoA_VecBool3V();
		// Note: This copy constructor must be defined in order to avoid a million unnecessary lvx/stvx calls when returning matrices by value!
		// https://ps3.scedev.net/forums/thread/24368
		SoA_VecBool3V(SoA_VecBool3V_ConstRef);
		SoA_VecBool3V_Ref operator= (SoA_VecBool3V_ConstRef);

		explicit SoA_VecBool3V(eF_F_FInitializer);
		explicit SoA_VecBool3V(eF_F_TInitializer);
		explicit SoA_VecBool3V(eF_T_FInitializer);
		explicit SoA_VecBool3V(eF_T_TInitializer);
		explicit SoA_VecBool3V(eT_F_FInitializer);
		explicit SoA_VecBool3V(eT_F_TInitializer);
		explicit SoA_VecBool3V(eT_T_FInitializer);
		explicit SoA_VecBool3V(eT_T_TInitializer);

		explicit SoA_VecBool3V(Vec::Vector_4V_In v);
		explicit SoA_VecBool3V(Vec::Vector_4V_In x,Vec::Vector_4V_In y,Vec::Vector_4V_In z);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		SoA_VecBool3V(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(SoA_VecBool3V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(SoA_VecBool3V);
			STRUCT_FIELD( x );
			STRUCT_FIELD( y );
			STRUCT_FIELD( z );
			STRUCT_END();
		}
#endif

		//============================================================================
		// Operators

		// Logical
		SoA_VecBool3V_Out operator==	(SoA_VecBool3V_In b) const;
		SoA_VecBool3V_Out operator!=	(SoA_VecBool3V_In b) const;
		SoA_VecBool3V_Out operator! () const;

		// Bitwise
		SoA_VecBool3V_Out	operator|	(SoA_VecBool3V_In b) const;
		SoA_VecBool3V_Out	operator&	(SoA_VecBool3V_In b) const;
		SoA_VecBool3V_Out	operator^	(SoA_VecBool3V_In b) const;
		void				operator|=	(SoA_VecBool3V_In b);
		void				operator&=	(SoA_VecBool3V_In b);
		void				operator^=	(SoA_VecBool3V_In b);

		void SetXIntrin128(Vec::Vector_4V_In x);
		void SetYIntrin128(Vec::Vector_4V_In y);
		void SetZIntrin128(Vec::Vector_4V_In z);
		Vec::Vector_4V_Out GetXIntrin128() const;
		Vec::Vector_4V_Out GetYIntrin128() const;
		Vec::Vector_4V_Out GetZIntrin128() const;

		void Print(bool newline=true);

	private:
		Vec::Vector_4V x;
		Vec::Vector_4V y;
		Vec::Vector_4V z;
	};

} // namespace rage

#include "vecbool3v_soa.inl"


#endif // VECTORMATH_VECBOOL3V_SOA_H
