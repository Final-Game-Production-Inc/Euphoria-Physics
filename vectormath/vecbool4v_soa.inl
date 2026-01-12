namespace rage
{

	__forceinline SoA_VecBool4V::SoA_VecBool4V()
	{
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(SoA_VecBool4V_ConstRef v)
		:	x(v.x), y( v.y ), z( v.z ), w( v.w )
	{
	}

	__forceinline SoA_VecBool4V_Ref SoA_VecBool4V::operator= (SoA_VecBool4V_ConstRef v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
		return *this;
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eF_F_F_FInitializer)
	{
		x = y = z = w = Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eF_F_F_TInitializer)
	{
		x = y = z = Vec::V4VConstant(V_ZERO);
		w = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eF_F_T_FInitializer)
	{
		x = y = w = Vec::V4VConstant(V_ZERO);
		z = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eF_F_T_TInitializer)
	{
		x = y = Vec::V4VConstant(V_ZERO);
		z = w = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eF_T_F_FInitializer)
	{
		x = z = w = Vec::V4VConstant(V_ZERO);
		y = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eF_T_F_TInitializer)
	{
		x = z = Vec::V4VConstant(V_ZERO);
		y = w = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eF_T_T_FInitializer)
	{
		x = w = Vec::V4VConstant(V_ZERO);
		y = z = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eF_T_T_TInitializer)
	{
		x = Vec::V4VConstant(V_ZERO);
		y = z = w = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eT_F_F_FInitializer)
	{
		y = z = w = Vec::V4VConstant(V_ZERO);
		x = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eT_F_F_TInitializer)
	{
		y = z = Vec::V4VConstant(V_ZERO);
		x = w = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eT_F_T_FInitializer)
	{
		y = w = Vec::V4VConstant(V_ZERO);
		x = z = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eT_F_T_TInitializer)
	{
		y = Vec::V4VConstant(V_ZERO);
		x = z = w = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eT_T_F_FInitializer)
	{
		z = w = Vec::V4VConstant(V_ZERO);
		x = y = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eT_T_F_TInitializer)
	{
		z = Vec::V4VConstant(V_ZERO);
		x = y = w = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eT_T_T_FInitializer)
	{
		w = Vec::V4VConstant(V_ZERO);
		x = y = z = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(eT_T_T_TInitializer)
	{
		x = y = z = w = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(Vec::Vector_4V_In a)
	{
		x = y = z = w = a;
	}

	__forceinline SoA_VecBool4V::SoA_VecBool4V(Vec::Vector_4V_In _x,Vec::Vector_4V_In _y,Vec::Vector_4V_In _z,Vec::Vector_4V_In_After3Args _w)
		: x(_x),y(_y),z(_z),w(_w)
	{
	}

	__forceinline SoA_VecBool4V_Out SoA_VecBool4V::operator== (SoA_VecBool4V_In b) const
	{
		return SoA_VecBool4V(
						Vec::V4IsEqualV( x, b.x ),
						Vec::V4IsEqualV( y, b.y ),
						Vec::V4IsEqualV( z, b.z ),
						Vec::V4IsEqualV( w, b.w )
						);
	}

	__forceinline SoA_VecBool4V_Out SoA_VecBool4V::operator!= (SoA_VecBool4V_In b) const
	{
		return SoA_VecBool4V(
						Vec::V4InvertBits( Vec::V4IsEqualV( x, b.x ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( y, b.y ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( z, b.z ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( w, b.w ) )
						);
	}

	__forceinline SoA_VecBool4V_Out SoA_VecBool4V::operator! () const
	{
		return SoA_VecBool4V(
						Vec::V4InvertBits( x ),
						Vec::V4InvertBits( y ),
						Vec::V4InvertBits( z ),
						Vec::V4InvertBits( w )
						);
	}

	__forceinline SoA_VecBool4V_Out SoA_VecBool4V::operator| (SoA_VecBool4V_In b) const
	{
		return SoA_VecBool4V(
						Vec::V4Or( x, b.x ),
						Vec::V4Or( y, b.y ),
						Vec::V4Or( z, b.z ),
						Vec::V4Or( w, b.w )
						);
	}

	__forceinline SoA_VecBool4V_Out SoA_VecBool4V::operator& (SoA_VecBool4V_In b) const
	{
		return SoA_VecBool4V(
						Vec::V4And( x, b.x ),
						Vec::V4And( y, b.y ),
						Vec::V4And( z, b.z ),
						Vec::V4And( w, b.w )
						);
	}

	__forceinline SoA_VecBool4V_Out SoA_VecBool4V::operator^ (SoA_VecBool4V_In b) const
	{
		return SoA_VecBool4V(
						Vec::V4Xor( x, b.x ),
						Vec::V4Xor( y, b.y ),
						Vec::V4Xor( z, b.z ),
						Vec::V4Xor( w, b.w )
						);
	}

	__forceinline void SoA_VecBool4V::operator|= (SoA_VecBool4V_In b)
	{
		x = Vec::V4Or( x, b.x );
		y = Vec::V4Or( y, b.y );
		z = Vec::V4Or( z, b.z );
		w = Vec::V4Or( w, b.w );
	}

	__forceinline void SoA_VecBool4V::operator&= (SoA_VecBool4V_In b)
	{
		x = Vec::V4And( x, b.x );
		y = Vec::V4And( y, b.y );
		z = Vec::V4And( z, b.z );
		w = Vec::V4And( w, b.w );
	}

	__forceinline void SoA_VecBool4V::operator^= (SoA_VecBool4V_In b)
	{
		x = Vec::V4Xor( x, b.x );
		y = Vec::V4Xor( y, b.y );
		z = Vec::V4Xor( z, b.z );
		w = Vec::V4Xor( w, b.w );
	}

	__forceinline void SoA_VecBool4V::SetXIntrin128(Vec::Vector_4V_In _x)
	{
		x = _x;
	}

	__forceinline void SoA_VecBool4V::SetYIntrin128(Vec::Vector_4V_In _y)
	{
		y = _y;
	}

	__forceinline void SoA_VecBool4V::SetZIntrin128(Vec::Vector_4V_In _z)
	{
		z = _z;
	}

	__forceinline void SoA_VecBool4V::SetWIntrin128(Vec::Vector_4V_In _w)
	{
		w = _w;
	}

	__forceinline Vec::Vector_4V SoA_VecBool4V::GetXIntrin128() const
	{
		return x;
	}

	__forceinline Vec::Vector_4V SoA_VecBool4V::GetYIntrin128() const
	{
		return y;
	}

	__forceinline Vec::Vector_4V SoA_VecBool4V::GetZIntrin128() const
	{
		return z;
	}

	__forceinline Vec::Vector_4V SoA_VecBool4V::GetWIntrin128() const
	{
		return w;
	}

} // namespace rage

