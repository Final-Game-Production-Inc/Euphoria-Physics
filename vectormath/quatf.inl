namespace rage
{

	__forceinline Quatf::Quatf(eZEROInitializer)
	{
		Vec::V4Set( v, Vec::V4Constant(V_ZERO) );
	}

	__forceinline Quatf::Quatf(eFLT_LARGE_8Initializer)
	{
		Vec::V4Set( v, Vec::V4Constant(V_FLT_LARGE_8) );
	}

	__forceinline Quatf::Quatf(eFLT_EPSInitializer)
	{
		Vec::V4Set( v, Vec::V4Constant(V_FLT_EPSILON) );
	}

	__forceinline Quatf::Quatf()
	{
	}

	__forceinline Quatf::Quatf(float s1, float s2, float s3, float s4)
	{
		Vec::V4Set( v, s1, s2, s3, s4 );
	}

	__forceinline Quatf::Quatf(Vec::Vector_4_In a)
		: v(a)
	{
		Vec::V4Set( v, a );
	}

	__forceinline float Quatf::GetX() const
	{
		return v.x;
	}

	__forceinline float Quatf::GetY() const
	{
		return v.y;
	}

	__forceinline float Quatf::GetZ() const
	{
		return v.z;
	}

	__forceinline float Quatf::GetW() const
	{
		return v.w;
	}

	__forceinline Vec3f_Out Quatf::GetXYZ() const
	{
		return Vec3f(v.x,v.y,v.z);
	}

	__forceinline Vec::Vector_4_ConstRef Quatf::GetIntrinConstRef() const
	{
		return v;
	}

	__forceinline Vec::Vector_4_Ref Quatf::GetIntrinRef()
	{
		return v;
	}

	__forceinline Vec::Vector_4_Out Quatf::GetIntrin() const
	{
		return v;
	}

	__forceinline void Quatf::SetIntrin( Vec::Vector_4_In a )
	{
		v = a;
	}

	__forceinline void Quatf::SetX( float newX )
	{
		v.x = newX;
	}

	__forceinline void Quatf::SetY( float newY )
	{
		v.y = newY;
	}

	__forceinline void Quatf::SetZ( float newZ )
	{
		v.z = newZ;
	}

	__forceinline void Quatf::SetW( float newW )
	{
		v.w = newW;
	}

	__forceinline void Quatf::SetXYZ( Vec3f_In newXYZ )
	{
		v.x = newXYZ.GetX();
		v.y = newXYZ.GetY();
		v.z = newXYZ.GetZ();
	}

	__forceinline void Quatf::ZeroComponents()
	{
		Vec::V4ZeroComponents( v );
	}

	__forceinline void Quatf::SetWZero()
	{
		Vec::V4SetWZero( v );
	}

	//============================================================================
	// Operators

	__forceinline const float& Quatf::operator[] (u32 elem) const
	{
		VecAssertMsg( elem <= 3 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

	__forceinline float& Quatf::operator[] (u32 elem)
	{
		VecAssertMsg( elem <= 3 , "Invalid element index." );
		return Vec::GetElemRef( &v, elem );
	}

} // namespace rage
