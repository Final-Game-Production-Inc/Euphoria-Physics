
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

namespace rage
{
namespace Vec
{

	//============================================================================
	// Standard Algebra

	__forceinline Vector_4V_Out V2Normalize(Vector_4V_In inVector)
	{
		Vector_4V invSqrtector = V2InvMagV( inVector );
		return V4Scale(inVector, invSqrtector);
	}

	__forceinline Vector_4V_Out V2NormalizeSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold)
	{
		Vector_4V dotVector = V2MagSquaredV(inVector);
		Vector_4V invSqrtector = V4InvSqrt( dotVector );
		Vector_4V resultVector = V4Scale(inVector, invSqrtector);

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_GT_THEN_ELSE( dotVector, magSqThreshold, resultVector, errValVect );
	}

	__forceinline Vector_4V_Out V2NormalizeFast(Vector_4V_In inVector)
	{
		Vector_4V invSqrtector = V2InvMagVFast( inVector );
		return V4Scale(inVector, invSqrtector);
	}

	__forceinline Vector_4V_Out V2NormalizeFastSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold)
	{
		Vector_4V dotVector = V2MagSquaredV(inVector);
		Vector_4V invSqrtector = V4InvSqrtFast(dotVector);
		Vector_4V resultVector = V4Scale(inVector, invSqrtector);

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_GT_THEN_ELSE( dotVector, magSqThreshold, resultVector, errValVect );
	}

	__forceinline float V2Dot(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V2DotV(a, b) );
	}

	__forceinline float V2Cross(Vector_4V_In a, Vector_4V_In b)
	{
		Vector_4V vect2 = V4Permute<Y, X, W, W>( b );
		Vector_4V vect3 = V4Scale( a, vect2 );
		Vector_4V vect4 = V4SplatY( vect3 );
		Vector_4V outVect = V4Subtract( vect3, vect4 );
		return GetX( outVect );
	}

	__forceinline Vector_4V_Out V2CrossV(Vector_4V_In a, Vector_4V_In b)
	{
		// We have a:(X,Y,Z,W), b:(A,B,C,D).
		// We want (XB-AY).

		// 2 non-dependent instructions.
		Vector_4V _xxyy = V4MergeXY( a, a );
		Vector_4V _b__a = V4Permute<Y,Z,W,X>( b );

		// 1 instruction.
		Vector_4V _xb__ay = V4Scale( _xxyy, _b__a );

		// 2 non-dependent instructions.
		Vector_4V _xb_xb_xb_xb = V4SplatX( _xb__ay );
		Vector_4V _ay_ay_ay_ay = V4SplatW( _xb__ay );

		// 1 instruction.
		return V4Subtract( _xb_xb_xb_xb, _ay_ay_ay_ay );
	}

	__forceinline Vector_4V_Out V2AngleV(Vector_4V_In v1, Vector_4V_In v2)
	{
		Vector_4V magSquaredProduct = V4Scale( V2MagSquaredV(v1), V2MagSquaredV(v2) );
		return V4Arccos( V4Scale( V2DotV( v1, v2 ), V4InvSqrt(magSquaredProduct) ) );
	}

	__forceinline Vector_4V_Out V2AngleVNormInput(Vector_4V_In v1, Vector_4V_In v2)
	{
		return V4Arccos( V2DotV( v1, v2 ) );
	}

	__forceinline Vector_4V_Out V2WhichSideOfLineV(Vector_4V_In point, Vector_4V_In lineP1, Vector_4V_In lineP2)
	{
		// scalar version:
		// return ( (lineP2.x - lineP1.x)*(point.y - lineP1.y) - (point.x - lineP1.x)*(lineP2.y - lineP1.y) );

		Vector_4V lineDiffs = V4Subtract( lineP2, lineP1 );
		Vector_4V pointLine1Diffs = V4Subtract( point, lineP1 );
		// lineDiffs		= <(lineP2.x - lineP1.x), (lineP2.y - lineP1.y), ..., ...>
		// pointLine1Diffs	= <(point.x - lineP1.x),  (point.y - lineP1.y) , ..., ...>

		pointLine1Diffs = V4Permute<Y,X,Y,X>(pointLine1Diffs);
		Vector_4V product = V4Scale(lineDiffs, pointLine1Diffs);
		Vector_4V product2 = V4SplatY(product);
		// product	=	<X, Y, ..., ...>
		// product2	=	<Y, Y, Y  , Y>

		return V4SplatX( V4Subtract(product, product2) );
	}

	__forceinline Vector_4V_Out V2Rotate(Vector_4V_In inVector, Vector_4V_In radians)
	{
		//float tsin = sinf(radians);
		//float tcos = cosf(radians);
		//float t	= x*tcos - y*tsin;
		//y			= x*tsin + y*tcos;
		//x = t;

		// TODO: This could probably be done faster. That V4PermuteTwo and lack of madd's suck.

		Vector_4V sinResult;
		Vector_4V cosResult;
		V4SinAndCos( sinResult, cosResult, radians );

		cosResult = V4Scale( cosResult, inVector );
		sinResult = V4Scale( sinResult, inVector );

		// sinResult = <x*tsin, y*tsin, ..., ...>
		// cosResult = <x*tcos, y*tcos, ..., ...>

		Vector_4V cosReversed = V4Permute<Y,X,W,W>( cosResult );
		// cosReversed = <y*tcos, x*tcos, ..., ...>

		Vector_4V minus = V4Subtract( cosReversed, sinResult ); // .y has our x result.
		Vector_4V plus = V4Add( cosReversed, sinResult ); // .x has our y result.

		return V4PermuteTwo<Y1,X2,W1,W1>( minus, plus );
	}

	__forceinline Vector_4V_Out V2AddNet( Vector_4V_In inVector, Vector_4V_In toAdd )
	{
		Vector_4V zero = V4VConstant(V_ZERO);
		Vector_4V dot = V2DotV( inVector, toAdd );
		Vector_4V modifiedAdd = V4SubtractScaled( toAdd, inVector, V4Scale( dot, V2InvMagSquaredV( inVector ) ) );
		modifiedAdd = V4SelectFT( V4IsGreaterThanV(dot, zero), zero, modifiedAdd );
		return V4Add( inVector, modifiedAdd );
	}

	__forceinline Vector_4V_Out V2Extend( Vector_4V_In inVector, Vector_4V_In amount )
	{
		Vector_4V invMag = V2InvMagV( inVector );
		Vector_4V scaleValue = V4AddScaled( V4VConstant(V_ONE), invMag, amount );
		return V4Scale( inVector, scaleValue );
	}

	__forceinline Vector_4V_Out V2ApproachStraight(Vector_4V_InOut position, Vector_4V_In goal, Vector_4V_In rate, Vector_4V_In time, unsigned int& rResult)
	{
		Vector_4V directionXY = V4Subtract( goal, position );
		Vector_4V unitDirectionXY = V2Normalize( directionXY );
		Vector_4V scalarDistance = V4Scale(rate, time);
		Vector_4V finalPosition = V4AddScaled( position, unitDirectionXY, scalarDistance );

		Vector_4V directionXYNew = V4Subtract( goal, position );
		Vector_4V unitDirectionXYNew = V3Normalize( directionXYNew );

		unsigned int haventReachedGoal = V2IsCloseAll( unitDirectionXY, unitDirectionXYNew, V4VConstant(V_FLT_EPSILON) );
		rResult = (haventReachedGoal ^ 0x1);

		return finalPosition;
	}

	__forceinline Vector_4V_Out V2Reflect( Vector_4V_In inVector, Vector_4V_In wall2DNormal )
	{
		return V4SubtractScaled( inVector,
								 V4Scale( V4VConstant<0x40000000,0x40000000,0x40000000,0x40000000>(),
								 V2DotV( inVector, wall2DNormal )),
								 wall2DNormal );
	}

	

	//============================================================================
	// Magnitude and distance

	__forceinline float V2Mag( Vector_4V_In v )
	{
		return GetX( V2MagV(v) );
	}

	__forceinline float V2MagFast( Vector_4V_In v )
	{
		return GetX( V2MagVFast(v) );
	}

	__forceinline Vector_4V_Out V2MagV( Vector_4V_In v )
	{
		return V4Sqrt( V2MagSquaredV(v) );
	}

	__forceinline Vector_4V_Out V2MagVFast( Vector_4V_In v )
	{
		return V4SqrtFast( V2MagSquaredV(v) );
	}

	__forceinline float V2MagSquared( Vector_4V_In v )
	{
		return GetX( V2MagSquaredV( v ) );
	}

	__forceinline float V2InvMag( Vector_4V_In v )
	{
		return GetX( V2InvMagV( v ) );
	}

	__forceinline float V2InvMagSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V2InvMagVSafe( v, errValVect ) );
	}

	__forceinline float V2InvMagFast( Vector_4V_In v )
	{
		return GetX( V2InvMagVFast( v ) );
	}

	__forceinline float V2InvMagFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V2InvMagVFastSafe( v, errValVect ) );
	}

	__forceinline Vector_4V_Out V2InvMagV( Vector_4V_In v )
	{
		return V4InvSqrt( V2MagSquaredV(v) );
	}

	__forceinline Vector_4V_Out V2InvMagVSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V magSqVector = V2MagSquaredV( v );
		Vector_4V resultVector = V4InvSqrt( magSqVector );

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_EQ_THEN_ELSE( magSqVector, V4VConstant(V_ZERO), errValVect, resultVector );
	}

	__forceinline Vector_4V_Out V2InvMagVFast( Vector_4V_In v )
	{
		return V4InvSqrtFast( V2MagSquaredV(v) );
	}

	__forceinline Vector_4V_Out V2InvMagVFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V magSqVector = V2MagSquaredV( v );
		Vector_4V resultVector = V4InvSqrtFast( magSqVector );

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_EQ_THEN_ELSE( magSqVector, V4VConstant(V_ZERO), errValVect, resultVector );
	}

	__forceinline float V2InvMagSquared( Vector_4V_In v )
	{
		return GetX( V2InvMagSquaredV( v ) );
	}

	__forceinline float V2InvMagSquaredSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V2InvMagSquaredVSafe( v, errValVect ) );
	}

	__forceinline float V2InvMagSquaredFast( Vector_4V_In v )
	{
		return GetX( V2InvMagSquaredVFast( v ) );
	}

	__forceinline float V2InvMagSquaredFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V2InvMagSquaredVFastSafe( v, errValVect ) );
	}

	__forceinline Vector_4V_Out V2InvMagSquaredV( Vector_4V_In v )
	{
		Vector_4V outVect = V2MagSquaredV( v );
		return V4Invert( outVect );
	}

	__forceinline Vector_4V_Out V2InvMagSquaredVSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V outVect = V2MagSquaredV( v );
		return V4InvertSafe( outVect, errValVect );
	}

	__forceinline Vector_4V_Out V2InvMagSquaredVFast( Vector_4V_In v )
	{
		Vector_4V outVect = V2MagSquaredV( v );
		return V4InvertFast( outVect );
	}

	__forceinline Vector_4V_Out V2InvMagSquaredVFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V outVect = V2MagSquaredV( v );
		return V4InvertFastSafe( outVect, errValVect );
	}

	__forceinline float V2Dist(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V2DistV(a, b) );
	}

	__forceinline float V2DistFast(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V2DistVFast(a, b) );
	}

	__forceinline Vector_4V_Out V2DistV(Vector_4V_In a, Vector_4V_In b)
	{
		return V2MagV( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V2DistVFast(Vector_4V_In a, Vector_4V_In b)
	{
		return V2MagVFast( V4Subtract( a, b ) );
	}

	__forceinline float V2InvDist(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V2InvDistV( a, b ) );
	}

	__forceinline float V2InvDistSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V2InvDistVSafe( a, b, errValVect ) );
	}

	__forceinline float V2InvDistFast(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V2InvDistVFast( a, b ) );
	}

	__forceinline float V2InvDistFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V2InvDistVFastSafe( a, b, errValVect ) );
	}

	__forceinline Vector_4V_Out V2InvDistV(Vector_4V_In a, Vector_4V_In b)
	{
		return V2InvMagV( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V2InvDistVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return V2InvMagVSafe( V4Subtract( a, b ), errValVect );
	}

	__forceinline Vector_4V_Out V2InvDistVFast(Vector_4V_In a, Vector_4V_In b)
	{
		return V2InvMagVFast( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V2InvDistVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return V2InvMagVFastSafe( V4Subtract( a, b ), errValVect );
	}

	__forceinline float V2DistSquared(Vector_4V_In a, Vector_4V_In b)
	{
		return V2MagSquared( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V2DistSquaredV(Vector_4V_In a, Vector_4V_In b)
	{
		return V2MagSquaredV( V4Subtract( a, b ) );
	}

	__forceinline float V2InvDistSquared(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V2InvDistSquaredV( a, b ) );
	}

	__forceinline float V2InvDistSquaredSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V2InvDistSquaredVSafe( a, b, errValVect ) );
	}

	__forceinline float V2InvDistSquaredFast(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V2InvDistSquaredVFast( a, b ) );
	}

	__forceinline float V2InvDistSquaredFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V2InvDistSquaredVFastSafe( a, b, errValVect ) );
	}

	__forceinline Vector_4V_Out V2InvDistSquaredV(Vector_4V_In a, Vector_4V_In b)
	{
		Vector_4V outVector = V2DistSquaredV( a, b );
		return V4Invert( outVector );
	}

	__forceinline Vector_4V_Out V2InvDistSquaredVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		Vector_4V outVector = V2DistSquaredV( a, b );
		return V4InvertSafe( outVector, errValVect );
	}

	__forceinline Vector_4V_Out V2InvDistSquaredVFast(Vector_4V_In a, Vector_4V_In b)
	{
		Vector_4V outVector = V2DistSquaredV( a, b );
		return V4InvertFast( outVector );
	}

	__forceinline Vector_4V_Out V2InvDistSquaredVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		Vector_4V outVector = V2DistSquaredV( a, b );
		return V4InvertFastSafe( outVector, errValVect );
	}

	//============================================================================
	// Comparison functions

	__forceinline unsigned int V2SameSignAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return V2IsGreaterThanOrEqualAll( V4Scale( inVector1, inVector2 ), V4VConstant(V_ZERO) );
	}

	__forceinline unsigned int V2IsBetweenNegAndPosBounds( Vector_4V_In testVector, Vector_4V_In boundsVector )
	{
		Vector_4V testV = V4Permute<X, Y, X, X>( testVector );
		Vector_4V boundsV = V4Permute<X, Y, X, X>( boundsVector );
		return V4IsBetweenNegAndPosBounds( testV, boundsV );
	}

	__forceinline unsigned int V2IsZeroAll(Vector_4V_In inVector)
	{
		return V2IsEqualAll( inVector, V4VConstant(V_ZERO) );
	}

	__forceinline unsigned int V2IsZeroNone(Vector_4V_In inVector)
	{
		return V2IsEqualNone( inVector, V4VConstant(V_ZERO) );
	}

	__forceinline unsigned int V2IsCloseAll(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps)
	{
		Vector_4V minusEpsVect = V4Subtract(inVector1, eps);
		Vector_4V plusEpsVect = V4Add(inVector1, eps);
		Vector_4V testVect1 = V4IsGreaterThanOrEqualV(inVector2, minusEpsVect);
		Vector_4V testVect2 = V4IsLessThanOrEqualV(inVector2, plusEpsVect);
		Vector_4V bothVect = V4And(testVect1, testVect2);
		return V2IsEqualIntAll( bothVect, V4VConstant(V_MASKXYZW) );
	}

	__forceinline unsigned int V2IsCloseNone(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps)
	{
		Vector_4V minusEpsVect = V4Subtract(inVector1, eps);
		Vector_4V plusEpsVect = V4Add(inVector1, eps);
		Vector_4V testVect1 = V4IsGreaterThanOrEqualV(inVector2, minusEpsVect);
		Vector_4V testVect2 = V4IsLessThanOrEqualV(inVector2, plusEpsVect);
		Vector_4V bothVect = V4And(testVect1, testVect2);
		return V2IsEqualIntNone( bothVect, V4VConstant(V_MASKXYZW) );
	}

	__forceinline Vector_4V_Out V2MaxElement(Vector_4V_In inVector)
	{
		Vector_4V x = V4SplatX( inVector );
		Vector_4V y = V4SplatY( inVector );
		return V4Max( x, y );
	}

	__forceinline Vector_4V_Out V2MinElement(Vector_4V_In inVector)
	{
		Vector_4V x = V4SplatX( inVector );
		Vector_4V y = V4SplatY( inVector );
		return V4Min( x, y );
	}

	
} // namespace Vec
} // namespace rage
