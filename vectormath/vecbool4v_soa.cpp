
#include "vecbool4v_soa.h"

#if !__NO_OUTPUT

namespace rage
{
	void SoA_VecBool4V::Print(bool newline)
	{
		union Temp
		{
			float f;
			unsigned int i;
		} x0,x1,x2,x3,y0,y1,y2,y3,z0,z1,z2,z3,w0,w1,w2,w3;
		x0.f = Vec::GetX( x );
		x1.f = Vec::GetY( x );
		x2.f = Vec::GetZ( x );
		x3.f = Vec::GetW( x );
		y0.f = Vec::GetX( y );
		y1.f = Vec::GetY( y );
		y2.f = Vec::GetZ( y );
		y3.f = Vec::GetW( y );
		z0.f = Vec::GetX( z );
		z1.f = Vec::GetY( z );
		z2.f = Vec::GetZ( z );
		z3.f = Vec::GetW( z );
		w0.f = Vec::GetX( w );
		w1.f = Vec::GetY( w );
		w2.f = Vec::GetZ( w );
		w3.f = Vec::GetW( w );

		VecAssertMsg(	(x0.i==0xFFFFFFFF || x0.i==0) &&
				(x1.i==0xFFFFFFFF || x1.i==0) &&
				(x2.i==0xFFFFFFFF || x2.i==0) &&
				(x3.i==0xFFFFFFFF || x3.i==0) &&
				(y0.i==0xFFFFFFFF || y0.i==0) &&
				(y1.i==0xFFFFFFFF || y1.i==0) &&
				(y2.i==0xFFFFFFFF || y2.i==0) &&
				(y3.i==0xFFFFFFFF || y3.i==0) &&
				(z0.i==0xFFFFFFFF || z0.i==0) &&
				(z1.i==0xFFFFFFFF || z1.i==0) &&
				(z2.i==0xFFFFFFFF || z2.i==0) &&
				(z3.i==0xFFFFFFFF || z3.i==0) &&
				(w0.i==0xFFFFFFFF || w0.i==0) &&
				(w1.i==0xFFFFFFFF || w1.i==0) &&
				(w2.i==0xFFFFFFFF || w2.i==0) &&
				(w3.i==0xFFFFFFFF || w3.i==0) ,
				"Warning: SoA_VecBool4V does not have all 0 or 0xFFFFFFFF words."
				);

		if( newline )
			Printf( "<[%s,%s,%s,%s],[%s,%s,%s,%s],[%s,%s,%s,%s],[%s,%s,%s,%s]>\n",
				(x0.i==0xFFFFFFFF?"T":"F"),
				(x1.i==0xFFFFFFFF?"T":"F"),
				(x2.i==0xFFFFFFFF?"T":"F"),
				(x3.i==0xFFFFFFFF?"T":"F"),
				(y0.i==0xFFFFFFFF?"T":"F"),
				(y1.i==0xFFFFFFFF?"T":"F"),
				(y2.i==0xFFFFFFFF?"T":"F"),
				(y3.i==0xFFFFFFFF?"T":"F"),
				(z0.i==0xFFFFFFFF?"T":"F"),
				(z1.i==0xFFFFFFFF?"T":"F"),
				(z2.i==0xFFFFFFFF?"T":"F"),
				(z3.i==0xFFFFFFFF?"T":"F"),
				(w0.i==0xFFFFFFFF?"T":"F"),
				(w1.i==0xFFFFFFFF?"T":"F"),
				(w2.i==0xFFFFFFFF?"T":"F"),
				(w3.i==0xFFFFFFFF?"T":"F") );
		else
			Printf( "<[%s,%s,%s,%s],[%s,%s,%s,%s],[%s,%s,%s,%s],[%s,%s,%s,%s]>",
				(x0.i==0xFFFFFFFF?"T":"F"),
				(x1.i==0xFFFFFFFF?"T":"F"),
				(x2.i==0xFFFFFFFF?"T":"F"),
				(x3.i==0xFFFFFFFF?"T":"F"),
				(y0.i==0xFFFFFFFF?"T":"F"),
				(y1.i==0xFFFFFFFF?"T":"F"),
				(y2.i==0xFFFFFFFF?"T":"F"),
				(y3.i==0xFFFFFFFF?"T":"F"),
				(z0.i==0xFFFFFFFF?"T":"F"),
				(z1.i==0xFFFFFFFF?"T":"F"),
				(z2.i==0xFFFFFFFF?"T":"F"),
				(z3.i==0xFFFFFFFF?"T":"F"),
				(w0.i==0xFFFFFFFF?"T":"F"),
				(w1.i==0xFFFFFFFF?"T":"F"),
				(w2.i==0xFFFFFFFF?"T":"F"),
				(w3.i==0xFFFFFFFF?"T":"F") );
	}
} // namespace rage

#endif