template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v1__SW_v1__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MXY_v1__SW_v1__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v1__SY_v2__ = V4MergeXY( v1, _SY_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MXY_v1__SY_v2__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v1__SY_v2__ = V4MergeXY( v1, _SY_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MXY_v1__SY_v2__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MXY_v1__SZ_v2__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v1__SZ_v2__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MXY_v1__SW_v2__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v1__SW_v2__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v1__SZ_v1__ = V4MergeXY( v1, _SZ_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v1___SY_v1__ = V4MergeXY( _SW_v1_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SZ_v1__, _MXY__SW_v1___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v1__SZ_v1__ = V4MergeXY( v1, _SZ_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v1___SY_v2__ = V4MergeXY( _SW_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v1__SZ_v1__, _MXY__SW_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v1__SZ_v1__ = V4MergeXY( v1, _SZ_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MXY_v1__SZ_v1__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v1__SY_v2__ = V4MergeXY( v1, _SY_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v1___SY_v1__ = V4MergeXY( _SW_v1_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SY_v2__, _MXY__SW_v1___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v1__SY_v2__ = V4MergeXY( v1, _SY_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MXY_v1__SY_v2__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v1___SY_v1__ = V4MergeXY( _SW_v1_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SZ_v2__, _MXY__SW_v1___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v1___SY_v2__ = V4MergeXY( _SW_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v1__SZ_v2__, _MXY__SW_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MXY_v1__SZ_v2__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v1___SY_v1__ = V4MergeXY( _SW_v1_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SW_v2__, _MXY__SW_v1___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v1___SY_v2__ = V4MergeXY( _SW_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v1__SW_v2__, _MXY__SW_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v1__SZ_v1__ = V4MergeXY( v1, _SZ_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SZ_v1__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v1__SZ_v1__ = V4MergeXY( v1, _SZ_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MXY_v1__SZ_v1__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v1__SZ_v1__ = V4MergeXY( v1, _SZ_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MXY_v1__SZ_v1__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SW_v1__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MXY_v1__SW_v1__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SZ_v2__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MXY_v1__SZ_v2__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SW_v2__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MXY_v1__SW_v2__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v1__SZ_v1__ = V4MergeXY( v1, _SZ_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v2___SY_v1__ = V4MergeXY( _SW_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SZ_v1__, _MXY__SW_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v1__SZ_v1__ = V4MergeXY( v1, _SZ_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MXY_v1__SZ_v1__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v1__SZ_v1__ = V4MergeXY( v1, _SZ_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v2___SY_v2__ = V4MergeXY( _SW_v2_, _SY_v2_ );
	return V4MergeXY( _MXY_v1__SZ_v1__, _MXY__SW_v2___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v2___SY_v1__ = V4MergeXY( _SW_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SW_v1__, _MXY__SW_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v2___SY_v2__ = V4MergeXY( _SW_v2_, _SY_v2_ );
	return V4MergeXY( _MXY_v1__SW_v1__, _MXY__SW_v2___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v1__SY_v2__ = V4MergeXY( v1, _SY_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v2___SY_v1__ = V4MergeXY( _SW_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SY_v2__, _MXY__SW_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY_v1__SY_v2__ = V4MergeXY( v1, _SY_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MXY_v1__SY_v2__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v2___SY_v1__ = V4MergeXY( _SW_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v1__SZ_v2__, _MXY__SW_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MXY_v1__SZ_v2__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v2___SY_v2__ = V4MergeXY( _SW_v2_, _SY_v2_ );
	return V4MergeXY( _MXY_v1__SZ_v2__, _MXY__SW_v2___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v1___SZ_v1__ = V4MergeXY( _SX_v1_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SW_v1__, _MXY__SX_v1___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MXY_v1__SW_v1__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v1___SZ_v2__ = V4MergeXY( _SX_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v1__SW_v1__, _MXY__SX_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v1__SX_v2__ = V4MergeXY( v1, _SX_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v1___SZ_v1__ = V4MergeXY( _SX_v1_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SX_v2__, _MXY__SX_v1___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v1__SX_v2__ = V4MergeXY( v1, _SX_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v1___SZ_v2__ = V4MergeXY( _SX_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v1__SX_v2__, _MXY__SX_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v1___SZ_v1__ = V4MergeXY( _SX_v1_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SZ_v2__, _MXY__SX_v1___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MXY_v1__SZ_v2__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v1___SZ_v1__ = V4MergeXY( _SX_v1_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SW_v2__, _MXY__SX_v1___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MXY_v1__SW_v2__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v1___SZ_v2__ = V4MergeXY( _SX_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v1__SW_v2__, _MXY__SX_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v1__SX_v1__ = V4MergeXY( v1, _SX_v1_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SZ_v1___SZ_v2__ = V4MergeXY( _SZ_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v1__SX_v1__, _MXY__SZ_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v1__SX_v2__ = V4MergeXY( v1, _SX_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SZ_v1___SZ_v2__ = V4MergeXY( _SZ_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v1__SX_v2__, _MXY__SZ_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v1___SX_v1__ = V4MergeXY( _SZ_v1_, _SX_v1_ );
	return V4MergeZW( _MXY_v1__SZ_v2__, _MXY__SZ_v1___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v1___SX_v2__ = V4MergeXY( _SZ_v1_, _SX_v2_ );
	return V4MergeZW( _MXY_v1__SZ_v2__, _MXY__SZ_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SZ_v1___SZ_v2__ = V4MergeXY( _SZ_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v1__SW_v2__, _MXY__SZ_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v1__SX_v1__ = V4MergeXY( v1, _SX_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v2___SZ_v1__ = V4MergeXY( _SX_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SX_v1__, _MXY__SX_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v1__SX_v1__ = V4MergeXY( v1, _SX_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v2___SZ_v2__ = V4MergeXY( _SX_v2_, _SZ_v2_ );
	return V4MergeZW( _MXY_v1__SX_v1__, _MXY__SX_v2___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MXY_v1__SW_v1__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v2___SZ_v1__ = V4MergeXY( _SX_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SW_v1__, _MXY__SX_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v2___SZ_v2__ = V4MergeXY( _SX_v2_, _SZ_v2_ );
	return V4MergeZW( _MXY_v1__SW_v1__, _MXY__SX_v2___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MXY_v1__SZ_v2__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v1__SZ_v2__ = V4MergeXY( v1, _SZ_v2_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v2___SZ_v1__ = V4MergeXY( _SX_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SZ_v2__, _MXY__SX_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MXY_v1__SW_v2__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v2___SZ_v1__ = V4MergeXY( _SX_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SW_v2__, _MXY__SX_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v2___SZ_v2__ = V4MergeXY( _SX_v2_, _SZ_v2_ );
	return V4MergeZW( _MXY_v1__SW_v2__, _MXY__SX_v2___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v1__SX_v1__ = V4MergeXY( v1, _SX_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SZ_v2___SZ_v1__ = V4MergeXY( _SZ_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SX_v1__, _MXY__SZ_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v1__SX_v1__ = V4MergeXY( v1, _SX_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v2___SX_v2__ = V4MergeXY( _SZ_v2_, _SX_v2_ );
	return V4MergeZW( _MXY_v1__SX_v1__, _MXY__SZ_v2___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v2___SX_v1__ = V4MergeXY( _SZ_v2_, _SX_v1_ );
	return V4MergeZW( _MXY_v1__SW_v1__, _MXY__SZ_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SZ_v2___SZ_v1__ = V4MergeXY( _SZ_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SW_v1__, _MXY__SZ_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v1__SW_v1__ = V4MergeXY( v1, _SW_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v2___SX_v2__ = V4MergeXY( _SZ_v2_, _SX_v2_ );
	return V4MergeZW( _MXY_v1__SW_v1__, _MXY__SZ_v2___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v1__SX_v2__ = V4MergeXY( v1, _SX_v2_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v2___SX_v1__ = V4MergeXY( _SZ_v2_, _SX_v1_ );
	return V4MergeZW( _MXY_v1__SX_v2__, _MXY__SZ_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v1__SX_v2__ = V4MergeXY( v1, _SX_v2_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SZ_v2___SZ_v1__ = V4MergeXY( _SZ_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SX_v2__, _MXY__SZ_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v1__SW_v2__ = V4MergeXY( v1, _SW_v2_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SZ_v2___SZ_v1__ = V4MergeXY( _SZ_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v1__SW_v2__, _MXY__SZ_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v1__SX_v1__ = V4MergeZW( v1, _SX_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SX_v1__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v1__SX_v1__ = V4MergeZW( v1, _SX_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MZW_v1__SX_v1__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v1__SX_v1__ = V4MergeZW( v1, _SX_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v1__SX_v1__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v1__SX_v2__ = V4MergeZW( v1, _SX_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SX_v2__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v1__SX_v2__ = V4MergeZW( v1, _SX_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MZW_v1__SX_v2__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v1__SX_v2__ = V4MergeZW( v1, _SX_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v1__SX_v2__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SY_v2__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v1__SY_v2__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v1__SW_v2__ = V4MergeZW( v1, _SW_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SW_v2__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v1__SW_v2__ = V4MergeZW( v1, _SW_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MZW_v1__SW_v2__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v1___SY_v2__ = V4MergeXY( _SW_v1_, _SY_v2_ );
	return V4MergeXY( _MZW_v1__SY_v1__, _MXY__SW_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v1__SY_v1__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v1___SY_v1__ = V4MergeXY( _SW_v1_, _SY_v1_ );
	return V4MergeXY( _MZW_v1__SY_v2__, _MXY__SW_v1___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v1__SY_v2__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v1__SW_v2__ = V4MergeZW( v1, _SW_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v1___SY_v1__ = V4MergeXY( _SW_v1_, _SY_v1_ );
	return V4MergeXY( _MZW_v1__SW_v2__, _MXY__SW_v1___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v1__SW_v2__ = V4MergeZW( v1, _SW_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v1___SY_v2__ = V4MergeXY( _SW_v1_, _SY_v2_ );
	return V4MergeXY( _MZW_v1__SW_v2__, _MXY__SW_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v1__SX_v1__ = V4MergeZW( v1, _SX_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MZW_v1__SX_v1__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v1__SX_v1__ = V4MergeZW( v1, _SX_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SX_v1__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v1__SX_v1__ = V4MergeZW( v1, _SX_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MZW_v1__SX_v1__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SY_v1__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MZW_v1__SY_v1__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v1__SX_v2__ = V4MergeZW( v1, _SX_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MZW_v1__SX_v2__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v1__SX_v2__ = V4MergeZW( v1, _SX_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SX_v2__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v1__SX_v2__ = V4MergeZW( v1, _SX_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MZW_v1__SX_v2__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v1__SW_v2__ = V4MergeZW( v1, _SW_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MZW_v1__SW_v2__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MZW_v1__SW_v2__ = V4MergeZW( v1, _SW_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SW_v2__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v1__SX_v1__ = V4MergeZW( v1, _SX_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SX_v1__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v1__SX_v1__ = V4MergeZW( v1, _SX_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v2___SY_v2__ = V4MergeXY( _SW_v2_, _SY_v2_ );
	return V4MergeXY( _MZW_v1__SX_v1__, _MXY__SW_v2___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SY_v1__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v2___SY_v2__ = V4MergeXY( _SW_v2_, _SY_v2_ );
	return V4MergeXY( _MZW_v1__SY_v1__, _MXY__SW_v2___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v1__SX_v2__ = V4MergeZW( v1, _SX_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v2___SY_v1__ = V4MergeXY( _SW_v2_, _SY_v1_ );
	return V4MergeXY( _MZW_v1__SX_v2__, _MXY__SW_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v1__SX_v2__ = V4MergeZW( v1, _SX_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SX_v2__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v2___SY_v1__ = V4MergeXY( _SW_v2_, _SY_v1_ );
	return V4MergeXY( _MZW_v1__SY_v2__, _MXY__SW_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v1__SY_v2__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v1__SZ_v1__ = V4MergeZW( v1, _SZ_v1_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v1__SZ_v1__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v1___SZ_v1__ = V4MergeXY( _SX_v1_, _SZ_v1_ );
	return V4MergeZW( _MZW_v1__SY_v2__, _MXY__SX_v1___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v1__SY_v2__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v1__SZ_v2__ = V4MergeZW( v1, _SZ_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v1__SZ_v2__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v1___SX_v1__ = V4MergeXY( _SZ_v1_, _SX_v1_ );
	return V4MergeZW( _MZW_v1__SY_v1__, _MXY__SZ_v1___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v1___SX_v2__ = V4MergeXY( _SZ_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v1__SY_v1__, _MXY__SZ_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SZ_v1___SZ_v2__ = V4MergeXY( _SZ_v1_, _SZ_v2_ );
	return V4MergeZW( _MZW_v1__SY_v1__, _MXY__SZ_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v1___SX_v1__ = V4MergeXY( _SZ_v1_, _SX_v1_ );
	return V4MergeZW( _MZW_v1__SY_v2__, _MXY__SZ_v1___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v1___SX_v2__ = V4MergeXY( _SZ_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v1__SY_v2__, _MXY__SZ_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SZ_v1___SZ_v2__ = V4MergeXY( _SZ_v1_, _SZ_v2_ );
	return V4MergeZW( _MZW_v1__SY_v2__, _MXY__SZ_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v1__SZ_v2__ = V4MergeZW( v1, _SZ_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v1___SX_v1__ = V4MergeXY( _SZ_v1_, _SX_v1_ );
	return V4MergeZW( _MZW_v1__SZ_v2__, _MXY__SZ_v1___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v1__SZ_v2__ = V4MergeZW( v1, _SZ_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v1___SX_v2__ = V4MergeXY( _SZ_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v1__SZ_v2__, _MXY__SZ_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v1__SY_v1__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v2___SZ_v2__ = V4MergeXY( _SX_v2_, _SZ_v2_ );
	return V4MergeZW( _MZW_v1__SY_v1__, _MXY__SX_v2___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v1__SZ_v1__ = V4MergeZW( v1, _SZ_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v1__SZ_v1__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v1__SZ_v2__ = V4MergeZW( v1, _SZ_v2_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v1__SZ_v2__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v2___SX_v1__ = V4MergeXY( _SZ_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v1__SY_v1__, _MXY__SZ_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SZ_v2___SZ_v1__ = V4MergeXY( _SZ_v2_, _SZ_v1_ );
	return V4MergeZW( _MZW_v1__SY_v1__, _MXY__SZ_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v1__SY_v1__ = V4MergeZW( v1, _SY_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v2___SX_v2__ = V4MergeXY( _SZ_v2_, _SX_v2_ );
	return V4MergeZW( _MZW_v1__SY_v1__, _MXY__SZ_v2___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v1__SZ_v1__ = V4MergeZW( v1, _SZ_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v2___SX_v1__ = V4MergeXY( _SZ_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v1__SZ_v1__, _MXY__SZ_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v1__SZ_v1__ = V4MergeZW( v1, _SZ_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v2___SX_v2__ = V4MergeXY( _SZ_v2_, _SX_v2_ );
	return V4MergeZW( _MZW_v1__SZ_v1__, _MXY__SZ_v2___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v2___SX_v1__ = V4MergeXY( _SZ_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v1__SY_v2__, _MXY__SZ_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SZ_v2___SZ_v1__ = V4MergeXY( _SZ_v2_, _SZ_v1_ );
	return V4MergeZW( _MZW_v1__SY_v2__, _MXY__SZ_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v1__SY_v2__ = V4MergeZW( v1, _SY_v2_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v2___SX_v2__ = V4MergeXY( _SZ_v2_, _SX_v2_ );
	return V4MergeZW( _MZW_v1__SY_v2__, _MXY__SZ_v2___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SZ_v1__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MXY_v2__SZ_v1__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SW_v1__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MXY_v2__SW_v1__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v2__SZ_v2__ = V4MergeXY( v2, _SZ_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MXY_v2__SZ_v2__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v2__SZ_v2__ = V4MergeXY( v2, _SZ_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SZ_v2__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v2__SZ_v2__ = V4MergeXY( v2, _SZ_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MXY_v2__SZ_v2__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MXY_v2__SW_v2__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SW_v2__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v2__SY_v1__ = V4MergeXY( v2, _SY_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v1___SY_v2__ = V4MergeXY( _SW_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SY_v1__, _MXY__SW_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v2__SY_v1__ = V4MergeXY( v2, _SY_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MXY_v2__SY_v1__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v1___SY_v1__ = V4MergeXY( _SW_v1_, _SY_v1_ );
	return V4MergeXY( _MXY_v2__SZ_v1__, _MXY__SW_v1___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v1___SY_v2__ = V4MergeXY( _SW_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SZ_v1__, _MXY__SW_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MXY_v2__SZ_v1__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v2__SZ_v2__ = V4MergeXY( v2, _SZ_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v1___SY_v1__ = V4MergeXY( _SW_v1_, _SY_v1_ );
	return V4MergeXY( _MXY_v2__SZ_v2__, _MXY__SW_v1___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v2__SZ_v2__ = V4MergeXY( v2, _SZ_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v1___SY_v2__ = V4MergeXY( _SW_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SZ_v2__, _MXY__SW_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v2__SZ_v2__ = V4MergeXY( v2, _SZ_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MXY_v2__SZ_v2__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v1___SY_v1__ = V4MergeXY( _SW_v1_, _SY_v1_ );
	return V4MergeXY( _MXY_v2__SW_v2__, _MXY__SW_v1___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v1___SY_v2__ = V4MergeXY( _SW_v1_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SW_v2__, _MXY__SW_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v2__SY_v1__ = V4MergeXY( v2, _SY_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MXY_v2__SY_v1__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v2__SY_v1__ = V4MergeXY( v2, _SY_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MXY_v2__SY_v1__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v2__SZ_v1__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MXY_v2__SZ_v1__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v2__SW_v1__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MXY_v2__SW_v1__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v2__SW_v2__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MXY_v2__SW_v2__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v2__SY_v1__ = V4MergeXY( v2, _SY_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MXY_v2__SY_v1__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY_v2__SY_v1__ = V4MergeXY( v2, _SY_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v2___SY_v2__ = V4MergeXY( _SW_v2_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SY_v1__, _MXY__SW_v2___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v2___SY_v1__ = V4MergeXY( _SW_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v2__SZ_v1__, _MXY__SW_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MXY_v2__SZ_v1__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v2___SY_v2__ = V4MergeXY( _SW_v2_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SZ_v1__, _MXY__SW_v2___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v2___SY_v1__ = V4MergeXY( _SW_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v2__SW_v1__, _MXY__SW_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v2___SY_v2__ = V4MergeXY( _SW_v2_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SW_v1__, _MXY__SW_v2___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v2__SZ_v2__ = V4MergeXY( v2, _SZ_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v2___SY_v1__ = V4MergeXY( _SW_v2_, _SY_v1_ );
	return V4MergeXY( _MXY_v2__SZ_v2__, _MXY__SW_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v2__SZ_v2__ = V4MergeXY( v2, _SZ_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MXY_v2__SZ_v2__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY_v2__SZ_v2__ = V4MergeXY( v2, _SZ_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v2___SY_v2__ = V4MergeXY( _SW_v2_, _SY_v2_ );
	return V4MergeXY( _MXY_v2__SZ_v2__, _MXY__SW_v2___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MXY_v2__SZ_v1__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v1___SZ_v2__ = V4MergeXY( _SX_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SZ_v1__, _MXY__SX_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v1___SZ_v1__ = V4MergeXY( _SX_v1_, _SZ_v1_ );
	return V4MergeZW( _MXY_v2__SW_v1__, _MXY__SX_v1___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MXY_v2__SW_v1__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v1___SZ_v2__ = V4MergeXY( _SX_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SW_v1__, _MXY__SX_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v2__SX_v2__ = V4MergeXY( v2, _SX_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v1___SZ_v1__ = V4MergeXY( _SX_v1_, _SZ_v1_ );
	return V4MergeZW( _MXY_v2__SX_v2__, _MXY__SX_v1___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v2__SX_v2__ = V4MergeXY( v2, _SX_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v1___SZ_v2__ = V4MergeXY( _SX_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SX_v2__, _MXY__SX_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v1___SZ_v1__ = V4MergeXY( _SX_v1_, _SZ_v1_ );
	return V4MergeZW( _MXY_v2__SW_v2__, _MXY__SX_v1___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MXY_v2__SW_v2__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v1___SZ_v2__ = V4MergeXY( _SX_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SW_v2__, _MXY__SX_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v2__SX_v1__ = V4MergeXY( v2, _SX_v1_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v1___SX_v2__ = V4MergeXY( _SZ_v1_, _SX_v2_ );
	return V4MergeZW( _MXY_v2__SX_v1__, _MXY__SZ_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v2__SX_v1__ = V4MergeXY( v2, _SX_v1_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SZ_v1___SZ_v2__ = V4MergeXY( _SZ_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SX_v1__, _MXY__SZ_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SZ_v1___SZ_v2__ = V4MergeXY( _SZ_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SW_v1__, _MXY__SZ_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v2__SX_v2__ = V4MergeXY( v2, _SX_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v1___SX_v1__ = V4MergeXY( _SZ_v1_, _SX_v1_ );
	return V4MergeZW( _MXY_v2__SX_v2__, _MXY__SZ_v1___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v2__SX_v2__ = V4MergeXY( v2, _SX_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SZ_v1___SZ_v2__ = V4MergeXY( _SZ_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SX_v2__, _MXY__SZ_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v1___SX_v1__ = V4MergeXY( _SZ_v1_, _SX_v1_ );
	return V4MergeZW( _MXY_v2__SW_v2__, _MXY__SZ_v1___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v1___SX_v2__ = V4MergeXY( _SZ_v1_, _SX_v2_ );
	return V4MergeZW( _MXY_v2__SW_v2__, _MXY__SZ_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z1,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SZ_v1___SZ_v2__ = V4MergeXY( _SZ_v1_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SW_v2__, _MXY__SZ_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v2__SX_v1__ = V4MergeXY( v2, _SX_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v2___SZ_v1__ = V4MergeXY( _SX_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v2__SX_v1__, _MXY__SX_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v2__SX_v1__ = V4MergeXY( v2, _SX_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v2___SZ_v2__ = V4MergeXY( _SX_v2_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SX_v1__, _MXY__SX_v2___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MXY_v2__SZ_v1__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v2___SZ_v2__ = V4MergeXY( _SX_v2_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SZ_v1__, _MXY__SX_v2___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MXY_v2__SW_v1__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v2___SZ_v1__ = V4MergeXY( _SX_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v2__SW_v1__, _MXY__SX_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v2___SZ_v2__ = V4MergeXY( _SX_v2_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SW_v1__, _MXY__SX_v2___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MXY_v2__SW_v2__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v2___SZ_v1__ = V4MergeXY( _SX_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v2__SW_v2__, _MXY__SX_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY_v2__SW_v2__ = V4MergeXY( v2, _SW_v2_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v2___SZ_v2__ = V4MergeXY( _SX_v2_, _SZ_v2_ );
	return V4MergeZW( _MXY_v2__SW_v2__, _MXY__SX_v2___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY_v2__SX_v1__ = V4MergeXY( v2, _SX_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SZ_v2___SZ_v1__ = V4MergeXY( _SZ_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v2__SX_v1__, _MXY__SZ_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v2___SX_v1__ = V4MergeXY( _SZ_v2_, _SX_v1_ );
	return V4MergeZW( _MXY_v2__SZ_v1__, _MXY__SZ_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY_v2__SZ_v1__ = V4MergeXY( v2, _SZ_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v2___SX_v2__ = V4MergeXY( _SZ_v2_, _SX_v2_ );
	return V4MergeZW( _MXY_v2__SZ_v1__, _MXY__SZ_v2___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY_v2__SW_v1__ = V4MergeXY( v2, _SW_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SZ_v2___SZ_v1__ = V4MergeXY( _SZ_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v2__SW_v1__, _MXY__SZ_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY_v2__SX_v2__ = V4MergeXY( v2, _SX_v2_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SZ_v2___SZ_v1__ = V4MergeXY( _SZ_v2_, _SZ_v1_ );
	return V4MergeZW( _MXY_v2__SX_v2__, _MXY__SZ_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v2__SX_v1__ = V4MergeZW( v2, _SX_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MZW_v2__SX_v1__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v2__SX_v1__ = V4MergeZW( v2, _SX_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MZW_v2__SX_v1__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v2__SX_v1__ = V4MergeZW( v2, _SX_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SX_v1__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v2__SW_v1__ = V4MergeZW( v2, _SW_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MZW_v2__SW_v1__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v2__SW_v1__ = V4MergeZW( v2, _SW_v1_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SW_v1__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v2__SX_v2__ = V4MergeZW( v2, _SX_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MZW_v2__SX_v2__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v2__SX_v2__ = V4MergeZW( v2, _SX_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SY_v1___SY_v2__ = V4MergeXY( _SY_v1_, _SY_v2_ );
	return V4MergeXY( _MZW_v2__SX_v2__, _MXY__SY_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v2__SX_v2__ = V4MergeZW( v2, _SX_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SX_v2__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v1___SW_v1__ = V4MergeXY( _SY_v1_, _SW_v1_ );
	return V4MergeXY( _MZW_v2__SY_v2__, _MXY__SY_v1___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v1___SW_v2__ = V4MergeXY( _SY_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SY_v2__, _MXY__SY_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v2__SX_v1__ = V4MergeZW( v2, _SX_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v1___SY_v2__ = V4MergeXY( _SW_v1_, _SY_v2_ );
	return V4MergeXY( _MZW_v2__SX_v1__, _MXY__SW_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v2__SX_v1__ = V4MergeZW( v2, _SX_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SX_v1__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v1___SY_v2__ = V4MergeXY( _SW_v1_, _SY_v2_ );
	return V4MergeXY( _MZW_v2__SY_v1__, _MXY__SW_v1___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SY_v1__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v2__SX_v2__ = V4MergeZW( v2, _SX_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v1___SY_v1__ = V4MergeXY( _SW_v1_, _SY_v1_ );
	return V4MergeXY( _MZW_v2__SX_v2__, _MXY__SW_v1___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v2__SX_v2__ = V4MergeZW( v2, _SX_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SX_v2__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v1___SY_v1__ = V4MergeXY( _SW_v1_, _SY_v1_ );
	return V4MergeXY( _MZW_v2__SY_v2__, _MXY__SW_v1___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W1,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SW_v1___SW_v2__ = V4MergeXY( _SW_v1_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SY_v2__, _MXY__SW_v1___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v2__SX_v1__ = V4MergeZW( v2, _SX_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MZW_v2__SX_v1__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v2__SX_v1__ = V4MergeZW( v2, _SX_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v2__SX_v1__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MZW_v2__SX_v1__ = V4MergeZW( v2, _SX_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SX_v1__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v2__SY_v1__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SY_v1__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v2__SW_v1__ = V4MergeZW( v2, _SW_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MZW_v2__SW_v1__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v2__SW_v1__ = V4MergeZW( v2, _SW_v1_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SW_v1__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v2__SX_v2__ = V4MergeZW( v2, _SX_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SY_v2___SY_v1__ = V4MergeXY( _SY_v2_, _SY_v1_ );
	return V4MergeXY( _MZW_v2__SX_v2__, _MXY__SY_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v2__SX_v2__ = V4MergeZW( v2, _SX_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SY_v2___SW_v1__ = V4MergeXY( _SY_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v2__SX_v2__, _MXY__SY_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MZW_v2__SX_v2__ = V4MergeZW( v2, _SX_v2_ );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _MXY__SY_v2___SW_v2__ = V4MergeXY( _SY_v2_, _SW_v2_ );
	return V4MergeXY( _MZW_v2__SX_v2__, _MXY__SY_v2___SW_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v2__SY_v1__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v2___SY_v2__ = V4MergeXY( _SW_v2_, _SY_v2_ );
	return V4MergeXY( _MZW_v2__SY_v1__, _MXY__SW_v2___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v2__SW_v1__ = V4MergeZW( v2, _SW_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v2___SY_v1__ = V4MergeXY( _SW_v2_, _SY_v1_ );
	return V4MergeXY( _MZW_v2__SW_v1__, _MXY__SW_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MZW_v2__SW_v1__ = V4MergeZW( v2, _SW_v1_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MXY__SW_v2___SY_v2__ = V4MergeXY( _SW_v2_, _SY_v2_ );
	return V4MergeXY( _MZW_v2__SW_v1__, _MXY__SW_v2___SY_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MXY__SW_v2___SY_v1__ = V4MergeXY( _SW_v2_, _SY_v1_ );
	return V4MergeXY( _MZW_v2__SY_v2__, _MXY__SW_v2___SY_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SW_v2_ = V4SplatW( v2 );
	Vector_4V _SW_v1_ = V4SplatW( v1 );
	Vector_4V _MXY__SW_v2___SW_v1__ = V4MergeXY( _SW_v2_, _SW_v1_ );
	return V4MergeXY( _MZW_v2__SY_v2__, _MXY__SW_v2___SW_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v2__SZ_v1__ = V4MergeZW( v2, _SZ_v1_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v2__SZ_v1__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SX_v1___SZ_v1__ = V4MergeXY( _SX_v1_, _SZ_v1_ );
	return V4MergeZW( _MZW_v2__SY_v2__, _MXY__SX_v1___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v2__SY_v2__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v2__SZ_v2__ = V4MergeZW( v2, _SZ_v2_ );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SX_v1___SX_v2__ = V4MergeXY( _SX_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v2__SZ_v2__, _MXY__SX_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v1___SX_v1__ = V4MergeXY( _SZ_v1_, _SX_v1_ );
	return V4MergeZW( _MZW_v2__SY_v1__, _MXY__SZ_v1___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v1___SX_v2__ = V4MergeXY( _SZ_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v2__SY_v1__, _MXY__SZ_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SZ_v1___SZ_v2__ = V4MergeXY( _SZ_v1_, _SZ_v2_ );
	return V4MergeZW( _MZW_v2__SY_v1__, _MXY__SZ_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v1___SX_v1__ = V4MergeXY( _SZ_v1_, _SX_v1_ );
	return V4MergeZW( _MZW_v2__SY_v2__, _MXY__SZ_v1___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v1___SX_v2__ = V4MergeXY( _SZ_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v2__SY_v2__, _MXY__SZ_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SZ_v1___SZ_v2__ = V4MergeXY( _SZ_v1_, _SZ_v2_ );
	return V4MergeZW( _MZW_v2__SY_v2__, _MXY__SZ_v1___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v2__SZ_v2__ = V4MergeZW( v2, _SZ_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v1___SX_v1__ = V4MergeXY( _SZ_v1_, _SX_v1_ );
	return V4MergeZW( _MZW_v2__SZ_v2__, _MXY__SZ_v1___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z1,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v2__SZ_v2__ = V4MergeZW( v2, _SZ_v2_ );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v1___SX_v2__ = V4MergeXY( _SZ_v1_, _SX_v2_ );
	return V4MergeZW( _MZW_v2__SZ_v2__, _MXY__SZ_v1___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v2__SY_v1__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MXY__SX_v2___SZ_v2__ = V4MergeXY( _SX_v2_, _SZ_v2_ );
	return V4MergeZW( _MZW_v2__SY_v1__, _MXY__SX_v2___SZ_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v2__SZ_v1__ = V4MergeZW( v2, _SZ_v1_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v2__SZ_v1__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _MZW_v2__SZ_v2__ = V4MergeZW( v2, _SZ_v2_ );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SX_v2___SX_v1__ = V4MergeXY( _SX_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v2__SZ_v2__, _MXY__SX_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v2___SX_v1__ = V4MergeXY( _SZ_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v2__SY_v1__, _MXY__SZ_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SZ_v2___SZ_v1__ = V4MergeXY( _SZ_v2_, _SZ_v1_ );
	return V4MergeZW( _MZW_v2__SY_v1__, _MXY__SZ_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v1_ = V4SplatY( v1 );
	Vector_4V _MZW_v2__SY_v1__ = V4MergeZW( v2, _SY_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v2___SX_v2__ = V4MergeXY( _SZ_v2_, _SX_v2_ );
	return V4MergeZW( _MZW_v2__SY_v1__, _MXY__SZ_v2___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v2__SZ_v1__ = V4MergeZW( v2, _SZ_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v2___SX_v1__ = V4MergeXY( _SZ_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v2__SZ_v1__, _MXY__SZ_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MZW_v2__SZ_v1__ = V4MergeZW( v2, _SZ_v1_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v2___SX_v2__ = V4MergeXY( _SZ_v2_, _SX_v2_ );
	return V4MergeZW( _MZW_v2__SZ_v1__, _MXY__SZ_v2___SX_v2__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v1_ = V4SplatX( v1 );
	Vector_4V _MXY__SZ_v2___SX_v1__ = V4MergeXY( _SZ_v2_, _SX_v1_ );
	return V4MergeZW( _MZW_v2__SY_v2__, _MXY__SZ_v2___SX_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SZ_v1_ = V4SplatZ( v1 );
	Vector_4V _MXY__SZ_v2___SZ_v1__ = V4MergeXY( _SZ_v2_, _SZ_v1_ );
	return V4MergeZW( _MZW_v2__SY_v2__, _MXY__SZ_v2___SZ_v1__ );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	Vector_4V _SY_v2_ = V4SplatY( v2 );
	Vector_4V _MZW_v2__SY_v2__ = V4MergeZW( v2, _SY_v2_ );
	Vector_4V _SZ_v2_ = V4SplatZ( v2 );
	Vector_4V _SX_v2_ = V4SplatX( v2 );
	Vector_4V _MXY__SZ_v2___SX_v2__ = V4MergeXY( _SZ_v2_, _SX_v2_ );
	return V4MergeZW( _MZW_v2__SY_v2__, _MXY__SZ_v2___SX_v2__ );
}

