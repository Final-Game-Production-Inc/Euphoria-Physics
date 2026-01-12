#ifndef VECTORMATH_XBOXMATH_RAGE_H
#define VECTORMATH_XBOXMATH_RAGE_H

// Hack to use XMVECTOR (without clogging the global namespace) for vector benchmarking.

#define CONST const
#define VOID void

#define OutputDebugStringA(str) rage::Printf(str)

namespace xb
{
	typedef unsigned int UINT;
	typedef char CHAR;
	typedef unsigned char BYTE;
	typedef short SHORT;
	typedef unsigned short USHORT;

	typedef __w64 unsigned int UINT_PTR, *PUINT_PTR;

	typedef int INT;
	typedef bool BOOL;
	typedef float FLOAT;
	typedef __int64 INT64;
	typedef unsigned __int64 UINT64;

#include <xboxmath.h>
} // namespace xb

#undef VOID
#undef CONST

#endif // VECTORMATH_XBOXMATH_RAGE_H
