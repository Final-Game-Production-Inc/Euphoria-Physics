
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

namespace rage
{
namespace Vec
{

	//============================================================================
	// Standard Algebra

	__forceinline Vector_4V_Out V3AddNet( Vector_4V_In inVector, Vector_4V_In toAdd )
	{
		Vector_4V zero = V4VConstant(V_ZERO);
		Vector_4V dot = V3DotV( inVector, toAdd );
		Vector_4V invMagSquared = V3InvMagSquaredV( inVector );
		Vector_4V modifiedAdd = V4SubtractScaled( toAdd, inVector, V4Scale( dot, invMagSquared ) );
		Vector_4V dotIsPositive = V4IsGreaterThanV(dot, zero);
		// Checking if the inverse mag spat out QNANs or not.
		Vector_4V invMagSquaredIsNonNan = V4IsNotNanV(invMagSquared);
		modifiedAdd = V4SelectFT( V4And(dotIsPositive, invMagSquaredIsNonNan), toAdd, modifiedAdd );
		return V4Add( inVector, modifiedAdd );
	}

	__forceinline Vector_4V_Out V3Extend( Vector_4V_In inVector, Vector_4V_In amount )
	{
		Vector_4V invMag = V3InvMagV( inVector );
		Vector_4V scaleValue = V4AddScaled( V4VConstant(V_ONE), invMag, amount );
		return V4Scale( inVector, scaleValue );
	}

	__forceinline Vector_4V_Out V3AngleV(Vector_4V_In v1, Vector_4V_In v2)
	{
		Vector_4V magSquaredProduct = V4Scale( V3MagSquaredV(v1), V3MagSquaredV(v2) );
		return V4Arccos( V4Clamp(V4Scale( V3DotV( v1, v2 ), V4InvSqrt(magSquaredProduct) ), V4VConstant(V_NEGONE), V4VConstant(V_ONE)) );
	}

	__forceinline Vector_4V_Out V3AngleVNormInput(Vector_4V_In v1, Vector_4V_In v2)
	{
		return V4Arccos( V4Clamp(V3DotV( v1, v2 ), V4VConstant(V_NEGONE), V4VConstant(V_ONE)) );
	}

	inline Vector_4V_Out V3AngleXV(Vector_4V_In v1, Vector_4V_In v2)
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _negone = V4VConstant<0xBF800000,0xBF800000,0xBF800000,0xBF800000>();
		Vector_4V _one = V4VConstant(V_ONE);

		Vector_4V permutedV1 = V4Permute<Y,Z,W,W>( v1 );
		Vector_4V permutedV2 = V4Permute<Y,Z,W,W>( v2 );
		Vector_4V magSquaredProduct = V4Scale( V2MagSquaredV(permutedV1), V2MagSquaredV(permutedV2) ); // result only in x,y components, due to V2*V() calls.
		Vector_4V invMag = V4InvSqrt( magSquaredProduct );

		Vector_4V crossX = V3Cross( v1, v2 ); // only X is important.

		Vector_4V multiplier = V4SelectFT( V4IsGreaterThanV( crossX, _zero ), _negone, _one );
		Vector_4V result =  V4Scale( multiplier, V4Arccos( V4Clamp( V4Scale(V2DotV(permutedV1, permutedV2), invMag), _negone, _one ) ) );
		return V4SplatX( result );
	}

	inline Vector_4V_Out V3AngleYV(Vector_4V_In v1, Vector_4V_In v2)
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _negone = V4VConstant<0xBF800000,0xBF800000,0xBF800000,0xBF800000>();
		Vector_4V _one = V4VConstant(V_ONE);

		Vector_4V permutedV1 = V4Permute<X,Z,W,W>( v1 );
		Vector_4V permutedV2 = V4Permute<X,Z,W,W>( v2 );
		Vector_4V magSquaredProduct = V4Scale( V2MagSquaredV(permutedV1), V2MagSquaredV(permutedV2) ); // result only in x,y components, due to V2*V() calls.
		Vector_4V invMag = V4InvSqrt( magSquaredProduct );

		Vector_4V crossY = V3Cross( v1, v2 ); // only Y is important.

		Vector_4V multiplier = V4SelectFT( V4IsGreaterThanV( crossY, _zero ), _negone, _one );
		Vector_4V result = V4Scale( multiplier, V4Arccos( V4Clamp( V4Scale(V2DotV(permutedV1, permutedV2), invMag), _negone, _one ) ) );
		return V4SplatY( result );
	}

	inline Vector_4V_Out V3AngleZV(Vector_4V_In v1, Vector_4V_In v2)
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _negone = V4VConstant<0xBF800000,0xBF800000,0xBF800000,0xBF800000>();
		Vector_4V _one = V4VConstant(V_ONE);

		Vector_4V magSquaredProduct = V4Scale( V2MagSquaredV(v1), V2MagSquaredV(v2) ); // result only in x,y components, due to V2*V() calls.
		Vector_4V invMag = V4InvSqrt( magSquaredProduct );

		Vector_4V crossZ = V4SplatZ( V3Cross( v1, v2 ) ); // only Z is important.

		Vector_4V multiplier = V4SelectFT( V4IsGreaterThanV( crossZ, _zero ), _negone, _one );
		Vector_4V result = V4Scale( multiplier, V4Arccos( V4Clamp( V4Scale(V2DotV(v1, v2), invMag), _negone, _one ) ) );
		return V4SplatX( result );
	}

	inline Vector_4V_Out V3AngleXVNormInput(Vector_4V_In v1, Vector_4V_In v2)
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _negone = V4VConstant<0xBF800000,0xBF800000,0xBF800000,0xBF800000>();
		Vector_4V _one = V4VConstant(V_ONE);

		Vector_4V permutedV1 = V4Permute<Y,Z,W,W>( v1 );
		Vector_4V permutedV2 = V4Permute<Y,Z,W,W>( v2 );

		Vector_4V crossX = V3Cross( v1, v2 ); // only X is important.

		Vector_4V multiplier = V4SelectFT( V4IsGreaterThanV( crossX, _zero ), _negone, _one );
		Vector_4V result = V4Scale( multiplier, V4Arccos( V4Clamp( V2DotV(permutedV1, permutedV2), _negone, _one ) ) );
		return V4SplatX( result );
	}

	inline Vector_4V_Out V3AngleYVNormInput(Vector_4V_In v1, Vector_4V_In v2)
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _negone = V4VConstant<0xBF800000,0xBF800000,0xBF800000,0xBF800000>();
		Vector_4V _one = V4VConstant(V_ONE);

		Vector_4V permutedV1 = V4Permute<X,Z,W,W>( v1 );
		Vector_4V permutedV2 = V4Permute<X,Z,W,W>( v2 );

		Vector_4V crossY = V3Cross( v1, v2 ); // only Y is important.

		Vector_4V multiplier = V4SelectFT( V4IsGreaterThanV( crossY, _zero ), _negone, _one );
		Vector_4V result = V4Scale( multiplier, V4Arccos( V4Clamp( V2DotV(permutedV1, permutedV2), _negone, _one ) ) );
		return V4SplatY( result );
	}

	inline Vector_4V_Out V3AngleZVNormInput(Vector_4V_In v1, Vector_4V_In v2)
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _negone = V4VConstant<0xBF800000,0xBF800000,0xBF800000,0xBF800000>();
		Vector_4V _one = V4VConstant(V_ONE);

		Vector_4V crossZ = V4SplatZ( V3Cross( v1, v2 ) ); // only Z is important.

		Vector_4V multiplier = V4SelectFT( V4IsGreaterThanV( crossZ, _zero ), _negone, _one );
		Vector_4V result = V4Scale( multiplier, V4Arccos( V4Clamp( V2DotV(v1, v2), _negone, _one ) ) );
		return V4SplatX( result );
	}

	inline Vector_4V_Out V3ApproachStraight(Vector_4V_In position, Vector_4V_In goal, Vector_4V_In rate, Vector_4V_In_After3Args time, unsigned int& rResult)
	{
		Vector_4V directionXYZ = V4Subtract( goal, position );
		Vector_4V unitDirectionXYZ = V3Normalize( directionXYZ );
		Vector_4V scalarDistance = V4Scale(rate, time);
		Vector_4V finalPosition = V4AddScaled( position, unitDirectionXYZ, scalarDistance );

		Vector_4V directionXYZNew = V4Subtract( goal, finalPosition );
		Vector_4V unitDirectionXYZNew = V3Normalize( directionXYZNew );

		unsigned int haventReachedGoal = V3IsCloseAll( unitDirectionXYZ, unitDirectionXYZNew, V4VConstant(V_FLT_EPSILON) );
		rResult = (haventReachedGoal ^ 0x1);

		return finalPosition;
	}

	__forceinline void V3MakeOrthonormals(Vector_4V_In inVector, Vector_4V_InOut ortho1, Vector_4V_InOut ortho2)
	{
		mthAssertf( V4IsLessThanOrEqualAll( V4Abs( V4Subtract( V4Abs( V3MagV(inVector) ), V4VConstant(V_ONE)) ), V4VConstant(V_FLT_SMALL_2) ) , "'inVector' needs to be normalized!" );
		Vector_4V absVector = V4Abs( inVector );
		Vector_4V yAbsVector = V4SplatY( absVector );
		Vector_4V xAbsVector = V4SplatX( absVector );
		ortho1 = V4SelectFT(	V4IsLessThanOrEqualV( xAbsVector, yAbsVector ),
							V4Scale( V4Permute<Z,Y,X,Y>(inVector), V4VConstant<U32_NEGONE,0,U32_ONE,0>()),
							V4Scale( V4Permute<X,Z,Y,X>(inVector), V4VConstant<0,U32_ONE,U32_NEGONE,0>())	);
		ortho1 = V3Normalize( ortho1 );
		ortho2 = V3Cross( inVector, ortho1 );

		mthAssertf( V4IsLessThanOrEqualAll( V4Abs( V4Subtract( V4Abs( V3MagV(ortho1) ), V4VConstant(V_ONE)) ), V4VConstant(V_FLT_SMALL_2) ) , "'ortho1' should be normalized by now!" );
		mthAssertf( V4IsLessThanOrEqualAll( V4Abs( V4Subtract( V4Abs( V3MagV(ortho2) ), V4VConstant(V_ONE)) ), V4VConstant(V_FLT_SMALL_2) ) , "'ortho2' should be normalized by now!" );
	}

	__forceinline Vector_4V_Out V3Reflect( Vector_4V_In inVector, Vector_4V_In planeNormal )
	{
		return V4SubtractScaled(	inVector,
									V4Scale( V4VConstant<0x40000000,0x40000000,0x40000000,0x40000000>(),
									V3DotV( inVector, planeNormal )),
									planeNormal );
	}

	inline Vector_4V_Out V3RotateAboutXAxis( Vector_4V_In inVector, Vector_4V_In radians )
	{
		Vector_4V sine, cosine;
		V4SinAndCos( sine, cosine, radians );
		return V3RotateAboutXAxis( inVector, sine, cosine );
	}

	inline Vector_4V_Out V3RotateAboutXAxis( Vector_4V_In inVector, Vector_4V_In sine, Vector_4V_In cosine )
	{
		// scalar version:
		//float t = y * tcos - z * tsin;
		//z = y * tsin + z * tcos;
		//y = t;

		// TODO: This could probably be done faster. That V4PermuteTwo and lack of madd's suck.

		Vector_4V sinResult;
		Vector_4V cosResult;
		cosResult = V4Scale( cosine, inVector );
		sinResult = V4Scale( sine, inVector );

		// sinResult = <..., y*tsin, z*tsin, ...>
		// cosResult = <..., y*tcos, z*tcos, ...>

		Vector_4V cosReversed = V4Permute<W,Z,Y,W>( cosResult );
		// cosReversed = <..., z*tcos, y*tcos, ..., ...>

		Vector_4V minus = V4Subtract( cosReversed, sinResult ); // .z has our y result.
		Vector_4V plus = V4Add( cosReversed, sinResult ); // .y has our z result.

		// Combine minus, plus, and inVector.
		Vector_4V _XxxY = V4MergeXY( inVector, plus );
		return V4PermuteTwo<X1,Z2,W1,W1>( _XxxY, minus );
	}

	inline Vector_4V_Out V3RotateAboutYAxis( Vector_4V_In inVector, Vector_4V_In radians )
	{
		Vector_4V sine, cosine;
		V4SinAndCos( sine, cosine, radians );
		return V3RotateAboutYAxis( inVector, sine, cosine );
	}

	inline Vector_4V_Out V3RotateAboutYAxis( Vector_4V_In inVector, Vector_4V_In sine, Vector_4V_In cosine )
	{
		// scalar version:
		//float t = z * tcos - x * tsin;
		//x = z * tsin + x * tcos;
		//z = t;

		// TODO: This could probably be done faster. That V4PermuteTwo and lack of madd's suck.

		Vector_4V sinResult;
		Vector_4V cosResult;
		cosResult = V4Scale( cosine, inVector );
		sinResult = V4Scale( sine, inVector );

		// sinResult = <x*tsin, ..., z*tsin, ...>
		// cosResult = <x*tcos, ..., z*tcos, ...>

		Vector_4V cosReversed = V4Permute<Z,W,X,W>( cosResult );
		// cosReversed = <z*tcos, ..., x*tcos, ...>

		Vector_4V minus = V4Subtract( cosReversed, sinResult ); // .x has our z result.
		Vector_4V plus = V4Add( cosReversed, sinResult ); // .z has our x result.

		// Combine minus, plus, and inVector.
		Vector_4V _xZYx = V4MergeXY( inVector, minus );
		return V4PermuteTwo<Z2,Z1,Y1,W1>( _xZYx, plus );
	}

	inline Vector_4V_Out V3RotateAboutZAxis( Vector_4V_In inVector, Vector_4V_In radians )
	{
		Vector_4V sine, cosine;
		V4SinAndCos( sine, cosine, radians );
		return V3RotateAboutZAxis( inVector, sine, cosine );
	}

	inline Vector_4V_Out V3RotateAboutZAxis( Vector_4V_In inVector, Vector_4V_In sine, Vector_4V_In cosine )
	{
		// scalar version:
		//float t = x * tcos - y * tsin;
		//y = x * tsin + y * tcos;
		//x = t;

		// TODO: This could probably be done faster. That V4PermuteTwo and lack of madd's suck.

		Vector_4V sinResult;
		Vector_4V cosResult;
		cosResult = V4Scale( cosine, inVector );
		sinResult = V4Scale( sine, inVector );

		// sinResult = <x*tsin, y*tsin, ..., ...>
		// cosResult = <x*tcos, y*tcos, ..., ...>

		Vector_4V cosReversed = V4Permute<Y,X,W,W>( cosResult );
		// cosReversed = <y*tcos, x*tcos, ..., ...>

		Vector_4V minus = V4Subtract( cosReversed, sinResult ); // .y has our x result.
		Vector_4V plus = V4Add( cosReversed, sinResult ); // .x has our y result.

		// Combine minus, plus, and inVector.
		Vector_4V _xYXx = V4MergeXY( minus, plus );
		return V4PermuteTwo<Z1,Y1,Z2,W1>( _xYXx, inVector );
	}

	inline void V3ResultToIndexZYX( s32& outInt, Vector_4V_In maskVector )
	{
		Vector_4V intoLSBs = V4And( maskVector, V4VConstant<1,2,4,0>() );
		Vector_4V intoLSBsX = V4SplatX( intoLSBs );
		Vector_4V intoLSBsY = V4SplatY( intoLSBs );
		Vector_4V intoLSBsZ = V4SplatZ( intoLSBs );
		Vector_4V ORed = V4Or( V4Or( intoLSBsX, intoLSBsY ), intoLSBsZ );

		V4StoreScalar32FromSplatted( outInt, ORed );
	}

	inline void V3ResultToIndexZYX( u32& outInt, Vector_4V_In maskVector )
	{
		Vector_4V intoLSBs = V4And( maskVector, V4VConstant<1,2,4,0>() );
		Vector_4V intoLSBsX = V4SplatX( intoLSBs );
		Vector_4V intoLSBsY = V4SplatY( intoLSBs );
		Vector_4V intoLSBsZ = V4SplatZ( intoLSBs );
		Vector_4V ORed = V4Or( V4Or( intoLSBsX, intoLSBsY ), intoLSBsZ );
		
		V4StoreScalar32FromSplatted( outInt, ORed );
	}

	inline void V3ResultToIndexXYZ( u32& outInt, Vector_4V_In maskVector )
	{
		Vector_4V intoLSBs = V4And( maskVector, V4VConstant<4,2,1,0>() );
		Vector_4V intoLSBsX = V4SplatX( intoLSBs );
		Vector_4V intoLSBsY = V4SplatY( intoLSBs );
		Vector_4V intoLSBsZ = V4SplatZ( intoLSBs );
		Vector_4V ORed = V4Or( V4Or( intoLSBsX, intoLSBsY ), intoLSBsZ );

		V4StoreScalar32FromSplatted( outInt, ORed );
	}

	__forceinline Vector_4V_Out V3Normalize(Vector_4V_In inVector)
	{
		Vector_4V invSqrtector = V3InvMagV( inVector );
		return V4Scale(inVector, invSqrtector);
	}

	__forceinline Vector_4V_Out V3NormalizeSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold)
	{
		Vector_4V dotVector = V3MagSquaredV(inVector);
		Vector_4V invSqrtector = V4InvSqrt( dotVector );
		Vector_4V resultVector = V4Scale(inVector, invSqrtector);

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_GT_THEN_ELSE( dotVector, magSqThreshold, resultVector, errValVect );
	}

	__forceinline Vector_4V_Out V3NormalizeFast(Vector_4V_In inVector)
	{
		Vector_4V invSqrtector = V3InvMagVFast( inVector );
		return V4Scale(inVector, invSqrtector);
	}

	__forceinline Vector_4V_Out V3NormalizeFastSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold)
	{
		Vector_4V dotVector = V3MagSquaredV(inVector);
		Vector_4V invSqrtector = V4InvSqrtFast(dotVector);
		Vector_4V resultVector = V4Scale(inVector, invSqrtector);

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_GT_THEN_ELSE( dotVector, magSqThreshold, resultVector, errValVect );
	}

	__forceinline float V3Dot(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V3DotV(a, b) );
	}

	__forceinline Vector_4V_Out V3Cross(Vector_4V_In a, Vector_4V_In b)
	{
		Vector_4V vect1 = V4Permute<Y, Z, X, W>( a );
		Vector_4V vect2 = V4Permute<Z, X, Y, W>( b );
		Vector_4V vect3 = V4Scale( vect1, vect2 );

		vect1 = V4Permute<Z, X, Y, W>( a );
		vect2 = V4Permute<Y, Z, X, W>( b );

		return V4SubtractScaled( vect3, vect2, vect1 );
	}

	__forceinline Vector_4V_Out V3AddCrossed(Vector_4V_In toAddTo, Vector_4V_In a, Vector_4V_In b)
	{
		Vector_4V vect1 = V4Permute<Y, Z, X, W>( a );
		Vector_4V vect2 = V4Permute<Z, X, Y, W>( b );
		Vector_4V vect3 = V4AddScaled( toAddTo, vect1, vect2 );

		vect1 = V4Permute<Z, X, Y, W>( a );
		vect2 = V4Permute<Y, Z, X, W>( b );

		return V4SubtractScaled( vect3, vect2, vect1 );
	}

	__forceinline Vector_4V_Out V3SubtractCrossed(Vector_4V_In toSubtractFrom, Vector_4V_In a, Vector_4V_In b)
	{
		// Just reverse the 2nd and 3rd inputs to V3AddCrossed(), and we have this ( [A x B] = -[B x A] ).

		Vector_4V vect1 = V4Permute<Y, Z, X, W>( b );
		Vector_4V vect2 = V4Permute<Z, X, Y, W>( a );
		Vector_4V vect3 = V4AddScaled( toSubtractFrom, vect1, vect2 );

		vect1 = V4Permute<Z, X, Y, W>( b );
		vect2 = V4Permute<Y, Z, X, W>( a );

		return V4SubtractScaled( vect3, vect2, vect1 );
	}

	//============================================================================
	// Magnitude and distance

	inline Vector_4V_Out V3ClampMag( Vector_4V_In v, Vector_4V_In minMagSplatted, Vector_4V_In maxMagSplatted )
	{
		Vector_4V result = v;
		Vector_4V maxMag2 = V4Scale( maxMagSplatted, maxMagSplatted );
		Vector_4V minMag2 = V4Scale( minMagSplatted, minMagSplatted );
		Vector_4V mag2 = V3MagSquaredV( result );
		Vector_4V invMag = V4InvSqrt(mag2);
		Vector_4V isGtMax = V4IsGreaterThanV( mag2, maxMag2 );
		Vector_4V isLtMin = V4IsLessThanV( mag2, minMag2 );
		Vector_4V normalizedResult = V4Scale( result, invMag );
		result = V4SelectFT( isGtMax, V4SelectFT( isLtMin, result, V4Scale( minMagSplatted, normalizedResult ) ), V4Scale( maxMagSplatted, normalizedResult ) );
		return result;
	}

	inline Vector_4V_Out V3ClampMagOld( Vector_4V_In v, float fMinMag, float fMaxMag )
	{
		Vector_4V result = v;
		float mag2 = V3MagSquared( result );
		if (mag2 > fMaxMag*fMaxMag)
		{
			// The vector's magnitude is larger than maxMag, so scale it down.
			float fToSplat = fMaxMag*FPInvSqrt(mag2);
			Vector_4V factor;
			V4Set( factor, fToSplat );
			result = V4Scale(result, factor);
		}
		else if (mag2 < fMinMag*fMinMag)
		{
			// The vector's magnitude is smaller than minMag, so scale it up.
			if (mag2 > 1.0e-12f)
			{
				// The vector's magnitude is large enough to tell what its direction is and to scale it up.
				float fToSplat = fMinMag*FPInvSqrt(mag2);
				Vector_4V factor;
				V4Set( factor, fToSplat );
				result = V4Scale(result, factor);
			}
			else
			{
				// The vector is nearly zero, so make it point up with the minimum magnitude.
				Vector_4V unitUp = V4VConstant(V_Y_AXIS_WZERO);
				float fToSplat = fMinMag;
				Vector_4V factor;
				V4Set( factor, fToSplat );
				result = V4Scale(unitUp, factor);
			}
		}
		else if (!FPIsFinite(mag2))
		{
			// The vector's magnitude is not a number, so make it point up with the minimum magnitude.
			Vector_4V unitUp = V4VConstant(V_Y_AXIS_WZERO);
			float fToSplat = fMinMag;
			Vector_4V factor;
			V4Set( factor, fToSplat );
			result = V4Scale(unitUp, factor);
		}
		return result;
	}

	__forceinline float V3Mag( Vector_4V_In v )
	{
		return GetX( V3MagV(v) );
	}

	__forceinline float V3MagFast( Vector_4V_In v )
	{
		return GetX( V3MagVFast(v) );
	}

	__forceinline Vector_4V_Out V3MagV( Vector_4V_In v )
	{
		return V4Sqrt( V3MagSquaredV(v) );
	}

	__forceinline Vector_4V_Out V3MagVFast( Vector_4V_In v )
	{
		return V4SqrtFast( V3MagSquaredV(v) );
	}

	__forceinline Vector_4V_Out V3MagXYV( Vector_4V_In v )
	{
		return V4Sqrt( V3MagXYSquaredV(v) );
	}

	__forceinline Vector_4V_Out V3MagXZV( Vector_4V_In v )
	{
		return V4Sqrt( V3MagXZSquaredV(v) );
	}

	__forceinline Vector_4V_Out V3MagYZV( Vector_4V_In v )
	{
		return V4Sqrt( V3MagYZSquaredV(v) );
	}

	__forceinline Vector_4V_Out V3MagXYVFast( Vector_4V_In v )
	{
		return V4SqrtFast( V3MagXYSquaredV(v) );
	}

	__forceinline Vector_4V_Out V3MagXZVFast( Vector_4V_In v )
	{
		return V4SqrtFast( V3MagXZSquaredV(v) );
	}

	__forceinline Vector_4V_Out V3MagYZVFast( Vector_4V_In v )
	{
		return V4SqrtFast( V3MagYZSquaredV(v) );
	}

	__forceinline Vector_4V_Out V3DistXYV( Vector_4V_In a, Vector_4V_In b )
	{
		return V3MagXYV( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V3DistXZV( Vector_4V_In a, Vector_4V_In b )
	{
		return V3MagXZV( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V3DistYZV( Vector_4V_In a, Vector_4V_In b )
	{
		return V3MagYZV( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V3DistXYVFast( Vector_4V_In a, Vector_4V_In b )
	{
		return V3MagXYVFast( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V3DistXZVFast( Vector_4V_In a, Vector_4V_In b )
	{
		return V3MagXZVFast( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V3DistYZVFast( Vector_4V_In a, Vector_4V_In b )
	{
		return V3MagYZVFast( V4Subtract( a, b ) );
	}

	__forceinline float V3MagSquared( Vector_4V_In v )
	{
		return GetX( V3MagSquaredV( v ) );
	}

	__forceinline float V3InvMag( Vector_4V_In v )
	{
		return GetX( V3InvMagV( v ) );
	}

	__forceinline float V3InvMagSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V3InvMagVSafe( v, errValVect ) );
	}

	__forceinline float V3InvMagFast( Vector_4V_In v )
	{
		return GetX( V3InvMagVFast( v ) );
	}

	__forceinline float V3InvMagFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V3InvMagVFastSafe( v, errValVect ) );
	}

	__forceinline Vector_4V_Out V3InvMagV( Vector_4V_In v )
	{
		return V4InvSqrt( V3MagSquaredV(v) );
	}

	__forceinline Vector_4V_Out V3InvMagVSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V magSqVector = V3MagSquaredV( v );
		Vector_4V resultVector = V4InvSqrt( magSqVector );

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_EQ_THEN_ELSE( magSqVector, V4VConstant(V_ZERO), errValVect, resultVector );
	}

	__forceinline Vector_4V_Out V3InvMagVFast( Vector_4V_In v )
	{
		return V4InvSqrtFast( V3MagSquaredV(v) );
	}

	__forceinline Vector_4V_Out V3InvMagVFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V magSqVector = V3MagSquaredV( v );
		Vector_4V resultVector = V4InvSqrtFast( magSqVector );

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_EQ_THEN_ELSE( magSqVector, V4VConstant(V_ZERO), errValVect, resultVector );
	}

	__forceinline float V3Dist(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V3DistV(a, b) );
	}

	__forceinline float V3DistFast(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V3DistVFast(a, b) );
	}

	__forceinline Vector_4V_Out V3DistV(Vector_4V_In a, Vector_4V_In b)
	{
		return V3MagV( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V3DistVFast(Vector_4V_In a, Vector_4V_In b)
	{
		return V3MagVFast( V4Subtract( a, b ) );
	}

	__forceinline float V3InvDist(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V3InvDistV( a, b ) );
	}

	__forceinline float V3InvDistSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V3InvDistVSafe( a, b, errValVect ) );
	}

	__forceinline float V3InvDistFast(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V3InvDistVFast( a, b ) );
	}

	__forceinline float V3InvDistFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V3InvDistVFastSafe( a, b, errValVect ) );
	}

	__forceinline Vector_4V_Out V3InvDistV(Vector_4V_In a, Vector_4V_In b)
	{
		return V3InvMagV( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V3InvDistVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return V3InvMagVSafe( V4Subtract( a, b ), errValVect );
	}

	__forceinline Vector_4V_Out V3InvDistVFast(Vector_4V_In a, Vector_4V_In b)
	{
		return V3InvMagVFast( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V3InvDistVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return V3InvMagVFastSafe( V4Subtract( a, b ), errValVect );
	}

	__forceinline float V3DistSquared(Vector_4V_In a, Vector_4V_In b)
	{
		return V3MagSquared( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V3DistSquaredV(Vector_4V_In a, Vector_4V_In b)
	{
		return V3MagSquaredV( V4Subtract( a, b ) );
	}

	__forceinline float V3InvMagSquared( Vector_4V_In v )
	{
		return GetX( V3InvMagSquaredV( v ) );
	}

	__forceinline float V3InvMagSquaredSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V3InvMagSquaredVSafe( v, errValVect ) );
	}

	__forceinline float V3InvMagSquaredFast( Vector_4V_In v )
	{
		return GetX( V3InvMagSquaredVFast( v ) );
	}

	__forceinline float V3InvMagSquaredFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V3InvMagSquaredVFastSafe( v, errValVect ) );
	}

	__forceinline Vector_4V_Out V3InvMagSquaredV( Vector_4V_In v )
	{
		Vector_4V outVect = V3MagSquaredV( v );
		return V4Invert( outVect );
	}

	__forceinline Vector_4V_Out V3InvMagSquaredVSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V outVect = V3MagSquaredV( v );
		return V4InvertSafe( outVect, errValVect );
	}

	__forceinline Vector_4V_Out V3InvMagSquaredVFast( Vector_4V_In v )
	{
		Vector_4V outVect = V3MagSquaredV( v );
		return V4InvertFast( outVect );
	}

	__forceinline Vector_4V_Out V3InvMagSquaredVFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V outVect = V3MagSquaredV( v );
		return V4InvertFastSafe( outVect, errValVect );
	}

	__forceinline float V3InvDistSquared(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V3InvDistSquaredV( a, b ) );
	}

	__forceinline float V3InvDistSquaredSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V3InvDistSquaredVSafe( a, b, errValVect ) );
	}

	__forceinline float V3InvDistSquaredFast(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V3InvDistSquaredVFast( a, b ) );
	}

	__forceinline float V3InvDistSquaredFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V3InvDistSquaredVFastSafe( a, b, errValVect ) );
	}

	__forceinline Vector_4V_Out V3InvDistSquaredV(Vector_4V_In a, Vector_4V_In b)
	{
		Vector_4V outVector = V3DistSquaredV( a, b );
		return V4Invert( outVector );
	}

	__forceinline Vector_4V_Out V3InvDistSquaredVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		Vector_4V outVector = V3DistSquaredV( a, b );
		return V4InvertSafe( outVector, errValVect );
	}

	__forceinline Vector_4V_Out V3InvDistSquaredVFast(Vector_4V_In a, Vector_4V_In b)
	{
		Vector_4V outVector = V3DistSquaredV( a, b );
		return V4InvertFast( outVector );
	}

	__forceinline Vector_4V_Out V3InvDistSquaredVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		Vector_4V outVector = V3DistSquaredV( a, b );
		return V4InvertFastSafe( outVector, errValVect );
	}

	//============================================================================
	// Comparison functions

	__forceinline unsigned int V3SameSignAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return V3IsGreaterThanOrEqualAll( V4Scale( inVector1, inVector2 ), V4VConstant(V_ZERO) );
	}

	__forceinline unsigned int V3IsBetweenNegAndPosBounds( Vector_4V_In testVector, Vector_4V_In boundsVector )
	{
		Vector_4V testV = V4Permute<X, Y, Z, X>( testVector );
		Vector_4V boundsV = V4Permute<X, Y, Z, X>( boundsVector );
		return V4IsBetweenNegAndPosBounds( testV, boundsV );
	}

	__forceinline unsigned int V3IsZeroAll(Vector_4V_In inVector)
	{
		return V3IsEqualAll( inVector, V4VConstant(V_ZERO) );
	}

	__forceinline unsigned int V3IsZeroNone(Vector_4V_In inVector)
	{
		return V3IsEqualNone( inVector, V4VConstant(V_ZERO) );
	}

	__forceinline unsigned int V3IsCloseAll(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps)
	{
		Vector_4V minusEpsVect = V4Subtract(inVector1, eps);
		Vector_4V plusEpsVect = V4Add(inVector1, eps);
		Vector_4V testVect1 = V4IsGreaterThanOrEqualV(inVector2, minusEpsVect);
		Vector_4V testVect2 = V4IsLessThanOrEqualV(inVector2, plusEpsVect);
		Vector_4V bothVect = V4And(testVect1, testVect2);
		return V3IsEqualIntAll( bothVect, V4VConstant(V_MASKXYZW) );
	}

	__forceinline unsigned int V3IsCloseNone(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps)
	{
		Vector_4V minusEpsVect = V4Subtract(inVector1, eps);
		Vector_4V plusEpsVect = V4Add(inVector1, eps);
		Vector_4V testVect1 = V4IsGreaterThanOrEqualV(inVector2, minusEpsVect);
		Vector_4V testVect2 = V4IsLessThanOrEqualV(inVector2, plusEpsVect);
		Vector_4V bothVect = V4And(testVect1, testVect2);
		return V3IsEqualIntNone( bothVect, V4VConstant(V_MASKXYZW) );
	}

	__forceinline Vector_4V_Out V3MaxElement(Vector_4V_In inVector)
	{
		Vector_4V x = V4SplatX( inVector );
		Vector_4V y = V4SplatY( inVector );
		Vector_4V z = V4SplatZ( inVector );
		return V4Max( V4Max(x,y), z );
	}

	__forceinline Vector_4V_Out V3MinElement(Vector_4V_In inVector)
	{
		Vector_4V x = V4SplatX( inVector );
		Vector_4V y = V4SplatY( inVector );
		Vector_4V z = V4SplatZ( inVector );
		return V4Min( V4Min(x,y), z );
	}

	//============================================================================
	// Quaternions

	__forceinline Vector_4V_Out V3QuatRotate( Vector_4V_In inVect, Vector_4V_In inQuat )
	{
		Vector_4V _xyz0 = inVect;
		V4SetWZero( _xyz0 );

		Vector_4V quatConjugate = V4QuatConjugate( inQuat );

		// Out = Q * V * Q^-1
		// TODO: (why is my formula reverse of XMMISC.INL's? That is, why are they doing the inverse transform?)
		Vector_4V resultVec_xyz = V4QuatMultiply( inQuat, _xyz0 );
		return V4QuatMultiply( resultVec_xyz, quatConjugate );
	}

	__forceinline Vector_4V_Out V3QuatRotateReverse( Vector_4V_In inVect, Vector_4V_In inQuat )
	{
		Vector_4V _xyz0 = inVect;
		V4SetWZero( _xyz0 );

		Vector_4V quatConjugate = V4QuatConjugate( inQuat );

		// Out = Q^-1 * V * Q
		Vector_4V resultVec_xyz = V4QuatMultiply( _xyz0, inQuat );
		return V4QuatMultiply( quatConjugate, resultVec_xyz );
	}

	
} // namespace Vec
} // namespace rage
