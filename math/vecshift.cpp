// ======================
// math/vecshift.cpp
// (c) 2013 RockstarNorth
// ======================

#include "math/amath.h"
#include "math/vecshift.h"

namespace rage {

#if RSG_DEV && RSG_ASSERT
void GenerateMask128_TEST()
{
	const int w = 128;

	for (int i0 = 0; i0 <= w; i0++)
	{
		for (int i1 = i0; i1 <= w; i1++)
		{
			// =================================
			// REFERENCE
			// =================================

			u64 maskA_REFERENCE = 0;
			u64 maskB_REFERENCE = 0;

			for (int i = i0; i < i1; i++)
			{
				if (i < 64)
				{
					maskB_REFERENCE |= BIT64(i);
				}
				else
				{
					maskA_REFERENCE |= BIT64(i - 64);
				}
			}

			if (0)
			{
				Displayf(
					"GenerateMask128(%03d,%03d): 0x%08x%08x.%08x%08x",
					i0,
					i1,
					(u32)(maskA_REFERENCE>>32),
					(u32)(maskA_REFERENCE),
					(u32)(maskB_REFERENCE>>32),
					(u32)(maskB_REFERENCE)
				);
			}

			// =================================
			// TEST1 - shifting
			// =================================
			{
				const int sr = w - i1;
				const int sl = 0 + i0;
#if RSG_CPU_X64
				const u64 maskA_TEST1 = ClampedShiftRight64(~0ULL, sr) & ClampedShiftLeft64(~0ULL, sl - 64);
				const u64 maskB_TEST1 = ClampedShiftLeft64(~0ULL, sl) & ClampedShiftRight64(~0ULL, sr - 64);
#else
				u64 maskA_TEST1 = ((~0ULL) >> sr) & ((~0ULL) << Max<int>(0, sl - 64));
				u64 maskB_TEST1 = ((~0ULL) << sl) & ((~0ULL) >> Max<int>(0, sr - 64));

				if (i0 == i1)
				{
					maskA_TEST1 = 0;
					maskB_TEST1 = 0;
				}
#endif
				Assertf(
					maskA_TEST1 == maskA_REFERENCE &&
					maskB_TEST1 == maskB_REFERENCE,
					"GenerateMask128(%03d,%03d): TEST1 = 0x%08x%08x.%08x%08x, should be 0x%08x%08x.%08x%08x",
					i0,
					i1,
					(u32)(maskA_TEST1>>32),
					(u32)(maskA_TEST1),
					(u32)(maskB_TEST1>>32),
					(u32)(maskB_TEST1),
					(u32)(maskA_REFERENCE>>32),
					(u32)(maskA_REFERENCE),
					(u32)(maskB_REFERENCE>>32),
					(u32)(maskB_REFERENCE)
				);
			}

			// =================================
			// TEST2 - intrinsics (shift left)
			// =================================
			{
				const Vec4V bounds((float)i0, (float)i1, 0.0f, 0.0f);
				const Vec4V mask = GenerateMaskShiftLeft128(FloatToIntRaw<0>(bounds));

				const u64 maskA_TEST2 = ((const u64*)&mask)[RSG_BE ? 0 : 1];
				const u64 maskB_TEST2 = ((const u64*)&mask)[RSG_BE ? 1 : 0];

				Assertf(
					maskA_TEST2 == maskA_REFERENCE &&
					maskB_TEST2 == maskB_REFERENCE,
					"GenerateMask128(%03d,%03d): TEST2 (intrinsics shift left) = 0x%08x%08x.%08x%08x, should be 0x%08x%08x.%08x%08x",
					i0,
					i1,
					(u32)(maskA_TEST2>>32),
					(u32)(maskA_TEST2),
					(u32)(maskB_TEST2>>32),
					(u32)(maskB_TEST2),
					(u32)(maskA_REFERENCE>>32),
					(u32)(maskA_REFERENCE),
					(u32)(maskB_REFERENCE>>32),
					(u32)(maskB_REFERENCE)
				);
			}

			// =================================
			// TEST3 - intrinsics (shift right)
			// =================================
			{
				const Vec4V bounds((float)(w - i1), (float)(w - i0), 0.0f, 0.0f);
				const Vec4V mask = GenerateMaskShiftRight128(FloatToIntRaw<0>(bounds));

				const u64 maskA_TEST3 = ((const u64*)&mask)[RSG_BE ? 0 : 1];
				const u64 maskB_TEST3 = ((const u64*)&mask)[RSG_BE ? 1 : 0];

				Assertf(
					maskA_TEST3 == maskA_REFERENCE &&
					maskB_TEST3 == maskB_REFERENCE,
					"GenerateMask128(%03d,%03d): TEST2 (intrinsics shift right) = 0x%08x%08x.%08x%08x, should be 0x%08x%08x.%08x%08x",
					i0,
					i1,
					(u32)(maskA_TEST3>>32),
					(u32)(maskA_TEST3),
					(u32)(maskB_TEST3>>32),
					(u32)(maskB_TEST3),
					(u32)(maskA_REFERENCE>>32),
					(u32)(maskA_REFERENCE),
					(u32)(maskB_REFERENCE>>32),
					(u32)(maskB_REFERENCE)
				);
			}
		}
	}
}
#endif // RSG_DEV && RSG_ASSERT

} // namespace rage

