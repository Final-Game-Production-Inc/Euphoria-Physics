#ifndef VECTORMATH_VEC3V_H
#define VECTORMATH_VEC3V_H

#include "data/struct.h"
#include "vectormath.h"
#include "vec2v.h"
#include "scalarv.h"
#include "vecboolv.h"

namespace rage
{
	class Vec3V;

	//================================================
	// TYPES
	//================================================

	typedef Vec3V			Vec3V_Val;
	typedef Vec3V*			Vec3V_Ptr;
	typedef const Vec3V*	Vec3V_ConstPtr;
	typedef Vec3V&			Vec3V_Ref;
	typedef const Vec3V&	Vec3V_ConstRef;

	// Please use Vec::V3Param128 & Vec::V3Param128_After3Args instead of Vec3V_In, when writing your functions.
	// This is more performant.

#if __WIN32PC
	typedef Vec3V_ConstRef	Vec3V_In;
#else
	typedef const Vec3V_Val	Vec3V_In;
#endif
	typedef const Vec3V_Val	Vec3V_Out; // "const" to warn that this isn't a "return by ref" parameter
	typedef Vec3V_Ref		Vec3V_InOut;

// Use this to pass the vector as three floats into a function, e.g. Displayf("<%f, %f, %f>, VEC3V_ARGS(vec));
#define VEC3V_ARGS(v) (v).GetXf(),(v).GetYf(),(v).GetZf()

	//================================================
	// VEC3V
	//================================================

#if __XENON && UNIQUE_VECTORIZED_TYPE
	__declspec(passinreg)
#endif
	class Vec3V
	{
		friend class Vec4V;
		friend class QuatV;
		friend class Mat33V;
		friend class Mat34V;
		friend class Mat44V;

	public:
		// PLEASE KEEP THESE DEFINED IN THE SAME ORDER AS THE ENUMS IN vectortypes.h !
		// vector constant generation syntax: Vec3V(V_ZERO)
		explicit Vec3V(eZEROInitializer);
		explicit Vec3V(eONEInitializer);
		explicit Vec3V(eTWOInitializer);
		explicit Vec3V(eTHREEInitializer);
		explicit Vec3V(eFOURInitializer);
		explicit Vec3V(eFIVEInitializer);
		explicit Vec3V(eSIXInitializer);
		explicit Vec3V(eSEVENInitializer);
		explicit Vec3V(eEIGHTInitializer);
		explicit Vec3V(eNINEInitializer);
		explicit Vec3V(eTENInitializer);
		explicit Vec3V(eELEVENInitializer);
		explicit Vec3V(eTWELVEInitializer);
		explicit Vec3V(eTHIRTEENInitializer);
		explicit Vec3V(eFOURTEENInitializer);
		explicit Vec3V(eFIFTEENInitializer);
		explicit Vec3V(eNEGONEInitializer);
		explicit Vec3V(eNEGTWOInitializer);
		explicit Vec3V(eNEGTHREEInitializer);
		explicit Vec3V(eNEGFOURInitializer);
		explicit Vec3V(eNEGFIVEInitializer);
		explicit Vec3V(eNEGSIXInitializer);
		explicit Vec3V(eNEGSEVENInitializer);
		explicit Vec3V(eNEGEIGHTInitializer);
		explicit Vec3V(eNEGNINEInitializer);
		explicit Vec3V(eNEGTENInitializer);
		explicit Vec3V(eNEGELEVENInitializer);
		explicit Vec3V(eNEGTWELVEInitializer);
		explicit Vec3V(eNEGTHIRTEENInitializer);
		explicit Vec3V(eNEGFOURTEENInitializer);
		explicit Vec3V(eNEGFIFTEENInitializer);
		explicit Vec3V(eNEGSIXTEENInitializer);

		explicit Vec3V(eNEG_FLT_MAXInitializer);
		explicit Vec3V(eFLT_MAXInitializer);
		explicit Vec3V(eFLT_MINInitializer);
		explicit Vec3V(eFLT_LARGE_2Initializer);
		explicit Vec3V(eFLT_LARGE_4Initializer);
		explicit Vec3V(eFLT_LARGE_6Initializer);
		explicit Vec3V(eFLT_LARGE_8Initializer);
		explicit Vec3V(eFLT_EPSILONInitializer);
		explicit Vec3V(eFLT_SMALL_6Initializer);
		explicit Vec3V(eFLT_SMALL_5Initializer);
		explicit Vec3V(eFLT_SMALL_4Initializer);
		explicit Vec3V(eFLT_SMALL_3Initializer);
		explicit Vec3V(eFLT_SMALL_2Initializer);
		explicit Vec3V(eFLT_SMALL_1Initializer);
		explicit Vec3V(eFLT_SMALL_12Initializer);
		explicit Vec3V(eONE_PLUS_EPSILONInitializer);
		explicit Vec3V(eONE_MINUS_FLT_SMALL_3Initializer);

		explicit Vec3V(eZERO_WONEInitializer);
		explicit Vec3V(eONE_WZEROInitializer);

		explicit Vec3V(eX_AXIS_WONEInitializer);
		explicit Vec3V(eY_AXIS_WONEInitializer);
		explicit Vec3V(eZ_AXIS_WONEInitializer);

		explicit Vec3V(eX_AXIS_WZEROInitializer);
		explicit Vec3V(eY_AXIS_WZEROInitializer);
		explicit Vec3V(eZ_AXIS_WZEROInitializer);

		explicit Vec3V(eMASKXInitializer);
		explicit Vec3V(eMASKYInitializer);
		explicit Vec3V(eMASKZInitializer);
		explicit Vec3V(eMASKWInitializer);
		explicit Vec3V(eMASKXYInitializer);
		explicit Vec3V(eMASKXZInitializer);
		explicit Vec3V(eMASKXWInitializer);
		explicit Vec3V(eMASKYZInitializer);
		explicit Vec3V(eMASKYWInitializer);
		explicit Vec3V(eMASKZWInitializer);
		explicit Vec3V(eMASKYZWInitializer);
		explicit Vec3V(eMASKXZWInitializer);
		explicit Vec3V(eMASKXYWInitializer);
		explicit Vec3V(eMASKXYZInitializer);
		explicit Vec3V(eMASKXYZWInitializer);

		explicit Vec3V(eQUARTERInitializer);
		explicit Vec3V(eTHIRDInitializer);
		explicit Vec3V(eHALFInitializer);
		explicit Vec3V(eNEGHALFInitializer);
		explicit Vec3V(eINFInitializer);
		explicit Vec3V(eNEGINFInitializer);
		explicit Vec3V(eNANInitializer);
		explicit Vec3V(eLOG2_TO_LOG10Initializer);

		explicit Vec3V(eONE_OVER_1024Initializer);
		explicit Vec3V(eONE_OVER_PIInitializer);
		explicit Vec3V(eTWO_OVER_PIInitializer);
		explicit Vec3V(ePIInitializer);
		explicit Vec3V(eTWO_PIInitializer);
		explicit Vec3V(ePI_OVER_TWOInitializer);
		explicit Vec3V(eNEG_PIInitializer);
		explicit Vec3V(eNEG_PI_OVER_TWOInitializer);
		explicit Vec3V(eTO_DEGREESInitializer);
		explicit Vec3V(eTO_RADIANSInitializer);
		explicit Vec3V(eSQRT_TWOInitializer);
		explicit Vec3V(eONE_OVER_SQRT_TWOInitializer);
		explicit Vec3V(eSQRT_THREEInitializer);
		explicit Vec3V(eEInitializer);

		explicit Vec3V(eINT_1Initializer);
		explicit Vec3V(eINT_2Initializer);
		explicit Vec3V(eINT_3Initializer);
		explicit Vec3V(eINT_4Initializer);
		explicit Vec3V(eINT_5Initializer);
		explicit Vec3V(eINT_6Initializer);
		explicit Vec3V(eINT_7Initializer);
		explicit Vec3V(eINT_8Initializer);
		explicit Vec3V(eINT_9Initializer);
		explicit Vec3V(eINT_10Initializer);
		explicit Vec3V(eINT_11Initializer);
		explicit Vec3V(eINT_12Initializer);
		explicit Vec3V(eINT_13Initializer);
		explicit Vec3V(eINT_14Initializer);
		explicit Vec3V(eINT_15Initializer);

		explicit Vec3V(e7FFFFFFFInitializer);
		explicit Vec3V(e80000000Initializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		Vec3V(class datResource&) {}

#if !UNIQUE_VECTORIZED_TYPE
		// PURPOSE: For scalar fallback.
		Vec3V(const Vec::Vector_4V_Persistent& v);
#endif // !UNIQUE_VECTORIZED_TYPE

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(Vec3V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Vec3V);
			STRUCT_FIELD( v );
			STRUCT_END();
		}
#endif

		Vec3V();
		explicit Vec3V(const float&,const float&,const float&);
		explicit Vec3V(ScalarV_In);
		explicit Vec3V(ScalarV_In,ScalarV_In,ScalarV_In);
		explicit Vec3V(Vec2V_In,ScalarV_In);
		explicit Vec3V(ScalarV_In,Vec2V_In);
		explicit Vec3V(Vec::Vector_4V_In);
		explicit Vec3V(VecBoolV_In);
		explicit Vec3V(BoolV_In);
	private:
		explicit Vec3V(int,int,int); // DO NOT USE
	public:

#if __WIN32PC
		Vec3V(Vec3V_ConstRef _v);
#endif

		// operator= cripples pass-by-register on Xenon as of XDK 7776.0!
#if !__XENON
		Vec3V_ConstRef operator=(Vec3V_ConstRef _v);
#endif

		void SetIntrin128(Vec::Vector_4V_In v);
		Vec::Vector_4V_Val GetIntrin128() const;
		Vec::Vector_4V_Ref GetIntrin128Ref();
		Vec::Vector_4V_ConstRef GetIntrin128ConstRef() const;
		VecBoolV_Out AsVecBoolV() const;
		ScalarV_Out AsScalarV() const; // Be careful of using this. Funcs like 'IsLessThan(ScalarV, ScalarV)' assume that x=y=z=w, and can give wrong results!!

		ScalarV_Out GetX() const; // All-vector-pipeline.
		ScalarV_Out GetY() const; //
		ScalarV_Out GetZ() const; //
		Vec2V_Out GetXY() const;
		void SetX( ScalarV_In newX ); // All-vector-pipeline.
		void SetY( ScalarV_In newY ); //
		void SetZ( ScalarV_In newZ ); //

		void SetXInMemory( ScalarV_In newX ); // All-vector-pipeline, use when '*this* is in memory.
		void SetYInMemory( ScalarV_In newY );
		void SetZInMemory( ScalarV_In newZ );

		void SetX( const float& floatVal ); // All-vector-pipeline, as long as 'floatVal' is in memory.
		void SetY( const float& floatVal );
		void SetZ( const float& floatVal );

		float GetXf() const; // All-float-pipeline, as long as '*this' is in memory.
		float GetYf() const; //
		float GetZf() const; //
		float GetElemf( unsigned elem ) const;
		void SetElemf( unsigned elem, float fVal );
		void SetXf( float floatVal ); // All-float-pipeline, as long as '*this' is in memory.
		void SetYf( float floatVal ); //
		void SetZf( float floatVal ); //

		int GetXi() const; // All-int-pipeline, as long as '*this' is in memory.
		int GetYi() const; //
		int GetZi() const; //
		int GetElemi( unsigned elem ) const;
		void SetElemi( unsigned elem, int intVal );
		void SetXi( int intVal ); // All-int-pipeline, as long as '*this' is in memory.
		void SetYi( int intVal ); //
		void SetZi( int intVal ); //

		void ZeroComponents();

		// Although W is undefined, you're still allowed to stuff something in there explicitly,
		// for memory storage's sake.
		void SetW( ScalarV_In newW );
		void SetWInMemory( ScalarV_In newW );
		void SetW( const float& floatVal );
		void SetWf( float floatVal );
		void SetWi( int intVal );
		ScalarV_Out GetW() const;
		float GetWf() const;
		int GetWi() const;

		void SetWZero();

		//============================================================================
		// Output

#if !__NO_OUTPUT
		void Print(bool newline=true) const;
		void Print(const char * label, bool newline=true) const;
		void PrintHex(bool newline=true) const;
#endif

		//============================================================================
		// Permute operations

		template <u32 permX, u32 permY>
		Vec2V_Out Get() const;

		template <u32 permX, u32 permY, u32 permZ>
		Vec3V_Out Get() const;

		template <u32 permX, u32 permY>
		void Set(Vec2V_In b);

		template <u32 permX, u32 permY, u32 permZ>
		void Set(Vec3V_In b);

		//============================================================================
		// Operators

		// Logical
		VecBoolV_Out	operator==	(Vec3V_In b) const;
		VecBoolV_Out	operator!=	(Vec3V_In b) const;
		VecBoolV_Out	operator<	(Vec3V_In bigVector) const;
		VecBoolV_Out	operator<=	(Vec3V_In bigVector) const;
		VecBoolV_Out	operator>	(Vec3V_In smallVector) const;
		VecBoolV_Out	operator>=	(Vec3V_In smallVector) const;

		// Arithmetic
		Vec3V_Out		operator*	(Vec3V_In b) const; // per-element multiply!
		Vec3V_Out		operator*	(ScalarV_In b) const;
		friend Vec3V_Out operator*	(ScalarV_In a, Vec3V_In b);
		Vec3V_Out		operator/	(Vec3V_In b) const; // per-element divide!
		Vec3V_Out		operator/	(ScalarV_In b) const;
		Vec3V_Out		operator+	(Vec3V_In b) const;
		Vec3V_Out		operator-	(Vec3V_In b) const;
		void			operator*=	(Vec3V_In b);
		void			operator*=	(ScalarV_In b);
		void			operator/=	(Vec3V_In b);
		void			operator/=	(ScalarV_In b);
		void			operator+=	(Vec3V_In b);
		void			operator-=	(Vec3V_In b);
		Vec3V_Out		operator+	() const;
		Vec3V_Out		operator-	() const;

		// Bitwise
		Vec3V_Out		operator|	(Vec3V_In b) const;
		Vec3V_Out		operator&	(Vec3V_In b) const;
		Vec3V_Out		operator^	(Vec3V_In b) const;
		void			operator|=	(Vec3V_In b);
		void			operator&=	(Vec3V_In b);
		void			operator^=	(Vec3V_In b);
		Vec3V_Out		operator~	() const;

		// Element access. Warning: Expensive operations.
		const float&	operator[]	(unsigned elem) const;
		float&			operator[]	(unsigned elem);

	public:
		Vec::Vector_4V v;
	}
#if __PPU
	__attribute__((vecreturn))
#endif
	;

} // namespace rage

#include "vec3v.inl"


#endif // VECTORMATH_VEC3V_H
