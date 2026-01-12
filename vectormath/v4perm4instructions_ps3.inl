template <>
__forceinline Vector_4V_Out V4Permute<X,X,Z,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeXY( __mergexy_v___splatz_v____, __splatx_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,X,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeZW( __splatx_v__, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeXY( __mergexy_v___splatw_v____, __splaty_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Y,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeXY( __mergexy_v___splatw_v____, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,X,Y>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeXY( __splatx_v__, __mergezw_v___splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeXY( __mergexy_v___splatw_v____, __splatz_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,W,W>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return V4MergeXY( __mergexy_v___splatw_v____, __vsldoi_v_v_8__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,X,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	return V4MergeZW( __splatx_v__, __mergezw_v___splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergexy___splatw_v_____splaty_v____ = V4MergeXY( __splatw_v__, __splaty_v__ );
	return V4MergeXY( v, __mergexy___splatw_v_____splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,Z,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return V4MergeXY( __mergexy_v___splatz_v____, __vsldoi_v_v_12__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,Z,W>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeXY( __mergexy_v___splatz_v____, __splatw_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,W,X>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return V4MergeXY( __mergexy_v___splatw_v____, __vsldoi_v_v_12__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __mergezw___splatw_v___v__ = V4MergeZW( __splatw_v__, v );
	return V4MergeXY( __mergexy_v___splatw_v____, __mergezw___splatw_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,X,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	Vector_4V __mergezw___splatx_v___v__ = V4MergeZW( __splatx_v__, v );
	return V4MergeZW( __mergexy_v___splatx_v____, __mergezw___splatx_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,Y,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeXY( __splaty_v__, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,Z,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	return V4MergeXY( __vsldoi_v_v_4__, __mergexy_v___splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,Z,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeXY( __vsldoi_v_v_4__, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,W,X>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeZW( __mergexy_v___splatw_v____, __splatx_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return V4MergeZW( __mergexy_v___splatw_v____, __vsldoi_v_v_8__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,X,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return V4MergeZW( __mergexy_v___splatx_v____, __vsldoi_v_v_12__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,Z,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergexy___splaty_v___v__ = V4MergeXY( __splaty_v__, v );
	return V4MergeXY( __vsldoi_v_v_4__, __mergexy___splaty_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeZW( __mergexy_v___splatw_v____, __splaty_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return V4MergeZW( __mergexy_v___splatw_v____, __vsldoi_v_v_12__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,X,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeZW( __mergexy_v___splatx_v____, __splatz_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,Y,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	return V4MergeXY( __splaty_v__, __mergezw_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,Z,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	return V4MergeXY( __vsldoi_v_v_4__, __mergezw_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Z,Z,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeXY( __vsldoi_v_v_4__, __mergezw_v___splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,X,X>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeZW( __mergexy_v___splatx_v____, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,X,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeZW( __mergexy_v___splatx_v____, __splatw_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	return V4MergeZW( __splaty_v__, __mergezw_v___splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,Z,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergezw___splatw_v___v__ = V4MergeZW( __splatw_v__, v );
	return V4MergeXY( __vsldoi_v_v_4__, __mergezw___splatw_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,W,X>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeZW( __mergexy_v___splatw_v____, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,Y,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeXY( __mergezw_v___splaty_v____, __splatx_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,Z,W>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeXY( __splatz_v__, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy___splatx_v_____splatz_v____ = V4MergeXY( __splatx_v__, __splatz_v__ );
	return V4MergeZW( v, __mergexy___splatx_v_____splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,X,Y>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeXY( __mergezw_v___splatx_v____, __splaty_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,X,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeXY( __mergezw_v___splatx_v____, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,Y,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __mergexy___splaty_v___v__ = V4MergeXY( __splaty_v__, v );
	return V4MergeXY( __mergezw_v___splaty_v____, __mergexy___splaty_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeXY( __mergezw_v___splaty_v____, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,Z,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	return V4MergeZW( __splatz_v__, __mergexy_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,X,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeXY( __mergezw_v___splatx_v____, __splatz_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,X,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return V4MergeXY( __mergezw_v___splatx_v____, __vsldoi_v_v_8__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,Y,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return V4MergeXY( __mergezw_v___splaty_v____, __vsldoi_v_v_8__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,Y,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return V4MergeXY( __mergezw_v___splaty_v____, __vsldoi_v_v_12__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,Y,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeXY( __mergezw_v___splaty_v____, __splatw_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,W,Z,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeZW( __splatz_v__, __mergezw_v___splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,X,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw___splatx_v___v__ = V4MergeZW( __splatx_v__, v );
	return V4MergeZW( __vsldoi_v_v_4__, __mergezw___splatx_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,Z,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	Vector_4V __splatx_v__ = V4SplatX( v );
	return V4MergeZW( __mergezw_v___splatz_v____, __splatx_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,Z,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	Vector_4V __vsldoi_v_v_8__ = vec_sld( v, v, 8 );
	return V4MergeZW( __mergezw_v___splatz_v____, __vsldoi_v_v_8__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	return V4MergeXY( __splatw_v__, __mergexy_v___splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,X,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	return V4MergeZW( __vsldoi_v_v_4__, __mergexy_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,X,W>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeZW( __vsldoi_v_v_4__, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return V4MergeZW( __mergezw_v___splaty_v____, __vsldoi_v_v_12__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,Z,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	Vector_4V __splaty_v__ = V4SplatY( v );
	return V4MergeZW( __mergezw_v___splatz_v____, __splaty_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,Z,Z>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	Vector_4V __vsldoi_v_v_12__ = vec_sld( v, v, 12 );
	return V4MergeZW( __mergezw_v___splatz_v____, __vsldoi_v_v_12__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,W,X>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	return V4MergeZW( __splatw_v__, __mergexy_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,X,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy___splatz_v___v__ = V4MergeXY( __splatz_v__, v );
	return V4MergeZW( __vsldoi_v_v_4__, __mergexy___splatz_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,Y,Z>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	return V4MergeZW( __mergezw_v___splaty_v____, __splatz_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,Z,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	Vector_4V __mergexy___splatz_v___v__ = V4MergeXY( __splatz_v__, v );
	return V4MergeZW( __mergezw_v___splatz_v____, __mergexy___splatz_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeXY( __splatw_v__, __mergezw_v___splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,X,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	return V4MergeZW( __vsldoi_v_v_4__, __mergezw_v___splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,Y,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeZW( __mergezw_v___splaty_v____, __vsldoi_v_v_4__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,Y,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __splatw_v__ = V4SplatW( v );
	return V4MergeZW( __mergezw_v___splaty_v____, __splatw_v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,Z,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	return V4MergeZW( __mergezw_v___splatz_v____, __vsldoi_v_v_4__ );
}

