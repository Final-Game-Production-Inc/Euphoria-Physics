
// Note: Everything here must be __forceinline'd, since *this will not be
// passed in vector registers! Force-inlining is okay here, since these functions
// are simple calls to free functions (which are only inline'd, not forced, but
// always pass via vector registers when not inlined).

namespace rage
{
	__forceinline QuatV::QuatV(eZEROInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_ZERO) );
	}
	__forceinline QuatV::QuatV(eFLT_LARGE_8Initializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_FLT_LARGE_8) );
	}
	__forceinline QuatV::QuatV(eFLT_EPSILONInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_FLT_EPSILON) );
	}
	__forceinline QuatV::QuatV(eFLT_SMALL_6Initializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_FLT_SMALL_6) );
	}
	__forceinline QuatV::QuatV(eFLT_SMALL_5Initializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_FLT_SMALL_5) );
	}
	__forceinline QuatV::QuatV(eIDENTITYInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_ZERO_WONE) );
	}

	__forceinline QuatV::QuatV()
	{
	}

#if __WIN32PC
	__forceinline QuatV::QuatV(QuatV_ConstRef _v)
		: v(_v.v)
	{
	}
#endif

#if !__XENON
	__forceinline QuatV_ConstRef QuatV::operator=(QuatV_ConstRef _v)
	{
		v = _v.v;
		return *this;
	}
#endif

	__forceinline QuatV::QuatV(const float& s1, const float& s2, const float& s3, const float& s4)
	{
		Vec::V4Set( v, s1, s2, s3, s4 );
	}

	__forceinline QuatV::QuatV(Vec::Vector_4V_In a)
		: v( a )
	{
	}
	
	__forceinline QuatV::QuatV(ScalarV_In a)
		: v( a.GetIntrin128() )
	{
	}

	__forceinline QuatV::QuatV(ScalarV_In a, ScalarV_In b, ScalarV_In c, ScalarV_In d)
		: v(Vec::V4MergeXY( Vec::V4MergeXY( a.GetIntrin128(), c.GetIntrin128() ), Vec::V4MergeXY( b.GetIntrin128(), d.GetIntrin128() )))
	{
	}

	__forceinline QuatV::QuatV(ScalarV_In a, ScalarV_In b, Vec2V_In c)
		: v( Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( Vec::V4PermuteTwo<Vec::Y1,Vec::Z1,Vec::W1,Vec::X2>( a.GetIntrin128(), b.GetIntrin128() ), c.GetIntrin128() ) )
	{
	}

	__forceinline QuatV::QuatV(ScalarV_In a, Vec2V_In b, ScalarV_In c)
		: v( Vec::V4PermuteTwo<Vec::Y1,Vec::Z1,Vec::W1,Vec::X2>( Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( a.GetIntrin128(), b.GetIntrin128() ), c.GetIntrin128() ) )
	{
	}

	__forceinline QuatV::QuatV(Vec2V_In a, ScalarV_In b, ScalarV_In c)
		: v( Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>( a.GetIntrin128(), a.GetIntrin128() ), Vec::V4MergeXY( b.GetIntrin128(), c.GetIntrin128() ) ) )
	{
	}

	__forceinline QuatV::QuatV(ScalarV_In a, Vec3V_In b)
		: v( Vec::V4PermuteTwo<Vec::W1,Vec::X2,Vec::Y2,Vec::Z2>( a.GetIntrin128(), b.GetIntrin128() ) )
	{
	}

	__forceinline QuatV::QuatV(Vec2V_In a, Vec2V_In b)
		: v( Vec::V4PermuteTwo<Vec::X1, Vec::Y1, Vec::X2, Vec::Y2>( a.GetIntrin128(), b.GetIntrin128() ) )
	{
	}

	__forceinline QuatV::QuatV(Vec3V_In a, ScalarV_In b)
		: v( Vec::V4PermuteTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::X2>( a.GetIntrin128(), b.GetIntrin128() ) )
	{
	}

	__forceinline QuatV::QuatV(Vec3V_In a)
		: v( a.GetIntrin128() )
	{
	}

	__forceinline void QuatV::SetIntrin128(Vec::Vector_4V_In _v)
	{
		v = _v;
	}

	__forceinline Vec::Vector_4V_Val QuatV::GetIntrin128() const
	{
		return v;
	}

	__forceinline Vec::Vector_4V_Ref QuatV::GetIntrin128Ref()
	{
		return v;
	}

	__forceinline Vec::Vector_4V_ConstRef QuatV::GetIntrin128ConstRef() const
	{
		return v;
	}

	__forceinline ScalarV_Out QuatV::GetX() const
	{
		return ScalarV( Vec::V4SplatX(v) );
	}

	__forceinline ScalarV_Out QuatV::GetY() const
	{
		return ScalarV( Vec::V4SplatY(v) );
	}

	__forceinline ScalarV_Out QuatV::GetZ() const
	{
		return ScalarV( Vec::V4SplatZ(v) );
	}

	__forceinline ScalarV_Out QuatV::GetW() const
	{
		return ScalarV( Vec::V4SplatW(v) );
	}

	__forceinline Vec3V_Out QuatV::GetXYZ() const
	{
		return Vec3V( v );
	}

	__forceinline void QuatV::SetX( ScalarV_In newX )
	{
		v = Vec::V4PermuteTwo<Vec::W1,Vec::Y2,Vec::Z2,Vec::W2>( newX.GetIntrin128(), v );
	}

	__forceinline void QuatV::SetY( ScalarV_In newY )
	{
		// Equivalent if specializations aren't used, optimal for each platform if specializations are used.
#if __PS3
		v = Vec::V4PermuteTwo<Vec::X2,Vec::X1,Vec::Z2,Vec::W2>( newY.GetIntrin128(), v ); // Specialization.
#elif __XENON
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Z1,Vec::Z2,Vec::W2>( newY.GetIntrin128(), v ); // Specialization.
#else
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Y1,Vec::Z2,Vec::W2>( newY.GetIntrin128(), v ); // Whatever.
#endif
	}

	__forceinline void QuatV::SetZ( ScalarV_In newZ )
	{
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Y2,Vec::X1,Vec::W2>( newZ.GetIntrin128(), v );
	}

	__forceinline void QuatV::SetW( ScalarV_In newW )
	{
		v = Vec::V4PermuteTwo<Vec::X2,Vec::Y2,Vec::Z2,Vec::X1>( newW.GetIntrin128(), v );
	}

	__forceinline void QuatV::SetXYZ( Vec3V_In newXYZ )
	{
		v = Vec::V4PermuteTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::W2>( newXYZ.GetIntrin128(), v );
	}

	__forceinline void QuatV::SetXInMemory( ScalarV_In newX )
	{
		Vec::SetXInMemory( v, newX.GetIntrin128() );
	}

	__forceinline void QuatV::SetYInMemory( ScalarV_In newY )
	{
		Vec::SetYInMemory( v, newY.GetIntrin128() );
	}

	__forceinline void QuatV::SetZInMemory( ScalarV_In newZ )
	{
		Vec::SetZInMemory( v, newZ.GetIntrin128() );
	}

	__forceinline void QuatV::SetWInMemory( ScalarV_In newW )
	{
		Vec::SetWInMemory( v, newW.GetIntrin128() );
	}

	__forceinline void QuatV::SetX( const float& floatVal )
	{
		Vec::SetX( v, floatVal );
	}

	__forceinline void QuatV::SetY( const float& floatVal )
	{
		Vec::SetY( v, floatVal );
	}

	__forceinline void QuatV::SetZ( const float& floatVal )
	{
		Vec::SetZ( v, floatVal );
	}

	__forceinline void QuatV::SetW( const float& floatVal )
	{
		Vec::SetW( v, floatVal );
	}

	__forceinline float QuatV::GetXf() const
	{
		return Vec::GetX( v );
	}

	__forceinline float QuatV::GetYf() const
	{
		return Vec::GetY( v );
	}

	__forceinline float QuatV::GetZf() const
	{
		return Vec::GetZ( v );
	}

	__forceinline float QuatV::GetWf() const
	{
		return Vec::GetW( v );
	}

	__forceinline void QuatV::SetElemf( unsigned elem, float fVal )
	{
		VecAssertMsg( elem <= 3 , "Invalid elem!" );
		(*this)[elem] = fVal;
	}

	__forceinline float QuatV::GetElemf( unsigned elem ) const
	{
		VecAssertMsg( elem <= 3 , "Invalid elem!" );
		return (*this)[elem];
	}

	__forceinline void QuatV::SetXf( float floatVal )
	{
		Vec::SetXInMemory( v, floatVal );
	}

	__forceinline void QuatV::SetYf( float floatVal )
	{
		Vec::SetYInMemory( v, floatVal );
	}

	__forceinline void QuatV::SetZf( float floatVal )
	{
		Vec::SetZInMemory( v, floatVal );
	}

	__forceinline void QuatV::SetWf( float floatVal )
	{
		Vec::SetWInMemory( v, floatVal );
	}

	__forceinline void QuatV::ZeroComponents()
	{
		Vec::V4ZeroComponents( v );
	}

	__forceinline void QuatV::SetWZero()
	{
		Vec::V4SetWZero( v );
	}

	//============================================================================
	// Operators

	__forceinline VecBoolV_Out QuatV::operator== (QuatV_In b) const
	{
		return VecBoolV( Vec::V4IsEqualV( v, b.v ) );
	}

	__forceinline VecBoolV_Out QuatV::operator!= (QuatV_In b) const
	{
		return VecBoolV( Vec::V4InvertBits( Vec::V4IsEqualV( v, b.v ) ) );
	}

	__forceinline const float& QuatV::operator[] (unsigned elem) const
	{
		return Vec::GetElemRef( &v, elem );
	}

	__forceinline float& QuatV::operator[] (unsigned elem)
	{
		return Vec::GetElemRef( &v, elem );
	}

} // namespace rage
