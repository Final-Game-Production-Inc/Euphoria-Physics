//
// grcore/texturexenon.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#if	__XENON

#include "channel.h"
#include "config.h"
#include "device.h"
#include "image.h"

#include "vector/matrix34.h"
#include "string/string.h"
#include "system/cache.h"
#include "system/memory.h"
#include "system/param.h"
#include "diag/tracker.h"
#include "data/resource.h"
#include "data/struct.h"
#include "grcore/texturexenon.h"
#include "grcore/texturereference.h"
#include "grcore/texturecontrol.h"
#include "grcore/channel.h"

#define DBG 0
#include "system/xtl.h"
#include <xgraphics.h>
#undef DBG

namespace rage {

// from device_d3d.cpp
extern D3DTexture *g_FrontBuffers[2];
extern int g_CurrentFrontBuffer;
extern grcDeviceSurface *g_BackBuffer;
extern grcDeviceSurface *g_DepthStencil;


XPARAM(hdr); // MC4 branch specific bugfix (pull out when we figure the correct integrations with the latest texturexenon code (without the m_format member))

inline u32 XGTEXTURE_DESC_GetSize(const XGTEXTURE_DESC& desc)
{
	return desc.WidthInBlocks * desc.HeightInBlocks * desc.DepthInBlocks * desc.BytesPerBlock;
}


grcTextureFactory *grcTextureFactory::CreatePagedTextureFactory(bool bMakeActive)
{
	grcTextureFactory *pFactory = rage_new grcTextureFactoryXenon;

	if	(bMakeActive)
	{
		sm_Instance = pFactory;
	}

	return pFactory;
}

grcTextureFactoryXenon::grcTextureFactoryXenon()
	: m_PreviousDepthLock(false)
	, m_PreviousDepth(0)
	, m_PreviousWidth(0)
	, m_PreviousHeight(0)
	, m_CurrentTarget(0)
	, m_CurrentDepth(0)
 {
	 for (u32 i = 0; i < grcmrtCount; ++i)
	 {
		 sm_DefaultRenderTargets[i].Resize(1);
		 sm_DefaultRenderTargets[i].End() = NULL;
	 }

	for (int i = 0; i < MAX_RENDER_TARGETS; ++i)
	{
		m_PreviousTargets[i] = 0;
		m_CurrentTargets[i] = 0;
	}

	grcRenderTargetPoolMgr::Init();

	m_FrontBuffers[0] = rage_new grcRenderTargetXenon((grcDeviceTexture *)g_FrontBuffers[0]);
	g_FrontBuffers[0]->AddRef();
	m_FrontBuffers[1] = rage_new grcRenderTargetXenon((grcDeviceTexture *)g_FrontBuffers[1]);
	g_FrontBuffers[1]->AddRef();
#if HACK_GTA4 //Could be useful to other 360 projects - triple buffer the swap chain - look at xdk docs for more info (search QuerySwapStatus)
	m_FrontBuffers[2] = rage_new grcRenderTargetXenon((grcDeviceTexture *)g_FrontBuffers[2]);
	g_FrontBuffers[2]->AddRef();
#endif

	// Grab the data used by the grcDevice on Xenon builds to encapsulate with a rendertarget/texture
	m_BackBuffer = rage_new grcRenderTargetXenon;
	m_BackBuffer->InitWithSurface((grcDeviceSurface *) g_BackBuffer);
	g_BackBuffer->AddRef();
	SetDefaultRenderTarget(grcmrtColor0, m_BackBuffer);

	// Use current front buffer memory as depth buffer
	u32 poolAllocSize;
	m_DepthBuffers[0] = rage_new grcRenderTargetXenon;
	m_DepthBuffers[0]->InitWithDepthSurface((grcDeviceSurface *) g_DepthStencil, &poolAllocSize);
	m_DepthBuffers[0]->m_Texture->Format.BaseAddress = g_FrontBuffers[0]->Format.BaseAddress;

	m_DepthBuffers[1] = rage_new grcRenderTargetXenon;
	m_DepthBuffers[1]->InitWithDepthSurface((grcDeviceSurface *) g_DepthStencil, &poolAllocSize);
	m_DepthBuffers[1]->m_Texture->Format.BaseAddress = g_FrontBuffers[1]->Format.BaseAddress;

#if HACK_GTA4
	m_DepthBuffers[2] = rage_new grcRenderTargetXenon;
	m_DepthBuffers[2]->InitWithDepthSurface((grcDeviceSurface *) g_DepthStencil, &poolAllocSize);
	m_DepthBuffers[2]->m_Texture->Format.BaseAddress = g_FrontBuffers[2]->Format.BaseAddress;
#endif //HACK_GTA4

	g_DepthStencil->AddRef();
}

grcTextureFactoryXenon::~grcTextureFactoryXenon()
{
	m_BackBuffer->Release();
	m_DepthBuffers[0]->Release();
	m_DepthBuffers[1]->Release();
	m_FrontBuffers[0]->Release();
	m_FrontBuffers[1]->Release();
#if HACK_GTA4
	m_DepthBuffers[2]->Release();
	m_FrontBuffers[2]->Release();
#endif //HACK_GTA4
}

grcTexture *grcTextureFactoryXenon::Create(const char *pFilename,grcTextureFactory::TextureCreateParams *params)
{
	char Buffer[256];

	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);
	RAGE_TRACK_NAME( pFilename );

	sysMemUseMemoryBucket TEXTURES(grcTexture::sm_MemoryBucket);

	StringNormalize(Buffer, pFilename, sizeof(Buffer));

	grcTexture	*tex = LookupTextureReference(Buffer);
	if (tex)
		return tex;

	tex = rage_new grcTextureXenon(Buffer,params);

	// Did the creation work?
	if (!tex->GetTexturePtr())
	{
		// Nope.
		tex->Release();
		return NULL;
	}

	return(tex);
}

grcTexture *grcTextureFactoryXenon::Create(grcImage *pImage,grcTextureFactory::TextureCreateParams *params)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);
	
	grcTexture *tex = rage_new grcTextureXenon(pImage,params);

	// Did the creation work?
	if (!tex->GetTexturePtr())
	{
		// Nope.
		tex->Release();
		return NULL;
	}

	return(tex);
}

grcTexture *grcTextureFactoryXenon::Create(u32 width, u32 height, u32 format, void* pBuffer, u32 mipLevels, TextureCreateParams * /*params*/)
{     
   Assert(pBuffer && "pBuffer must be allocated");

   D3DTexture* pTexture = rage_new D3DTexture;

   XGSetTextureHeader( 
      width,
      height,
      mipLevels, 
      D3DUSAGE_CPU_CACHED_MEMORY,
      (D3DFORMAT)format,
      0,
      0,
      XGHEADER_CONTIGUOUS_MIP_OFFSET,
      0,
      pTexture,
      NULL,
      NULL ); 

	if (mipLevels > 1)
	{	
		// Because we use XGHEADER_CONTIGUOUS_MIP_OFFSET, pass the same address both 
		// for the base and mip offsets
		XGOffsetBaseTextureAddress( pTexture, pBuffer, pBuffer );
	}
	else
	{
		pTexture->Format.BaseAddress = ((DWORD)pBuffer) >> 12;
	}
   pTexture->Common |= D3DCOMMON_CPU_CACHED_MEMORY;
   

   grcTexture *tex = rage_new grcTextureXenon(width, height, mipLevels, pTexture);

   return tex;
}


grcRenderTarget* grcTextureFactoryXenon::CreateRenderTarget(const char *pName, const grcTextureObject *pTexture)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(RenderTarget);
	RAGE_TRACK_NAME(pName);

	grcRenderTarget * rt = rage_new grcRenderTargetXenon( pName, pTexture);

	RegisterRenderTarget(rt);

	return rt;
}



grcRenderTarget *grcTextureFactoryXenon::CreateRenderTarget( const char *name, grcRenderTargetType type, int width, int height, int bitsPerPixel, CreateParams *_params) 
{

	RAGE_TRACK(Graphics);
	RAGE_TRACK(grcRenderTarget);
	RAGE_TRACK_NAME(name);

	CreateParams params;
	if (_params)
		params = *_params;

	if (params.PoolID==kRTPoolIDInvalid)
	{
		// check for old style memory pool allocation, etc.
		if (grcRenderTargetMemPool * oldStylePool = grcRenderTargetMemPool::GetActiveMemPool())
		{ 
			params.PoolID = oldStylePool->GetPoolID();
			params.PoolHeap = oldStylePool->GetActiveHeap();
			params.AllocateFromPoolOnCreate = true;
		}
	} 
	

	grcRenderTarget * rt = rage_new grcRenderTargetXenon( name, type, width, height, bitsPerPixel, &params);

	RegisterRenderTarget(rt);

	return rt;
}


void grcRenderTargetPoolEntry::InitializeMemory(const grcRTPoolCreateParams & /*params*/)
{
	// xenon does not really need to do any of the ps3 tile checks ,etc...
	m_IsInitialised = true;
}


// AllocatePoolMemory() is platform specific
void grcRenderTargetPoolEntry::AllocatePoolMemory(u32 size, bool /*physical*/, int /*alignment*/, void * buffer)
{
	m_Size = size;

	if( buffer )
	{
		m_AllocatedMemory = false;
		m_PoolMemory = buffer;
	}
	else
	{
		m_AllocatedMemory = true;
		DWORD dwAllocAttributes = MAKE_XALLOC_ATTRIBUTES( 0, FALSE, TRUE, FALSE, 128, XALLOC_PHYSICAL_ALIGNMENT_4K, XALLOC_MEMPROTECT_WRITECOMBINE, FALSE, XALLOC_MEMTYPE_PHYSICAL );
		m_PoolMemory = XMemAlloc(size,dwAllocAttributes);
		Assert(m_PoolMemory);
	}
}

void grcRenderTargetPoolEntry::FreePoolMemory()
{
	if (m_AllocatedMemory)
	{
		DWORD dwAllocAttributes = MAKE_XALLOC_ATTRIBUTES( 0, FALSE, TRUE, FALSE, 128, XALLOC_PHYSICAL_ALIGNMENT_4K, XALLOC_MEMPROTECT_WRITECOMBINE, FALSE, XALLOC_MEMTYPE_PHYSICAL );
		XMemFree(m_PoolMemory,dwAllocAttributes);
	}

	m_PoolMemory = NULL;
}




void grcTextureFactoryXenon::LockRenderTarget(int index,const grcRenderTarget *_target, const grcRenderTarget *_depth, u32 layer, bool lockDepth, u32 D3D11_OR_ORBIS_ONLY(mipToLock))
{
	const u32 mipLevel = 0;	// nobody ever used this when it was a parameter?

	// Dirty dirty const casting!!!
	grcRenderTargetXenon* target = const_cast<grcRenderTargetXenon*>(static_cast<const grcRenderTargetXenon*>(_target));
	grcRenderTargetXenon* depth = const_cast<grcRenderTargetXenon*>(static_cast<const grcRenderTargetXenon*>(_depth));

	Assert(!target || (layer < target->GetLayerCount() && mipLevel < (u32)target->GetMipMapCount()));
	Assert(!depth || depth->GetMipMapCount() == 1);
	Assert(index < MAX_RENDER_TARGETS && "Index is outside range of supported render targets");
	Assert(!m_PreviousTargets[index]);
	Assert(index || !m_PreviousDepth);
	Assert(target || depth);

	GRCDEVICE.GetRenderTarget(index, &m_PreviousTargets[index]);

	IDirect3DSurface9* d3dTargetSurface;
	IDirect3DSurface9* d3dDepthSurface;
	if (target)
	{
		target->LockSurface(layer, mipLevel);
		d3dTargetSurface = target->GetSurface();
	}
	else
	{
		d3dTargetSurface = NULL;
	}
	if (depth && lockDepth)
	{
		// When target is a cube map, there is a good chance that depth is just a 2D texture
		// To avoid problems, we ignore the input layer in those cases
		depth->LockSurface(depth->GetType() == grcrtCubeMap ? layer : 0, mipLevel);
		d3dDepthSurface = depth->GetSurface();
	}
	else
	{
		d3dDepthSurface = NULL;
	}

	GRCDEVICE.SetRenderTarget(index, d3dTargetSurface);
	
	if (index == 0)
	{
		if (lockDepth)
		{
			GRCDEVICE.GetDepthStencilSurface(&m_PreviousDepth);
			GRCDEVICE.SetDepthStencilSurface(d3dDepthSurface);
			m_CurrentDepth = static_cast<grcRenderTarget*>(depth);
		}
		
		m_PreviousWidth = GRCDEVICE.GetWidth();
		m_PreviousHeight = GRCDEVICE.GetHeight();
		
		if (target)
		{
			GRCDEVICE.SetSize(Max(target->GetWidth()>>mipLevel,1),Max(target->GetHeight()>>mipLevel,1));
		}
		else if (depth)
		{
			GRCDEVICE.SetSize(depth->GetWidth(),depth->GetHeight());
		}
		m_PreviousDepthLock = lockDepth;
	}
	else
	{
		// NOTE: What do we want to do here?  Warn that width/height not same as index 0????
		//	 Warn that addition depth surfaces will be ignored???
	}
	
	m_CurrentTargets[index] = static_cast<grcRenderTarget*>(target);

	grcRenderTarget::LogTexture("LockRenderTarget",m_CurrentTargets[index]);
	if (index == 0 && lockDepth)
		grcRenderTarget::LogTexture("LockRenderTarget",m_CurrentDepth);
}


void grcTextureFactoryXenon::UnlockRenderTarget(int index, const grcResolveFlags* resolveFlags)
{
	Assert(index < MAX_RENDER_TARGETS && "Index is outside range of supported render targets");
	Assert(index || m_PreviousTargets[index]);

	grcRenderTarget::LogTexture("UnlockRenderTarget",m_CurrentTargets[index]);
	if (index == 0 && m_PreviousDepthLock) 
		grcRenderTarget::LogTexture("UnlockRenderTarget",m_CurrentDepth);

	// we can't use a texture atlas and specify offset values at the same time ... this is checked in Realize()

	// do the depth realize first, since color realize can clear depth during resolve
	if (index == 0 && m_PreviousDepthLock && m_CurrentDepth && (!resolveFlags || resolveFlags->NeedResolve))
	{
		m_CurrentDepth->Realize((m_CurrentTargets[index])?NULL:resolveFlags, index); // pass null clear flags to depth resolve, unless we don't have a color buffer.

		// we need auto-generated mip-maps for the depth buffer here ...
		if (m_CurrentDepth->GetMipMapCount() > 1 && (!resolveFlags || resolveFlags->MipMap))
		{
			m_CurrentDepth->CreateMipMaps( resolveFlags , index);
		}
	}

	grcRenderTarget* currentRt = m_CurrentTargets[index];
	if (currentRt)
	{
		if (resolveFlags)
		{
			{
				grcResolveFlags flags = *resolveFlags;
				if (!m_CurrentDepth)
				{
					flags.ClearDepthStencil = false; // clearing depth when there is none corrupts the framebuffer
				}

				if (resolveFlags->NeedResolve)
				{
					currentRt->Realize(&flags, index);
				}
			}

			// blur the result if required
			if (resolveFlags->BlurResult)
			{
				currentRt->Blur(resolveFlags);
			}
		}
		else
		{
			currentRt->Realize(NULL, index);
		}

		// we need a few auto-generated mip-maps for color maps here ...
		if (currentRt->GetMipMapCount() > 1 && (!resolveFlags || resolveFlags->MipMap))
		{
			currentRt->CreateMipMaps(resolveFlags, index);	
		}
	}

	if (index == 0)
	{
		if (m_PreviousDepthLock)
		{
			GRCDEVICE.SetDepthStencilSurface(m_PreviousDepth);
			m_PreviousDepth->Release();
		}
		m_PreviousDepth = 0;
		GRCDEVICE.SetSize(m_PreviousWidth,m_PreviousHeight);
	}

	// There MAY be a bug in the XeDK where changing the current render target before resolving 
	// the depth target causes the depth target to be resolved incorrectly.  In any case, it
	// didn't work correctly on beta hardware until I moved these lines until after the block above.
	GRCDEVICE.SetRenderTarget(index, m_PreviousTargets[index]);
	if (m_PreviousTargets[index])
	{
		m_PreviousTargets[index]->Release();
	}
	m_PreviousTargets[index] = 0;
}

void grcTextureFactoryXenon::LockMRT(const grcRenderTarget* color[grcmrtColorCount],const grcRenderTarget * depth, const u32* D3D11_ONLY(mipsToLock))
{
	for (s32 iIndex = 0; iIndex < grcmrtColorCount; iIndex++)
	{
		LockRenderTarget(iIndex, color[iIndex], depth, 0, true);
	}
}

void grcTextureFactoryXenon::UnlockMRT(const grcResolveFlagsMrt* resolveFlags)
{
	for (s32 iIndex = 0; iIndex < grcmrtColorCount; iIndex++)
	{
		UnlockRenderTarget(iIndex, (resolveFlags != NULL) ? (const grcResolveFlags*)resolveFlags[iIndex] : NULL);
	}
}

grcRenderTarget *grcTextureFactoryXenon::GetBackBuffer(bool realize) {
	Assert("Can't access backbuffer while it is not being used" && (m_PreviousTargets[0] == 0 || !realize));
	// Make sure backbuffer is in main memory
	grcResolveFlags flags;
	if(m_BackBuffer->GetSurface()->Format == D3DFMT_A2B10G10R10F_EDRAM)
		flags.ColorExpBias = -GRCDEVICE.GetColorExpBias();
	flags.ClearColor = false;
	flags.ClearDepthStencil = false;
	if (realize)
		m_BackBuffer->Realize(&flags ,0);
	return m_BackBuffer;
}

const grcRenderTarget *grcTextureFactoryXenon::GetBackBuffer(bool realize) const {
		return GetBackBuffer(realize);
}

grcRenderTarget *grcTextureFactoryXenon::GetFrontBuffer(bool nextBuffer) {
	if (nextBuffer)	// the one we are going to write to this frame
		return m_FrontBuffers[g_CurrentFrontBuffer];
	else			// the one we wrote to last frame...
#if HACK_GTA4 //Could be useful to other 360 projects - triple buffer the swap chain - look at xdk docs for more info (search QuerySwapStatus)
		return m_FrontBuffers[((g_CurrentFrontBuffer==0)?2:g_CurrentFrontBuffer-1)];
#else
		return m_FrontBuffers[1-g_CurrentFrontBuffer];
#endif
}

const grcRenderTarget *grcTextureFactoryXenon::GetFrontBuffer(bool nextBuffer) const {
	return GetFrontBuffer(nextBuffer);
}

grcRenderTarget* grcTextureFactoryXenon::GetFrontBufferFromIndex(int index) {
	return m_FrontBuffers[index];
}

const grcRenderTarget* grcTextureFactoryXenon::GetFrontBufferFromIndex(int index) const {
	return GetFrontBufferFromIndex(index);
}

int grcTextureFactoryXenon::GetCurrentFrontBufferIndex()
{
	return g_CurrentFrontBuffer;
}

const grcRenderTarget *grcTextureFactoryXenon::GetFrontBufferDepth(bool realize) const {
	return GetBackBufferDepth(realize);
}

grcRenderTarget *grcTextureFactoryXenon::GetFrontBufferDepth(bool realize) {
	return GetBackBufferDepth(realize);
}

grcRenderTarget *grcTextureFactoryXenon::GetBackBufferDepth(bool realize) {
	Assert("Can't access zbufffer while it is not being used" && (m_PreviousDepth == 0 || !realize));
	grcRenderTarget* depthBuffer = m_DepthBuffers[g_CurrentFrontBuffer];
	if (realize)
		depthBuffer->Realize(NULL,0);
	return depthBuffer;
}

const grcRenderTarget *grcTextureFactoryXenon::GetBackBufferDepth(bool realize) const {
	return GetBackBufferDepth(realize);
}

// Just like GetBackBufferDepth, but does away with the assert.  Sometimes the game just knows better.
const grcRenderTarget *grcTextureFactoryXenon::GetBackBufferDepthForceRealize()
{
	grcRenderTarget* depthBuffer = m_DepthBuffers[g_CurrentFrontBuffer];
	depthBuffer->Realize(NULL,0);
	return depthBuffer;
}


bool grcTextureFactoryXenon::CanGetBackBufferDepth()
{
	return (m_PreviousDepth == 0);
}

bool grcTextureFactoryXenon::CanGetBackBuffer()
{
	return (m_PreviousTargets[0] == 0);
}

u32 grcTextureFactoryXenon::GetTextureDataSize(u32 width, u32 height, u32 format, u32 mipLevels, u32 numSlices, bool UNUSED_PARAM(bIsCubeMap), bool UNUSED_PARAM(bIsLinear), bool UNUSED_PARAM(bLocalMemory))
{

	u32 mips        = mipLevels;
	u32 layers      = numSlices+1;
	grcImage::Format imgFormat	= (grcImage::Format)format;
	u32 bpp         = grcImage::GetFormatBitsPerPixel(imgFormat);
	u32 blockSize   = grcImage::IsFormatDXTBlockCompressed(imgFormat) ? 4 : 1;
	u32 w           = Max<u32>(blockSize, width);
	u32 h           = Max<u32>(blockSize, height);
	u32 sizeInBytes = 0;

	while (mips != 0)
	{
		u32 pitch = w * bpp / 8;
		
		// pitch must be a multiple of 256 bytes
		pitch = ((pitch+255) & (~255));

		sizeInBytes += (h*layers*pitch);
		w = Max<u32>(blockSize, w/2);
		h = Max<u32>(blockSize, h/2);
		mips--;
	}

	return sizeInBytes;
}

// ----------------------------------------------
// ** grcRenderTargetXenon
// ----------------------------------------------

grcRenderTargetXenon::grcRenderTargetXenon(grcDeviceTexture *baseTex) {
	m_Name = StringDuplicate("Custom Color Tex");
	m_Texture = baseTex;
	m_CachedTexturePtr = m_Texture;
	m_PoolID = kRTPoolIDInvalid;
	m_PoolHeap = 0;
	m_OffsetInPool = 0;
	m_PoolAllocSize = 0;
	m_CurrentPoolMemPtr = NULL;

	XGTEXTURE_DESC surfaceDescription;
	XGGetTextureDesc(m_Texture, 0, &surfaceDescription);
	SetPhysicalSize(XGTEXTURE_DESC_GetSize(surfaceDescription));

	m_Width = (u16) surfaceDescription.Width;
	m_Height = (u16) surfaceDescription.Height;

	for (u32 i = 1; i < m_Texture->GetLevelCount(); ++i)
	{
		XGGetTextureDesc(m_Texture, i, &surfaceDescription);
		SetPhysicalSize(GetPhysicalSize() + XGTEXTURE_DESC_GetSize(surfaceDescription));
	}
	
	m_Format = grctfA8R8G8B8;  
	m_Surface = 0;

// 	char buffer[128];
// 	prettyprinter(buffer, 128, GetPhysicalSize());
// 	Printf("\"%s\" %dx%dx%d %d %11s\n", GetName(), GetWidth(), GetHeight(), GetDepth(), surfaceDescription.Format, buffer);
}

grcRenderTargetXenon::grcRenderTargetXenon()
	: m_Width(0)
	, m_Height(0)
	, m_Bits(0)
	, m_Surface(0)
	, m_Format(grctfA8R8G8B8)
	, m_LockedLayer(0)
	, m_LockedMip(0)
	, m_PoolAllocSize(0)
	, m_MipAddrOffset(0)
	, m_CurrentPoolMemPtr(NULL)
	, m_Texture(NULL)
{
}

extern D3DFORMAT g_grcDepthFormat;


grcRenderTargetXenon::grcRenderTargetXenon(const char* name)
	: m_Width(0)
	, m_Height(0)
	, m_Bits(0)
	, m_Surface(0)
	, m_Format(grctfA8R8G8B8)
	, m_LockedLayer(0)
	, m_LockedMip(0)
	, m_PoolAllocSize(0)
	, m_MipAddrOffset(0)
	, m_CurrentPoolMemPtr(NULL)
	, m_Texture(NULL)
{
	m_Name = StringDuplicate(name);
}
grcRenderTargetXenon::grcRenderTargetXenon( const char *name, grcRenderTargetType type, 
										   int width, int height, int bpp, grcTextureFactory::CreateParams *_params) 
{
	grcTextureFactory::CreateParams params;
	if (_params)
		params = *_params; 

	m_Name = StringDuplicate(name);
	m_Width = (u16) width;
	m_Height = (u16) height;
	m_LayerCount = (type == grcrtCubeMap) ? 6 : 1;
	m_Type = type;
	m_Texture = 0;
	m_Surface = 0;
	
	m_PoolID = params.PoolID;
	m_PoolHeap = params.PoolHeap;
	m_OffsetInPool = params.PoolOffset;
	m_PoolAllocSize = 0;
	m_MipAddrOffset = 0;
	m_CurrentPoolMemPtr = NULL;

	bool useFloat = params.UseFloat;

	int multisample = params.Multisample;
	bool isResolvable = params.IsResolvable;
	bool isRenderable = params.IsRenderable;
	bool useHierZ =  params.UseHierZ;
	int mipLevels =  params.MipLevels;

	bool preallocated = params.basePtr != 0 && params.mipPtr != 0;

	//m_Format = params.Format;

	D3DMULTISAMPLE_TYPE multisampleType = multisample==4 ? D3DMULTISAMPLE_4_SAMPLES 
		: multisample==2? D3DMULTISAMPLE_2_SAMPLES : D3DMULTISAMPLE_NONE;
	grcTextureFormat tf = params.Format;
	// I would like to remove the 32 and 16 bpp flags at some point: anyone a problem with that?
	if (tf == grctfNone)
		tf = useFloat? (bpp==32? grctfR32F : grctfR16F) : (bpp==32? grctfA8R8G8B8 : grctfR5G6B5);

	m_Format = tf;

	// grcTextureFormat_REFERENCE
	static const D3DFORMAT edramFormatLU[] = {
		D3DFMT_UNKNOWN,
		D3DFMT_A8R8G8B8,   // use in edram for D3DFMT_R5G6B5 texture
		D3DFMT_A8R8G8B8,
		D3DFMT_G16R16_EDRAM,   // D3DFMT_R16F is not a valid render target format on 360
		D3DFMT_R32F,
		D3DFMT_A2B10G10R10F_EDRAM,
		D3DFMT_A2R10G10B10,			// A10B10G10R10 for ATI graphics cards on PC or a 10-bit integer format for 360
		D3DFMT_A16B16G16R16F,
		D3DFMT_G16R16_EDRAM,
		D3DFMT_G16R16F,
		D3DFMT_A32B32G32R32F,
		D3DFMT_A16B16G16R16F,
		D3DFMT_A16B16G16R16_EDRAM,
		D3DFMT_A8R8G8B8,
		D3DFMT_R32F,
		D3DFMT_A8R8G8B8,
		D3DFMT_A8R8G8B8,
		D3DFMT_A8R8G8B8,  // use in edram for D3DFMT_A1R5G5B5 texture
		D3DFMT_D24S8,
		D3DFMT_A8R8G8B8,  // use in edram for D3DFMT_A4R4G4B4 texture 
#if __XENON
		D3DFMT_G32R32F,
		D3DFMT_D24FS8,
#else
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
#endif
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_Q8W8V8U8,
		D3DFMT_UNKNOWN
	};
	CompileTimeAssert(NELEM(edramFormatLU) == grctfCount);

	// grcTextureFormat_REFERENCE
	static const D3DFORMAT texFormatLU[] = {
		D3DFMT_UNKNOWN,
		D3DFMT_R5G6B5,
		D3DFMT_A8R8G8B8,
		D3DFMT_R16F,
		D3DFMT_R32F,
		D3DFMT_A16B16G16R16F_EXPAND,
		D3DFMT_A2R10G10B10,		// A10B10G10R10 for ATI graphics cards on PC or a 10-bit integer format for 360
		D3DFMT_A16B16G16R16F_EXPAND,
		D3DFMT_G16R16,
		D3DFMT_G16R16F,
		D3DFMT_A32B32G32R32F,
		D3DFMT_A16B16G16R16F,
		D3DFMT_A16B16G16R16,
		D3DFMT_L8,
		D3DFMT_L16,
		D3DFMT_UNKNOWN,
		D3DFMT_G8R8,
		D3DFMT_A1R5G5B5,
		D3DFMT_D24S8,
		D3DFMT_A4R4G4B4,
#if __XENON
		D3DFMT_G32R32F,
		D3DFMT_A8R8G8B8,
#else
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
#endif
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_Q8W8V8U8,
		D3DFMT_UNKNOWN
	};
	CompileTimeAssert(NELEM(texFormatLU) == grctfCount);


	D3DFORMAT edramFormat = edramFormatLU[tf];
	D3DFORMAT texFormat = texFormatLU[tf];

	if (texFormat == D3DFMT_A16B16G16R16)
		texFormat = (D3DFORMAT)MAKED3DFMT(GPUTEXTUREFORMAT_16_16_16_16, GPUENDIAN_8IN16, TRUE, GPUSIGN_ALL_SIGNED, GPUNUMFORMAT_INTEGER, GPUSWIZZLE_ABGR);

	if (!params.InTiledMemory)
	{
		texFormat = (D3DFORMAT)MAKELINFMT(texFormat);
	}

	const DWORD edramGpuFormat = (edramFormat << D3DFORMAT_TEXTUREFORMAT_SHIFT) & D3DFORMAT_TEXTUREFORMAT_MASK;
	if (params.IsSRGB &&
		edramGpuFormat == GPUTEXTUREFORMAT_8_8_8_8 ) 
	{
		edramFormat = static_cast<D3DFORMAT>(MAKESRGBFMT(edramFormat));
		texFormat = static_cast<D3DFORMAT>(MAKESRGBFMT(texFormat));
	}

	D3DSURFACE_PARAMETERS spBuf = { 0, 0, 0, D3DHIZFUNC_DEFAULT };
	D3DSURFACE_PARAMETERS *sp = NULL;
	if (params.HasParent) 
	{
		// TODO: When we re-enable hier-z, use XGHierarchicalZSize to compute appropriate tile memory
		sp = &spBuf;
		if (params.Parent) {
			D3DSurface *parentSurface = ((grcRenderTargetXenon*)params.Parent)->m_Surface;
			// NOTE: ColorInfo and DepthInfo are in a union together but ColorBase and DepthBase overlap properly.
			DWORD base = parentSurface->ColorInfo.ColorBase;
			// Size appears to be conveniently expressed in bytes for us already.
			spBuf.Base = base + ((parentSurface->Size + GPU_EDRAM_TILE_SIZE-1) / GPU_EDRAM_TILE_SIZE);
		}
		else	// AllocateSwapChain allocates the depthstencil first so use that to figure out hier z base size.
			spBuf.Base = g_DepthStencil->DepthInfo.DepthBase;
		
		spBuf.ColorExpBias = params.ColorExpBias;
	}

	bool allocFromPoolOnCreate=params.AllocateFromPoolOnCreate;	
	
	// We need to keep a d3d surface for the render targets EDRAM usage, 
	//	and a d3d texture for use when rendering.
	if (type == grcrtPermanent || type == grcrtBackBuffer || type == grcrtCubeMap) {
		bpp = 32;	// HACK - get "ERR[D3D]: Can't render to requested format." if you try 16.
		if ( type == grcrtBackBuffer )
		{
			(m_Surface = g_BackBuffer)->AddRef();
		}
		else if (isRenderable)
		{
			const char* message = NULL;
			switch (IDirect3DDevice9_CreateRenderTarget(NULL, width, height, edramFormat, multisampleType, 0, false, &m_Surface, sp))
			{
			case D3DERR_NOTAVAILABLE:
				message = "D3DERR_NOTAVAILABLE";
				break;
			case D3DERR_INVALIDCALL:
				message = "D3DERR_INVALIDCALL";
				break;
			case D3DERR_OUTOFVIDEOMEMORY:
				message = "D3DERR_OUTOFVIDEOMEMORY";
				break;
			case E_OUTOFMEMORY:
				message = "E_OUTOFMEMORY : This is out of Edram Memory , so modifying the creation params could solve this";
				break;
			default:
				break;
			}
			Assertf(!message, "Unable to create %d x %d color render target (%s)", width, height, message);
		}
		
		// if it is resolvable
		if(isResolvable)
		{
			if (FAILED(GRCDEVICE.CreateTexture(width,height,1,mipLevels,D3DUSAGE_RENDERTARGET,
				texFormat, D3DPOOL_DEFAULT, type == grcrtCubeMap ? grcImage::CUBE : grcImage::STANDARD, &m_Texture, (m_PoolID == kRTPoolIDInvalid && !preallocated)?NULL:&m_PoolAllocSize)))
			{
				Quitf("Unable to create backup texture for rendertarget - try using @extramemory=NNN on command line (not response file) (this is usually caused by having optimizations locally disabled in some files)");
			}
			

			if(m_PoolID != kRTPoolIDInvalid)
			{
				if(m_PoolID == kRTPoolIDAuto) // allocate a pool just the right size if they asked for it
				{
					m_PoolID = grcTextureFactory::CreateAutoRTPool(name, params, m_PoolAllocSize);
				}

				Assert(m_PoolAllocSize<=grcRenderTargetPoolMgr::GetAllocated(m_PoolID));
				Assert(m_Texture->Format.BaseAddress==0);
				m_MipAddrOffset = m_Texture->Format.MipAddress; // save this for later when the render target is allocated from the pool
			}
		}
	}
	else if (type == grcrtDepthBuffer || type == grcrtShadowMap) 
	{
		spBuf.HierarchicalZBase = useHierZ ? params.HiZBase : 0xffffffff;

#if __ASSERT
		// Make sure that we aren't setting a HiZ base that will put us beyond the end of embedded HiZ memory
		if ( useHierZ )
		{
			u32 HiZSize = XGHierarchicalZSize( width, height, multisampleType );
			if ( GPU_HIERARCHICAL_Z_TILES < HiZSize + params.HiZBase )
			{
				Assertf( false,"Depth buffer (\"%s\") is trying to overrun HiZ memory: base=0x%x size=0x%x (max HiZ tile addr=0x%x)\n",m_Name, params.HiZBase, HiZSize, GPU_HIERARCHICAL_Z_TILES);
			}
		}
#endif // __ASSERT

		D3DFORMAT DepthFormat;

		if(m_Format == grctfD24S8)
		{
			DepthFormat = edramFormat;
		}
		else
		{
			Assertf(bpp==32,"D3DFMT_D16 is not a valid render target resolution on the 360. Please set bpp to 32");
			DepthFormat = g_grcDepthFormat;
		}

		if ( isRenderable && FAILED(IDirect3DDevice9_CreateDepthStencilSurface(NULL, width,height, DepthFormat,multisampleType,0,0,&m_Surface,sp)))
			Quitf("Unable to create %d x %d depth render target - try using @extramemory=NNN on command line (not response file) (this is usually caused by having optimizations locally disabled in some files)",width,height);

		// if it is resolvable
		if(isResolvable)
		{
			if(m_Format == grctfD24S8)
				DepthFormat = texFormat;
			else
				DepthFormat = (bpp==32? g_grcDepthFormat : D3DFMT_D16);

			if (FAILED(GRCDEVICE.CreateTexture(width, height, 1, mipLevels,D3DUSAGE_RENDERTARGET, DepthFormat,D3DPOOL_DEFAULT,grcImage::STANDARD,&m_Texture, (m_PoolID == kRTPoolIDInvalid)?NULL:&m_PoolAllocSize)))
				Quitf("Unable to create backup texture for depth render target - try using @extramemory=NNN on command line (not response file) (this is usually caused by having optimizations locally disabled in some files)");
		
			if(m_PoolID != kRTPoolIDInvalid)
			{
				if(m_PoolID == kRTPoolIDAuto) // allocate a pool just the right size if they asked for it
				{
					m_PoolID = grcTextureFactory::CreateAutoRTPool(name, params, m_PoolAllocSize);
				}

				Assert(m_Texture->Format.BaseAddress==0);
				m_MipAddrOffset = m_Texture->Format.MipAddress; // save this for later when the render target is allocated from the pool
			}

		}
	}
	else
	{
		grcWarningf("unsupported render target type %d", type);
	}
	
	if (m_PoolID==kRTPoolIDInvalid)
	{
		m_Allocated = true;						// if we are not from a pool, our memory was allocated during creation

		if (preallocated)
		{
			m_Texture->Format.BaseAddress = ((DWORD)params.basePtr) >> 12;
			m_Texture->Format.MipAddress = ((DWORD)params.mipPtr) >> 12;
		}
	}
	else if (allocFromPoolOnCreate)
	{
		Assert(!preallocated);
		AllocateMemoryFromPool(); 
	}

	m_CachedTexturePtr = m_Texture;

#if __ASSERT
	if ( sp && m_Surface)
	{
		const DWORD size = ((m_Surface->Size + GPU_EDRAM_TILE_SIZE-1) / GPU_EDRAM_TILE_SIZE) * GPU_EDRAM_TILE_SIZE; // Round to a multiple of the tile size
		const DWORD startAddr = sp->Base * GPU_EDRAM_TILE_SIZE;
		const DWORD endAddr = startAddr + size-1;
		Assertf( endAddr < GPU_EDRAM_SIZE, "This Render target has run off the end of EDRAM: name=\"%s\" start=0x%X end=0x%X (size=%d, overflow=%d)", m_Name, startAddr, endAddr, size, endAddr-GPU_EDRAM_SIZE+1 );
	}
#endif // __ASSERT
}

grcRenderTargetXenon::grcRenderTargetXenon( const char *pName, const grcTextureObject *pTexture)
{
	GPUTEXTURE_FETCH_CONSTANT constant = pTexture->Format;
	Assert(constant.Dimension == GPUDIMENSION_2D);

	// don't use dimensions from GPUTEXTURE_FETCH_CONSTANT or, if you do, mind that 
	// the Width and Height fields are actually width-1 and height-1, and we end up
	// with render targets one pixel smaller on each side (its resolvable texture
	// would still have the correct size)
	IDirect3DTexture9* pD3DTexture = const_cast<IDirect3DTexture9*>(static_cast<const IDirect3DTexture9*>(pTexture));
	XGTEXTURE_DESC surfaceDescription;
	XGGetTextureDesc(pD3DTexture, 0, &surfaceDescription);
	
	m_Name = StringDuplicate(pName);
	m_Width = (u16) surfaceDescription.Width;
	m_Height = (u16) surfaceDescription.Height;
	m_Type = grcrtPermanent;
	m_Texture = 0;
	m_Surface = 0;
	
	m_PoolID = kRTPoolIDInvalid;
	m_PoolHeap = 0;
	m_OffsetInPool = 0;
	m_PoolAllocSize = 0;
	m_MipAddrOffset = 0;
	m_CurrentPoolMemPtr = NULL;

	D3DMULTISAMPLE_TYPE multisampleType = D3DMULTISAMPLE_NONE;
	
	
	switch(constant.DataFormat)
	{
		case GPUTEXTUREFORMAT_5_6_5:
			m_Format = grctfR5G6B5;
			break;
		case GPUTEXTUREFORMAT_8_8_8_8:
			m_Format = grctfA8R8G8B8;
			break;
		case GPUTEXTUREFORMAT_16_FLOAT:
			m_Format = grctfR16F;
			break;
		case GPUTEXTUREFORMAT_32_FLOAT:
			m_Format = grctfR32F;
			break;
		case GPUTEXTUREFORMAT_2_10_10_10:
			m_Format = grctfA2B10G10R10;
			break;
		case GPUTEXTUREFORMAT_2_10_10_10_FLOAT_EDRAM:
			m_Format = grctfA2B10G10R10ATI;
			break;
		case GPUTEXTUREFORMAT_16_16_16_16_EXPAND:
			m_Format = grctfA16B16G16R16F;
			break;
		case GPUTEXTUREFORMAT_16_16:
			m_Format = grctfG16R16;
			break;
		case GPUTEXTUREFORMAT_16_16_FLOAT:
			m_Format = grctfG16R16F;
			break;
		case GPUTEXTUREFORMAT_32_32_32_32_FLOAT:
			m_Format = grctfA32B32G32R32F;
			break;
		case GPUTEXTUREFORMAT_16_16_16_16_FLOAT:
			m_Format = grctfA16B16G16R16F_NoExpand;
			break;
		case GPUTEXTUREFORMAT_16_16_16_16:
			m_Format = grctfA16B16G16R16;
			break;
		case GPUTEXTUREFORMAT_DXT1:
			m_Format = grctfA16B16G16R16;
			m_Width >>= 2;
			m_Height >>= 2;
			break;
		case GPUTEXTUREFORMAT_8:
			m_Format = grctfL8;
			break;
		case GPUTEXTUREFORMAT_1_5_5_5:
			m_Format = grctfA1R5G5B5;
			break;
		case GPUTEXTUREFORMAT_24_8:
			m_Format = grctfD24S8;
			break;
		case GPUTEXTUREFORMAT_4_4_4_4:
			m_Format = grctfA4R4G4B4;
			break;
		case GPUTEXTUREFORMAT_32_32_FLOAT:
			m_Format = grctfG32R32F;
			break;
		case GPUTEXTUREFORMAT_24_8_FLOAT:
			m_Format = grctfD24FS8_ReadStencil;
			break;
		//case GPUTEXTUREFORMAT_8_8_8_8:
		//	m_Format = grctfA8B8G8R8_SNORM;
		//	break;
		default:
			Quitf("type not supported");
			break;
	}
		
	// grcTextureFormat_REFERENCE
	static const D3DFORMAT edramFormatLU[] = {
		D3DFMT_UNKNOWN,
		D3DFMT_A8R8G8B8,   // use in edram for D3DFMT_R5G6B5 texture
		D3DFMT_A8R8G8B8,
		D3DFMT_R16F,
		D3DFMT_R32F,
		D3DFMT_A2B10G10R10F_EDRAM,
		D3DFMT_A2R10G10B10,			// A10B10G10R10 for ATI graphics cards on PC or a 10-bit integer format for 360
		D3DFMT_A16B16G16R16F,
		D3DFMT_G16R16_EDRAM,
		D3DFMT_G16R16F,
		D3DFMT_A32B32G32R32F,
		D3DFMT_A16B16G16R16F,
		D3DFMT_A16B16G16R16_EDRAM,
		D3DFMT_A8R8G8B8,
		D3DFMT_R32F,
		D3DFMT_A8R8G8B8,
		D3DFMT_A8R8G8B8,
		D3DFMT_A8R8G8B8,  // use in edram for D3DFMT_A1R5G5B5 texture
		D3DFMT_D24S8,
		D3DFMT_A8R8G8B8,  // use in edram for D3DFMT_A4R4G4B4 texture 
#if __XENON
		D3DFMT_G32R32F,
		D3DFMT_D24FS8,
#else
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
#endif
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_Q8W8V8U8,
		D3DFMT_UNKNOWN
	};
	CompileTimeAssert(NELEM(edramFormatLU) == grctfCount);

	// grcTextureFormat_REFERENCE
	static const D3DFORMAT texFormatLU[] = {
		D3DFMT_UNKNOWN,
		D3DFMT_R5G6B5,
		D3DFMT_A8R8G8B8,
		D3DFMT_R16F,
		D3DFMT_R32F,
		D3DFMT_A16B16G16R16F_EXPAND,
		D3DFMT_A2R10G10B10,		// A10B10G10R10 for ATI graphics cards on PC or a 10-bit integer format for 360
		D3DFMT_A16B16G16R16F_EXPAND,
		D3DFMT_G16R16,
		D3DFMT_G16R16F,
		D3DFMT_A32B32G32R32F,
		D3DFMT_A16B16G16R16F,
		D3DFMT_A16B16G16R16,
		D3DFMT_L8,
		D3DFMT_L16,
		D3DFMT_UNKNOWN,
		D3DFMT_G8R8,
		D3DFMT_A1R5G5B5,
		D3DFMT_D24S8,
		D3DFMT_A4R4G4B4,
#if __XENON
		D3DFMT_G32R32F,
		D3DFMT_A8R8G8B8,
#else
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
#endif
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_UNKNOWN,
		D3DFMT_Q8W8V8U8,
		D3DFMT_UNKNOWN
	};
	CompileTimeAssert(NELEM(texFormatLU) == grctfCount);


	D3DFORMAT edramFormat = edramFormatLU[m_Format];
	D3DFORMAT texFormat = texFormatLU[m_Format];

	if (texFormat == D3DFMT_A16B16G16R16)
		texFormat = (D3DFORMAT)MAKED3DFMT(GPUTEXTUREFORMAT_16_16_16_16, GPUENDIAN_8IN16, TRUE, GPUSIGN_ALL_SIGNED, GPUNUMFORMAT_INTEGER, GPUSWIZZLE_ABGR);

	if ( !constant.Tiled )
	{
		texFormat = (D3DFORMAT)MAKELINFMT(texFormat);
	}

	const DWORD edramGpuFormat = (edramFormat << D3DFORMAT_TEXTUREFORMAT_SHIFT) & D3DFORMAT_TEXTUREFORMAT_MASK;

	bool isSRGB =	constant.SignX == GPUSIGN_GAMMA && 
					constant.SignY == GPUSIGN_GAMMA && 
					constant.SignZ == GPUSIGN_GAMMA;


	if (isSRGB && edramGpuFormat == GPUTEXTUREFORMAT_8_8_8_8 ) 
	{
		edramFormat = static_cast<D3DFORMAT>(MAKESRGBFMT(edramFormat));
		texFormat = static_cast<D3DFORMAT>(MAKESRGBFMT(texFormat));
	}

	D3DSURFACE_PARAMETERS spBuf = { 0, 0, 0, D3DHIZFUNC_DEFAULT };
	
	const char* message = NULL;
	switch (IDirect3DDevice9_CreateRenderTarget(NULL, m_Width, m_Height, edramFormat, multisampleType, 0, false, &m_Surface, &spBuf))
	{
	case D3DERR_NOTAVAILABLE:
		message = "D3DERR_NOTAVAILABLE";
		break;
	case D3DERR_INVALIDCALL:
		message = "D3DERR_INVALIDCALL";
		break;
	case D3DERR_OUTOFVIDEOMEMORY:
		message = "D3DERR_OUTOFVIDEOMEMORY";
		break;
	case E_OUTOFMEMORY:
		message = "E_OUTOFMEMORY : This is out of Edram Memory , so modifying the creation params could solve this";
		break;
	default:
		break;
	}
		
	if (FAILED(GRCDEVICE.CreateTexture(m_Width, m_Height, 1, ((D3DBaseTexture*)pTexture)->GetLevelCount() , D3DUSAGE_RENDERTARGET, texFormat, D3DPOOL_DEFAULT, grcImage::STANDARD, &m_Texture, &m_PoolAllocSize)))
	{
		Quitf("Unable to create backup texture for rendertarget - try using @extramemory=NNN on command line (not response file) (this is usually caused by having optimizations locally disabled in some files)");
	}
			
	
	m_Allocated = true;

	m_Texture->Format.BaseAddress = pTexture->Format.BaseAddress;
	m_Texture->Format.MipAddress = pTexture->Format.MipAddress;

	m_CachedTexturePtr = m_Texture;
}

grcRenderTargetXenon::~grcRenderTargetXenon() 
{
	grcTextureFactory::UnregisterRenderTarget(this);

	if (m_PoolID!=kRTPoolIDInvalid)
	{
		if (IsAllocated())
			ReleaseMemoryToPool();
	}

	if ( m_Texture )
		GRCDEVICE.DeleteTexture(m_Texture);
	if ( m_Surface )
	{		
		GRCDEVICE.DeleteSurface(m_Surface);
	}
}

void grcRenderTargetXenon::Untile(int level)
{
	grcTextureLock lock;
	LockRect(0, level, lock);

	D3DFORMAT format = static_cast<D3DFORMAT>(lock.RawFormat);
	Assert(XGIsTiledFormat(format));

	DWORD flags = 0;
	if (!XGIsPackedTexture(m_Texture))
	{
		flags |= XGTILE_NONPACKED;
	}
	if (XGIsBorderTexture(m_Texture))
	{
		flags |= XGTILE_BORDER;
	}

	XGUntileTextureLevel(this->GetWidth(), this->GetHeight(), level, XGGetGpuFormat(format), flags, lock.Base, lock.Pitch, NULL, lock.Base, NULL);

	UnlockRect(lock);
}

void grcRenderTargetXenon::AllocateMemoryFromPool()
{
	Assertf(!m_Allocated,"Attempting to lock an already locked target \"%s\"",m_Name);

	if (m_PoolID!=kRTPoolIDInvalid)
	{
		grcRenderTarget::LogTexture("Lock",this);
		Assert(m_PoolAllocSize);
		m_CurrentPoolMemPtr = grcRenderTargetPoolMgr::AllocateTextureMem(m_PoolID, m_PoolHeap, m_PoolAllocSize, m_OffsetInPool);
		Assert(m_CurrentPoolMemPtr);
		m_Texture->Format.BaseAddress = ((DWORD)m_CurrentPoolMemPtr) >> 12;
		if( m_MipAddrOffset )
			m_Texture->Format.MipAddress = m_Texture->Format.BaseAddress + m_MipAddrOffset;
		else
			m_Texture->Format.MipAddress = 0;

		m_Allocated = true;
	}
}

void grcRenderTargetXenon::ReleaseMemoryToPool()
{
	Assertf(m_Allocated,"Attempting to unlock an already unlocked target \"%s\"",m_Name);

	if (m_PoolID!=kRTPoolIDInvalid)
	{
		grcRenderTarget::LogTexture("Unlock",this);
		Assert(m_CurrentPoolMemPtr);
		grcRenderTargetPoolMgr::FreeTextureMem(m_PoolID, m_PoolHeap, m_CurrentPoolMemPtr);
		m_CurrentPoolMemPtr = NULL;
		m_Allocated = false;
	}
}

void grcRenderTargetXenon::UpdateMemoryLocation(const grcTextureObject *pTexture)
{
	Assertf(m_Allocated,"Attempting to update a non allocated target \"%s\"",m_Name);
	// No equivalent 360 flag, so no assert
	//Assertf(m_BooleanTraits.IsSet(grcrttUseTextureMemory),"Attempting to update a texture based target \"%s\"",m_Name);
	
	m_Texture->Format.BaseAddress = pTexture->Format.BaseAddress;
	m_Texture->Format.MipAddress = pTexture->Format.MipAddress;
}

bool grcRenderTargetXenon::LockRect(int layer, int mipLevel,grcTextureLock &lock, u32 ASSERT_ONLY(uLockFlags)) const
{
	Assert(IsAllocated() || (uLockFlags & grcsAllowVRAMLock) != 0);
	IDirect3DTexture9* d3dTexture = static_cast<IDirect3DTexture9*>(m_Texture);

	XGTEXTURE_DESC surfaceDescription;
	XGGetTextureDesc(d3dTexture, mipLevel, &surfaceDescription);
	lock.BitsPerPixel = surfaceDescription.BitsPerPixel;
	lock.Width = surfaceDescription.Width;
	lock.Height = surfaceDescription.Height;
	lock.RawFormat = surfaceDescription.Format;
	lock.MipLevel = mipLevel;
	lock.Layer = layer;

	switch (d3dTexture->GetType())
	{
	case D3DRTYPE_VOLUMETEXTURE:
		{
			D3DLOCKED_BOX lockedBox;
			if (SUCCEEDED(static_cast<IDirect3DVolumeTexture9*>(m_Texture)->LockBox(layer, &lockedBox, NULL, 0)))
			{
				Assert(lockedBox.RowPitch * lock.Height == lockedBox.SlicePitch);
				lock.Base = reinterpret_cast<char*>(lockedBox.pBits) + lockedBox.SlicePitch * mipLevel;
				lock.Pitch = lockedBox.RowPitch;
				break;
			}
			else
			{
				return false;
			}
		}
	case D3DRTYPE_CUBETEXTURE:
		{
			D3DLOCKED_RECT lockedRect;
			if (SUCCEEDED(static_cast<IDirect3DCubeTexture9*>(m_Texture)->LockRect(static_cast<D3DCUBEMAP_FACES>(layer), mipLevel, &lockedRect, NULL, 0)))
			{
				lock.Base = lockedRect.pBits;
				lock.Pitch = lockedRect.Pitch;
				break;
			}
			else
			{
				return false;
			}
		}
	case D3DRTYPE_TEXTURE:
		{
			D3DLOCKED_RECT lockedRect;
			if (SUCCEEDED(d3dTexture->LockRect(mipLevel, &lockedRect, NULL, 0)))
			{
				lock.Base = lockedRect.pBits;
				lock.Pitch = lockedRect.Pitch;
				break;
			}
			else
			{
				return false;
			}
		}

	default:
		Quitf("Invalid D3D resource (texture) type");
		return false;
	}

	return true;
}

void grcRenderTargetXenon::UnlockRect(const grcTextureLock &lock) const
{
	IDirect3DTexture9* d3dTexture = static_cast<IDirect3DTexture9*>(m_Texture);

	switch (d3dTexture->GetType())
	{
	case D3DRTYPE_VOLUMETEXTURE:
		static_cast<IDirect3DVolumeTexture9*>(m_Texture)->UnlockBox(lock.Layer);
		break;
	case D3DRTYPE_CUBETEXTURE:
		static_cast<IDirect3DCubeTexture9*>(m_Texture)->UnlockRect(static_cast<D3DCUBEMAP_FACES>(lock.Layer), lock.MipLevel);
		break;
	case D3DRTYPE_TEXTURE:
		d3dTexture->UnlockRect(lock.MipLevel);
		break;
	default:
		Quitf("Invalid D3D resource (texture) type");
	}
}

void grcRenderTargetXenon::SetColorExpBias(int val) 
{ 
	GetSurface()->ColorInfo.ColorExpBias = val; 
}

int grcRenderTargetXenon::GetColorExpBias() const
{ 
	return (int)GetSurface()->ColorInfo.ColorExpBias;
}

const grcTextureObject* grcRenderTargetXenon::GetTexturePtr() const {
	return (m_Texture);
}

grcTextureObject* grcRenderTargetXenon::GetTexturePtr() {
	return (m_Texture);
}

grcDeviceSurface* grcRenderTargetXenon::GetSurface(bool /*bind*/ /*= false*/) const {
	return m_Surface;
}

void grcRenderTargetXenon::Realize(const grcResolveFlags * clearFlags, int index) 
{
	Assert(IsAllocated());
	Assert(!clearFlags || clearFlags->NeedResolve);

	static u32 resolveTarget[] = 
	{ 
		D3DRESOLVE_RENDERTARGET0,
		D3DRESOLVE_RENDERTARGET1,
		D3DRESOLVE_RENDERTARGET2,
		D3DRESOLVE_RENDERTARGET3
	};

	if (!m_Texture)
		return;

	D3DPOINT D3DOffset;
	D3DPOINT* pD3DOffset = &D3DOffset;

	// if the memory area where the render target needs to be stored is bigger than the EDRAM render target
	// Resolve might show an error message similar to "source is bigger than the 1600x1600 EDRAM render target"
	// specifying the source rectangle makes it go away
	D3DRECT rect;
	rect.x1 = 0;
	rect.y1 = 0;
	rect.x2 = Max(m_Width>>m_LockedMip,1);
	rect.y2 = Max(m_Height>>m_LockedMip,1);
	Assertf(rect.x2 == GRCDEVICE.GetWidth(), "Resolve size mismatch. rect.x2 %d, m_Width %u, m_LockedMip %u, GRCDEVICE.GetWidth %d", rect.x2, m_Width, m_LockedMip, GRCDEVICE.GetWidth());
	Assertf(rect.y2 == GRCDEVICE.GetHeight(), "Resolve size mismatch. rect.y2 %d, m_Height %u, m_LockedMip %u, GRCDEVICE.GetHeight %d", rect.y2, m_Height, m_LockedMip, GRCDEVICE.GetHeight());

	// a depth buffer can not use a texture atlas
	if (m_Type == grcrtDepthBuffer)
	{
		pD3DOffset = NULL;

		if (clearFlags)
		{	
			u32 flags = D3DRESOLVE_FRAGMENT0 | D3DRESOLVE_DEPTHSTENCIL;

			if (clearFlags->ClearDepthStencil)
				flags |= D3DRESOLVE_CLEARDEPTHSTENCIL; 

			GRCDEVICE.GetCurrent()->Resolve( flags, &rect, m_Texture, pD3DOffset, 
											 0, 0, NULL, grcDevice::FixDepth(clearFlags->Depth), clearFlags->Stencil, NULL);
		}
		else
		{
			GRCDEVICE.GetCurrent()->Resolve( D3DRESOLVE_FRAGMENT0 | D3DRESOLVE_DEPTHSTENCIL, &rect, m_Texture, pD3DOffset, 
											 0, 0, NULL, 0, 0, NULL);
		}
	}
	// a shadow map might use a texture atlas
	else if (m_Type == grcrtShadowMap) 
	{
		pD3DOffset = NULL;

		if (clearFlags)
		{	
			u32 flags = D3DRESOLVE_FRAGMENT0 | D3DRESOLVE_DEPTHSTENCIL;

			if (clearFlags->ClearDepthStencil)
				flags |= D3DRESOLVE_CLEARDEPTHSTENCIL; 

			GRCDEVICE.GetCurrent()->Resolve( flags, &rect, m_Texture, pD3DOffset, 
				0, 0, NULL, grcDevice::FixDepth(clearFlags->Depth), clearFlags->Stencil, NULL);
		}
		else
		{
			GRCDEVICE.GetCurrent()->Resolve( D3DRESOLVE_FRAGMENT0 | D3DRESOLVE_DEPTHSTENCIL, &rect, m_Texture, pD3DOffset, 
				0, 0, NULL, 0, 0, NULL);
		}
	}
	else 
	{
		pD3DOffset = NULL;

		if (clearFlags) 
		{
			u32 flags = resolveTarget[index] | D3DRESOLVE_ALLFRAGMENTS;
			
			if (clearFlags->ClearColor)
				flags |= D3DRESOLVE_CLEARRENDERTARGET;
			if (clearFlags->ClearDepthStencil)
				flags |= D3DRESOLVE_CLEARDEPTHSTENCIL;
			if(clearFlags->ColorExpBias)
			{
				Assert(clearFlags->ColorExpBias >= -32 && clearFlags->ColorExpBias < 32);
				flags |= D3DRESOLVE_EXPONENTBIAS(clearFlags->ColorExpBias);
			}
			D3DVECTOR4 clearColor = { clearFlags->Color.GetRedf(), clearFlags->Color.GetGreenf(), clearFlags->Color.GetBluef(), clearFlags->Color.GetAlphaf() };
			GRCDEVICE.GetCurrent()->Resolve(flags,  &rect, m_Texture, pD3DOffset, 
				 m_LockedMip, m_LockedLayer, &clearColor, grcDevice::FixDepth(clearFlags->Depth), clearFlags->Stencil, NULL);
		}
		else
		{
			GRCDEVICE.GetCurrent()->Resolve( resolveTarget[index] | D3DRESOLVE_ALLFRAGMENTS, &rect, m_Texture, pD3DOffset, 
				 m_LockedMip, m_LockedLayer, NULL, 0, 0, NULL);
		}
	}
}

void grcRenderTargetXenon::InitWithDepthSurface(grcDeviceSurface *surface, u32* poolAllocSize) 
{
	m_Name = StringDuplicate("Custom Depth Tgt");

	XGTEXTURE_DESC surfaceDescription;
	XGGetSurfaceDesc(surface, &surfaceDescription);

	SetPhysicalSize(surfaceDescription.SlicePitch * surfaceDescription.Depth);
	m_Width = (u16) surfaceDescription.Width;
	m_Height = (u16) surfaceDescription.Height;
	m_Texture = 0;
	m_Surface = 0;

	m_Format = grctfNone;

	m_Surface = surface;
	m_Type = grcrtDepthBuffer;
	// For depth-stencil resolve operations, the texture must have the same format as the depth-stencil buffer
	if (FAILED(GRCDEVICE.CreateTexture(surfaceDescription.Width, surfaceDescription.Height, 1, 1,D3DUSAGE_RENDERTARGET,g_grcDepthFormat,D3DPOOL_DEFAULT, grcImage::STANDARD, &m_Texture, poolAllocSize)))
		Quitf("Unable to create backup texture for depth render target - try using @extramemory=NNN on command line (not response file) (this is usually caused by having optimizations locally disabled in some files)");
		
	m_Allocated = true;
	m_CachedTexturePtr = m_Texture;
}

void grcRenderTargetXenon::InitWithSurface(grcDeviceSurface *surface) {
	m_Name = StringDuplicate("Custom Color Tgt");

	XGTEXTURE_DESC surfaceDescription;
	XGGetSurfaceDesc(surface, &surfaceDescription);

	SetPhysicalSize(surfaceDescription.SlicePitch * surfaceDescription.Depth);
	m_Width = (u16) surfaceDescription.Width;
	m_Height = (u16) surfaceDescription.Height;
	m_Texture = 0;
	m_Surface = 0;

	m_Format = (PARAM_hdr.Get()) ? grctfA2B10G10R10 : grctfA8R8G8B8;

	m_Surface = surface;
	m_Type = grcrtPermanent;
	if (FAILED(GRCDEVICE.CreateTexture(surfaceDescription.Width,surfaceDescription.Height,1,1,D3DUSAGE_RENDERTARGET,
		surfaceDescription.Format==D3DFMT_A2B10G10R10F_EDRAM?D3DFMT_A16B16G16R16F_EXPAND:surfaceDescription.Format,
		D3DPOOL_DEFAULT, grcImage::STANDARD, &m_Texture )))
		Quitf("Unable to create backup texture for rendertarget - try using @extramemory=NNN on command line (not response file) (this is usually caused by having optimizations locally disabled in some files)");
	
	m_Allocated = true;
	m_CachedTexturePtr = m_Texture;
}

//
//
// creates fake A8R8G8B8 texture header and sets up its base address to given depth rendertarget's base surface address;
// this way this depth rendertarget can be sampled in pixelshader as A8R8G8B8 texture
// and makes it is possible to read stencil buffer values, etc.
//
// not solved yet:
// - elegant destruction of this (but shouldn't it be alive during whole life of the runtime app?)
// - use only when you know what you're doing with this texture and rendertarget it's patched to
//
bool grcRenderTargetXenon::SetupAsFakeStencilTexture(grcRenderTarget *pDepthRT)
{
	Assert(this->m_Texture==NULL);	// assume empty texture
	Assert(pDepthRT);
	Assert(pDepthRT->GetType()==grcrtDepthBuffer);

	m_Width = (u16)pDepthRT->GetWidth();
	m_Height = (u16)pDepthRT->GetHeight();

	// creates an A8R8G8B8 texture header and initializes it by calling XGSetTextureHeader:
	D3DTexture *pStencilTexture = rage_new D3DTexture;
	Assert(pStencilTexture);
	XGSetTextureHeader(m_Width, m_Height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
		0, 0, m_Width*sizeof(DWORD), pStencilTexture, NULL, NULL );

	// decode the pointer to the D24S8 texture data and copy it to the A8R8G8B8 texture header by calling XGOffsetBaseTextureAddress:
	IDirect3DTexture9 *pDepthTexture = static_cast<IDirect3DTexture9*>(pDepthRT->GetTexturePtr());
	Assert(pDepthTexture);
	DWORD dwBaseAddress = pDepthTexture->Format.BaseAddress << GPU_TEXTURE_ADDRESS_SHIFT;
	Assert(dwBaseAddress);
	XGOffsetBaseTextureAddress(pStencilTexture, (VOID*)dwBaseAddress, NULL);

	m_Texture = (D3DBaseTexture*)pStencilTexture;
	m_CachedTexturePtr = m_Texture;

	return(true);
}


grcTextureXenon::grcTextureXenon()
{
	m_Width = (u16) 0;
	m_Height = (u16) 0;
	m_MipCount = 0;

	m_Texture = NULL;
	SetPhysicalSize(0);
}

grcTextureXenon::grcTextureXenon(u32 width, u32 height, u32 mipLevels, grcDeviceTexture* baseTex) {

	m_Width = (u16) width;
	m_Height = (u16) height;
	m_MipCount = mipLevels;

	m_Texture = baseTex;
	m_CachedTexturePtr = m_Texture;

	SetPhysicalSize(0);
	
	XGTEXTURE_DESC surfaceDescription;
	//memset(&surfaceDescription, 0, sizeof(surfaceDescription));
	for (u32 i = 0; i < m_Texture->GetLevelCount(); ++i)
	{
		XGGetTextureDesc(m_Texture, i, &surfaceDescription);
		SetPhysicalSize(GetPhysicalSize() + XGTEXTURE_DESC_GetSize(surfaceDescription));
	}

// 	char buffer[128];
// 	prettyprinter(buffer, 128, GetPhysicalSize());
// 	Printf("\"%s\" %dx%dx%d %d %11s\n", GetName(), GetWidth(), GetHeight(), GetDepth(), surfaceDescription.Format, buffer);
}

grcTextureXenon::grcTextureXenon(const char *pFilename,grcTextureFactory::TextureCreateParams *params)
{
	grcImage	*pImage = NULL;

	// grcWarningf("Texture : %s \n", pFilename );

	sysMemStartTemp();
	if	(strcmp(pFilename, "none") && strcmp(pFilename, "nonresident"))
		pImage = grcImage::Load(pFilename);
	sysMemEndTemp();

	if	(pImage)
	{
		int	w = pImage->GetWidth();
		int	h = pImage->GetHeight();

		if	((w & 3) || (h & 3))
		{
			grcErrorf("grcTextureXenon - Texture '%s' - invalid resolution %d by %d", pFilename, w, h);
			sysMemStartTemp();
			pImage->Release();
			sysMemEndTemp();
			pImage = NULL;
		}
	}

	if	(!pImage)
	{
		u32 Texel = strcmp(pFilename, "nonresident") ? 0xFFFFFFFF : 0x40FFFFFF;

		sysMemStartTemp();
		const int dummySize = 32;
		pImage = grcImage::Create(dummySize, dummySize, 1, grcImage::A8R8G8B8, grcImage::STANDARD, false, 0);
		sysMemEndTemp();
		u32 *texels = (u32*) pImage->GetBits();
		for (u32 i=0; i<dummySize*dummySize; i++)
			texels[i] = Texel;
	}

	Init(pFilename, pImage, params);

	sysMemStartTemp();
	pImage->Release();
	sysMemEndTemp();
}

grcTextureXenon::grcTextureXenon(grcImage *pImage,grcTextureFactory::TextureCreateParams *params) 
{
	const char* name = "image";
#if __BANK || __RESOURCECOMPILER
	name = grcTexture::GetCustomLoadName(name);
#endif // __BANK || __RESOURCECOMPILER
	Init(name,pImage,params);
}

static inline D3DFORMAT ExpandGPUFormat(D3DFORMAT format)
{
	u32 GPU_format = ((u32)format & D3DFORMAT_TEXTUREFORMAT_MASK) >> D3DFORMAT_TEXTUREFORMAT_SHIFT;

	switch (GPU_format)
	{
	case GPUTEXTUREFORMAT_16_FLOAT          : GPU_format = GPUTEXTUREFORMAT_16_EXPAND         ; break;
	case GPUTEXTUREFORMAT_16_16_FLOAT       : GPU_format = GPUTEXTUREFORMAT_16_16_EXPAND      ; break;
	case GPUTEXTUREFORMAT_16_16_16_16_FLOAT : GPU_format = GPUTEXTUREFORMAT_16_16_16_16_EXPAND; break;
	}

	return (D3DFORMAT)(((u32)format & ~D3DFORMAT_TEXTUREFORMAT_MASK) | (GPU_format << D3DFORMAT_TEXTUREFORMAT_SHIFT));
}

static inline D3DFORMAT UnExpandGPUFormat(D3DFORMAT format)
{
	u32 GPU_format = ((u32)format & D3DFORMAT_TEXTUREFORMAT_MASK) >> D3DFORMAT_TEXTUREFORMAT_SHIFT;

	switch (GPU_format)
	{
	case GPUTEXTUREFORMAT_16_EXPAND          : GPU_format = GPUTEXTUREFORMAT_16_FLOAT         ; break;
	case GPUTEXTUREFORMAT_16_16_EXPAND       : GPU_format = GPUTEXTUREFORMAT_16_16_FLOAT      ; break;
	case GPUTEXTUREFORMAT_16_16_16_16_EXPAND : GPU_format = GPUTEXTUREFORMAT_16_16_16_16_FLOAT; break;
	}

	return (D3DFORMAT)(((u32)format & ~D3DFORMAT_TEXTUREFORMAT_MASK) | (GPU_format << D3DFORMAT_TEXTUREFORMAT_SHIFT));
}

static inline D3DFORMAT ExpandGPUFormat_AS_16_16_16_16(D3DFORMAT format)
{
	u32 GPU_format = ((u32)format & D3DFORMAT_TEXTUREFORMAT_MASK) >> D3DFORMAT_TEXTUREFORMAT_SHIFT;

	switch (GPU_format)
	{
	case GPUTEXTUREFORMAT_8_8_8_8    : GPU_format = GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16   ; break;
	case GPUTEXTUREFORMAT_DXT1       : GPU_format = GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16      ; break;
	case GPUTEXTUREFORMAT_DXT2_3     : GPU_format = GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16    ; break;
	case GPUTEXTUREFORMAT_DXT4_5     : GPU_format = GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16    ; break;
	case GPUTEXTUREFORMAT_2_10_10_10 : GPU_format = GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16; break;
	case GPUTEXTUREFORMAT_10_11_11   : GPU_format = GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16  ; break;
	case GPUTEXTUREFORMAT_11_11_10   : GPU_format = GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16  ; break;
	}

	return (D3DFORMAT)(((u32)format & ~D3DFORMAT_TEXTUREFORMAT_MASK) | (GPU_format << D3DFORMAT_TEXTUREFORMAT_SHIFT));
}

/*static*/ inline D3DFORMAT UnExpandGPUFormat_AS_16_16_16_16(D3DFORMAT format)
{
	u32 GPU_format = ((u32)format & D3DFORMAT_TEXTUREFORMAT_MASK) >> D3DFORMAT_TEXTUREFORMAT_SHIFT;

	switch (GPU_format)
	{
	case GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16    : GPU_format = GPUTEXTUREFORMAT_8_8_8_8   ; break;
	case GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16       : GPU_format = GPUTEXTUREFORMAT_DXT1      ; break;
	case GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16     : GPU_format = GPUTEXTUREFORMAT_DXT2_3    ; break;
	case GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16     : GPU_format = GPUTEXTUREFORMAT_DXT4_5    ; break;
	case GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16 : GPU_format = GPUTEXTUREFORMAT_2_10_10_10; break;
	case GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16   : GPU_format = GPUTEXTUREFORMAT_10_11_11  ; break;
	case GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16   : GPU_format = GPUTEXTUREFORMAT_11_11_10  ; break;
	}

	return (D3DFORMAT)(((u32)format & ~D3DFORMAT_TEXTUREFORMAT_MASK) | (GPU_format << D3DFORMAT_TEXTUREFORMAT_SHIFT));
}

#define GPUSWIZZLE_RED      (GPUSWIZZLE_X<<D3DFORMAT_SWIZZLEX_SHIFT | GPUSWIZZLE_0<<D3DFORMAT_SWIZZLEY_SHIFT | GPUSWIZZLE_0<<D3DFORMAT_SWIZZLEZ_SHIFT | GPUSWIZZLE_1<<D3DFORMAT_SWIZZLEW_SHIFT) // R001 (matches DXGI red formats)
#define GPUSWIZZLE_REDGREEN (GPUSWIZZLE_X<<D3DFORMAT_SWIZZLEX_SHIFT | GPUSWIZZLE_Y<<D3DFORMAT_SWIZZLEY_SHIFT | GPUSWIZZLE_0<<D3DFORMAT_SWIZZLEZ_SHIFT | GPUSWIZZLE_1<<D3DFORMAT_SWIZZLEW_SHIFT) // RG01 (matches DXGI red/green formats)

// these two formats don't exist in d3d9types.h for some reason ..
#define D3DFMT_R8  (_D3DFORMAT)MAKED3DFMT(GPUTEXTUREFORMAT_8, GPUENDIAN_NONE, TRUE, GPUSIGN_ALL_UNSIGNED, GPUNUMFORMAT_FRACTION, GPUSWIZZLE_OOOR)
#define D3DFMT_R16 (_D3DFORMAT)MAKED3DFMT(GPUTEXTUREFORMAT_16, GPUENDIAN_8IN16, TRUE, GPUSIGN_ALL_UNSIGNED, GPUNUMFORMAT_FRACTION, GPUSWIZZLE_OOOR)

// these formats are fixed so that they match DXGI
#define D3DFMT_R8_fixed      (_D3DFORMAT)((D3DFMT_R8      & ~D3DFORMAT_SWIZZLE_MASK) | GPUSWIZZLE_RED)
#define D3DFMT_R16_fixed     (_D3DFORMAT)((D3DFMT_R16     & ~D3DFORMAT_SWIZZLE_MASK) | GPUSWIZZLE_RED)
#define D3DFMT_R16F_fixed    (_D3DFORMAT)((D3DFMT_R16F    & ~D3DFORMAT_SWIZZLE_MASK) | GPUSWIZZLE_RED)
#define D3DFMT_R32F_fixed    (_D3DFORMAT)((D3DFMT_R32F    & ~D3DFORMAT_SWIZZLE_MASK) | GPUSWIZZLE_RED)
#define D3DFMT_CTX1_fixed    (_D3DFORMAT)((D3DFMT_CTX1    & ~D3DFORMAT_SWIZZLE_MASK) | GPUSWIZZLE_REDGREEN)
#define D3DFMT_DXN_fixed     (_D3DFORMAT)((D3DFMT_DXN     & ~D3DFORMAT_SWIZZLE_MASK) | GPUSWIZZLE_REDGREEN)
#define D3DFMT_G8R8_fixed    (_D3DFORMAT)((D3DFMT_G8R8    & ~D3DFORMAT_SWIZZLE_MASK) | GPUSWIZZLE_REDGREEN)
#define D3DFMT_G16R16_fixed  (_D3DFORMAT)((D3DFMT_G16R16  & ~D3DFORMAT_SWIZZLE_MASK) | GPUSWIZZLE_REDGREEN)
#define D3DFMT_G16R16F_fixed (_D3DFORMAT)((D3DFMT_G16R16F & ~D3DFORMAT_SWIZZLE_MASK) | GPUSWIZZLE_REDGREEN)
#define D3DFMT_G32R32F_fixed (_D3DFORMAT)((D3DFMT_G32R32F & ~D3DFORMAT_SWIZZLE_MASK) | GPUSWIZZLE_REDGREEN)

static D3DFORMAT GetD3DFormat(grcImage::Format imgFormat, bool bIsLinear) {
	// NOTE!  There is a copy of this code on the tool side and the game side
	// [tool] -> rage/base/tools/libs/grconvert/texturexenonproxy.cpp
	// [game] -> rage/base/src/grcore/texturexenon.cpp
	// These must be kept in sync.

	D3DFORMAT format = D3DFMT_UNKNOWN;

	switch (imgFormat)
	{
	case grcImage::UNKNOWN                     : format = D3DFMT_UNKNOWN          ; break;
	case grcImage::DXT1                        : format = D3DFMT_DXT1             ; break;
	case grcImage::DXT3                        : format = D3DFMT_DXT3             ; break;
	case grcImage::DXT5                        : format = D3DFMT_DXT5             ; break;
	case grcImage::CTX1                        : format = D3DFMT_CTX1_fixed       ; break;
	case grcImage::DXT3A                       : format = D3DFMT_DXT3A            ; break;
	case grcImage::DXT3A_1111                  : format = D3DFMT_DXT3A_1111       ; break;
	case grcImage::DXT5A                       : format = D3DFMT_DXT5A            ; break;
	case grcImage::DXN                         : format = D3DFMT_DXN_fixed        ; break;
	case grcImage::A8R8G8B8                    : format = D3DFMT_A8R8G8B8         ; break;
	case grcImage::A8B8G8R8                    : format = D3DFMT_A8B8G8R8         ; break;
	case grcImage::A8                          : format = D3DFMT_A8               ; break;
	case grcImage::L8                          : format = D3DFMT_L8               ; break;
	case grcImage::A8L8                        : format = D3DFMT_A8L8             ; break;
	case grcImage::A4R4G4B4                    : format = D3DFMT_A4R4G4B4         ; break;
	case grcImage::A1R5G5B5                    : format = D3DFMT_A1R5G5B5         ; break;
	case grcImage::R5G6B5                      : format = D3DFMT_R5G6B5           ; break;
	case grcImage::R3G3B2                      : format = D3DFMT_UNKNOWN          ; break; // not supported
	case grcImage::A8R3G3B2                    : format = D3DFMT_UNKNOWN          ; break; // not supported
	case grcImage::A4L4                        : format = D3DFMT_UNKNOWN          ; break; // not supported
	case grcImage::A2R10G10B10                 : format = D3DFMT_A2R10G10B10      ; break;
	case grcImage::A2B10G10R10                 : format = D3DFMT_A2B10G10R10      ; break;
	case grcImage::A16B16G16R16                : format = D3DFMT_A16B16G16R16     ; break;
	case grcImage::G16R16                      : format = D3DFMT_G16R16_fixed     ; break;
	case grcImage::L16                         : format = D3DFMT_L16              ; break;
	case grcImage::A16B16G16R16F               : format = D3DFMT_A16B16G16R16F    ; break;
	case grcImage::G16R16F                     : format = D3DFMT_G16R16F_fixed    ; break;
	case grcImage::R16F                        : format = D3DFMT_R16F_fixed       ; break;
	case grcImage::A32B32G32R32F               : format = D3DFMT_A32B32G32R32F    ; break;
	case grcImage::G32R32F                     : format = D3DFMT_G32R32F_fixed    ; break;
	case grcImage::R32F                        : format = D3DFMT_R32F_fixed       ; break;
	case grcImage::D15S1                       : format = D3DFMT_UNKNOWN          ; break; // not supported
	case grcImage::D24S8                       : format = D3DFMT_D24S8            ; break;
	case grcImage::D24FS8                      : format = D3DFMT_D24FS8           ; break;
	case grcImage::P4                          : format = D3DFMT_UNKNOWN          ; break; // not supported
	case grcImage::P8                          : format = D3DFMT_UNKNOWN          ; break; // not supported
	case grcImage::A8P8                        : format = D3DFMT_UNKNOWN          ; break; // not supported
	case grcImage::R8                          : format = D3DFMT_R8_fixed         ; break;
	case grcImage::R16                         : format = D3DFMT_R16_fixed        ; break;
	case grcImage::G8R8                        : format = D3DFMT_G8R8_fixed       ; break;
	case grcImage::LINA32B32G32R32F_DEPRECATED : format = D3DFMT_LIN_A32B32G32R32F; break;
	case grcImage::LINA8R8G8B8_DEPRECATED      : format = D3DFMT_LIN_A8R8G8B8     ; break;
	case grcImage::LIN8_DEPRECATED             : format = D3DFMT_LIN_L8           ; break;
	case grcImage::RGBE                        : format = D3DFMT_A8R8G8B8         ; break;
	}

	if (format == D3DFMT_UNKNOWN && 0) // cast unsupported formats?
	{
		switch (grcImage::GetFormatBitsPerPixel(imgFormat))
		{
		case   4 : format = D3DFMT_DXT1         ; break;
		case   8 : format = D3DFMT_L8           ; break;
		case  16 : format = D3DFMT_A1R5G5B5     ; break;
		case  32 : format = D3DFMT_A8R8G8B8     ; break;
		case  64 : format = D3DFMT_A16B16G16R16 ; break;
		case 128 : format = D3DFMT_A32B32G32R32F; break;
		}
	}

	if (format != D3DFMT_UNKNOWN)
	{
		if (bIsLinear)
		{
			format = (D3DFORMAT)MAKELINFMT(format);
		}

		if (0) // don't do this here .. don't use float textures if you need them to be filterable (but we still want to have the option to use float textures)
		{
			format = ExpandGPUFormat(format); // expand x16_FLOAT formats to x16_EXPAND, so they will be filterable
		}
	}

	return format;
}

static D3DFORMAT GetD3DFormat(grcImage *image,grcTextureFactory::TextureCreateParams *params) {
	// NOTE!  There is a copy of this code on the tool side and the game side
	// [tool] -> rage/base/tools/libs/grconvert/texturexenonproxy.cpp
	// [game] -> rage/base/src/grcore/texturexenon.cpp
	// These must be kept in sync.

	return GetD3DFormat(image->GetFormat(), (params && params->Format == grcTextureFactory::TextureCreateParams::LINEAR) || image->IsLinear());

}


u32 grcTextureXenon::GetImageFormat() const
{
	XGTEXTURE_DESC desc;
	XGGetTextureDesc(m_Texture, 0, &desc);

	switch ((D3DFORMAT_TEXTUREFORMAT_MASK|D3DFORMAT_SWIZZLE_MASK) & desc.Format) // handle ambiguous cases which can only be resolved by swizzle
	{
	case (D3DFORMAT_TEXTUREFORMAT_MASK|D3DFORMAT_SWIZZLE_MASK) & D3DFMT_L8        : return grcImage::L8;
	case (D3DFORMAT_TEXTUREFORMAT_MASK|D3DFORMAT_SWIZZLE_MASK) & D3DFMT_A8        : return grcImage::A8;
	case (D3DFORMAT_TEXTUREFORMAT_MASK|D3DFORMAT_SWIZZLE_MASK) & D3DFMT_R8_fixed  : return grcImage::R8;
	case (D3DFORMAT_TEXTUREFORMAT_MASK|D3DFORMAT_SWIZZLE_MASK) & D3DFMT_L16       : return grcImage::L16;
	case (D3DFORMAT_TEXTUREFORMAT_MASK|D3DFORMAT_SWIZZLE_MASK) & D3DFMT_R16_fixed : return grcImage::R16;
	case (D3DFORMAT_TEXTUREFORMAT_MASK|D3DFORMAT_SWIZZLE_MASK) & D3DFMT_A8L8      : return grcImage::A8L8;
	case (D3DFORMAT_TEXTUREFORMAT_MASK|D3DFORMAT_SWIZZLE_MASK) & D3DFMT_G8R8_fixed: return grcImage::G8R8;
	}

	const u32 GPU_format = ((u32)desc.Format & D3DFORMAT_TEXTUREFORMAT_MASK) >> D3DFORMAT_TEXTUREFORMAT_SHIFT;

	switch (GPU_format)
	{
	case GPUTEXTUREFORMAT_1_REVERSE                 : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_1                         : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_8                         : return grcImage::L8           ; // note that this could be A8 or L8 (or anything else with a single 8-bit channel)
	case GPUTEXTUREFORMAT_1_5_5_5                   : return grcImage::A1R5G5B5     ;
	case GPUTEXTUREFORMAT_5_6_5                     : return grcImage::R5G6B5       ;
	case GPUTEXTUREFORMAT_6_5_5                     : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_8_8_8_8                   : return grcImage::A8R8G8B8     ;
	case GPUTEXTUREFORMAT_2_10_10_10                : return grcImage::A2R10G10B10  ;
	case GPUTEXTUREFORMAT_8_A                       : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_8_B                       : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_8_8                       : return grcImage::A8L8         ; // could be A8L8 or G8R8 - but we should have handled this earlier
	case GPUTEXTUREFORMAT_Cr_Y1_Cb_Y0_REP           : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_Y1_Cr_Y0_Cb_REP           : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_16_16_EDRAM               : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_8_8_8_8_A                 : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_4_4_4_4                   : return grcImage::A4R4G4B4     ;
	case GPUTEXTUREFORMAT_10_11_11                  : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_11_11_10                  : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_DXT1                      : return grcImage::DXT1         ;
	case GPUTEXTUREFORMAT_DXT2_3                    : return grcImage::DXT3         ;
	case GPUTEXTUREFORMAT_DXT4_5                    : return grcImage::DXT5         ;
	case GPUTEXTUREFORMAT_16_16_16_16_EDRAM         : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_24_8                      : return grcImage::D24S8        ;
	case GPUTEXTUREFORMAT_24_8_FLOAT                : return grcImage::D24FS8       ;
	case GPUTEXTUREFORMAT_16                        : return grcImage::L16          ; // could be L16 or R16 - but we should have handled this earlier
	case GPUTEXTUREFORMAT_16_16                     : return grcImage::G16R16       ;
	case GPUTEXTUREFORMAT_16_16_16_16               : return grcImage::A16B16G16R16 ;
	case GPUTEXTUREFORMAT_16_EXPAND                 : return grcImage::L16          ; // could be L16 or R16 - but we should have handled this earlier
	case GPUTEXTUREFORMAT_16_16_EXPAND              : return grcImage::G16R16       ;
	case GPUTEXTUREFORMAT_16_16_16_16_EXPAND        : return grcImage::A16B16G16R16 ;
	case GPUTEXTUREFORMAT_16_FLOAT                  : return grcImage::R16F         ;
	case GPUTEXTUREFORMAT_16_16_FLOAT               : return grcImage::G16R16F      ;
	case GPUTEXTUREFORMAT_16_16_16_16_FLOAT         : return grcImage::A16B16G16R16F;
	case GPUTEXTUREFORMAT_32                        : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_32_32                     : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_32_32_32_32               : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_32_FLOAT                  : return grcImage::R32F         ;
	case GPUTEXTUREFORMAT_32_32_FLOAT               : return grcImage::G32R32F      ;
	case GPUTEXTUREFORMAT_32_32_32_32_FLOAT         : return grcImage::A32B32G32R32F;
	case GPUTEXTUREFORMAT_32_AS_8                   : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_32_AS_8_8                 : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_16_MPEG                   : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_16_16_MPEG                : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_8_INTERLACED              : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_32_AS_8_INTERLACED        : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_32_AS_8_8_INTERLACED      : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_16_INTERLACED             : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_16_MPEG_INTERLACED        : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_16_16_MPEG_INTERLACED     : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_DXN                       : return grcImage::DXN          ;
	case GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16    : return grcImage::A8R8G8B8     ;
	case GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16       : return grcImage::DXT1         ;
	case GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16     : return grcImage::DXT3         ;
	case GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16     : return grcImage::DXT5         ;
	case GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16 : return grcImage::A2R10G10B10  ;
	case GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16   : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16   : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_32_32_32_FLOAT            : return grcImage::UNKNOWN      ; // not supported
	case GPUTEXTUREFORMAT_DXT3A                     : return grcImage::DXT3A        ;
	case GPUTEXTUREFORMAT_DXT5A                     : return grcImage::DXT5A        ;
	case GPUTEXTUREFORMAT_CTX1                      : return grcImage::CTX1         ;
	case GPUTEXTUREFORMAT_DXT3A_AS_1_1_1_1          : return grcImage::DXT3A_1111   ;
	case GPUTEXTUREFORMAT_8_8_8_8_GAMMA_EDRAM       : return grcImage::A8R8G8B8     ;
	case GPUTEXTUREFORMAT_2_10_10_10_FLOAT_EDRAM    : return grcImage::A2R10G10B10  ;
	}

	return grcImage::UNKNOWN;
}

static u32 _SetTextureSwizzle(grcTexture::eTextureSwizzle swizzle)
{
	switch (swizzle)
	{
	case grcTexture::TEXTURE_SWIZZLE_R : return GPUSWIZZLE_X;
	case grcTexture::TEXTURE_SWIZZLE_G : return GPUSWIZZLE_Y;
	case grcTexture::TEXTURE_SWIZZLE_B : return GPUSWIZZLE_Z;
	case grcTexture::TEXTURE_SWIZZLE_A : return GPUSWIZZLE_W;
	case grcTexture::TEXTURE_SWIZZLE_0 : return GPUSWIZZLE_0;
	case grcTexture::TEXTURE_SWIZZLE_1 : return GPUSWIZZLE_1;
	}

	return 0;
}

void grcTextureXenon::SetTextureSwizzle(eTextureSwizzle r, eTextureSwizzle g, eTextureSwizzle b, eTextureSwizzle a, bool bApplyToExistingSwizzle)
{
	if (bApplyToExistingSwizzle)
	{
		eTextureSwizzle existing[4];

		GetTextureSwizzle(existing[0], existing[1], existing[2], existing[3]);

		r = ApplyToExistingSwizzle(r, existing);
		g = ApplyToExistingSwizzle(g, existing);
		b = ApplyToExistingSwizzle(b, existing);
		a = ApplyToExistingSwizzle(a, existing);
	}

	m_Texture->Format.SwizzleX = _SetTextureSwizzle(r);
	m_Texture->Format.SwizzleY = _SetTextureSwizzle(g);
	m_Texture->Format.SwizzleZ = _SetTextureSwizzle(b);
	m_Texture->Format.SwizzleW = _SetTextureSwizzle(a);
}

static grcTexture::eTextureSwizzle _GetTextureSwizzle(u32 swizzle)
{
	switch (swizzle)
	{
	case GPUSWIZZLE_X : return grcTexture::TEXTURE_SWIZZLE_R;
	case GPUSWIZZLE_Y : return grcTexture::TEXTURE_SWIZZLE_G;
	case GPUSWIZZLE_Z : return grcTexture::TEXTURE_SWIZZLE_B;
	case GPUSWIZZLE_W : return grcTexture::TEXTURE_SWIZZLE_A;
	case GPUSWIZZLE_0 : return grcTexture::TEXTURE_SWIZZLE_0;
	case GPUSWIZZLE_1 : return grcTexture::TEXTURE_SWIZZLE_1;
	}

	return grcTexture::TEXTURE_SWIZZLE_0;
}

void grcTextureXenon::GetTextureSwizzle(eTextureSwizzle& r, eTextureSwizzle& g, eTextureSwizzle& b, eTextureSwizzle& a) const
{
	r = _GetTextureSwizzle(m_Texture->Format.SwizzleX);
	g = _GetTextureSwizzle(m_Texture->Format.SwizzleY);
	b = _GetTextureSwizzle(m_Texture->Format.SwizzleZ);
	a = _GetTextureSwizzle(m_Texture->Format.SwizzleW);
}

void grcTextureXenon::Init(const char *pFilename,grcImage *pImage,grcTextureFactory::TextureCreateParams *params)
{
	// bool	bHasAlpha = pImage->GetFormat() == grcImage::DXT5;
	D3DFORMAT nFormat = GetD3DFormat(pImage,params);

	if(pImage->IsSRGB())		// was texture saved in gamma 2.2 space? If so, fix up the texture header to bring it back down to 1.0
	{
		nFormat = (D3DFORMAT)MAKESRGBFMT(nFormat);
		nFormat = ExpandGPUFormat_AS_16_16_16_16(nFormat);
	}
#if __BANK_TEXTURE_CONTROL
	if (gTextureControl.m_enabled)
	{
		if (1)
		{
			gTextureControl.DisplayFormat(pImage, (u32)nFormat, 0);
		}

		u32 tempFormat = (u32)nFormat;
		u32 tempRemap  = 0; // dummy

		if (gTextureControl.UpdateFormat(pImage, tempFormat, tempRemap))
		{
			nFormat = (D3DFORMAT)tempFormat;
		}
	}
#endif // __BANK_TEXTURE_CONTROL

	m_Name = grcSaveTextureNames ? StringDuplicate(pFilename) : 0;

	u8 imageType = (u8) pImage->GetType();

	if (params && params->Buffer)
	{
		D3DTexture* pTexture = rage_new D3DTexture;

		XGSetTextureHeader(
			pImage->GetWidth(),
			pImage->GetHeight(),
			pImage->GetExtraMipCount() + 1,
			D3DUSAGE_CPU_CACHED_MEMORY,
			nFormat,
			0,
			0,
			XGHEADER_CONTIGUOUS_MIP_OFFSET,
			0,
			pTexture,
			NULL,
			NULL ); 

		m_Texture = pTexture;
		m_Texture->Common |= D3DCOMMON_CPU_CACHED_MEMORY;
		m_Texture->Format.BaseAddress = ((DWORD)params->Buffer) >> 12;
	}
	else if (FAILED(GRCDEVICE.CreateTexture(pImage->GetWidth(),pImage->GetHeight(),pImage->GetDepth(),pImage->GetExtraMipCount()+1,D3DUSAGE_CPU_CACHED_MEMORY,nFormat,D3DPOOL_MANAGED,imageType,&m_Texture)))
	{
		grcErrorf("Error creating the %dx%dx%d texture '%s' - probably out of memory", pImage->GetWidth(),pImage->GetHeight(),pImage->GetDepth(), pFilename);
		return;
	}

	Assign(m_LayerCount,pImage->GetLayerCount()-1);
	m_Width = pImage->GetWidth();
	m_Height = pImage->GetHeight();
	m_MipCount = (u8)(pImage->GetExtraMipCount() + 1);

	Copy(pImage);

	SetPhysicalSize(0);
	XGTEXTURE_DESC surfaceDescription;
	//memset(&surfaceDescription, 0, sizeof(surfaceDescription));
	for (u32 i = 0; i < m_Texture->GetLevelCount(); ++i)
	{
		XGGetTextureDesc(m_Texture, i, &surfaceDescription);
		SetPhysicalSize(GetPhysicalSize() + XGTEXTURE_DESC_GetSize(surfaceDescription));
	}

	m_CachedTexturePtr = m_Texture;
// 	char buffer[128];
// 	prettyprinter(buffer, 128, GetPhysicalSize());
// 	Printf("\"%s\" %dx%dx%d %d %11s\n", GetName(), GetWidth(), GetHeight(), GetDepth(), surfaceDescription.Format, buffer);
}


void CopyVolumeMip( u8* dstBase, int rowPitch, int slicePitch,
					const grcImage	*pMip, u32 w, u32 h, int bytesPerBlock )
{
	u32 s = 0;
	u32 y=0;
	u32 sl=0;
	while (s < pMip->GetSize()){
		u8* dstMem = (u8*) dstBase + rowPitch*y + slicePitch*sl;
		u8* srcMem = (u8*) pMip->GetBits() + pMip->GetStride()*y + pMip->GetHeight()*pMip->GetStride()*sl;

		sysMemCpy(  dstMem,srcMem,  w*bytesPerBlock ); 
		y++;
		if ( y==h)
		{
			sl++;
			y=0;
		}
		s =  pMip->GetHeight()*pMip->GetStride()*sl;
	}
	Assert( y== 0 );
	//Assert( sl==(desc.Depth>>i));
}
bool grcTextureXenon::Copy(const grcImage *pImage)
{
	if (GetLayerCount() != pImage->GetLayerCount() || m_Width != pImage->GetWidth() || m_Height != pImage->GetHeight() || 
			m_MipCount != pImage->GetExtraMipCount()+1)
		return false;

	u8 imageType = (u8) pImage->GetType();
	D3DLOCKED_RECT	rect = { 0, 0 };
	D3DLOCKED_BOX	box = { 0, 0, 0 };

	sysMemStartTemp();
	const grcImage	*pLayer = pImage;
	int			layer = 0;
	bool		bIsPacked = (XGIsPackedTexture(static_cast<IDirect3DTexture9 *>(m_Texture)) == TRUE);
	while ( pLayer ) {
		const grcImage	*pMip = pLayer;
		XGTEXTURE_DESC desc;
		XGGetTextureDesc(m_Texture, 0, &desc);
		for	(int i = 0; i < pLayer->GetExtraMipCount() + 1; i++)
		{
			if ( imageType == grcImage::CUBE ) {
				// MAKE SURE D3D STILL MAINTAINS CUBEMAPS IN SEQUENTIAL ORDER
				CompileTimeAssert(
					D3DCUBEMAP_FACE_NEGATIVE_X==D3DCUBEMAP_FACE_POSITIVE_X+1 &&
					D3DCUBEMAP_FACE_POSITIVE_Y==D3DCUBEMAP_FACE_NEGATIVE_X+1 &&
					D3DCUBEMAP_FACE_NEGATIVE_Y==D3DCUBEMAP_FACE_POSITIVE_Y+1 &&
					D3DCUBEMAP_FACE_POSITIVE_Z==D3DCUBEMAP_FACE_NEGATIVE_Y+1 &&
					D3DCUBEMAP_FACE_NEGATIVE_Z==D3DCUBEMAP_FACE_POSITIVE_Z+1);

				static_cast<IDirect3DCubeTexture9 *>(m_Texture)->LockRect((D3DCUBEMAP_FACES)(D3DCUBEMAP_FACE_POSITIVE_X+layer), i, &rect, NULL, 0 );
			}
			else if ( imageType == grcImage::VOLUME ) {
				static_cast<IDirect3DVolumeTexture9 *>(m_Texture)->LockBox(i,&box,NULL,0);
			}
			else {		
				static_cast<IDirect3DTexture9*>(m_Texture)->LockRect(i, &rect, NULL, 0);
			}

			if( desc.Format & D3DFORMAT_TILED_MASK )
			{
				int		srcStride = pMip->GetStride() * (desc.Width / desc.WidthInBlocks);
				if (imageType == grcImage::VOLUME) {
					D3DBOX srcBox = {0,0,desc.Width>>i,desc.Height>>i,0,desc.Depth>>i};
					u32 w = desc.Width>>i;
					u32 h = desc.Height>>i; 
					u8* tempImage = rage_new u8[ box.SlicePitch* (desc.Depth>>i)];

					CopyVolumeMip((u8*)tempImage, box.RowPitch, box.SlicePitch,pMip,w,h,desc.BytesPerBlock);
					XGTileVolumeTextureLevel(desc.Width,desc.Height,desc.Depth,i,XGGetGpuFormat(UnExpandGPUFormat(desc.Format)),
							bIsPacked?0:XGTILE_NONPACKED, box.pBits, NULL , tempImage,
							box.RowPitch, box.SlicePitch, &srcBox);
					delete[] tempImage;
				}
				else {
					// Assert(!(desc.Flags & XGTDESC_PACKED));
					XGTileTextureLevel(desc.Width,desc.Height,i,XGGetGpuFormat(UnExpandGPUFormat(desc.Format)),
						bIsPacked?0:XGTILE_NONPACKED,rect.pBits,NULL,pMip->GetBits(),srcStride,NULL);
				}
			}
			else
			{
				// NOTE -- this does not work if stride is not a multiple of 256 bytes!
				if ( imageType != grcImage::VOLUME  ){
					sysMemCpy( imageType == grcImage::VOLUME ? box.pBits : rect.pBits, pMip->GetBits(),  pMip->GetSize() ); 
				}
				else
				{
					u32 w = desc.Width>>i;
					u32 h = desc.Height>>i; 
					CopyVolumeMip((u8*)box.pBits, box.RowPitch, box.SlicePitch,pMip,w,h,desc.BytesPerBlock);
				}
			}

			if ( imageType == grcImage::CUBE ) {
				static_cast<IDirect3DCubeTexture9 *>(m_Texture)->UnlockRect((D3DCUBEMAP_FACES)layer, i);
			}
			else if ( imageType == grcImage::VOLUME ) {
				static_cast<IDirect3DVolumeTexture9 *>(m_Texture)->UnlockBox(i);
			}
			else {
				static_cast<IDirect3DTexture9 *>(m_Texture)->UnlockRect(i);
			}
			pMip = pMip->GetNext();
		}
		pLayer = pLayer->GetNextLayer();
		layer++;
	}
	sysMemEndTemp();

	return true;
}

bool grcTextureXenon::Copy2D(const void* pSrc, u32 imgFormat, u32 width, u32 height, u32 numMips)
{
	grcImage::Format format = (grcImage::Format)(imgFormat);

	// get out if something doesn't match
	if (GetLayerCount() != 1U || format != (grcImage::Format)GetImageFormat() ||  m_Width != width || m_Height != height || m_MipCount != (int)numMips)
	{
		return false;
	}

	// get texture description
	D3DLOCKED_RECT	rect = { 0, 0 };
	XGTEXTURE_DESC desc;
	XGGetTextureDesc(m_Texture, 0, &desc);

	bool bIsPacked = (XGIsPackedTexture(static_cast<IDirect3DTexture9 *>(m_Texture)) == TRUE);

	// cache mip data
	const u32 bpp		= grcImage::GetFormatBitsPerPixel(format);
	const u32 blockSize	= grcImage::IsFormatDXTBlockCompressed(format) ? 4 : 1;

	const u32 baseWidth = Max<u32>(blockSize, width);
	const u32 baseHeight = Max<u32>(blockSize, height);

	u32 mipWidth =  baseWidth;
	u32 mipHeight = baseHeight;

	u32 mipPitch = mipWidth*bpp/8;

	u32 mipSizeInBytes = (mipHeight*mipPitch);
	u32 curMipOffset = 0;

	const char* pSrcMip = static_cast<const char*>(pSrc);

	// process and upload mips
	for (u32 i = 0; i < numMips; i++)
	{
		// lock mip
		static_cast<IDirect3DTexture9*>(m_Texture)->LockRect(i, &rect, NULL, 0);

		// update the desc
		XGGetTextureDesc(m_Texture, i, &desc);

		if( desc.Format & D3DFORMAT_TILED_MASK )
		{
			int	srcStride = (mipWidth*bpp/8) * (desc.Width / desc.WidthInBlocks);
			XGTileTextureLevel(baseWidth, baseHeight,i,XGGetGpuFormat(UnExpandGPUFormat(desc.Format)), bIsPacked?0:XGTILE_NONPACKED,rect.pBits,NULL, pSrcMip, srcStride,NULL);
		}
		else
		{
			// NOTE -- this does not work if stride is not a multiple of 256 bytes!
			sysMemCpy(rect.pBits, pSrcMip, mipSizeInBytes); 
		}

		// unlock mip
		static_cast<IDirect3DTexture9 *>(m_Texture)->UnlockRect(i);

		// skip current mip pixel data
		pSrcMip += mipSizeInBytes;

		// update mip data
		mipWidth = Max<u32>(blockSize, mipWidth/2);
		mipHeight = Max<u32>(blockSize, mipHeight/2);

		mipPitch = mipWidth*bpp/8;

		curMipOffset += mipSizeInBytes;
		mipSizeInBytes = (mipHeight*mipPitch);

	}

	return true;
}

//NOTE THAT THIS DOES NOT ALLOCATE/DEALLOCATE MEMORY.  THE MEMORY FOR THIS TEXTURE MUST BE MANAGED BY THE CLIENT.  
void grcTextureXenon::Resize(u32 width, u32 height)
{   
	IDirect3DTexture9* d3dTexture = static_cast<IDirect3DTexture9*>(m_Texture);
	XGTEXTURE_DESC surfaceDescription;
	XGGetTextureDesc(d3dTexture, 0, &surfaceDescription);

	//save off base address
	u32 baseAddress = d3dTexture->Format.BaseAddress;

	//this texture could be rendering i think. Should we lock before we do this?  or block? or pray?
	XGSetTextureHeader( 
		width,
		height,
		1, 
		D3DUSAGE_CPU_CACHED_MEMORY,
		surfaceDescription.Format,
		0,
		0,
		XGHEADER_CONTIGUOUS_MIP_OFFSET,
		0,
		d3dTexture,
		NULL,
		NULL ); 

	m_Width = (u16) width;
	m_Height = (u16) height;
	m_MipCount = 0;

	d3dTexture->Common |= D3DCOMMON_CPU_CACHED_MEMORY;

	d3dTexture->Format.BaseAddress = baseAddress;

	SetPhysicalSize(0);

	for (u32 i = 0; i < m_Texture->GetLevelCount(); ++i)
	{
		XGGetTextureDesc(m_Texture, i, &surfaceDescription);
		SetPhysicalSize(GetPhysicalSize() + XGTEXTURE_DESC_GetSize(surfaceDescription));
	}

// 	char buffer[128];
// 	prettyprinter(buffer, 128, GetPhysicalSize());
// 	Printf("\"%s\" %dx%dx%d %d %11s\n", GetName(), GetWidth(), GetHeight(), GetDepth(), surfaceDescription.Format, buffer);
}

int grcTextureXenon::GetBitsPerPixel() const
{
	XGTEXTURE_DESC surfaceDescription;
	XGGetTextureDesc(m_Texture, 0, &surfaceDescription);
	return surfaceDescription.BitsPerPixel;
}

int grcRenderTargetXenon::GetBitsPerPixel() const
{
	XGTEXTURE_DESC surfaceDescription;
	XGGetTextureDesc(m_Texture, 0, &surfaceDescription);
	return surfaceDescription.BitsPerPixel;
}

bool grcTextureXenon::LockRect(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags) const
{
	IDirect3DTexture9* d3dTexture = static_cast<IDirect3DTexture9*>(m_Texture);

	XGTEXTURE_DESC surfaceDescription;
	XGGetTextureDesc(d3dTexture, mipLevel, &surfaceDescription);
	lock.BitsPerPixel = surfaceDescription.BitsPerPixel;
	lock.Width = surfaceDescription.Width;
	lock.Height = surfaceDescription.Height;
	lock.RawFormat = surfaceDescription.Format;
	lock.MipLevel = mipLevel;
	lock.Layer = layer;

	u32 d3dLockFlags = 0;

	d3dLockFlags |= (uLockFlags & grcsNoOverwrite) ? D3DLOCK_NOOVERWRITE : 0;
	// Add more flags here if any of the rage flags map to the D3D ones. 
	// RMS - One D3D flag that sounds useful, but at the moment I don't have any good test cases for, is D3DLOCK_READONLY.

	switch (d3dTexture->GetType())
	{
	case D3DRTYPE_VOLUMETEXTURE:
		{
			D3DLOCKED_BOX lockedBox;
			if (SUCCEEDED(static_cast<IDirect3DVolumeTexture9*>(m_Texture)->LockBox(mipLevel, &lockedBox, NULL, d3dLockFlags)))
			{
				Assert(lockedBox.RowPitch * lock.Height == lockedBox.SlicePitch);
				lock.Base = reinterpret_cast<char*>(lockedBox.pBits) + lockedBox.SlicePitch *layer;
				lock.Pitch = lockedBox.RowPitch;
				break;
			}
			else
			{
				return false;
			}
		}
	case D3DRTYPE_CUBETEXTURE:
		{
			D3DLOCKED_RECT lockedRect;
			if (SUCCEEDED(static_cast<IDirect3DCubeTexture9*>(m_Texture)->LockRect(static_cast<D3DCUBEMAP_FACES>(layer), mipLevel, &lockedRect, NULL, d3dLockFlags)))
			{
				lock.Base = lockedRect.pBits;
				lock.Pitch = lockedRect.Pitch;
				break;
			}
			else
			{
				return false;
			}
		}
	case D3DRTYPE_TEXTURE:
		{
			D3DLOCKED_RECT lockedRect;
			if (SUCCEEDED(d3dTexture->LockRect(mipLevel, &lockedRect, NULL, d3dLockFlags)))
			{
				lock.Base = lockedRect.pBits;
				lock.Pitch = lockedRect.Pitch;
				break;
			}
			else
			{
				return false;
			}
		}

	default:
		Quitf("Invalid D3D resource (texture) type");
		return false;
	}

	return true;
}

void grcTextureXenon::UnlockRect(const grcTextureLock &lock) const
{
	IDirect3DTexture9* d3dTexture = static_cast<IDirect3DTexture9*>(m_Texture);

	switch (d3dTexture->GetType())
	{
	case D3DRTYPE_VOLUMETEXTURE:
		static_cast<IDirect3DVolumeTexture9*>(m_Texture)->UnlockBox(lock.Layer);
		break;
	case D3DRTYPE_CUBETEXTURE:
		static_cast<IDirect3DCubeTexture9*>(m_Texture)->UnlockRect(static_cast<D3DCUBEMAP_FACES>(lock.Layer), lock.MipLevel);
		break;
	case D3DRTYPE_TEXTURE:
		d3dTexture->UnlockRect(lock.MipLevel);
		break;
	default:
		Quitf("Invalid D3D resource (texture) type");
	}
}

grcTextureXenon::~grcTextureXenon	(void)
{
	if (m_Texture)
		GRCDEVICE.DeleteTexture(m_Texture);
}

grcTextureObject*	grcTextureXenon::GetTexturePtr()
{
	return m_Texture;
}

const grcTextureObject*	grcTextureXenon::GetTexturePtr() const
{
	return m_Texture;
}

void grcTextureFactoryXenon::PlaceTexture(class datResource &rsc,grcTexture &tex) {
	switch (tex.GetResourceType()) {
		case grcTexture::NORMAL: ::new (&tex) grcTextureXenon(rsc); break;
		case grcTexture::RENDERTARGET: Assert(0 && "unsafe to reference a rendertarget"); break;
		case grcTexture::REFERENCE: ::new (&tex) grcTextureReference(rsc); break;
		default: Quitf("Bad resource type %d in grcTextureFactoryXenon::PlaceTexture",tex.GetResourceType());
	}
}


grcTextureXenon::grcTextureXenon(datResource &rsc) : grcTextureXenonProxy(rsc) {
	rsc.PointerFixup(m_Name);
	rsc.PointerFixup(m_Texture);
	m_Texture->Format.BaseAddress += rsc.GetFixup((void*)(m_Texture->Format.BaseAddress << 12)) >> 12;
	if (m_Texture->Format.MipAddress)
		m_Texture->Format.MipAddress += rsc.GetFixup((void*)(m_Texture->Format.MipAddress << 12)) >> 12;
	m_CachedTexturePtr = GetTexturePtr();
}

bool grcTextureXenon::IsGammaEnabled() const
{
	return
	(
		m_Texture->Format.SignX == GPUSIGN_GAMMA &&
		m_Texture->Format.SignY == GPUSIGN_GAMMA &&
		m_Texture->Format.SignZ == GPUSIGN_GAMMA
	);
}

void grcTextureXenon::SetGammaEnabled(bool enabled)
{
	if (enabled && m_Texture->Format.DataFormat == GPUTEXTUREFORMAT_8_8_8_8)
	{
		m_Texture->Format.SignX = GPUSIGN_GAMMA;
		m_Texture->Format.SignY = GPUSIGN_GAMMA;
		m_Texture->Format.SignZ = GPUSIGN_GAMMA;
	}
	else
	{
		m_Texture->Format.SignX = GPUSIGN_UNSIGNED;
		m_Texture->Format.SignY = GPUSIGN_UNSIGNED;
		m_Texture->Format.SignZ = GPUSIGN_UNSIGNED;
	}
}

grcTexture::ChannelBits grcTextureXenon::FindUsedChannels() const
{

	XGTEXTURE_DESC surfaceDescription;
	XGGetTextureDesc(m_Texture, 0, &surfaceDescription);

	u32 fmt = (u32)surfaceDescription.Format;

	ChannelBits bits(false);
	u32 swizzleX = (fmt & D3DFORMAT_SWIZZLEX_MASK) >> D3DFORMAT_SWIZZLEX_SHIFT;
	u32 swizzleY = (fmt & D3DFORMAT_SWIZZLEY_MASK) >> D3DFORMAT_SWIZZLEY_SHIFT;
	u32 swizzleZ = (fmt & D3DFORMAT_SWIZZLEZ_MASK) >> D3DFORMAT_SWIZZLEZ_SHIFT;
	u32 swizzleW = (fmt & D3DFORMAT_SWIZZLEW_MASK) >> D3DFORMAT_SWIZZLEW_SHIFT;

	if (swizzleX == GPUSWIZZLE_X ||
		swizzleX == GPUSWIZZLE_Y || 
		swizzleX == GPUSWIZZLE_Z ||
		swizzleX == GPUSWIZZLE_W)
	{
		bits.Set(CHANNEL_RED);
	}
	if (swizzleY == GPUSWIZZLE_X ||
		swizzleY == GPUSWIZZLE_Y || 
		swizzleY == GPUSWIZZLE_Z ||
		swizzleY == GPUSWIZZLE_W)
	{
		bits.Set(CHANNEL_GREEN);
	}
	if (swizzleZ == GPUSWIZZLE_X ||
		swizzleZ == GPUSWIZZLE_Y || 
		swizzleZ == GPUSWIZZLE_Z ||
		swizzleZ == GPUSWIZZLE_W)
	{
		bits.Set(CHANNEL_BLUE);
	}
	if (swizzleW == GPUSWIZZLE_X ||
		swizzleW == GPUSWIZZLE_Y || 
		swizzleW == GPUSWIZZLE_Z ||
		swizzleW == GPUSWIZZLE_W)
	{
		bits.Set(CHANNEL_ALPHA);
	}

	// Check for some special cases
	// treat all formats as tiled (to make the switch shorter)
	fmt |= D3DFORMAT_TILED_MASK;
	switch(fmt)
	{
		// Depth
	case D3DFMT_D16:
	case D3DFMT_D24X8:
	case D3DFMT_D32:
		bits.Reset();
		bits.Set(CHANNEL_DEPTH);
		break;

		// Depth/Stencil
	case D3DFMT_D24S8:
	case D3DFMT_D24FS8:
		bits.Reset();
		bits.Set(CHANNEL_DEPTH);
		bits.Set(CHANNEL_STENCIL);
		break;

	case D3DFMT_G8R8_G8B8: // alternate pixels carry different color components
	case D3DFMT_R8G8_B8G8: // alternate pixels carry different color components
		bits.Reset();

	default:
		break;
	}

	return bits;
}

void grcTextureXenon::Tile(int level)
{
	grcTextureLock lock;
	LockRect(0, level, lock);

	ASSERT_ONLY(D3DFORMAT format = static_cast<D3DFORMAT>(lock.RawFormat);)
	Assert(XGIsTiledFormat(format));

	DWORD flags = 0;
	if (!XGIsPackedTexture(m_Texture))
	{
		flags |= XGTILE_NONPACKED;
	}
	if (XGIsBorderTexture(m_Texture))
	{
		flags |= XGTILE_BORDER;
	}

	u32 blockSize = (GetImageFormat() == grcImage::DXT1) ? 8 : 16;
	Assert(GetImageFormat() == grcImage::DXT1 || GetImageFormat() == grcImage::DXT5);

	//XGTileTextureLevel(this->GetWidth(), this->GetHeight(), level, XGGetGpuFormat(format), flags, lock.Base, NULL, lock.Base, lock.Pitch, NULL);
	XGTileSurface(lock.Base, lock.Width >> 2, lock.Height >> 2, NULL, lock.Base, lock.Pitch, NULL, blockSize);

	UnlockRect(lock);
}

u32 grcTextureXenon::GetInternalFormat(u32 imgFormat, bool bIsLinear)
{
	return (u32)GetD3DFormat((grcImage::Format)imgFormat, bIsLinear);
}

int grcRenderTargetXenon::GetMipMapCount() const 
{
	return m_Texture ? m_Texture->GetLevelCount() : 1;
}

bool grcRenderTargetXenon::IsGammaEnabled() const
{
	return
	(
		m_Texture &&
		m_Texture->Format.SignX == GPUSIGN_GAMMA &&
		m_Texture->Format.SignY == GPUSIGN_GAMMA &&
		m_Texture->Format.SignZ == GPUSIGN_GAMMA
	);
}

void grcRenderTargetXenon::SetGammaEnabled(bool enabled)
{
	if (m_Texture)
	{
		if (enabled && m_Texture->Format.DataFormat == GPUTEXTUREFORMAT_8_8_8_8)
		{
			m_Texture->Format.SignX = GPUSIGN_GAMMA;
			m_Texture->Format.SignY = GPUSIGN_GAMMA;
			m_Texture->Format.SignZ = GPUSIGN_GAMMA;
		}
		else
		{
			m_Texture->Format.SignX = GPUSIGN_UNSIGNED;
			m_Texture->Format.SignY = GPUSIGN_UNSIGNED;
			m_Texture->Format.SignZ = GPUSIGN_UNSIGNED;
		}
	}
}

//
//	PURPOSE
//		Create mip maps for the texture on the fly
//		mip maps allow for a lot better data coherency which is important
//		for hardware across the board.
//
//	NOTES
// This should be called after the top level mip map has been created
//
void grcRenderTargetXenon::CreateMipMaps( const grcResolveFlags* resolveFlags, int index)
{
	Assert( m_Texture );
	Assert( index >=0 && index <= grcmrtColorCount );
	Assertf( !XGIsPackedTexture(m_Texture), "%s has a packed mip chain", GetName());

	if (GetMipMapCount() == 1)
	{
		return;
	}

	static u32 resolveTarget[] = 
	{ 
		D3DRESOLVE_RENDERTARGET0,
		D3DRESOLVE_RENDERTARGET1,
		D3DRESOLVE_RENDERTARGET2,
		D3DRESOLVE_RENDERTARGET3
	};

	Vector2 sourceOffset( 0.0f, 0.0f );
	Vector2 sourceSize( 1.0f, 1.0f );
	D3DPOINT topCorner;
	topCorner.x = topCorner.y = 0;
	
	D3DPOINT* texOffset = &topCorner;

	bool needsTgtBind = m_Width > GRCDEVICE.GetWidth() || m_Height > GRCDEVICE.GetHeight();

	grcDeviceSurface* prevTgts[4];
	for(int i = 0; i < 4; i++)
	{
		if(i != index)
		{
			GRCDEVICE.GetRenderTarget(i, &prevTgts[i]);
			GRCDEVICE.SetRenderTarget(i, NULL);
		}
	}

	grcDeviceSurface *prevTgt = NULL;
	if(needsTgtBind)
	{
		GRCDEVICE.GetRenderTarget(0, &prevTgt);

		IDirect3DSurface9* tsurface = GetSurface();
		LockSurface(0);
		GRCDEVICE.SetRenderTarget(0, tsurface);
	}

	D3DPOINT pt(*texOffset); 

	D3DRECT	rect;
	rect.x1 = 0;
 	rect.y1 = 0;
 	rect.x2 = m_Width;
 	rect.y2 = m_Height;

	D3DVECTOR4 clearColor;
	if ( resolveFlags )
	{
		clearColor.x = resolveFlags->Color.GetRedf();
		clearColor.y = resolveFlags->Color.GetGreenf();
		clearColor.z = resolveFlags->Color.GetBluef(); 
		clearColor.w = resolveFlags->Color.GetAlphaf();
	}
	
	UINT levels = GetMipMapCount();
	
	for (UINT i = 1; i < levels; i++)
	{
		pt.x >>= 1;
		pt.y >>= 1;

		rect.x2 = Max<LONG>(rect.x2 >> 1, 1);
		rect.y2 = Max<LONG>(rect.y2 >> 1, 1);

		sm_FastMipMapper.DownSample( this, index, i, sourceOffset, sourceSize, m_Type );	// might be overly expensive setting all the state and stuff

		if (m_Type == grcrtShadowMap || m_Type == grcrtDepthBuffer) 
		{
			if (resolveFlags)
				GRCDEVICE.GetCurrent()->Resolve( D3DRESOLVE_DEPTHSTENCIL, NULL, m_Texture, NULL, i, 0, NULL, grcDevice::FixDepth(resolveFlags->Depth),
					resolveFlags->Stencil, NULL);
			else
				GRCDEVICE.GetCurrent()->Resolve( D3DRESOLVE_FRAGMENT0 | D3DRESOLVE_DEPTHSTENCIL, NULL, m_Texture, NULL, i, 0, NULL, 0, 0, NULL);
		}
		else
		{
			// Resolve to the mip-level of our texture.
			if (resolveFlags)
				// we don't write to z so we don't need to clear it
				GRCDEVICE.GetCurrent()->Resolve( D3DRESOLVE_ALLFRAGMENTS | ((resolveFlags->ClearColor)?D3DRESOLVE_CLEARRENDERTARGET:0) | resolveTarget[index],
				&rect, m_Texture, &pt, i, 0, &clearColor, grcDevice::FixDepth(resolveFlags->Depth), resolveFlags->Stencil, NULL);
			else
				GRCDEVICE.GetCurrent()->Resolve( D3DRESOLVE_ALLFRAGMENTS | resolveTarget[index], &rect, m_Texture, &pt, i, 0, NULL, 1.0f, 0, NULL );
		}
	}

	for(int i = 0; i < 4; i++)
	{
		if(i != index)
		{
			if(prevTgts[i])
			{
				GRCDEVICE.SetRenderTarget(i, prevTgts[i]);
				prevTgts[i]->Release();
			}
		}
	}

	if(needsTgtBind)
	{
		GRCDEVICE.SetRenderTarget(0, prevTgt);
		prevTgt->Release();
	}
}

void grcRenderTargetXenon::Blur(const grcResolveFlags* clearFlags)
{
	Assert( m_Type != grcrtDepthBuffer && m_Type != grcrtShadowMap);

	D3DRECT rect;
	Vector2 sourceOffset( 0.0f, 0.0f );
	Vector2 sourceSize( 1.0f, 1.0f );
	D3DPOINT* texOffset = NULL;
	rect.x1 = 0;
	rect.y1 = 0;
	rect.x2 = m_Width;
	rect.y2 = m_Height;

	D3DVECTOR4 clearColor;
	if ( clearFlags )
	{
		clearColor.x= clearFlags->Color.GetRedf();
		clearColor.y = clearFlags->Color.GetGreenf();
		clearColor.z = clearFlags->Color.GetBluef(); 
		clearColor.w = clearFlags->Color.GetAlphaf();
	}

	sm_FastMipMapper.Blur( this, clearFlags->BlurKernelSize, sourceOffset, sourceSize  );	

	if ( clearFlags == 0 )
	{
		GRCDEVICE.GetCurrent()->Resolve( D3DRESOLVE_ALLFRAGMENTS, &rect, m_Texture, texOffset, 0, 0, 
			NULL, 1.0f, 0, NULL );
	}
	else
	{
		// we don't write to z so we don't need to clear it
		GRCDEVICE.GetCurrent()->Resolve( D3DRESOLVE_ALLFRAGMENTS |((clearFlags->ClearColor)?D3DRESOLVE_CLEARRENDERTARGET:0), &rect, m_Texture, texOffset, 0, 0, 
			&clearColor, grcDevice::FixDepth(clearFlags->Depth), clearFlags->Stencil, NULL);
	}
}

grcTexture::ChannelBits grcRenderTargetXenon::FindUsedChannels() const
{
	return FindUsedChannelsFromRageFormat(m_Format);
}

void grcRenderTargetXenon::SetExpAdjust(int iExpAdjust)
{
	m_Texture->Format.ExpAdjust = iExpAdjust;
}

u16 grcRenderTargetXenon::GetHiZSize() const
{
	u16 numHiZTiles = 0;

	if (GetType() == grcrtDepthBuffer || GetType() == grcrtShadowMap) 
	{
		D3DSURFACE_DESC desc;
		m_Surface->GetDesc( &desc );

		numHiZTiles = static_cast<u16>( XGHierarchicalZSize( desc.Width, desc.Height, desc.MultiSampleType ) );
	}

	return numHiZTiles;
}

/* typedef enum
{
	GPUSIGN_UNSIGNED                                = 0,
	GPUSIGN_SIGNED                                  = 1,
	GPUSIGN_BIAS                                    = 2,
	GPUSIGN_GAMMA                                   = 3,
} GPUSIGN; */

static u32 _GetSignedMask(D3DBaseTexture *tex)
{
	return (tex->Format.SignX? 1 : 0) | (tex->Format.SignY? 2 : 0) | (tex->Format.SignZ? 4 : 0) | (tex->Format.SignW? 8 : 0);
}

static void _SetSignedMask(D3DBaseTexture *tex,u32 mask)
{
	tex->Format.SignX = (mask & 1) >> 0;
	tex->Format.SignY = (mask & 2) >> 1;
	tex->Format.SignZ = (mask & 4) >> 2;
	tex->Format.SignW = (mask & 8) >> 3;
}

u32 grcTextureXenon::GetTextureSignedMask() const {
	return _GetSignedMask(m_Texture);
}

void grcTextureXenon::SetTextureSignedMask(u32 mask) {
	_SetSignedMask(m_Texture,mask);
}

u32 grcRenderTargetXenon::GetTextureSignedMask() const {
	return _GetSignedMask(m_Texture);
}

void grcRenderTargetXenon::SetTextureSignedMask(u32 mask) {
	_SetSignedMask(m_Texture,mask);
}


}	// namespace rage

#endif	// __XENON

