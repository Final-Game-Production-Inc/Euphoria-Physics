template <>
__forceinline Vector_4V_Out V4Permute<X,X,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	return V4MergeXY( __mergexy_v___splatw_v____, __mergexy_v___splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,Z,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	return V4MergeXY( __mergexy_v___splatz_v____, __mergezw_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,Z,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeXY( __mergexy_v___splatz_v____, __mergezw_v___splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,Z,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeXY( __mergexy_v___splatw_v____, __mergezw_v___splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,Z,Z>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergezw___splatw_v___v__ = V4MergeZW( __splatw_v__, v );
	return V4MergeXY( __mergexy_v___splatz_v____, __mergezw___splatw_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<X,W,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergexy___splatw_v_____splaty_v____ = V4MergeXY( __splatw_v__, __splaty_v__ );
	return V4MergeXY( __mergexy_v___splatw_v____, __mergexy___splatw_v_____splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,X,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy___splatx_v_____splatz_v____ = V4MergeXY( __splatx_v__, __splatz_v__ );
	return V4MergeZW( __mergexy_v___splatx_v____, __mergexy___splatx_v_____splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,W,W>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw___splatx_v___v__ = V4MergeZW( __splatx_v__, v );
	return V4MergeZW( __mergexy_v___splatw_v____, __mergezw___splatx_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,Y,X,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeZW( __mergexy_v___splatx_v____, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,X,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	return V4MergeZW( __mergexy_v___splatx_v____, __mergezw_v___splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,Z,Y>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergexy___splatw_v_____splaty_v____ = V4MergeXY( __splatw_v__, __splaty_v__ );
	return V4MergeXY( __vsldoi_v_v_4__, __mergexy___splatw_v_____splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,W,Y>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeZW( __mergexy_v___splatw_v____, __mergezw_v___splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,W,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	return V4MergeZW( __mergexy_v___splatw_v____, __mergezw_v___splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,X,Z>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	return V4MergeXY( __mergezw_v___splatx_v____, __mergexy_v___splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,X,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeXY( __mergezw_v___splatx_v____, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,X,Y,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeXY( __mergezw_v___splaty_v____, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,X,X>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergexy___splaty_v___v__ = V4MergeXY( __splaty_v__, v );
	return V4MergeXY( __mergezw_v___splatx_v____, __mergexy___splaty_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,Y,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy___splaty_v_____splatw_v____ = V4MergeXY( __splaty_v__, __splatw_v__ );
	return V4MergeXY( __mergezw_v___splaty_v____, __mergexy___splaty_v_____splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Z,Y,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	return V4MergeXY( __mergezw_v___splaty_v____, __mergezw_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,X,X,Z>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy___splatx_v_____splatz_v____ = V4MergeXY( __splatx_v__, __splatz_v__ );
	return V4MergeZW( __vsldoi_v_v_4__, __mergexy___splatx_v_____splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,Y,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	return V4MergeZW( __mergezw_v___splaty_v____, __mergexy_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,Y,W>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	return V4MergeZW( __mergezw_v___splaty_v____, __mergexy_v___splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Y,Z,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy_v___splatx_v____ = V4MergeXY( v, __splatx_v__ );
	return V4MergeZW( __mergezw_v___splatz_v____, __mergexy_v___splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,X,X>( Vector_4V_In v )
{
	Vector_4V __vsldoi_v_v_4__ = vec_sld( v, v, 4 );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy___splatz_v_____splatx_v____ = V4MergeXY( __splatz_v__, __splatx_v__ );
	return V4MergeZW( __vsldoi_v_v_4__, __mergexy___splatz_v_____splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,Y,Y>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy___splatz_v___v__ = V4MergeXY( __splatz_v__, v );
	return V4MergeZW( __mergezw_v___splaty_v____, __mergexy___splatz_v___v__ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,Z,X>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy___splatz_v_____splatx_v____ = V4MergeXY( __splatz_v__, __splatx_v__ );
	return V4MergeZW( __mergezw_v___splatz_v____, __mergexy___splatz_v_____splatx_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,W,Z,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergezw_v___splatz_v____ = V4MergeZW( v, __splatz_v__ );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	return V4MergeZW( __mergezw_v___splatz_v____, __mergezw_v___splaty_v____ );
}

