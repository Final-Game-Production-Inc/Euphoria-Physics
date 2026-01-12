
#include "vec2v.h"

#if !__NO_OUTPUT

namespace rage
{
	void Vec2V::Print(bool newline) const
	{
		Vec::V2Print( v, newline );
	}

	void Vec2V::PrintHex(bool newline) const
	{
		Vec::V2PrintHex( v, newline );
	}
} // namespace rage

#endif	// __NO_OUTPUT
