
#include "quatf.h"

#if !__NO_OUTPUT

namespace rage
{

	void Quatf::Print(bool newline) const
	{
		Vec::V4Print( v, newline );
	}

	void Quatf::PrintHex(bool newline) const
	{
		Vec::V4PrintHex( v, newline );
	}

} // namespace rage

#endif