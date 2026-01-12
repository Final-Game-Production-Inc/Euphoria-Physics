template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _SX_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _SX_v1_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _SX_v1_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _SX_v1_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v1_32_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v1_32_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v1_32_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v1_2_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v1_48_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v1_48_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v1_48_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _MXY_v1_v2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v1_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _MXY_v1_v2_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _MXY_v1_v2_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _MXY_v1_v2_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v1_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v1_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v1_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _DOI_v1_v1_8_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _SX_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( _DOI_v1_v1_8_, _RLI_v1_v2_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _SX_v1_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v1_ = V4MergeXY( v1, v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _MXY_v1_v1_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _DOI_v1_v1_8_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vsldoi( _DOI_v1_v1_8_, _DOI_v1_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v1_3_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_7_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _MXY_v1_v2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _MXY_v1_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _DOI_v1_v1_8_, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _DOI_v1_v1_8_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return __vsldoi( _DOI_v1_v1_8_, _PRM_v2_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return __vsldoi( _DOI_v1_v1_8_, _PRM_v2_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return __vsldoi( _DOI_v1_v1_8_, _PRM_v2_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _DOI_v1_v1_8_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _DOI_v1_v1_8_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _DOI_v1_v1_8_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_2_2_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _SX_v1_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _SX_v1_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _SX_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _SX_v1_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _DOI_v1_v1_8_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _DOI_v1_v1_8_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v1_2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v1_2_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v1_2_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v1_2_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _MXY_v1_v2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _MXY_v1_v2_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _PRM_v1_2_, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _MXY_v1_v2_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _MXY_v1_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return __vsldoi( _PRM_v1_2_, _PRM_v2_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return __vsldoi( _PRM_v1_2_, _PRM_v2_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return __vsldoi( _PRM_v1_2_, _PRM_v2_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _PRM_v1_2_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vsldoi( _PRM_v1_2_, _DOI_v2_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_2_2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v2_v1_2_2_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _SX_v1_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _SX_v1_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v2_v1_12_3_ = __vrlimi( v2, v1, 12, 3 );
	return __vsldoi( _SX_v1_, _RLI_v2_v1_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _SX_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_13_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _DOI_v1_v1_8_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v1_2_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v1_32_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _PRM_v1_3_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v1_2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v1_3_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v1_48_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_15_ = __vpermwi( v1, 15 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_15_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v1_3_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _MXY_v1_v2_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v1_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _MXY_v1_v2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _MXY_v1_v2_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _MXY_v1_v2_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _PRM_v1_3_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return __vsldoi( _PRM_v1_3_, _PRM_v2_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v1_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( _SX_v1_, _RLI_v2_v1_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return __vsldoi( _PRM_v1_3_, _PRM_v2_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v1_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_3_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vsldoi( _PRM_v1_3_, _DOI_v2_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v1_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _PRM_v1_3_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _RLI_v2_v1_2_2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _SX_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _SX_v1_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _SX_v1_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _SX_v1_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _SX_v1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _SX_v1_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _SX_v1_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v1_32_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v1_32_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v1_32_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _PRM_v1_2_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _PRM_v1_2_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _PRM_v1_2_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v1_48_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v1_48_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v1_48_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _PRM_v1_3_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _PRM_v1_3_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _PRM_v1_3_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _PRM_v1_3_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _MXY_v1_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _MXY_v1_v2_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _MXY_v1_v2_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _MXY_v1_v2_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _MXY_v1_v2_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _SX_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _SX_v1_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _SX_v1_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _SX_v1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _SX_v1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _SX_v1_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return __vsldoi( _SX_v1_, _RLI_v1_v2_8_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _DOI_v1_v1_8_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v1_2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _PRM_v1_2_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _PRM_v1_2_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _PRM_v1_2_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v1_3_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _PRM_v1_3_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _PRM_v1_3_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _PRM_v1_3_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _PRM_v1_3_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( _MXY_v1_v2_, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _MXY_v2_v2_ = V4MergeXY( v2, v2 );
	return __vrlimi( _PRM_v1_2_, _MXY_v2_v2_, 6, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _MXY_v1_v2_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _MXY_v1_v2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v2_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v2_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_12_1_ = __vrlimi( v1, v2, 12, 1 );
	return __vsldoi( _SX_v1_, _RLI_v1_v2_12_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_2_2_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v2_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _SX_v1_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _SX_v1_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _SX_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _SX_v1_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _SX_v1_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _SX_v1_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _SX_v1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _DOI_v1_v1_8_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _DOI_v1_v1_8_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v1_2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v1_2_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v1_2_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _PRM_v1_2_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _PRM_v1_2_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v1_3_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _PRM_v1_3_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _PRM_v1_3_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _PRM_v1_3_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _MXY_v1_v2_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _MXY_v1_v2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _MXY_v1_v2_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _MXY_v1_v2_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_2_2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _SX_v1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _SX_v1_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _RLI_v2_v1_2_2_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _SX_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _SX_v1_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _SX_v1_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _SX_v1_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _SX_v1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( _SX_v1_, _RLI_v1_v2_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _DOI_v1_v1_8_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v1_2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v1_32_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _RLI_v2_v1_2_2_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _PRM_v1_2_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _PRM_v1_2_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _PRM_v1_2_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v1_3_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v1_48_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _RLI_v2_v1_2_2_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v1_3_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _PRM_v1_3_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _PRM_v1_3_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _PRM_v1_3_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _PRM_v1_3_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _MXY_v1_v2_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _MXY_v1_v2_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _RLI_v1_v2_12_3_ = __vrlimi( v1, v2, 12, 3 );
	return __vsldoi( _SX_v1_, _RLI_v1_v2_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _MXY_v1_v2_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _MXY_v1_v2_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _MXY_v1_v2_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _RLI_v2_v1_2_2_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _RLI_v2_v1_2_2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v1_64_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v1_64_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v1_64_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _SY_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _SY_v1_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _SY_v1_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _SY_v1_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _DOI_v1_v1_4_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v1_112_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v1_112_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v1_112_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v1_7_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _PRM_v1_4_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _SY_v1_, _MXY_v1_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_4_3_ = __vrlimi( v1, v2, 4, 3 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_4_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return __vsldoi( _PRM_v1_4_, _RLI_v2_v1_4_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _PRM_v1_4_, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_6_3_ = __vrlimi( v1, v2, 6, 3 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_6_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _PRM_v1_4_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return __vsldoi( _PRM_v1_4_, _PRM_v2_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _MXY_v1_v2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _MXY_v1_v2_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v2_v1_8_1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v2_v1_8_1_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_8_1_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _PRM_v1_4_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_4_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _PRM_v1_4_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return __vsldoi( _PRM_v1_4_, _PRM_v2_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_4_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_6_1_ = __vrlimi( v1, v2, 6, 1 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_6_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_6_2_ = __vrlimi( v1, v2, 6, 2 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_6_2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return __vsldoi( _PRM_v1_4_, _PRM_v2_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _PRM_v1_4_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _SY_v1_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v1_4_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _SY_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _SY_v1_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _SY_v1_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _SY_v1_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _DOI_v1_v1_12_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vsldoi( _SY_v1_, _DOI_v1_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v1_7_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_23_ = __vpermwi( v1, 23 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_23_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v1_7_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _SY_v1_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_8_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( _SY_v1_, _RLI_v2_v1_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return __vsldoi( _SY_v1_, _RLI_v2_v1_4_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _SY_v1_, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _SY_v1_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return __vsldoi( _SY_v1_, _PRM_v2_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _MXY_v1_v2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _MXY_v1_v2_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return __vsldoi( _SY_v1_, _PRM_v2_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return __vsldoi( _SY_v1_, _DOI_v2_v1_4_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _SY_v1_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _SY_v1_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return __vsldoi( _SY_v1_, _PRM_v2_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _SY_v1_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vsldoi( _SY_v1_, _DOI_v2_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return __vsldoi( _SY_v1_, _DOI_v2_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _SY_v1_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _SY_v1_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _SY_v1_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _SY_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _SY_v1_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _DOI_v1_v1_12_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _DOI_v1_v1_4_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _DOI_v1_v1_12_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v1_7_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v1_7_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _DOI_v1_v1_12_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return __vsldoi( _DOI_v1_v1_12_, _RLI_v1_v2_8_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( _DOI_v1_v1_12_, _RLI_v2_v1_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return __vsldoi( _DOI_v1_v1_12_, _RLI_v2_v1_4_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _DOI_v1_v1_12_, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _DOI_v1_v1_12_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return __vsldoi( _DOI_v1_v1_12_, _PRM_v2_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _MXY_v1_v2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _DOI_v1_v1_12_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _DOI_v1_v1_12_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _SY_v1_, _MZW_v1_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _DOI_v1_v1_12_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return __vsldoi( _DOI_v1_v1_12_, _PRM_v2_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _DOI_v1_v1_12_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v1_4_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v1_64_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v2_v1_12_3_ = __vrlimi( v2, v1, 12, 3 );
	return __vsldoi( _SY_v1_, _RLI_v2_v1_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v1_4_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _SY_v1_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _SY_v1_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_29_ = __vpermwi( v1, 29 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_29_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _SY_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _DOI_v1_v1_4_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _PRM_v1_7_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _DOI_v1_v1_12_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v1_7_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v1_112_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_31_ = __vpermwi( v1, 31 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_31_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v1_7_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _PRM_v1_7_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return __vsldoi( _PRM_v1_7_, _RLI_v1_v2_8_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( _PRM_v1_7_, _RLI_v2_v1_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return __vsldoi( _PRM_v1_7_, _RLI_v2_v1_4_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _PRM_v1_7_, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vsldoi( _SY_v1_, _DOI_v1_v2_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _PRM_v1_7_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return __vsldoi( _PRM_v1_7_, _PRM_v2_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _MXY_v1_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _MXY_v1_v2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _MXY_v1_v2_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_8_1_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( _SY_v1_, _RLI_v2_v1_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _MXY_v1_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _PRM_v1_7_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _PRM_v1_7_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _PRM_v1_7_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return __vsldoi( _PRM_v1_7_, _PRM_v2_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_7_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vsldoi( _PRM_v1_7_, _DOI_v2_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return __vsldoi( _PRM_v1_7_, _PRM_v2_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _PRM_v1_7_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v1_64_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v1_64_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v1_64_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _PRM_v1_4_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _PRM_v1_4_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _PRM_v1_4_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _PRM_v1_4_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _SY_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _SY_v1_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _SY_v1_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _SY_v1_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _SY_v1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _SY_v1_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _SY_v1_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _DOI_v1_v1_4_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _DOI_v1_v1_4_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( _DOI_v1_v1_4_, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _DOI_v1_v1_4_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v1_112_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v1_112_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v1_112_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v1_7_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _PRM_v1_7_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _PRM_v1_7_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _PRM_v1_7_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _PRM_v1_7_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _SY_v1_, _SX_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v2_v2_ = V4MergeXY( v2, v2 );
	return __vsldoi( _SY_v1_, _MXY_v2_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return __vsldoi( _SY_v1_, _PRM_v2_8_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_12_ = __vpermwi( v2, 12 );
	return __vsldoi( _SY_v1_, _PRM_v2_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v2_v1_8_1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v2_v1_8_1_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_8_1_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _MXY_v1_v2_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _MXY_v1_v2_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_66_ = __vpermwi( v1, 66 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return __vrlimi( _PRM_v1_66_, _PRM_v2_2_, 6, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _SY_v1_, _PRM_v2_32_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_36_ = __vpermwi( v2, 36 );
	return __vsldoi( _SY_v1_, _PRM_v2_36_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_40_ = __vpermwi( v2, 40 );
	return __vsldoi( _SY_v1_, _PRM_v2_40_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_44_ = __vpermwi( v2, 44 );
	return __vsldoi( _SY_v1_, _PRM_v2_44_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _MXY_v1_v2_, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v1_4_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _MXY_v1_v2_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _PRM_v1_4_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _PRM_v1_4_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _PRM_v1_4_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _PRM_v1_4_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return __vsldoi( _MXY_v1_v2_, _PRM_v1_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _SY_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_8_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _SY_v1_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _SY_v1_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _SY_v1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _SY_v1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _SY_v1_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _MXY_v1_v2_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _MXY_v1_v2_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _DOI_v1_v1_12_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _DOI_v1_v1_4_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _DOI_v1_v1_4_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _DOI_v1_v1_4_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return V4MergeXY( _DOI_v1_v1_4_, _PRM_v2_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v1_7_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _MXY_v1_v2_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v1_7_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _PRM_v1_7_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _PRM_v1_7_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _PRM_v1_7_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _MXY_v1_v2_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return __vsldoi( _MXY_v1_v2_, _RLI_v1_v2_8_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( _MXY_v1_v2_, _RLI_v2_v1_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return __vsldoi( _MXY_v1_v2_, _RLI_v2_v1_4_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return __vsldoi( _SY_v1_, _PRM_v2_64_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_68_ = __vpermwi( v2, 68 );
	return __vsldoi( _SY_v1_, _PRM_v2_68_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_72_ = __vpermwi( v2, 72 );
	return __vsldoi( _SY_v1_, _PRM_v2_72_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_76_ = __vpermwi( v2, 76 );
	return __vsldoi( _SY_v1_, _PRM_v2_76_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _MXY_v1_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _MXY_v1_v2_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _MXY_v1_v2_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _MXY_v1_v2_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_12_1_ = __vrlimi( v1, v2, 12, 1 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_12_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_3_ = __vrlimi( v1, v2, 3, 3 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return __vsldoi( _RLI_v1_v2_3_3_, _SW_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_96_ = __vpermwi( v2, 96 );
	return __vsldoi( _SY_v1_, _PRM_v2_96_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_100_ = __vpermwi( v2, 100 );
	return __vsldoi( _SY_v1_, _PRM_v2_100_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_104_ = __vpermwi( v2, 104 );
	return __vsldoi( _SY_v1_, _PRM_v2_104_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vsldoi( _MXY_v1_v2_, _DOI_v2_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_66_ = __vpermwi( v1, 66 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return __vrlimi( _PRM_v1_66_, _PRM_v2_7_, 6, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v1_4_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _PRM_v1_4_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _PRM_v1_4_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _PRM_v1_4_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _SY_v1_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _SY_v1_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _SY_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _SY_v1_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _SY_v1_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _SY_v1_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _SY_v1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _DOI_v1_v1_12_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _DOI_v1_v1_4_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _DOI_v1_v1_12_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _DOI_v1_v1_4_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _DOI_v1_v1_4_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _DOI_v1_v1_4_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _DOI_v1_v1_4_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v1_7_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v1_7_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v1_7_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v1_7_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _PRM_v1_7_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _PRM_v1_7_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _PRM_v1_7_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vrlimi( _SX_v2_, _DOI_v2_v1_8_, 13, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_66_ = __vpermwi( v1, 66 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return __vrlimi( _PRM_v1_66_, _PRM_v2_2_, 6, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _SY_v1_, _PRM_v2_128_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_132_ = __vpermwi( v2, 132 );
	return __vsldoi( _SY_v1_, _PRM_v2_132_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_136_ = __vpermwi( v2, 136 );
	return __vsldoi( _SY_v1_, _PRM_v2_136_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_140_ = __vpermwi( v2, 140 );
	return __vsldoi( _SY_v1_, _PRM_v2_140_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _MXY_v1_v2_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _MXY_v1_v2_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_160_ = __vpermwi( v2, 160 );
	return __vsldoi( _SY_v1_, _PRM_v2_160_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_164_ = __vpermwi( v2, 164 );
	return __vsldoi( _SY_v1_, _PRM_v2_164_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _SY_v1_, _SZ_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2_v2_ = V4MergeZW( v2, v2 );
	return __vsldoi( _SY_v1_, _MZW_v2_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v1_4_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v1_64_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _RLI_v2_v1_2_3_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v1_4_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _PRM_v1_4_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _PRM_v1_4_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _PRM_v1_4_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _PRM_v1_4_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _SY_v1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _SY_v1_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _SY_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _SY_v1_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _SY_v1_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _SY_v1_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _SY_v1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _DOI_v1_v1_4_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _DOI_v1_v1_4_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _RLI_v2_v1_2_3_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _DOI_v1_v1_12_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _DOI_v1_v1_4_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( _DOI_v1_v1_4_, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _DOI_v1_v1_4_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _DOI_v1_v1_4_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v1_7_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v1_112_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_1_ = __vrlimi( v1, v2, 2, 1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _RLI_v1_v2_2_1_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v1_7_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _PRM_v1_7_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _PRM_v1_7_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _PRM_v1_7_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _PRM_v1_7_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return __vsldoi( _RLI_v1_v2_3_1_, _SY_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_12_3_ = __vrlimi( v1, v2, 12, 3 );
	return __vsldoi( _SY_v1_, _RLI_v1_v2_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return __vsldoi( _RLI_v1_v2_3_1_, _SW_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_192_ = __vpermwi( v2, 192 );
	return __vsldoi( _SY_v1_, _PRM_v2_192_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return __vsldoi( _SY_v1_, _DOI_v2_v2_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_200_ = __vpermwi( v2, 200 );
	return __vsldoi( _SY_v1_, _PRM_v2_200_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_204_ = __vpermwi( v2, 204 );
	return __vsldoi( _SY_v1_, _PRM_v2_204_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _MXY_v1_v2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_8_1_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_66_ = __vpermwi( v1, 66 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return __vrlimi( _PRM_v1_66_, _PRM_v2_13_, 6, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _MXY_v1_v2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _MXY_v1_v2_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _MXY_v1_v2_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _MXY_v1_v2_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _RLI_v2_v1_2_3_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _RLI_v2_v1_2_3_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_67_ = __vpermwi( v1, 67 );
	Vector_4V _MZW_v2_v2_ = V4MergeZW( v2, v2 );
	return __vrlimi( _PRM_v1_67_, _MZW_v2_v2_, 6, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _SY_v1_, _PRM_v2_224_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_228_ = __vpermwi( v2, 228 );
	return __vsldoi( _SY_v1_, _PRM_v2_228_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_232_ = __vpermwi( v2, 232 );
	return __vsldoi( _SY_v1_, _PRM_v2_232_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _PRM_v2_236_ = __vpermwi( v2, 236 );
	return __vsldoi( _SY_v1_, _PRM_v2_236_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( _RLI_v2_v1_2_3_, _RLI_v1_v2_8_3_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v1_128_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v1_128_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v1_128_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v1_144_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v1_144_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v1_144_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _SZ_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _SZ_v1_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _SZ_v1_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _SZ_v1_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _DOI_v1_v1_8_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _DOI_v1_v1_8_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _DOI_v1_v1_8_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _PRM_v1_8_, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_6_3_ = __vrlimi( v1, v2, 6, 3 );
	return __vsldoi( _SZ_v1_, _RLI_v1_v2_6_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _PRM_v1_8_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v2_v1_8_2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v2_v1_8_2_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_8_2_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _MZW_v1_v2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v1_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _MZW_v1_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _MZW_v1_v2_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _MZW_v1_v2_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _MZW_v1_v2_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_6_2_ = __vrlimi( v1, v2, 6, 2 );
	return __vsldoi( _SZ_v1_, _RLI_v1_v2_6_2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return __vsldoi( _PRM_v1_8_, _PRM_v2_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _PRM_v1_8_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _PRM_v1_9_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v1_8_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( _PRM_v1_9_, _RLI_v1_v2_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v1_9_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_37_ = __vpermwi( v1, 37 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_37_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _SZ_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v2_v1_12_1_ = __vrlimi( v2, v1, 12, 1 );
	return __vsldoi( _SZ_v1_, _RLI_v2_v1_12_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _SZ_v1_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vsldoi( _PRM_v1_9_, _DOI_v1_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_39_ = __vpermwi( v1, 39 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_39_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _PRM_v1_9_, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _PRM_v1_9_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return __vsldoi( _PRM_v1_9_, _PRM_v2_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return __vsldoi( _SZ_v1_, _RLI_v2_v1_8_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _MZW_v1_v2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _MZW_v1_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v1_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _PRM_v1_9_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( _MZW_v1_v2_, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_9_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_1_3_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return __vsldoi( _PRM_v1_9_, _DOI_v2_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _PRM_v1_9_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _SZ_v1_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _SZ_v1_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _SZ_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _SZ_v1_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _MZW_v1_v2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _MZW_v1_v2_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_1_3_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _MZW_v1_v2_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v1_v2_1_3_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v1_8_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v1_128_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v1_9_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v1_144_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_45_ = __vpermwi( v1, 45 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_45_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v1_9_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _SZ_v1_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _SZ_v1_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _SZ_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _DOI_v1_v1_8_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v1_ = V4MergeZW( v1, v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _MZW_v1_v1_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_8_2_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _MZW_v1_v2_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _MZW_v1_v2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _MZW_v1_v2_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _MZW_v1_v2_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v1_128_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v1_128_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v1_128_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _PRM_v1_8_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _PRM_v1_8_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _PRM_v1_8_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _PRM_v1_8_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v1_144_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v1_144_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v1_144_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _PRM_v1_9_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _PRM_v1_9_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _PRM_v1_9_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _PRM_v1_9_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _SZ_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _SZ_v1_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _SZ_v1_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _SZ_v1_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _SZ_v1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _SZ_v1_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _SZ_v1_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _DOI_v1_v1_8_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _DOI_v1_v1_8_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _DOI_v1_v1_8_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _RLI_v1_v2_1_1_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return __vsldoi( _RLI_v1_v2_1_1_, _RLI_v1_v2_8_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( _RLI_v1_v2_1_1_, _RLI_v2_v1_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v2_v1_8_2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v2_v1_8_2_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_8_2_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _MZW_v1_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _MZW_v1_v2_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _MZW_v1_v2_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _MZW_v1_v2_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _MZW_v1_v2_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vsldoi( _RLI_v1_v2_1_1_, _DOI_v2_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( _RLI_v1_v2_1_1_, _RLI_v1_v2_8_3_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _RLI_v1_v2_1_2_, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _RLI_v1_v2_1_2_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _PRM_v1_8_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _PRM_v1_8_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _PRM_v1_8_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return __vsldoi( _RLI_v1_v2_1_2_, _PRM_v1_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v1_9_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return __vsldoi( _SZ_v1_, _RLI_v1_v2_8_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _PRM_v1_9_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _PRM_v1_9_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _PRM_v1_9_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _PRM_v1_9_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _RLI_v1_v2_1_2_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _SZ_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _RLI_v1_v2_1_2_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _SZ_v1_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _SZ_v1_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _SZ_v1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _SZ_v1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _SZ_v1_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return __vsldoi( _RLI_v1_v2_1_2_, _DOI_v1_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _RLI_v1_v2_1_2_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _RLI_v1_v2_1_2_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( _RLI_v1_v2_1_2_, _RLI_v2_v1_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_12_1_ = __vrlimi( v1, v2, 12, 1 );
	return __vsldoi( _SZ_v1_, _RLI_v1_v2_12_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_1_3_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _MZW_v1_v2_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _MZW_v1_v2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vsldoi( _RLI_v1_v2_1_2_, _DOI_v2_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v1_8_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _PRM_v1_8_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _PRM_v1_8_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _PRM_v1_8_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v1_9_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _PRM_v1_9_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _PRM_v1_9_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _PRM_v1_9_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _SZ_v1_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _SZ_v1_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _SZ_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _SZ_v1_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _SZ_v1_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _SZ_v1_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _SZ_v1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_1_3_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _MZW_v1_v2_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _MZW_v1_v2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v1_v2_1_3_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _MZW_v1_v2_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _MZW_v1_v2_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v1_8_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v1_128_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _RLI_v1_v2_1_0_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v1_8_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _PRM_v1_8_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _PRM_v1_8_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _PRM_v1_8_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _PRM_v1_8_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v1_9_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v1_144_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( _SZ_v1_, _RLI_v1_v2_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v1_9_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _PRM_v1_9_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _PRM_v1_9_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _PRM_v1_9_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _PRM_v1_9_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _SZ_v1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _SZ_v1_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _RLI_v1_v2_1_0_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _SZ_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _SZ_v1_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _SZ_v1_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _SZ_v1_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _SZ_v1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _DOI_v1_v1_8_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _MZW_v2_v1_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return __vsldoi( _RLI_v1_v2_1_0_, _RLI_v1_v2_8_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _RLI_v1_v2_12_3_ = __vrlimi( v1, v2, 12, 3 );
	return __vsldoi( _SZ_v1_, _RLI_v1_v2_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_8_2_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _MZW_v1_v2_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _MZW_v1_v2_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _RLI_v1_v2_1_0_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_1_3_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _MZW_v1_v2_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _MZW_v1_v2_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _MZW_v1_v2_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( _RLI_v1_v2_1_0_, _RLI_v1_v2_8_3_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _DOI_v1_v1_12_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _DOI_v1_v1_12_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _DOI_v1_v1_12_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v1_208_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v1_208_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v1_208_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v1_224_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v1_224_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v1_224_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _SW_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _SW_v1_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _SW_v1_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _SW_v1_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _DOI_v1_v2_4_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _DOI_v1_v2_4_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _DOI_v1_v2_12_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _DOI_v1_v2_12_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _DOI_v1_v2_12_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v1_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _DOI_v1_v1_4_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _DOI_v1_v1_4_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return __vsldoi( _DOI_v1_v1_4_, _PRM_v2_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _DOI_v1_v1_4_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _MZW_v1_v2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _MZW_v1_v2_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return __vsldoi( _DOI_v1_v1_4_, _PRM_v2_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _DOI_v1_v1_4_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _PRM_v1_13_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _DOI_v1_v1_4_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( _PRM_v1_13_, _RLI_v1_v2_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v1_13_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_53_ = __vpermwi( v1, 53 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_53_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v1_13_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v1_14_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vsldoi( _PRM_v1_13_, _DOI_v1_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _SW_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_55_ = __vpermwi( v1, 55 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_55_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _SW_v1_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _DOI_v1_v2_4_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _DOI_v1_v2_4_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _PRM_v1_13_, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _DOI_v1_v2_4_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _PRM_v1_13_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v1_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return __vsldoi( _PRM_v1_13_, _PRM_v2_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return __vsldoi( _PRM_v1_13_, _PRM_v2_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _PRM_v1_13_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _PRM_v1_13_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _PRM_v1_13_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return __vsldoi( _PRM_v1_13_, _PRM_v2_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_13_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vsldoi( _PRM_v1_13_, _DOI_v2_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _MZW_v1_v2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _MZW_v1_v2_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return __vsldoi( _PRM_v1_13_, _DOI_v2_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _MZW_v1_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _PRM_v1_13_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v1_13_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v1_13_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v1_13_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v1_13_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _SW_v1_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _SW_v1_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _SW_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _SW_v1_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _DOI_v1_v2_4_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return __vsldoi( _PRM_v1_14_, _PRM_v2_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return __vsldoi( _PRM_v1_14_, _PRM_v2_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _PRM_v1_14_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _PRM_v1_14_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _PRM_v1_14_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return __vsldoi( _PRM_v1_14_, _PRM_v2_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_14_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vsldoi( _PRM_v1_14_, _DOI_v2_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _MZW_v1_v2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _DOI_v1_v1_4_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _DOI_v1_v1_12_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _DOI_v1_v1_4_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v1_13_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v1_208_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_61_ = __vpermwi( v1, 61 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _PRM_v1_61_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v1_13_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v1_14_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v1_224_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _SW_v1_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v1_14_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _SW_v1_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _SW_v1_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _SW_v1_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _SW_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _DOI_v1_v2_4_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _DOI_v1_v2_4_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _DOI_v1_v2_12_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _SW_v1_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _DOI_v1_v2_4_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v1_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return __vsldoi( _SW_v1_, _PRM_v2_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _SW_v1_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _SW_v1_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _SW_v1_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return __vsldoi( _SW_v1_, _PRM_v2_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _SW_v1_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vsldoi( _SW_v1_, _DOI_v2_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _MZW_v1_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _MZW_v1_v2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _MZW_v1_v2_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return __vsldoi( _SW_v1_, _PRM_v2_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _SW_v1_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _DOI_v1_v1_12_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _DOI_v1_v1_12_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _DOI_v1_v1_12_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _DOI_v1_v1_4_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _DOI_v1_v1_4_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _DOI_v1_v1_4_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _DOI_v1_v1_4_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v1_208_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v1_208_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v1_208_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v1_13_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _PRM_v1_13_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _PRM_v1_13_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _PRM_v1_13_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _PRM_v1_13_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v1_224_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v1_224_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v1_224_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _PRM_v1_14_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _PRM_v1_14_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _PRM_v1_14_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _PRM_v1_14_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _SW_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _SW_v1_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _SW_v1_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _SW_v1_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _SW_v1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _SW_v1_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _SW_v1_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _DOI_v1_v2_12_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _DOI_v1_v2_12_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _DOI_v1_v2_12_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _DOI_v1_v2_4_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _DOI_v1_v2_4_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return __vrlimi( _DOI_v1_v2_12_, _RLI_v1_v2_1_3_, 3, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _DOI_v1_v2_4_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _DOI_v1_v2_4_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vsldoi( _DOI_v1_v2_4_, _DOI_v2_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( _DOI_v1_v2_4_, _RLI_v1_v2_8_3_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _MZW_v1_v2_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _MZW_v1_v2_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _DOI_v1_v1_4_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _DOI_v1_v1_4_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _DOI_v1_v1_4_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _DOI_v1_v1_4_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _DOI_v1_v1_4_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v1_13_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v1_13_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _PRM_v1_13_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _PRM_v1_13_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _PRM_v1_13_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v1_14_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _PRM_v1_14_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _PRM_v1_14_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _PRM_v1_14_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _PRM_v1_14_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _SW_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _SW_v1_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _SW_v1_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _SW_v1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _SW_v1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _SW_v1_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _DOI_v1_v2_4_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _DOI_v1_v2_4_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _DOI_v1_v2_4_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v2_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return __vrlimi( _DOI_v2_v1_4_, _RLI_v1_v2_8_1_, 12, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _MZW_v1_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _MZW_v1_v2_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _MZW_v1_v2_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _DOI_v1_v1_4_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _DOI_v1_v1_4_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _DOI_v1_v1_4_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _DOI_v1_v1_4_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v1_13_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v1_13_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v1_13_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v1_13_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _PRM_v1_13_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _PRM_v1_13_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _PRM_v1_13_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v1_14_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _PRM_v1_14_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _PRM_v1_14_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _PRM_v1_14_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _SW_v1_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _SW_v1_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _SW_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _SW_v1_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _SW_v1_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _SW_v1_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _SW_v1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _DOI_v1_v2_4_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _DOI_v1_v2_4_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _MZW_v1_v2_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _MZW_v1_v2_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _DOI_v1_v1_4_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _DOI_v1_v1_12_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _MZW_v1_v2_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _DOI_v1_v1_4_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _DOI_v1_v1_4_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _DOI_v1_v1_4_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _DOI_v1_v1_4_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _DOI_v1_v1_4_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v1_13_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v1_208_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v1_13_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _PRM_v1_13_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _PRM_v1_13_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _PRM_v1_13_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _PRM_v1_13_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v1_14_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v1_224_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _MZW_v1_v2_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v1_14_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _PRM_v1_14_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _PRM_v1_14_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _PRM_v1_14_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _PRM_v1_14_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _SW_v1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _SW_v1_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _MZW_v1_v2_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _SW_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _SW_v1_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _SW_v1_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _SW_v1_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _SW_v1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _DOI_v1_v2_4_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _DOI_v1_v2_12_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _DOI_v1_v2_4_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _DOI_v1_v2_4_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _DOI_v1_v2_4_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _DOI_v1_v2_4_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vrlimi( _SY_v2_, _RLI_v1_v2_8_3_, 13, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return __vsldoi( _MZW_v1_v2_, _RLI_v1_v2_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _MZW_v1_v2_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _MZW_v1_v2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( _MZW_v1_v2_, _RLI_v1_v2_8_3_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _MZW_v1_v2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _MZW_v1_v2_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _MZW_v1_v2_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _MZW_v1_v2_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _MXY_v2_v1_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v1_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _MXY_v2_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _MXY_v2_v1_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _MXY_v2_v1_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _MXY_v2_v1_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v1_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v2_v1_4_2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v2_v1_4_2_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_4_2_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _SX_v2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _SX_v2_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _SX_v2_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _SX_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _SX_v2_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _SX_v2_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _SX_v2_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _PRM_v2_2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _PRM_v2_2_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _PRM_v2_2_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v2_32_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v2_32_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v2_32_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _PRM_v2_3_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _PRM_v2_3_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _PRM_v2_3_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _PRM_v2_3_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v2_48_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v2_48_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v2_48_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _MXY_v2_v1_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _MXY_v2_v1_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v1_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( _MXY_v2_v1_, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_16_ = __vpermwi( v1, 16 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return __vrlimi( _PRM_v1_16_, _PRM_v2_2_, 9, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v1_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v2_v1_12_1_ = __vrlimi( v2, v1, 12, 1 );
	return __vsldoi( _SX_v2_, _RLI_v2_v1_12_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _SX_v2_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _SX_v2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _SX_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _SX_v2_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _SX_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _SX_v2_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return __vsldoi( _SX_v2_, _RLI_v2_v1_8_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _DOI_v1_v2_8_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _PRM_v2_2_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _PRM_v2_2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _PRM_v2_2_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v2_2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _PRM_v2_3_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _PRM_v2_3_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _PRM_v2_3_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _PRM_v2_3_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v2_3_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _MXY_v2_v1_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _MXY_v2_v1_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _MXY_v2_v1_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _MXY_v2_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _SX_v2_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _SX_v2_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _SX_v2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _SX_v2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _SX_v2_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _SX_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _SX_v2_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _DOI_v1_v2_8_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _DOI_v1_v2_8_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _PRM_v2_2_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _PRM_v2_2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v2_2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v2_2_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v2_2_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _PRM_v2_3_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _PRM_v2_3_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _PRM_v2_3_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _MXY_v2_v1_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _MXY_v2_v1_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _MXY_v2_v1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _MXY_v2_v1_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v2_v1_12_3_ = __vrlimi( v2, v1, 12, 3 );
	return __vsldoi( _SX_v2_, _RLI_v2_v1_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v1_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v1_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _RLI_v1_v2_2_2_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_4_2_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _SX_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _SX_v2_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _SX_v2_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _SX_v2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _SX_v2_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _SX_v2_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _RLI_v1_v2_2_2_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _SX_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( _SX_v2_, _RLI_v2_v1_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _DOI_v1_v2_8_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _PRM_v2_2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _PRM_v2_2_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _PRM_v2_2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v2_2_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v2_32_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _RLI_v1_v2_2_2_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _PRM_v2_3_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _PRM_v2_3_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _PRM_v2_3_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _PRM_v2_3_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v2_3_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v2_48_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _RLI_v1_v2_2_2_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v2_3_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _MXY_v2_v1_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _MXY_v2_v1_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _MXY_v2_v1_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _MXY_v2_v1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v2_v1_4_2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v2_v1_4_2_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_4_2_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _SX_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _SX_v2_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _SX_v2_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _SX_v2_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v2_32_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v2_32_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v2_32_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v2_2_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v2_48_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v2_48_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v2_48_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _DOI_v1_v2_8_, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _DOI_v1_v2_8_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return __vsldoi( _DOI_v1_v2_8_, _PRM_v1_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _MXY_v2_v1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _MXY_v2_v1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return __vsldoi( _DOI_v1_v2_8_, _PRM_v1_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return __vsldoi( _DOI_v1_v2_8_, _PRM_v1_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _DOI_v1_v2_8_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _DOI_v1_v2_8_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _DOI_v1_v2_8_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _DOI_v1_v2_8_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _SX_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( _DOI_v1_v2_8_, _RLI_v2_v1_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _SX_v2_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v2_ = V4MergeXY( v2, v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _MXY_v2_v2_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _DOI_v1_v2_8_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vsldoi( _DOI_v1_v2_8_, _DOI_v2_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v2_3_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_7_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _PRM_v2_2_, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _MXY_v2_v1_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _MXY_v2_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return __vsldoi( _PRM_v2_2_, _PRM_v1_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _MXY_v2_v1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _MXY_v2_v1_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return __vsldoi( _PRM_v2_2_, _PRM_v1_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return __vsldoi( _PRM_v2_2_, _PRM_v1_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _PRM_v2_2_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return __vsldoi( _PRM_v2_2_, _DOI_v1_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _SX_v2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _SX_v2_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _SX_v2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _SX_v2_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _DOI_v1_v2_8_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _DOI_v1_v2_8_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v2_2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v2_2_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v2_2_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v2_2_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v2_3_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _MXY_v2_v1_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _MXY_v2_v1_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _PRM_v2_3_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return __vsldoi( _PRM_v2_3_, _PRM_v1_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _MXY_v2_v1_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _MXY_v2_v1_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _MXY_v2_v1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( _SX_v2_, _RLI_v1_v2_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return __vsldoi( _PRM_v2_3_, _PRM_v1_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_3_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return __vsldoi( _PRM_v2_3_, _DOI_v1_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_4_2_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _PRM_v2_3_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _SX_v2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _SX_v2_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _RLI_v1_v2_12_3_ = __vrlimi( v1, v2, 12, 3 );
	return __vsldoi( _SX_v2_, _RLI_v1_v2_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _SX_v2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_13_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _DOI_v1_v2_8_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v2_2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v2_32_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _PRM_v2_3_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v2_2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v2_3_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v2_48_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_15_ = __vpermwi( v2, 15 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_15_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v2_3_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _SY_v2_, _SX_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v1_v1_ = V4MergeXY( v1, v1 );
	return __vsldoi( _SY_v2_, _MXY_v1_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return __vsldoi( _SY_v2_, _PRM_v1_8_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_12_ = __vpermwi( v1, 12 );
	return __vsldoi( _SY_v2_, _PRM_v1_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _MXY_v2_v1_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _MXY_v2_v1_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_8_1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v1_v2_8_1_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_8_1_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _SY_v2_, _PRM_v1_32_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_36_ = __vpermwi( v1, 36 );
	return __vsldoi( _SY_v2_, _PRM_v1_36_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_40_ = __vpermwi( v1, 40 );
	return __vsldoi( _SY_v2_, _PRM_v1_40_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_44_ = __vpermwi( v1, 44 );
	return __vsldoi( _SY_v2_, _PRM_v1_44_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return __vrlimi( _PRM_v1_8_, _PRM_v2_9_, 9, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _PRM_v2_4_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _PRM_v2_4_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _PRM_v2_4_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _PRM_v2_4_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v2_64_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v2_64_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v2_64_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _SY_v2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _SY_v2_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _SY_v2_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _SY_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _SY_v2_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _SY_v2_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _SY_v2_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _DOI_v1_v2_12_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _DOI_v1_v2_12_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _DOI_v1_v2_12_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _DOI_v1_v2_12_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _DOI_v2_v1_4_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _DOI_v2_v1_4_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _DOI_v2_v1_4_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _PRM_v2_7_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _PRM_v2_7_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _PRM_v2_7_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _PRM_v2_7_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v2_112_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v2_112_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v2_112_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v2_7_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return __vsldoi( _SY_v2_, _PRM_v1_64_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_68_ = __vpermwi( v1, 68 );
	return __vsldoi( _SY_v2_, _PRM_v1_68_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_72_ = __vpermwi( v1, 72 );
	return __vsldoi( _SY_v2_, _PRM_v1_72_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_76_ = __vpermwi( v1, 76 );
	return __vsldoi( _SY_v2_, _PRM_v1_76_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _MXY_v2_v1_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return __vsldoi( _MXY_v2_v1_, _RLI_v1_v2_4_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( _MXY_v2_v1_, _RLI_v1_v2_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return __vsldoi( _MXY_v2_v1_, _RLI_v1_v2_4_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _MXY_v2_v1_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _MXY_v2_v1_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _MXY_v2_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_96_ = __vpermwi( v1, 96 );
	return __vsldoi( _SY_v2_, _PRM_v1_96_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_100_ = __vpermwi( v1, 100 );
	return __vsldoi( _SY_v2_, _PRM_v1_100_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_104_ = __vpermwi( v1, 104 );
	return __vsldoi( _SY_v2_, _PRM_v1_104_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _MXY_v2_v1_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_12_1_ = __vrlimi( v2, v1, 12, 1 );
	return __vsldoi( _SY_v2_, _RLI_v2_v1_12_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_3_3_ = __vrlimi( v2, v1, 3, 3 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return __vsldoi( _RLI_v2_v1_3_3_, _SW_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vsldoi( _MXY_v2_v1_, _DOI_v1_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_28_ = __vpermwi( v1, 28 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return __vrlimi( _PRM_v1_28_, _PRM_v2_9_, 9, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _PRM_v2_4_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _PRM_v2_4_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _PRM_v2_4_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _PRM_v2_4_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _MXY_v2_v1_, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v2_4_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _MXY_v2_v1_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _SY_v2_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _SY_v2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _SY_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _SY_v2_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return __vsldoi( _MXY_v2_v1_, _PRM_v2_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _SY_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return __vsldoi( _SY_v2_, _RLI_v2_v1_8_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _SY_v2_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _DOI_v1_v2_12_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _DOI_v1_v2_12_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _DOI_v1_v2_12_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _DOI_v1_v2_12_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _MXY_v2_v1_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _DOI_v1_v2_12_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _MXY_v2_v1_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _PRM_v2_7_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _PRM_v2_7_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _PRM_v2_7_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v2_7_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _MXY_v2_v1_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v2_7_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _SY_v2_, _PRM_v1_128_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_132_ = __vpermwi( v1, 132 );
	return __vsldoi( _SY_v2_, _PRM_v1_132_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_136_ = __vpermwi( v1, 136 );
	return __vsldoi( _SY_v2_, _PRM_v1_136_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_140_ = __vpermwi( v1, 140 );
	return __vsldoi( _SY_v2_, _PRM_v1_140_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vrlimi( _SX_v1_, _DOI_v1_v2_8_, 13, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return __vrlimi( _PRM_v1_32_, _PRM_v2_9_, 9, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _MXY_v2_v1_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_160_ = __vpermwi( v1, 160 );
	return __vsldoi( _SY_v2_, _PRM_v1_160_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_164_ = __vpermwi( v1, 164 );
	return __vsldoi( _SY_v2_, _PRM_v1_164_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _SY_v2_, _SZ_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1_v1_ = V4MergeZW( v1, v1 );
	return __vsldoi( _SY_v2_, _MZW_v1_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _PRM_v2_4_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _PRM_v2_4_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _PRM_v2_4_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _SY_v2_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _SY_v2_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _SY_v2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _SY_v2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _SY_v2_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _SY_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _SY_v2_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _DOI_v1_v2_12_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _DOI_v1_v2_12_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _DOI_v1_v2_12_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _PRM_v2_7_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _PRM_v2_7_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _PRM_v2_7_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v2_7_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v2_7_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v2_7_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v2_7_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_192_ = __vpermwi( v1, 192 );
	return __vsldoi( _SY_v2_, _PRM_v1_192_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return __vsldoi( _SY_v2_, _DOI_v1_v1_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_200_ = __vpermwi( v1, 200 );
	return __vsldoi( _SY_v2_, _PRM_v1_200_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_204_ = __vpermwi( v1, 204 );
	return __vsldoi( _SY_v2_, _PRM_v1_204_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _RLI_v1_v2_2_3_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return __vsldoi( _RLI_v1_v2_2_3_, _RLI_v1_v2_4_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_12_3_ = __vrlimi( v2, v1, 12, 3 );
	return __vsldoi( _SY_v2_, _RLI_v2_v1_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return __vsldoi( _RLI_v1_v2_2_3_, _RLI_v1_v2_4_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _MXY_v2_v1_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _MXY_v2_v1_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _MXY_v2_v1_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_8_1_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_52_ = __vpermwi( v1, 52 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return __vrlimi( _PRM_v1_52_, _PRM_v2_9_, 9, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _MXY_v2_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _SY_v2_, _PRM_v1_224_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_228_ = __vpermwi( v1, 228 );
	return __vsldoi( _SY_v2_, _PRM_v1_228_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_232_ = __vpermwi( v1, 232 );
	return __vsldoi( _SY_v2_, _PRM_v1_232_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_236_ = __vpermwi( v1, 236 );
	return __vsldoi( _SY_v2_, _PRM_v1_236_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _RLI_v1_v2_2_3_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _RLI_v1_v2_2_3_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_56_ = __vpermwi( v1, 56 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return __vrlimi( _PRM_v1_56_, _PRM_v2_13_, 9, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( _RLI_v1_v2_2_3_, _RLI_v2_v1_8_3_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _PRM_v2_4_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _PRM_v2_4_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _PRM_v2_4_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _PRM_v2_4_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v2_4_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v2_64_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _RLI_v1_v2_2_3_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v2_4_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _SY_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _SY_v2_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _SY_v2_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _SY_v2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _SY_v2_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _SY_v2_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( _SY_v2_, _RLI_v2_v1_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _SY_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _DOI_v1_v2_12_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _DOI_v1_v2_12_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _DOI_v1_v2_12_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _DOI_v1_v2_12_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _DOI_v1_v2_12_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _DOI_v2_v1_4_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _RLI_v1_v2_2_3_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _DOI_v1_v2_12_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _PRM_v2_7_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _PRM_v2_7_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _PRM_v2_7_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _PRM_v2_7_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v2_7_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v2_112_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _RLI_v1_v2_2_3_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v2_7_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _PRM_v2_4_, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_6_3_ = __vrlimi( v2, v1, 6, 3 );
	return __vsldoi( _SY_v2_, _RLI_v2_v1_6_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _PRM_v2_4_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return __vsldoi( _PRM_v2_4_, _PRM_v1_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _PRM_v2_4_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _SY_v2_, _MXY_v2_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_4_3_ = __vrlimi( v2, v1, 4, 3 );
	return __vsldoi( _SY_v2_, _RLI_v2_v1_4_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return __vsldoi( _PRM_v2_4_, _RLI_v1_v2_4_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_8_1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v1_v2_8_1_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_8_1_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _MXY_v2_v1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _MXY_v2_v1_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _PRM_v2_4_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return __vsldoi( _PRM_v2_4_, _PRM_v1_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_4_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_6_1_ = __vrlimi( v2, v1, 6, 1 );
	return __vsldoi( _SY_v2_, _RLI_v2_v1_6_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _PRM_v2_4_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( _SY_v2_, _RLI_v2_v1_4_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_6_2_ = __vrlimi( v2, v1, 6, 2 );
	return __vsldoi( _SY_v2_, _RLI_v2_v1_6_2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return __vsldoi( _PRM_v2_4_, _PRM_v1_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _PRM_v2_4_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v2_64_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v2_64_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v2_64_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _SY_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _SY_v2_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _SY_v2_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _SY_v2_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _DOI_v2_v1_4_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _DOI_v2_v1_4_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _DOI_v2_v1_4_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v2_112_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v2_112_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v2_112_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v2_7_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _SY_v2_, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _SY_v2_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return __vsldoi( _SY_v2_, _PRM_v1_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _SY_v2_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return __vsldoi( _SY_v2_, _RLI_v1_v2_4_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( _SY_v2_, _RLI_v1_v2_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return __vsldoi( _SY_v2_, _RLI_v1_v2_4_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return __vsldoi( _SY_v2_, _PRM_v1_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return __vsldoi( _SY_v2_, _DOI_v1_v1_4_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _MXY_v2_v1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _MXY_v2_v1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _SY_v2_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return __vsldoi( _SY_v2_, _PRM_v1_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _SY_v2_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return __vsldoi( _SY_v2_, _DOI_v1_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _SY_v2_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _SY_v2_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return __vsldoi( _SY_v2_, _DOI_v1_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _SY_v2_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _SY_v2_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v2_4_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( _SY_v2_, _RLI_v2_v1_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _SY_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _SY_v2_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _SY_v2_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _DOI_v1_v2_12_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _SY_v2_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vsldoi( _SY_v2_, _DOI_v2_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v2_7_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_23_ = __vpermwi( v2, 23 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_23_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v2_7_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _DOI_v1_v2_12_, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _DOI_v1_v2_12_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return __vsldoi( _DOI_v1_v2_12_, _PRM_v1_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _DOI_v1_v2_12_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return __vsldoi( _DOI_v1_v2_12_, _RLI_v1_v2_4_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( _DOI_v1_v2_12_, _RLI_v1_v2_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return __vsldoi( _DOI_v1_v2_12_, _RLI_v1_v2_4_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _MXY_v2_v1_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _MXY_v2_v1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _DOI_v1_v2_12_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return __vsldoi( _DOI_v1_v2_12_, _PRM_v1_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _DOI_v1_v2_12_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _DOI_v1_v2_12_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _DOI_v1_v2_12_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _SY_v2_, _MZW_v2_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v2_4_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _SY_v2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _SY_v2_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _SY_v2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _SY_v2_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _DOI_v1_v2_12_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v2_7_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v2_7_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _PRM_v2_7_, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vsldoi( _SY_v2_, _DOI_v2_v1_12_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _PRM_v2_7_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return __vsldoi( _PRM_v2_7_, _PRM_v1_48_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _PRM_v2_7_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return __vsldoi( _PRM_v2_7_, _RLI_v1_v2_4_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( _PRM_v2_7_, _RLI_v1_v2_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return __vsldoi( _PRM_v2_7_, _RLI_v1_v2_4_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _MXY_v2_v1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_8_1_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( _SY_v2_, _RLI_v1_v2_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _MXY_v2_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _MXY_v2_v1_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _MXY_v2_v1_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _MXY_v2_v1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _PRM_v2_7_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return __vsldoi( _PRM_v2_7_, _PRM_v1_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_7_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return __vsldoi( _PRM_v2_7_, _DOI_v1_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _PRM_v2_7_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _PRM_v2_7_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return __vsldoi( _PRM_v2_7_, _PRM_v1_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _PRM_v2_7_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v2_4_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v2_64_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_12_3_ = __vrlimi( v1, v2, 12, 3 );
	return __vsldoi( _SY_v2_, _RLI_v1_v2_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v2_4_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _SY_v2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _SY_v2_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_29_ = __vpermwi( v2, 29 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_29_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _SY_v2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _DOI_v1_v2_12_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _DOI_v2_v1_4_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _PRM_v2_7_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _DOI_v1_v2_12_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v2_7_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v2_112_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_31_ = __vpermwi( v2, 31 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_31_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v2_7_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _RLI_v2_v1_1_1_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return __vsldoi( _RLI_v2_v1_1_1_, _RLI_v1_v2_4_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( _RLI_v2_v1_1_1_, _RLI_v1_v2_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v1_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _MZW_v2_v1_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v1_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _MZW_v2_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _MZW_v2_v1_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _MZW_v2_v1_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _MZW_v2_v1_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vsldoi( _RLI_v2_v1_1_1_, _DOI_v1_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( _RLI_v2_v1_1_1_, _RLI_v2_v1_8_3_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _PRM_v2_8_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _PRM_v2_8_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _PRM_v2_8_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _PRM_v2_8_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v2_128_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v2_128_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v2_128_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _PRM_v2_9_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _PRM_v2_9_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _PRM_v2_9_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _PRM_v2_9_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v2_144_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v2_144_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v2_144_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _SZ_v2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _SZ_v2_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _SZ_v2_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _SZ_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _SZ_v2_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _SZ_v2_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _SZ_v2_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _DOI_v2_v1_8_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _DOI_v2_v1_8_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _DOI_v2_v1_8_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _RLI_v2_v1_1_2_, _MXY_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( _RLI_v2_v1_1_2_, _RLI_v1_v2_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v1_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_1_2_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _MZW_v2_v1_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _MZW_v2_v1_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v2_v1_12_1_ = __vrlimi( v2, v1, 12, 1 );
	return __vsldoi( _SZ_v2_, _RLI_v2_v1_12_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_1_3_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vsldoi( _RLI_v2_v1_1_2_, _DOI_v1_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _PRM_v2_8_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _PRM_v2_8_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _PRM_v2_8_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return __vsldoi( _RLI_v2_v1_1_2_, _SX_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _RLI_v2_v1_1_2_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _PRM_v2_9_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _PRM_v2_9_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _PRM_v2_9_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _PRM_v2_9_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return __vsldoi( _RLI_v2_v1_1_2_, _PRM_v2_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v2_9_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return __vsldoi( _SZ_v2_, _RLI_v2_v1_8_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _SZ_v2_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _SZ_v2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _SZ_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _SZ_v2_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return __vsldoi( _RLI_v2_v1_1_2_, _PRM_v2_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _SZ_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _RLI_v2_v1_1_2_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _SZ_v2_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return __vsldoi( _RLI_v2_v1_1_2_, _DOI_v2_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _RLI_v2_v1_1_2_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_1_2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v2_v1_1_2_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _MZW_v2_v1_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _MZW_v2_v1_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_1_3_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _MZW_v2_v1_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _MZW_v2_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v2_v1_1_3_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _PRM_v2_8_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _PRM_v2_8_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _PRM_v2_8_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _PRM_v2_9_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _PRM_v2_9_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _PRM_v2_9_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _SZ_v2_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _SZ_v2_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _SZ_v2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _SZ_v2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _SZ_v2_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _SZ_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _SZ_v2_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return __vsldoi( _RLI_v1_v2_2_0_, _RLI_v1_v2_4_0_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v2_v1_12_3_ = __vrlimi( v2, v1, 12, 3 );
	return __vsldoi( _SZ_v2_, _RLI_v2_v1_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v1_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _RLI_v2_v1_1_2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _MZW_v2_v1_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v1_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _MZW_v2_v1_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _MZW_v2_v1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _MZW_v2_v1_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _RLI_v1_v2_2_0_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _RLI_v2_v1_1_3_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( _RLI_v1_v2_2_0_, _RLI_v2_v1_8_3_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _PRM_v2_8_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _PRM_v2_8_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _PRM_v2_8_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _PRM_v2_8_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v2_8_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v2_128_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _RLI_v1_v2_2_0_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v2_8_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _PRM_v2_9_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _PRM_v2_9_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _PRM_v2_9_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _PRM_v2_9_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v2_9_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v2_144_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( _SZ_v2_, _RLI_v2_v1_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v2_9_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _SZ_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _SZ_v2_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _SZ_v2_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _SZ_v2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _SZ_v2_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _SZ_v2_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _RLI_v1_v2_2_0_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _SZ_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _DOI_v2_v1_8_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _MZW_v1_v2_, _SZ_v2_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _PRM_v2_8_, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v2_v1_6_3_ = __vrlimi( v2, v1, 6, 3 );
	return __vsldoi( _SZ_v2_, _RLI_v2_v1_6_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _PRM_v2_8_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _MZW_v2_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _MZW_v2_v1_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _MZW_v2_v1_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _MZW_v2_v1_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _MZW_v2_v1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v2_v1_6_2_ = __vrlimi( v2, v1, 6, 2 );
	return __vsldoi( _SZ_v2_, _RLI_v2_v1_6_2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return __vsldoi( _PRM_v2_8_, _PRM_v1_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _PRM_v2_8_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v2_128_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v2_128_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v2_128_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v2_144_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v2_144_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v2_144_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _SZ_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _SZ_v2_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _SZ_v2_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _SZ_v2_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _DOI_v2_v1_8_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _DOI_v2_v1_8_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _DOI_v2_v1_8_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _PRM_v2_9_, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _PRM_v2_9_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return __vsldoi( _PRM_v2_9_, _PRM_v1_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return __vsldoi( _SZ_v2_, _RLI_v1_v2_8_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_1_2_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v2_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _PRM_v2_9_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( _MZW_v2_v1_, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_9_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_1_3_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _MZW_v2_v1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _MZW_v2_v1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v2_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return __vsldoi( _PRM_v2_9_, _DOI_v1_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _PRM_v2_9_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _PRM_v2_9_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v2_8_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( _PRM_v2_9_, _RLI_v2_v1_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v2_9_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_37_ = __vpermwi( v2, 37 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_37_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _SZ_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_12_1_ = __vrlimi( v1, v2, 12, 1 );
	return __vsldoi( _SZ_v2_, _RLI_v1_v2_12_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _SZ_v2_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vsldoi( _PRM_v2_9_, _DOI_v2_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_39_ = __vpermwi( v2, 39 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_39_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_1_2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v2_v1_1_2_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_1_3_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _MZW_v2_v1_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v2_v1_1_3_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _MZW_v2_v1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _MZW_v2_v1_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v2_8_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v2_9_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _SZ_v2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _SZ_v2_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _SZ_v2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _SZ_v2_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _RLI_v2_v1_1_2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _MZW_v2_v1_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _MZW_v2_v1_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _MZW_v2_v1_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _MZW_v2_v1_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _MZW_v2_v1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v2_8_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v2_128_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v2_9_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v2_144_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_45_ = __vpermwi( v2, 45 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_45_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v2_9_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _SZ_v2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _SZ_v2_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _SZ_v2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _DOI_v2_v1_8_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v2_ = V4MergeZW( v2, v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _MZW_v2_v2_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _DOI_v2_v1_4_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _DOI_v2_v1_4_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _DOI_v2_v1_12_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _DOI_v2_v1_12_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _DOI_v2_v1_12_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return __vrlimi( _DOI_v2_v1_12_, _RLI_v1_v2_1_1_, 3, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _DOI_v2_v1_4_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _DOI_v2_v1_4_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _MZW_v2_v1_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _MZW_v2_v1_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vsldoi( _DOI_v2_v1_4_, _DOI_v1_v2_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( _DOI_v2_v1_4_, _RLI_v2_v1_8_3_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _DOI_v2_v2_4_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _DOI_v2_v2_4_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _DOI_v2_v2_4_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _DOI_v2_v2_4_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _DOI_v2_v2_12_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _DOI_v2_v2_12_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _DOI_v2_v2_12_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _PRM_v2_13_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _PRM_v2_13_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _PRM_v2_13_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _PRM_v2_13_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v2_208_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v2_208_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v2_208_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v2_13_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _PRM_v2_14_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( _PRM_v2_14_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _PRM_v2_14_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _PRM_v2_14_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _PRM_v2_224_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _PRM_v2_224_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _PRM_v2_224_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeXY( _SW_v2_, _SX_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _SW_v2_, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _SW_v2_, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _SW_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _SW_v2_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _SW_v2_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _SW_v2_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _DOI_v2_v1_4_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _DOI_v2_v1_4_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _DOI_v2_v1_4_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v1_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v1_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _MZW_v2_v1_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _MZW_v2_v1_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vrlimi( _DOI_v1_v2_4_, _RLI_v1_v2_8_3_, 12, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _MZW_v2_v1_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _DOI_v2_v2_4_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _DOI_v2_v2_4_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _DOI_v2_v2_4_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _DOI_v2_v2_4_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _DOI_v2_v2_4_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _PRM_v2_13_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _PRM_v2_13_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _PRM_v2_13_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v2_13_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v2_13_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _PRM_v2_14_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _PRM_v2_14_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( _PRM_v2_14_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _PRM_v2_14_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _PRM_v2_14_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _SW_v2_, _PRM_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeXY( _SW_v2_, _SY_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _SW_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _SW_v2_, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _SW_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _SW_v2_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _DOI_v2_v1_4_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v1_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _MZW_v2_v1_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _DOI_v2_v2_4_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _DOI_v2_v2_4_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _DOI_v2_v2_4_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _PRM_v2_13_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _PRM_v2_13_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _PRM_v2_13_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v2_13_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v2_13_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v2_13_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v2_13_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _PRM_v2_14_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _PRM_v2_14_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _PRM_v2_14_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _SW_v2_, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _SW_v2_, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _SW_v2_, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _SW_v2_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _SW_v2_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _SW_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _SW_v2_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _DOI_v2_v1_4_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _DOI_v2_v1_4_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _DOI_v2_v1_4_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _DOI_v2_v1_12_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _DOI_v2_v1_4_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v1_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v1_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _RLI_v1_v2_6_1_ = __vrlimi( v1, v2, 6, 1 );
	return __vrlimi( _SY_v1_, _RLI_v1_v2_6_1_, 13, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _MZW_v2_v1_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _MZW_v2_v1_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _MZW_v2_v1_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _MZW_v2_v1_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _MZW_v2_v1_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( _MZW_v2_v1_, _RLI_v2_v1_8_3_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _MZW_v2_v1_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _DOI_v2_v2_4_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _DOI_v2_v2_4_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _DOI_v2_v2_4_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _DOI_v2_v2_4_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _DOI_v2_v2_4_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _DOI_v2_v2_12_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( _MZW_v2_v1_, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _DOI_v2_v2_4_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _PRM_v2_13_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _PRM_v2_13_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _PRM_v2_13_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _PRM_v2_13_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v2_13_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v2_208_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v2_13_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _PRM_v2_14_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _PRM_v2_14_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _PRM_v2_14_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeZW( _PRM_v2_14_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _PRM_v2_14_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _PRM_v2_224_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( _MZW_v2_v1_, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _PRM_v2_14_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _SW_v2_, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _SW_v2_, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _SW_v2_, _PRM_v1_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _SW_v2_, _SW_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _SW_v2_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _SW_v2_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( _MZW_v2_v1_, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _SW_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _DOI_v2_v1_12_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _DOI_v2_v1_12_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _DOI_v2_v1_12_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _DOI_v2_v1_4_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _DOI_v2_v1_4_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _DOI_v2_v2_4_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return __vsldoi( _DOI_v2_v2_4_, _PRM_v1_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _DOI_v2_v2_4_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _DOI_v2_v2_4_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return __vsldoi( _DOI_v2_v2_4_, _PRM_v1_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _DOI_v2_v2_4_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _MZW_v2_v1_, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( _MZW_v2_v1_, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _DOI_v2_v2_12_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _DOI_v2_v2_12_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _DOI_v2_v2_12_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v2_208_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v2_208_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v2_208_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _PRM_v2_224_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _PRM_v2_224_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _PRM_v2_224_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _SW_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _SW_v2_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _SW_v2_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _SW_v2_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return __vsldoi( _PRM_v2_13_, _SX_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _DOI_v2_v1_4_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _PRM_v2_13_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _DOI_v2_v1_4_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _DOI_v2_v1_4_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return __vsldoi( _PRM_v2_13_, _PRM_v1_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return __vsldoi( _PRM_v2_13_, _PRM_v1_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v2_64_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v2_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _PRM_v2_13_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return __vsldoi( _PRM_v2_13_, _PRM_v1_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_13_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return __vsldoi( _PRM_v2_13_, _DOI_v1_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _PRM_v2_13_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _PRM_v2_13_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return __vsldoi( _PRM_v2_13_, _DOI_v1_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _MZW_v2_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _PRM_v2_13_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeZW( _MZW_v2_v1_, _SY_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _MZW_v2_v1_, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _PRM_v2_13_, _MXY_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _DOI_v2_v2_4_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( _PRM_v2_13_, _RLI_v2_v1_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v2_13_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_53_ = __vpermwi( v2, 53 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_53_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v2_13_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _PRM_v2_14_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vsldoi( _PRM_v2_13_, _DOI_v2_v1_12_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _SW_v2_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_55_ = __vpermwi( v2, 55 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_55_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _SW_v2_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _DOI_v2_v1_4_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _DOI_v2_v1_4_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return __vsldoi( _PRM_v2_14_, _PRM_v1_64_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return __vsldoi( _PRM_v2_14_, _PRM_v1_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v2_144_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _PRM_v2_14_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return __vsldoi( _PRM_v2_14_, _PRM_v1_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_14_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return __vsldoi( _PRM_v2_14_, _DOI_v1_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _PRM_v2_14_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _PRM_v2_14_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _MZW_v2_v1_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _MZW_v2_v1_, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _DOI_v2_v2_4_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v2_13_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v2_13_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v2_13_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v2_13_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _PRM_v2_14_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _SW_v2_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _SW_v2_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _SW_v2_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _SW_v2_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _DOI_v2_v1_12_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( _SW_v2_, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _DOI_v2_v1_4_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _DOI_v2_v1_4_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _DOI_v2_v1_4_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _DOI_v2_v1_4_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return __vsldoi( _SW_v2_, _PRM_v1_112_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return __vsldoi( _SW_v2_, _PRM_v1_128_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return __vsldoi( _SW_v2_, _PRM_v1_144_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _SW_v2_, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return __vsldoi( _SW_v2_, _DOI_v1_v1_8_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return __vsldoi( _SW_v2_, _RLI_v2_v1_8_2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( _SW_v2_, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _MZW_v2_v1_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return __vsldoi( _SW_v2_, _PRM_v1_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( _SW_v2_, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _MZW_v2_v1_, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _MZW_v2_v1_, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _MZW_v2_v1_, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _DOI_v2_v2_4_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _DOI_v2_v2_12_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _DOI_v2_v2_4_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v2_13_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v2_208_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_61_ = __vpermwi( v2, 61 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _PRM_v2_61_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v2_13_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _PRM_v2_14_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _PRM_v2_224_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( _SW_v2_, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _PRM_v2_14_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _SW_v2_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _SW_v2_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( _SW_v2_, _SZ_v1_, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _SW_v2_, _MZW_v2_v1_ );
}

