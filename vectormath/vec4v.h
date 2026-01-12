#ifndef VECTORMATH_VEC4V_H
#define VECTORMATH_VEC4V_H

#include "data/struct.h"
#include "vectormath.h"
#include "quatv.h"
#include "vec3v.h"
#include "vec2v.h"
#include "scalarv.h"
#include "vecboolv.h"

#if !UNIQUE_VECTORIZED_TYPE
// To have scalar fallback work with old RAGE code/data structs.
#include "math/intrinsics.h"
#endif

namespace rage
{

	class Vec4V;

	//================================================
	// TYPES
	//================================================

	typedef Vec4V			Vec4V_Val;
	typedef Vec4V*			Vec4V_Ptr;
	typedef const Vec4V*	Vec4V_ConstPtr;
	typedef Vec4V&			Vec4V_Ref;
	typedef const Vec4V&	Vec4V_ConstRef;

#if __WIN32PC
	typedef Vec4V_ConstRef	Vec4V_In;
#else
	typedef const Vec4V_Val	Vec4V_In;
#endif
	typedef const Vec4V_Val	Vec4V_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef Vec4V_Ref		Vec4V_InOut;

// Use this to pass the vector as four floats into a function, e.g. Displayf("<%f, %f, %f, %f>, VEC4V_ARGS(vec));
#define VEC4V_ARGS(v) (v).GetXf(),(v).GetYf(),(v).GetZf(),(v).GetWf()

	//================================================
	// VEC4V
	//================================================

#if __XENON && UNIQUE_VECTORIZED_TYPE
	__declspec(passinreg)
#endif
	class Vec4V
	{

		friend class Mat33V;
		friend class Mat34V;
		friend class Mat44V;

	public:
		// PLEASE KEEP THESE DEFINED IN THE SAME ORDER AS THE ENUMS IN vectortypes.h !
		// vector constant generation syntax: Vec4V(V_ZERO)
		explicit Vec4V(eZEROInitializer);
		explicit Vec4V(eONEInitializer);
		explicit Vec4V(eTWOInitializer);
		explicit Vec4V(eTHREEInitializer);
		explicit Vec4V(eFOURInitializer);
		explicit Vec4V(eFIVEInitializer);
		explicit Vec4V(eSIXInitializer);
		explicit Vec4V(eSEVENInitializer);
		explicit Vec4V(eEIGHTInitializer);
		explicit Vec4V(eNINEInitializer);
		explicit Vec4V(eTENInitializer);
		explicit Vec4V(eELEVENInitializer);
		explicit Vec4V(eTWELVEInitializer);
		explicit Vec4V(eTHIRTEENInitializer);
		explicit Vec4V(eFOURTEENInitializer);
		explicit Vec4V(eFIFTEENInitializer);
		explicit Vec4V(eNEGONEInitializer);
		explicit Vec4V(eNEGTWOInitializer);
		explicit Vec4V(eNEGTHREEInitializer);
		explicit Vec4V(eNEGFOURInitializer);
		explicit Vec4V(eNEGFIVEInitializer);
		explicit Vec4V(eNEGSIXInitializer);
		explicit Vec4V(eNEGSEVENInitializer);
		explicit Vec4V(eNEGEIGHTInitializer);
		explicit Vec4V(eNEGNINEInitializer);
		explicit Vec4V(eNEGTENInitializer);
		explicit Vec4V(eNEGELEVENInitializer);
		explicit Vec4V(eNEGTWELVEInitializer);
		explicit Vec4V(eNEGTHIRTEENInitializer);
		explicit Vec4V(eNEGFOURTEENInitializer);
		explicit Vec4V(eNEGFIFTEENInitializer);
		explicit Vec4V(eNEGSIXTEENInitializer);

		explicit Vec4V(eNEG_FLT_MAXInitializer);
		explicit Vec4V(eFLT_MAXInitializer);
		explicit Vec4V(eFLT_MINInitializer);
		explicit Vec4V(eFLT_LARGE_2Initializer);
		explicit Vec4V(eFLT_LARGE_4Initializer);
		explicit Vec4V(eFLT_LARGE_6Initializer);
		explicit Vec4V(eFLT_LARGE_8Initializer);
		explicit Vec4V(eFLT_EPSILONInitializer);
		explicit Vec4V(eFLT_SMALL_6Initializer);
		explicit Vec4V(eFLT_SMALL_5Initializer);
		explicit Vec4V(eFLT_SMALL_4Initializer);
		explicit Vec4V(eFLT_SMALL_3Initializer);
		explicit Vec4V(eFLT_SMALL_2Initializer);
		explicit Vec4V(eFLT_SMALL_1Initializer);
		explicit Vec4V(eFLT_SMALL_12Initializer);
		explicit Vec4V(eONE_PLUS_EPSILONInitializer);
		explicit Vec4V(eONE_MINUS_FLT_SMALL_3Initializer);

		explicit Vec4V(eZERO_WONEInitializer);
		explicit Vec4V(eONE_WZEROInitializer);

		explicit Vec4V(eX_AXIS_WONEInitializer);
		explicit Vec4V(eY_AXIS_WONEInitializer);
		explicit Vec4V(eZ_AXIS_WONEInitializer);

		explicit Vec4V(eX_AXIS_WZEROInitializer);
		explicit Vec4V(eY_AXIS_WZEROInitializer);
		explicit Vec4V(eZ_AXIS_WZEROInitializer);

		explicit Vec4V(eMASKXInitializer);
		explicit Vec4V(eMASKYInitializer);
		explicit Vec4V(eMASKZInitializer);
		explicit Vec4V(eMASKWInitializer);
		explicit Vec4V(eMASKXYInitializer);
		explicit Vec4V(eMASKXZInitializer);
		explicit Vec4V(eMASKXWInitializer);
		explicit Vec4V(eMASKYZInitializer);
		explicit Vec4V(eMASKYWInitializer);
		explicit Vec4V(eMASKZWInitializer);
		explicit Vec4V(eMASKYZWInitializer);
		explicit Vec4V(eMASKXZWInitializer);
		explicit Vec4V(eMASKXYWInitializer);
		explicit Vec4V(eMASKXYZInitializer);
		explicit Vec4V(eMASKXYZWInitializer);

		explicit Vec4V(eQUARTERInitializer);
		explicit Vec4V(eTHIRDInitializer);
		explicit Vec4V(eHALFInitializer);
		explicit Vec4V(eNEGHALFInitializer);
		explicit Vec4V(eINFInitializer);
		explicit Vec4V(eNEGINFInitializer);
		explicit Vec4V(eNANInitializer);
		explicit Vec4V(eLOG2_TO_LOG10Initializer);

		explicit Vec4V(eONE_OVER_1024Initializer);
		explicit Vec4V(eONE_OVER_PIInitializer);
		explicit Vec4V(eTWO_OVER_PIInitializer);
		explicit Vec4V(ePIInitializer);
		explicit Vec4V(eTWO_PIInitializer);
		explicit Vec4V(ePI_OVER_TWOInitializer);
		explicit Vec4V(eNEG_PIInitializer);
		explicit Vec4V(eNEG_PI_OVER_TWOInitializer);
		explicit Vec4V(eTO_DEGREESInitializer);
		explicit Vec4V(eTO_RADIANSInitializer);
		explicit Vec4V(eSQRT_TWOInitializer);
		explicit Vec4V(eONE_OVER_SQRT_TWOInitializer);
		explicit Vec4V(eSQRT_THREEInitializer);
		explicit Vec4V(eEInitializer);

		explicit Vec4V(eINT_1Initializer);
		explicit Vec4V(eINT_2Initializer);
		explicit Vec4V(eINT_3Initializer);
		explicit Vec4V(eINT_4Initializer);
		explicit Vec4V(eINT_5Initializer);
		explicit Vec4V(eINT_6Initializer);
		explicit Vec4V(eINT_7Initializer);
		explicit Vec4V(eINT_8Initializer);
		explicit Vec4V(eINT_9Initializer);
		explicit Vec4V(eINT_10Initializer);
		explicit Vec4V(eINT_11Initializer);
		explicit Vec4V(eINT_12Initializer);
		explicit Vec4V(eINT_13Initializer);
		explicit Vec4V(eINT_14Initializer);
		explicit Vec4V(eINT_15Initializer);

		explicit Vec4V(e7FFFFFFFInitializer);
		explicit Vec4V(e80000000Initializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		Vec4V(class datResource&) {}

#if !UNIQUE_VECTORIZED_TYPE
		// PURPOSE: For scalar fallback.
		Vec4V(const Vec::Vector_4V_Persistent& v);
#endif // !UNIQUE_VECTORIZED_TYPE

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(Vec4V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Vec4V);
			STRUCT_FIELD( v );
			STRUCT_END();
		}
#endif

		Vec4V();
		explicit Vec4V(const float&,const float&,const float&,const float&);
		explicit Vec4V(Vec::Vector_4V_In);
		explicit Vec4V(VecBoolV_In);
		explicit Vec4V(BoolV_In);
		explicit Vec4V(ScalarV_In);
		explicit Vec4V(ScalarV_In,ScalarV_In,ScalarV_In,ScalarV_In);
		explicit Vec4V(ScalarV_In,ScalarV_In,Vec2V_In);
		explicit Vec4V(ScalarV_In,Vec2V_In,ScalarV_In);
		explicit Vec4V(Vec2V_In,ScalarV_In,ScalarV_In);
		explicit Vec4V(ScalarV_In,Vec3V_In);
		explicit Vec4V(Vec2V_In,Vec2V_In);
		explicit Vec4V(Vec3V_In,ScalarV_In);
		explicit Vec4V(Vec3V_In); // Leaves w undefined.
		explicit Vec4V(Vec2V_In); // Leaves z,w undefined.
		explicit Vec4V(QuatV_In);
	private:
		explicit Vec4V(int,int,int,int); // DO NOT USE
	public:

#if __WIN32PC
		Vec4V(Vec4V_ConstRef _v);
#endif

		// operator= cripples pass-by-register on Xenon as of XDK 7776.0!
#if !__XENON
		Vec4V_ConstRef operator=(Vec4V_ConstRef _v);
#endif

//#if !UNIQUE_VECTORIZED_TYPE
//		explicit Vec4V(const Vector4&);
//		explicit Vec4V(Vec::Vector_4V_Legacy);
//#endif // !UNIQUE_VECTORIZED_TYPE

		//============================================================================
		// Getters / setters

		void SetIntrin128(Vec::Vector_4V_In v);
		Vec::Vector_4V_Val GetIntrin128() const;
		Vec::Vector_4V_Ref GetIntrin128Ref();
		Vec::Vector_4V_ConstRef GetIntrin128ConstRef() const;
		VecBoolV_Out AsVecBoolV() const;
		ScalarV_Out AsScalarV() const;

		ScalarV_Out GetX() const; // All-vector-pipeline, use when '*this' is in registers.
		ScalarV_Out GetY() const;
		ScalarV_Out GetZ() const;
		ScalarV_Out GetW() const;
		Vec2V_Out GetXY() const;
		Vec2V_Out GetZW() const;
		Vec3V_Out GetXYZ() const;
		void SetX( ScalarV_In newX ); // All-vector-pipeline, use when '*this' is in registers.
		void SetY( ScalarV_In newY );
		void SetZ( ScalarV_In newZ );
		void SetW( ScalarV_In newW );
		void SetXYZ( Vec3V_In newXYZ );

		void SetXInMemory( ScalarV_In newX ); // All-vector-pipeline, use when '*this' is in memory.
		void SetYInMemory( ScalarV_In newY );
		void SetZInMemory( ScalarV_In newZ );
		void SetWInMemory( ScalarV_In newW );

		void SetX( const float& floatVal ); // All-vector-pipeline, as long as 'floatVal' is in memory.
		void SetY( const float& floatVal );
		void SetZ( const float& floatVal );
		void SetW( const float& floatVal );

		float GetXf() const; // All-float-pipeline, as long as '*this' is in memory.
		float GetYf() const;
		float GetZf() const;
		float GetWf() const;
		float GetElemf( unsigned elem ) const;
		void SetElemf( unsigned elem, float fVal );
		void SetXf( float floatVal ); // All-float-pipeline, as long as '*this' is in memory.
		void SetYf( float floatVal );
		void SetZf( float floatVal );
		void SetWf( float floatVal );

		int GetXi() const; // All-int-pipeline, as long as '*this' is in memory.
		int GetYi() const;
		int GetZi() const;
		int GetWi() const;
		int GetElemi( unsigned elem ) const;
		void SetElemi( unsigned elem, int intVal );
		void SetXi( int intVal ); // All-int-pipeline, as long as '*this' is in memory.
		void SetYi( int intVal );
		void SetZi( int intVal );
		void SetWi( int intVal );

		void ZeroComponents();
		void SetWZero();

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

		template <u32 permX, u32 permY, u32 permZ>
		Vec3V_Out Get() const;

		template <u32 permX, u32 permY, u32 permZ, u32 permW>
		Vec4V_Out Get() const;

		template <u32 permX, u32 permY>
		void Set(Vec2V_In b);

		template <u32 permX, u32 permY, u32 permZ>
		void Set(Vec3V_In b);

		template <u32 permX, u32 permY, u32 permZ, u32 permW>
		void Set(Vec4V_In b);

#if __XENON || __PS3
		template <u8 byte0,u8 byte1,u8 byte2,u8 byte3,u8 byte4,u8 byte5,u8 byte6,u8 byte7,u8 byte8,u8 byte9,u8 byte10,u8 byte11,u8 byte12,u8 byte13,u8 byte14,u8 byte15>
		Vec4V_Out ByteGet() const;

		Vec4V_Out Get( Vec4V_In controlVec ) const;
#endif

		//============================================================================
		// Operators

		// Logical
		VecBoolV_Out	operator==	(Vec4V_In b) const;
		VecBoolV_Out	operator!=	(Vec4V_In b) const;
		VecBoolV_Out	operator<	(Vec4V_In bigVector) const;
		VecBoolV_Out	operator<=	(Vec4V_In bigVector) const;
		VecBoolV_Out	operator>	(Vec4V_In smallVector) const;
		VecBoolV_Out	operator>=	(Vec4V_In smallVector) const;

		// Arithmetic
		Vec4V_Out		operator*	(Vec4V_In b) const; // per-element multiply!
		Vec4V_Out		operator*	(ScalarV_In b) const;
		friend Vec4V_Out operator*	(ScalarV_In a, Vec4V_In b);
		Vec4V_Out		operator/	(Vec4V_In b) const; // per-element divide!
		Vec4V_Out		operator/	(ScalarV_In b) const;
		Vec4V_Out		operator+	(Vec4V_In b) const;
		Vec4V_Out		operator-	(Vec4V_In b) const;
		void			operator*=	(Vec4V_In b);
		void			operator*=	(ScalarV_In b);
		void			operator/=	(Vec4V_In b);
		void			operator/=	(ScalarV_In b);
		void			operator+=	(Vec4V_In b);
		void			operator-=	(Vec4V_In b);
		Vec4V_Out		operator+	() const;
		Vec4V_Out		operator-	() const;

		// Bitwise
		Vec4V_Out		operator|	(Vec4V_In b) const;
		Vec4V_Out		operator&	(Vec4V_In b) const;
		Vec4V_Out		operator^	(Vec4V_In b) const;
		void			operator|=	(Vec4V_In b);
		void			operator&=	(Vec4V_In b);
		void			operator^=	(Vec4V_In b);
		Vec4V_Out		operator~	() const;

		// Element access. Warning: Expensive operations.
		const float&	operator[]	(unsigned elem) const;
		float&			operator[]	(unsigned elem);	

	private:
		Vec::Vector_4V v;
	}
#if __PPU
	__attribute__((vecreturn))
#endif
	;

} // namespace rage

#include "vec4v.inl"


#endif // VECTORMATH_VEC4V_H
