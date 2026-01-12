
#include "vec4f.h"

#if !__NO_OUTPUT

namespace rage
{

	void Vec4f::Print(bool newline) const
	{
		Vec::V4Print( v, newline );
	}

	void Vec4f::PrintHex(bool newline) const
	{
		Vec::V4PrintHex( v, newline );
	}

} // namespace rage

#endif
