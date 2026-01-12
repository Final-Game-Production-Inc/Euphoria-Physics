namespace rage
{
	__forceinline BoolV::BoolV()
	{
	}

 	__forceinline BoolV::BoolV(bool b)
 	{
		v = Vec::V4IsTrueV(b);
 	}

#if __WIN32PC
	__forceinline BoolV::BoolV(BoolV_ConstRef _v)
		: v(_v.v)
	{
	}
#endif

#if !__XENON
	__forceinline BoolV_ConstRef BoolV::operator=(BoolV_ConstRef _v)
	{
		v = _v.v;
		return *this;
	}
#endif

	__forceinline BoolV::BoolV(eZEROInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_FALSE) );
	}

	__forceinline BoolV::BoolV(eMASKXYZWInitializer)
	{
		Vec::V4Set( v, Vec::V4VConstant(V_TRUE) );
	}

	__forceinline BoolV::BoolV(Vec::Vector_4V_In a)
	{
		v = a;
	}

	__forceinline bool BoolV::Getb() const
	{
		return Vec::V4IsEqualIntAll( v, Vec::V4VConstant(V_ZERO) ) == 0;
	}

	__forceinline BoolV_Out BoolV::operator== (BoolV_In b) const
	{
		return BoolV( Vec::V4IsEqualIntV( v, b.v ) );
	}

	__forceinline BoolV_Out BoolV::operator!= (BoolV_In b) const
	{
		return BoolV( Vec::V4InvertBits( Vec::V4IsEqualV( v, b.v ) ) );
	}

	__forceinline BoolV_Out BoolV::operator! () const
	{
		return BoolV( Vec::V4InvertBits( v ) );
	}

	__forceinline BoolV_Out BoolV::operator| (BoolV_In b) const
	{
		return BoolV( Vec::V4Or( v, b.v ) );
	}

	__forceinline BoolV_Out BoolV::operator& (BoolV_In b) const
	{
		return BoolV( Vec::V4And( v, b.v ) );
	}

	__forceinline BoolV_Out BoolV::operator^ (BoolV_In b) const
	{
		return BoolV( Vec::V4Xor( v, b.v ) );
	}

	__forceinline void BoolV::operator|= (BoolV_In b)
	{
		v = Vec::V4Or( v, b.v );
	}

	__forceinline void BoolV::operator&= (BoolV_In b)
	{
		v = Vec::V4And( v, b.v );
	}

	__forceinline void BoolV::operator^= (BoolV_In b)
	{
		v = Vec::V4Xor( v, b.v );
	}

	__forceinline void BoolV::SetIntrin128(Vec::Vector_4V_In _v)
	{
		v = _v;
	}

	__forceinline Vec::Vector_4V BoolV::GetIntrin128() const
	{
		return v;
	}

	__forceinline Vec::Vector_4V_Ref BoolV::GetIntrin128Ref()
	{
		return v;
	}

	__forceinline Vec::Vector_4V_ConstRef BoolV::GetIntrin128ConstRef() const
	{
		return v;
	}

	namespace sysEndian
	{
		template<> inline void SwapMe(BoolV& v) { return SwapMe(v.GetIntrin128Ref()); }
	} // namespace sysEndian

} // namespace rage

