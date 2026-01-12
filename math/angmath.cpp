//
// math/angmath.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "angmath.h"
#include "constants.h"

namespace rage {

float AngleClamp( float angle, float min, float max )
{
	angle = CanonicalizeAngle( angle );

	if (fabsf(max - min) > 2.0f * PI - SMALL_FLOAT)
	{
		return angle;
	}

	min = CanonicalizeAngle( min );
	max = CanonicalizeAngle( max );
	
	if( max < min )
	{
		if( angle >= min && angle <= max + (2.0f * PI) )
		{
			return angle;
		}
	}
	else
	{
		if( angle >= min && angle <= max )
		{
			return angle;
		}
	}
	return IsCloserToAngle( angle, min, max ) ? min : max;
}

float AngleClamp( float angle, float min, float max, float prev, float histAdj )
{
	angle = CanonicalizeAngle( angle );

	if (fabsf(max - min) > 2.0f * PI - SMALL_FLOAT)
	{
		return angle;
	}

	min = CanonicalizeAngle( min );
	max = CanonicalizeAngle( max );
	prev = CanonicalizeAngle( prev );

	if( max < min )
	{
		if( angle >= min && angle <= max + (2.0f * PI) )
		{
			return angle;
		}
	}
	else
	{
		if( angle >= min && angle <= max )
		{
			return angle;
		}
	}
	return IsCloserToAngle( angle, min, max, prev, histAdj ) ? min : max;
}

bool IsCloserToAngle (float baseAngle, float angle1, float angle2, float prev, float histAdj)
{
	// If the previous angle was within the limits use the unbiased behavior
	if( angle2 < angle1 )
	{
		if( prev > angle1 + SMALL_FLOAT && prev < angle2 + (2.0f * PI) - SMALL_FLOAT )
		{
			return IsCloserToAngle(baseAngle, angle1, angle2);
		}
	}
	else
	{
		if( prev > angle1 + SMALL_FLOAT && prev < angle2 - SMALL_FLOAT )
		{
			return IsCloserToAngle(baseAngle, angle1, angle2);
		}
	}

	// Otherwise determine which limit the previous angle hit and bias in its favor
	if (IsCloserToAngle(prev, angle1, angle2))
	{
		return fabsf(SubtractAngleShorter(baseAngle, angle1)) < fabsf(SubtractAngleShorter(baseAngle, angle2)) + histAdj;
	}
	else
	{
		return fabsf(SubtractAngleShorter(baseAngle, angle1)) + histAdj < fabsf(SubtractAngleShorter(baseAngle, angle2));
	}
}

} //namespace rage 
