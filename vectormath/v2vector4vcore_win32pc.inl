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
		return V2IsLessThanAll(V4Abs(inVector),V4VConstant(V_INF));
	}

	__forceinline void V2Set( Vector_4V_InOut inoutVector, float x0, float y0 )
	{
		inoutVector = _mm_setr_ps( x0, y0, x0, x0 );
	}

	__forceinline Vector_4V_Out V2DotV(Vector_4V_In a, Vector_4V_In b)
	{
		Vector_4V vectProdXY = V4Scale( a, b );
		Vector_4V vectProdXXXX = V4SplatX( vectProdXY );
		Vector_4V vectProdYYYY = V4SplatY( vectProdXY );
		return V4Add( vectProdXXXX, vectProdYYYY );
	}

	__forceinline Vector_4V_Out V2MagSquaredV( Vector_4V_In v )
	{
		Vector_4V vectProdXY = V4Scale( v, v );
		Vector_4V vectProdXXXX = V4SplatX( vectProdXY );
		Vector_4V vectProdYYYY = V4SplatY( vectProdXY );
		return V4Add( vectProdXXXX, vectProdYYYY );
	}

	//============================================================================
	// Comparison functions

	__forceinline unsigned int V2IsEqualIntAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x3) == 0x3 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//return ((xy_yy_zw_wwResult.m128_u32[0]) & 0x1);
	}

	__forceinline unsigned int V2IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x3) == 0x0 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4Or( xyzwResult, yywwResult );
		//return ((xy_yy_zw_wwResult.m128_u32[0]) & 0x1) ^ 0x1;
	}

	__forceinline unsigned int V2IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x3) == 0x3 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//return ((xy_yy_zw_wwResult.m128_u32[0]) & 0x1);
	}

	__forceinline unsigned int V2IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x3) == 0x0 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4Or( xyzwResult, yywwResult );
		//return ((xy_yy_zw_wwResult.m128_u32[0]) & 0x1) ^ 0x1;
	}

	__forceinline unsigned int V2IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V xyzwResult = V4IsGreaterThanV( bigVector, smallVector );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x3) == 0x3 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsGreaterThanV( bigVector, smallVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//return ((xy_yy_zw_wwResult.m128_u32[0]) & 0x1);
	}

	__forceinline unsigned int V2IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V xyzwResult = V4IsGreaterThanOrEqualV( bigVector, smallVector );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x3) == 0x3 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsGreaterThanOrEqualV( bigVector, smallVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//return ((xy_yy_zw_wwResult.m128_u32[0]) & 0x1);
	}

	__forceinline unsigned int V2IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V xyzwResult = V4IsLessThanV( smallVector, bigVector );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x3) == 0x3 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsLessThanV( smallVector, bigVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//return ((xy_yy_zw_wwResult.m128_u32[0]) & 0x1);
	}

	__forceinline unsigned int V2IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V xyzwResult = V4IsLessThanOrEqualV( smallVector, bigVector );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x3) == 0x3 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsLessThanOrEqualV( smallVector, bigVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//return ((xy_yy_zw_wwResult.m128_u32[0]) & 0x1);
	}

	
} // namespace Vec
} // namespace rage
