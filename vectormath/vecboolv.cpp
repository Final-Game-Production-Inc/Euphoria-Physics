
#include "vecboolv.h"

#if !__NO_OUTPUT

namespace rage
{

	void VecBoolV::Print(bool newline) const
	{
		union Temp
		{
			float f;
			unsigned int i;
		} x, y, z, w;
		x.f = Vec::GetX( v );
		y.f = Vec::GetY( v );
		z.f = Vec::GetZ( v );
		w.f = Vec::GetW( v );

		VecAssertMsg(	(x.i==0xFFFFFFFF || x.i==0) &&
				(y.i==0xFFFFFFFF || y.i==0) &&
				(z.i==0xFFFFFFFF || z.i==0) &&
				(w.i==0xFFFFFFFF || w.i==0) ,
				"Warning: VecBoolV does not have all 0 or 0xFFFFFFFF words."
				);

		if( newline )
			Printf( "<%s,%s,%s,%s>\n",
			(x.i==0xFFFFFFFF?"T":"F"),
			(y.i==0xFFFFFFFF?"T":"F"),
			(z.i==0xFFFFFFFF?"T":"F"),
			(w.i==0xFFFFFFFF?"T":"F") );
		else
			Printf( "<%s,%s,%s,%s>",
			(x.i==0xFFFFFFFF?"T":"F"),
			(y.i==0xFFFFFFFF?"T":"F"),
			(z.i==0xFFFFFFFF?"T":"F"),
			(w.i==0xFFFFFFFF?"T":"F") );
	}
} // namespace rage

#endif