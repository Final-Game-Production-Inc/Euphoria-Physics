#include "vectorconfig.h"
#include "vectorutility.h"
#include "vectormath.h"

#if __WIN32PC
#pragma warning(disable:4668)
#include <intrin.h>
#pragma warning(error:4668)
#endif


namespace rage
{
	float GetAllF()
	{
		return GetAsFloat( 0xFFFFFFFF );
	}

	float GetNaN()
	{
		return GetAsFloat( 0x7FC00000 );
	}

	float GetInf()
	{
		return GetAsFloat( 0x7F800000 );
	}

	float GetNegInf()
	{
		return GetAsFloat( 0xFF800000 );
	}

	float Get0x7FFFFFFF()
	{
		return GetAsFloat( 0x7FFFFFFF );
	}

	float Get0x80000000()
	{
		return GetAsFloat( 0x80000000 );
	}

	float GetAsFloat(unsigned int num)
	{
		union
		{
			unsigned int u;
			float f;
		} Temp;
		Temp.u = num;
		return Temp.f;
	}

	unsigned int GetAsUint(float num)
	{
		union
		{
			unsigned int u;
			float f;
		} Temp;
		Temp.f = num;
		return Temp.u;
	}

#if __WIN32PC
	eProcVendor GetProcVendor()
	{
		// References:
		// http://softpixel.com/~cwright/programming/simd/cpuid.php

		eProcVendor retVal = VENDOR_OTHER;
		char vendorStr[12];

		u32 cpuInfo[4];
		__cpuid((int*)cpuInfo,0);

		*(u32*)(&vendorStr[0]) = cpuInfo[1];
		*(u32*)(&vendorStr[4]) = cpuInfo[3];
		*(u32*)(&vendorStr[8]) = cpuInfo[2];

		// Avoiding pulling in string.h.
		if(
			vendorStr[0] == 'G' &&
			vendorStr[1] == 'e' &&
			vendorStr[2] == 'n' &&
			vendorStr[3] == 'u' &&
			vendorStr[4] == 'i' &&
			vendorStr[5] == 'n' &&
			vendorStr[6] == 'e' &&
			vendorStr[7] == 'I' &&
			vendorStr[8] == 'n' &&
			vendorStr[9] == 't' &&
			vendorStr[10] == 'e' &&
			vendorStr[11] == 'l'
			)
		{
			retVal = VENDOR_INTEL;
		}
		else if(
			vendorStr[0] == 'A' &&
			vendorStr[1] == 'u' &&
			vendorStr[2] == 't' &&
			vendorStr[3] == 'h' &&
			vendorStr[4] == 'e' &&
			vendorStr[5] == 'n' &&
			vendorStr[6] == 't' &&
			vendorStr[7] == 'i' &&
			vendorStr[8] == 'c' &&
			vendorStr[9] == 'A' &&
			vendorStr[10] == 'M' &&
			vendorStr[11] == 'D'
			)
		{
			retVal = VENDOR_AMD;
		}
		return retVal;
	}

	u32 GetDataCacheLineSize()
	{
		// References:
		// http://softpixel.com/~cwright/programming/simd/cpuid.php

		u32 cacheLineSize = 0;

		eProcVendor procVendor = GetProcVendor();
		u8 code = 0;
		u32 cpuInfo[4];

		// A more accurate answer for the cache size requires using code 2
		// and setting ECX to the level of the cache to query, but VS2005
		// doesn't support the necessary __cpuidex instruction.
		if( procVendor == VENDOR_INTEL )
		{
			__cpuid((int*)cpuInfo,1);
			code =  u8(cpuInfo[1] >> 8);
			cacheLineSize = (u32)code * 8;
		}

		else if( procVendor == VENDOR_AMD )
		{
			__cpuid((int*)cpuInfo,0x80000005);
			code = u8(cpuInfo[2]);
			cacheLineSize = (u32)code;
		}

		else
		{
			mthWarningf( "Unknown CPU vendor, and thus unknown cache line size (assuming 64 bytes)." );
			cacheLineSize = 64;
		}

		return cacheLineSize;
	}
#endif // __WIN32PC

namespace Vec
{

	// Disable denormals, which can hurt performance in rare cases.
	void DisableDenormals()
	{
#if RSG_CPU_INTEL && (UNIQUE_VECTORIZED_TYPE)
		// http://software.intel.com/en-us/articles/x87-and-sse-floating-point-assists-in-ia-32-flush-to-zero-ftz-and-denormals-are-zero-daz
		// Diff between FTZ and DAZ: http://labcalc.phys.uniroma1.it/home/fortran/doc/main_for/mergedProjects/fpops_for/common/fpops_set_ftz_daz.htm
		int oldVals = _mm_getcsr();
		int newVals = oldVals;
		// Set flush-to-zero mode
		newVals |= _MM_FLUSH_ZERO_ON | _MM_MASK_UNDERFLOW;
		// Set denormals-as-zero mode (SSE2 and higher, except for some early Pentium 4 models)
		newVals |= _MM_MASK_DENORM /*| 0x0040*/; // 2nd flag irrelevant
		_mm_setcsr( newVals );
#elif __XENON && (UNIQUE_VECTORIZED_TYPE)
		// "Denormals already disabled on VMX128."
		// -- Cristi, MS Game Developer Support
#elif __SPU && (UNIQUE_VECTORIZED_TYPE)
		// "Denormals flushed to 0."
		// -- http://www.research.ibm.com/cell/SPU.html
#elif __PPU && (UNIQUE_VECTORIZED_TYPE)
		// Seem to be disabled by default, but just in case.
		vector unsigned short currentStatus = vec_mfvscr();
		vector unsigned short orBit = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0 };
		currentStatus = vec_or( currentStatus, orBit );
		vec_mtvscr( currentStatus );
#endif
	}

	void AssertDenormalsDisabled()
	{
#if RSG_CPU_INTEL && (UNIQUE_VECTORIZED_TYPE) && __ASSERT
		int csr = _mm_getcsr();
		mthAssertf( (csr&_MM_FLUSH_ZERO_ON) && (csr&_MM_MASK_UNDERFLOW) && (csr&_MM_MASK_DENORM), "Something very recently enabled denormals! Find it!\n" );
#elif __PPU && (UNIQUE_VECTORIZED_TYPE) && __ASSERT
		vector unsigned short currentStatus = vec_mfvscr();
		vector unsigned short andBit = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0 };
		currentStatus = vec_and( currentStatus, andBit );
		Vector_4V statusVec = (Vector_4V)currentStatus;
		mthAssertf( V4IsEqualIntAll( statusVec, V4VConstant(V_ZERO) ) != 0, "Something very recently enabled denormals! Find it!\n" );
#endif
	}

#if __WIN32PC
	eSSESupport GetSSESupportLevel()
	{
		// References:
		// http://msdn2.microsoft.com/en-us/library/xs6aek1h(VS.80).aspx and http://agner.org/optimize/

		int cpuInfo[4];
		__cpuid(cpuInfo,1);

		if (!(cpuInfo[3] & (1<<23)))
			return VEC_SCALAR;		// no mmx
		else if (!(cpuInfo[3] & (1<<25)))
			return VEC_MMX;		// no sse
		else if (!(cpuInfo[3] & (1<<26)))
			return VEC_SSE;		// no sse2
		else if (!(cpuInfo[2] & (1<<0)))
			return VEC_SSE2;		// no sse3
		else if (!(cpuInfo[2] & (1<<9)))
			return VEC_SSE3;
		else if (!(cpuInfo[2] & (1<<19)))
			return VEC_SSSE3;
		else if (!(cpuInfo[2] & (1<<20)))
			return VEC_SSE4_1;
		else
			return VEC_SSE4_2;

#if 0
		__asm {
			mov     eax, 1
			cpuid				// Get CPU info
			bt		edx, 23		// Check for MMX.
			jnc		SCALAR		// If no MMX, stop here.
			bt		edx, 25		// Check for SSE.
			jnc		MMX			// If no SSE, stop here.
			bt		edx, 26		// Check for SSE2.
			jnc		SSE			// If no SSE2, stop here.
			test	ecx, 1		// Check for SSE3.
			jz		SSE2		// If no SSE3, stop here.
			bt		ecx, 9		// Check for SSSE3.
			jnc		SSE3		// If no SSSE3, stop here.
			bt		ecx, 19		// Check for SSE4.1.
			jnc		SSSE3		// If no SSE4.1, stop here.
			bt		ecx, 20		// Check for SSE4.2.
			jnc		SSE4_1		// If no SSE4.2, stop here.
			jmp		SSE4_2		// We have SSE4.2.

SCALAR:		mov		dword ptr [retVal], 0
			jmp		DONE
MMX:		mov		dword ptr [retVal], 1
			jmp		DONE
SSE:		mov		dword ptr [retVal], 2
			jmp		DONE
SSE2:		mov		dword ptr [retVal], 3
			jmp		DONE
SSE3:		mov		dword ptr [retVal], 4
			jmp		DONE
SSSE3:		mov		dword ptr [retVal], 5
			jmp		DONE
SSE4_1:		mov		dword ptr [retVal], 6
			jmp		DONE
SSE4_2:		mov		dword ptr [retVal], 7

DONE:
		}

		return retVal;
#endif

		// This exception-catching method below doesn't compile for anything higher
		// than what the compiler supports...!

		////================================================
		//// Try for MMX.
		////================================================
		//__try {
		//	__asm {
		//		pxor mm0, mm0
		//		emms
		//	}
		//}
		//__except( EXCEPTION_EXECUTE_HANDLER ) {
		//	return retVal;
		//}
		//retVal = VEC_MMX;

		////================================================
		//// Try for SSE.
		////================================================
		//__try {
		//	__asm {
		//		xorps xmm0, xmm0
		//	}
		//}
		//__except( EXCEPTION_EXECUTE_HANDLER ) {
		//	return retVal;
		//}
		//retVal = VEC_SSE;

		////================================================
		//// Try for SSE2.
		////================================================
		//__try {
		//	__asm {
		//		xorpd xmm0, xmm0
		//	}
		//}
		//__except( EXCEPTION_EXECUTE_HANDLER ) {
		//	return retVal;
		//}
		//retVal = VEC_SSE2;

		////================================================
		//// Try for SSE3.
		////================================================
		//__try {
		//	__asm {
		//		haddps xmm0, xmm0
		//	}
		//}
		//__except( EXCEPTION_EXECUTE_HANDLER ) {
		//	return retVal;
		//}
		//retVal = VEC_SSE3;

		////================================================
		//// Try for Supplementary SSE3.
		////================================================
		//__try {
		//	__asm {
		//		pshufb xmm0, xmm0
		//	}
		//}
		//__except( EXCEPTION_EXECUTE_HANDLER ) {
		//	return retVal;
		//}
		//retVal = VEC_SSSE3;

		////================================================
		//// Try for SSE4.1.
		////================================================
		//
	}

	void PrintSSESupportLevelInfo(eSSESupport supp)
	{
		switch( supp )
		{
		case VEC_SCALAR:
			Printf( "No SSE support.\n" );
			break;
		case VEC_MMX:
			Printf( "MMX supported.\n" );
			break;
		case VEC_SSE:
			Printf( "SSE supported.\n" );
			break;
		case VEC_SSE2:
			Printf( "SSE2 supported.\n" );
			break;
		case VEC_SSE3:
			Printf( "SSE3 supported.\n" );
			break;
		case VEC_SSSE3:
			Printf( "Supplementary SSE3 supported.\n" );
			break;
		case VEC_SSE4_1:
			Printf( "SSE4.1 supported.\n" );
			break;
		case VEC_SSE4_2:
			Printf( "SSE4.2 supported.\n" );
			break;
		default:
			Printf( "Couldn't determine level of SSE support.\n" );
			break;
		};
	}
#endif // __WIN32PC

} // namespace Vec

} // namespace rage

