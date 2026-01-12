
namespace rage
{
	__forceinline SoA_Vec2V::SoA_Vec2V(eZEROInitializer)
	{
		y = x = Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_Vec2V::SoA_Vec2V(eONEInitializer)
	{
		y = x = Vec::V4VConstant(V_ONE);
	}

	__forceinline SoA_Vec2V::SoA_Vec2V(eMASKXYInitializer)
	{
		y = x = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_Vec2V::SoA_Vec2V(eFLT_LARGE_8Initializer)
	{
		y = x = Vec::V4VConstant(V_FLT_LARGE_8);
	}

	__forceinline SoA_Vec2V::SoA_Vec2V(eFLT_EPSInitializer)
	{
		y = x = Vec::V4VConstant(V_FLT_EPSILON);
	}

	__forceinline SoA_Vec2V::SoA_Vec2V(eFLTMAXInitializer)
	{
		y = x = Vec::V4VConstant(V_FLT_MAX);
	}

	__forceinline SoA_Vec2V::SoA_Vec2V()
	{
	}

	__forceinline SoA_Vec2V::SoA_Vec2V(SoA_Vec2V_ConstRef v)
		:	x(v.x), y( v.y )
	{
	}

	__forceinline SoA_Vec2V_Ref SoA_Vec2V::operator= (SoA_Vec2V_ConstRef v)
	{
		x = v.x;
		y = v.y;
		return *this;
	}

	__forceinline SoA_Vec2V::SoA_Vec2V(const float& _x, const float& _y)
	{
		Vec::V4Set( x, _x );
		Vec::V4Set( y, _y );
	}

	__forceinline SoA_Vec2V::SoA_Vec2V(SoA_ScalarV_In scalar)
	{
		x = y = scalar.GetIntrin128();
	}

	__forceinline SoA_Vec2V::SoA_Vec2V(SoA_ScalarV_In _x, SoA_ScalarV_In _y)
		: x( _x.GetIntrin128() ), y( _y.GetIntrin128() )
	{
	}

	__forceinline SoA_Vec2V::SoA_Vec2V(Vec::Vector_4V_In scalar)
	{
		x = y = scalar;
	}

	__forceinline SoA_Vec2V::SoA_Vec2V(Vec::Vector_4V_In _x, Vec::Vector_4V_In _y)
		: x( _x ), y( _y )
	{
	}

	__forceinline void SoA_Vec2V::SetXIntrin128(Vec::Vector_4V_In _v)
	{
		x = _v;
	}

	__forceinline void SoA_Vec2V::SetYIntrin128(Vec::Vector_4V_In _v)
	{
		y = _v;
	}

	__forceinline Vec::Vector_4V_Out SoA_Vec2V::GetXIntrin128() const
	{
		return x;
	}

	__forceinline Vec::Vector_4V_Out SoA_Vec2V::GetYIntrin128() const
	{
		return y;
	}

	__forceinline Vec::Vector_4V_Ref SoA_Vec2V::GetXIntrin128Ref()
	{
		return x;
	}

	__forceinline Vec::Vector_4V_Ref SoA_Vec2V::GetYIntrin128Ref()
	{
		return x;
	}

	__forceinline SoA_ScalarV_Out SoA_Vec2V::GetX() const
	{
		return SoA_ScalarV( x );
	}

	__forceinline SoA_ScalarV_Out SoA_Vec2V::GetY() const
	{
		return SoA_ScalarV( y );
	}

	__forceinline void SoA_Vec2V::SetX( SoA_ScalarV_In newX )
	{
		x = newX.GetIntrin128();
	}

	__forceinline void SoA_Vec2V::SetY( SoA_ScalarV_In newY )
	{
		y = newY.GetIntrin128();
	}

	__forceinline void SoA_Vec2V::ZeroComponents()
	{
		x = y = Vec::V4VConstant(V_ZERO);
	}

	//============================================================================
	// Operators

	//__forceinline VecBoolV_Out SoA_Vec2V::operator== (Vec2V_In b) const
	//{
	//	return VecBoolV( Vec::V4IsEqualV( v, b.v ) );
	//}

	//__forceinline VecBoolV_Out SoA_Vec2V::operator!= (Vec2V_In b) const
	//{
	//	return VecBoolV( Vec::V4InvertBits( Vec::V4IsEqualV( v, b.v ) ) );
	//}

	//__forceinline VecBoolV_Out	SoA_Vec2V::operator< (Vec2V_In bigVector) const
	//{
	//	return VecBoolV( Vec::V4IsLessThanV( v, bigVector.v ) );
	//}

	//__forceinline VecBoolV_Out	SoA_Vec2V::operator<= (Vec2V_In bigVector) const
	//{
	//	return VecBoolV( Vec::V4IsLessThanOrEqualV( v, bigVector.v ) );
	//}

	//__forceinline VecBoolV_Out	SoA_Vec2V::operator> (Vec2V_In smallVector) const
	//{
	//	return VecBoolV( Vec::V4IsGreaterThanV( v, smallVector.v ) );
	//}

	//__forceinline VecBoolV_Out	SoA_Vec2V::operator>= (Vec2V_In smallVector) const
	//{
	//	return VecBoolV( Vec::V4IsGreaterThanOrEqualV( v, smallVector.v ) );
	//}

	__forceinline SoA_Vec2V_Out SoA_Vec2V::operator* (SoA_Vec2V_In b) const
	{
		return SoA_Vec2V(
					Vec::V4Scale( x, b.x ),
					Vec::V4Scale( y, b.y )
					);
	}

	__forceinline SoA_Vec2V_Out SoA_Vec2V::operator* (SoA_ScalarV_In b) const
	{
		return SoA_Vec2V(
			Vec::V4Scale( x, b.GetIntrin128() ),
			Vec::V4Scale( y, b.GetIntrin128() )
			);
	}

	__forceinline SoA_Vec2V_Out operator* (SoA_ScalarV_In a, SoA_Vec2V_In b)
	{
		return SoA_Vec2V(
			Vec::V4Scale( a.GetIntrin128(), b.GetXIntrin128() ),
			Vec::V4Scale( a.GetIntrin128(), b.GetYIntrin128() )
			);
	}

	__forceinline SoA_Vec2V_Out SoA_Vec2V::operator/ (SoA_Vec2V_In b) const
	{
		// Inlined Newton-Raphson inversions to make sure that we only generate one V4VConstant(V_HALF) and V4VConstant(V_ONE).
		Vec::Vector_4V v_half = Vec::V4VConstant(V_HALF);
		Vec::Vector_4V v_one = Vec::V4VConstant(V_ONE);

		// Invert #0
		Vec::Vector_4V rsqrt0 = Vec::V4InvSqrtFast(b.x);
		Vec::Vector_4V squaredEstimate0 = Vec::V4Scale(rsqrt0, rsqrt0);
		Vec::Vector_4V halfEstimate0 = Vec::V4Scale(v_half, rsqrt0);
		Vec::Vector_4V inv0 = Vec::V4AddScaled(rsqrt0, Vec::V4SubtractScaled(v_one, squaredEstimate0, b.x), halfEstimate0);

		// Invert #1
		Vec::Vector_4V rsqrt1 = Vec::V4InvSqrtFast(b.y);
		Vec::Vector_4V squaredEstimate1 = Vec::V4Scale(rsqrt1, rsqrt1);
		Vec::Vector_4V halfEstimate1 = Vec::V4Scale(v_half, rsqrt1);
		Vec::Vector_4V inv1 = Vec::V4AddScaled(rsqrt1, Vec::V4SubtractScaled(v_one, squaredEstimate1, b.y), halfEstimate1);

		return SoA_Vec2V(
			Vec::V4Scale( x, inv0 ),
			Vec::V4Scale( y, inv1 )
			);
	}

	__forceinline SoA_Vec2V_Out SoA_Vec2V::operator/ (SoA_ScalarV_In b) const
	{
		Vec::Vector_4V inv0 = Vec::V4Invert( b.GetIntrin128() );
		return SoA_Vec2V(
			Vec::V4Scale( x, inv0 ),
			Vec::V4Scale( y, inv0 )
			);
	}

	__forceinline SoA_Vec2V_Out SoA_Vec2V::operator+ (SoA_Vec2V_In b) const
	{
		return SoA_Vec2V(
					Vec::V4Add( x, b.x ),
					Vec::V4Add( y, b.y )
					);
	}

	__forceinline SoA_Vec2V_Out SoA_Vec2V::operator- (SoA_Vec2V_In b) const
	{
		return SoA_Vec2V(
					Vec::V4Subtract( x, b.x ),
					Vec::V4Subtract( y, b.y )
					);
	}

	__forceinline void SoA_Vec2V::operator*= (SoA_Vec2V_In b)
	{
		x = Vec::V4Scale( x, b.x );
		y = Vec::V4Scale( y, b.y );
	}

	__forceinline void SoA_Vec2V::operator*= (SoA_ScalarV_In b)
	{
		x = Vec::V4Scale( x, b.GetIntrin128() );
		y = Vec::V4Scale( y, b.GetIntrin128() );
	}

	__forceinline void SoA_Vec2V::operator/= (SoA_Vec2V_In b)
	{
		(*this) = (*this) / b;
	}

	__forceinline void SoA_Vec2V::operator/= (SoA_ScalarV_In b)
	{
		(*this) = (*this) / b;
	}

	__forceinline void SoA_Vec2V::operator+= (SoA_Vec2V_In b)
	{
		x = Vec::V4Add( x, b.x );
		y = Vec::V4Add( y, b.y );
	}

	__forceinline void SoA_Vec2V::operator-= (SoA_Vec2V_In b)
	{
		x = Vec::V4Subtract( x, b.x );
		y = Vec::V4Subtract( y, b.y );
	}

	__forceinline SoA_Vec2V_Out SoA_Vec2V::operator- () const
	{
		Vec::Vector_4V INT_80000000 = Vec::V4VConstant(V_80000000);
		return SoA_Vec2V(
					Vec::V4Xor( x, INT_80000000 ),
					Vec::V4Xor( y, INT_80000000 )
					);
	}

	__forceinline SoA_Vec2V_Out SoA_Vec2V::operator| (SoA_Vec2V_In b) const
	{
		return SoA_Vec2V(
					Vec::V4Or( x, b.x ),
					Vec::V4Or( y, b.y )
					);
	}

	__forceinline SoA_Vec2V_Out SoA_Vec2V::operator& (SoA_Vec2V_In b) const
	{
		return SoA_Vec2V(
					Vec::V4And( x, b.x ),
					Vec::V4And( y, b.y )
					);
	}

	__forceinline SoA_Vec2V_Out SoA_Vec2V::operator^ (SoA_Vec2V_In b) const
	{
		return SoA_Vec2V(
					Vec::V4Xor( x, b.x ),
					Vec::V4Xor( y, b.y )
					);
	}

	__forceinline void SoA_Vec2V::operator|= (SoA_Vec2V_In b)
	{
		x = Vec::V4Or( x, b.x );
		y = Vec::V4Or( y, b.y );
	}

	__forceinline void SoA_Vec2V::operator&= (SoA_Vec2V_In b)
	{
		x = Vec::V4And( x, b.x );
		y = Vec::V4And( y, b.y );
	}

	__forceinline void SoA_Vec2V::operator^= (SoA_Vec2V_In b)
	{
		x = Vec::V4Xor( x, b.x );
		y = Vec::V4Xor( y, b.y );
	}

	__forceinline Vec::Vector_4V_ConstRef SoA_Vec2V::operator[] (u32 elem) const
	{
		VecAssertMsg( elem <= 1 , "Invalid element index." );
		return (&x)[elem];
	}

	__forceinline Vec::Vector_4V_Ref SoA_Vec2V::operator[] (u32 elem)
	{
		VecAssertMsg( elem <= 1 , "Invalid element index." );
		return (&x)[elem];
	}
} // namespace rage
