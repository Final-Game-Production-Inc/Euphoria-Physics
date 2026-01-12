// 
// data/compress.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_COMPRESS_H
#define DATA_COMPRESS_H

namespace rage {

/*
	PURPOSE: Compress a chunk of memory
	PARAMS: dest - output buffer (could be about 12% larger than src
		in the absolute worst case)
        sizeofDest  - number of bytes available in the dest buffer.
		src         - source buffer
		sizeofSrc   - number of bytes to compress
		comp_slide_size - controls amount of lookback we support, which affects
			both compression time and stack usage.  Possible values are:
			512 (5k stack), 1024 (9k stack), 2048 (17k stack), or 4096 (33k stack)
	RETURNS: number of bytes in output buffer, or zero on failure.
	NOTES: If gzip compressed data to about 50% of its original size,
		this method will compress it to about 65% of its original size.
		Only uses a 4k lookback, so it's designed for smaller datasets
		than the fiCompress code.  It also doesn't use the heap either.
		Maximum compressed output size is capped at 64k.
*/
u32 datCompress(u8 *dest,const u32 sizeofDest,const u8 *src,const u32 sizeofSrc,const u32 comp_slide_size = 1024);

/*
	PURPOSE: Returns the maximum amount of space required to compress
		a set of data of the given size
	PARAMS: size - number of bytes
	RETURNS: upper bound
	NOTES: Despite the comments above about the upper bound being around
		112% of size, we'll pad it up to 125%.
*/
u32 datCompressUpperBound( u32 size );

/*
	PURPOSE: Decompress a chunk of memory compressed with datCompress
	PARAMS: dest - output buffer
		sizeofDest  - number of bytes available in destination buffer.
		src         - source buffer containing compressed data
		sizeofSrc - number of bytes in source buffer.  This should be the
			return value of 
	RETURNS: number of bytes stored in output buffer
	NOTES: In-place decompression will generally not work even if you
		move the input buffer as high as possible within the output
		buffer.  Imagine a test case with very high redundancy early
		in the file but then no redundancy at the end; the output will
		overrun the input since it takes up to nine bits of input to
		produce eight bits of output.
*/
u32 datDecompress(u8 *dest,const u32 sizeofDest,const u8 *src,const u32 sizeOfSrc);

/*
	PURPOSE: Decompress a chunk of memory compressed with datCompress
	PARAMS: dest - output buffer
		sizeofDest  - number of bytes available in destination buffer.
		offsetOfSrc - offset into dest buffer of source buffer containing compressed data
		sizeofSrc - number of bytes in source buffer.  This should be the
			return value of 
	RETURNS: number of bytes stored in output buffer, or zero on failure
		(likely due to src overrunning dest)
*/
u32 datDecompressInPlace(u8 *dest,const u32 sizeofDest,const u32 offsetOfSrc,const u32 sizeOfSrc);

}	// namespace rage

#endif
