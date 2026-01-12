
#if RSG_CPU_INTEL && !defined(__ORBIS__)	// really should be checking for GCC/CLANG here.
#pragma warning(disable: 4714)
#endif // RSG_CPU_INTEL

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
		return V3IsLessThanAll(V4Abs(inVector),V4VConstant(V_INF));
	}

	__forceinline void V3Set( Vector_4V_InOut inoutVector, float x0, float y0, float z0 )
	{
		inoutVector = _mm_setr_ps( x0, y0, z0, x0 );
	}

	__forceinline Vector_4V_Out V3DotV(Vector_4V_In a, Vector_4V_In b)
	{
		// TODO: SSE3+ would help here.

		Vector_4V vecProd = V4Scale( a, b );
		return V4Add( V4Add( V4SplatX(vecProd), V4SplatY(vecProd) ), V4SplatZ(vecProd) );
	}

	__forceinline Vector_4V_Out V3MagSquaredV( Vector_4V_In v )
	{
		Vector_4V vecProd = V4Scale( v, v );
		return V4Add( V4Add( V4SplatX(vecProd), V4SplatY(vecProd) ), V4SplatZ(vecProd) );
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

	__forceinline Vector_4V_Out V3MagYZSquaredV( Vector_4V_In v )
	{
		Vector_4V _y = V4SplatY( v );
		Vector_4V _z = V4SplatZ( v );
		Vector_4V _yy = V4Scale( _y, _y );
		Vector_4V _yy_plus_zz = V4AddScaled( _yy, _z, _z );
		return _yy_plus_zz;
	}

	//============================================================================
	// Comparison functions

	__forceinline unsigned int V3IsEqualIntAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		// METHOD 1:
		Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x7) == 0x7 ? 1u : 0u);
		//004010C0  pcmpeqd     xmm0,xmm1 
		//004010C4  movmskps    eax,xmm0 
		//004010C7  and         eax,7 
		//004010CA  xor         ecx,ecx 
		//004010CC  cmp         al,7 
		//004010CE  sete        cl   
		//004010D1  mov         eax,ecx

		// METHOD 2:
		//inVector1 = V4Permute<X, Y, Z, X>( inVector1 );
		//inVector2 = V4Permute<X, Y, Z, X>( inVector2 );
		//Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		//int result = _mm_movemask_ps( xyzwResult );
		//return ((result) == 0x7 ? 1u : 0u);

		//00401200  shufps      xmm0,xmm0,24h 
		//00401204  shufps      xmm1,xmm1,24h 
		//00401208  xor         ecx,ecx 
		//0040120A  pcmpeqd     xmm0,xmm1 
		//0040120E  movmskps    eax,xmm0 
		//00401211  cmp         eax,7 
		//00401214  sete        cl   
		//00401217  mov         eax,ecx 

		// METHOD 3:
		//Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zzzzResult = V4Permute<Z, Z, Z, Z>( xyzwResult );
		//Vector_4V xyz_yyz_zwz_wwzResult = V4And( xy_yy_zw_wwResult, zzzzResult );
		//return ((xyz_yyz_zwz_wwzResult.m128_u32[0]) & 0x1);

		//00401200  push        ebp  
		//00401201  mov         ebp,esp 
		//00401203  and         esp,0FFFFFFF0h 
		//00401206  sub         esp,10h 
		//00401209  pcmpeqd     xmm0,xmm1 
		//0040120D  movaps      xmm1,xmm0 
		//00401210  shufps      xmm1,xmm0,0F5h 
		//00401214  andps       xmm1,xmm0 
		//00401217  shufps      xmm0,xmm0,0AAh 
		//0040121B  andps       xmm1,xmm0 
		//0040121E  movaps      xmmword ptr [esp],xmm1 
		//00401222  mov         eax,dword ptr [esp] 
		//00401225  and         eax,1 
		//00401228  mov         esp,ebp 
		//0040122A  pop         ebp  

		
	}

	__forceinline unsigned int V3IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x7) == 0x0 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4Or( xyzwResult, yywwResult );
		//Vector_4V zzzzResult = V4Permute<Z, Z, Z, Z>( xyzwResult );
		//Vector_4V xyz_yyz_zwz_wwzResult = V4Or( xy_yy_zw_wwResult, zzzzResult );
		//return ((xyz_yyz_zwz_wwzResult.m128_u32[0]) & 0x1) ^ 0x1;
	}

	__forceinline unsigned int V3IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x7) == 0x7 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zzzzResult = V4Permute<Z, Z, Z, Z>( xyzwResult );
		//Vector_4V xyz_yyz_zwz_wwzResult = V4And( xy_yy_zw_wwResult, zzzzResult );
		//return ((xyz_yyz_zwz_wwzResult.m128_u32[0]) & 0x1);
	}

	__forceinline unsigned int V3IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x7) == 0x0 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4Or( xyzwResult, yywwResult );
		//Vector_4V zzzzResult = V4Permute<Z, Z, Z, Z>( xyzwResult );
		//Vector_4V xyz_yyz_zwz_wwzResult = V4Or( xy_yy_zw_wwResult, zzzzResult );
		//return ((xyz_yyz_zwz_wwzResult.m128_u32[0]) & 0x1) ^ 0x1;
	}

	__forceinline unsigned int V3IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V xyzwResult = V4IsGreaterThanV( bigVector, smallVector );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x7) == 0x7 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsGreaterThanV( bigVector, smallVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zzzzResult = V4Permute<Z, Z, Z, Z>( xyzwResult );
		//Vector_4V xyz_yyz_zwz_wwzResult = V4And( xy_yy_zw_wwResult, zzzzResult );
		//return ((xyz_yyz_zwz_wwzResult.m128_u32[0]) & 0x1);
	}

	__forceinline unsigned int V3IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V xyzwResult = V4IsGreaterThanOrEqualV( bigVector, smallVector );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x7) == 0x7 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsGreaterThanOrEqualV( bigVector, smallVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zzzzResult = V4Permute<Z, Z, Z, Z>( xyzwResult );
		//Vector_4V xyz_yyz_zwz_wwzResult = V4And( xy_yy_zw_wwResult, zzzzResult );
		//return ((xyz_yyz_zwz_wwzResult.m128_u32[0]) & 0x1);
	}

	__forceinline unsigned int V3IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V xyzwResult = V4IsLessThanV( smallVector, bigVector );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x7) == 0x7 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsLessThanV( smallVector, bigVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zzzzResult = V4Permute<Z, Z, Z, Z>( xyzwResult );
		//Vector_4V xyz_yyz_zwz_wwzResult = V4And( xy_yy_zw_wwResult, zzzzResult );
		//return ((xyz_yyz_zwz_wwzResult.m128_u32[0]) & 0x1);
	}

	__forceinline unsigned int V3IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V xyzwResult = V4IsLessThanOrEqualV( smallVector, bigVector );
		int result = _mm_movemask_ps( xyzwResult );
		return ((result & 0x7) == 0x7 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsLessThanOrEqualV( smallVector, bigVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zzzzResult = V4Permute<Z, Z, Z, Z>( xyzwResult );
		//Vector_4V xyz_yyz_zwz_wwzResult = V4And( xy_yy_zw_wwResult, zzzzResult );
		//return ((xyz_yyz_zwz_wwzResult.m128_u32[0]) & 0x1);
	}




} // namespace Vec
} // namespace rage
