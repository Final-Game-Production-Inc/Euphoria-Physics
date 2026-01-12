namespace rage
{

	__forceinline Vec3f::Vec3f(eZEROInitializer)
	{
		Vec::V3Set( v, Vec::V3Constant(V_ZERO) );
	}

	__forceinline Vec3f::Vec3f(eIDENTITYInitializer)
	{
		Vec::V3Set( v, Vec::V3Constant(V_ONE) );
	}

	__forceinline Vec3f::Vec3f(eMASKXYZInitializer)
	{
		Vec::V3Set( v, Vec::V3Constant(V_MASKXYZ) );
	}

	__forceinline Vec3f::Vec3f(eFLT_LARGE_8Initializer)
	{
		Vec::V3Set( v, Vec::V3Constant(V_FLT_LARGE_8) );
	}

	__forceinline Vec3f::Vec3f(eFLT_EPSInitializer)
	{
		Vec::V3Set( v, Vec::V3Constant(V_FLT_EPSILON) );
	}

	__forceinline Vec3f::Vec3f(eFLTMAXInitializer)
	{
		Vec::V3Set( v, Vec::V3Constant(V_FLT_MAX) );
	}

	__forceinline Vec3f::Vec3f()
	{
	}

	__forceinline Vec3f::Vec3f(const Vec3f& _v)
		: v(_v.v)
	{		
	}

	__forceinline Vec3f& Vec3f::operator=(const Vec3f& _v)
	{
		v = _v.v;
		return *this;
	}

	__forceinline Vec3f::Vec3f(float s1, float s2, float s3)
	{
		Vec::V3Set( v, s1, s2, s3 );
	}

	__forceinline Vec3f::Vec3f(Vec::Vector_3_In a)
	{
		Vec::V3Set( v, a );
	}

	__forceinline float Vec3f::GetX() const
	{
		return v.x;
	}

	__forceinline float Vec3f::GetY() const
	{
		return v.y;
	}

	__forceinline float Vec3f::GetZ() const
	{
		return v.z;
	}

	__forceinline Vec::Vector_3_ConstRef Vec3f::GetIntrinConstRef() const
	{
		return v;
	}

	__forceinline Vec::Vector_3_Ref Vec3f::GetIntrinRef()
	{
		return v;
	}

	__forceinline Vec::Vector_3_Out Vec3f::GetIntrin() const
	{
		return v;
	}

	__forceinline void Vec3f::SetIntrin( Vec::Vector_3_In a )
	{
		v = a;
	}

	__forceinline void Vec3f::SetX( float newX )
	{
		v.x = newX;
	}

	__forceinline void Vec3f::SetY( float newY )
	{
		v.y = newY;
	}

	__forceinline void Vec3f::SetZ( float newZ )
	{
		v.z = newZ;
	}

	__forceinline void Vec3f::ZeroComponents()
	{
		Vec::V3ZeroComponents( v );
	}

	//============================================================================
	// Operators

	__forceinline Vec3f_Out Vec3f::operator* (Vec3f_In b) const
	{
		return Vec3f( Vec::V3Scale( v, b.v ) );
	}

	__forceinline Vec3f_Out Vec3f::operator* (float b) const
	{
		return Vec3f( Vec::V3Scale( v, b ) );
	}

	__forceinline Vec3f_Out operator* (float a, Vec3f_In b)
	{
		return Vec3f( Vec::V3Scale( b.v, a ) );
	}

	__forceinline Vec3f_Out Vec3f::operator/ (Vec3f_In b) const
	{
		return Vec3f( Vec::V3InvScale( v, b.v ) );
	}

	__forceinline Vec3f_Out Vec3f::operator/ (float b) const
	{
		return Vec3f( Vec::V3InvScale( v, b ) );
	}

	__forceinline Vec3f_Out Vec3f::operator+ (Vec3f_In b) const
	{
		return Vec3f( Vec::V3Add( v, b.v ) );
	}

	__forceinline Vec3f_Out Vec3f::operator- (Vec3f_In b) const
	{
		return Vec3f( Vec::V3Subtract( v, b.v ) );
	}

	__forceinline void Vec3f::operator*= (Vec3f_In b)
	{
		v = Vec::V3Scale( v, b.v );
	}

	__forceinline void Vec3f::operator*= (float b)
	{
		v = Vec::V3Scale( v, b );
	}

	__forceinline void Vec3f::operator/= (Vec3f_In b)
	{
		v = Vec::V3InvScale( v, b.v );
	}

	__forceinline void Vec3f::operator/= (float b)
	{
		v = Vec::V3InvScale( v, b );
	}

	__forceinline void Vec3f::operator+= (Vec3f_In b)
	{
		v = Vec::V3Add( v, b.v );
	}

	__forceinline void Vec3f::operator-= (Vec3f_In b)
	{
		v = Vec::V3Subtract( v, b.v );
	}

	__forceinline Vec3f_Out Vec3f::operator- () const
	{
		return Vec3f( Vec::V3Negate( v ) );
	}

	__forceinline Vec3f_Out Vec3f::operator| (Vec3f_In b) const
	{
		return Vec3f( Vec::V3Or( v, b.v ) );
	}

	__forceinline Vec3f_Out Vec3f::operator& (Vec3f_In b) const
	{
		return Vec3f( Vec::V3And( v, b.v ) );
	}

	__forceinline Vec3f_Out Vec3f::operator^ (Vec3f_In b) const
	{
		return Vec3f( Vec::V3Xor( v, b.v ) );
	}

	__forceinline void Vec3f::operator|= (Vec3f_In b)
	{
		v = Vec::V3Or( v, b.v );
	}

	__forceinline void Vec3f::operator&= (Vec3f_In b)
	{
		v = Vec::V3And( v, b.v );
	}

	__forceinline void Vec3f::operator^= (Vec3f_In b)
	{
		v = Vec::V3Xor( v, b.v );
	}

	__forceinline const float& Vec3f::operator[] (u32 elem) const
	{
		VecAssertMsg( elem <= 2 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

	__forceinline float& Vec3f::operator[] (u32 elem)
	{
		VecAssertMsg( elem <= 2 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

} // namespace rage
