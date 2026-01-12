
#include "mat34v.h"

#if !__NO_OUTPUT

namespace rage
{


	void Mat34V::Print() const
	{
		Printf( "[%f, %f, %f, %f]\n", GetM00f(), GetM01f(), GetM02f(), GetM03f() );
		Printf( "[%f, %f, %f, %f]\n", GetM10f(), GetM11f(), GetM12f(), GetM13f() );
		Printf( "[%f, %f, %f, %f]\n", GetM20f(), GetM21f(), GetM22f(), GetM23f() );
	}

	void Mat34V::PrintHex() const
	{
		union {
			float f;
			unsigned int i;
		} x, y, z, w;

		x.f = GetM00f();	y.f = GetM01f();	z.f = GetM02f();	w.f = GetM03f();
		Printf( "[0x%X, 0x%X, 0x%X, 0x%X]\n", x.i, y.i, z.i, w.i );
		x.f = GetM10f();	y.f = GetM11f();	z.f = GetM12f();	w.f = GetM13f();
		Printf( "[0x%X, 0x%X, 0x%X, 0x%X]\n", x.i, y.i, z.i, w.i );
		x.f = GetM20f();	y.f = GetM21f();	z.f = GetM22f();	w.f = GetM23f();
		Printf( "[0x%X, 0x%X, 0x%X, 0x%X]\n", x.i, y.i, z.i, w.i );
	}

} // namespace rage

#endif
