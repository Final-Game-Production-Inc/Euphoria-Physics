//
// system/stack_inline.h
//
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_STACK_INLINE_H 
#define SYSTEM_STACK_INLINE_H

#if __PS3
#include <cell/dbg.h>
#elif __XENON
// xbdm.h makes a mess of things, so pull out the one thing we need.
extern "C" long __stdcall DmCaptureStackBackTrace(unsigned long FramesToCapture, void **BackTrace);
#endif

namespace rage {

const u32 sysStackMask = (1<<21)-1;

// buf should be an unsigned integer type, and count should be the number
// of frames to capture.  Use macros instead of functions to make sure
// the compiler doesn't insert an extra unnecessary stack frame.
#if __PS3
#define STACK_INLINE_CAPTURE(buf,count)					cellDbgPpuThreadGetStackBackTrace(0,count,buf,NULL);
inline u32 sysStackCompressFrameEntry(u32 entry)		{ return (entry >> 4) & sysStackMask; }
inline u32 sysStackDecompressFrameEntry(u32 entry)		{ return (entry & sysStackMask) << 4; }
#elif __XENON
#define STACK_INLINE_CAPTURE(buf,count)					DmCaptureStackBackTrace(count,(VOID**)(buf));
inline u32 sysStackCompressFrameEntry(u32 entry)		{ return ((entry - 0x82000000) >> 4) & sysStackMask; }
inline u32 sysStackDecompressFrameEntry(u32 entry)		{ return ((entry & sysStackMask) << 4) + 0x82000000; }
#else
#define STACK_INLINE_CAPTURE(buf,count)		buf[0] = 0
inline u32 sysStackCompressFrameEntry(u32)				{ return 0; }
inline u32 sysStackDecompressFrameEntry(u32)			{ return 0; }
#endif

// PURPOSE: Compress a stack frame by storing every three words in the
//		source as two words in the destination.  Note that the algorithm
//		is "lossy" in that it may be off by an instruction or two, but that should
//		still be more than sufficient for an accurate backtrace.
inline void sysCompressStack(u32 *dest,size_t destCount,const u32 *src,size_t srcCount)
{
	// This function compresses three source entries into two 3x21bit destination entries.
	Assert((destCount % 2) == 0);
	Assert((srcCount % 3) == 0);
	while (destCount && srcCount)
	{
		u32 src0 = sysStackCompressFrameEntry(src[0]);
		u32 src1 = sysStackCompressFrameEntry(src[1]);
		u32 src2 = sysStackCompressFrameEntry(src[2]);

		dest[0] = src0 | (src2 << 21);			// 11 lower bits
		dest[1] = src1 | ((src2 >> 11) << 21);	// 10 upper bits

		dest+=2; destCount-=2;
		src+=3; srcCount-=3;
	}
}

// PURPOSE: Decompress a stack frame by storing every two words in the
//		source as three words in the destination.
inline void sysDecompressStack(u32 *dest,size_t destCount,const u32 *src,size_t srcCount)
{
	Assert((destCount % 3) == 0);
	Assert((srcCount % 2) == 0);
	while (destCount && srcCount)
	{
		dest[0] = sysStackDecompressFrameEntry(src[0]);
		dest[1] = sysStackDecompressFrameEntry(src[1]);
		u32 src2 = (src[0] >> 21) | ((src[1] >> 21) << 11);
		dest[2] = sysStackDecompressFrameEntry(src2);

		dest+=3; destCount-=3;
		src+=2; srcCount-=2;
	}
}

} // namespace rage

#endif
