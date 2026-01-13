#ifndef GRCORE_RENDER_TARGET_DURANGO_H
#define GRCORE_RENDER_TARGET_DURANGO_H

#if	RSG_DURANGO // && __D3D11 __D3D11 toolbuilder_gta5_dev_rex_2_targets doesn`t like __D3D11 being here

#include	"atl/array.h"
#include	"grcore/device.h"
#include	"grcore/effect_mrt_config.h"
#include	"grcore/texture.h"
#include	"vector/vector3.h"
#include	"grcore/resourcecache.h"
#include	"grcore/rendertarget_common.h"
#include	"grcore/texturepc.h"
#include	"grcore/locktypes.h"

struct D3D11_TEXTURE2D_DESC;
struct XG_RESOURCE_LAYOUT;
enum XG_TILE_MODE;

namespace rage 
{

//enabling this test checks if the depth buffer has been written to determine if a copy is necessary, triggers assert if extra unnecessary copy detected.
#define D3D11_RENDER_TARGET_COPY_OPTIMISATION_TEST	(0)	
//Track DX11 runtime conflicts when the texture is being rendered to and bound to the shader
#define D3D11_TRACK_RT_VIOLATIONS	( __ASSERT && RSG_PC && __D3D11 )


#if DEVICE_EQAA
class grcTextureDurango;
class grcRenderTargetDurango;

struct CoverageData
{
	CoverageData();
	const grcTexture		*texture;
	grcRenderTargetDurango	*donor;
	bool					compressionEnabled;
	bool					manualDecoding;
	ResolveType				resolveType;
	u32						supersampleFrequency;
};
#endif // DEVICE_EQAA

class	grcRenderTargetDurango	: public grcRenderTarget
{
	friend class grcTextureFactoryDX11;
	friend class ESRAMManager;

public:
	grcRenderTargetDurango(const char *name,grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params, grcRenderTargetDurango * origTarget);
	grcRenderTargetDurango(const char *name, const grcTextureObject *pTexture, DepthStencilFlags depthStencilReadOnly = DepthStencilRW , grcTextureFactory::CreateParams *params = NULL);
	grcRenderTargetDurango(const char *name, const grcTextureObject *pTexture, grcTextureFormat eShaderViewFormat, DepthStencilFlags depthStencilReadOnly = DepthStencilRW, grcTextureFactory::CreateParams *params = NULL );
	virtual	~grcRenderTargetDurango();
private:
	void DestroyInternalData();
#if DYNAMIC_ESRAM
	bool AltSwapTest() const;
#endif

public:
	int					GetWidth		(void) const { return(m_Width); }
	int					GetHeight		(void) const { return(m_Height); }
	int					GetDepth		(void) const { return 1; }
	int					GetMipMapCount	(void) const { return m_MipLevels; }
	int					GetArraySize	(void) const { return m_ArraySize; }
	int					GetBitsPerPixel() const { return m_BitsPerPix; }
	virtual grcDevice::MSAAMode GetMSAA() const { return m_Multisample; }

	const grcTextureFormat GetFormat() const {return (grcTextureFormat)m_Format;}
	grcTexture::ChannelBits FindUsedChannels() const;

	virtual u32			GetImageFormat() const;

	const grcTextureObject  *GetTexturePtr(void) const;
	grcTextureObject		*GetTexturePtr(void);
	grcDeviceView			*GetTextureView();
	const grcDeviceView		*GetTargetView(u32 uMip = 0, u32 uLayer = 0);
	const grcDeviceView 	*GetUnorderedAccessView() const;
#if	DYNAMIC_ESRAM
	grcTextureObject		*GetTextureObject(void);
#endif

	inline const grcTextureObject*	DebugGetLastResolved() const;
	inline void 					DebugSetUnresolved();
#if DEBUG_TRACK_MSAA_RESOLVES
	bool							HasBeenResolvedTo(grcRenderTarget* resolveTo);
#endif
	void							Resolve(grcRenderTarget* resolveTo, int destSliceIndex = 0);
	inline grcTextureObject*		GetResolveTarget()	{ return GetTexturePtr(); }
	const grcTextureObject *		GetFragmentMaskTexture() const	{ return NULL; }


	bool				LockRect(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags = (grcsRead | grcsWrite)) const;
	void				UnlockRect(const grcTextureLock &/*lock*/) const;

	bool				Copy(const grcTexture* pSource, s32 dstSliceIndex, s32 dstMipIndex, s32 srcSliceIndex, s32 srcMipIndex);
	bool				CopyTo(grcImage* pImage, bool bInvert = false, u32 uPixelOffset = 0);
	bool				Copy2D(const void * /*pSrc*/, u32 /*imgFormat*/, u32 /*uWidth*/, u32 /*uHeight*/, u32 /*numMips*/) { return false; }

	void				ClearAsync(u32 clearVal0, u32 clearVal1);

	bool				IsValid() const;
	u32					GetRequiredMemory() const;
	void				Update();
	void				GenerateMipmaps();
	void				UpdateLockableTexture(bool bInitUpdate);
	void				UpdateMemoryLocation(const grcTextureObject *);

	void *              GetAllocationStart() { return m_VirtualAddress; }

#if DEVICE_EQAA
	const CoverageData& GetCoverageData() const	{ return m_Coverage; }
	void SetFragmentMaskDonor(grcRenderTargetDurango *donor, ResolveType resolveType);
	const grcTexture* GetResolveFragmentMask() const	{ return (m_Coverage.donor ? m_Coverage.donor : this )->GetCoverageData().texture; }
#endif // DEVICE_EQAA

#if	DYNAMIC_ESRAM
	void				SetAltTarget(grcRenderTarget* target){m_AltTarget = target;}
	grcRenderTarget*	GetAltTarget() const {return m_AltTarget;}
	void				SetUseAltTarget(bool bUse){m_UseAltTarget = bUse;}
	bool				GetUseAltTarget() const {return AltSwapTest();}
	void				SetUseAltTestFunc(bool (*funcptr)()){m_UseAltTestFunc = funcptr;}
#endif
	u64*				GetPlaneOffsets() {return m_PlaneOffsets;}
	u64*				GetPlaneSizes() {return m_PlaneSizes;}

protected:
	void				SetPrivateData();
	bool				SetTextureView(u32 uFormat);
	static bool			IsReadOnlyFormat(u32 format);
	static bool			IsDepthTypeOrFormat(u32 type, u32 format);
	u32					GetDepthTextureFormat();

	void				CreateFromTextureObject(const char *name, const grcTextureObject *pTexture, DepthStencilFlags depthStencilReadOnly = DepthStencilRW, grcTextureFactory::CreateParams *params = NULL );
	void				CreateFromTextureObject(const char *name, const grcTextureObject *pTexture, grcTextureFormat eShaderViewFormat, DepthStencilFlags depthStencilReadOnly = DepthStencilRW, grcTextureFactory::CreateParams *params = NULL );
	void				ReCreate(grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params);
	void				CreateSurface();
	void				CreateSRVAndTargetViews(DepthStencilFlags depthStencilFlags, u32 sliceCount);
	void				DestroySRVAndTargetViews();
	bool				CreateTargetView(u32 uArraySlice, u32 uMip, u32 uArraySize, DepthStencilFlags depthStencilReadOnly = DepthStencilRW);


	grcDeviceView		*m_pShaderResourceView;
	grcDeviceView		*m_pUnorderedAccessView;
	atArray<grcDeviceView*> m_TargetViews;

#if DEBUG_TRACK_MSAA_RESOLVES
	// Track resolves to ensure they are not done redundantly.  Tracking
	// requires multi-threaded rendering to be disabled.
	const grcTextureObject*	m_DebugLastResolvedObject;
#endif

#if	DYNAMIC_ESRAM
	grcRenderTarget		*m_AltTarget;
#endif
	grcDeviceTexture	*m_Texture;
	grcDeviceTexture	*m_LockableTexture;
	bool				m_Lockable;
	bool				m_LockableTexUpdated;
	u32					m_Format;
	u16					m_Width;
	u16					m_Height;
	u16					m_Depth;
	u8					Unused_Padding;
	u8					m_BitsPerPix;
	grcDevice::MSAAMode	m_Multisample;
#if !DEVICE_EQAA
	u8					m_MultisampleQuality;
#endif
	u8					m_ArraySize;
	u8					m_PerArraySliceRTV;

	u16					m_SlotId;
	u16					m_MipLevels;

	void*				m_VirtualAddress;
	u64					m_VirtualSize;
	void*				m_StagingVirtualAddress;
	void*				m_ESRAMVirtualAddress;
	u32					m_ESRAMNumPages;
	u64 				m_ESRAMPhase;
	u32					m_ESRAMMaxSize;

	//Stuff for Async clears
	u64					m_PlaneOffsets[2];
	u64					m_PlaneSizes[2];

#if	DYNAMIC_ESRAM
	bool				(*m_UseAltTestFunc)();
#endif

	bool				m_bUseAsUAV                     : 1;
	bool				m_CreatedFromTextureObject      : 1;
	bool				m_CreatedFromPreAllcatedMem     : 1;  // owner will free them mem
	bool                m_CompressedDepth               : 1;
#if	DYNAMIC_ESRAM
	bool				m_UseAltTarget                  : 1;
#endif

	DepthStencilFlags	m_DepthStencilFlags;

#if DEVICE_EQAA
	CoverageData		m_Coverage;
#endif // DEVICE_EQAA
};

class ESRAMManager 
{
public:
	static u32 GetMainPlaneId(XG_RESOURCE_LAYOUT& colorLayout);
	static void* Alloc(D3D11_TEXTURE2D_DESC &oDesc, XG_RESOURCE_LAYOUT& colorLayout,  XG_TILE_MODE& tileMode, grcRenderTargetDurango* rt);
	static void* VirtualMap(XG_RESOURCE_LAYOUT& colorLayout, void **pESRAMVirtualAddress, u32* ESRAMPageList, u32 ESRAMNumPages, bool bPartMap);
	static void Free(void *VirtualAddress, u64 size, void *ESRAMVirtualAddress, u32 ESRAMNumPages);
#if	DYNAMIC_ESRAM
	static void ESRAMCopyIn(grcRenderTarget* dst, grcRenderTarget* src);
	static void ESRAMCopyOut(grcRenderTarget* dst, grcRenderTarget* src);
#endif
};

inline const grcTextureObject* grcRenderTargetDurango::DebugGetLastResolved() const
{
#if DEBUG_TRACK_MSAA_RESOLVES
	return m_DebugLastResolvedObject;
#else
	return NULL;
#endif
}

inline void grcRenderTargetDurango::DebugSetUnresolved()
{
#if DEBUG_TRACK_MSAA_RESOLVES
	m_DebugLastResolvedObject = NULL;
#endif
}

// DOM-IGNORE-END

} // namespace rage

#endif // RSG_DURANGO

#endif // GRCORE_RENDER_TARGET_DURANGO_H
