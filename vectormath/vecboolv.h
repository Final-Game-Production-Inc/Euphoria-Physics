#ifndef VECTORMATH_VECBOOLV_H
#define VECTORMATH_VECBOOLV_H

#include "data/struct.h"
#include "boolv.h"
#include "vectormath.h"

namespace rage
{

	class VecBoolV;

	//================================================
	// TYPES
	//================================================

	typedef VecBoolV			VecBoolV_Val;
	typedef VecBoolV*			VecBoolV_Ptr;
	typedef const VecBoolV*		VecBoolV_ConstPtr;
	typedef VecBoolV&			VecBoolV_Ref;
	typedef const VecBoolV&		VecBoolV_ConstRef;

#if __WIN32PC
	typedef VecBoolV_ConstRef	VecBoolV_In;
#else
	typedef const VecBoolV_Val	VecBoolV_In;
#endif
	typedef const VecBoolV_Val	VecBoolV_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef VecBoolV_Ref		VecBoolV_InOut;

	//================================================
	// VECBOOLV
	//================================================

#if __XENON && UNIQUE_VECTORIZED_TYPE
	__declspec(passinreg)
#endif
	class VecBoolV
	{
		friend class ScalarV;
		friend class Vec2V;
		friend class Vec3V;
		friend class Vec4V;
		friend class QuatV;
		friend class Mat33V;
		friend class Mat34V;
		friend class Mat44V;

	public:
		// vector constant generation syntax: VecBoolV( V_T_F_F_T )
		explicit VecBoolV(eZEROInitializer);
		explicit VecBoolV(eMASKWInitializer);
		explicit VecBoolV(eMASKZInitializer);
		explicit VecBoolV(eMASKZWInitializer);
		explicit VecBoolV(eMASKYInitializer);
		explicit VecBoolV(eMASKYWInitializer);
		explicit VecBoolV(eMASKYZInitializer);
		explicit VecBoolV(eMASKYZWInitializer);
		explicit VecBoolV(eMASKXInitializer);
		explicit VecBoolV(eMASKXWInitializer);
		explicit VecBoolV(eMASKXZInitializer);
		explicit VecBoolV(eMASKXZWInitializer);
		explicit VecBoolV(eMASKXYInitializer);
		explicit VecBoolV(eMASKXYWInitializer);
		explicit VecBoolV(eMASKXYZInitializer);
		explicit VecBoolV(eMASKXYZWInitializer);

		VecBoolV();
		explicit VecBoolV(Vec::Vector_4V_In);

		explicit VecBoolV(BoolV_In);

#if __WIN32PC
		VecBoolV(VecBoolV_ConstRef _v);
#endif

		// operator= cripples pass-by-register on Xenon as of XDK 7776.0!
#if !__XENON
		VecBoolV_ConstRef operator=(VecBoolV_ConstRef _v);
#endif		

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		VecBoolV(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(VecBoolV);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(VecBoolV);
			STRUCT_FIELD( v );
			STRUCT_END();
		}
#endif

		BoolV_Out GetX() const;
		BoolV_Out GetY() const;
		BoolV_Out GetZ() const;
		BoolV_Out GetW() const;
		void SetX( BoolV_In newX );
		void SetY( BoolV_In newY );
		void SetZ( BoolV_In newZ );
		void SetW( BoolV_In newW );

		//============================================================================
		// Permute operations

		template <u32 permX, u32 permY>
		VecBoolV_Out Get() const;

		template <u32 permX, u32 permY, u32 permZ>
		VecBoolV_Out Get() const;

		template <u32 permX, u32 permY, u32 permZ, u32 permW>
		VecBoolV_Out Get() const;

		template <u32 permX, u32 permY>
		void Set(VecBoolV_In b);

		template <u32 permX, u32 permY, u32 permZ>
		void Set(VecBoolV_In b);

		template <u32 permX, u32 permY, u32 permZ, u32 permW>
		void Set(VecBoolV_In b);

		//============================================================================
		// Operators

		// Logical
		VecBoolV_Out	operator==	(VecBoolV_In b) const;
		VecBoolV_Out	operator!=	(VecBoolV_In b) const;
		VecBoolV_Out	operator!	() const;

		// Bitwise
		VecBoolV_Out	operator|	(VecBoolV_In b) const;
		VecBoolV_Out	operator&	(VecBoolV_In b) const;
		VecBoolV_Out	operator^	(VecBoolV_In b) const;
		void			operator|=	(VecBoolV_In b);
		void			operator&=	(VecBoolV_In b);
		void			operator^=	(VecBoolV_In b);

		void SetIntrin128(Vec::Vector_4V_In v);
		Vec::Vector_4V GetIntrin128() const;
		Vec::Vector_4V_Ref GetIntrin128Ref();
		Vec::Vector_4V_ConstRef GetIntrin128ConstRef() const;

		void Print(bool newline=true) const;

	private:
		Vec::Vector_4V v;
	}
#if __PPU
	__attribute__((vecreturn))
#endif
	;

} // namespace rage

#include "vecboolv.inl"


#endif // VECTORMATH_VECBOOLV_H
