
#include "vec4v.h"

#if !__NO_OUTPUT

namespace rage
{
	void Vec4V::Print(bool newline) const
	{
		Vec::V4Print( v, newline );
	}

	void Vec4V::PrintHex(bool newline) const
	{
		Vec::V4PrintHex( v, newline );
	}
} // namespace rage

#endif	// __NO_OUTPUT
