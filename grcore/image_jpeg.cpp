// 
// grcore/image_jpeg.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/image.h"

#include "file/asset.h"
#include "system/alloca.h"

#include "jpeg/cdjpeg.h"

#include "grcore/texture.h"
#if __XENON
#include "grcore/texturexenon.h"
#elif __PS3
#include "grcore/texturegcm.h"
#elif RSG_ORBIS
#include <grcore/texture_gnm.h>
#elif RSG_DURANGO
#include "grcore/texture_durango.h"
#elif RSG_PC
#include "grcore/resourcecache.h"
#endif 

#include <setjmp.h>

#if __XENON
#include "system/xtl.h"
#define DBG 0
#include "xgraphics.h"
#undef DBG
#endif

typedef struct {
	struct jpeg_source_mgr pub;	/* public fields */

	rage::fiStream * infile;		/* source stream */
	JOCTET * buffer;		/* start of buffer */
	boolean start_of_file;	/* have we gotten any data yet? */
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

#define INPUT_BUF_SIZE  4096	/* choose an efficiently fread'able size */


/*
* Initialize source --- called by jpeg_read_header
* before any data is actually read.
*/

METHODDEF(void)
init_source (j_decompress_ptr cinfo)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;

	/* We reset the empty-input-file flag for each image,
	* but we don't clear the input buffer.
	* This is correct behavior for reading a series of images from one source.
	*/
	src->start_of_file = TRUE;
}


/*
* Fill the input buffer --- called whenever buffer is emptied.
*
* In typical applications, this should read fresh data into the buffer
* (ignoring the current state of next_input_byte & bytes_in_buffer),
* reset the pointer & count to the start of the buffer, and return TRUE
* indicating that the buffer has been reloaded.  It is not necessary to
* fill the buffer entirely, only to obtain at least one more byte.
*
* There is no such thing as an EOF return.  If the end of the file has been
* reached, the routine has a choice of ERREXIT() or inserting fake data into
* the buffer.  In most cases, generating a warning message and inserting a
* fake EOI marker is the best course of action --- this will allow the
* decompressor to output however much of the image is there.  However,
* the resulting error message is misleading if the real problem is an empty
* input file, so we handle that case specially.
*
* In applications that need to be able to suspend compression due to input
* not being available yet, a FALSE return indicates that no more data can be
* obtained right now, but more may be forthcoming later.  In this situation,
* the decompressor will return to its caller (with an indication of the
* number of scanlines it has read, if any).  The application should resume
* decompression after it has loaded more data into the input buffer.  Note
* that there are substantial restrictions on the use of suspension --- see
* the documentation.
*
* When suspending, the decompressor will back up to a convenient restart point
* (typically the start of the current MCU). next_input_byte & bytes_in_buffer
* indicate where the restart point will be if the current call returns FALSE.
* Data beyond this point must be rescanned after resumption, so move it to
* the front of the buffer rather than discarding it.
*/

METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;
	size_t nbytes;

	nbytes = src->infile->Read(src->buffer, INPUT_BUF_SIZE);

	if (nbytes <= 0) {
		if (src->start_of_file)	/* Treat empty input file as fatal error */
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		WARNMS(cinfo, JWRN_JPEG_EOF);
		/* Insert a fake EOI marker */
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = FALSE;

	return TRUE;
}


/*
* Skip data --- used to skip over a potentially large amount of
* uninteresting data (such as an APPn marker).
*
* Writers of suspendable-input applications must note that skip_input_data
* is not granted the right to give a suspension return.  If the skip extends
* beyond the data currently in the buffer, the buffer can be marked empty so
* that the next read will cause a fill_input_buffer call that can suspend.
* Arranging for additional bytes to be discarded before reloading the input
* buffer is the application writer's problem.
*/

METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;

	/* Just a dumb implementation for now.  Could use fseek() except
	* it doesn't work on pipes.  Not clear that being smart is worth
	* any trouble anyway --- large skips are infrequent.
	*/
	if (num_bytes > 0) {
		while (num_bytes > (long) src->pub.bytes_in_buffer) {
			num_bytes -= (long) src->pub.bytes_in_buffer;
			(void) fill_input_buffer(cinfo);
			/* note we assume that fill_input_buffer will never return FALSE,
			* so suspension need not be handled.
			*/
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}


/*
* An additional method that can be provided by data source modules is the
* resync_to_restart method for error recovery in the presence of RST markers.
* For the moment, this source module just uses the default resync method
* provided by the JPEG library.  That method assumes that no backtracking
* is possible.
*/


/*
* Terminate source --- called by jpeg_finish_decompress
* after all data has been read.  Often a no-op.
*
* NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
* application must deal with any cleanup that should happen even
* for error exit.
*/

METHODDEF(void)
term_source (j_decompress_ptr UNUSED_PARAM(cinfo))
{
	/* no work necessary here */
}

typedef struct grcimg_custom_error_mgr * grcimg_custom_error_ptr;  

struct  grcimg_custom_error_mgr 
{
	struct jpeg_error_mgr pub;
	unsigned char pad[12]; // explicit padding to avoid warning about extra padding because jmp_buf is 16-byte aligned
	jmp_buf setjmp_buffer;

};  

METHODDEF(void) grcimg_custom_error_exit (j_common_ptr cinfo)
{
	 grcimg_custom_error_ptr customerr = ( grcimg_custom_error_ptr) cinfo->err;   

	// display error
	(*cinfo->err->output_message) (cinfo);

	// jump to cleanup point
	longjmp(customerr->setjmp_buffer, 1);
}


/*
* Prepare for input from a stdio stream.
* The caller must have already opened the stream, and is responsible
* for closing it after finishing decompression.
*/

GLOBAL(void)
jpeg_fiStream_src (j_decompress_ptr cinfo, rage::fiStream * infile)
{
	my_src_ptr src;

	/* The source object and input buffer are made permanent so that a series
	* of JPEG images can be read from the same file by calling jpeg_stdio_src
	* only before the first one.  (If we discarded the buffer at the end of
	* one image, we'd likely lose the start of the next one.)
	* This makes it unsafe to use this manager and a different source
	* manager serially with the same JPEG object.  Caveat programmer.
	*/
	if (cinfo->src == NULL) {	/* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			SIZEOF(my_source_mgr));
		src = (my_src_ptr) cinfo->src;
		src->buffer = (JOCTET *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			INPUT_BUF_SIZE * SIZEOF(JOCTET));
	}

	src = (my_src_ptr) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->infile = infile;
	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}



typedef struct {
	struct jpeg_destination_mgr pub; /* public fields */

	rage::fiStream * outfile;		/* target stream */
	JOCTET * buffer;		/* start of buffer */
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;

#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */

METHODDEF(void)
init_destination (j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

	/* Allocate the output buffer --- it will be released when done with image */
	dest->buffer = (JOCTET *)
		(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
		OUTPUT_BUF_SIZE * SIZEOF(JOCTET));

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}


/*
* Empty the output buffer --- called whenever buffer fills up.
*
* In typical applications, this should write the entire output buffer
* (ignoring the current state of next_output_byte & free_in_buffer),
* reset the pointer & count to the start of the buffer, and return TRUE
* indicating that the buffer has been dumped.
*
* In applications that need to be able to suspend compression due to output
* overrun, a FALSE return indicates that the buffer cannot be emptied now.
* In this situation, the compressor will return to its caller (possibly with
* an indication that it has not accepted all the supplied scanlines).  The
* application should resume compression after it has made more room in the
* output buffer.  Note that there are substantial restrictions on the use of
* suspension --- see the documentation.
*
* When suspending, the compressor will back up to a convenient restart point
* (typically the start of the current MCU). next_output_byte & free_in_buffer
* indicate where the restart point will be if the current call returns FALSE.
* Data beyond this point will be regenerated after resumption, so do not
* write it out when emptying the buffer externally.
*/

METHODDEF(boolean)
empty_output_buffer (j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

	if (dest->outfile->Write(dest->buffer, OUTPUT_BUF_SIZE) !=
		(size_t) OUTPUT_BUF_SIZE)
		ERREXIT(cinfo, JERR_FILE_WRITE);

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

	return TRUE;
}


/*
* Terminate destination --- called by jpeg_finish_compress
* after all data has been written.  Usually needs to flush buffer.
*
* NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
* application must deal with any cleanup that should happen even
* for error exit.
*/

METHODDEF(void)
term_destination (j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;
	size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

	/* Write any data remaining in the buffer */
	if (datacount > 0) {
		if (dest->outfile->Write(dest->buffer, (int)datacount) != (int)datacount)
			ERREXIT(cinfo, JERR_FILE_WRITE);
	}
	dest->outfile->Flush();
	/* Make sure we wrote the output file OK */
	//if (ferror(dest->outfile))
	//	ERREXIT(cinfo, JERR_FILE_WRITE);
}


/*
* Prepare for output to a stdio stream.
* The caller must have already opened the stream, and is responsible
* for closing it after finishing compression.
*/

void 
jpeg_fiStream_dest (j_compress_ptr cinfo, rage::fiStream * outfile)
{
	my_dest_ptr dest;

	/* The destination object is made permanent so that multiple JPEG images
	* can be written to the same file without re-executing jpeg_stdio_dest.
	* This makes it dangerous to use this manager and a different destination
	* manager serially with the same JPEG object, because their private object
	* sizes may be different.  Caveat programmer.
	*/
	if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
		cinfo->dest = (struct jpeg_destination_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			SIZEOF(my_destination_mgr));
	}

	dest = (my_dest_ptr) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->outfile = outfile;
}


extern long jdiv_round_up (long a, long b);

//! Extracted to shared location, to remove duplication
inline void copyJpegBuffer( jpeg_decompress_struct& cInfo, unsigned char* destination, rage::u32 const width, rage::u32 const stride )
{
	unsigned char *rgb = Alloca(unsigned char, width * 3);
	unsigned char *dest = destination;

	while (cInfo.output_scanline < cInfo.output_height) 
	{
		jpeg_read_scanlines(&cInfo, &rgb, 1);
		for ( rage::u32 i = 0; i< width; ++i ) 
		{
#if RSG_ORBIS || RSG_PC || RSG_DURANGO
			rage::Color32 c(rgb[i*3+2], rgb[i*3+1], rgb[i*3+0], 255);
			*(rage::u32*)&dest[i*4] = c.GetDeviceColor();
#else

#if __BE
			dest[i*4+0] = 255;

#if __PS3
			dest[i*4+1] = rgb[i*3+2];
			dest[i*4+2] = rgb[i*3+1];
			dest[i*4+3] = rgb[i*3+0];
#else
			dest[i*4+1] = rgb[i*3+0];
			dest[i*4+2] = rgb[i*3+1];
			dest[i*4+3] = rgb[i*3+2];
#endif 

#else
			dest[i*4+3] = 255;

#if __PS3
			dest[i*4+2] = rgb[i*3+2];
			dest[i*4+1] = rgb[i*3+1];
			dest[i*4+0] = rgb[i*3+0];
#else
			dest[i*4+2] = rgb[i*3+0];
			dest[i*4+1] = rgb[i*3+1];
			dest[i*4+0] = rgb[i*3+2];
#endif

#endif

#endif
		}

		dest += stride;
	}
}

namespace rage {

//	All the other static members of grcImage are defined in image.cpp. Is that where these two should be?
grcJpegSaveComMarker grcImage::sm_SaveComMarker;
grcJpegLoadComMarker grcImage::sm_LoadComMarker;


grcImage *grcImage::LoadJPEG(const char *filename , grcImage* image,bool resize) {
   fiStream *S = ASSET.Open(filename,"jpg",true);
   return LoadJPEG(S,image,resize);
}

grcImage *grcImage::LoadJPEG(fiStream *S , grcImage* image, bool resize) {
	if (!S)
		return NULL;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	/* Initialize the JPEG decompression object with default error handling. */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_fiStream_src(&cinfo, S);
	(void) jpeg_read_header(&cinfo, TRUE);

    //create a new image if one was not given
    if (image == NULL)
    {
		image = grcImage::Create(cinfo.image_width,cinfo.image_height,1,grcImage::A8R8G8B8,grcImage::STANDARD,0,0);
    }
	else if (resize)
	{
		//re format the image if one was not given
		if (cinfo.image_width != image->GetWidth() || cinfo.image_height != image->GetHeight())
		{
			image->Resize(cinfo.image_width,cinfo.image_height);
		}		
	}

	(void) jpeg_start_decompress(&cinfo);

	copyJpegBuffer( cinfo, image->GetBits(), image->GetWidth(), image->GetStride() );

	(void) jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	S->Close();

	return image;
}



#if __PS3 && HACK_GTA4
// Gamma conversion table based on gammaCorrect = (((f<=.0031308f) ? 12.92f * f : 1.055f * pow(f,<GAMMA>/2.4f) - 0.055f)
// with GAMMA == 1.1
static unsigned char s_gamma[256] = 
{
	0x00, 0x07, 0x0F, 0x15, 0x1A, 0x1E, 0x22, 0x25, 0x29, 0x2C, 0x2E, 0x31, 0x34, 0x36, 0x39, 0x3B, 
	0x3D, 0x3F, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4B, 0x4D, 0x4E, 0x50, 0x52, 0x53, 0x55, 0x56, 0x58, 
	0x59, 0x5B, 0x5C, 0x5E, 0x5F, 0x61, 0x62, 0x63, 0x65, 0x66, 0x67, 0x68, 0x6A, 0x6B, 0x6C, 0x6D, 
	0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
	0x90, 0x91, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 
	0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA5, 0xA6, 0xA7, 0xA8, 0xA8, 0xA9, 
	0xAA, 0xAB, 0xAB, 0xAC, 0xAD, 0xAE, 0xAE, 0xAF, 0xB0, 0xB1, 0xB1, 0xB2, 0xB3, 0xB4, 0xB4, 0xB5, 
	0xB6, 0xB6, 0xB7, 0xB8, 0xB8, 0xB9, 0xBA, 0xBA, 0xBB, 0xBC, 0xBD, 0xBD, 0xBE, 0xBF, 0xBF, 0xC0,
	0xC1, 0xC1, 0xC2, 0xC2, 0xC3, 0xC4, 0xC4, 0xC5, 0xC6, 0xC6, 0xC7, 0xC8, 0xC8, 0xC9, 0xCA, 0xCA, 
	0xCB, 0xCB, 0xCC, 0xCD, 0xCD, 0xCE, 0xCE, 0xCF, 0xD0, 0xD0, 0xD1, 0xD1, 0xD2, 0xD3, 0xD3, 0xD4, 
	0xD4, 0xD5, 0xD6, 0xD6, 0xD7, 0xD7, 0xD8, 0xD9, 0xD9, 0xDA, 0xDA, 0xDB, 0xDB, 0xDC, 0xDD, 0xDD, 
	0xDE, 0xDE, 0xDF, 0xDF, 0xE0, 0xE0, 0xE1, 0xE2, 0xE2, 0xE3, 0xE3, 0xE4, 0xE4, 0xE5, 0xE5, 0xE6, 
	0xE7, 0xE7, 0xE8, 0xE8, 0xE9, 0xE9, 0xEA, 0xEA, 0xEB, 0xEB, 0xEC, 0xEC, 0xED, 0xED, 0xEE, 0xEE, 
	0xEF, 0xF0, 0xF0, 0xF1, 0xF1, 0xF2, 0xF2, 0xF3, 0xF3, 0xF4, 0xF4, 0xF5, 0xF5, 0xF6, 0xF6, 0xF7, 
	0xF7, 0xF8, 0xF8, 0xF9, 0xF9, 0xFA, 0xFA, 0xFB, 0xFB, 0xFC, 0xFC, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE
};
#endif // __PS3

bool grcImage::SaveStreamToJPEG(const char *filename,void* pBits,int width, int height, int stride,int quality) {

	fiStream *S = ASSET.Create(filename,"jpg");
	if (!S)
		return false;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	/* Initialize the JPEG decompression object with default error handling. */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_fiStream_dest(&cinfo, S);

	cinfo.in_color_space = JCS_RGB; /* arbitrary guess */
	jpeg_set_defaults(&cinfo);
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.data_precision = 8;

	jpeg_set_quality(&cinfo, quality, TRUE);

	(void) jpeg_start_compress(&cinfo,TRUE);

	unsigned char *src = (unsigned char*) pBits;
	unsigned char *rgb = Alloca(unsigned char, width * 3);

	while (cinfo.next_scanline < cinfo.image_height) {
		for (int i=0; i<width; i++) {

#if __PS3 && HACK_GTA4
#if __BE

			if (sm_gammaCorrect)
			{
				rgb[i*3+0] = s_gamma[src[i*4+1]];
				rgb[i*3+1] = s_gamma[src[i*4+2]];
				rgb[i*3+2] = s_gamma[src[i*4+3]];
			}
			else
			{
				rgb[i*3+0] = src[i*4+1];
				rgb[i*3+1] = src[i*4+2];
				rgb[i*3+2] = src[i*4+3];
			}
#else
			if (sm_gammaCorrect)
			{
				rgb[i*3+0] = s_gamma[src[i*4+2]];
				rgb[i*3+1] = s_gamma[src[i*4+1]];
				rgb[i*3+2] = s_gamma[src[i*4+0]];
			}
			else
			{
				rgb[i*3+0] = src[i*4+2];
				rgb[i*3+1] = src[i*4+1];
				rgb[i*3+2] = src[i*4+0];
			}
#endif
#else
#if __BE
			rgb[i*3+0] = src[i*4+1];
			rgb[i*3+1] = src[i*4+2];
			rgb[i*3+2] = src[i*4+3];
#else
			rgb[i*3+0] = src[i*4+2];
			rgb[i*3+1] = src[i*4+1];
			rgb[i*3+2] = src[i*4+0];
#endif
#endif
		}
		jpeg_write_scanlines(&cinfo, &rgb, 1);
		src += stride;
	}

	(void) jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	S->Close();
	return true;
}

bool grcImage::SaveJPEG(const char *filename,int quality) const {
	fiStream *S = ASSET.Create(filename,"jpg");
	if (S) {
		bool result = SaveJPEG(S,quality);
		S->Close();
		return result;
	}
	else
		return false;
}

bool grcImage::SaveJPEG(fiStream* S,int quality) const {
	if (!S)
		return false;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	/* Initialize the JPEG decompression object with default error handling. */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_fiStream_dest(&cinfo, S);

	cinfo.in_color_space = JCS_RGB; /* arbitrary guess */
	jpeg_set_defaults(&cinfo);
	cinfo.image_width = m_Width;
	cinfo.image_height = m_Height;
	cinfo.input_components = 3;
	cinfo.data_precision = 8;

	jpeg_set_quality(&cinfo, quality, TRUE);

	(void) jpeg_start_compress(&cinfo,TRUE);

	unsigned char *src = m_Bits;
	unsigned char *rgb = Alloca(unsigned char, m_Width * 3);

	while (cinfo.next_scanline < cinfo.image_height) {
		for (int i=0; i<m_Width; i++) {
#if __BE
			rgb[i*3+0] = src[i*4+1];
			rgb[i*3+1] = src[i*4+2];
			rgb[i*3+2] = src[i*4+3];
#else
			rgb[i*3+0] = src[i*4+2];
			rgb[i*3+1] = src[i*4+1];
			rgb[i*3+2] = src[i*4+0];
#endif
		}
		jpeg_write_scanlines(&cinfo, &rgb, 1);
		src += m_Stride;
	}

	(void) jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	return true;
}

#if __XENON
// TBR: tidy these functions up, temporarily copied from xgraphics.h 
// to avoid bringing in an awful lot of unnecessary code, which was also causing compile errors

//------------------------------------------------------------------------
// Calculate the log2 of a texel pitch which is less than or equal to 16
// and a power of 2.s
inline u32 XenonLog2LE16(u32 TexelPitch)
{
#if defined(_X86_) || defined(_AMD64_)
	return (TexelPitch >> 2) + ((TexelPitch >> 1) >> (TexelPitch >> 2));
#else
	return 31 - _CountLeadingZeros(TexelPitch);
#endif
}

//------------------------------------------------------------------------
// Translate the address of a surface texel/block from 2D array coordinates into
// a tiled memory offset measured in texels/blocks.
inline u32 XenonAddress2DTiledOffset(
	u32 x,             // x coordinate of the texel/block
	u32 y,             // y coordinate of the texel/block
	u32 Width,         // Width of the image in texels/blocks
	u32 TexelPitch     // Size of an image texel/block in bytes
	)
{
	u32 AlignedWidth;
	u32 LogBpp;
	u32 Macro;
	u32 Micro;
	u32 Offset;

	AlignedWidth = (Width + 31) & ~31;
	LogBpp       = XenonLog2LE16(TexelPitch);
	Macro        = ((x >> 5) + (y >> 5) * (AlignedWidth >> 5)) << (LogBpp + 7);
	Micro        = (((x & 7) + ((y & 6) << 2)) << LogBpp);
	Offset       = Macro + ((Micro & ~15) << 1) + (Micro & 15) + ((y & 8) << (3 + LogBpp)) + ((y & 1) << 4);

	return (((Offset & ~511) << 3) + ((Offset & 448) << 2) + (Offset & 63) +
		((y & 16) << 7) + (((((y & 8) >> 2) + (x >> 3)) & 3) << 6)) >> LogBpp;
}
#endif


#if __WIN32
	#define __SRC_RED		1
	#define __SRC_GREEN		2
	#define __SRC_BLUE		3
#elif __PS3
	#define __SRC_RED		3
	#define __SRC_GREEN		2
	#define __SRC_BLUE		1
#elif RSG_ORBIS
	#define __SRC_RED		2
	#define __SRC_GREEN		1
	#define __SRC_BLUE		0
#endif


static u32 grcImageGetNearestPowerOfTwo(u32 n) 
{
	u32 ret = 1;
	while (ret < n) 
	{
		ret <<= 1;
	}
	return ret;
}

static u32 grcImageJPEGGetClosestScaleDenom(u32 srcWidth, u32 srcHeight, u32& dstWidth, u32& dstHeight)
{
	u32 scaleDenom = 1;

	if (dstWidth > srcWidth || dstHeight > srcHeight) 
	{
		Assertf(0, "grcImageJPEGGetClosestScaleDenom: output dimensions w:%u h:%u cannot be greater than source dimensions w:%u h:%u", dstWidth, dstHeight, srcWidth, srcHeight);		

		dstWidth = srcWidth;
		dstHeight = srcHeight;
	}
	else
	{
		u32 widthRatio = jdiv_round_up(srcWidth, dstWidth);
		u32 heightRatio = jdiv_round_up(srcHeight, dstHeight);

		if (widthRatio != heightRatio) 
		{
			Assertf(0, "grcImageJPEGGetClosestScaleDenom: non-uniform scaling is not supported, source is w:%u h:%u, output scale is w:%u h:%u."
				" This results in a width ratio of %u and height ratio of %u", srcWidth, srcHeight, dstWidth, dstHeight, widthRatio, heightRatio );		
			
			//! Pick the best fit ratio to avoid crashing. Means we end up with some garbage images... but better than a crash!
			widthRatio = widthRatio > heightRatio ? widthRatio : heightRatio;
			heightRatio = heightRatio > widthRatio ? heightRatio : widthRatio;
		}

		// supported scale ratios are 1/1, 1/2, 1/4, 1/8
		scaleDenom = Min(grcImageGetNearestPowerOfTwo(widthRatio), 8U);

		dstWidth = jdiv_round_up(srcWidth, scaleDenom);
		dstHeight = jdiv_round_up(srcHeight, scaleDenom);
	}

	return scaleDenom;
}

u32 grcImage::GetSizesOfClosestAvailableScaleDenom(u32 srcWidth, u32 srcHeight, u32& dstWidth, u32& dstHeight, u32 &dstTextureDataSize, bool bLocalMemory)
{
	u32 scaleDenom = grcImageJPEGGetClosestScaleDenom(srcWidth, srcHeight, dstWidth, dstHeight);

	dstTextureDataSize = grcTextureFactory::GetInstance().GetTextureDataSize(dstWidth, dstHeight, A8R8G8B8, 1, 0, false, true, bLocalMemory);

	return scaleDenom;
}


int grcImage::SaveRGBA8SurfaceToJPEG(const char* filename, int quality, void* pSrc, int width, int height, int pitch)
{
	// tbr: profile this
	fiSafeStream SafeStream(ASSET.Create(filename,"jpg"));

	if (SafeStream == NULL) 
	{
		return false;
	}

	return SaveRGBA8SurfaceToJPEG(SafeStream, quality, pSrc, width, height, pitch, false);
}

int grcImage::SaveRGBA8SurfaceToJPEG(fiStream* pStream, int quality, void* pSrc, int width, int height, int pitch, bool bCloseStreamAtEnd)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	/* Initialize the JPEG decompression object with default error handling. */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_fiStream_dest(&cinfo, pStream);

	cinfo.in_color_space = JCS_RGB; /* arbitrary guess */
	jpeg_set_defaults(&cinfo);
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.data_precision = 8;

	jpeg_set_quality(&cinfo, quality, TRUE);

	(void) jpeg_start_compress(&cinfo,TRUE);

	sm_SaveComMarker.WriteComMarkerData(cinfo);

	unsigned char* pDestRGBScanline = Alloca(unsigned char, width * 3);

#if __XENON
	pitch = pitch;
	int j = 0;
#else
	const unsigned char* pSrcRGBAScanline = (const unsigned char*)pSrc;
#endif

	while (cinfo.next_scanline < cinfo.image_height) {
		for (int i = 0; i < width; i++) 
		{

#if __XENON
			const int addr = XenonAddress2DTiledOffset(i, j, width, sizeof(u32));
			const u32 r = ((const u32*)pSrc)[addr];
#else
			const u32 r = ((const u32*)pSrcRGBAScanline)[i];
#endif
			
	
#if __XENON || __PS3 || RSG_ORBIS
			const unsigned char* px = (const unsigned char*)(&r);
			pDestRGBScanline[i*3 + 0] = px[__SRC_RED];
			pDestRGBScanline[i*3 + 1] = px[__SRC_GREEN];
			pDestRGBScanline[i*3 + 2] = px[__SRC_BLUE];
#else
			Color32 c;
			c.SetFromDeviceColor(r);
			pDestRGBScanline[i*3 + 0] = c.GetRed();
			pDestRGBScanline[i*3 + 1] = c.GetGreen();
			pDestRGBScanline[i*3 + 2] = c.GetBlue();
#endif
		}
		jpeg_write_scanlines(&cinfo, &pDestRGBScanline, 1);

#if __XENON
		j++;
#else
		pSrcRGBAScanline = (pSrcRGBAScanline + pitch);
#endif
	}

	(void) jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	int const c_fileSize = pStream->Size();

	if (bCloseStreamAtEnd)
	{
		pStream->Close();
	}

	return c_fileSize;
}


int grcImage::SaveRGB8SurfaceToJPEG(const char* filename, int quality, void* pSrc, int width, int height, int pitch)
{
	fiSafeStream SafeStream(ASSET.Create(filename,"jpg"));

	if (SafeStream == NULL) 
	{
		return false;
	}

	return SaveRGB8SurfaceToJPEG(SafeStream, quality, pSrc, width, height, pitch, false);
}


int grcImage::SaveRGB8SurfaceToJPEG(fiStream* pStream, int quality, void* pSrc, int width, int height, int pitch, bool bCloseStreamAtEnd)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	/* Initialize the JPEG decompression object with default error handling. */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_fiStream_dest(&cinfo, pStream);

	cinfo.in_color_space = JCS_RGB; /* arbitrary guess */
	jpeg_set_defaults(&cinfo);
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.data_precision = 8;

	jpeg_set_quality(&cinfo, quality, TRUE);

	(void) jpeg_start_compress(&cinfo,TRUE);

	sm_SaveComMarker.WriteComMarkerData(cinfo);

	unsigned char *pSrcRGBScanline = (unsigned char *)pSrc;

	while (cinfo.next_scanline < cinfo.image_height) 
	{
		jpeg_write_scanlines(&cinfo, &pSrcRGBScanline, 1);
		pSrcRGBScanline += pitch;
	}

	(void) jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	int const c_fileSize = pStream->Size();

	if (bCloseStreamAtEnd)
	{
		pStream->Close();
	}

	return c_fileSize;
}


grcTexture* grcImage::LoadJPEGToRGBA8Surface(const char* filename, void* pDst, u32 desiredWidth, u32 desiredHeight, bool bIsSystemMemory)
{
	fiStream *S = ASSET.Open(filename,"jpg",true);

	if (S == NULL) 
	{
		Displayf("grcImage::LoadJPEGToRGBA8Surface: failed to open \"%s\"", filename);
		return NULL;
	}

	return LoadJPEGToRGBA8Surface(S, pDst, desiredWidth, desiredHeight, bIsSystemMemory);
}

static void grcImageNullifyCachedTextureDataPointers(grcTexture* pTexture)
{
#if __PS3
	grcTextureGCM* pPlatformTex = (grcTextureGCM*)pTexture;
	CellGcmTexture* pGcmTex = pPlatformTex->GetTexturePtr();
	pGcmTex->offset = 0;
#elif __XENON
	grcTextureXenon* pPlatformTex = (grcTextureXenon*)pTexture;
	D3DTexture* pD3DTex = reinterpret_cast<D3DTexture*>(pPlatformTex->GetTexturePtr());
	pD3DTex->Format.BaseAddress = 0;
	pD3DTex->Format.MipAddress = 0;
#else
	(void)pTexture;
#endif
}

#ifdef _MSC_VER
#	pragma warning( push )
#	pragma warning( disable : 4611 )	//  warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
#endif
grcTexture* grcImage::LoadJPEGToRGBA8Surface(fiStream* pStream, void* pDst, u32 desiredWidth, u32 desiredHeight, bool bIsSystemMemory)
{
#if !RSG_ORBIS
	if (pDst == NULL) {
		Displayf("grcImage::LoadJPEGToRGBA8Surface: storage memory is NULL");
		pStream->Close();
		return NULL;
	}
#else //!RSG_ORBIS
	Assertf(pDst == NULL, "On Orbis the buffer space is allocated internally to the texture");
#endif

	// create texture first: if libjpeg returns upon error we need to be able to 
	// delete the texture
	grcTextureFactory::TextureCreateParams params(	(bIsSystemMemory ? grcTextureFactory::TextureCreateParams::SYSTEM : grcTextureFactory::TextureCreateParams::VIDEO), 
		grcTextureFactory::TextureCreateParams::LINEAR, grcsRead);

	u32 destWidth = desiredWidth;
	u32 destHeight = desiredHeight;

#if __XENON
	// Xenon implementation of grcTextureFactory::Create expects a D3DFORMAT type...
	const unsigned int format = (unsigned int)MAKELINFMT(D3DFMT_A8R8G8B8);
#elif __PS3
	const unsigned int format = grcImage::A8B8G8R8;
#else
	const unsigned int format = grctfA8R8G8B8; // Needs to be 2 to match ConvertToD3DFormat()
#endif

#if ! RSG_PC
	grcTexture* pTexture = grcTextureFactory::GetInstance().Create(destWidth, destHeight, format, pDst, 1, &params);

	// bail if we could not allocate the texture
	if (pTexture == NULL) {
		Displayf("grcImage::LoadJPEGToRGBA8Surface: failed to create texture");
		pStream->Close();
		return NULL;
	}
#else
	// don't create the texture until we have the pixels to fill it
	grcTexture* pTexture = NULL;
#endif

	struct jpeg_decompress_struct cinfo;
	grcimg_custom_error_mgr jerr;

	// Set up our custom error callback
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = grcimg_custom_error_exit;

	// If something goes wrong while decoding (ie: bad jpeg data) we need to clean up
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		Displayf("grcImage::LoadJPEGToRGBA8Surface: bad JPEG data");
		pStream->Close();
		if (pTexture)
		{
			grcImageNullifyCachedTextureDataPointers(pTexture);
			pTexture->Release();
		}
		return NULL;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_fiStream_src(&cinfo, pStream);

	sm_LoadComMarker.SetUpComMarkerForReading(cinfo);

	(void) jpeg_read_header(&cinfo, TRUE);

	sm_LoadComMarker.CheckForComMarkerThatHasBeenRead(cinfo);
	sm_LoadComMarker.ResetComMarkerForReading(cinfo);

	u32 scaleDenom = grcImageJPEGGetClosestScaleDenom(cinfo.image_width, cinfo.image_height, destWidth, destHeight);

	cinfo.scale_num = 1;
	cinfo.scale_denom = scaleDenom;

	(void)jpeg_calc_output_dimensions(&cinfo);

	if (destWidth != cinfo.output_width || destHeight != cinfo.output_height) 
	{
		Displayf("grcImage::LoadJPEGToRGBA8Surface: dest. dimensions (%ux%upx) do not match compressor output dimensions (%ux%upx)", destWidth, destHeight, cinfo.output_width, cinfo.output_height);
		jpeg_destroy_decompress(&cinfo);
		pStream->Close();
		if (pTexture)
		{
			grcImageNullifyCachedTextureDataPointers(pTexture);
			pTexture->Release();
		}
		return NULL;
	}

#if RSG_PC
	const u32 width = destWidth;
	const u32 stride = width * grcTextureFactory::GetBitsPerPixel((grcTextureFormat)format)/8;
#else
	const u32 width = pTexture->GetWidth();
#if __XENON
	XGTEXTURE_DESC surfaceDescription;
	XGGetTextureDesc(pTexture->GetTexturePtr(), 0, &surfaceDescription);
	const u32 stride = surfaceDescription.RowPitch;
#elif RSG_DURANGO
	grcTextureDurango* pTextureDurango = static_cast<grcTextureDurango *>( pTexture );
	const u32 stride = pTextureDurango->GetStride(0);
#else
	const u32 stride = pTexture->GetWidth()*pTexture->GetBitsPerPixel()/8;
#endif
#endif	// RSG_PC

	(void) jpeg_start_decompress(&cinfo);

#if RSG_ORBIS
	const sce::Gnm::Texture *gnmTexture = reinterpret_cast<const sce::Gnm::Texture*>(pTexture->GetTexturePtr());

	uint64_t offset;
	uint64_t size;

#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
	sce::GpuAddress::computeSurfaceOffsetAndSize(
#else
	sce::GpuAddress::computeTextureSurfaceOffsetAndSize(
#endif
		&offset,&size,gnmTexture,0,0);
	u32* tempDestination = rage_new u32[size];
	pDst = tempDestination;
#endif

	u8* dest = (u8*)pDst;

	copyJpegBuffer( cinfo, dest, width, stride );

	(void) jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	pStream->Close();

#if RSG_PC
	// non-placement texture so create *after* we have the image
	params.Memory = grcTextureFactory::TextureCreateParams::SYSTEM;
	BANK_ONLY(grcTexture::SetCustomLoadName("LoadJPEGToRGBA8Surface");)
#if USE_RESOURCE_CACHE
	ASSERT_ONLY(grcResourceCache::SetSafeResourceCreate(true);)
#endif
	pTexture = grcTextureFactory::GetInstance().Create(destWidth, destHeight, format, pDst, 1, &params);
#if USE_RESOURCE_CACHE
	ASSERT_ONLY(grcResourceCache::SetSafeResourceCreate(false);)
#endif
	BANK_ONLY(grcTexture::SetCustomLoadName(NULL);)

	// bail if we could not allocate the texture
	if (pTexture == NULL) {
		Displayf("grcImage::LoadJPEGToRGBA8Surface: failed to create texture");
		return NULL;
	}
#endif

#if RSG_ORBIS
	sce::GpuAddress::TilingParameters tp;
	tp.initFromTexture(gnmTexture, 0, 0);

	sce::GpuAddress::tileSurface(((char*)gnmTexture->getBaseAddress() + offset), pDst, &tp);
	delete [] tempDestination;
#endif

	return pTexture;

}

#ifdef _MSC_VER
#	pragma warning( pop ) //  warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
#endif
// This functions allows encoding jpeg images with non-multiple of 4 dimensions;
// the output image is cropped by by rounding down its dimensions to the next multiple of 4
grcTexture* grcImage::LoadJPEGToDXT1Surface(const char* filename, grcImage::DXTCompressFuncType pFnc, void* pDst, u32 dstBufferSize, u32 desiredWidth, u32 desiredHeight, bool bIsSystemMemory, void* pScratchBuffer, u32 scratchBufferSize, u32 maxScanlinesPerSlice)
{
	fiStream *S = ASSET.Open(filename,"jpg",true);

	if (S == NULL) 
	{
		Displayf("grcImage::LoadJPEGToDXT1Surface: failed to open \"%s\"", filename);
		return NULL;
	}

	return LoadJPEGToDXT1Surface(S, pFnc, pDst, dstBufferSize, desiredWidth, desiredHeight, bIsSystemMemory, pScratchBuffer, scratchBufferSize, maxScanlinesPerSlice);
}

#ifdef _MSC_VER
#	pragma warning( push )
#	pragma warning( disable : 4611 )	//  warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
#endif
// This functions allows encoding jpeg images with non-multiple of 4 dimensions;
// the output image is cropped by by rounding down its dimensions to the next multiple of 4
grcTexture* grcImage::LoadJPEGToDXT1Surface(fiStream* pStream, grcImage::DXTCompressFuncType pFnc, void* pDst, u32 dstBufferSize, u32 desiredWidth, u32 desiredHeight, bool CONSOLE_ONLY(bIsSystemMemory), void* pScratchBuffer, u32 scratchBufferSize, u32 maxScanlinesPerSlice)
{
	// Check function pointer
	if (pFnc == NULL) {
		Displayf("grcImage::LoadJPEGToDXT1Surface: function pointer is NULL");
		pStream->Close();
	}
	// Check destination buffer
	if (pDst == NULL) {
		Displayf("grcImage::LoadJPEGToDXT1Surface: storage memory is NULL");
		pStream->Close();
		return NULL;
	}
	// Check scratch buffer for uncompressed data
	if (pScratchBuffer == NULL) {
		Displayf("grcImage::LoadJPEGToDXT1Surface: scratch memory is NULL");
		pStream->Close();
		return NULL;
	}
	// Check at  width and height are at least 4 px
	if (desiredWidth < 4 || desiredHeight < 4) {
		Displayf("grcImage::LoadJPEGToDXT1Surface: width (%u) height(%u) less than 4", desiredWidth, desiredHeight);
		pStream->Close();
		return NULL;
	}
	// Check scratch buffer size for uncompressed data
	if (desiredWidth*4*maxScanlinesPerSlice > scratchBufferSize) {
		Displayf("grcImage::LoadJPEGToDXT1Surface: scratch buffer is too small");
		pStream->Close();
		return NULL;
	}
	// Check the number of scanlines per slice is multiple of 4
	if (maxScanlinesPerSlice % 4 != 0) {
		Displayf("grcImage::LoadJPEGToDXT1Surface: maxScanlinesPerSlice is not a multiple of 4");
		pStream->Close();
		return NULL;
	}

	// Create texture first: if libjpeg returns upon error we need to be able to 
	// delete the texture
#if RSG_ORBIS || RSG_DURANGO
	grcTextureFactory::TextureCreateParams::Format_t layout = grcTextureFactory::TextureCreateParams::TILED;
	grcTextureFactory::TextureCreateParams params(	(bIsSystemMemory ? grcTextureFactory::TextureCreateParams::SYSTEM : grcTextureFactory::TextureCreateParams::VIDEO), 
		layout, grcsRead);
#else
	grcTextureFactory::TextureCreateParams params( grcTextureFactory::TextureCreateParams::SYSTEM,
		grcTextureFactory::TextureCreateParams::TILED,
		grcsRead, NULL,
		grcTextureFactory::TextureCreateParams::NORMAL );
#endif

#if __DEV
	memset(pDst, 0, dstBufferSize);
#endif

	// If needed, round down image dimensions to the next multiple of 4
	u32 destWidth = (desiredWidth - (desiredWidth%4));
#if RSG_PC
	u32 destHeight = desiredHeight - (desiredHeight%4);
#else
	u32 destHeight = desiredHeight;
#endif

	const unsigned int format = grctfDXT1;

	struct jpeg_decompress_struct cinfo;
	grcimg_custom_error_mgr jerr;

	// Set up our custom error callback
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = grcimg_custom_error_exit;

	// If something goes wrong while decoding (ie: bad jpeg data) we need to clean up
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		Displayf("grcImage::LoadJPEGToRGBA8Surface: bad JPEG data");
		pStream->Close();
		return NULL;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_fiStream_src(&cinfo, pStream);

	sm_LoadComMarker.SetUpComMarkerForReading(cinfo);

	(void) jpeg_read_header(&cinfo, TRUE);

	sm_LoadComMarker.CheckForComMarkerThatHasBeenRead(cinfo);
	sm_LoadComMarker.ResetComMarkerForReading(cinfo);

	u32 scaleDenom = grcImageJPEGGetClosestScaleDenom(cinfo.image_width, cinfo.image_height, desiredWidth, desiredHeight);

	cinfo.scale_num = 1;
	cinfo.scale_denom = scaleDenom;

	(void)jpeg_calc_output_dimensions(&cinfo);

	// Adjusted dimensions from cinfo must match ours
	u32 outputWidth = (cinfo.output_width  - (cinfo.output_width%4));
	u32 outputHeight = (cinfo.output_height);
	if (destWidth != outputWidth CONSOLE_ONLY(|| destHeight != outputHeight)) 
	{
		Displayf("grcImage::LoadJPEGToDXT1Surface: dest. dimensions (%ux%upx) do not match compressor output dimensions (%ux%upx)", destWidth, destHeight, outputWidth, outputHeight);
		jpeg_destroy_decompress(&cinfo);
		pStream->Close();
		return NULL;
	}

	// Width is the adjusted desiredWidth
	const u32 width = destWidth;
	const u32 bpp = 4;

#if RSG_DURANGO
	const u32 paddedWidth = (width+255) & (~255);
	const u32 dxtStride = paddedWidth*bpp/8;
#else
	const u32 dxtStride = width*bpp/8;
#endif

//#endif

	const u32 rgbaStride =  width*4;
	unsigned curRgbaRow = 0;

	(void) jpeg_start_decompress(&cinfo);

	// We still need to allocate for desiredWidth, since it's the number of pixels 
	// the jpeg decoder will read for a scanline
	unsigned char *rgb = Alloca(unsigned char, desiredWidth * 3);
	unsigned char *pRgbaRow = (unsigned char *)pScratchBuffer;

	u8* pDxtRow = (u8*)pDst + (curRgbaRow * dxtStride);
	
	while (cinfo.output_scanline < outputHeight) {
		jpeg_read_scanlines(&cinfo, &rgb, 1);
		for (int i=0; i<(int)width; i++) {
#if __WIN32PC || RSG_DURANGO
			Color32 c(rgb[i*3+0], rgb[i*3+1], rgb[i*3+2], 255);
			*(u32*)&pRgbaRow[i*4] = c.GetDeviceColor();
#elif RSG_ORBIS
			Color32 c(255, rgb[i*3+2], rgb[i*3+1], rgb[i*3+0]);
			*(u32*)&pRgbaRow[i*4] = c.GetDeviceColor();
#else

#if __BE
			pRgbaRow[i*4+0] = 255;

#if __PS3
			pRgbaRow[i*4+1] = rgb[i*3+2];
			pRgbaRow[i*4+2] = rgb[i*3+1];
			pRgbaRow[i*4+3] = rgb[i*3+0];
#else
			pRgbaRow[i*4+1] = rgb[i*3+0];
			pRgbaRow[i*4+2] = rgb[i*3+1];
			pRgbaRow[i*4+3] = rgb[i*3+2];
#endif 

#else
			pRgbaRow[i*4+3] = 255;

#if __PS3
			pRgbaRow[i*4+2] = rgb[i*3+2];
			pRgbaRow[i*4+1] = rgb[i*3+1];
			pRgbaRow[i*4+0] = rgb[i*3+0];
#else
			pRgbaRow[i*4+2] = rgb[i*3+0];
			pRgbaRow[i*4+1] = rgb[i*3+1];
			pRgbaRow[i*4+0] = rgb[i*3+2];
#endif

#endif

#endif
		}

		const u32 numRgbaRows = curRgbaRow+1;
		// Have we batched enough scanlines?
		if ((numRgbaRows % maxScanlinesPerSlice) == 0)
		{

			// Check we won't overflow the destination buffer
			if (pDxtRow < ( (u8*)pDst + dstBufferSize))
			{
				// Compress batched RGBA data
				pFnc(pDxtRow, (unsigned char *)pScratchBuffer, dxtStride, rgbaStride, maxScanlinesPerSlice/4);
			}
			else
			{
				// If we are, do nothing but assert
				Assertf(0, "grcImage::LoadJPEGToDXT1Surface: dest. buffer overrun avoided");
			}

			// Compute current offset in DXT buffer
			pDxtRow = (u8*)pDst + ((numRgbaRows/4) * dxtStride * maxScanlinesPerSlice);

			// Reset offset for RGBA buffer
			pRgbaRow = (unsigned char *)pScratchBuffer;
			memset(pScratchBuffer, 0, scratchBufferSize);
		}
		else
		{
			pRgbaRow += rgbaStride;
		}

		// Scratch buffer overflow check 
		if (pRgbaRow > ((u8*)pScratchBuffer + scratchBufferSize))
		{
			Assertf(0, "grcImage::LoadJPEGToDXT1Surface: scratch buffer overrun avoided");
			pRgbaRow = (u8*)pScratchBuffer;
		}

		curRgbaRow++;
	}

	// Do the last block
	if (destHeight % 4 != 0)
	{
		if (pDxtRow < ( (u8*)pDst + dstBufferSize))
		{
			// Compress batched RGBA data
			pFnc(pDxtRow, (unsigned char *)pScratchBuffer, dxtStride, rgbaStride, maxScanlinesPerSlice/4);
		}
		else
		{
			// If we are, do nothing but assert
			Assertf(0, "grcImage::LoadJPEGToDXT1Surface: dest. buffer overrun avoided");
		}

	}

	(void) jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	pStream->Close();

	grcTexture* pTexture = grcTextureFactory::GetInstance().Create(destWidth, destHeight, format, pDst, 1, &params);

	// Bail if we could not allocate the texture
	if (pTexture == NULL) {
		Displayf("grcImage::LoadJPEGToDXT1Surface: failed to create texture");
		return NULL;
	}

#if RSG_DURANGO
	static_cast<grcTextureDurango*>(pTexture)->TileInPlace();
#endif

#if RSG_ORBIS
	const sce::Gnm::Texture *gnmTexture = reinterpret_cast<const sce::Gnm::Texture*>(pTexture->GetTexturePtr());

	uint64_t offset;
	uint64_t size;

#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
	sce::GpuAddress::computeSurfaceOffsetAndSize(
#else
	sce::GpuAddress::computeTextureSurfaceOffsetAndSize(
#endif
		&offset,&size,gnmTexture,0,0);

	sce::GpuAddress::TilingParameters tp;
	tp.initFromTexture(gnmTexture, 0, 0);

	sce::GpuAddress::tileSurface(((char*)gnmTexture->getBaseAddress() + offset), pDst, &tp);
#endif

	return pTexture;
}
#ifdef _MSC_VER
#	pragma warning( pop ) //  warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
#endif

bool grcImage::GetJPEGInfoForRGBA8Surface(const char* filename, u32& width, u32& height, u32& pitch)
{
	fiStream *S = ASSET.Open(filename,"jpg",true);

	if (S == NULL) 
	{
		Displayf("grcImage::GetJPEGInfoForRGBA8Surface: failed to open \"%s\"", filename);
		return false;
	}

	return GetJPEGInfoForRGBA8Surface(S, width, height, pitch);
}

#if ( RSG_PC || RSG_DURANGO )
#pragma warning(push) 
#pragma warning(disable: 4611)
#endif

bool grcImage::GetJPEGInfoForRGBA8Surface(fiStream* pStream, u32& width, u32& height, u32& pitch)
{
	struct jpeg_decompress_struct cinfo;
	struct grcimg_custom_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = grcimg_custom_error_exit;

    if( setjmp( jerr.setjmp_buffer ) ) 
    {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
        Displayf("grcImage::GetJPEGInfoForRGBA8Surface: bad JPEG data");
        jpeg_destroy_decompress(&cinfo);
        pStream->Close();
        return false;
      }

	jpeg_create_decompress(&cinfo);
	jpeg_fiStream_src(&cinfo, pStream);
	(void) jpeg_read_header(&cinfo, TRUE);

	height = cinfo.image_height;
	width = cinfo.image_width;

#if __XENON
	// pitch size needs to be multiple of 256
	pitch = (((width*4)+255) & (~255));
#else
	pitch = width*4;
#endif	

	jpeg_destroy_decompress(&cinfo);

	pStream->Close();
	return true;
}

#if ( RSG_PC || RSG_DURANGO )
#pragma warning(pop)
#endif

void grcImage::SetComMarkerToSave(const char *pStringToSave)
{
	sm_SaveComMarker.SetComMarkerToSave(pStringToSave);
}

void grcImage::ClearComMarkerToSave()
{
	sm_SaveComMarker.ClearComMarkerToSave();
}

void grcImage::SetComMarkerToLoad(ReadComMarkerCB ReadComMarkerCallback, u32 MaxLengthOfStringToLoad)
{
	sm_LoadComMarker.SetComMarkerToLoad(ReadComMarkerCallback, MaxLengthOfStringToLoad);
}

void grcImage::ClearComMarkerToLoad()
{
	sm_LoadComMarker.ClearComMarkerToLoad();
}


// ******************************************************************************
// **
// ** grcJpegSaveComMarker
// **
// ******************************************************************************

void grcJpegSaveComMarker::SetComMarkerToSave(const char *pStringToSave)
{
	Assertf(m_pStringToSave == NULL, "grcJpegSaveComMarker::SetComMarkerToSave - expected m_pStringToSave to be NULL before setting. Maybe it hasn't been cleared after a previous save");

	m_pStringToSave = pStringToSave;
}

void grcJpegSaveComMarker::ClearComMarkerToSave()
{
	m_pStringToSave = NULL;
}

void grcJpegSaveComMarker::WriteComMarkerData(struct jpeg_compress_struct &cinfo)
{
	//	call jpeg_write_marker() after jpeg_start_compress() and before the first call to jpeg_write_scanlines()
	if (m_pStringToSave)
	{
		jpeg_write_marker(&cinfo, JPEG_COM, (const JOCTET *) m_pStringToSave, ustrlen(m_pStringToSave));
	}
}


// ******************************************************************************
// **
// ** grcJpegLoadComMarker
// **
// ******************************************************************************

void grcJpegLoadComMarker::SetComMarkerToLoad(ReadComMarkerCB ReadComMarkerCallback, u32 MaxLengthOfStringToLoad)
{
	Assertf(m_ReadComMarkerCB == NULL, "grcJpegLoadComMarker::SetComMarkerToLoad - expected m_ReadComMarkerCB to be NULL before setting. Maybe it hasn't been cleared after a previous load");
	Assertf(m_MaxExpectedLengthOfStringToLoad == 0, "grcJpegLoadComMarker::SetComMarkerToLoad - expected m_MaxExpectedLengthOfStringToLoad to be 0 before setting. Maybe it hasn't been cleared after a previous load");

	m_ReadComMarkerCB = ReadComMarkerCallback;
	m_MaxExpectedLengthOfStringToLoad = MaxLengthOfStringToLoad;
}

void grcJpegLoadComMarker::ClearComMarkerToLoad()
{
	m_ReadComMarkerCB = NULL;
	m_MaxExpectedLengthOfStringToLoad = 0;
}

//	Call this before jpeg_read_header so that we know that the COM marker needs to be handled
void grcJpegLoadComMarker::SetUpComMarkerForReading(struct jpeg_decompress_struct &cinfo)
{
	if (m_ReadComMarkerCB)
	{
		if (Verifyf(m_MaxExpectedLengthOfStringToLoad > 0, "grcJpegLoadComMarker::SetUpComMarkerForReading - pointer is set but size is 0"))
		{
			jpeg_save_markers(&cinfo, JPEG_COM, m_MaxExpectedLengthOfStringToLoad);
		}
	}
}

//	Call this after jpeg_read_header to check if the COM marker has been found
void grcJpegLoadComMarker::CheckForComMarkerThatHasBeenRead(struct jpeg_decompress_struct &cinfo)
{
	//	After jpeg_read_header() completes, you can examine the special markers by following the cinfo->marker_list pointer chain
	jpeg_saved_marker_ptr pCurrentMarker = cinfo.marker_list;
	while (pCurrentMarker)
	{
		if (pCurrentMarker->marker == JPEG_COM)
		{
			if (m_ReadComMarkerCB)
			{
				m_ReadComMarkerCB(pCurrentMarker);
			}
		}

		pCurrentMarker = pCurrentMarker->next;
	}
}

void grcJpegLoadComMarker::ResetComMarkerForReading(struct jpeg_decompress_struct &cinfo)
{
	//	Reset the length limit to zero for all marker types after finishing jpeg_read_header
//	if (m_ReadComMarkerCB)
	{
		jpeg_save_markers(&cinfo, JPEG_COM, 0);
	}
}

// ******************************************************************************
// **
// ** End of grcJpegLoadComMarker
// **
// ******************************************************************************


}	// namespace rage
