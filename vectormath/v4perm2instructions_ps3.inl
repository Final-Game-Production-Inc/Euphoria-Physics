template <>
__forceinline Vector_4V_Out V4Permute<X,X,X,Y>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeXY( __splatx_v__, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,X,Y,X>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeXY( v, __splatx_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,X,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	return vec_sld( __splatx_v__, v, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,Y,X>( Vector_4V_In v )
{
	Vector_4V __mergexy_v_v__ = V4MergeXY( v, v );
	return vec_sld( __mergexy_v_v__, v, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeXY( v, __splaty_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,Y,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeXY( v, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,Z,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return vec_sld( __vsldoi_v_v_12__, v, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,X,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeZW( __splatx_v__, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeXY( v, __splatz_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,Y,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return V4MergeXY( v, __vsldoi_v_v_8__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,Y,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return V4MergeXY( v, __vsldoi_v_v_12__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,Y,W>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeXY( v, __splatw_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeXY( __splaty_v__, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __splaty_v__, v, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,Z,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeXY( __vsldoi_v_v_4__, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,X,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __splaty_v__, v, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,Y,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __splaty_v__, v, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,Z,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeXY( __vsldoi_v_v_4__, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,X,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return vec_sld( __vsldoi_v_v_12__, v, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,Y,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeZW( __splaty_v__, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,W,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( v, __splaty_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( v, __splatz_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,W,W>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( v, __splatw_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __splatz_v__, v, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,Z,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeXY( __splatz_v__, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,W,X>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeZW( v, __splatx_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,W,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return V4MergeZW( v, __vsldoi_v_v_8__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,W,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeZW( v, __splaty_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,W,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return V4MergeZW( v, __vsldoi_v_v_12__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,X,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __splatz_v__, v, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,Z,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __splatz_v__, v, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,Z,W>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeZW( __splatz_v__, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeZW( v, __splatz_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,X,X>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	return vec_sld( v, __splatx_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( v, __splaty_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,Y,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return vec_sld( v, __vsldoi_v_v_4__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,Z,Z>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( v, __splatz_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,W,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeZW( v, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,W,W>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeZW( v, __splatw_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,Y,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return vec_sld( __vsldoi_v_v_8__, v, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeXY( __splatw_v__, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( v, __splaty_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,Z,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return vec_sld( v, __vsldoi_v_v_4__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,X,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeZW( __vsldoi_v_v_4__, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,Z,Z>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( v, __splatz_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,Z,W>( Vector_4V_In v )
{
	Vector_4V __mergezw_v_v__ = V4MergeZW( v, v );
	return vec_sld( v, __mergezw_v_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,W,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return vec_sld( v, __vsldoi_v_v_8__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,W,W>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeZW( __splatw_v__, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,X,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeZW( __vsldoi_v_v_4__, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,X,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return vec_sld( v, __vsldoi_v_v_12__, 12 );
}

