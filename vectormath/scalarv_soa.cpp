
#include "scalarv_soa.h"

#if !__NO_OUTPUT

namespace rage
{
	void SoA_ScalarV::Print(bool newline) const
	{
		if( newline )
			Printf( "<[%f,%f,%f,%f]>\n", Vec::GetX(x),Vec::GetY(x),Vec::GetZ(x),Vec::GetW(x) );
		else
			Printf( "<[%f,%f,%f,%f]>", Vec::GetX(x),Vec::GetY(x),Vec::GetZ(x),Vec::GetW(x) );		
	}

	void SoA_ScalarV::PrintHex(bool newline) const
	{
		union {
			float f;
			unsigned int i;
		} x0, x1, x2, x3;
		x0.f = Vec::GetX(x);
		x1.f = Vec::GetY(x);
		x2.f = Vec::GetZ(x);
		x3.f = Vec::GetW(x);

		if( newline )
			Printf( "<[0x%X,0x%X,0x%X,0x%X]>\n", x0.i,x1.i,x2.i,x3.i );
		else
			Printf( "<[0x%X,0x%X,0x%X,0x%X]>", x0.i,x1.i,x2.i,x3.i );
	}

} // namespace rage

#endif