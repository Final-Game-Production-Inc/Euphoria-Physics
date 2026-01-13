// 
// grcore/image_png.cpp 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#include "image.h"

#include "file/asset.h"
#include "zlib/zlib.h"
#include "system/ipc.h"

namespace rage {

#if __BE
static inline void WriteNetInt(fiStream *S,int i)
{
	S->Write(&i,4);
}
#else
static void WriteNetInt(fiStream *S,int i)
{
	S->PutCh(u8(i >> 24));
	S->PutCh(u8(i >> 16));
	S->PutCh(u8(i >> 8));
	S->PutCh(u8(i >> 0));
}
#endif

static void WriteChunk(fiStream *S,const char *tag,const void *data, int dataSize) {
	WriteNetInt(S,dataSize);
	S->Write(tag,4);
	uLong crc = crc32(0,(Bytef*)tag,4);
	if (data && dataSize) {
		crc = crc32(crc,(Bytef*)data,dataSize);
		S->Write(data,dataSize);
	}
	WriteNetInt(S,crc);
}

inline int png_abs(int a) { return a<0?-a:a; }

static fiStream *writerStream;
static void *s_writeAddr;
static uInt s_writeLength;
static sysIpcEvent s_writeReady,	// worker to parent
	s_beginWrite;					// parent to worker
static void file_writer(void *)
{
	for (;;) {
		sysIpcSetEvent(s_writeReady);
		sysIpcWaitEvent(s_beginWrite);
		writerStream->Write(s_writeAddr,s_writeLength);
	}
}

// http://www.w3.org/TR/PNG/
bool grcImage::WritePNG(const char *filename, void (*copyscan)(u8*,void*,int,int,int,u8*),int width,int height,void *base,int stride,float gamma) {
	writerStream = ASSET.Create(filename,"png");
	if (!writerStream)
		return false;

	// Write the PNG file header
	static u8 header[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	writerStream->Write(header,sizeof(header));

	// Write the PNG image header chunk
	// Note: All multi-byte integers are in network (big-endian) byte order.
	// Each chunk is of the following form:
	// LENGTH TYPE [payload] CRC
	// LENGTH is a u32 in network byte order which contains the length of the payload (possibly zero if not present)
	// TYPE is a FOURCC code identifying the chunk type.  There are lots of rules on this, but we don't care because
	// we only need three chunks to define a minimal valid PNG file.
	// CRC is a crc code of both the TYPE file and any payload.  It's defined to use the same CRC method zlib uses, conveniently.
	struct IHDR { u8 width[4], height[4], depth, colorType, compressionMethod, filterMethod, interlaceMethod; } ihdr;
	memset(&ihdr,0,sizeof(ihdr));
	ihdr.width[2] = u8(width>>8);
	ihdr.width[3] = u8(width);
	ihdr.height[2] = u8(height>>8);
	ihdr.height[3] = u8(height);
	ihdr.depth = 8;
	ihdr.colorType = 2;
	WriteChunk(writerStream,"IHDR",&ihdr,sizeof(ihdr));

	u8 gammalut[256];
	if (gamma) {
		// s32 igamma = sysEndian::NtoB(s32(gamma / 100000.0f));
		// WriteChunk(S,"gAMA",&igamma,4);
		gamma *= 0.45454545454545454545454545454545f;	// normalize to 2.2
		for (int i=0; i<256; i++) {
			float f = powf((float)i,gamma);
			if (f < 0) gammalut[i] = 0;
			else if (f > 255) gammalut[i] = 255;
			else gammalut[i] = u8(f);
		}
	}
	else
		for (int i=0; i<256; i++)
			gammalut[i] = u8(i);
	
	// Prepare the IDAT (image data) chunk.  This data is conditioned by filters (a different one on each scanline) and
	// then run through zlib's deflate algorithm.  We don't know the size ahead of time so reserve space for it now
	// and fill it in after finishing everything else.
	int idatStart = writerStream->Tell();
	int idatLength = 0;
	WriteNetInt(writerStream,0);
	writerStream->Write("IDAT",4);
	uLong crc = crc32(0,(Bytef*)"IDAT",4);

	// Prepare compressor using default libpng settings
	z_stream zs;
	memset(&zs,0,sizeof(zs));
	if (deflateInit2(&zs,Z_BEST_SPEED,Z_DEFLATED,15,8,Z_RLE) < 0) {
		Errorf("Error in deflateInit2 saving %s",filename);
		writerStream->Close();
		return false;
	}

	// Allocate scanline buffers.  We copy scanlines in from the source using a callback
	// because the memory we're reading may be very slow on some platforms (PS3 VRAM)
	// http://www.w3.org/TR/PNG/#9
	const uInt outSize = 16384;
	uInt outBufferOffset = 0;
	int w3 = (width+1)*3;

#define PAETH_ONLY	1

	u8 *outBuffer = Alloca(u8, outSize + outSize + 5 * w3);
	u8 *prevScan = outBuffer + outSize + outSize, *none = prevScan + w3, *sub = none + w3,
		/**up = sub + w3, */ *average = sub + w3, *paeth = average + w3;
	memset(prevScan,0,w3+3);	// previous scanline assumed to be initially all zero (and leftmost hidden pixel on none0, and its filter type)
	// Initialize the other four filter types (we never look at the first hidden pixel in these arrays)
	sub[2] = 1; /*up[2] = 2;*/ average[2] = 3; paeth[2] = 4;

	// Prepare compression output buffer
	zs.next_out = outBuffer;
	zs.avail_out = outSize;

	/* static bool crippleFilters;
	crippleFilters = !crippleFilters; */

	bool didSomething;

	static sysIpcThreadId writer;
	if (!writer) {
		s_writeReady = sysIpcCreateEvent();
		s_beginWrite = sysIpcCreateEvent();
		writer = sysIpcCreateThread(file_writer,NULL,sysIpcMinThreadStackSize,PRIO_NORMAL,"png_writer");
	}
	else	// worker is already waiting on s_beginWrite by now, so unclog ourselves.
		sysIpcSetEvent(s_writeReady);
	int row = 0;
	do {
		didSomething = false;
		// Make sure input is available
		if (zs.avail_in == 0 && height) {
			copyscan(none+3,base,row,width,stride,gammalut);
			++row;

			// http://www.w3.org/TR/PNG/#12Filter-selection
			// Note that in the code below, we rely on a filter type of 0 being none
			// to avoid having to rewrite the filter value while still being able
			// to access the very leftmost "previous" pixel.
			// In testing neither sub nor up perform particularly well, so in order
			// to get the number of comparisons down we'll skip 'up'
#if !PAETH_ONLY
			int noneSum = 0, subSum = 0, /*upSum = 0,*/ averageSum = 0, paethSum = 0;
			for (int i3=3; i3<w3; i3+=3) {
				for (int i=0; i<3; i++) {
					u8 x = none[i3+i];
					u8 a = none[i3+i-3];
					u8 b = prevScan[i3+i];
					u8 c = prevScan[i3+i-3];
					noneSum += png_abs(s8(x));
					subSum += png_abs(s8(sub[i3+i] = u8(x - a)));
					// upSum += png_abs(s8(up[i3+i] = u8(x - b)));
					averageSum += png_abs(s8(average[i3+i] = u8(x - ((a + b) >> 1))));
					int p = a + b - c;
					int pa = png_abs(p-a);
					int pb = png_abs(p-b);
					int pc = png_abs(p-c);
					u8 Pr =(pa<=pb && pa<=pc)? a : (pb<=pc)? b : c;
					paethSum += png_abs(s8(paeth[i3+i] = u8(x - Pr)));
				}
			}

			// Do three total comparisons in binary tree to find best sum
			u8 *best01p, *best34p;
			int best01, best34;
			if (noneSum <= subSum)
				best01p = none, best01 = noneSum;
			else
				best01p = sub, best01 = subSum;
			if (averageSum <= paethSum)
				best34p = average, best34 = averageSum;
			else
				best34p = paeth, best34 = paethSum;
			zs.next_in = (best01<=best34)? best01p + 2: best34p + 2;
#else
			for (int i3=3; i3<w3; i3+=3) {
				u8 x0 = none[i3+0], x1 = none[i3+1], x2 = none[i3+2];
				u8 a0 = none[i3+0-3], a1 = none[i3+1-3], a2 = none[i3+2-3];
				u8 b0 = prevScan[i3+0], b1 = prevScan[i3+1], b2 = prevScan[i3+2];
				u8 c0 = prevScan[i3+0-3], c1 = prevScan[i3+1-3], c2 = prevScan[i3+2-3];
				int p0 = a0 + b0 - c0;
				int p1 = a1 + b1 - c1;
				int p2 = a2 + b2 - c2;
				int pa0 = png_abs(p0-a0);
				int pa1 = png_abs(p1-a1);
				int pa2 = png_abs(p2-a2);
				int pb0 = png_abs(p0-b0);
				int pb1 = png_abs(p1-b1);
				int pb2 = png_abs(p2-b2);
				int pc0 = png_abs(p0-c0);
				int pc1= png_abs(p1-c1);
				int pc2 = png_abs(p2-c2);
				u8 Pr0 =(pa0<=pb0 && pa0<=pc0)? a0 : (pb0<=pc0)? b0 : c0;
				u8 Pr1 =(pa1<=pb1 && pa1<=pc1)? a1 : (pb1<=pc1)? b1 : c1;
				u8 Pr2 =(pa2<=pb2 && pa2<=pc2)? a2 : (pb2<=pc2)? b2 : c2;
				paeth[i3+0] = u8(x0 - Pr0);
				paeth[i3+1] = u8(x1 - Pr1);
				paeth[i3+2] = u8(x2 - Pr2);
			}
			zs.next_in = paeth + 2;
#endif
			zs.avail_in = w3 - 2;
			height--;
			didSomething = true;

			// Bounce prevScan and none
			u8 *swap = prevScan;
			prevScan = none;
			none = swap;
		}
		// Run compressor
		if (deflate(&zs,height? Z_NO_FLUSH : Z_FINISH) < 0) {
			Errorf("Error in deflate saving %s",filename);
			sysIpcWaitEvent(s_writeReady);
			writerStream->Close();
			return false;
		}
		// Dump any output immediately
		if (zs.avail_out != outSize) {
			int thisSize = outSize - zs.avail_out;
			sysIpcWaitEvent(s_writeReady);
			s_writeAddr = outBuffer + outBufferOffset;
			s_writeLength = thisSize;
			sysIpcSetEvent(s_beginWrite);
			crc = crc32(crc,outBuffer + outBufferOffset,thisSize);
			idatLength += thisSize;
			outBufferOffset ^= outSize;
			zs.next_out = outBuffer + outBufferOffset;
			zs.avail_out = outSize;
			didSomething = true;
		}
	} while (didSomething);
	sysIpcWaitEvent(s_writeReady);

	// Tear down compressor
	deflateEnd(&zs);

	// Write final IDAT CRC
	WriteNetInt(writerStream,crc);

	// Write the end marker chunk.  No payload, but CRC still needs to be valid.
	WriteChunk(writerStream,"IEND",NULL,0);

	// Go back and fix the length of the IDAT chunk.
	writerStream->Seek(idatStart);
	WriteNetInt(writerStream,idatLength);

	writerStream->Close();

	return true;
}

static void copyscan_image(u8 *dest,void *base,int row,int width,int stride,u8 *gammalut) {
	u32 *src = (u32*)((char*)base + row * stride);
	while (width--) {
		u32 n = *src++;
		dest[0] = gammalut[u8(n >> 16)];
		dest[1] = gammalut[u8(n >> 8)];
		dest[2] = gammalut[u8(n)];
		dest += 3;
	}
}

bool grcImage::SavePNG(const char *filename,float gamma) {
	return WritePNG(filename,copyscan_image,m_Width,m_Height,m_Bits,GetStride(),gamma);
}


}