//
// grcore/image.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_IMAGE_H
#define GRCORE_IMAGE_H

#include "vector/color32.h"
#include "vector/vector3.h"
#include "file/stream.h"
#include "jpeg/jpeglib.h"

#if __RESOURCECOMPILER
#include "atl/array.h"
#include <map>
#endif // __RESOURCECOMPILER

namespace rage {

class  Vector4;
class  grcTexture;
struct grcTextureLock;

class grcJpegSaveComMarker
{
public:
	grcJpegSaveComMarker() : m_pStringToSave(NULL) {}

	void SetComMarkerToSave(const char *pStringToSave);

	void ClearComMarkerToSave();

	void WriteComMarkerData(struct jpeg_compress_struct &cinfo);

private:
	const char *m_pStringToSave;
};


typedef void (*ReadComMarkerCB)(jpeg_saved_marker_ptr pMarker);

class grcJpegLoadComMarker
{
public:
	grcJpegLoadComMarker() : m_ReadComMarkerCB(NULL), m_MaxExpectedLengthOfStringToLoad(0) {}

	void SetComMarkerToLoad(ReadComMarkerCB ReadComMarkerCallback, u32 MaxLengthOfStringToLoad);

	void ClearComMarkerToLoad();

	//	Call this before jpeg_read_header so that we know to handle the COM marker
	void SetUpComMarkerForReading(struct jpeg_decompress_struct &cinfo);

	//	Call this after jpeg_read_header to check whether a COM marker has been found
	void CheckForComMarkerThatHasBeenRead(struct jpeg_decompress_struct &cinfo);

	// Call this after CheckForComMarkerThatHasBeenRead to reset the data that had been set by SetUpComMarkerForReading()
	void ResetComMarkerForReading(struct jpeg_decompress_struct &cinfo);

private:
	ReadComMarkerCB m_ReadComMarkerCB;
	u32 m_MaxExpectedLengthOfStringToLoad;
};

/*
PURPOSE
	grcImage is a simple, portable image management class.  All textures are created
	from grcImages.  The color formats used by grcImage are platform-independent and
	are converted as necessary when making textures.
	<FLAG Component>
*/
class grcImage {
	~grcImage();
	friend class GccIgnoreWarning;
public:
	grcImage();

	// Supported image types; these intentionally do not match D3DFMT_ values because those are platform-dependent.
	// When adding new types, make sure to update:
	// rage/base/src/grcore/image.cpp
	// rage/base/src/grcore/texture_d3d9.cpp (GetD3DFormat, etc.)
	// rage/base/src/grcore/texture_d3d11.cpp (GetD3DFormat, etc.)
	// rage/base/src/grcore/texturexenon.cpp (GetD3DFormat)
	// rage/base/src/grcore/texturegcm.cpp (Init)
	// rage/base/tools/grconvert/texturexenonproxy.cpp (Init)
	// rage/framework/tools/src/cli/ragebuilder/textureconversion/textureconversion
	// etc.
	enum Format {
		UNKNOWN                    , // Undefined
		DXT1                       , // DXT1 (aka 'BC1')
		DXT3                       , // DXT3 (aka 'BC2')
		DXT5                       , // DXT5 (aka 'BC3', 'DXT5NM', 'RXGB')
		CTX1                       , // CTX1 (like DXT1 but anchor colors are 8.8 instead of 5.6.5)
		DXT3A                      , // alpha block of DXT3 (XENON-specific)
		DXT3A_1111                 , // alpha block of DXT3, split into four 1-bit channels (XENON-specific)
		DXT5A                      , // alpha block of DXT5 (aka 'BC4', 'ATI1')
		DXN                        , // DXN (aka 'BC5', 'ATI2', '3Dc', 'RGTC', 'LATC', etc.) // [CLEMENSP]
		BC6                        , // BC6 (specifically BC6H_UF16)
		BC7                        , // BC7
		A8R8G8B8                   , // 32-bit color with alpha, matches Color32 class
		A8B8G8R8                   , // 32-bit color with alpha, provided for completeness
		A8                         , // 8-bit alpha-only (color is black)
		L8                         , // 8-bit luminance (R=G=B=L, alpha is opaque)
		A8L8                       , // 16-bit alpha + luminance
		A4R4G4B4                   , // 16-bit color and alpha
		A1R5G5B5                   , // 16-bit color with 1-bit alpha
		R5G6B5                     , // 16-bit color
		R3G3B2                     , // 8-bit color (not supported on consoles)
		A8R3G3B2                   , // 16-bit color with 8-bit alpha (not supported on consoles)
		A4L4                       , // 8-bit alpha + luminance (not supported on consoles)
		A2R10G10B10                , // 32-bit color with 2-bit alpha
		A2B10G10R10                , // 32-bit color with 2-bit alpha
		A16B16G16R16               , // 64-bit four channel fixed point (s10e5 per channel -- sign, 5 bit exponent, 10 bit mantissa)
		G16R16                     , // 32-bit two channel fixed point
		L16                        , // 16-bit luminance
		A16B16G16R16F              , // 64-bit four channel floating point (s10e5 per channel)
		G16R16F                    , // 32-bit two channel floating point (s10e5 per channel)
		R16F                       , // 16-bit single channel floating point (s10e5 per channel)
		A32B32G32R32F              , // 128-bit four channel floating point (s23e8 per channel)
		G32R32F                    , // 64-bit two channel floating point (s23e8 per channel)
		R32F                       , // 32-bit single channel floating point (s23e8 per channel)
		D15S1                      , // 16-bit depth + stencil (depth is 15-bit fixed point, stencil is 1-bit) (not supported on consoles)
		D24S8                      , // 32-bit depth + stencil (depth is 24-bit fixed point, stencil is 8-bit)
		D24FS8                     , // 32-bit depth + stencil (depth is 24-bit s15e8, stencil is 8-bit)
		P4                         , // 4-bit palettized (not supported on consoles)
		P8                         , // 8-bit palettized (not supported on consoles)
		A8P8                       , // 16-bit palettized with 8-bit alpha (not supported on consoles)
		R8                         , // non-standard R001 format (8 bits per channel)
		R16                        , // non-standard R001 format (16 bits per channel)
		G8R8                       , // non-standard RG01 format (8 bits per channel)
		LINA32B32G32R32F_DEPRECATED,
		LINA8R8G8B8_DEPRECATED     ,
		LIN8_DEPRECATED            ,
		RGBE                       ,
		LAST_FORMAT = RGBE
	};
	enum { FORMAT_COUNT = LAST_FORMAT + 1 };

	enum FormatFlags { // doesn't hurt to put this in its own enum, and this would allow switching on Format without 'default'
		FORMAT_FLAG_sRGB   = 0x80000000,
		FORMAT_FLAG_LINEAR = 0x40000000,
		FORMAT_FLAG_SYSMEM = 0x20000000,
		FORMAT_MASK        = 0x1fffffff,
	};

	enum ImageType {
		STANDARD, // 2D image (still supports mipmaps, and texture array layers)
		CUBE,     // Cube map image (+x,-x,+y,-y,+z,-z, stored as a 6-layer 2D image)
		DEPTH,    // Depth map image
		VOLUME,   // Volume texture
		FORCETYPE32 = 0x7FFFFFFF
	};

	static __forceinline bool IsFormatDXTBlockCompressed(Format format)
	{
		return
		(
			format == DXT1       ||
			format == DXT3       ||
			format == DXT5       ||
			format == CTX1       ||
			format == DXT3A      ||
			format == DXT3A_1111 ||
			format == DXT5A      ||
			format == DXN        ||
            format == BC6        ||
            format == BC7
		);
	}

	static __forceinline bool IsFormatGreaterThan8BitsPerComponent(Format format)
	{
		if (GetFormatBitsPerPixel(format) > 32) // most >8bpc formats are handled with this test
		{
			return true;
		}

		return
		(
			format == A2R10G10B10   ||
			format == A2B10G10R10   ||
			format == A16B16G16R16  ||
			format == G16R16        ||
			format == R16           ||
			format == L16           ||
			format == A16B16G16R16F ||
			format == G16R16F       ||
			format == R16F          ||
			format == A32B32G32R32F ||
			format == G32R32F       ||
			format == R32F          ||
			format == D15S1         ||
			format == D24S8         ||
			format == D24FS8
		);
	}

	static __forceinline bool IsFormatDepthCompatible(Format format)
	{
		return
		(
			format == L16	||	//workaround for the missing D16
			format == R32F	||	//workaround for the missing D32F
			format == D15S1	||
			format == D24S8	||
			format == D24FS8
		);
	}

	static __forceinline int GetFormatBitsPerPixel(Format format)
	{
		switch (format)
		{
		case UNKNOWN                     : return   0;
		case DXT1                        : return   4;
		case DXT3                        : return   8;
		case DXT5                        : return   8;
		case CTX1                        : return   4;
		case DXT3A                       : return   4;
		case DXT3A_1111                  : return   4;
		case DXT5A                       : return   4;
		case DXN                         : return   8;
		case BC6                         : return   8;
		case BC7                         : return   8;
		case A8R8G8B8                    : return  32;
		case A8B8G8R8                    : return  32;
		case A8                          : return   8;
		case L8                          : return   8;
		case A8L8                        : return  16;
		case A4R4G4B4                    : return  16;
		case A1R5G5B5                    : return  16;
		case R5G6B5                      : return  16;
		case R3G3B2                      : return   8;
		case A8R3G3B2                    : return  16;
		case A4L4                        : return   8;
		case A2R10G10B10                 : return  32;
		case A2B10G10R10                 : return  32;
		case A16B16G16R16                : return  64;
		case G16R16                      : return  32;
		case L16                         : return  16;
		case A16B16G16R16F               : return  64;
		case G16R16F                     : return  32;
		case R16F                        : return  16;
		case A32B32G32R32F               : return 128;
		case G32R32F                     : return  64;
		case R32F                        : return  32;
		case D15S1                       : return  16;
		case D24S8                       : return  32;
		case D24FS8                      : return  32;
		case P4                          : return   4;
		case P8                          : return   8;
		case A8P8                        : return  16;
		case R8                          : return   8;
		case R16                         : return  16;
		case G8R8                        : return  16;
		case LINA32B32G32R32F_DEPRECATED : return 128;
		case LINA8R8G8B8_DEPRECATED      : return  32;
		case LIN8_DEPRECATED             : return   8;
		case RGBE                        : return  32;
		}

		return 0;
	}

	static __forceinline const char* GetFormatString(Format format)
	{
		switch (format)
		{
#define SWITCH_STRING(s) case s: return #s
		SWITCH_STRING(UNKNOWN                    );
		SWITCH_STRING(DXT1                       );
		SWITCH_STRING(DXT3                       );
		SWITCH_STRING(DXT5                       );
		SWITCH_STRING(CTX1                       );
		SWITCH_STRING(DXT3A                      );
		SWITCH_STRING(DXT3A_1111                 );
		SWITCH_STRING(DXT5A                      );
		SWITCH_STRING(DXN                        );
		SWITCH_STRING(BC6                        );
		SWITCH_STRING(BC7                        );
		SWITCH_STRING(A8R8G8B8                   );
		SWITCH_STRING(A8B8G8R8                   );
		SWITCH_STRING(A8                         );
		SWITCH_STRING(L8                         );
		SWITCH_STRING(A8L8                       );
		SWITCH_STRING(A4R4G4B4                   );
		SWITCH_STRING(A1R5G5B5                   );
		SWITCH_STRING(R5G6B5                     );
		SWITCH_STRING(R3G3B2                     );
		SWITCH_STRING(A8R3G3B2                   );
		SWITCH_STRING(A4L4                       );
		SWITCH_STRING(A2R10G10B10                );
		SWITCH_STRING(A2B10G10R10                );
		SWITCH_STRING(A16B16G16R16               );
		SWITCH_STRING(G16R16                     );
		SWITCH_STRING(L16                        );
		SWITCH_STRING(A16B16G16R16F              );
		SWITCH_STRING(G16R16F                    );
		SWITCH_STRING(R16F                       );
		SWITCH_STRING(A32B32G32R32F              );
		SWITCH_STRING(G32R32F                    );
		SWITCH_STRING(R32F                       );
		SWITCH_STRING(D15S1                      );
		SWITCH_STRING(D24S8                      );
		SWITCH_STRING(D24FS8                     );
		SWITCH_STRING(P4                         );
		SWITCH_STRING(P8                         );
		SWITCH_STRING(A8P8                       );
		SWITCH_STRING(R8                         );
		SWITCH_STRING(R16                        );
		SWITCH_STRING(G8R8                       );
		SWITCH_STRING(LINA32B32G32R32F_DEPRECATED);
		SWITCH_STRING(LINA8R8G8B8_DEPRECATED     );
		SWITCH_STRING(LIN8_DEPRECATED            );
		SWITCH_STRING(RGBE                       );
#undef  SWITCH_STRING
		}

		return "";
	}

	static __forceinline int GetFormatByteSwapSize(Format format)
	{
		if (IsFormatDXTBlockCompressed(format))
		{
			return 2; // all DXT blocks are 2-byte swapped
		}
		else if (format == A16B16G16R16 || format == A16B16G16R16F)
		{
			return 2; // matching D3DFMT specs for 360, not sure why it's like this ..
		}

		return Clamp<int>(GetFormatBitsPerPixel(format)/8, 1, 4);
	}

	static void ByteSwapData(void* data, int dataSize, int swapSize);

	__forceinline int GetPhysicalWidth () const { const int mask = IsFormatDXTBlockCompressed(GetFormat()) ? 3 : 0; return (m_Width + mask)&~mask; }
	__forceinline int GetPhysicalHeight() const { const int mask = IsFormatDXTBlockCompressed(GetFormat()) ? 3 : 0; return (m_Height + mask)&~mask; }
	__forceinline int GetPhysicalDepth () const { return m_Depth; }

	// PURPOSE:	Loads an image from specified file.
	// PARAMS:	filename - name of file to load; appropriate platform-specific extension 
	//				is added if missing.
	// RETURNS:	Pointer to newly loaded image, or NULL if file was not found.
	static grcImage *Load(const char *filename);

	// PURPOSE:	Reformats an image
	// PARAMS:	newFormat - Format of new image
	// RETURNS:	Pointer to new image of same size as original.  If new format
	//			is same as original, this may just increase the reference count
	//			on the original image.  Returns NULL if it is unable to perform
	//			the requested conversion.
	// NOTES:	Only does the exact image pointed to, not any layers or mipmaps.
	grcImage *Reformat(Format newFormat);

	// PURPOSE:	Returns a checkerboard image of the desired size and color.
	// PARAMS:	size - Size of the image (always square)
	//			color1 - First checker color
	//			color2 - Second checker color
	// RETURNS:	New checkerboard image (never any mipmaps, always A8R8G8B8)
	//			The checker block size is always four texels; this make it easier to
	//			spot problems when textures are not heavily tiled.
	static grcImage *MakeChecker(int size,Color32 color1,Color32 color2);

	// PURPOSE:	Loads an image from specified file.
	// PARAMS:	filename - name of file to load; "dds" extension 
	//				is added if missing.
	// RETURNS:	Pointer to newly loaded image, or NULL if file was not found.
	static grcImage *LoadDDS(const char *filename);

	// PURPOSE:	Loads an image from specified file
	// PARAMS:	filename - name of file to load; "jpg" extension 
	//				is added if missing.
    //          image - optional image to populate with loaded data
	// RETURNS:	Pointer to newly loaded image, or NULL if file was not found.
	// NOTES:	This type is not automatically supported by Load to avoid a permanent
	//			dependency on a large library.
	static grcImage *LoadJPEG(const char *filename,grcImage* image = NULL , bool useFileAttributes = false);

	// PURPOSE:	Loads an image from specified file
	// PARAMS:	pStream - fiStream that is used for loading
	//          image - optional image to populate with loaded data
	// RETURNS:	Pointer to newly loaded image, or NULL if file was not found.
	// NOTES:	This type is not automatically supported by Load to avoid a permanent
	//			dependency on a large library.
	static grcImage *LoadJPEG(fiStream* pStream,grcImage* image = NULL , bool useFileAttributes = false);

	// PURPOSE:	Saves an image to specified file.
	// PARAMS:	filename - name of file to save; "dds" extension is added if missing
	// RETURNS:	True on success, false on failure
	bool SaveDDS(const char *filename, bool saveAsDX10Format = false) const;

	// PURPOSE:	Creates a DDS header for 32bit RGBA file of specified size (for screenshots)
	// PARAMS:	filename - Name to create, .dds is added if not present
	//			width - Width of image
	//			height - Height of image
	// RETURNS:	Pointer to stream if successful, or NULL on failure.
	static fiStream *SaveDDSHeader(const char *filename,int width,int height);

	// PURPOSE:	Write a 24bpp PNG file using the supplied image data
	// PARAMS:	filename - file to create
	//			copyscan - Callback which copies N pixels from source into r,g,b byte order using supplied gamma lut
	//			width, height - Extents of image
	//			base - Base address of image data
	//			stride - Distance in bytes between scanlines
	//			gamma - image gamma (gAMA chunk) to write, or zero to not write anything (the default)
	// RETURNS:	True on success, false if file could not be created
	static bool WritePNG(const char *filename,void (*copyscan)(u8*,void*,int,int,int,u8*),int width,int height,void *base,int stride,float gamma);

	// Save image data out as a PNG file.
	bool SavePNG(const char *filename,float gamma);

	// PURPOSE:	Saves an image to specified file.
	// PARAMS:	filename - name of file to save; "jpg" extension is added if missing
	//			quality - image quality, 0 to 100 percent
	// RETURNS:	True on success, false on failure
	bool SaveJPEG(const char *filename,int quality = 75) const;

	// PURPOSE:	Saves an image to specified file.
	// PARAMS:	pStream - fiStream that is used for saving
	//			quality - image quality, 0 to 100 percent
	// RETURNS:	True on success, false on failure
	// NOTES:	NO LONGER closes the stream itself so that the caller can check the resulting size.
	bool SaveJPEG(fiStream* pStream,int quality = 75) const;

	static bool SaveStreamToJPEG(const char *filename,void* pBits,int width, int height, int stride,int quality);

	// PURPOSE:	Creates an image of specified size and format; bits are uninitialized.
	// PARAMS:	width - Width of image to create
	//			height - Height of image to create
	//			depth - Depth of image to create (implies type VOLUME if greater than one)
	//			format - format of image to create
	//			extraMipmaps - Number of additional mipmaps to create
	//			extraLayers - number of extra layers to create. For cubemaps, a layer
	//				will correspond to a face of the cube. For volume textures, 
	//				the layer term will refer to the discrete texture layer of that volume.
	// RETURNS:	New image (with mip chain)
	static grcImage *Create(u32 width,u32 height,u32 depth,Format format, ImageType type, u32 extraMipmaps, u32 extraLayers);
private:
	static grcImage *CreateInternal(u32 width,u32 height,u32 depth,Format format, ImageType type, u32 extraMipmaps, u32 extraLayers);
public:

	// PURPOSE:	Changes the size of this image to the given parameters.  
	//			Currently no mipmaps or layers are supported.  This image should not have either.
	//			NO MEMORY IS DEALLOCATED OR ALLOCATED HERE.  IT IS THE RESPONSABILITY OF THE CLIENT TO MANAGE MEMORY FOR this grcImage*
	// PARAMS:	width -   Width of image
	//			height -  Height of image
	//			depth -	  Depth of image (implies type VOLUME if greater than one)
	//			format -  format of image
	void Resize(u32 width,u32 height);

	// PURPOSE:	Clears image to a specific color
	// PARAMS:	color - Color to clear image to
	void Clear(u32 color);

	// RETURNS: Format of this image
	inline Format GetFormat() const;

	// RETURNS: if image is stored in sRGB 2.2 gamma space.
	inline bool IsSRGB() const;
	inline bool IsLinear() const;
	inline bool IsSysMem() const;

#if __RESOURCECOMPILER
	inline void SetIsSysMem();
#endif

	// RETURNS: Type of this image
	inline ImageType GetType() const;

	// RETURNS: Width of image
	inline u16 GetWidth() const;

	// RETURNS: Stride of image (width times bytes per pixel)
	inline u32 GetStride() const;

	// RETURNS: Height of image
	inline u16 GetHeight() const;

	// RETURNS: Depth of image (usually 1)
	inline u16 GetDepth() const;

	// RETURNS: Area of image (width * height)
	inline u32 GetArea() const;

	// RETURNS: Volume of image (width * height * depth) (usually just the area)
	inline u32 GetVolume() const;

	// RETURNS: Size of image (stride * height * depth)
	inline u32 GetSize() const;

	// RETURNS: Pointer to image bits
	inline u8 *GetBits() const;

	// RETURNS: Pointer to image bits for given mip and z slice
	inline u8* GetBits(u32 slice, u32 mipLevel) const;

#if !__RESOURCECOMPILER
	// RETURNS: Color exponent for HDR textures
	inline const Vector3& GetColorExponent() const;

	// RETURNS: Color offset for HDR textures
	inline const Vector3& GetColorOffset() const;
#endif // !__RESOURCECOMPILER

	// RETURNS: Pointer to image bits, but only if it's full RGBA
	inline Color32* GetColor32() const;

	// RETURNS: Number of extra mipmaps in this image
	int GetExtraMipCount() const;

	// RETURNS: address of pixel at location (x,y), works for ALL image formats (block compressed formats assume x and y are multiples of 4)
	void* GetPixelAddr(int x, int y, int z = 0) const;
	__forceinline void* GetPixelRowAddr(int y, int z = 0) const { return GetPixelAddr(0, y, z); }

	// these GetPixel methods should replace the older ones below .. eventually
	void    GetPixelBlock(Vector4 block[4*4], int x, int y, int z = 0) const;
	void    GetPixelBlock(Color32 block[4*4], int x, int y, int z = 0) const;
	Vector4 GetPixelVector4(int x, int y, int z = 0) const;
	Color32 GetPixelColor32(int x, int y, int z = 0) const;

	void LoadAsTextureAlias(const grcTexture* texture, const grcTextureLock* lock, int mipCount = 1); // this creates an alias to the texture data, so do not delete or release the image
	void ReleaseAlias();

	// PURPOSE: Returns RGBA color value at specified texel
	// PARAMS:	x, y - coordinates to read
	// NOTES:	Only supports the following formats properly:
	//			 - A8R8G8B8
	//			 - LINA8R8G8B8_DEPRECATED
	//			 - RGBE
	//			 - R32F
	//			 - G16R16
	//			 - A8
	//			 - L8
	//			All other formats such as DXT return undefined results.
	u32 GetPixel(int x,int y) const;

	// PURPOSE: Writes color at specified texel
	// PARAMS:	x, y coordinates to set pixel at
	//			color - value to set at this texel
	// NOTES:	DXT1 textures work, but the only colors that
	//			are supported are black (transparent), black (opaque),
	//			and white (opaque).  All other colors are mapped
	//			to 50% grey (opaque).  This is intended for fonts.
	//			DXT5 textures do not currently work.
	void SetPixel(int x,int y,u32 color);

	// PURPOSE: Writes a vector at specified texel
	// PARAMS:	x, y coordinates to set pixel at
	//			color - value to set at this texel
	// NOTES:	Only supports 4 element vector types currently
	void SetPixelVector(int x,int y,float color[4] );


	// PURPOSE: Do a texture lookup into the image given a u and v parameter
	// PARAMS:
	//	u - the width value of the desired texel
	//	v - the height value of the desired texel
	// RETURNS: A Color32 returning the specified pixel
	// NOTES: For now this function only implements a direct texel lookup without filtering
	//			See GetPixel for a list of supported texture formats that will return a valid color.
	Color32 Tex2D(float u, float v) const;

	// PURPOSE:	Child image in mip chain, or NULL if none
	inline grcImage *GetNext() const;

	// PURPOSE:	Does a bilinear scale on image to specified size
	// PARAMS:	newWidth - new width of image
	//			newHeight - new height of image
	// NOTES:	Only A8R8G8B8 images are supported.  Child mip levels
	//			are resized appropriately, but resizing a mip level
	//			smaller than 1x1 is an error.
	void Scale(int newWidth,int newHeight);

   // PURPOSE:	Does a bilinear scale on image to specified size
   //          Creates a new image that is a scaled version of the given srcImage
   // PARAMS:	newWidth - new width of image
   //			   newHeight - new height of image
   //          srcImage - image that the new image is scaled from
   // RETURNS: A new image that is a scaled version of the given srcImage
   // NOTES:	Only A8R8G8B8 images are supported.  Child mip levels
   //			are NOT resized
   static grcImage* CreateScaledImage(int newWidth,int newHeight , grcImage* srcImage);

	// RETURNS: Pointer to LUT if it exists
	// NOTES:	Same LUT is shared by all mip levels of an image.
	inline Color32* GetLut() const;

	// PURPOSE: Increment reference count on image
	inline void AddRef() const;

	// PURPOSE: Decrement reference count on image, freeing it when it reaches zero
	inline int Release() const;

	// PURPOSE: Flip the image vertically.
	void FlipVertical();

	// RETURNS: The pointer to the next image layer (NULL if not existant or not the highest mipmap level)
	inline grcImage *GetNextLayer() const;

	// RETURNS:	Number of layers in the image; this is computed, not stored.
	u32 GetLayerCount() const;

	// PURPOSE: Sets the minimum miplevel size we will load; miplevels with either axis below
	//			this size will be ignored (except that we still guarantee at least one miplevel) 
	// NOTES:	No effect on multi-layer images like cubemaps.
	static void SetMinMipSize(u32 minCount);

	static u32 GetMinMipSize();

	// PURPOSE: Sets the maximum number of mip levels we will load; miplevels below this number 
	//			will be ignored
	// NOTES:	No effect on multi-layer images like cubemaps.
	static void SetMaxMipLevels(u32 maxCount);

	static u32 GetMaxMipLevels();

	// PURPOSE:	Set maximum mip size, in bytes (256*1024, 512*1024, or 1024*1024 are typical values)
	// NOTES:	Not the same as area, since it takes the bits per pixel into account; zero means no limit.
	static void SetMaxMipBytes(u32 maxBytes);

	// RETURNS: Max number of bytes in a single miplevel, or zero if there's no limit.
	static u32 GetMaxMipBytes();

	// PURPOSE: Sets the minimum number of mip levels we will load (if available); this is useful in
	//			conjunction with SetMinMipSize() for example, to make sure small textures get at least
	//			some mip levels..
	// NOTES:	No effect on multi-layer images like cubemaps.
	static void SetMinMipLevels(u32 minCount);

	static u32 GetMinMipLevels();

	// PURPOSE: Sets the maximum miplevel size we will load; miplevels with either axis above
	//			this size will be ignored (except that we still guarantee at least one miplevel) 
	// NOTES:	No effect on multi-layer images like cubemaps.
	static void SetMaxMipSize(u32 maxCount);

	static u32 GetMaxMipSize();

	// PURPOSE: Sets the ratio of the texture size we will load; miplevels with either axis above
	//			the size (width or height) times the ratio will be ignored (except that we still guarantee at least one miplevel) 
	// NOTES:	No effect on multi-layer images like cubemaps.
	static void SetSizeRatio(float ratio);

	static float GetSizeRatio();

	// PURPOSE: Enable automatic conversion of 64bpp floating point to RGBE at loading time.
	static void SetRgbeConversionFlag(bool flag);

	// PURPOSE: Returns current state of conversion flag.
	static bool GetRgbeConversionFlag();

	// RETURNS:	True if image contains alpha (ie DXT3, DXT5, A8, or A8R8G8B8 with non-unit alpha channel).
	// NOTES:	If the image type is A8R8G8B8, we only scan the topmost miplevel, but the function
	//			should definitely have its result cached in higher-level code.
	bool HasAlpha() const;

	// PURPOSE: Enables automatic conversion from gamma 2.2 to 1.0 on load of textures
	static void SetUseSRGBTextures(bool flag = true)		{		sm_useSRGBTextures = flag;	}
	static bool GetUseSRGBTextures()						{		return sm_useSRGBTextures;	}

#if __PS3 && HACK_GTA4
	// PURPOSE: Permit / deny use of the table s_gamma when taking pictures via SaveStreamToJPEG
	static void SetUseGammaCorrection(bool flag = true)		{		sm_gammaCorrect = flag;	}
	static bool GetUseGammaCorrection()						{		return sm_gammaCorrect;	}
#endif

#if __RESOURCECOMPILER
	// PURPOSE: Enable storing up to 44 bytes of random data in the grcImage (assume pad0,pad1,pad2,m_ColorExp,m_ColorOfs are consecutive in memory)
	class grcImageConstructorInfo
	{
	public:
		char m_swizzle[5]; // 4 chars plus terminator, e.g. "RGB1"
		u8   m_virtualHdMips;
		bool m_bIsLdMipChain:1;
		bool m_forceContiguousMips:1; // xenon-only
		u32  m_debugTag; // used for debugging
		u32  m_overrideAlignment;
	};
	grcImageConstructorInfo& GetConstructorInfo()
	{
		#define OffsetOfNext(s,m) (OffsetOf(s,m) + sizeof(s::m))
		CompileTimeAssert(OffsetOfNext(grcImage,pad0)       <= OffsetOf(grcImage,pad1));
		CompileTimeAssert(OffsetOfNext(grcImage,pad1)       <= OffsetOf(grcImage,pad2));
		CompileTimeAssert(OffsetOfNext(grcImage,pad2)       <= OffsetOf(grcImage,m_ColorExp));
		CompileTimeAssert(OffsetOfNext(grcImage,m_ColorExp) <= OffsetOf(grcImage,m_ColorOfs));
		CompileTimeAssert(OffsetOfNext(grcImage,m_ColorOfs) >= OffsetOf(grcImage,pad0) + sizeof(grcImageConstructorInfo));
		#undef OffsetOfNext
		return *reinterpret_cast<grcImageConstructorInfo*>(&pad0);
	}
#endif


	// PURPOSE: Store  RGBA8 surface to JPEG in-place (it's assumed that it's safe to read from pSrc)
	//			On 360, pSrc is assumed to be tiled
	static int SaveRGBA8SurfaceToJPEG(fiStream* pStream, int quality, void* pSrc, int width, int height, int pitch, bool bCloseStreamAtEnd);
	static int SaveRGBA8SurfaceToJPEG(const char* filename, int quality, void* pSrc, int width, int height, int pitch);

	// PURPOSE: As above but takes RGB8.
	static int SaveRGB8SurfaceToJPEG(fiStream* pStream, int quality, void* pSrc, int width, int height, int pitch, bool bCloseStreamAtEnd);
	static int SaveRGB8SurfaceToJPEG(const char* filename, int quality, void* pSrc, int width, int height, int pitch);

	// PURPOSE: Load JPEG into an RGBA8 grcTexture 
	//			Storage is provided by the client code.
	static grcTexture* LoadJPEGToRGBA8Surface(fiStream* pStream, void* pDst, u32 desiredWidth, u32 desiredHeight, bool bIsSystemMemory);
	static grcTexture* LoadJPEGToRGBA8Surface(const char* filename, void* pDst, u32 desiredWidth, u32 desiredHeight, bool bIsSystemMemory);


	// PURPOSE: Load JPEG into a DXT1 grcTexture 
	//			Compression function (DXTCompressFuncType) is provided by the client code.
	//			Storage is provided by the client code.
	typedef bool (*DXTCompressFuncType)(void* pDst, const void* pSrc, u32 dstPitch, u32 srcPitch, u32 numDxtBlocksBatched);

	static grcTexture* LoadJPEGToDXT1Surface(fiStream* pStream, grcImage::DXTCompressFuncType func, void* pDst, u32 dstBufferSize, u32 desiredWidth, u32 desiredHeight, bool bIsSystemMemory, void* pScratchBuffer, u32 scratchBufferSize, u32 maxScanlinesPerSlice);
	static grcTexture* LoadJPEGToDXT1Surface(const char* filename, grcImage::DXTCompressFuncType func, void* pDst, u32 dstBufferSize, u32 desiredWidth, u32 desiredHeight, bool bIsSystemMemory, void* pScratchBuffer, u32 scratchBufferSize, u32 maxScanlinesPerSlice);


	// PURPOSE: Extracts dimension info from JPEG
	//			Can be used to determine memory requirements for LoadJPEGToRGBA8Surface
	static bool GetJPEGInfoForRGBA8Surface(fiStream* pStream, u32& width, u32& height, u32& pitch);
	static bool GetJPEGInfoForRGBA8Surface(const char* pFilename, u32& width, u32& height, u32& pitch);

	// PURPOSE: Used for writing a COM marker in SaveRGBA8SurfaceToJPEG
	static void SetComMarkerToSave(const char *pStringToSave);
	static void ClearComMarkerToSave();

	// PURPOSE: Used for reading a COM marker in LoadJPEGToRGBA8Surface
	static void SetComMarkerToLoad(ReadComMarkerCB ReadComMarkerCallback, u32 MaxLengthOfStringToLoad);
	static void ClearComMarkerToLoad();

	// PURPOSE: To be used before allocating the buffer that will be passed to LoadJPEGToRGBA8Surface
	//	Pass the width and height you'd like to use for the RGBA8Surface
	//	The closest valid width and height will be returned (along with the number of bytes that needs to be allocated to store the image)
	static u32 GetSizesOfClosestAvailableScaleDenom(u32 srcWidth, u32 srcHeight, u32& dstWidth, u32& dstHeight, u32 &dstTextureDataSize, bool bLocalMemory);

	// PURPOSE: Used to extract information from the header of a DDS file
	static void GetDDSInfoFromFile(const char* pFilename, grcImage::Format& format, u32& width, u32& height, u32& numMips, u32& offsetToPixelData, u32& pixelDataSize);

	// PURPOSE: Used to preprocess DDS pixel data per-platform
	static void ProcessDDSData(grcImage::Format format, void*  pData, u32 dataSize);

private:
	void RecalculateStride();

	u16 m_Width, m_Height;		// +0
	Format m_Format;			// +4
	ImageType m_Type;			// +8
	u16 m_Stride;				// +12
	u8 m_Depth;					// +14
	u8 m_StrideHi;				// +15
	u8 *m_Bits;					// +16
	Color32 *m_Lut;				// +20
	grcImage *m_Next;			// +24
	grcImage *m_NextLayer;		// +28
	mutable int m_RefCount;		// +32

	// NOTE -- resource compiler stores image constructor info here
	int pad0, pad1, pad2;		// +36,40,44
	// HDR texture components 
	Vector3 m_ColorExp;			// +48
	Vector3 m_ColorOfs; 

	static bool sm_RgbeConversion;
	static u32 sm_MinMipSize;
	static u32 sm_MaxMipSize;
	static u32 sm_MaxMipLevels;
	static u32 sm_MinMipLevels;
	static u32 sm_MaxMipBytes;
	static float sm_SizeRatio;
	static bool sm_useSRGBTextures;
#if __PS3 && HACK_GTA4
	static bool sm_gammaCorrect;
#endif

	static grcJpegSaveComMarker sm_SaveComMarker;
	static grcJpegLoadComMarker sm_LoadComMarker;

#if __RESOURCECOMPILER
public:

	typedef std::map<int, grcImage*> ImageList;
	
	// Callback type for multiple output texture loading.
	typedef bool (*CustomLoadFuncType)( const char* path, ImageList& images, void** outParams );

	static CustomLoadFuncType sm_customLoadFunc;
	static bool               sm_customLoad; // flag to indicate that image is being custom loaded, and should not have mips removed etc.
	static const char*        sm_platformString;

	static void SetCustomLoadFunc( CustomLoadFuncType func );
	static bool RunCustomLoadFunc( const char* path, ImageList& outputs, void** outParams = NULL );

	static bool SaveDDS(const char* path, const void* bits, int w, int h, Format format);
#endif // __RESOURCECOMPILER
};

inline grcImage::Format grcImage::GetFormat() const {
	return (Format)(m_Format & (u32)FORMAT_MASK);
}

inline bool grcImage::IsSRGB() const {
	return sm_useSRGBTextures && (m_Format & (u32)FORMAT_FLAG_sRGB) != 0;
}

inline bool grcImage::IsLinear() const {
	return (m_Format & (u32)FORMAT_FLAG_LINEAR) != 0;
}

inline bool grcImage::IsSysMem() const {
	return (m_Format & (u32)FORMAT_FLAG_SYSMEM) != 0;
}

#if __RESOURCECOMPILER
inline void grcImage::SetIsSysMem() {
	m_Format = (Format)((u32)m_Format | (u32)FORMAT_FLAG_SYSMEM);
}
#endif

inline grcImage::ImageType grcImage::GetType() const {
	return m_Type;
}

inline u16 grcImage::GetWidth() const {
	return m_Width;
}

inline u32 grcImage::GetStride() const {
	return m_Stride + ((u32)m_StrideHi << 16);
}

inline u16 grcImage::GetHeight() const {
	return m_Height;
}

inline u16 grcImage::GetDepth() const {
	return m_Depth;
}

inline u32 grcImage::GetArea() const {
	return m_Width * m_Height;
}

inline u32 grcImage::GetVolume() const {
	return m_Width * m_Height * m_Depth;
}

#if !__RESOURCECOMPILER
inline const Vector3& grcImage::GetColorExponent() const {
	return m_ColorExp;
}

inline const Vector3& grcImage::GetColorOffset() const {
	return m_ColorOfs;
}
#endif // !__RESOURCECOMPILER

inline u32 grcImage::GetSize() const {
	return m_Stride * GetPhysicalHeight() * m_Depth;
}

inline u8* grcImage::GetBits() const {
	return m_Bits;
}

inline u8* grcImage::GetBits(u32 slice, u32 mipLevel) const {
	const grcImage* mip = this;

	for (u32 i = 0; i < mipLevel; ++i)
	{
		mip = mip->GetNext();
		FastAssert(mip);
	}

	FastAssert(slice < mip->GetDepth());
	const u32 sliceSize = mip->GetStride() * mip->GetPhysicalHeight();
	return mip->GetBits() + sliceSize * slice;
}


inline Color32* grcImage::GetColor32() const {
	FastAssert(GetFormat() == A8R8G8B8);
	return (Color32*) m_Bits;
}

inline grcImage* grcImage::GetNext() const {
	return m_Next;
}

inline Color32* grcImage::GetLut() const {
	return m_Lut;
}

inline void grcImage::AddRef() const {
	++m_RefCount;
}

inline int grcImage::Release() const {
	FastAssert(m_RefCount);
	if (--m_RefCount == 0) {
		delete this;
		return 0;
	}
	else
		return m_RefCount;
}

inline grcImage *grcImage::GetNextLayer() const {
	return m_NextLayer;
}

inline void grcImage::SetRgbeConversionFlag(bool flag) {
	sm_RgbeConversion = flag;
}

inline bool grcImage::GetRgbeConversionFlag() {
	return sm_RgbeConversion;
}


}	// namespace rage

#endif
