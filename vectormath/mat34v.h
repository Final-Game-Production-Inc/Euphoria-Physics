#ifndef VECTORMATH_MAT34V_H
#define VECTORMATH_MAT34V_H

#include "data/struct.h"
#include "vectormath.h"
#include "mat33v.h"
#include "vec4v.h"
#include "vec3v.h"
#include "vec2v.h"
#include "scalarv.h"
#include "vecboolv.h"

// A 3x4 matrix, typically used for transformation, although can be used for general matrix math as well.

namespace rage
{
	class Mat34V;

	////================================================
	//// TYPES
	////================================================

	typedef Mat34V				Mat34V_Val;
	typedef Mat34V*				Mat34V_Ptr;
	typedef const Mat34V*		Mat34V_ConstPtr;
	typedef Mat34V&				Mat34V_Ref;
	typedef const Mat34V&		Mat34V_ConstRef;

	typedef Mat34V_ConstRef		Mat34V_In;
	typedef const Mat34V_Val	Mat34V_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef Mat34V_Ref			Mat34V_InOut;

	// Pass-by-vector-registers macros. For private use only.
	// The different *_DECL() versions are because Win32PC may only pass 3 arguments at most by vector registers. The rest must be by const ref.
#define MAT34V_DECL(m)			rage::Vec::Vector_4V_In m##_col0,rage::Vec::Vector_4V_In m##_col1,rage::Vec::Vector_4V_In m##_col2,rage::Vec::Vector_4V_In_After3Args m##_col3
#define MAT34V_DECL2(m)			rage::Vec::Vector_4V_In_After3Args m##_col0,rage::Vec::Vector_4V_In_After3Args m##_col1,rage::Vec::Vector_4V_In_After3Args m##_col2,rage::Vec::Vector_4V_In_After3Args m##_col3
#define MAT34V_DECL3(m)			rage::Vec::Vector_4V_In m##_col0,rage::Vec::Vector_4V_In m##_col1,rage::Vec::Vector_4V_In_After3Args m##_col2,rage::Vec::Vector_4V_In_After3Args m##_col3
#define MAT34V_ARG(m)			(m).GetCol0Intrin128(),(m).GetCol1Intrin128(),(m).GetCol2Intrin128(),(m).GetCol3Intrin128()
#define MAT34V_ARG_FLOAT_RC(m)	(m).GetCol0().GetXf(),(m).GetCol1().GetXf(),(m).GetCol2().GetXf(),(m).GetCol3().GetXf(), (m).GetCol0().GetYf(),(m).GetCol1().GetYf(),(m).GetCol2().GetYf(),(m).GetCol3().GetYf(), (m).GetCol0().GetZf(),(m).GetCol1().GetZf(),(m).GetCol2().GetZf(),(m).GetCol3().GetZf()
#define MAT34V_ARG_GET(m)		Mat34V( m##_col0, m##_col1, m##_col2, m##_col3 )
	

	//================================================
	// MAT34V (COL-MAJOR)
	//================================================

	class Mat34V
	{
		friend class Mat44V;

	public:
		// matrix constant generation syntax: Mat34V(V_ZERO)
		explicit Mat34V(eZEROInitializer);
		explicit Mat34V(eIDENTITYInitializer);
		explicit Mat34V(eFLT_EPSILONInitializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		Mat34V(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(Mat34V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Mat34V);
			STRUCT_FIELD( m_col0 );
			STRUCT_FIELD( m_col1 );
			STRUCT_FIELD( m_col2 );
			STRUCT_FIELD( m_col3 );
			STRUCT_END();
		}
#endif

		Mat34V();
		// Note: This copy constructor must be defined in order to avoid a million unnecessary lvx/stvx calls when returning matrices by value!
		// https://ps3.scedev.net/forums/thread/24368
		Mat34V(Mat34V_ConstRef);
		Mat34V_ConstRef operator= (Mat34V_ConstRef);

		// Note: This constructor does column major initialization. I.e. the first 3 arguments form column 0
		explicit Mat34V(	const float&,const float&,const float&,
							const float&,const float&,const float&,
							const float&,const float&,const float&,
							const float&,const float&,const float&	);

		explicit Mat34V(	eCOL_MAJORInitializer,
			const float&,const float&,const float&,
			const float&,const float&,const float&,
			const float&,const float&,const float&,
			const float&,const float&,const float&	);

		explicit Mat34V(	eROW_MAJORInitializer,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&	);

		explicit Mat34V(Vec::Vector_4V_In); // Puts the vector in each column.
		explicit Mat34V(	Vec::Vector_4V_In,
							Vec::Vector_4V_In,
							Vec::Vector_4V_In,
							Vec::Vector_4V_In_After3Args	);
		explicit Mat34V(ScalarV_In);	// Puts the vector in each column.
		explicit Mat34V(Vec3V_In);		// Puts the vector in each column.
		explicit Mat34V(Vec3V_In col0, Vec3V_In col1, Vec3V_In col2, Vec3V_In col3);
		explicit Mat34V(Mat33V_In);
		explicit Mat34V(Mat33V_In mat, Vec3V_In col3);

		//============================================================================
		// Getters / setters

		Mat33V_Out GetMat33() const;
		Mat33V_Ref GetMat33Ref();
		Mat33V_ConstRef GetMat33ConstRef() const;

		float GetM00f() const;	float GetM01f() const;	float GetM02f() const;	float GetM03f() const;
		float GetM10f() const;	float GetM11f() const;	float GetM12f() const;	float GetM13f() const;
		float GetM20f() const;	float GetM21f() const;	float GetM22f() const;	float GetM23f() const;
		Vec3V_Out GetCol0() const;
		Vec3V_Out GetCol1() const;
		Vec3V_Out GetCol2() const;
		Vec3V_Out GetCol3() const;
		Vec3V_Ref GetCol0Ref();
		Vec3V_Ref GetCol1Ref();
		Vec3V_Ref GetCol2Ref();
		Vec3V_Ref GetCol3Ref();
		Vec3V_ConstRef GetCol0ConstRef() const;
		Vec3V_ConstRef GetCol1ConstRef() const;
		Vec3V_ConstRef GetCol2ConstRef() const;
		Vec3V_ConstRef GetCol3ConstRef() const;
		Vec3V_Out a() const;
		Vec3V_Out b() const;
		Vec3V_Out c() const;
		Vec3V_Out d() const;
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
		
		float GetElemf( unsigned row, unsigned col ) const;
		void SetElemf( unsigned row, unsigned col, float fVal );

		void SetM00( const float& fVal );	void SetM01( const float& fVal );	void SetM02( const float& fVal );	void SetM03( const float& fVal );
		void SetM10( const float& fVal );	void SetM11( const float& fVal );	void SetM12( const float& fVal );	void SetM13( const float& fVal );
		void SetM20( const float& fVal );	void SetM21( const float& fVal );	void SetM22( const float& fVal );	void SetM23( const float& fVal );
		void SetM00f( float fVal );	void SetM01f( float fVal );	void SetM02f( float fVal );	void SetM03f( float fVal );
		void SetM10f( float fVal );	void SetM11f( float fVal );	void SetM12f( float fVal );	void SetM13f( float fVal );
		void SetM20f( float fVal );	void SetM21f( float fVal );	void SetM22f( float fVal );	void SetM23f( float fVal );
		void SetCol0( Vec3V_In col0 );
		void SetCol1( Vec3V_In col1 );
		void SetCol2( Vec3V_In col2 );
		void SetCol3( Vec3V_In col3 );
		void Seta( Vec3V_In col0 );
		void Setb( Vec3V_In col1 );
		void Setc( Vec3V_In col2 );
		void Setd( Vec3V_In col3 );
		void SetCols( Vec3V_In col0, Vec3V_In col1, Vec3V_In col2, Vec3V_In col3 );
		void SetCol0Intrin128( Vec::Vector_4V_In col0 );
		void SetCol1Intrin128( Vec::Vector_4V_In col1 );
		void SetCol2Intrin128( Vec::Vector_4V_In col2 );
		void SetCol3Intrin128( Vec::Vector_4V_In col3 );
		void SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2 );
		void SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2, Vec::Vector_4V_In_After3Args col3 );
		// Use this when possibile! It sometimes generates more optimal scheduling than the other SetCols() or constructor!
		void SetCols( Vec3V_In col );
		// Use this when possibile! It sometimes generates more optimal scheduling than the other SetColsIntrin128() or constructor!
		void SetColsIntrin128( Vec::Vector_4V_In col );
		void SetM30M31M32Zero();
		void SetIdentity3x3();
		void Set3x3( Mat34V_ConstRef newMat );
		void Set3x3( Mat33V_ConstRef newMat );

		VecBoolV_Out IsOrthonormal3x3V(ScalarV_In magTolerance, ScalarV_In angTolerance) const;
		VecBoolV_Out IsOrthonormal3x3V(ScalarV_In toleranceSqVect) const;
		bool IsOrthonormal3x3(ScalarV_In magTolerance, ScalarV_In angTolerance) const;
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
		Mat34V_Out			operator+	(Mat34V_In b) const;
		Mat34V_Out			operator-	(Mat34V_In b) const;
		void				operator+=	(Mat34V_In b);
		void				operator-=	(Mat34V_In b);
		Mat34V_Out			operator-	() const;

		// Bitwise
		Mat34V_Out			operator|	(Mat34V_In b) const;
		Mat34V_Out			operator&	(Mat34V_In b) const;
		Mat34V_Out			operator^	(Mat34V_In b) const;
		void				operator|=	(Mat34V_In b);
		void				operator&=	(Mat34V_In b);
		void				operator^=	(Mat34V_In b);

		// Col element access.
		Vec3V_ConstRef	operator[]	(unsigned col) const;
		Vec3V_Ref		operator[]	(unsigned col);

	private:
		Vec::Vector_4V m_col0;			// _m11_m21_m31(_m41) (                             )
		Vec::Vector_4V m_col1;			// _m12_m22_m32(_m42) ( typically a rotation matrix )
		Vec::Vector_4V m_col2;			// _m13_m23_m33(_m43) (                             )
		Vec::Vector_4V m_col3;			// _m14_m24_m34(_m44) ( typically a translation vector )
	};

} // namespace rage

#include "mat34v.inl"

#endif // VECTORMATH_MAT34V_H
