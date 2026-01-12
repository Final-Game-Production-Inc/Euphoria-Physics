namespace rage
{
namespace Vec
{

	__forceinline unsigned int V2IsNotNanAll(Vector_4V_In inVector)
	{
#if !__SPU
		return V2IsEqualAll(inVector, inVector);
#else
		qword firstNanShl1 = si_iohl(si_ilhu(0xff00),2);
		qword inVectorShl1 = si_a((qword)inVector,(qword)inVector);
		qword mask = si_clgt(firstNanShl1,inVectorShl1);
		return si_to_uint(si_clgti(si_gb(mask),11));
#endif
	}

	__forceinline unsigned int V2IsFiniteAll( Vector_4V_In inVector )
	{
#if !__SPU
		vec_uint4 t0 = (vec_uint4)vec_vspltisw(-1);
		vec_uint4 t1 = (vec_uint4)vec_vspltisw(-8);         // -8 ≡ 24 mod 32
		vec_uint4 t2 = (vec_uint4)vec_vspltisw(1);
		vec_uint4 t3 = vec_vslw(t0, t1);                    // 0xff000000
		vec_uint4 in = vec_vslw((vec_uint4)inVector, t2);
		in = vec_vsldoi(t2, in, 8);
		return vec_all_gt(t3, in);
#else
		qword infShl1 = si_ilhu(0xff00);
		qword inVectorShl1 = si_a((qword)inVector,(qword)inVector);
		qword mask = si_clgt(infShl1,inVectorShl1);
		return si_to_uint(si_clgti(si_gb(mask),11));
#endif
	}

	__forceinline void V2Set( Vector_4V_InOut inoutVector, float x0, float y0 )
	{
		inoutVector = VECTOR4V_LITERAL(x0, y0, x0, x0);

		// (The "else" part of the code below is causing "uninitialized variable" warnings.)

		//if (__builtin_constant_p(x0) & __builtin_constant_p(y0))
		//{
		//	inoutVector = VECTOR4V_LITERAL(x0, y0, x0, x0);
		//}
		//else
		//{
		//	f32 *pVect = (f32*)&inoutVector;
		//	pVect[0] = x0;
		//	pVect[1] = y0;
		//}
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
		Vector_4V inV1 = V4Permute<X, X, Y, Y>( inVector1 );
		Vector_4V inV2 = V4Permute<X, X, Y, Y>( inVector2 );

		return vec_all_eq( (Vector_4V_uint)inV1, (Vector_4V_uint)inV2 );
	}

	__forceinline unsigned int V2IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V inV1 = V4Permute<X, X, Y, Y>( inVector1 );
		Vector_4V inV2 = V4Permute<X, X, Y, Y>( inVector2 );

		return vec_all_ne( (Vector_4V_uint)inV1, (Vector_4V_uint)inV2 );
	}

	__forceinline unsigned int V2IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V inV1 = V4Permute<X, X, Y, Y>( inVector1 );
		Vector_4V inV2 = V4Permute<X, X, Y, Y>( inVector2 );

		return vec_all_eq( inV1, inV2 );
	}

	__forceinline unsigned int V2IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V inV1 = V4Permute<X, X, Y, Y>( inVector1 );
		Vector_4V inV2 = V4Permute<X, X, Y, Y>( inVector2 );

		return vec_all_ne( inV1, inV2 );
	}

	__forceinline unsigned int V2IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V bigV = V4Permute<X, X, Y, Y>( bigVector );
		Vector_4V smallV = V4Permute<X, X, Y, Y>( smallVector );

		return vec_all_gt( bigV, smallV );
	}

	__forceinline unsigned int V2IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V bigV = V4Permute<X, X, Y, Y>( bigVector );
		Vector_4V smallV = V4Permute<X, X, Y, Y>( smallVector );

		return vec_all_ge( bigV, smallV );
	}

	__forceinline unsigned int V2IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V bigV = V4Permute<X, X, Y, Y>( bigVector );
		Vector_4V smallV = V4Permute<X, X, Y, Y>( smallVector );

		return vec_all_lt( smallV, bigV );
	}

	__forceinline unsigned int V2IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V bigV = V4Permute<X, X, Y, Y>( bigVector );
		Vector_4V smallV = V4Permute<X, X, Y, Y>( smallVector );

		return vec_all_le( smallV, bigV );
	}

	
} // namespace Vec
} // namespace rage
