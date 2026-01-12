//
// math/amath.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "amath.h"

#include <float.h>
#include <string.h>

namespace rage {

#if !__FINAL
// NOTE: Simple prime number functions that are meant to ONLY be used in !__FINAL
// Why? Because calculating prime numbers at runtime is slow and stupid.  Don't ever do this in final.
// Remember that the following functions are designed to be GOD-AWFUL SLOW!!!
bool IsPrime(size_t value)
{
	for (size_t i = 3; true; i += 2)
	{
		const size_t part = value / i;		
		if (part < i)
			return true;

		if (value == (part * i))
			return false;
	}
}

size_t GetNextPrime(size_t value)
{
	if (value <= 2)
		return 2;

	if (!(value & 1))
		++value;

	while (!IsPrime(value))
		value += 2;

	return value;
}
#endif

#if !__XENON && !__PPU

// fast inverse sqrtf()  (1.0f/sqrtf())
//  NOTE: 1.0f/sqrtf(0.0f) will return 0.0f, instead of NaN, since it is more useful
//
// Written by Ken Turkowski. From Graphics Gems V

/* Specified parameters */
#define LOOKUP_BITS    7   /* Number of mantissa bits for lookup */
#define EXP_POS       23   /* Position of the exponent */
#define EXP_BIAS     127   /* Bias of exponent */

/* Derived parameters */
#define LOOKUP_POS   (EXP_POS-LOOKUP_BITS)  // Position of mantissa lookup 
#define SEED_POS     (EXP_POS-8)            // Position of mantissa seed 
#define TABLE_SIZE   (2 << LOOKUP_BITS)     // Number of entries in table
#define LOOKUP_MASK  (TABLE_SIZE - 1)           // Mask for table input
#define GET_EXP(a)   (((a) >> EXP_POS) & 0xFF)  // Extract exponent
#define SET_EXP(a)   ((a) << EXP_POS)           // Set exponent
#define GET_EMANT(a) (((a) >> LOOKUP_POS) & LOOKUP_MASK)   // Extended mantissa MSB's 
#define SET_MANTSEED(a) (((unsigned int)(a)) << SEED_POS) // Set mantissa 8 MSB's 

static unsigned char s_InvSqrtSeed[256] = {
	0x6a,0x68,0x67,0x66,0x64,0x63,0x62,0x60,0x5f,0x5e,0x5c,0x5b,0x5a,0x59,0x57,0x56,
	0x55,0x54,0x53,0x52,0x50,0x4f,0x4e,0x4d,0x4c,0x4b,0x4a,0x49,0x48,0x47,0x46,0x45,
	0x44,0x43,0x42,0x41,0x40,0x3f,0x3e,0x3d,0x3c,0x3b,0x3a,0x39,0x38,0x37,0x36,0x35,
	0x34,0x34,0x33,0x32,0x31,0x30,0x2f,0x2f,0x2e,0x2d,0x2c,0x2b,0x2a,0x2a,0x29,0x28,
	0x27,0x27,0x26,0x25,0x24,0x24,0x23,0x22,0x21,0x21,0x20,0x1f,0x1f,0x1e,0x1d,0x1c,
	0x1c,0x1b,0x1a,0x1a,0x19,0x18,0x18,0x17,0x16,0x16,0x15,0x15,0x14,0x13,0x13,0x12,
	0x11,0x11,0x10,0x10,0x0f,0x0e,0x0e,0x0d,0x0d,0x0c,0x0c,0x0b,0x0a,0x0a,0x09,0x09,
	0x08,0x08,0x07,0x07,0x06,0x05,0x05,0x04,0x04,0x03,0x03,0x02,0x02,0x01,0x01,0x00,
	0xff,0xfe,0xfc,0xfa,0xf8,0xf6,0xf4,0xf2,0xf0,0xef,0xed,0xeb,0xe9,0xe8,0xe6,0xe4,
	0xe2,0xe1,0xdf,0xde,0xdc,0xda,0xd9,0xd7,0xd6,0xd4,0xd3,0xd1,0xd0,0xce,0xcd,0xcb,
	0xca,0xc8,0xc7,0xc5,0xc4,0xc3,0xc1,0xc0,0xbf,0xbd,0xbc,0xbb,0xb9,0xb8,0xb7,0xb6,
	0xb4,0xb3,0xb2,0xb1,0xb0,0xae,0xad,0xac,0xab,0xaa,0xa8,0xa7,0xa6,0xa5,0xa4,0xa3,
	0xa2,0xa1,0xa0,0x9f,0x9e,0x9c,0x9b,0x9a,0x99,0x98,0x97,0x96,0x95,0x94,0x93,0x92,
	0x91,0x90,0x8f,0x8f,0x8e,0x8d,0x8c,0x8b,0x8a,0x89,0x88,0x87,0x86,0x85,0x85,0x84,
	0x83,0x82,0x81,0x80,0x7f,0x7f,0x7e,0x7d,0x7c,0x7b,0x7a,0x7a,0x79,0x78,0x77,0x76,
	0x76,0x75,0x74,0x73,0x73,0x72,0x71,0x70,0x70,0x6f,0x6e,0x6d,0x6d,0x6c,0x6b,0x6a,
};


// The following returns the inverse square root 
float invsqrtf_fast (float x)//lint !e765
{
	union 
	{
		float			f;
		unsigned int	i;
	} alias;

	mthAssertf(x >= 0.0f, "Can't take sqrt of a negative number (%f)", x);
	alias.f = x;
	 
	float halfx = x * 0.5f;
	alias.i = (SET_EXP(((3*EXP_BIAS-1) - GET_EXP(alias.i)) >> 1) |
		                   SET_MANTSEED(s_InvSqrtSeed[GET_EMANT(alias.i)]));

	// 0.0 check during memory write is "free" 
	if (halfx==0.0f) alias.i = 0x00000000;
	
	alias.f = (1.5f - alias.f * alias.f * halfx) * alias.f;    // accurate to 2*LOOKUP_BITS
	return (1.5f - alias.f * alias.f * halfx) * alias.f; // accurate to 4*LOOKUP_BITS
}
#endif // !__XENON && !__PPU

#if __WIN32
int MathErr(_exception *except) {
	Printf("\n\n");

	// Prevent any NANs or infinities and set the return value to 0 in those cases
	if(!FPIsFinite(float(except->retval))) except->retval=0;

	switch(except->type) { 
		case DOMAIN:
			mthErrorf("matherr()- Domain error in %s (%f,%f)",except->name,except->arg1,except->arg2);
			if(strcmp(except->name,"asin")==0 || strcmp(except->name,"asinf")==0) {		//lint !e1055 !e526 valid strcmp
				if(except->arg1>=1.0f) except->retval=0.5f*PI;
				else if(except->arg1<=-1.0f) except->retval=-0.5f*PI;
				else except->retval=0.0f;
			}
			if(strcmp(except->name,"acos")==0 || strcmp(except->name,"acosf")==0) {
				if(except->arg1>=1.0f) except->retval=0.0f;
				else if(except->arg1<=-1.0f) except->retval=PI;
				else except->retval=0.0f;
			}
			break;
		case SING:
			mthErrorf("matherr()- Singularity error in %s (%f,%f)",except->name,except->arg1,except->arg2);
			break;
		case OVERFLOW:
			mthErrorf("matherr()- Overflow error in %s (%f,%f)",except->name,except->arg1,except->arg2);
			break;
		case PLOSS:
			mthErrorf("matherr()- PLoss error in %s (%f,%f)",except->name,except->arg1,except->arg2);
			break;
		case TLOSS:
			mthErrorf("matherr()- TLoss error in %s (%f,%f)",except->name,except->arg1,except->arg2);
			break;
		case UNDERFLOW:
			mthErrorf("matherr()- Underflow error in %s (%f,%f)",except->name,except->arg1,except->arg2);
			break;
		default:
			mthErrorf("matherr()- type undefined");
			break;
	}

	mthErrorf("matherr()- returning %f\n\n",except->retval);
	return 1;		// No matter what, pretend everything is OK
}
#endif

#if __PPU
// Fast sin approximation for PSN - ( though valid for PPC in general )

//#include <bits/sincos_t.h>		// Coeffs from vmx mathlib - I'm too lazy to reproduce them..
#define _SINCOS_CC0  -0.0013602249f
#define _SINCOS_CC1   0.0416566950f
#define _SINCOS_CC2  -0.4999990225f
#define _SINCOS_SC0  -0.0001950727f
#define _SINCOS_SC1   0.0083320758f
#define _SINCOS_SC2  -0.1666665247f


float Sinf( float angle )
{
	double ang1;
	double ang2;
	double ang3;
	double s;
	double c;
	double cang1;
	double cang2;
	double k1,k2;

/* Angle in radians, first scale so 0-2PI is 0-1 range, and also remap to cosine curve ( pi/2- or 0.25- )
 */

	ang1 = 0.25 - (angle*(1/(2*M_PI)));
	cang1 = angle*(1/(2*M_PI));		// sin = cos(PI/2-a) - quicker to do everything 

/* Now range reduce fraction to -1 to +1 range */

	ang1 = ang1-(double)(long long)ang1;
	cang1 = cang1-(double)(long long)cang1;

/* Cosine is even, so ignore negative part... */

	ang1 = __fabs( ang1 );
	cang1 = __fabs( cang1 );

/* Full range curve is symetric, so bias by -PI(-0.5) and ignore sign */

	ang1 = __fabs( ang1-0.5 );
	cang1 = __fabs( cang1-0.5 );
	
/* Rebias into -PI/2 to +PI/2 range for approx.. */

	ang1 = (ang1*2*M_PI)-(M_PI/2);
	cang1 = (cang1*2*M_PI)-(M_PI/2);

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
	k1 = Selectf( ang1,1.0,-1.0 );
	k2 = Selectf( ang1,cang2,-cang2 );

	s = ang1 + s*ang3;
	c = k1 + c*k2;
	
	return Selectf( cang1,s,c );
}

void cos_and_sin( float &cos,float &sin,float angle )
{
	double ang1,ang2,ang3;
	double cang1,cang2,cang3;

	double s,s1;
	double c,c1;

	double k;
	double k1,k2;
	double k1a,k2a;


/* Angle in radians, first scale so 0-2PI is 0-1 range, and also remap to cosine curve ( pi/2- or 0.25- )
 */


	ang1 = 0.25 - (angle*(1/(2*M_PI))); // /(2*M_PI);
	cang1 = angle*(1/(2*M_PI));		// sin = cos(PI/2-a) - quicker to do everything 

/* Now range reduce fraction to -1 to +1 range */

	ang1 = ang1-(double)(long long)ang1;
	cang1 = cang1-(double)(long long)cang1;

/* Cosine is even, so ignore negative part... */

	ang1 = __fabs( ang1 );
	cang1 = __fabs( cang1 );

/* Full range curve is symetric, so bias by -PI(-0.5) and ignore sign */

	ang1 = __fabs( ang1-0.5 );
	cang1 = __fabs( cang1-0.5 );

/* Rebias into -PI/2 to +PI/2 range for approx.. */

	ang1 = (ang1*2*M_PI)-(M_PI/2);
	cang1 = (cang1*2*M_PI)-(M_PI/2);

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

	k1 = Selectf( ang1,1.0,-1.0 );
	k2 = Selectf( ang1,cang2,-cang2 );
	k1a = Selectf( cang1,1.0,-1.0 );
	k2a = Selectf( cang1,ang2,-ang2 );

	s = ang1 + s*ang3;
	c = k1 + c*k2;
	s1 = cang1 + s1*cang3;
	c1 = k1a + c1*k2a;

	cos = Selectf( k,c1,s1 );

	sin = Selectf( k,s,c );
}

float Atan2f( float yp,float xp )
{
		double angle;
		double y,y2,y3,y4;
		double x,x2,x3,x4;
		double v;
		double bias,sign;

/* Return 0 for Atan2f(0,0) */

		if ((0.0==yp)&&(0.0==xp)) return 0.0;

/* Use rational approx with y/x in - to save redundant divides..
 *
 *	Sign of Y/X shows quadrant,
 *	Max Y
 *
 */


/* Check quadrants... */

		bias = 0.0;
		sign = 1.0;

		if (xp<0)
		{
				sign = -1.0;
				bias = -(3.14159265358979323846264338327950287);
		}

		y = __fabs( yp );
		x = __fabs( xp );

		if (y>x)
		{
			bias = sign*-(1.57079632679489661923132169163975145);
			sign = - sign;
			
			v = y;
			y = x;
			x = v;
		}

		if (yp<0) sign = -sign;
	
/* Fold around to +/- pi/12 range using tan(a-pi/6) if >pi/12 */


		if (y>=(x*(0.26794919243112270647255365849412763)))
		{
				v = ((1.7320508075688772935274463415058725)*y)-x;
				x = y+(x*(1.7320508075688772935274463415058725));
				y = v;
				bias += (0.52359877559829887307710723054658382);
		}

		
		y2 = y*y;
		y3 = y*y2;
		y4 = y2*y2;
		x2 = x*x;
		x3 = x*x2;
		x4 = x2*x2;

		v = x4 + ((y2*x2)*(0.8483080898))+((0.0828021377)*(y4));
		angle = (y*x3)+((y3*x)*(0.5149751685));

		angle = (bias*v)+angle;
		v = v*sign;


		angle = angle/v;	

		return angle;
}





#endif

} //namespace rage 

using namespace rage;


