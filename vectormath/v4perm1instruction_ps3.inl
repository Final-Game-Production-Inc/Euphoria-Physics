template <>
__forceinline Vector_4V_Out V4Permute<X,X,X,X>( Vector_4V_In v )
{
	return V4SplatX( v );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,X,Y,Y>( Vector_4V_In v )
{
	return V4MergeXY( v, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,Y,Y>( Vector_4V_In v )
{
	return V4SplatY( v );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,W,X>( Vector_4V_In v )
{
	return vec_sld( v, v, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,Z,Z>( Vector_4V_In v )
{
	return V4SplatZ( v );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,W,W>( Vector_4V_In v )
{
	return V4MergeZW( v, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,X,Y>( Vector_4V_In v )
{
	return vec_sld( v, v, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,Y,Z>( Vector_4V_In v )
{
	return vec_sld( v, v, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,W,W>( Vector_4V_In v )
{
	return V4SplatW( v );
}

