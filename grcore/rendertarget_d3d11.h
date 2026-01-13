#ifndef GRCORE_RENDER_TARGET_DX11_H
#define GRCORE_RENDER_TARGET_DX11_H

#if	RSG_PC

#include	"atl/array.h"
#include	"grcore/device.h"
#include	"grcore/texture.h"
#include	"vector/vector3.h"
#include	"grcore/resourcecache.h"
#include	"grcore/texturepc.h"
#include	"grcore/locktypes.h"

#if RSG_DURANGO
struct D3D11_TEXTURE2D_DESC;
struct XG_RESOURCE_LAYOUT;
enum XG_TILE_MODE;
#endif

namespace rage 
{

//enabling this test checks if the depth buffer has been written to determine if a copy is necessary, triggers assert if extra unnecessary copy detected.
#define D3D11_RENDER_TARGET_COPY_OPTIMISATION_TEST	(0)	
//Track DX11 runtime conflicts when the texture is being rendered to and bound to the shader
#define D3D11_TRACK_RT_VIOLATIONS	( __ASSERT && RSG_PC && __D3D11 )


class	grcRenderTargetDX11	: public grcRenderTargetPC
{
	friend class grcTextureFactoryPC;
	friend class grcTextureFactoryDX11;

public:
	grcRenderTargetDX11(const char *name,grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params);
	grcRenderTargetDX11(const char *name, const grcTextureObject *pTexture, DepthStencilFlags depthStencilReadOnly = DepthStencilRW,grcTextureFactory::CreateParams *params = NULL );
	grcRenderTargetDX11(const char *name, const grcTextureObject *pTexture, grcTextureFormat eShaderViewFormat, DepthStencilFlags depthStencilReadOnly = DepthStencilRW,  grcTextureFactory::CreateParams *params = NULL );
	virtual	~grcRenderTargetDX11();

	void DestroyInternalData();

	const grcTextureObject  *GetTexturePtr	(void) const;
	grcTextureObject		*GetTexturePtr	(void);

	grcDeviceView *GetTextureView();
	const grcDeviceView *GetTextureView() const;

	const grcDeviceView *GetSecondaryTextureView() const { return m_pSecondaryShaderResourceView; }
	void SetSecondaryTextureView(const grcDeviceView *view) { m_pSecondaryShaderResourceView = view; }

	const grcDeviceView	*GetTargetView	(u32 uMip = 0, u32 uLayer = 0);

	const grcDeviceView 	*GetUnorderedAccessView() const;

	virtual bool		LockRect(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags = (grcsRead | grcsWrite)) const;
	virtual void		UnlockRect(const grcTextureLock &/*lock*/) const;

	virtual bool		Copy(const grcTexture* pSource, s32 dstSliceIndex, s32 dstMipIndex, s32 srcSliceIndex, s32 srcMipIndex);
	bool				CopyTo(grcImage* pImage, bool bInvert = false, u32 uPixelOffset = 0);
	bool				Copy2D(const void * /*pSrc*/, u32 /*imgFormat*/, u32 /*uWidth*/, u32 /*uHeight*/, u32 /*numMips*/) { return false; }

#if DEBUG_TRACK_MSAA_RESOLVES
	virtual bool		HasBeenResolvedTo(grcRenderTarget* resolveTo);
#endif

	using rage::grcRenderTargetPC::Resolve;
	virtual void		Resolve(grcRenderTarget* resolveTo, int destSliceIndex = 0);
	inline grcTextureObject* GetResolveTarget()	{ return GetTexturePtr(); }

	const grcTextureObject *GetFragmentMaskTexture() const	{ return NULL; }

	virtual bool		IsValid() const;

	u32					GetRequiredMemory() const;

	virtual void Update();

	virtual void	GenerateMipmaps();
	virtual u32		GetImageFormat() const;

	void UpdateLockableTexture(bool bInitUpdate);

#if D3D11_RENDER_TARGET_COPY_OPTIMISATION_TEST
	void SetHasBeenWrittenTo(bool val) {m_bHasBeenWrittenTo = val;}
	//Currently only used for depth buffer
	bool HasBeenWrittenTo() { return m_bHasBeenWrittenTo;}

	static void CheckDepthWriting();
#endif

protected:
	void				DeviceLost();
	void				DeviceReset();
	void				SetPrivateData();
	bool				CreateTargetView(u32 uArraySlice, u32 uMip, u32 uArraySize, DepthStencilFlags depthStencilReadOnly = DepthStencilRW);
	bool				SetTextureView(u32 uFormat);
	bool				IsReadOnlyFormat(u32 format);

	virtual void		ReCreate(grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params);
	virtual void		CreateSurface();
	virtual u32			GetDepthTextureFormat();
	void				CreateFromTextureObject(const char *name, const grcTextureObject *pTexture, DepthStencilFlags depthStencilReadOnly = DepthStencilRW, grcTextureFactory::CreateParams *params = NULL );
	void				CreateFromTextureObject(const char *name, const grcTextureObject *pTexture, grcTextureFormat eShaderViewFormat, DepthStencilFlags depthStencilReadOnly = DepthStencilRW, grcTextureFactory::CreateParams *params = NULL );

	grcDeviceView		*m_pShaderResourceView;
	const grcDeviceView	*m_pSecondaryShaderResourceView;
	grcDeviceView		*m_pUnorderedAccessView;
	atArray<grcDeviceView*> m_TargetViews;

#if RSG_DURANGO
	void*				m_VirtualAddress;
	u64					m_VirtualSize;
	void*				m_ESRAMVirtualAddress;
	u32					m_ESRAMNumPages;
	u32 				m_ESRAMPhase;
#endif
	bool				m_bUseAsUAV;
	bool				m_CreatedFromTextureObject;

#if D3D11_RENDER_TARGET_COPY_OPTIMISATION_TEST
	bool				m_bHasBeenWrittenTo;
#endif
};

// DOM-IGNORE-END

} // namespace rage

#endif // RSG_PC

#endif // GRCORE_RENDER_TARGET_DX11_H
