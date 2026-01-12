#ifndef VECTORMATH_VEC2V_H
#define VECTORMATH_VEC2V_H

#include "data/struct.h"
#include "vectormath.h"
#include "scalarv.h"
#include "vecboolv.h"

namespace rage
{
	class Vec2V;

	//================================================
	// TYPES
	//================================================

	typedef Vec2V			Vec2V_Val;
	typedef Vec2V*			Vec2V_Ptr;
	typedef const Vec2V*	Vec2V_ConstPtr;
	typedef Vec2V&			Vec2V_Ref;
	typedef const Vec2V&	Vec2V_ConstRef;

#if __WIN32PC
	typedef Vec2V_ConstRef	Vec2V_In;
#else
	typedef const Vec2V_Val	Vec2V_In;
#endif
	typedef const Vec2V_Val	Vec2V_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef Vec2V_Ref		Vec2V_InOut;

// Use this to pass the vector as two floats into a function, e.g. Displayf("<%f, %f>, VEC2V_ARGS(vec));
#define VEC2V_ARGS(v) (v).GetXf(),(v).GetYf()

	//================================================
	// VEC2V
	//================================================

#if __XENON && UNIQUE_VECTORIZED_TYPE
	__declspec(passinreg)
#endif
	class Vec2V
	{
		friend class Vec3V;
		friend class Vec4V;
		friend class QuatV;
		friend class Mat33V;
		friend class Mat34V;
		friend class Mat44V;

	public:
		// PLEASE KEEP THESE DEFINED IN THE SAME ORDER AS THE ENUMS IN vectortypes.h !
		// vector constant generation syntax: Vec2V(V_ZERO)
		explicit Vec2V(eZEROInitializer);
		explicit Vec2V(eONEInitializer);
		explicit Vec2V(eTWOInitializer);
		explicit Vec2V(eTHREEInitializer);
		explicit Vec2V(eFOURInitializer);
		explicit Vec2V(eFIVEInitializer);
		explicit Vec2V(eSIXInitializer);
		explicit Vec2V(eSEVENInitializer);
		explicit Vec2V(eEIGHTInitializer);
		explicit Vec2V(eNINEInitializer);
		explicit Vec2V(eTENInitializer);
		explicit Vec2V(eELEVENInitializer);
		explicit Vec2V(eTWELVEInitializer);
		explicit Vec2V(eTHIRTEENInitializer);
		explicit Vec2V(eFOURTEENInitializer);
		explicit Vec2V(eFIFTEENInitializer);
		explicit Vec2V(eNEGONEInitializer);
		explicit Vec2V(eNEGTWOInitializer);
		explicit Vec2V(eNEGTHREEInitializer);
		explicit Vec2V(eNEGFOURInitializer);
		explicit Vec2V(eNEGFIVEInitializer);
		explicit Vec2V(eNEGSIXInitializer);
		explicit Vec2V(eNEGSEVENInitializer);
		explicit Vec2V(eNEGEIGHTInitializer);
		explicit Vec2V(eNEGNINEInitializer);
		explicit Vec2V(eNEGTENInitializer);
		explicit Vec2V(eNEGELEVENInitializer);
		explicit Vec2V(eNEGTWELVEInitializer);
		explicit Vec2V(eNEGTHIRTEENInitializer);
		explicit Vec2V(eNEGFOURTEENInitializer);
		explicit Vec2V(eNEGFIFTEENInitializer);
		explicit Vec2V(eNEGSIXTEENInitializer);

		explicit Vec2V(eNEG_FLT_MAXInitializer);
		explicit Vec2V(eFLT_MAXInitializer);
		explicit Vec2V(eFLT_MINInitializer);
		explicit Vec2V(eFLT_LARGE_2Initializer);
		explicit Vec2V(eFLT_LARGE_4Initializer);
		explicit Vec2V(eFLT_LARGE_6Initializer);
		explicit Vec2V(eFLT_LARGE_8Initializer);
		explicit Vec2V(eFLT_EPSILONInitializer);
		explicit Vec2V(eFLT_SMALL_6Initializer);
		explicit Vec2V(eFLT_SMALL_5Initializer);
		explicit Vec2V(eFLT_SMALL_4Initializer);
		explicit Vec2V(eFLT_SMALL_3Initializer);
		explicit Vec2V(eFLT_SMALL_2Initializer);
		explicit Vec2V(eFLT_SMALL_1Initializer);
		explicit Vec2V(eFLT_SMALL_12Initializer);
		explicit Vec2V(eONE_PLUS_EPSILONInitializer);
		explicit Vec2V(eONE_MINUS_FLT_SMALL_3Initializer);

		explicit Vec2V(eZERO_WONEInitializer);
		explicit Vec2V(eONE_WZEROInitializer);

		explicit Vec2V(eX_AXIS_WONEInitializer);
		explicit Vec2V(eY_AXIS_WONEInitializer);

		explicit Vec2V(eZ_AXIS_WONEInitializer);
		explicit Vec2V(eX_AXIS_WZEROInitializer);
		explicit Vec2V(eY_AXIS_WZEROInitializer);
		explicit Vec2V(eZ_AXIS_WZEROInitializer);

		explicit Vec2V(eMASKXInitializer);
		explicit Vec2V(eMASKYInitializer);
		explicit Vec2V(eMASKZInitializer);
		explicit Vec2V(eMASKWInitializer);
		explicit Vec2V(eMASKXYInitializer);
		explicit Vec2V(eMASKXZInitializer);
		explicit Vec2V(eMASKXWInitializer);
		explicit Vec2V(eMASKYZInitializer);
		explicit Vec2V(eMASKYWInitializer);
		explicit Vec2V(eMASKZWInitializer);
		explicit Vec2V(eMASKYZWInitializer);
		explicit Vec2V(eMASKXZWInitializer);
		explicit Vec2V(eMASKXYWInitializer);
		explicit Vec2V(eMASKXYZInitializer);
		explicit Vec2V(eMASKXYZWInitializer);

		explicit Vec2V(eQUARTERInitializer);
		explicit Vec2V(eTHIRDInitializer);
		explicit Vec2V(eHALFInitializer);
		explicit Vec2V(eNEGHALFInitializer);
		explicit Vec2V(eINFInitializer);
		explicit Vec2V(eNEGINFInitializer);
		explicit Vec2V(eNANInitializer);
		explicit Vec2V(eLOG2_TO_LOG10Initializer);

		explicit Vec2V(eONE_OVER_1024Initializer);
		explicit Vec2V(eONE_OVER_PIInitializer);
		explicit Vec2V(eTWO_OVER_PIInitializer);
		explicit Vec2V(ePIInitializer);
		explicit Vec2V(eTWO_PIInitializer);
		explicit Vec2V(ePI_OVER_TWOInitializer);
		explicit Vec2V(eNEG_PIInitializer);
		explicit Vec2V(eNEG_PI_OVER_TWOInitializer);
		explicit Vec2V(eTO_DEGREESInitializer);
		explicit Vec2V(eTO_RADIANSInitializer);
		explicit Vec2V(eSQRT_TWOInitializer);
		explicit Vec2V(eONE_OVER_SQRT_TWOInitializer);
		explicit Vec2V(eSQRT_THREEInitializer);
		explicit Vec2V(eEInitializer);

		explicit Vec2V(eINT_1Initializer);
		explicit Vec2V(eINT_2Initializer);
		explicit Vec2V(eINT_3Initializer);
		explicit Vec2V(eINT_4Initializer);
		explicit Vec2V(eINT_5Initializer);
		explicit Vec2V(eINT_6Initializer);
		explicit Vec2V(eINT_7Initializer);
		explicit Vec2V(eINT_8Initializer);
		explicit Vec2V(eINT_9Initializer);
		explicit Vec2V(eINT_10Initializer);
		explicit Vec2V(eINT_11Initializer);
		explicit Vec2V(eINT_12Initializer);
		explicit Vec2V(eINT_13Initializer);
		explicit Vec2V(eINT_14Initializer);
		explicit Vec2V(eINT_15Initializer);

		explicit Vec2V(e7FFFFFFFInitializer);
		explicit Vec2V(e80000000Initializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		Vec2V(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(Vec2V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Vec2V);
			STRUCT_FIELD( v );
			STRUCT_END();
		}
#endif

		Vec2V();
		explicit Vec2V(const float&,const float&);
		explicit Vec2V(ScalarV_In);
		explicit Vec2V(ScalarV_In,ScalarV_In);
		explicit Vec2V(Vec::Vector_4V_In);
		explicit Vec2V(VecBoolV_In);
		explicit Vec2V(BoolV_In);
	private:
		explicit Vec2V(int,int); // DO NOT USE
	public:

#if __WIN32PC
		Vec2V(Vec2V_ConstRef _v);
#endif

		// operator= cripples pass-by-register on Xenon as of XDK 7776.0!
#if !__XENON
		Vec2V_ConstRef operator=(Vec2V_ConstRef _v);
#endif

		void SetIntrin128(Vec::Vector_4V_In v);
		Vec::Vector_4V_Val GetIntrin128() const;
		Vec::Vector_4V_Ref GetIntrin128Ref();
		Vec::Vector_4V_ConstRef GetIntrin128ConstRef() const;
		VecBoolV_Out AsVecBoolV() const;

		ScalarV_Out GetX() const;
		ScalarV_Out GetY() const;
		void SetX( ScalarV_In newX );
		void SetY( ScalarV_In newY );
		void SetXInMemory( ScalarV_In newX );
		void SetYInMemory( ScalarV_In newY );
		void SetX( const float& floatVal );
		void SetY( const float& floatVal );

		float GetXf() const;
		float GetYf() const;
		float GetElemf( unsigned elem ) const;
		void SetElemf( unsigned elem, float fVal );
		void SetXf( float floatVal );
		void SetYf( float floatVal );

		int GetXi() const;
		int GetYi() const;
		int GetElemi( unsigned elem ) const;
		void SetElemi( unsigned elem, int intVal );
		void SetXi( int intVal );
		void SetYi( int intVal );

		void ZeroComponents();

		//============================================================================
		// Output

#if !__NO_OUTPUT
		void Print(bool newline=true) const;
		void PrintHex(bool newline=true) const;
#endif

		//============================================================================
		// Permute operations

		template <u32 permX, u32 permY>
		Vec2V_Out Get() const;

		//============================================================================
		// Operators

		// Logical
		VecBoolV_Out	operator==	(Vec2V_In b) const;
		VecBoolV_Out	operator!=	(Vec2V_In b) const;
		VecBoolV_Out	operator<	(Vec2V_In bigVector) const;
		VecBoolV_Out	operator<=	(Vec2V_In bigVector) const;
		VecBoolV_Out	operator>	(Vec2V_In smallVector) const;
		VecBoolV_Out	operator>=	(Vec2V_In smallVector) const;

		// Arithmetic
		Vec2V_Out		operator*	(Vec2V_In b) const; // per-element multiply!
		Vec2V_Out		operator*	(ScalarV_In b) const;
		friend Vec2V_Out operator*	(ScalarV_In a, Vec2V_In b);
		Vec2V_Out		operator/	(Vec2V_In b) const; // per-element divide!
		Vec2V_Out		operator/	(ScalarV_In b) const;
		Vec2V_Out		operator+	(Vec2V_In b) const;
		Vec2V_Out		operator-	(Vec2V_In b) const;
		void			operator*=	(Vec2V_In b);
		void			operator*=	(ScalarV_In b);
		void			operator/=	(Vec2V_In b);
		void			operator/=	(ScalarV_In b);
		void			operator+=	(Vec2V_In b);
		void			operator-=	(Vec2V_In b);
		Vec2V_Out		operator+	() const;
		Vec2V_Out		operator-	() const;

		// Bitwise
		Vec2V_Out		operator|	(Vec2V_In b) const;
		Vec2V_Out		operator&	(Vec2V_In b) const;
		Vec2V_Out		operator^	(Vec2V_In b) const;
		void			operator|=	(Vec2V_In b);
		void			operator&=	(Vec2V_In b);
		void			operator^=	(Vec2V_In b);
		Vec2V_Out		operator~	() const;

		// Element access.
		// Warning: Expensive operations.
		const float&	operator[]	(u32 elem) const;
		float&			operator[]	(u32 elem);

	private:
		Vec::Vector_4V v;
	}
#if __PPU
	__attribute__((vecreturn))
#endif
	;

} // namespace rage

#include "vec2v.inl"


#endif // VECTORMATH_VEC2V_H
