#ifndef VECTORMATH_SCALARV_H
#define VECTORMATH_SCALARV_H

#include "data/struct.h"
#include "vectormath.h"
#include "boolv.h"

// This vector always has 4 components of the same value, by convention!

namespace rage
{

	class ScalarV;

	//================================================
	// TYPES
	//================================================

	typedef ScalarV					ScalarV_Val;
	typedef ScalarV*				ScalarV_Ptr;
	typedef const ScalarV*			ScalarV_ConstPtr;
	typedef ScalarV&				ScalarV_Ref;
	typedef const ScalarV&			ScalarV_ConstRef;

#if __WIN32PC
	typedef ScalarV_ConstRef		ScalarV_In;
#else
	typedef const ScalarV_Val		ScalarV_In;
#endif
	typedef const ScalarV_Val		ScalarV_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef ScalarV_Ref				ScalarV_InOut;

	//================================================
	// SCALARV
	//================================================

#if __XENON && UNIQUE_VECTORIZED_TYPE
	__declspec(passinreg)
#endif
	class ScalarV
	{
		friend class Vec2V;
		friend class Vec3V;
		friend class Vec4V;
		friend class QuatV;
		friend class Mat33V;
		friend class Mat34V;
		friend class Mat44V;

	public:
		// PLEASE KEEP THESE DEFINED IN THE SAME ORDER AS THE ENUMS IN vectortypes.h !
		// vector constant generation syntax: ScalarV(V_ZERO)
		explicit ScalarV(eZEROInitializer);
		explicit ScalarV(eONEInitializer);
		explicit ScalarV(eTWOInitializer);
		explicit ScalarV(eTHREEInitializer);
		explicit ScalarV(eFOURInitializer);
		explicit ScalarV(eFIVEInitializer);
		explicit ScalarV(eSIXInitializer);
		explicit ScalarV(eSEVENInitializer);
		explicit ScalarV(eEIGHTInitializer);
		explicit ScalarV(eNINEInitializer);
		explicit ScalarV(eTENInitializer);
		explicit ScalarV(eELEVENInitializer);
		explicit ScalarV(eTWELVEInitializer);
		explicit ScalarV(eTHIRTEENInitializer);
		explicit ScalarV(eFOURTEENInitializer);
		explicit ScalarV(eFIFTEENInitializer);
		explicit ScalarV(eNEGONEInitializer);
		explicit ScalarV(eNEGTWOInitializer);
		explicit ScalarV(eNEGTHREEInitializer);
		explicit ScalarV(eNEGFOURInitializer);
		explicit ScalarV(eNEGFIVEInitializer);
		explicit ScalarV(eNEGSIXInitializer);
		explicit ScalarV(eNEGSEVENInitializer);
		explicit ScalarV(eNEGEIGHTInitializer);
		explicit ScalarV(eNEGNINEInitializer);
		explicit ScalarV(eNEGTENInitializer);
		explicit ScalarV(eNEGELEVENInitializer);
		explicit ScalarV(eNEGTWELVEInitializer);
		explicit ScalarV(eNEGTHIRTEENInitializer);
		explicit ScalarV(eNEGFOURTEENInitializer);
		explicit ScalarV(eNEGFIFTEENInitializer);
		explicit ScalarV(eNEGSIXTEENInitializer);

		explicit ScalarV(eNEG_FLT_MAXInitializer);
		explicit ScalarV(eFLT_MAXInitializer);
		explicit ScalarV(eFLT_MINInitializer);
		explicit ScalarV(eFLT_LARGE_2Initializer);
		explicit ScalarV(eFLT_LARGE_4Initializer);
		explicit ScalarV(eFLT_LARGE_6Initializer);
		explicit ScalarV(eFLT_LARGE_8Initializer);
		explicit ScalarV(eFLT_EPSILONInitializer);
		explicit ScalarV(eFLT_SMALL_6Initializer);
		explicit ScalarV(eFLT_SMALL_5Initializer);
		explicit ScalarV(eFLT_SMALL_4Initializer);
		explicit ScalarV(eFLT_SMALL_3Initializer);
		explicit ScalarV(eFLT_SMALL_2Initializer);
		explicit ScalarV(eFLT_SMALL_1Initializer);
		explicit ScalarV(eFLT_SMALL_12Initializer);
		explicit ScalarV(eONE_PLUS_EPSILONInitializer);
		explicit ScalarV(eONE_MINUS_FLT_SMALL_3Initializer);

		//explicit ScalarV(eZERO_WONEInitializer);
		//explicit ScalarV(eONE_WZEROInitializer);

		//explicit ScalarV(eX_AXIS_WONEInitializer);
		//explicit ScalarV(eY_AXIS_WONEInitializer);
		//explicit ScalarV(eZ_AXIS_WONEInitializer);

		//explicit ScalarV(eX_AXIS_WZEROInitializer);
		//explicit ScalarV(eY_AXIS_WZEROInitializer);
		//explicit ScalarV(eZ_AXIS_WZEROInitializer);

		//explicit ScalarV(eMASKXInitializer);
		//explicit ScalarV(eMASKYInitializer);
		//explicit ScalarV(eMASKZInitializer);
		//explicit ScalarV(eMASKWInitializer);
		//explicit ScalarV(eMASKXYInitializer);
		//explicit ScalarV(eMASKXZInitializer);
		//explicit ScalarV(eMASKXWInitializer);
		//explicit ScalarV(eMASKYZInitializer);
		//explicit ScalarV(eMASKYWInitializer);
		//explicit ScalarV(eMASKZWInitializer);
		//explicit ScalarV(eMASKYZWInitializer);
		//explicit ScalarV(eMASKXZWInitializer);
		//explicit ScalarV(eMASKXYWInitializer);
		//explicit ScalarV(eMASKXYZInitializer);
		//explicit ScalarV(eMASKXYZWInitializer);

		explicit ScalarV(eQUARTERInitializer);
		explicit ScalarV(eTHIRDInitializer);
		explicit ScalarV(eHALFInitializer);
		explicit ScalarV(eNEGHALFInitializer);
		explicit ScalarV(eINFInitializer);
		explicit ScalarV(eNEGINFInitializer);
		explicit ScalarV(eNANInitializer);
		explicit ScalarV(eLOG2_TO_LOG10Initializer);

		explicit ScalarV(eONE_OVER_1024Initializer);
		explicit ScalarV(eONE_OVER_PIInitializer);
		explicit ScalarV(eTWO_OVER_PIInitializer);
		explicit ScalarV(ePIInitializer);
		explicit ScalarV(eTWO_PIInitializer);
		explicit ScalarV(ePI_OVER_TWOInitializer);
		explicit ScalarV(eNEG_PIInitializer);
		explicit ScalarV(eNEG_PI_OVER_TWOInitializer);
		explicit ScalarV(eTO_DEGREESInitializer);
		explicit ScalarV(eTO_RADIANSInitializer);
		explicit ScalarV(eSQRT_TWOInitializer);
		explicit ScalarV(eONE_OVER_SQRT_TWOInitializer);
		explicit ScalarV(eSQRT_THREEInitializer);
		explicit ScalarV(eEInitializer);

		explicit ScalarV(eINT_1Initializer);
		explicit ScalarV(eINT_2Initializer);
		explicit ScalarV(eINT_3Initializer);
		explicit ScalarV(eINT_4Initializer);
		explicit ScalarV(eINT_5Initializer);
		explicit ScalarV(eINT_6Initializer);
		explicit ScalarV(eINT_7Initializer);
		explicit ScalarV(eINT_8Initializer);
		explicit ScalarV(eINT_9Initializer);
		explicit ScalarV(eINT_10Initializer);
		explicit ScalarV(eINT_11Initializer);
		explicit ScalarV(eINT_12Initializer);
		explicit ScalarV(eINT_13Initializer);
		explicit ScalarV(eINT_14Initializer);
		explicit ScalarV(eINT_15Initializer);

		explicit ScalarV(e7FFFFFFFInitializer);
		explicit ScalarV(e80000000Initializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		ScalarV(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(ScalarV);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(ScalarV);
			STRUCT_FIELD( v );
			STRUCT_END();
		}
#endif

		ScalarV();
		explicit ScalarV(const float&);
		explicit ScalarV(Vec::Vector_4V_In);

#if __WIN32PC
		ScalarV(ScalarV_ConstRef _v);
#endif

		// operator= cripples pass-by-register on Xenon as of XDK 7776.0!
#if !__XENON
		ScalarV_ConstRef operator=(ScalarV_ConstRef _v);
#endif

		void SetIntrin128(Vec::Vector_4V_In v);
		Vec::Vector_4V_Val GetIntrin128() const;
		Vec::Vector_4V_Ref GetIntrin128Ref();
		Vec::Vector_4V_ConstRef GetIntrin128ConstRef() const;
		BoolV_Out AsBoolV() const;

		float Getf() const;
		void Setf( float floatVal );
		void Set( const float& rFloatVal );

		int Geti() const;
		void Seti( int intVal );

		void ZeroComponents();

		//============================================================================
		// Comparison functions

		// Make sure the vector is still splatted.
		bool IsValid() const;

		//============================================================================
		// Output

		void Print(bool newline=true) const;
		void PrintHex(bool newline=true) const;

		//============================================================================
		// Operators

		// Logical
		BoolV_Out		operator==	(ScalarV_In b) const;
		BoolV_Out		operator!=	(ScalarV_In b) const;
		BoolV_Out		operator<	(ScalarV_In bigVector) const;
		BoolV_Out		operator<=	(ScalarV_In bigVector) const;
		BoolV_Out		operator>	(ScalarV_In smallVector) const;
		BoolV_Out		operator>=	(ScalarV_In smallVector) const;

		// Arithmetic
		ScalarV_Out		operator*	(ScalarV_In b) const;
		ScalarV_Out		operator/	(ScalarV_In b) const;
		ScalarV_Out		operator+	(ScalarV_In b) const;
		ScalarV_Out		operator-	(ScalarV_In b) const;
		void			operator*=	(ScalarV_In b);
		void			operator/=	(ScalarV_In b);
		void			operator+=	(ScalarV_In b);
		void			operator-=	(ScalarV_In b);
		ScalarV_Out		operator+	() const;
		ScalarV_Out		operator-	() const;

		// Bitwise
		ScalarV_Out		operator|	(ScalarV_In b) const;
		ScalarV_Out		operator&	(ScalarV_In b) const;
		ScalarV_Out		operator^	(ScalarV_In b) const;
		void			operator|=	(ScalarV_In b);
		void			operator&=	(ScalarV_In b);
		void			operator^=	(ScalarV_In b);
		ScalarV_Out		operator~	() const;

	private:
		Vec::Vector_4V v;
	}
#if __PPU
	__attribute__((vecreturn))
#endif
	;

} // namespace rage

#include "scalarv.inl"


#endif // VECTORMATH_SCALARV_H
