//
// grcore/texture.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "grcore/image.h"
#include "grcore/image_dxt.h"
#include "grcore/channel.h"
#include "grcore/texture.h"
#include "grcore/texturedefault.h"
#include "grcore/texturereference.h"
#include "grcore/texturecontrol.h"
#include "grcore/d3dwrapper.h"
#include "grcore/quads.h"
#include "grcore/im.h"
#include "grcore/effect.h"
#include "grcore/effect_values.h"
#include "diag/art_channel.h"
#include "diag/output.h"

#include "atl/map.h"
#include "string/string.h"
#include "data/resource.h"
#include "data/struct.h"
#include "data/resourcehelpers.h"
#include "paging/dictionary.h"
#include "system/memory.h"
#include "system/param.h"
#include "system/externalheap.h"
#include "file/asset.h"
#include "file/token.h"
#include "shaderlib/rage_constants.h"

#if __XENON
#include "system/xtl.h"
#endif

#if __BANK_TEXTURE_CONTROL
#include "bank/button.h"
#endif

namespace rage {

PARAM(srgb, "[grcore] enable automatic SRGB conversion for the front/backbuffers");
PARAM(notextures, "[grcore] force all CreateTexture calls to return empty texture");
PARAM(rendertargetlog, "[grcore] the name of the render target log (dumping is enabled via the render target widget)");
PARAM(rendertargetpoollog, "[grcore] the name of the render target pool log (dumping is enabled via the render target widget)");

s32 grcTextureSuppressReferenceWarning = 0;

int grcTexture::sm_MemoryBucket = 1;

template<> DICTIONARY_THREAD pgDictionary<grcTexture>* pgDictionary<grcTexture>::sm_CurrentDict = NULL;

const grcTexture* grcTexture::None;
const grcTexture* grcTexture::NoneDepth;
const grcTexture* grcTexture::NoneBlack;
const grcTexture* grcTexture::NoneBlackTransparent;
const grcTexture* grcTexture::NoneWhite;
const grcTexture* grcTexture::Nonresident;
grcFastMipMapper grcTexture::sm_FastMipMapper;
grcTexture *grcTextureFactory::sm_NotImplementedTexture = 0;
grcRenderTarget *grcTextureFactory::sm_NotImplementedRenderTarget = 0;
grcTextureFactory* grcTextureFactory::sm_Instance = 0;
grcTextureFactory* grcTextureFactory::sm_PrevInstance = 0;
grcTextureFactory::RenderTargetStack grcTextureFactory::sm_DefaultRenderTargets[grcmrtCount];
atArray<grcRenderTarget *> grcTextureFactory::sm_RenderTargets;
atArray<grcRenderTargetMemPool*> grcRenderTargetMemPool::sm_MemPools;

#if __RESOURCECOMPILER

grcTexture::CustomProcessFuncType grcTexture::sm_customProcessFunc = NULL;

void grcTexture::SetCustomProcessFunc(CustomProcessFuncType func)
{
	sm_customProcessFunc = func;
}

bool grcTexture::RunCustomProcessFunc(const void* params)
{
	if (sm_customProcessFunc)
	{
		return sm_customProcessFunc(this, params);
	}

	return false;
}

#endif // __RESOURCECOMPILER

void grcTexture::SetSuppressReferenceWarning()
{
	++grcTextureSuppressReferenceWarning;
}

void grcTexture::ClearSuppressReferenceWarning()
{
	--grcTextureSuppressReferenceWarning;
}

#if __BANK || __RESOURCECOMPILER

grcTexture::CustomLoadFuncType grcTexture::sm_customLoadFunc = NULL;
const char*                    grcTexture::sm_customLoadName = NULL;
const char*                    grcTexture::sm_txdExtension   = "---";

void grcTexture::SetCustomLoadFunc(CustomLoadFuncType func, const char* txdExtension)
{
	sm_customLoadFunc = func;
	sm_txdExtension   = txdExtension;
}

void grcTexture::SetCustomLoadName(const char* name)
{
	sm_customLoadName = name;
}

const char* grcTexture::GetCustomLoadName(const char* name)
{
	return sm_customLoadName ? sm_customLoadName : name;
}

grcTexture* grcTexture::RunCustomLoadFunc(const char* filename)
{
	if (sm_customLoadFunc)
	{
		return sm_customLoadFunc(filename);
	}

	return NULL;
}

const char* grcTexture::GetTxdExtension()
{
	return sm_txdExtension;
}

grcTexture* grcTexture::Clone(const char* name) const
{
	const grcTexture* srcTexture = this;
	grcTexture* newTexture = NULL;

	if (srcTexture->GetDepth() == 1 && srcTexture->GetLayerCount() == 1) // only works for 2D textures
	{
		const int              w =                   srcTexture->GetWidth          ();
		const int              h =                   srcTexture->GetHeight         ();
		const int              m =                   srcTexture->GetMipMapCount    ();
		const grcImage::Format f = (grcImage::Format)srcTexture->GetImageFormat    ();
		const u8               c =                   srcTexture->GetConversionFlags();
		const u16              t =                   srcTexture->GetTemplateType   ();

		eTextureSwizzle swizzle[4];
		srcTexture->GetTextureSwizzle(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);

		// create new image because we can't create a texture directly from w,h,m,f?!
		grcImage* newImage = grcImage::Create(w, h, 1, f, grcImage::STANDARD, m - 1, 0);

		if (newImage)
		{
			SetCustomLoadName(name);
			newTexture = grcTextureFactory::GetInstance().Create(newImage);
			SetCustomLoadName(NULL);

			if (newTexture)
			{
				for (int i = 0; i < m; i++) // copy each mipmap
				{
					grcTextureLock srcLock;
					grcTextureLock newLock;

					AssertVerify( srcTexture->LockRect(0, i, srcLock, grcsRead  | grcsAllowVRAMLock) );
					AssertVerify( newTexture->LockRect(0, i, newLock, grcsWrite | grcsAllowVRAMLock) );

					if (newLock.Base && srcLock.Base)
					{
						const int blockSize = grcImage::IsFormatDXTBlockCompressed( f ) ? 4 : 1;
						sysMemCpy(newLock.Base, srcLock.Base, ( newLock.Pitch / blockSize ) * newLock.Height);
					}

					srcTexture->UnlockRect(srcLock);
					newTexture->UnlockRect(newLock);
				}

				newTexture->SetTextureSwizzle(swizzle[0], swizzle[1], swizzle[2], swizzle[3], false);
				newTexture->SetConversionFlags(c);
				newTexture->SetTemplateType(t);
			}
			else
			{
				// grcTextureFactory::GetInstance().Create failed
			}

			newImage->Release();
		}
		else
		{
			// grcImage::Create failed
		}
	}
	else
	{
		// texture is a cubemap or a volume texture
	}

	return newTexture;
}

#endif // __BANK || __RESOURCECOMPILER

#if __BANK
char grcRenderTarget::sm_LogName[256];
bool grcRenderTarget::sm_EnableLoggingNextFrame = false;
utimer_t grcRenderTarget::sm_StartTicks;				// the frame start time value
fiStream * grcRenderTarget::sm_LogStream		= NULL;

char grcRenderTargetPoolMgr::sm_LogName[256];
bool grcRenderTargetPoolMgr::sm_EnableLoggingNextFrame = false;
#endif

bool grcEnableTrilinear = false;
bool grcEnableSwizzle = true;

bool grcRegenQuick;
bool grcMakeTags;
int grcShaderGroupIndex;
bool grcSaveTextureNames = true;
bool grcUseUniqueTextureRemapIds = true;

// Really dumb (but fast) data structure, only suitable for types without a ctor or dtor.
template <typename _T,size_t size> class atSparseHash
{
public:
	_T* Access(u32 code)
	{
		u32 index = code % size;
		if (m_Codes[index] == code)
			return &m_Values[index];
		else
			return NULL;
	}
	void Kill()
	{
		// TODO: Invoke dtors?
		memset(m_Codes, 0, sizeof(m_Codes));
	}
	void Delete(u32 code)
	{
		m_Codes[code % size] = 0;
	}
	void Insert(u32 code,_T value)
	{
		u32 index = code % size;
		Assertf(!m_Codes[index],"atSparseHash collision on %x, try size other than %" SIZETFMT "u",code,size);
		m_Codes[index] = code;
		m_Values[index] = value;
	}
	u32 m_Codes[size];
	_T m_Values[size];
};

static atSparseHash<grcTextureHandle, 89> s_TextureRefHash;

grcTextureFactory::grcTextureFactory() {
}


grcTextureFactory::~grcTextureFactory() {
	if (sm_Instance == this)
		sm_Instance = 0;
}


void grcTextureFactory::InitClass() {

	// Reset all the global texture pointers
	memset(sm_GlobalTextures, 0, sizeof(sm_GlobalTextures));

	grcTextureFactory& textureFactory = GetInstance();

	grcTexture::None = textureFactory.Create("none");
	grcTexture::Nonresident = textureFactory.Create("nonresident");

	grcImage *image = grcImage::MakeChecker(8, Color32(255, 229, 53), Color32(53, 253, 255));
	BANK_ONLY(grcTexture::SetCustomLoadName("Not Implemented");)
	sm_NotImplementedTexture = textureFactory.Create(image);
	BANK_ONLY(grcTexture::SetCustomLoadName(NULL);)
	sm_NotImplementedRenderTarget = textureFactory.CreateRenderTarget("Not Implemented", grcrtPermanent, 1, 1, 32);
	image->Release();

	image = grcImage::Create(4, 4, 1, grcImage::R32F, grcImage::DEPTH, 0, 0);
	for (int i=0; i<16; ++i)
	{
		*reinterpret_cast<float*>(image->GetBits()) = 1.f;	//dummy depth value
	}
	BANK_ONLY(grcTexture::SetCustomLoadName("NoneDepth");)
	grcTexture::NoneDepth = textureFactory.Create(image);
	image->Release();

	image = grcImage::Create(4, 4, 1, grcImage::DXT1, grcImage::STANDARD, 0, 0);
	DXT::DXT1_BLOCK* block = reinterpret_cast<DXT::DXT1_BLOCK*>(image->GetBits());
	block->SetColour(DXT::ARGB8888(0,0,0,255));
	BANK_ONLY(grcTexture::SetCustomLoadName("NoneBlack");)
	grcTexture::NoneBlack = textureFactory.Create(image);
	block->SetColour(DXT::ARGB8888(0,0,0,0));
	BANK_ONLY(grcTexture::SetCustomLoadName("NoneBlackTransparent");)
	grcTexture::NoneBlackTransparent = textureFactory.Create(image);	
	block->SetColour(DXT::ARGB8888(255,255,255,255));
	BANK_ONLY(grcTexture::SetCustomLoadName("NoneWhite");)
	grcTexture::NoneWhite = textureFactory.Create(image);
	image->Release();
	BANK_ONLY(grcTexture::SetCustomLoadName(NULL);)

	grcCompositeRenderTargetHelper::InitClass();
}


void grcTextureFactory::ShutdownClass() {
	UnloadGlobalTextures();
	if (grcTexture::None->Release())
		grcWarningf("Probable leak: %d outstanding references to None texture.",grcTexture::None->GetRefCount());
	grcTexture::None = 0;
	if (grcTexture::NoneBlack->Release())
		grcWarningf("Probable leak: %d outstanding references to NoneBlack texture.",grcTexture::NoneBlack->GetRefCount());
	grcTexture::NoneBlack = 0;
	if (grcTexture::NoneWhite->Release())
		grcWarningf("Probable leak: %d outstanding references to NoneWhite texture.",grcTexture::NoneWhite->GetRefCount());
	grcTexture::NoneWhite = 0;
	ASSERT_ONLY(int refCount =) grcTexture::Nonresident->Release();
	Assert(refCount == 0);
	grcTexture::Nonresident = 0;
	s_TextureRefHash.Kill();
	sm_NotImplementedTexture->Release();
	if(sm_NotImplementedRenderTarget)
		sm_NotImplementedRenderTarget->Release();
}


int grcTextureFactory::sm_MaxAnisotropy = 16;

#if __BANK
sysCriticalSectionToken grcTextureFactory::sm_CritSectionToken;

// For B*1923551 - Rare crash in CREnderTargets::UpdateBank. List size count becode stale during function execution.
sysCriticalSectionToken &grcTextureFactory::GetRenderTargetListCritSectionToken()
{
	return sm_CritSectionToken;
}
#endif //__BANK


void grcTextureFactory::RegisterRenderTarget(grcRenderTarget* renderTarget)
{
	BANK_ONLY(sysCriticalSection cs(grcTextureFactory::sm_CritSectionToken));
	sysMemStartTemp();
	sm_RenderTargets.Grow() = renderTarget;
	sysMemEndTemp();
}

void grcTextureFactory::UnregisterRenderTarget(grcRenderTarget* renderTarget)
{
	BANK_ONLY(sysCriticalSection cs(grcTextureFactory::sm_CritSectionToken));
	for (int i=0; i<sm_RenderTargets.GetCount(); )
		if (sm_RenderTargets[i] == renderTarget) {
			sm_RenderTargets.Delete(i);
			return;
		}
		else
			++i;
}

grcTexture * grcTextureFactory::sm_GlobalTextures[MAX_GLOBAL_TEXTURES];

void grcTextureFactory::PreloadGlobalTextures( const char *path ) {

	// Support loading multiple lists
	int count = 0;
	for (int i=0; i<MAX_GLOBAL_TEXTURES; i++) {
		if (sm_GlobalTextures[i]) {
			count = i;
			break;
		}
	}

	ASSET.PushFolder(path);
	fiStream *S = ASSET.Open("globaltex","list");
	if (S) {
		grcDisplayf("Preloading global textures from '%s'...",path);
		fiTokenizer T(path,S);
		char textureVarName[128];
		char textureFileName[128];

		ASSET.PushFolder("globaltex");

		while (T.GetToken(textureVarName,sizeof(textureVarName))) {
			T.GetToken(textureFileName, sizeof(textureFileName));
			
			grcTexture * texture = Create(textureFileName);
			if (!texture) {
				grcErrorf("Could not create texture '%s' from globaltex.list in '%s'", textureFileName, path);
				continue;
			}
			sm_GlobalTextures[count++] = texture;
			RegisterTextureReference(textureVarName, texture);

			if (count >= MAX_GLOBAL_TEXTURES)
			{
				grcWarningf("Reached max number of global textures from globaltex.list in '%s'", path);
				break;
			}
		}

		ASSET.PopFolder();

		S->Close();
	}
	else
		grcErrorf("globaltex.list file missing in '%s'",path);
	ASSET.PopFolder();
}

void grcTextureFactory::UnloadGlobalTextures()
{
	for (int i=0; i<MAX_GLOBAL_TEXTURES; i++)
	{
		if (sm_GlobalTextures[i])
		{
			sm_GlobalTextures[i]->Release();
			sm_GlobalTextures[i] = NULL;
		}
	}
}

#if HACK_GTA4
#define STRIP_EXT_FROM_NAME(name) Assertf(!strchr(name,'.'),"Texture '%s' contains an extension, no longer allowed!",name);
#else
#define STRIP_EXT_FROM_NAME(name) \
	char noExt[RAGE_MAX_PATH];	\
	safecpy(noExt,name); \
	char *ext = strrchr(noExt,'.'); \
	if (ext) { \
		*ext = '\0'; \
		name = noExt; \
	}
#endif

void grcTextureFactory::RegisterTextureReference(const char *name,const grcTexture *texture) {
	if(texture) {
		STRIP_EXT_FROM_NAME(name);
		grcTextureHandle handle;
		handle = const_cast<grcTexture*>(texture);
		s_TextureRefHash.Insert(atStringHash(name),handle);
	}
}


grcTexture* grcTextureFactory::LookupTextureReference(const char *name) {
	grcTexture *result = NULL;
	STRIP_EXT_FROM_NAME(name);

	u32 code = atStringHash(name);

	if (code == ATSTRINGHASH("none",0x1d632ba1) && grcTexture::None)
		result = const_cast<grcTexture*>(grcTexture::None);
	else if (const grcTextureHandle* handle = s_TextureRefHash.Access(code))
#if GRCTEXTUREHANDLE_IS_PGREF
		result = *handle;
#else
		result = *handle->ptr;
#endif
	else if (pgDictionary<grcTexture> *td = pgDictionary<grcTexture>::GetCurrent()) {
		do {
			result = td->LookupLocal(code);
			td = td->GetParent();
		} while (td && !result);
		if (!result && !grcTextureSuppressReferenceWarning)
			artWarningf("Texture '%s' not found!",name);
	}
	if (result)
		result->AddRef();
	return result;
}


grcTexture* grcTextureFactory::CreateTexture(const char *name) {
#if !__FINAL
	if (PARAM_notextures.Get()) {
		if (grcTexture::None)
			grcTexture::None->AddRef();
		return const_cast<grcTexture*>(grcTexture::None);
	}
#endif

	STRIP_EXT_FROM_NAME(name);

	if (const grcTextureHandle* handle = s_TextureRefHash.Access(atStringHash(name)))
		return rage_new grcTextureReference(name,
#if GRCTEXTUREHANDLE_IS_PGREF
		*handle
#else
		*handle->ptr
#endif
		);

	grcTexture *tex = NULL;
	// Try a dictionary reference first
	// TODO: We need to make sure this TD is part of the active shader group,
	// and not a global dictionary.  Note that we intentionally don't search the hierarchy here.
	if (pgDictionary<grcTexture> *td = pgDictionary<grcTexture>::GetCurrent()) {
		u32 code = td->ComputeHash(name);
		tex = td->LookupLocal(code);
		if (tex) {
			tex->AddRef();
			return tex;
		}
	}
#if __RESOURCECOMPILER
	tex = rage_new grcTextureReference(name,NULL);
#else
	if (!tex) {
		// If that doesn't work, try loading it directly.
		grcImage *image = grcImage::Load(name);
		if (image) {
			tex = GetInstance().Create(image);
			Assert(tex);
			image->Release();
		}
		// Finally, create an indirect reference if all else fails
		else
			tex = rage_new grcTextureReference(name,NULL);
	}
#endif
	return tex;
}


grcTexture* grcTextureFactory::ConnectToTextureReference(const char *name) {
	STRIP_EXT_FROM_NAME(name);
	if (const grcTextureHandle* handle = s_TextureRefHash.Access(atStringHash(name)))
#if GRCTEXTUREHANDLE_IS_PGREF
		return *handle; /// ->ptr;
#else
		return *handle->ptr;
#endif
	else
		return NULL;
}


void grcTextureFactory::DeleteTextureReference(const char *name) {
	s_TextureRefHash.Delete(atStringHash(name));
}

int grcTextureFactory::GetBitsPerPixel(grcTextureFormat eFormat)
{
	switch(eFormat)
	{
	case grctfNone:
	WIN32PC_ONLY(case grctfNULL:)
		return 0;
	case grctfL8:
		return 8;

	case grctfR5G6B5:
#if !__PS3
	case grctfR16F:
	case grctfA1R5G5B5:
	case grctfA4R4G4B4:
#else
	case grctfD16:
#endif
		return 16;

	case grctfA8R8G8B8:
	case grctfR32F:
#if __D3D11 || RSG_ORBIS
	case grctfR11G11B10F:
#endif
#if !__PS3
	case grctfA2B10G10R10:
	case grctfA2B10G10R10ATI:
	case grctfA8B8G8R8_SNORM:
#endif 
	case grctfG16R16:
	case grctfG16R16F:
	case grctfD24S8:
	XENON_ONLY(case grctfD24FS8_ReadStencil:)
	WIN32PC_ONLY(case grctfD32F:)
	WIN32PC_ONLY(case grctfX8R8G8B8:)
	WIN32PC_ONLY(case grctfX24G8:)
		return 32;

#if __D3D11
	case grctfD32FS8:
	case grctfX32S8:
		return 40;
#endif // __D3D11

	case grctfA16B16G16R16F:
#if !__PS3
	NONPSN_FMT_ONLY(case grctfA16B16G16R16F_NoExpand:)
	NONPSN_FMT_ONLY(case grctfA16B16G16R16:)
#endif
	XENON_ONLY(case grctfG32R32F:)
		return 64;

	case grctfA32B32G32R32F:
		return 128;
	default:
		Errorf("Unknown Texture Format %d", eFormat);
	}
	return 0;
}

grcTexture::grcTexture(u8 type) : m_Name(NULL), m_RefCount(1), m_ResourceTypeAndConversionFlags(type), m_LayerCount(0), m_CachedTexturePtr(0), m_PhysicalSizeAndTemplateType(0), m_HandleIndex(0) { 
	memset(&m_Texture, 0, sizeof(m_Texture));
}

grcTexture::grcTexture(class datResource &) { 
}


grcTexture* grcTexture::GetReference() {
	return this;
}


const grcTexture* grcTexture::GetReference() const {
	return this;
}

void grcTexture::InitFastMipMapper() 
{
	if (!sm_FastMipMapper.IsInitialized())
	{
		sm_FastMipMapper.Init();
	}
}

void grcTexture::Place(grcTexture* that,datResource &rsc) {
	Assert(that->GetResourceType() == grcTexture::NORMAL);
#if __WIN32PC && __D3D9
	::new (that) grcTextureDX9(rsc);
#elif RSG_PC && __D3D11
	::new (that) grcTextureDX11(rsc);
#elif RSG_DURANGO
	::new (that) grcTextureDurango(rsc);
#else
	::new (that) grcTextureDefault(rsc);
#endif
}

#if __DECLARESTRUCT
void grcTexture::DeclareStruct(datTypeStruct &s) {
	pgBase::DeclareStruct(s);
	STRUCT_BEGIN(grcTexture);
#if __RESOURCECOMPILER
	SetHandleIndex(0);			// Make sure the handle is zero so we know to reallocate when streamed in.
	STRUCT_FIELD(m_Texture);
#endif
	STRUCT_FIELD(m_Name);
	STRUCT_FIELD(m_RefCount);
	STRUCT_FIELD(m_ResourceTypeAndConversionFlags);
	STRUCT_FIELD(m_LayerCount);
	STRUCT_PADDING64(m_pad64);
	STRUCT_FIELD_VP(m_CachedTexturePtr);
	STRUCT_FIELD(m_PhysicalSizeAndTemplateType);
	STRUCT_FIELD(m_HandleIndex);
	STRUCT_END();
}
#endif

grcTexture::ChannelBits grcTexture::FindUsedChannelsFromRageFormat(u32 rageFormat)
{
	ChannelBits bits(false);

	switch(rageFormat)
	{
		// RGB
	case grctfR5G6B5:
#if __D3D11
	case grctfR11G11B10F:
#endif
#if __WIN32PC
	case grctfX8R8G8B8:
#endif
		bits.Set(CHANNEL_RED);
		bits.Set(CHANNEL_GREEN);
		bits.Set(CHANNEL_BLUE);
		break;

		// RGBA
	case grctfA8R8G8B8:
	case grctfA16B16G16R16F:
	case grctfA32B32G32R32F:
	case grctfA1R5G5B5:
#if !__PS3
	case grctfA2B10G10R10:
	case grctfA2B10G10R10ATI:
	case grctfA16B16G16R16F_NoExpand:
	case grctfA16B16G16R16:
	case grctfA4R4G4B4:
#endif
		// Luma
	case grctfL8:
		bits.Set(CHANNEL_RED);
		bits.Set(CHANNEL_GREEN);
		bits.Set(CHANNEL_BLUE);
		bits.Set(CHANNEL_ALPHA);
		break;

		// G + R
	case grctfG16R16:
	case grctfG16R16F:
#if __XENON
	case grctfG32R32F:
#endif
		bits.Set(CHANNEL_GREEN);
		bits.Set(CHANNEL_RED);
		break;

		// R
#if !__PS3
	case grctfR16F:
#endif
	case grctfR32F:
		bits.Set(CHANNEL_RED);
		break;

		// Depth
#if !__XENON
#if __PS3
	case grctfD16:
#elif __WIN32PC || RSG_DURANGO
	case grctfD32F:
#endif
		bits.Set(CHANNEL_DEPTH);
		break;
#endif // !__XENON

		// Depth/Stencil
	case grctfD24S8:
#if __WIN32PC
	case grctfX24G8:
	case grctfD32FS8:
	case grctfX32S8:
#elif __XENON
	case grctfD24FS8_ReadStencil:
#endif
		bits.Set(CHANNEL_DEPTH);
		bits.Set(CHANNEL_STENCIL);
		break;

		// None
	case grctfNone: 
	default:
		break;

	}
	return bits;
}


grcTextureFactory::CreateParams::CreateParams(u16 poolID)
	: UseFloat( false )
	, Multisample( 0 )
#if !DEVICE_EQAA
	, MultisampleQuality( 0 )
#endif
	, IsResolvable( true )
	, MipLevels( 1 )
	, IsRenderable( true )
	, UseHierZ( true )
	, HiZBase( 0 )
	, HasParent( false )
	, Parent( NULL )
	, Lockable( false )
	, ColorExpBias( 0 )
	, IsSRGB(false)
	, Format( grctfNone )
	, InTiledMemory(!__PS3)
#if __PS3
	, UseCustomZCullAllocation(false)
#endif
#if __PPU || __WIN32PC || RSG_DURANGO || RSG_ORBIS
	, CreateAABuffer(true)
	, InLocalMemory(true)
	, IsSwizzled(false)
	, EnableCompression(true)
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	, StereoRTMode(grcDevice::AUTO)
	, ArraySize(1)
	, PerArraySliceRTV(0)
#endif
#if RSG_DURANGO
	, ESRAMPhase(0)
	, ESRAMMaxSize(0)
#endif
#if RSG_ORBIS
	, ForceNoTiling(false)
	, EnableFastClear(false)	//most of the targets are created for the full-screen draws, with no clears involved
	, EnableStencil(true)
#endif // RSG_ORBIS
#if DEVICE_MSAA
	, SupersampleFrequency(0)
#endif
#if DEVICE_EQAA
	, EnableHighQualityResolve(false)
	, ForceHardwareResolve(false)
	, ForceFragmentMask(false)
	, EnableNanDetect(false)
#endif // DEVICE_EQAA
	, UseAsUAV(false)
#endif // __PPU || __WIN32PC
	, PoolID(poolID)
	, PoolHeap(0)				
	, PoolOffset(0)
	, AllocateFromPoolOnCreate(false)  // NOTE: for compatibility this will be changed to true for anyone using the old Activate/deactivate render target pool system
	, basePtr(0)
	, mipPtr(0)
{
}

bool grcTextureFactory::CanGetBackBufferDepth()
{
	return true;
}

bool grcTextureFactory::CanGetBackBuffer()
{
	return true;
}

const grcRenderTarget* grcTextureFactory::GetBackBufferDepthForceRealize()
{
	return NULL;
}

// helper function to make a pool for a rendertarget that requested we make one for them..
u16 grcTextureFactory::CreateAutoRTPool(const char *name, const CreateParams &params, u32 size)
{
	char autoPoolName[1024];
	formatf(autoPoolName,sizeof(autoPoolName),"AutoPool: %s",name);

	grcRTPoolCreateParams poolParams;
#if __GCM
	poolParams.m_Alignment = gcm::GetSurfaceAlignment(params.InTiledMemory, params.UseHierZ);	
	poolParams.m_Tiled = params.InTiledMemory;
	poolParams.m_Zculled = params.UseHierZ;
	poolParams.m_PhysicalMem = params.InLocalMemory;
	poolParams.m_MSAA = params.Multisample;
	poolParams.m_Compression = params.EnableCompression?0xff:0x00;
#endif

	poolParams.m_Size = Max((u32)grcRenderTargetPoolEntry::kDefaultAlignment,size);
	poolParams.m_InitOnCreate = false;
	poolParams.m_AllocOnCreate = params.AllocateFromPoolOnCreate;
	return grcRenderTargetPoolMgr::CreatePool(autoPoolName,poolParams);
}


grcTexture::~grcTexture() 
{
#if ENABLE_DEFRAGMENTATION
#if GRCTEXTUREHANDLE_IS_PGREF
	pgHandleBase::Unregister(this);
#else
	grcTextureHandle::Unregister(this);
#endif
#endif
	StringFree(m_Name);
}

grcRenderTarget::~grcRenderTarget() 
{

}

#if __BANK

void grcRenderTarget::BeginFrame()
{
	if (sm_EnableLoggingNextFrame)
	{
		sm_EnableLoggingNextFrame = false;
		sm_LogStream = fiStream::Create(sm_LogName);
		sm_StartTicks = sysTimer::GetTicks();
		
		if (sm_LogStream)
		{
			// print csv header
			char buffer[1024];
			formatf(buffer, sizeof(buffer),"%-10s,%-18s,%-40s,%-10s,%-7s,%-4s,%-4s,%-80s,%-9s,%-10s\n", "Time", "Operation", "Target Name", "Address", "Size", "W", "H","Pool Name", "Pool Heap", "Pool Offset");
			sm_LogStream->Write(buffer, (int)strlen(buffer));
		}
	}
}

void grcRenderTarget::EndFrame()
{
	if (sm_LogStream)
	{
		sm_LogStream->Close();
		sm_LogStream = NULL;
	}
}

void grcRenderTarget::LogTexture(const char* operation, const grcRenderTarget* tex)
{
	if(sm_LogStream && tex)
	{
		utimer_t ticks = sysTimer::GetTicks() - grcRenderTarget::sm_StartTicks;

		const char * texName = tex->GetName()?tex->GetName():"(unknown)";
		const char * poolName = "(none)";
		void * ptr = NULL;
		ptrdiff_t offset = 0;
		int heap = 0;

#if __GCM
		// GCM builds deal with offsets, always.
		ptr = (void*)((grcRenderTargetGCM*)tex)->GetMemoryOffset();
#else
		grcTextureLock texLock;
		if( tex->GetTexturePtr() && tex->LockRect(0, 0, texLock) )
		{
			ptr = texLock.Base;
			tex->UnlockRect(texLock);
		}
#endif

		if(tex->m_PoolID != kRTPoolIDInvalid)
		{
			void * poolMemBase = grcRenderTargetPoolMgr::GetPoolMemory(tex->m_PoolID);
#if __GCM
			// convert to offset so we can subtract from the ptr;
			poolMemBase = (void*)((grcRenderTargetPoolMgr::GetIsPhysical(tex->m_PoolID)) ? gcm::LocalOffset(poolMemBase) : gcm::MainOffset(poolMemBase));
#endif
			offset = (size_t)ptr - (size_t)poolMemBase;
			poolName = grcRenderTargetPoolMgr::GetName(tex->m_PoolID);
		}

		char buffer[1024];
		formatf(buffer, sizeof(buffer), "0x%08x,%-18s,%-40s,0x%08x,%-7d,%-4d,%-4d,%-80s,%-9d,0x%08x\n",	(u32)ticks, operation, texName, ptr,tex->GetPhysicalSize(), 
																										tex->GetWidth(),tex->GetHeight(), poolName, heap, offset);
		sm_LogStream->Write(buffer, (int)strlen(buffer));
	}
}

void grcRenderTarget::RequestSaveLog()
{
	sm_EnableLoggingNextFrame = true;
}

void grcRenderTarget::AddLoggingWidgets(bkBank& bk)
{
	const char *logFile = "c:\\rtlog.csv";
	PARAM_rendertargetlog.Get(logFile);
	formatf(sm_LogName,sizeof(sm_LogName),logFile);
	
	bk.PushGroup("RenderTarget Logging");
	bk.AddText("LogName",sm_LogName,sizeof(sm_LogName));
	bk.AddButton("Save Log", CFA(grcRenderTarget::RequestSaveLog));
	bk.PopGroup();

	// Not yet implement
	//grcRenderTargetPoolMgr::AddWidgets(bk);
}
#endif


void grcFastMipMapper::Init()
{
	m_Shader = grcEffect::Create("rage_fastmipmap");

	m_DepthTechnique = m_Shader->LookupTechnique("drawDepth");	
	m_ColorTechnique = m_Shader->LookupTechnique("draw");
#if __PPU
	m_ColorCutOutTechnique = m_Shader->LookupTechnique("drawCutOut");
#endif // __PPU
	m_BlurTechnique = m_Shader->LookupTechnique("drawBlur");

	// depth texture
	m_BaseTextureID = m_Shader->LookupVar( "BaseTex" );
	m_TexelSizeID = m_Shader->LookupVar( "TexelSize" );

	grcSamplerStateHandle h = m_Shader->GetSamplerState(m_BaseTextureID);
	grcSamplerStateDesc desc;
	grcStateBlock::GetSamplerStateDesc(h, desc);
	for (int level=0; level<m_SamplerStates.GetMaxCount(); level++) {
		desc.MinLod = desc.MaxLod = float(level);
		m_SamplerStates[level] = grcStateBlock::CreateSamplerState(desc);
	}

	grcBlendStateDesc blendDesc;
	blendDesc.IndependentBlendEnable = true;

	for (int i=0; i<MAX_RAGE_RENDERTARGET_COUNT; i++)
		blendDesc.BlendRTDesc[i].RenderTargetWriteMask = grcRSV::COLORWRITEENABLE_NONE;

	for (int i=0; i<MAX_RAGE_RENDERTARGET_COUNT; i++)
	{
		blendDesc.BlendRTDesc[i].RenderTargetWriteMask = grcRSV::COLORWRITEENABLE_ALL;
		m_BlendStates[i] = grcStateBlock::CreateBlendState(blendDesc);
		blendDesc.BlendRTDesc[i].RenderTargetWriteMask = grcRSV::COLORWRITEENABLE_NONE;
	}

	m_IsInitialized = true;
}

grcFastMipMapper::~grcFastMipMapper()
{
	delete m_Shader;
}

void grcFastMipMapper::DownSample(const grcRenderTarget* baseRenderTarget, int index, int level, const Vector2& sourceOffset, const Vector2& sourceSize, grcRenderTargetType type)
{
	Assert(baseRenderTarget->GetMipMapCount() == 1 || level > 0);
	const Color32 color(1.0f, 1.0f, 1.0f, 1.0f);

	m_Shader->SetVar( m_BaseTextureID, baseRenderTarget); 
	u32 width = Max<u32>(baseRenderTarget->GetWidth() >> level, 1u);
	u32 height = Max<u32>(baseRenderTarget->GetHeight() >> level, 1u);
	m_Shader->SetVar(m_TexelSizeID, Vector4(1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height), static_cast<float>(width), static_cast<float>(height)));

	if (level > 0)
		m_Shader->PushSamplerState(m_BaseTextureID, m_SamplerStates[level-1]);

	// only write to the specified render target
	grcStateBlock::SetBlendState(m_BlendStates[index]);

	int success = 0;
#if __PPU
	// Hack!!! 1 bit alpha on the PS3 is a big pain in the arse. This assumes that pixels we don't touch
	// should have zero alpha
	if (baseRenderTarget->GetSurfaceFormat() == CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5)
	{
		GRCDEVICE.Clear(true, Color32(0, 0, 0, 0), false, 1.0f, 0);
		Assert(type != grcrtDepthBuffer && type != grcrtShadowMap);
		success = m_Shader->BeginDraw(m_ColorCutOutTechnique, false);
	}
	else
#endif // __PPU
	{
		if (type == grcrtDepthBuffer || type == grcrtShadowMap)
		{
			success = m_Shader->BeginDraw(m_DepthTechnique, false);
		}
		else
		{
			success = m_Shader->BeginDraw(m_ColorTechnique, false);
		}
	}
	if ( success  > 0)
	{
		m_Shader->BeginPass(0);
		
#if __XENON
		GRCDEVICE.BlitRectfNoSetup(-0.5f,			// x1 - Destination base x
			-0.5f,			// y1 - Destination base y
			width - 0.5f,			// x2 - Destination opposite-corner x
			height - 0.5f,			// y1 - Destination opposite-corner y
			0.1f,			// z - Destination z value.  Note that the z value is expected to be in 0..1 space
			sourceOffset.x,			// u1 - Source texture base u (in normalized texels)
			sourceOffset.y,			// v1 - Source texture base v (in normalized texels)
			sourceOffset.x + sourceSize.x,			// u2 - Source texture opposite-corner u (in normalized texels)
			sourceOffset.y + sourceSize.y,			// v2 - Source texture opposite-corner v (in normalized texels)
			color		// color - reference to Packed color value
		);
#else
		grcDrawSingleQuadf(-1.0f,1.0f,1.0f,-1.0f,0.0f,sourceOffset.x,sourceOffset.y,sourceOffset.x+sourceSize.x,sourceOffset.y+sourceSize.y,color);
#endif // __XENON

		m_Shader->EndPass();
	}
	m_Shader->EndDraw();

	m_Shader->SetVar(m_BaseTextureID, const_cast<grcTexture*>(grcTexture::None));

	if (level > 0)
		m_Shader->PopSamplerState(m_BaseTextureID);
}

void grcFastMipMapper::Blur(const grcRenderTarget* baseRenderTarget, float kernelSize, const Vector2& sourceOffset, const Vector2& sourceSize)
{
	kernelSize = Clamp( kernelSize, 0.0f, 4.0f );	// clamp to range
	Color32 color(1.0f,1.0f,1.0f,1.0f);

	m_Shader->SetVar( m_BaseTextureID,  baseRenderTarget); 
	m_Shader->SetVar( m_TexelSizeID, Vector4((kernelSize * sourceSize.x / baseRenderTarget->GetWidth()), 
											(kernelSize * sourceSize.y / baseRenderTarget->GetHeight()), 0.0f, 0.0f ));

	

	m_Shader->BeginDraw( m_BlurTechnique , false );
	m_Shader->BeginPass(0);

#if __XENON
	float width = static_cast<float>(baseRenderTarget->GetWidth());
	float height = static_cast<float>(baseRenderTarget->GetHeight());

	// blit
	GRCDEVICE.BlitRectfNoSetup(-0.5f,			// x1 - Destination base x
		-0.5f,					// y1 - Destination base y
		width - 0.5f,			// x2 - Destination opposite-corner x
		height - 0.5f,			// y1 - Destination opposite-corner y

		// rendering into the Z buffer might need 0.0f here
		0.0f,					// z - Destination z value.  Note that the z value is expected to be in 0..1 space
		sourceOffset.x,			// u1 - Source texture base u (in normalized texels)
		sourceOffset.y,			// v1 - Source texture base v (in normalized texels)
		sourceOffset.x + sourceSize.x,			// u2 - Source texture opposite-corner u (in normalized texels)
		sourceOffset.y + sourceSize.y,			// v2 - Source texture opposite-corner v (in normalized texels)
		color		// color - reference to Packed color value
	);

#else
	grcDrawSingleQuadf(-1.0f,1.0f,1.0f,-1.0f,0.0f,sourceOffset.x,sourceOffset.y,sourceOffset.x+sourceSize.x,sourceOffset.y+sourceSize.y,color);
#endif // __XENON

	m_Shader->EndPass();
	m_Shader->EndDraw();

	m_Shader->SetVar(m_BaseTextureID, const_cast<grcTexture*>(grcTexture::None));
}

void grcCompositeRenderTargetHelper::InitClass()
{

	grcDepthStencilStateDesc dssDesc;
	dssDesc.DepthFunc = grcRSV::CMP_ALWAYS;
}
grcCompositeRenderTargetHelper::CompositeParams::CompositeParams()
: srcColor(NULL)
, srcDepth(NULL)
, dstColor(NULL)
, dstDepth(NULL)
, compositeEffect(NULL)
, compositeTechnique(grcetNONE)
, compositeSrcColorMap(grcevNONE)
, compositeSrcDepthMap(grcevNONE)
, compositeDstDepthMap(grcevNONE)
, lockTarget(true)
, unlockTarget(true)
, resolveFlags(NULL)
, compositeRTblendType(COMPOSITE_RT_BLEND_COMPOSITE_ALPHA)
, depth(0.0f)
{

	grcEffect& defaultEffect = GRCDEVICE.GetDefaultEffect();
	compositeEffect = &defaultEffect;
	compositeTechnique = compositeEffect->LookupTechnique("CopyTransparent");	
	compositeSrcColorMap = compositeEffect->LookupVar("TransparentSrcMap");
	compositeSrcDepthMap = compositeEffect->LookupVar("LowResDepthPointMap");
	compositeDstDepthMap = compositeEffect->LookupVar("DepthPointMap");

}

grcCompositeRenderTargetHelper::BlitParams::BlitParams()
	: srcColor(NULL)
	, dstColor(NULL)
	, blitEffect(NULL)
	, blitTechnique(grcetNONE)
	, blitSrcMapVar(grcevNONE)
	, lockTarget(true)
	, unlockTarget(true)
	, resolveFlags(NULL)
{

	grcEffect& defaultEffect = GRCDEVICE.GetDefaultEffect();
	blitEffect = &defaultEffect;
	blitTechnique = blitEffect->LookupTechnique("Copy");
	blitSrcMapVar = blitEffect->LookupVar("DiffuseTex");

}

grcCompositeRenderTargetHelper::DownsampleParams::DownsampleParams()
	: srcDepth(NULL)
	, dstDepth(NULL)
	PS3_ONLY(, srcPatchedDepth(NULL))
	, depthDownsampleEffect(NULL)
	, depthDownsampleTechnique(grcetNONE)
	, depthDownsampleSrcMapVar(grcevNONE)
{
	grcEffect& defaultEffect = GRCDEVICE.GetDefaultEffect();
	depthDownsampleEffect = &defaultEffect;
	depthDownsampleTechnique = depthDownsampleEffect->LookupTechnique("DownSampleMinDepth");
	depthDownsampleSrcMapVar = depthDownsampleEffect->LookupVar("DepthPointMap");
}

void grcCompositeRenderTargetHelper::CompositeDstToSrc(const CompositeParams& params)
{
	Assert(params.srcColor);
	Assert(params.dstColor);

	if(params.lockTarget)
	{
		grcTextureFactory& textureFactory = grcTextureFactory::GetInstance();
		textureFactory.LockRenderTarget(0, params.dstColor, params.dstDepth);
	}

	params.compositeEffect->SetVar(params.compositeSrcColorMap, params.srcColor);
#if __PS3
	params.compositeEffect->SetVar(params.compositeSrcDepthMap, params.srcPatchedDepth);
	params.compositeEffect->SetVar(params.compositeDstDepthMap, params.dstPatchedDepth);
#else
	params.compositeEffect->SetVar(params.compositeSrcDepthMap, params.srcDepth);
	params.compositeEffect->SetVar(params.compositeDstDepthMap, params.dstDepth);
#endif

	params.compositeEffect->Bind(params.compositeTechnique);

	grcStateBlock::SetRasterizerState(grcStateBlock::RS_NoBackfaceCull);

	if(params.compositeRTblendType == COMPOSITE_RT_BLEND_ADD)
		grcStateBlock::SetBlendState(grcStateBlock::BS_Add);
	else
		grcStateBlock::SetBlendState(grcStateBlock::BS_CompositeAlpha);

	grcCompositeRenderTargetHelper::DrawFullScreenTri(params.depth);


	params.compositeEffect->UnBind();

	// release references to textures
	params.compositeEffect->SetVar(params.compositeSrcColorMap, (grcTexture*)NULL);
	grcResolveFlags localFlags;
#if __XENON
	if(params.resolveFlags)
	{
		localFlags = *(params.resolveFlags);
	}
	localFlags.ColorExpBias = -((grcRenderTargetXenon*)params.dstColor)->GetColorExpBias();
#endif

	if(params.unlockTarget)
	{
		grcTextureFactory::GetInstance().UnlockRenderTarget(0,&localFlags);
	}

}

void grcCompositeRenderTargetHelper::BlitDstToSrc(const BlitParams& params)
{
	//Printf("grcCompositeRenderTarget::BlitDst %s->%s\n", m_DstColorRenderTarget->GetName(), target->GetName());

	Assert(params.srcColor);
	Assert(params.dstColor);
	grcTextureFactory& textureFactory = grcTextureFactory::GetInstance();

	textureFactory.LockRenderTarget(0, params.dstColor, NULL);

	params.blitEffect->SetVar(params.blitSrcMapVar, params.srcColor);
	params.blitEffect->Bind(params.blitTechnique);

	grcCompositeRenderTargetHelper::DrawFullScreenTri();

	params.blitEffect->UnBind();

	params.blitEffect->SetVar(params.blitSrcMapVar, (grcTexture*)NULL);

	grcResolveFlags localFlags;
#if __XENON
	if(params.resolveFlags)
	{
		localFlags = *(params.resolveFlags);
	}
	localFlags.ColorExpBias = -((grcRenderTargetXenon*)params.dstColor)->GetColorExpBias();
#endif

	textureFactory.UnlockRenderTarget(0,&localFlags);
}

void grcCompositeRenderTargetHelper::DownsampleDepth(const DownsampleParams &params)
{
	PIXBegin(0, "Downsample Depth");
	GRCDBG_PUSH("Downsample Depth");

	GRC_ALLOC_SCOPE_AUTO_PUSH_POP()

	Assert(params.srcDepth && params.dstDepth);
#if HACK_GTA4
	PPU_ONLY(Assert(params.srcPatchedDepth));
#endif // HACK_GTA4


	if ((params.srcDepth->GetWidth() !=  params.dstDepth->GetWidth()
		|| params.srcDepth->GetHeight() != params.dstDepth->GetHeight()
		|| params.srcDepth->GetMSAA() != params.dstDepth->GetMSAA()))
	{
		//Printf("grcCompositeRenderTarget::DownsampleDepth %s->%s\n", m_SrcDepthRenderTarget->GetName(), m_DstDepthRenderTarget->GetName());

		grcRasterizerStateHandle rs_prev = grcStateBlock::RS_Active;
		grcDepthStencilStateHandle ds_prev = grcStateBlock::DSS_Active;
		grcTextureFactory& textureFactory = grcTextureFactory::GetInstance();
		textureFactory.LockRenderTarget(0, NULL, params.dstDepth);

#if __XENON
		GRCDEVICE.GetCurrent()->SetRenderState( D3DRS_HIZWRITEENABLE, TRUE );
#endif

		GRCDEVICE.Clear(false, Color32(0, 0, 0, 0), true, 1.0f, true, 0);
		grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_LessEqual);
		grcStateBlock::SetRasterizerState(grcStateBlock::RS_NoBackfaceCull);

#if HACK_GTA4
#if __PPU
		params.depthDownsampleEffect->SetVar(params.depthDownsampleSrcMapVar, params.srcPatchedDepth);
#else // __PPU
		params.depthDownsampleEffect->SetVar(params.depthDownsampleSrcMapVar, params.srcDepth);
#endif // __PPU
#else // HACK_GTA4
		PPU_ONLY(u8 depthTextureFormat = GRCDEVICE.PatchShadowToDepthBuffer(params.srcDepth , false));

		params.depthDownsampleEffect->SetVar(params.depthDownsampleSrcMapVar, params.srcDepth);
#endif // HACK_GTA4
		params.depthDownsampleEffect->Bind(params.depthDownsampleTechnique);

		grcCompositeRenderTargetHelper::DrawFullScreenTri();

		params.depthDownsampleEffect->UnBind();
		params.depthDownsampleEffect->SetVar(params.depthDownsampleSrcMapVar, (grcTexture*)NULL);

#if !HACK_GTA4
		PPU_ONLY(GRCDEVICE.PatchDepthToShadowBuffer(params.srcDepth, depthTextureFormat, false));
#endif // !HACK_GTA4

		grcStateBlock::SetDepthStencilState(ds_prev);
		grcStateBlock::SetRasterizerState(rs_prev);

		textureFactory.UnlockRenderTarget(0);
	}

	GRCDBG_POP();
	PIXEnd();
}

void grcCompositeRenderTargetHelper::DrawFullScreenTri(float fDepth)
{
	// 360 requires an offset to align pixels and texels, PS3 and DX10+ do not.
	const bool bUseOffset = false; //__XENON ? true : false;  // Disabling this since the up/down-sampler shader is expecting mis-aligned texel coords
	float fHalfTexelOffsetX = 0.f;
	float fHalfTexelOffsetY = 0.f;

	if ( bUseOffset )
	{
		fHalfTexelOffsetX = 0.5f/GRCDEVICE.GetWidth();
		fHalfTexelOffsetY = 0.5f/GRCDEVICE.GetHeight();
	}

	// use a tri as per docs to avoid edges on screen
	grcBegin(drawTris,3);
	grcVertex(-1.0f,  1.0f, fDepth, 0.0f,0.0f,0.0f, Color32(255,255,255,255), 0.0f+fHalfTexelOffsetX, 0.0f+fHalfTexelOffsetY);
	grcVertex( 3.0f,  1.0f, fDepth, 0.0f,0.0f,0.0f, Color32(255,255,255,255), 2.0f+fHalfTexelOffsetX, 0.0f+fHalfTexelOffsetY);
	grcVertex(-1.0f, -3.0f, fDepth, 0.0f,0.0f,0.0f, Color32(255,255,255,255), 0.0f+fHalfTexelOffsetX, 2.0f+fHalfTexelOffsetY);
	grcEnd();
}

//
//  Rendertarget Pool Manager platform independent code.
//

atPool<grcRenderTargetPoolEntry * > grcRenderTargetPoolMgr::sm_PoolEntries;

void grcRenderTargetPoolMgr::Init(int maxPools/*=64*/)
{
	sm_PoolEntries.Init((u16)maxPools);
}

void grcRenderTargetPoolMgr::Shutdown()
{
	sm_PoolEntries.Reset(); // we should really check for non zero ref counts here...
}

u16 grcRenderTargetPoolMgr::CreatePool(const char* PS3_ONLY(name) XENON_ONLY(name), const grcRTPoolCreateParams & PS3_ONLY(params) XENON_ONLY(params), void* PS3_ONLY(buffer) XENON_ONLY(buffer))
{
	u16 ret = kRTPoolIDInvalid;
#if RSG_PS3 || RSG_XENON	// just always fail for __WIN32PC builds, they don't work with pools (yet?)
	if (Verifyf(sm_PoolEntries.GetSize() > sm_PoolEntries.GetCount()+1, "Ran out of grcRenderTargetPoolEntries. call grcRenderTargetPool::Init() with a larger number"))
	{
		if (grcRenderTargetPoolEntry* ptr = rage_new grcRenderTargetPoolEntry(name, params, buffer))
		{
			grcRenderTargetPoolEntry** poolEntry = sm_PoolEntries.New();
			ret = (u16)sm_PoolEntries.GetIndex(poolEntry);
			
			*poolEntry = ptr;
			
			if (params.m_InitOnCreate)			// if we are to init on create (should always be when using the new system system), let the platform specific function deal with that. 
				ptr->InitializeMemory(params);
		}
	}
#endif
	return ret;
}

void grcRenderTargetPoolMgr::DestroyPool(u16 poolID)
{
	if (VerifyPoolID(poolID))
	{
		grcRenderTargetPoolEntry** poolEntry = sm_PoolEntries.GetElemByIndex(poolID);
		*poolEntry = NULL;
		sm_PoolEntries.Delete(poolEntry);
	}
}



void * grcRenderTargetPoolMgr::AllocateTextureMem(u16 poolID, u8 heapID, int size, int offset, int alignment /* = grcRenderTargetPoolEntry::kDefaultAlignment*/)
{
	if (VerifyPoolID(poolID))
	{
		return (*sm_PoolEntries.GetElemByIndex(poolID))->AllocateTextureMem(heapID, size, offset, alignment );
	}
	else
	{
		return NULL;
	}
}

void grcRenderTargetPoolMgr::FreeTextureMem(u16 poolID, u8 heapID, void *ptr)
{
	if (VerifyPoolID(poolID))
	{
		(*sm_PoolEntries.GetElemByIndex(poolID))->FreeTextureMem(ptr, heapID);
	}
}


int grcRenderTargetPoolMgr::GetFreeMem(u16 poolID, u8 heapID)
{
	if (VerifyPoolAndHeapIDs(poolID, heapID))
		return (int)(*sm_PoolEntries.GetElemByIndex(poolID))->m_PoolHeaps[heapID]->GetMemoryFree();
	else
		return 0;
}

int grcRenderTargetPoolMgr::GetUsedMem(u16 poolID, u8 heapID)
{
	if (VerifyPoolAndHeapIDs(poolID, heapID))
		return (int)(*sm_PoolEntries.GetElemByIndex(poolID))->m_PoolHeaps[heapID]->GetMemoryUsed(-1);
	else
		return 0;
}

int grcRenderTargetPoolMgr::GetLargestFreeBlock(u16 poolID, u8 heapID)
{
	if (VerifyPoolAndHeapIDs(poolID, heapID))
		return (int)(*sm_PoolEntries.GetElemByIndex(poolID))->m_PoolHeaps[heapID]->GetLargestAvailableBlock();
	else
		return 0;
}




#if __BANK
void grcRenderTargetPoolMgr::AddWidgets(bkBank& bk)
{
	const char *logFile = "c:\\rtpoollog.csv";
	PARAM_rendertargetpoollog.Get(logFile);
	formatf(sm_LogName,sizeof(sm_LogName),logFile);

	bk.PushGroup("RenderTargetPool Logging");
	bk.AddText("LogName",sm_LogName,sizeof(sm_LogName));
	bk.AddButton("Save Log", CFA(grcRenderTarget::RequestSaveLog));
	// should add a widget to change the log name
	bk.PopGroup();
}

void grcRenderTargetPoolMgr::RequestSaveLog()
{
	sm_EnableLoggingNextFrame = true;
}

#endif


grcRenderTargetPoolEntry::grcRenderTargetPoolEntry(const char *name, const grcRTPoolCreateParams & params, void * buffer /*=NULL*/)
{
	m_IsInitialised = false;
	m_Name = __FINAL?NULL:StringDuplicate(name);
#if __GCM
	m_Pitch = params.m_Pitch;
	m_BitDepth = (u8)params.m_BitDepth;
	m_MSAA = (u8)params.m_MSAA;

	m_IsPhysicalMem = params.m_PhysicalMem;
	m_IsInTiledMem = params.m_Tiled; 

	m_TiledIndex = 0;
	m_Compression = (m_IsInTiledMem && m_IsPhysicalMem) ? params.m_Compression : 0;
#endif

	m_LowestMemFree = params.m_Size;

	if( params.m_AllocOnCreate )
	{
#if __GCM
		AllocatePoolMemory( params.m_Size, params.m_PhysicalMem, params.m_Alignment, buffer);
#else
		AllocatePoolMemory( params.m_Size, false, 0, buffer);
#endif
		Assert(m_PoolMemory);
	}
	else
	{
		m_AllocatedMemory = false;
		m_PoolMemory = NULL;
		m_Size = params.m_Size;
	}

	// allocate heaps for tracking allocations. for new style usage this will just be 1, for old it will match the old heap count
	m_PoolHeaps.Reserve(params.m_HeapCount);
	m_PoolHeapWorkspace.Reserve(params.m_HeapCount);

	for (int i=0;i< params.m_HeapCount; i++)
	{
		u8 * workspace = rage_new u8[COMPUTE_WORKSPACE_SIZE(params.m_MaxTargets)]; 
		m_PoolHeapWorkspace.Append() = workspace;
		m_PoolHeaps.Append() = rage_new sysExternalHeap;
		m_PoolHeaps[i]->Init(params.m_Size, params.m_MaxTargets, workspace, 0);
	}
}


grcRenderTargetPoolEntry::~grcRenderTargetPoolEntry()
{
	for (int i=0;i< m_PoolHeaps.GetCount(); i++)
	{
		delete m_PoolHeaps[i];
		delete m_PoolHeapWorkspace[i];
	}

	m_PoolHeaps.Reset();
	m_PoolHeapWorkspace.Reset();

	FreePoolMemory(); // the actual memory freeing is platform specific

	m_PoolMemory = NULL;
}

void* grcRenderTargetPoolEntry::AllocateTextureMem(u8 heapID, int size, int forcedOffset, int alignment /* = kDefaultAlignment*/)
{
	void * ret = NULL;
	if (heapID==kRTPoolHeapInvalid)
	{
		// they want a specific offset in the pool memory.
		Assertf((forcedOffset%alignment) == 0, "A RenderTarget is requesting an offset (%d) that is not compatibly with the alignment needed (%d for Pool \"%s\")", forcedOffset, alignment, m_Name);
		// the assert below is not quite valid when we use offset that at partial offset into the buffer that has putch greater than our texture width
		// example: a non tiled pool has a pitch of 1024x256 tall, but we have a 16 bits texture that is only 128x256, we could start this texture 256 bytes over from he pitch aligned edge, but still completely fit in the pool.
		// for now disable it for non tiled pools. in the future, we need to do a proper check here.
		if (Verifyf(alignment == 128 || forcedOffset+size <= m_Size, "A RenderTarget is requesting an offset (%d) and size (%d) that will cause the target to go past the end of the pool's (\"%s\") memory", forcedOffset, size, m_Name))
		{
			ret = (void*)((u8*)m_PoolMemory + forcedOffset);
		}
	}
	else if (Verifyf(heapID < m_PoolHeaps.GetCount(), "Invalid heapID (%d) used for Render target pool \"%s\", range is (0..%d)",heapID,m_Name,m_PoolHeaps.GetCount()))
	{
		sysExternalHeap * heap = m_PoolHeaps[heapID];

#if __PS3
		// the external heap only support powers of 2 allocations, so we need to do it ourselves here.
		// this is simplifed since alignment within a pool that is tiled is always 8*pitch for all targets in it.

		ASSERT_ONLY(int oldSize = size);
		int oldAligment = alignment;
	
		if (m_IsInTiledMem)
		{
			size = ((size+alignment-1)/alignment)*alignment;
			alignment = 128;
		}
#endif

		size_t offset = heap->Allocate(size,alignment);
		if (offset != ~0)
		{
#if __PS3
			if (m_IsInTiledMem)// adjust alignment manually 
			{
				int alignedOffset = ((offset+oldAligment-1)/oldAligment)*oldAligment;
				Assert (alignedOffset + oldSize <= offset + size);
				offset = alignedOffset;
			}
#endif
			ret = (void*)((u8*)m_PoolMemory + offset);
			int freeMem = (int)heap->GetMemoryFree();
			if (freeMem< m_LowestMemFree)
				m_LowestMemFree = freeMem;
		}
		else
		{
			Errorf("Failed to allocate memory from rendertarget pool \"%s\", %d bytes requested, %" SIZETFMT "d bytes available (% " SIZETFMT "d byte in largest free block)",m_Name,size,heap->GetMemoryFree(),heap->GetLargestAvailableBlock());
		}
	
	}

	return ret;
}

void grcRenderTargetPoolEntry::FreeTextureMem(void * ptr, u8 heapID)
{
	// if it's not allocated from a heap, don't free it (was a direct offset)
	if (heapID != kRTPoolHeapInvalid && Verifyf(heapID < m_PoolHeaps.GetCount(), "Invalid heapID (%d) used for Render target pool \"%s\", range is (0..%d)",heapID,m_Name,m_PoolHeaps.GetCount()))
	{
		size_t offset = (size_t)((u8*)ptr - (u8*)m_PoolMemory);
		sysExternalHeap * heap = m_PoolHeaps[heapID];
		Verifyf( heap->Free(offset), "Tried to free rendertarget memory (0x%p) from pool \"%s\", heap %d, which was not allocated from that pool/heap",ptr,m_Name,heapID);
	}
}



// OLD grcRenderTargetMemPool system legacy support, makes updateing code easier, but this should go away eventually.

grcRenderTargetMemPool* grcRenderTargetMemPool::sm_ActivePool = NULL;


grcRenderTargetMemPool::grcRenderTargetMemPool(const char* name, int size, u8 heapCount, int GCM_ONLY(alignment) /*= kDefaultAlignment*/, bool GCM_ONLY(isPhysical) /*= true*/, u32 GCM_ONLY(pitch) /*= 0*/, u8 GCM_ONLY(compression) /*= 0*/, void* poolMem /*= NULL*/)
{
	sm_MemPools.Grow() = const_cast<grcRenderTargetMemPool*>(this);

	// create a pool with the new system that approximates the old style
	grcRTPoolCreateParams poolParams;
	poolParams.m_Size = size;	// is this valid yet? we don't really have the pitch, etc.

	Assert(heapCount<=255); // must fit in a u8
	poolParams.m_HeapCount = heapCount;
	
	// these rest of the parameters are not available until the first render target is allocated from this pool so flag this as not initialized on create.
	poolParams.m_InitOnCreate = 0;				

#if __GCM
	poolParams.m_Alignment = alignment;			
	poolParams.m_PhysicalMem = isPhysical;
	poolParams.m_Pitch = pitch;
	poolParams.m_Compression = compression;
#endif // __GCM

	m_PoolID = grcRenderTargetPoolMgr::CreatePool(name, poolParams, poolMem);
	m_ActiveHeap = 0;
}

grcRenderTargetMemPool::~grcRenderTargetMemPool()
{
	grcRenderTargetPoolMgr::DestroyPool(m_PoolID);  // remove the pool mgr.
	sm_MemPools.DeleteMatches(this);				// delete it from our list.
}

void grcRenderTargetMemPool::Activate(u8 heapID, bool ASSERT_ONLY(reset))
{	
	Assert(heapID < grcRenderTargetPoolMgr::GetHeapCount(m_PoolID));

	m_ActiveHeap = heapID;
	sm_ActivePool = this;

	// reseting does not work with a real heap system, there are already allocating into the heap, what happens to them?
	// 	if (reset)
	// 		m_Used[m_ActiveHeap] = 0;
	Assertf(!reset || grcRenderTargetPoolMgr::GetUsedMem(m_PoolID,heapID)==0,"Reseting a grcRenderTargetMemPool heap (%d) after allocating from it is no long supported. (Pool=\"%s\")",heapID, grcRenderTargetPoolMgr::GetName(m_PoolID));
}


void* grcRenderTargetMemPool::AllocateTextureMem(int size, int alignment /*= kDefaultAlignment*/)
{
	AssertMsg(sm_ActivePool, "No render target memory pool is active");

	Assert(grcRenderTargetPoolMgr::GetIsInitialised(sm_ActivePool->m_PoolID));

	return grcRenderTargetPoolMgr::AllocateTextureMem(sm_ActivePool->m_PoolID, sm_ActivePool->m_ActiveHeap, size, 0, alignment);
}


#if HACK_GTA4
int grcRenderTargetMemPool::GetMaxUsedSize()
{
	FastAssert(sm_ActivePool);
	u32 max = 0;

	int heapCount = GetHeapCount();
	u16 poolID = sm_ActivePool->m_PoolID;
	for (u8 i = 0; i < heapCount; ++i) {
		u32 val = grcRenderTargetPoolMgr::GetUsedMem(poolID,i);
		if (val > max)
			max = val;
	}
	return max;
}

int grcRenderTargetMemPool::GetMinFreeSize()
{
	FastAssert(sm_ActivePool);
	u16 poolID = sm_ActivePool->m_PoolID;

	u32 min = grcRenderTargetPoolMgr::GetAllocated(poolID);
	int heapCount = GetHeapCount();

	for (u8 i = 0; i < heapCount; ++i) {
		u32 val = grcRenderTargetPoolMgr::GetFreeMem(poolID,i);
		if (val < min)
			min = val;
	}
	return min;
}

int grcRenderTargetMemPool::GetMinWorstFreeSize()
{
	FastAssert(sm_ActivePool);
	u16 poolID = sm_ActivePool->m_PoolID;
	return grcRenderTargetPoolMgr::GetLowestFreeMem(poolID);
}
#endif // HACK_GTA4

}	// namespace rage
