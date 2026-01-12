#ifndef VECTORMATH_MAT44V_SOA_H
#define VECTORMATH_MAT44V_SOA_H

#include "data/struct.h"
#include "vectormath.h"
#include "vec4v_soa.h"

namespace rage
{
	class SoA_Mat44V;			// col-major matrix to pre-multiply by

	//================================================
	// TYPES
	//================================================

	typedef SoA_Mat44V				SoA_Mat44V_Val;
	typedef SoA_Mat44V*				SoA_Mat44V_Ptr;
	typedef SoA_Mat44V&				SoA_Mat44V_Ref;
	typedef const SoA_Mat44V&		SoA_Mat44V_ConstRef;

	typedef SoA_Mat44V_ConstRef		SoA_Mat44V_In;
	typedef const SoA_Mat44V_Val	SoA_Mat44V_Out;	// "const" to warn that this isn't a "return by ref" parameter
	typedef SoA_Mat44V_Ref			SoA_Mat44V_InOut;

	// Pass-by-12-vector-registers macros. For private use only.
#define MAT44V_SOA_DECL(m)			rage::Vec::Vector_4V_In m##_00,rage::Vec::Vector_4V_In m##_10,rage::Vec::Vector_4V_In m##_20,rage::Vec::Vector_4V_In_After3Args m##_30, \
									rage::Vec::Vector_4V_In_After3Args m##_01,rage::Vec::Vector_4V_In_After3Args m##_11,rage::Vec::Vector_4V_In_After3Args m##_21,rage::Vec::Vector_4V_In_After3Args m##_31, \
									rage::Vec::Vector_4V_In_After3Args m##_02,rage::Vec::Vector_4V_In_After3Args m##_12,rage::Vec::Vector_4V_In_After3Args m##_22,rage::Vec::Vector_4V_In_After3Args m##_32, \
									rage::Vec::Vector_4V_In_After3Args m##_03,rage::Vec::Vector_4V_In_After3Args m##_13,rage::Vec::Vector_4V_In_After3Args m##_23,rage::Vec::Vector_4V_In_After3Args m##_33
#define MAT44V_SOA_DECL2(m)			rage::Vec::Vector_4V_In_After3Args m##_00,rage::Vec::Vector_4V_In_After3Args m##_10,rage::Vec::Vector_4V_In_After3Args m##_20,rage::Vec::Vector_4V_In_After3Args m##_30, \
									rage::Vec::Vector_4V_In_After3Args m##_01,rage::Vec::Vector_4V_In_After3Args m##_11,rage::Vec::Vector_4V_In_After3Args m##_21,rage::Vec::Vector_4V_In_After3Args m##_31, \
									rage::Vec::Vector_4V_In_After3Args m##_02,rage::Vec::Vector_4V_In_After3Args m##_12,rage::Vec::Vector_4V_In_After3Args m##_22,rage::Vec::Vector_4V_In_After3Args m##_32, \
									rage::Vec::Vector_4V_In_After3Args m##_03,rage::Vec::Vector_4V_In_After3Args m##_13,rage::Vec::Vector_4V_In_After3Args m##_23,rage::Vec::Vector_4V_In_After3Args m##_33
#define MAT44V_SOA_DECL3(m)			rage::Vec::Vector_4V_In m##_00,rage::Vec::Vector_4V_In m##_10,rage::Vec::Vector_4V_In_After3Args m##_20,rage::Vec::Vector_4V_In_After3Args m##_30, \
									rage::Vec::Vector_4V_In_After3Args m##_01,rage::Vec::Vector_4V_In_After3Args m##_11,rage::Vec::Vector_4V_In_After3Args m##_21,rage::Vec::Vector_4V_In_After3Args m##_31, \
									rage::Vec::Vector_4V_In_After3Args m##_02,rage::Vec::Vector_4V_In_After3Args m##_12,rage::Vec::Vector_4V_In_After3Args m##_22,rage::Vec::Vector_4V_In_After3Args m##_32, \
									rage::Vec::Vector_4V_In_After3Args m##_03,rage::Vec::Vector_4V_In_After3Args m##_13,rage::Vec::Vector_4V_In_After3Args m##_23,rage::Vec::Vector_4V_In_After3Args m##_33
#define MAT44V_SOA_ARG(m)			(m).GetM00Intrin128(),(m).GetM10Intrin128(),(m).GetM20Intrin128(),(m).GetM30Intrin128(),			\
									(m).GetM01Intrin128(),(m).GetM11Intrin128(),(m).GetM21Intrin128(),(m).GetM31Intrin128(),			\
									(m).GetM02Intrin128(),(m).GetM12Intrin128(),(m).GetM22Intrin128(),(m).GetM32Intrin128(),			\
									(m).GetM03Intrin128(),(m).GetM13Intrin128(),(m).GetM23Intrin128(),(m).GetM33Intrin128()
#define MAT44V_SOA_ARG_GET(m)		SoA_Mat44V( m##_00,m##_10,m##_20,m##_30,m##_01,m##_11,m##_21,m##_31,m##_02,m##_12,m##_22,m##_32,m##_03,m##_13,m##_23,m##_33 )
	

	//================================================
	// MAT44V (COL-MAJOR)
	//================================================

	class SoA_Mat44V
	{

	public:
		enum eZEROInitializer		{	ZERO = 0,		};
		enum eIDENTITYInitializer	{	IDENTITY = 0,	};
		enum eFLT_EPSInitializer	{	FLT_EPS = 0,	};

		enum eROW_MAJORInitializer	{	ROW_MAJOR	};
		enum eCOL_MAJORInitializer	{	COL_MAJOR	};

	public:
		// matrix constant generation syntax: SoA_Mat44V( SoA_Mat44V::ZERO )
		explicit SoA_Mat44V(eZEROInitializer);
		explicit SoA_Mat44V(eIDENTITYInitializer);
		explicit SoA_Mat44V(eFLT_EPSInitializer);

		// PURPOSE: Resource constructor, for structures created from a resource
		// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
		SoA_Mat44V(class datResource&) {}

		// PURPOSE: Used by the rorc resourcing system
		DECLARE_DUMMY_PLACE(SoA_Mat44V);

		// PURPOSE: Used by the rorc resourcing system
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(SoA_Mat44V);
			STRUCT_FIELD( m_00 );
			STRUCT_FIELD( m_10 );
			STRUCT_FIELD( m_20 );
			STRUCT_FIELD( m_30 );
			STRUCT_FIELD( m_01 );
			STRUCT_FIELD( m_11 );
			STRUCT_FIELD( m_21 );
			STRUCT_FIELD( m_31 );
			STRUCT_FIELD( m_02 );
			STRUCT_FIELD( m_12 );
			STRUCT_FIELD( m_22 );
			STRUCT_FIELD( m_32 );
			STRUCT_FIELD( m_03 );
			STRUCT_FIELD( m_13 );
			STRUCT_FIELD( m_23 );
			STRUCT_FIELD( m_33 );
			STRUCT_END();
		}
#endif

		SoA_Mat44V();
		// Note: This copy constructor must be defined in order to avoid a million unnecessary lvx/stvx calls when returning matrices by value!
		// https://ps3.scedev.net/forums/thread/24368
		SoA_Mat44V(SoA_Mat44V_ConstRef);
		SoA_Mat44V_Ref operator= (SoA_Mat44V_ConstRef);

		// Note: This constructor does column major initialization. I.e. the first 4 arguments form column 0
		explicit SoA_Mat44V(	const float&,const float&,const float&,const float&,
							const float&,const float&,const float&,const float&,
							const float&,const float&,const float&,const float&,
							const float&,const float&,const float&,const float&	);

		explicit SoA_Mat44V(	eCOL_MAJORInitializer,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&	);

		explicit SoA_Mat44V(	eROW_MAJORInitializer,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&,
			const float&,const float&,const float&,const float&	);

		explicit SoA_Mat44V(Vec::Vector_4V_In); // Puts the intrinsic vector in each element.
		explicit SoA_Mat44V(	Vec::Vector_4V_In _00,Vec::Vector_4V_In _10,Vec::Vector_4V_In _20,Vec::Vector_4V_In_After3Args _30,
							Vec::Vector_4V_In_After3Args _01,Vec::Vector_4V_In_After3Args _11,Vec::Vector_4V_In_After3Args _21,Vec::Vector_4V_In_After3Args _31,
							Vec::Vector_4V_In_After3Args _02,Vec::Vector_4V_In_After3Args _12,Vec::Vector_4V_In_After3Args _22,Vec::Vector_4V_In_After3Args _32,
							Vec::Vector_4V_In_After3Args _03,Vec::Vector_4V_In_After3Args _13,Vec::Vector_4V_In_After3Args _23,Vec::Vector_4V_In_After3Args _33	);
		explicit SoA_Mat44V(SoA_Vec4V_In col); // Puts the vector in each column.
		explicit SoA_Mat44V(SoA_Vec4V_In col0, SoA_Vec4V_In col1, SoA_Vec4V_In col2, SoA_Vec4V_In col3);

		//============================================================================
		// Getters / setters

		SoA_ScalarV_Out GetM00() const;	SoA_ScalarV_Out GetM01() const;	SoA_ScalarV_Out GetM02() const;	SoA_ScalarV_Out GetM03() const;
		SoA_ScalarV_Out GetM10() const;	SoA_ScalarV_Out GetM11() const;	SoA_ScalarV_Out GetM12() const;	SoA_ScalarV_Out GetM13() const;
		SoA_ScalarV_Out GetM20() const;	SoA_ScalarV_Out GetM21() const;	SoA_ScalarV_Out GetM22() const;	SoA_ScalarV_Out GetM23() const;
		SoA_ScalarV_Out GetM30() const;	SoA_ScalarV_Out GetM31() const;	SoA_ScalarV_Out GetM32() const;	SoA_ScalarV_Out GetM33() const;

		Vec::Vector_4V_Out GetM00Intrin128() const;	Vec::Vector_4V_Out GetM01Intrin128() const;	Vec::Vector_4V_Out GetM02Intrin128() const;	Vec::Vector_4V_Out GetM03Intrin128() const;
		Vec::Vector_4V_Out GetM10Intrin128() const;	Vec::Vector_4V_Out GetM11Intrin128() const;	Vec::Vector_4V_Out GetM12Intrin128() const;	Vec::Vector_4V_Out GetM13Intrin128() const;
		Vec::Vector_4V_Out GetM20Intrin128() const;	Vec::Vector_4V_Out GetM21Intrin128() const;	Vec::Vector_4V_Out GetM22Intrin128() const;	Vec::Vector_4V_Out GetM23Intrin128() const;
		Vec::Vector_4V_Out GetM30Intrin128() const;	Vec::Vector_4V_Out GetM31Intrin128() const;	Vec::Vector_4V_Out GetM32Intrin128() const;	Vec::Vector_4V_Out GetM33Intrin128() const;
		
		SoA_Vec4V_Out GetCol0() const;
		SoA_Vec4V_Out GetCol1() const;
		SoA_Vec4V_Out GetCol2() const;
		SoA_Vec4V_Out GetCol3() const;
		SoA_Vec4V_Out GetRow0() const;
		SoA_Vec4V_Out GetRow1() const;
		SoA_Vec4V_Out GetRow2() const;
		SoA_Vec4V_Out GetRow3() const;

		void SetM00( SoA_ScalarV_In vVal );	void SetM01( SoA_ScalarV_In vVal );	void SetM02( SoA_ScalarV_In vVal );	void SetM03( SoA_ScalarV_In vVal );
		void SetM10( SoA_ScalarV_In vVal );	void SetM11( SoA_ScalarV_In vVal );	void SetM12( SoA_ScalarV_In vVal );	void SetM13( SoA_ScalarV_In vVal );
		void SetM20( SoA_ScalarV_In vVal );	void SetM21( SoA_ScalarV_In vVal );	void SetM22( SoA_ScalarV_In vVal );	void SetM23( SoA_ScalarV_In vVal );
		void SetM30( SoA_ScalarV_In vVal );	void SetM31( SoA_ScalarV_In vVal );	void SetM32( SoA_ScalarV_In vVal );	void SetM33( SoA_ScalarV_In vVal );

		void SetM00Intrin128( Vec::Vector_4V_In vVal );	void SetM01Intrin128( Vec::Vector_4V_In vVal );	void SetM02Intrin128( Vec::Vector_4V_In vVal );	void SetM03Intrin128( Vec::Vector_4V_In vVal );
		void SetM10Intrin128( Vec::Vector_4V_In vVal );	void SetM11Intrin128( Vec::Vector_4V_In vVal );	void SetM12Intrin128( Vec::Vector_4V_In vVal );	void SetM13Intrin128( Vec::Vector_4V_In vVal );
		void SetM20Intrin128( Vec::Vector_4V_In vVal );	void SetM21Intrin128( Vec::Vector_4V_In vVal );	void SetM22Intrin128( Vec::Vector_4V_In vVal );	void SetM23Intrin128( Vec::Vector_4V_In vVal );
		void SetM30Intrin128( Vec::Vector_4V_In vVal );	void SetM31Intrin128( Vec::Vector_4V_In vVal );	void SetM32Intrin128( Vec::Vector_4V_In vVal );	void SetM33Intrin128( Vec::Vector_4V_In vVal );

		void SetCol0( SoA_Vec4V_In col0 );
		void SetCol1( SoA_Vec4V_In col1 );
		void SetCol2( SoA_Vec4V_In col2 );
		void SetCol3( SoA_Vec4V_In col3 );
		void SetCols( SoA_Vec4V_In col0, SoA_Vec4V_In col1, SoA_Vec4V_In col2, SoA_Vec4V_In col3 );
		void SetRow0( SoA_Vec4V_In row0 );
		void SetRow1( SoA_Vec4V_In row1 );
		void SetRow2( SoA_Vec4V_In row2 );
		void SetRow3( SoA_Vec4V_In row3 );
		void SetRows( SoA_Vec4V_In row0, SoA_Vec4V_In row1, SoA_Vec4V_In row2, SoA_Vec4V_In row3 );
		
		// Use these when possibile! They sometimes generate more optimal scheduling than the other Set*()'s or constructors!
		void SetCols( SoA_Vec4V_In col );
		void SetRows( SoA_Vec4V_In row );
		void SetCol0Intrin128( Vec::Vector_4V_In col0 );
		void SetCol1Intrin128( Vec::Vector_4V_In col1 );
		void SetCol2Intrin128( Vec::Vector_4V_In col2 );
		void SetCol3Intrin128( Vec::Vector_4V_In col3 );
		void SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2, Vec::Vector_4V_In_After3Args col3 );
		void SetRow0Intrin128( Vec::Vector_4V_In row0 );
		void SetRow1Intrin128( Vec::Vector_4V_In row1 );
		void SetRow2Intrin128( Vec::Vector_4V_In row2 );
		void SetRow3Intrin128( Vec::Vector_4V_In row3 );
		void SetRowsIntrin128( Vec::Vector_4V_In row0, Vec::Vector_4V_In row1, Vec::Vector_4V_In row2, Vec::Vector_4V_In_After3Args row3 );
		void SetAllIntrin128( Vec::Vector_4V_In vVal );

		//SoA_ScalarV_Out IsOrthonormal3x3V(SoA_ScalarV_In toleranceSqVect) const;

		//============================================================================
		// Output

		void Print() const;
		void PrintHex() const;

		//============================================================================
		// Operators

		// Arithmetic
		SoA_Mat44V_Out	operator+	(SoA_Mat44V_In b) const;
		SoA_Mat44V_Out	operator-	(SoA_Mat44V_In b) const;
		void			operator+=	(SoA_Mat44V_In b);
		void			operator-=	(SoA_Mat44V_In b);
		SoA_Mat44V_Out	operator-	() const;

		// Bitwise
		SoA_Mat44V_Out	operator|	(SoA_Mat44V_In b) const;
		SoA_Mat44V_Out	operator&	(SoA_Mat44V_In b) const;
		SoA_Mat44V_Out	operator^	(SoA_Mat44V_In b) const;
		void			operator|=	(SoA_Mat44V_In b);
		void			operator&=	(SoA_Mat44V_In b);
		void			operator^=	(SoA_Mat44V_In b);

		// Element access.
		Vec::Vector_4V_ConstRef	operator[]	(u32 elem) const;
		Vec::Vector_4V_Ref		operator[]	(u32 elem);

	private:
		// COL 0
		Vec::Vector_4V m_00;
		Vec::Vector_4V m_10;
		Vec::Vector_4V m_20;
		Vec::Vector_4V m_30;

		// COL 1
		Vec::Vector_4V m_01;
		Vec::Vector_4V m_11;
		Vec::Vector_4V m_21;
		Vec::Vector_4V m_31;

		// COL 2
		Vec::Vector_4V m_02;
		Vec::Vector_4V m_12;
		Vec::Vector_4V m_22;
		Vec::Vector_4V m_32;

		// COL 3
		Vec::Vector_4V m_03;
		Vec::Vector_4V m_13;
		Vec::Vector_4V m_23;
		Vec::Vector_4V m_33;
	};

} // namespace rage

#include "mat44v_soa.inl"

#endif // VECTORMATH_MAT44V_SOA_H
