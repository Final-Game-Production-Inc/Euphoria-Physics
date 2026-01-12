#ifndef VECTORMATH_BOOLV_H
#define VECTORMATH_BOOLV_H

#include "data/struct.h"
#include "vectormath.h"

namespace rage
{

	class BoolV;

	//================================================
	// TYPES
	//================================================

	typedef BoolV			BoolV_Val;
	typedef BoolV*			BoolV_Ptr;
	typedef const BoolV*	BoolV_ConstPtr;
	typedef BoolV&			BoolV_Ref;
	typedef const BoolV&	BoolV_ConstRef;

#if __WIN32PC
	typedef BoolV_ConstRef	BoolV_In;
#else
	typedef const BoolV_Val	BoolV_In;
#endif
	typedef const BoolV_Val	BoolV_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef BoolV_Ref		BoolV_InOut;

	//================================================
	// VECBOOLV
	//================================================

#if __XENON && UNIQUE_VECTORIZED_TYPE
	__declspec(passinreg)
#endif
	class BoolV
	{
		friend class ScalarV;
		friend class Vec2V;
		friend class Vec3V;
		friend class Vec4V;
		friend class QuatV;
		friend class Mat33V;
		friend class Mat34V;
		friend class Mat44V;
		friend class VecBoolV;

	public:

	public:
		explicit BoolV(eZEROInitializer);		// False. e.g.: BoolV(V_F)
		explicit BoolV(eMASKXYZWInitializer);	// True. e.g.: BoolV(V_T)

		BoolV();
		explicit BoolV(bool);
		explicit BoolV(Vec::Vector_4V_In);

#if __WIN32PC
		BoolV(BoolV_ConstRef _v);
#endif

		// operator= cripples pass-by-register on Xenon as of XDK 7776.0!
#if !__XENON
		BoolV_ConstRef operator=(BoolV_ConstRef _v);
#endif		

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		BoolV(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(BoolV);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(BoolV);
			STRUCT_FIELD( v );
			STRUCT_END();

		}
#endif

		bool Getb() const;

		//============================================================================
		// Operators

		// Logical
		BoolV_Out operator==	(BoolV_In b) const;
		BoolV_Out operator!=	(BoolV_In b) const;
		BoolV_Out operator!		() const;

		// Bitwise
		BoolV_Out	operator|	(BoolV_In b) const;
		BoolV_Out	operator&	(BoolV_In b) const;
		BoolV_Out	operator^	(BoolV_In b) const;
		void		operator|=	(BoolV_In b);
		void		operator&=	(BoolV_In b);
		void		operator^=	(BoolV_In b);

		void SetIntrin128(Vec::Vector_4V_In v);
		Vec::Vector_4V GetIntrin128() const;
		Vec::Vector_4V_Ref GetIntrin128Ref();
		Vec::Vector_4V_ConstRef GetIntrin128ConstRef() const;

		void Print(bool newline=true);

	private:
		Vec::Vector_4V v;
	}
#if __PPU
	__attribute__((vecreturn))
#endif
	;

} // namespace rage

#include "boolv.inl"


#endif // VECTORMATH_BOOLV_H
