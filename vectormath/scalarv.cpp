
#include "scalarv.h"

namespace rage
{

#if !__NO_OUTPUT

	void ScalarV::Print(bool newline) const
	{
		mthAssertf( IsValid(), "Scalar is invalid" );

		Vec::V1Print( v, newline );
	}

	void ScalarV::PrintHex(bool newline) const
	{
		mthAssertf( IsValid(), "Scalar is invalid" );

		Vec::V1PrintHex( v, newline );
	}
#endif


	bool ScalarV::IsValid() const
	{
		union
		{
			float f;
			unsigned int u;
		} x, y, z, w;
		x.f = Vec::GetX(v);
		y.f = Vec::GetY(v);
		z.f = Vec::GetZ(v);
		w.f = Vec::GetW(v);
		return (x.u == y.u && x.u == z.u && x.u == w.u );
	}

} // namespace rage


