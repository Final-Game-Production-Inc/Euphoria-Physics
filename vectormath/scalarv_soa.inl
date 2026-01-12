
namespace rage
{
	__forceinline SoA_ScalarV::SoA_ScalarV(eZEROInitializer)
	{
		x = Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_ScalarV::SoA_ScalarV(eONEInitializer)
	{
		x = Vec::V4VConstant(V_ONE);
	}

	__forceinline SoA_ScalarV::SoA_ScalarV(eMASKXInitializer)
	{
		x = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_ScalarV::SoA_ScalarV(eFLT_LARGE_8Initializer)
	{
		x = Vec::V4VConstant(V_FLT_LARGE_8);
	}

	__forceinline SoA_ScalarV::SoA_ScalarV(eFLT_EPSInitializer)
	{
		x = Vec::V4VConstant(V_FLT_EPSILON);
	}

	__forceinline SoA_ScalarV::SoA_ScalarV(eFLTMAXInitializer)
	{
		x = Vec::V4VConstant(V_FLT_MAX);
	}

	__forceinline SoA_ScalarV::SoA_ScalarV()
	{
	}

#if __WIN32PC
	__forceinline SoA_ScalarV::SoA_ScalarV(SoA_ScalarV_ConstRef v)
		:	x(v.x)
	{
	}
#endif

#if !__XENON
	__forceinline SoA_ScalarV_ConstRef SoA_ScalarV::operator= (SoA_ScalarV_ConstRef v)
	{
		x = v.x;
		return *this;
	}
#endif

	__forceinline SoA_ScalarV::SoA_ScalarV(Vec::Vector_4V_In scalar)
		: x( scalar )
	{
	}

	__forceinline void SoA_ScalarV::SetIntrin128(Vec::Vector_4V_In _v)
	{
		x = _v;
	}

	__forceinline Vec::Vector_4V_Out SoA_ScalarV::GetIntrin128() const
	{
		return x;
	}

	__forceinline void SoA_ScalarV::ZeroComponents()
	{
		x = Vec::V4VConstant(V_ZERO);
	}

	//============================================================================
	// Operators

	__forceinline SoA_VecBool1V_Out SoA_ScalarV::operator== (SoA_ScalarV_In b) const
	{
		return SoA_VecBool1V( Vec::V4IsEqualV( x, b.x ) );
	}

	__forceinline SoA_VecBool1V_Out SoA_ScalarV::operator!= (SoA_ScalarV_In b) const
	{
		return SoA_VecBool1V( Vec::V4InvertBits( Vec::V4IsEqualV( x, b.x ) ) );
	}

	__forceinline SoA_VecBool1V_Out SoA_ScalarV::operator< (SoA_ScalarV_In bigVector) const
	{
		return SoA_VecBool1V( Vec::V4IsLessThanV( x, bigVector.x ) );
	}

	__forceinline SoA_VecBool1V_Out SoA_ScalarV::operator<= (SoA_ScalarV_In bigVector) const
	{
		return SoA_VecBool1V( Vec::V4IsLessThanOrEqualV( x, bigVector.x ) );
	}

	__forceinline SoA_VecBool1V_Out SoA_ScalarV::operator> (SoA_ScalarV_In smallVector) const
	{
		return SoA_VecBool1V( Vec::V4IsGreaterThanV( x, smallVector.x ) );
	}

	__forceinline SoA_VecBool1V_Out SoA_ScalarV::operator>= (SoA_ScalarV_In smallVector) const
	{
		return SoA_VecBool1V( Vec::V4IsGreaterThanOrEqualV( x, smallVector.x ) );
	}

	__forceinline SoA_ScalarV_Out SoA_ScalarV::operator* (SoA_ScalarV_In b) const
	{
		return SoA_ScalarV(
			Vec::V4Scale( x, b.x )
			);
	}

	__forceinline SoA_ScalarV_Out SoA_ScalarV::operator/ (SoA_ScalarV_In b) const
	{
		return SoA_ScalarV(
			Vec::V4InvScale( x, b.x )
			);
	}

	__forceinline SoA_ScalarV_Out SoA_ScalarV::operator+ (SoA_ScalarV_In b) const
	{
		return SoA_ScalarV(
					Vec::V4Add( x, b.x )
					);
	}

	__forceinline SoA_ScalarV_Out SoA_ScalarV::operator- (SoA_ScalarV_In b) const
	{
		return SoA_ScalarV(
					Vec::V4Subtract( x, b.x )
					);
	}

	__forceinline void SoA_ScalarV::operator*= (SoA_ScalarV_In b)
	{
		x = Vec::V4Scale( x, b.x );
	}

	__forceinline void SoA_ScalarV::operator/= (SoA_ScalarV_In b)
	{
		x = Vec::V4InvScale( x, b.x );
	}

	__forceinline void SoA_ScalarV::operator+= (SoA_ScalarV_In b)
	{
		x = Vec::V4Add( x, b.x );
	}

	__forceinline void SoA_ScalarV::operator-= (SoA_ScalarV_In b)
	{
		x = Vec::V4Subtract( x, b.x );
	}

	__forceinline SoA_ScalarV_Out SoA_ScalarV::operator- () const
	{
		return SoA_ScalarV(
					Vec::V4Negate( x )
					);
	}

	__forceinline SoA_ScalarV_Out SoA_ScalarV::operator| (SoA_ScalarV_In b) const
	{
		return SoA_ScalarV(
					Vec::V4Or( x, b.x )
					);
	}

	__forceinline SoA_ScalarV_Out SoA_ScalarV::operator& (SoA_ScalarV_In b) const
	{
		return SoA_ScalarV(
					Vec::V4And( x, b.x )
					);
	}

	__forceinline SoA_ScalarV_Out SoA_ScalarV::operator^ (SoA_ScalarV_In b) const
	{
		return SoA_ScalarV(
					Vec::V4Xor( x, b.x )
					);
	}

	__forceinline void SoA_ScalarV::operator|= (SoA_ScalarV_In b)
	{
		x = Vec::V4Or( x, b.x );
	}

	__forceinline void SoA_ScalarV::operator&= (SoA_ScalarV_In b)
	{
		x = Vec::V4And( x, b.x );
	}

	__forceinline void SoA_ScalarV::operator^= (SoA_ScalarV_In b)
	{
		x = Vec::V4Xor( x, b.x );
	}
} // namespace rage
