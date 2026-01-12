
// Note: Everything here must be __forceinline'd, since *this will not be
// passed in vector registers! Force-inlining is okay here, since these functions
// are simple calls to free functions (which are only inline'd, not forced, but
// always pass via vector registers when not inlined).

namespace rage
{
	__forceinline Mat34V::Mat34V(eZEROInitializer)
	{
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		m_col0 = m_col1 = m_col2 = m_col3 = _zero;
	}

	__forceinline Mat34V::Mat34V(eIDENTITYInitializer)
	{
#if __XENON && UNIQUE_VECTORIZED_TYPE // take advantage of __vupkd3d
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V zeroInZ_oneInW = __vupkd3d(_zero, VPACK_NORMSHORT2);
		m_col0 = Vec::V4Permute<Vec::W,Vec::Z,Vec::Z,Vec::Z>( zeroInZ_oneInW );
		m_col1 = Vec::V4Permute<Vec::Z,Vec::W,Vec::Z,Vec::Z>( zeroInZ_oneInW );
		m_col2 = Vec::V4Permute<Vec::Z,Vec::Z,Vec::W,Vec::Z>( zeroInZ_oneInW );
		m_col3 = _zero;
#else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V _1000 = Vec::V4VConstant(V_X_AXIS_WZERO);
		Vec::Vector_4V _0100 = Vec::V4MergeXY( _zero, _1000 );
		Vec::Vector_4V _0010 = Vec::V4MergeXY( _0100, _zero );
		m_col0 = _1000;
		m_col1 = _0100;
		m_col2 = _0010;
		m_col3 = _zero;
#endif
	}

	__forceinline Mat34V::Mat34V(eFLT_EPSILONInitializer)
	{
		Vec::Vector_4V _fltSmall = Vec::V4VConstant(V_FLT_EPSILON);
		m_col0 = m_col1 = m_col2 = m_col3 = _fltSmall;
	}

	__forceinline Mat34V::Mat34V()
	{
	}

	__forceinline Mat34V::Mat34V(Mat34V_ConstRef other)
		:	m_col0( other.m_col0 ),
			m_col1( other.m_col1 ),
			m_col2( other.m_col2 ),
			m_col3( other.m_col3 )
	{
	}

	__forceinline Mat34V_ConstRef Mat34V::operator= (Mat34V_ConstRef other)
	{
		m_col0 = other.m_col0;
		m_col1 = other.m_col1;
		m_col2 = other.m_col2;
		m_col3 = other.m_col3;
		return *this;
	}

	__forceinline Mat34V::Mat34V(	const float& f00, const float& f10, const float& f20,
									const float& f01, const float& f11, const float& f21,
									const float& f02, const float& f12, const float& f22,
									const float& f03, const float& f13, const float& f23	)
	{
		Vec::V4Set( m_col0, f00, f10, f20, f20 );
		Vec::V4Set( m_col1, f01, f11, f21, f21 );
		Vec::V4Set( m_col2, f02, f12, f22, f22 );
		Vec::V4Set( m_col3, f03, f13, f23, f23 );
	}

	__forceinline Mat34V::Mat34V(	eCOL_MAJORInitializer,
		const float& f00, const float& f10, const float& f20,
		const float& f01, const float& f11, const float& f21,
		const float& f02, const float& f12, const float& f22,
		const float& f03, const float& f13, const float& f23	)
	{
		Vec::V4Set( m_col0, f00, f10, f20, f20 );
		Vec::V4Set( m_col1, f01, f11, f21, f21 );
		Vec::V4Set( m_col2, f02, f12, f22, f22 );
		Vec::V4Set( m_col3, f03, f13, f23, f23 );
	}

	__forceinline Mat34V::Mat34V( eROW_MAJORInitializer,
		const float& f00, const float& f01, const float& f02, const float& f03, 
		const float& f10, const float& f11, const float& f12, const float& f13, 
		const float& f20, const float& f21, const float& f22, const float& f23	)
	{
		Vec::V4Set( m_col0, f00, f10, f20, f20 );
		Vec::V4Set( m_col1, f01, f11, f21, f21 );
		Vec::V4Set( m_col2, f02, f12, f22, f22 );
		Vec::V4Set( m_col3, f03, f13, f23, f23 );
	}

	__forceinline Mat34V::Mat34V(Vec::Vector_4V_In v)
	{
		m_col0 = m_col1 = m_col2 = m_col3 = v;
	}

	__forceinline Mat34V::Mat34V(	Vec::Vector_4V_In col1,
									Vec::Vector_4V_In col2,
									Vec::Vector_4V_In col3,
									Vec::Vector_4V_In_After3Args col4	)
		:	m_col0( col1 ),
			m_col1( col2 ),
			m_col2( col3 ),
			m_col3( col4 )
	{
	}

	__forceinline Mat34V::Mat34V(ScalarV_In s)
	{
		m_col0 = m_col1 = m_col2 = m_col3 = s.v;
	}

	__forceinline Mat34V::Mat34V(Vec3V_In v)
	{
		m_col0 = m_col1 = m_col2 = m_col3 = v.v;
	}

	__forceinline Mat34V::Mat34V(Vec3V_In col0, Vec3V_In col1, Vec3V_In col2, Vec3V_In col3)
		:	m_col0( col0.v ),
			m_col1( col1.v ),
			m_col2( col2.v ),
			m_col3( col3.v )
	{
	}

	__forceinline Mat34V::Mat34V(Mat33V_In mat)
		:	m_col0( mat.m_col0 ),
			m_col1( mat.m_col1 ),
			m_col2( mat.m_col2 )
	{
	}

	__forceinline Mat34V::Mat34V(Mat33V_In mat, Vec3V_In col3)
		:	m_col0( mat.m_col0 ),
			m_col1( mat.m_col1 ),
			m_col2( mat.m_col2 ),
			m_col3( col3.v )
	{
	}

	__forceinline Mat33V_Out Mat34V::GetMat33() const
	{
		return Mat33V( m_col0, m_col1, m_col2 );
	}

	__forceinline Mat33V_Ref Mat34V::GetMat33Ref()
	{
		return *reinterpret_cast<Mat33V*>(this);
	}

	__forceinline Mat33V_ConstRef Mat34V::GetMat33ConstRef() const
	{
		return *reinterpret_cast<const Mat33V*>(this);
	}

	__forceinline float Mat34V::GetM00f() const
	{
		return Vec::GetX(m_col0);
	}

	__forceinline float Mat34V::GetM01f() const
	{
		return Vec::GetX(m_col1);
	}

	__forceinline float Mat34V::GetM02f() const
	{
		return Vec::GetX(m_col2);
	}

	__forceinline float Mat34V::GetM03f() const
	{
		return Vec::GetX(m_col3);
	}
	
	__forceinline float Mat34V::GetM10f() const
	{
		return Vec::GetY(m_col0);
	}

	__forceinline float Mat34V::GetM11f() const
	{
		return Vec::GetY(m_col1);
	}

	__forceinline float Mat34V::GetM12f() const
	{
		return Vec::GetY(m_col2);
	}

	__forceinline float Mat34V::GetM13f() const
	{
		return Vec::GetY(m_col3);
	}

	__forceinline float Mat34V::GetM20f() const
	{
		return Vec::GetZ(m_col0);
	}

	__forceinline float Mat34V::GetM21f() const
	{
		return Vec::GetZ(m_col1);
	}

	__forceinline float Mat34V::GetM22f() const
	{
		return Vec::GetZ(m_col2);
	}

	__forceinline float Mat34V::GetM23f() const
	{
		return Vec::GetZ(m_col3);
	}

	__forceinline Vec3V_Out Mat34V::GetCol0() const
	{
		return Vec3V( m_col0 );
	}

	__forceinline Vec3V_Out Mat34V::GetCol1() const
	{
		return Vec3V( m_col1 );
	}

	__forceinline Vec3V_Out Mat34V::GetCol2() const
	{
		return Vec3V( m_col2 );
	}

	__forceinline Vec3V_Out Mat34V::GetCol3() const
	{
		return Vec3V( m_col3 );
	}

	__forceinline Vec3V_Ref Mat34V::GetCol0Ref()
	{
		return *(reinterpret_cast<Vec3V*>(&m_col0));
	}

	__forceinline Vec3V_Ref Mat34V::GetCol1Ref()
	{
		return *(reinterpret_cast<Vec3V*>(&m_col1));
	}

	__forceinline Vec3V_Ref Mat34V::GetCol2Ref()
	{
		return *(reinterpret_cast<Vec3V*>(&m_col2));
	}

	__forceinline Vec3V_Ref Mat34V::GetCol3Ref()
	{
		return *(reinterpret_cast<Vec3V*>(&m_col3));
	}

	__forceinline Vec3V_ConstRef Mat34V::GetCol0ConstRef() const
	{
		return *(reinterpret_cast<const Vec3V*>(&m_col0));
	}

	__forceinline Vec3V_ConstRef Mat34V::GetCol1ConstRef() const
	{
		return *(reinterpret_cast<const Vec3V*>(&m_col1));
	}

	__forceinline Vec3V_ConstRef Mat34V::GetCol2ConstRef() const
	{
		return *(reinterpret_cast<const Vec3V*>(&m_col2));
	}

	__forceinline Vec3V_ConstRef Mat34V::GetCol3ConstRef() const
	{
		return *(reinterpret_cast<const Vec3V*>(&m_col3));
	}

	__forceinline Vec3V_Out Mat34V::a() const
	{
		return Vec3V( m_col0 );
	}

	__forceinline Vec3V_Out Mat34V::b() const
	{
		return Vec3V( m_col1 );
	}

	__forceinline Vec3V_Out Mat34V::c() const
	{
		return Vec3V( m_col2 );
	}

	__forceinline Vec3V_Out Mat34V::d() const
	{
		return Vec3V( m_col3 );
	}

	__forceinline Vec::Vector_4V_Out Mat34V::GetCol0Intrin128() const
	{
		return m_col0;
	}

	__forceinline Vec::Vector_4V_Out Mat34V::GetCol1Intrin128() const
	{
		return m_col1;
	}

	__forceinline Vec::Vector_4V_Out Mat34V::GetCol2Intrin128() const
	{
		return m_col2;
	}

	__forceinline Vec::Vector_4V_Out Mat34V::GetCol3Intrin128() const
	{
		return m_col3;
	}

	__forceinline Vec::Vector_4V_Ref Mat34V::GetCol0Intrin128Ref()
	{
		return m_col0;
	}

	__forceinline Vec::Vector_4V_Ref Mat34V::GetCol1Intrin128Ref()
	{
		return m_col1;
	}

	__forceinline Vec::Vector_4V_Ref Mat34V::GetCol2Intrin128Ref()
	{
		return m_col2;
	}

	__forceinline Vec::Vector_4V_Ref Mat34V::GetCol3Intrin128Ref()
	{
		return m_col3;
	}

	__forceinline Vec::Vector_4V_ConstRef Mat34V::GetCol0Intrin128ConstRef() const
	{
		return m_col0;
	}

	__forceinline Vec::Vector_4V_ConstRef Mat34V::GetCol1Intrin128ConstRef() const
	{
		return m_col1;
	}

	__forceinline Vec::Vector_4V_ConstRef Mat34V::GetCol2Intrin128ConstRef() const
	{
		return m_col2;
	}

	__forceinline Vec::Vector_4V_ConstRef Mat34V::GetCol3Intrin128ConstRef() const
	{
		return m_col3;
	}

	__forceinline float Mat34V::GetElemf( unsigned row, unsigned col ) const
	{
		VecAssertMsg( row <= 2 && col <= 3 , "Invalid row or col index!" );

		return Vec3V((*this)[col]).GetElemf( row );
	}

	__forceinline void Mat34V::SetElemf( unsigned row, unsigned col, float fVal )
	{
		VecAssertMsg( row <= 2 && col <= 3 , "Invalid row or col index!" );

		(((Vec3V_Ptr)(this))[col]).SetElemf( row, fVal );
	}

	__forceinline void Mat34V::Seta( Vec3V_In col0 )
	{
		m_col0 = col0.v;
	}

	__forceinline void Mat34V::Setb( Vec3V_In col1 )
	{
		m_col1 = col1.v;
	}

	__forceinline void Mat34V::Setc( Vec3V_In col2 )
	{
		m_col2 = col2.v;
	}

	__forceinline void Mat34V::Setd( Vec3V_In col3 )
	{
		m_col3 = col3.v;
	}

	__forceinline void Mat34V::SetCol0( Vec3V_In col0 )
	{
		m_col0 = col0.v;
	}

	__forceinline void Mat34V::SetCol1( Vec3V_In col1 )
	{
		m_col1 = col1.v;
	}

	__forceinline void Mat34V::SetCol2( Vec3V_In col2 )
	{
		m_col2 = col2.v;
	}

	__forceinline void Mat34V::SetCol3( Vec3V_In col3 )
	{
		m_col3 = col3.v;
	}

	__forceinline void Mat34V::SetCols( Vec3V_In col0, Vec3V_In col1, Vec3V_In col2, Vec3V_In col3 )
	{
		m_col0 = col0.v;
		m_col1 = col1.v;
		m_col2 = col2.v;
		m_col3 = col3.v;
	}

	__forceinline void Mat34V::SetCol0Intrin128( Vec::Vector_4V_In col0 )
	{
		m_col0 = col0;
	}

	__forceinline void Mat34V::SetCol1Intrin128( Vec::Vector_4V_In col1 )
	{
		m_col1 = col1;
	}

	__forceinline void Mat34V::SetCol2Intrin128( Vec::Vector_4V_In col2 )
	{
		m_col2 = col2;
	}

	__forceinline void Mat34V::SetCol3Intrin128( Vec::Vector_4V_In col3 )
	{
		m_col3 = col3;
	}

	__forceinline void Mat34V::SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2 )
	{
		m_col0 = col0;
		m_col1 = col1;
		m_col2 = col2;
	}

	__forceinline void Mat34V::SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2, Vec::Vector_4V_In_After3Args col3 )
	{
		m_col0 = col0;
		m_col1 = col1;
		m_col2 = col2;
		m_col3 = col3;
	}

	__forceinline void Mat34V::SetCols( Vec3V_In col )
	{
		m_col0 = m_col1 = m_col2 = m_col3 = col.v;
	}

	__forceinline void Mat34V::SetColsIntrin128( Vec::Vector_4V_In col )
	{
		m_col0 = m_col1 = m_col2 = m_col3 = col;
	}

	__forceinline void Mat34V::SetM30M31M32Zero()
	{
		Vec::V4SetWZero( m_col0 );
		Vec::V4SetWZero( m_col1 );
		Vec::V4SetWZero( m_col2 );
	}

	__forceinline void Mat34V::SetIdentity3x3()
	{
#if __XENON && UNIQUE_VECTORIZED_TYPE // take advantage of __vupkd3d
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V zeroInZ_oneInW = __vupkd3d(_zero, VPACK_NORMSHORT2);
		m_col0 = Vec::V4Permute<Vec::W,Vec::Z,Vec::Z,Vec::Z>( zeroInZ_oneInW );
		m_col1 = Vec::V4Permute<Vec::Z,Vec::W,Vec::Z,Vec::Z>( zeroInZ_oneInW );
		m_col2 = Vec::V4Permute<Vec::Z,Vec::Z,Vec::W,Vec::Z>( zeroInZ_oneInW );
#else
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		Vec::Vector_4V _1000 = Vec::V4VConstant(V_X_AXIS_WZERO);
		Vec::Vector_4V _0100 = Vec::V4MergeXY( _zero, _1000 );
		Vec::Vector_4V _0010 = Vec::V4MergeXY( _0100, _zero );
		m_col0 = _1000;
		m_col1 = _0100;
		m_col2 = _0010;
#endif
	}

	__forceinline void Mat34V::Set3x3( Mat34V_ConstRef newMat )
	{
		m_col0 = newMat.m_col0;
		m_col1 = newMat.m_col1;
		m_col2 = newMat.m_col2;
	}

	__forceinline void Mat34V::Set3x3( Mat33V_ConstRef newMat )
	{
		m_col0 = newMat.m_col0;
		m_col1 = newMat.m_col1;
		m_col2 = newMat.m_col2;
	}

	__forceinline void Mat34V::SetM00f( float fVal )
	{
		Vec::SetXInMemory(m_col0, fVal);
	}

	__forceinline void Mat34V::SetM01f( float fVal )
	{
		Vec::SetXInMemory(m_col1, fVal);
	}

	__forceinline void Mat34V::SetM02f( float fVal )
	{
		Vec::SetXInMemory(m_col2, fVal);
	}

	__forceinline void Mat34V::SetM03f( float fVal )
	{
		Vec::SetXInMemory(m_col3, fVal);
	}

	__forceinline void Mat34V::SetM10f( float fVal )
	{
		Vec::SetYInMemory(m_col0, fVal);
	}

	__forceinline void Mat34V::SetM11f( float fVal )
	{
		Vec::SetYInMemory(m_col1, fVal);
	}

	__forceinline void Mat34V::SetM12f( float fVal )
	{
		Vec::SetYInMemory(m_col2, fVal);
	}

	__forceinline void Mat34V::SetM13f( float fVal )
	{
		Vec::SetYInMemory(m_col3, fVal);
	}

	__forceinline void Mat34V::SetM20f( float fVal )
	{
		Vec::SetZInMemory(m_col0, fVal);
	}

	__forceinline void Mat34V::SetM21f( float fVal )
	{
		Vec::SetZInMemory(m_col1, fVal);
	}

	__forceinline void Mat34V::SetM22f( float fVal )
	{
		Vec::SetZInMemory(m_col2, fVal);
	}

	__forceinline void Mat34V::SetM23f( float fVal )
	{
		Vec::SetZInMemory(m_col3, fVal);
	}

	__forceinline void Mat34V::SetM00( const float& fVal )
	{
		Vec::SetX(m_col0, fVal);
	}

	__forceinline void Mat34V::SetM01( const float& fVal )
	{
		Vec::SetX(m_col1, fVal);
	}

	__forceinline void Mat34V::SetM02( const float& fVal )
	{
		Vec::SetX(m_col2, fVal);
	}

	__forceinline void Mat34V::SetM03( const float& fVal )
	{
		Vec::SetX(m_col3, fVal);
	}

	__forceinline void Mat34V::SetM10( const float& fVal )
	{
		Vec::SetY(m_col0, fVal);
	}

	__forceinline void Mat34V::SetM11( const float& fVal )
	{
		Vec::SetY(m_col1, fVal);
	}

	__forceinline void Mat34V::SetM12( const float& fVal )
	{
		Vec::SetY(m_col2, fVal);
	}

	__forceinline void Mat34V::SetM13( const float& fVal )
	{
		Vec::SetY(m_col3, fVal);
	}

	__forceinline void Mat34V::SetM20( const float& fVal )
	{
		Vec::SetZ(m_col0, fVal);
	}

	__forceinline void Mat34V::SetM21( const float& fVal )
	{
		Vec::SetZ(m_col1, fVal);
	}

	__forceinline void Mat34V::SetM22( const float& fVal )
	{
		Vec::SetZ(m_col2, fVal);
	}

	__forceinline void Mat34V::SetM23( const float& fVal )
	{
		Vec::SetZ(m_col3, fVal);
	}

	__forceinline bool Mat34V::IsOrthonormal3x3(ScalarV_In magTolerance, ScalarV_In angTolerance) const
	{
		return Vec::V4IsEqualIntAll( IsOrthonormal3x3V( magTolerance, angTolerance ).GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) != 0;
	}

	__forceinline bool Mat34V::IsOrthonormal3x3(ScalarV_In toleranceSqVect) const
	{
		return Vec::V4IsEqualIntAll( IsOrthonormal3x3V( toleranceSqVect ).GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) != 0;
	}

	__forceinline VecBoolV_Out Mat34V::IsOrthonormal3x3V(ScalarV_In magTolerance, ScalarV_In angTolerance) const
	{
		// The input is the difference in the square of the magnitude (to first order).
		// Which, btw, can be approximated by: square(1+tolerance)-1 = about 2*tolerance.
		// Thus, if you don't know your tolerance until runtime, square your tolerance.
		// If you know your tolerance at compile-time, use the "2*tolerance" approximation to generate a const tol vector.
		//
		// The angle tolerance is the cosine of the pi/2 - the angle between the axes, if the axes are unit length.
		// Because the dot product also multiplies the magnitude of the vectors, the tolerance on non-unit length
		// vectors should be accounted for in the angle tolerance.
		//
		return Mat33V(m_col0,m_col1,m_col2).IsOrthonormalV(magTolerance, angTolerance);
	}

	__forceinline VecBoolV_Out Mat34V::IsOrthonormal3x3V(ScalarV_In toleranceSqVect) const
	{
		// The input is the difference in the square of the magnitude (to first order).
		// Which, btw, can be approximated by: square(1+tolerance)-1 = about 2*tolerance.
		// Thus, if you don't know your tolerance until runtime, square your tolerance.
		// If you know your tolerance at compile-time, use the "2*tolerance" approximation to generate a const tol vector.

		return Mat33V(m_col0,m_col1,m_col2).IsOrthonormalV(toleranceSqVect);
	}

	__forceinline VecBoolV_Out Mat34V::IsOrthogonal3x3V(ScalarV_In angTolerance) const
	{
		return Mat33V(m_col0,m_col1,m_col2).IsOrthogonalV(angTolerance);
	}

	__forceinline bool Mat34V::IsOrthogonal3x3(ScalarV_In angTolerance) const
	{
		return Vec::V4IsEqualIntAll( IsOrthogonal3x3V( angTolerance ).GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) != 0;
	}

	__forceinline Mat34V_Out Mat34V::operator+ (Mat34V_In other) const
	{
		return Mat34V(	Vec::V4Add( m_col0, other.m_col0 ),
						Vec::V4Add( m_col1, other.m_col1 ),
						Vec::V4Add( m_col2, other.m_col2 ),
						Vec::V4Add( m_col3, other.m_col3 )	);
	}

	__forceinline Mat34V_Out Mat34V::operator- (Mat34V_In other) const
	{
		return Mat34V(	Vec::V4Subtract( m_col0, other.m_col0 ),
						Vec::V4Subtract( m_col1, other.m_col1 ),
						Vec::V4Subtract( m_col2, other.m_col2 ),
						Vec::V4Subtract( m_col3, other.m_col3 )	);
	}

	__forceinline void Mat34V::operator+= (Mat34V_In other)
	{
		m_col0 = Vec::V4Add( m_col0, other.m_col0 );
		m_col1 = Vec::V4Add( m_col1, other.m_col1 );
		m_col2 = Vec::V4Add( m_col2, other.m_col2 );
		m_col3 = Vec::V4Add( m_col3, other.m_col3 );
	}

	__forceinline void Mat34V::operator-= (Mat34V_In other)
	{
		m_col0 = Vec::V4Subtract( m_col0, other.m_col0 );
		m_col1 = Vec::V4Subtract( m_col1, other.m_col1 );
		m_col2 = Vec::V4Subtract( m_col2, other.m_col2 );
		m_col3 = Vec::V4Subtract( m_col3, other.m_col3 );
	}

	__forceinline Mat34V_Out Mat34V::operator- () const
	{
		Vec::Vector_4V INT_80000000 = Vec::V4VConstant(V_80000000);
		return Mat34V(	Vec::V4Xor( m_col0, INT_80000000 ),
						Vec::V4Xor( m_col1, INT_80000000 ),
						Vec::V4Xor( m_col2, INT_80000000 ),
						Vec::V4Xor( m_col3, INT_80000000 )	);
	}

	__forceinline Mat34V_Out Mat34V::operator| (Mat34V_In other) const
	{
		return Mat34V(	Vec::V4Or( m_col0, other.m_col0 ),
						Vec::V4Or( m_col1, other.m_col1 ),
						Vec::V4Or( m_col2, other.m_col2 ),
						Vec::V4Or( m_col3, other.m_col3 )	);
	}

	__forceinline Mat34V_Out Mat34V::operator& (Mat34V_In other) const
	{
		return Mat34V(	Vec::V4And( m_col0, other.m_col0 ),
						Vec::V4And( m_col1, other.m_col1 ),
						Vec::V4And( m_col2, other.m_col2 ),
						Vec::V4And( m_col3, other.m_col3 )	);
	}

	__forceinline Mat34V_Out Mat34V::operator^ (Mat34V_In other) const
	{
		return Mat34V(	Vec::V4Xor( m_col0, other.m_col0 ),
						Vec::V4Xor( m_col1, other.m_col1 ),
						Vec::V4Xor( m_col2, other.m_col2 ),
						Vec::V4Xor( m_col3, other.m_col3 )	);
	}

	__forceinline void Mat34V::operator|= (Mat34V_In other)
	{
		m_col0 = Vec::V4Or( m_col0, other.m_col0 );
		m_col1 = Vec::V4Or( m_col1, other.m_col1 );
		m_col2 = Vec::V4Or( m_col2, other.m_col2 );
		m_col3 = Vec::V4Or( m_col3, other.m_col3 );
	}

	__forceinline void Mat34V::operator&= (Mat34V_In other)
	{
		m_col0 = Vec::V4And( m_col0, other.m_col0 );
		m_col1 = Vec::V4And( m_col1, other.m_col1 );
		m_col2 = Vec::V4And( m_col2, other.m_col2 );
		m_col3 = Vec::V4And( m_col3, other.m_col3 );
	}

	__forceinline void Mat34V::operator^= (Mat34V_In other)
	{
		m_col0 = Vec::V4Xor( m_col0, other.m_col0 );
		m_col1 = Vec::V4Xor( m_col1, other.m_col1 );
		m_col2 = Vec::V4Xor( m_col2, other.m_col2 );
		m_col3 = Vec::V4Xor( m_col3, other.m_col3 );
	}

	__forceinline Vec3V_ConstRef Mat34V::operator[] (unsigned col) const
	{
		VecAssertMsg( col <= 3 , "Invalid column index." );
		return ((const Vec3V_Ptr)(this))[col];
	}

	__forceinline Vec3V_Ref Mat34V::operator[] (unsigned col)
	{
		VecAssertMsg( col <= 3 , "Invalid column index." );
		return ((Vec3V_Ptr)(this))[col];
	}

	namespace sysEndian
	{	
		template<> inline void SwapMe(Mat34V& m) {
			SwapMe(m.GetCol0Ref());
			SwapMe(m.GetCol1Ref());
			SwapMe(m.GetCol2Ref());
			SwapMe(m.GetCol3Ref());
		}
	} // namespace sysEndian

} // namespace rage
