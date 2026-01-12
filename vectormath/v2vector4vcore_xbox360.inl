namespace rage
{
namespace Vec
{


	__forceinline unsigned int V2IsNotNanAll(Vector_4V_In inVector)
	{
		return V2IsEqualAll(inVector, inVector);
	}

	__forceinline unsigned int V2IsFiniteAll( Vector_4V_In inVector )
	{
		__vector4 t0 = __vspltisw(-1);
		__vector4 t1 = __vspltisw(-8);          // -8 ≡ 24 mod 32
		__vector4 t2 = __vspltisw(1);
		__vector4 t3 = __vslw(t0, t1);          // 0xff000000
		__vector4 in = __vslw(inVector, t2);
		in = __vsldoi(t2, in, 8);
		unsigned int cr;
		__vcmpgtuwR(t3, in, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline void V2Set( Vector_4V_InOut inoutVector, float x0, float y0 )
	{
		SetX( inoutVector, x0 );
		SetY( inoutVector, y0 );
	}

	__forceinline Vector_4V_Out V2DotV(Vector_4V_In a, Vector_4V_In b)
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		// Zero out the elements we don't care about...
		Vector_4V dot1 = __vsldoi(_zero, a, 8);
		Vector_4V dot2 = __vsldoi(_zero, b, 8);
		// ...then do the full dot product.
		return V4DotV( dot1, dot2 );
	}

	__forceinline Vector_4V_Out V2MagSquaredV( Vector_4V_In v )
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		// Zero out the elements we don't care about...
		Vector_4V dot1 = __vsldoi(_zero, v, 8);
		// ...then do the full dot product.
		return V4DotV( dot1, dot1 );
	}

	//============================================================================
	// Comparison functions

	__forceinline unsigned int V2IsEqualIntAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, X, Y, Y>( inVector1 );
		inVector2 = V4Permute<X, X, Y, Y>( inVector2 );

		unsigned int cr;
		__vcmpequwR( inVector1, inVector2, &cr );
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V2IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, X, Y, Y>( inVector1 );
		inVector2 = V4Permute<X, X, Y, Y>( inVector2 );

		unsigned int cr;
		__vcmpequwR( inVector1, inVector2, &cr );
		return (cr & 0x20) >> 5;
	}

	__forceinline unsigned int V2IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, X, Y, Y>( inVector1 );
		inVector2 = V4Permute<X, X, Y, Y>( inVector2 );

		unsigned int cr;
		__vcmpeqfpR(inVector1, inVector2, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V2IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, X, Y, Y>( inVector1 );
		inVector2 = V4Permute<X, X, Y, Y>( inVector2 );

		unsigned int cr;
		__vcmpeqfpR(inVector1, inVector2, &cr);
		return (cr & 0x20) >> 5;
	}

	__forceinline unsigned int V2IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		bigVector = V4Permute<X, X, Y, Y>( bigVector );
		smallVector = V4Permute<X, X, Y, Y>( smallVector );

		unsigned int cr;
		__vcmpgtfpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V2IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		bigVector = V4Permute<X, X, Y, Y>( bigVector );
		smallVector = V4Permute<X, X, Y, Y>( smallVector );

		unsigned int cr;
		__vcmpgefpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V2IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		bigVector = V4Permute<X, X, Y, Y>( bigVector );
		smallVector = V4Permute<X, X, Y, Y>( smallVector );

		unsigned int cr;
		__vcmpgtfpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V2IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		bigVector = V4Permute<X, X, Y, Y>( bigVector );
		smallVector = V4Permute<X, X, Y, Y>( smallVector );

		unsigned int cr;
		__vcmpgefpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}

	
} // namespace Vec
} // namespace rage
