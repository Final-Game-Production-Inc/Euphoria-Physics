template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4SplatX( v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeXY( v1, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 6 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 7 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 9 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 10 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 11 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 13 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 14 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 15 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 16 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 17 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 18 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 19 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 20 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 21 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 22 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 23 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 24 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 25 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 26 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 1, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 1, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 1, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 1, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 28 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 29 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 30 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 31 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 2, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 3, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 2, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 3, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 2, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 3, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 2, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 3, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 32 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 33 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 34 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 35 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 36 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 37 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 38 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 39 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 40 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 41 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 42 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 43 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 44 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 45 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 46 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 47 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 48 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 49 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 50 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 51 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 52 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 53 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 54 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 55 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 56 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 57 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 58 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 59 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 60 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 61 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 62 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 63 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeXY( v1, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 4, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 5, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 6, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 7, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 4, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 5, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 6, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Y2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 7, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 4, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 5, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 6, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,Z2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 7, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 4, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 5, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 6, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X1,W2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 7, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 64 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 65 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 66 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 67 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 68 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 69 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 70 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 71 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 72 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 73 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 74 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 75 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 76 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 77 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 78 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,X1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 79 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 80 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 81 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 82 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 83 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 84 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4SplatY( v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 86 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 87 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 88 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 89 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 90 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 91 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 92 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 93 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 94 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 95 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 96 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 97 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 98 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 99 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 100 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 101 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 102 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 103 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 104 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 105 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 106 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 107 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v1, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 109 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 110 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 111 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v1, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 14, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 13, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Z1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 12, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 112 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 113 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 114 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 115 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 116 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 117 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 118 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 119 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 120 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 121 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 122 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 123 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 124 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 125 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 126 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,W1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 127 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 11, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 10, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 9, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y1,Y2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 8, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 128 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 129 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 130 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 131 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 132 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 133 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 134 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 135 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 136 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 137 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 138 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 139 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 140 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 141 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 142 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,X1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 143 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 144 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 145 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 146 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 147 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 148 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 149 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 150 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 151 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 152 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 153 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 154 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 155 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 156 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 157 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 158 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 159 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 160 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 161 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 162 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 163 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 164 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 165 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 166 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 167 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 168 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 169 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4SplatZ( v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 171 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 172 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 173 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 174 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeZW( v1, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 176 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v1, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 178 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 179 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 14, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 180 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 181 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 182 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 183 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 184 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 185 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 186 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 187 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 188 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 189 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 190 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 191 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v1, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 13, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,W1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 12, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 11, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 10, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 9, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Y2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 8, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeZW( v1, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 192 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 193 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 194 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 195 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 196 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 197 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v1, v1, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 199 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 14, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 200 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 201 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 202 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 203 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 204 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 205 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 206 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 207 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 13, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 12, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 208 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 209 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 210 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 211 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 212 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 213 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 214 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 215 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 216 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 217 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 218 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 219 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 220 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 221 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 222 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 223 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 224 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 225 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 226 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 227 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 228 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 229 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 230 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 231 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 232 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 233 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 234 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 235 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 236 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 237 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 238 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Z1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 239 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 240 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 241 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 242 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,X1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 243 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 244 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 245 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 246 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Y1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 247 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 248 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 249 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 250 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 251 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 252 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 253 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v1, 254 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,W1,W1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4SplatW( v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v1, v2, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 11, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 10, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 9, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W1,Y2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 8, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 7, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 6, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeXY( v2, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 5, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 4, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 8, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 9, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 10, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 11, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 7, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 6, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 5, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 4, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 7, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 6, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 5, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W1,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 4, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4SplatX( v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeXY( v2, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 6 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 7 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 9 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 10 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 11 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 13 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 14 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,X2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 15 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 3, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 2, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 3, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 2, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 12, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 13, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W1,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 3, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 2, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 16 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 17 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 18 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 19 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 20 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 21 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 22 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 23 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 1, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 1, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z2,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v2, v1, 1, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 14, 0 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 24 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 25 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 26 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 28 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 29 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 30 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Y2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 31 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 32 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 33 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 34 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 35 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 36 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 37 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 38 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 39 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 40 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 41 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 42 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 43 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 44 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 45 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 46 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,Z2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 47 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 48 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 49 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 50 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 51 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 52 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 53 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 54 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 55 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 56 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 57 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 58 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 59 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 60 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 61 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 62 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<X2,W2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 63 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 8, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 9, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 10, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y1,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 11, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 64 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 65 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 66 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 67 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 68 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 69 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 70 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 71 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 72 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 73 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 74 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 75 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 76 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 77 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 78 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,X2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 79 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 80 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 81 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 82 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 83 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 84 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4SplatY( v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 86 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 87 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 88 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 89 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 90 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 91 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 92 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 93 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 94 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Y2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 95 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 12, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 13, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 96 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 97 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 98 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 99 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 100 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 101 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 102 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 103 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 104 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 105 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 106 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 107 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,X1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v2, v1, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 14, 1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v2, v2, 4 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 109 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 110 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,Z2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 111 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 112 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 113 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 114 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 115 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 116 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 117 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 118 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 119 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 120 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 121 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 122 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 123 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 124 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 125 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 126 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Y2,W2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 127 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 8, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 9, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 10, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 11, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeZW( v2, v1 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 128 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 129 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 130 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 131 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 132 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 133 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 134 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 135 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 136 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 137 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 138 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 139 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 140 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 141 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 142 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,X2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 143 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 144 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 145 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 146 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 147 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 148 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 149 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 150 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 151 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 152 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 153 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 154 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 155 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 156 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 157 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 158 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Y2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 159 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 160 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 161 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 162 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 163 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 164 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 165 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 166 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 167 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 168 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 169 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4SplatZ( v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 171 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 172 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 173 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 174 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4MergeZW( v2, v2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X1,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v2, v1, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 12, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 13, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 14, 2 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 176 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v2, v2, 8 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 178 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 179 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 180 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 181 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 182 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 183 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 184 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 185 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 186 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 187 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 188 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 189 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 190 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<Z2,W2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 191 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X1,Y1,Z1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v2, v1, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 8, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 9, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 10, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y1,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 11, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z1,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 12, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z1,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 13, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 192 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 193 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 194 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 195 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vrlimi( v1, v2, 14, 3 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 196 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 197 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vsldoi( v2, v2, 12 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 199 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 200 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 201 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 202 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 203 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 204 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 205 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 206 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,X2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 207 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 208 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 209 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 210 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 211 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 212 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 213 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 214 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 215 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 216 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 217 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 218 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 219 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 220 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 221 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 222 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Y2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 223 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 224 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 225 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 226 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 227 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 228 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 229 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 230 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 231 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 232 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 233 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 234 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 235 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 236 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 237 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 238 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,Z2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 239 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 240 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 241 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 242 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,X2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 243 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 244 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 245 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 246 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Y2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 247 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 248 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 249 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 250 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,Z2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 251 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W2,X2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 252 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 253 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return __vpermwi( v2, 254 );
}

template <>
__forceinline Vector_4V_Out V4PermuteTwo<W2,W2,W2,W2>( Vector_4V_In v1, Vector_4V_In v2 )
{
	return V4SplatW( v2 );
}

