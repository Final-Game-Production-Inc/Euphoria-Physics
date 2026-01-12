
namespace rage
{
	__forceinline SoA_Mat44V::SoA_Mat44V(eZEROInitializer)
	{
		m_00=m_10=m_20=m_30=m_01=m_11=m_21=m_31=m_02=m_12=m_22=m_32=m_03=m_13=m_23=m_33= Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_Mat44V::SoA_Mat44V(eIDENTITYInitializer)
	{
		m_00 = m_11 = m_22 = m_33 = Vec::V4VConstant(V_ONE);
		m_10=m_20=m_30=m_01=m_21=m_31=m_02=m_12=m_32=m_03=m_13=m_23= Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_Mat44V::SoA_Mat44V(eFLT_EPSInitializer)
	{
		m_00=m_10=m_20=m_30=m_01=m_11=m_21=m_31=m_02=m_12=m_22=m_32=m_03=m_13=m_23=m_33= Vec::V4VConstant(V_FLT_EPSILON);
	}

	__forceinline SoA_Mat44V::SoA_Mat44V()
	{
	}

	__forceinline SoA_Mat44V::SoA_Mat44V(SoA_Mat44V_ConstRef a)
		:	m_00(a.m_00), m_10( a.m_10 ), m_20( a.m_20 ), m_30( a.m_30 ),
			m_01(a.m_01), m_11( a.m_11 ), m_21( a.m_21 ), m_31( a.m_31 ),
			m_02(a.m_02), m_12( a.m_12 ), m_22( a.m_22 ), m_32( a.m_32 ),
			m_03(a.m_03), m_13( a.m_13 ), m_23( a.m_23 ), m_33( a.m_33 )
	{
	}

	__forceinline SoA_Mat44V_Ref SoA_Mat44V::operator= (SoA_Mat44V_ConstRef a)
	{
		m_00 = a.m_00; m_10 = a.m_10; m_20 = a.m_20; m_30 = a.m_30;
		m_01 = a.m_01; m_11 = a.m_11; m_21 = a.m_21; m_31 = a.m_31;
		m_02 = a.m_02; m_12 = a.m_12; m_22 = a.m_22; m_32 = a.m_32;
		m_03 = a.m_03; m_13 = a.m_13; m_23 = a.m_23; m_33 = a.m_33;
		return *this;
	}

	//__forceinline SoA_Mat44V::SoA_Mat44V(const float& a)
	//{
	//	Vec::V4Set( m_00, a );
	//	m_10=m_20=m_30=m_01=m_11=m_21=m_31=m_02=m_12=m_22=m_32=m_03=m_13=m_23=m_33=m_00;
	//}

	__forceinline SoA_Mat44V::SoA_Mat44V(	const float& f00, const float& f10, const float& f20, const float& f30,
									const float& f01, const float& f11, const float& f21, const float& f31,
									const float& f02, const float& f12, const float& f22, const float& f32,
									const float& f03, const float& f13, const float& f23, const float& f33	)
	{
		Vec::V4Set( m_00, f00 );
		Vec::V4Set( m_10, f10 );
		Vec::V4Set( m_20, f20 );
		Vec::V4Set( m_30, f30 );
		Vec::V4Set( m_01, f01 );
		Vec::V4Set( m_11, f11 );
		Vec::V4Set( m_21, f21 );
		Vec::V4Set( m_31, f31 );
		Vec::V4Set( m_02, f02 );
		Vec::V4Set( m_12, f12 );
		Vec::V4Set( m_22, f22 );
		Vec::V4Set( m_32, f32 );
		Vec::V4Set( m_03, f03 );
		Vec::V4Set( m_13, f13 );
		Vec::V4Set( m_23, f23 );
		Vec::V4Set( m_33, f33 );
	}


	__forceinline SoA_Mat44V::SoA_Mat44V(	eCOL_MAJORInitializer,
		const float& f00, const float& f10, const float& f20, const float& f30,
		const float& f01, const float& f11, const float& f21, const float& f31,
		const float& f02, const float& f12, const float& f22, const float& f32,
		const float& f03, const float& f13, const float& f23, const float& f33	)
	{
		Vec::V4Set( m_00, f00 );
		Vec::V4Set( m_10, f10 );
		Vec::V4Set( m_20, f20 );
		Vec::V4Set( m_30, f30 );
		Vec::V4Set( m_01, f01 );
		Vec::V4Set( m_11, f11 );
		Vec::V4Set( m_21, f21 );
		Vec::V4Set( m_31, f31 );
		Vec::V4Set( m_02, f02 );
		Vec::V4Set( m_12, f12 );
		Vec::V4Set( m_22, f22 );
		Vec::V4Set( m_32, f32 );
		Vec::V4Set( m_03, f03 );
		Vec::V4Set( m_13, f13 );
		Vec::V4Set( m_23, f23 );
		Vec::V4Set( m_33, f33 );
	}


	__forceinline SoA_Mat44V::SoA_Mat44V(	eROW_MAJORInitializer,
		const float& f00, const float& f01, const float& f02, const float& f03,
		const float& f10, const float& f11, const float& f12, const float& f13,
		const float& f20, const float& f21, const float& f22, const float& f23,
		const float& f30, const float& f31, const float& f32, const float& f33	)
	{
		Vec::V4Set( m_00, f00 );
		Vec::V4Set( m_10, f10 );
		Vec::V4Set( m_20, f20 );
		Vec::V4Set( m_30, f30 );
		Vec::V4Set( m_01, f01 );
		Vec::V4Set( m_11, f11 );
		Vec::V4Set( m_21, f21 );
		Vec::V4Set( m_31, f31 );
		Vec::V4Set( m_02, f02 );
		Vec::V4Set( m_12, f12 );
		Vec::V4Set( m_22, f22 );
		Vec::V4Set( m_32, f32 );
		Vec::V4Set( m_03, f03 );
		Vec::V4Set( m_13, f13 );
		Vec::V4Set( m_23, f23 );
		Vec::V4Set( m_33, f33 );
	}

	__forceinline SoA_Mat44V::SoA_Mat44V(Vec::Vector_4V_In a)
	{
		m_00=m_10=m_20=m_30=m_01=m_11=m_21=m_31=m_02=m_12=m_22=m_32=m_03=m_13=m_23=m_33= a;
	}

	__forceinline SoA_Mat44V::SoA_Mat44V(	Vec::Vector_4V_In _00,Vec::Vector_4V_In _10,Vec::Vector_4V_In _20,Vec::Vector_4V_In_After3Args _30,
									Vec::Vector_4V_In_After3Args _01,Vec::Vector_4V_In_After3Args _11,Vec::Vector_4V_In_After3Args _21,Vec::Vector_4V_In_After3Args _31,
									Vec::Vector_4V_In_After3Args _02,Vec::Vector_4V_In_After3Args _12,Vec::Vector_4V_In_After3Args _22,Vec::Vector_4V_In_After3Args _32,
									Vec::Vector_4V_In_After3Args _03,Vec::Vector_4V_In_After3Args _13,Vec::Vector_4V_In_After3Args _23,Vec::Vector_4V_In_After3Args _33	)
		:	m_00(_00), m_10(_10), m_20(_20), m_30(_30),
			m_01(_01), m_11(_11), m_21(_21), m_31(_31),
			m_02(_02), m_12(_12), m_22(_22), m_32(_32),
			m_03(_03), m_13(_13), m_23(_23), m_33(_33)
	{
	}

	__forceinline SoA_Mat44V::SoA_Mat44V(SoA_Vec4V_In col)
	{
		m_00 = m_01 = m_02 = m_03 = col.GetXIntrin128();
		m_10 = m_11 = m_12 = m_13 = col.GetYIntrin128();
		m_20 = m_21 = m_22 = m_23 = col.GetZIntrin128();
		m_30 = m_31 = m_32 = m_33 = col.GetWIntrin128();
	}

	__forceinline SoA_Mat44V::SoA_Mat44V(SoA_Vec4V_In col0, SoA_Vec4V_In col1, SoA_Vec4V_In col2, SoA_Vec4V_In col3)
		:	m_00(col0.GetXIntrin128()), m_10(col0.GetYIntrin128()), m_20(col0.GetZIntrin128()), m_30(col0.GetWIntrin128()),
			m_01(col1.GetXIntrin128()), m_11(col1.GetYIntrin128()), m_21(col1.GetZIntrin128()), m_31(col1.GetWIntrin128()),
			m_02(col2.GetXIntrin128()), m_12(col2.GetYIntrin128()), m_22(col2.GetZIntrin128()), m_32(col2.GetWIntrin128()),
			m_03(col3.GetXIntrin128()), m_13(col3.GetYIntrin128()), m_23(col3.GetZIntrin128()), m_33(col3.GetWIntrin128())
	{
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM00() const
	{
		return SoA_ScalarV(m_00);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM01() const
	{
		return SoA_ScalarV(m_01);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM02() const
	{
		return SoA_ScalarV(m_02);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM03() const
	{
		return SoA_ScalarV(m_03);
	}
	
	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM10() const
	{
		return SoA_ScalarV(m_10);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM11() const
	{
		return SoA_ScalarV(m_11);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM12() const
	{
		return SoA_ScalarV(m_12);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM13() const
	{
		return SoA_ScalarV(m_13);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM20() const
	{
		return SoA_ScalarV(m_20);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM21() const
	{
		return SoA_ScalarV(m_21);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM22() const
	{
		return SoA_ScalarV(m_22);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM23() const
	{
		return SoA_ScalarV(m_23);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM30() const
	{
		return SoA_ScalarV(m_30);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM31() const
	{
		return SoA_ScalarV(m_31);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM32() const
	{
		return SoA_ScalarV(m_32);
	}

	__forceinline SoA_ScalarV_Out SoA_Mat44V::GetM33() const
	{
		return SoA_ScalarV(m_33);
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM00Intrin128() const
	{
		return m_00;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM01Intrin128() const
	{
		return m_01;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM02Intrin128() const
	{
		return m_02;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM03Intrin128() const
	{
		return m_03;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM10Intrin128() const
	{
		return m_10;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM11Intrin128() const
	{
		return m_11;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM12Intrin128() const
	{
		return m_12;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM13Intrin128() const
	{
		return m_13;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM20Intrin128() const
	{
		return m_20;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM21Intrin128() const
	{
		return m_21;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM22Intrin128() const
	{
		return m_22;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM23Intrin128() const
	{
		return m_23;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM30Intrin128() const
	{
		return m_30;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM31Intrin128() const
	{
		return m_31;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM32Intrin128() const
	{
		return m_32;
	}

	__forceinline Vec::Vector_4V_Out SoA_Mat44V::GetM33Intrin128() const
	{
		return m_33;
	}

	__forceinline SoA_Vec4V_Out SoA_Mat44V::GetCol0() const
	{
		return SoA_Vec4V( m_00, m_10, m_20, m_30 );
	}

	__forceinline SoA_Vec4V_Out SoA_Mat44V::GetCol1() const
	{
		return SoA_Vec4V( m_01, m_11, m_21, m_31 );
	}

	__forceinline SoA_Vec4V_Out SoA_Mat44V::GetCol2() const
	{
		return SoA_Vec4V( m_02, m_12, m_22, m_32 );
	}

	__forceinline SoA_Vec4V_Out SoA_Mat44V::GetCol3() const
	{
		return SoA_Vec4V( m_03, m_13, m_23, m_33 );
	}

	__forceinline SoA_Vec4V_Out SoA_Mat44V::GetRow0() const
	{
		return SoA_Vec4V( m_00, m_01, m_02, m_03 );
	}

	__forceinline SoA_Vec4V_Out SoA_Mat44V::GetRow1() const
	{
		return SoA_Vec4V( m_10, m_11, m_12, m_13 );
	}

	__forceinline SoA_Vec4V_Out SoA_Mat44V::GetRow2() const
	{
		return SoA_Vec4V( m_20, m_21, m_22, m_23 );
	}

	__forceinline SoA_Vec4V_Out SoA_Mat44V::GetRow3() const
	{
		return SoA_Vec4V( m_30, m_31, m_32, m_33 );
	}

	__forceinline void SoA_Mat44V::SetM00( SoA_ScalarV_In vVal )
	{
		m_00 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM01( SoA_ScalarV_In vVal )
	{
		m_01 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM02( SoA_ScalarV_In vVal )
	{
		m_02 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM03( SoA_ScalarV_In vVal )
	{
		m_03 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM10( SoA_ScalarV_In vVal )
	{
		m_10 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM11( SoA_ScalarV_In vVal )
	{
		m_11 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM12( SoA_ScalarV_In vVal )
	{
		m_12 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM13( SoA_ScalarV_In vVal )
	{
		m_13 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM20( SoA_ScalarV_In vVal )
	{
		m_20 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM21( SoA_ScalarV_In vVal )
	{
		m_21 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM22( SoA_ScalarV_In vVal )
	{
		m_22 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM23( SoA_ScalarV_In vVal )
	{
		m_23 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM30( SoA_ScalarV_In vVal )
	{
		m_30 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM31( SoA_ScalarV_In vVal )
	{
		m_31 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM32( SoA_ScalarV_In vVal )
	{
		m_32 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM33( SoA_ScalarV_In vVal )
	{
		m_33 = vVal.GetIntrin128();
	}

	__forceinline void SoA_Mat44V::SetM00Intrin128( Vec::Vector_4V_In vVal )
	{
		m_00 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM01Intrin128( Vec::Vector_4V_In vVal )
	{
		m_01 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM02Intrin128( Vec::Vector_4V_In vVal )
	{
		m_02 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM03Intrin128( Vec::Vector_4V_In vVal )
	{
		m_03 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM10Intrin128( Vec::Vector_4V_In vVal )
	{
		m_10 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM11Intrin128( Vec::Vector_4V_In vVal )
	{
		m_11 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM12Intrin128( Vec::Vector_4V_In vVal )
	{
		m_12 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM13Intrin128( Vec::Vector_4V_In vVal )
	{
		m_13 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM20Intrin128( Vec::Vector_4V_In vVal )
	{
		m_20 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM21Intrin128( Vec::Vector_4V_In vVal )
	{
		m_21 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM22Intrin128( Vec::Vector_4V_In vVal )
	{
		m_22 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM23Intrin128( Vec::Vector_4V_In vVal )
	{
		m_23 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM30Intrin128( Vec::Vector_4V_In vVal )
	{
		m_30 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM31Intrin128( Vec::Vector_4V_In vVal )
	{
		m_31 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM32Intrin128( Vec::Vector_4V_In vVal )
	{
		m_32 = vVal;
	}

	__forceinline void SoA_Mat44V::SetM33Intrin128( Vec::Vector_4V_In vVal )
	{
		m_33 = vVal;
	}

	__forceinline void SoA_Mat44V::SetCol0( SoA_Vec4V_In col0 )
	{
		m_00 = col0.GetXIntrin128();
		m_10 = col0.GetYIntrin128();
		m_20 = col0.GetZIntrin128();
		m_30 = col0.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetCol1( SoA_Vec4V_In col1 )
	{
		m_01 = col1.GetXIntrin128();
		m_11 = col1.GetYIntrin128();
		m_21 = col1.GetZIntrin128();
		m_31 = col1.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetCol2( SoA_Vec4V_In col2 )
	{
		m_02 = col2.GetXIntrin128();
		m_12 = col2.GetYIntrin128();
		m_22 = col2.GetZIntrin128();
		m_32 = col2.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetCol3( SoA_Vec4V_In col3 )
	{
		m_03 = col3.GetXIntrin128();
		m_13 = col3.GetYIntrin128();
		m_23 = col3.GetZIntrin128();
		m_33 = col3.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetCols( SoA_Vec4V_In col0, SoA_Vec4V_In col1, SoA_Vec4V_In col2, SoA_Vec4V_In col3 )
	{
		m_00 = col0.GetXIntrin128();
		m_10 = col0.GetYIntrin128();
		m_20 = col0.GetZIntrin128();
		m_30 = col0.GetWIntrin128();
		m_01 = col1.GetXIntrin128();
		m_11 = col1.GetYIntrin128();
		m_21 = col1.GetZIntrin128();
		m_31 = col1.GetWIntrin128();
		m_02 = col2.GetXIntrin128();
		m_12 = col2.GetYIntrin128();
		m_22 = col2.GetZIntrin128();
		m_32 = col2.GetWIntrin128();
		m_03 = col3.GetXIntrin128();
		m_13 = col3.GetYIntrin128();
		m_23 = col3.GetZIntrin128();
		m_33 = col3.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetRow0( SoA_Vec4V_In row0 )
	{
		m_00 = row0.GetXIntrin128();
		m_01 = row0.GetYIntrin128();
		m_02 = row0.GetZIntrin128();
		m_03 = row0.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetRow1( SoA_Vec4V_In row1 )
	{
		m_10 = row1.GetXIntrin128();
		m_11 = row1.GetYIntrin128();
		m_12 = row1.GetZIntrin128();
		m_13 = row1.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetRow2( SoA_Vec4V_In row2 )
	{
		m_20 = row2.GetXIntrin128();
		m_21 = row2.GetYIntrin128();
		m_22 = row2.GetZIntrin128();
		m_23 = row2.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetRow3( SoA_Vec4V_In row3 )
	{
		m_30 = row3.GetXIntrin128();
		m_31 = row3.GetYIntrin128();
		m_32 = row3.GetZIntrin128();
		m_33 = row3.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetRows( SoA_Vec4V_In row0, SoA_Vec4V_In row1, SoA_Vec4V_In row2, SoA_Vec4V_In row3 )
	{
		m_00 = row0.GetXIntrin128();
		m_01 = row0.GetYIntrin128();
		m_02 = row0.GetZIntrin128();
		m_03 = row0.GetWIntrin128();
		m_10 = row1.GetXIntrin128();
		m_11 = row1.GetYIntrin128();
		m_12 = row1.GetZIntrin128();
		m_13 = row1.GetWIntrin128();
		m_20 = row2.GetXIntrin128();
		m_21 = row2.GetYIntrin128();
		m_22 = row2.GetZIntrin128();
		m_23 = row2.GetWIntrin128();
		m_30 = row3.GetXIntrin128();
		m_31 = row3.GetYIntrin128();
		m_32 = row3.GetZIntrin128();
		m_33 = row3.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetCols( SoA_Vec4V_In col )
	{
		m_00 = m_01 = m_02 = m_03 = col.GetXIntrin128();
		m_10 = m_11 = m_12 = m_13 = col.GetYIntrin128();
		m_20 = m_21 = m_22 = m_23 = col.GetZIntrin128();
		m_30 = m_31 = m_32 = m_33 = col.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetRows( SoA_Vec4V_In row )
	{
		m_00 = m_10 = m_20 = m_30 = row.GetXIntrin128();
		m_01 = m_11 = m_21 = m_31 = row.GetYIntrin128();
		m_02 = m_12 = m_22 = m_32 = row.GetZIntrin128();
		m_03 = m_13 = m_23 = m_33 = row.GetWIntrin128();
	}

	__forceinline void SoA_Mat44V::SetCol0Intrin128( Vec::Vector_4V_In col0 )
	{
		m_00 = m_10 = m_20 = m_30 = col0;
	}

	__forceinline void SoA_Mat44V::SetCol1Intrin128( Vec::Vector_4V_In col1 )
	{
		m_01 = m_11 = m_21 = m_31 = col1;
	}

	__forceinline void SoA_Mat44V::SetCol2Intrin128( Vec::Vector_4V_In col2 )
	{
		m_02 = m_12 = m_22 = m_32 = col2;
	}

	__forceinline void SoA_Mat44V::SetCol3Intrin128( Vec::Vector_4V_In col3 )
	{
		m_03 = m_13 = m_23 = m_33 = col3;
	}

	__forceinline void SoA_Mat44V::SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2, Vec::Vector_4V_In_After3Args col3 )
	{
		m_00 = m_10 = m_20 = m_30 = col0;
		m_01 = m_11 = m_21 = m_31 = col1;
		m_02 = m_12 = m_22 = m_32 = col2;
		m_03 = m_13 = m_23 = m_33 = col3;
	}

	__forceinline void SoA_Mat44V::SetRow0Intrin128( Vec::Vector_4V_In row0 )
	{
		m_00 = m_01 = m_02 = m_03 = row0;
	}

	__forceinline void SoA_Mat44V::SetRow1Intrin128( Vec::Vector_4V_In row1 )
	{
		m_10 = m_11 = m_12 = m_13 = row1;
	}

	__forceinline void SoA_Mat44V::SetRow2Intrin128( Vec::Vector_4V_In row2 )
	{
		m_20 = m_21 = m_22 = m_23 = row2;
	}

	__forceinline void SoA_Mat44V::SetRow3Intrin128( Vec::Vector_4V_In row3 )
	{
		m_30 = m_31 = m_32 = m_33 = row3;
	}

	__forceinline void SoA_Mat44V::SetRowsIntrin128( Vec::Vector_4V_In row0, Vec::Vector_4V_In row1, Vec::Vector_4V_In row2, Vec::Vector_4V_In_After3Args row3 )
	{
		m_00 = m_01 = m_02 = m_03 = row0;
		m_10 = m_11 = m_12 = m_13 = row1;
		m_20 = m_21 = m_22 = m_23 = row2;
		m_30 = m_31 = m_32 = m_33 = row3;
	}

	__forceinline void SoA_Mat44V::SetAllIntrin128( Vec::Vector_4V_In vVal )
	{
		m_00=m_10=m_20=m_30=m_01=m_11=m_21=m_31=m_02=m_12=m_22=m_32=m_03=m_13=m_23=m_33= vVal;
	}

	//__forceinline SoA_ScalarV_Out SoA_Mat44V::IsOrthonormalV(SoA_ScalarV_In toleranceSqVect) const
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

	__forceinline SoA_Mat44V_Out SoA_Mat44V::operator+ (SoA_Mat44V_In b) const
	{
		return SoA_Mat44V(	Vec::V4Add( m_00, b.m_00 ), Vec::V4Add( m_10, b.m_10 ), Vec::V4Add( m_20, b.m_20 ), Vec::V4Add( m_30, b.m_30 ),
						Vec::V4Add( m_01, b.m_01 ), Vec::V4Add( m_11, b.m_11 ), Vec::V4Add( m_21, b.m_21 ), Vec::V4Add( m_31, b.m_31 ),
						Vec::V4Add( m_02, b.m_02 ), Vec::V4Add( m_12, b.m_12 ), Vec::V4Add( m_22, b.m_22 ), Vec::V4Add( m_32, b.m_32 ),
						Vec::V4Add( m_03, b.m_03 ), Vec::V4Add( m_13, b.m_13 ), Vec::V4Add( m_23, b.m_23 ), Vec::V4Add( m_33, b.m_33 )	);
	}

	__forceinline SoA_Mat44V_Out SoA_Mat44V::operator- (SoA_Mat44V_In b) const
	{
		return SoA_Mat44V(	Vec::V4Subtract( m_00, b.m_00 ), Vec::V4Subtract( m_10, b.m_10 ), Vec::V4Subtract( m_20, b.m_20 ), Vec::V4Subtract( m_30, b.m_30 ),
						Vec::V4Subtract( m_01, b.m_01 ), Vec::V4Subtract( m_11, b.m_11 ), Vec::V4Subtract( m_21, b.m_21 ), Vec::V4Subtract( m_31, b.m_31 ),
						Vec::V4Subtract( m_02, b.m_02 ), Vec::V4Subtract( m_12, b.m_12 ), Vec::V4Subtract( m_22, b.m_22 ), Vec::V4Subtract( m_32, b.m_32 ),
						Vec::V4Subtract( m_03, b.m_03 ), Vec::V4Subtract( m_13, b.m_13 ), Vec::V4Subtract( m_23, b.m_23 ), Vec::V4Subtract( m_33, b.m_33 )	);
	}

	__forceinline void SoA_Mat44V::operator+= (SoA_Mat44V_In b)
	{
		m_00 = Vec::V4Add( m_00, b.m_00 );
		m_10 = Vec::V4Add( m_10, b.m_10 );
		m_20 = Vec::V4Add( m_20, b.m_20 );
		m_30 = Vec::V4Add( m_30, b.m_30 );
		m_01 = Vec::V4Add( m_01, b.m_01 );
		m_11 = Vec::V4Add( m_11, b.m_11 );
		m_21 = Vec::V4Add( m_21, b.m_21 );
		m_31 = Vec::V4Add( m_31, b.m_31 );
		m_02 = Vec::V4Add( m_02, b.m_02 );
		m_12 = Vec::V4Add( m_12, b.m_12 );
		m_22 = Vec::V4Add( m_22, b.m_22 );
		m_32 = Vec::V4Add( m_32, b.m_32 );
		m_03 = Vec::V4Add( m_03, b.m_03 );
		m_13 = Vec::V4Add( m_13, b.m_13 );
		m_23 = Vec::V4Add( m_23, b.m_23 );
		m_33 = Vec::V4Add( m_33, b.m_33 );
	}

	__forceinline void SoA_Mat44V::operator-= (SoA_Mat44V_In b)
	{
		m_00 = Vec::V4Subtract( m_00, b.m_00 );
		m_10 = Vec::V4Subtract( m_10, b.m_10 );
		m_20 = Vec::V4Subtract( m_20, b.m_20 );
		m_30 = Vec::V4Subtract( m_30, b.m_30 );
		m_01 = Vec::V4Subtract( m_01, b.m_01 );
		m_11 = Vec::V4Subtract( m_11, b.m_11 );
		m_21 = Vec::V4Subtract( m_21, b.m_21 );
		m_31 = Vec::V4Subtract( m_31, b.m_31 );
		m_02 = Vec::V4Subtract( m_02, b.m_02 );
		m_12 = Vec::V4Subtract( m_12, b.m_12 );
		m_22 = Vec::V4Subtract( m_22, b.m_22 );
		m_32 = Vec::V4Subtract( m_32, b.m_32 );
		m_03 = Vec::V4Subtract( m_03, b.m_03 );
		m_13 = Vec::V4Subtract( m_13, b.m_13 );
		m_23 = Vec::V4Subtract( m_23, b.m_23 );
		m_33 = Vec::V4Subtract( m_33, b.m_33 );
	}

	__forceinline SoA_Mat44V_Out SoA_Mat44V::operator- () const
	{
		Vec::Vector_4V INT_80000000 = Vec::V4VConstant(V_80000000);
		return SoA_Mat44V(	Vec::V4Xor( m_00, INT_80000000 ), Vec::V4Xor( m_10, INT_80000000 ), Vec::V4Xor( m_20, INT_80000000 ), Vec::V4Xor( m_30, INT_80000000 ),
						Vec::V4Xor( m_01, INT_80000000 ), Vec::V4Xor( m_11, INT_80000000 ), Vec::V4Xor( m_21, INT_80000000 ), Vec::V4Xor( m_31, INT_80000000 ),
						Vec::V4Xor( m_02, INT_80000000 ), Vec::V4Xor( m_12, INT_80000000 ), Vec::V4Xor( m_22, INT_80000000 ), Vec::V4Xor( m_32, INT_80000000 ),
						Vec::V4Xor( m_03, INT_80000000 ), Vec::V4Xor( m_13, INT_80000000 ), Vec::V4Xor( m_23, INT_80000000 ), Vec::V4Xor( m_33, INT_80000000 )	);
	}

	__forceinline SoA_Mat44V_Out SoA_Mat44V::operator| (SoA_Mat44V_In b) const
	{
		return SoA_Mat44V(	Vec::V4Or( m_00, b.m_00 ), Vec::V4Or( m_10, b.m_10 ), Vec::V4Or( m_20, b.m_20 ), Vec::V4Or( m_30, b.m_30 ),
						Vec::V4Or( m_01, b.m_01 ), Vec::V4Or( m_11, b.m_11 ), Vec::V4Or( m_21, b.m_21 ), Vec::V4Or( m_31, b.m_31 ),
						Vec::V4Or( m_02, b.m_02 ), Vec::V4Or( m_12, b.m_12 ), Vec::V4Or( m_22, b.m_22 ), Vec::V4Or( m_32, b.m_32 ),
						Vec::V4Or( m_03, b.m_03 ), Vec::V4Or( m_13, b.m_13 ), Vec::V4Or( m_23, b.m_23 ), Vec::V4Or( m_33, b.m_33 )	);
	}

	__forceinline SoA_Mat44V_Out SoA_Mat44V::operator& (SoA_Mat44V_In b) const
	{
		return SoA_Mat44V(	Vec::V4And( m_00, b.m_00 ), Vec::V4And( m_10, b.m_10 ), Vec::V4And( m_20, b.m_20 ), Vec::V4And( m_30, b.m_30 ),
						Vec::V4And( m_01, b.m_01 ), Vec::V4And( m_11, b.m_11 ), Vec::V4And( m_21, b.m_21 ), Vec::V4And( m_31, b.m_31 ),
						Vec::V4And( m_02, b.m_02 ), Vec::V4And( m_12, b.m_12 ), Vec::V4And( m_22, b.m_22 ), Vec::V4And( m_32, b.m_32 ),
						Vec::V4And( m_03, b.m_03 ), Vec::V4And( m_13, b.m_13 ), Vec::V4And( m_23, b.m_23 ), Vec::V4And( m_33, b.m_33 )	);
	}

	__forceinline SoA_Mat44V_Out SoA_Mat44V::operator^ (SoA_Mat44V_In b) const
	{
		return SoA_Mat44V(	Vec::V4Xor( m_00, b.m_00 ), Vec::V4Xor( m_10, b.m_10 ), Vec::V4Xor( m_20, b.m_20 ), Vec::V4Xor( m_30, b.m_30 ),
						Vec::V4Xor( m_01, b.m_01 ), Vec::V4Xor( m_11, b.m_11 ), Vec::V4Xor( m_21, b.m_21 ), Vec::V4Xor( m_31, b.m_31 ),
						Vec::V4Xor( m_02, b.m_02 ), Vec::V4Xor( m_12, b.m_12 ), Vec::V4Xor( m_22, b.m_22 ), Vec::V4Xor( m_32, b.m_32 ),
						Vec::V4Xor( m_03, b.m_03 ), Vec::V4Xor( m_13, b.m_13 ), Vec::V4Xor( m_23, b.m_23 ), Vec::V4Xor( m_33, b.m_33 )	);
	}

	__forceinline void SoA_Mat44V::operator|= (SoA_Mat44V_In b)
	{
		m_00 = Vec::V4Or( m_00, b.m_00 );
		m_10 = Vec::V4Or( m_10, b.m_10 );
		m_20 = Vec::V4Or( m_20, b.m_20 );
		m_30 = Vec::V4Or( m_30, b.m_30 );
		m_01 = Vec::V4Or( m_01, b.m_01 );
		m_11 = Vec::V4Or( m_11, b.m_11 );
		m_21 = Vec::V4Or( m_21, b.m_21 );
		m_31 = Vec::V4Or( m_31, b.m_31 );
		m_02 = Vec::V4Or( m_02, b.m_02 );
		m_12 = Vec::V4Or( m_12, b.m_12 );
		m_22 = Vec::V4Or( m_22, b.m_22 );
		m_32 = Vec::V4Or( m_32, b.m_32 );
		m_03 = Vec::V4Or( m_03, b.m_03 );
		m_13 = Vec::V4Or( m_13, b.m_13 );
		m_23 = Vec::V4Or( m_23, b.m_23 );
		m_33 = Vec::V4Or( m_33, b.m_33 );
	}

	__forceinline void SoA_Mat44V::operator&= (SoA_Mat44V_In b)
	{
		m_00 = Vec::V4And( m_00, b.m_00 );
		m_10 = Vec::V4And( m_10, b.m_10 );
		m_20 = Vec::V4And( m_20, b.m_20 );
		m_30 = Vec::V4And( m_30, b.m_30 );
		m_01 = Vec::V4And( m_01, b.m_01 );
		m_11 = Vec::V4And( m_11, b.m_11 );
		m_21 = Vec::V4And( m_21, b.m_21 );
		m_31 = Vec::V4And( m_31, b.m_31 );
		m_02 = Vec::V4And( m_02, b.m_02 );
		m_12 = Vec::V4And( m_12, b.m_12 );
		m_22 = Vec::V4And( m_22, b.m_22 );
		m_32 = Vec::V4And( m_32, b.m_32 );
		m_03 = Vec::V4And( m_03, b.m_03 );
		m_13 = Vec::V4And( m_13, b.m_13 );
		m_23 = Vec::V4And( m_23, b.m_23 );
		m_33 = Vec::V4And( m_33, b.m_33 );
	}

	__forceinline void SoA_Mat44V::operator^= (SoA_Mat44V_In b)
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
		m_03 = Vec::V4Xor( m_03, b.m_03 );
		m_13 = Vec::V4Xor( m_13, b.m_13 );
		m_23 = Vec::V4Xor( m_23, b.m_23 );
	}

	__forceinline Vec::Vector_4V_ConstRef SoA_Mat44V::operator[] (u32 elem) const
	{
		VecAssertMsg( elem <= 15 , "Invalid element index." );
		return (&m_00)[elem];
	}

	__forceinline Vec::Vector_4V_Ref SoA_Mat44V::operator[] (u32 elem)
	{
		VecAssertMsg( elem <= 15 , "Invalid element index." );
		return (&m_00)[elem];
	}

} // namespace rage
