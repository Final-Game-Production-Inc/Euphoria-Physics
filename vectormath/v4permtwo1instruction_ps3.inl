#ifdef __SNC__
#pragma diag_suppress 828
#endif

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X1,X1>( Vector_4V_In v1, Vector_4V_In UNUSED_PARAM(v2) )
{
	return V4SplatX( v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In UNUSED_PARAM(v2) )
{
	return V4MergeXY( v1, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeXY( v1, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In UNUSED_PARAM(v2) )
{
	return V4SplatY( v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,X1>( Vector_4V_In v1, Vector_4V_In UNUSED_PARAM(v2) )
{
	return vec_sld( v1, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return vec_sld( v1, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In UNUSED_PARAM(v2) )
{
	return V4SplatZ( v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In UNUSED_PARAM(v2) )
{
	return V4MergeZW( v1, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X1,Y1>( Vector_4V_In v1, Vector_4V_In UNUSED_PARAM(v2) )
{
	return vec_sld( v1, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return vec_sld( v1, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeZW( v1, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In UNUSED_PARAM(v2) )
{
	return vec_sld( v1, v1, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W1,W1>( Vector_4V_In v1, Vector_4V_In UNUSED_PARAM(v2) )
{
	return V4SplatW( v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return vec_sld( v1, v2, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeXY( v2, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X2,X2>( Vector_4V_In UNUSED_PARAM(v1), Vector_4V_In v2 )
{
	return V4SplatX( v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,Y2>( Vector_4V_In UNUSED_PARAM(v1), Vector_4V_In v2 )
{
	return V4MergeXY( v2, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y2,Y2>( Vector_4V_In UNUSED_PARAM(v1), Vector_4V_In v2 )
{
	return V4SplatY( v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return vec_sld( v2, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,X2>( Vector_4V_In UNUSED_PARAM(v1), Vector_4V_In v2 )
{
	return vec_sld( v2, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeZW( v2, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,Z2>( Vector_4V_In UNUSED_PARAM(v1), Vector_4V_In v2 )
{
	return V4SplatZ( v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W2,W2>( Vector_4V_In UNUSED_PARAM(v1), Vector_4V_In v2 )
{
	return V4MergeZW( v2, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return vec_sld( v2, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X2,Y2>( Vector_4V_In UNUSED_PARAM(v1), Vector_4V_In v2 )
{
	return vec_sld( v2, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return vec_sld( v2, v1, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y2,Z2>( Vector_4V_In UNUSED_PARAM(v1), Vector_4V_In v2 )
{
	return vec_sld( v2, v2, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W2,W2>( Vector_4V_In UNUSED_PARAM(v1), Vector_4V_In v2 )
{
	return V4SplatW( v2 );
}

#if defined(__SNC__) && __ASSERT && !RAGE_MINIMAL_ASSERTS
#pragma diag_error 828
#endif
