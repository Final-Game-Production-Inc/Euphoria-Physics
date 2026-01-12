#include "vectormath.h"

#if !__NO_OUTPUT

namespace rage
{
namespace Vec
{
	void V4Print(Vector_4_In inVector, bool newline)
	{
		if( newline )
			Printf( "<%f,%f,%f,%f>\n", GetX(inVector), GetY(inVector), GetZ(inVector), GetW(inVector) );
		else
			Printf( "<%f,%f,%f,%f>", GetX(inVector), GetY(inVector), GetZ(inVector), GetW(inVector) );
	}

	void V4PrintHex(Vector_4_In inVector, bool newline)
	{
		union {
			float f;
			unsigned int i;
		} x, y, z, w;
		x.f = GetX(inVector);
		y.f = GetY(inVector);
		z.f = GetZ(inVector);
		w.f = GetW(inVector);

		if( newline )
			Printf( "<0x%X,0x%X,0x%X,0x%X>\n", x.i, y.i, z.i, w.i );
		else
			Printf( "<0x%X,0x%X,0x%X,0x%X>", x.i, y.i, z.i, w.i );
	}

#if UNIQUE_VECTORIZED_TYPE
	void V4Print(Vector_4V_In inVector, bool newline)
	{
		if( newline )
			Printf( "<%f,%f,%f,%f>\n", GetX(inVector), GetY(inVector), GetZ(inVector), GetW(inVector) );
		else
			Printf( "<%f,%f,%f,%f>", GetX(inVector), GetY(inVector), GetZ(inVector), GetW(inVector) );
	}

	void V4PrintHex(Vector_4V_In inVector, bool newline)
	{
		union {
			float f;
			unsigned int i;
		} x, y, z, w;
		x.f = GetX(inVector);
		y.f = GetY(inVector);
		z.f = GetZ(inVector);
		w.f = GetW(inVector);

		if( newline )
			Printf( "<0x%X,0x%X,0x%X,0x%X>\n", x.i, y.i, z.i, w.i );
		else
			Printf( "<0x%X,0x%X,0x%X,0x%X>", x.i, y.i, z.i, w.i );
	}
#endif // UNIQUE_VECTORIZED_TYPE

	void V3Print(Vector_3_In inVector, bool newline)
	{
		if( newline )
			Printf( "<%f,%f,%f>\n", GetX(inVector), GetY(inVector), GetZ(inVector) );
		else
			Printf( "<%f,%f,%f>", GetX(inVector), GetY(inVector), GetZ(inVector) );
	}

	void V3PrintHex(Vector_3_In inVector, bool newline)
	{
		union {
			float f;
			unsigned int i;
		} x, y, z;
		x.f = GetX(inVector);
		y.f = GetY(inVector);
		z.f = GetZ(inVector);

		if( newline )
			Printf( "<0x%X,0x%X,0x%X>\n", x.i, y.i, z.i );
		else
			Printf( "<0x%X,0x%X,0x%X>", x.i, y.i, z.i );
	}

#if UNIQUE_VECTORIZED_TYPE
	void V3Print(Vector_4V_In inVector, bool newline)
	{
		if( newline )
			Printf( "<%f,%f,%f>\n", GetX(inVector), GetY(inVector), GetZ(inVector) );
		else
			Printf( "<%f,%f,%f>", GetX(inVector), GetY(inVector), GetZ(inVector) );
	}

	void V3Print(Vector_4V_In inVector, const char * label, bool newline)
	{
		if( newline )
			Printf( "%s: <%f,%f,%f>\n", label, GetX(inVector), GetY(inVector), GetZ(inVector) );
		else
			Printf( "%s: <%f,%f,%f>", label, GetX(inVector), GetY(inVector), GetZ(inVector) );
	}

	void V3PrintHex(Vector_4V_In inVector, bool newline)
	{
		union {
			float f;
			unsigned int i;
		} x, y, z;
		x.f = GetX(inVector);
		y.f = GetY(inVector);
		z.f = GetZ(inVector);

		if( newline )
			Printf( "<0x%X,0x%X,0x%X>\n", x.i, y.i, z.i );
		else
			Printf( "<0x%X,0x%X,0x%X>", x.i, y.i, z.i );
	}
#endif // UNIQUE_VECTORIZED_TYPE

	void V3Print(Vector_4_In inVector, bool newline)
	{
		if( newline )
			Printf( "<%f,%f,%f>\n", GetX(inVector), GetY(inVector), GetZ(inVector) );
		else
			Printf( "<%f,%f,%f>", GetX(inVector), GetY(inVector), GetZ(inVector) );
	}

	void V3PrintHex(Vector_4_In inVector, bool newline)
	{
		union {
			float f;
			unsigned int i;
		} x, y, z;
		x.f = GetX(inVector);
		y.f = GetY(inVector);
		z.f = GetZ(inVector);

		if( newline )
			Printf( "<0x%X,0x%X,0x%X>\n", x.i, y.i, z.i );
		else
			Printf( "<0x%X,0x%X,0x%X>", x.i, y.i, z.i );
	}

	void V2Print(Vector_2_In inVector, bool newline)
	{
		if( newline )
			Printf( "<%f,%f>\n", GetX(inVector), GetY(inVector) );
		else
			Printf( "<%f,%f>", GetX(inVector), GetY(inVector) );
	}

	void V2PrintHex(Vector_2_In inVector, bool newline)
	{
		union {
			float f;
			unsigned int i;
		} x, y;
		x.f = GetX(inVector);
		y.f = GetY(inVector);

		if( newline )
			Printf( "<0x%X,0x%X>\n", x.i, y.i );
		else
			Printf( "<0x%X,0x%X>", x.i, y.i );
	}

#if UNIQUE_VECTORIZED_TYPE
	void V2Print(Vector_4V_In inVector, bool newline)
	{
		if( newline )
			Printf( "<%f,%f>\n", GetX(inVector), GetY(inVector) );
		else
			Printf( "<%f,%f>", GetX(inVector), GetY(inVector) );
	}

	void V2PrintHex(Vector_4V_In inVector, bool newline)
	{
		union {
			float f;
			unsigned int i;
		} x, y;
		x.f = GetX(inVector);
		y.f = GetY(inVector);

		if( newline )
			Printf( "<0x%X,0x%X>\n", x.i, y.i );
		else
			Printf( "<0x%X,0x%X>", x.i, y.i );
	}
#endif // UNIQUE_VECTORIZED_TYPE

	void V2Print(Vector_4_In inVector, bool newline)
	{
		if( newline )
			Printf( "<%f,%f>\n", GetX(inVector), GetY(inVector) );
		else
			Printf( "<%f,%f>", GetX(inVector), GetY(inVector) );
	}

	void V2PrintHex(Vector_4_In inVector, bool newline)
	{
		union {
			float f;
			unsigned int i;
		} x, y;
		x.f = GetX(inVector);
		y.f = GetY(inVector);

		if( newline )
			Printf( "<0x%X,0x%X>\n", x.i, y.i );
		else
			Printf( "<0x%X,0x%X>", x.i, y.i );
	}

#if UNIQUE_VECTORIZED_TYPE
	void V1Print(Vector_4V_In inVector, bool newline)
	{
		if( newline )
			Printf( "<%f>\n", GetX(inVector) );
		else
			Printf( "<%f>", GetX(inVector) );
	}

	void V1PrintHex(Vector_4V_In inVector, bool newline)
	{
		union {
			float f;
			unsigned int i;
		} x;
		x.f = GetX(inVector);

		if( newline )
			Printf( "<0x%X>\n", x.i );
		else
			Printf( "<0x%X>", x.i );
	}

#endif // UNIQUE_VECTORIZED_TYPE

	void V1Print(Vector_4_In inVector, bool newline)
	{
		if( newline )
			Printf( "<%f>\n", GetX(inVector) );
		else
			Printf( "<%f>", GetX(inVector) );
	}

	void V1PrintHex(Vector_4_In inVector, bool newline)
	{
		union {
			float f;
			unsigned int i;
		} x;
		x.f = GetX(inVector);

		if( newline )
			Printf( "<0x%X>\n", x.i );
		else
			Printf( "<0x%X>", x.i );
	}

} // namespace Vec
} // namespace rage

#endif
