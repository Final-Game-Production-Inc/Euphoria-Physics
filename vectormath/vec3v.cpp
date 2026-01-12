
#include "vec3v.h"

#if !__NO_OUTPUT

namespace rage
{
	void Vec3V::Print(bool newline) const
	{
		Vec::V3Print( v, newline );
	}

	void Vec3V::Print(const char * label, bool newline) const
	{
		Vec::V3Print( v, label, newline );
	}

	void Vec3V::PrintHex(bool newline) const
	{
		Vec::V3PrintHex( v, newline );
	}
} // namespace rage

#endif
