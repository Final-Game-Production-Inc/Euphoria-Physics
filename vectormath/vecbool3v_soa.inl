namespace rage
{

	__forceinline SoA_VecBool3V::SoA_VecBool3V()
	{
	}

	__forceinline SoA_VecBool3V::SoA_VecBool3V(SoA_VecBool3V_ConstRef v)
		:	x(v.x), y( v.y ), z( v.z )
	{
	}

	__forceinline SoA_VecBool3V_Ref SoA_VecBool3V::operator= (SoA_VecBool3V_ConstRef v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	__forceinline SoA_VecBool3V::SoA_VecBool3V(eF_F_FInitializer)
	{
		x = y = z = Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_VecBool3V::SoA_VecBool3V(eF_F_TInitializer)
	{
		x = y = Vec::V4VConstant(V_ZERO);
		z = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool3V::SoA_VecBool3V(eF_T_FInitializer)
	{
		x = z = Vec::V4VConstant(V_ZERO);
		y = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool3V::SoA_VecBool3V(eF_T_TInitializer)
	{
		x = Vec::V4VConstant(V_ZERO);
		y = z = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool3V::SoA_VecBool3V(eT_F_FInitializer)
	{
		y = z = Vec::V4VConstant(V_ZERO);
		x = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool3V::SoA_VecBool3V(eT_F_TInitializer)
	{
		y = Vec::V4VConstant(V_ZERO);
		x = z = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool3V::SoA_VecBool3V(eT_T_FInitializer)
	{
		z = Vec::V4VConstant(V_ZERO);
		x = y = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool3V::SoA_VecBool3V(eT_T_TInitializer)
	{
		x = y = z = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool3V::SoA_VecBool3V(Vec::Vector_4V_In a)
	{
		x = y = z = a;
	}

	__forceinline SoA_VecBool3V::SoA_VecBool3V(Vec::Vector_4V_In _x,Vec::Vector_4V_In _y,Vec::Vector_4V_In _z)
		: x(_x),y(_y),z(_z)
	{
	}

	__forceinline SoA_VecBool3V_Out SoA_VecBool3V::operator== (SoA_VecBool3V_In b) const
	{
		return SoA_VecBool3V(
						Vec::V4IsEqualV( x, b.x ),
						Vec::V4IsEqualV( y, b.y ),
						Vec::V4IsEqualV( z, b.z )
						);
	}

	__forceinline SoA_VecBool3V_Out SoA_VecBool3V::operator!= (SoA_VecBool3V_In b) const
	{
		return SoA_VecBool3V(
						Vec::V4InvertBits( Vec::V4IsEqualV( x, b.x ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( y, b.y ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( z, b.z ) )
						);
	}

	__forceinline SoA_VecBool3V_Out SoA_VecBool3V::operator! () const
	{
		return SoA_VecBool3V(
						Vec::V4InvertBits( x ),
						Vec::V4InvertBits( y ),
						Vec::V4InvertBits( z )
						);
	}

	__forceinline SoA_VecBool3V_Out SoA_VecBool3V::operator| (SoA_VecBool3V_In b) const
	{
		return SoA_VecBool3V(
						Vec::V4Or( x, b.x ),
						Vec::V4Or( y, b.y ),
						Vec::V4Or( z, b.z )
						);
	}

	__forceinline SoA_VecBool3V_Out SoA_VecBool3V::operator& (SoA_VecBool3V_In b) const
	{
		return SoA_VecBool3V(
						Vec::V4And( x, b.x ),
						Vec::V4And( y, b.y ),
						Vec::V4And( z, b.z )
						);
	}

	__forceinline SoA_VecBool3V_Out SoA_VecBool3V::operator^ (SoA_VecBool3V_In b) const
	{
		return SoA_VecBool3V(
						Vec::V4Xor( x, b.x ),
						Vec::V4Xor( y, b.y ),
						Vec::V4Xor( z, b.z )
						);
	}

	__forceinline void SoA_VecBool3V::operator|= (SoA_VecBool3V_In b)
	{
		x = Vec::V4Or( x, b.x );
		y = Vec::V4Or( y, b.y );
		z = Vec::V4Or( z, b.z );
	}

	__forceinline void SoA_VecBool3V::operator&= (SoA_VecBool3V_In b)
	{
		x = Vec::V4And( x, b.x );
		y = Vec::V4And( y, b.y );
		z = Vec::V4And( z, b.z );
	}

	__forceinline void SoA_VecBool3V::operator^= (SoA_VecBool3V_In b)
	{
		x = Vec::V4Xor( x, b.x );
		y = Vec::V4Xor( y, b.y );
		z = Vec::V4Xor( z, b.z );
	}

	__forceinline void SoA_VecBool3V::SetXIntrin128(Vec::Vector_4V_In _x)
	{
		x = _x;
	}

	__forceinline void SoA_VecBool3V::SetYIntrin128(Vec::Vector_4V_In _y)
	{
		y = _y;
	}

	__forceinline void SoA_VecBool3V::SetZIntrin128(Vec::Vector_4V_In _z)
	{
		z = _z;
	}

	__forceinline Vec::Vector_4V SoA_VecBool3V::GetXIntrin128() const
	{
		return x;
	}

	__forceinline Vec::Vector_4V SoA_VecBool3V::GetYIntrin128() const
	{
		return y;
	}

	__forceinline Vec::Vector_4V SoA_VecBool3V::GetZIntrin128() const
	{
		return z;
	}

} // namespace rage
