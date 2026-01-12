
#include "boolv.h"

#if !__NO_OUTPUT

namespace rage
{

	void BoolV::Print(bool newline)
	{
		union Temp
		{
			float f;
			unsigned int i;
		} x;
		x.f = Vec::GetX( v );

#if __ASSERT
		Temp y, z, w;
		y.f = Vec::GetY( v );
		z.f = Vec::GetZ( v );
		w.f = Vec::GetW( v );

		VecAssertMsg(	(x.i==0xFFFFFFFF && y.i==0xFFFFFFFF && z.i==0xFFFFFFFF && w.i==0xFFFFFFFF) ||
						(x.i==0 && y.i==0 && z.i==0 && w.i==0),
				"Warning: BoolV does not have all 0 or 0xFFFFFFFF words."
				);
#endif

		if( newline )
			Printf( "<%s>\n",
			(x.i!=0x0?"T":"F") );
		else
			Printf( "<%s>",
			(x.i!=0x0?"T":"F") );
	}

} // namespace rage

#endif
