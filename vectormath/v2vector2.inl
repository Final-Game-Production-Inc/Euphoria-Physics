
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

namespace rage
{
namespace Vec
{

#ifndef V2GETELEM
	__forceinline float GetElem( Vector_2_In inVector, unsigned int elem )
	{
		mthAssertf( elem <= 1 , "Invalid element index %d out of range [0,1]", elem );
		return ((float*)(&inVector))[elem];
	}
#endif // V2GETELEM

#ifndef V2GETELEMREF
	__forceinline float& GetElemRef( Vector_2_Ptr pInVector, unsigned int elem )
	{
		mthAssertf( elem <= 1 , "Invalid element index %d out of range [0,1]", elem );
		return ((float*)(pInVector))[elem];
	}
#endif // V2GETELEMREF

#ifndef V2GETELEMREF_CONST
	__forceinline const float& GetElemRef( Vector_2_ConstPtr pInVector, unsigned int elem )
	{
		mthAssertf( elem <= 1 , "Invalid element index %d out of range [0,1]", elem );
		return ((const float*)(pInVector))[elem];
	}
#endif // V2GETELEMREF_CONST

#ifndef V2GETX
	__forceinline float GetX( Vector_2_In inVector )
	{
		return inVector.x;
	}
#endif // V2GETX

#ifndef V2GETY
	__forceinline float GetY( Vector_2_In inVector )
	{
		return inVector.y;
	}
#endif // V2GETY

#ifndef V2GETXV
	__forceinline Vector_2_Out GetXV( Vector_2_In inVector )
	{
		Vector_2 outVect;
		float x = inVector.x;
		outVect.x = x;
		outVect.y = x;
		return outVect;
	}
#endif // V2GETXV

#ifndef V2GETYV
	__forceinline Vector_2_Out GetYV( Vector_2_In inVector )
	{
		Vector_2 outVect;
		float y = inVector.y;
		outVect.x = y;
		outVect.y = y;
		return outVect;
	}
#endif // V2GETYV

#ifndef V2SETX
	__forceinline void SetX( Vector_2_InOut inoutVector, float floatVal )
	{
		inoutVector.x = floatVal;
	}
#endif // V2SETX

#ifndef V2SETY
	__forceinline void SetY( Vector_2_InOut inoutVector, float floatVal )
	{
		inoutVector.y = floatVal;
	}
#endif // V2SETY

#ifndef V2SPLATX
	__forceinline Vector_2_Out V2SplatX( Vector_2_In inVector )
	{
		Vector_2 outVect;
		float x = inVector.x;
		outVect.x = x;
		outVect.y = x;
		return outVect;
	}
#endif // V2SPLATX

#ifndef V2SPLATY
	__forceinline Vector_2_Out V2SplatY( Vector_2_In inVector )
	{
		Vector_2 outVect;
		float y = inVector.y;
		outVect.x = y;
		outVect.y = y;
		return outVect;
	}
#endif // V2SPLATY

#ifndef V2SET_2F
	__forceinline void V2Set( Vector_2_InOut inoutVector, float x0, float y0 )
	{
		inoutVector.x = x0;
		inoutVector.y = y0;
	}
#endif // V2SET_2F

#ifndef V2SET_V
	__forceinline void V2Set( Vector_2_InOut inoutVector, Vector_2_In inVector )
	{
		inoutVector = inVector;
	}
#endif // V2SET_V

#ifndef V2SET
	__forceinline void V2Set( Vector_2_InOut inoutVector, float s )
	{
		inoutVector.x = s;
		inoutVector.y = s;
	}
#endif // V2SET

#ifndef V2ZEROCOMPONENTS
	__forceinline void V2ZeroComponents( Vector_2_InOut inoutVector )
	{
		inoutVector.x = inoutVector.y = 0.0f;
	}
#endif // V2ZEROCOMPONENTS


	//============================================================================
	// Standard Algebra

#ifndef V2ANGLE
	__forceinline float V2Angle(Vector_2_In v1, Vector_2_In v2)
	{
		float magSquaredProduct = V2MagSquared(v1) * V2MagSquared(v2);
		float invSqrt = FPInvSqrt( magSquaredProduct );
		return FPACos( V2Dot( v1, v2 ) * invSqrt );
	}
#endif // V2ANGLE

#ifndef V2ANGLENORMINPUT
	__forceinline float V2AngleNormInput(Vector_2_In v1, Vector_2_In v2)
	{
		return FPACos( V2Dot( v1, v2 ) );
	}
#endif // V2ANGLENORMINPUT

#ifndef V2ANGLEV
	__forceinline Vector_2_Out V2AngleV(Vector_2_In v1, Vector_2_In v2)
	{
		float magSquaredProduct = V2MagSquared(v1) * V2MagSquared(v2);
		float invSqrt = FPInvSqrt( magSquaredProduct );
		Vector_2 outVect;
		outVect.x = outVect.y = FPACos( V2Dot( v1, v2 ) * invSqrt );
		return outVect;
	}
#endif // V2ANGLEV

#ifndef V2ANGLEVNORMINPUT
	__forceinline Vector_2_Out V2AngleVNormInput(Vector_2_In v1, Vector_2_In v2)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = FPACos( V2Dot( v1, v2 ) );
		return outVect;
	}
#endif // V2ANGLEVNORMINPUT

#ifndef V2WHICHSIDEOFLINE
	__forceinline float V2WhichSideOfLine(Vector_2_In point, Vector_2_In lineP1, Vector_2_In lineP2)
	{
		return
			( (lineP2.x - lineP1.x)*(point.y - lineP1.y)
			-
			(point.x - lineP1.x)*(lineP2.y - lineP1.y) );
	}
#endif // V2WHICHSIDEOFLINE

#ifndef V2WHICHSIDEOFLINEV
	__forceinline Vector_2_Out V2WhichSideOfLineV(Vector_2_In point, Vector_2_In lineP1, Vector_2_In lineP2)
	{
		Vector_2 outVect;
		outVect.x = outVect.y =
			( (lineP2.x - lineP1.x)*(point.y - lineP1.y)
			-
			(point.x - lineP1.x)*(lineP2.y - lineP1.y) );
		return outVect;
	}
#endif // V2WHICHSIDEOFLINEV

#ifndef V2ROTATE
	__forceinline Vector_2_Out V2Rotate(Vector_2_In inVector, float radians)
	{
		float tsin, tcos;
		FPSinAndCos(tsin, tcos, radians);
		Vector_2_Out outVect;
		outVect.x = inVector.x*tcos - inVector.y*tsin;
		outVect.y = inVector.x*tsin + inVector.y*tcos;
		return outVect;
	}
#endif // V2ROTATE

#ifndef V2ADDNET
	__forceinline Vector_2_Out V2AddNet( Vector_2_In inVector, Vector_2_In toAdd )
	{
		Vector_2 modifiedAdd;
		float dot = V2Dot(inVector, toAdd);
		if( dot > 0.0f )
		{
			// The given vector has a positive dot product with this vector, so remove its parallel component.
			modifiedAdd = V2SubtractScaled( toAdd, inVector, dot*V2InvMagSquared( inVector ) );
		}

		// Add the given vector to this vector, less any positive parallel component.
		return V2Add( inVector, modifiedAdd );
	}
#endif // V2ADDNET

#ifndef V2EXTEND
	__forceinline Vector_2_Out V2Extend( Vector_2_In inVector, Vector_2_In amount )
	{
		float invMag = V2InvMag( inVector );
		Vector_2 scaleValue;
		scaleValue = V2AddScaled( V2Constant(V_ONE), amount, invMag );
		return V2Scale( inVector, scaleValue );
	}
#endif // V2EXTEND

#ifndef V2APPROACHSTRAIGHT
	__forceinline Vector_2_Out V2ApproachStraight(Vector_2_In position, Vector_2_In goal, float rate, float time, unsigned int& rResult)
	{
		Vector_2 directionXY = V2Subtract( goal, position );
		Vector_2 unitDirectionXY = V2Normalize( directionXY );
		float scalarDistance = rate * time;
		Vector_2 finalPosition = V2AddScaled( position, unitDirectionXY, scalarDistance );

		Vector_2 directionXYNew = V2Subtract( goal, position );
		Vector_2 unitDirectionXYNew = V2Normalize( directionXYNew );

		unsigned int haventReachedGoal = V2IsCloseAll( unitDirectionXY, unitDirectionXYNew, SMALL_FLOAT );
		rResult = (haventReachedGoal ^ 0x1);

		return finalPosition;
	}
#endif // V2APPROACHSTRAIGHT

#ifndef V2REFLECT
	__forceinline Vector_2_Out V2Reflect( Vector_2_In inVector, Vector_2_In wall2DNormal )
	{
		float dot = V2Dot( inVector, wall2DNormal );
		Vector_2 outVect;
		outVect.x = (inVector.x - 2.0f*dot*wall2DNormal.x);
		outVect.y = (inVector.y - 2.0f*dot*wall2DNormal.y);
		return outVect;
	}
#endif // V2REFLECT

#ifndef V2SCALE
	__forceinline Vector_2_Out V2Scale( Vector_2_In inVector, float floatVal )
	{
		Vector_2 outVect;
		outVect.x = inVector.x * floatVal;
		outVect.y = inVector.y * floatVal;
		return outVect;
	}
#endif // V2SCALE

#ifndef V2INVSCALE
	__forceinline Vector_2_Out V2InvScale( Vector_2_In inVector, float floatVal )
	{
		Vector_2 outVect;
		float invVal = 1.0f/floatVal;
		outVect.x = inVector.x * invVal;
		outVect.y = inVector.y * invVal;
		return outVect;
	}
#endif // V2INVSCALE

#ifndef V2INVSCALE_V
	__forceinline Vector_2_Out V2InvScale( Vector_2_In inVector, Vector_2_In floatVal )
	{
		Vector_2 outVect;
		outVect.x = inVector.x / floatVal.x;
		outVect.y = inVector.y / floatVal.y;
		return outVect;
	}
#endif // V2INVSCALE_V

#ifndef V2INVSCALESAFE
	__forceinline Vector_2_Out V2InvScaleSafe( Vector_2_In inVector, float floatVal, float errVal )
	{
		Vector_2 outVect;
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
#endif // V2INVSCALESAFE

#ifndef V2INVSCALESAFE_V
	__forceinline Vector_2_Out V2InvScaleSafe( Vector_2_In inVector, Vector_2_In floatVal, float errVal )
	{
		Vector_2 outVect;
		outVect.x = (floatVal.x != 0.0f) ? inVector.x/floatVal.x : errVal;
		outVect.y = (floatVal.y != 0.0f) ? inVector.y/floatVal.y : errVal;
		return outVect;
	}
#endif // V2INVSCALESAFE_V

#ifndef V2INVSCALEFAST
	__forceinline Vector_2_Out V2InvScaleFast( Vector_2_In inVector, float floatVal )
	{
		Vector_2 outVect;
		float inv = FPInvertFast(floatVal);
		outVect.x = inVector.x * inv;
		outVect.y = inVector.y * inv;
		return outVect;
	}
#endif // V2INVSCALEFAST

#ifndef V2INVSCALEFAST_V
	__forceinline Vector_2_Out V2InvScaleFast( Vector_2_In inVector, Vector_2_In floatVal )
	{
		Vector_2 outVect;
		outVect.x = inVector.x * FPInvertFast(floatVal.x);
		outVect.y = inVector.y * FPInvertFast(floatVal.y);
		return outVect;
	}
#endif // V2INVSCALEFAST_V

#ifndef V2INVSCALEFASTSAFE
	__forceinline Vector_2_Out V2InvScaleFastSafe( Vector_2_In inVector, float floatVal, float errVal )
	{
		Vector_2 outVect;
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

		//Vector_2 outVect;
		//float inv = FPInvertFast(floatVal);
		//outVect.x = FPIfEqThenElse( floatVal, 0.0f, errValVect.x, inVector.x*inv );
		//outVect.y = FPIfEqThenElse( floatVal, 0.0f, errValVect.y, inVector.y*inv );
		//return outVect;
	}
#endif // V2INVSCALEFASTSAFE

#ifndef V2INVSCALEFASTSAFE_V
	__forceinline Vector_2_Out V2InvScaleFastSafe( Vector_2_In inVector, Vector_2_In floatVal, float errVal )
	{
		Vector_2 outVect;
		outVect.x = (floatVal.x != 0.0f) ? inVector.x*FPInvertFast(floatVal.x) : errVal;
		outVect.y = (floatVal.y != 0.0f) ? inVector.y*FPInvertFast(floatVal.y) : errVal;
		return outVect;

		//Vector_2 outVect;
		//outVect.x = FPIfEqThenElse( floatVal.x, 0.0f, errValVect.x, inVector.x*FPInvertFast(floatVal.x) );
		//outVect.y = FPIfEqThenElse( floatVal.y, 0.0f, errValVect.y, inVector.y*FPInvertFast(floatVal.y) );
		//return outVect;
	}
#endif // V2INVSCALEFASTSAFE_V

#ifndef V2ADD_2F
	__forceinline Vector_2_Out V2Add( Vector_2_In inVector, float sx, float sy )
	{
		Vector_2 outVect;
		outVect.x = inVector.x + sx;
		outVect.y = inVector.y + sy;
		return outVect;
	}
#endif // V2ADD_2F

#ifndef V2ADD_V
	__forceinline Vector_2_Out V2Add( Vector_2_In inVector1, Vector_2_In inVector2 )
	{
		Vector_2 outVect;
		outVect.x = inVector1.x + inVector2.x;
		outVect.y = inVector1.y + inVector2.y;
		return outVect;
	}
#endif // V2ADD_V

#ifndef V2ADDINT_V
	__forceinline Vector_2_Out V2AddInt( Vector_2_In inVector1, Vector_2_In inVector2 )
	{
		Vector_2 outVect;

		union
		{
			float f;
			int i;
		} Temp1, Temp2, TempResult;

		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;
		TempResult.i = Temp1.i + Temp2.i;
		outVect.x = TempResult.f;

		Temp1.f = inVector1.y;
		Temp2.f = inVector2.y;
		TempResult.i = Temp1.i + Temp2.i;
		outVect.y = TempResult.f;

		return outVect;
	}
#endif // V2ADDINT_V

#ifndef V2SUBTRACTINT_V
	__forceinline Vector_2_Out V2SubtractInt( Vector_2_In inVector1, Vector_2_In inVector2 )
	{
		Vector_2 outVect;

		union
		{
			float f;
			int i;
		} Temp1, Temp2, TempResult;

		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;
		TempResult.i = Temp1.i - Temp2.i;
		outVect.x = TempResult.f;

		Temp1.f = inVector1.y;
		Temp2.f = inVector2.y;
		TempResult.i = Temp1.i - Temp2.i;
		outVect.y = TempResult.f;

		return outVect;
	}
#endif // V2SUBTRACTINT_V

#ifndef V2ADDSCALED
	__forceinline Vector_2_Out V2AddScaled( Vector_2_In inVector1, Vector_2_In inVector2, float floatValue )
	{
		Vector_2 outVect;
		outVect.x = inVector1.x + inVector2.x * floatValue;
		outVect.y = inVector1.y + inVector2.y * floatValue;
		return outVect;
	}
#endif // V2ADDSCALED

#ifndef V2ADDSCALED_V
	__forceinline Vector_2_Out V2AddScaled( Vector_2_In inVector1, Vector_2_In inVector2, Vector_2_In floatValue )
	{
		Vector_2 outVect;
		outVect.x = inVector1.x + inVector2.x * floatValue.x;
		outVect.y = inVector1.y + inVector2.y * floatValue.y;
		return outVect;
	}
#endif // V2ADDSCALED_V

#ifndef V2SUBTRACT_2F
	__forceinline Vector_2_Out V2Subtract( Vector_2_In inVector, float sx, float sy )
	{
		Vector_2 outVect;
		outVect.x = inVector.x - sx;
		outVect.y = inVector.y - sy;
		return outVect;
	}
#endif // V2SUBTRACT_2F

#ifndef V2SUBTRACT_V
	__forceinline Vector_2_Out V2Subtract( Vector_2_In inVector1, Vector_2_In inVector2 )
	{
		Vector_2 outVect;
		outVect.x = inVector1.x - inVector2.x;
		outVect.y = inVector1.y - inVector2.y;
		return outVect;
	}
#endif // V2SUBTRACT_V

#ifndef V2SUBTRACTSCALED
	__forceinline Vector_2_Out V2SubtractScaled( Vector_2_In inVector1, Vector_2_In inVector2, float floatValue )
	{
		Vector_2 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue;
		outVect.y = inVector1.y - inVector2.y * floatValue;
		return outVect;
	}
#endif // V2SUBTRACTSCALED

#ifndef V2SUBTRACTSCALED_V
	__forceinline Vector_2_Out V2SubtractScaled( Vector_2_In inVector1, Vector_2_In inVector2, Vector_2_In floatValue )
	{
		Vector_2 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue.x;
		outVect.y = inVector1.y - inVector2.y * floatValue.y;
		return outVect;
	}
#endif // V2SUBTRACTSCALED_V

#ifndef V2SCALE_V
	__forceinline Vector_2_Out V2Scale( Vector_2_In inVector1, Vector_2_In inVector2 )
	{
		Vector_2 outVect;
		outVect.x = inVector1.x * inVector2.x;
		outVect.y = inVector1.y * inVector2.y;
		return outVect;
	}
#endif // V2SCALE_V

#ifndef V2NEGATE
	__forceinline Vector_2_Out V2Negate(Vector_2_In inVector)
	{
		Vector_2 outVect;
		outVect.x = -inVector.x;
		outVect.y = -inVector.y;
		return outVect;
	}
#endif // V2NEGATE

#ifndef V2ABS
	__forceinline Vector_2_Out V2Abs(Vector_2_In inVector)
	{
		Vector_2 outVect;
		outVect.x = FPAbs(inVector.x);
		outVect.y = FPAbs(inVector.y);
		return outVect;
	}
#endif // V2ABS

#ifndef V2INVERTBITS
	__forceinline Vector_2_Out V2InvertBits(Vector_2_In inVector)
	{
		Vector_2 outVect;

		union
		{
			float f;
			int i;
		} Temp1, TempResult;

		Temp1.f = inVector.x;
		TempResult.i = Temp1.i ^ 0xFFFFFFFF;
		outVect.x = TempResult.f;

		Temp1.f = inVector.y;
		TempResult.i = Temp1.i ^ 0xFFFFFFFF;
		outVect.y = TempResult.f;

		return outVect;
	}
#endif // V2INVERTBITS

#ifndef V2INVERT
	__forceinline Vector_2_Out V2Invert(Vector_2_In inVector)
	{
		Vector_2 outVect;
		outVect.x = 1.0f/inVector.x;
		outVect.y = 1.0f/inVector.y;
		return outVect;
	}
#endif // V2INVERT

#ifndef V2INVERTSAFE
	__forceinline Vector_2_Out V2InvertSafe(Vector_2_In inVector, float errVal)
	{
		Vector_2 outVect;
		outVect.x = ( inVector.x == 0.0f ? errVal : 1.0f/inVector.x );
		outVect.y = ( inVector.y == 0.0f ? errVal : 1.0f/inVector.y );
		return outVect;
	}
#endif // V2INVERTSAFE

#ifndef V2INVERTFAST
	__forceinline Vector_2_Out V2InvertFast(Vector_2_In inVector)
	{
		Vector_2 outVect;
		outVect.x = FPInvertFast(inVector.x);
		outVect.y = FPInvertFast(inVector.y);
		return outVect;
	}
#endif // V2INVERTFAST

#ifndef V2INVERTFASTSAFE
	__forceinline Vector_2_Out V2InvertFastSafe(Vector_2_In inVector, float errVal)
	{
		Vector_2 outVect;
		outVect.x = ( inVector.x == 0.0f ? errVal : FPInvertFast(inVector.x) );
		outVect.y = ( inVector.y == 0.0f ? errVal : FPInvertFast(inVector.y) );
		return outVect;
	}
#endif // V2INVERTFASTSAFE

#ifndef V2NORMALIZE
	__forceinline Vector_2_Out V2Normalize(Vector_2_In inVector)
	{
		Vector_2 outVect;
		float invMag = V2InvMag(inVector);
		outVect.x = inVector.x * invMag;
		outVect.y = inVector.y * invMag;
		return outVect;
	}
#endif // V2NORMALIZE

#ifndef V2NORMALIZESAFE
	__forceinline Vector_2_Out V2NormalizeSafe(Vector_2_In inVector, float errVal, float magSqThreshold)
	{
		//Vector_2 outVect = inVector;
		//float mag = V2Mag(inVector);
		//outVect.x = FPIfGtZeroThenElse(mag, inVector.x/mag, errValVect.x);
		//outVect.y = FPIfGtZeroThenElse(mag, inVector.y/mag, errValVect.y);
		//return outVect;

		Vector_2 outVect;
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
#endif // V2NORMALIZESAFE

#ifndef V2NORMALIZEFAST
	__forceinline Vector_2_Out V2NormalizeFast(Vector_2_In inVector)
	{
		Vector_2 outVect;
		float invMag = V2InvMagFast(inVector);
		outVect.x = inVector.x * invMag;
		outVect.y = inVector.y * invMag;
		return outVect;
	}
#endif // V2NORMALIZEFAST

#ifndef V2NORMALIZEFASTSAFE
	__forceinline Vector_2_Out V2NormalizeFastSafe(Vector_2_In inVector, float errVal, float magSqThreshold)
	{
		//Vector_2 outVect = inVector;
		//float mag = V2Mag(inVector);
		//outVect.x = FPIfGtZeroThenElse(mag, inVector.x/mag, errValVect.x);
		//outVect.y = FPIfGtZeroThenElse(mag, inVector.y/mag, errValVect.y);
		//return outVect;

		Vector_2 outVect;
		float mag2 = V2MagSquared(inVector);
		if(mag2 > magSqThreshold)
		{
			float invMag = FPInvSqrtFast(mag2);
			outVect.x = inVector.x*invMag;
			outVect.y = inVector.y*invMag;
		}
		else
		{
			outVect.x = outVect.y = errVal;
		}
		return outVect;
	}
#endif // V2NORMALIZEFASTSAFE

#ifndef V2DOT
	__forceinline float V2Dot(Vector_2_In a, Vector_2_In b)
	{
		return a.x*b.x + a.y*b.y;
	}
#endif // V2DOT

#ifndef V2DOTV
	__forceinline Vector_2_Out V2DotV(Vector_2_In a, Vector_2_In b)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2Dot(a, b);
		return outVect;
	}
#endif // V2DOTV

#ifndef V2CROSS
	__forceinline float V2Cross(Vector_2_In a, Vector_2_In b)
	{
		return (a.x * b.y - a.y * b.x);
	}
#endif // V2CROSS

#ifndef V2CROSSV
	__forceinline Vector_2_Out V2CrossV(Vector_2_In a, Vector_2_In b)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2Cross(a, b);
		return outVect;
	}
#endif // V2CROSSV

#ifndef V2AVERAGE
	__forceinline Vector_2_Out V2Average(Vector_2_In a, Vector_2_In b)
	{
		Vector_2 outVect;
		outVect.x = 0.5f * (a.x + b.x);
		outVect.y = 0.5f * (a.y + b.y);
		return outVect;
	}
#endif // V2AVERAGE

#ifndef V2LERP
	__forceinline Vector_2_Out V2Lerp( float t, Vector_2_In a, Vector_2_In b )
	{
		Vector_2 outVect;
		outVect.x = FPLerp(t, a.x, b.x);
		outVect.y = FPLerp(t, a.y, b.y);
		return outVect;
	}
#endif // V2LERP

#ifndef V2LERP_V
	__forceinline Vector_2_Out V2Lerp( Vector_2_In t, Vector_2_In a, Vector_2_In b )
	{
		Vector_2 outVect;
		outVect.x = FPLerp(t.x, a.x, b.x);
		outVect.y = FPLerp(t.y, a.y, b.y);
		return outVect;
	}
#endif // V2LERP_V

#ifndef V2POW
	__forceinline Vector_2_Out V2Pow( Vector_2_In x, Vector_2_In y )
	{
		Vector_2 outVect;
		outVect.x = FPPow(x.x, y.x);
		outVect.y = FPPow(x.y, y.y);
		return outVect;
	}
#endif // V2POW

#ifndef V2EXPT
	__forceinline Vector_2_Out V2Expt( Vector_2_In x )
	{
		Vector_2 outVect;
		outVect.x = FPExpt(x.x);
		outVect.y = FPExpt(x.y);
		return outVect;
	}
#endif // V2EXPT

#ifndef V2LOG2
	__forceinline Vector_2_Out V2Log2( Vector_2_In x )
	{
		
		Vector_2 outVect;
		outVect.x = FPLog2(x.x);
		outVect.y = FPLog2(x.y);
		return outVect;
	}
#endif

#ifndef V2LOG10
	__forceinline Vector_2_Out V2Log10( Vector_2_In x )
	{
		Vector_2 outVect;
		outVect.x = FPLog10(x.x);
		outVect.y = FPLog10(x.y);
		return outVect;
	}
#endif // V2LOG10

#ifndef V2SLOWINOUT
	__forceinline Vector_2_Out V2SlowInOut( Vector_2_In t )
	{
		Vector_2 outVect;
		outVect.x = FPSlowInOut( t.x );
		outVect.y = FPSlowInOut( t.y );
		return outVect;
	}
#endif // V2SLOWINOUT

#ifndef V2SLOWIN
	__forceinline Vector_2_Out V2SlowIn( Vector_2_In t )
	{
		Vector_2 outVect;
		outVect.x = FPSlowIn( t.x );
		outVect.y = FPSlowIn( t.y );
		return outVect;
	}
#endif // V2SLOWIN

#ifndef V2SLOWOUT
	__forceinline Vector_2_Out V2SlowOut( Vector_2_In t )
	{
		Vector_2 outVect;
		outVect.x = FPSlowOut( t.x );
		outVect.y = FPSlowOut( t.y );
		return outVect;
	}
#endif // V2SLOWOUT

#ifndef V2BELLINOUT
	__forceinline Vector_2_Out V2BellInOut( Vector_2_In t )
	{
		Vector_2 outVect;
		outVect.x = FPBellInOut( t.x );
		outVect.y = FPBellInOut( t.y );
		return outVect;
	}
#endif // V2BELLINOUT

#ifndef V2RAMP
	__forceinline Vector_2_Out V2Ramp( Vector_2_In x, Vector_2_In funcInA, Vector_2_In funcInB, Vector_2_In funcOutA, Vector_2_In funcOutB )
	{
		Vector_2 outVect;
		outVect.x = FPRamp( x.x, funcInA.x, funcInB.x, funcOutA.x, funcOutB.x );
		outVect.y = FPRamp( x.y, funcInA.y, funcInB.y, funcOutA.y, funcOutB.y );
		return outVect;
	}
#endif // V2RAMP

#ifndef V2RAMPFAST
	__forceinline Vector_2_Out V2RampFast( Vector_2_In x, Vector_2_In funcInA, Vector_2_In funcInB, Vector_2_In funcOutA, Vector_2_In funcOutB )
	{
		Vector_2 outVect;
		outVect.x = FPRampFast( x.x, funcInA.x, funcInB.x, funcOutA.x, funcOutB.x );
		outVect.y = FPRampFast( x.y, funcInA.y, funcInB.y, funcOutA.y, funcOutB.y );
		return outVect;
	}
#endif // V2RAMPFAST

#ifndef V2RANGE
	__forceinline Vector_2_Out V2Range( Vector_2_In t, Vector_2_In lower, Vector_2_In upper )
	{
		Vector_2 outVect;
		outVect.x = FPRange( t.x, lower.x, upper.x );
		outVect.y = FPRange( t.y, lower.y, upper.y );
		return outVect;
	}
#endif // V2RANGE

#ifndef V2RANGEFAST
	__forceinline Vector_2_Out V2RangeFast( Vector_2_In t, Vector_2_In lower, Vector_2_In upper )
	{
		Vector_2 outVect;
		outVect.x = FPRangeFast( t.x, lower.x, upper.x );
		outVect.y = FPRangeFast( t.y, lower.y, upper.y );
		return outVect;
	}
#endif // V2RANGEFAST

#ifndef V2RANGECLAMP
	__forceinline Vector_2_Out V2RangeClamp( Vector_2_In t, Vector_2_In lower, Vector_2_In upper )
	{
		Vector_2 outVect;
		outVect.x = FPRangeClamp( t.x, lower.x, upper.x );
		outVect.y = FPRangeClamp( t.y, lower.y, upper.y );
		return outVect;
	}
#endif // V2RANGECLAMP

#ifndef V2RANGECLAMPFAST
	__forceinline Vector_2_Out V2RangeClampFast( Vector_2_In t, Vector_2_In lower, Vector_2_In upper )
	{
		Vector_2 outVect;
		outVect.x = FPRangeClampFast( t.x, lower.x, upper.x );
		outVect.y = FPRangeClampFast( t.y, lower.y, upper.y );
		return outVect;
	}
#endif // V2RANGECLAMPFAST

	//============================================================================
	// Magnitude and distance

#ifndef V2SQRT
	__forceinline Vector_2_Out V2Sqrt( Vector_2_In v )
	{
		Vector_2 outVect;
		outVect.x = FPSqrt(v.x);
		outVect.y = FPSqrt(v.y);
		return outVect;
	}
#endif // V2SQRT

#ifndef V2SQRTSAFE
	__forceinline Vector_2_Out V2SqrtSafe( Vector_2_In v, float errVal )
	{
		Vector_2 outVect;
		outVect.x = FPSqrtSafe( v.x, errVal );
		outVect.y = FPSqrtSafe( v.y, errVal );
		return outVect;
	}
#endif // V2SQRTSAFE

#ifndef V2SQRTFAST
	__forceinline Vector_2_Out V2SqrtFast( Vector_2_In v )
	{
		return V2Sqrt( v );
	}
#endif // V2SQRTFAST

#ifndef V2SQRTFASTSAFE
	__forceinline Vector_2_Out V2SqrtFastSafe( Vector_2_In v, float errVal )
	{
		return V2SqrtSafe( v, errVal );
	}
#endif // V2SQRTFASTSAFE

#ifndef V2INVSQRT
	__forceinline Vector_2_Out V2InvSqrt( Vector_2_In v )
	{
		Vector_2 outVect;
		outVect.x = FPInvSqrt(v.x);
		outVect.y = FPInvSqrt(v.y);
		return outVect;
	}
#endif // V2INVSQRT

#ifndef V2INVSQRTSAFE
	__forceinline Vector_2_Out V2InvSqrtSafe( Vector_2_In v, float errVal )
	{
		Vector_2 outVect;
		outVect.x = FPInvSqrtSafe(v.x, errVal);
		outVect.y = FPInvSqrtSafe(v.y, errVal);
		return outVect;
	}
#endif // V2INVSQRTSAFE

#ifndef V2INVSQRTFAST
	__forceinline Vector_2_Out V2InvSqrtFast( Vector_2_In v )
	{
		Vector_2 outVect;
		outVect.x = FPInvSqrtFast(v.x);
		outVect.y = FPInvSqrtFast(v.y);
		return outVect;
	}
#endif // V2INVSQRTFAST

#ifndef V2INVSQRTFASTSAFE
	__forceinline Vector_2_Out V2InvSqrtFastSafe( Vector_2_In v, float errVal )
	{
		Vector_2 outVect;
		outVect.x = FPInvSqrtFastSafe(v.x, errVal);
		outVect.y = FPInvSqrtFastSafe(v.y, errVal);
		return outVect;
	}
#endif // V2INVSQRTFASTSAFE

#ifndef V2MAG
	__forceinline float V2Mag( Vector_2_In v )
	{
		return FPSqrt( V2Dot( v, v ) );
	}
#endif // V2MAG

#ifndef V2MAGV
	__forceinline Vector_2_Out V2MagV( Vector_2_In v )
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2Mag( v );
		return outVect;
	}
#endif // V2MAGV

#ifndef V2MAGFAST
	__forceinline float V2MagFast( Vector_2_In v )
	{
		return V2Mag( v );
	}
#endif // V2MAGFAST

#ifndef V2MAGVFAST
	__forceinline Vector_2_Out V2MagVFast( Vector_2_In v )
	{
		return V2MagV( v );
	}
#endif // V2MAGVFAST

#ifndef V2MAGSQUARED
	__forceinline float V2MagSquared( Vector_2_In v )
	{
		return V2Dot( v, v );
	}
#endif // V2MAGSQUARED

#ifndef V2MAGSQUAREDV
	__forceinline Vector_2_Out V2MagSquaredV( Vector_2_In v )
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2MagSquared( v );
		return outVect;
	}
#endif // V2MAGSQUAREDV

#ifndef V2INVMAG
	__forceinline float V2InvMag( Vector_2_In v )
	{
		return FPInvSqrt( V2Dot( v, v ) );
	}
#endif // V2INVMAG

#ifndef V2INVMAGSAFE
	__forceinline float V2InvMagSafe( Vector_2_In v, float errVal )
	{
		float dot = V2Dot( v, v );
		return FPInvSqrtSafe( dot, errVal );
	}
#endif // V2INVMAGSAFE

#ifndef V2INVMAGFAST
	__forceinline float V2InvMagFast( Vector_2_In v )
	{
		return FPInvSqrtFast( V2Dot( v, v ) );
	}
#endif // V2INVMAGFAST

#ifndef V2INVMAGFASTSAFE
	__forceinline float V2InvMagFastSafe( Vector_2_In v, float errVal )
	{
		return FPInvSqrtFastSafe( V2Dot( v, v ), errVal );
	}
#endif // V2INVMAGFASTSAFE

#ifndef V2INVMAGV
	__forceinline Vector_2_Out V2InvMagV( Vector_2_In v )
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvMag( v );
		return outVect;
	}
#endif // V2INVMAGV

#ifndef V2INVMAGVSAFE
	__forceinline Vector_2_Out V2InvMagVSafe( Vector_2_In v, float errVal )
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvMagSafe( v, errVal );
		return outVect;
	}
#endif // V2INVMAGVSAFE

#ifndef V2INVMAGVFAST
	__forceinline Vector_2_Out V2InvMagVFast( Vector_2_In v )
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvMagFast( v );
		return outVect;
	}
#endif // V2INVMAGVFAST

#ifndef V2INVMAGVFASTSAFE
	__forceinline Vector_2_Out V2InvMagVFastSafe( Vector_2_In v, float errVal )
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvMagFastSafe( v, errVal );
		return outVect;
	}
#endif // V2INVMAGVFASTSAFE

#ifndef V2INVMAGSQUARED
	__forceinline float V2InvMagSquared( Vector_2_In v )
	{
		return 1.0f/V2MagSquared( v );
	}
#endif // V2INVMAGSQUARED

#ifndef V2INVMAGSQUAREDSAFE
	__forceinline float V2InvMagSquaredSafe( Vector_2_In v, float errVal )
	{
		float magSq = V2MagSquared( v );
		return FPIfGtZeroThenElse( magSq, 1.0f/magSq, errVal );
	}
#endif // V2INVMAGSQUAREDSAFE

#ifndef V2INVMAGSQUAREDFAST
	__forceinline float V2InvMagSquaredFast( Vector_2_In v )
	{
		return FPInvertFast( V2MagSquared( v ) );
	}
#endif // V2INVMAGSQUAREDFAST

#ifndef V2INVMAGSQUAREDFASTSAFE
	__forceinline float V2InvMagSquaredFastSafe( Vector_2_In v, float errVal )
	{
		return FPInvertFastSafe( V2MagSquared( v ), errVal );
	}
#endif // V2INVMAGSQUAREDFASTSAFE

#ifndef V2INVMAGSQUAREDV
	__forceinline Vector_2_Out V2InvMagSquaredV( Vector_2_In v )
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvMagSquared( v );
		return outVect;
	}
#endif // V2INVMAGSQUAREDV

#ifndef V2INVMAGSQUAREDVSAFE
	__forceinline Vector_2_Out V2InvMagSquaredVSafe( Vector_2_In v, float errVal )
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvMagSquaredSafe( v, errVal );
		return outVect;
	}
#endif // V2INVMAGSQUAREDVSAFE

#ifndef V2INVMAGSQUAREDVFAST
	__forceinline Vector_2_Out V2InvMagSquaredVFast( Vector_2_In v )
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvMagSquaredFast( v );
		return outVect;
	}
#endif // V2INVMAGSQUAREDVFAST

#ifndef V2INVMAGSQUAREDVFASTSAFE
	__forceinline Vector_2_Out V2InvMagSquaredVFastSafe( Vector_2_In v, float errVal )
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvMagSquaredFastSafe( v, errVal );
		return outVect;
	}
#endif // V2INVMAGSQUAREDVFASTSAFE

#ifndef V2DIST
	__forceinline float V2Dist(Vector_2_In a, Vector_2_In b)
	{
		return V2Mag( V2Subtract( a, b ) );
	}
#endif // V2DIST

#ifndef V2DISTFAST
	__forceinline float V2DistFast(Vector_2_In a, Vector_2_In b)
	{
		return V2Dist( a, b );
	}
#endif // V2DISTFAST

#ifndef V2DISTV
	__forceinline Vector_2_Out V2DistV(Vector_2_In a, Vector_2_In b)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2Dist( a, b );
		return outVect;
	}
#endif // V2DISTV

#ifndef V2DISTVFAST
	__forceinline Vector_2_Out V2DistVFast(Vector_2_In a, Vector_2_In b)
	{
		return V2DistV( a, b );
	}
#endif // V2DISTVFAST

#ifndef V2INVDIST
	__forceinline float V2InvDist(Vector_2_In a, Vector_2_In b)
	{
		return V2InvMag( V2Subtract( a, b ) );
	}
#endif // V2INVDIST

#ifndef V2INVDISTSAFE
	__forceinline float V2InvDistSafe(Vector_2_In a, Vector_2_In b, float errVal)
	{
		return V2InvMagSafe( V2Subtract( a, b ), errVal );
	}
#endif // V2INVDISTSAFE

#ifndef V2INVDISTFAST
	__forceinline float V2InvDistFast(Vector_2_In a, Vector_2_In b)
	{
		return V2InvMagFast( V2Subtract( a, b ) );
	}
#endif // V2INVDISTFAST

#ifndef V2INVDISTFASTSAFE
	__forceinline float V2InvDistFastSafe(Vector_2_In a, Vector_2_In b, float errVal)
	{
		return V2InvMagFastSafe( V2Subtract( a, b ), errVal );
	}
#endif // V2INVDISTFASTSAFE

#ifndef V2INVDISTV
	__forceinline Vector_2_Out V2InvDistV(Vector_2_In a, Vector_2_In b)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvDist( a, b );
		return outVect;
	}
#endif // V2INVDISTV

#ifndef V2INVDISTVSAFE
	__forceinline Vector_2_Out V2InvDistVSafe(Vector_2_In a, Vector_2_In b, float errVal)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvDistSafe( a, b, errVal );
		return outVect;
	}
#endif // V2INVDISTVSAFE

#ifndef V2INVDISTVFAST
	__forceinline Vector_2_Out V2InvDistVFast(Vector_2_In a, Vector_2_In b)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvDistFast( a, b );
		return outVect;
	}
#endif // V2INVDISTVFAST

#ifndef V2INVDISTVFASTSAFE
	__forceinline Vector_2_Out V2InvDistVFastSafe(Vector_2_In a, Vector_2_In b, float errVal)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvDistFastSafe( a, b, errVal );
		return outVect;
	}
#endif // V2INVDISTVFASTSAFE

#ifndef V2DISTSQUARED
	__forceinline float V2DistSquared(Vector_2_In a, Vector_2_In b)
	{
		return V2MagSquared( V2Subtract( a, b ) );
	}
#endif // V2DISTSQUARED

#ifndef V2DISTSQUAREDV
	__forceinline Vector_2_Out V2DistSquaredV(Vector_2_In a, Vector_2_In b)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2DistSquared( a, b );
		return outVect;
	}
#endif // V2DISTSQUAREDV

#ifndef V2INVDISTSQUARED
	__forceinline float V2InvDistSquared(Vector_2_In a, Vector_2_In b)
	{
		return V2InvMagSquared( V2Subtract( a, b ) );
	}
#endif // V2INVDISTSQUARED

#ifndef V2INVDISTSQUAREDSAFE
	__forceinline float V2InvDistSquaredSafe(Vector_2_In a, Vector_2_In b, float errVal)
	{
		return V2InvMagSquaredSafe( V2Subtract( a, b ), errVal );
	}
#endif // V2INVDISTSQUAREDSAFE

#ifndef V2INVDISTSQUAREDFAST
	__forceinline float V2InvDistSquaredFast(Vector_2_In a, Vector_2_In b)
	{
		return V2InvMagSquaredFast( V2Subtract( a, b ) );
	}
#endif // V2INVDISTSQUAREDFAST

#ifndef V2INVDISTSQUAREDFASTSAFE
	__forceinline float V2InvDistSquaredFastSafe(Vector_2_In a, Vector_2_In b, float errVal)
	{
		return V2InvMagSquaredFastSafe( V2Subtract( a, b ), errVal );
	}
#endif // V2INVDISTSQUAREDFASTSAFE

#ifndef V2INVDISTSQUAREDV
	__forceinline Vector_2_Out V2InvDistSquaredV(Vector_2_In a, Vector_2_In b)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvDistSquared( a, b );
		return outVect;
	}
#endif // V2INVDISTSQUAREDV

#ifndef V2INVDISTSQUAREDVSAFE
	__forceinline Vector_2_Out V2InvDistSquaredVSafe(Vector_2_In a, Vector_2_In b, float errVal)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvDistSquaredSafe( a, b, errVal );
		return outVect;
	}
#endif // V2INVDISTSQUAREDVSAFE

#ifndef V2INVDISTSQUAREDVFAST
	__forceinline Vector_2_Out V2InvDistSquaredVFast(Vector_2_In a, Vector_2_In b)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvDistSquaredFast( a, b );
		return outVect;
	}
#endif // V2INVDISTSQUAREDVFAST

#ifndef V2INVDISTSQUAREDVFASTSAFE
	__forceinline Vector_2_Out V2InvDistSquaredVFastSafe(Vector_2_In a, Vector_2_In b, float errVal)
	{
		Vector_2 outVect;
		outVect.x = outVect.y = V2InvDistSquaredFastSafe( a, b, errVal );
		return outVect;
	}
#endif // V2INVDISTSQUAREDVFASTSAFE

	//============================================================================
	// Conversion functions

#ifndef V2FLOATTOINTRAW
	template <int exponent>
	__forceinline Vector_2_Out V2FloatToIntRaw(Vector_2_In inVector)
	{
		float multiplier = static_cast<float>(1 << exponent);

		Vector_2 outVect;

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
#endif // V2FLOATTOINTRAW

#ifndef V2INTTOFLOATRAW
	template <int exponent>
	__forceinline Vector_2_Out V2IntToFloatRaw(Vector_2_In inVector)
	{
		float divider = static_cast<float>(1 << exponent);
		float invDiv = 1.0f/divider;

		Vector_2 outVect;

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
#endif // V2INTTOFLOATRAW

#ifndef V2ROUNDTONEARESTINT
	__forceinline Vector_2_Out V2RoundToNearestInt(Vector_2_In inVector)
	{
		Vector_2 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x+(FPIfGteZeroThenElse(inVector.x, 0.5f, -0.5f))) );
		outVect.y = static_cast<float>( static_cast<int>(inVector.y+(FPIfGteZeroThenElse(inVector.y, 0.5f, -0.5f))) );
		return outVect;
	}
#endif // V2ROUNDTONEARESTINT

#ifndef V2ROUNDTONEARESTINTZERO
	__forceinline Vector_2_Out V2RoundToNearestIntZero(Vector_2_In inVector)
	{
		Vector_2 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x) );
		outVect.y = static_cast<float>( static_cast<int>(inVector.y) );
		return outVect;
	}
#endif // V2ROUNDTONEARESTINTZERO

#ifndef V2ROUNDTONEARESTINTNEGINF
	__forceinline Vector_2_Out V2RoundToNearestIntNegInf(Vector_2_In inVector)
	{
		Vector_2 outVect;
		outVect.x = FPFloor( inVector.x );
		outVect.y = FPFloor( inVector.y );
		return outVect;
	}
#endif // V2ROUNDTONEARESTINTNEGINF

#ifndef V2ROUNDTONEARESTINTPOSINF
	__forceinline Vector_2_Out V2RoundToNearestIntPosInf(Vector_2_In inVector)
	{
		Vector_2 outVect;
		outVect.x = FPCeil( inVector.x );
		outVect.y = FPCeil( inVector.y );
		return outVect;
	}
#endif // V2ROUNDTONEARESTINTPOSINF

	//============================================================================
	// Comparison functions

#ifndef V2ISBETWEENNEGANDPOSBOUNDS
	__forceinline unsigned int V2IsBetweenNegAndPosBounds( Vector_2_In testVector, Vector_2_In boundsVector )
	{
		return (	testVector.x <= boundsVector.x && testVector.x >= -boundsVector.x &&
					testVector.y <= boundsVector.y && testVector.y >= -boundsVector.y  ? 1u : 0u );
	}
#endif // V2ISBETWEENNEGANDPOSBOUNDS

#ifndef V2ISZEROALL
	__forceinline unsigned int V2IsZeroAll(Vector_2_In inVector)
	{
		return ( inVector.x == 0.0f && inVector.y == 0.0f ? 1u : 0u );
	}
#endif // V2ISZEROALL

#ifndef V2ISZERONONE
	__forceinline unsigned int V2IsZeroNone(Vector_2_In inVector)
	{
		return ( inVector.x != 0.0f && inVector.y != 0.0f ? 1u : 0u );
	}
#endif // V2ISZERONONE

#ifndef V2ISEQUALALL
	__forceinline unsigned int V2IsEqualAll(Vector_2_In inVector1, Vector_2_In inVector2)
	{
		return ( inVector1.x == inVector2.x &&
				inVector1.y == inVector2.y	? 1u : 0u );
	}
#endif // V2ISEQUALALL

#ifndef V2ISEQUALNONE
	__forceinline unsigned int V2IsEqualNone(Vector_2_In inVector1, Vector_2_In inVector2)
	{
		return ( inVector1.x != inVector2.x &&
				inVector1.y != inVector2.y	? 1u : 0u );
	}
#endif // V2ISEQUALNONE

#ifndef V2ISEQUALV
	__forceinline Vector_2_Out V2IsEqualV(Vector_2_In inVector1, Vector_2_In inVector2)
	{
		Vector_2 outVect;
		outVect.x = (inVector1.x == inVector2.x ? allBitsF : 0.0f);
		outVect.y = (inVector1.y == inVector2.y ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V2ISEQUALV

#ifndef V2ISEQUALINTALL
	__forceinline unsigned int V2IsEqualIntAll(Vector_2_In inVector1, Vector_2_In inVector2)
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
#endif // V2ISEQUALINTALL

#ifndef V2ISEQUALINTNONE
	__forceinline unsigned int V2IsEqualIntNone(Vector_2_In inVector1, Vector_2_In inVector2)
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
#endif // V2ISEQUALINTNONE

#ifndef V2ISEQUALINTV
	__forceinline Vector_2_Out V2IsEqualIntV(Vector_2_In inVector1, Vector_2_In inVector2)
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

		Vector_2 outVect;
		outVect.x = (Temp1.i == Temp2.i ? allBitsF : 0.0f);
		outVect.y = (Temp3.i == Temp4.i ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V2ISEQUALINTV

#ifndef V2ISCLOSEV
	__forceinline Vector_2_Out V2IsCloseV(Vector_2_In inVector1, Vector_2_In inVector2, float eps)
	{
		Vector_2 outVect;
		outVect.x = (( inVector1.x >= inVector2.x - eps && inVector1.x <= inVector2.x + eps ) ? allBitsF : 0.0f);
		outVect.y = (( inVector1.y >= inVector2.y - eps && inVector1.y <= inVector2.y + eps ) ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V2ISCLOSEV

#ifndef V2ISCLOSEV_V
	__forceinline Vector_2_Out V2IsCloseV(Vector_2_In inVector1, Vector_2_In inVector2, Vector_2_In eps)
	{
		Vector_2 outVect;
		outVect.x = (( inVector1.x >= inVector2.x - eps.x && inVector1.x <= inVector2.x + eps.x ) ? allBitsF : 0.0f);
		outVect.y = (( inVector1.y >= inVector2.y - eps.y && inVector1.y <= inVector2.y + eps.y ) ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V2ISCLOSEV_V

#ifndef V2ISCLOSEALL
	__forceinline unsigned int V2IsCloseAll(Vector_2_In inVector1, Vector_2_In inVector2, float eps)
	{
		return ( ( inVector1.x >= inVector2.x - eps && inVector1.x <= inVector2.x + eps ) &&
				( inVector1.y >= inVector2.y - eps && inVector1.y <= inVector2.y + eps ) ? 1u : 0u);
	}
#endif // V2ISCLOSEALL

#ifndef V2ISCLOSENONE
	__forceinline unsigned int V2IsCloseNone(Vector_2_In inVector1, Vector_2_In inVector2, float eps)
	{
		return (( inVector1.x < inVector2.x - eps || inVector1.x > inVector2.x + eps ) &&
				( inVector1.y < inVector2.y - eps || inVector1.y > inVector2.y + eps ) ? 1u : 0u);
	}
#endif // V2ISCLOSENONE

#ifndef V2ISCLOSEALL_V
	__forceinline unsigned int V2IsCloseAll(Vector_2_In inVector1, Vector_2_In inVector2, Vector_2_In eps)
	{
		return (( inVector1.x >= inVector2.x - eps.x && inVector1.x <= inVector2.x + eps.x ) &&
				( inVector1.y >= inVector2.y - eps.y && inVector1.y <= inVector2.y + eps.y ) ? 1u : 0u);
	}
#endif // V2ISCLOSEALL_V

#ifndef V2ISCLOSENONE_V
	__forceinline unsigned int V2IsCloseNone(Vector_2_In inVector1, Vector_2_In inVector2, Vector_2_In eps)
	{
		return (( inVector1.x < inVector2.x - eps.x || inVector1.x > inVector2.x + eps.x ) &&
				( inVector1.y < inVector2.y - eps.y || inVector1.y > inVector2.y + eps.y ) ? 1u : 0u);
	}
#endif // V2ISCLOSENONE_V

#ifndef V2ISGREATERTHANALL
	__forceinline unsigned int V2IsGreaterThanAll(Vector_2_In bigVector, Vector_2_In smallVector)
	{
		return (( bigVector.x > smallVector.x ) &&
				( bigVector.y > smallVector.y ) ? 1u : 0u );
	}
#endif // V2ISGREATERTHANALL

#ifndef V2ISGREATERTHANV
	__forceinline Vector_2_Out V2IsGreaterThanV(Vector_2_In bigVector, Vector_2_In smallVector)
	{
		Vector_2 outVect;
		outVect.x = (bigVector.x > smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y > smallVector.y ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V2ISGREATERTHANV

#ifndef V2ISGREATERTHANOREQUALALL
	__forceinline unsigned int V2IsGreaterThanOrEqualAll(Vector_2_In bigVector, Vector_2_In smallVector)
	{
		return (( bigVector.x >= smallVector.x ) &&
				( bigVector.y >= smallVector.y ) ? 1u : 0u);
	}
#endif // V2ISGREATERTHANOREQUALALL

#ifndef V2ISGREATERTHANOREQUALV
	__forceinline Vector_2_Out V2IsGreaterThanOrEqualV(Vector_2_In bigVector, Vector_2_In smallVector)
	{
		Vector_2 outVect;
		outVect.x = (bigVector.x >= smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y >= smallVector.y ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V2ISGREATERTHANOREQUALV

#ifndef V2ISLESSTHANALL
	__forceinline unsigned int V2IsLessThanAll(Vector_2_In smallVector, Vector_2_In bigVector)
	{
		return (( bigVector.x > smallVector.x ) &&
				( bigVector.y > smallVector.y ) ? 1u : 0u);
	}
#endif // V2ISLESSTHANALL

#ifndef V2ISLESSTHANV
	__forceinline Vector_2_Out V2IsLessThanV(Vector_2_In smallVector, Vector_2_In bigVector)
	{
		Vector_2 outVect;
		outVect.x = (bigVector.x > smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y > smallVector.y ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V2ISLESSTHANV

#ifndef V2ISLESSTHANOREQUALALL
	__forceinline unsigned int V2IsLessThanOrEqualAll(Vector_2_In smallVector, Vector_2_In bigVector)
	{
		return (( bigVector.x >= smallVector.x ) &&
				( bigVector.y >= smallVector.y ) ? 1u : 0u);
	}
#endif // V2ISLESSTHANOREQUALALL

#ifndef V2ISLESSTHANOREQUALV
	__forceinline Vector_2_Out V2IsLessThanOrEqualV(Vector_2_In smallVector, Vector_2_In bigVector)
	{
		Vector_2 outVect;
		outVect.x = (bigVector.x >= smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y >= smallVector.y ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V2ISLESSTHANOREQUALV

#ifndef V2SELECT
	__forceinline Vector_2_Out V2SelectFT(Vector_2_In choiceVector, Vector_2_In zero, Vector_2_In nonZero)
	{
		Vector_2 outVect;

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
#endif // V2SELECT

#ifndef V2SELECTVECT
	__forceinline Vector_2_Out V2SelectVect(Vector_2_In choiceVectorX, Vector_2_In zero, Vector_2_In nonZero)
	{
		return (GetX(choiceVectorX) == 0.0f ? zero : nonZero );
	}
#endif // V2SELECTVECT

#ifndef V2MAX
	__forceinline Vector_2_Out V2Max(Vector_2_In inVector1, Vector_2_In inVector2)
	{
		Vector_2 outVect;
		outVect.x = FPMax( inVector1.x, inVector2.x );
		outVect.y = FPMax( inVector1.y, inVector2.y );
		return outVect;
	}
#endif // V2MAX()

#ifndef V2MIN
	__forceinline Vector_2_Out V2Min(Vector_2_In inVector1, Vector_2_In inVector2)
	{
		Vector_2 outVect;
		outVect.x = FPMin( inVector1.x, inVector2.x );
		outVect.y = FPMin( inVector1.y, inVector2.y );
		return outVect;
	}
#endif // V2MIN

#ifndef V2CLAMP
	__forceinline Vector_2_Out V2Clamp( Vector_2_In inVector, Vector_2_In lowBound, Vector_2_In highBound )
	{
		Vector_2 outVect;
		outVect.x = FPClamp( inVector.x, lowBound.x, highBound.x );
		outVect.y = FPClamp( inVector.y, lowBound.y, highBound.y );
		return outVect;
	}
#endif // V2CLAMP

#ifndef V2SATURATE
	__forceinline Vector_2_Out V2Saturate( Vector_2_In inVector )
	{
		Vector_2 outVect;
		outVect.x = FPClamp( inVector.x, 0.0f, 1.0f );
		outVect.y = FPClamp( inVector.y, 0.0f, 1.0f );
		return outVect;
	}
#endif // V2CLAMP

	//============================================================================
	// Bitwise operations

#ifndef V2SHIFTLEFT
	template <int shift>
	__forceinline Vector_2_Out V2ShiftLeft( Vector_2_In inVector )
	{
		Vector_2 outVect;

		union
		{
			float f;
			u32 i;
		} TempResult1, TempResult2;

		TempResult1.f = inVector.x;
		TempResult2.f = inVector.y;
		TempResult1.i <<= shift;
		TempResult2.i <<= shift;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		return outVect;
	}
#endif // V2SHIFTLEFT

#ifndef V2SHIFTRIGHT
	template <int shift>
	__forceinline Vector_2_Out V2ShiftRight( Vector_2_In inVector )
	{
		Vector_2 outVect;

		union
		{
			float f;
			u32 i;
		} TempResult1, TempResult2;

		TempResult1.f = inVector.x;
		TempResult2.f = inVector.y;
		TempResult1.i >>= shift;
		TempResult2.i >>= shift;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		return outVect;
	}
#endif // V2SHIFTRIGHT

#ifndef V2SHIFTRIGHTALGEBRAIC
	template <int shift>
	__forceinline Vector_2_Out V2ShiftRightAlgebraic( Vector_2_In inVector )
	{
		Vector_2 outVect;

		union
		{
			float f;
			s32 i;
		} TempResult1, TempResult2;

		TempResult1.f = inVector.x;
		TempResult2.f = inVector.y;
		TempResult1.i >>= shift;
		TempResult2.i >>= shift;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		return outVect;
	}
#endif // V2SHIFTRIGHTALGEBRAIC

#ifndef V2AND
	__forceinline Vector_2_Out V2And(Vector_2_In inVector1, Vector_2_In inVector2)
	{
		Vector_2 outVect;

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
#endif // V2AND

#ifndef V2OR
	__forceinline Vector_2_Out V2Or(Vector_2_In inVector1, Vector_2_In inVector2)
	{
		Vector_2 outVect;

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
#endif // V2OR

#ifndef V2XOR
	__forceinline Vector_2_Out V2Xor(Vector_2_In inVector1, Vector_2_In inVector2)
	{
		Vector_2 outVect;

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
#endif // V2XOR

#ifndef V2ANDC
	__forceinline Vector_2_Out V2Andc(Vector_2_In inVector1, Vector_2_In inVector2)
	{
		Vector_2 outVect;

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
#endif // V2ANDC


} // namespace Vec
} // namespace rage

