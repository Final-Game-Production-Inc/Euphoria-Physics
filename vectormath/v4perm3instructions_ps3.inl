template <>
__forceinline Vector_4V_Out V4Permute<X,X,X,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __splatx_v__, __splatz_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,X,X,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( __splatx_v__, __splatw_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,X,Y,W>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeXY( v, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,X,Z,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	return V4MergeXY( __mergexy_v___splatz_v____, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,X,Z,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __splatx_v__, __splatz_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,X,Z,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return vec_sld( __splatx_v__, __vsldoi_v_v_8__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,X,W,X>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return vec_sld( __splatx_v__, __vsldoi_v_v_12__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,X,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeXY( __mergexy_v___splatw_v____, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,X,W,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( __splatx_v__, __splatw_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,X,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return vec_sld( __vsldoi_v_v_8__, __splatx_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,X,Y>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeXY( __splatx_v__, __splaty_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,X,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeXY( __splatx_v__, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,Y,W>( Vector_4V_In v )
{
	Vector_4V __mergexy_v_v__ = V4MergeXY( v, v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( __mergexy_v_v__, __splatw_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,Z,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __vsldoi_v_v_12__, __splaty_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,Z,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __vsldoi_v_v_8__, __splatz_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,W,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return vec_sld( __vsldoi_v_v_8__, __vsldoi_v_v_12__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,W,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( __vsldoi_v_v_8__, __splatw_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,X,X>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	return V4MergeXY( __splatx_v__, __mergezw_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,X,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeXY( __splatx_v__, __splatz_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,Y,X>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	return V4MergeXY( v, __mergezw_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeXY( v, __mergezw_v___splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,Z,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __splatx_v__, __splatz_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,Z,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v_v__ = V4MergeZW( v, v );
	return vec_sld( __splatx_v__, __mergezw_v_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,W,X>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return vec_sld( __splatx_v__, __vsldoi_v_v_8__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,X,X>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeZW( __splatx_v__, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,X,Y>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return vec_sld( __splatx_v__, __vsldoi_v_v_12__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,X,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeXY( __splatx_v__, __splatw_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergezw___splatw_v___v__ = V4MergeZW( __splatw_v__, v );
	return V4MergeXY( v, __mergezw___splatw_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,W,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( __splatx_v__, __splatw_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,X,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return vec_sld( __splaty_v__, __splatx_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,X,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergexy_v_v__ = V4MergeXY( v, v );
	return vec_sld( __splaty_v__, __mergexy_v_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,Y,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeXY( __splaty_v__, __splatx_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,Z,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeXY( __vsldoi_v_v_4__, __splatx_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,X,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return vec_sld( __splaty_v__, __splatx_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeXY( __splaty_v__, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,Y,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( __splaty_v__, __splatw_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,Z,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeXY( __vsldoi_v_v_4__, __splaty_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,Z,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return vec_sld( __splaty_v__, __vsldoi_v_v_4__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,W,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return vec_sld( __splaty_v__, __vsldoi_v_v_12__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,W,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( __splaty_v__, __splatw_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,X,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return vec_sld( __vsldoi_v_v_12__, __splatx_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,X,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	return V4MergeZW( __mergexy_v___splatx_v____, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,Y,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __vsldoi_v_v_12__, __splaty_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeXY( __splaty_v__, __splatz_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,Z,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeXY( __vsldoi_v_v_4__, __splatz_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,Z,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return V4MergeXY( __vsldoi_v_v_4__, __vsldoi_v_v_8__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,X,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return vec_sld( __splaty_v__, __vsldoi_v_v_12__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,Y,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeZW( __splaty_v__, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeZW( __splaty_v__, __mergezw_v___splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,Y,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeXY( __splaty_v__, __splatw_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,Z,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return V4MergeXY( __vsldoi_v_v_4__, __vsldoi_v_v_12__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,Z,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeXY( __vsldoi_v_v_4__, __splatw_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,W,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( __splaty_v__, __splatw_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,X,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return vec_sld( __splatz_v__, __splatx_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,X,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v_v__ = V4MergeXY( v, v );
	return vec_sld( __splatz_v__, __mergexy_v_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeXY( __mergezw_v___splaty_v____, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,Z,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeXY( __splatz_v__, __splatx_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,Z,Z>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	return V4MergeXY( __splatz_v__, __mergexy_v___splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,W,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw___splatx_v___v__ = V4MergeZW( __splatx_v__, v );
	return V4MergeZW( v, __mergezw___splatx_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __splatz_v__, __splaty_v__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,Z,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeXY( __splatz_v__, __splaty_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,Z,Z>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeXY( __splatz_v__, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,Z,W>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return vec_sld( __splatz_v__, __vsldoi_v_v_4__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,W,X>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	return V4MergeZW( v, __mergexy_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,W,W>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeZW( v, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,X,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return vec_sld( __splatz_v__, __splatx_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __splatz_v__, __splaty_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return vec_sld( __splatz_v__, __vsldoi_v_v_4__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,Z,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __splatz_v__, __splaty_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,W,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return vec_sld( __splatz_v__, __vsldoi_v_v_8__, 12 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy___splatz_v___v__ = V4MergeXY( __splatz_v__, v );
	return V4MergeZW( v, __mergexy___splatz_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,X,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __vsldoi_v_v_4__, __splatz_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,X,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( __vsldoi_v_v_4__, __splatw_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,Z,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeZW( __splatz_v__, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,Z,W>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeXY( __splatz_v__, __splatw_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,W,Y>( Vector_4V_In v )
{
	Vector_4V __mergezw_v_v__ = V4MergeZW( v, v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __mergezw_v_v__, __splaty_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,W,Z>( Vector_4V_In v )
{
	Vector_4V __mergezw_v_v__ = V4MergeZW( v, v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __mergezw_v_v__, __splatz_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,X,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeZW( __vsldoi_v_v_4__, __splatx_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,X,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return V4MergeZW( __vsldoi_v_v_4__, __vsldoi_v_v_8__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,Y,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __vsldoi_v_v_4__, __splaty_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,Y,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( __vsldoi_v_v_8__, __splatw_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,Z,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __vsldoi_v_v_4__, __splatz_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,Z,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return vec_sld( __vsldoi_v_v_4__, __vsldoi_v_v_8__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,W,X>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeXY( __splatw_v__, __splatx_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,W,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return vec_sld( __vsldoi_v_v_4__, __splatw_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,X,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeZW( __vsldoi_v_v_4__, __splaty_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,X,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return V4MergeZW( __vsldoi_v_v_4__, __vsldoi_v_v_12__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeXY( __splatw_v__, __splaty_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeXY( __splatw_v__, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,W,W>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeZW( __splatw_v__, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,X,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeZW( __vsldoi_v_v_4__, __splatz_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,Y,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeZW( __mergezw_v___splaty_v____, v );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeXY( __splatw_v__, __splatz_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,X,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeZW( __vsldoi_v_v_4__, __splatw_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __splatw_v__, __splaty_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return vec_sld( __splatw_v__, __vsldoi_v_v_4__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,Z,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __splatw_v__, __splatz_v__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,Z,W>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return vec_sld( __splatw_v__, __vsldoi_v_v_8__, 8 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,W,X>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeZW( __splatw_v__, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return vec_sld( __splatw_v__, __splaty_v__, 4 );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return vec_sld( __splatw_v__, __splatz_v__, 4 );
}

