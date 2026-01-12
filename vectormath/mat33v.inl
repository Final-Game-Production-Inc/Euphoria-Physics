
// Note: Everything here must be __forceinline'd, since *this will not be
// passed in vector registers! Force-inlining is okay here, since these functions
// are simple calls to free functions (which are only __forceinline'd, not forced, but
// always pass via vector registers when not inlined).

namespace rage
{
	__forceinline Mat33V::Mat33V(eZEROInitializer)
	{
		Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
		m_col0 = m_col1 = m_col2 = _zero;
	}

	__forceinline Mat33V::Mat33V(eIDENTITYInitializer)
	{
#if __XENON && UNIQUE_VECTORIZED_TYPE // take advantage of __vupkd3d
		Vec::Vector_4V zeroInZ_oneInW = __vupkd3d(Vec::V4VConstant(V_ZERO), VPACK_NORMSHORT2);
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

	__forceinline Mat33V::Mat33V(eFLT_EPSILONInitializer)
	{
		Vec::Vector_4V _fltSmall = Vec::V4VConstant(V_FLT_EPSILON);
		m_col0 = m_col1 = m_col2 = _fltSmall;
	}

	__forceinline Mat33V::Mat33V()
	{
	}

	__forceinline Mat33V::Mat33V(Mat33V_ConstRef other)
		:	m_col0( other.m_col0 ),
			m_col1( other.m_col1 ),
			m_col2( other.m_col2 )
	{
	}

	__forceinline Mat33V_ConstRef Mat33V::operator= (Mat33V_ConstRef other)
	{
		m_col0 = other.m_col0;
		m_col1 = other.m_col1;
		m_col2 = other.m_col2;
		return *this;
	}

	__forceinline Mat33V::Mat33V(	const float& f00, const float& f10, const float& f20,
									const float& f01, const float& f11, const float& f21,
									const float& f02, const float& f12, const float& f22	)
	{
		Vec::V4Set( m_col0, f00, f10, f20, f20 );
		Vec::V4Set( m_col1, f01, f11, f21, f21 );
		Vec::V4Set( m_col2, f02, f12, f22, f22 );
	}

	__forceinline Mat33V::Mat33V(	eCOL_MAJORInitializer,
		const float& f00, const float& f10, const float& f20,
		const float& f01, const float& f11, const float& f21,
		const float& f02, const float& f12, const float& f22	)
	{
		Vec::V4Set( m_col0, f00, f10, f20, f20 );
		Vec::V4Set( m_col1, f01, f11, f21, f21 );
		Vec::V4Set( m_col2, f02, f12, f22, f22 );
	}

	__forceinline Mat33V::Mat33V(	eROW_MAJORInitializer,
		const float& f00, const float& f01, const float& f02,
		const float& f10, const float& f11, const float& f12,
		const float& f20, const float& f21, const float& f22	)
	{
		Vec::V4Set( m_col0, f00, f10, f20, f20 );
		Vec::V4Set( m_col1, f01, f11, f21, f21 );
		Vec::V4Set( m_col2, f02, f12, f22, f22 );
	}

	__forceinline Mat33V::Mat33V(Vec::Vector_4V_In v)
	{
		m_col0 = m_col1 = m_col2 = v;
	}

	__forceinline Mat33V::Mat33V(	Vec::Vector_4V_In col1,
									Vec::Vector_4V_In col2,
									Vec::Vector_4V_In col3	)
		:	m_col0( col1 ),
			m_col1( col2 ),
			m_col2( col3 )
	{
	}

	__forceinline Mat33V::Mat33V(Vec3V_In v)
	{
		m_col0 = m_col1 = m_col2 = v.v;
	}

	__forceinline Mat33V::Mat33V(Vec3V_In col0, Vec3V_In col1, Vec3V_In col2)
		:	m_col0( col0.v ),
			m_col1( col1.v ),
			m_col2( col2.v )
	{
	}

	__forceinline float Mat33V::GetM00f() const
	{
		return Vec::GetX(m_col0);
	}

	__forceinline float Mat33V::GetM01f() const
	{
		return Vec::GetX(m_col1);
	}

	__forceinline float Mat33V::GetM02f() const
	{
		return Vec::GetX(m_col2);
	}
	
	__forceinline float Mat33V::GetM10f() const
	{
		return Vec::GetY(m_col0);
	}

	__forceinline float Mat33V::GetM11f() const
	{
		return Vec::GetY(m_col1);
	}

	__forceinline float Mat33V::GetM12f() const
	{
		return Vec::GetY(m_col2);
	}

	__forceinline float Mat33V::GetM20f() const
	{
		return Vec::GetZ(m_col0);
	}

	__forceinline float Mat33V::GetM21f() const
	{
		return Vec::GetZ(m_col1);
	}

	__forceinline float Mat33V::GetM22f() const
	{
		return Vec::GetZ(m_col2);
	}

	__forceinline Vec3V_Out Mat33V::GetCol0() const
	{
		return Vec3V( m_col0 );
	}

	__forceinline Vec3V_Out Mat33V::GetCol1() const
	{
		return Vec3V( m_col1 );
	}

	__forceinline Vec3V_Out Mat33V::GetCol2() const
	{
		return Vec3V( m_col2 );
	}

	__forceinline Vec3V_Ref Mat33V::GetCol0Ref()
	{
		return *(reinterpret_cast<Vec3V*>(&m_col0));
	}

	__forceinline Vec3V_Ref Mat33V::GetCol1Ref()
	{
		return *(reinterpret_cast<Vec3V*>(&m_col1));
	}

	__forceinline Vec3V_Ref Mat33V::GetCol2Ref()
	{
		return *(reinterpret_cast<Vec3V*>(&m_col2));
	}

	__forceinline Vec3V_ConstRef Mat33V::GetCol0ConstRef() const
	{
		return *(reinterpret_cast<const Vec3V*>(&m_col0));
	}

	__forceinline Vec3V_ConstRef Mat33V::GetCol1ConstRef() const
	{
		return *(reinterpret_cast<const Vec3V*>(&m_col1));
	}

	__forceinline Vec3V_ConstRef Mat33V::GetCol2ConstRef() const
	{
		return *(reinterpret_cast<const Vec3V*>(&m_col2));
	}

	__forceinline Vec3V_Out Mat33V::a() const
	{
		return Vec3V( m_col0 );
	}

	__forceinline Vec3V_Out Mat33V::b() const
	{
		return Vec3V( m_col1 );
	}

	__forceinline Vec3V_Out Mat33V::c() const
	{
		return Vec3V( m_col2 );
	}

	__forceinline Vec::Vector_4V_Out Mat33V::GetCol0Intrin128() const
	{
		return m_col0;
	}

	__forceinline Vec::Vector_4V_Out Mat33V::GetCol1Intrin128() const
	{
		return m_col1;
	}

	__forceinline Vec::Vector_4V_Out Mat33V::GetCol2Intrin128() const
	{
		return m_col2;
	}

	__forceinline Vec::Vector_4V_Ref Mat33V::GetCol0Intrin128Ref()
	{
		return m_col0;
	}

	__forceinline Vec::Vector_4V_Ref Mat33V::GetCol1Intrin128Ref()
	{
		return m_col1;
	}

	__forceinline Vec::Vector_4V_Ref Mat33V::GetCol2Intrin128Ref()
	{
		return m_col2;
	}

	__forceinline Vec::Vector_4V_ConstRef Mat33V::GetCol0Intrin128ConstRef() const
	{
		return m_col0;
	}

	__forceinline Vec::Vector_4V_ConstRef Mat33V::GetCol1Intrin128ConstRef() const
	{
		return m_col1;
	}

	__forceinline Vec::Vector_4V_ConstRef Mat33V::GetCol2Intrin128ConstRef() const
	{
		return m_col2;
	}

	__forceinline float Mat33V::GetElemf( unsigned row, unsigned col ) const
	{
		VecAssertMsg( row <= 2 && col <= 2 , "Invalid row or col index!" );

		return Vec3V((*this)[col]).GetElemf( row );
	}

	__forceinline void Mat33V::SetElemf( unsigned row, unsigned col, float fVal )
	{
		VecAssertMsg( row <= 2 && col <= 2 , "Invalid row or col index!" );

		(((Vec3V_Ptr)(this))[col]).SetElemf( row, fVal );
	}

	__forceinline void Mat33V::SetCol0( Vec3V_In col0 )
	{
		m_col0 = col0.v;
	}

	__forceinline void Mat33V::SetColsIntrin128( Vec::Vector_4V_In col0, Vec::Vector_4V_In col1, Vec::Vector_4V_In col2 )
	{
		m_col0 = col0;
		m_col1 = col1;
		m_col2 = col2;
	}

	__forceinline void Mat33V::SetCol1( Vec3V_In col1 )
	{
		m_col1 = col1.v;
	}

	__forceinline void Mat33V::SetCol2( Vec3V_In col2 )
	{
		m_col2 = col2.v;
	}

	__forceinline void Mat33V::SetCols( Vec3V_In col0, Vec3V_In col1, Vec3V_In col2 )
	{
		m_col0 = col0.v;
		m_col1 = col1.v;
		m_col2 = col2.v;
	}

	__forceinline void Mat33V::SetCol0Intrin128( Vec::Vector_4V_In col0 )
	{
		m_col0 = col0;
	}

	__forceinline void Mat33V::SetCol1Intrin128( Vec::Vector_4V_In col1 )
	{
		m_col1 = col1;
	}

	__forceinline void Mat33V::SetCol2Intrin128( Vec::Vector_4V_In col2 )
	{
		m_col2 = col2;
	}

	__forceinline void Mat33V::SetCols( Vec3V_In col )
	{
		m_col0 = m_col1 = m_col2 = col.v;
	}

	__forceinline void Mat33V::SetColsIntrin128( Vec::Vector_4V_In col )
	{
		m_col0 = m_col1 = m_col2 = col;
	}

	__forceinline void Mat33V::SetM00f( float fVal )
	{
		Vec::SetXInMemory(m_col0, fVal);
	}

	__forceinline void Mat33V::SetM01f( float fVal )
	{
		Vec::SetXInMemory(m_col1, fVal);
	}

	__forceinline void Mat33V::SetM02f( float fVal )
	{
		Vec::SetXInMemory(m_col2, fVal);
	}

	__forceinline void Mat33V::SetM10f( float fVal )
	{
		Vec::SetYInMemory(m_col0, fVal);
	}

	__forceinline void Mat33V::SetM11f( float fVal )
	{
		Vec::SetYInMemory(m_col1, fVal);
	}

	__forceinline void Mat33V::SetM12f( float fVal )
	{
		Vec::SetYInMemory(m_col2, fVal);
	}

	__forceinline void Mat33V::SetM20f( float fVal )
	{
		Vec::SetZInMemory(m_col0, fVal);
	}

	__forceinline void Mat33V::SetM21f( float fVal )
	{
		Vec::SetZInMemory(m_col1, fVal);
	}

	__forceinline void Mat33V::SetM22f( float fVal )
	{
		Vec::SetZInMemory(m_col2, fVal);
	}

	__forceinline void Mat33V::SetM00( const float& fVal )
	{
		Vec::SetX(m_col0, fVal);
	}

	__forceinline void Mat33V::SetM01( const float& fVal )
	{
		Vec::SetX(m_col1, fVal);
	}

	__forceinline void Mat33V::SetM02( const float& fVal )
	{
		Vec::SetX(m_col2, fVal);
	}

	__forceinline void Mat33V::SetM10( const float& fVal )
	{
		Vec::SetY(m_col0, fVal);
	}

	__forceinline void Mat33V::SetM11( const float& fVal )
	{
		Vec::SetY(m_col1, fVal);
	}

	__forceinline void Mat33V::SetM12( const float& fVal )
	{
		Vec::SetY(m_col2, fVal);
	}

	__forceinline void Mat33V::SetM20( const float& fVal )
	{
		Vec::SetZ(m_col0, fVal);
	}

	__forceinline void Mat33V::SetM21( const float& fVal )
	{
		Vec::SetZ(m_col1, fVal);
	}

	__forceinline void Mat33V::SetM22( const float& fVal )
	{
		Vec::SetZ(m_col2, fVal);
	}

	__forceinline bool Mat33V::IsOrthonormal(ScalarV_In magTolerance, ScalarV_In angTolerance) const
	{
		return Vec::V4IsEqualIntAll( IsOrthonormalV( magTolerance, angTolerance ).GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) != 0;
	}

	__forceinline bool Mat33V::IsOrthonormal(ScalarV_In toleranceSqVect) const
	{
		return Vec::V4IsEqualIntAll( IsOrthonormalV( toleranceSqVect ).GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) != 0;
	}

	__forceinline VecBoolV_Out Mat33V::IsOrthonormalV(ScalarV_In magTolerance_, ScalarV_In angTolerance_) const
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
		Vec::Vector_4V magTolerance = magTolerance_.GetIntrin128();
		Vec::Vector_4V angTolerance = angTolerance_.GetIntrin128();
		Vec::Vector_4V _negOne = Vec::V4VConstant(V_NEGONE);

		// The m_col0 vector3 has a length close enough to 1.
		Vec::Vector_4V cond0 = Vec::V4Abs( Vec::V4Add( _negOne, Vec::V3MagSquaredV(m_col0) ) );
		cond0 = Vec::V4IsGreaterThanOrEqualV( magTolerance, cond0 );

		// The m_col1 vector3 has a length close enough to 1.
		Vec::Vector_4V cond1 = Vec::V4Abs( Vec::V4Add( _negOne, Vec::V3MagSquaredV(m_col1) ) );
		cond1 = Vec::V4IsGreaterThanOrEqualV( magTolerance, cond1 );

		// The m_col2 vector3 has a length close enough to 1.
		Vec::Vector_4V cond2 = Vec::V4Abs( Vec::V4Add( _negOne, Vec::V3MagSquaredV(m_col2) ) );
		cond2 = Vec::V4IsGreaterThanOrEqualV( magTolerance, cond2 );

		// The m_col0 vector3 is sufficiently perpendicular to the m_col1 vector3.
		Vec::Vector_4V cond3 = Vec::V4IsGreaterThanOrEqualV( angTolerance, Vec::V4Abs( Vec::V3DotV( m_col0, m_col1 ) ) );

		// The m_col0 vector3 is sufficiently perpendicular to the m_col2 vector3.
		Vec::Vector_4V cond4 = Vec::V4IsGreaterThanOrEqualV( angTolerance, Vec::V4Abs( Vec::V3DotV( m_col0, m_col2 ) ) );

		// The m_col1 vector3 is sufficiently perpendicular to the m_col1 vector3.
		Vec::Vector_4V cond5 = Vec::V4IsGreaterThanOrEqualV( angTolerance, Vec::V4Abs( Vec::V3DotV( m_col1, m_col2 ) ) );

		return VecBoolV(
					Vec::V4And( cond5,
					Vec::V4And( cond4,
					Vec::V4And( cond3,
					Vec::V4And( cond2,
					Vec::V4And( cond1, cond0 ) ) ) ) )
				);
	}

	__forceinline VecBoolV_Out Mat33V::IsOrthonormalV(ScalarV_In toleranceSqVect) const
	{
		return IsOrthonormalV(toleranceSqVect, toleranceSqVect);
	}

	__forceinline VecBoolV_Out Mat33V::IsOrthogonalV(ScalarV_In angTolerance_) const
	{
		Vec::Vector_4V angTolerance = angTolerance_.GetIntrin128();

		// Use "unsafe" normalizes here, as we want to get NaNs for zero length
		// vectors (as that will then force the perpendicular checks to fail).
		Vec::Vector_4V col0 = Vec::V3Normalize( m_col0 );
		Vec::Vector_4V col1 = Vec::V3Normalize( m_col1 );
		Vec::Vector_4V col2 = Vec::V3Normalize( m_col2 );

		// The m_col0 vector3 is sufficiently perpendicular to the m_col1 vector3.
		Vec::Vector_4V cond0 = Vec::V4IsGreaterThanOrEqualV( angTolerance, Vec::V4Abs( Vec::V3DotV( col0, col1 ) ) );

		// The m_col0 vector3 is sufficiently perpendicular to the m_col2 vector3.
		Vec::Vector_4V cond1 = Vec::V4IsGreaterThanOrEqualV( angTolerance, Vec::V4Abs( Vec::V3DotV( col0, col2 ) ) );

		// The m_col1 vector3 is sufficiently perpendicular to the m_col1 vector3.
		Vec::Vector_4V cond2 = Vec::V4IsGreaterThanOrEqualV( angTolerance, Vec::V4Abs( Vec::V3DotV( col1, col2 ) ) );

		return VecBoolV(
					Vec::V4And( cond2,
					Vec::V4And( cond1, cond0 ) )
				);
	}

	__forceinline bool Mat33V::IsOrthogonal(ScalarV_In angTolerance) const
	{
		return Vec::V4IsEqualIntAll( IsOrthogonalV( angTolerance ).GetIntrin128(), Vec::V4VConstant(V_MASKXYZW) ) != 0;
	}

	__forceinline Mat33V_Out Mat33V::operator+ (Mat33V_In other) const
	{
		return Mat33V(	Vec::V4Add( m_col0, other.m_col0 ),
						Vec::V4Add( m_col1, other.m_col1 ),
						Vec::V4Add( m_col2, other.m_col2 )	);
	}

	__forceinline Mat33V_Out Mat33V::operator- (Mat33V_In other) const
	{
		return Mat33V(	Vec::V4Subtract( m_col0, other.m_col0 ),
						Vec::V4Subtract( m_col1, other.m_col1 ),
						Vec::V4Subtract( m_col2, other.m_col2 )	);
	}

	__forceinline void Mat33V::operator+= (Mat33V_In other)
	{
		m_col0 = Vec::V4Add( m_col0, other.m_col0 );
		m_col1 = Vec::V4Add( m_col1, other.m_col1 );
		m_col2 = Vec::V4Add( m_col2, other.m_col2 );
	}

	__forceinline void Mat33V::operator-= (Mat33V_In other)
	{
		m_col0 = Vec::V4Subtract( m_col0, other.m_col0 );
		m_col1 = Vec::V4Subtract( m_col1, other.m_col1 );
		m_col2 = Vec::V4Subtract( m_col2, other.m_col2 );
	}

	__forceinline Mat33V_Out Mat33V::operator- () const
	{
		Vec::Vector_4V INT_80000000 = Vec::V4VConstant(V_80000000);
		return Mat33V(	Vec::V4Xor( m_col0, INT_80000000 ),
						Vec::V4Xor( m_col1, INT_80000000 ),
						Vec::V4Xor( m_col2, INT_80000000 )	);
	}

	__forceinline Mat33V_Out Mat33V::operator| (Mat33V_In other) const
	{
		return Mat33V(	Vec::V4Or( m_col0, other.m_col0 ),
						Vec::V4Or( m_col1, other.m_col1 ),
						Vec::V4Or( m_col2, other.m_col2 )	);
	}

	__forceinline Mat33V_Out Mat33V::operator& (Mat33V_In other) const
	{
		return Mat33V(	Vec::V4And( m_col0, other.m_col0 ),
						Vec::V4And( m_col1, other.m_col1 ),
						Vec::V4And( m_col2, other.m_col2 )	);
	}

	__forceinline Mat33V_Out Mat33V::operator^ (Mat33V_In other) const
	{
		return Mat33V(	Vec::V4Xor( m_col0, other.m_col0 ),
						Vec::V4Xor( m_col1, other.m_col1 ),
						Vec::V4Xor( m_col2, other.m_col2 )	);
	}

	__forceinline void Mat33V::operator|= (Mat33V_In other)
	{
		m_col0 = Vec::V4Or( m_col0, other.m_col0 );
		m_col1 = Vec::V4Or( m_col1, other.m_col1 );
		m_col2 = Vec::V4Or( m_col2, other.m_col2 );
	}

	__forceinline void Mat33V::operator&= (Mat33V_In other)
	{
		m_col0 = Vec::V4And( m_col0, other.m_col0 );
		m_col1 = Vec::V4And( m_col1, other.m_col1 );
		m_col2 = Vec::V4And( m_col2, other.m_col2 );
	}

	__forceinline void Mat33V::operator^= (Mat33V_In other)
	{
		m_col0 = Vec::V4Xor( m_col0, other.m_col0 );
		m_col1 = Vec::V4Xor( m_col1, other.m_col1 );
		m_col2 = Vec::V4Xor( m_col2, other.m_col2 );
	}

	__forceinline Vec3V_ConstRef Mat33V::operator[] (unsigned col) const
	{
		VecAssertMsg( col <= 2 , "Invalid column index." );
		return ((const Vec3V_Ptr)(this))[col];
	}

	__forceinline Vec3V_Ref Mat33V::operator[] (unsigned col)
	{
		VecAssertMsg( col <= 2 , "Invalid column index." );
		return ((Vec3V_Ptr)(this))[col];
	}

	namespace sysEndian
	{	
		template<> inline void SwapMe(Mat33V& m) {
			SwapMe(m.GetCol0Ref());
			SwapMe(m.GetCol1Ref());
			SwapMe(m.GetCol2Ref());
		}
	} // namespace sysEndian


} // namespace rage
