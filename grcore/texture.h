//
// grcore/texture.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_TEXTURE_H
#define GRCORE_TEXTURE_H

#include "atl/array.h"
#include "atl/ownedptr.h"
#include "atl\pool.h"
#include "atl/queue.h"
#include "data/struct.h"
#include "effect_typedefs.h"
#include "grcore/device.h"
#include "grcore/effect.h"
#include "grcore/locktypes.h"
#include "grcore/stateblock.h"
#include "paging/base.h"
#include "paging/dictionary.h"
#include "vector/vector3.h"
#include "wrapper_gcm.h"			// Yeah this is ugly.

#if __BANK
#include "system/timer.h"
#endif

#define DYNAMIC_ESRAM (1 && RSG_DURANGO)

namespace rage {

class gfxTexture;
class gfxViewport;
class Matrix34;
struct grcResolveFlags;
class grcEffect;
class grcImage;
class grcFastMipMapper;

#define MAX_GLOBAL_TEXTURES 24

/* On PS2, trilinear filtering is expensive, so this boolean acts as a global enable / disable */
extern bool grcEnableTrilinear;

// PS2 only, for special use; defaults true; while false, it skips swizzling (useful for dynamic texture access)
extern bool grcEnableSwizzle;

// Keep this correct when loading multiple shadergroups from a single type file; necessary so that
// the same texture used in different shadergroups correctly gets unique tagfiles
extern int grcShaderGroupIndex;

// True (the default) if we should remember the texture names in each texture.
// Always off in the resource compiler.
extern bool grcSaveTextureNames;

enum DepthStencilFlags
{
	DepthStencilRW = 0x00,
	DepthReadOnly = 0x01,
	StencilReadOnly = 0x02,
	DepthStencilReadOnly = DepthReadOnly | StencilReadOnly,
};

struct grcTextureLock
{
	int MipLevel;
	void *Base;
	int Pitch;
	int BitsPerPixel;
	int Width, Height;
	int RawFormat;			// D3DFMT_... on D3D builds, or SCE_GS_PSM_... on PSX2.
	int Layer;				// Represents either face of cubemap or layer of volume texture
};

// Generic point class for specifying integer positions and offsets
struct grcPoint
{
	int x;
	int y;
};

// Generic rectangular structure
typedef struct grcRect {
	int x1;
	int y1;
	int x2;
	int y2;
} GRCRECT;

struct grcCellGcmTextureWrapper : CellGcmTexture
{
	inline void	SetWidth( uint16_t in) { width = in; }
	inline uint16_t GetWidth() const { return width; }; 
	inline void	SetHeight( uint16_t in) { height = in; }
	inline uint16_t GetHeight() const { return height; }; 
	inline void	SetDepth( uint16_t in) { depth = in; }
	inline uint16_t GetDepth() const { return depth; }; 

	inline void	SetFormat( uint8_t in) { format = in; }
	inline uint8_t GetFormat() const { return format; }; 
	inline void	SetDimension( uint8_t in) { dimension = in; }
	inline uint8_t GetDimension() const { return dimension; }; 
	inline void	SetMipMap( uint8_t in) { mipmap = in; }
	inline uint8_t GetMipMap() const { return mipmap; }; 
	inline void	SetBindFlag( uint8_t in) { _padding = in; }
	inline uint8_t GetBindFlag() const { return _padding; }; 
	inline void	SetTileMode( uint8_t in) { location = in; }
	inline uint8_t GetTileMode() const { return location; }; 
	inline void	SetImageType( uint8_t in) { cubemap = in; }
	inline uint8_t GetImageType() const { return cubemap; }; 

	inline void	SetOwnsMem( uint32_t in) { remap = in; }
	inline uint32_t GetOwnsMem() const { return remap; }; 
	inline void	SetUsesPreAllocatedMem( uint32_t in) { pitch = in; }
	inline uint32_t GetUsesPreAllocatedMem() const { return pitch; }; 
};


/*
PURPOSE
	grcTexture is the base class of all textures in grcore.  It is a pure virtual interface which
	has platform-specific subclasses created through factory interfaces.  
<FLAG Component>
*/
class grcTexture: public pgBase {
#if __TOOL
public:
#else
protected:
#endif
	virtual ~grcTexture();
public:
	static const int RORC_VERSION = 13;

	static int sm_MemoryBucket;

	grcTexture(u8 type = NORMAL);
	grcTexture(class datResource &);

	/* RETURNS: Human-readable name of texture, useful for debugging */
	const char *GetName() const { return m_Name; }

	/* PURPOSE: Ensures that texture is available in VRAM; used on PS2 for special multipass effects */
	virtual void Download() const { }

	/* PURPOSE: Returns the number of blocks of video memory necessary to make this texture
		resident at the current level of detail.  Will return zero on platforms that don't have
		dedicated texture memory, and will also return zero if the texture is already resident. */
	virtual int GetDownloadSize() const { return 0; }

	/* RETURNS: Width of the texture */
	virtual int GetWidth() const = 0;

	/* RETURNS: Height of the texture */
	virtual int GetHeight() const = 0;

	/* RETURNS: Depth of the texture */
	virtual int GetDepth() const = 0;

	/* RETURNS: Mip map count of the texture */
	virtual int GetMipMapCount() const = 0;

	virtual int GetArraySize() const = 0;

	/* RETURNS: Bits per pixel of the texture, or zero if unknown */
	virtual int GetBitsPerPixel() const = 0;

	/* RETURNS: If texture is a valid texture - Only good for PC */
	virtual bool IsValid() const { return false; }

	/* RETURNS: MSAA mode of the texture (number of samples and fragments per pixel) */
	virtual grcDevice::MSAAMode GetMSAA() const { return grcDevice::MSAA_None; }

	/* PURPOSE: Delete texture */
	//void Delete() { delete this; }

	/* RETURNS: Is automatic SRGB gamma correction enabled for the texture */
	virtual bool IsGammaEnabled() const { return false; }

	/* PURPOSE: Enables\disables automatic SRGB gamma correction for the texture */
	virtual void SetGammaEnabled(bool /*enabled*/) { }

	/* RETURNS: Mask of which elements of the texture are signed (as per COLORWRITEENABLE_ types) */
	virtual u32 GetTextureSignedMask() const { return 0; }

	/* RETURNS: Sets which elements of the texture are signed (as per COLORWRITEENABLE_ types) */
	/* NOTE: Not safe to call on a texture that may be in use in a different thread */
	virtual void SetTextureSignedMask(u32 /*mask*/) { }

	/* RETURNS: The original texture if this texture is actually a reference to another texture.
	This is used to attach textures in resources to externally defined textures that were
	registered via grcTextureFactory::RegisterTextureReference */
	virtual grcTexture* GetReference();

	// RETURNS:	True if texture is in SRGB format (and needs adjustment during read)
	virtual bool IsSRGB() const { return false; }

	virtual const grcTexture* GetReference() const;

	static void InitFastMipMapper();

#if BASE_DEBUG_NAME
	virtual const char *GetDebugName(char * /*buffer*/, size_t /*bufferSize*/) const		{ return GetName(); }
#endif // BASE_DEBUG_NAME


	/* Global texture used whenever "no texture" is desired.  Internally implemented as a
		small white splotch. */
	static const grcTexture* None;
	static const grcTexture* NoneDepth; // 4x4 R32F depth texture filled with 1.f
	static const grcTexture* NoneBlack; // 4x4 DXT1 texture with rgb=0, alpha=255
	static const grcTexture* NoneBlackTransparent; //4x4 DXT1 texture with rgb=0, alpha=0
	static const grcTexture* NoneWhite; // 4x4 DXT1 texture with rgb=255, alpha=255

	/* Global texture used whenever "missing textured" is desired.  Internally implemented as a
		small partially dst white splotch. */
	static const grcTexture* Nonresident;

	// Used during resource construction
	enum { 
		NORMAL,					// Standard texture
		RENDERTARGET,			// Render target
		REFERENCE,				// Reference to an existing texture
		DICTIONARY_REFERENCE,	// Reference to an existing texture in a dictionary
	};

	// Used during resource construction
	int GetResourceType() const { return m_ResourceTypeAndConversionFlags & 3; }

	// Used for debugging (texture viewer)
	inline u8 GetPrivatePadding() const { return m_Texture._padding; }

	// Returns Direct3D surface for ID3DXEffect integration
	// Returns a texture handle object for OpenGL builds
	virtual grcTextureObject *GetTexturePtr() { return NULL; }
	virtual const grcTextureObject *GetTexturePtr() const { return NULL; }

	virtual grcDeviceView *GetTextureView() { return NULL; }
#if RSG_PC
	virtual void UpdateGPUCopy() {}
#endif // RSG_PC
	virtual u32 GetImageFormat() const { return 0; } // cast this to grcImage::Format

	grcTextureObject *GetCachedTexturePtr() const { 
		if (!this) return NULL;				// Avoids complexity in higher-level code
#if __WIN32PC
		// On PC, we can routinely lose texture surfaces due to RAG or Alt+Enter, etc, so the
		// caching won't work right if you take a reference to a render target.  A better solution 
		// would be to track which textures are referenced and notify their upstream referencers.
		return const_cast<grcTextureObject*>(GetTexturePtr());
#else
		Assert(m_CachedTexturePtr == GetTexturePtr()); 
		return m_CachedTexturePtr; 
#endif
	}

	CellGcmTexture& GetGcmTexture() { return m_Texture; }
	void UpdatePackedTexture();

	/* PURPOSE:	Locks the specified miplevel of a texture for update.
	   PARAMS:	layer - The layer of the texture to lock (only set to nonzero for cubic or depth textures)
				mipLevel - Mip level of the texture we wish to lock
				lock - Structure which is filled with data about the lock
	   RETURNS:	True on success, false if not supported.
	   NOTES:	If LockRect returns true, you need to call UnlockRect eventually with
				the exact same layer and miplevel parameter */
	virtual bool LockRect(int /*layer*/, int /*mipLevel*/,grcTextureLock &/*lock*/, u32 /*uLockFlags*/ = (grcsRead | grcsWrite)) const { return false; }

	/* PURPOSE:	Unlocks the specified miplevel of a texture
	   PARAMS:	lock - The lock structure that had been returned by LockRect.
					Currently only MipLevel and Layer has to be preserved. */
	virtual void UnlockRect(const grcTextureLock &/*lock*/) const { }

	/* PURPOSE: Swizzles a 2D texture before it gets unlocked. 
	Note: this operation can be very slow.
	PARAMS:	lock - The lock structure that had been returned by LockRect. */
	virtual void SwizzleTexture2D(const grcTextureLock &/*lock*/) const { }

	/* PURPOSE: Unswizzles a 2D texture before it gets unlocked. 
	Note: this operation can be very slow.
	PARAMS:	lock - The lock structure that had been returned by LockRect. */
	virtual void UnswizzleTexture2D(const grcTextureLock &/*lock*/) const { }

	/*	PURPOSE
	Create a texture based on the supplied width and height.
	PARAMS
	width - width of the image
	height - width of the image
	format - format of the image (D3D format for textureXenon)
	*/
	virtual void Resize(u32 /*width*/, u32 /*height*/){}

	DECLARE_PLACE(grcTexture);

	// RETURNS: The number of layers for this texture
	inline u32 GetLayerCount() const;
	
    /* PURPOSE: this function is called when an effect is selecting this texture.
                the idea is that through this function grcTexture gets a chance
                to know in which effect(s) this texture is used.
                this can be helpful if you had a streamable-texture that would
                update the texture-pointer in all effects that use this texture,
                once the texture has become resident. */
    virtual void SetEffectInfo(grcEffect*, grcEffectVar) const { }

	// Helpful enums used for identifying layers (in particular, cube map faces)
	enum {
		CUBE_POS_X,		// Positive X face of cubemap
		CUBE_NEG_X,		// Negative X face of cubemap
		CUBE_POS_Y,		// Positive Y face of cubemap
		CUBE_NEG_Y,		// Negative Y face of cubemap
		CUBE_POS_Z,		// Positive Z face of cubemap
		CUBE_NEG_Z		// Negative Z face of cubemap
	};

#if __BANK
	virtual void AddWidgets(class bkBank&) { }
	virtual bool Save() { return false; }
#endif

#if __DECLARESTRUCT
	virtual void DeclareStruct(class datTypeStruct &s);
#endif

	virtual void CreateMipMaps(const grcResolveFlags* /*resolveFlags*/,  int /*index*/ = 0) {};
	virtual void Blur(const grcResolveFlags* /*resolveFlags*/) {};

	// PURPOSE:	Update a texture of the same size and format with a new image
	// PARAMS:	pImage - Image to copy in, format must match
	// RETURNS:	True on success, false on failure (format or size mismatch)
	virtual bool Copy(const grcImage *pImage) = 0;
	virtual bool Copy(const grcTexture* /*pTexture*/, s32 /*dstSliceIndex*/ = -1, s32 /*dstMipIndex*/ = -1, s32 /*srcSliceIndex*/ = 0, s32 /*srcMipIndex*/ = 0) { return false; }
	virtual bool Copy(const void * /*pvSrc*/, u32 /*uWidth*/, u32 /*uHeight*/, u32 /*uDepth*/) { return false; }
	virtual bool Copy2D(const void * /*pSrc*/, u32 /*imgFormat*/, u32 /*uWidth*/, u32 /*uHeight*/, u32 /*numMips*/) { return false; }
	virtual bool Copy2D(const void * /*pSrc*/, const grcPoint& /*oSrcDim*/, const grcRect& /*oDstRect*/, const grcTextureLock& /*lock*/, s32 /*iMipLevel*/ = 0) { Assert(0 && "Not Implemented"); return false; } 

	// PURPOSE: Used on 360 to tile a linear texture
	virtual void Tile(int) {}

	static const u32 TEXTURE_TEMPLATE_TYPE_LO_SIZE	= 7;
	static const u32 TEXTURE_TEMPLATE_TYPE_HI_SIZE	= 5;
	static const u32 TEXTURE_TEMPLATE_TYPE_LO_SHIFT	= 0;
	static const u32 TEXTURE_TEMPLATE_TYPE_HI_SHIFT	= 32 - TEXTURE_TEMPLATE_TYPE_HI_SIZE;
	static const u32 TEXTURE_TEMPLATE_TYPE_LO_MASK	= ((1UL << TEXTURE_TEMPLATE_TYPE_LO_SIZE) - 1) << TEXTURE_TEMPLATE_TYPE_LO_SHIFT;
	static const u32 TEXTURE_TEMPLATE_TYPE_HI_MASK	= ((1UL << TEXTURE_TEMPLATE_TYPE_HI_SIZE) - 1) << TEXTURE_TEMPLATE_TYPE_HI_SHIFT;

	static const u32 PHYSICAL_SIZE_MASK = ~(TEXTURE_TEMPLATE_TYPE_LO_MASK | TEXTURE_TEMPLATE_TYPE_HI_MASK);

#if __BANK || __RESOURCECOMPILER
	inline u16 GetTemplateType() const {
		const u16 lo = (u16)((m_PhysicalSizeAndTemplateType & TEXTURE_TEMPLATE_TYPE_LO_MASK) >> TEXTURE_TEMPLATE_TYPE_LO_SHIFT);
		const u16 hi = (u16)((m_PhysicalSizeAndTemplateType & TEXTURE_TEMPLATE_TYPE_HI_MASK) >> TEXTURE_TEMPLATE_TYPE_HI_SHIFT) << TEXTURE_TEMPLATE_TYPE_LO_SIZE;
		return lo | hi;
	}
	inline void SetTemplateType(u16 type) {
		const u32 lo = (((u32)type) << TEXTURE_TEMPLATE_TYPE_LO_SHIFT) & TEXTURE_TEMPLATE_TYPE_LO_MASK;
		const u32 hi = (((u32)type) << (TEXTURE_TEMPLATE_TYPE_HI_SHIFT - TEXTURE_TEMPLATE_TYPE_LO_SIZE)) & TEXTURE_TEMPLATE_TYPE_HI_MASK;
		m_PhysicalSizeAndTemplateType = lo | hi | (m_PhysicalSizeAndTemplateType & PHYSICAL_SIZE_MASK);
	}
#endif // __BANK || __RESOURCECOMPILER

	inline u32 GetPhysicalSize() const { return m_PhysicalSizeAndTemplateType & PHYSICAL_SIZE_MASK; }
	inline void SetPhysicalSize(u32 size) {	/*Assertf((size & ~PHYSICAL_SIZE_MASK) == 0, "size=0x%08x", size);*/ m_PhysicalSizeAndTemplateType = ((size + 127)&~127) | (m_PhysicalSizeAndTemplateType & ~PHYSICAL_SIZE_MASK); }

	enum {
		CHANNEL_ALPHA,
		CHANNEL_RED,
		CHANNEL_GREEN,
		CHANNEL_BLUE,
		CHANNEL_DEPTH,
		CHANNEL_STENCIL
		// That's all we check for right now - could add other channels later
	};
	typedef atFixedBitSet8 ChannelBits;

	virtual ChannelBits FindUsedChannels() const = 0;

	static ChannelBits FindUsedChannelsFromRageFormat(u32 rageFormat);

	void AddRef() const {
		sysInterlockedIncrement_NoWrapping(&m_RefCount);
	}

	void DecRef() const {
		sysInterlockedDecrement_NoWrapping(&m_RefCount);
	}

	int Release() const {
		Assert(m_RefCount);
		int result = (int) sysInterlockedDecrement_NoWrapping(&m_RefCount);
		if (result == 0) {
			delete this;
			return 0;
		}
		else
			return result;
	}

	int GetRefCount() const { return (int) m_RefCount; }

	u8   GetConversionFlags() const { return m_ResourceTypeAndConversionFlags & ~3; }
	void SetConversionFlags(u8 flags) { m_ResourceTypeAndConversionFlags = (m_ResourceTypeAndConversionFlags & 3) | (flags & ~3); }

	u32 GetHandleIndex() const { return m_HandleIndex; }
	void SetHandleIndex(u32 i) { m_HandleIndex = i; }
	void SetName(const char* pName) { Assert(m_Name == NULL); m_Name = grcSaveTextureNames ? StringDuplicate(pName) : 0;}

protected:
	grcCellGcmTextureWrapper m_Texture;							// 24 bytes.  Really ought to have 360 data here too, but it's 13 words instead of only 6.  Would fill out the cache line nicely though.
	const char				*m_Name;							// +32
	mutable u16				m_RefCount;							// +36
	u8						m_ResourceTypeAndConversionFlags;	// +38
	u8						m_LayerCount;						// This is actually one less that the real layer count so that we can store 256.
	datPadding64(4,m_pad64)
	grcTextureObject		*m_CachedTexturePtr;				// +40
	u32						m_PhysicalSizeAndTemplateType;		// +44
	u32						m_HandleIndex;						// +48

	static grcFastMipMapper	sm_FastMipMapper;

#if __RESOURCECOMPILER
public:
	typedef bool (*CustomProcessFuncType)(grcTexture* texture, const void* params);
	static void SetCustomProcessFunc(CustomProcessFuncType func);
	bool        RunCustomProcessFunc(const void* params);
private:
	static CustomProcessFuncType sm_customProcessFunc;
#endif // __RESOURCECOMPILER

public:
	static void	SetSuppressReferenceWarning();
	static void	ClearSuppressReferenceWarning();

#if __BANK || __RESOURCECOMPILER
	typedef grcTexture* (*CustomLoadFuncType)(const char* filename);
	static void        SetCustomLoadFunc(CustomLoadFuncType func, const char* txdExtension);
	static grcTexture* RunCustomLoadFunc(const char* filename);
	static void        SetCustomLoadName(const char* name);
	static const char* GetCustomLoadName(const char* name);
	static const char* GetTxdExtension();
	grcTexture*        Clone(const char* name) const;
private:
	static CustomLoadFuncType sm_customLoadFunc;
	static const char*        sm_customLoadName;
	static const char*        sm_txdExtension;
#endif // __BANK || __RESOURCECOMPILER

public:
	enum eTextureSwizzle // swizzle means "remap" in PS3-land
	{
		TEXTURE_SWIZZLE_R,
		TEXTURE_SWIZZLE_G,
		TEXTURE_SWIZZLE_B,
		TEXTURE_SWIZZLE_A,
		TEXTURE_SWIZZLE_0,
		TEXTURE_SWIZZLE_1,
	};

	// this only affects console textures
	virtual void SetTextureSwizzle(eTextureSwizzle   , eTextureSwizzle   , eTextureSwizzle   , eTextureSwizzle, bool) {}
	virtual void GetTextureSwizzle(eTextureSwizzle& r, eTextureSwizzle& g, eTextureSwizzle& b, eTextureSwizzle& a) const
	{
		r = TEXTURE_SWIZZLE_R;
		g = TEXTURE_SWIZZLE_G;
		b = TEXTURE_SWIZZLE_B;
		a = TEXTURE_SWIZZLE_A;
	}

	static eTextureSwizzle ApplyToExistingSwizzle(eTextureSwizzle swizzle, const eTextureSwizzle existing[4])
	{
		switch (swizzle)
		{
		case TEXTURE_SWIZZLE_R : return existing[0];
		case TEXTURE_SWIZZLE_G : return existing[1];
		case TEXTURE_SWIZZLE_B : return existing[2];
		case TEXTURE_SWIZZLE_A : return existing[3];
		case TEXTURE_SWIZZLE_0 : break;
		case TEXTURE_SWIZZLE_1 : break;
		}

		return swizzle;
	}

	inline Color32 Swizzle(const Color32& c) const
	{
		grcTexture::eTextureSwizzle swizzle[4];
		GetTextureSwizzle(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);
		return Swizzle(c, swizzle);
	}

	static inline Color32 Swizzle(const Color32& c, const grcTexture::eTextureSwizzle swizzle[4])
	{
		int temp[4] = {0, 0, 0, 0};

		for (int i = 0; i < 4; i++) // r,g,b,a
		{
			switch (swizzle[i])
			{
			case TEXTURE_SWIZZLE_R : temp[i] = c.GetRed  (); break;
			case TEXTURE_SWIZZLE_G : temp[i] = c.GetGreen(); break;
			case TEXTURE_SWIZZLE_B : temp[i] = c.GetBlue (); break;
			case TEXTURE_SWIZZLE_A : temp[i] = c.GetAlpha(); break;
			case TEXTURE_SWIZZLE_0 : temp[i] = 0x00; break;
			case TEXTURE_SWIZZLE_1 : temp[i] = 0xff; break;
			}
		}

		return Color32(temp[0], temp[1], temp[2], temp[3]);
	}

	inline Vec4V_Out Swizzle(Vec4V_In c) const
	{
		grcTexture::eTextureSwizzle swizzle[4];
		GetTextureSwizzle(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);
		return Swizzle(c, swizzle);
	}

	static inline Vec4V_Out Swizzle(Vec4V_In c, const grcTexture::eTextureSwizzle swizzle[4])
	{
		ScalarV temp[4];

		for (int i = 0; i < 4; i++) // r,g,b,a
		{
			switch (swizzle[i])
			{
			case TEXTURE_SWIZZLE_R : temp[i] = c.GetX(); break;
			case TEXTURE_SWIZZLE_G : temp[i] = c.GetY(); break;
			case TEXTURE_SWIZZLE_B : temp[i] = c.GetZ(); break;
			case TEXTURE_SWIZZLE_A : temp[i] = c.GetW(); break;
			case TEXTURE_SWIZZLE_0 : temp[i] = ScalarV(V_ZERO); break;
			case TEXTURE_SWIZZLE_1 : temp[i] = ScalarV(V_ONE); break;
			}
		}

		return Vec4V(temp[0], temp[1], temp[2], temp[3]);
	}
};

// Type of render target you want to create
enum grcRenderTargetType 
{ 
	grcrtFrontBuffer,		// Returns render target which is attached to front buffer
	grcrtBackBuffer,		// Returns render target which is attached to back buffer
	grcrtDepthBuffer,		// Create a new render target that is treated as a depth buffer (is NOT attached to system depth buffer)
	grcrtPermanent,			// Uses vram starting from top of memory on PS2.  Safe to allocate and destroy within a frame now.
	grcrtCubeMap,
	grcrtArrayMap = grcrtCubeMap,
	grcrtShadowMap,			// Shadow map support ... especially important on the PS3 
							// to retrieve depth values from the depth buffer you specify a A8R8G8B8 texture 
							// instead of DEPTH24_D8 by exchanging the texture format "behind the back of the GPU"
							// if you use a shadow map, you stay with the DEPTH24_D8 format, which will give you
							// 0 or 1 values depending on the depth comparison
	grcrtDepthBufferCubeMap,  // a cubemap of depth buffers

	grcrtVolume,			// Not supported on PS3 yet!

	grcrtCount
};

enum {kRTPoolIDInvalid = 0xffff, kRTPoolIDAuto=0xfffe};
enum {kRTPoolHeapInvalid = 0xff};

/*
PURPOSE
	grcRenderTarget is a special texture which can be rendered to like a frame buffer
<FLAG Component>
*/
class grcRenderTarget: public grcTexture {
public:
	virtual void Lock() {}
	virtual void Unlock() {}
	virtual void Realize(const grcResolveFlags * /*flags*/ = NULL,int /*index*/ = 0) { }
	virtual void Unrealize() { }
	grcRenderTarget() : grcTexture(grcTexture::RENDERTARGET), m_Type(grcrtPermanent), m_PoolID( kRTPoolIDInvalid), m_PoolHeap(0), m_Allocated(false), m_OffsetInPool(0) {}
	virtual ~grcRenderTarget();

	grcRenderTargetType GetType() const { return m_Type; }
	
	virtual void AllocateMemoryFromPool() {}
	virtual void ReleaseMemoryToPool() {}

	virtual void Untile(int) { }

	// anti-aliased render targets might have an extra render target as
	// a back buffer for the MSAA blit
	virtual const grcRenderTarget *GetAABufferPtr() const { return NULL; }
	virtual grcRenderTarget *GetAABufferPtr() { return NULL; }

	virtual u8		GetSurfaceFormat() const { return 0; }
	virtual u32		GetRequiredMemory() const { return 0; }

	virtual bool	IsSwizzled() const { return false; }

	bool			Copy(const grcImage * /*pImage*/) { return false; }
	virtual bool	Copy(const grcTexture* /*pTexture*/, s32 /*dstSliceIndex*/ = -1, s32 /*dstMipIndex*/ = -1, s32 /*srcSliceIndex*/ = 0, s32 /*srcMipIndex*/ = 0) { return false; }

	virtual bool	IsValid() const { return false; }


	bool			IsAllocated() const { return m_Allocated; }
	u16				GetPoolID() const {return m_PoolID;}
	u8				GetPoolHeap() const {return m_PoolHeap;}
	int				GetPoolOffset() const {return m_OffsetInPool;}

	// 
	// Update the memory location of a rendertarget based on a texture object.
	// Usefull only for rendertarget created using a base texture, where there's a chance the base texture has moved (defrag anyone ?).
	virtual void	UpdateMemoryLocation(const grcTextureObject *) {} 
	
#if __BANK
	static void		RequestSaveLog();	// request the next frame's render target usage be save to a log
	static void		AddLoggingWidgets(bkBank& bk);	// AddWidgets is already a virtual function, but I want a static version...
	static void		BeginFrame();
	static void		EndFrame();
	static void		LogTexture(const char* operation, const grcRenderTarget* tex);

	virtual bool	SaveTarget(const char * UNUSED_PARAM(pszOutName) = NULL) const { return false; }
#else
	inline static void	LogTexture(const char*, const grcRenderTarget*) {}
#endif

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	virtual void	GenerateMipmaps() = 0;
#endif

#if BASE_DEBUG_NAME && __ASSERT
	bool ExpectDuplicatesByName() const { return true; }
#endif

protected:
	grcRenderTargetType	m_Type;

	u16			m_PoolID;
	u8			m_PoolHeap;
	bool		m_Allocated;	
	int			m_OffsetInPool;  // only needed for render targets that use pool memory, but skip the heap allocate to specify a specific location

	u8	m_ArraySize;
	u8	m_PerArraySliceRTV;
#if __BANK
protected:
	static char	sm_LogName[256];			// name of the rendertarget log file
	static bool sm_EnableLoggingNextFrame;	// we want to start at the top of the frame.
	static utimer_t sm_StartTicks;			// the frame start time value
	static fiStream * sm_LogStream;			// the stream to write the log to
#endif

};



class grcRTPoolCreateParams
{
public:
	grcRTPoolCreateParams(int size=0) :	m_Size(size),
#if __GCM
								m_Alignment(16),
								m_Pitch(0),
								m_Type(grcrtPermanent),
								m_MSAA(grcDevice::MSAA_None),
								m_BitDepth(32),
								m_Compression(0xff),
								m_PhysicalMem(true),
								m_Tiled(true),
								m_Swizzled(false),
								m_Zculled(true),
#endif // __PS3
								m_MaxTargets(32),
								m_HeapCount(1),
								m_InitOnCreate(true),
								m_AllocOnCreate(true)
	{}

public:
	u32						m_Size;				// total memory size for the pool

#if __GCM
	u32						m_Alignment;		// the alignment we need for this pool
	u16						m_Pitch;			// the requested Pitch for the pool (must be greater or equal to the largest pitch of any target that will use the pool)
	grcRenderTargetType		m_Type;				// the type of textures that will be allocated from the pool, this is needed to determine the compression mode. mostly for just color or depth
	grcDevice::MSAAMode		m_MSAA;				// The MSAA type for the pool, this is needed to determine the compression mode.
	u8						m_BitDepth;			// the bit depth of targets in this pool (for pitch and compression calculation)
	u8						m_Compression;		// compression type, 0x0 for none, 0xff for auto detect based on type, etc.

	bool					m_PhysicalMem:1;	// should it be in physical memory?
	bool					m_Tiled:1;			// should it be tiled?
	bool					m_Swizzled:1;		// are we going to swizzle the targets in this pool, this is needed for pitch size calc and tiling info
	bool					m_Zculled:1;		// should it be in zcull memory
#endif

	u16						m_MaxTargets;		// the maximum number of targets that can be allocated from this pool
	u8						m_HeapCount;		// how many allocation heap should we have. should be 1 unless being used in compatibility mode.
	bool					m_InitOnCreate:1;	// should it init the texture memory for the pool(assign it to tiling, etc) when we create it
	bool					m_AllocOnCreate:1;	// 
};


class sysExternalHeap;

class grcRenderTargetPoolEntry
{
public:

#if __XENON
	static const int kDefaultAlignment = 4096;
#elif __PPU
	static const int kDefaultAlignment = 65536;
#else
	static const int kDefaultAlignment = 1;
#endif

	/*	PURPOSE
		create a render target pool entry.
		PARAMS
		name - human readable name for the pool
		params - the render target set up parameters. these include the requested size, alignment, and tiling/compression flags for the PSN
		buffer - a provided memory buffer to use for the pool, instead off allocating new memory.

		NOTES: on the PS3, rendertargets that are created from this pool, MUST match the flags set during initialization, or the creation will assert and fail
	*/
	grcRenderTargetPoolEntry(const char *name, const grcRTPoolCreateParams & params, void * buffer=NULL);
	~grcRenderTargetPoolEntry();

	void InitializeMemory(const grcRTPoolCreateParams & params);  // platform specif texture memory setup.

	void * AllocateTextureMem(u8 heapId, int size, int offset, int alignment = kDefaultAlignment);
	void  FreeTextureMem(void * ptr, u8 heapId);
	
private:
	void AllocatePoolMemory(u32 size, bool physical, int alignment, void * buffer);
	void FreePoolMemory();

public:
	void *							m_PoolMemory;		// the memory buffer for this pool
	atArray <sysExternalHeap * >	m_PoolHeaps;		// heap to track render target allocations
	atArray <u8 * >					m_PoolHeapWorkspace;// used by the m_PoolHeaps
	int								m_LowestMemFree;	// the lowest amount of free memory during any frame for any heap of this pool

	int								m_Size;				// total memory size for the pool
	u16								m_Pitch;			// the pitch of the pool memory

	u8								m_TiledIndex;		// the index of out tiled memory, -1 if not tiled
	u8								m_Compression;		// the type of compress we are using for this pool.

	u8								m_BitDepth;         // so we can verify the compress is correct for targets added
	u8								m_MSAA;				// also so we can verify compression.

#if __GCM
	bool							m_IsPhysicalMem:1;	// are we stored in physical memory?
	bool							m_IsInTiledMem:1;	// are we stored in physical memory?
	bool							m_IsColorTarget:1;	// are we a color target?
#endif
	bool							m_IsInitialised:1;	// have we been initialized
	bool							m_AllocatedMemory:1;// did we allocate the memory pointed to by m_PoolMemory
	
	const char*						m_Name;				// the name of this pool (NULL in final builds)
};

class grcRenderTargetPoolMgr
{
public:
	friend class grcRenderTarget;
	friend class grcRenderTargetGCM;

	/*	PURPOSE
		initialize the render target pool system. It will be call during grcDevice setup, if not call earlier by the Game code. 
		PARAMS
		maxPools - the max number of render target pools allowed
	*/
	
	static void Init(int maxPools=64);
	static void Shutdown();


	/*	PURPOSE
		Allocated a pool of main texture memory for shading between multi rendertargets (or collections of rendertargets) 
		PARAMS
		Name - An easy to read name for the pool (really helps during debugging)
		params - the pool creation params, most are needed only on PSN, do to extreme restrictions on pools there.
		buffer - a provided memory buffer to use for the pool, instead off allocating new memory.
	*/
	static u16 CreatePool(const char* name, const grcRTPoolCreateParams & params, void * buffer=NULL);

	/*	PURPOSE
		Destroy a render target memory pool.
		NOTES
		The ref count should be 0 or this will throw an assert/error message.
		PARAMS
		poolID - The Id of the pool to destroy
	*/
	static void DestroyPool(u16 poolID);

	/*	PURPOSE
		These functions query the specified memory pool
		
		PARAMS
		poolID - The Id of the pool to query
		heapID - option heap ID for pools with overlapping heaps
	*/
	static const char * GetName(u16 poolID)		{ if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID))->m_Name; else return NULL;}
	
	static int GetFreeMem(u16 poolID, u8 heapID=0);
	static int GetUsedMem(u16 poolID, u8 heapID=0);
	static int GetLargestFreeBlock(u16 poolID, u8 heapID=0);
	static void* GetPoolMemory(u16 poolID)		{ if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID))->m_PoolMemory; else return NULL;}
	static u32 GetAllocated(u16 poolID)			{ if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID))->m_Size;	else return 0;}

	// track the lowest amount of free memory we've encountered
	static int GetLowestFreeMem(u16 poolID)		{if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID))->m_LowestMemFree;	else return 0;}

	static bool VerifyPoolID(u16 ASSERT_ONLY(poolID))
	{
#if __ASSERT
		return Verifyf(!sm_PoolEntries.IsInFreeList(sm_PoolEntries.GetElemByIndex(poolID)),"Invalid rendertarget pool ID (%d,%p,%p)",poolID,sm_PoolEntries.GetElemByIndex(poolID),sm_PoolEntries.GetElemByIndex(poolID) ? *sm_PoolEntries.GetElemByIndex(poolID) : NULL);
#else
		return true;
#endif
	}

	static bool VerifyHeapID(u16 poolID, u8 heapID)
	{
		return Verifyf(heapID < (*sm_PoolEntries.GetElemByIndex(poolID))->m_PoolHeaps.GetCount(), "Invalid rendertarget pool Heap ID for Pool \"%s\" (%d, max is %d)",(*sm_PoolEntries.GetElemByIndex(poolID))->m_Name, heapID,(*sm_PoolEntries.GetElemByIndex(poolID))->m_PoolHeaps.GetCount());
	}

	static bool VerifyPoolAndHeapIDs(u16 poolID, u8 heapID)
	{
		return VerifyPoolID(poolID) && VerifyHeapID(poolID, heapID);
	}
	


#if __PPU
	static u32	GetMemoryPoolPitch(u16 poolID)	{ if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID))->m_Pitch; else return 0;}
	static bool	GetIsMemoryPoolTiled(u16 poolID){ if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID))->m_IsInTiledMem; else return 0;}
	static u8	GetTiledIndex(u16 poolID)		{ if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID))->m_TiledIndex; else return 0;}
	static bool	GetIsColorTarget(u16 poolID)	{ if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID))->m_IsColorTarget; else return 0;}
	static u8	GetCompression(u16 poolID)		{ if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID))->m_Compression; else return 0;}
	static bool	GetIsPhysical(u16 poolID)		{ if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID))->m_IsPhysicalMem; else return 0;}
#endif	//__PPU...

	static bool	GetIsInitialised(u16 poolID)	{ if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID))->m_IsInitialised; else return 0;}
	static u8	GetHeapCount(u16 poolID)		{ if (VerifyPoolID(poolID)) return (u8)(*sm_PoolEntries.GetElemByIndex(poolID))->m_PoolHeaps.GetCount(); else return 0;}


	/*	PURPOSE
		allocate texture memory from the active rendertarget pool

		PARAMS
			poolID - The Id of the pool to query
			heapID - heap to allocate from
			size - size of texture memory needed
			offset - offset, used if heap is kInvalidRTPoolHeap, to speficy a specifi offset in to the pool (use with caution!)
			alignment - requested alignment
		
		RETURNS
			Returns a pointer to the texture memory requested, or NULL if there was not enough memory available in the pool
	*/
	static void * AllocateTextureMem(u16 poolID, u8 heapID, int size, int offset, int alignment = grcRenderTargetPoolEntry::kDefaultAlignment);
	
	/*	PURPOSE
		free texture memory allocated from a pool 

		PARAMS
			poolID - The Id of the pool to query
			heapID - heap to free from
			size - size of texture memory needed
			ptr - the render target memory to free. it MUST have been allocated from the specified pooly and heap.
	*/
	static void FreeTextureMem(u16 poolID, u8 heapID, void *ptr);
	
	/*	PURPOSE
		Initialize the render target pools memory (i.e. check for tiling availability, etc)

		PARAMS
			poolID - The Id of the pool to query
			size - size of texture memory needed
			alignment - requested alignment
		
		RETURNS
			Returns a pointer to the texture memory requested, or NULL if there was not enough memory available in the pool
	*/
	static void * AllocatePoolMem(u16 poolID, u8 heapID, int size, int alignment = grcRenderTargetPoolEntry::kDefaultAlignment);

	static int GetMemPoolCount()										{return (int)sm_PoolEntries.GetCount();}
	static grcRenderTargetPoolEntry* GetPoolEntry(u16 poolID)	{if (VerifyPoolID(poolID)) return (*sm_PoolEntries.GetElemByIndex(poolID)); else return NULL;}

#if __BANK
	static void		AddWidgets(bkBank& bk);	
	static void		RequestSaveLog();	// request the next frame's render target usage be save to a log
#endif

protected:
	static void InitializeMemory(u16 poolID, const grcRTPoolCreateParams & params)	{if (VerifyPoolID(poolID)) (*sm_PoolEntries.GetElemByIndex(poolID))->InitializeMemory(params);}

private:
	static atPool<grcRenderTargetPoolEntry * > sm_PoolEntries;

#if __BANK
	static char sm_LogName[256];		// the name of the pool log file.
	static bool sm_EnableLoggingNextFrame;
#endif
};


/*
PURPOSE
	DEPRICATED: this is the old method is should no longer be used.

	grcRenderTargetMemPool provide a method for sharing the main memory backing store for rendertargets on a Xenon	
	A pool is allocated that is big enough to hold the largest of the overlapable rendertargets, 
	then activated before each set of rendertargets are created.

	Example:	
			renderTargetMemPool = rage_new grcRenderTargetMemPool(1024*1024);  // enough space to hold a 512x512 32 bit texture
			renderTargetMemPool->ActivateRenderTargetPool()
				target1 = grcTextureFactory::GetInstance().CreateRenderTarget("target1", rage::grcrtPermanent, 512, 512, 32);
			renderTargetMemPool->DeactivateRenderTargetPool();
			
			renderTargetMemPool->ActivateRenderTargetPool()
				target2 = grcTextureFactory::GetInstance().CreateRenderTarget("target2", rage::grcrtPermanent, 512, 512, 32);
			renderTargetMemPool->DeactivateRenderTargetPool();
		
		target1 and target 2 resolve to the same physical memory on the 360. 
		However, as long as one target is resolved and used as a texture source before the other is resolved, 
		there is no conflict, and 1 Meg of memory is "saved"

*/
class grcRenderTargetMemPool
{
public:

	/*	PURPOSE
		Allocated a pool of main texture memory for shading between multi rendertargets (or collections of rendertargets) 
	PARAMS
		size - the number of bytes to allocate in the pool.
		heapCount - the number of unique heaps to track
	*/
	grcRenderTargetMemPool(const char* name, int size, u8 heapCount = 1, int alignment = grcRenderTargetPoolEntry::kDefaultAlignment, bool isPhysical = true, u32 pitch = 0, u8 compression = 0, void *poolMem = NULL);
	~grcRenderTargetMemPool();	

	/*	PURPOSE
		make the current pool the global render target memory pool
	PARAMS
		heapId - the heap number to activate
		reset  - reset the memory used count if true, otherwise it continues from the end of the last allocation from this pool
	*/
	void Activate( u8 heapId=0, bool reset = true);
	
	/*	PURPOSE
	 reactivate a  pool using the same set as it last used. Useful for restoring temporarily disabled pools
	PARAMS
	*/
	void Reactivate( )				{sm_ActivePool = this;};

	void ResetToSize( int /*size*/ )
	{
			Assertf(0 , "Not implemented in the new grcRenderTargetPoolMgr system");
			// the new system uses a real heap memory allocator, so there is not easy way to reset the heap to a specific value.
// 			Assert( size <= m_MostUsed[ m_ActiveHeap] );
// 			m_Used[ m_ActiveHeap] = size;
	}

 	static bool	IsInitialised()					{ FastAssert(sm_ActivePool); return grcRenderTargetPoolMgr::GetIsInitialised(sm_ActivePool->m_PoolID);}
	static u8	GetHeapCount()					{ FastAssert(sm_ActivePool); return grcRenderTargetPoolMgr::GetHeapCount(sm_ActivePool->m_PoolID);}

	static const char*	GetName()				{ FastAssert(sm_ActivePool); return grcRenderTargetPoolMgr::GetName(sm_ActivePool->m_PoolID);}
	static u8			GetActiveHeap()			{ FastAssert(sm_ActivePool); return sm_ActivePool->m_ActiveHeap;}
	static void *		GetPoolMemory()			{ FastAssert(sm_ActivePool); return grcRenderTargetPoolMgr::GetPoolMemory(sm_ActivePool->m_PoolID);}
	static u16			GetPoolID()				{ FastAssert(sm_ActivePool); return sm_ActivePool->m_PoolID;}

	/*	PURPOSE
		get the lowest amount of free space ever  between activation that do not reset
	RETURNS
		Returns the smallest amount of free memory seen.
	*/
	static int GetLargestUsedSize()	{FastAssert(sm_ActivePool); return grcRenderTargetPoolMgr::GetLargestFreeBlock(sm_ActivePool->m_PoolID,sm_ActivePool->m_ActiveHeap);}
	static int GetUsedSize()		{FastAssert(sm_ActivePool); return grcRenderTargetPoolMgr::GetUsedMem(sm_ActivePool->m_PoolID,sm_ActivePool->m_ActiveHeap);}
	static int GetFreeSize()		{FastAssert(sm_ActivePool); return grcRenderTargetPoolMgr::GetFreeMem(sm_ActivePool->m_PoolID,sm_ActivePool->m_ActiveHeap);}
// 	static int GetWorstFreeSize()	{FastAssert(sm_ActivePool); return sm_ActivePool->m_Allocated-sm_ActivePool->m_MostUsed[sm_ActivePool->m_ActiveHeap];} 
	static u32 GetAllocated()		{FastAssert(sm_ActivePool); return grcRenderTargetPoolMgr::GetAllocated(sm_ActivePool->m_PoolID);}

#if HACK_GTA4
	static int GetMaxUsedSize();
	static int GetMinFreeSize();
	static int GetMinWorstFreeSize();
#endif // HACK_GTA4

	/*	PURPOSE
		Deactivate any active  grcRenderTargetMemPools.
		RETURNS
		Returns the formerly active pool, or NULL if none are active
	*/
	static grcRenderTargetMemPool * Deactivate( ) {grcRenderTargetMemPool *old=sm_ActivePool; sm_ActivePool=NULL; return old;}  

	/*	PURPOSE
		determine if a rendertarget memory pool is active
	RETURNS
		Returns true rendertarget memory pool is active
	*/
	static bool IsActivate( ) {return sm_ActivePool?true:false;}


	/*	RETURNS
			Returns active memory pool.
	*/
	static grcRenderTargetMemPool* GetActiveMemPool() {return sm_ActivePool;}
	

	/*	PURPOSE
		allocate texture memory from the active rendertarget pool

	PARAMS
		size - size of texture memory needed

	RETURNS
		Returns a pointer to the texture memory requested, or NULL if there was not enough memory available in the pool

	NOTES
		the higher level code need to keep track of whether the texture's memory was from a pools, 
		so it know if the texture memory needs to be freed
	*/
	static void * AllocateTextureMem(int size, int alignment = grcRenderTargetPoolEntry::kDefaultAlignment);

	static int GetMemPoolCount() {return sm_MemPools.GetCount();}
	static grcRenderTargetMemPool& GetMemPool(int index) {return *sm_MemPools[index];}
private:

	// this is all handled by the new grcRendertargetPoolMgr now, we just keep this info for compatibility
	u16	m_PoolID;
	u8	m_ActiveHeap;

	static grcRenderTargetMemPool * sm_ActivePool;		// this will go away when this class is reimplemented in terms of  the new rendertargetpoolMgr
	static atArray<grcRenderTargetMemPool * > sm_MemPools;
};


// Cube face identifiers.  Chosen to match order of D3DCUBEMAP_FACE_... enumerants
enum grcCubeFace {		
	grcPositiveX,		// Positive X
	grcNegativeX,		// Negative X
	grcPositiveY,		// Positive Y
	grcNegativeY,		// Negative Y
	grcPositiveZ,		// Positive Z
	grcNegativeZ,		// Negative Z

	grcCubeFaceCount
};


/*
	grcTextureParameters holds multi-platform texture tuning information
*/
struct grcTextureParameters {
	grcTextureParameters() : clamps(0), clampt(0), d3d_bias(0), ps2_bias(0), ps2_inclination(0) { }
	bool clamps, clampt;
	float d3d_bias, ps2_bias;
	int ps2_inclination;
};

#if __PPU
#define NONPSN_FMT_ONLY(x)	x##_DONTUSE
#define PSN_FMT_ONLY(x)		x
#else
#define NONPSN_FMT_ONLY(x)	x
#define PSN_FMT_ONLY(x)		x##_DONTUSE
#endif
#if __XENON
#define XENON_FMT_ONLY(x)	x
#else
#define XENON_FMT_ONLY(x)	x##_DONTUSE
#endif
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
#define PC_FMT_ONLY(x)	x
#else
#define PC_FMT_ONLY(x)	x##_DONTUSE
#endif

enum grcTextureFormat {
	grctfNone,
	grctfR5G6B5,									//PSN:CELL_GCM_TEXTURE_R5G6B5
	grctfA8R8G8B8,									//PSN:CELL_GCM_TEXTURE_A8R8G8B8
	NONPSN_FMT_ONLY(grctfR16F),

	grctfR32F,										//PSN:CELL_GCM_TEXTURE_X32_FLOAT
	NONPSN_FMT_ONLY(grctfA2B10G10R10),				// A10B10G10R10F_EDRAM on Xenon, A16B16G16R16F on PC.
	NONPSN_FMT_ONLY(grctfA2B10G10R10ATI),			// A10B10G10R10 for ATI graphics cards on PC or a 10-bit integer format for 360
	grctfA16B16G16R16F,								//PSN:CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT

	grctfG16R16,									// On PS3, you have to pack the values when writing to render targets of this type with the Cg intrinsics unpack_4ubyte(pack_2ushort(float2)), PSN:CELL_GCM_TEXTURE_Y16_X16
	grctfG16R16F,									// On PS3, you have to pack the values when writing to render targets of this type with the Cg intrinsics unpack_4ubyte(pack_2half(float2)), PSN:CELL_GCM_TEXTURE_Y16_X16_FLOAT
	grctfA32B32G32R32F,								//PSN:CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT
	NONPSN_FMT_ONLY(grctfA16B16G16R16F_NoExpand),	// Non-filterable put "pure" floating point on xenon

	NONPSN_FMT_ONLY(grctfA16B16G16R16),
	grctfL8,										//PSN:CELL_GCM_TEXTURE_B8
	NONPSN_FMT_ONLY(grctfL16),
	PC_FMT_ONLY(grctfG8R8),

	XENON_FMT_ONLY(grctfG8R8_XENON),
	grctfA1R5G5B5,									//PSN:CELL_GCM_TEXTURE_A1R5G5B5
	grctfD24S8,						
	NONPSN_FMT_ONLY(grctfA4R4G4B4),

	NONPSN_FMT_ONLY(grctfG32R32F),
	XENON_FMT_ONLY(grctfD24FS8_ReadStencil),		//resolves out to and r8g8b8a8 so that you can read stencil information
	grctfD16,
	PSN_FMT_ONLY(grctfG8B8),

	PC_FMT_ONLY(grctfD32F),
	PC_FMT_ONLY(grctfX8R8G8B8),
	PC_FMT_ONLY(grctfNULL),
	PC_FMT_ONLY(grctfX24G8),						//Used for reading stencil in dx11
	PC_FMT_ONLY(grctfA8),
	PC_FMT_ONLY(grctfR11G11B10F),
	PC_FMT_ONLY(grctfD32FS8),
	PC_FMT_ONLY(grctfX32S8),						// Used for reading 40bit depth buffer in dx11
	PC_FMT_ONLY(grctfDXT1),
	PC_FMT_ONLY(grctfDXT3),
	PC_FMT_ONLY(grctfDXT5),
	PC_FMT_ONLY(grctfDXT5A),
	PC_FMT_ONLY(grctfDXN),
	PC_FMT_ONLY(grctfBC6),
	PC_FMT_ONLY(grctfBC7),
	NONPSN_FMT_ONLY(grctfA8B8G8R8_SNORM),		    // Used for GPU damage writing /w blending, no support for integer signed blending format on PS3
	PC_FMT_ONLY(grctfA8B8G8R8),
#if RSG_DURANGO
	PC_FMT_ONLY(grctfNV12),
#endif
	grctfCount
};
// If you add or remove elements from this enumerant, search on grcTextureFormat_REFERENCE to find all the tables
// you will need to update.  At the very least, they are in texturegcm.cpp, texturexenon.cpp, and texture_d3d9.cpp.




/*
PURPOSE
	grcTextureFactory is the class factory for grcTexture and grcRenderTarget subclasses.
<FLAG Component>
*/
class grcTextureFactory {
public:

	/* Structure which can be optionally passed to grcTextureFactory::Create */
	struct TextureCreateParams {
		enum Memory_t { SYSTEM, VIDEO, STAGING }; // System indicates Dynamic CPU Write Access, Staging indicates CPU Read Access
		enum Format_t { LINEAR, TILED };
		enum Type_t { NORMAL, RENDERTARGET, REFERENCE, DICTIONARY_REFERENCE };
		enum MSAA_t { MSAA_NONE, MSAA_2X, MSAA_4X, MSAA_8X };
	#if RSG_PC
		enum ThreadUseHint_t { THREAD_USE_HINT_NONE, THREAD_USE_HINT_CAN_BE_UPDATE_THREAD };
	#endif // RSG_PC
#if RSG_DURANGO
		enum
		{ 
			ESRPHASE_NONE = 0,
			ESRPHASE_REFLECTION_MAP = 0x1L,
			ESRPHASE_WATER_SURFACE = 0x2L,
			ESRPHASE_GBUF = 0x4L,
			ESRPHASE_SHADOWS = 0x8L,
			ESRPHASE_RAIN_COLLSION_MAP = 0x10L,
			ESRPHASE_WATER_REFLECTION = 0x20L,
			ESRPHASE_RAIN_UPDATE = 0x40L,
			ESRPHASE_LIGHTING_0 = 0x80L,
			ESRPHASE_LIGHTING_1 = 0x100L,
			ESRPHASE_LIGHTING = (ESRPHASE_LIGHTING_0 | ESRPHASE_LIGHTING_1),
			ESRPHASE_DRAWSCENE_0  = 0x200L,
			ESRPHASE_DRAWSCENE_1  = 0x400L,
			ESRPHASE_DRAWSCENE_2  = 0x800L,
			ESRPHASE_DRAWSCENE_3  = 0x1000L,
			ESRPHASE_DRAWSCENE_4  = 0x2000L,
			ESRPHASE_DRAWSCENE_5  = 0x4000L,
			ESRPHASE_DRAWSCENE_6  = 0x8000L,
			ESRPHASE_DRAWSCENE_7  = 0x10000L,
			ESRPHASE_DRAWSCENE_8  = 0x20000L,
			ESRPHASE_DRAWSCENE_9  = 0x40000L,
			ESRPHASE_DRAWSCENE_10 = 0x80000L,
			ESRPHASE_DRAWSCENE_11 = 0x100000L,
			ESRPHASE_DRAWSCENE = (ESRPHASE_DRAWSCENE_0 | ESRPHASE_DRAWSCENE_1 | ESRPHASE_DRAWSCENE_2 | ESRPHASE_DRAWSCENE_3 | ESRPHASE_DRAWSCENE_4 | ESRPHASE_DRAWSCENE_5 | ESRPHASE_DRAWSCENE_6 | ESRPHASE_DRAWSCENE_7 | ESRPHASE_DRAWSCENE_8 | ESRPHASE_DRAWSCENE_9 | ESRPHASE_DRAWSCENE_10 | ESRPHASE_DRAWSCENE_11),
			ESRPHASE_DRAWSCENE_MAIN = (ESRPHASE_DRAWSCENE_0 | ESRPHASE_DRAWSCENE_1 | ESRPHASE_DRAWSCENE_2 | ESRPHASE_DRAWSCENE_3),
			ESRPHASE_DRAWSCENE_NDFX = (ESRPHASE_DRAWSCENE_4 | ESRPHASE_DRAWSCENE_5 | ESRPHASE_DRAWSCENE_6 | ESRPHASE_DRAWSCENE_7 | ESRPHASE_DRAWSCENE_8 | ESRPHASE_DRAWSCENE_9 | ESRPHASE_DRAWSCENE_10 | ESRPHASE_DRAWSCENE_11),
			ESRPHASE_HUD = 0x200000L,
			ESRPHASE_FRONTEND = 0x400000L,
			ESRPHASE_MAX = 22,
		};
#endif
		
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
		TextureCreateParams(Memory_t m, Format_t f, u32 flags = 0, void* buffer = NULL, Type_t t = NORMAL, MSAA_t a = MSAA_NONE, grcDevice::Stereo_t s = grcDevice::AUTO) : Memory(m), Format(f), LockFlags(flags), Buffer(buffer), Type(t), MSAA_Type(a), StereoRTMode(s), MipLevels(1) 
		{ 
		#if RSG_PC && __D3D11 
			ThreadUseHint = THREAD_USE_HINT_NONE;
		#endif // RSG_PC && __D3D11 
		}
#else
		TextureCreateParams(Memory_t m, Format_t f, u32 flags = 0, void* buffer = NULL, Type_t t = NORMAL, MSAA_t a = MSAA_NONE) : Memory(m), Format(f), LockFlags(flags), Buffer(buffer), Type(t), MSAA_Type(a) {}
#endif
		Memory_t Memory;
		Format_t Format;
		u32 LockFlags;
		void* Buffer;//client managed buffer to hold the texture data
		Type_t Type;
		MSAA_t MSAA_Type;
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
		grcDevice::Stereo_t StereoRTMode;
#endif // RSG_PC || RSG_DURANGO || RSG_ORBIS
		u32 MipLevels;
#if RSG_PC && __D3D11
		ThreadUseHint_t ThreadUseHint;
#endif // RSG_PC && __D3D11
	};

	grcTextureFactory();
	virtual ~grcTextureFactory();

	/*	PURPOSE
			Create a texture based on the named file
		PARAMS
			filename - name of the file to load
		RETURNS
			Pointer to new texture object.  Should never return a null pointer;
			if file could not be loaded, should return a small white texture (typically
			the same result you should get calling Create("none") in current implementations) */
	virtual grcTexture* Create(const char *filename,TextureCreateParams *params = NULL) = 0;

	/*	PURPOSE
			Create a texture based on the named file.  This version figures out whether there's
			an active texture dictionary or not and knows whether to return the original texture
			or a reference to if it is going to be a deferred load.
	*/
	static grcTexture* CreateTexture(const char *filename);

	/*	PURPOSE
			Create a texture based on the supplied image structure.
		PARAMS
			image - image to create; may be reformatted by callee.
		RETURNS
			Pointer to new texture object.  Should never return a null pointer. */
	virtual grcTexture* Create(class grcImage*,TextureCreateParams *params = NULL) = 0;

	/*	PURPOSE
			Create a texture based on the supplied width and height.
		PARAMS
			width - width of the image to create
			height - width of the image to create
			format - format of the image to create (D3D format for textureXenon) - PC TODO - Let function take care of conversion
			pBuffer - pre-allocated buffer to store color data
			numMips - number of mip levels
			params - special case texture creation parameters
		RETURNS
			Pointer to new texture object.  Should never return a null pointer. 
	*/
	virtual grcTexture* Create(u32 /*width*/, u32 /*height*/, u32 /*eFormat*/, void* /*pBuffer*/, u32 /*numMips*/ = 1, TextureCreateParams * /*params*/ = NULL) {return NULL;}
#if __D3D11 || RSG_ORBIS
	/*	PURPOSE
			Create a texture based on the supplied width, height and depth (a volume texture).
		PARAMS
			width - width of the image to create
			height - width of the image to create
			format - format of the image to create (D3D format for textureXenon) - PC TODO - Let function take care of conversion
			pBuffer - pre-allocated buffer to store color data
			params - special case texture creation parameters
		RETURNS
			Pointer to new texture object.  Should never return a null pointer. 
	*/
	virtual grcTexture* Create(u32 /*width*/, u32 /*height*/, u32 /*depth*/, u32 /*eFormat*/, void* /*pBuffer*/, TextureCreateParams * /*params*/ = NULL) {return NULL;}
#endif //__D3D11 || RSG_ORBIS

	/*	PURPOSE
			Computes physical size of texture data based on the parameters passed
		PARAMS
			width - width of the image to create
			height - width of the image to create
			mipLevels - number of mip levels
			bLocalMemory - whether the texture data is to be allocated in video or system memory
		RETURNS
			Pointer to new texture object.  Should never return a null pointer. 
	*/
	virtual u32 GetTextureDataSize(u32 /*width*/, u32 /*height*/, u32 /*format*/, u32 /*mipLevels*/, u32 /*numSlices*/, bool /*bIsCubeMap*/, bool /*bIsLinear*/, bool /*bLocalMemory*/) { FastAssert(0); return 0U; };

   /*	PURPOSE
	Load a list of global textures and fix up the references.  This needs to be called after the shaders are
	loaded.
	PARAMS
	path - The path to the global texture list */
	void PreloadGlobalTextures(const char *path);

	/*	PURPOSE
	Unload all the global textures.  This needs to be called to prevent a memory leak.
	PARAMS
	none*/
	static void UnloadGlobalTextures();

	/*	PURPOSE
			Register an existing texture as a named object; note that you do not have to register
			the reference under the same name as the texture itself; this is useful for aliasing
			a manually-loaded texture to a central name (ie dynamically load "skydome_evening_rain" and
			register it as "skydome" so that existing shaders can adapt to run-time changes).
		PARAMS
			name - Name to give this texture (matched on subsequent LookupTextureReference calls)
			texture - Texture object to associated with this name */
	void RegisterTextureReference(const char *name,const grcTexture *texture);

	/*	PURPOSE
			Deletes an existing named texture reference from the hash table */
	void DeleteTextureReference(const char *name);

	/*	RETURNS: A new grcTextureReference pointing to the original texture, if any, under that name. */
	grcTexture *LookupTextureReference(const char *name);

	/*	RETURNS:	Direct pointer to a registered texture (not a new copy of a reference to it) 
		NOTES:		Intentionally does not modify any reference counts, so be careful! */
	grcTexture *ConnectToTextureReference(const char *name);

	/* Structure which can be optionally passed to grcTextureFactory::CreateRenderTarget */
	struct CreateParams {
		CreateParams(u16 poolID=kRTPoolIDInvalid); 
		bool UseFloat;						// default false
		grcDevice::MSAAMode Multisample;	// Multisample level; 0, 2, or 4.  Default zero.
#if !DEVICE_EQAA
		u8  MultisampleQuality;				// Multisample quality level
#endif
		bool IsResolvable;					// Xenon only; default true, can only be false for depth targets
		int	 MipLevels;						// Xenon only; currently - Number of mip map levels to create when texture is resolved
		bool IsRenderable;					// Xenon only; default true, useful for rendertarget intended only for predicated tiling (can be bigger than will fit in EDRAM)
		bool UseHierZ;						// Xenon: default true, Enable Hierarchical Z if this is a depth target
											// PS3: enable ZCulling
		u16  HiZBase;						// Xenon only; offset in embedded hi-Z memory where the surface's hi-Z bits are kept, in units of hi-Z tiles. defaults to 0.
		bool HasParent;						// Xenon only; default false; if true, Parent must be NULL or existing rendertarget
		const grcRenderTarget *Parent;		// Xenon only; if HasParent is true, and NULL, then rendertarget starts at base of EDRAM
											//             if HasParent is true, and not NULL, then rendertarget starts immediately after this one
		bool Lockable;						// PC only; default false; if true, rendertarget is Lockable, but might be SLOOOOW!
											// this number is then used to multiply the texture size
		int ColorExpBias;					// Xenon only for now; Color exponent bias, biases float exponent before writing to EDRAM 7e3 buffer.
		bool IsSRGB;						// Automatic gamma conversion on read/write: default false.
		grcTextureFormat Format;

		bool InTiledMemory;					// PS3 default = false because we only have a fixed number of tiling regions. Xenon there is no such restriction, so the default = true

#if __PS3
		bool UseCustomZCullAllocation;		// PS3 only; defaults to false, if set to true this indicates that you will use the ForceZCullSharing api to setup zcull after target is allocated
#endif

#if __PPU || __WIN32PC || RSG_DURANGO || RSG_ORBIS
		bool CreateAABuffer;				// PS3 only: create the AA buffer for blitting into: default = true
		bool InLocalMemory;					// PS3 render targets only: default true, true = VRAM, false = HOST
		bool IsSwizzled;					// PS3 render targets only: default false. NOTE: all DDS textures loaded from file will be swizzled. Must be a power of 2 dimensions and can't be tiled
		bool EnableCompression;				// default true.
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
		u8 PerArraySliceRTV;				// DX10/11 per array slice rendertarget view
		u8 ArraySize;						// DX10/11 texture array
		grcDevice::Stereo_t StereoRTMode;
#endif
#if RSG_DURANGO
		u64	ESRAMPhase;						// ESRAM usage phase, if any
		u32	ESRAMMaxSize;					// ESRAM max size, if any
#endif

#if RSG_ORBIS
		bool ForceNoTiling;
		bool EnableFastClear;				// enable Cmask allocation and usage
		bool EnableStencil;
#endif // RSG_ORBIS
#if DEVICE_MSAA
		unsigned SupersampleFrequency;		// define the number of PS iterations per pixels
#endif
#if DEVICE_EQAA
		bool EnableHighQualityResolve;		// enable custom resolve code that accesses neighboring pixels
		bool ForceHardwareResolve;			// use HW resolve
		bool ForceFragmentMask;				// created Fmask even if numSamples == numFragments
		bool EnableNanDetect;				// Used to fix an issue with PS4 EQAA resolve returning NaNs
#endif // DEVICE_EQAA

		bool UseAsUAV;
#endif // __PPU || __WIN32PC || RSG_DURANGO || RSG_ORBIS

		u16  PoolID;						// The id of the pool this target should be placed in, setting it to kRTPoolIDAuto, a pool will be created for the target or , if set to kRTPoolIDInvalid, then it will cause the target to just be allocated from normal memory (unless an old style grcRenderTargetMemPool is "active", then it is used)
		u8   PoolHeap;						// the heap index for the target's allocation, set kRTPoolHeapInvalid, if using a specific offset into a RT Pool (i.e non allocated)
		int	 PoolOffset;					// if poolHeapID is kRTPoolHeapInvalid and poolID is a valid poolID, the target will be placed in the pool memory at this specific offset, and will not go through the normal RT Pool Heap Allocation
		bool AllocateFromPoolOnCreate;		// allocate the memory from the render target pool at creation time, otherwise it's up the the called to call AllocateMemoryFromPool() before using the rendertarget

		u32 basePtr;						// pointer to preallocated texture memory for top mip. pool needs to be invalid.
		u32 mipPtr;							// pointer to preallocated memory for mip chain excluding top mip. needs to be used together with basePtr
	};

	/* 
		PARAMS:  if realize is false, it will not realize the current edram version (if applicable), just return the backbuffer texture pointer
		RETURNS: A grcTexture representing the current backbuffer
	*/

	virtual const grcRenderTarget *GetBackBuffer(bool realize=true) const = 0;
	virtual grcRenderTarget *GetBackBuffer(bool realize=true) = 0;
	
	/*
		PARAMS: for the 360, if nextBuffer is true, it will return the Front buffer that will be rendered to at the end of this frame, otherwise the one used last frame
		RETURNS: A grcTexture representing the current frontbuffer
	*/
	virtual const grcRenderTarget *GetFrontBuffer(bool nextBuffer=false) const = 0;
	virtual grcRenderTarget *GetFrontBuffer(bool nextBuffer=false) = 0;

	/*
	PARAMS:  if realize is false, it will not realize the current edram version (if applicable), just return the front depth buffer texture pointer
	RETURNS: A grcTexture representing the current front buffer's current z buffer
	*/
	virtual const grcRenderTarget *GetFrontBufferDepth(bool realize=true) const = 0;
	virtual grcRenderTarget *GetFrontBufferDepth(bool realize=true) = 0;

	/* 
		PARAMS:  if realize is false, it will not realize the current edram version (if applicable), just return the back depth buffer texture pointer
		RETURNS: A grcTexture representing the backbuffer's current z buffer
	*/
	virtual const grcRenderTarget *GetBackBufferDepth(bool realize=true) const = 0;
	virtual grcRenderTarget *GetBackBufferDepth(bool realize=true) = 0;

	virtual const grcRenderTarget *GetBackBufferDepthForceRealize();

	/*
		PARAMS:	 none
		RETURNS: true if the depth buffer can be resolved, false if not.
	*/
	virtual bool CanGetBackBufferDepth();

	/*
		PARAMS:	 none
		RETURNS: true if the back buffer can be resolved, false if not.
	*/
	virtual bool CanGetBackBuffer();
	/*	PURPOSE
			Creates a render target of the specified type, size, and depth
		PARAMS
			name - Name associated with render target; used only for debugging
			type - Type of render target
			width - Width of render target; ignored if type is grcrtFrontBuffer or grcrtBackBuffer
			height - Height of render target; ignored if type is grcrtFrontBuffer or grcrtBackBuffer
			bitsPerPixel - Depth of render target; ignored if type is grcrtFrontBuffer or grcrtBackBuffer
			params - Pointer to extended parameter structure, or NULL to use defaults.
		NOTES
			See CreateParams definition for information of the various options for creating a render target, 
				including specifying a pool to allocated the target from, etc.
		RETURNS
			Pointer to new render target */
		virtual grcRenderTarget* CreateRenderTarget(const char *name, grcRenderTargetType type, int width, int height, int bitsPerPixel, CreateParams *params = NULL, grcRenderTarget* originalTexture = NULL) = 0;
	
	/*	PURPOSE
			Creates Rage render target of the resource that is not created through Rage texture factory
		PARAMS
			name - Name associated with render target; used only for debugging
			pTexture - Platform specific render target
		RETURNS
			Pointer to new render target */
	virtual grcRenderTarget* CreateRenderTarget(const char *pName, const grcTextureObject *pTexture, grcRenderTarget* originalTarget = NULL) = 0;

	/*	PURPOSE
			Convenience function for CreateRenderTarget */
	grcRenderTarget* CreateFrontBuffer(const char *name) { return CreateRenderTarget(name,grcrtFrontBuffer,0,0,0); }

	/*	PURPOSE
			Convenience function for CreateRenderTarget */
	grcRenderTarget* CreateBackBuffer(const char *name) { return CreateRenderTarget(name,grcrtBackBuffer,0,0,0); }

	/*	PURPOSE
			Make an existing texture and depth buffer render target active
		PARAMS
			index - Render target index (0-3 on Xenon, variable on PC builds)
			color - Render target to use for color buffer (cannot be of type grcrtDepthBuffer)
			depth - Render target to use for depth buffer (or 0 for none) (must be of type grcrtDepthBuffer or 	grcrtShadowMap)
			face - Cubemap face to use (only valid if the render target is really a cubemap)
			lockDepth - Whether or not to use the specified depth(true) or the existing depth(false)
		NOTES
			Cube maps are currently unsupported.  Call UnlockRenderTarget when done.  Locking render target changes 
			the current viewport, current camera (as specified by caller), and forces the identity matrix for 
			the world matrix. 
			Depth target only accepted for index = 0 */
	virtual void LockRenderTarget(int index,const grcRenderTarget *color,const grcRenderTarget *depth,
		        u32 layer = 0, bool lockDepth = true, u32 D3D11_OR_ORBIS_ONLY(mipToLock) = 0) = 0;

	/*	PURPOSE
			Restores previous render target; must match up with LockMRT call.  Previous viewport
			is restored, which also implicitly restores the previous camera and world matrices. 
		PARAMS
			index - Render target index (0 - 3 on Xenon, variable on PC builds)
			flags - Pointer to optional flag structure specifying clear information
		NOTES
			Unlocking index 0 will automatically restore depth/stencil surface */
	virtual void UnlockRenderTarget(int index,const grcResolveFlags* flags = NULL) = 0;

	/*	PURPOSE
			Sets the current array slice of the currently locked render/depth target
		PARAMS
			index - Render target index (0 - 3 on Xenon, variable on PC builds)
			bDepth - whether to lock depth target or color target
			uArrayIndex - array index to be selected
		NOTES
	 */
	virtual void SetArrayView(u32 uArrayIndex) {(void)uArrayIndex;}

	/*	PURPOSE
			Make an existing MRT and a depth buffer render target active
		PARAMS
			color - Render target to use for color buffer (cannot be of type grcrtDepthBuffer)
			depth - Render target to use for depth buffer (or 0 for none) (must be of type grcrtDepthBuffer or 	grcrtShadowMap)
		NOTES
			Call UnlockMRT when done.  
	*/
	virtual void LockMRT(const grcRenderTarget *color[grcmrtColorCount], const grcRenderTarget *depth, const u32* D3D11_ONLY(mipsToLock) = NULL) = 0;


	/*	PURPOSE
			Restores previous render target; must match up with LockMRT call.  Previous viewport
			is restored, which also implicitly restores the previous camera and world matrices. 
		PARAMS
			flags - Pointer to optional flag structure specifying clear information
		NOTES
			Unlocks all MRT render targets and sets the back or front buffer
	*/
	virtual void UnlockMRT(const grcResolveFlagsMrt* resolveFlags = NULL) = 0;

	/*	RETURNS
			Current instance of this factory */
	static grcTextureFactory& GetInstance() { FastAssert(sm_Instance); return *sm_Instance; }

	static bool HasInstance() { return sm_Instance != NULL; }

	/*	PURPOSE
			Changes the active instance; this class was originally designed as a singleton but
			that didn't match everybody's needs. */
	static void SetInstance(grcTextureFactory &instance) { sm_Instance = &instance; }

	static void PushInstance(grcTextureFactory &newInst) {
		FastAssert(!sm_PrevInstance);
		sm_PrevInstance = sm_Instance;
		sm_Instance = &newInst;
	}

	static void PopInstance() {
		FastAssert(sm_PrevInstance);
		sm_Instance = sm_PrevInstance;
		sm_PrevInstance = 0;
	}

	/*	PURPOSE
			Creates a string-based texture factory; it's useless, but is intended to allow grcore core to
			be linked in with exporters or other tools without requiring a real graphics pipeline
		PARAMS
			bMakeActive		- If true, sm_Instance will be set to point to the new factory.
		TODO
			This function used to behave as if bMakeActive == false, but all other
			Create..TextureFactory() functions default to bMakeActive == true. This
			function should too, so that they all work the same way. I didn't set it up
			with a default value yet just so that any users have time to adapt to the change. /FF
	*/
	static grcTextureFactory *CreateStringTextureFactory(bool bMakeActive);

//	/*	PURPOSE
//			Creates a texture factory that uses gfxTexture to do the real work; since replaced by
//			better versions on PS2 and Xbox, but is still used on PC builds. */
//	static grcTextureFactory *CreateGfxTextureFactory(bool bMakeActive = true);

	/*	PURPOSE
			Creates a texture factory suitable for any platform; on PC, it actually calls CreateGfxTextureFactory;
			on PS2 and Xbox, it will create platform-suitable subclassed factories */
	static grcTextureFactory *CreatePagedTextureFactory(bool bMakeActive = true);

	/*	PURPOSE
			Initializes the None and Nonresident textures using the current texture factory.  Call this
			once per application regardless of how many different texture factories you use at once */
	static void InitClass();

	/*	PURPOSE
			Frees the None and Nonresident textures.  Call this	once per application regardless of how 
			many different texture factories you use at once */
	static void ShutdownClass();

	/*	PURPOSE
			Causes all future texture binds to assume the object is at least this many unit away;
			this allows us to only stream in the minimum amount of texture data from disc as
			necessary, and, on PS2, download only as much of the texture to VRAM as necessary.
		PARAMS
			zDist - Nominal (closest) Z distance of all objects rendered subsequently */
	virtual void SetTextureLod(float /*zDist*/) { }

	/*	PURPOSE
			Works out what mipmap level would be required. Allows us to store with relevent meshes
			when baking textures.
		PARAMS
			zDist - Nominal (closest) Z distance at which we're inquiring*/
	int GetTextureLod(float /*zDist*/);

	/*	PURPOSE
			Performs resource fixups appropriate for the current factory on textures.
			This means that the factory that was active when the resource was created needs to
			be active when the resource is pulled back in again, or else all hell will break loose.
			Note that any pointer fixup on the texture itself ought to have already been done.
		PARAMS
			rsc - Resource object to use for fixup information
			tex - Texture to fix up. */
	virtual void PlaceTexture(class datResource& rsc,grcTexture &tex) = 0;

	virtual void ClearFreeList() { }

	// PURPOSE
	//		Get the maximum anisotropy value for this platform.
	static int GetMaxAnisotropy();
	static void SetMaxAnisotropy(int maxAnisotropy) { sm_MaxAnisotropy = maxAnisotropy;}

	// PURPOSE
	//		Get Bits Per Pixel of Texture formats
	static int GetBitsPerPixel(grcTextureFormat eFormat);

	// RETURNS: A simple checkboard texture useful for flagging items that aren't yet functional
	static grcTexture *GetNotImplementedTexture() { return sm_NotImplementedTexture; }

	static grcRenderTarget *GetNotImplementedRenderTarget() { return sm_NotImplementedRenderTarget; }

	virtual void SetDefaultRenderTarget(grcMrt mrt, grcRenderTarget* renderTarget)
	{
		FastAssert(!sm_DefaultRenderTargets[mrt].IsEmpty());
		sm_DefaultRenderTargets[mrt].End() = renderTarget;
	}

	virtual grcRenderTarget* GetDefaultRenderTarget(grcMrt mrt)
	{
		FastAssert(!sm_DefaultRenderTargets[mrt].IsEmpty());
		return sm_DefaultRenderTargets[mrt].End();
	}

	virtual void PushDefaultRenderTarget(grcMrt mrt, grcRenderTarget* renderTarget)
	{
		AssertVerify(sm_DefaultRenderTargets[mrt].Push(renderTarget));
	}

	virtual grcRenderTarget* PopDefaultRenderTarget(grcMrt mrt)
	{
		return sm_DefaultRenderTargets[mrt].PopEnd();
	}

	virtual void BindDefaultRenderTargets() = 0;

	struct RenderTargetDesc 
	{
		grcRenderTarget*	m_RenderTarget;
		u16					m_RenderTargetPoolID;
		u32					m_Heap;
	};

#if __BANK
	// For B*1923551 - Rare crash in CREnderTargets::UpdateBank. List size count becomes stale during function execution.
	static sysCriticalSectionToken &GetRenderTargetListCritSectionToken();
#endif //__BANK
	static int GetRenderTargetCount() {return sm_RenderTargets.GetCount();}
	static grcRenderTarget* GetRenderTarget(int index) { return sm_RenderTargets[index];}
	static void RegisterRenderTarget(grcRenderTarget* renderTarget);
	static void UnregisterRenderTarget(grcRenderTarget* renderTarget);

	// render target create helper function
	static u16 CreateAutoRTPool(const char *name, const CreateParams &params, u32 size);

#if DEBUG_SEALING_OF_DRAWLISTS
	virtual void SetDrawListDebugString(const char *pStr) { (void)pStr; }
	virtual char *GetDrawListDebugString() { return (char *)NULL; }
	virtual bool AreAnyRenderTargetsSet() { return false; }
	virtual bool HaveAnyDrawsBeenIssuedWithNoTargetsSet() { return false; }
	virtual void OutputSetRenderTargets() {}
	virtual void OnDraw() {}
#endif // DEBUG_SEALING_OF_DRAWLISTS

protected:
	static int sm_MaxAnisotropy; // max level of anisotropy supported by this platform
	static const u32 sm_MaxDefaultRenderTargetStackDepth = 4;
	typedef atQueue<grcRenderTarget*, sm_MaxDefaultRenderTargetStackDepth> RenderTargetStack;
	static RenderTargetStack sm_DefaultRenderTargets[grcmrtCount];
	static atArray<grcRenderTarget*> sm_RenderTargets;

private:
	static grcTextureFactory *sm_Instance;
	static grcTextureFactory *sm_PrevInstance;
	static float sm_GlobalMipAdjust, sm_GlobalMipScale;
	static grcTexture *sm_NotImplementedTexture;		// This texture means a feature is not yet implemented (orange & light blue checker)
	static grcRenderTarget *sm_NotImplementedRenderTarget;	// This is a 1x1 render target and means a feature is not yet implemented
	static grcTexture *sm_GlobalTextures[MAX_GLOBAL_TEXTURES];
#if __BANK
	static sysCriticalSectionToken sm_CritSectionToken;
#endif //__BANK
};

inline int grcTextureFactory::GetMaxAnisotropy()
{
	return sm_MaxAnisotropy;
}

inline u32 grcTexture::GetLayerCount() const
{
	return m_LayerCount+1;
}

//
//	PURPOSE
//		Creates a one level mipmap which is 4 times smaller than the base texture.
//	NOTES
//		The idea is that if you have a small texture ( 256 or less ) we could just have one or two
//		fastMipMaps and lerp between them in the pixel shader. As trilinear filtering is twice the cost
//		of bilinear filtering this should mean it should cost the same amount without all the hassle of
//		creating a proper mip map chain.
//
class grcFastMipMapper
{
public:
	inline grcFastMipMapper()
		: m_IsInitialized(false)
	{
	}
	void Init();
	~grcFastMipMapper();

	void DownSample(const grcRenderTarget* baseRenderTarget, int index, int level, const Vector2& sourceOffset, const Vector2& sourceSize, grcRenderTargetType type);
	void Blur(const grcRenderTarget* baseRenderTarget, float kernalSize, const Vector2& sourceOffset, const Vector2& sourceSize);
	inline bool IsInitialized() const { return m_IsInitialized; }

private:
	grcEffectTechnique	m_DepthTechnique;
	grcEffectTechnique	m_ColorTechnique;
#if __PPU
	grcEffectTechnique	m_ColorCutOutTechnique;
#endif // __PPU
	grcEffectTechnique	m_BlurTechnique;
	grcEffectVar		m_BaseTextureID;
	grcEffectVar		m_TexelSizeID;
	grcEffect			*m_Shader;
	bool				m_IsInitialized;
	atRangeArray<grcSamplerStateHandle,8>	m_SamplerStates;
	atRangeArray<grcBlendStateHandle, MAX_RAGE_RENDERTARGET_COUNT> m_BlendStates;
};

class grcCompositeRenderTargetHelper
{
public:
	enum CompositeRenderTargetBlendType_e
	{
		COMPOSITE_RT_BLEND_COMPOSITE_ALPHA = 0,
		COMPOSITE_RT_BLEND_ADD,
		COMPOSITE_RT_BLEND_TOTAL
	};

	struct CompositeParams
	{
		CompositeParams();

		const grcRenderTarget* srcColor;
		const grcRenderTarget* srcDepth;

		const grcRenderTarget* dstColor;
		const grcRenderTarget* dstDepth;

		grcEffect* compositeEffect;
		grcEffectTechnique compositeTechnique;
		grcEffectVar compositeSrcColorMap;
		grcEffectVar compositeSrcDepthMap;
		grcEffectVar compositeDstDepthMap;
		

		CompositeRenderTargetBlendType_e compositeRTblendType;

		bool lockTarget;
		bool unlockTarget;
		float depth;
		grcResolveFlags *resolveFlags;

	};
	
	struct BlitParams
	{
		BlitParams();

		grcRenderTarget* srcColor;
		grcRenderTarget* dstColor;

		grcEffect* blitEffect;
		grcEffectTechnique blitTechnique;
		grcEffectVar blitSrcMapVar;

		bool lockTarget;
		bool unlockTarget;
		grcResolveFlags *resolveFlags;

	};
	struct DownsampleParams
	{
		DownsampleParams();

		grcRenderTarget* srcDepth;
#if __PS3
		grcRenderTarget* srcPatchedDepth;
#endif // __PS3

		grcRenderTarget* dstDepth;

		grcEffect* depthDownsampleEffect;
		grcEffectTechnique depthDownsampleTechnique;
		grcEffectVar depthDownsampleSrcMapVar;

	};

	static void InitClass();
	static void CompositeDstToSrc(const CompositeParams& params);
	static void BlitDstToSrc(const BlitParams& params);
	static void DownsampleDepth(const DownsampleParams &params);

private:
	static void DrawFullScreenTri(float fDepth = 0.0f);

};

// PURPOSE : Platform agnostic iterator to allow modifying a texture.
//
// DESCRIPTION:  Allows game teams to treat it as an array of the given template type
// without having to worry about pitch issues and type is validated to be the same bit depth
// as the pixel. Preforms a lock in the constructor and unlocks in the desctructor
//
// SEE ALSO: grcTexture, grcTextureLock
//
// EXAMPLE: 
//	<B>Creating a noise texture on startup using the iterator</B>
// <CODE>
//	grcTexelIterator<u32>	itor( noiseTex );
//	while( itor.Next() )
//	{
//		*itor = rand() << 30 | rand () << 15 | rand();
//	}
// </CODE>
//
template<class TYPE>
class grcTexelIterator
{
public:
	// Locks the texel and setups things up for iteration.
	// Any errors which mean that no iterating over the texture occurs.
	grcTexelIterator( grcTexture* tex, u32 uLockFlags = grcsWrite | grcsRead) : m_tex(tex ),x(-1),y(0)
	{
		Assert( tex );
		// set these so if lock fails will not iterate over texture.
		m_lock.Width = m_lock.Height = 0;

		ASSERT_ONLY( bool res = ) tex->LockRect(0,0,m_lock, uLockFlags );
		AssertMsg( res ," Texture cannot be locked correctly");

		m_ptr = reinterpret_cast<u8*>(m_lock.Base);
		AssertMsg( sizeof( TYPE) == m_lock.BitsPerPixel/8 , "Format and Data Type Don't Match for TexelIterator" );
	}
	// Lock the texture
	~grcTexelIterator()
	{
		m_tex->UnlockRect( m_lock );
	}
	// Move to the next pixel on the texture
	bool Next()
	{
		x++;
		if ( x >= m_lock.Width )
		{
			x = 0; 
			y++;
			m_ptr +=m_lock.Pitch;
			if ( y >= m_lock.Height )
			{
				return false;
			}
		}
		return true;
	}
	// Return the current pixel as the given template type
	TYPE& operator*()
	{ 
		return *(reinterpret_cast<TYPE*>( m_ptr + ( m_lock.BitsPerPixel/8 )* x)); 
	}
private:
	grcTextureLock		m_lock;
	const grcTexture*	m_tex;

	u8*					m_ptr;
	int					x;
	int					y;
};


}	// namespace rage

#endif
