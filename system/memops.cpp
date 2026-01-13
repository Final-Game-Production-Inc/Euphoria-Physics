//
// system/memops.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#include "memops.h"

#if __PPU

#include <altivec.h>
#include <vec_types.h>
#include <sys/sys_fixed_addr.h>
#include "system/cache.h"

namespace rage {

void *sysMemCpySelect(void *dest, const void *src, size_t count)
{
	enum { RSX_LOCAL_SIZE = 0x10000000 };

	// This assert should be enabled when B*855190/908821 is fixed.
// 	// Reading from vram is bad, hmm ok
// 	FastAssert((u32)src-RSX_FB_BASE_ADDR >= (u32)RSX_LOCAL_SIZE);

	if (Unlikely((u32)dest-RSX_FB_BASE_ADDR < (u32)RSX_LOCAL_SIZE))
	{
		return sysMemCpyNonCachable(dest, src, count);
	}
	else if (!(((u32)dest)&127) && !(((u32)src)&15) && !(count&127))
	{
		return sysMemCpy128Impl(dest, src, count);
	}
	else if (!(((u32)dest)&15) && !(((u32)src)&15) && !(count&15) && count >= 256)
	{
		return sysMemCpyImpl(dest, src, count);
	}
	else
	{
		return memcpy(dest, src, count);
	}
}


void *sysMemCpyNonCachable(void *dest, const void *src, size_t count)
{
	if (Unlikely(!count))
		return dest;

	u8 *d8 = (u8*)dest;
	const u8 *s8 = (u8*)src;
	if ((uptr)d8&1)
	{
		*d8++ = *s8++;
		--count;
	}
	u16 *d16 = (u16*)d8;
	const u16 *s16 = (u16*)s8;
	if (((uptr)d16&2) && (count>=2))
	{
		*d16++ = *s16++;
		count -= 2;
	}
	u32 *d32 = (u32*)d16;
	const u32 *s32 = (u32*)s16;
	if (((uptr)d32&4) && (count>=4))
	{
		*d32++ = *s32++;
		count -= 4;
	}
	u64 *d64 = (u64*)d32;
	const u64 *s64 = (u64*)s32;
	if (((uptr)d64&8) && (count>=8))
	{
		*d64++ = *s64++;
		count -= 8;
	}
	vec_uchar16 *d128 = (vec_uchar16*)d64;
	const vec_uchar16 *s128 = (vec_uchar16*)s64;
	vec_uchar16 perm = vec_lvsl(0, (unsigned char*)s128);
	vec_uchar16 vl   = vec_lvx(0, s128);
	while (count >= 16)
	{
		vec_uchar16 vr = vec_lvx(16, s128);
		vec_uchar16 v = vec_vperm(vl, vr, perm);
		vl = vr;
		++s128;
		count -= 16;
		*d128++ = v;
	}
	d64 = (u64*)d128;
	s64 = (u64*)s128;
	if (count>=8)
	{
		*d64++ = *s64++;
		count -= 8;
	}
	d32 = (u32*)d64;
	s32 = (u32*)s64;
	if (count>=4)
	{
		*d32++ = *s32++;
		count -= 4;
	}
	d16 = (u16*)d32;
	s16 = (u16*)s32;
	if (count>=2)
	{
		*d16++ = *s16++;
		count -= 2;
	}
	d8 = (u8*)d16;
	s8 = (u8*)s16;
	if (count)
	{
		*d8++ = *s8++;
	}
	return dest;
}


void *sysMemCpy128Impl(void *dest, const void *src, size_t count)
{
	if (!count)
		return dest;

	const vec_uchar16 * RESTRICT s = (const vec_uchar16*)src;
	vec_uchar16 * RESTRICT d = (vec_uchar16*)dest;

	for (size_t i = 0, e = (count < 1024 ? count : 1024); i < e; i += 128)
		PrefetchDC2(s, i);

	ZeroDC(d, 0);

	for (size_t i = 0; i < count; i += 128)
	{
		if (i + 128 < count)
		{
			ZeroDC(d, 128);

			if (i + 1024 < count)
				PrefetchDC2(s, 1024);
		}

		const vec_uchar16 v0 = s[0];
		const vec_uchar16 v1 = s[1];
		const vec_uchar16 v2 = s[2];
		const vec_uchar16 v3 = s[3];
		const vec_uchar16 v4 = s[4];
		const vec_uchar16 v5 = s[5];
		const vec_uchar16 v6 = s[6];
		const vec_uchar16 v7 = s[7];

		d[0] = v0;
		d[1] = v1;
		d[2] = v2;
		d[3] = v3;
		d[4] = v4;
		d[5] = v5;
		d[6] = v6;
		d[7] = v7;

		d += 8;	s += 8;
	}

	return dest;
}


void *sysMemCpyImpl(void *dest, const void *src, size_t count)
{
	const vec_uchar16 * RESTRICT s = (const vec_uchar16*)src;
	vec_uchar16 * RESTRICT d = (vec_uchar16*)dest;

	for (size_t i = 0, e = (count < 1024 ? count : 1024); i < e; i += 128)
		PrefetchDC2(s, i);

	size_t leading = ((((u32)dest) + 127) & ~127) - (u32)dest; // bytes between dest and beginning of next cache line, may be 0
	FastAssert(leading < count);
	FastAssert(leading < 128);
	FastAssert((((u32)dest) & 127) == 0 || (128 - (((u32)dest) & 127)) == leading);

	size_t trailing = (((u32)dest) + count) & 127; // bytes used on final cache line
	FastAssert(trailing < count);
	FastAssert(trailing < 128);

	size_t middle = count - leading - trailing;
	FastAssert(!(((u32)middle) & 127));
	FastAssert(middle >= 128);	// ensure we still have at least one cache line to write

	for (size_t i = 0; i < leading; i += 16)
		*d++ = *s++;

	FastAssert(!(((u32)d)&127));		// ensure now 128 byte aligned

	ZeroDC(d, 0);

	for (size_t i = 0; i < middle; i += 128)
	{
		if (i + 128 < middle)
			ZeroDC(d, 128);

		if (i + 1024 < middle + trailing)
			PrefetchDC2(s, 1024);

		const vec_uchar16 v0 = s[0];
		const vec_uchar16 v1 = s[1];
		const vec_uchar16 v2 = s[2];
		const vec_uchar16 v3 = s[3];
		const vec_uchar16 v4 = s[4];
		const vec_uchar16 v5 = s[5];
		const vec_uchar16 v6 = s[6];
		const vec_uchar16 v7 = s[7];

		d[0] = v0;
		d[1] = v1;
		d[2] = v2;
		d[3] = v3;
		d[4] = v4;
		d[5] = v5;
		d[6] = v6;
		d[7] = v7;

		d += 8;	s += 8;
	}

	for (size_t i = 0; i < trailing; i += 16)
		*d++ = *s++;

	return dest;
}


void *sysMemCpyStreamingImpl(void *dest, const void *src, size_t count)
{
	const vec_uchar16 * RESTRICT s = (const vec_uchar16*)src;
	vec_uchar16 * RESTRICT d = (vec_uchar16*)dest;

	for (size_t i = 0, e = (count < 1024 ? count : 1024); i < e; i += 128)
		PrefetchDC2(s, i);

	size_t leading = ((((u32)dest) + 127) & ~127) - (u32)dest; // bytes between dest and beginning of next cache line, may be 0
	FastAssert(leading < count);
	FastAssert(leading < 128);
	FastAssert((((u32)dest) & 127) == 0 || (128 - (((u32)dest) & 127)) == leading);

	size_t trailing = (((u32)dest) + count) & 127; // bytes used on final cache line
	FastAssert(trailing < count);
	FastAssert(trailing < 128);

	size_t middle = count - leading - trailing;

	for (size_t i = 0; i < leading; i += 16)
		*d++ = *s++;

	FastAssert(!(((u32)d)&127));// ensure now 128 byte aligned
	FastAssert(middle >= 128);	// ensure we still have at least one cache line to write

	ZeroDC(d, 0);

	for (size_t i = 0; i < middle; i += 128)
	{
		if (i + 128 < middle)
			ZeroDC(d, 128);

		if (i + 1024 < middle + trailing)
			PrefetchDC2(s, 1024);

		const vec_uchar16 v0 = s[0];
		const vec_uchar16 v1 = s[1];
		const vec_uchar16 v2 = s[2];
		const vec_uchar16 v3 = s[3];
		const vec_uchar16 v4 = s[4];
		const vec_uchar16 v5 = s[5];
		const vec_uchar16 v6 = s[6];
		const vec_uchar16 v7 = s[7];
		FreeDC(s, 0);

		d[0] = v0;
		d[1] = v1;
		d[2] = v2;
		d[3] = v3;
		d[4] = v4;
		d[5] = v5;
		d[6] = v6;
		d[7] = v7;
		FreeDC(d, 0);

		d += 8;	s += 8;
	}

	for (size_t i = 0; i < trailing; i += 16)
		*d++ = *s++;

	return dest;
}


void *sysMemSet128Impl(void *dest, int c, size_t count)
{
	if (!count)
		return dest;

	vec_uchar16 *p = (vec_uchar16*)dest;
	vec_uchar16 v = vec_splats((u8)c);

	for (size_t i = 0, e = (count < 1024 ? count : 1024); i < e; i += 128)
		ZeroDC(p, i);

	for (size_t i = 0; i < count; i += 128)
	{
		if (i + 1024 < count)
			ZeroDC(p, 1024);

		p[0] = v; p[1] = v;	p[2] = v; p[3] = v;
		p[4] = v; p[5] = v;	p[6] = v; p[7] = v;

		p += 8;
	}

	return dest;
}


void *sysMemSetSelect(void *dest, int c, size_t count)
{
	if (!(((u32)dest)&127) && !(count&127))
	{
		if ((u8)c == 0)
		{
			for (size_t i = 0; i < count; i += 128)
				ZeroDC(dest, i);
			return dest;
		}
		else if (count >= 256)
		{
			return sysMemSet128Impl(dest, c, count);
		}
		else
		{
			return memset(dest, c, count);
		}
	}
	else if (!(((u32)dest)&15) && !(count&15) && count >= 256)
	{
		if ((u8)c == 0)
		{
			return sysMemZeroImpl(dest, count);
		}
		else
		{
			return sysMemSetImpl(dest, c, count);
		}
	}
	else
	{
		return memset(dest, c, count);
	}
}


void *sysMemSetImpl(void *dest, int c, size_t count)
{
	size_t leading = ((((u32)dest) + 127) & ~127) - (u32)dest; // bytes between dest and beginning of next cache line, may be 0
	FastAssert(leading < count);
	FastAssert(leading < 128);
	FastAssert((((u32)dest) & 127) == 0 || (128 - (((u32)dest) & 127)) == leading);

	size_t trailing = (((u32)dest) + count) & 127; // bytes used on final cache line
	FastAssert(trailing < count);
	FastAssert(trailing < 128);

	size_t middle = count - leading - trailing;
	FastAssert(!(((u32)middle) & 127));
	FastAssert(middle >= 128);	// ensure we still have at least one cache line to write

	u8 b = c;
	vec_uchar16 * RESTRICT p = (vec_uchar16*)dest;
	vec_uchar16 v = vec_splats(b);

	for (size_t i = 0; i < leading; i += 16)
		*p++ = v;

	FastAssert(!(((u32)p)&127));		// ensure now 128 byte aligned

	for (size_t i = 0, e = (middle < 1024 ? middle : 1024); i < e; i += 128)
		ZeroDC(p, i);

	for (size_t i = 0; i < middle; i += 128)
	{
		if (i + 1024 < middle)
			ZeroDC(p, 1024);

		p[0] = v; p[1] = v;	p[2] = v; p[3] = v;
		p[4] = v; p[5] = v;	p[6] = v; p[7] = v;

		p += 8;
	}

	for (size_t i = 0; i < trailing; i += 16)
		*p++ = v;

	return dest;
}


void *sysMemZeroImpl(void *dest, size_t count)
{
	size_t leading = ((((u32)dest) + 127) & ~127) - (u32)dest; // bytes between dest and beginning of next cache line, may be 0
	FastAssert(leading < count);
	FastAssert(leading < 128);
	FastAssert((((u32)dest) & 127) == 0 || (128 - (((u32)dest) & 127)) == leading);

	size_t trailing = (((u32)dest) + count) & 127; // bytes used on final cache line
	FastAssert(trailing < count);
	FastAssert(trailing < 128);

	size_t middle = count - leading - trailing;
	FastAssert(!(((u32)middle) & 127));
	FastAssert(middle >= 128);	// ensure we still have at least one cache line to write

	vec_int4 * RESTRICT p = (vec_int4*)dest;
	vec_int4 v = vec_splat_s32(0);

	for (size_t i = 0; i < leading; i += 16)
		*p++ = v;

	FastAssert(!(((u32)p)&127));		// ensure now 128 byte aligned

	for (size_t i = 0; i < middle; i += 128)
		ZeroDC(p, i);

	p = (vec_int4*)((char*)p + middle);

	for (size_t i = 0; i < trailing; i += 16)
		*p++ = v;

	return dest;
}

} // namespace rage

#endif // __PPU
