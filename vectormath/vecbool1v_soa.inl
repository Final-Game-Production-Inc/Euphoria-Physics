namespace rage
{
	__forceinline SoA_VecBool1V::SoA_VecBool1V()
	{
	}

#if __WIN32PC
	__forceinline SoA_VecBool1V::SoA_VecBool1V(SoA_VecBool1V_ConstRef v)
		:	x(v.x)
	{
	}
#endif

#if !__XENON
	__forceinline SoA_VecBool1V_ConstRef SoA_VecBool1V::operator= (SoA_VecBool1V_ConstRef v)
	{
		x = v.x;
		return *this;
	}
#endif

	__forceinline SoA_VecBool1V::SoA_VecBool1V(eFInitializer)
	{
		x = Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_VecBool1V::SoA_VecBool1V(eTInitializer)
	{
		x = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_VecBool1V::SoA_VecBool1V(Vec::Vector_4V_In a)
	{
		x = a;
	}

	__forceinline SoA_VecBool1V_Out SoA_VecBool1V::operator== (SoA_VecBool1V_In b) const
	{
		return SoA_VecBool1V(
						Vec::V4IsEqualV( x, b.x )
						);
	}

	__forceinline SoA_VecBool1V_Out SoA_VecBool1V::operator!= (SoA_VecBool1V_In b) const
	{
		return SoA_VecBool1V(
						Vec::V4InvertBits( Vec::V4IsEqualV( x, b.x ) )
						);
	}

	__forceinline SoA_VecBool1V_Out SoA_VecBool1V::operator! () const
	{
		return SoA_VecBool1V(
						Vec::V4InvertBits( x )
						);
	}

	__forceinline SoA_VecBool1V_Out SoA_VecBool1V::operator| (SoA_VecBool1V_In b) const
	{
		return SoA_VecBool1V(
						Vec::V4Or( x, b.x )
						);
	}

	__forceinline SoA_VecBool1V_Out SoA_VecBool1V::operator& (SoA_VecBool1V_In b) const
	{
		return SoA_VecBool1V(
						Vec::V4And( x, b.x )
						);
	}

	__forceinline SoA_VecBool1V_Out SoA_VecBool1V::operator^ (SoA_VecBool1V_In b) const
	{
		return SoA_VecBool1V(
						Vec::V4Xor( x, b.x )
						);
	}

	__forceinline void SoA_VecBool1V::operator|= (SoA_VecBool1V_In b)
	{
		x = Vec::V4Or( x, b.x );
	}

	__forceinline void SoA_VecBool1V::operator&= (SoA_VecBool1V_In b)
	{
		x = Vec::V4And( x, b.x );
	}

	__forceinline void SoA_VecBool1V::operator^= (SoA_VecBool1V_In b)
	{
		x = Vec::V4Xor( x, b.x );
	}

	__forceinline void SoA_VecBool1V::SetIntrin128(Vec::Vector_4V_In _x)
	{
		x = _x;
	}

	__forceinline Vec::Vector_4V_Out SoA_VecBool1V::GetIntrin128() const
	{
		return x;
	}


} // namespace rage
