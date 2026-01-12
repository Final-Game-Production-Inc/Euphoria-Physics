namespace rage
{

	__forceinline SoA_VecBool2V::SoA_VecBool2V()
	{
	}

	__forceinline SoA_VecBool2V::SoA_VecBool2V(SoA_VecBool2V_ConstRef v)
		:	x(v.x), y( v.y )
	{
	}

	__forceinline SoA_VecBool2V_Ref SoA_VecBool2V::operator= (SoA_VecBool2V_ConstRef v)
	{
		x = v.x;
		y = v.y;
		return *this;
	}

	__forceinline SoA_VecBool2V::SoA_VecBool2V(eF_FInitializer)
	{
		x = y = Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_VecBool2V::SoA_VecBool2V(eF_TInitializer)
	{
		x = Vec::V4VConstant(V_ZERO);
		y = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool2V::SoA_VecBool2V(eT_FInitializer)
	{
		y = Vec::V4VConstant(V_ZERO);
		x = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool2V::SoA_VecBool2V(eT_TInitializer)
	{
		x = y = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool2V::SoA_VecBool2V(Vec::Vector_4V_In a)
	{
		x = y = a;
	}

	__forceinline SoA_VecBool2V::SoA_VecBool2V(Vec::Vector_4V_In _x,Vec::Vector_4V_In _y)
		: x(_x),y(_y)
	{
	}

	__forceinline SoA_VecBool2V_Out SoA_VecBool2V::operator== (SoA_VecBool2V_In b) const
	{
		return SoA_VecBool2V(
						Vec::V4IsEqualV( x, b.x ),
						Vec::V4IsEqualV( y, b.y )
						);
	}

	__forceinline SoA_VecBool2V_Out SoA_VecBool2V::operator!= (SoA_VecBool2V_In b) const
	{
		return SoA_VecBool2V(
						Vec::V4InvertBits( Vec::V4IsEqualV( x, b.x ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( y, b.y ) )
						);
	}

	__forceinline SoA_VecBool2V_Out SoA_VecBool2V::operator! () const
	{
		return SoA_VecBool2V(
						Vec::V4InvertBits( x ),
						Vec::V4InvertBits( y )
						);
	}

	__forceinline SoA_VecBool2V_Out SoA_VecBool2V::operator| (SoA_VecBool2V_In b) const
	{
		return SoA_VecBool2V(
						Vec::V4Or( x, b.x ),
						Vec::V4Or( y, b.y )
						);
	}

	__forceinline SoA_VecBool2V_Out SoA_VecBool2V::operator& (SoA_VecBool2V_In b) const
	{
		return SoA_VecBool2V(
						Vec::V4And( x, b.x ),
						Vec::V4And( y, b.y )
						);
	}

	__forceinline SoA_VecBool2V_Out SoA_VecBool2V::operator^ (SoA_VecBool2V_In b) const
	{
		return SoA_VecBool2V(
						Vec::V4Xor( x, b.x ),
						Vec::V4Xor( y, b.y )
						);
	}

	__forceinline void SoA_VecBool2V::operator|= (SoA_VecBool2V_In b)
	{
		x = Vec::V4Or( x, b.x );
		y = Vec::V4Or( y, b.y );
	}

	__forceinline void SoA_VecBool2V::operator&= (SoA_VecBool2V_In b)
	{
		x = Vec::V4And( x, b.x );
		y = Vec::V4And( y, b.y );
	}

	__forceinline void SoA_VecBool2V::operator^= (SoA_VecBool2V_In b)
	{
		x = Vec::V4Xor( x, b.x );
		y = Vec::V4Xor( y, b.y );
	}

	__forceinline void SoA_VecBool2V::SetXIntrin128(Vec::Vector_4V_In _x)
	{
		x = _x;
	}

	__forceinline void SoA_VecBool2V::SetYIntrin128(Vec::Vector_4V_In _y)
	{
		y = _y;
	}

	__forceinline Vec::Vector_4V_Out SoA_VecBool2V::GetXIntrin128() const
	{
		return x;
	}

	__forceinline Vec::Vector_4V_Out SoA_VecBool2V::GetYIntrin128() const
	{
		return y;
	}

} // namespace rage
