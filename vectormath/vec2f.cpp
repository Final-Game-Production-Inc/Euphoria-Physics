
#include "vec2f.h"

#if !__NO_OUTPUT

namespace rage
{

	void Vec2f::Print(bool newline) const
	{
		Vec::V2Print( v, newline );
	}

	void Vec2f::PrintHex(bool newline) const
	{
		Vec::V2PrintHex( v, newline );
	}

} // namespace rage

#endif
