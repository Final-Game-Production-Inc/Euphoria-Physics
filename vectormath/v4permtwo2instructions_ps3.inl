template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _SX_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( v1, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return vec_sld( _SX_v1_, v1, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( v1, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _MXY_v1_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _MXY_v1_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return vec_sld( _SX_v1_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v1_ = V4MergeXY( v1, v1 );
	return vec_sld( _MXY_v1_v1_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( v1, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return V4MergeXY( v1, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v1_ = V4MergeXY( v1, v1 );
	return vec_sld( _MXY_v1_v1_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
	return vec_sld( _DOI_v1_v1_12_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
	return vec_sld( _DOI_v1_v1_12_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = vec_sld( v1, v1, 8 );
	return vec_sld( _DOI_v1_v1_8_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _SX_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( v1, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = vec_sld( v1, v1, 8 );
	return V4MergeXY( v1, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( v1, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
	return V4MergeXY( v1, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( v1, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return V4MergeXY( v1, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _SX_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( v1, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( v1, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _MXY_v1_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return vec_sld( _SX_v1_, v2, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return vec_sld( _MXY_v2_v1_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return vec_sld( _MXY_v2_v1_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( v1, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return V4MergeXY( v1, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _SX_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( v1, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( v1, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = vec_sld( v2, v1, 8 );
	return V4MergeXY( v1, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = vec_sld( v2, v1, 12 );
	return V4MergeXY( v1, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = vec_sld( v2, v2, 12 );
	return V4MergeXY( v1, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( v1, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _SY_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( _SY_v1_, v1, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return V4MergeXY( _DOI_v1_v1_4_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( _SY_v1_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( _SY_v1_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( _SY_v1_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return V4MergeXY( _DOI_v1_v1_4_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( _SY_v1_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _MXY_v1_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
	return vec_sld( _DOI_v1_v1_12_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _SY_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( v1, _SY_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( v1, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return vec_sld( v1, _SW_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( v1, _SY_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( v1, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return vec_sld( v1, _SW_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
	return vec_sld( _DOI_v1_v1_12_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _MXY_v1_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _SY_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return V4MergeXY( _DOI_v1_v1_4_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( _SY_v1_, v2, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return vec_sld( _MXY_v1_v2_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return vec_sld( _MXY_v1_v2_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return vec_sld( _MXY_v1_v2_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _SY_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _MXY_v1_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( _SZ_v1_, v1, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _SZ_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( v1, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = vec_sld( v1, v1, 8 );
	return V4MergeZW( v1, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _MZW_v1_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( v1, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
	return V4MergeZW( v1, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( v1, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( _SZ_v1_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( _SZ_v1_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _SZ_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( _SZ_v1_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( v1, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( _SZ_v1_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _MZW_v1_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return vec_sld( v1, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return vec_sld( v1, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( v1, _SY_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return vec_sld( v1, _DOI_v1_v1_4_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( v1, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return vec_sld( v1, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return V4MergeZW( v1, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( v1, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = vec_sld( v1, v2, 4 );
	return V4MergeZW( v1, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( v1, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return vec_sld( v1, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return vec_sld( v1, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( v1, _SY_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return vec_sld( v1, _DOI_v2_v1_4_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return vec_sld( v1, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( v1, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = vec_sld( v2, v1, 8 );
	return vec_sld( v1, _DOI_v2_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = vec_sld( v2, v1, 12 );
	return vec_sld( v1, _DOI_v2_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = vec_sld( v2, v2, 12 );
	return vec_sld( v1, _DOI_v2_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return vec_sld( v1, _SW_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _SZ_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( v1, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = vec_sld( v1, v2, 8 );
	return V4MergeZW( v1, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( _SZ_v1_, v2, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _MZW_v1_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( v1, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( v1, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return V4MergeZW( v1, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _SZ_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( v1, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return V4MergeZW( v1, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( v1, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = vec_sld( v2, v2, 4 );
	return V4MergeZW( v1, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( v1, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = vec_sld( v1, v1, 8 );
	return vec_sld( _DOI_v1_v1_8_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = vec_sld( v1, v1, 8 );
	return vec_sld( _DOI_v1_v1_8_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _SW_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return vec_sld( _DOI_v1_v1_4_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( v1, _SY_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return vec_sld( v1, _DOI_v1_v1_4_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return V4MergeZW( _DOI_v1_v1_4_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( v1, _SZ_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v1_ = V4MergeZW( v1, v1 );
	return vec_sld( v1, _MZW_v1_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = vec_sld( v1, v1, 8 );
	return vec_sld( v1, _DOI_v1_v1_8_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _SW_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = vec_sld( v1, v2, 8 );
	return vec_sld( v1, _DOI_v1_v2_8_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = vec_sld( v1, v2, 4 );
	return V4MergeZW( _DOI_v1_v2_4_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return vec_sld( v1, _MZW_v1_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _MZW_v1_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return V4MergeZW( _DOI_v1_v1_4_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
	return vec_sld( v1, _DOI_v1_v1_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = vec_sld( v1, v2, 4 );
	return V4MergeZW( _DOI_v1_v2_4_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return vec_sld( v1, _DOI_v1_v2_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _MZW_v1_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = vec_sld( v1, v2, 4 );
	return vec_sld( _DOI_v1_v2_4_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _SW_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = vec_sld( v1, v2, 8 );
	return vec_sld( _DOI_v1_v2_8_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = vec_sld( v1, v2, 8 );
	return vec_sld( _DOI_v1_v2_8_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( v1, _SY_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return vec_sld( v1, _DOI_v2_v1_4_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return V4MergeZW( _DOI_v1_v1_4_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return vec_sld( v1, _MZW_v2_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _SW_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = vec_sld( v1, v2, 4 );
	return V4MergeZW( _DOI_v1_v2_4_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( v1, _SZ_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v2_ = V4MergeZW( v2, v2 );
	return vec_sld( v1, _MZW_v2_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = vec_sld( v2, v1, 8 );
	return vec_sld( v1, _DOI_v2_v1_8_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_8_ = vec_sld( v2, v2, 8 );
	return vec_sld( v1, _DOI_v2_v2_8_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _MZW_v1_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = vec_sld( v2, v1, 12 );
	return vec_sld( v1, _DOI_v2_v1_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return vec_sld( _MZW_v1_v2_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = vec_sld( v2, v2, 12 );
	return vec_sld( v1, _DOI_v2_v2_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _MXY_v2_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return vec_sld( _SX_v2_, v1, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _SX_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( v2, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( v2, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return vec_sld( _MXY_v1_v2_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( v2, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return V4MergeXY( v2, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return vec_sld( _MXY_v1_v2_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _SX_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( v2, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = vec_sld( v1, v1, 8 );
	return V4MergeXY( v2, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( v2, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
	return V4MergeXY( v2, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( v2, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return V4MergeXY( v2, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _MXY_v2_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return vec_sld( _SX_v2_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _MXY_v2_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _SX_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( v2, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( v2, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return vec_sld( _SX_v2_, v2, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = vec_sld( v1, v2, 8 );
	return vec_sld( _DOI_v1_v2_8_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v2_ = V4MergeXY( v2, v2 );
	return vec_sld( _MXY_v2_v2_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v2_ = V4MergeXY( v2, v2 );
	return vec_sld( _MXY_v2_v2_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( v2, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return V4MergeXY( v2, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return vec_sld( _DOI_v1_v2_12_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return vec_sld( _DOI_v1_v2_12_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _SX_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( v2, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( v2, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = vec_sld( v2, v1, 8 );
	return V4MergeXY( v2, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = vec_sld( v2, v1, 12 );
	return V4MergeXY( v2, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = vec_sld( v2, v2, 12 );
	return V4MergeXY( v2, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( v2, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( _SY_v2_, v1, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _SY_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return vec_sld( _MXY_v2_v1_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return vec_sld( _MXY_v2_v1_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return vec_sld( _MXY_v2_v1_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _MXY_v2_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _SY_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return V4MergeZW( _DOI_v1_v2_12_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _SY_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( _SY_v2_, v2, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( _SY_v2_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _MXY_v2_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( _SY_v2_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( _SY_v2_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( _SY_v2_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return V4MergeZW( _DOI_v1_v2_12_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return vec_sld( _DOI_v1_v2_12_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _MXY_v2_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return vec_sld( _DOI_v1_v2_12_, _DOI_v1_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return vec_sld( _DOI_v1_v2_12_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _SY_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return V4MergeZW( _DOI_v1_v2_12_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( v2, _SY_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( v2, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return vec_sld( v2, _SW_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( v2, _SY_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( v2, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return vec_sld( v2, _SW_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( _SZ_v2_, v1, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _MZW_v2_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _SZ_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( v2, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = vec_sld( v1, v1, 8 );
	return V4MergeZW( v2, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( v2, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
	return V4MergeZW( v2, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( v2, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _SZ_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( v2, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return V4MergeZW( v2, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( v2, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = vec_sld( v1, v2, 4 );
	return V4MergeZW( v2, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( v2, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _MZW_v2_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( _SZ_v2_, v2, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _SZ_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( v2, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = vec_sld( v1, v2, 8 );
	return V4MergeZW( v2, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( v2, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( v2, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return V4MergeZW( v2, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( _SZ_v2_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _MZW_v2_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( _SZ_v2_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( _SZ_v2_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( _SZ_v2_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _SZ_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( v2, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return vec_sld( v2, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return vec_sld( v2, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( v2, _SY_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return vec_sld( v2, _DOI_v1_v1_4_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( v2, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = vec_sld( v1, v1, 8 );
	return vec_sld( v2, _DOI_v1_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return vec_sld( v2, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
	return vec_sld( v2, _DOI_v1_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return vec_sld( v2, _SW_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return vec_sld( v2, _DOI_v1_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return vec_sld( v2, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return vec_sld( v2, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( v2, _SY_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return vec_sld( v2, _DOI_v2_v1_4_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return vec_sld( v2, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( v2, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return V4MergeZW( v2, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( v2, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = vec_sld( v2, v2, 4 );
	return V4MergeZW( v2, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( v2, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = vec_sld( v2, v1, 8 );
	return vec_sld( _DOI_v2_v1_8_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = vec_sld( v2, v1, 8 );
	return vec_sld( _DOI_v2_v1_8_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return vec_sld( _DOI_v2_v1_4_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return vec_sld( _DOI_v2_v1_4_, _DOI_v2_v1_4_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _SW_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return vec_sld( v2, _SY_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = vec_sld( v1, v1, 4 );
	return vec_sld( v2, _DOI_v1_v1_4_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return V4MergeZW( _DOI_v2_v1_4_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return vec_sld( v2, _SZ_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v1_ = V4MergeZW( v1, v1 );
	return vec_sld( v2, _MZW_v1_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = vec_sld( v1, v1, 8 );
	return vec_sld( v2, _DOI_v1_v1_8_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _MZW_v2_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = vec_sld( v1, v2, 8 );
	return vec_sld( v2, _DOI_v1_v2_8_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = vec_sld( v2, v2, 4 );
	return V4MergeZW( _DOI_v2_v2_4_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return vec_sld( v2, _MZW_v1_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _SW_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
	return vec_sld( v2, _DOI_v1_v1_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = vec_sld( v1, v2, 12 );
	return vec_sld( v2, _DOI_v1_v2_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return vec_sld( _MZW_v2_v1_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = vec_sld( v2, v2, 4 );
	return vec_sld( _DOI_v2_v2_4_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_8_ = vec_sld( v2, v2, 8 );
	return vec_sld( _DOI_v2_v2_8_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_8_ = vec_sld( v2, v2, 8 );
	return vec_sld( _DOI_v2_v2_8_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _SW_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return vec_sld( v2, _SY_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return vec_sld( v2, _DOI_v2_v1_4_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return V4MergeZW( _DOI_v2_v1_4_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return vec_sld( v2, _MZW_v2_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _MZW_v2_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = vec_sld( v2, v2, 4 );
	return V4MergeZW( _DOI_v2_v2_4_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return vec_sld( v2, _SZ_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v2_ = V4MergeZW( v2, v2 );
	return vec_sld( v2, _MZW_v2_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = vec_sld( v2, v1, 8 );
	return vec_sld( v2, _DOI_v2_v1_8_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_8_ = vec_sld( v2, v2, 8 );
	return vec_sld( v2, _DOI_v2_v2_8_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _SW_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = vec_sld( v2, v1, 4 );
	return V4MergeZW( _DOI_v2_v1_4_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = vec_sld( v2, v1, 12 );
	return vec_sld( v2, _DOI_v2_v1_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _MZW_v2_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = vec_sld( v2, v2, 4 );
	return V4MergeZW( _DOI_v2_v2_4_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = vec_sld( v2, v2, 12 );
	return vec_sld( v2, _DOI_v2_v2_12_, 12 );
}

