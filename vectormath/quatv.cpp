
#include "quatv.h"

#if !__NO_OUTPUT

namespace rage
{

	void QuatV::Print(bool newline) const
	{
		Vec::V4Print( v, newline );
	}

	void QuatV::PrintHex(bool newline) const
	{
		Vec::V4PrintHex( v, newline );
	}

} // namespace rage

#endif