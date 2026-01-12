
namespace rage
{
	__forceinline SoA_Vec4V::SoA_Vec4V(eZEROInitializer)
	{
		w = z = y = x = Vec::V4VConstant(V_ZERO);
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(eONEInitializer)
	{
		w = z = y = x = Vec::V4VConstant(V_ONE);
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(eMASKXYZWInitializer)
	{
		w = z = y = x = Vec::V4VConstant(V_MASKXYZW);
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(eFLT_LARGE_8Initializer)
	{
		w = z = y = x = Vec::V4VConstant(V_FLT_LARGE_8);
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(eFLT_EPSInitializer)
	{
		w = z = y = x = Vec::V4VConstant(V_FLT_EPSILON);
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(eFLTMAXInitializer)
	{
		w = z = y = x = Vec::V4VConstant(V_FLT_MAX);
	}

	__forceinline SoA_Vec4V::SoA_Vec4V()
	{
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(SoA_Vec4V_ConstRef v)
		:	x( v.x ), y( v.y ), z( v.z ), w( v.w )
	{
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(SoA_QuatV_In q)
		:	x( q.GetXIntrin128() ), y( q.GetYIntrin128() ), z( q.GetZIntrin128() ), w( q.GetWIntrin128() )
	{
	}

	__forceinline SoA_Vec4V_Ref SoA_Vec4V::operator= (SoA_Vec4V_ConstRef v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
		return *this;
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(const float& _x, const float& _y, const float& _z, const float& _w)
	{
		Vec::V4Set( x, _x );
		Vec::V4Set( y, _y );
		Vec::V4Set( z, _z );
		Vec::V4Set( w, _w );
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(Vec::Vector_4V_In scalar)
	{
		x = y = z = w = scalar;
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(Vec::Vector_4V_In _x, Vec::Vector_4V_In _y, Vec::Vector_4V_In _z, Vec::Vector_4V_In_After3Args _w)
		: x( _x ), y( _y ), z( _z ), w( _w )
	{
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(SoA_ScalarV_In _x, SoA_ScalarV_In _y, SoA_ScalarV_In _z, SoA_ScalarV_In _w)
		: x( _x.GetIntrin128() ), y( _y.GetIntrin128() ), z( _z.GetIntrin128() ), w( _w.GetIntrin128() )
	{
	}

	__forceinline SoA_Vec4V::SoA_Vec4V(SoA_ScalarV_In _v)
	{
		x = y = z = w = _v.GetIntrin128();
	}

	__forceinline void SoA_Vec4V::SetXIntrin128(Vec::Vector_4V_In _v)
	{
		x = _v;
	}

	__forceinline void SoA_Vec4V::SetYIntrin128(Vec::Vector_4V_In _v)
	{
		y = _v;
	}

	__forceinline void SoA_Vec4V::SetZIntrin128(Vec::Vector_4V_In _v)
	{
		z = _v;
	}

	__forceinline void SoA_Vec4V::SetWIntrin128(Vec::Vector_4V_In _v)
	{
		w = _v;
	}

	__forceinline Vec::Vector_4V_Out SoA_Vec4V::GetXIntrin128() const
	{
		return x;
	}

	__forceinline Vec::Vector_4V_Out SoA_Vec4V::GetYIntrin128() const
	{
		return y;
	}

	__forceinline Vec::Vector_4V_Out SoA_Vec4V::GetZIntrin128() const
	{
		return z;
	}

	__forceinline Vec::Vector_4V_Out SoA_Vec4V::GetWIntrin128() const
	{
		return w;
	}

	__forceinline Vec::Vector_4V_Ref SoA_Vec4V::GetXIntrin128Ref()
	{
		return x;
	}

	__forceinline Vec::Vector_4V_Ref SoA_Vec4V::GetYIntrin128Ref()
	{
		return x;
	}

	__forceinline Vec::Vector_4V_Ref SoA_Vec4V::GetZIntrin128Ref()
	{
		return z;
	}

	__forceinline Vec::Vector_4V_Ref SoA_Vec4V::GetWIntrin128Ref()
	{
		return w;
	}

	__forceinline SoA_ScalarV_Out SoA_Vec4V::GetX() const
	{
		return SoA_ScalarV( x );
	}

	__forceinline SoA_ScalarV_Out SoA_Vec4V::GetY() const
	{
		return SoA_ScalarV( y );
	}

	__forceinline SoA_ScalarV_Out SoA_Vec4V::GetZ() const
	{
		return SoA_ScalarV( z );
	}

	__forceinline SoA_ScalarV_Out SoA_Vec4V::GetW() const
	{
		return SoA_ScalarV( w );
	}

	__forceinline SoA_Vec3V_Out SoA_Vec4V::GetXYZ() const
	{
		return SoA_Vec3V( x, y, z );
	}

	__forceinline void SoA_Vec4V::SetX( SoA_ScalarV_In newX )
	{
		x = newX.GetIntrin128();
	}

	__forceinline void SoA_Vec4V::SetY( SoA_ScalarV_In newY )
	{
		y = newY.GetIntrin128();
	}

	__forceinline void SoA_Vec4V::SetZ( SoA_ScalarV_In newZ )
	{
		z = newZ.GetIntrin128();
	}

	__forceinline void SoA_Vec4V::SetW( SoA_ScalarV_In newW )
	{
		w = newW.GetIntrin128();
	}

	__forceinline SoA_QuatV_Out SoA_Vec4V::AsQuatV() const
	{
		return SoA_QuatV( x, y, z, w );
	}

	__forceinline void SoA_Vec4V::ZeroComponents()
	{
		x = y = z = w = Vec::V4VConstant(V_ZERO);
	}

	//============================================================================
	// Operators

	__forceinline SoA_VecBool4V_Out SoA_Vec4V::operator== (SoA_Vec4V_In b) const
	{
		return SoA_VecBool4V(
						Vec::V4IsEqualV( x, b.x ),
						Vec::V4IsEqualV( y, b.y ),
						Vec::V4IsEqualV( z, b.z ),
						Vec::V4IsEqualV( w, b.w )
						);
	}

	__forceinline SoA_VecBool4V_Out SoA_Vec4V::operator!= (SoA_Vec4V_In b) const
	{
		return SoA_VecBool4V(
						Vec::V4InvertBits( Vec::V4IsEqualV( x, b.x ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( y, b.y ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( z, b.z ) ),
						Vec::V4InvertBits( Vec::V4IsEqualV( w, b.w ) )
						);
	}

	__forceinline SoA_VecBool4V_Out	SoA_Vec4V::operator< (SoA_Vec4V_In bigVector) const
	{
		return SoA_VecBool4V(
						Vec::V4IsLessThanV( x, bigVector.x ),
						Vec::V4IsLessThanV( y, bigVector.y ),
						Vec::V4IsLessThanV( z, bigVector.z ),
						Vec::V4IsLessThanV( w, bigVector.w )
						);
	}

	__forceinline SoA_VecBool4V_Out	SoA_Vec4V::operator<= (SoA_Vec4V_In bigVector) const
	{
		return SoA_VecBool4V(
						Vec::V4IsLessThanOrEqualV( x, bigVector.x ),
						Vec::V4IsLessThanOrEqualV( y, bigVector.y ),
						Vec::V4IsLessThanOrEqualV( z, bigVector.z ),
						Vec::V4IsLessThanOrEqualV( w, bigVector.w )
						);
	}

	__forceinline SoA_VecBool4V_Out	SoA_Vec4V::operator> (SoA_Vec4V_In smallVector) const
	{
		return SoA_VecBool4V(
						Vec::V4IsGreaterThanV( x, smallVector.x ),
						Vec::V4IsGreaterThanV( y, smallVector.y ),
						Vec::V4IsGreaterThanV( z, smallVector.z ),
						Vec::V4IsGreaterThanV( w, smallVector.w )
						);
	}

	__forceinline SoA_VecBool4V_Out	SoA_Vec4V::operator>= (SoA_Vec4V_In smallVector) const
	{
		return SoA_VecBool4V(
						Vec::V4IsGreaterThanOrEqualV( x, smallVector.x ),
						Vec::V4IsGreaterThanOrEqualV( y, smallVector.y ),
						Vec::V4IsGreaterThanOrEqualV( z, smallVector.z ),
						Vec::V4IsGreaterThanOrEqualV( w, smallVector.w )
						);
	}

	__forceinline SoA_Vec4V_Out SoA_Vec4V::operator* (SoA_Vec4V_In b) const
	{
		return SoA_Vec4V(
					Vec::V4Scale( x, b.x ),
					Vec::V4Scale( y, b.y ),
					Vec::V4Scale( z, b.z ),
					Vec::V4Scale( w, b.w )
					);
	}

	__forceinline SoA_Vec4V_Out SoA_Vec4V::operator* (SoA_ScalarV_In b) const
	{
		return SoA_Vec4V(
					Vec::V4Scale( x, b.GetIntrin128() ),
					Vec::V4Scale( y, b.GetIntrin128() ),
					Vec::V4Scale( z, b.GetIntrin128() ),
					Vec::V4Scale( w, b.GetIntrin128() )
					);
	}

	__forceinline SoA_Vec4V_Out operator* (SoA_ScalarV_In a, SoA_Vec4V_In b)
	{
		return SoA_Vec4V(
					Vec::V4Scale( a.GetIntrin128(), b.GetXIntrin128() ),
					Vec::V4Scale( a.GetIntrin128(), b.GetYIntrin128() ),
					Vec::V4Scale( a.GetIntrin128(), b.GetZIntrin128() ),
					Vec::V4Scale( a.GetIntrin128(), b.GetWIntrin128() )
					);
	}

	__forceinline SoA_Vec4V_Out SoA_Vec4V::operator/ (SoA_Vec4V_In b) const
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

		// Invert #2
		Vec::Vector_4V rsqrt2 = Vec::V4InvSqrtFast(b.z);
		Vec::Vector_4V squaredEstimate2 = Vec::V4Scale(rsqrt2, rsqrt2);
		Vec::Vector_4V halfEstimate2 = Vec::V4Scale(v_half, rsqrt2);
		Vec::Vector_4V inv2 = Vec::V4AddScaled(rsqrt2, Vec::V4SubtractScaled(v_one, squaredEstimate2, b.z), halfEstimate2);

		// Invert #3
		Vec::Vector_4V rsqrt3 = Vec::V4InvSqrtFast(b.w);
		Vec::Vector_4V squaredEstimate3 = Vec::V4Scale(rsqrt3, rsqrt3);
		Vec::Vector_4V halfEstimate3 = Vec::V4Scale(v_half, rsqrt3);
		Vec::Vector_4V inv3 = Vec::V4AddScaled(rsqrt3, Vec::V4SubtractScaled(v_one, squaredEstimate3, b.w), halfEstimate3);

		return SoA_Vec4V(
			Vec::V4Scale( x, inv0 ),
			Vec::V4Scale( y, inv1 ),
			Vec::V4Scale( z, inv2 ),
			Vec::V4Scale( w, inv3 )
			);
	}

	__forceinline SoA_Vec4V_Out SoA_Vec4V::operator/ (SoA_ScalarV_In b) const
	{
		Vec::Vector_4V inv0 = Vec::V4Invert( b.GetIntrin128() );
		return SoA_Vec4V(
			Vec::V4Scale( x, inv0 ),
			Vec::V4Scale( y, inv0 ),
			Vec::V4Scale( z, inv0 ),
			Vec::V4Scale( w, inv0 )
			);
	}

	__forceinline SoA_Vec4V_Out SoA_Vec4V::operator+ (SoA_Vec4V_In b) const
	{
		return SoA_Vec4V(
					Vec::V4Add( x, b.x ),
					Vec::V4Add( y, b.y ),
					Vec::V4Add( z, b.z ),
					Vec::V4Add( w, b.w )
					);
	}

	__forceinline SoA_Vec4V_Out SoA_Vec4V::operator- (SoA_Vec4V_In b) const
	{
		return SoA_Vec4V(
					Vec::V4Subtract( x, b.x ),
					Vec::V4Subtract( y, b.y ),
					Vec::V4Subtract( z, b.z ),
					Vec::V4Subtract( w, b.w )
					);
	}

	__forceinline void SoA_Vec4V::operator*= (SoA_Vec4V_In b)
	{
		x = Vec::V4Scale( x, b.x );
		y = Vec::V4Scale( y, b.y );
		z = Vec::V4Scale( z, b.z );
		w = Vec::V4Scale( w, b.w );
	}

	__forceinline void SoA_Vec4V::operator*= (SoA_ScalarV_In b)
	{
		x = Vec::V4Scale( x, b.GetIntrin128() );
		y = Vec::V4Scale( y, b.GetIntrin128() );
		z = Vec::V4Scale( z, b.GetIntrin128() );
		w = Vec::V4Scale( w, b.GetIntrin128() );
	}

	__forceinline void SoA_Vec4V::operator/= (SoA_Vec4V_In b)
	{
		(*this) = (*this) / b;
	}

	__forceinline void SoA_Vec4V::operator/= (SoA_ScalarV_In b)
	{
		(*this) = (*this) / b;
	}

	__forceinline void SoA_Vec4V::operator+= (SoA_Vec4V_In b)
	{
		x = Vec::V4Add( x, b.x );
		y = Vec::V4Add( y, b.y );
		z = Vec::V4Add( z, b.z );
		w = Vec::V4Add( w, b.w );
	}

	__forceinline void SoA_Vec4V::operator-= (SoA_Vec4V_In b)
	{
		x = Vec::V4Subtract( x, b.x );
		y = Vec::V4Subtract( y, b.y );
		z = Vec::V4Subtract( z, b.z );
		w = Vec::V4Subtract( w, b.w );
	}

	__forceinline SoA_Vec4V_Out SoA_Vec4V::operator- () const
	{
		Vec::Vector_4V INT_80000000 = Vec::V4VConstant(V_80000000);
		return SoA_Vec4V(
					Vec::V4Xor( x, INT_80000000 ),
					Vec::V4Xor( y, INT_80000000 ),
					Vec::V4Xor( z, INT_80000000 ),
					Vec::V4Xor( w, INT_80000000 )
					);
	}

	__forceinline SoA_Vec4V_Out SoA_Vec4V::operator| (SoA_Vec4V_In b) const
	{
		return SoA_Vec4V(
					Vec::V4Or( x, b.x ),
					Vec::V4Or( y, b.y ),
					Vec::V4Or( z, b.z ),
					Vec::V4Or( w, b.w )
					);
	}

	__forceinline SoA_Vec4V_Out SoA_Vec4V::operator& (SoA_Vec4V_In b) const
	{
		return SoA_Vec4V(
					Vec::V4And( x, b.x ),
					Vec::V4And( y, b.y ),
					Vec::V4And( z, b.z ),
					Vec::V4And( w, b.w)
					);
	}

	__forceinline SoA_Vec4V_Out SoA_Vec4V::operator^ (SoA_Vec4V_In b) const
	{
		return SoA_Vec4V(
					Vec::V4Xor( x, b.x ),
					Vec::V4Xor( y, b.y ),
					Vec::V4Xor( z, b.z ),
					Vec::V4Xor( w, b.w )
					);
	}

	__forceinline void SoA_Vec4V::operator|= (SoA_Vec4V_In b)
	{
		x = Vec::V4Or( x, b.x );
		y = Vec::V4Or( y, b.y );
		z = Vec::V4Or( z, b.z );
		w = Vec::V4Or( z, b.w );
	}

	__forceinline void SoA_Vec4V::operator&= (SoA_Vec4V_In b)
	{
		x = Vec::V4And( x, b.x );
		y = Vec::V4And( y, b.y );
		z = Vec::V4And( z, b.z );
		w = Vec::V4And( w, b.w);
	}

	__forceinline void SoA_Vec4V::operator^= (SoA_Vec4V_In b)
	{
		x = Vec::V4Xor( x, b.x );
		y = Vec::V4Xor( y, b.y );
		z = Vec::V4Xor( z, b.z );
		w = Vec::V4Xor( w, b.w );
	}

	__forceinline Vec::Vector_4V_ConstRef SoA_Vec4V::operator[] (u32 elem) const
	{
		VecAssertMsg( elem <= 3 , "Invalid element index." );
		return (&x)[elem];
	}

	__forceinline Vec::Vector_4V_Ref SoA_Vec4V::operator[] (u32 elem)
	{
		VecAssertMsg( elem <= 3 , "Invalid element index." );
		return (&x)[elem];
	}
} // namespace rage
