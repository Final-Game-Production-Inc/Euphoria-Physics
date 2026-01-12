//
// math/simplemath.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "simplemath.h"

#include "amath.h"
#include "constants.h"

namespace rage {

float CubeRoot (float number)
{
	if (number>0.0f)
	{
		return powf(number,0.333333333333f);
	}

	if (number<0.0f)
	{
		return -powf(-number,0.333333333333f);
	}

	return 0.0f;
}


float ArcTangent (float y, float x)
{
	if (x>VERY_SMALL_FLOAT)
	{
		if (fabsf(y)>VERY_SMALL_FLOAT)
		{
			// The cosine is positive and the sine is not nearly zero, so find the inverse tangent.
			return atanf(y/x);
		}

		// The cosine is positive and the sine is nearly zero, so the angle is zero.
		return 0.0f;
	}

	if (x<-VERY_SMALL_FLOAT)
	{
		if (y>VERY_SMALL_FLOAT)
		{
			// The cosine is negative and the sine is positive, so the inverse tangent in the range
			// of +-PI is PI minus the inverse of the negative tangent.
			return PI-atanf(-y/x);
		}

		if (y<-VERY_SMALL_FLOAT)
		{
			// The cosine is negative and the sine is negative, so the inverse tangent in the range
			// of +-PI is -PI plus the inverse of the tangent.
			return -PI+atanf(y/x);
		}

		// The cosine is negative and the sine is nearly zero, so the angle is PI (or -PI).
		return PI;
	}

	if (y>VERY_SMALL_FLOAT)
	{
		// The cosine is nearly zero and the sine is positive, so the angle is half PI.
		return 0.5f*PI;
	}

	if (y<-VERY_SMALL_FLOAT)
	{
		// The cosine is nearly zero and the sine is negative, so the angle is minus half PI.
		return -0.5f*PI;
	}

	// The cosine and sine are both nearly zero, so return 0.
	return 0.0f;
}


bool FindInverseBilinear (float x, float z, float ax, float az, float bx, float bz, float cx, float cz,
							 float dx, float dz, float &s, float &t)
{
	//This could increase the speed of this function depending on the platform; it doesn't for PS2.
	//if(!IntersectPointFlat(x,z,ax,az,bx,bz,cx,cz,dx,dz))
	//{
	//	return false;
	//}

	float A = ax - bx - cx + dx;
	float B = bx - ax;
	float C = cx - ax;
	float D = ax;
	float E = az - bz - cz + dz;
	float F = bz - az;
	float G = cz - az;
	float H = az;

	float I = (G * A) - (C * E);
	float J = (E * x) - (F * C) - (D * E) + (G * B) + (H * A) - (A * z);
	float K = (F * x) - (D * F) + (B * H) - (B * z);


	if ( fabsf(I) <= 0.0001f*fabsf(J) )
	{
		// The quad has a pair of nearly parallel sides
		if ( fabsf(J)>0.5f*fabsf(K) ) {
			t = -K/J;
		}
		else
		{
			return false;		// Here J<2K. We'd return false even if J<K.
		}
	}
	else
	{
		float L = (J * J) - (4.0f * I * K);
		if( L<=0.0f )
		{
			// L<0 is impossible, so it must equal zero within roundoff errors
			L = 0.0f;
		}
		else
		{
			L = sqrtf(L);	// Form the square root
		}

		if ( J<=0.0f )
		{					//Choose the more stable calculation.
			t = (-J + L) / (2.0f*I);		// We know I is nonzero from above
		}
		else
		{
			t = -2.0f*K / (J + L);		// 2nd way to calculate the same root (J - nonzero)
		}

	}

	if( t<0.0f || t>1.0f)
	{
		return false;
	}

	float denomX = (A * t) + B;
	float denomZ = (E * t) + F;
	if ( denomX>denomZ )
	{
		s = (x - (C * t) - D) / denomX;
	}
	else
	{
		s = (z - (G * t) - H) / denomZ;
	}

	if ( s<0.0f || s>1.0f )
	{
		return false;
	}

	return true;
}

} // namespace rage

// EOF vector/simplemath.cpp
