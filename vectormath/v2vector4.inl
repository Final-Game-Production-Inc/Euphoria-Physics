
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

namespace rage
{
namespace Vec
{

#ifndef V2SET_2F__4
	__forceinline void V2Set( Vector_4_InOut inoutVector, float x0, float y0 )
	{
		inoutVector.x = x0;
		inoutVector.y = y0;
	}
#endif // V2SET_2F__4

#ifndef V2SET_V__4
	__forceinline void V2Set( Vector_4_InOut inoutVector, Vector_4_In inVector )
	{
		inoutVector.x = inVector.x;
		inoutVector.y = inVector.y;
	}
#endif // V2SET_V__4

#ifndef V2SET__4
	__forceinline void V2Set( Vector_4_InOut inoutVector, float s )
	{
		inoutVector.x = s;
		inoutVector.y = s;
	}
#endif // V2SET__4

#ifndef V2ZEROCOMPONENTS__4
	__forceinline void V2ZeroComponents( Vector_4_InOut inoutVector )
	{
		inoutVector.x = inoutVector.y = 0.0f;
	}
#endif // V2ZEROCOMPONENTS__4

	//============================================================================
	// Standard Algebra

#ifndef V2ANGLEV__4
	__forceinline Vector_4_Out V2AngleV(Vector_4_In v1, Vector_4_In v2)
	{
		float magSquaredProduct = V2MagSquared(v1) * V2MagSquared(v2);
		float invSqrt = FPInvSqrt( magSquaredProduct );
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = FPACos( V2Dot( v1, v2 ) * invSqrt );
		return outVect;
	}
#endif // V2ANGLEV__4

#ifndef V2ANGLEVNORMINPUT__4
	__forceinline Vector_4_Out V2AngleVNormInput(Vector_4_In v1, Vector_4_In v2)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = FPACos( V2Dot( v1, v2 ) );
		return outVect;
	}
#endif // V2ANGLEVNORMINPUT__4

#ifndef V2WHICHSIDEOFLINEV__4
	__forceinline Vector_4_Out V2WhichSideOfLineV(Vector_4_In point, Vector_4_In lineP1, Vector_4_In lineP2)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = 
			( (lineP2.x - lineP1.x)*(point.y - lineP1.y)
			-
			(point.x - lineP1.x)*(lineP2.y - lineP1.y) );
		return outVect;
	}
#endif // V2WHICHSIDEOFLINEV__4

#ifndef V2ROTATE__4
	__forceinline Vector_4_Out V2Rotate(Vector_4_In inVector, float radians)
	{
		float tsin, tcos;
		FPSinAndCos(tsin, tcos, radians);
		Vector_4_Out outVect;
		outVect.x = inVector.x*tcos - inVector.y*tsin;
		outVect.y = inVector.x*tsin + inVector.y*tcos;
		return outVect;
	}
#endif // V2ROTATE__4

#ifndef V2ADDNET__4
	__forceinline Vector_4_Out V2AddNet( Vector_4_In inVector, Vector_4_In toAdd )
	{
		Vector_4 modifiedAdd;
		float dot = V2Dot(inVector, toAdd);
		if( dot > 0.0f )
		{
			// The given vector has a positive dot product with this vector, so remove its parallel component.
			modifiedAdd = V2SubtractScaled( toAdd, inVector, dot*V2InvMagSquared( inVector ) );
		}

		// Add the given vector to this vector, less any positive parallel component.
		return V2Add( inVector, modifiedAdd );
	}
#endif // V2ADDNET__4

#ifndef V2EXTEND__4
	__forceinline Vector_4_Out V2Extend( Vector_4_In inVector, Vector_4_In amount )
	{
		float invMag = V2InvMag( inVector );
		Vector_4 scaleValue;
		scaleValue = V2AddScaled( V4Constant(V_ONE), amount, invMag );
		return V2Scale( inVector, scaleValue );
	}
#endif // V2EXTEND__4

#ifndef V2APPROACHSTRAIGHT__4
	__forceinline Vector_4_Out V2ApproachStraight(Vector_4_In position, Vector_4_In goal, float rate, float time, unsigned int& rResult)
	{
		Vector_4 directionXY = V2Subtract( goal, position );
		Vector_4 unitDirectionXY = V2Normalize( directionXY );
		float scalarDistance = rate * time;
		Vector_4 finalPosition = V2AddScaled( position, unitDirectionXY, scalarDistance );

		Vector_4 directionXYNew = V2Subtract( goal, position );
		Vector_4 unitDirectionXYNew = V2Normalize( directionXYNew );

		unsigned int haventReachedGoal = V2IsCloseAll( unitDirectionXY, unitDirectionXYNew, SMALL_FLOAT );
		rResult = (haventReachedGoal ^ 0x1);

		return finalPosition;
	}
#endif // V2APPROACHSTRAIGHT__4

#ifndef V2REFLECT__4
	__forceinline Vector_4_Out V2Reflect( Vector_4_In inVector, Vector_4_In wall2DNormal )
	{
		float dot = V2Dot( inVector, wall2DNormal );
		Vector_4 outVect;
		outVect.x = (inVector.x - 2.0f*dot*wall2DNormal.x);
		outVect.y = (inVector.y - 2.0f*dot*wall2DNormal.y);
		return outVect;
	}
#endif // V2REFLECT__4

#ifndef V2SCALE__4
	__forceinline Vector_4_Out V2Scale( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x * floatVal;
		outVect.y = inVector.y * floatVal;
		return outVect;
	}
#endif // V2SCALE__4

#ifndef V2SCALE_V__4
	__forceinline Vector_4_Out V2Scale( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x * inVector2.x;
		outVect.y = inVector1.y * inVector2.y;
		return outVect;
	}
#endif // V2SCALE_V__4

#ifndef V2INVSCALE__4
	__forceinline Vector_4_Out V2InvScale( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		float invVal = 1.0f/floatVal;
		outVect.x = inVector.x * invVal;
		outVect.y = inVector.y * invVal;
		return outVect;
	}
#endif // V2INVSCALE__4

#ifndef V2INVSCALE_V__4
	__forceinline Vector_4_Out V2InvScale( Vector_4_In inVector, Vector_4_In floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x / floatVal.x;
		outVect.y = inVector.y / floatVal.y;
		return outVect;
	}
#endif // V2INVSCALE_V__4

#ifndef V2INVSCALESAFE__4
	__forceinline Vector_4_Out V2InvScaleSafe( Vector_4_In inVector, float floatVal, float errVal )
	{
		Vector_4 outVect;
		if( floatVal != 0.0f )
		{
			float invVal = 1.0f/floatVal;
			outVect.x = inVector.x * invVal;
			outVect.y = inVector.y * invVal;
		}
		else
		{
			outVect.x = outVect.y = errVal;
		}
		return outVect;
	}
#endif // V2INVSCALESAFE__4

#ifndef V2INVSCALESAFE_V__4
	__forceinline Vector_4_Out V2InvScaleSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal )
	{
		Vector_4 outVect;
		outVect.x = (floatVal.x != 0.0f) ? inVector.x/floatVal.x : errVal;
		outVect.y = (floatVal.y != 0.0f) ? inVector.y/floatVal.y : errVal;
		return outVect;
	}
#endif // V2INVSCALESAFE_V__4

#ifndef V2INVSCALEFAST__4
	__forceinline Vector_4_Out V2InvScaleFast( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		float inv = FPInvertFast(floatVal);
		outVect.x = inVector.x * inv;
		outVect.y = inVector.y * inv;
		return outVect;
	}
#endif // V2INVSCALEFAST__4

#ifndef V2INVSCALEFAST_V__4
	__forceinline Vector_4_Out V2InvScaleFast( Vector_4_In inVector, Vector_4_In floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x * FPInvertFast(floatVal.x);
		outVect.y = inVector.y * FPInvertFast(floatVal.y);
		return outVect;
	}
#endif // V2INVSCALEFAST_V__4

#ifndef V2INVSCALEFASTSAFE__4
	__forceinline Vector_4_Out V2InvScaleFastSafe( Vector_4_In inVector, float floatVal, float errVal )
	{
		Vector_4 outVect;
		float inv = FPInvertFast(floatVal);
		if( floatVal != 0.0f )
		{
			outVect.x = inVector.x*inv;
			outVect.y = inVector.y*inv;
		}
		else
		{
			outVect.x = outVect.y = errVal;
		}
		return outVect;
	}
#endif // V2INVSCALEFASTSAFE__4

#ifndef V2INVSCALEFASTSAFE_V__4
	__forceinline Vector_4_Out V2InvScaleFastSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal )
	{
		Vector_4 outVect;
		outVect.x = (floatVal.x != 0.0f) ? inVector.x*FPInvertFast(floatVal.x) : errVal;
		outVect.y = (floatVal.y != 0.0f) ? inVector.y*FPInvertFast(floatVal.y) : errVal;
		return outVect;
	}
#endif // V2INVSCALEFASTSAFE_V__4

#ifndef V2ADD_2F__4
	__forceinline Vector_4_Out V2Add( Vector_4_In inVector, float sx, float sy )
	{
		Vector_4 outVect;
		outVect.x = inVector.x + sx;
		outVect.y = inVector.y + sy;
		return outVect;
	}
#endif // V2ADD_2F__4

#ifndef V2ADD_V__4
	__forceinline Vector_4_Out V2Add( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x + inVector2.x;
		outVect.y = inVector1.y + inVector2.y;
		return outVect;
	}
#endif // V2ADD_V__4

#ifndef V2ADDSCALED__4
	__forceinline Vector_4_Out V2AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x + inVector2.x * floatValue;
		outVect.y = inVector1.y + inVector2.y * floatValue;
		return outVect;
	}
#endif // V2ADDSCALED__4

#ifndef V2ADDSCALED_V__4
	__forceinline Vector_4_Out V2AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x + inVector2.x * floatValue.x;
		outVect.y = inVector1.y + inVector2.y * floatValue.y;
		return outVect;
	}
#endif // V2ADDSCALED_V__4

#ifndef V2SUBTRACT_2F__4
	__forceinline Vector_4_Out V2Subtract( Vector_4_In inVector, float sx, float sy )
	{
		Vector_4 outVect;
		outVect.x = inVector.x - sx;
		outVect.y = inVector.y - sy;
		return outVect;
	}
#endif // V2SUBTRACT_2F__4

#ifndef V2SUBTRACT_V__4
	__forceinline Vector_4_Out V2Subtract( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x - inVector2.x;
		outVect.y = inVector1.y - inVector2.y;
		return outVect;
	}
#endif // V2SUBTRACT_V__4

#ifndef V2SUBTRACTSCALED__4
	__forceinline Vector_4_Out V2SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue;
		outVect.y = inVector1.y - inVector2.y * floatValue;
		return outVect;
	}
#endif // V2SUBTRACTSCALED__4

#ifndef V2SUBTRACTSCALED_V__4
	__forceinline Vector_4_Out V2SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue.x;
		outVect.y = inVector1.y - inVector2.y * floatValue.y;
		return outVect;
	}
#endif // V2SUBTRACTSCALED_V__4

#ifndef V2NEGATE__4
	__forceinline Vector_4_Out V2Negate(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = -inVector.x;
		outVect.y = -inVector.y;
		return outVect;
	}
#endif // V2NEGATE__4

#ifndef V2ABS__4
	__forceinline Vector_4_Out V2Abs(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPAbs(inVector.x);
		outVect.y = FPAbs(inVector.y);
		return outVect;
	}
#endif // V2ABS__4

#ifndef V2INVERT__4
	__forceinline Vector_4_Out V2Invert(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = 1.0f/inVector.x;
		outVect.y = 1.0f/inVector.y;
		return outVect;
	}
#endif // V2INVERT__4

#ifndef V2INVERTSAFE__4
	__forceinline Vector_4_Out V2InvertSafe(Vector_4_In inVector, float errVal)
	{
		Vector_4 outVect;
		outVect.x = ( inVector.x == 0.0f ? errVal : 1.0f/inVector.x );
		outVect.y = ( inVector.y == 0.0f ? errVal : 1.0f/inVector.y );
		return outVect;
	}
#endif // V2INVERTSAFE__4

#ifndef V2INVERTFAST__4
	__forceinline Vector_4_Out V2InvertFast(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPInvertFast(inVector.x);
		outVect.y = FPInvertFast(inVector.y);
		return outVect;
	}
#endif // V2INVERTFAST__4

#ifndef V2INVERTFASTSAFE__4
	__forceinline Vector_4_Out V2InvertFastSafe(Vector_4_In inVector, float errVal)
	{
		Vector_4 outVect;
		outVect.x = ( inVector.x == 0.0f ? errVal : FPInvertFast(inVector.x) );
		outVect.y = ( inVector.y == 0.0f ? errVal : FPInvertFast(inVector.y) );
		return outVect;
	}
#endif // V2INVERTFASTSAFE__4

#ifndef V2CROSS__4
	__forceinline float V2Cross(Vector_4_In a, Vector_4_In b)
	{
		return (a.x * b.y - a.y * b.x);
	}
#endif // V2CROSS__4

#ifndef V2CROSSV__4
	__forceinline Vector_4_Out V2CrossV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2Cross(a, b);
		return outVect;
	}
#endif // V2CROSSV__4

#ifndef V2AVERAGE__4
	__forceinline Vector_4_Out V2Average(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = 0.5f * (a.x + b.x);
		outVect.y = 0.5f * (a.y + b.y);
		return outVect;
	}
#endif // V2AVERAGE__4

#ifndef V2LERP__4
	__forceinline Vector_4_Out V2Lerp( float t, Vector_4_In a, Vector_4_In b )
	{
		Vector_4 outVect;
		outVect.x = FPLerp(t, a.x, b.x);
		outVect.y = FPLerp(t, a.y, b.y);
		return outVect;
	}
#endif // V2LERP__4

#ifndef V2LERP_V__4
	__forceinline Vector_4_Out V2Lerp( Vector_4_In t, Vector_4_In a, Vector_4_In b )
	{
		Vector_4 outVect;
		outVect.x = FPLerp(t.x, a.x, b.x);
		outVect.y = FPLerp(t.y, a.y, b.y);
		return outVect;
	}
#endif // V2LERP_V__4

#ifndef V2POW__4
	__forceinline Vector_4_Out V2Pow( Vector_4_In x, Vector_4_In y )
	{
		Vector_4 outVect;
		outVect.x = FPPow(x.x, y.x);
		outVect.y = FPPow(x.y, y.y);
		return outVect;
	}
#endif // V2POW__4

#ifndef V2EXPT__4
	__forceinline Vector_4_Out V2Expt( Vector_4_In x )
	{
		Vector_4 outVect;
		outVect.x = FPExpt(x.x);
		outVect.y = FPExpt(x.y);
		return outVect;
	}
#endif // V2EXPT__4

#ifndef V2LOG2__4
	__forceinline Vector_4_Out V2Log2( Vector_4_In x )
	{
		// log2(f) = ln(f)/ln(2)
		Vector_4 outVect;
		outVect.x = FPLog2(x.x);
		outVect.y = FPLog2(x.y);
		return outVect;
	}
#endif // V2LOG2__4

#ifndef V2LOG10__4
	__forceinline Vector_4_Out V2Log10( Vector_4_In x )
	{
		Vector_4 outVect;
		outVect.x = FPLog10(x.x);
		outVect.y = FPLog10(x.y);
		return outVect;
	}
#endif // V2LOG10__4

	//============================================================================
	// Magnitude and distance

#ifndef V2DOT__4
	__forceinline float V2Dot(Vector_4_In a, Vector_4_In b)
	{
		return a.x*b.x + a.y*b.y;
	}
#endif // V2DOT__4

#ifndef V2DOTV__4
	__forceinline Vector_4_Out V2DotV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2Dot(a, b);
		return outVect;
	}
#endif // V2DOTV__4

#ifndef V2NORMALIZE__4
	__forceinline Vector_4_Out V2Normalize(Vector_4_In inVector)
	{
		Vector_4 outVect;
		float invMag = V2InvMag(inVector);
		outVect.x = inVector.x * invMag;
		outVect.y = inVector.y * invMag;
		return outVect;
	}
#endif // V2NORMALIZE__4

#ifndef V2NORMALIZESAFE__4
	__forceinline Vector_4_Out V2NormalizeSafe(Vector_4_In inVector, float errVal, float magSqThreshold)
	{
		Vector_4 outVect;
		float mag2 = V2MagSquared(inVector);
		if(mag2 > magSqThreshold)
		{
			float invMag = FPInvSqrt(mag2);
			outVect.x = inVector.x*invMag;
			outVect.y = inVector.y*invMag;
		}
		else
		{
			outVect.x = outVect.y = errVal;
		}
		return outVect;
	}
#endif // V2NORMALIZESAFE__4

#ifndef V2NORMALIZEFAST__4
	__forceinline Vector_4_Out V2NormalizeFast(Vector_4_In inVector)
	{
		Vector_4 outVect;
		float invMag = V2InvMagFast(inVector);
		outVect.x = inVector.x * invMag;
		outVect.y = inVector.y * invMag;
		return outVect;
	}
#endif // V2NORMALIZEFAST

#ifndef V2NORMALIZEFASTSAFE__4
	__forceinline Vector_4_Out V2NormalizeFastSafe(Vector_4_In inVector, float errVal, float magSqThreshold)
	{
		Vector_4 outVect;
		float mag2 = V2MagSquared(inVector);
		if(mag2 > magSqThreshold)
		{
			float invMag = FPInvSqrtFast( mag2 );
			outVect.x = inVector.x*invMag;
			outVect.y = inVector.y*invMag;
		}
		else
		{
			outVect.x = outVect.y = errVal;
		}
		return outVect;
	}
#endif // V2NORMALIZEFASTSAFE__4

#ifndef V2SQRT__4
	__forceinline Vector_4_Out V2Sqrt( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = FPSqrt(v.x);
		outVect.y = FPSqrt(v.y);
		return outVect;
	}
#endif // V2SQRT__4

#ifndef V2SQRTSAFE__4
	__forceinline Vector_4_Out V2SqrtSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPSqrtSafe( v.x, errVal );
		outVect.y = FPSqrtSafe( v.y, errVal );
		return outVect;
	}
#endif // V2SQRTSAFE__4

#ifndef V2SQRTFAST__4
	__forceinline Vector_4_Out V2SqrtFast( Vector_4_In v )
	{
		return V2Sqrt( v );
	}
#endif // V2SQRTFAST__4

#ifndef V2SQRTFASTSAFE__4
	__forceinline Vector_4_Out V2SqrtFastSafe( Vector_4_In v, float errVal )
	{
		return V2SqrtSafe( v, errVal );
	}
#endif // V2SQRTFASTSAFE__4

#ifndef V2INVSQRT__4
	__forceinline Vector_4_Out V2InvSqrt( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrt(v.x);
		outVect.y = FPInvSqrt(v.y);
		return outVect;
	}
#endif // V2INVSQRT__4

#ifndef V2INVSQRTSAFE__4
	__forceinline Vector_4_Out V2InvSqrtSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtSafe(v.x, errVal);
		outVect.y = FPInvSqrtSafe(v.y, errVal);
		return outVect;
	}
#endif // V2INVSQRTSAFE__4

#ifndef V2INVSQRTFAST__4
	__forceinline Vector_4_Out V2InvSqrtFast( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtFast(v.x);
		outVect.y = FPInvSqrtFast(v.y);
		return outVect;
	}
#endif // V2INVSQRTFAST__4

#ifndef V2INVSQRTFASTSAFE__4
	__forceinline Vector_4_Out V2InvSqrtFastSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtFastSafe(v.x, errVal);
		outVect.y = FPInvSqrtFastSafe(v.y, errVal);
		return outVect;
	}
#endif // V2INVSQRTFASTSAFE__4

#ifndef V2MAG__4
	__forceinline float V2Mag( Vector_4_In v )
	{
		return FPSqrt( V2Dot( v, v ) );
	}
#endif // V2MAG__4

#ifndef V2MAGV__4
	__forceinline Vector_4_Out V2MagV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2Mag( v );
		return outVect;
	}
#endif // V2MAGV__4

#ifndef V2MAGFAST__4
	__forceinline float V2MagFast( Vector_4_In v )
	{
		return V2Mag( v );
	}
#endif // V2MAGFAST__4

#ifndef V2MAGVFAST__4
	__forceinline Vector_4_Out V2MagVFast( Vector_4_In v )
	{
		return V2MagV( v );
	}
#endif // V2MAGVFAST__4

#ifndef V2MAGSQUARED__4
	__forceinline float V2MagSquared( Vector_4_In v )
	{
		return V2Dot( v, v );
	}
#endif // V2MAGSQUARED__4

#ifndef V2MAGSQUAREDV__4
	__forceinline Vector_4_Out V2MagSquaredV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2MagSquared( v );
		return outVect;
	}
#endif // V2MAGSQUAREDV__4

#ifndef V2INVMAG__4
	__forceinline float V2InvMag( Vector_4_In v )
	{
		return FPInvSqrt( V2Dot( v, v ) );
	}
#endif // V2INVMAG__4

#ifndef V2INVMAGSAFE__4
	__forceinline float V2InvMagSafe( Vector_4_In v, float errVal )
	{
		float dot = V2Dot( v, v );
		return FPInvSqrtSafe( dot, errVal );
	}
#endif // V2INVMAGSAFE__4

#ifndef V2INVMAGFAST__4
	__forceinline float V2InvMagFast( Vector_4_In v )
	{
		return FPInvSqrtFast( V2Dot( v, v ) );
	}
#endif // V2INVMAGFAST__4

#ifndef V2INVMAGFASTSAFE__4
	__forceinline float V2InvMagFastSafe( Vector_4_In v, float errVal )
	{
		return FPInvSqrtFastSafe( V2Dot( v, v ), errVal );
	}
#endif // V2INVMAGFASTSAFE__4

#ifndef V2INVMAGV__4
	__forceinline Vector_4_Out V2InvMagV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvMag( v );
		return outVect;
	}
#endif // V2INVMAGV__4

#ifndef V2INVMAGVSAFE__4
	__forceinline Vector_4_Out V2InvMagVSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvMagSafe( v, errVal );
		return outVect;
	}
#endif // V2INVMAGVSAFE__4

#ifndef V2INVMAGVFAST__4
	__forceinline Vector_4_Out V2InvMagVFast( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvMagFast( v );
		return outVect;
	}
#endif // V2INVMAGVFAST__4

#ifndef V2INVMAGVFASTSAFE__4
	__forceinline Vector_4_Out V2InvMagVFastSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvMagFastSafe( v, errVal );
		return outVect;
	}
#endif // V2INVMAGVFASTSAFE__4

#ifndef V2INVMAGSQUARED__4
	__forceinline float V2InvMagSquared( Vector_4_In v )
	{
		return 1.0f/V2MagSquared( v );
	}
#endif // V2INVMAGSQUARED__4

#ifndef V2INVMAGSQUAREDSAFE__4
	__forceinline float V2InvMagSquaredSafe( Vector_4_In v, float errVal )
	{
		float magSq = V2MagSquared( v );
		return FPIfGtZeroThenElse( magSq, 1.0f/magSq, errVal );
	}
#endif // V2INVMAGSQUAREDSAFE__4

#ifndef V2INVMAGSQUAREDFAST__4
	__forceinline float V2InvMagSquaredFast( Vector_4_In v )
	{
		return FPInvertFast( V2MagSquared( v ) );
	}
#endif // V2INVMAGSQUAREDFAST__4

#ifndef V2INVMAGSQUAREDFASTSAFE__4
	__forceinline float V2InvMagSquaredFastSafe( Vector_4_In v, float errVal )
	{
		return FPInvertFastSafe( V2MagSquared( v ), errVal );
	}
#endif // V2INVMAGSQUAREDFASTSAFE__4

#ifndef V2INVMAGSQUAREDV__4
	__forceinline Vector_4_Out V2InvMagSquaredV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvMagSquared( v );
		return outVect;
	}
#endif // V2INVMAGSQUAREDV__4

#ifndef V2INVMAGSQUAREDVSAFE__4
	__forceinline Vector_4_Out V2InvMagSquaredVSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvMagSquaredSafe( v, errVal );
		return outVect;
	}
#endif // V2INVMAGSQUAREDVSAFE__4

#ifndef V2INVMAGSQUAREDVFAST__4
	__forceinline Vector_4_Out V2InvMagSquaredVFast( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvMagSquaredFast( v );
		return outVect;
	}
#endif // V2INVMAGSQUAREDVFAST__4

#ifndef V2INVMAGSQUAREDVFASTSAFE__4
	__forceinline Vector_4_Out V2InvMagSquaredVFastSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvMagSquaredFastSafe( v, errVal );
		return outVect;
	}
#endif // V2INVMAGSQUAREDVFASTSAFE__4

#ifndef V2DIST__4
	__forceinline float V2Dist(Vector_4_In a, Vector_4_In b)
	{
		return V2Mag( V2Subtract( a, b ) );
	}
#endif // V2DIST__4

#ifndef V2DISTFAST__4
	__forceinline float V2DistFast(Vector_4_In a, Vector_4_In b)
	{
		return V2Dist( a, b );
	}
#endif // V2DISTFAST__4

#ifndef V2DISTV__4
	__forceinline Vector_4_Out V2DistV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2Dist( a, b );
		return outVect;
	}
#endif // V2DISTV__4

#ifndef V2DISTVFAST__4
	__forceinline Vector_4_Out V2DistVFast(Vector_4_In a, Vector_4_In b)
	{
		return V2DistV( a, b );
	}
#endif // V2DISTVFAST__4

#ifndef V2INVDIST__4
	__forceinline float V2InvDist(Vector_4_In a, Vector_4_In b)
	{
		return V2InvMag( V2Subtract( a, b ) );
	}
#endif // V2INVDIST__4

#ifndef V2INVDISTSAFE__4
	__forceinline float V2InvDistSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V2InvMagSafe( V2Subtract( a, b ), errVal );
	}
#endif // V2INVDISTSAFE__4

#ifndef V2INVDISTFAST__4
	__forceinline float V2InvDistFast(Vector_4_In a, Vector_4_In b)
	{
		return V2InvMagFast( V2Subtract( a, b ) );
	}
#endif // V2INVDISTFAST__4

#ifndef V2INVDISTFASTSAFE__4
	__forceinline float V2InvDistFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V2InvMagFastSafe( V2Subtract( a, b ), errVal );
	}
#endif // V2INVDISTFASTSAFE__4

#ifndef V2INVDISTV__4
	__forceinline Vector_4_Out V2InvDistV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvDist( a, b );
		return outVect;
	}
#endif // V2INVDISTV__4

#ifndef V2INVDISTVSAFE__4
	__forceinline Vector_4_Out V2InvDistVSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvDistSafe( a, b, errVal );
		return outVect;
	}
#endif // V2INVDISTVSAFE__4

#ifndef V2INVDISTVFAST__4
	__forceinline Vector_4_Out V2InvDistVFast(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvDistFast( a, b );
		return outVect;
	}
#endif // V2INVDISTVFAST__4

#ifndef V2INVDISTVFASTSAFE__4
	__forceinline Vector_4_Out V2InvDistVFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvDistFastSafe( a, b, errVal );
		return outVect;
	}
#endif // V2INVDISTVFASTSAFE__4

#ifndef V2DISTSQUARED__4
	__forceinline float V2DistSquared(Vector_4_In a, Vector_4_In b)
	{
		return V2MagSquared( V2Subtract( a, b ) );
	}
#endif // V2DISTSQUARED__4

#ifndef V2DISTSQUAREDV__4
	__forceinline Vector_4_Out V2DistSquaredV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2DistSquared( a, b );
		return outVect;
	}
#endif // V2DISTSQUAREDV__4

#ifndef V2INVDISTSQUARED__4
	__forceinline float V2InvDistSquared(Vector_4_In a, Vector_4_In b)
	{
		return V2InvMagSquared( V2Subtract( a, b ) );
	}
#endif // V2INVDISTSQUARED__4

#ifndef V2INVDISTSQUAREDSAFE__4
	__forceinline float V2InvDistSquaredSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V2InvMagSquaredSafe( V2Subtract( a, b ), errVal );
	}
#endif // V2INVDISTSQUAREDSAFE__4

#ifndef V2INVDISTSQUAREDFAST__4
	__forceinline float V2InvDistSquaredFast(Vector_4_In a, Vector_4_In b)
	{
		return V2InvMagSquaredFast( V2Subtract( a, b ) );
	}
#endif // V2INVDISTSQUAREDFAST__4

#ifndef V2INVDISTSQUAREDFASTSAFE__4
	__forceinline float V2InvDistSquaredFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V2InvMagSquaredFastSafe( V2Subtract( a, b ), errVal );
	}
#endif // V2INVDISTSQUAREDFASTSAFE__4

#ifndef V2INVDISTSQUAREDV__4
	__forceinline Vector_4_Out V2InvDistSquaredV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvDistSquared( a, b );
		return outVect;
	}
#endif // V2INVDISTSQUAREDV__4

#ifndef V2INVDISTSQUAREDVSAFE__4
	__forceinline Vector_4_Out V2InvDistSquaredVSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvDistSquaredSafe( a, b, errVal );
		return outVect;
	}
#endif // V2INVDISTSQUAREDVSAFE__4

#ifndef V2INVDISTSQUAREDVFAST__4
	__forceinline Vector_4_Out V2InvDistSquaredVFast(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvDistSquaredFast( a, b );
		return outVect;
	}
#endif // V2INVDISTSQUAREDVFAST__4

#ifndef V2INVDISTSQUAREDVFASTSAFE__4
	__forceinline Vector_4_Out V2InvDistSquaredVFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V2InvDistSquaredFastSafe( a, b, errVal );
		return outVect;
	}
#endif // V2INVDISTSQUAREDVFASTSAFE__4

	//============================================================================
	// Conversion functions

#ifndef V2FLOATTOINTRAW__4
	template <int exponent>
	__forceinline Vector_4_Out V2FloatToIntRaw(Vector_4_In inVector)
	{
		float multiplier = static_cast<float>(1 << exponent);

		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult;

		TempResult.i = static_cast<int>(inVector.x * multiplier);
		outVect.x = TempResult.f;

		TempResult.i = static_cast<int>(inVector.y * multiplier);
		outVect.y = TempResult.f;

		return outVect;
	}
#endif // V2FLOATTOINTRAW__4

#ifndef V2INTTOFLOATRAW__4
	template <int exponent>
	__forceinline Vector_4_Out V2IntToFloatRaw(Vector_4_In inVector)
	{
		float divider = static_cast<float>(1 << exponent);
		float invDiv = 1.0f/divider;

		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} Temp1;

		Temp1.f = inVector.x;
		outVect.x = static_cast<float>(Temp1.i) * invDiv;
		Temp1.f = inVector.y;
		outVect.y = static_cast<float>(Temp1.i) * invDiv;
		return outVect;
	}
#endif // V2INTTOFLOATRAW__4

#ifndef V2ROUNDTONEARESTINT__4
	__forceinline Vector_4_Out V2RoundToNearestInt(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x+(FPIfGteZeroThenElse(inVector.x, 0.5f, -0.5f))) );
		outVect.y = static_cast<float>( static_cast<int>(inVector.y+(FPIfGteZeroThenElse(inVector.y, 0.5f, -0.5f))) );
		return outVect;
	}
#endif // V2ROUNDTONEARESTINT__4

#ifndef V2ROUNDTONEARESTINTZERO__4
	__forceinline Vector_4_Out V2RoundToNearestIntZero(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x) );
		outVect.y = static_cast<float>( static_cast<int>(inVector.y) );
		return outVect;
	}
#endif // V2ROUNDTONEARESTINTZERO__4

#ifndef V2ROUNDTONEARESTINTNEGINF__4
	__forceinline Vector_4_Out V2RoundToNearestIntNegInf(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPFloor( inVector.x );
		outVect.y = FPFloor( inVector.y );
		return outVect;
	}
#endif // V2ROUNDTONEARESTINTNEGINF__4

#ifndef V2ROUNDTONEARESTINTPOSINF__4
	__forceinline Vector_4_Out V2RoundToNearestIntPosInf(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPCeil( inVector.x );
		outVect.y = FPCeil( inVector.y );
		return outVect;
	}
#endif // V2ROUNDTONEARESTINTPOSINF__4

	//============================================================================
	// Comparison functions

#ifndef V2ISBETWEENNEGANDPOSBOUNDS__4
	__forceinline unsigned int V2IsBetweenNegAndPosBounds( Vector_4_In testVector, Vector_4_In boundsVector )
	{
		return (	testVector.x <= boundsVector.x && testVector.x >= -boundsVector.x &&
					testVector.y <= boundsVector.y && testVector.y >= -boundsVector.y ? 1u : 0u );
	}
#endif // V2ISBETWEENNEGANDPOSBOUNDS__4

#ifndef V2SAMESIGNALL__4
	__forceinline unsigned int V2SameSignAll(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		return (
			(*(int*)( &inVector1.x ) & 0x80000000) == (*(int*)( &inVector2.x ) & 0x80000000) &&
			(*(int*)( &inVector1.y ) & 0x80000000) == (*(int*)( &inVector2.y ) & 0x80000000)
			) ? 1u : 0u;
	}
#endif // V2SAMESIGNALL__4

#ifndef V2ISZEROALL__4
	__forceinline unsigned int V2IsZeroAll(Vector_4_In inVector)
	{
		return ( inVector.x == 0.0f && inVector.y == 0.0f ? 1u : 0u );
	}
#endif // V2ISZEROALL__4

#ifndef V2ISZERONONE__4
	__forceinline unsigned int V2IsZeroNone(Vector_4_In inVector)
	{
		return ( inVector.x != 0.0f && inVector.y != 0.0f ? 1u : 0u );
	}
#endif // V2ISZERONONE__4

#ifndef V2ISEQUALALL__4
	__forceinline unsigned int V2IsEqualAll(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		return ( inVector1.x == inVector2.x &&
				inVector1.y == inVector2.y ? 1u : 0u );
	}
#endif // V2ISEQUALALL__4

#ifndef V2ISEQUALNONE__4
	__forceinline unsigned int V2IsEqualNone(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		return ( inVector1.x != inVector2.x &&
				inVector1.y != inVector2.y ? 1u : 0u );
	}
#endif // V2ISEQUALNONE__4

#ifndef V2ISEQUALINTALL__4
	__forceinline unsigned int V2IsEqualIntAll(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		union
		{
			float f;
			int i;
		} Temp1, Temp2, Temp3, Temp4;
		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;
		Temp3.f = inVector1.y;
		Temp4.f = inVector2.y;

		return (Temp1.i == Temp2.i &&
				Temp3.i == Temp4.i ? 1u : 0u );
	}
#endif // V2ISEQUALINTALL__4

#ifndef V2ISEQUALINTNONE__4
	__forceinline unsigned int V2IsEqualIntNone(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		union
		{
			float f;
			int i;
		} Temp1, Temp2, Temp3, Temp4;
		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;
		Temp3.f = inVector1.y;
		Temp4.f = inVector2.y;

		return (Temp1.i != Temp2.i &&
				Temp3.i != Temp4.i ? 1u : 0u );
	}
#endif // V2ISEQUALINTNONE__4

#ifndef V2ISCLOSEALL__4
	__forceinline unsigned int V2IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, float eps)
	{
		return ( ( inVector1.x >= inVector2.x - eps && inVector1.x <= inVector2.x + eps ) &&
				( inVector1.y >= inVector2.y - eps && inVector1.y <= inVector2.y + eps ) ? 1u : 0u);
	}
#endif // V2ISCLOSEALL__4

#ifndef V2ISCLOSENONE__4
	__forceinline unsigned int V2IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, float eps)
	{
		return (( inVector1.x < inVector2.x - eps || inVector1.x > inVector2.x + eps ) &&
				( inVector1.y < inVector2.y - eps || inVector1.y > inVector2.y + eps ) ? 1u : 0u);
	}
#endif // V2ISCLOSENONE__4

#ifndef V2ISCLOSEALL_V__4
	__forceinline unsigned int V2IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps)
	{
		return (( inVector1.x >= inVector2.x - eps.x && inVector1.x <= inVector2.x + eps.x ) &&
				( inVector1.y >= inVector2.y - eps.y && inVector1.y <= inVector2.y + eps.y ) ? 1u : 0u);
	}
#endif // V2ISCLOSEALL_V__4

#ifndef V2ISCLOSENONE_V__4
	__forceinline unsigned int V2IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps)
	{
		return (( inVector1.x < inVector2.x - eps.x || inVector1.x > inVector2.x + eps.x ) &&
				( inVector1.y < inVector2.y - eps.y || inVector1.y > inVector2.y + eps.y ) ? 1u : 0u);
	}
#endif // V2ISCLOSENONE_V__4

#ifndef V2ISGREATERTHANALL__4
	__forceinline unsigned int V2IsGreaterThanAll(Vector_4_In bigVector, Vector_4_In smallVector)
	{
		return (( bigVector.x > smallVector.x ) &&
				( bigVector.y > smallVector.y ) ? 1u : 0u );
	}
#endif // V2ISGREATERTHANALL__4

#ifndef V2ISGREATERTHANOREQUALALL__4
	__forceinline unsigned int V2IsGreaterThanOrEqualAll(Vector_4_In bigVector, Vector_4_In smallVector)
	{
		return (( bigVector.x >= smallVector.x ) &&
				( bigVector.y >= smallVector.y ) ? 1u : 0u);
	}
#endif // V2ISGREATERTHANOREQUALALL__4

#ifndef V2ISLESSTHANALL__4
	__forceinline unsigned int V2IsLessThanAll(Vector_4_In smallVector, Vector_4_In bigVector)
	{
		return (( bigVector.x > smallVector.x ) &&
				( bigVector.y > smallVector.y ) ? 1u : 0u);
	}
#endif // V2ISLESSTHANALL__4

#ifndef V2ISLESSTHANOREQUALALL__4
	__forceinline unsigned int V2IsLessThanOrEqualAll(Vector_4_In smallVector, Vector_4_In bigVector)
	{
		return (( bigVector.x >= smallVector.x ) &&
				( bigVector.y >= smallVector.y ) ? 1u : 0u);
	}
#endif // V2ISLESSTHANOREQUALALL__4

#ifndef V2SELECT__4
	__forceinline Vector_4_Out V2SelectFT(Vector_4_In choiceVector, Vector_4_In zero, Vector_4_In nonZero)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult, Temp1, Temp2, Temp3;

		Temp1.f = choiceVector.x;
		Temp2.f = zero.x;
		Temp3.f = nonZero.x;
		TempResult.i = (Temp2.i & ~Temp1.i) | (Temp3.i & Temp1.i);
		outVect.x = TempResult.f;

		Temp1.f = choiceVector.y;
		Temp2.f = zero.y;
		Temp3.f = nonZero.y;
		TempResult.i = (Temp2.i & ~Temp1.i) | (Temp3.i & Temp1.i);
		outVect.y = TempResult.f;

		return outVect;
	}
#endif // V2SELECT__4

#ifndef V2SELECTVECT__4
	__forceinline Vector_4_Out V2SelectVect(Vector_4_In choiceVectorX, Vector_4_In zero, Vector_4_In nonZero)
	{
		return (GetX(choiceVectorX) == 0.0f ? zero : nonZero );
	}
#endif // V2SELECTVECT__4

#ifndef V2MAX__4
	__forceinline Vector_4_Out V2Max(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = FPMax( inVector1.x, inVector2.x );
		outVect.y = FPMax( inVector1.y, inVector2.y );
		return outVect;
	}
#endif // V2MAX__4

#ifndef V2MIN__4
	__forceinline Vector_4_Out V2Min(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = FPMin( inVector1.x, inVector2.x );
		outVect.y = FPMin( inVector1.y, inVector2.y );
		return outVect;
	}
#endif // V2MIN__4

#ifndef V2MINELEMENT__4
	__forceinline Vector_4_Out V2MinElement(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = FPMin( inVector.x, inVector.y );
		return outVect;
	}
#endif // V2MINELEMENT__4

#ifndef V2MAXELEMENT__4
	__forceinline Vector_4_Out V2MaxElement(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = FPMax( inVector.x, inVector.y );
		return outVect;
	}
#endif // V2MAXELEMENT__4

#ifndef V2CLAMP__4
	__forceinline Vector_4_Out V2Clamp( Vector_4_In inVector, Vector_4_In lowBound, Vector_4_In highBound )
	{
		Vector_4 outVect;
		outVect.x = FPClamp( inVector.x, lowBound.x, highBound.x );
		outVect.y = FPClamp( inVector.y, lowBound.y, highBound.y );
		return outVect;
	}
#endif // V2CLAMP__4

#ifndef V2SATURATE__4
	__forceinline Vector_4_Out V2Clamp( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPClamp( inVector.x, 0.0f, 1.0f );
		outVect.y = FPClamp( inVector.y, 0.0f, 1.0f );
		return outVect;
	}
#endif // V2SATURATE__4

	//============================================================================
	// Bitwise operations

#ifndef V2AND__4
	__forceinline Vector_4_Out V2And(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, Temp1, Temp2;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		TempResult1.i &= Temp1.i;
		TempResult2.i &= Temp2.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		return outVect;
	}
#endif // V2AND__4

#ifndef V2OR__4
	__forceinline Vector_4_Out V2Or(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, Temp1, Temp2;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		TempResult1.i |= Temp1.i;
		TempResult2.i |= Temp2.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		return outVect;
	}
#endif // V2OR__4

#ifndef V2XOR__4
	__forceinline Vector_4_Out V2Xor(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, Temp1, Temp2;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		TempResult1.i ^= Temp1.i;
		TempResult2.i ^= Temp2.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		return outVect;
	}
#endif // V2XOR__4

#ifndef V2ANDC__4
	__forceinline Vector_4_Out V2Andc(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, Temp1, Temp2;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		TempResult1.i &= ~Temp1.i;
		TempResult2.i &= ~Temp2.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		return outVect;
	}
#endif // V2ANDC__4


} // namespace Vec
} // namespace rage
