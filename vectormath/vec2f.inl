namespace rage
{

	__forceinline Vec2f::Vec2f(eZEROInitializer)
	{
		Vec::V2Set( v, Vec::V2Constant(V_ZERO) );
	}

	__forceinline Vec2f::Vec2f(eFLT_LARGE_8Initializer)
	{
		Vec::V2Set( v, Vec::V2Constant(V_FLT_LARGE_8) );
	}

	__forceinline Vec2f::Vec2f(eFLTMAXInitializer)
	{
		Vec::V2Set( v, Vec::V2Constant(V_FLT_MAX) );
	}

	__forceinline Vec2f::Vec2f()
	{
	}

	__forceinline Vec2f::Vec2f(float s1, float s2)
	{
		Vec::V2Set( v, s1, s2 );
	}

	__forceinline Vec2f::Vec2f(Vec::Vector_2_In a)
	{
		Vec::V2Set( v, a );
	}

	__forceinline Vec::Vector_2_Out Vec2f::GetIntrin() const
	{
		return v;
	}

	__forceinline Vec::Vector_2_Ref Vec2f::GetIntrinRef()
	{
		return v;
	}

	__forceinline Vec::Vector_2_ConstRef Vec2f::GetIntrinConstRef() const
	{
		return v;
	}

	__forceinline void Vec2f::SetIntrin( Vec::Vector_2_In a )
	{
		v = a;
	}

	__forceinline float Vec2f::GetX() const
	{
		return v.x;
	}

	__forceinline float Vec2f::GetY() const
	{
		return v.y;
	}

	__forceinline void Vec2f::SetX( float newX )
	{
		v.x = newX;
	}

	__forceinline void Vec2f::SetY( float newY )
	{
		v.y = newY;
	}

	__forceinline void Vec2f::ZeroComponents()
	{
		Vec::V2ZeroComponents( v );
	}

	//============================================================================
	// Operators

	__forceinline Vec2f_Out Vec2f::operator* (Vec2f_In b) const
	{
		return Vec2f( Vec::V2Scale( v, b.v ) );
	}

	__forceinline Vec2f_Out Vec2f::operator* (float b) const
	{
		return Vec2f( Vec::V2Scale( v, b ) );
	}

	__forceinline Vec2f_Out operator* (float a, Vec2f_In b)
	{
		return Vec2f( Vec::V2Scale( b.v, a ) );
	}

	__forceinline Vec2f_Out Vec2f::operator/ (Vec2f_In b) const
	{
		return Vec2f( Vec::V2InvScale( v, b.v ) );
	}

	__forceinline Vec2f_Out Vec2f::operator/ (float b) const
	{
		return Vec2f( Vec::V2InvScale( v, b ) );
	}

	__forceinline Vec2f_Out Vec2f::operator+ (Vec2f_In b) const
	{
		return Vec2f( Vec::V2Add( v, b.v ) );
	}

	__forceinline Vec2f_Out Vec2f::operator- (Vec2f_In b) const
	{
		return Vec2f( Vec::V2Subtract( v, b.v ) );
	}

	__forceinline void Vec2f::operator*= (Vec2f_In b)
	{
		v = Vec::V2Scale( v, b.v );
	}

	__forceinline void Vec2f::operator*= (float b)
	{
		v = Vec::V2Scale( v, b );
	}

	__forceinline void Vec2f::operator/= (Vec2f_In b)
	{
		v = Vec::V2InvScale( v, b.v );
	}

	__forceinline void Vec2f::operator/= (float b)
	{
		v = Vec::V2InvScale( v, b );
	}

	__forceinline void Vec2f::operator+= (Vec2f_In b)
	{
		v = Vec::V2Add( v, b.v );
	}

	__forceinline void Vec2f::operator-= (Vec2f_In b)
	{
		v = Vec::V2Subtract( v, b.v );
	}

	__forceinline Vec2f_Out Vec2f::operator- () const
	{
		return Vec2f( Vec::V2Negate( v ) );
	}

	__forceinline Vec2f_Out Vec2f::operator| (Vec2f_In b) const
	{
		return Vec2f( Vec::V2Or( v, b.v ) );
	}

	__forceinline Vec2f_Out Vec2f::operator& (Vec2f_In b) const
	{
		return Vec2f( Vec::V2And( v, b.v ) );
	}

	__forceinline Vec2f_Out Vec2f::operator^ (Vec2f_In b) const
	{
		return Vec2f( Vec::V2Xor( v, b.v ) );
	}

	__forceinline void Vec2f::operator|= (Vec2f_In b)
	{
		v = Vec::V2Or( v, b.v );
	}

	__forceinline void Vec2f::operator&= (Vec2f_In b)
	{
		v = Vec::V2And( v, b.v );
	}

	__forceinline void Vec2f::operator^= (Vec2f_In b)
	{
		v = Vec::V2Xor( v, b.v );
	}

	__forceinline const float& Vec2f::operator[] (u32 elem) const
	{
		VecAssertMsg( elem <= 1 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

	__forceinline float& Vec2f::operator[] (u32 elem)
	{
		VecAssertMsg( elem <= 1 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

} // namespace rage
