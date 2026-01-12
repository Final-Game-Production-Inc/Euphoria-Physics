
// Note: Everything here must be __forceinline'd, since *this will not be
// passed in vector registers! Force-inlining is okay here, since these functions
// are simple calls to free functions (which are only inline'd, not forced, but
// always pass via vector registers when not inlined).

namespace rage
{
	__forceinline Mat44V::Mat44V(eZEROInitializer)
	{
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		m_col0 = m_col1 = m_col2 = m_col3 = _zero;
	}

	__forceinline Mat44V::Mat44V(eIDENTITYInitializer)
	{
#if __XENON && UNIQUE_VECTORIZED_TYPE // take advantage of __vupkd3d
		Vec::Vector_4V zeroInZ_oneInW = __vupkd3d(Vec::V4VConstant(V_ZERO), VPACK_NORMSHORT2);
		m_col0 = Vec::V4Permute<Vec::W,Vec::Z,Vec::Z,Vec::Z>( zeroInZ_oneInW );
		m_col1 = Vec::V4Permute<Vec::Z,Vec::W,Vec::Z,Vec::Z>( zeroInZ_oneInW );
		m_col2 = Vec::V4Permute<Vec::Z,Vec::Z,Vec::W,Vec::Z>( zeroInZ_oneInW );
		m_col3 = Vec::V4Permute<Vec::Z,Vec::Z,Vec::Z,Vec::W>( zeroInZ_oneInW );
#else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V _0001 = Vec::V4VConstant(V_ZERO_WONE);
		Vec::Vector_4V _0010 = Vec::V4MergeZW( _0001, _zero );
		Vec::Vector_4V _0100 = Vec::V4MergeZW( _zero, _0010 );
		Vec::Vector_4V _1000 = Vec::V4MergeZW( _0010, _zero );
		m_col0 = _1000;
		m_col1 = _0100;
		m_col2 = _0010;
		m_col3 = _0001;
#endif
	}

	__forceinline Mat44V::Mat44V(eFLT_EPSILONInitializer)
	{
		Vec::Vector_4V _fltSmall = Vec::V4VConstant(V_FLT_EPSILON);
		m_col0 = m_col1 = m_col2 = m_col3 = _fltSmall;
	}

	__forceinline Mat44V::Mat44V()
	{
	}

	__forceinline Mat44V::Mat44V(Mat44V_ConstRef other)
		:	m_col0( other.m_col0 ),
			m_col1( other.m_col1 ),
			m_col2( other.m_col2 ),
			m_col3( other.m_col3 )
	{
	}

	__forceinline Mat44V_ConstRef Mat44V::operator= (Mat44V_ConstRef other)
	{
		m_col0 = other.m_col0;
		m_col1 = other.m_col1;
		m_col2 = other.m_col2;
		m_col3 = other.m_col3;
		return *this;
	}


	__forceinline Mat44V::Mat44V(	const float& f00, const float& f10, const float& f20, const float& f30,
									const float& f01, const float& f11, const float& f21, const float& f31,
									const float& f02, const float& f12, const float& f22, const float& f32,
									const float& f03, const float& f13, const float& f23, const float& f33	)
	{
		Vec::V4Set( m_col0, f00, f10, f20, f30 );
		Vec::V4Set( m_col1, f01, f11, f21, f31 );
		Vec::V4Set( m_col2, f02, f12, f22, f32 );
		Vec::V4Set( m_col3, f03, f13, f23, f33 );
	}



	__forceinline Mat44V::Mat44V(	eCOL_MAJORInitializer, 
		const float& f00, const float& f10, const float& f20, const float& f30,
		const float& f01, const float& f11, const float& f21, const float& f31,
		const float& f02, const float& f12, const float& f22, const float& f32,
		const float& f03, const float& f13, const float& f23, const float& f33	)
	{
		Vec::V4Set( m_col0, f00, f10, f20, f30 );
		Vec::V4Set( m_col1, f01, f11, f21, f31 );
		Vec::V4Set( m_col2, f02, f12, f22, f32 );
		Vec::V4Set( m_col3, f03, f13, f23, f33 );
	}

	__forceinline Mat44V::Mat44V(	eROW_MAJORInitializer, 
		const float& f00, const float& f01, const float& f02, const float& f03,
		const float& f10, const float& f11, const float& f12, const float& f13,
		const float& f20, const float& f21, const float& f22, const float& f23,
		const float& f30, const float& f31, const float& f32, const float& f33	)
	{
		Vec::V4Set( m_col0, f00, f10, f20, f30 );
		Vec::V4Set( m_col1, f01, f11, f21, f31 );
		Vec::V4Set( m_col2, f02, f12, f22, f32 );
		Vec::V4Set( m_col3, f03, f13, f23, f33 );
	}


	__forceinline Mat44V::Mat44V(Vec::Vector_4V_In v)
	{
		m_col0 = m_col1 = m_col2 = m_col3 = v;
	}

	__forceinline Mat44V::Mat44V(	Vec::Vector_4V_In col1,
									Vec::Vector_4V_In col2,
									Vec::Vector_4V_In col3,
									Vec::Vector_4V_In_After3Args col4	)
		:	m_col0( col1 ),
			m_col1( col2 ),
			m_col2( col3 ),
			m_col3( col4 )
	{
	}

	__forceinline Mat44V::Mat44V(Vec4V_In v)
	{
		m_col0 = m_col1 = m_col2 = m_col3 = v.v;
	}

	__forceinline Mat44V::Mat44V(Vec4V_In col0, Vec4V_In col1, Vec4V_In col2, Vec4V_In col3)
		:	m_col0( col0.v ),
			m_col1( col1.v ),
			m_col2( col2.v ),
			m_col3( col3.v )
	{
	}

	__forceinline Mat44V::Mat44V(Mat33V_In mat)
		:	m_col0( mat.m_col0 ),
			m_col1( mat.m_col1 ),
			m_col2( mat.m_col2 )
	{
	}

	__forceinline Mat44V::Mat44V(Mat33V_In mat, Vec4V_In col3)
		:	m_col0( mat.m_col0 ),
			m_col1( mat.m_col1 ),
			m_col2( mat.m_col2 ),
			m_col3( col3.v )
	{
		SetM30M31M32Zero();
	}

	__forceinline Mat44V::Mat44V(Mat34V_In mat)
		:	m_col0( mat.m_col0 ),
			m_col1( mat.m_col1 ),
			m_col2( mat.m_col2 ),
			m_col3( mat.m_col3 )
	{
	}

	__forceinline Mat34V_Out Mat44V::GetMat34() const
	{
		return Mat34V( m_col0, m_col1, m_col2, m_col3 );
	}

	__forceinline float Mat44V::GetM00f() const
	{
		return Vec::GetX(m_col0);
	}

	__forceinline float Mat44V::GetM01f() const
	{
		return Vec::GetX(m_col1);
	}

	__forceinline float Mat44V::GetM02f() const
	{
		return Vec::GetX(m_col2);
	}

	__forceinline float Mat44V::GetM03f() const
	{
		return Vec::GetX(m_col3);
	}
	
	__forceinline float Mat44V::GetM10f() const
	{
		return Vec::GetY(m_col0);
	}

	__forceinline float Mat44V::GetM11f() const
	{
		return Vec::GetY(m_col1);
	}

	__forceinline float Mat44V::GetM12f() const
	{
		return Vec::GetY(m_col2);
	}

	__forceinline float Mat44V::GetM13f() const
	{
		return Vec::GetY(m_col3);
	}

	__forceinline float Mat44V::GetM20f() const
	{
		return Vec::GetZ(m_col0);
	}

	__forceinline float Mat44V::GetM21f() const
	{
		return Vec::GetZ(m_col1);
	}

	__forceinline float Mat44V::GetM22f() const
	{
		return Vec::GetZ(m_col2);
	}

	__forceinline float Mat44V::GetM23f() const
	{
		return Vec::GetZ(m_col3);
	}

	__forceinline float Mat44V::GetM30f() const
	{
		return Vec::GetW(m_col0);
	}

	__forceinline float Mat44V::GetM31f() const
	{
		return Vec::GetW(m_col1);
	}

	__forceinline float Mat44V::GetM32f() const
	{
		return Vec::GetW(m_col2);
	}

	__forceinline float Mat44V::GetM33f() const
	{
		return Vec::GetW(m_col3);
	}

	__forceinline float Mat44V::GetElemf( unsigned row, unsigned col ) const
	{
		VecAssertMsg( row <= 3 && col <= 3, "Invalid row or col index!" );

		return Vec4V((*this)[col]).GetElemf( row );
	}

	__forceinline Vec4V_Out Mat44V::GetCol( unsigned col ) const
	{
		return Vec4V( (*this)[col] );
	}

	__forceinline Vec4V_Out Mat44V::GetCol0() const
	{
		return Vec4V( m_col0 );
	}

	__forceinline Vec4V_Out Mat44V::GetCol1() const
	{
		return Vec4V( m_col1 );
	}

	__forceinline Vec4V_Out Mat44V::GetCol2() const
	{
		return Vec4V( m_col2 );
	}

	__forceinline Vec4V_Out Mat44V::GetCol3() const
	{
		return Vec4V( m_col3 );
	}

	__forceinline Vec4V_Ref Mat44V::GetCol0Ref()
	{
		return *(reinterpret_cast<Vec4V*>(&m_col0));
	}

	__forceinline Vec4V_Ref Mat44V::GetCol1Ref()
	{
		return *(reinterpret_cast<Vec4V*>(&m_col1));
	}

	__forceinline Vec4V_Ref Mat44V::GetCol2Ref()
	{
		return *(reinterpret_cast<Vec4V*>(&m_col2));
	}

	__forceinline Vec4V_Ref Mat44V::GetCol3Ref()
	{
		return *(reinterpret_cast<Vec4V*>(&m_col3));
	}

	__forceinline Vec4V_ConstRef Mat44V::GetCol0ConstRef() const
	{
		return *(reinterpret_cast<const Vec4V*>(&m_col0));
	}

	__forceinline Vec4V_ConstRef Mat44V::GetCol1ConstRef() const
	{
		return *(reinterpret_cast<const Vec4V*>(&m_col1));
	}

	__forceinline Vec4V_ConstRef Mat44V::GetCol2ConstRef() const
	{
		return *(reinterpret_cast<const Vec4V*>(&m_col2));
	}

	__forceinline Vec4V_ConstRef Mat44V::GetCol3ConstRef() const
	{
		return *(reinterpret_cast<const Vec4V*>(&m_col3));
	}

	__forceinline Vec4V_Out Mat44V::a() const
	{
		return Vec4V( m_col0 );
	}

	__forceinline Vec4V_Out Mat44V::b() const
	{
		return Vec4V( m_col1 );
	}

	__forceinline Vec4V_Out Mat44V::c() const
	{
		return Vec4V( m_col2 );
	}

	__forceinline Vec4V_Out Mat44V::d() const
	{
		return Vec4V( m_col3 );
	}

	__forceinline Vec::Vector_4V_Out Mat44V::GetCol0Intrin128() const
	{
		return m_col0;
	}

	__forceinline Vec::Vector_4V_Out Mat44V::GetCol1Intrin128() const
	{
		return m_col1;
	}

	__forceinline Vec::Vector_4V_Out Mat44V::GetCol2Intrin128() const
	{
		return m_col2;
	}

	__forceinline Vec::Vector_4V_Out Mat44V::GetCol3Intrin128() const
	{
		return m_col3;
	}

	__forceinline Vec::Vector_4V_Ref Mat44V::GetCol0Intrin128Ref()
	{
		return m_col0;
	}

	__forceinline Vec::Vector_4V_Ref Mat44V::GetCol1Intrin128Ref()
	{
		return m_col1;
	}

	__forceinline Vec::Vector_4V_Ref Mat44V::GetCol2Intrin128Ref()
	{
		return m_col2;
	}

	__forceinline Vec::Vector_4V_Ref Mat44V::GetCol3Intrin128Ref()
	{
		return m_col3;
	}

	__forceinline Vec::Vector_4V_ConstRef Mat44V::GetCol0Intrin128ConstRef() const
	{
		return m_col0;
	}

	__forceinline Vec::Vector_4V_ConstRef Mat44V::GetCol1Intrin128ConstRef() const
	{
		return m_col1;
	}

	__forceinline Vec::Vector_4V_ConstRef Mat44V::GetCol2Intrin128ConstRef() const
	{
		return m_col2;
	}

	__forceinline Vec::Vector_4V_ConstRef Mat44V::GetCol3Intrin128ConstRef() const
	{
		return m_col3;
	}

	__forceinline void Mat44V::SetCol0( Vec4V_In col0 )
	{
		m_col0 = col0.v;
	}

	__forceinline void Mat44V::SetCol1( Vec4V_In col1 )
	{
		m_col1 = col1.v;
	}

	__forceinline void Mat44V::SetCol2( Vec4V_In col2 )
	{
		m_col2 = col2.v;
	}

	__forceinline void Mat44V::SetCol3( Vec4V_In col3 )
	{
		m_col3 = col3.v;
	}

	__forceinline void Mat44V::SetCols( Vec4V_In col0, Vec4V_In col1, Vec4V_In col2, Vec4V_In col3 )
	{
		m_col0 = col0.v;
		m_col1 = col1.v;
		m_col2 = col2.v;
		m_col3 = col3.v;
	}

	__forceinline void Mat44V::SetCol0Intrin128( Vec::Vector_4V_In col0 )
	{
		m_col0 = col0;
	}

	__forceinline void Mat44V::SetCol1Intrin128( Vec::Vector_4V_In col1 )
	{
		m_col1 = col1;
	}

	__forceinline void Mat44V::SetCol2Intrin128( Vec::Vector_4V_In col2 )
	{
		m_col2 = col2;
	}

	__forceinline void Mat44V::SetCol3Intrin128( Vec::Vector_4V_In col3 )
	{
		m_col3 = col3;
	}

	__forceinline void Mat44V::SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2 )
	{
		m_col0 = col0;
		m_col1 = col1;
		m_col2 = col2;
	}

	__forceinline void Mat44V::SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2, Vec::Vector_4V_In_After3Args col3 )
	{
		m_col0 = col0;
		m_col1 = col1;
		m_col2 = col2;
		m_col3 = col3;
	}

	__forceinline void Mat44V::SetCols( Vec4V_In col )
	{
		m_col0 = m_col1 = m_col2 = m_col3 = col.v;
	}

	__forceinline void Mat44V::SetColsIntrin128( Vec::Vector_4V_In col )
	{
		m_col0 = m_col1 = m_col2 = m_col3 = col;
	}

	__forceinline void Mat44V::SetM30M31M32Zero()
	{
		Vec::Vector_4V maskxyz = Vec::V4VConstant(V_MASKXYZ);
		m_col0 = Vec::V4And( m_col0, maskxyz );
		m_col1 = Vec::V4And( m_col1, maskxyz );
		m_col2 = Vec::V4And( m_col2, maskxyz );
	}

	__forceinline void Mat44V::SetElemf( unsigned row, unsigned col, float fVal )
	{
		VecAssertMsg( row <= 3 && col <= 3 , "Invalid row or col index!" );

		(((Vec4V_Ptr)(this))[col]).SetElemf( row, fVal );
	}

	__forceinline void Mat44V::SetM00f( float fVal )
	{
		Vec::SetXInMemory(m_col0, fVal);
	}

	__forceinline void Mat44V::SetM01f( float fVal )
	{
		Vec::SetXInMemory(m_col1, fVal);
	}

	__forceinline void Mat44V::SetM02f( float fVal )
	{
		Vec::SetXInMemory(m_col2, fVal);
	}

	__forceinline void Mat44V::SetM03f( float fVal )
	{
		Vec::SetXInMemory(m_col3, fVal);
	}

	__forceinline void Mat44V::SetM10f( float fVal )
	{
		Vec::SetYInMemory(m_col0, fVal);
	}

	__forceinline void Mat44V::SetM11f( float fVal )
	{
		Vec::SetYInMemory(m_col1, fVal);
	}

	__forceinline void Mat44V::SetM12f( float fVal )
	{
		Vec::SetYInMemory(m_col2, fVal);
	}

	__forceinline void Mat44V::SetM13f( float fVal )
	{
		Vec::SetYInMemory(m_col3, fVal);
	}

	__forceinline void Mat44V::SetM20f( float fVal )
	{
		Vec::SetZInMemory(m_col0, fVal);
	}

	__forceinline void Mat44V::SetM21f( float fVal )
	{
		Vec::SetZInMemory(m_col1, fVal);
	}

	__forceinline void Mat44V::SetM22f( float fVal )
	{
		Vec::SetZInMemory(m_col2, fVal);
	}

	__forceinline void Mat44V::SetM23f( float fVal )
	{
		Vec::SetZInMemory(m_col3, fVal);
	}

	__forceinline void Mat44V::SetM30f( float fVal )
	{
		Vec::SetWInMemory(m_col0, fVal);
	}

	__forceinline void Mat44V::SetM31f( float fVal )
	{
		Vec::SetWInMemory(m_col1, fVal);
	}

	__forceinline void Mat44V::SetM32f( float fVal )
	{
		Vec::SetWInMemory(m_col2, fVal);
	}

	__forceinline void Mat44V::SetM33f( float fVal )
	{
		Vec::SetWInMemory(m_col3, fVal);
	}

	__forceinline void Mat44V::SetCol0f( float x, float y, float z, float w )
	{
		Vec::SetXYZWInMemory(m_col0, x, y, z, w);
	}

	__forceinline void Mat44V::SetCol1f( float x, float y, float z, float w )
	{
		Vec::SetXYZWInMemory(m_col1, x, y, z, w);
	}

	__forceinline void Mat44V::SetCol2f( float x, float y, float z, float w )
	{
		Vec::SetXYZWInMemory(m_col2, x, y, z, w);
	}

	__forceinline void Mat44V::SetCol3f( float x, float y, float z, float w )
	{
		Vec::SetXYZWInMemory(m_col3, x, y, z, w);
	}

	__forceinline void Mat44V::SetM00( const float& fVal )
	{
		Vec::SetX(m_col0, fVal);
	}

	__forceinline void Mat44V::SetM01( const float& fVal )
	{
		Vec::SetX(m_col1, fVal);
	}

	__forceinline void Mat44V::SetM02( const float& fVal )
	{
		Vec::SetX(m_col2, fVal);
	}

	__forceinline void Mat44V::SetM03( const float& fVal )
	{
		Vec::SetX(m_col3, fVal);
	}

	__forceinline void Mat44V::SetM10( const float& fVal )
	{
		Vec::SetY(m_col0, fVal);
	}

	__forceinline void Mat44V::SetM11( const float& fVal )
	{
		Vec::SetY(m_col1, fVal);
	}

	__forceinline void Mat44V::SetM12( const float& fVal )
	{
		Vec::SetY(m_col2, fVal);
	}

	__forceinline void Mat44V::SetM13( const float& fVal )
	{
		Vec::SetY(m_col3, fVal);
	}

	__forceinline void Mat44V::SetM20( const float& fVal )
	{
		Vec::SetZ(m_col0, fVal);
	}

	__forceinline void Mat44V::SetM21( const float& fVal )
	{
		Vec::SetZ(m_col1, fVal);
	}

	__forceinline void Mat44V::SetM22( const float& fVal )
	{
		Vec::SetZ(m_col2, fVal);
	}

	__forceinline void Mat44V::SetM23( const float& fVal )
	{
		Vec::SetZ(m_col3, fVal);
	}

	__forceinline void Mat44V::SetM30( const float& fVal )
	{
		Vec::SetW(m_col0, fVal);
	}

	__forceinline void Mat44V::SetM31( const float& fVal )
	{
		Vec::SetW(m_col1, fVal);
	}

	__forceinline void Mat44V::SetM32( const float& fVal )
	{
		Vec::SetW(m_col2, fVal);
	}

	__forceinline void Mat44V::SetM33( const float& fVal )
	{
		Vec::SetW(m_col3, fVal);
	}

	__forceinline bool Mat44V::IsOrthonormal3x3(ScalarV_In toleranceSqVect) const
	{
		return Vec::V4IsEqualIntAll( IsOrthonormal3x3V( toleranceSqVect ).GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) != 0;
	}

	__forceinline VecBoolV_Out Mat44V::IsOrthonormal3x3V(ScalarV_In toleranceSqVect) const
	{
		// The input is the difference in the square of the magnitude (to first order).
		// Which, btw, can be approximated by: square(1+tolerance)-1 = about 2*tolerance.
		// Thus, if you don't know your tolerance until runtime, square your tolerance.
		// If you know your tolerance at compile-time, use the "2*tolerance" approximation to generate a const tol vector.

		return Mat33V(m_col0,m_col1,m_col2).IsOrthonormalV(toleranceSqVect);
	}

	__forceinline VecBoolV_Out Mat44V::IsOrthogonal3x3V(ScalarV_In angTolerance) const
	{
		return Mat33V(m_col0,m_col1,m_col2).IsOrthogonalV(angTolerance);
	}

	__forceinline bool Mat44V::IsOrthogonal3x3(ScalarV_In angTolerance) const
	{
		return Vec::V4IsEqualIntAll( IsOrthogonal3x3V( angTolerance ).GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) != 0;
	}

	__forceinline Mat44V_Out Mat44V::operator+ (Mat44V_In other) const
	{
		return Mat44V(	Vec::V4Add( m_col0, other.m_col0 ),
						Vec::V4Add( m_col1, other.m_col1 ),
						Vec::V4Add( m_col2, other.m_col2 ),
						Vec::V4Add( m_col3, other.m_col3 )	);
	}

	__forceinline Mat44V_Out Mat44V::operator- (Mat44V_In other) const
	{
		return Mat44V(	Vec::V4Subtract( m_col0, other.m_col0 ),
						Vec::V4Subtract( m_col1, other.m_col1 ),
						Vec::V4Subtract( m_col2, other.m_col2 ),
						Vec::V4Subtract( m_col3, other.m_col3 )	);
	}

	__forceinline void Mat44V::operator+= (Mat44V_In other)
	{
		m_col0 = Vec::V4Add( m_col0, other.m_col0 );
		m_col1 = Vec::V4Add( m_col1, other.m_col1 );
		m_col2 = Vec::V4Add( m_col2, other.m_col2 );
		m_col3 = Vec::V4Add( m_col3, other.m_col3 );
	}

	__forceinline void Mat44V::operator-= (Mat44V_In other)
	{
		m_col0 = Vec::V4Subtract( m_col0, other.m_col0 );
		m_col1 = Vec::V4Subtract( m_col1, other.m_col1 );
		m_col2 = Vec::V4Subtract( m_col2, other.m_col2 );
		m_col3 = Vec::V4Subtract( m_col3, other.m_col3 );
	}

	__forceinline Mat44V_Out Mat44V::operator- () const
	{
		Vec::Vector_4V INT_80000000 = Vec::V4VConstant(V_80000000);
		return Mat44V(	Vec::V4Xor( m_col0, INT_80000000 ),
						Vec::V4Xor( m_col1, INT_80000000 ),
						Vec::V4Xor( m_col2, INT_80000000 ),
						Vec::V4Xor( m_col3, INT_80000000 )	);
	}

	__forceinline Mat44V_Out Mat44V::operator| (Mat44V_In other) const
	{
		return Mat44V(	Vec::V4Or( m_col0, other.m_col0 ),
						Vec::V4Or( m_col1, other.m_col1 ),
						Vec::V4Or( m_col2, other.m_col2 ),
						Vec::V4Or( m_col3, other.m_col3 )	);
	}

	__forceinline Mat44V_Out Mat44V::operator& (Mat44V_In other) const
	{
		return Mat44V(	Vec::V4And( m_col0, other.m_col0 ),
						Vec::V4And( m_col1, other.m_col1 ),
						Vec::V4And( m_col2, other.m_col2 ),
						Vec::V4And( m_col3, other.m_col3 )	);
	}

	__forceinline Mat44V_Out Mat44V::operator^ (Mat44V_In other) const
	{
		return Mat44V(	Vec::V4Xor( m_col0, other.m_col0 ),
						Vec::V4Xor( m_col1, other.m_col1 ),
						Vec::V4Xor( m_col2, other.m_col2 ),
						Vec::V4Xor( m_col3, other.m_col3 )	);
	}

	__forceinline void Mat44V::operator|= (Mat44V_In other)
	{
		m_col0 = Vec::V4Or( m_col0, other.m_col0 );
		m_col1 = Vec::V4Or( m_col1, other.m_col1 );
		m_col2 = Vec::V4Or( m_col2, other.m_col2 );
		m_col3 = Vec::V4Or( m_col3, other.m_col3 );
	}

	__forceinline void Mat44V::operator&= (Mat44V_In other)
	{
		m_col0 = Vec::V4And( m_col0, other.m_col0 );
		m_col1 = Vec::V4And( m_col1, other.m_col1 );
		m_col2 = Vec::V4And( m_col2, other.m_col2 );
		m_col3 = Vec::V4And( m_col3, other.m_col3 );
	}

	__forceinline void Mat44V::operator^= (Mat44V_In other)
	{
		m_col0 = Vec::V4Xor( m_col0, other.m_col0 );
		m_col1 = Vec::V4Xor( m_col1, other.m_col1 );
		m_col2 = Vec::V4Xor( m_col2, other.m_col2 );
		m_col3 = Vec::V4Xor( m_col3, other.m_col3 );
	}

	__forceinline Vec4V_ConstRef Mat44V::operator[] (unsigned col) const
	{
		VecAssertMsg( col <= 3 , "Invalid column index." );
		return ((const Vec4V_Ptr)(this))[col];
	}

	__forceinline Vec4V_Ref Mat44V::operator[] (unsigned col)
	{
		VecAssertMsg( col <= 3 , "Invalid column index." );
		return ((Vec4V_Ptr)(this))[col];
	}

	namespace sysEndian
	{	
		template<> inline void SwapMe(Mat44V& m) {
			SwapMe(m.GetCol0Ref());
			SwapMe(m.GetCol1Ref());
			SwapMe(m.GetCol2Ref());
			SwapMe(m.GetCol3Ref());
		}
	} // namespace sysEndian

} // namespace rage
