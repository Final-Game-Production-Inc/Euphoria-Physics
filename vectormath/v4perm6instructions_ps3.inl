template <>
__forceinline Vector_4V_Out V4Permute<X,W,Z,Y>( Vector_4V_In v )
{
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy_v___splatz_v____ = V4MergeXY( v, __splatz_v__ );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergexy___splatw_v_____splaty_v____ = V4MergeXY( __splatw_v__, __splaty_v__ );
	return V4MergeXY( __mergexy_v___splatz_v____, __mergexy___splatw_v_____splaty_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Y,X,W,Z>( Vector_4V_In v )
{
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy_v___splatw_v____ = V4MergeXY( v, __splatw_v__ );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __mergexy___splatx_v_____splatz_v____ = V4MergeXY( __splatx_v__, __splatz_v__ );
	return V4MergeZW( __mergexy_v___splatw_v____, __mergexy___splatx_v_____splatz_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<Z,Y,X,W>( Vector_4V_In v )
{
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergezw_v___splatx_v____ = V4MergeZW( v, __splatx_v__ );
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __splatw_v__ = V4SplatW( v );
	Vector_4V __mergexy___splaty_v_____splatw_v____ = V4MergeXY( __splaty_v__, __splatw_v__ );
	return V4MergeXY( __mergezw_v___splatx_v____, __mergexy___splaty_v_____splatw_v____ );
}

template <>
__forceinline Vector_4V_Out V4Permute<W,Z,Y,X>( Vector_4V_In v )
{
	Vector_4V __splaty_v__ = V4SplatY( v );
	Vector_4V __mergezw_v___splaty_v____ = V4MergeZW( v, __splaty_v__ );
	Vector_4V __splatz_v__ = V4SplatZ( v );
	Vector_4V __splatx_v__ = V4SplatX( v );
	Vector_4V __mergexy___splatz_v_____splatx_v____ = V4MergeXY( __splatz_v__, __splatx_v__ );
	return V4MergeZW( __mergezw_v___splaty_v____, __mergexy___splatz_v_____splatx_v____ );
}

