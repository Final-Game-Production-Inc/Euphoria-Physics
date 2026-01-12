
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

namespace rage
{
namespace Vec
{

#ifndef V3SET_3F__4
	__forceinline void V3Set( Vector_4_InOut inoutVector, float x0, float y0, float z0 )
	{
		inoutVector.x = x0;
		inoutVector.y = y0;
		inoutVector.z = z0;
	}
#endif // V3SET_3F__4

#ifndef V3SET_V__4
	__forceinline void V3Set( Vector_4_InOut inoutVector, Vector_4_In inVector )
	{
		inoutVector.x = inVector.x;
		inoutVector.y = inVector.y;
		inoutVector.z = inVector.z;
	}
#endif // V3SET_V__4

#ifndef V3SET__4
	__forceinline void V3Set( Vector_4_InOut inoutVector, float s )
	{
		inoutVector.x = s;
		inoutVector.y = s;
		inoutVector.z = s;
	}
#endif // V3SET__4

#ifndef V3ZEROCOMPONENTS__4
	__forceinline void V3ZeroComponents( Vector_4_InOut inoutVector )
	{
		inoutVector.x = inoutVector.y = inoutVector.z = 0.0f;
	}
#endif // V3ZEROCOMPONENTS__4

	//============================================================================
	// Standard Algebra

#ifndef V3ADDNET__4
	__forceinline Vector_4_Out V3AddNet( Vector_4_In inVector, Vector_4_In toAdd )
	{
		Vector_4 modifiedAdd = toAdd;
		float dot = V3Dot(inVector, toAdd);
		if( dot > 0.0f )
		{
			// The given vector has a positive dot product with this vector, so remove its parallel component.
			modifiedAdd = V3SubtractScaled( toAdd, inVector, dot*V3InvMagSquared( inVector ) );
		}

		// Add the given vector to this vector, less any positive parallel component.
		return V3Add( inVector, modifiedAdd );
	}
#endif // V3ADDNET__4

#ifndef V3EXTEND__4
	__forceinline Vector_4_Out V3Extend( Vector_4_In inVector, Vector_4_In amount )
	{
		float invMag = V3InvMag( inVector );
		Vector_4 scaleValue;
		scaleValue = V3AddScaled( V4Constant(V_ONE), amount, invMag );
		return V3Scale( inVector, scaleValue );
	}
#endif // V3EXTEND__4

#ifndef V3ANGLEV__4
	__forceinline Vector_4_Out V3AngleV(Vector_4_In v1, Vector_4_In v2)
	{
		float magSquaredProduct = V3MagSquared(v1) * V3MagSquared(v2);
		float invMag = FPInvSqrt(magSquaredProduct);
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = FPACos( V3Dot( v1, v2 ) * invMag );
		return outVect;
	}
#endif // V3ANGLEV__4

#ifndef V3ANGLEVNORMINPUT__4
	__forceinline Vector_4_Out V3AngleVNormInput(Vector_4_In v1, Vector_4_In v2)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = FPACos( V3Dot( v1, v2 ) );
		return outVect;
	}
#endif // V3ANGLEVNORMINPUT__4

#ifndef V3ANGLEXV__4
	__forceinline Vector_4_Out V3AngleXV(Vector_4_In v1, Vector_4_In v2)
	{
		float mag2 = (v1.y * v1.y + v1.z * v1.z) * (v2.y * v2.y + v2.z * v2.z);
		float invMag = FPInvSqrt( mag2 );
		float valToClamp = (v1.y*v2.y + v1.z*v2.z)*invMag;

		float retVal = FPIfGteZeroThenElse(v1.y*v2.z-v1.z*v2.y, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = retVal;
		return outVect;
	}

#endif // V3ANGLEXV__4

#ifndef V3ANGLEYV__4
	__forceinline Vector_4_Out V3AngleYV(Vector_4_In v1, Vector_4_In v2)
	{
		float mag2 = (v1.x * v1.x + v1.z * v1.z) * (v2.x * v2.x + v2.z * v2.z);
		float invMag = FPInvSqrt( mag2 );
		float valToClamp = (v1.x * v2.x + v1.z * v2.z) * invMag;

		float retVal = FPIfGteZeroThenElse(v1.z*v2.x-v1.x*v2.z, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = retVal;
		return outVect;
	}
#endif // V3ANGLEYV__4

#ifndef V3ANGLEZV__4
	__forceinline Vector_4_Out V3AngleZV(Vector_4_In v1, Vector_4_In v2)
	{
		float mag2 = (v1.x * v1.x + v1.y * v1.y) * (v2.x * v2.x + v2.y * v2.y);
		float invMag = FPInvSqrt( mag2 );
		float valToClamp = (v1.x * v2.x + v1.y * v2.y) * invMag;

		float retVal = FPIfGteZeroThenElse(v1.x*v2.y-v1.y*v2.x, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = retVal;
		return outVect;
	}
#endif // V3ANGLEZV__4

#ifndef V3ANGLEXVNORMINPUT__4
	__forceinline Vector_4_Out V3AngleXVNormInput(Vector_4_In v1, Vector_4_In v2)
	{
		float valToClamp = (v1.y*v2.y + v1.z*v2.z);

		float retVal = FPIfGteZeroThenElse(v1.y*v2.z-v1.z*v2.y, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = retVal;
		return outVect;
	}
#endif // V3ANGLEXVNORMINPUT__4

#ifndef V3ANGLEYVNORMINPUT__4
	__forceinline Vector_4_Out V3AngleYVNormInput(Vector_4_In v1, Vector_4_In v2)
	{
		float valToClamp = (v1.x * v2.x + v1.z * v2.z);

		float retVal = FPIfGteZeroThenElse(v1.z*v2.x-v1.x*v2.z, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = retVal;
		return outVect;
	}
#endif // V3ANGLEYVNORMINPUT__4

#ifndef V3ANGLEZVNORMINPUT__4
	__forceinline Vector_4_Out V3AngleZVNormInput(Vector_4_In v1, Vector_4_In v2)
	{
		float valToClamp = (v1.x * v2.x + v1.y * v2.y);

		float retVal = FPIfGteZeroThenElse(v1.x*v2.y-v1.y*v2.x, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = retVal;
		return outVect;
	}
#endif // V3ANGLEZVNORMINPUT__4

#ifndef V3APPROACHSTRAIGHT__4
	__forceinline Vector_4_Out V3ApproachStraight(Vector_4_In position, Vector_4_In goal, float rate, float time, unsigned int& rResult)
	{
		Vector_4 directionXYZ = V3Subtract( goal, position );
		Vector_4 unitDirectionXYZ = V3Normalize( directionXYZ );
		float scalarDistance = rate * time;
		Vector_4 finalPosition = V3AddScaled( position, unitDirectionXYZ, scalarDistance );

		Vector_4 directionXYZNew = V3Subtract( goal, finalPosition );
		Vector_4 unitDirectionXYZNew = V3Normalize( directionXYZNew );

		unsigned int haventReachedGoal = V3IsCloseAll( unitDirectionXYZ, unitDirectionXYZNew, SMALL_FLOAT );
		rResult = (haventReachedGoal ^ 0x1);

		return finalPosition;
	}
#endif // V3APPROACHSTRAIGHT__4

#ifndef V3MAKEORTHONORMALS__4
	__forceinline void V3MakeOrthonormals(Vector_4_In inVector, Vector_4_InOut ortho1, Vector_4_InOut ortho2)
	{
		// Make sure this vector has a length near 1.
		mthAssertf( FPAbs( V3MagSquared(inVector) - 1.0f ) < SMALL_FLOAT, "InVector must have length near 1.0" );

		// See which coordinate axis is most parallel to this vector.
		if( FPAbs(inVector.x) >= FPAbs(inVector.y) )
		{
			// This vector is as close or closer to the x axis than to the y axis, so it must be closest to either x or z.
			// Make the second unit axis vector equal to this cross y.
			ortho1.x = -inVector.z;
			ortho1.y = 0.0f;
			ortho1.z = inVector.x;
		}
		else
		{
			// This vector is closer to the y axis than to the x axis, so it must be closest to either y or z.
			// Make the second unit axis vector equal to this cross x.
			ortho1.x = 0.0f;
			ortho1.y = inVector.z;
			ortho1.z = -inVector.y;
		}

		// Normalize the second unit axis vector.
		ortho1 = V3Normalize( ortho1 );
		mthAssertf( FPAbs( V3MagSquared(ortho1) - 1.0f ) < SMALL_FLOAT, "Couldn't normalize second unit vector");

		// Make the third unit axis vector the cross product of the first two.
		ortho2 = V3Cross( inVector, ortho1 );
		mthAssertf( FPAbs( V3MagSquared(ortho2) - 1.0f ) < SMALL_FLOAT, "Couldn't normalize third unit vector");
	}
#endif // V3MAKEORTHONORMALS__4

#ifndef V3REFLECT__4
	__forceinline Vector_4_Out V3Reflect( Vector_4_In inVector, Vector_4_In planeNormal )
	{
		float dot = V3Dot( inVector, planeNormal );
		Vector_4 outVect;
		outVect.x = (inVector.x - 2.0f*dot*planeNormal.x);
		outVect.y = (inVector.y - 2.0f*dot*planeNormal.y);
		outVect.z = (inVector.z - 2.0f*dot*planeNormal.z);
		return outVect;
	}
#endif // V3REFLECT__4

#ifndef V3ROTATEABOUTXAXIS__4
	__forceinline Vector_4_Out V3RotateAboutXAxis( Vector_4_In inVector, float radians )
	{
		float tsin, tcos;
		FPSinAndCos(tsin, tcos, radians);

		Vector_4 outVect;
		outVect.x = inVector.x;
		outVect.y = inVector.y * tcos - inVector.z * tsin;
		outVect.z = inVector.y * tsin + inVector.z * tcos;
		return outVect;
	}
#endif // V3ROTATEABOUTXAXIS__4

#ifndef V3ROTATEABOUTYAXIS__4
	__forceinline Vector_4_Out V3RotateAboutYAxis( Vector_4_In inVector, float radians )
	{
		float tsin, tcos;
		FPSinAndCos(tsin, tcos, radians);

		Vector_4 outVect;
		outVect.x = inVector.z * tsin + inVector.x * tcos;
		outVect.y = inVector.y;
		outVect.z = inVector.z * tcos - inVector.x * tsin;
		return outVect;
	}
#endif // V3ROTATEABOUTYAXIS__4

#ifndef V3ROTATEABOUTZAXIS__4
	__forceinline Vector_4_Out V3RotateAboutZAxis( Vector_4_In inVector, float radians )
	{
		float tsin, tcos;
		FPSinAndCos(tsin, tcos, radians);

		Vector_4 outVect;
		outVect.x = inVector.x * tcos - inVector.y * tsin;
		outVect.y = inVector.x * tsin + inVector.y * tcos;
		outVect.z = inVector.z;
		return outVect;
	}
#endif // V3ROTATEABOUTZAXIS__4

#ifndef V3SCALE__4
	__forceinline Vector_4_Out V3Scale( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x * floatVal;
		outVect.y = inVector.y * floatVal;
		outVect.z = inVector.z * floatVal;
		return outVect;
	}
#endif // V3SCALE__4

#ifndef V3INVSCALE__4
	__forceinline Vector_4_Out V3InvScale( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		float invVal = 1.0f/floatVal;
		outVect.x = inVector.x * invVal;
		outVect.y = inVector.y * invVal;
		outVect.z = inVector.z * invVal;
		return outVect;
	}
#endif // V3INVSCALE__4

#ifndef V3INVSCALE_V__4
	__forceinline Vector_4_Out V3InvScale( Vector_4_In inVector, Vector_4_In floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x / floatVal.x;
		outVect.y = inVector.y / floatVal.y;
		outVect.z = inVector.z / floatVal.z;
		return outVect;
	}
#endif // V3INVSCALE_V__4

#ifndef V3INVSCALESAFE__4
	__forceinline Vector_4_Out V3InvScaleSafe( Vector_4_In inVector, float floatVal, float errVal )
	{
		Vector_4 outVect;
		if( floatVal != 0.0f )
		{
			float invVal = 1.0f/floatVal;
			outVect.x = inVector.x * invVal;
			outVect.y = inVector.y * invVal;
			outVect.z = inVector.z * invVal;
		}
		else
		{
			outVect.x = outVect.y = outVect.z = errVal;
		}
		return outVect;
	}
#endif // V3INVSCALESAFE__4

#ifndef V3INVSCALESAFE_V__4
	__forceinline Vector_4_Out V3InvScaleSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal  )
	{
		Vector_4 outVect;
		outVect.x = (floatVal.x != 0.0f) ? inVector.x/floatVal.x : errVal;
		outVect.y = (floatVal.y != 0.0f) ? inVector.y/floatVal.y : errVal;
		outVect.z = (floatVal.z != 0.0f) ? inVector.z/floatVal.z : errVal;
		return outVect;
	}
#endif // V3INVSCALESAFE_V__4

#ifndef V3INVSCALEFAST__4
	__forceinline Vector_4_Out V3InvScaleFast( Vector_4_In inVector, float floatVal )
	{
		Vector_4 outVect;
		float inv = FPInvertFast(floatVal);
		outVect.x = inVector.x * inv;
		outVect.y = inVector.y * inv;
		outVect.z = inVector.z * inv;
		return outVect;
	}
#endif // V3INVSCALEFAST__4

#ifndef V3INVSCALEFAST_V__4
	__forceinline Vector_4_Out V3InvScaleFast( Vector_4_In inVector, Vector_4_In floatVal )
	{
		Vector_4 outVect;
		outVect.x = inVector.x * FPInvertFast(floatVal.x);
		outVect.y = inVector.y * FPInvertFast(floatVal.y);
		outVect.z = inVector.z * FPInvertFast(floatVal.z);
		return outVect;
	}
#endif // V3INVSCALEFAST_V__4

#ifndef V3INVSCALEFASTSAFE__4
	__forceinline Vector_4_Out V3InvScaleFastSafe( Vector_4_In inVector, float floatVal, float errVal  )
	{
		Vector_4 outVect;
		if( floatVal != 0.0f )
		{
			float inv = FPInvertFast(floatVal);
			outVect.x = inVector.x * inv;
			outVect.y = inVector.y * inv;
			outVect.z = inVector.z * inv;
		}
		else
		{
			outVect.x = outVect.y = outVect.z = errVal;
		}
		return outVect;
	}
#endif // V3INVSCALEFASTSAFE__4

#ifndef V3INVSCALEFASTSAFE_V__4
	__forceinline Vector_4_Out V3InvScaleFastSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal  )
	{
		Vector_4 outVect;
		outVect.x = (floatVal.x != 0.0f) ? inVector.x*FPInvertFast(floatVal.x) : errVal;
		outVect.y = (floatVal.y != 0.0f) ? inVector.y*FPInvertFast(floatVal.y) : errVal;
		outVect.z = (floatVal.z != 0.0f) ? inVector.z*FPInvertFast(floatVal.z) : errVal;
		return outVect;
	}
#endif // V3INVSCALEFASTSAFE_V__4

#ifndef V3CLAMPMAG__4
	inline Vector_4_Out V3ClampMag( Vector_4_In v, float minMag, float maxMag )
	{
		Vector_4 result;
		float mag2 = V3MagSquared( v );
		float invMag = FPInvSqrt( mag2 );
		float maxMag2 = maxMag * maxMag;
		float minMag2 = minMag * minMag;
		float multiplier = FPIfGteThenElse( mag2, maxMag2, maxMag*invMag, FPIfLteThenElse(mag2, minMag2, minMag*invMag, 1.0f) );
		result = V3Scale( v, multiplier );
		return result;
	}
#endif // V3CLAMPMAG__4

#ifndef V3ADD_3F__4
	__forceinline Vector_4_Out V3Add( Vector_4_In inVector, float sx, float sy, float sz )
	{
		Vector_4 outVect;
		outVect.x = inVector.x + sx;
		outVect.y = inVector.y + sy;
		outVect.z = inVector.z + sz;
		return outVect;
	}
#endif // V3ADD_3F__4

#ifndef V3ADD_V__4
	__forceinline Vector_4_Out V3Add( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x + inVector2.x;
		outVect.y = inVector1.y + inVector2.y;
		outVect.z = inVector1.z + inVector2.z;
		return outVect;
	}
#endif // V3ADD_V__4

#ifndef V3ADDSCALED__4
	__forceinline Vector_4_Out V3AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x + inVector2.x * floatValue;
		outVect.y = inVector1.y + inVector2.y * floatValue;
		outVect.z = inVector1.z + inVector2.z * floatValue;
		return outVect;
	}
#endif // V3ADDSCALED__4

#ifndef V3ADDSCALED_V__4
	__forceinline Vector_4_Out V3AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x + inVector2.x * floatValue.x;
		outVect.y = inVector1.y + inVector2.y * floatValue.y;
		outVect.z = inVector1.z + inVector2.z * floatValue.z;
		return outVect;
	}
#endif // V3ADDSCALED_V__4

#ifndef V3SUBTRACT_3F__4
	__forceinline Vector_4_Out V3Subtract( Vector_4_In inVector, float sx, float sy, float sz )
	{
		Vector_4 outVect;
		outVect.x = inVector.x - sx;
		outVect.y = inVector.y - sy;
		outVect.z = inVector.z - sz;
		return outVect;
	}
#endif // V3SUBTRACT_3F__4

#ifndef V3SUBTRACT_V__4
	__forceinline Vector_4_Out V3Subtract( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x - inVector2.x;
		outVect.y = inVector1.y - inVector2.y;
		outVect.z = inVector1.z - inVector2.z;
		return outVect;
	}
#endif // V3SUBTRACT_V__4

#ifndef V3SUBTRACTSCALED__4
	__forceinline Vector_4_Out V3SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue;
		outVect.y = inVector1.y - inVector2.y * floatValue;
		outVect.z = inVector1.z - inVector2.z * floatValue;
		return outVect;
	}
#endif // V3SUBTRACTSCALED__4

#ifndef V3SUBTRACTSCALED_V__4
	__forceinline Vector_4_Out V3SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue.x;
		outVect.y = inVector1.y - inVector2.y * floatValue.y;
		outVect.z = inVector1.z - inVector2.z * floatValue.z;
		return outVect;
	}
#endif // V3SUBTRACTSCALED_V__4

#ifndef V3SCALE_V__4
	__forceinline Vector_4_Out V3Scale( Vector_4_In inVector1, Vector_4_In inVector2 )
	{
		Vector_4 outVect;
		outVect.x = inVector1.x * inVector2.x;
		outVect.y = inVector1.y * inVector2.y;
		outVect.z = inVector1.z * inVector2.z;
		return outVect;
	}
#endif // V3SCALE_V__4

#ifndef V3NEGATE__4
	__forceinline Vector_4_Out V3Negate(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = -inVector.x;
		outVect.y = -inVector.y;
		outVect.z = -inVector.z;
		return outVect;
	}
#endif // V3NEGATE__4

#ifndef V3ABS__4
	__forceinline Vector_4_Out V3Abs(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPAbs(inVector.x);
		outVect.y = FPAbs(inVector.y);
		outVect.z = FPAbs(inVector.z);
		return outVect;
	}
#endif // V3ABS__4

#ifndef V3INVERT__4
	__forceinline Vector_4_Out V3Invert(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = 1.0f/inVector.x;
		outVect.y = 1.0f/inVector.y;
		outVect.z = 1.0f/inVector.z;
		return outVect;
	}
#endif // V3INVERT__4

#ifndef V3INVERTSAFE__4
	__forceinline Vector_4_Out V3InvertSafe(Vector_4_In inVector, float errVal )
	{
		Vector_4 outVect;
		outVect.x = ( inVector.x == 0.0f ? errVal : 1.0f/inVector.x );
		outVect.y = ( inVector.y == 0.0f ? errVal : 1.0f/inVector.y );
		outVect.z = ( inVector.z == 0.0f ? errVal : 1.0f/inVector.z );
		return outVect;
	}
#endif // V3INVERTSAFE__4

#ifndef V3INVERTFAST__4
	__forceinline Vector_4_Out V3InvertFast(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPInvertFast(inVector.x);
		outVect.y = FPInvertFast(inVector.y);
		outVect.z = FPInvertFast(inVector.z);
		return outVect;
	}
#endif // V3INVERTFAST__4

#ifndef V3INVERTFASTSAFE__4
	__forceinline Vector_4_Out V3InvertFastSafe(Vector_4_In inVector, float errVal)
	{
		Vector_4 outVect;
		outVect.x = ( inVector.x == 0.0f ? errVal : FPInvertFast(inVector.x) );
		outVect.y = ( inVector.y == 0.0f ? errVal : FPInvertFast(inVector.y) );
		outVect.z = ( inVector.z == 0.0f ? errVal : FPInvertFast(inVector.z) );
		return outVect;
	}
#endif // V3INVERTFASTSAFE__4

#ifndef V3RESULTTOINDEXZYX__4
	__forceinline void V3ResultToIndexZYX( u32& outInt, Vector_4_In maskVector )
	{
#	if UNIQUE_VECTORIZED_TYPE
		outInt = ( *(u32*)(&maskVector.x) & 4 ) | ( *(u32*)(&maskVector.y) & 2 ) | ( *(u32*)(&maskVector.z) & 1 );
#	else
		outInt = ( *(u32*)(&maskVector.x) & 1 ) | ( *(u32*)(&maskVector.y) & 2 ) | ( *(u32*)(&maskVector.z) & 4 );
#	endif // UNIQUE_VECTORIZED_TYPE
	}
#endif // V3RESULTTOINDEXZYX__4

#ifndef V3RESULTTOINDEXXYZ__4
	__forceinline void V3ResultToIndexXYZ( u32& outInt, Vector_4_In maskVector )
	{
#	if UNIQUE_VECTORIZED_TYPE
		outInt = ( *(u32*)(&maskVector.z) & 4 ) | ( *(u32*)(&maskVector.y) & 2 ) | ( *(u32*)(&maskVector.x) & 1 );
#	else
		outInt = ( *(u32*)(&maskVector.z) & 1 ) | ( *(u32*)(&maskVector.y) & 2 ) | ( *(u32*)(&maskVector.x) & 4 );
#	endif // UNIQUE_VECTORIZED_TYPE
	}
#endif // V3RESULTTOINDEXXYZ__4

#ifndef V3NORMALIZE__4
	__forceinline Vector_4_Out V3Normalize(Vector_4_In inVector)
	{
		Vector_4 outVect;
		float invMag = V3InvMag(inVector);
		outVect.x = inVector.x * invMag;
		outVect.y = inVector.y * invMag;
		outVect.z = inVector.z * invMag;
		return outVect;
	}
#endif // V3NORMALIZE__4

#ifndef V3NORMALIZESAFE__4
	__forceinline Vector_4_Out V3NormalizeSafe(Vector_4_In inVector, float errVal, float magSqThreshold)
	{
		Vector_4 outVect;
		float mag2 = V3MagSquared(inVector);
		if(mag2 > magSqThreshold)
		{
			float invMag = FPInvSqrt(mag2);
			outVect.x = inVector.x*invMag;
			outVect.y = inVector.y*invMag;
			outVect.z = inVector.z*invMag;
		}
		else
		{
			outVect.x = outVect.y = outVect.z = errVal;
		}
		return outVect;
	}
#endif // V3NORMALIZESAFE__4

#ifndef V3NORMALIZESAFE_V__4
	__forceinline Vector_4_Out V3NormalizeSafe(Vector_4_In inVector, Vector_4_In errVal, Vector_4_In magSqThreshold )
	{
		Vector_4 outVect;
		float mag2 = V3MagSquared(inVector);
		float invMag = FPInvSqrt(mag2);
		outVect.x = mag2 > magSqThreshold.x ? inVector.x*invMag : errVal.x;
		outVect.y = mag2 > magSqThreshold.y ? inVector.y*invMag : errVal.y;
		outVect.z = mag2 > magSqThreshold.z ? inVector.z*invMag : errVal.z;
		return outVect;
	}
#endif // V3NORMALIZESAFE_V__4

#ifndef V3NORMALIZEFAST__4
	__forceinline Vector_4_Out V3NormalizeFast(Vector_4_In inVector)
	{
		Vector_4 outVect;
		float invMag = V3InvMagFast(inVector);
		outVect.x = inVector.x * invMag;
		outVect.y = inVector.y * invMag;
		outVect.z = inVector.z * invMag;
		return outVect;
	}
#endif // V3NORMALIZEFAST

#ifndef V3NORMALIZEFASTSAFE__4
	__forceinline Vector_4_Out V3NormalizeFastSafe(Vector_4_In inVector, float errVal, float magSqThreshold)
	{
		Vector_4 outVect;
		float mag2 = V3MagSquared(inVector);
		if(mag2 > magSqThreshold)
		{
			float invMag = FPInvSqrtFast(mag2);
			outVect.x = inVector.x*invMag;
			outVect.y = inVector.y*invMag;
			outVect.z = inVector.z*invMag;
			outVect.w = inVector.w*invMag;
		}
		else
		{
			outVect.x = outVect.y = outVect.z = errVal;
		}
		return outVect;
	}
#endif // V3NORMALIZEFASTSAFE__4

#ifndef V3DOT__4
	__forceinline float V3Dot(Vector_4_In a, Vector_4_In b)
	{
		return a.x*b.x + a.y*b.y + a.z*b.z;
	}
#endif // V3DOT__4

#ifndef V3DOTV__4
	__forceinline Vector_4_Out V3DotV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3Dot(a, b);
		return outVect;
	}
#endif // V3DOTV__4

#ifndef V3CROSS__4
	__forceinline Vector_4_Out V3Cross(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = a.y * b.z - a.z * b.y;
		outVect.y = a.z * b.x - a.x * b.z; 
		outVect.z = a.x * b.y - a.y * b.x;
		outVect.w = 0.0f;	// Since this behavior happens by chance (for free) in the vectorized version, we duplicate it here.
		return outVect;
	}
#endif // V3CROSS__4

#ifndef V3ADDCROSSED__4
	__forceinline Vector_4_Out V3AddCrossed(Vector_4_In toAddTo, Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = toAddTo.x + a.y * b.z - a.z * b.y;
		outVect.y = toAddTo.y + a.z * b.x - a.x * b.z; 
		outVect.z = toAddTo.z + a.x * b.y - a.y * b.x;
		outVect.w = toAddTo.w;	// Since this behavior happens by chance (for free) in the vectorized version, we duplicate it here.
		return outVect;
	}
#endif // V3ADDCROSSED__4

#ifndef V3SUBTRACTCROSSED__4
	__forceinline Vector_4_Out V3SubtractCrossed(Vector_4_In toSubtractFrom, Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = toSubtractFrom.x + b.y * a.z - b.z * a.y;
		outVect.y = toSubtractFrom.y + b.z * a.x - b.x * a.z; 
		outVect.z = toSubtractFrom.z + b.x * a.y - b.y * a.x;
		outVect.w = toSubtractFrom.w;	// Since this behavior happens by chance (for free) in the vectorized version, we duplicate it here.
		return outVect;
	}
#endif // V3SUBTRACTCROSSED__4

#ifndef V3AVERAGE__4
	__forceinline Vector_4_Out V3Average(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = 0.5f * (a.x + b.x);
		outVect.y = 0.5f * (a.y + b.y);
		outVect.z = 0.5f * (a.z + b.z);
		return outVect;
	}
#endif // V3AVERAGE__4

#ifndef V3LERP__4
	__forceinline Vector_4_Out V3Lerp( float t, Vector_4_In a, Vector_4_In b )
	{
		Vector_4 outVect;
		outVect.x = FPLerp(t, a.x, b.x);
		outVect.y = FPLerp(t, a.y, b.y);
		outVect.z = FPLerp(t, a.z, b.z);
		return outVect;
	}
#endif // V3LERP__4

#ifndef V3LERP_V__4
	__forceinline Vector_4_Out V3Lerp( Vector_4_In t, Vector_4_In a, Vector_4_In b )
	{
		Vector_4 outVect;
		outVect.x = FPLerp(t.x, a.x, b.x);
		outVect.y = FPLerp(t.y, a.y, b.y);
		outVect.z = FPLerp(t.z, a.z, b.z);
		return outVect;
	}
#endif // V3LERP_V__4

#ifndef V3POW__4
	__forceinline Vector_4_Out V3Pow( Vector_4_In x, Vector_4_In y )
	{
		Vector_4 outVect;
		outVect.x = FPPow(x.x, y.x);
		outVect.y = FPPow(x.y, y.y);
		outVect.z = FPPow(x.z, y.z);
		return outVect;
	}
#endif // V3POW__4

#ifndef V3EXPT__4
	__forceinline Vector_4_Out V3Expt( Vector_4_In x )
	{
		Vector_4 outVect;
		outVect.x = FPExpt(x.x);
		outVect.y = FPExpt(x.y);
		outVect.z = FPExpt(x.z);
		return outVect;
	}
#endif // V3EXPT__4

#ifndef V3LOG2__4
	__forceinline Vector_4_Out V3Log2( Vector_4_In x )
	{
		// log2(f) = ln(f)/ln(2)
		Vector_4 outVect;
		outVect.x = FPLog2(x.x);
		outVect.y = FPLog2(x.y);
		outVect.z = FPLog2(x.z);
		return outVect;
	}
#endif // V3LOG2__4

#ifndef V3LOG10__4
	__forceinline Vector_4_Out V3Log10( Vector_4_In x )
	{
		Vector_4 outVect;
		outVect.x = FPLog10(x.x);
		outVect.y = FPLog10(x.y);
		outVect.z = FPLog10(x.z);
		return outVect;
	}
#endif // V3LOG10__4

	//============================================================================
	// Magnitude and distance

#ifndef V3SQRT__4
	__forceinline Vector_4_Out V3Sqrt( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = FPSqrt(v.x);
		outVect.y = FPSqrt(v.y);
		outVect.z = FPSqrt(v.z);
		return outVect;
	}
#endif // V3SQRT__4

#ifndef V3SQRTSAFE__4
	__forceinline Vector_4_Out V3SqrtSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPSqrtSafe( v.x, errVal );
		outVect.y = FPSqrtSafe( v.y, errVal );
		outVect.z = FPSqrtSafe( v.z, errVal );
		return outVect;
	}
#endif // V3SQRTSAFE__4

#ifndef V3SQRTFAST__4
	__forceinline Vector_4_Out V3SqrtFast( Vector_4_In v )
	{
		return V3Sqrt( v );
	}
#endif // V3SQRTFAST__4

#ifndef V3SQRTFASTSAFE__4
	__forceinline Vector_4_Out V3SqrtFastSafe( Vector_4_In v, float errVal )
	{
		return V3SqrtSafe( v, errVal );
	}
#endif // V3SQRTFASTSAFE__4

#ifndef V3INVSQRT__4
	__forceinline Vector_4_Out V3InvSqrt( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrt(v.x);
		outVect.y = FPInvSqrt(v.y);
		outVect.z = FPInvSqrt(v.z);
		return outVect;
	}
#endif // V3INVSQRT__4

#ifndef V3INVSQRTSAFE__4
	__forceinline Vector_4_Out V3InvSqrtSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtSafe(v.x, errVal);
		outVect.y = FPInvSqrtSafe(v.y, errVal);
		outVect.z = FPInvSqrtSafe(v.z, errVal);
		return outVect;
	}
#endif // V3INVSQRTSAFE__4

#ifndef V3INVSQRTFAST__4
	__forceinline Vector_4_Out V3InvSqrtFast( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtFast(v.x);
		outVect.y = FPInvSqrtFast(v.y);
		outVect.z = FPInvSqrtFast(v.z);
		return outVect;
	}
#endif // V3INVSQRTFAST__4

#ifndef V3INVSQRTFASTSAFE__4
	__forceinline Vector_4_Out V3InvSqrtFastSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = FPInvSqrtFastSafe(v.x, errVal);
		outVect.y = FPInvSqrtFastSafe(v.y, errVal);
		outVect.z = FPInvSqrtFastSafe(v.z, errVal);
		return outVect;
	}
#endif // V3INVSQRTFASTSAFE__4

#ifndef V3MAG__4
	__forceinline float V3Mag( Vector_4_In v )
	{
		return FPSqrt( V3Dot( v, v ) );
	}
#endif // V3MAG__4

#ifndef V3MAGV__4
	__forceinline Vector_4_Out V3MagV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3Mag( v );
		return outVect;
	}
#endif // V3MAGV__4

#ifndef V3MAGFAST__4
	__forceinline float V3MagFast( Vector_4_In v )
	{
		return V3Mag( v );
	}
#endif // V3MAGFAST__4

#ifndef V3MAGVFAST__4
	__forceinline Vector_4_Out V3MagVFast( Vector_4_In v )
	{
		return V3MagV( v );
	}
#endif // V3MAGVFAST__4

#ifndef V3MAGXYSQUARED__4
	__forceinline float V3MagXYSquared( Vector_4_In v )
	{
		return (v.x*v.x + v.y*v.y);
	}
#endif // V3MAGXYSQUARED__4

#ifndef V3MAGXZSQUARED__4
	__forceinline float V3MagXZSquared( Vector_4_In v )
	{
		return (v.x*v.x + v.z*v.z);
	}
#endif // V3MAGXZSQUARED__4

#ifndef V3MAGYZSQUARED__4
	__forceinline float V3MagYZSquared( Vector_4_In v )
	{
		return (v.y*v.y + v.z*v.z);
	}
#endif // V3MAGYZSQUARED__4

#ifndef V3MAGXYSQUAREDV__4
	__forceinline Vector_4_Out V3MagXYSquaredV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3MagXYSquared( v );
		return outVect;
	}
#endif // V3MAGXYSQUAREDV__4

#ifndef V3MAGXZSQUAREDV__4
	__forceinline Vector_4_Out V3MagXZSquaredV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3MagXZSquared( v );
		return outVect;
	}
#endif // V3MAGXZSQUAREDV__4

#ifndef V3MAGYZSQUAREDV__4
	__forceinline Vector_4_Out V3MagYZSquaredV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3MagYZSquared( v );
		return outVect;
	}
#endif // V3MAGYZSQUAREDV__4

#ifndef V3MAGXY__4
	__forceinline float V3MagXY( Vector_4_In v )
	{
		return FPSqrt( V3MagXYSquared( v ) );
	}
#endif // V3MAGXY__4

#ifndef V3MAGXZ__4
	__forceinline float V3MagXZ( Vector_4_In v )
	{
		return FPSqrt( V3MagXZSquared( v ) );
	}
#endif // V3MAGXZ__4

#ifndef V3MAGYZ__4
	__forceinline float V3MagYZ( Vector_4_In v )
	{
		return FPSqrt( V3MagYZSquared( v ) );
	}
#endif // V3MAGYZ__4

#ifndef V3MAGXYV__4
	__forceinline Vector_4_Out V3MagXYV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3MagXY( v );
		return outVect;
	}
#endif // V3MAGXYV__4

#ifndef V3MAGXZV__4
	__forceinline Vector_4_Out V3MagXZV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3MagXZ( v );
		return outVect;
	}
#endif // V3MAGXZV__4

#ifndef V3MAGYZV__4
	__forceinline Vector_4_Out V3MagYZV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3MagYZ( v );
		return outVect;
	}
#endif // V3MAGYZV__4

#ifndef V3MAGYZVFAST__4
	__forceinline Vector_4_Out V3MagXYVFast( Vector_4_In v )
	{
		return V3MagXYV( v );
	}
#endif // V3MAGYZVFAST__4

#ifndef V3MAGXZVFAST__4
	__forceinline Vector_4_Out V3MagXZVFast( Vector_4_In v )
	{
		return V3MagXZV( v );
	}
#endif // V3MAGXZVFAST__4

#ifndef V3MAGYZVFAST__4
	__forceinline Vector_4_Out V3MagYZVFast( Vector_4_In v )
	{
		return V3MagYZV( v );
	}
#endif // V3MAGYZVFAST__4

#ifndef V3MAGSQUARED__4
	__forceinline float V3MagSquared( Vector_4_In v )
	{
		return V3Dot( v, v );
	}
#endif // V3MAGSQUARED__4

#ifndef V3MAGSQUAREDV__4
	__forceinline Vector_4_Out V3MagSquaredV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3MagSquared( v );
		return outVect;
	}
#endif // V3MAGSQUAREDV__4

#ifndef V3INVMAG__4
	__forceinline float V3InvMag( Vector_4_In v )
	{
		return FPInvSqrt( V3Dot( v, v ) );
	}
#endif // V3INVMAG__4

#ifndef V3INVMAGSAFE__4
	__forceinline float V3InvMagSafe( Vector_4_In v, float errVal )
	{
		float dot = V3Dot( v, v );
		return FPInvSqrtSafe( dot, errVal );
	}
#endif // V3INVMAGSAFE__4

#ifndef V3INVMAGFAST__4
	__forceinline float V3InvMagFast( Vector_4_In v )
	{
		return FPInvSqrtFast( V3Dot( v, v ) );
	}
#endif // V3INVMAGFAST__4

#ifndef V3INVMAGFASTSAFE__4
	__forceinline float V3InvMagFastSafe( Vector_4_In v, float errVal )
	{
		float dot = V3Dot( v, v );
		return FPInvSqrtFastSafe( dot, errVal );
	}
#endif // V3INVMAGFASTSAFE__4

#ifndef V3INVMAGV__4
	__forceinline Vector_4_Out V3InvMagV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvMag( v );
		return outVect;
	}
#endif // V3INVMAGV__4

#ifndef V3INVMAGVSAFE__4
	__forceinline Vector_4_Out V3InvMagVSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvMagSafe( v, errVal );
		return outVect;
	}
#endif // V3INVMAGVSAFE__4

#ifndef V3INVMAGVFAST__4
	__forceinline Vector_4_Out V3InvMagVFast( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvMagFast( v );
		return outVect;
	}
#endif // V3INVMAGVFAST__4

#ifndef V3INVMAGVFASTSAFE__4
	__forceinline Vector_4_Out V3InvMagVFastSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvMagFastSafe( v, errVal );
		return outVect;
	}
#endif // V3INVMAGVFASTSAFE__4

#ifndef V3INVMAGSQUARED__4
	__forceinline float V3InvMagSquared( Vector_4_In v )
	{
		return 1.0f/V3MagSquared( v );
	}
#endif // V3INVMAGSQUARED__4

#ifndef V3INVMAGSQUAREDSAFE__4
	__forceinline float V3InvMagSquaredSafe( Vector_4_In v, float errVal )
	{
		float magSq = V3MagSquared( v );
		return FPIfGtZeroThenElse( magSq, 1.0f/magSq, errVal );
	}
#endif // V3INVMAGSQUAREDSAFE__4

#ifndef V3INVMAGSQUAREDFAST__4
	__forceinline float V3InvMagSquaredFast( Vector_4_In v )
	{
		return FPInvertFast( V3MagSquared( v ) );
	}
#endif // V3INVMAGSQUAREDFAST__4

#ifndef V3INVMAGSQUAREDFASTSAFE__4
	__forceinline float V3InvMagSquaredFastSafe( Vector_4_In v, float errVal )
	{
		return FPInvertFastSafe( V3MagSquared( v ), errVal );
	}
#endif // V3INVMAGSQUAREDFASTSAFE__4

#ifndef V3INVMAGSQUAREDV__4
	__forceinline Vector_4_Out V3InvMagSquaredV( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvMagSquared( v );
		return outVect;
	}
#endif // V3INVMAGSQUAREDV__4

#ifndef V3INVMAGSQUAREDVSAFE__4
	__forceinline Vector_4_Out V3InvMagSquaredVSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvMagSquaredSafe( v, errVal );
		return outVect;
	}
#endif // V3INVMAGSQUAREDVSAFE__4

#ifndef V3INVMAGSQUAREDVFAST__4
	__forceinline Vector_4_Out V3InvMagSquaredVFast( Vector_4_In v )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvMagSquaredFast( v );
		return outVect;
	}
#endif // V3INVMAGSQUAREDVFAST__4

#ifndef V3INVMAGSQUAREDVFASTSAFE__4
	__forceinline Vector_4_Out V3InvMagSquaredVFastSafe( Vector_4_In v, float errVal )
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvMagSquaredFastSafe( v, errVal );
		return outVect;
	}
#endif // V3INVMAGSQUAREDVFASTSAFE__4

#ifndef V3DIST__4
	__forceinline float V3Dist(Vector_4_In a, Vector_4_In b)
	{
		return V3Mag( V3Subtract( a, b ) );
	}
#endif // V3DIST__4

#ifndef V3DISTFAST__4
	__forceinline float V3DistFast(Vector_4_In a, Vector_4_In b)
	{
		return V3Dist( a, b );
	}
#endif // V3DISTFAST__4

#ifndef V3DISTV__4
	__forceinline Vector_4_Out V3DistV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3Dist( a, b );
		return outVect;
	}
#endif // V3DISTV__4

#ifndef V3DISTVFAST__4
	__forceinline Vector_4_Out V3DistVFast(Vector_4_In a, Vector_4_In b)
	{
		return V3DistV( a, b );
	}
#endif // V3DISTVFAST__4

#ifndef V3INVDIST__4
	__forceinline float V3InvDist(Vector_4_In a, Vector_4_In b)
	{
		return V3InvMag( V3Subtract( a, b ) );
	}
#endif // V3INVDIST__4

#ifndef V3INVDISTSAFE__4
	__forceinline float V3InvDistSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V3InvMagSafe( V3Subtract( a, b ), errVal );
	}
#endif // V3INVDISTSAFE__4

#ifndef V3INVDISTFAST__4
	__forceinline float V3InvDistFast(Vector_4_In a, Vector_4_In b)
	{
		return V3InvMagFast( V3Subtract( a, b ) );
	}
#endif // V3INVDISTFAST__4

#ifndef V3INVDISTFASTSAFE__4
	__forceinline float V3InvDistFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V3InvMagFastSafe( V3Subtract( a, b ), errVal );
	}
#endif // V3INVDISTFASTSAFE__4

#ifndef V3INVDISTV__4
	__forceinline Vector_4_Out V3InvDistV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvDist( a, b );
		return outVect;
	}
#endif // V3INVDISTV__4

#ifndef V3INVDISTVSAFE__4
	__forceinline Vector_4_Out V3InvDistVSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvDistSafe( a, b, errVal );
		return outVect;
	}
#endif // V3INVDISTVSAFE__4

#ifndef V3INVDISTVFAST__4
	__forceinline Vector_4_Out V3InvDistVFast(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvDistFast( a, b );
		return outVect;
	}
#endif // V3INVDISTVFAST__4

#ifndef V3INVDISTVFASTSAFE__4
	__forceinline Vector_4_Out V3InvDistVFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvDistFastSafe( a, b, errVal );
		return outVect;
	}
#endif // V3INVDISTVFASTSAFE__4

#ifndef V3DISTSQUARED__4
	__forceinline float V3DistSquared(Vector_4_In a, Vector_4_In b)
	{
		return V3MagSquared( V3Subtract( a, b ) );
	}
#endif // V3DISTSQUARED__4

#ifndef V3DISTSQUAREDV__4
	__forceinline Vector_4_Out V3DistSquaredV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3DistSquared( a, b );
		return outVect;
	}
#endif // V3DISTSQUAREDV__4

#ifndef V3INVDISTSQUARED__4
	__forceinline float V3InvDistSquared(Vector_4_In a, Vector_4_In b)
	{
		return V3InvMagSquared( V3Subtract( a, b ) );
	}
#endif // V3INVDISTSQUARED__4

#ifndef V3INVDISTSQUAREDSAFE__4
	__forceinline float V3InvDistSquaredSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V3InvMagSquaredSafe( V3Subtract( a, b ), errVal );
	}
#endif // V3INVDISTSQUAREDSAFE__4

#ifndef V3INVDISTSQUAREDFAST__4
	__forceinline float V3InvDistSquaredFast(Vector_4_In a, Vector_4_In b)
	{
		return V3InvMagSquaredFast( V3Subtract( a, b ) );
	}
#endif // V3INVDISTSQUAREDFAST__4

#ifndef V3INVDISTSQUAREDFASTSAFE__4
	__forceinline float V3InvDistSquaredFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		return V3InvMagSquaredFastSafe( V3Subtract( a, b ), errVal );
	}
#endif // V3INVDISTSQUAREDFASTSAFE__4

#ifndef V3INVDISTSQUAREDV__4
	__forceinline Vector_4_Out V3InvDistSquaredV(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvDistSquared( a, b );
		return outVect;
	}
#endif // V3INVDISTSQUAREDV__4

#ifndef V3INVDISTSQUAREDVSAFE__4
	__forceinline Vector_4_Out V3InvDistSquaredVSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvDistSquaredSafe( a, b, errVal );
		return outVect;
	}
#endif // V3INVDISTSQUAREDVSAFE__4

#ifndef V3INVDISTSQUAREDVFAST__4
	__forceinline Vector_4_Out V3InvDistSquaredVFast(Vector_4_In a, Vector_4_In b)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvDistSquaredFast( a, b );
		return outVect;
	}
#endif // V3INVDISTSQUAREDVFAST__4

#ifndef V3INVDISTSQUAREDVFASTSAFE__4
	__forceinline Vector_4_Out V3InvDistSquaredVFastSafe(Vector_4_In a, Vector_4_In b, float errVal)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = V3InvDistSquaredFastSafe( a, b, errVal );
		return outVect;
	}
#endif // V3INVDISTSQUAREDVFASTSAFE__4

	//============================================================================
	// Conversion functions

#ifndef V3FLOATTOINTRAW__4
	template <int exponent>
	__forceinline Vector_4_Out V3FloatToIntRaw(Vector_4_In inVector)
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

		TempResult.i = static_cast<int>(inVector.z * multiplier);
		outVect.z = TempResult.f;

		return outVect;
	}
#endif // V3FLOATTOINTRAW__4

#ifndef V3INTTOFLOATRAW__4
	template <int exponent>
	__forceinline Vector_4_Out V3IntToFloatRaw(Vector_4_In inVector)
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
		Temp1.f = inVector.z;
		outVect.z = static_cast<float>(Temp1.i) * invDiv;
		return outVect;
	}
#endif // V3INTTOFLOATRAW__4

#ifndef V3ROUNDTONEARESTINT__4
	__forceinline Vector_4_Out V3RoundToNearestInt(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x+(FPIfGteZeroThenElse(inVector.x, 0.5f, -0.5f))) );
		outVect.y = static_cast<float>( static_cast<int>(inVector.y+(FPIfGteZeroThenElse(inVector.y, 0.5f, -0.5f))) );
		outVect.z = static_cast<float>( static_cast<int>(inVector.z+(FPIfGteZeroThenElse(inVector.z, 0.5f, -0.5f))) );
		return outVect;
	}
#endif // V3ROUNDTONEARESTINT__4

#ifndef V3ROUNDTONEARESTINTZERO__4
	__forceinline Vector_4_Out V3RoundToNearestIntZero(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x) );
		outVect.y = static_cast<float>( static_cast<int>(inVector.y) );
		outVect.z = static_cast<float>( static_cast<int>(inVector.z) );
		return outVect;
	}
#endif // V3ROUNDTONEARESTINTZERO__4

#ifndef V3ROUNDTONEARESTINTNEGINF__4
	__forceinline Vector_4_Out V3RoundToNearestIntNegInf(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPFloor( inVector.x );
		outVect.y = FPFloor( inVector.y );
		outVect.z = FPFloor( inVector.z );
		return outVect;
	}
#endif // V3ROUNDTONEARESTINTNEGINF__4

#ifndef V3ROUNDTONEARESTINTPOSINF__4
	__forceinline Vector_4_Out V3RoundToNearestIntPosInf(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = FPCeil( inVector.x );
		outVect.y = FPCeil( inVector.y );
		outVect.z = FPCeil( inVector.z );
		return outVect;
	}
#endif // V3ROUNDTONEARESTINTPOSINF__4

	//============================================================================
	// Comparison functions

#ifndef V3ISBETWEENNEGANDPOSBOUNDS__4
	__forceinline unsigned int V3IsBetweenNegAndPosBounds( Vector_4_In testVector, Vector_4_In boundsVector )
	{
		return (	testVector.x <= boundsVector.x && testVector.x >= -boundsVector.x &&
					testVector.y <= boundsVector.y && testVector.y >= -boundsVector.y &&
					testVector.z <= boundsVector.z && testVector.z >= -boundsVector.z ? 1u : 0u );
	}
#endif // V3ISBETWEENNEGANDPOSBOUNDS__4

#ifndef V3SAMESIGNALL__4
	__forceinline unsigned int V3SameSignAll(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		return (
			(*(int*)( &inVector1.x ) & 0x80000000) == (*(int*)( &inVector2.x ) & 0x80000000) &&
			(*(int*)( &inVector1.y ) & 0x80000000) == (*(int*)( &inVector2.y ) & 0x80000000) &&
			(*(int*)( &inVector1.z ) & 0x80000000) == (*(int*)( &inVector2.z ) & 0x80000000)
			) ? 1u : 0u;
	}
#endif // V3SAMESIGNALL__4

#ifndef V3ISZEROALL__4
	__forceinline unsigned int V3IsZeroAll(Vector_4_In inVector)
	{
		return ( inVector.x == 0.0f && inVector.y == 0.0f && inVector.z == 0.0f ? 1u : 0u );
	}
#endif // V3ISZEROALL__4

#ifndef V3ISZERONONE__4
	__forceinline unsigned int V3IsZeroNone(Vector_4_In inVector)
	{
		return ( inVector.x != 0.0f && inVector.y != 0.0f && inVector.z != 0.0f ? 1u : 0u );
	}
#endif // V3ISZERONONE__4

#ifndef V3ISEQUALALL__4
	__forceinline unsigned int V3IsEqualAll(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		return ( inVector1.x == inVector2.x &&
				inVector1.y == inVector2.y &&
				inVector1.z == inVector2.z ? 1u : 0u );
	}
#endif // V3ISEQUALALL__4

#ifndef V3ISEQUALNONE__4
	__forceinline unsigned int V3IsEqualNone(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		return ( inVector1.x != inVector2.x &&
				inVector1.y != inVector2.y &&
				inVector1.z != inVector2.z ? 1u : 0u );
	}
#endif // V3ISEQUALNONE__4

#ifndef V3ISEQUALINTALL__4
	__forceinline unsigned int V3IsEqualIntAll(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		union
		{
			float f;
			int i;
		} Temp1, Temp2, Temp3, Temp4, Temp5, Temp6;
		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;
		Temp3.f = inVector1.y;
		Temp4.f = inVector2.y;
		Temp5.f = inVector1.z;
		Temp6.f = inVector2.z;

		return (Temp1.i == Temp2.i &&
				Temp3.i == Temp4.i &&
				Temp5.i == Temp6.i ? 1u : 0u );
	}
#endif // V3ISEQUALINTALL__4

#ifndef V3ISEQUALINTNONE__4
	__forceinline unsigned int V3IsEqualIntNone(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		union
		{
			float f;
			int i;
		} Temp1, Temp2, Temp3, Temp4, Temp5, Temp6;
		Temp1.f = inVector1.x;
		Temp2.f = inVector2.x;
		Temp3.f = inVector1.y;
		Temp4.f = inVector2.y;
		Temp5.f = inVector1.z;
		Temp6.f = inVector2.z;

		return (Temp1.i != Temp2.i &&
				Temp3.i != Temp4.i &&
				Temp5.i != Temp6.i ? 1u : 0u );
	}
#endif // V3ISEQUALINTNONE__4

#ifndef V3ISCLOSEALL__4
	__forceinline unsigned int V3IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, float eps)
	{
		return ( ( inVector1.x >= inVector2.x - eps && inVector1.x <= inVector2.x + eps ) &&
				( inVector1.y >= inVector2.y - eps && inVector1.y <= inVector2.y + eps ) &&
				( inVector1.z >= inVector2.z - eps && inVector1.z <= inVector2.z + eps ) ? 1u : 0u);
	}
#endif // V3ISCLOSEALL__4

#ifndef V3ISCLOSENONE__4
	__forceinline unsigned int V3IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, float eps)
	{
		return (( inVector1.x < inVector2.x - eps || inVector1.x > inVector2.x + eps ) &&
				( inVector1.y < inVector2.y - eps || inVector1.y > inVector2.y + eps ) &&
				( inVector1.z < inVector2.z - eps || inVector1.z > inVector2.z + eps ) ? 1u : 0u);
	}
#endif // V3ISCLOSENONE__4

#ifndef V3ISCLOSEALL_V__4
	__forceinline unsigned int V3IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps)
	{
		return (( inVector1.x >= inVector2.x - eps.x && inVector1.x <= inVector2.x + eps.x ) &&
				( inVector1.y >= inVector2.y - eps.y && inVector1.y <= inVector2.y + eps.y ) &&
				( inVector1.z >= inVector2.z - eps.z && inVector1.z <= inVector2.z + eps.z ) ? 1u : 0u);
	}
#endif // V3ISCLOSEALL_V__4

#ifndef V3ISCLOSENONE_V__4
	__forceinline unsigned int V3IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps)
	{
		return (( inVector1.x < inVector2.x - eps.x || inVector1.x > inVector2.x + eps.x ) &&
				( inVector1.y < inVector2.y - eps.y || inVector1.y > inVector2.y + eps.y ) &&
				( inVector1.z < inVector2.z - eps.z || inVector1.z > inVector2.z + eps.z ) ? 1u : 0u);
	}
#endif // V3ISCLOSENONE_V__4

#ifndef V3ISGREATERTHANALL__4
	__forceinline unsigned int V3IsGreaterThanAll(Vector_4_In bigVector, Vector_4_In smallVector)
	{
		return (( bigVector.x > smallVector.x ) &&
				( bigVector.y > smallVector.y ) &&
				( bigVector.z > smallVector.z ) ? 1u : 0u );
	}
#endif // V3ISGREATERTHANALL__4

#ifndef V3ISGREATERTHANOREQUALALL__4
	__forceinline unsigned int V3IsGreaterThanOrEqualAll(Vector_4_In bigVector, Vector_4_In smallVector)
	{
		return (( bigVector.x >= smallVector.x ) &&
				( bigVector.y >= smallVector.y ) &&
				( bigVector.z >= smallVector.z ) ? 1u : 0u);
	}
#endif // V3ISGREATERTHANOREQUALALL__4

#ifndef V3ISLESSTHANALL__4
	__forceinline unsigned int V3IsLessThanAll(Vector_4_In smallVector, Vector_4_In bigVector)
	{
		return (( bigVector.x > smallVector.x ) &&
				( bigVector.y > smallVector.y ) &&
				( bigVector.z > smallVector.z ) ? 1u : 0u);
	}
#endif // V3ISLESSTHANALL__4

#ifndef V3ISLESSTHANOREQUALALL__4
	__forceinline unsigned int V3IsLessThanOrEqualAll(Vector_4_In smallVector, Vector_4_In bigVector)
	{
		return (( bigVector.x >= smallVector.x ) &&
				( bigVector.y >= smallVector.y ) &&
				( bigVector.z >= smallVector.z ) ? 1u : 0u);
	}
#endif // V3ISLESSTHANOREQUALALL__4

#ifndef V3SELECT__4
	__forceinline Vector_4_Out V3SelectFT(Vector_4_In choiceVector, Vector_4_In zero, Vector_4_In nonZero)
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

		Temp1.f = choiceVector.z;
		Temp2.f = zero.z;
		Temp3.f = nonZero.z;
		TempResult.i = (Temp2.i & ~Temp1.i) | (Temp3.i & Temp1.i);
		outVect.z = TempResult.f;

		return outVect;
	}
#endif // V3SELECT__4

#ifndef V3SELECTVECT__4
	__forceinline Vector_4_Out V3SelectVect(Vector_4_In choiceVectorX, Vector_4_In zero, Vector_4_In nonZero)
	{
		return (GetX(choiceVectorX) == 0.0f ? zero : nonZero );
	}
#endif // V3SELECTVECT__4

#ifndef V3MAX__4
	__forceinline Vector_4_Out V3Max(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = FPMax(inVector1.x, inVector2.x);
		outVect.y = FPMax(inVector1.y, inVector2.y);
		outVect.z = FPMax(inVector1.z, inVector2.z);
		return outVect;
	}
#endif // V3MAX__4

#ifndef V3MIN__4
	__forceinline Vector_4_Out V3Min(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;
		outVect.x = FPMin(inVector1.x, inVector2.x);
		outVect.y = FPMin(inVector1.y, inVector2.y);
		outVect.z = FPMin(inVector1.z, inVector2.z);
		return outVect;
	}
#endif // V3MIN__4

#ifndef V3MINELEMENT__4
	__forceinline Vector_4_Out V3MinElement(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = FPMin( FPMin( inVector.x, inVector.y ), inVector.z );
		return outVect;
	}
#endif // V3MINELEMENT__4

#ifndef V3MAXELEMENT__4
	__forceinline Vector_4_Out V3MaxElement(Vector_4_In inVector)
	{
		Vector_4 outVect;
		outVect.x = outVect.y = outVect.z = outVect.w = FPMax( FPMax( inVector.x, inVector.y ), inVector.z );
		return outVect;
	}
#endif // V3MAXELEMENT__4

#ifndef V3CLAMP__4
	__forceinline Vector_4_Out V3Clamp( Vector_4_In inVector, Vector_4_In lowBound, Vector_4_In highBound )
	{
		Vector_4 outVect;
		outVect.x = FPClamp(inVector.x, lowBound.x, highBound.x);
		outVect.y = FPClamp(inVector.y, lowBound.y, highBound.y);
		outVect.z = FPClamp(inVector.z, lowBound.z, highBound.z);
		return outVect;
	}
#endif // V3CLAMP__4

#ifndef V3SATURATE__4
	__forceinline Vector_4_Out V3Saturate( Vector_4_In inVector )
	{
		Vector_4 outVect;
		outVect.x = FPClamp(inVector.x, 0.0f, 1.0f);
		outVect.y = FPClamp(inVector.y, 0.0f, 1.0f);
		outVect.z = FPClamp(inVector.z, 0.0f, 1.0f);
		return outVect;
	}
#endif // V3SATURATE__4

	//============================================================================
	// Quaternions

#ifndef V3QUATROTATE__4
	__forceinline Vector_4_Out V3QuatRotate( Vector_4_In inVect, Vector_4_In inQuat )
	{
		Vector_4 _xyz0 = inVect;
		V4SetWZero( _xyz0 );

		Vector_4 quatConjugate = V4QuatConjugate( inQuat );

		// Out = Q * V * Q^-1
		// TODO: (why is my formula reverse of XMMISC.INL's? That is, why are they doing the inverse transform?)
		Vector_4 resultVec_xyz = V4QuatMultiply( inQuat, _xyz0 );
		return V4QuatMultiply( resultVec_xyz, quatConjugate );
	}
#endif // V3QUATROTATE__4

#ifndef V3QUATROTATEREVERSE__4
	__forceinline Vector_4_Out V3QuatRotateReverse( Vector_4_In inVect, Vector_4_In inQuat )
	{
		Vector_4 _xyz0 = inVect;
		V4SetWZero( _xyz0 );

		Vector_4 quatConjugate = V4QuatConjugate( inQuat );

		// Out = Q^-1 * V * Q
		Vector_4 resultVec_xyz = V4QuatMultiply( _xyz0, inQuat );
		return V4QuatMultiply( quatConjugate, resultVec_xyz );
	}
#endif // V3QUATROTATEREVERSE__4


	//============================================================================
	// Bitwise operations

#ifndef V3AND__4
	__forceinline Vector_4_Out V3And(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, TempResult3, Temp1, Temp2, Temp3;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		TempResult3.f = inVector1.z;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		Temp3.f = inVector2.z;
		TempResult1.i &= Temp1.i;
		TempResult2.i &= Temp2.i;
		TempResult3.i &= Temp3.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		return outVect;
	}
#endif // V3AND__4

#ifndef V3OR__4
	__forceinline Vector_4_Out V3Or(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, TempResult3, Temp1, Temp2, Temp3;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		TempResult3.f = inVector1.z;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		Temp3.f = inVector2.z;
		TempResult1.i |= Temp1.i;
		TempResult2.i |= Temp2.i;
		TempResult3.i |= Temp3.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		return outVect;
	}
#endif // V3OR__4

#ifndef V3XOR__4
	__forceinline Vector_4_Out V3Xor(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, TempResult3, Temp1, Temp2, Temp3;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		TempResult3.f = inVector1.z;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		Temp3.f = inVector2.z;
		TempResult1.i ^= Temp1.i;
		TempResult2.i ^= Temp2.i;
		TempResult3.i ^= Temp3.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		return outVect;
	}
#endif // V3XOR__4

#ifndef V3ANDC__4
	__forceinline Vector_4_Out V3Andc(Vector_4_In inVector1, Vector_4_In inVector2)
	{
		Vector_4 outVect;

		union
		{
			float f;
			int i;
		} TempResult1, TempResult2, TempResult3, Temp1, Temp2, Temp3;

		TempResult1.f = inVector1.x;
		TempResult2.f = inVector1.y;
		TempResult3.f = inVector1.z;
		Temp1.f = inVector2.x;
		Temp2.f = inVector2.y;
		Temp3.f = inVector2.z;
		TempResult1.i &= ~Temp1.i;
		TempResult2.i &= ~Temp2.i;
		TempResult3.i &= ~Temp3.i;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		return outVect;
	}
#endif // V3ANDC__4

	
} // namespace Vec
} // namespace rage

