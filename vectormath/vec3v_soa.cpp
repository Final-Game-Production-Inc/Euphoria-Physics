
#include "vec3v_soa.h"

#if !__NO_OUTPUT

namespace rage
{
	void SoA_Vec3V::Print(bool newline) const
	{
		if( newline )
			Printf( "<[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f]>\n",
				Vec::GetX(x),Vec::GetY(x),Vec::GetZ(x),Vec::GetW(x),
				Vec::GetX(y),Vec::GetY(y),Vec::GetZ(y),Vec::GetW(y),
				Vec::GetX(z),Vec::GetY(z),Vec::GetZ(z),Vec::GetW(z) );	
		else
			Printf( "<[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f]>",
				Vec::GetX(x),Vec::GetY(x),Vec::GetZ(x),Vec::GetW(x),
				Vec::GetX(y),Vec::GetY(y),Vec::GetZ(y),Vec::GetW(y),
				Vec::GetX(z),Vec::GetY(z),Vec::GetZ(z),Vec::GetW(z) );		
	}

	void SoA_Vec3V::PrintHex(bool newline) const
	{
		union {
			float f;
			unsigned int i;
		} x0, x1, x2, x3, y0, y1, y2, y3, z0, z1, z2, z3;
		x0.f = Vec::GetX(x);
		x1.f = Vec::GetY(x);
		x2.f = Vec::GetZ(x);
		x3.f = Vec::GetW(x);
		y0.f = Vec::GetX(y);
		y1.f = Vec::GetY(y);
		y2.f = Vec::GetZ(y);
		y3.f = Vec::GetW(y);
		z0.f = Vec::GetX(z);
		z1.f = Vec::GetY(z);
		z2.f = Vec::GetZ(z);
		z3.f = Vec::GetW(z);

		if( newline )
			Printf( "<[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X]>\n",
				x0.i,x1.i,x2.i,x3.i,
				y0.i,y1.i,y2.i,y3.i,
				z0.i,z1.i,z2.i,z3.i );
		else
			Printf( "<[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X]>",
				x0.i,x1.i,x2.i,x3.i,
				y0.i,y1.i,y2.i,y3.i,
				z0.i,z1.i,z2.i,z3.i );
	}

} // namespace rage

#endif
