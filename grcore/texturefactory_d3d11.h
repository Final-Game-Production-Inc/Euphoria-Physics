#ifndef GRCORE_TEXTURE_FACTORY_DX11_H
#define GRCORE_TEXTURE_FACTORY_DX11_H

#if	RSG_PC || RSG_DURANGO // && __D3D11 toolbuilder_gta5_dev_rex_2_targets doesn`t like __D3D11 being here

#include	"atl/array.h"
#include	"grcore/device.h"
#include	"grcore/texture.h"
#include	"vector/vector3.h"
#include	"grcore/resourcecache.h"
#include	"grcore/texturepc.h"
#include	"grcore/locktypes.h"
#include	"grcore/texturedefault.h"

#define SRC_DST_TRACKING (0 && __DEV && RSG_PC)

namespace rage 
{

// DOM-IGNORE-BEGIN
class grcRenderTargetDX11;
class grcBufferD3D11Resource;

#if RSG_PC
class grcTextureFactoryDX11 : public grcTextureFactoryPC // PC versions.
#else // RSG_PC
class grcTextureFactoryDX11 : public grcTextureFactory // Durango version.
#endif // RSG_PC
{
public:
	static void CreatePagedTextureFactory();
public:
	grcTextureFactoryDX11();
	virtual ~grcTextureFactoryDX11();

#if __D3D11
	// Texture format functions.
	bool				SupportsFormat(grcTextureFormat);
	u32					Translate(grcTextureFormat);
	u32					GetD3DFormat(grcImage *);
	static u32			GetD3DFromatFromGRCImageFormat(u32 grcImageFormat);
	u32					GetImageFormat(u32);
	static u32			GetImageFormatStatic(u32 uInternalFormat);
	u32					GetBitsPerPixel(u32 uInternalFormat);
	static u32			GetD3D10BitsPerPixel(u32 uInternalFormat);
	static u32			TranslateToTypelessFormat(u32);
	static u32			TranslateToTextureFormat(u32);
	static grcTextureFormat	TranslateToRageFormat(u32 uD3DFormat);
	static u32			ConvertToD3DFormat(grcTextureFormat);
	static u32			TranslateDX9ToDX11Format(u32 uD3D9Format, bool bSRGB);
#endif // __D3D11

	// Texture creation functions.
	grcTexture			*Create				(const char *pFilename,TextureCreateParams *params);
	grcTexture			*Create				(class grcImage*,TextureCreateParams *params);
	grcTexture			*Create				(u32 width, u32 height, u32 eFormat, void* pBuffer, u32 numMips, TextureCreateParams *params);
	grcTexture			*Create				(u32 width, u32 height, u32 depth, u32 eFormat, void* pBuffer, TextureCreateParams *params);
#if DEVICE_EQAA
	grcTexture			*CreateAsFmask		(grcDeviceTexture *eqaaTexture, const grcDevice::MSAAMode &mode);
#endif // DEVICE_EQAA

#if __D3D11
	// Render target functions.
	grcRenderTarget		*CreateRenderTarget	(const char *pName, grcRenderTargetType eType, int nWidth, int nHeight, int nBitsPerPixel, CreateParams *params, grcRenderTarget* originalTarget = NULL);
	grcRenderTarget		*CreateRenderTarget	(const char *pName, const grcTextureObject *pTexture, grcRenderTarget* originalTarget = NULL);
	grcRenderTarget		*CreateRenderTarget	(const char *pName, const grcTextureObject *pTexture, grcTextureFormat textureFormat, DepthStencilFlags depthStencilReadOnly = DepthStencilRW, grcRenderTarget* originalTarget = NULL, rage::grcTextureFactory::CreateParams *_params = NULL);
	void				LockRenderTarget	(int index, const grcRenderTarget *pColor, const grcRenderTarget *pDepth, u32 layer = 0,bool lockDepth = true, u32 mipToLock = 0);
	void				UnlockRenderTarget	(int index,const grcResolveFlags * poResolveFlags);
	void				LockMRT				(const grcRenderTarget *pColor[grcmrtColorCount], const grcRenderTarget *pDepth, const u32 *mipsToLock = NULL);
	void				UnlockMRT			(const grcResolveFlagsMrt* resolveFlags = NULL);
	void				LockMRTWithUAV		(const grcRenderTarget *color[grcmrtColorCount], const grcRenderTarget *depth, const u32* layersToLock, const u32* mipsToLock, const grcBufferUAV* UAVs[grcMaxUAVViews], const u32* pUAVInitialCounts);
	void				LockDepthAndCurrentColorTarget( const grcRenderTarget *depth );
	void				UnlockDepthAndCurrentColorTarget(const grcResolveFlags * poResolveFlags);

	virtual u32			GetTextureDataSize	(u32 width, u32 height, u32 format, u32 mipLevels, u32 numSlices, bool bIsCubeMap, bool bIsLinear, bool bLocalMemory);

	bool				IsTargetLocked		(const grcRenderTarget *target) const;

	virtual void SetArrayView(u32 uArrayIndex);

private:
	void				LockMRTInternal		(const grcRenderTarget *pColor[grcmrtColorCount], const grcRenderTarget *pDepth, const u32 *layersToLock = NULL, const u32 *mipsToLock = NULL);
	void				UnlockMRTInternal	(const grcResolveFlagsMrt* resolveFlags = NULL);
public:
	void				PlaceTexture		(class datResource &, grcTexture &);
#endif // __D3D11

#if DEVICE_EQAA
	void FinishRendering();
	void EnableMultisample(grcRenderTarget *target);	// a workaround to mark created targets as multisampled, needed for S/1 EQAA
#endif // DEVICE_EQAA

	// Front/Back buffer functions.
	virtual const grcRenderTarget *GetBackBuffer(bool realize=true) const;
	virtual grcRenderTarget *GetBackBuffer(bool realize=true);
	virtual const grcRenderTarget *GetFrontBuffer(bool UNUSED_PARAM(nextBuffer)) const;
	virtual grcRenderTarget *GetFrontBuffer(bool UNUSED_PARAM(nextBuffer));
	virtual const grcRenderTarget *GetFrontBufferDepth(bool realize=true) const;
	virtual grcRenderTarget *GetFrontBufferDepth(bool realize=true);
	virtual const grcRenderTarget *GetBackBufferDepth(bool realize=true) const;
	virtual grcRenderTarget *GetBackBufferDepth(bool realize=true);

#if __D3D11
	virtual void BindDefaultRenderTargets() {}
	grcRenderTargetD3D11* GetActiveDepthBuffer();

	virtual void Lost();
	virtual void Reset();
#if RSG_PC
	grcTexture *CreateStereoTexture();
#endif	

	ASSERT_ONLY(s32 GetRenderTargetStackCount() { return s_RenderTargetStackTop;})
#endif // __D3D11

#if DEBUG_SEALING_OF_DRAWLISTS
	virtual void SetDrawListDebugString(const char *pStr);
	virtual char *GetDrawListDebugString();
	virtual bool AreAnyRenderTargetsSet();
	virtual bool HaveAnyDrawsBeenIssuedWithNoTargetsSet();
	virtual void OutputSetRenderTargets();
	virtual void OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS

#if __DEV
	static void StartLogging(fiStream* pStream) {pLoggingStream = pStream;}
	static void StopLogging() {pLoggingStream = NULL;}
#endif

#if SRC_DST_TRACKING
public:
	typedef std::multimap< const grcRenderTarget*, const grcRenderTarget* > MultiMapTargetTextureSources; // Destination Texture / Source Texture List
	static MultiMapTargetTextureSources sm_mmapSourceTextureList;
	static sysCriticalSectionToken		sm_Lock;
#else
protected:
#endif // SRC_DST_TRACKING

#if __DEV
	static fiStream* pLoggingStream;
#endif

	static const int	MAX_RENDER_TARGETS = 8;
	static const int   RENDER_TARGET_STACK_SIZE = 8;

#if __D3D11
	typedef struct _TARGETINFO
	{
		const grcRenderTargetD3D11*	apColorTargets[MAX_RENDER_TARGETS];
		const grcRenderTargetD3D11*	pDepthTarget;
		const grcDeviceView*		apColorViews[MAX_RENDER_TARGETS];
		const grcDeviceView*		pDepthView;
		bool						bDepthLocked;
		u32							uWidth;
		u32							uHeight;
		u32							uNumViews;
	} TARGETINFO;

	static __THREAD s32 s_RenderTargetStackTop; 
	static __THREAD TARGETINFO s_RenderTargetStack[RENDER_TARGET_STACK_SIZE];
#if DEBUG_SEALING_OF_DRAWLISTS
	static __THREAD bool s_DrawCallsBeenIssuedWithNoTargetsSet;
	static __THREAD char grcTextureFactoryDX11::s_DrawListDebugString[256];
#endif // DEBUG_SEALING_OF_DRAWLISTS

	static grcDeviceTexture	*sm_BackBufferTexture;

private:
	void				SetupMRTStack		(const grcRenderTarget *color[grcmrtColorCount], const grcRenderTarget *depth, const u32 *layersToLock, const u32 *mipsToLock, TARGETINFO*& pStackTop );
#endif // __D3D11
};

	// DOM-IGNORE-END

}	// namespace rage

#endif // #if	RSG_PC || RSG_DURANGO // && __D3D11 toolbuilder_gta5_dev_rex_2_targets doesn`t like __D3D11 being here

#endif // GRCORE_TEXTURE_FACTORY_DX11_H
