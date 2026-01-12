
#include "vecbool1v_soa.h"

#if !__NO_OUTPUT

namespace rage
{
	void SoA_VecBool1V::Print(bool newline)
	{
		union Temp
		{
			float f;
			unsigned int i;
		} x0,x1,x2,x3;
		x0.f = Vec::GetX( x );
		x1.f = Vec::GetY( x );
		x2.f = Vec::GetZ( x );
		x3.f = Vec::GetW( x );
		VecAssertMsg(	(x0.i==0xFFFFFFFF || x0.i==0) &&
				(x1.i==0xFFFFFFFF || x1.i==0) &&
				(x2.i==0xFFFFFFFF || x2.i==0) &&
				(x3.i==0xFFFFFFFF || x3.i==0) ,
				"Warning: SoA_VecBool1V does not have all 0 or 0xFFFFFFFF words."
				);

		if( newline )
			Printf( "<[%s,%s,%s,%s]>\n",
				(x0.i==0xFFFFFFFF?"T":"F"),
				(x1.i==0xFFFFFFFF?"T":"F"),
				(x2.i==0xFFFFFFFF?"T":"F"),
				(x3.i==0xFFFFFFFF?"T":"F") );
		else
			Printf( "<[%s,%s,%s,%s]>",
				(x0.i==0xFFFFFFFF?"T":"F"),
				(x1.i==0xFFFFFFFF?"T":"F"),
				(x2.i==0xFFFFFFFF?"T":"F"),
				(x3.i==0xFFFFFFFF?"T":"F") );
	}
} // namespace rage

#endif
