//
// math/angmath.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef MATH_ANGMATH_H
#define MATH_ANGMATH_H

#include "amath.h"

#define		FAST_ANGMATH		1

namespace rage {

/* PURPOSE
	angmath provides math functions using angles, sines and cosines.

	<FLAG Component>
*/

// PURPOSE:	Clamp the given angle between the given minimum and maximum values.
// PARAMS:
//	angle -	the angle in radians
//	min -	the minimum angle in radians
//	max -	the maximum angle in radians
// RETURN:	the angle between negative PI and PI that is clamped between the given minimum and maximum
// NOTES:	This clamps the given angle between the minimum and maximum around a circle, so the minimum
//			can be greater than the maximum, and none of them need to be between -PI and PI.
float AngleClamp (float angle, float min, float max);

// PURPOSE:	Clamp the given angle between the given minimum and maximum values.
// PARAMS:
//	angle -	the angle in radians
//	min -	the minimum angle in radians
//	max -	the maximum angle in radians
//  prev -  the angle from the previous frame
//  histAdj - the angle the result is biased by when choosing a limit
// RETURN:	the angle between negative PI and PI that is clamped between the given minimum and maximum
// NOTES:	This clamps the given angle between the minimum and maximum around a circle, so the minimum
//			can be greater than the maximum, and none of them need to be between -PI and PI.
float AngleClamp (float angle, float min, float max, float prev, float histAdj);

//PURPOSE:	Find the the equivalent angle between negative PI and PI of an angle in radians
//			(the canonical form).
//PARAMS:
//	angle -	the angle in radians
//RETURN:	the equivalent angle between negative PI and PI
__forceinline float CanonicalizeAngle (float angle)
{
#if FAST_ANGMATH
	float shift = Selectf( angle, PI, -PI );
#else
	float shift = PI;
	if ( angle < 0.0f )
	{
		shift = -PI;
	}
#endif

	angle += shift;

	angle = fmodf(angle, 2.0f * PI);

	angle -= shift;

#if FAST_ANGMATH
	return Selectf( -(angle+PI), PI, angle );
#else
	if ( angle == -PI )
	{
		return PI;
	}
	else
	{
		return angle;
	}
#endif
}


//PURPOSE:	Find the the equivalent angle between negative PI and PI of an angle in radians
//			(the canonical form).
//PARAMS:
//	angle -	the angle in radians
//RETURN:	the equivalent angle between negative PI and PI
//NOTES:
//	This is for regularly canonicalized angles; it assumes the angle is between -2*PI and +2*PI.
__forceinline float CanonicalizeAngleFast (float angle)
{
	//if (angle>PI) return angle-2.0f*PI;
	angle = Selectf(-(angle - PI), angle, angle - 2.f * PI);

	// if (angle<-PI) return angle+2.0f*PI;
	angle = Selectf(angle + PI, angle, angle + 2.f * PI);

	mthAssertf(angle >= -PI && angle <= PI,"CanonicalizeAngleFast failed, it got angle %f, call the non-fast version when the input is not between -2PI and 2PI",angle);

	return angle;
}


// PURPOSE:	Compute the difference between two angles, returning a value between -PI and PI for
//			the shorter of the two arcs between the two angles.
// PARAMS:
//	angle1 -	the first angle in radians
//	angle2 -	the second angle in radians
// RETURN:	the angular length of the shorter arc between the two input angles
__forceinline float SubtractAngleShorter (float angle1, float angle2)
{
	return CanonicalizeAngle(angle1 - angle2);
}

// PURPOSE:	Compute the difference between two angles, returning a value between -PI and PI for
//			the shorter of the two arcs between the two angles. The original two angles must be canonicalized first
// PARAMS:
//	angle1 -	the first angle in radians
//	angle2 -	the second angle in radians
// RETURN:	the angular length of the shorter arc between the two input angles
__forceinline float SubtractAngleShorterFast (float angle1, float angle2)
{
	return CanonicalizeAngleFast(angle1 - angle2);
}

// PURPOSE:	Compute the difference between two angles, returning a value between -PI and PI for
//			the longer of the two arcs between the two angles.
// PARAMS:
//	angle1 -	the first angle in radians
//	angle2 -	the second angle in radians
// RETURN:	the angular length of the longer arc between the two input angles
__forceinline float SubtractAngleLonger (float angle1, float angle2)
{
	float shortDist = CanonicalizeAngle(angle1 - angle2);
#if FAST_ANGMATH
	float temp = (2.0f * PI) - shortDist;
	return Selectf( -shortDist, -temp, temp );
#else
	return ( shortDist > 0.0f ? (2.0f * PI) - shortDist : shortDist - (2.0f * PI) );
#endif
}


// PURPOSE:	Compute the difference between two angles, returning a value between between 0 and 2 * PI
//			representing the arc between the two in the positive direction.
// PARAMS:
//	angle1 -	the first angle in radians
//	angle2 -	the second angle in radians
// RETURN:	the angular length of the positive arc between the two input angles
__forceinline float SubtractAnglePositive (float angle1, float angle2)
{
	float result = CanonicalizeAngle(angle2 - angle1);

#if FAST_ANGMATH
	return Selectf( result, result, (result + 2.0f * PI) );
#else
	if( result >= 0.0f )
	{
		return result;
	}
	else
	{
		return result + 2.0f * PI;
	}
#endif

}


// PURPOSE:	Compute the difference between two angles, returning a value between between 0 and -2 * PI
//			representing the arc between the two in the negative direction.
// PARAMS:
//	angle1 -	the first angle in radians
//	angle2 -	the second angle in radians
// RETURN:	the angular length of the negative arc between the two input angles
__forceinline float SubtractAngleNegative (float angle1, float angle2)
{
	float result = CanonicalizeAngle(angle2 - angle1);

#if FAST_ANGMATH
	return Selectf( result, result - 2.0f * PI, result );
#else
	if( result < 0.0f )
	{
		return result;
	}
	else
	{
		return result - 2.0f * PI;
	}
#endif
}


// PURPOSE:	Linearly interpolate between the two given angles, handling the crossover between PI and -PI.
// PARAMS:
//	t -			the fraction of the difference to interpolate between the two angles
//	angle1 -	the first angle in radians
//	angle2 -	the second angle in radians
// RETURN:	the linearly interpolated angle between the two input angles
// NOTES:	When the parameter t is 0, the result is angle1, and when t is 1, the result is angle2.
__forceinline float InterpolateAngle (float t, float angle1, float angle2)
{
	return CanonicalizeAngle(angle1 + t * SubtractAngleShorter(angle2, angle1));
}


// PURPOSE:	Find which of two given angles is closer to a third given angle.
// PARAMS:
//	baseAngle -	the angle to compare with the other two
//	angle1 -	the first comparison angle in radians
//	angle2 -	the second comparison angle in radians
// RETURN:	true if baseAngle is closer to angle1, false if baseAngle is closer to angle2
__forceinline bool IsCloserToAngle (float baseAngle, float angle1, float angle2)
{
	return fabsf(SubtractAngleShorter(baseAngle, angle1)) < fabsf(SubtractAngleShorter(baseAngle, angle2));
}


// PURPOSE:	Find which of two given angles is closer to a third given angle.
// PARAMS:
//	baseAngle -	the angle to compare with the other two
//	angle1 -	the first comparison angle in radians
//	angle2 -	the second comparison angle in radians
//  prev -  the angle from the previous frame
//  histAdj - the bias towards the limit the previous angle was limited to
// RETURN:	true if baseAngle is closer to angle1, false if baseAngle is closer to angle2
bool IsCloserToAngle (float baseAngle, float angle1, float angle2, float prev, float histAdj);


} // namespace rage

#endif // MATH_ANGMATH_H
