
namespace rage
{
	__forceinline SoA_Mat33V::SoA_Mat33V(eZEROInitializer)
	{
		m_00=m_10=m_20=m_01=m_11=m_21=m_02=m_12=m_22= Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_Mat33V::SoA_Mat33V(eIDENTITYInitializer)
	{
		m_00 = m_11 = m_22 = Vec::V4VConstant(V_ONE);
		m_10=m_20=m_01=m_21=m_02=m_12= Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_Mat33V::SoA_Mat33V(eFLT_EPSInitializer)
	{
		m_00=m_10=m_20=m_01=m_11=m_21=m_02=m_12=m_22= Vec::V4VConstant(V_FLT_EPSILON);
	}

	__forceinline SoA_Mat33V::SoA_Mat33V()
	{
	}

	__forceinline SoA_Mat33V::SoA_Mat33V(SoA_Mat33V_ConstRef a)
		:	m_00(a.m_00), m_10( a.m_10 ), m_20( a.m_20 ),
			m_01(a.m_01), m_11( a.m_11 ), m_21( a.m_21 ),
			m_02(a.m_02), m_12( a.m_12 ), m_22( a.m_22 )
			
	{
	}

	__forceinline SoA_Mat33V_Ref SoA_Mat33V::operator= (SoA_Mat33V_ConstRef a)
	{
		m_00 = a.m_00; m_10 = a.m_10; m_20 = a.m_20;
		m_01 = a.m_01; m_11 = a.m_11; m_21 = a.m_21;
		m_02 = a.m_02; m_12 = a.m_12; m_22 = a.m_22;
		return *this;
	}

	//__forceinline SoA_Mat33V::SoA_Mat33V(const float& a)
	//{
	//	Vec::V4Set( m_00, a );
	//	m_10=m_20=m_01=m_11=m_21=m_02=m_12=m_22=m_00;
	//}

	__forceinline SoA_Mat33V::SoA_Mat33V(	const float& f00, const float& f10, const float& f20,
									const float& f01, const float& f11, const float& f21,
									const float& f02, const float& f12, const float& f22	)
	{
		Vec::V4Set( m_00, f00 );
		Vec::V4Set( m_10, f10 );
		Vec::V4Set( m_20, f20 );
		Vec::V4Set( m_01, f01 );
		Vec::V4Set( m_11, f11 );
		Vec::V4Set( m_21, f21 );
		Vec::V4Set( m_02, f02 );
		Vec::V4Set( m_12, f12 );
		Vec::V4Set( m_22, f22 );
	}


	__forceinline SoA_Mat33V::SoA_Mat33V(	eCOL_MAJORInitializer,
		const float& f00, const float& f10, const float& f20,
		const float& f01, const float& f11, const float& f21,
		const float& f02, const float& f12, const float& f22	)
	{
		Vec::V4Set( m_00, f00 );
		Vec::V4Set( m_10, f10 );
		Vec::V4Set( m_20, f20 );
		Vec::V4Set( m_01, f01 );
		Vec::V4Set( m_11, f11 );
		Vec::V4Set( m_21, f21 );
		Vec::V4Set( m_02, f02 );
		Vec::V4Set( m_12, f12 );
		Vec::V4Set( m_22, f22 );
	}


	__forceinline SoA_Mat33V::SoA_Mat33V(	eROW_MAJORInitializer,
		const float& f00, const float& f01, const float& f02,
		const float& f10, const float& f11, const float& f12,
		const float& f20, const float& f21, const float& f22	)
	{
		Vec::V4Set( m_00, f00 );
		Vec::V4Set( m_10, f10 );
		Vec::V4Set( m_20, f20 );
		Vec::V4Set( m_01, f01 );
		Vec::V4Set( m_11, f11 );
		Vec::V4Set( m_21, f21 );
		Vec::V4Set( m_02, f02 );
		Vec::V4Set( m_12, f12 );
		Vec::V4Set( m_22, f22 );
	}

	__forceinline SoA_Mat33V::SoA_Mat33V(Vec::Vector_4V_In a)
	{
		m_00=m_10=m_20=m_01=m_11=m_21=m_02=m_12=m_22 = a;
	}

	__forceinline SoA_Mat33V::SoA_Mat33V(	Vec::Vector_4V_In _00,Vec::Vector_4V_In _10,Vec::Vector_4V_In _20,
									Vec::Vector_4V_In_After3Args _01,Vec::Vector_4V_In_After3Args _11,Vec::Vector_4V_In_After3Args _21,
									Vec::Vector_4V_In_After3Args _02,Vec::Vector_4V_In_After3Args _12,Vec::Vector_4V_In_After3Args _22	)
		:	m_00(_00), m_10(_10), m_20(_20),
			m_01(_01), m_11(_11), m_21(_21),
			m_02(_02), m_12(_12), m_22(_22)
	{
	}

	__forceinline SoA_Mat33V::SoA_Mat33V(SoA_Vec3V_In col)
	{
		m_00 = m_01 = m_02 = col.GetXIntrin128();
		m_10 = m_11 = m_12 = col.GetYIntrin128();
		m_20 = m_21 = m_22 = col.GetZIntrin128();
	}

	__forceinline SoA_Mat33V::SoA_Mat33V(SoA_Vec3V_In col0, SoA_Vec3V_In col1, SoA_Vec3V_In col2)
		:	m_00(col0.GetXIntrin128()), m_10(col0.GetYIntrin128()), m_20(col0.GetZIntrin128()),
			m_01(col1.GetXIntrin128()), m_11(col1.GetYIntrin128()), m_21(col1.GetZIntrin128()),
			m_02(col2.GetXIntrin128()), m_12(col2.GetYIntrin128()), m_22(col2.GetZIntrin128())
	{
	}

	__forceinline SoA_Mat33V::SoA_Mat33V(	SoA_ScalarV_In _00, SoA_ScalarV_In _10, SoA_ScalarV_In _20,
									SoA_ScalarV_In _01, SoA_ScalarV_In _11, SoA_ScalarV_In _21, 
									SoA_ScalarV_In _02, SoA_ScalarV_In _12, SoA_ScalarV_In _22	)
		:	m_00(_00.GetIntrin128()), m_10(_10.GetIntrin128()), m_20(_20.GetIntrin128()),
			m_01(_01.GetIntrin128()), m_11(_11.GetIntrin128()), m_21(_21.GetIntrin128()),
			m_02(_02.GetIntrin128()), m_12(_12.GetIntrin128()), m_22(_22.GetIntrin128())
	{
	}

	__forceinline SoA_ScalarV_Out SoA_Mat33V::GetM00() const
	{
		return SoA_ScalarV(m_00);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat33V::GetM01() const
	{
		return SoA_ScalarV(m_01);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat33V::GetM02() const
	{
		return SoA_ScalarV(m_02);
	}
	
	__forceinline SoA_ScalarV_Out SoA_Mat33V::GetM10() const
	{
		return SoA_ScalarV(m_10);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat33V::GetM11() const
	{
		return SoA_ScalarV(m_11);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat33V::GetM12() const
	{
		return SoA_ScalarV(m_12);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat33V::GetM20() const
	{
		return SoA_ScalarV(m_20);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat33V::GetM21() const
	{
		return SoA_ScalarV(m_21);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat33V::GetM22() const
	{
		return SoA_ScalarV(m_22);
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat33V::GetM00Intrin128() const
	{
		return m_00;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat33V::GetM01Intrin128() const
	{
		return m_01;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat33V::GetM02Intrin128() const
	{
		return m_02;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat33V::GetM10Intrin128() const
	{
		return m_10;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat33V::GetM11Intrin128() const
	{
		return m_11;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat33V::GetM12Intrin128() const
	{
		return m_12;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat33V::GetM20Intrin128() const
	{
		return m_20;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat33V::GetM21Intrin128() const
	{
		return m_21;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat33V::GetM22Intrin128() const
	{
		return m_22;
	}

	__forceinline SoA_Vec3V_Out SoA_Mat33V::GetCol0() const
	{
		return SoA_Vec3V( m_00, m_10, m_20 );
	}

	__forceinline SoA_Vec3V_Out SoA_Mat33V::GetCol1() const
	{
		return SoA_Vec3V( m_01, m_11, m_21 );
	}

	__forceinline SoA_Vec3V_Out SoA_Mat33V::GetCol2() const
	{
		return SoA_Vec3V( m_02, m_12, m_22 );
	}

	__forceinline SoA_Vec3V_Out SoA_Mat33V::GetRow0() const
	{
		return SoA_Vec3V( m_00, m_01, m_02 );
	}

	__forceinline SoA_Vec3V_Out SoA_Mat33V::GetRow1() const
	{
		return SoA_Vec3V( m_10, m_11, m_12 );
	}

	__forceinline SoA_Vec3V_Out SoA_Mat33V::GetRow2() const
	{
		return SoA_Vec3V( m_20, m_21, m_22 );
	}

	__forceinline void SoA_Mat33V::SetM00( SoA_ScalarV_In vVal )
	{
		m_00 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat33V::SetM01( SoA_ScalarV_In vVal )
	{
		m_01 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat33V::SetM02( SoA_ScalarV_In vVal )
	{
		m_02 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat33V::SetM10( SoA_ScalarV_In vVal )
	{
		m_10 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat33V::SetM11( SoA_ScalarV_In vVal )
	{
		m_11 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat33V::SetM12( SoA_ScalarV_In vVal )
	{
		m_12 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat33V::SetM20( SoA_ScalarV_In vVal )
	{
		m_20 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat33V::SetM21( SoA_ScalarV_In vVal )
	{
		m_21 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat33V::SetM22( SoA_ScalarV_In vVal )
	{
		m_22 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat33V::SetM00Intrin128( Vec::Vector_4V_In vVal )
	{
		m_00 = vVal;
	}

	__forceinline void SoA_Mat33V::SetM01Intrin128( Vec::Vector_4V_In vVal )
	{
		m_01 = vVal;
	}

	__forceinline void SoA_Mat33V::SetM02Intrin128( Vec::Vector_4V_In vVal )
	{
		m_02 = vVal;
	}

	__forceinline void SoA_Mat33V::SetM10Intrin128( Vec::Vector_4V_In vVal )
	{
		m_10 = vVal;
	}

	__forceinline void SoA_Mat33V::SetM11Intrin128( Vec::Vector_4V_In vVal )
	{
		m_11 = vVal;
	}

	__forceinline void SoA_Mat33V::SetM12Intrin128( Vec::Vector_4V_In vVal )
	{
		m_12 = vVal;
	}

	__forceinline void SoA_Mat33V::SetM20Intrin128( Vec::Vector_4V_In vVal )
	{
		m_20 = vVal;
	}

	__forceinline void SoA_Mat33V::SetM21Intrin128( Vec::Vector_4V_In vVal )
	{
		m_21 = vVal;
	}

	__forceinline void SoA_Mat33V::SetM22Intrin128( Vec::Vector_4V_In vVal )
	{
		m_22 = vVal;
	}

	__forceinline void SoA_Mat33V::SetCol0( SoA_Vec3V_In col0 )
	{
		m_00 = col0.GetXIntrin128();
		m_10 = col0.GetYIntrin128();
		m_20 = col0.GetZIntrin128();
	}

	__forceinline void SoA_Mat33V::SetCol1( SoA_Vec3V_In col1 )
	{
		m_01 = col1.GetXIntrin128();
		m_11 = col1.GetYIntrin128();
		m_21 = col1.GetZIntrin128();
	}

	__forceinline void SoA_Mat33V::SetCol2( SoA_Vec3V_In col2 )
	{
		m_02 = col2.GetXIntrin128();
		m_12 = col2.GetYIntrin128();
		m_22 = col2.GetZIntrin128();
	}

	__forceinline void SoA_Mat33V::SetCols( SoA_Vec3V_In col0, SoA_Vec3V_In col1, SoA_Vec3V_In col2 )
	{
		m_00 = col0.GetXIntrin128();
		m_10 = col0.GetYIntrin128();
		m_20 = col0.GetZIntrin128();
		m_01 = col1.GetXIntrin128();
		m_11 = col1.GetYIntrin128();
		m_21 = col1.GetZIntrin128();
		m_02 = col2.GetXIntrin128();
		m_12 = col2.GetYIntrin128();
		m_22 = col2.GetZIntrin128();
	}

	__forceinline void SoA_Mat33V::SetRow0( SoA_Vec3V_In row0 )
	{
		m_00 = row0.GetXIntrin128();
		m_01 = row0.GetYIntrin128();
		m_02 = row0.GetZIntrin128();
	}

	__forceinline void SoA_Mat33V::SetRow1( SoA_Vec3V_In row1 )
	{
		m_10 = row1.GetXIntrin128();
		m_11 = row1.GetYIntrin128();
		m_12 = row1.GetZIntrin128();
	}

	__forceinline void SoA_Mat33V::SetRow2( SoA_Vec3V_In row2 )
	{
		m_20 = row2.GetXIntrin128();
		m_21 = row2.GetYIntrin128();
		m_22 = row2.GetZIntrin128();
	}

	__forceinline void SoA_Mat33V::SetRows( SoA_Vec3V_In row0, SoA_Vec3V_In row1, SoA_Vec3V_In row2 )
	{
		m_00 = row0.GetXIntrin128();
		m_01 = row0.GetYIntrin128();
		m_02 = row0.GetZIntrin128();
		m_10 = row1.GetXIntrin128();
		m_11 = row1.GetYIntrin128();
		m_12 = row1.GetZIntrin128();
		m_20 = row2.GetXIntrin128();
		m_21 = row2.GetYIntrin128();
		m_22 = row2.GetZIntrin128();
	}

	__forceinline void SoA_Mat33V::SetCols( SoA_Vec3V_In col )
	{
		m_00 = m_01 = m_02 = col.GetXIntrin128();
		m_10 = m_11 = m_12 = col.GetYIntrin128();
		m_20 = m_21 = m_22 = col.GetZIntrin128();
	}

	__forceinline void SoA_Mat33V::SetRows( SoA_Vec3V_In row )
	{
		m_00 = m_10 = m_20 = row.GetXIntrin128();
		m_01 = m_11 = m_21 = row.GetYIntrin128();
		m_02 = m_12 = m_22 = row.GetZIntrin128();
	}

	__forceinline void SoA_Mat33V::SetCol0Intrin128( Vec::Vector_4V_In col0 )
	{
		m_00 = m_10 = m_20 = col0;
	}

	__forceinline void SoA_Mat33V::SetCol1Intrin128( Vec::Vector_4V_In col1 )
	{
		m_01 = m_11 = m_21 = col1;
	}

	__forceinline void SoA_Mat33V::SetCol2Intrin128( Vec::Vector_4V_In col2 )
	{
		m_02 = m_12 = m_22 = col2;
	}

	__forceinline void SoA_Mat33V::SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2 )
	{
		m_00 = m_10 = m_20 = col0;
		m_01 = m_11 = m_21 = col1;
		m_02 = m_12 = m_22 = col2;
	}

	__forceinline void SoA_Mat33V::SetRow0Intrin128( Vec::Vector_4V_In row0 )
	{
		m_00 = m_01 = m_02 = row0;
	}

	__forceinline void SoA_Mat33V::SetRow1Intrin128( Vec::Vector_4V_In row1 )
	{
		m_10 = m_11 = m_12 = row1;
	}

	__forceinline void SoA_Mat33V::SetRow2Intrin128( Vec::Vector_4V_In row2 )
	{
		m_20 = m_21 = m_22 = row2;
	}

	__forceinline void SoA_Mat33V::SetRowsIntrin128( Vec::Vector_4V_In row0, Vec::Vector_4V_In row1, Vec::Vector_4V_In row2 )
	{
		m_00 = m_01 = m_02 = row0;
		m_10 = m_11 = m_12 = row1;
		m_20 = m_21 = m_22 = row2;
	}

	__forceinline void SoA_Mat33V::SetAllIntrin128( Vec::Vector_4V_In vVal )
	{
		m_00=m_10=m_20=m_01=m_11=m_21=m_02=m_12=m_22 = vVal;
	}

	//__forceinline SoA_ScalarV_Out SoA_Mat33V::IsOrthonormalV(SoA_ScalarV_In toleranceSqVect) const
	//{
	//	// The input is the difference in the square of the magnitude (to first order).
	//	// Which, btw, can be approximated by: square(1+tolerance)-1 = about 2*tolerance.
	//	// Thus, if you don't know your tolerance until runtime, square your tolerance.
	//	// If you know your tolerance at compile-time, use the "2*tolerance" approximation to generate a const tol vector.
	//	Vec::Vector_4V sqError = toleranceSqVect.GetIntrin128();
	//	Vec::Vector_4V _negOne = Vec::V4VConstant(V_NEGONE);
	//	
	//	// The m_col0 vector3 has a length close enough to 1.
	//	Vec::Vector_4V cond0 = Vec::V4Abs( Vec::V4Add( _negOne, Vec::V3MagSquaredV(m_col0) ) );
	//	cond0 = Vec::V4IsGreaterThanOrEqualV( sqError, cond0 );

	//	// The m_col1 vector3 has a length close enough to 1.
	//	Vec::Vector_4V cond1 = Vec::V4Abs( Vec::V4Add( _negOne, Vec::V3MagSquaredV(m_col1) ) );
	//	cond1 = Vec::V4IsGreaterThanOrEqualV( sqError, cond1 );

	//	// The m_col2 vector3 has a length close enough to 1.
	//	Vec::Vector_4V cond2 = Vec::V4Abs( Vec::V4Add( _negOne, Vec::V3MagSquaredV(m_col2) ) );
	//	cond2 = Vec::V4IsGreaterThanOrEqualV( sqError, cond2 );

	//	// The m_col0 vector3 is sufficiently perpendicular to the m_col1 vector3.
	//	Vec::Vector_4V cond3 = Vec::V4IsGreaterThanOrEqualV( sqError, Vec::V3DotV( m_col0, m_col1 ) );

	//	// The m_col0 vector3 is sufficiently perpendicular to the m_col2 vector3.
	//	Vec::Vector_4V cond4 = Vec::V4IsGreaterThanOrEqualV( sqError, Vec::V3DotV( m_col0, m_col2 ) );

	//	// The m_col1 vector3 is sufficiently perpendicular to the m_col1 vector3.
	//	Vec::Vector_4V cond5 = Vec::V4IsGreaterThanOrEqualV( sqError, Vec::V3DotV( m_col1, m_col2 ) );

	//	return SoA_ScalarV(
	//				Vec::V4And( cond5,
	//				Vec::V4And( cond4,
	//				Vec::V4And( cond3,
	//				Vec::V4And( cond2,
	//				Vec::V4And( cond1, cond0 ) ) ) ) )
	//			);
	//}

	__forceinline SoA_Mat33V_Out SoA_Mat33V::operator+ (SoA_Mat33V_In b) const
	{
		return SoA_Mat33V(	Vec::V4Add( m_00, b.m_00 ), Vec::V4Add( m_10, b.m_10 ), Vec::V4Add( m_20, b.m_20 ),
						Vec::V4Add( m_01, b.m_01 ), Vec::V4Add( m_11, b.m_11 ), Vec::V4Add( m_21, b.m_21 ),
						Vec::V4Add( m_02, b.m_02 ), Vec::V4Add( m_12, b.m_12 ), Vec::V4Add( m_22, b.m_22 )	);
	}

	__forceinline SoA_Mat33V_Out SoA_Mat33V::operator- (SoA_Mat33V_In b) const
	{
		return SoA_Mat33V(	Vec::V4Subtract( m_00, b.m_00 ), Vec::V4Subtract( m_10, b.m_10 ), Vec::V4Subtract( m_20, b.m_20 ),
						Vec::V4Subtract( m_01, b.m_01 ), Vec::V4Subtract( m_11, b.m_11 ), Vec::V4Subtract( m_21, b.m_21 ),
						Vec::V4Subtract( m_02, b.m_02 ), Vec::V4Subtract( m_12, b.m_12 ), Vec::V4Subtract( m_22, b.m_22 )	);
	}

	__forceinline void SoA_Mat33V::operator+= (SoA_Mat33V_In b)
	{
		m_00 = Vec::V4Add( m_00, b.m_00 );
		m_10 = Vec::V4Add( m_10, b.m_10 );
		m_20 = Vec::V4Add( m_20, b.m_20 );
		m_01 = Vec::V4Add( m_01, b.m_01 );
		m_11 = Vec::V4Add( m_11, b.m_11 );
		m_21 = Vec::V4Add( m_21, b.m_21 );
		m_02 = Vec::V4Add( m_02, b.m_02 );
		m_12 = Vec::V4Add( m_12, b.m_12 );
		m_22 = Vec::V4Add( m_22, b.m_22 );
	}

	__forceinline void SoA_Mat33V::operator-= (SoA_Mat33V_In b)
	{
		m_00 = Vec::V4Subtract( m_00, b.m_00 );
		m_10 = Vec::V4Subtract( m_10, b.m_10 );
		m_20 = Vec::V4Subtract( m_20, b.m_20 );
		m_01 = Vec::V4Subtract( m_01, b.m_01 );
		m_11 = Vec::V4Subtract( m_11, b.m_11 );
		m_21 = Vec::V4Subtract( m_21, b.m_21 );
		m_02 = Vec::V4Subtract( m_02, b.m_02 );
		m_12 = Vec::V4Subtract( m_12, b.m_12 );
		m_22 = Vec::V4Subtract( m_22, b.m_22 );
	}

	__forceinline SoA_Mat33V_Out SoA_Mat33V::operator- () const
	{
		Vec::Vector_4V INT_80000000 = Vec::V4VConstant(V_80000000);
		return SoA_Mat33V(	Vec::V4Xor( m_00, INT_80000000 ), Vec::V4Xor( m_10, INT_80000000 ), Vec::V4Xor( m_20, INT_80000000 ),
						Vec::V4Xor( m_01, INT_80000000 ), Vec::V4Xor( m_11, INT_80000000 ), Vec::V4Xor( m_21, INT_80000000 ),
						Vec::V4Xor( m_02, INT_80000000 ), Vec::V4Xor( m_12, INT_80000000 ), Vec::V4Xor( m_22, INT_80000000 )	);
	}

	__forceinline SoA_Mat33V_Out SoA_Mat33V::operator| (SoA_Mat33V_In b) const
	{
		return SoA_Mat33V(	Vec::V4Or( m_00, b.m_00 ), Vec::V4Or( m_10, b.m_10 ), Vec::V4Or( m_20, b.m_20 ),
						Vec::V4Or( m_01, b.m_01 ), Vec::V4Or( m_11, b.m_11 ), Vec::V4Or( m_21, b.m_21 ),
						Vec::V4Or( m_02, b.m_02 ), Vec::V4Or( m_12, b.m_12 ), Vec::V4Or( m_22, b.m_22 )	);
	}

	__forceinline SoA_Mat33V_Out SoA_Mat33V::operator& (SoA_Mat33V_In b) const
	{
		return SoA_Mat33V(	Vec::V4And( m_00, b.m_00 ), Vec::V4And( m_10, b.m_10 ), Vec::V4And( m_20, b.m_20 ),
						Vec::V4And( m_01, b.m_01 ), Vec::V4And( m_11, b.m_11 ), Vec::V4And( m_21, b.m_21 ),
						Vec::V4And( m_02, b.m_02 ), Vec::V4And( m_12, b.m_12 ), Vec::V4And( m_22, b.m_22 )	);
	}

	__forceinline SoA_Mat33V_Out SoA_Mat33V::operator^ (SoA_Mat33V_In b) const
	{
		return SoA_Mat33V(	Vec::V4Xor( m_00, b.m_00 ), Vec::V4Xor( m_10, b.m_10 ), Vec::V4Xor( m_20, b.m_20 ),
						Vec::V4Xor( m_01, b.m_01 ), Vec::V4Xor( m_11, b.m_11 ), Vec::V4Xor( m_21, b.m_21 ),
						Vec::V4Xor( m_02, b.m_02 ), Vec::V4Xor( m_12, b.m_12 ), Vec::V4Xor( m_22, b.m_22 )	);
	}

	__forceinline void SoA_Mat33V::operator|= (SoA_Mat33V_In b)
	{
		m_00 = Vec::V4Or( m_00, b.m_00 );
		m_10 = Vec::V4Or( m_10, b.m_10 );
		m_20 = Vec::V4Or( m_20, b.m_20 );
		m_01 = Vec::V4Or( m_01, b.m_01 );
		m_11 = Vec::V4Or( m_11, b.m_11 );
		m_21 = Vec::V4Or( m_21, b.m_21 );
		m_02 = Vec::V4Or( m_02, b.m_02 );
		m_12 = Vec::V4Or( m_12, b.m_12 );
		m_22 = Vec::V4Or( m_22, b.m_22 );
	}

	__forceinline void SoA_Mat33V::operator&= (SoA_Mat33V_In b)
	{
		m_00 = Vec::V4And( m_00, b.m_00 );
		m_10 = Vec::V4And( m_10, b.m_10 );
		m_20 = Vec::V4And( m_20, b.m_20 );
		m_01 = Vec::V4And( m_01, b.m_01 );
		m_11 = Vec::V4And( m_11, b.m_11 );
		m_21 = Vec::V4And( m_21, b.m_21 );
		m_02 = Vec::V4And( m_02, b.m_02 );
		m_12 = Vec::V4And( m_12, b.m_12 );
		m_22 = Vec::V4And( m_22, b.m_22 );
	}

	__forceinline void SoA_Mat33V::operator^= (SoA_Mat33V_In b)
	{
		m_00 = Vec::V4Xor( m_00, b.m_00 );
		m_10 = Vec::V4Xor( m_10, b.m_10 );
		m_20 = Vec::V4Xor( m_20, b.m_20 );
		m_01 = Vec::V4Xor( m_01, b.m_01 );
		m_11 = Vec::V4Xor( m_11, b.m_11 );
		m_21 = Vec::V4Xor( m_21, b.m_21 );
		m_02 = Vec::V4Xor( m_02, b.m_02 );
		m_12 = Vec::V4Xor( m_12, b.m_12 );
		m_22 = Vec::V4Xor( m_22, b.m_22 );
	}

	__forceinline Vec::Vector_4V_ConstRef SoA_Mat33V::operator[] (u32 elem) const
	{
		VecAssertMsg( elem <= 8 , "Invalid element index." );
		return (&m_00)[elem];
	}

	__forceinline Vec::Vector_4V_Ref SoA_Mat33V::operator[] (u32 elem)
	{
		VecAssertMsg( elem <= 8 , "Invalid element index." );
		return (&m_00)[elem];
	}
} // namespace rage
