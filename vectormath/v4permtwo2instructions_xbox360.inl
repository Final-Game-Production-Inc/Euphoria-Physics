template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( v1, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( v1, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( v1, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( v1, _RLI_v1_v2_4_2_ );
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
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v1_v2_4_0_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v1_v2_4_0_, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v1_ = V4MergeXY( v1, v1 );
	return __vsldoi( _MXY_v1_v1_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( v1, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( v1, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( v1, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( v1, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( v1, _RLI_v2_v1_8_3_ );
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
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( v1, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( v1, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( v1, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeXY( v1, _SX_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return V4MergeXY( v1, _PRM_v2_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_48_ = __vpermwi( v2, 48 );
	return V4MergeXY( v1, _PRM_v2_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( _MXY_v1_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( _RLI_v1_v2_4_0_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( _RLI_v1_v2_4_1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( _RLI_v1_v2_4_2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 48 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return __vpermwi( _RLI_v1_v2_1_2_, 50 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return __vsldoi( _MXY_v2_v1_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( v1, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_64_ = __vpermwi( v2, 64 );
	return V4MergeXY( v1, _PRM_v2_64_ );
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
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeXY( v1, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_112_ = __vpermwi( v2, 112 );
	return V4MergeXY( v1, _PRM_v2_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return __vpermwi( _RLI_v1_v2_1_2_, 56 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return __vpermwi( _RLI_v1_v2_1_2_, 58 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return __vpermwi( _RLI_v1_v2_2_3_, 44 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return __vpermwi( _RLI_v1_v2_4_0_, 30 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 52 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_6_3_ = __vrlimi( v1, v2, 6, 3 );
	return __vpermwi( _RLI_v1_v2_6_3_, 39 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 60 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return __vpermwi( _RLI_v1_v2_1_2_, 62 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return __vpermwi( _RLI_v1_v2_2_3_, 43 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return __vpermwi( _DOI_v2_v1_4_, 199 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return __vpermwi( _DOI_v2_v1_4_, 203 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_0_ = __vrlimi( v1, v2, 5, 0 );
	return __vpermwi( _RLI_v1_v2_5_0_, 30 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	return V4MergeZW( _SX_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( v1, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( v1, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_128_ = __vpermwi( v2, 128 );
	return V4MergeXY( v1, _PRM_v2_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_144_ = __vpermwi( v2, 144 );
	return V4MergeXY( v1, _PRM_v2_144_ );
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
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return V4MergeXY( v1, _DOI_v2_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( _PRM_v1_2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( _PRM_v1_3_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_1_ = __vrlimi( v1, v2, 5, 1 );
	return __vpermwi( _RLI_v1_v2_5_1_, 28 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return __vrlimi( v1, _PRM_v2_2_, 6, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return __vpermwi( _DOI_v2_v1_4_, 211 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_6_0_ = __vrlimi( v1, v2, 6, 0 );
	return __vpermwi( _RLI_v1_v2_6_0_, 39 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return __vpermwi( _DOI_v2_v1_4_, 215 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 43 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( v1, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( v1, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_12_ = __vsldoi( v2, v2, 12 );
	return V4MergeXY( v1, _DOI_v2_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return V4MergeXY( v1, _PRM_v2_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return V4MergeXY( v1, _PRM_v2_224_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( v1, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_0_ = __vrlimi( v1, v2, 5, 0 );
	return __vpermwi( _RLI_v1_v2_5_0_, 54 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return __vrlimi( v1, _PRM_v2_13_, 6, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_6_1_ = __vrlimi( v1, v2, 6, 1 );
	return __vpermwi( _RLI_v1_v2_6_1_, 39 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vpermwi( _DOI_v2_v1_12_, 67 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vpermwi( _DOI_v2_v1_8_, 226 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 75 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return __vsldoi( _SY_v1_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_22_ = __vpermwi( v1, 22 );
	return __vsldoi( _PRM_v1_22_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return __vsldoi( _SY_v1_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _MXY_v1_v2_, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vpermwi( _DOI_v2_v1_8_, 242 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 91 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_2_3_, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return __vsldoi( _DOI_v1_v1_12_, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _MXY_v1_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_2_3_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 120 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 123 );
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
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( _DOI_v1_v1_4_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 148 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 150 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return __vpermwi( _DOI_v1_v2_4_, 61 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return __vpermwi( _DOI_v1_v2_4_, 62 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return __vrlimi( v2, _RLI_v1_v2_2_2_, 13, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_0_ = __vrlimi( v1, v2, 10, 0 );
	return __vpermwi( _RLI_v1_v2_10_0_, 73 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_0_ = __vrlimi( v1, v2, 10, 0 );
	return __vpermwi( _RLI_v1_v2_10_0_, 75 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	return __vpermwi( _RLI_v1_v2_3_1_, 120 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	return __vpermwi( _RLI_v1_v2_3_1_, 121 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_9_0_ = __vrlimi( v1, v2, 9, 0 );
	return __vpermwi( _RLI_v1_v2_9_0_, 78 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 188 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return __vpermwi( _RLI_v1_v2_1_2_, 126 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( _PRM_v1_4_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	return V4MergeZW( _SY_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( _PRM_v1_7_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_0_ = __vrlimi( v1, v2, 10, 0 );
	return __vpermwi( _RLI_v1_v2_10_0_, 97 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_0_ = __vrlimi( v1, v2, 10, 0 );
	return __vpermwi( _RLI_v1_v2_10_0_, 99 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( _MXY_v1_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vpermwi( _DOI_v2_v1_8_, 194 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vpermwi( _DOI_v2_v1_8_, 195 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return __vpermwi( _RLI_v1_v2_1_3_, 126 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 107 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( _RLI_v2_v1_2_3_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	return __vsldoi( _RLI_v1_v2_3_1_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vpermwi( _DOI_v2_v1_8_, 210 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vpermwi( _DOI_v2_v1_12_, 131 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( v1, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _MZW_v1_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_37_ = __vpermwi( v1, 37 );
	return __vsldoi( _PRM_v1_37_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_38_ = __vpermwi( v1, 38 );
	return __vsldoi( _PRM_v1_38_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( v1, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( v1, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( v1, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( v1, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( v1, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( v1, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_1_1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_1_1_, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _MZW_v1_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v1_v2_1_0_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v1_v2_1_0_, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vsldoi( v1, _RLI_v1_v2_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( v1, _MZW_v1_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( v1, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( v1, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( v1, _PRM_v2_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return __vsldoi( v1, _DOI_v2_v1_4_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( v1, _SZ_v2_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_208_ = __vpermwi( v2, 208 );
	return __vsldoi( v1, _PRM_v2_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( v1, _PRM_v2_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeXY( _SZ_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( v1, _RLI_v1_v2_2_2_ );
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
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return V4MergeZW( v1, _DOI_v1_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( v1, _PRM_v2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( v1, _PRM_v2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( _MZW_v1_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_9_0_ = __vrlimi( v1, v2, 9, 0 );
	return __vpermwi( _RLI_v1_v2_9_0_, 142 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( v1, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( v1, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( v1, _PRM_v2_4_ );
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
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( v1, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( v1, _PRM_v2_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return __vpermwi( _RLI_v1_v2_1_2_, 188 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vpermwi( _DOI_v1_v2_8_, 60 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return __vrlimi( v2, _PRM_v1_2_, 9, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_0_ = __vrlimi( v1, v2, 5, 0 );
	return __vpermwi( _RLI_v1_v2_5_0_, 158 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( _PRM_v1_8_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( _PRM_v1_9_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( _SZ_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( v1, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( v1, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( v1, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( v1, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( v1, _PRM_v2_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( v1, _PRM_v2_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( v1, _SZ_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( _RLI_v1_v2_1_1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( _RLI_v1_v2_1_2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( _RLI_v1_v2_1_0_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
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
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( v1, _DOI_v2_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( v1, _PRM_v2_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( v1, _PRM_v2_14_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( v1, _SW_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_0_ = __vrlimi( v1, v2, 5, 0 );
	return __vpermwi( _RLI_v1_v2_5_0_, 182 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 60 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_8_3_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 200 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 203 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_6_2_ = __vrlimi( v1, v2, 6, 2 );
	return __vsldoi( v1, _RLI_v1_v2_6_2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_53_ = __vpermwi( v1, 53 );
	return __vsldoi( _PRM_v1_53_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_54_ = __vpermwi( v1, 54 );
	return __vsldoi( _PRM_v1_54_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_12_1_ = __vrlimi( v2, v1, 12, 1 );
	return __vsldoi( v1, _RLI_v2_v1_12_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return __vsldoi( v1, _RLI_v2_v1_8_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 216 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 219 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _DOI_v1_v2_4_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return __vpermwi( _RLI_v1_v2_4_1_, 228 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vsldoi( v1, _MZW_v1_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _MZW_v1_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_12_3_ = __vrlimi( v2, v1, 12, 3 );
	return __vsldoi( v1, _RLI_v2_v1_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _DOI_v1_v2_4_, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_8_3_, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( v1, _RLI_v2_v1_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 248 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 166 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _MZW_v1_v2_, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	return V4MergeXY( _SW_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( _RLI_v2_v1_8_3_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vpermwi( _DOI_v1_v2_12_, 28 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_32_ = __vpermwi( v2, 32 );
	return __vsldoi( v1, _PRM_v2_32_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_36_ = __vpermwi( v2, 36 );
	return __vsldoi( v1, _PRM_v2_36_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_40_ = __vpermwi( v2, 40 );
	return __vsldoi( v1, _PRM_v2_40_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_44_ = __vpermwi( v2, 44 );
	return __vsldoi( v1, _PRM_v2_44_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_12_3_ = __vrlimi( v1, v2, 12, 3 );
	return __vpermwi( _RLI_v1_v2_12_3_, 210 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return __vpermwi( _RLI_v1_v2_2_3_, 224 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return __vpermwi( _RLI_v1_v2_4_0_, 210 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return __vpermwi( _RLI_v1_v2_2_3_, 228 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return __vpermwi( _RLI_v1_v2_4_0_, 216 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vpermwi( _DOI_v1_v2_8_, 112 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return __vpermwi( _RLI_v1_v2_2_3_, 236 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vpermwi( _DOI_v1_v2_8_, 116 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_6_3_ = __vrlimi( v1, v2, 6, 3 );
	return __vpermwi( _RLI_v1_v2_6_3_, 228 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vpermwi( _DOI_v1_v2_8_, 120 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return __vpermwi( _RLI_v1_v2_2_3_, 232 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vpermwi( _DOI_v1_v2_8_, 124 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vpermwi( _DOI_v1_v2_8_, 125 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_6_0_ = __vrlimi( v1, v2, 6, 0 );
	return __vpermwi( _RLI_v1_v2_6_0_, 216 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return __vrlimi( v2, _PRM_v1_7_, 9, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vpermwi( _DOI_v1_v2_12_, 44 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_96_ = __vpermwi( v2, 96 );
	return __vsldoi( v1, _PRM_v2_96_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_100_ = __vpermwi( v2, 100 );
	return __vsldoi( v1, _PRM_v2_100_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_104_ = __vpermwi( v2, 104 );
	return __vsldoi( v1, _PRM_v2_104_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return __vrlimi( v2, _RLI_v1_v2_4_2_, 11, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( _DOI_v1_v1_4_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( _PRM_v1_13_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( _PRM_v1_14_, v2 );
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
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( _DOI_v1_v2_4_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_6_0_ = __vrlimi( v1, v2, 6, 0 );
	return __vpermwi( _RLI_v1_v2_6_0_, 228 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vpermwi( _DOI_v1_v2_12_, 56 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 232 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 233 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 148 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 150 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_160_ = __vpermwi( v2, 160 );
	return __vsldoi( v1, _PRM_v2_160_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_164_ = __vpermwi( v2, 164 );
	return __vsldoi( v1, _PRM_v2_164_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return __vsldoi( v1, _SZ_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v2_ = V4MergeZW( v2, v2 );
	return __vsldoi( v1, _MZW_v2_v2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( _MZW_v1_v2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( v1, _RLI_v1_v2_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_12_3_ = __vrlimi( v1, v2, 12, 3 );
	return __vsldoi( v1, _RLI_v1_v2_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_1_ = __vrlimi( v1, v2, 10, 1 );
	return __vpermwi( _RLI_v1_v2_10_1_, 227 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_6_1_ = __vrlimi( v1, v2, 6, 1 );
	return __vpermwi( _RLI_v1_v2_6_1_, 228 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 182 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_224_ = __vpermwi( v2, 224 );
	return __vsldoi( v1, _PRM_v2_224_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_228_ = __vpermwi( v2, 228 );
	return __vsldoi( v1, _PRM_v2_228_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_232_ = __vpermwi( v2, 232 );
	return __vsldoi( v1, _PRM_v2_232_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_236_ = __vpermwi( v2, 236 );
	return __vsldoi( v1, _PRM_v2_236_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 188 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _MXY_v2_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v1_v2_8_0_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, v1 );
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
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return V4MergeXY( v2, _PRM_v1_32_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_48_ = __vpermwi( v1, 48 );
	return V4MergeXY( v2, _PRM_v1_48_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeXY( v2, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_0_ = __vrlimi( v1, v2, 4, 0 );
	return V4MergeXY( v2, _RLI_v1_v2_4_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_1_ = __vrlimi( v1, v2, 4, 1 );
	return V4MergeXY( v2, _RLI_v1_v2_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_4_2_ = __vrlimi( v1, v2, 4, 2 );
	return V4MergeXY( v2, _RLI_v1_v2_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 97 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	return __vpermwi( _RLI_v1_v2_3_1_, 210 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 105 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_0_ = __vrlimi( v1, v2, 10, 0 );
	return __vpermwi( _RLI_v1_v2_10_0_, 22 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	return __vpermwi( _RLI_v1_v2_3_1_, 214 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return __vpermwi( _DOI_v1_v2_4_, 199 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return __vpermwi( _DOI_v1_v2_4_, 203 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_0_ = __vrlimi( v1, v2, 10, 0 );
	return __vpermwi( _RLI_v1_v2_10_0_, 30 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 101 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_0_ = __vrlimi( v1, v2, 10, 0 );
	return __vpermwi( _RLI_v1_v2_10_0_, 18 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_64_ = __vpermwi( v1, 64 );
	return V4MergeXY( v2, _PRM_v1_64_ );
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
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeXY( v2, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_112_ = __vpermwi( v1, 112 );
	return V4MergeXY( v2, _PRM_v1_112_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vsldoi( _MXY_v1_v2_, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_1_ = __vrlimi( v2, v1, 8, 1 );
	return V4MergeXY( v2, _RLI_v2_v1_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_0_ = __vrlimi( v1, v2, 10, 0 );
	return __vpermwi( _RLI_v1_v2_10_0_, 24 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_0_ = __vrlimi( v1, v2, 10, 0 );
	return __vpermwi( _RLI_v1_v2_10_0_, 26 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	return __vpermwi( _RLI_v1_v2_3_1_, 219 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_11_0_ = __vrlimi( v1, v2, 11, 0 );
	return __vpermwi( _RLI_v1_v2_11_0_, 30 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return __vpermwi( _RLI_v1_v2_1_1_, 227 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return __vrlimi( v2, _PRM_v1_2_, 6, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return __vpermwi( _DOI_v1_v2_4_, 211 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_9_0_ = __vrlimi( v1, v2, 9, 0 );
	return __vpermwi( _RLI_v1_v2_9_0_, 39 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return __vpermwi( _DOI_v1_v2_4_, 215 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_9_0_ = __vrlimi( v1, v2, 9, 0 );
	return __vpermwi( _RLI_v1_v2_9_0_, 43 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	return V4MergeZW( _SX_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_128_ = __vpermwi( v1, 128 );
	return V4MergeXY( v2, _PRM_v1_128_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_144_ = __vpermwi( v1, 144 );
	return V4MergeXY( v2, _PRM_v1_144_ );
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
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeXY( v2, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_2_ = __vrlimi( v2, v1, 8, 2 );
	return V4MergeXY( v2, _RLI_v2_v1_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeXY( v2, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return V4MergeZW( _PRM_v2_2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_3_ = __vpermwi( v2, 3 );
	return V4MergeZW( _PRM_v2_3_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_0_ = __vrlimi( v1, v2, 10, 0 );
	return __vpermwi( _RLI_v1_v2_10_0_, 54 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return __vrlimi( v2, _PRM_v1_13_, 6, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_12_3_ = __vrlimi( v1, v2, 12, 3 );
	return __vpermwi( _RLI_v1_v2_12_3_, 120 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vpermwi( _DOI_v1_v2_12_, 67 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeXY( v2, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return V4MergeXY( v2, _PRM_v1_208_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return V4MergeXY( v2, _PRM_v1_224_ );
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
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeXY( v2, _DOI_v1_v2_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return V4MergeXY( v2, _RLI_v2_v1_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _MXY_v2_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( _MXY_v2_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v1_v2_8_0_, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( _RLI_v1_v2_8_0_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( _RLI_v2_v1_4_1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeXY( v2, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_0_ = __vrlimi( v1, v2, 8, 0 );
	return V4MergeXY( v2, _RLI_v1_v2_8_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return V4MergeXY( v2, _RLI_v2_v1_4_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_2_ = __vrlimi( v2, v1, 4, 2 );
	return V4MergeXY( v2, _RLI_v2_v1_4_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v2_ = V4MergeXY( v2, v2 );
	return __vsldoi( _MXY_v2_v2_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return V4MergeXY( v2, _RLI_v1_v2_8_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( _RLI_v1_v2_2_2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( v2, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( v2, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return V4MergeXY( v2, _DOI_v2_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( v2, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 193 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 195 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return __vpermwi( _DOI_v2_v1_4_, 61 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return __vpermwi( _DOI_v2_v1_4_, 62 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_2_ = __vrlimi( v1, v2, 3, 2 );
	return __vrlimi( v1, _RLI_v1_v2_3_2_, 13, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return __vpermwi( _RLI_v1_v2_1_2_, 203 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_0_ = __vrlimi( v1, v2, 5, 0 );
	return __vpermwi( _RLI_v1_v2_5_0_, 75 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_6_3_ = __vrlimi( v1, v2, 6, 3 );
	return __vpermwi( _RLI_v1_v2_6_3_, 141 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return __vpermwi( _RLI_v1_v2_2_3_, 142 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_6_0_ = __vrlimi( v1, v2, 6, 0 );
	return __vpermwi( _RLI_v1_v2_6_0_, 78 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return V4MergeXY( _SY_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return __vpermwi( _MXY_v1_v2_, 233 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_3_ = __vrlimi( v1, v2, 3, 3 );
	return __vpermwi( _RLI_v1_v2_3_3_, 151 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return __vpermwi( _RLI_v1_v2_1_2_, 227 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_0_ = __vrlimi( v1, v2, 5, 0 );
	return __vpermwi( _RLI_v1_v2_5_0_, 99 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _MXY_v2_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vpermwi( _DOI_v1_v2_8_, 194 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vpermwi( _DOI_v1_v2_8_, 195 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_9_2_ = __vrlimi( v1, v2, 9, 2 );
	return __vpermwi( _RLI_v1_v2_9_2_, 232 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_0_ = __vrlimi( v1, v2, 5, 0 );
	return __vpermwi( _RLI_v1_v2_5_0_, 107 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_2_3_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_4_ = __vpermwi( v2, 4 );
	return V4MergeZW( _PRM_v2_4_, v1 );
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
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return V4MergeZW( _DOI_v1_v2_12_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return V4MergeZW( _PRM_v2_7_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vpermwi( _DOI_v1_v2_8_, 210 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vpermwi( _DOI_v1_v2_12_, 131 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vpermwi( _DOI_v1_v2_8_, 226 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_13_0_ = __vrlimi( v1, v2, 13, 0 );
	return __vpermwi( _RLI_v1_v2_13_0_, 75 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return __vsldoi( _SY_v2_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _MXY_v2_v1_, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_8_ = __vsldoi( v1, v2, 8 );
	return __vpermwi( _DOI_v1_v2_8_, 242 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_0_ = __vrlimi( v1, v2, 5, 0 );
	return __vpermwi( _RLI_v1_v2_5_0_, 91 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_2_3_, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	return __vsldoi( _SY_v2_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_22_ = __vpermwi( v2, 22 );
	return __vsldoi( _PRM_v2_22_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v2_12_ = __vsldoi( v1, v2, 12 );
	return __vsldoi( _DOI_v1_v2_12_, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( _MXY_v2_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( _RLI_v1_v2_2_3_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_13_0_ = __vrlimi( v1, v2, 13, 0 );
	return __vpermwi( _RLI_v1_v2_13_0_, 120 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_0_ = __vrlimi( v1, v2, 5, 0 );
	return __vpermwi( _RLI_v1_v2_5_0_, 123 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _MZW_v2_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 142 );
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
	Vector_4V _DOI_v1_v1_8_ = __vsldoi( v1, v1, 8 );
	return V4MergeZW( v2, _DOI_v1_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_2_ = __vpermwi( v1, 2 );
	return V4MergeZW( v2, _PRM_v1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_3_ = __vpermwi( v1, 3 );
	return V4MergeZW( v2, _PRM_v1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_2_ = __vrlimi( v2, v1, 2, 2 );
	return V4MergeZW( v2, _RLI_v2_v1_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_10_0_ = __vrlimi( v1, v2, 10, 0 );
	return __vpermwi( _RLI_v1_v2_10_0_, 148 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vpermwi( _DOI_v2_v1_8_, 60 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_2_ = __vpermwi( v2, 2 );
	return __vrlimi( v1, _PRM_v2_2_, 9, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 158 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_4_ = __vpermwi( v1, 4 );
	return V4MergeZW( v2, _PRM_v1_4_ );
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
	Vector_4V _DOI_v1_v1_12_ = __vsldoi( v1, v1, 12 );
	return V4MergeZW( v2, _DOI_v1_v1_12_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_7_ = __vpermwi( v1, 7 );
	return V4MergeZW( v2, _PRM_v1_7_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v1_v2_ = V4MergeXY( v1, v2 );
	return V4MergeZW( v2, _MXY_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_2_3_ = __vrlimi( v2, v1, 2, 3 );
	return V4MergeZW( v2, _RLI_v2_v1_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_1_1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v1_v2_2_0_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_8_ = __vpermwi( v2, 8 );
	return V4MergeZW( _PRM_v2_8_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_9_ = __vpermwi( v2, 9 );
	return V4MergeZW( _PRM_v2_9_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	return V4MergeZW( _SZ_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_8_ = __vpermwi( v1, 8 );
	return V4MergeZW( v2, _PRM_v1_8_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_9_ = __vpermwi( v1, 9 );
	return V4MergeZW( v2, _PRM_v1_9_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return V4MergeZW( v2, _SZ_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_1_ = __vrlimi( v1, v2, 1, 1 );
	return V4MergeZW( v2, _RLI_v1_v2_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_2_ = __vrlimi( v1, v2, 1, 2 );
	return V4MergeZW( v2, _RLI_v1_v2_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_3_ = __vrlimi( v1, v2, 1, 3 );
	return V4MergeZW( v2, _RLI_v1_v2_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_1_0_ = __vrlimi( v1, v2, 1, 0 );
	return V4MergeZW( v2, _RLI_v1_v2_1_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return __vpermwi( _RLI_v1_v2_2_0_, 182 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 105 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return V4MergeZW( v2, _DOI_v1_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_13_ = __vpermwi( v1, 13 );
	return V4MergeZW( v2, _PRM_v1_13_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_14_ = __vpermwi( v1, 14 );
	return V4MergeZW( v2, _PRM_v1_14_ );
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
	Vector_4V _DOI_v1_v2_4_ = __vsldoi( v1, v2, 4 );
	return V4MergeZW( v2, _DOI_v1_v2_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return V4MergeZW( v2, _MZW_v1_v2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _MZW_v2_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_2_ = __vrlimi( v1, v2, 2, 2 );
	return V4MergeZW( v2, _RLI_v1_v2_2_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_37_ = __vpermwi( v2, 37 );
	return __vsldoi( _PRM_v2_37_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_38_ = __vpermwi( v2, 38 );
	return __vsldoi( _PRM_v2_38_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MXY_v2_v1_ = V4MergeXY( v2, v1 );
	return V4MergeZW( v2, _MXY_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_3_ = __vrlimi( v1, v2, 2, 3 );
	return V4MergeZW( v2, _RLI_v1_v2_2_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_1_1_, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( _RLI_v2_v1_1_1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_2_ = __vrlimi( v1, v2, 8, 2 );
	return V4MergeXY( _RLI_v1_v2_8_2_, _RLI_v1_v2_8_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeXY( _MZW_v2_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v1_v2_2_0_, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( _RLI_v1_v2_2_0_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_1_ = __vrlimi( v2, v1, 1, 1 );
	return V4MergeZW( v2, _RLI_v2_v1_1_1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_2_ = __vrlimi( v2, v1, 1, 2 );
	return V4MergeZW( v2, _RLI_v2_v1_1_2_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_1_3_ = __vrlimi( v2, v1, 1, 3 );
	return V4MergeZW( v2, _RLI_v2_v1_1_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_0_ = __vrlimi( v1, v2, 2, 0 );
	return V4MergeZW( v2, _RLI_v1_v2_2_0_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( v2, _PRM_v1_32_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v1_v1_4_ = __vsldoi( v1, v1, 4 );
	return __vsldoi( v2, _DOI_v1_v1_4_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( v2, _SZ_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_208_ = __vpermwi( v1, 208 );
	return __vsldoi( v2, _PRM_v1_208_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( v2, _PRM_v1_224_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_4_1_ = __vrlimi( v2, v1, 4, 1 );
	return __vsldoi( v2, _RLI_v2_v1_4_1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( v2, _MZW_v2_v1_, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( v2, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( v2, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_8_3_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_32_ = __vpermwi( v1, 32 );
	return __vsldoi( v2, _PRM_v1_32_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_36_ = __vpermwi( v1, 36 );
	return __vsldoi( v2, _PRM_v1_36_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_40_ = __vpermwi( v1, 40 );
	return __vsldoi( v2, _PRM_v1_40_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_44_ = __vpermwi( v1, 44 );
	return __vsldoi( v2, _PRM_v1_44_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vpermwi( _DOI_v2_v1_12_, 28 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_6_1_ = __vrlimi( v1, v2, 6, 1 );
	return __vpermwi( _RLI_v1_v2_6_1_, 141 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeXY( _SW_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	return __vpermwi( _RLI_v1_v2_3_1_, 147 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vpermwi( _DOI_v2_v1_8_, 120 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	return __vpermwi( _RLI_v1_v2_3_1_, 151 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vpermwi( _DOI_v2_v1_8_, 124 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vpermwi( _DOI_v2_v1_8_, 125 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_96_ = __vpermwi( v1, 96 );
	return __vsldoi( v2, _PRM_v1_96_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_100_ = __vpermwi( v1, 100 );
	return __vsldoi( v2, _PRM_v1_100_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_104_ = __vpermwi( v1, 104 );
	return __vsldoi( v2, _PRM_v1_104_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_9_0_ = __vrlimi( v1, v2, 9, 0 );
	return __vpermwi( _RLI_v1_v2_9_0_, 216 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_7_ = __vpermwi( v2, 7 );
	return __vrlimi( v1, _PRM_v2_7_, 9, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vpermwi( _DOI_v2_v1_12_, 44 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_12_2_ = __vrlimi( v1, v2, 12, 2 );
	return __vrlimi( v1, _RLI_v1_v2_12_2_, 11, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	return __vpermwi( _RLI_v1_v2_3_1_, 159 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_11_0_ = __vrlimi( v1, v2, 11, 0 );
	return __vpermwi( _RLI_v1_v2_11_0_, 210 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_11_1_ = __vrlimi( v1, v2, 11, 1 );
	return __vpermwi( _RLI_v1_v2_11_1_, 147 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_11_0_ = __vrlimi( v1, v2, 11, 0 );
	return __vpermwi( _RLI_v1_v2_11_0_, 216 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vpermwi( _DOI_v2_v1_8_, 112 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_3_1_ = __vrlimi( v1, v2, 3, 1 );
	return __vpermwi( _RLI_v1_v2_3_1_, 155 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_8_ = __vsldoi( v2, v1, 8 );
	return __vpermwi( _DOI_v2_v1_8_, 116 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _DOI_v2_v1_4_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_9_0_ = __vrlimi( v1, v2, 9, 0 );
	return __vpermwi( _RLI_v1_v2_9_0_, 228 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_12_ = __vsldoi( v2, v1, 12 );
	return __vpermwi( _DOI_v2_v1_12_, 56 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_160_ = __vpermwi( v1, 160 );
	return __vsldoi( v2, _PRM_v1_160_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_164_ = __vpermwi( v1, 164 );
	return __vsldoi( v2, _PRM_v1_164_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	return __vsldoi( v2, _SZ_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v1_ = V4MergeZW( v1, v1 );
	return __vsldoi( v2, _MZW_v1_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_9_0_ = __vrlimi( v1, v2, 9, 0 );
	return __vpermwi( _RLI_v1_v2_9_0_, 232 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_0_ = __vrlimi( v1, v2, 5, 0 );
	return __vpermwi( _RLI_v1_v2_5_0_, 233 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 193 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 195 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _MZW_v2_v1_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v2_4_ = __vsldoi( v2, v2, 4 );
	return V4MergeZW( _DOI_v2_v2_4_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_13_ = __vpermwi( v2, 13 );
	return V4MergeZW( _PRM_v2_13_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_14_ = __vpermwi( v2, 14 );
	return V4MergeZW( _PRM_v2_14_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	return V4MergeZW( _SW_v2_, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_12_3_ = __vrlimi( v2, v1, 12, 3 );
	return __vsldoi( v2, _RLI_v2_v1_12_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_2_1_ = __vrlimi( v1, v2, 2, 1 );
	return __vpermwi( _RLI_v1_v2_2_1_, 182 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_224_ = __vpermwi( v1, 224 );
	return __vsldoi( v2, _PRM_v1_224_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_228_ = __vpermwi( v1, 228 );
	return __vsldoi( v2, _PRM_v1_228_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_232_ = __vpermwi( v1, 232 );
	return __vsldoi( v2, _PRM_v1_232_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v1_236_ = __vpermwi( v1, 236 );
	return __vsldoi( v2, _PRM_v1_236_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_12_3_ = __vrlimi( v1, v2, 12, 3 );
	return __vpermwi( _RLI_v1_v2_12_3_, 57 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 227 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 233 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_8_3_ = __vrlimi( v2, v1, 8, 3 );
	return __vsldoi( v2, _RLI_v2_v1_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_8_3_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_9_0_ = __vrlimi( v1, v2, 9, 0 );
	return __vpermwi( _RLI_v1_v2_9_0_, 200 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_9_0_ = __vrlimi( v1, v2, 9, 0 );
	return __vpermwi( _RLI_v1_v2_9_0_, 203 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v2_v1_6_2_ = __vrlimi( v2, v1, 6, 2 );
	return __vsldoi( v2, _RLI_v2_v1_6_2_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_1_ = __vrlimi( v1, v2, 8, 1 );
	return __vsldoi( v2, _RLI_v1_v2_8_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_13_0_ = __vrlimi( v1, v2, 13, 0 );
	return __vpermwi( _RLI_v1_v2_13_0_, 216 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_5_0_ = __vrlimi( v1, v2, 5, 0 );
	return __vpermwi( _RLI_v1_v2_5_0_, 219 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_53_ = __vpermwi( v2, 53 );
	return __vsldoi( _PRM_v2_53_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _PRM_v2_54_ = __vpermwi( v2, 54 );
	return __vsldoi( _PRM_v2_54_, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_12_1_ = __vrlimi( v1, v2, 12, 1 );
	return __vsldoi( v2, _RLI_v1_v2_12_1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _DOI_v2_v1_4_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_13_3_ = __vrlimi( v1, v2, 13, 3 );
	return __vpermwi( _RLI_v1_v2_13_3_, 57 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return __vsldoi( v2, _MZW_v2_v1_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _MZW_v2_v1_, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _DOI_v2_v1_4_ = __vsldoi( v2, v1, 4 );
	return V4MergeZW( _DOI_v2_v1_4_, _DOI_v2_v1_4_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return V4MergeXY( _RLI_v1_v2_8_3_, _RLI_v1_v2_8_3_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_8_3_ = __vrlimi( v1, v2, 8, 3 );
	return __vsldoi( v2, _RLI_v1_v2_8_3_, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_9_0_ = __vrlimi( v1, v2, 9, 0 );
	return __vpermwi( _RLI_v1_v2_9_0_, 248 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v1_v2_ = V4MergeZW( v1, v2 );
	return __vpermwi( _MZW_v1_v2_, 243 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _MZW_v2_v1_ = V4MergeZW( v2, v1 );
	return V4MergeZW( _MZW_v2_v1_, _MZW_v2_v1_ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _RLI_v1_v2_12_3_ = __vrlimi( v1, v2, 12, 3 );
	return __vsldoi( v2, _RLI_v1_v2_12_3_, 12 );
}

