#ifndef VECTORMATH_MAT44V_H
#define VECTORMATH_MAT44V_H

#include "data/struct.h"
#include "vectormath.h"
#include "mat34v.h"
#include "mat33v.h"
#include "vec4v.h"
#include "vec3v.h"
#include "vec2v.h"
#include "scalarv.h"
#include "vecboolv.h"

namespace rage
{
	class Mat44V;			// col-major matrix to pre-multiply by

	//================================================
	// TYPES
	//================================================

	typedef Mat44V				Mat44V_Val;
	typedef Mat44V*				Mat44V_Ptr;
	typedef const Mat44V*		Mat44V_ConstPtr;
	typedef Mat44V&				Mat44V_Ref;
	typedef const Mat44V&		Mat44V_ConstRef;

	typedef Mat44V_ConstRef		Mat44V_In;
	typedef const Mat44V_Val	Mat44V_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef Mat44V_Ref			Mat44V_InOut;

	// Pass-by-vector-registers macros. For private use only.
	// The different *_DECL() versions are because Win32PC may only pass 3 arguments at most by vector registers. The rest must be by const ref.
#define MAT44V_DECL(m)			rage::Vec::Vector_4V_In m##_col0,rage::Vec::Vector_4V_In m##_col1,rage::Vec::Vector_4V_In m##_col2,rage::Vec::Vector_4V_In_After3Args m##_col3
#define MAT44V_DECL2(m)			rage::Vec::Vector_4V_In_After3Args m##_col0,rage::Vec::Vector_4V_In_After3Args m##_col1,rage::Vec::Vector_4V_In_After3Args m##_col2,rage::Vec::Vector_4V_In_After3Args m##_col3
#define MAT44V_DECL3(m)			rage::Vec::Vector_4V_In m##_col0,rage::Vec::Vector_4V_In m##_col1,rage::Vec::Vector_4V_In_After3Args m##_col2,rage::Vec::Vector_4V_In_After3Args m##_col3
#define MAT44V_ARG(m)			(m).GetCol0Intrin128(),(m).GetCol1Intrin128(),(m).GetCol2Intrin128(),(m).GetCol3Intrin128()
#define MAT44V_ARG_GET(m)		Mat44V( m##_col0, m##_col1, m##_col2, m##_col3 )
	

	//================================================
	// MAT44V (COL-MAJOR)
	//================================================

	class Mat44V
	{

	public:
		// matrix constant generation syntax: Mat44V(V_ZERO)
		explicit Mat44V(eZEROInitializer);
		explicit Mat44V(eIDENTITYInitializer);
		explicit Mat44V(eFLT_EPSILONInitializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		Mat44V(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(Mat44V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Mat44V);
			STRUCT_FIELD( m_col0 );
			STRUCT_FIELD( m_col1 );
			STRUCT_FIELD( m_col2 );
			STRUCT_FIELD( m_col3 );
			STRUCT_END();
		}
#endif

		Mat44V();
		// Note: This copy constructor must be defined in order to avoid a million unnecessary lvx/stvx calls when returning matrices by value!
		// https://ps3.scedev.net/forums/thread/24368
		Mat44V(Mat44V_ConstRef);
		Mat44V_ConstRef operator= (Mat44V_ConstRef);

		// Note: This constructor does column major initialization. I.e. the first 4 arguments form column 0
		explicit Mat44V(	const float&,const float&,const float&,const float&,
							const float&,const float&,const float&,const float&,
							const float&,const float&,const float&,const float&,
							const float&,const float&,const float&,const float&	);

		explicit Mat44V(	eCOL_MAJORInitializer,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&	);

		explicit Mat44V(	eROW_MAJORInitializer,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&	);

		explicit Mat44V(Vec::Vector_4V_In); // Puts the vector in each column.
		explicit Mat44V(	Vec::Vector_4V_In,
							Vec::Vector_4V_In,
							Vec::Vector_4V_In,
							Vec::Vector_4V_In_After3Args	);
		explicit Mat44V(Vec4V_In); // Puts the vector in each column.
		explicit Mat44V(Vec4V_In col0, Vec4V_In col1, Vec4V_In col2, Vec4V_In col3);
		explicit Mat44V(Mat33V_In);
		explicit Mat44V(Mat33V_In mat, Vec4V_In col3); // M30,M31,M32 are set to 0.0f
		explicit Mat44V(Mat34V_In mat); // M30,M31,M32,M33 are undefined

		//============================================================================
		// Getters / setters
		Mat34V_Out GetMat34() const;

		float GetM00f() const;	float GetM01f() const;	float GetM02f() const;	float GetM03f() const;
		float GetM10f() const;	float GetM11f() const;	float GetM12f() const;	float GetM13f() const;
		float GetM20f() const;	float GetM21f() const;	float GetM22f() const;	float GetM23f() const;
		float GetM30f() const;	float GetM31f() const;	float GetM32f() const;	float GetM33f() const;
		float GetElemf( unsigned row, unsigned col ) const;
		Vec4V_Out GetCol( unsigned col ) const;
		Vec4V_Out GetCol0() const;
		Vec4V_Out GetCol1() const;
		Vec4V_Out GetCol2() const;
		Vec4V_Out GetCol3() const;
		Vec4V_Ref GetCol0Ref();
		Vec4V_Ref GetCol1Ref();
		Vec4V_Ref GetCol2Ref();
		Vec4V_Ref GetCol3Ref();
		Vec4V_ConstRef GetCol0ConstRef() const;
		Vec4V_ConstRef GetCol1ConstRef() const;
		Vec4V_ConstRef GetCol2ConstRef() const;
		Vec4V_ConstRef GetCol3ConstRef() const;
		Vec4V_Out a() const;
		Vec4V_Out b() const;
		Vec4V_Out c() const;
		Vec4V_Out d() const;
		Vec::Vector_4V_Out GetCol0Intrin128() const;
		Vec::Vector_4V_Out GetCol1Intrin128() const;
		Vec::Vector_4V_Out GetCol2Intrin128() const;
		Vec::Vector_4V_Out GetCol3Intrin128() const;
		Vec::Vector_4V_Ref GetCol0Intrin128Ref();
		Vec::Vector_4V_Ref GetCol1Intrin128Ref();
		Vec::Vector_4V_Ref GetCol2Intrin128Ref();
		Vec::Vector_4V_Ref GetCol3Intrin128Ref();
		Vec::Vector_4V_ConstRef GetCol0Intrin128ConstRef() const;
		Vec::Vector_4V_ConstRef GetCol1Intrin128ConstRef() const;
		Vec::Vector_4V_ConstRef GetCol2Intrin128ConstRef() const;
		Vec::Vector_4V_ConstRef GetCol3Intrin128ConstRef() const;

		void SetM00( const float& fVal );	void SetM01( const float& fVal );	void SetM02( const float& fVal );	void SetM03( const float& fVal );
		void SetM10( const float& fVal );	void SetM11( const float& fVal );	void SetM12( const float& fVal );	void SetM13( const float& fVal );
		void SetM20( const float& fVal );	void SetM21( const float& fVal );	void SetM22( const float& fVal );	void SetM23( const float& fVal );
		void SetM30( const float& fVal );	void SetM31( const float& fVal );	void SetM32( const float& fVal );	void SetM33( const float& fVal );
		void SetM00f( float fVal );	void SetM01f( float fVal );	void SetM02f( float fVal );	void SetM03f( float fVal );
		void SetM10f( float fVal );	void SetM11f( float fVal );	void SetM12f( float fVal );	void SetM13f( float fVal );
		void SetM20f( float fVal );	void SetM21f( float fVal );	void SetM22f( float fVal );	void SetM23f( float fVal );
		void SetM30f( float fVal );	void SetM31f( float fVal );	void SetM32f( float fVal );	void SetM33f( float fVal );
		void SetElemf( unsigned row, unsigned col, float fVal );
		void SetCol0( Vec4V_In col0 );
		void SetCol1( Vec4V_In col1 );
		void SetCol2( Vec4V_In col2 );
		void SetCol3( Vec4V_In col3 );
		void SetCols( Vec4V_In col0, Vec4V_In col1, Vec4V_In col2, Vec4V_In col3 );
		void SetCol0f( float x, float y, float z, float w);
		void SetCol1f( float x, float y, float z, float w);
		void SetCol2f( float x, float y, float z, float w);
		void SetCol3f( float x, float y, float z, float w);
		void SetCol0Intrin128( Vec::Vector_4V_In col0 );
		void SetCol1Intrin128( Vec::Vector_4V_In col1 );
		void SetCol2Intrin128( Vec::Vector_4V_In col2 );
		void SetCol3Intrin128( Vec::Vector_4V_In col3 );
		void SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2 );
		void SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2, Vec::Vector_4V_In_After3Args col3 );
		// Use this when possibile! It sometimes generates more optimal scheduling than the other SetCols() or constructor!
		void SetCols( Vec4V_In col );
		// Use this when possibile! It sometimes generates more optimal scheduling than the other SetColsIntrin128() or constructor!
		void SetColsIntrin128( Vec::Vector_4V_In col );
		void SetM30M31M32Zero();

		VecBoolV_Out IsOrthonormal3x3V(ScalarV_In toleranceSqVect) const;
		bool IsOrthonormal3x3(ScalarV_In toleranceSqVect) const;

		VecBoolV_Out IsOrthogonal3x3V(ScalarV_In angTolerance) const;
		bool IsOrthogonal3x3(ScalarV_In angTolerance) const;

		//============================================================================
		// Output
		void Print() const;
		void PrintHex() const;

		//============================================================================
		// Operators

		// Arithmetic
		Mat44V_Out		operator+	(Mat44V_In b) const;
		Mat44V_Out		operator-	(Mat44V_In b) const;
		void			operator+=	(Mat44V_In b);
		void			operator-=	(Mat44V_In b);
		Mat44V_Out		operator-	() const;

		// Bitwise
		Mat44V_Out		operator|	(Mat44V_In b) const;
		Mat44V_Out		operator&	(Mat44V_In b) const;
		Mat44V_Out		operator^	(Mat44V_In b) const;
		void			operator|=	(Mat44V_In b);
		void			operator&=	(Mat44V_In b);
		void			operator^=	(Mat44V_In b);

		// Col element access.
		Vec4V_ConstRef	operator[]	(unsigned col) const;
		Vec4V_Ref		operator[]	(unsigned col);

	public:
		Vec::Vector_4V m_col0; // _m11_m21_m31_m41
		Vec::Vector_4V m_col1; // _m12_m22_m32_m42
		Vec::Vector_4V m_col2; // _m13_m23_m33_m43
		Vec::Vector_4V m_col3; // _m14_m24_m34_m44
	};

} // namespace rage

#include "mat44v.inl"


#endif // VECTORMATH_MAT44V_H
