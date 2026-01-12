namespace rage
{
namespace Vec
{

	__forceinline unsigned int V3IsNotNanAll(Vector_4V_In inVector)
	{
#if !__SPU
		return V3IsEqualAll(inVector, inVector);
#else
		qword firstNanShl1 = si_iohl(si_ilhu(0xff00),2);
		qword inVectorShl1 = si_a((qword)inVector,(qword)inVector);
		qword mask = si_clgt(firstNanShl1,inVectorShl1);
		return si_to_uint(si_clgti(si_gb(mask),13));
#endif
	}

	__forceinline unsigned int V3IsFiniteAll( Vector_4V_In inVector )
	{
#if !__SPU
		vec_uint4 t0 = (vec_uint4)vec_vspltisw(-1);
		vec_uint4 t1 = (vec_uint4)vec_vspltisw(-8);         // -8 ≡ 24 mod 32
		vec_uint4 t2 = (vec_uint4)vec_vspltisw(1);
		vec_uint4 t3 = vec_vslw(t0, t1);                    // 0xff000000
		vec_uint4 in = vec_vslw((vec_uint4)inVector, t2);
		in = vec_vsldoi(t2, in, 12);
		return vec_all_gt(t3, in);
#else
		qword infShl1 = si_ilhu(0xff00);
		qword inVectorShl1 = si_a((qword)inVector,(qword)inVector);
		qword mask = si_clgt(infShl1,inVectorShl1);
		return si_to_uint(si_clgti(si_gb(mask),13));
#endif
	}

	__forceinline void V3Set( Vector_4V_InOut inoutVector, float x0, float y0, float z0 )
	{
		inoutVector = VECTOR4V_LITERAL(x0, y0, z0, x0);

		// (The "else" part of the code below is causing "uninitialized variable" warnings.)

		//if (__builtin_constant_p(x0) & __builtin_constant_p(y0) & __builtin_constant_p(z0))
		//{
		//	inoutVector = VECTOR4V_LITERAL(x0, y0, z0, x0);
		//}
		//else
		//{
			//f32 *pVect = (f32*)&inoutVector;
			//pVect[0] = x0;
			//pVect[1] = y0;
			//pVect[2] = z0;
		//}
	}

	__forceinline Vector_4V_Out V3DotV(Vector_4V_In a, Vector_4V_In b)
	{
		//Vector_4V vecProd = V4Scale( a, b );
		//return V4Add( V4Add( V4SplatX(vecProd), V4SplatY(vecProd) ), V4SplatZ(vecProd) );

		//// TODO: Faster? Maybe. Same # of dependent instructions (4). vec_sld() and vec_madd() are on different pipelines, btw.
		Vector_4V vecProd = V4Scale( a, b );
		vecProd = V4AddScaled( vecProd, vec_sld(a, a, 4), vec_sld(b, b, 4) );
		vecProd = V4AddScaled( vecProd, vec_sld(a, a, 8), vec_sld(b, b, 8) );
		return V4SplatX( vecProd );
	}

	__forceinline Vector_4V_Out V3MagSquaredV( Vector_4V_In v )
	{
		Vector_4V vecProd = V4Scale( v, v );
		return V4Add( V4Add( V4SplatX(vecProd), V4SplatY(vecProd) ), V4SplatZ(vecProd) );

		//// TODO: Faster? Maybe. Same # of dependent instructions (4). vec_sld() and vec_madd() are on different pipelines, btw.
		//Vector_4V vecProd = V4Scale( v, v );
		//Vector_4V tempv1 = vec_sld(v, v, 4);
		//Vector_4V tempv2 = vec_sld(v, v, 8);
		//vecProd = V4AddScaled( vecProd, tempv1, tempv1 );
		//vecProd = V4AddScaled( vecProd, tempv2, tempv2 );
		//return V4SplatX( vecProd );
	}

	__forceinline Vector_4V_Out V3MagXYSquaredV( Vector_4V_In v )
	{
		Vector_4V _x = V4SplatX( v );
		Vector_4V _y = V4SplatY( v );
		Vector_4V _xx = V4Scale( _x, _x );
		Vector_4V _xx_plus_yy = V4AddScaled( _xx, _y, _y );
		return _xx_plus_yy;
	}

	__forceinline Vector_4V_Out V3MagXZSquaredV( Vector_4V_In v )
	{
		Vector_4V _x = V4SplatX( v );
		Vector_4V _z = V4SplatZ( v );
		Vector_4V _xx = V4Scale( _x, _x );
		Vector_4V _xx_plus_zz = V4AddScaled( _xx, _z, _z );
		return _xx_plus_zz;
	}

	__forceinline Vector_4V_Out V3MagYZVSquared( Vector_4V_In v )
	{
		Vector_4V _y = V4SplatY( v );
		Vector_4V _z = V4SplatZ( v );
		Vector_4V _yy = V4Scale( _y, _y );
		Vector_4V _yy_plus_zz = V4AddScaled( _yy, _z, _z );
		return _yy_plus_zz;
	}

	//============================================================================
	// Comparison functions

#if __SPU

	__forceinline unsigned int V3IsEqualIntAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, Y, Z, X>( inVector1 ); // This is a 2-instruction permute on PS3 PPU, if v4perm2instructions_ps3.inl is #included from v4vector4vcore_ps3.inl.
		inVector2 = V4Permute<X, Y, Z, X>( inVector2 );

		return vec_all_eq( (Vector_4V_uint)inVector1, (Vector_4V_uint)inVector2 );
	}

	__forceinline unsigned int V3IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, Y, Z, X>( inVector1 );
		inVector2 = V4Permute<X, Y, Z, X>( inVector2 );

		return vec_all_ne( (Vector_4V_uint)inVector1, (Vector_4V_uint)inVector2 );
	}

	__forceinline unsigned int V3IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, Y, Z, X>( inVector1 );
		inVector2 = V4Permute<X, Y, Z, X>( inVector2 );

		return vec_all_eq( inVector1, inVector2 );
	}

	__forceinline unsigned int V3IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		inVector1 = V4Permute<X, Y, Z, X>( inVector1 );
		inVector2 = V4Permute<X, Y, Z, X>( inVector2 );

		return vec_all_ne( inVector1, inVector2 );
	}

	__forceinline unsigned int V3IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		bigVector = V4Permute<X, Y, Z, X>( bigVector );
		smallVector = V4Permute<X, Y, Z, X>( smallVector );

		return vec_all_gt( bigVector, smallVector );
	}

	__forceinline unsigned int V3IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		bigVector = V4Permute<X, Y, Z, X>( bigVector );
		smallVector = V4Permute<X, Y, Z, X>( smallVector );

		return vec_all_ge( bigVector, smallVector );
	}

	__forceinline unsigned int V3IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		bigVector = V4Permute<X, Y, Z, X>( bigVector );
		smallVector = V4Permute<X, Y, Z, X>( smallVector );

		return vec_all_lt( smallVector, bigVector );
	}

	__forceinline unsigned int V3IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		bigVector = V4Permute<X, Y, Z, X>( bigVector );
		smallVector = V4Permute<X, Y, Z, X>( smallVector );

		return vec_all_le( smallVector, bigVector );
	}

#else // not __SPU -- faster versions without the "expensive" XYZX permute

	__forceinline unsigned int V3IsEqualIntAll(Vector_4V_In a, Vector_4V_In b)
	{
		const Vector_4V z = (Vector_4V)vec_splat_s8(0);
		return vec_all_eq((Vector_4V_uint)vec_sld(z, a, 12), (Vector_4V_uint)vec_sld(z, b, 12));
	}

	__forceinline unsigned int V3IsEqualIntNone(Vector_4V_In a, Vector_4V_In b)
	{
		const Vector_4V g = (Vector_4V)vec_splat_s8(1);
		const Vector_4V z = (Vector_4V)vec_splat_s8(0);
		return vec_all_ne((Vector_4V_uint)vec_sld(g, a, 12), (Vector_4V_uint)vec_sld(z, b, 12));
	}

	__forceinline unsigned int V3IsEqualAll(Vector_4V_In a, Vector_4V_In b)
	{
		const Vector_4V z = (Vector_4V)vec_splat_s8(0);
		return vec_all_eq(vec_sld(z, a, 12), vec_sld(z, b, 12));
	}

	__forceinline unsigned int V3IsEqualNone(Vector_4V_In a, Vector_4V_In b)
	{
		const Vector_4V g = (Vector_4V)vec_splat_s8(1); // '0x01010101' is something != 0 in floating-point
		const Vector_4V z = (Vector_4V)vec_splat_s8(0);
		return vec_all_ne(vec_sld(g, a, 12), vec_sld(z, b, 12));
	}

	__forceinline unsigned int V3IsGreaterThanAll(Vector_4V_In a, Vector_4V_In b)
	{
		const Vector_4V g = (Vector_4V)vec_splat_s8(1); // '0x01010101' is something > 0 in floating-point
		const Vector_4V z = (Vector_4V)vec_splat_s8(0);
		return vec_all_gt(vec_sld(g, a, 12), vec_sld(z, b, 12));
	}

	__forceinline unsigned int V3IsGreaterThanOrEqualAll(Vector_4V_In a, Vector_4V_In b)
	{
		const Vector_4V z = (Vector_4V)vec_splat_s8(0);
		return vec_all_ge(vec_sld(z, a, 12), vec_sld(z, b, 12));
	}

	__forceinline unsigned int V3IsLessThanAll(Vector_4V_In a, Vector_4V_In b)
	{
		const Vector_4V g = (Vector_4V)vec_splat_s8(1); // '0x01010101' is something > 0 in floating-point
		const Vector_4V z = (Vector_4V)vec_splat_s8(0);
		return vec_all_lt(vec_sld(z, a, 12), vec_sld(g, b, 12));
	}

	__forceinline unsigned int V3IsLessThanOrEqualAll(Vector_4V_In a, Vector_4V_In b)
	{
		const Vector_4V z = (Vector_4V)vec_splat_s8(0);
		return vec_all_le(vec_sld(z, a, 12), vec_sld(z, b, 12));
	}

#endif // not __SPU

} // namespace Vec
} // namespace rage
