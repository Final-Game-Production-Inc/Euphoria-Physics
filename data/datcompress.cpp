//
// data/compress.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "compress.h"
#include "system/alloca.h"

#include <stddef.h>
#include <string.h>

namespace rage {

// FILE *log;

/*
	known problem:
	our algorithm sometimes outputs three windows where
	one literal and two windows would suffice; it happens
	in situations like this:

	abcxxxbcdefghijklmnopqrstuvxyz123abcdefghijklmnopqrstuvxyz123

	we output abc,defghijklmnopqrstu,vwxyz123 (three windows, 3*17 bits)
	when we could output:
	LITERAL a,bcdefghijklmnopqrs,tuvwxyz123 (9 + 2*17 bits)

	this is costing us about 1-2% compression by rough estimate.
*/

// Must be 2 or 3.
#define min_k			3

// Determines max number of bytes in a window
const int k_width = 4;
const int max_k	= (min_k + (1<<k_width) - 1);

// Max window distance backward
// We hobble this slightly to conserve stack space.  This code is used
// primarily by networking, so it's not going to benefit from a large window.
// Could be 16 instead of 14 for a 4k lookback.
// const unsigned comp_slide_size = 1 << (14 - k_width);

// int ignore_mismatch;

const int HEADER_SIZE = 2;

#if !__SPU

// PURPOSE:	This is used within datCompress().
struct heap_entry {
	u16 follow;
	s16 next;
	const u8 *rest;
};

// PURPOSE:	This is used within datCompress().
struct queue_entry {
	s16 oldest, newest;
};

u32 datCompress(u8 *outbase,const u32 sizeofDest, const u8 *data,const u32 sizeofSrc,const u32 comp_slide_size) {
	if (!sizeofSrc)
		return 0;
	if (comp_slide_size != 512 && comp_slide_size != 1024 && comp_slide_size != 2048 && comp_slide_size != 4096)
		return 0;

    const u8* eob = outbase + sizeofDest;

	unsigned i = 0;
	heap_entry *heap = Alloca(heap_entry,comp_slide_size);		// 8 bytes times max lookback
	queue_entry queue[256];					// 1k total
	s16 freePtr = 0;

	unsigned here, follow;

	u8 *output = outbase + HEADER_SIZE;

	int control = 1;
	u8 *controlPtr = output++;

	/* set all Oldest ptrs to -1 */
	memset(queue,-1,sizeof(queue));
	
	here = data[0];
#if min_k == 3
	follow = data[1] | (data[2] << 8);
#else
	follow = data[1];
#endif

	int length;

	while (i < sizeofSrc && output < eob) {
		ptrdiff_t best_k = 1, best_j = 0;
		int search = queue[here].oldest;

		if (i < sizeofSrc - 2) while (search != -1) {
			/* definitely compressible! */
			if (follow == heap[search].follow) {
				int k = min_k;
				while (i + k < sizeofSrc && k < max_k &&
					data[i + k] == heap[search].rest[k])
					k++;
				/* if this is as good, but closer, use it
					instead, hoping that cache is
					fresher */
				if (k > best_k) {
					best_k = k;
					best_j = heap[search].rest - data;
				}
			}
			search = heap[search].next;
		}

		control <<= 1;			// make room for incoming control bit

		if (best_k != 1) {
			ptrdiff_t offset = i - best_j - 1;
			ptrdiff_t count = best_k - min_k;
			// Assert(count >= 0 && count <= 15);
			// Assert(offset >= 0 && offset <= 4095);
            if(output+2 >= eob){goto failed;}
			*output++ = (u8) (count | (offset << k_width));
			*output++ = (u8) (offset >> (8-k_width));
			// fprintf(log,"[WINDOW %d,%d]\n",offset,count);
		}
		else {
			control |= 1;
            if(output+1 >= eob){goto failed;}
			*output++ = (u8) here;
			// fprintf(log,"[LITERAL %d]\n",here);
		}

		if (control & 256) {	// it's full, so dump it
			*controlPtr = (u8) control;
            if(output+1 >= eob){goto failed;}
			controlPtr = output++;
			control = 1;
		}

		/* update heap for however many we compressed */
		do {
			if (i >= comp_slide_size) {
				s16 *oldest = 
					&queue[data[i - comp_slide_size]].oldest;
				Assert(*oldest == freePtr);
				*oldest = heap[*oldest].next;
			}
			if (queue[here].oldest == -1) {
				/* nothing in its queue, 
					no chance of compression */
				queue[here].oldest = queue[here].newest =
					freePtr;
			}
			else {
				/* link old Newest to this new entry */
				heap[queue[here].newest].next = freePtr;
				/* update Newest to here */
				queue[here].newest = freePtr;
			}
			heap[freePtr].follow = (unsigned short) follow;
			heap[freePtr].next = -1;
			heap[freePtr].rest = data + i;
			freePtr = (freePtr + 1) & u16(comp_slide_size - 1);
			i++;
#if min_k == 3
			here = follow & 255;
			follow >>= 8;
			if (i + 2 < sizeofSrc)
				follow |= (data[i+2] << 8);
#else
			here = follow;
			if (i + 1 < sizeofSrc)
				follow = data[i+1];
			else
				follow = 0;
#endif
		} while (--best_k);
	}

	// make sure last control byte is flushed
	while (!(control & 256))
		control <<= 1;
	*controlPtr = (u8) control;

	length = ptrdiff_t_to_int(output - outbase - HEADER_SIZE);

	// Encode length at start of compressed stream
	Assert(length <= 65535);
	if (length > 65535) {
		Errorf("Failed to create valid compressed data, produced size is %dby (%dKb). Maximum compressed output size is capped at 64k.", length, length >> 10);
		goto failed;
	}

	outbase[0] = (u8) length;
	outbase[1] = (u8) (length >> 8);

	return (u32) (length + HEADER_SIZE);

failed:

    return 0;
}

u32 datCompressUpperBound( u32 size )
{
	return ( size + (size >> 2) ) + HEADER_SIZE;
}

#endif

// #include <stdio.h>
// extern FILE *log;

#if __SPU && 0
#define decomp_error(x) spu_printf x
#else
#define decomp_error(x) 
#endif

u32 datDecompress(u8 *dest,const u32 destSize,const u8 *src,const u32 srcSize) {
	const u8 * const base = dest;
	const u8 * const srcStop = src + srcSize;
	const u8 * const destStop = dest + destSize;

	u32 srcLength = *src++;
	srcLength += (*src++ << 8);
	if (srcLength + HEADER_SIZE != srcSize)	{	// invalid data (header doesn't match inputs)
		decomp_error(("invalid header %d + %d != %d\n",srcLength,HEADER_SIZE,srcSize));
		return 0;
	}

	while (src < srcStop) {
		u8 control = *src++;
		for (int i=0; src < srcStop && i<8; i++, control <<= 1) {
			if (control & 0x80) {
				// fprintf(log,"[LITERAL %d]\n",*src);
				if (dest >= destStop) {
					decomp_error(("dest >= destStop\n"));
					return 0;	// invalid data (would write past end of dest)
				}
				if (src >= srcStop) {
					decomp_error(("src >= srcStop\n"));
					return 0;	// invalid data (would read past valid input)
				}
				*dest++ = *src++;
			}
			else {
				if (src + 1 >= srcStop) {
					decomp_error(("would read past src\n"));
					return 0;	// invalid data (would read past src)
				}
				u8 count = src[0];
				int offset = (src[1] << 4) | (count >> 4);
				// fprintf(log,"[WINDOW %d,%d]\n",offset,count & 15);
				const u8 *ptr = dest - offset - 1;
				if (ptr < base) {
					decomp_error(("would read before dest buffer\n"));
					return 0;	// invalid data (would read before dest buffer)
				}
				src += 2;
				count = (u8) (count & 15) + min_k;	// must be positive, so loop below will always execute once
				if (dest + count > destStop) {
					decomp_error(("would copy past end of dest\n"));
					return 0;  // invalid data (would copy past end of dest)
				}
				do {
					*dest++ = *ptr++;
				} while (--count);
			}
		}
	}

	Assert(src == srcStop);
	Assert(dest <= destStop);

	return (u32)(dest - base);
}

// HACK 
// Colin Hughes rolled some optimised versions of datDecompressInPlace for GTA4. The first implementation is the inline asm 
// version below. However, he also created datdecompressinplace.spuasm for even more speed.
// 
// Using the spuasm version requires some hackery:
//
// 1. Inside the job file before anything else:
//		#define ASMSPUDECOMPRESS 
//
// 2. Add the following to the spu_compile_flags.bat:
//		spu-lv2-gcc -c -xassembler-with-cpp ../../../../rage/base/src/data/datdecompressinplace.spuasm -o datdecompressinplace_spu.obj
//		set OBJECTS=datdecompressinplace_spu.obj
//
// 3. Instead of including this file, extern declare the datCompressInPlace function:
//		#ifdef ASMSPUDECOMPRESS
//			extern "C" u32 datDecompressInPlace(u8 *dest,const u32 destSize,const u32 offsetOfSrc,const u32 srcSize);
//		#else
//			#include "data/datcompress.cpp"
//		#endif
//  
// HACK


#if __SPU && !defined(ASMSPUDECOMPRESS) && 0 // KS - TODO

// Try to get assembly for SPE version
u32 datDecompressInPlace(u8 *dest,const u32 destSize,const u32 offsetOfSrc,const u32 srcSize) {

	qword t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;

	asm( 
	"a %[src],%[src],%[dest]					;	fsmbi %[last],0xffff						\n"		// Init src pointer and byte mak
	"ilh %[temp],0x303							;	cdd %[fetch],0($1)							\n"		// Splat byte and 000102030405060718191a1b1c1d1e1f
	"ai %[src],%[src],2							;	fsmbi %[byte],1								\n"		// Skip header, -1 constant
	"ai %[size],%[size],-2						;	rotqbyi %[dst],%[dest],0					\n"		// Adjust size, init write ptr
	"sfi %[temp1],%[dest],0						;	shufb %[sbyte],%[src],%[src],%[temp]		\n"		// Mask rotate amount, splat src address
	"andbi %[fetch],%[fetch],15					;	lqx %[dqw],%[dest],%[last]					\n"		// 000102030405060708090a0b0c0d0e0f, prefetch dummy dqw
	"rotqby %[byte],%[byte],%[temp1]															\n"		// Rotate mask byte 
	"ahi %[fetch],%[fetch],0x101																\n"		// 0102030405060708090a0b0c0d0e0f10 constant for param fetch
	"andbi %[sbyte],%[sbyte],15																	\n"		// Source byte in QW 
	

/*
	Fetch mode control byte, and 1 QW of parameter data
	Prerotate parameter data to match alignment of destination data
*/

	".p2align 3	\n"

	"__control:	\n"		

	"a %[temp],%[sbyte],%[fetch]				;	lqd %[param],0(%[src])						\n"		// Generate QW parameter fetch from src
	"sfi %[temp1],%[dst],0						;	lqd %[param1],16(%[src])					\n"		// negate dst to prepare insert patterns
	"ila %[mode],0xc080							;	hbrr __litbranch,__literal					\n"		// Prefetch hint
	"ai %[size],%[size],-1						;	rotqby %[temp],%[temp],%[temp1]				\n"		// Rotate fetch to align with dest
	"a %[mode],%[sbyte],%[mode]					;												\n"		// Prepare mode and constant 
	"											;	brz %[size],__finish						\n"		// Just in case!
	"ahi %[sbyte],%[sbyte],0x101				;	shufb %[mode],%[param],%[param],%[mode]		\n"		// Get mode in word as MM00FF00
	"ai %[src],%[src],1							;	shufb %[param],%[param],%[param1],%[temp]	\n"		// Get QW of params from src after mode byte
	
/*
	Literal loop
	Predicted, with exits for termination, new mode byte, and block mode
*/
	".p2align 3	\n"
	
	"__literal:	\n"

	"cgti %[temp],%[mode],-1					;	stqx %[dqw],%[dst],%[last]					\n"		// Check mode, and store previous dest QW
	"a %[mode],%[mode],%[mode]					;	lqd %[dqw],0(%[dst])						\n"		// Get dest QW
	"ahi %[sbyte],%[sbyte],0x101				;	brnz %[temp],__block						\n"		// switch to block mode maybe?
	"ai %[dst],%[dst],1							;	rotqbyi %[byte],%[byte],-1					\n"		// Rotate mask QW to match dst write byte
	"ai %[size],%[size],-1						;												\n"
	"andbi %[sbyte],%[sbyte],15					;												\n"		// Ensure that src byte offset is always valid
	"ai %[src],%[src],1							;	brz %[size],__finishlit						\n"		// Exit 
	"selb %[dqw],%[dqw],%[param],%[byte]		;	brhz %[mode],__control						\n"		// Move byte from param to destQW - exit to control fetch if needed
	"__litbranch: \n"
	"											br __literal									\n"		// Predicted loop..
	
/* 
	Block loop
	Copy data byte by byte from window offset
*/
	
	"__block:	\n"

	"											;	rotqby %[temp],%[param],%[dst]				\n"		// Get 2 parameter bytes as 0,1
	"ai %[src],%[src],2							;	hbrr __bytebranch,__byte					\n"		// Remove parameter bytes from literal stream
	"ahi %[sbyte],%[sbyte],0x101				;	rotqbyi %[byte],%[byte],-1					\n"		// Not pipelined same way
	"ai %[param1],%[dst],-1						;	rotqbyi %[param],%[param],2					\n"		// Remove block control from param stream
	"rotmi %[ptr],%[temp],-28					;	rotqmbyi %[count],%[temp],-3				\n"		// Get low 4 bits of window offset, and also get count
	"rotmi %[temp1],%[temp],-12					;	lqx %[dqw],%[dst],%[last]					\n"		// Get high 8 bits of window offset
	"ila %[temp],0xff0							;												\n"		
	"andbi %[sbyte],%[sbyte],15					;												\n"
	"andi %[count],%[count],15					;												\n"		// Count
	"selb %[ptr],%[ptr],%[temp1],%[temp]		;												\n"		// Offset
	"ilh %[temp1],0x303							;												\n"		
	"sf %[ptr],%[ptr],%[param1]					;												\n"		// Ptr
	"ai %[count],%[count],3						;	shufb %[temp1],%[ptr],%[ptr],%[temp1]		\n"		// Splat bytes of ptr
	

/*
	Byte copy...
*/

	".p2align 3 \n"
	"__byte:	\n"

	"ai %[ptr],%[ptr],1							;	stqx %[dqw],%[dst],%[last]					\n"	// Adjust ptr, and store prev	
	"ahi %[fetch],%[fetch],-0x101				;	lqd %[dqw],0(%[dst])						\n"	// fetch=10..1f and fetch dqw
	"ai %[dst],%[dst],1							;	lqx %[param1],%[ptr],%[last]				\n"	// Adjust dst, and fetch ptr qw
	"orbi %[fetch],%[fetch],0x10				;	rotqbyi %[param],%[param],-1				\n"	// 2nd QW constant, rotate params ( to avoid recalculating alignment later)
	"andbi %[temp1],%[temp1],15					;												\n"	// byte in ptr qw
	"ilh %[temp],0x303							;												\n" // preload byte splat constant
	"selb %[temp1],%[fetch],%[temp1],%[byte]	;	lnop										\n" // Prepare shuffle mask to insert unaligned byte
	"andbi %[fetch],%[fetch],15					;	rotqbyi %[byte],%[byte],-1					\n" // restore old fetch constant and rotate byte mask
	"ai %[count],%[count],-1					;	shufb %[dqw],%[param1],%[dqw],%[temp1]		\n" // Adjust count and insert ptr byte into dqw
	"ahi %[fetch],%[fetch],0x101				;	shufb %[temp1],%[ptr],%[ptr],%[temp]		\n" // finish constant restore and splat ptr byte for next loop
	"clgti %[temp],%[size],2\n"																		// Check for finish condition in idle time
	"__bytebranch:	\n"
	"											brnz %[count],__byte				\n"


/*
	End loop, and decide whats next :)

	We need to refresh param, and update src / sbyte -	
*/

	"ai %[size],%[size],-2						;	rotqbyi %[byte],%[byte],1			\n"
	"												brz %[temp],__finish				\n"
	"												brhz %[mode],__control				\n"
	"												hbrr __litbranch2,__literal2		\n"

/*
	back to literals, copy code here to reduce branch hit..
*/

	".p2align 3	\n"
	
	"__literal2:	\n"

	"cgti %[temp],%[mode],-1					;	stqx %[dqw],%[dst],%[last]					\n"		// Check mode, and store previous dest QW
	"a %[mode],%[mode],%[mode]					;	lqd %[dqw],0(%[dst])						\n"		// Get dest QW
	"ahi %[sbyte],%[sbyte],0x101				;	brnz %[temp],__block						\n"		// switch to block mode maybe?
	"ai %[dst],%[dst],1							;	rotqbyi %[byte],%[byte],-1					\n"		// Rotate mask QW to match dst write byte
	"ai %[size],%[size],-1						;												\n"
	"andbi %[sbyte],%[sbyte],15					;												\n"		// Ensure that src byte offset is always valid
	"ai %[src],%[src],1							;	brz %[size],__finishlit						\n"		// Exit 
	"selb %[dqw],%[dqw],%[param],%[byte]		;	brhz %[mode],__control						\n"		// Move byte from param to destQW - exit to control fetch if needed
	"__litbranch2: \n"
	"											br __literal2									\n"		// Predicted loop..




/*
	All finished... return size
*/

"__finishlit:	\n"
	"selb %[dqw],%[dqw],%[param],%[byte]														\n"		// Move byte from param to destQW
	
"__finish:	\n"
	"sf %[dest],%[dest],%[dst]			\n"													// Size is current write - original
	"stqx %[dqw],%[dst],%[last]				\n"													// Final write
//	"bi $0		\n"																			// Return directly to calling function

	
	:	[dest]"+r"( dest ),
		[src]"+r"(offsetOfSrc),
		[size]"+r"(srcSize ),

		[temp]"=r"(t0),
		[mode]"=r"(t1),
		[dqw]"=r"(t2),
		[byte]"=r"(t3),
		[param]"=r"(t4),
		[param1]"=r"(t5),
		[sbyte]"=r"(t6),
		[fetch]"=r"(t7),
		[dst]"=r"(t8),
		[temp1]"=r"(t9),
		[last]"=r"(t10),
		[ptr]"=r"(t11),
		[count]"=r"(t12)
		

	);

	return (u32)dest;
}

#else // __SPU && !defined(ASMSPUDECOMPRESS)

u32 datDecompressInPlace(u8 *dest,const u32 destSize,const u32 offsetOfSrc,const u32 srcSize) {
	const u8 * src = dest + offsetOfSrc;
	const u8 * const base = dest;
	const u8 * const srcStop = src + srcSize;
	const u8 * const destStop = dest + destSize;

	u32 srcLength = *src++;
	srcLength += (*src++ << 8);
	if (srcLength + HEADER_SIZE != srcSize)	{	// invalid data (header doesn't match inputs)
		decomp_error(("invalid header %d + %d != %d\n",srcLength,HEADER_SIZE,srcSize));
		return 0;
	}

	while (src < srcStop) {
		u8 control = *src++;
		for (int i=0; src < srcStop && i<8; i++, control <<= 1) {
			if (control & 0x80) {
				// fprintf(log,"[LITERAL %d]\n",*src);
				if (dest > src) {
					decomp_error(("in-place failure, need more slop"));
					return 0;
				}
				if (dest >= destStop) {
					decomp_error(("dest >= destStop\n"));
					return 0;	// invalid data (would write past end of dest)
				}
				if (src >= srcStop) {
					decomp_error(("src >= srcStop\n"));
					return 0;	// invalid data (would read past valid input)
				}
				*dest++ = *src++;
			}
			else {
				if (src + 1 >= srcStop) {
					decomp_error(("would read past src\n"));
					return 0;	// invalid data (would read past src)
				}
				u8 count = src[0];
				int offset = (src[1] << 4) | (count >> 4);
				// fprintf(log,"[WINDOW %d,%d]\n",offset,count & 15);
				const u8 *ptr = dest - offset - 1;
				if (ptr < base) {
					decomp_error(("would read before dest buffer\n"));
					return 0;	// invalid data (would read before dest buffer)
				}
				src += 2;
				count = (u8) (count & 15) + min_k;	// must be positive, so loop below will always execute once
				if (dest + count > destStop) {
					decomp_error(("would copy past end of dest\n"));
					return 0;  // invalid data (would copy past end of dest)
				}
				do {
					*dest++ = *ptr++;
				} while (--count);
				if (dest > src) {
					decomp_error(("in-place failure, need more slop"));
					return 0;
				}
			}
		}
	}

	Assert(src == srcStop);
	Assert(dest <= destStop);

	return (u32)(dest - base);
}

#endif // __SPU && !defined(ASMSPUDECOMPRESS)

}	// namespace rage
