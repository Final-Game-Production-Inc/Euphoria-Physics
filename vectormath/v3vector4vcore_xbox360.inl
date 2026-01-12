namespace rage
{
namespace Vec
{


	__forceinline unsigned int V3IsNotNanAll(Vector_4V_In inVector)
	{
		return V3IsEqualAll(inVector, inVector);
	}

	__forceinline unsigned int V3IsFiniteAll( Vector_4V_In inVector )
	{
		__vector4 t0 = __vspltisw(-1);
		__vector4 t1 = __vspltisw(-8);          // -8 ≡ 24 mod 32
		__vector4 t2 = __vspltisw(1);
		__vector4 t3 = __vslw(t0, t1);          // 0xff000000
		__vector4 in = __vslw(inVector, t2);
		in = __vsldoi(t2, in, 12);
		unsigned int cr;
		__vcmpgtuwR(t3, in, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline void V3Set( Vector_4V_InOut inoutVector, float x0, float y0, float z0 )
	{
		SetX( inoutVector, x0 );
		SetY( inoutVector, y0 );
		SetZ( inoutVector, z0 );
		SetW( inoutVector, x0 );
	}

	__forceinline Vector_4V_Out V3DotV(Vector_4V_In a, Vector_4V_In b)
	{
		return __vdot3fp(a, b);
	}

	__forceinline Vector_4V_Out V3MagSquaredV( Vector_4V_In v )
	{
		return __vdot3fp(v, v);
	}

	__forceinline Vector_4V_Out V3MagXYSquaredV( Vector_4V_In v )
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _x0y0 = V4MergeXY( v, _zero );
		return __vdot3fp( _x0y0, _x0y0 );

		// Other platforms use this:
		//Vector_4V _x = V4SplatX( v );
		//Vector_4V _y = V4SplatY( v );
		//Vector_4V _xx = V4Scale( _x, _x );
		//Vector_4V _xx_plus_yy = V4AddScaled( _xx, _y, _y );
		//return _xx_plus_yy;
	}

	__forceinline Vector_4V_Out V3MagXZSquaredV( Vector_4V_In v )
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _x0z0 = V4PermuteTwo<X1,W2,Z1,Y2>( v, _zero );
		return __vdot3fp( _x0z0, _x0z0 );

		// Other platforms use this:
		//Vector_4V _x = V4SplatX( v );
		//Vector_4V _z = V4SplatZ( v );
		//Vector_4V _xx = V4Scale( _x, _x );
		//Vector_4V _xx_plus_zz = V4AddScaled( _xx, _z, _z );
		//return _xx_plus_zz;
	}

	__forceinline Vector_4V_Out V3MagYZSquaredV( Vector_4V_In v )
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _yz00 = V4PermuteTwo<Y1,Z1,Z2,W2>( v, _zero );
		return __vdot3fp( _yz00, _yz00 );

		// Other platforms use this:
		//Vector_4V _y = V4SplatY( v );
		//Vector_4V _z = V4SplatZ( v );
		//Vector_4V _yy = V4Scale( _y, _y );
		//Vector_4V _yy_plus_zz = V4AddScaled( _yy, _z, _z );
		//return _yy_plus_zz;
	}

	//============================================================================
	// Comparison functions

	__forceinline unsigned int V3IsEqualIntAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, Y, Z, X>( inVector1 );
		inVector2 = V4Permute<X, Y, Z, X>( inVector2 );

		unsigned int cr;
		__vcmpequwR( inVector1, inVector2, &cr );
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V3IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, Y, Z, X>( inVector1 );
		inVector2 = V4Permute<X, Y, Z, X>( inVector2 );

		unsigned int cr;
		__vcmpequwR( inVector1, inVector2, &cr );
		return (cr & 0x20) >> 5;
	}

	__forceinline unsigned int V3IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, Y, Z, X>( inVector1 );
		inVector2 = V4Permute<X, Y, Z, X>( inVector2 );

		unsigned int cr;
		__vcmpeqfpR(inVector1, inVector2, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V3IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, Y, Z, X>( inVector1 );
		inVector2 = V4Permute<X, Y, Z, X>( inVector2 );

		unsigned int cr;
		__vcmpeqfpR(inVector1, inVector2, &cr);
		return (cr & 0x20) >> 5;
	}

	__forceinline unsigned int V3IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		bigVector = V4Permute<X, Y, Z, X>( bigVector );
		smallVector = V4Permute<X, Y, Z, X>( smallVector );

		unsigned int cr;
		__vcmpgtfpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V3IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		bigVector = V4Permute<X, Y, Z, X>( bigVector );
		smallVector = V4Permute<X, Y, Z, X>( smallVector );

		unsigned int cr;
		__vcmpgefpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V3IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		bigVector = V4Permute<X, Y, Z, X>( bigVector );
		smallVector = V4Permute<X, Y, Z, X>( smallVector );

		unsigned int cr;
		__vcmpgtfpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V3IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		bigVector = V4Permute<X, Y, Z, X>( bigVector );
		smallVector = V4Permute<X, Y, Z, X>( smallVector );

		unsigned int cr;
		__vcmpgefpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}


	
} // namespace Vec
} // namespace rage
