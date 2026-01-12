namespace rage
{

	__forceinline Vec4f::Vec4f(eZEROInitializer)
	{
		Vec::V4Set( v, Vec::V4Constant(V_ZERO) );		
	}

	__forceinline Vec4f::Vec4f(eONEInitializer)
	{
		Vec::V4Set( v, Vec::V4Constant(V_ONE) );
	}

	__forceinline Vec4f::Vec4f(eMASKXYZWInitializer)
	{
		Vec::V4Set( v, Vec::V4Constant(V_MASKXYZW) );
	}

	__forceinline Vec4f::Vec4f(eFLT_LARGE_8Initializer)
	{
		Vec::V4Set( v, Vec::V4Constant(V_FLT_LARGE_8) );
	}

	__forceinline Vec4f::Vec4f(eFLT_EPSInitializer)
	{
		Vec::V4Set( v, Vec::V4Constant(V_FLT_EPSILON) );
	}

	__forceinline Vec4f::Vec4f(eFLTMAXInitializer)
	{
		Vec::V4Set( v, Vec::V4Constant(V_FLT_MAX) );
	}

	__forceinline Vec4f::Vec4f(eZERO_WONEInitializer)
	{
		Vec::V4Set( v, Vec::V4Constant(V_ZERO_WONE) );
	}

	__forceinline Vec4f::Vec4f()
	{
	}

	__forceinline Vec4f::Vec4f(float s1, float s2, float s3, float s4)
	{
		Vec::V4Set( v, s1, s2, s3, s4 );
	}

	__forceinline Vec4f::Vec4f(Vec::Vector_4_In a)
	{
		Vec::V4Set( v, a );
	}

	__forceinline float Vec4f::GetX() const
	{
		return v.x;
	}

	__forceinline float Vec4f::GetY() const
	{
		return v.y;
	}

	__forceinline float Vec4f::GetZ() const
	{
		return v.z;
	}

	__forceinline float Vec4f::GetW() const
	{
		return v.w;
	}

	__forceinline int Vec4f::GetXi() const
	{
		return *(reinterpret_cast<const int *>(&v.x));
	}

	__forceinline int Vec4f::GetYi() const
	{
		return *(reinterpret_cast<const int *>(&v.y));
	}

	__forceinline int Vec4f::GetZi() const
	{
		return *(reinterpret_cast<const int *>(&v.z));
	}

	__forceinline int Vec4f::GetWi() const
	{
		return *(reinterpret_cast<const int *>(&v.w));
	}


	__forceinline Vec3f_Out Vec4f::GetXYZ() const
	{
		return Vec3f( v.x, v.y, v.z );
	}

	__forceinline void Vec4f::SetXYZ( Vec3f_In newXYZ )
	{
		v.x = newXYZ.v.x;
		v.y = newXYZ.v.y;
		v.z = newXYZ.v.z;
	}

	__forceinline void Vec4f::SetX( float newX )
	{
		v.x = newX;
	}

	__forceinline void Vec4f::SetY( float newY )
	{
		v.y = newY;
	}

	__forceinline void Vec4f::SetZ( float newZ )
	{
		v.z = newZ;
	}

	__forceinline void Vec4f::SetW( float newW )
	{
		v.w = newW;
	}

	__forceinline void Vec4f::SetXi( int newX )
	{
		*(reinterpret_cast<int *>(&v.x)) = newX;
	}

	__forceinline void Vec4f::SetYi( int newY )
	{
		*(reinterpret_cast<int *>(&v.y)) = newY;
	}

	__forceinline void Vec4f::SetZi( int newZ )
	{
		*(reinterpret_cast<int *>(&v.z)) = newZ;
	}

	__forceinline void Vec4f::SetWi( int newW )
	{
		*(reinterpret_cast<int *>(&v.w)) = newW;
	}

	__forceinline Vec::Vector_4_ConstRef Vec4f::GetIntrinConstRef() const
	{
		return v;
	}

	__forceinline Vec::Vector_4_Ref Vec4f::GetIntrinRef()
	{
		return v;
	}

	__forceinline Vec::Vector_4_Out Vec4f::GetIntrin() const
	{
		return v;
	}

	__forceinline void Vec4f::SetIntrin( Vec::Vector_4_In a )
	{
		v = a;
	}

	__forceinline void Vec4f::ZeroComponents()
	{
		Vec::V4ZeroComponents( v );
	}

	__forceinline void Vec4f::SetWZero()
	{
		Vec::V4SetWZero( v );
	}

	//============================================================================
	// Operators

	__forceinline Vec4f_Out Vec4f::operator* (Vec4f_In b) const
	{
		return Vec4f( Vec::V4Scale( v, b.v ) );
	}

	__forceinline Vec4f_Out Vec4f::operator* (float b) const
	{
		return Vec4f( Vec::V4Scale( v, b ) );
	}

	__forceinline Vec4f_Out operator* (float a, Vec4f_In b)
	{
		return Vec4f( Vec::V4Scale( b.v, a ) );
	}

	__forceinline Vec4f_Out Vec4f::operator/ (Vec4f_In b) const
	{
		return Vec4f( Vec::V4InvScale( v, b.v ) );
	}

	__forceinline Vec4f_Out Vec4f::operator/ (float b) const
	{
		return Vec4f( Vec::V4InvScale( v, b ) );
	}

	__forceinline Vec4f_Out Vec4f::operator+ (Vec4f_In b) const
	{
		return Vec4f( Vec::V4Add( v, b.v ) );
	}

	__forceinline Vec4f_Out Vec4f::operator- (Vec4f_In b) const
	{
		return Vec4f( Vec::V4Subtract( v, b.v ) );
	}

	__forceinline void Vec4f::operator*= (Vec4f_In b)
	{
		v = Vec::V4Scale( v, b.v );
	}

	__forceinline void Vec4f::operator*= (float b)
	{
		v = Vec::V4Scale( v, b );
	}

	__forceinline void Vec4f::operator/= (Vec4f_In b)
	{
		v = Vec::V4InvScale( v, b.v );
	}

	__forceinline void Vec4f::operator/= (float b)
	{
		v = Vec::V4InvScale( v, b );
	}

	__forceinline void Vec4f::operator+= (Vec4f_In b)
	{
		v = Vec::V4Add( v, b.v );
	}

	__forceinline void Vec4f::operator-= (Vec4f_In b)
	{
		v = Vec::V4Subtract( v, b.v );
	}

	__forceinline Vec4f_Out Vec4f::operator- () const
	{
		return Vec4f( Vec::V4Negate( v ) );
	}

	__forceinline Vec4f_Out Vec4f::operator| (Vec4f_In b) const
	{
		return Vec4f( Vec::V4Or( v, b.v ) );
	}

	__forceinline Vec4f_Out Vec4f::operator& (Vec4f_In b) const
	{
		return Vec4f( Vec::V4And( v, b.v ) );
	}

	__forceinline Vec4f_Out Vec4f::operator^ (Vec4f_In b) const
	{
		return Vec4f( Vec::V4Xor( v, b.v ) );
	}

	__forceinline void Vec4f::operator|= (Vec4f_In b)
	{
		v = Vec::V4Or( v, b.v );
	}

	__forceinline void Vec4f::operator&= (Vec4f_In b)
	{
		v = Vec::V4And( v, b.v );
	}

	__forceinline void Vec4f::operator^= (Vec4f_In b)
	{
		v = Vec::V4Xor( v, b.v );
	}

	__forceinline const float& Vec4f::operator[] (u32 elem) const
	{
		VecAssertMsg( elem <= 3 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

	__forceinline float& Vec4f::operator[] (u32 elem)
	{
		VecAssertMsg( elem <= 3 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

} // namespace rage
