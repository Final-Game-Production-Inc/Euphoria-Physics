
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

namespace rage
{
namespace Vec
{

#ifndef V3GETELEM
	__forceinline float GetElem( Vector_3_In inVector, unsigned int elem )
	{
		mthAssertf( elem <= 2 , "Invalid element index %d out of range [0,2]", elem);
		return ((float*)(&inVector))[elem];
	}
#endif // V3GETELEM

#ifndef V3GETELEMREF
	__forceinline float& GetElemRef( Vector_3_Ptr pInVector, unsigned int elem )
	{
		mthAssertf( elem <= 2 , "Invalid element index %d out of range [0,2]", elem );
		return ((float*)(pInVector))[elem];
	}
#endif // V3GETELEMREF

#ifndef V3GETELEMREF_CONST
	__forceinline const float& GetElemRef( Vector_3_ConstPtr pInVector, unsigned int elem )
	{
		mthAssertf( elem <= 2 , "Invalid element index %d out of range [0,2]", elem );
		return ((const float*)(pInVector))[elem];
	}
#endif // V3GETELEMREF_CONST

#ifndef V3GETX
	__forceinline float GetX( Vector_3_In inVector )
	{
		return inVector.x;
	}
#endif // V3GETX

#ifndef V3GETY
	__forceinline float GetY( Vector_3_In inVector )
	{
		return inVector.y;
	}
#endif // V3GETY

#ifndef V3GETZ
	__forceinline float GetZ( Vector_3_In inVector )
	{
		return inVector.z;
	}
#endif // V3GETZ

#ifndef V3GETXV
	__forceinline Vector_3_Out GetXV( Vector_3_In inVector )
	{
		Vector_3 outVect;
		float x = inVector.x;
		outVect.x = x;
		outVect.y = x;
		outVect.z = x;
		return outVect;
	}
#endif // V3GETXV

#ifndef V3GETYV
	__forceinline Vector_3_Out GetYV( Vector_3_In inVector )
	{
		Vector_3 outVect;
		float y = inVector.y;
		outVect.x = y;
		outVect.y = y;
		outVect.z = y;
		return outVect;
	}
#endif // V3GETYV

#ifndef V3GETZV
	__forceinline Vector_3_Out GetZV( Vector_3_In inVector )
	{
		Vector_3 outVect;
		float z = inVector.z;
		outVect.x = z;
		outVect.y = z;
		outVect.z = z;
		return outVect;
	}
#endif // V3GETZV

#ifndef V3SETX
	__forceinline void SetX( Vector_3_InOut inoutVector, float floatVal )
	{
		inoutVector.x = floatVal;
	}
#endif // V3SETX

#ifndef V3SETY
	__forceinline void SetY( Vector_3_InOut inoutVector, float floatVal )
	{
		inoutVector.y = floatVal;
	}
#endif // V3SETY

#ifndef V3SETZ
	__forceinline void SetZ( Vector_3_InOut inoutVector, float floatVal )
	{
		inoutVector.z = floatVal;
	}
#endif // V3SETZ

#ifndef V3SPLATX
	__forceinline Vector_3_Out V3SplatX( Vector_3_In inVector )
	{
		Vector_3 outVect;
		float x = inVector.x;
		outVect.x = x;
		outVect.y = x;
		outVect.z = x;
		return outVect;
	}
#endif // V3SPLATX

#ifndef V3SPLATY
	__forceinline Vector_3_Out V3SplatY( Vector_3_In inVector )
	{
		Vector_3 outVect;
		float y = inVector.y;
		outVect.x = y;
		outVect.y = y;
		outVect.z = y;
		return outVect;
	}
#endif // V3SPLATY

#ifndef V3SPLATZ
	__forceinline Vector_3_Out V3SplatZ( Vector_3_In inVector )
	{
		Vector_3 outVect;
		float z = inVector.z;
		outVect.x = z;
		outVect.y = z;
		outVect.z = z;
		return outVect;
	}
#endif // V3SPLATZ

#ifndef V3SET_3F
	__forceinline void V3Set( Vector_3_InOut inoutVector, float x0, float y0, float z0 )
	{
		inoutVector.x = x0;
		inoutVector.y = y0;
		inoutVector.z = z0;
	}
#endif // V3SET_3F

#ifndef V3SET_V
	__forceinline void V3Set( Vector_3_InOut inoutVector, Vector_3_In inVector )
	{
		inoutVector = inVector;
	}
#endif // V3SET_V

#ifndef V3SET
	__forceinline void V3Set( Vector_3_InOut inoutVector, float s )
	{
		inoutVector.x = s;
		inoutVector.y = s;
		inoutVector.z = s;
	}
#endif // V3SET

#ifndef V3ZEROCOMPONENTS
	__forceinline void V3ZeroComponents( Vector_3_InOut inoutVector )
	{
		inoutVector.x = inoutVector.y = inoutVector.z = 0.0f;
	}
#endif // V3ZEROCOMPONENTS


	//============================================================================
	// Standard Algebra

#ifndef V3ADDNET
	__forceinline Vector_3_Out V3AddNet( Vector_3_In inVector, Vector_3_In toAdd )
	{
		Vector_3 modifiedAdd;
		float dot = V3Dot(inVector, toAdd);
		if( dot > 0.0f )
		{
			// The given vector has a positive dot product with this vector, so remove its parallel component.
			modifiedAdd = V3SubtractScaled( toAdd, inVector, dot*V3InvMagSquared( inVector ) );
		}

		// Add the given vector to this vector, less any positive parallel component.
		return V3Add( inVector, modifiedAdd );
	}
#endif // V3ADDNET

#ifndef V3EXTEND
	__forceinline Vector_3_Out V3Extend( Vector_3_In inVector, Vector_3_In amount )
	{
		float invMag = V3InvMag( inVector );
		Vector_3 scaleValue;
		scaleValue = V3AddScaled( V3Constant(V_ONE), amount, invMag );
		return V3Scale( inVector, scaleValue );
	}
#endif // V3EXTEND

#ifndef V3ANGLE
	__forceinline float V3Angle(Vector_3_In v1, Vector_3_In v2)
	{
		float magSquaredProduct = V3MagSquared(v1) * V3MagSquared(v2);
		float invMag = FPInvSqrt(magSquaredProduct);
		return FPACos( V3Dot( v1, v2 ) * invMag );
	}
#endif // V3ANGLE

#ifndef V3ANGLENORMINPUT
	__forceinline float V3AngleNormInput(Vector_3_In v1, Vector_3_In v2)
	{
		return FPACos( V3Dot( v1, v2 ) );
	}
#endif // V3ANGLENORMINPUT

#ifndef V3ANGLEX
	__forceinline float V3AngleX(Vector_3_In v1, Vector_3_In v2)
	{
		float mag2 = (v1.y * v1.y + v1.z * v1.z) * (v2.y * v2.y + v2.z * v2.z);
		float invMag = FPInvSqrt( mag2 );
		float valToClamp = (v1.y*v2.y + v1.z*v2.z)*invMag;

		return FPIfGteZeroThenElse(v1.y*v2.z-v1.z*v2.y, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
	}
#endif // V3ANGLEX

#ifndef V3ANGLEY
	__forceinline float V3AngleY(Vector_3_In v1, Vector_3_In v2)
	{
		float mag2 = (v1.x * v1.x + v1.z * v1.z) * (v2.x * v2.x + v2.z * v2.z);
		float invMag = FPInvSqrt( mag2 );
		float valToClamp = (v1.x * v2.x + v1.z * v2.z) * invMag;

		return FPIfGteZeroThenElse(v1.z*v2.x-v1.x*v2.z, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
	}
#endif // V3ANGLEY

#ifndef V3ANGLEZ
	__forceinline float V3AngleZ(Vector_3_In v1, Vector_3_In v2)
	{
		float mag2 = (v1.x * v1.x + v1.y * v1.y) * (v2.x * v2.x + v2.y * v2.y);
		float invMag = FPInvSqrt( mag2 );
		float valToClamp = (v1.x * v2.x + v1.y * v2.y) * invMag;

		return FPIfGteZeroThenElse(v1.x*v2.y-v1.y*v2.x, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
	}
#endif // V3ANGLEZ

#ifndef V3ANGLEXNORMINPUT
	__forceinline float V3AngleXNormInput(Vector_3_In v1, Vector_3_In v2)
	{
		float valToClamp = (v1.y*v2.y + v1.z*v2.z);

		return FPIfGteZeroThenElse(v1.y*v2.z-v1.z*v2.y, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
	}
#endif // V3ANGLEXNORMINPUT

#ifndef V3ANGLEYNORMINPUT
	__forceinline float V3AngleYNormInput(Vector_3_In v1, Vector_3_In v2)
	{
		float valToClamp = (v1.x * v2.x + v1.z * v2.z);

		return FPIfGteZeroThenElse(v1.z*v2.x-v1.x*v2.z, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
	}
#endif // V3ANGLEYNORMINPUT

#ifndef V3ANGLEZNORMINPUT
	__forceinline float V3AngleZNormInput(Vector_3_In v1, Vector_3_In v2)
	{
		float valToClamp = (v1.x * v2.x + v1.y * v2.y);

		return FPIfGteZeroThenElse(v1.x*v2.y-v1.y*v2.x, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
	}
#endif // V3ANGLEZNORMINPUT

#ifndef V3ANGLEV
	__forceinline Vector_3_Out V3AngleV(Vector_3_In v1, Vector_3_In v2)
	{
		float magSquaredProduct = V3MagSquared(v1) * V3MagSquared(v2);
		float invMag = FPInvSqrt(magSquaredProduct);
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = FPACos( V3Dot( v1, v2 ) * invMag );
		return outVect;
	}
#endif // V3ANGLEV

#ifndef V3ANGLEVNORMINPUT
	__forceinline Vector_3_Out V3AngleVNormInput(Vector_3_In v1, Vector_3_In v2)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = FPACos( V3Dot( v1, v2 ) );
		return outVect;
	}
#endif // V3ANGLEVNORMINPUT

#ifndef V3ANGLEXV
	__forceinline Vector_3_Out V3AngleXV(Vector_3_In v1, Vector_3_In v2)
	{
		float mag2 = (v1.y * v1.y + v1.z * v1.z) * (v2.y * v2.y + v2.z * v2.z);
		float invMag = FPInvSqrt( mag2 );
		float valToClamp = (v1.y*v2.y + v1.z*v2.z)*invMag;

		float retVal = FPIfGteZeroThenElse(v1.y*v2.z-v1.z*v2.y, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = retVal;
		return outVect;
	}

#endif // V3ANGLEXV

#ifndef V3ANGLEYV
	__forceinline Vector_3_Out V3AngleYV(Vector_3_In v1, Vector_3_In v2)
	{
		float mag2 = (v1.x * v1.x + v1.z * v1.z) * (v2.x * v2.x + v2.z * v2.z);
		float invMag = FPInvSqrt( mag2 );
		float valToClamp = (v1.x * v2.x + v1.z * v2.z) * invMag;

		float retVal = FPIfGteZeroThenElse(v1.z*v2.x-v1.x*v2.z, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = retVal;
		return outVect;
	}
#endif // V3ANGLEYV

#ifndef V3ANGLEZV
	__forceinline Vector_3_Out V3AngleZV(Vector_3_In v1, Vector_3_In v2)
	{
		float mag2 = (v1.x * v1.x + v1.y * v1.y) * (v2.x * v2.x + v2.y * v2.y);
		float invMag = FPInvSqrt( mag2 );
		float valToClamp = (v1.x * v2.x + v1.y * v2.y) * invMag;

		float retVal = FPIfGteZeroThenElse(v1.x*v2.y-v1.y*v2.x, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = retVal;
		return outVect;
	}
#endif // V3ANGLEZV

#ifndef V3ANGLEXVNORMINPUT
	__forceinline Vector_3_Out V3AngleXVNormInput(Vector_3_In v1, Vector_3_In v2)
	{
		float valToClamp = (v1.y*v2.y + v1.z*v2.z);

		float retVal = FPIfGteZeroThenElse(v1.y*v2.z-v1.z*v2.y, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = retVal;
		return outVect;
	}
#endif // V3ANGLEXVNORMINPUT

#ifndef V3ANGLEYVNORMINPUT
	__forceinline Vector_3_Out V3AngleYVNormInput(Vector_3_In v1, Vector_3_In v2)
	{
		float valToClamp = (v1.x * v2.x + v1.z * v2.z);

		float retVal = FPIfGteZeroThenElse(v1.z*v2.x-v1.x*v2.z, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = retVal;
		return outVect;
	}
#endif // V3ANGLEYVNORMINPUT

#ifndef V3ANGLEZVNORMINPUT
	__forceinline Vector_3_Out V3AngleZVNormInput(Vector_3_In v1, Vector_3_In v2)
	{
		float valToClamp = (v1.x * v2.x + v1.y * v2.y);

		float retVal = FPIfGteZeroThenElse(v1.x*v2.y-v1.y*v2.x, 1.0f, -1.0f) *
			FPACos( FPClamp( valToClamp, -1.0f, 1.0f ) );
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = retVal;
		return outVect;
	}
#endif // V3ANGLEZVNORMINPUT

#ifndef V3APPROACHSTRAIGHT
	__forceinline Vector_3_Out V3ApproachStraight(Vector_3_In position, Vector_3_In goal, float rate, float time, unsigned int& rResult)
	{
		Vector_3 directionXYZ = V3Subtract( goal, position );
		Vector_3 unitDirectionXYZ = V3Normalize( directionXYZ );
		float scalarDistance = rate * time;
		Vector_3 finalPosition = V3AddScaled( position, unitDirectionXYZ, scalarDistance );

		Vector_3 directionXYZNew = V3Subtract( goal, finalPosition );
		Vector_3 unitDirectionXYZNew = V3Normalize( directionXYZNew );

		unsigned int haventReachedGoal = V3IsCloseAll( unitDirectionXYZ, unitDirectionXYZNew, SMALL_FLOAT );
		rResult = (haventReachedGoal ^ 0x1);

		return finalPosition;
	}
#endif // V3APPROACHSTRAIGHT

#ifndef V3MAKEORTHONORMALS
	__forceinline void V3MakeOrthonormals(Vector_3_In inVector, Vector_3_InOut ortho1, Vector_3_InOut ortho2)
	{
		// Make sure this vector has a length near 1.
		mthAssertf( FPAbs( V3MagSquared(inVector) - 1.0f ) < SMALL_FLOAT, "Input vector's length must be near 1.0f" );

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
#endif // V3MAKEORTHONORMALS

#ifndef V3REFLECT
	__forceinline Vector_3_Out V3Reflect( Vector_3_In inVector, Vector_3_In planeNormal )
	{
		float dot = V3Dot( inVector, planeNormal );
		Vector_3 outVect;
		outVect.x = (inVector.x - 2.0f*dot*planeNormal.x);
		outVect.y = (inVector.y - 2.0f*dot*planeNormal.y);
		outVect.z = (inVector.z - 2.0f*dot*planeNormal.z);
		return outVect;
	}
#endif // V3REFLECT

#ifndef V3ROTATEABOUTXAXIS
	__forceinline Vector_3_Out V3RotateAboutXAxis( Vector_3_In inVector, float radians )
	{
		float tsin, tcos;
		FPSinAndCos(tsin, tcos, radians);

		Vector_3 outVect;
		outVect.x = inVector.x;
		outVect.y = inVector.y * tcos - inVector.z * tsin;
		outVect.z = inVector.y * tsin + inVector.z * tcos;
		return outVect;
	}
#endif // V3ROTATEABOUTXAXIS

#ifndef V3ROTATEABOUTYAXIS
	__forceinline Vector_3_Out V3RotateAboutYAxis( Vector_3_In inVector, float radians )
	{
		float tsin, tcos;
		FPSinAndCos(tsin, tcos, radians);

		Vector_3 outVect;
		outVect.x = inVector.z * tsin + inVector.x * tcos;
		outVect.y = inVector.y;
		outVect.z = inVector.z * tcos - inVector.x * tsin;
		return outVect;
	}
#endif // V3ROTATEABOUTYAXIS

#ifndef V3ROTATEABOUTZAXIS
	__forceinline Vector_3_Out V3RotateAboutZAxis( Vector_3_In inVector, float radians )
	{
		float tsin, tcos;
		FPSinAndCos(tsin, tcos, radians);

		Vector_3 outVect;
		outVect.x = inVector.x * tcos - inVector.y * tsin;
		outVect.y = inVector.x * tsin + inVector.y * tcos;
		outVect.z = inVector.z;
		return outVect;
	}
#endif // V3ROTATEABOUTZAXIS

#ifndef V3CLAMPMAG
	inline Vector_3_Out V3ClampMag( Vector_3_In v, float minMag, float maxMag )
	{
		Vector_3 result = v;
		float mag2 = V3MagSquared( result );
		float invMag = FPInvSqrt( mag2 );
		float maxMag2 = maxMag * maxMag;
		float minMag2 = minMag * minMag;
		float multiplier = FPIfGteThenElse( mag2, maxMag2, maxMag*invMag, FPIfLteThenElse(mag2, minMag2, minMag*invMag, 1.0f) );
		result = V3Scale( result, multiplier );
		return result;
	}
#endif // V3CLAMPMAG

#ifndef V3SCALE
	__forceinline Vector_3_Out V3Scale( Vector_3_In inVector, float floatVal )
	{
		Vector_3 outVect;
		outVect.x = inVector.x * floatVal;
		outVect.y = inVector.y * floatVal;
		outVect.z = inVector.z * floatVal;
		return outVect;
	}
#endif // V3SCALE

#ifndef V3INVSCALE
	__forceinline Vector_3_Out V3InvScale( Vector_3_In inVector, float floatVal )
	{
		Vector_3 outVect;
		float invVal = 1.0f/floatVal;
		outVect.x = inVector.x * invVal;
		outVect.y = inVector.y * invVal;
		outVect.z = inVector.z * invVal;
		return outVect;
	}
#endif // V3INVSCALE

#ifndef V3INVSCALE_V
	__forceinline Vector_3_Out V3InvScale( Vector_3_In inVector, Vector_3_In floatVal )
	{
		Vector_3 outVect = inVector;
		outVect.x = inVector.x / floatVal.x;
		outVect.y = inVector.y / floatVal.y;
		outVect.z = inVector.z / floatVal.z;
		return outVect;
	}
#endif // V3INVSCALE_V

#ifndef V3INVSCALESAFE
	__forceinline Vector_3_Out V3InvScaleSafe( Vector_3_In inVector, float floatVal, float errVal )
	{
		Vector_3 outVect;
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
#endif // V3INVSCALESAFE

#ifndef V3INVSCALESAFE_V
	__forceinline Vector_3_Out V3InvScaleSafe( Vector_3_In inVector, Vector_3_In floatVal, float errVal )
	{
		Vector_3 outVect;
		outVect.x = (floatVal.x != 0.0f) ? inVector.x/floatVal.x : errVal;
		outVect.y = (floatVal.y != 0.0f) ? inVector.y/floatVal.y : errVal;
		outVect.z = (floatVal.z != 0.0f) ? inVector.z/floatVal.z : errVal;
		return outVect;
	}
#endif // V3INVSCALESAFE_V

#ifndef V3INVSCALEFAST
	__forceinline Vector_3_Out V3InvScaleFast( Vector_3_In inVector, float floatVal )
	{
		Vector_3 outVect;
		float inv = FPInvertFast(floatVal);
		outVect.x = inVector.x * inv;
		outVect.y = inVector.y * inv;
		outVect.z = inVector.z * inv;
		return outVect;
	}
#endif // V3INVSCALEFAST

#ifndef V3INVSCALEFAST_V
	__forceinline Vector_3_Out V3InvScaleFast( Vector_3_In inVector, Vector_3_In floatVal )
	{
		Vector_3 outVect;
		outVect.x = inVector.x * FPInvertFast(floatVal.x);
		outVect.y = inVector.y * FPInvertFast(floatVal.y);
		outVect.z = inVector.z * FPInvertFast(floatVal.z);
		return outVect;
	}
#endif // V3INVSCALEFAST_V

#ifndef V3INVSCALEFASTSAFE
	__forceinline Vector_3_Out V3InvScaleFastSafe( Vector_3_In inVector, float floatVal, float errVal )
	{
		Vector_3 outVect;
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
#endif // V3INVSCALEFASTSAFE

#ifndef V3INVSCALEFASTSAFE_V
	__forceinline Vector_3_Out V3InvScaleFastSafe( Vector_3_In inVector, Vector_3_In floatVal, float errVal )
	{
		Vector_3 outVect;
		outVect.x = (floatVal.x != 0.0f) ? inVector.x*FPInvertFast(floatVal.x) : errVal;
		outVect.y = (floatVal.y != 0.0f) ? inVector.y*FPInvertFast(floatVal.y) : errVal;
		outVect.z = (floatVal.z != 0.0f) ? inVector.z*FPInvertFast(floatVal.z) : errVal;
		return outVect;
	}
#endif // V3INVSCALEFASTSAFE_V

#ifndef V3ADD_3F
	__forceinline Vector_3_Out V3Add( Vector_3_In inVector, float sx, float sy, float sz )
	{
		Vector_3 outVect;
		outVect.x = inVector.x + sx;
		outVect.y = inVector.y + sy;
		outVect.z = inVector.z + sz;
		return outVect;
	}
#endif // V3ADD_3F

#ifndef V3ADD_V
	__forceinline Vector_3_Out V3Add( Vector_3_In inVector1, Vector_3_In inVector2 )
	{
		Vector_3 outVect = inVector1;
		outVect.x = inVector1.x + inVector2.x;
		outVect.y = inVector1.y + inVector2.y;
		outVect.z = inVector1.z + inVector2.z;
		return outVect;
	}
#endif // V3ADD_V

#ifndef V3ADDINT_V
	__forceinline Vector_3_Out V3AddInt( Vector_3_In inVector1, Vector_3_In inVector2 )
	{
		Vector_3 outVect;

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

		Temp1.f = inVector1.z;
		Temp2.f = inVector2.z;
		TempResult.i = Temp1.i + Temp2.i;
		outVect.z = TempResult.f;

		return outVect;
	}
#endif // V3ADDINT_V

#ifndef V3SUBTRACTINT_V
	__forceinline Vector_3_Out V3SubtractInt( Vector_3_In inVector1, Vector_3_In inVector2 )
	{
		Vector_3 outVect;

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

		Temp1.f = inVector1.z;
		Temp2.f = inVector2.z;
		TempResult.i = Temp1.i - Temp2.i;
		outVect.z = TempResult.f;

		return outVect;
	}
#endif // V3SUBTRACTINT_V

#ifndef V3ADDSCALED
	__forceinline Vector_3_Out V3AddScaled( Vector_3_In inVector1, Vector_3_In inVector2, float floatValue )
	{
		Vector_3 outVect;
		outVect.x = inVector1.x + inVector2.x * floatValue;
		outVect.y = inVector1.y + inVector2.y * floatValue;
		outVect.z = inVector1.z + inVector2.z * floatValue;
		return outVect;
	}
#endif // V3ADDSCALED

#ifndef V3ADDSCALED_V
	__forceinline Vector_3_Out V3AddScaled( Vector_3_In inVector1, Vector_3_In inVector2, Vector_3_In floatValue )
	{
		Vector_3 outVect;
		outVect.x = inVector1.x + inVector2.x * floatValue.x;
		outVect.y = inVector1.y + inVector2.y * floatValue.y;
		outVect.z = inVector1.z + inVector2.z * floatValue.z;
		return outVect;
	}
#endif // V3ADDSCALED_V

#ifndef V3SUBTRACT_3F
	__forceinline Vector_3_Out V3Subtract( Vector_3_In inVector, float sx, float sy, float sz )
	{
		Vector_3 outVect;
		outVect.x = inVector.x - sx;
		outVect.y = inVector.y - sy;
		outVect.z = inVector.z - sz;
		return outVect;
	}
#endif // V3SUBTRACT_3F

#ifndef V3SUBTRACT_V
	__forceinline Vector_3_Out V3Subtract( Vector_3_In inVector1, Vector_3_In inVector2 )
	{
		Vector_3 outVect;
		outVect.x = inVector1.x - inVector2.x;
		outVect.y = inVector1.y - inVector2.y;
		outVect.z = inVector1.z - inVector2.z;
		return outVect;
	}
#endif // V3SUBTRACT_V

#ifndef V3SUBTRACTSCALED
	__forceinline Vector_3_Out V3SubtractScaled( Vector_3_In inVector1, Vector_3_In inVector2, float floatValue )
	{
		Vector_3 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue;
		outVect.y = inVector1.y - inVector2.y * floatValue;
		outVect.z = inVector1.z - inVector2.z * floatValue;
		return outVect;
	}
#endif // V3SUBTRACTSCALED

#ifndef V3SUBTRACTSCALED_V
	__forceinline Vector_3_Out V3SubtractScaled( Vector_3_In inVector1, Vector_3_In inVector2, Vector_3_In floatValue )
	{
		Vector_3 outVect;
		outVect.x = inVector1.x - inVector2.x * floatValue.x;
		outVect.y = inVector1.y - inVector2.y * floatValue.y;
		outVect.z = inVector1.z - inVector2.z * floatValue.z;
		return outVect;
	}
#endif // V3SUBTRACTSCALED_V

#ifndef V3SCALE_V
	__forceinline Vector_3_Out V3Scale( Vector_3_In inVector1, Vector_3_In inVector2 )
	{
		Vector_3 outVect;
		outVect.x = inVector1.x * inVector2.x;
		outVect.y = inVector1.y * inVector2.y;
		outVect.z = inVector1.z * inVector2.z;
		return outVect;
	}
#endif // V3SCALE_V

#ifndef V3NEGATE
	__forceinline Vector_3_Out V3Negate(Vector_3_In inVector)
	{
		Vector_3 outVect;
		outVect.x = -inVector.x;
		outVect.y = -inVector.y;
		outVect.z = -inVector.z;
		return outVect;
	}
#endif // V3NEGATE

#ifndef V3ABS
	__forceinline Vector_3_Out V3Abs(Vector_3_In inVector)
	{
		Vector_3 outVect;
		outVect.x = FPAbs(inVector.x);
		outVect.y = FPAbs(inVector.y);
		outVect.z = FPAbs(inVector.z);
		return outVect;
	}
#endif // V3ABS

#ifndef V3INVERTBITS
	__forceinline Vector_3_Out V3InvertBits(Vector_3_In inVector)
	{
		Vector_3 outVect;

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

		Temp1.f = inVector.z;
		TempResult.i = Temp1.i ^ 0xFFFFFFFF;
		outVect.z = TempResult.f;

		return outVect;
	}
#endif // V3INVERTBITS

#ifndef V3INVERT
	__forceinline Vector_3_Out V3Invert(Vector_3_In inVector)
	{
		Vector_3 outVect;
		outVect.x = 1.0f/inVector.x;
		outVect.y = 1.0f/inVector.y;
		outVect.z = 1.0f/inVector.z;
		return outVect;
	}
#endif // V3INVERT

#ifndef V3INVERTSAFE
	__forceinline Vector_3_Out V3InvertSafe(Vector_3_In inVector, float errVal)
	{
		Vector_3 outVect;
		outVect.x = ( inVector.x == 0.0f ? errVal : 1.0f/inVector.x );
		outVect.y = ( inVector.y == 0.0f ? errVal : 1.0f/inVector.y );
		outVect.z = ( inVector.z == 0.0f ? errVal : 1.0f/inVector.z );
		return outVect;
	}
#endif // V3INVERTSAFE

#ifndef V3INVERTFAST
	__forceinline Vector_3_Out V3InvertFast(Vector_3_In inVector)
	{
		Vector_3 outVect;
		outVect.x = FPInvertFast(inVector.x);
		outVect.y = FPInvertFast(inVector.y);
		outVect.z = FPInvertFast(inVector.z);
		return outVect;
	}
#endif // V3INVERTFAST

#ifndef V3INVERTFASTSAFE
	__forceinline Vector_3_Out V3InvertFastSafe(Vector_3_In inVector, float errVal)
	{
		Vector_3 outVect;
		outVect.x = ( inVector.x == 0.0f ? errVal : FPInvertFast(inVector.x) );
		outVect.y = ( inVector.y == 0.0f ? errVal : FPInvertFast(inVector.y) );
		outVect.z = ( inVector.z == 0.0f ? errVal : FPInvertFast(inVector.z) );
		return outVect;
	}
#endif // V3INVERTFASTSAFE

#ifndef V3NORMALIZE
	__forceinline Vector_3_Out V3Normalize(Vector_3_In inVector)
	{
		Vector_3 outVect;
		float invMag = V3InvMag(inVector);
		outVect.x = inVector.x * invMag;
		outVect.y = inVector.y * invMag;
		outVect.z = inVector.z * invMag;
		return outVect;
	}
#endif // V3NORMALIZE

#ifndef V3NORMALIZESAFE
	__forceinline Vector_3_Out V3NormalizeSafe(Vector_3_In inVector, float errVal, float magSqThreshold)
	{
		Vector_3 outVect;
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
#endif // V3NORMALIZESAFE

#ifndef V3NORMALIZEFAST
	__forceinline Vector_3_Out V3NormalizeFast(Vector_3_In inVector)
	{
		Vector_3 outVect;
		float invMag = V3InvMagFast(inVector);
		outVect.x = inVector.x * invMag;
		outVect.y = inVector.y * invMag;
		outVect.z = inVector.z * invMag;
		return outVect;
	}
#endif // V3NORMALIZEFAST

#ifndef V3NORMALIZEFASTSAFE
	__forceinline Vector_3_Out V3NormalizeFastSafe(Vector_3_In inVector, float errVal, float magSqThreshold)
	{
		Vector_3 outVect;
		float mag2 = V3MagSquared(inVector);
		if(mag2 > magSqThreshold)
		{
			float invMag = FPInvSqrtFast(mag2);
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
#endif // V3NORMALIZEFASTSAFE

#ifndef V3DOT
	__forceinline float V3Dot(Vector_3_In a, Vector_3_In b)
	{
		return a.x*b.x + a.y*b.y + a.z*b.z;
	}
#endif // V3DOT

#ifndef V3DOTV
	__forceinline Vector_3_Out V3DotV(Vector_3_In a, Vector_3_In b)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3Dot(a, b);
		return outVect;
	}
#endif // V3DOTV

#ifndef V3CROSS
	__forceinline Vector_3_Out V3Cross(Vector_3_In a, Vector_3_In b)
	{
		Vector_3 outVect;
		outVect.x = a.y * b.z - a.z * b.y;
		outVect.y = a.z * b.x - a.x * b.z; 
		outVect.z = a.x * b.y - a.y * b.x;
		return outVect;
	}
#endif // V3CROSS

#ifndef V3ADDCROSSED
	__forceinline Vector_3_Out V3AddCrossed(Vector_3_In toAddTo, Vector_3_In a, Vector_3_In b)
	{
		Vector_3 outVect;
		outVect.x = toAddTo.x + a.y * b.z - a.z * b.y;
		outVect.y = toAddTo.y + a.z * b.x - a.x * b.z; 
		outVect.z = toAddTo.z + a.x * b.y - a.y * b.x;
		return outVect;
	}
#endif // V3ADDCROSSED

#ifndef V3SUBTRACTCROSSED
	__forceinline Vector_3_Out V3SubtractCrossed(Vector_3_In toSubtractFrom, Vector_3_In a, Vector_3_In b)
	{
		Vector_3 outVect;
		outVect.x = toSubtractFrom.x + b.y * a.z - b.z * a.y;
		outVect.y = toSubtractFrom.y + b.z * a.x - b.x * a.z; 
		outVect.z = toSubtractFrom.z + b.x * a.y - b.y * a.x;
		return outVect;
	}
#endif // V3SUBTRACTCROSSED

#ifndef V3AVERAGE
	__forceinline Vector_3_Out V3Average(Vector_3_In a, Vector_3_In b)
	{
		Vector_3 outVect;
		outVect.x = 0.5f * (a.x + b.x);
		outVect.y = 0.5f * (a.y + b.y);
		outVect.z = 0.5f * (a.z + b.z);
		return outVect;
	}
#endif // V3AVERAGE

#ifndef V3LERP
	__forceinline Vector_3_Out V3Lerp( float t, Vector_3_In a, Vector_3_In b )
	{
		Vector_3 outVect;
		outVect.x = FPLerp(t, a.x, b.x);
		outVect.y = FPLerp(t, a.y, b.y);
		outVect.z = FPLerp(t, a.z, b.z);
		return outVect;
	}
#endif // V3LERP

#ifndef V3LERP_V
	__forceinline Vector_3_Out V3Lerp( Vector_3_In t, Vector_3_In a, Vector_3_In b )
	{
		Vector_3 outVect;
		outVect.x = FPLerp(t.x, a.x, b.x);
		outVect.y = FPLerp(t.y, a.y, b.y);
		outVect.z = FPLerp(t.z, a.z, b.z);
		return outVect;
	}
#endif // V3LERP_V

#ifndef V3POW
	__forceinline Vector_3_Out V3Pow( Vector_3_In x, Vector_3_In y )
	{
		Vector_3 outVect;
		outVect.x = FPPow(x.x, y.x);
		outVect.y = FPPow(x.y, y.y);
		outVect.z = FPPow(x.z, y.z);
		return outVect;
	}
#endif // V3POW

#ifndef V3EXPT
	__forceinline Vector_3_Out V3Expt( Vector_3_In x )
	{
		Vector_3 outVect;
		outVect.x = FPExpt(x.x);
		outVect.y = FPExpt(x.y);
		outVect.z = FPExpt(x.z);
		return outVect;
	}
#endif // V3EXPT

#ifndef V3LOG2
	__forceinline Vector_3_Out V3Log2( Vector_3_In x )
	{
		// log2(f) = ln(f)/ln(2)
		Vector_3 outVect;
		outVect.x = FPLog2(x.x);
		outVect.y = FPLog2(x.y);
		outVect.z = FPLog2(x.z);
		return outVect;
	}
#endif

#ifndef V3LOG10
	__forceinline Vector_3_Out V3Log10( Vector_3_In x )
	{
		Vector_3 outVect;
		outVect.x = FPLog10(x.x);
		outVect.y = FPLog10(x.y);
		outVect.z = FPLog10(x.z);
		return outVect;
	}
#endif // V3LOG10

#ifndef V3SLOWINOUT
	__forceinline Vector_3_Out V3SlowInOut( Vector_3_In t )
	{
		Vector_3 outVect;
		outVect.x = FPSlowInOut( t.x );
		outVect.y = FPSlowInOut( t.y );
		outVect.z = FPSlowInOut( t.z );
		return outVect;
	}
#endif // V3SLOWINOUT

#ifndef V3SLOWIN
	__forceinline Vector_3_Out V3SlowIn( Vector_3_In t )
	{
		Vector_3 outVect;
		outVect.x = FPSlowIn( t.x );
		outVect.y = FPSlowIn( t.y );
		outVect.z = FPSlowIn( t.z );
		return outVect;
	}
#endif // V3SLOWIN

#ifndef V3SLOWOUT
	__forceinline Vector_3_Out V3SlowOut( Vector_3_In t )
	{
		Vector_3 outVect;
		outVect.x = FPSlowOut( t.x );
		outVect.y = FPSlowOut( t.y );
		outVect.z = FPSlowOut( t.z );
		return outVect;
	}
#endif // V3SLOWOUT

#ifndef V3BELLINOUT
	__forceinline Vector_3_Out V3BellInOut( Vector_3_In t )
	{
		Vector_3 outVect;
		outVect.x = FPBellInOut( t.x );
		outVect.y = FPBellInOut( t.y );
		outVect.z = FPBellInOut( t.z );
		return outVect;
	}
#endif // V3BELLINOUT

#ifndef V3RAMP
	__forceinline Vector_3_Out V3Ramp( Vector_3_In x, Vector_3_In funcInA, Vector_3_In funcInB, Vector_3_In funcOutA, Vector_3_In funcOutB )
	{
		Vector_3 outVect;
		outVect.x = FPRamp( x.x, funcInA.x, funcInB.x, funcOutA.x, funcOutB.x );
		outVect.y = FPRamp( x.y, funcInA.y, funcInB.y, funcOutA.y, funcOutB.y );
		outVect.z = FPRamp( x.z, funcInA.z, funcInB.z, funcOutA.z, funcOutB.z );
		return outVect;
	}
#endif // V3RAMP

#ifndef V3RAMPFAST
	__forceinline Vector_3_Out V3RampFast( Vector_3_In x, Vector_3_In funcInA, Vector_3_In funcInB, Vector_3_In funcOutA, Vector_3_In funcOutB )
	{
		Vector_3 outVect;
		outVect.x = FPRampFast( x.x, funcInA.x, funcInB.x, funcOutA.x, funcOutB.x );
		outVect.y = FPRampFast( x.y, funcInA.y, funcInB.y, funcOutA.y, funcOutB.y );
		outVect.z = FPRampFast( x.z, funcInA.z, funcInB.z, funcOutA.z, funcOutB.z );
		return outVect;
	}
#endif // V3RAMPFAST

#ifndef V3RANGE
	__forceinline Vector_3_Out V3Range( Vector_3_In t, Vector_3_In lower, Vector_3_In upper )
	{
		Vector_3 outVect;
		outVect.x = FPRange( t.x, lower.x, upper.x );
		outVect.y = FPRange( t.y, lower.y, upper.y );
		outVect.z = FPRange( t.z, lower.z, upper.z );
		return outVect;
	}
#endif // V3RANGE

#ifndef V3RANGEFAST
	__forceinline Vector_3_Out V3RangeFast( Vector_3_In t, Vector_3_In lower, Vector_3_In upper )
	{
		Vector_3 outVect;
		outVect.x = FPRangeFast( t.x, lower.x, upper.x );
		outVect.y = FPRangeFast( t.y, lower.y, upper.y );
		outVect.z = FPRangeFast( t.z, lower.z, upper.z );
		return outVect;
	}
#endif // V3RANGEFAST

#ifndef V3RANGECLAMP
	__forceinline Vector_3_Out V3RangeClamp( Vector_3_In t, Vector_3_In lower, Vector_3_In upper )
	{
		Vector_3 outVect;
		outVect.x = FPRangeClamp( t.x, lower.x, upper.x );
		outVect.y = FPRangeClamp( t.y, lower.y, upper.y );
		outVect.z = FPRangeClamp( t.z, lower.z, upper.z );
		return outVect;
	}
#endif // V3RANGECLAMP

#ifndef V3RANGECLAMPFAST
	__forceinline Vector_3_Out V3RangeClampFast( Vector_3_In t, Vector_3_In lower, Vector_3_In upper )
	{
		Vector_3 outVect;
		outVect.x = FPRangeClampFast( t.x, lower.x, upper.x );
		outVect.y = FPRangeClampFast( t.y, lower.y, upper.y );
		outVect.z = FPRangeClampFast( t.z, lower.z, upper.z );
		return outVect;
	}
#endif // V3RANGECLAMPFAST

	//============================================================================
	// Magnitude and distance

#ifndef V3SQRT
	__forceinline Vector_3_Out V3Sqrt( Vector_3_In v )
	{
		Vector_3 outVect;
		outVect.x = FPSqrt(v.x);
		outVect.y = FPSqrt(v.y);
		outVect.z = FPSqrt(v.z);
		return outVect;
	}
#endif // V3SQRT

#ifndef V3SQRTSAFE
	__forceinline Vector_3_Out V3SqrtSafe( Vector_3_In v, float errVal )
	{
		Vector_3 outVect;
		outVect.x = FPSqrtSafe( v.x, errVal );
		outVect.y = FPSqrtSafe( v.y, errVal );
		outVect.z = FPSqrtSafe( v.z, errVal );
		return outVect;
	}
#endif // V3SQRTSAFE

#ifndef V3SQRTFAST
	__forceinline Vector_3_Out V3SqrtFast( Vector_3_In v )
	{
		return V3Sqrt( v );
	}
#endif // V3SQRTFAST

#ifndef V3SQRTFASTSAFE
	__forceinline Vector_3_Out V3SqrtFastSafe( Vector_3_In v, float errVal )
	{
		return V3SqrtSafe( v, errVal );
	}
#endif // V3SQRTFASTSAFE

#ifndef V3INVSQRT
	__forceinline Vector_3_Out V3InvSqrt( Vector_3_In v )
	{
		Vector_3 outVect;
		outVect.x = FPInvSqrt(v.x);
		outVect.y = FPInvSqrt(v.y);
		outVect.z = FPInvSqrt(v.z);
		return outVect;
	}
#endif // V3INVSQRT

#ifndef V3INVSQRTSAFE
	__forceinline Vector_3_Out V3InvSqrtSafe( Vector_3_In v, float errVal )
	{
		Vector_3 outVect;
		outVect.x = FPInvSqrtSafe(v.x, errVal);
		outVect.y = FPInvSqrtSafe(v.y, errVal);
		outVect.z = FPInvSqrtSafe(v.z, errVal);
		return outVect;
	}
#endif // V3INVSQRTSAFE

#ifndef V3INVSQRTFAST
	__forceinline Vector_3_Out V3InvSqrtFast( Vector_3_In v )
	{
		Vector_3 outVect;
		outVect.x = FPInvSqrtFast(v.x);
		outVect.y = FPInvSqrtFast(v.y);
		outVect.z = FPInvSqrtFast(v.z);
		return outVect;
	}
#endif // V3INVSQRTFAST

#ifndef V3INVSQRTFASTSAFE
	__forceinline Vector_3_Out V3InvSqrtFastSafe( Vector_3_In v, float errVal )
	{
		Vector_3 outVect;
		outVect.x = FPInvSqrtFastSafe(v.x, errVal);
		outVect.y = FPInvSqrtFastSafe(v.y, errVal);
		outVect.z = FPInvSqrtFastSafe(v.z, errVal);
		return outVect;
	}
#endif // V3INVSQRTFASTSAFE

#ifndef V3MAG
	__forceinline float V3Mag( Vector_3_In v )
	{
		return FPSqrt( V3Dot( v, v ) );
	}
#endif // V3MAG

#ifndef V3MAGV
	__forceinline Vector_3_Out V3MagV( Vector_3_In v )
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3Mag( v );
		return outVect;
	}
#endif // V3MAGV

#ifndef V3MAGFAST
	__forceinline float V3MagFast( Vector_3_In v )
	{
		return V3Mag( v );
	}
#endif // V3MAGFAST

#ifndef V3MAGVFAST
	__forceinline Vector_3_Out V3MagVFast( Vector_3_In v )
	{
		return V3MagV( v );
	}
#endif // V3MAGVFAST

#ifndef V3MAGSQUARED
	__forceinline float V3MagSquared( Vector_3_In v )
	{
		return V3Dot( v, v );
	}
#endif // V3MAGSQUARED

#ifndef V3MAGSQUAREDV
	__forceinline Vector_3_Out V3MagSquaredV( Vector_3_In v )
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3MagSquared( v );
		return outVect;
	}
#endif // V3MAGSQUAREDV

#ifndef V3INVMAG
	__forceinline float V3InvMag( Vector_3_In v )
	{
		return FPInvSqrt( V3Dot( v, v ) );
	}
#endif // V3INVMAG

#ifndef V3INVMAGSAFE
	__forceinline float V3InvMagSafe( Vector_3_In v, float errVal )
	{
		float dot = V3Dot( v, v );
		return FPInvSqrtSafe( dot, errVal );
	}
#endif // V3INVMAGSAFE

#ifndef V3INVMAGFAST
	__forceinline float V3InvMagFast( Vector_3_In v )
	{
		return FPInvSqrtFast( V3Dot( v, v ) );
	}
#endif // V3INVMAGFAST

#ifndef V3INVMAGFASTSAFE
	__forceinline float V3InvMagFastSafe( Vector_3_In v, float errVal )
	{
		float dot = V3Dot( v, v );
		return FPInvSqrtFastSafe( dot, errVal );
	}
#endif // V3INVMAGFASTSAFE

#ifndef V3INVMAGV
	__forceinline Vector_3_Out V3InvMagV( Vector_3_In v )
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvMag( v );
		return outVect;
	}
#endif // V3INVMAGV

#ifndef V3INVMAGVSAFE
	__forceinline Vector_3_Out V3InvMagVSafe( Vector_3_In v, float errVal )
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvMagSafe( v, errVal );
		return outVect;
	}
#endif // V3INVMAGVSAFE

#ifndef V3INVMAGVFAST
	__forceinline Vector_3_Out V3InvMagVFast( Vector_3_In v )
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvMagFast( v );
		return outVect;
	}
#endif // V3INVMAGVFAST

#ifndef V3INVMAGVFASTSAFE
	__forceinline Vector_3_Out V3InvMagVFastSafe( Vector_3_In v, float errVal )
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvMagFastSafe( v, errVal );
		return outVect;
	}
#endif // V3INVMAGVFASTSAFE

#ifndef V3INVMAGSQUARED
	__forceinline float V3InvMagSquared( Vector_3_In v )
	{
		return 1.0f/V3MagSquared( v );
	}
#endif // V3INVMAGSQUARED

#ifndef V3INVMAGSQUAREDSAFE
	__forceinline float V3InvMagSquaredSafe( Vector_3_In v, float errVal )
	{
		float magSq = V3MagSquared( v );
		return FPIfGtZeroThenElse( magSq, 1.0f/magSq, errVal );
	}
#endif // V3INVMAGSQUAREDSAFE

#ifndef V3INVMAGSQUAREDFAST
	__forceinline float V3InvMagSquaredFast( Vector_3_In v )
	{
		return FPInvertFast( V3MagSquared( v ) );
	}
#endif // V3INVMAGSQUAREDFAST

#ifndef V3INVMAGSQUAREDFASTSAFE
	__forceinline float V3InvMagSquaredFastSafe( Vector_3_In v, float errVal )
	{
		return FPInvertFastSafe( V3MagSquared( v ), errVal );
	}
#endif // V3INVMAGSQUAREDFASTSAFE

#ifndef V3INVMAGSQUAREDV
	__forceinline Vector_3_Out V3InvMagSquaredV( Vector_3_In v )
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvMagSquared( v );
		return outVect;
	}
#endif // V3INVMAGSQUAREDV

#ifndef V3INVMAGSQUAREDVSAFE
	__forceinline Vector_3_Out V3InvMagSquaredVSafe( Vector_3_In v, float errVal )
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvMagSquaredSafe( v, errVal );
		return outVect;
	}
#endif // V3INVMAGSQUAREDVSAFE

#ifndef V3INVMAGSQUAREDVFAST
	__forceinline Vector_3_Out V3InvMagSquaredVFast( Vector_3_In v )
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvMagSquaredFast( v );
		return outVect;
	}
#endif // V3INVMAGSQUAREDVFAST

#ifndef V3INVMAGSQUAREDVFASTSAFE
	__forceinline Vector_3_Out V3InvMagSquaredVFastSafe( Vector_3_In v, float errVal )
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvMagSquaredFastSafe( v, errVal );
		return outVect;
	}
#endif // V3INVMAGSQUAREDVFASTSAFE

#ifndef V3DIST
	__forceinline float V3Dist(Vector_3_In a, Vector_3_In b)
	{
		return V3Mag( V3Subtract( a, b ) );
	}
#endif // V3DIST

#ifndef V3DISTFAST
	__forceinline float V3DistFast(Vector_3_In a, Vector_3_In b)
	{
		return V3Dist( a, b );
	}
#endif // V3DISTFAST

#ifndef V3DISTV
	__forceinline Vector_3_Out V3DistV(Vector_3_In a, Vector_3_In b)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3Dist( a, b );
		return outVect;
	}
#endif // V3DISTV

#ifndef V3DISTVFAST
	__forceinline Vector_3_Out V3DistVFast(Vector_3_In a, Vector_3_In b)
	{
		return V3DistV( a, b );
	}
#endif // V3DISTVFAST

#ifndef V3INVDIST
	__forceinline float V3InvDist(Vector_3_In a, Vector_3_In b)
	{
		return V3InvMag( V3Subtract( a, b ) );
	}
#endif // V3INVDIST

#ifndef V3INVDISTSAFE
	__forceinline float V3InvDistSafe(Vector_3_In a, Vector_3_In b, float errVal)
	{
		return V3InvMagSafe( V3Subtract( a, b ), errVal );
	}
#endif // V3INVDISTSAFE

#ifndef V3INVDISTFAST
	__forceinline float V3InvDistFast(Vector_3_In a, Vector_3_In b)
	{
		return V3InvMagFast( V3Subtract( a, b ) );
	}
#endif // V3INVDISTFAST

#ifndef V3INVDISTFASTSAFE
	__forceinline float V3InvDistFastSafe(Vector_3_In a, Vector_3_In b, float errVal)
	{
		return V3InvMagFastSafe( V3Subtract( a, b ), errVal );
	}
#endif // V3INVDISTFASTSAFE

#ifndef V3INVDISTV
	__forceinline Vector_3_Out V3InvDistV(Vector_3_In a, Vector_3_In b)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvDist( a, b );
		return outVect;
	}
#endif // V3INVDISTV

#ifndef V3INVDISTVSAFE
	__forceinline Vector_3_Out V3InvDistVSafe(Vector_3_In a, Vector_3_In b, float errVal)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvDistSafe( a, b, errVal );
		return outVect;
	}
#endif // V3INVDISTVSAFE

#ifndef V3INVDISTVFAST
	__forceinline Vector_3_Out V3InvDistVFast(Vector_3_In a, Vector_3_In b)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvDistFast( a, b );
		return outVect;
	}
#endif // V3INVDISTVFAST

#ifndef V3INVDISTVFASTSAFE
	__forceinline Vector_3_Out V3InvDistVFastSafe(Vector_3_In a, Vector_3_In b, float errVal)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvDistFastSafe( a, b, errVal );
		return outVect;
	}
#endif // V3INVDISTVFASTSAFE

#ifndef V3DISTSQUARED
	__forceinline float V3DistSquared(Vector_3_In a, Vector_3_In b)
	{
		return V3MagSquared( V3Subtract( a, b ) );
	}
#endif // V3DISTSQUARED

#ifndef V3DISTSQUAREDV
	__forceinline Vector_3_Out V3DistSquaredV(Vector_3_In a, Vector_3_In b)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3DistSquared( a, b );
		return outVect;
	}
#endif // V3DISTSQUAREDV

#ifndef V3INVDISTSQUARED
	__forceinline float V3InvDistSquared(Vector_3_In a, Vector_3_In b)
	{
		return V3InvMagSquared( V3Subtract( a, b ) );
	}
#endif // V3INVDISTSQUARED

#ifndef V3INVDISTSQUAREDSAFE
	__forceinline float V3InvDistSquaredSafe(Vector_3_In a, Vector_3_In b, float errVal)
	{
		return V3InvMagSquaredSafe( V3Subtract( a, b ), errVal );
	}
#endif // V3INVDISTSQUAREDSAFE

#ifndef V3INVDISTSQUAREDFAST
	__forceinline float V3InvDistSquaredFast(Vector_3_In a, Vector_3_In b)
	{
		return V3InvMagSquaredFast( V3Subtract( a, b ) );
	}
#endif // V3INVDISTSQUAREDFAST

#ifndef V3INVDISTSQUAREDFASTSAFE
	__forceinline float V3InvDistSquaredFastSafe(Vector_3_In a, Vector_3_In b, float errVal)
	{
		return V3InvMagSquaredFastSafe( V3Subtract( a, b ), errVal );
	}
#endif // V3INVDISTSQUAREDFASTSAFE

#ifndef V3INVDISTSQUAREDV
	__forceinline Vector_3_Out V3InvDistSquaredV(Vector_3_In a, Vector_3_In b)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvDistSquared( a, b );
		return outVect;
	}
#endif // V3INVDISTSQUAREDV

#ifndef V3INVDISTSQUAREDVSAFE
	__forceinline Vector_3_Out V3InvDistSquaredVSafe(Vector_3_In a, Vector_3_In b, float errVal)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvDistSquaredSafe( a, b, errVal );
		return outVect;
	}
#endif // V3INVDISTSQUAREDVSAFE

#ifndef V3INVDISTSQUAREDVFAST
	__forceinline Vector_3_Out V3InvDistSquaredVFast(Vector_3_In a, Vector_3_In b)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvDistSquaredFast( a, b );
		return outVect;
	}
#endif // V3INVDISTSQUAREDVFAST

#ifndef V3INVDISTSQUAREDVFASTSAFE
	__forceinline Vector_3_Out V3InvDistSquaredVFastSafe(Vector_3_In a, Vector_3_In b, float errVal)
	{
		Vector_3 outVect;
		outVect.x = outVect.y = outVect.z = V3InvDistSquaredFastSafe( a, b, errVal );
		return outVect;
	}
#endif // V3INVDISTSQUAREDVFASTSAFE

	//============================================================================
	// Conversion functions

#ifndef V3FLOATTOINTRAW
	template <int exponent>
	__forceinline Vector_3_Out V3FloatToIntRaw(Vector_3_In inVector)
	{
		float multiplier = static_cast<float>(1 << exponent);

		Vector_3 outVect;

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
#endif // V3FLOATTOINTRAW

#ifndef V3INTTOFLOATRAW
	template <int exponent>
	__forceinline Vector_3_Out V3IntToFloatRaw(Vector_3_In inVector)
	{
		float divider = static_cast<float>(1 << exponent);
		float invDiv = 1.0f/divider;

		Vector_3 outVect;

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
#endif // V3INTTOFLOATRAW

#ifndef V3ROUNDTONEARESTINT
	__forceinline Vector_3_Out V3RoundToNearestInt(Vector_3_In inVector)
	{
		Vector_3 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x+(FPIfGteZeroThenElse(inVector.x, 0.5f, -0.5f))) );
		outVect.y = static_cast<float>( static_cast<int>(inVector.y+(FPIfGteZeroThenElse(inVector.y, 0.5f, -0.5f))) );
		outVect.z = static_cast<float>( static_cast<int>(inVector.z+(FPIfGteZeroThenElse(inVector.z, 0.5f, -0.5f))) );
		return outVect;
	}
#endif // V3ROUNDTONEARESTINT

#ifndef V3ROUNDTONEARESTINTZERO
	__forceinline Vector_3_Out V3RoundToNearestIntZero(Vector_3_In inVector)
	{
		Vector_3 outVect;
		outVect.x = static_cast<float>( static_cast<int>(inVector.x) );
		outVect.y = static_cast<float>( static_cast<int>(inVector.y) );
		outVect.z = static_cast<float>( static_cast<int>(inVector.z) );
		return outVect;
	}
#endif // V3ROUNDTONEARESTINTZERO

#ifndef V3ROUNDTONEARESTINTNEGINF
	__forceinline Vector_3_Out V3RoundToNearestIntNegInf(Vector_3_In inVector)
	{
		Vector_3 outVect;
		outVect.x = FPFloor( inVector.x );
		outVect.y = FPFloor( inVector.y );
		outVect.z = FPFloor( inVector.z );
		return outVect;
	}
#endif // V3ROUNDTONEARESTINTNEGINF

#ifndef V3ROUNDTONEARESTINTPOSINF
	__forceinline Vector_3_Out V3RoundToNearestIntPosInf(Vector_3_In inVector)
	{
		Vector_3 outVect;
		outVect.x = FPCeil( inVector.x );
		outVect.y = FPCeil( inVector.y );
		outVect.z = FPCeil( inVector.z );
		return outVect;
	}
#endif // V3ROUNDTONEARESTINTPOSINF

	//============================================================================
	// Comparison functions

#ifndef V3ISBETWEENNEGANDPOSBOUNDS
	__forceinline unsigned int V3IsBetweenNegAndPosBounds( Vector_3_In testVector, Vector_3_In boundsVector )
	{
		return (	testVector.x <= boundsVector.x && testVector.x >= -boundsVector.x &&
					testVector.y <= boundsVector.y && testVector.y >= -boundsVector.y &&
					testVector.z <= boundsVector.z && testVector.z >= -boundsVector.z ? 1u : 0u );
	}
#endif // V3ISBETWEENNEGANDPOSBOUNDS

#ifndef V3ISZEROALL
	__forceinline unsigned int V3IsZeroAll(Vector_3_In inVector)
	{
		return ( inVector.x == 0.0f && inVector.y == 0.0f && inVector.z == 0.0f ? 1u : 0u );
	}
#endif // V3ISZEROALL

#ifndef V3ISZERONONE
	__forceinline unsigned int V3IsZeroNone(Vector_3_In inVector)
	{
		return ( inVector.x != 0.0f && inVector.y != 0.0f && inVector.z != 0.0f ? 1u : 0u );
	}
#endif // V3ISZERONONE

#ifndef V3ISEQUALALL
	__forceinline unsigned int V3IsEqualAll(Vector_3_In inVector1, Vector_3_In inVector2)
	{
		return ( inVector1.x == inVector2.x &&
				inVector1.y == inVector2.y &&
				inVector1.z == inVector2.z ? 1u : 0u );
	}
#endif // V3ISEQUALALL

#ifndef V3ISEQUALNONE
	__forceinline unsigned int V3IsEqualNone(Vector_3_In inVector1, Vector_3_In inVector2)
	{
		return ( inVector1.x != inVector2.x &&
				inVector1.y != inVector2.y &&
				inVector1.z != inVector2.z ? 1u : 0u );
	}
#endif // V3ISEQUALNONE

#ifndef V3ISEQUALV
	__forceinline Vector_3_Out V3IsEqualV(Vector_3_In inVector1, Vector_3_In inVector2)
	{
		Vector_3 outVect;
		outVect.x = (inVector1.x == inVector2.x ? allBitsF : 0.0f);
		outVect.y = (inVector1.y == inVector2.y ? allBitsF : 0.0f);
		outVect.z = (inVector1.z == inVector2.z ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V3ISEQUALV

#ifndef V3ISEQUALINTALL
	__forceinline unsigned int V3IsEqualIntAll(Vector_3_In inVector1, Vector_3_In inVector2)
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
#endif // V3ISEQUALINTALL

#ifndef V3ISEQUALINTNONE
	__forceinline unsigned int V3IsEqualIntNone(Vector_3_In inVector1, Vector_3_In inVector2)
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
#endif // V3ISEQUALINTNONE

#ifndef V3ISEQUALINTV
	__forceinline Vector_3_Out V3IsEqualIntV(Vector_3_In inVector1, Vector_3_In inVector2)
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

		Vector_3 outVect;
		outVect.x = (Temp1.i == Temp2.i ? allBitsF : 0.0f);
		outVect.y = (Temp3.i == Temp4.i ? allBitsF : 0.0f);
		outVect.z = (Temp5.i == Temp6.i ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V3ISEQUALINTV

#ifndef V3ISCLOSEV
	__forceinline Vector_3_Out V3IsCloseV(Vector_3_In inVector1, Vector_3_In inVector2, float eps)
	{
		Vector_3 outVect;
		outVect.x = (( inVector1.x >= inVector2.x - eps && inVector1.x <= inVector2.x + eps ) ? allBitsF : 0.0f);
		outVect.y = (( inVector1.y >= inVector2.y - eps && inVector1.y <= inVector2.y + eps ) ? allBitsF : 0.0f);
		outVect.z = (( inVector1.z >= inVector2.z - eps && inVector1.z <= inVector2.z + eps ) ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V3ISCLOSEV

#ifndef V3ISCLOSEV_V
	__forceinline Vector_3_Out V3IsCloseV(Vector_3_In inVector1, Vector_3_In inVector2, Vector_3_In eps)
	{
		Vector_3 outVect;
		outVect.x = (( inVector1.x >= inVector2.x - eps.x && inVector1.x <= inVector2.x + eps.x ) ? allBitsF : 0.0f);
		outVect.y = (( inVector1.y >= inVector2.y - eps.y && inVector1.y <= inVector2.y + eps.y ) ? allBitsF : 0.0f);
		outVect.z = (( inVector1.z >= inVector2.z - eps.z && inVector1.z <= inVector2.z + eps.z ) ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V3ISCLOSEV_V

#ifndef V3ISCLOSEALL
	__forceinline unsigned int V3IsCloseAll(Vector_3_In inVector1, Vector_3_In inVector2, float eps)
	{
		return ( ( inVector1.x >= inVector2.x - eps && inVector1.x <= inVector2.x + eps ) &&
				( inVector1.y >= inVector2.y - eps && inVector1.y <= inVector2.y + eps ) &&
				( inVector1.z >= inVector2.z - eps && inVector1.z <= inVector2.z + eps ) ? 1u : 0u);
	}
#endif // V3ISCLOSEALL

#ifndef V3ISCLOSENONE
	__forceinline unsigned int V3IsCloseNone(Vector_3_In inVector1, Vector_3_In inVector2, float eps)
	{
		return (( inVector1.x < inVector2.x - eps || inVector1.x > inVector2.x + eps ) &&
				( inVector1.y < inVector2.y - eps || inVector1.y > inVector2.y + eps ) &&
				( inVector1.z < inVector2.z - eps || inVector1.z > inVector2.z + eps ) ? 1u : 0u);
	}
#endif // V3ISCLOSENONE

#ifndef V3ISCLOSEALL_V
	__forceinline unsigned int V3IsCloseAll(Vector_3_In inVector1, Vector_3_In inVector2, Vector_3_In eps)
	{
		return (( inVector1.x >= inVector2.x - eps.x && inVector1.x <= inVector2.x + eps.x ) &&
				( inVector1.y >= inVector2.y - eps.y && inVector1.y <= inVector2.y + eps.y ) &&
				( inVector1.z >= inVector2.z - eps.z && inVector1.z <= inVector2.z + eps.z ) ? 1u : 0u);
	}
#endif // V3ISCLOSEALL_V

#ifndef V3ISCLOSENONE_V
	__forceinline unsigned int V3IsCloseNone(Vector_3_In inVector1, Vector_3_In inVector2, Vector_3_In eps)
	{
		return (( inVector1.x < inVector2.x - eps.x || inVector1.x > inVector2.x + eps.x ) &&
				( inVector1.y < inVector2.y - eps.y || inVector1.y > inVector2.y + eps.y ) &&
				( inVector1.z < inVector2.z - eps.z || inVector1.z > inVector2.z + eps.z ) ? 1u : 0u);
	}
#endif // V3ISCLOSENONE_V

#ifndef V3ISGREATERTHANALL
	__forceinline unsigned int V3IsGreaterThanAll(Vector_3_In bigVector, Vector_3_In smallVector)
	{
		return (( bigVector.x > smallVector.x ) &&
				( bigVector.y > smallVector.y ) &&
				( bigVector.z > smallVector.z ) ? 1u : 0u );
	}
#endif // V3ISGREATERTHANALL

#ifndef V3ISGREATERTHANV
	__forceinline Vector_3_Out V3IsGreaterThanV(Vector_3_In bigVector, Vector_3_In smallVector)
	{
		Vector_3 outVect;
		outVect.x = (bigVector.x > smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y > smallVector.y ? allBitsF : 0.0f);
		outVect.z = (bigVector.z > smallVector.z ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V3ISGREATERTHANV

#ifndef V3ISGREATERTHANOREQUALALL
	__forceinline unsigned int V3IsGreaterThanOrEqualAll(Vector_3_In bigVector, Vector_3_In smallVector)
	{
		return (( bigVector.x >= smallVector.x ) &&
				( bigVector.y >= smallVector.y ) &&
				( bigVector.z >= smallVector.z ) ? 1u : 0u);
	}
#endif // V3ISGREATERTHANOREQUALALL

#ifndef V3ISGREATERTHANOREQUALV
	__forceinline Vector_3_Out V3IsGreaterThanOrEqualV(Vector_3_In bigVector, Vector_3_In smallVector)
	{
		Vector_3 outVect;
		outVect.x = (bigVector.x >= smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y >= smallVector.y ? allBitsF : 0.0f);
		outVect.z = (bigVector.z >= smallVector.z ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V3ISGREATERTHANOREQUALV

#ifndef V3ISLESSTHANALL
	__forceinline unsigned int V3IsLessThanAll(Vector_3_In smallVector, Vector_3_In bigVector)
	{
		return (( bigVector.x > smallVector.x ) &&
				( bigVector.y > smallVector.y ) &&
				( bigVector.z > smallVector.z ) ? 1u : 0u);
	}
#endif // V3ISLESSTHANALL

#ifndef V3ISLESSTHANV
	__forceinline Vector_3_Out V3IsLessThanV(Vector_3_In smallVector, Vector_3_In bigVector)
	{
		Vector_3 outVect;
		outVect.x = (bigVector.x > smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y > smallVector.y ? allBitsF : 0.0f);
		outVect.z = (bigVector.z > smallVector.z ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V3ISLESSTHANV

#ifndef V3ISLESSTHANOREQUALALL
	__forceinline unsigned int V3IsLessThanOrEqualAll(Vector_3_In smallVector, Vector_3_In bigVector)
	{
		return (( bigVector.x >= smallVector.x ) &&
				( bigVector.y >= smallVector.y ) &&
				( bigVector.z >= smallVector.z ) ? 1u : 0u);
	}
#endif // V3ISLESSTHANOREQUALALL

#ifndef V3ISLESSTHANOREQUALV
	__forceinline Vector_3_Out V3IsLessThanOrEqualV(Vector_3_In smallVector, Vector_3_In bigVector)
	{
		Vector_3 outVect;
		outVect.x = (bigVector.x >= smallVector.x ? allBitsF : 0.0f);
		outVect.y = (bigVector.y >= smallVector.y ? allBitsF : 0.0f);
		outVect.z = (bigVector.z >= smallVector.z ? allBitsF : 0.0f);
		return outVect;
	}
#endif // V3ISLESSTHANOREQUALV

#ifndef V3SELECT
	__forceinline Vector_3_Out V3SelectFT(Vector_3_In choiceVector, Vector_3_In zero, Vector_3_In nonZero)
	{
		Vector_3 outVect;

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
#endif // V3SELECT

#ifndef V3SELECTVECT
	__forceinline Vector_3_Out V3SelectVect(Vector_3_In choiceVectorX, Vector_3_In zero, Vector_3_In nonZero)
	{
		return (GetX(choiceVectorX) == 0.0f ? zero : nonZero );
	}
#endif // V3SELECTVECT

#ifndef V3MAX
	__forceinline Vector_3_Out V3Max(Vector_3_In inVector1, Vector_3_In inVector2)
	{
		Vector_3 outVect;
		outVect.x = FPMax(inVector1.x, inVector2.x);
		outVect.y = FPMax(inVector1.y, inVector2.y);
		outVect.z = FPMax(inVector1.z, inVector2.z);
		return outVect;
	}
#endif // V3MAX()

#ifndef V3MIN
	__forceinline Vector_3_Out V3Min(Vector_3_In inVector1, Vector_3_In inVector2)
	{
		Vector_3 outVect;
		outVect.x = FPMin(inVector1.x, inVector2.x);
		outVect.y = FPMin(inVector1.y, inVector2.y);
		outVect.z = FPMin(inVector1.z, inVector2.z);
		return outVect;
	}
#endif // V3MIN

#ifndef V3CLAMP
	__forceinline Vector_3_Out V3Clamp( Vector_3_In inVector, Vector_3_In lowBound, Vector_3_In highBound )
	{
		Vector_3 outVect;
		outVect.x = FPClamp(inVector.x, lowBound.x, highBound.x);
		outVect.y = FPClamp(inVector.y, lowBound.y, highBound.y);
		outVect.z = FPClamp(inVector.z, lowBound.z, highBound.z);
		return outVect;
	}
#endif // V3CLAMP

#ifndef V3SATURATE
	__forceinline Vector_3_Out V3Saturate( Vector_3_In inVector )
	{
		Vector_3 outVect;
		outVect.x = FPClamp(inVector.x, 0.0f, 1.0f);
		outVect.y = FPClamp(inVector.y, 0.0f, 1.0f);
		outVect.z = FPClamp(inVector.z, 0.0f, 1.0f);
		return outVect;
	}
#endif // V3SATURATE

	//============================================================================
	// Quaternions

#ifndef V3QUATROTATE
	__forceinline Vector_3_Out V3QuatRotate( Vector_3_In inVect, Vector_4_In inQuat )
	{
		Vector_4 _xyz0 = VEC4_LITERAL( inVect.x, inVect.y, inVect.z, 0.0f );

		Vector_4 quatConjugate = V4QuatConjugate( inQuat );

		// Out = Q * V * Q^-1
		// TODO: (why is my formula reverse of XMMISC.INL's? That is, why are they doing the inverse transform?)
		Vector_4 resultVec_xyz = V4QuatMultiply( inQuat, _xyz0 );
		resultVec_xyz = V4QuatMultiply( resultVec_xyz, quatConjugate );

		Vector_3 retVal = VEC3_LITERAL( resultVec_xyz.x, resultVec_xyz.y, resultVec_xyz.z );
		return retVal;
	}
#endif // V3QUATROTATE

#ifndef V3QUATROTATEREVERSE
	__forceinline Vector_3_Out V3QuatRotateReverse( Vector_3_In inVect, Vector_4_In inQuat )
	{
		Vector_4 _xyz0 = VEC4_LITERAL( inVect.x, inVect.y, inVect.z, 0.0f );

		Vector_4 quatConjugate = V4QuatConjugate( inQuat );

		// Out = Q^-1 * V * Q
		Vector_4 resultVec_xyz = V4QuatMultiply( _xyz0, inQuat );
		resultVec_xyz = V4QuatMultiply( quatConjugate, resultVec_xyz );
		
		Vector_3 retVal = VEC3_LITERAL( resultVec_xyz.x, resultVec_xyz.y, resultVec_xyz.z );
		return retVal;
	}
#endif // V3QUATROTATEREVERSE

	//============================================================================
	// Bitwise operations

#ifndef V3SHIFTLEFT
	template <int shift>
	__forceinline Vector_3_Out V3ShiftLeft( Vector_3_In inVector )
	{
		Vector_3 outVect;

		union
		{
			float f;
			u32 i;
		} TempResult1, TempResult2, TempResult3;

		TempResult1.f = inVector.x;
		TempResult2.f = inVector.y;
		TempResult3.f = inVector.z;
		TempResult1.i <<= shift;
		TempResult2.i <<= shift;
		TempResult3.i <<= shift;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		return outVect;
	}
#endif // V3SHIFTLEFT

#ifndef V3SHIFTRIGHT
	template <int shift>
	__forceinline Vector_3_Out V3ShiftRight( Vector_3_In inVector )
	{
		Vector_3 outVect;

		union
		{
			float f;
			u32 i;
		} TempResult1, TempResult2, TempResult3;

		TempResult1.f = inVector.x;
		TempResult2.f = inVector.y;
		TempResult3.f = inVector.z;
		TempResult1.i >>= shift;
		TempResult2.i >>= shift;
		TempResult3.i >>= shift;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		return outVect;
	}
#endif // V3SHIFTRIGHT

#ifndef V3SHIFTRIGHTALGEBRAIC
	template <int shift>
	__forceinline Vector_3_Out V3ShiftRightAlgebraic( Vector_3_In inVector )
	{
		Vector_3 outVect;

		union
		{
			float f;
			s32 i;
		} TempResult1, TempResult2, TempResult3;

		TempResult1.f = inVector.x;
		TempResult2.f = inVector.y;
		TempResult3.f = inVector.z;
		TempResult1.i >>= shift;
		TempResult2.i >>= shift;
		TempResult3.i >>= shift;

		outVect.x = TempResult1.f;
		outVect.y = TempResult2.f;
		outVect.z = TempResult3.f;
		return outVect;
	}
#endif // V3SHIFTRIGHTALGEBRAIC

#ifndef V3AND
	__forceinline Vector_3_Out V3And(Vector_3_In inVector1, Vector_3_In inVector2)
	{
		Vector_3 outVect;

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
#endif // V3AND

#ifndef V3OR
	__forceinline Vector_3_Out V3Or(Vector_3_In inVector1, Vector_3_In inVector2)
	{
		Vector_3 outVect;

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
#endif // V3OR

#ifndef V3XOR
	__forceinline Vector_3_Out V3Xor(Vector_3_In inVector1, Vector_3_In inVector2)
	{
		Vector_3 outVect;

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
#endif // V3XOR

#ifndef V3ANDC
	__forceinline Vector_3_Out V3Andc(Vector_3_In inVector1, Vector_3_In inVector2)
	{
		Vector_3 outVect;

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
#endif // V3ANDC


} // namespace Vec
} // namespace rage

