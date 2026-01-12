
#include "mat33v.h"

#if !__NO_OUTPUT

namespace rage
{


	void Mat33V::Print() const
	{
		Printf( "[%f, %f, %f]\n", GetM00f(), GetM01f(), GetM02f() );
		Printf( "[%f, %f, %f]\n", GetM10f(), GetM11f(), GetM12f() );
		Printf( "[%f, %f, %f]\n", GetM20f(), GetM21f(), GetM22f() );
	}

	void Mat33V::PrintHex() const
	{
		union {
			float f;
			unsigned int i;
		} x, y, z;

		x.f = GetM00f();	y.f = GetM01f();	z.f = GetM02f();
		Printf( "[0x%X, 0x%X, 0x%X]\n", x.i, y.i, z.i );
		x.f = GetM10f();	y.f = GetM11f();	z.f = GetM12f();
		Printf( "[0x%X, 0x%X, 0x%X]\n", x.i, y.i, z.i );
		x.f = GetM20f();	y.f = GetM21f();	z.f = GetM22f();
		Printf( "[0x%X, 0x%X, 0x%X]\n", x.i, y.i, z.i );
	}

} // namespace rage

#endif
