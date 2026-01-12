#include "mathops.h"


#ifndef M_PI
#define M_PI	(3.14159265358979323846264338327950288f)
#endif

#define _SINCOS_CC0  -0.0013602249f
#define _SINCOS_CC1   0.0416566950f
#define _SINCOS_CC2  -0.4999990225f
#define _SINCOS_SC0  -0.0001950727f
#define _SINCOS_SC1   0.0083320758f
#define _SINCOS_SC2  -0.1666665247f

namespace rage
{

#if __PPU
float __Z_E_R_O__;
#endif

#if (__XENON || __PPU) && USE_INTRINSICS
	float FPSin_Imp( float angle )
	{
		float ang1;
		float ang2;
		float ang3;
		float s;
		float c;
		float cang1;
		float cang2;
		float k1,k2;

		/* Angle in radians, first scale so 0-2PI is 0-1 range, and also remap to cosine curve ( pi/2- or 0.25- )
		*/

		ang1 = 0.25f - (angle*(1/(2*M_PI)));
		cang1 = angle*(1.0f/(2.0f*M_PI));		// sin = cos(PI/2-a) - quicker to do everything 

		/* Now range reduce fraction to -1 to +1 range */

		ang1 = ang1-(float)(long long)ang1;
		cang1 = cang1-(float)(long long)cang1;

		/* Cosine is even, so ignore negative part... */

		ang1 = __fabs( ang1 );
		cang1 = __fabs( cang1 );

		/* Full range curve is symetric, so bias by -PI(-0.5) and ignore sign */

		ang1 = __fabs( ang1-0.5f );
		cang1 = __fabs( cang1-0.5f );

		/* Rebias into -PI/2 to +PI/2 range for approx.. */

		ang1 = (ang1*2.0f*M_PI)-(M_PI/2);
		cang1 = (cang1*2.0f*M_PI)-(M_PI/2);

		/*
		* Run taylor series...
		*
		* s = x - x3/3! + x5/5! - x7/7!
		* c = 1 - x2/2! + x4/4! - x8/8!
		*
		*
		* Refactorise as:
		*
		* s = x + A x3 + B x5 + C x7
		*  = x + x3( A + x2 ( B + C x2 ) )
		* c = 1 + A x2 + B x4 + C x6
		*  = 1 + x2 (A + x2 ( B + C x2 ) )
		*
		*/

		ang2 = ang1*ang1;
		cang2 = cang1*cang1;

		s = _SINCOS_SC1 + _SINCOS_SC0*ang2;
		c = _SINCOS_CC1 + _SINCOS_CC0*cang2;
		ang3 = ang1*ang2;
		cang1 = cang2-ang2;

		s = _SINCOS_SC2 +  s*ang2;
		c = _SINCOS_CC2 + c*cang2;
		k1 = (float)__fsel( ang1,1.0f,-1.0f );
		k2 = (float)__fsel( ang1,cang2,-cang2 );

		s = ang1 + s*ang3;
		c = k1 + c*k2;

		return (float)__fsel( cang1,s,c );
	}
#endif // (__XENON || __PPU) && USE_INTRINSICS

#if (__XENON || __PPU) && USE_INTRINSICS
	void FPSinAndCos_Imp( float& sin, float& cos, float angle )
	{
		float ang1,ang2,ang3;
		float cang1,cang2,cang3;

		float s,s1;
		float c,c1;

		float k;
		float k1,k2;
		float k1a,k2a;


		/* Angle in radians, first scale so 0-2PI is 0-1 range, and also remap to cosine curve ( pi/2- or 0.25- )
		*/


		ang1 = 0.25f - (angle*(1.0f/(2.0f*M_PI))); // /(2*M_PI);
		cang1 = angle*(1.0f/(2.0f*M_PI));		// sin = cos(PI/2-a) - quicker to do everything 

		/* Now range reduce fraction to -1 to +1 range */

		ang1 = ang1-(float)(long long)ang1;
		cang1 = cang1-(float)(long long)cang1;

		/* Cosine is even, so ignore negative part... */

		ang1 = __fabs( ang1 );
		cang1 = __fabs( cang1 );

		/* Full range curve is symetric, so bias by -PI(-0.5) and ignore sign */

		ang1 = __fabs( ang1-0.5f );
		cang1 = __fabs( cang1-0.5f );

		/* Rebias into -PI/2 to +PI/2 range for approx.. */

		ang1 = (ang1*2.0f*M_PI)-(M_PI/2.0f);
		cang1 = (cang1*2.0f*M_PI)-(M_PI/2.0f);

		/*
		* Run taylor series...
		*
		* s = x - x3/3! + x5/5! - x7/7!
		* c = 1 - x2/2! + x4/4! - x6/6!
		*
		*
		* Refactorise as:
		*
		* s = x + A x3 + B x5 + C x7
		*  = x + x3( A + x2 ( B + C x2 ) )
		* c = 1 + A x2 + B x4 + C x6
		*  = 1 + x2 (A + x2 ( B + C x2 ) )
		*
		*/

		ang2 = ang1*ang1;
		cang2 = cang1*cang1;

		s = _SINCOS_SC1 + _SINCOS_SC0*ang2;
		c = _SINCOS_CC1 + _SINCOS_CC0*cang2;

		s1 = _SINCOS_SC1 + _SINCOS_SC0*cang2;
		c1 = _SINCOS_CC1 + _SINCOS_CC0*ang2;

		ang3 = ang1*ang2;
		cang3 = cang1*cang2;

		k = cang2-ang2;

		s = _SINCOS_SC2 +  s*ang2;
		c = _SINCOS_CC2 + c*cang2;
		s1 = _SINCOS_SC2 + s1*cang2;
		c1 = _SINCOS_CC2 + c1*ang2;

		k1 =(float) __fsel( ang1,1.0f,-1.0f );
		k2 = (float)__fsel( ang1,cang2,-cang2 );
		k1a = (float)__fsel( cang1,1.0f,-1.0f );
		k2a = (float)__fsel( cang1,ang2,-ang2 );

		s = ang1 + s*ang3;
		c = k1 + c*k2;
		s1 = cang1 + s1*cang3;
		c1 = k1a + c1*k2a;

		cos = (float)__fsel( k,c1,s1 );

		sin = (float)__fsel( k,s,c );
	}
#endif // (__XENON || __PPU) && USE_INTRINSICS

#if (__XENON || __PPU) && USE_INTRINSICS
	float FPATan2_Imp( float yp, float xp )
	{
		float angle;
		float y,y2,y3,y4;
		float x,x2,x3,x4;
		float v;
		float bias,sign;

		/* Return 0 for Atan2f(0,0) */

		if ((0.0f==yp)&&(0.0f==xp)) return 0.0f;

		/* Use rational approx with y/x in - to save redundant divides..
		*
		*	Sign of Y/X shows quadrant,
		*	Max Y
		*
		*/

		/* Check quadrants... */

		bias = 0.0f;
		sign = 1.0f;

		if (xp<0.0f)
		{
			sign = -1.0f;
			bias = -(M_PI);
		}

		y = __fabs( yp );
		x = __fabs( xp );

		if (y>x)
		{
			bias = sign*-(1.57079632679489661923132169163975145f);
			sign = -sign;

			v = y;
			y = x;
			x = v;
		}

		if (yp<0.0f) sign = -sign;

		/* Fold around to +/- pi/12 range using tan(a-pi/6) if >pi/12 */


		if (y>=(x*(0.26794919243112270647255365849412763f)))
		{
			v = ((1.7320508075688772935274463415058725f)*y)-x;
			x = y+(x*(1.7320508075688772935274463415058725f));
			y = v;
			bias += (0.52359877559829887307710723054658382f);
		}


		y2 = y*y;
		y3 = y*y2;
		y4 = y2*y2;
		x2 = x*x;
		x3 = x*x2;
		x4 = x2*x2;

		v = x4 + ((y2*x2)*(0.8483080898f))+((0.0828021377f)*(y4));
		angle = (y*x3)+((y3*x)*(0.5149751685f));

		angle = (bias*v)+angle;
		v = v*sign;


		angle = angle/v;	

		return angle;
	}
#endif // (__XENON || __PPU) && USE_INTRINSICS

}

#undef _SINCOS_CC0
#undef _SINCOS_CC1
#undef _SINCOS_CC2
#undef _SINCOS_SC0
#undef _SINCOS_SC1
#undef _SINCOS_SC2
