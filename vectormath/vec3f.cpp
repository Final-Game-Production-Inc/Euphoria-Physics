
#include "vec3f.h"

#if !__NO_OUTPUT

namespace rage
{

	void Vec3f::Print(bool newline) const
	{
		Vec::V3Print( v, newline );
	}

	void Vec3f::PrintHex(bool newline) const
	{
		Vec::V3PrintHex( v, newline );
	}

} // namespace rage

#endif
