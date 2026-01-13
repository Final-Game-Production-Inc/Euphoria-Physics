// 
// grcore/texturegcm.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "data/resource.h"
#include "diag/tracker.h"
#include "file/limits.h"
#include "grcore/device.h"
#include "grcore/effect_values.h"
#include "grcore/image.h"
#include "grcore/wrapper_gcm.h"
#include "grcore/texturegcm.h"
#include "grcore/texturereference.h"
#include "grcore/texturecontrol.h"
#include "math/intrinsics.h"
#include "string/string.h"
#include "system/memory.h"
#include "system/typeinfo.h"
#include "system/nelem.h"
#include "system/param.h"

#define GetSurfaceName() grcRenderTargetMemPool::IsActivate() ? grcRenderTargetMemPool::GetName() : GetName()

#if __GCM || (__RESOURCECOMPILER && !__64BIT)
#include "device.h"

namespace rage
{

// tiled memory management
atFixedBitSet<15> grcRenderTargetGCM::sm_BitFieldTiledMemory;

// z culling management
grcRenderTargetGCM::zCullRecord grcRenderTargetGCM::sm_ZCullRegionsRecord[8];
atFixedBitSet<8> grcRenderTargetGCM::sm_BitFieldZCulling;

// grcRenderTargetGCM static member variables
u32 grcRenderTargetGCM::sm_TiledMemorySize = 0;

// z cull allocation start
u32 grcRenderTargetGCM::sm_ZCullStart = 0;

// tiling bank counters
u32	grcRenderTargetGCM::sm_CompressionTag = 0;
u32	grcRenderTargetGCM::sm_ColorBank = 0;
#if !HACK_GTA4
	u32	grcRenderTargetGCM::sm_DepthBank = 0;
#endif // !HACK_GTA4

// The width of the window the game is running in
extern int g_WindowWidth;

// The height of the window the game is running in
extern int g_WindowHeight;
#if __PPU
extern u32 g_FrontBufferCount;
extern grcRenderTargetGCM *g_FrontBuffer[];
extern grcRenderTargetGCM *g_BackBuffer;
extern grcRenderTargetGCM *g_DepthBuffer;
extern grcRenderTargetGCM *g_DepthBackBuffer;
extern int g_grcDepthFormat;
#else
int g_grcDepthFormat; // Define this variable because it can't be externed from device_gcm.cpp on WIN32 builds (resourcing)
#endif

XPARAM(hdr);
XPARAM(mrt);
XPARAM(srgb);
NOSTRIP_PARAM(nodepth, "[grcore] disable creation of a depth buffer by rage");
PARAM(bb720pwidth, "[grcore] back buffer width when running 720p");
PARAM(bb720pheight, "[grcore] back buffer height when running 720p");
PARAM(texturesinsystemmemory,"[grcore] Force all PS3 textures to be in system memory");
PARAM(fbcount,"[grcore] Number of front buffers (default = 2, max 8)");

// seems like this should work, but it doesn't
#define DEPTH_TARGET_USES_TEXTURE 0

#if __GCM

// NEW render target pool system
void grcRenderTargetPoolEntry::InitializeMemory(const grcRTPoolCreateParams & params)
{
	m_IsInitialised = true;
	m_IsColorTarget = params.m_Type != grcrtDepthBuffer && params.m_Type != grcrtShadowMap && params.m_Type != grcrtDepthBufferCubeMap;
	m_IsInTiledMem = params.m_Tiled;
	m_Pitch = params.m_Pitch;

	// Validate the size of the whole memory pool to see if it will
	// fit in tiled memory

	if (m_IsInTiledMem)
	{
		if (grcRenderTargetGCM::sm_BitFieldTiledMemory.CountBits(false) == 0)
		{
			grcWarningf("Only %d slots available for tiled memory, \"%s\" will not be tiled.", grcRenderTargetGCM::sm_BitFieldTiledMemory.GetNumBits(), m_Name);
			m_IsInTiledMem = false;
		}

		if ((grcRenderTargetGCM::sm_TiledMemorySize + m_Size) > 0x800 * 0x10000)
		{
			grcWarningf("We have a maximum of 128MB for tiled render targets, \"%s\" will not be tiled.", m_Name);
			m_IsInTiledMem = false;
		}

		if (params.m_Swizzled)
		{
			grcWarningf("You can't have a swizzled render target in tiled memory, disabling tiling for Pool \"%s\"",m_Name);
			m_IsInTiledMem = false;
		}
	}

	if (m_IsInTiledMem)
	{		
		// the following is mostly copied from MoveIntoTiledMem, but doe not assume we are a rendertarget.
		// eventually, when all rendertargets come from pools, MoveIntoTileMemory will go away and this will be used by all.

		// find a free tiled memory slot
		for (m_TiledIndex = 0; m_TiledIndex < grcRenderTargetGCM::sm_BitFieldTiledMemory.GetNumBits(); ++m_TiledIndex)
		{
			if(grcRenderTargetGCM::sm_BitFieldTiledMemory.IsClear(m_TiledIndex))
			{
				grcRenderTargetGCM::sm_BitFieldTiledMemory.Set(m_TiledIndex);
				break;
			}
		}

		// just skip all this if there are not more tiles available...
		if(m_TiledIndex<grcRenderTargetGCM::sm_BitFieldTiledMemory.GetNumBits())
		{
			// this determines which DRAM page is accessed
			// if a color and a depth buffer access the same DRAM page simultaneously, a page miss will occur 
			// ... whatever the result of a page miss is will then happen :-)
			// to move as most render targets as possible in different banks, I just count up here and reset to 0 if 3 is reached,
			// because we have four different DRAM banks
			//
			// from my understanding, render targets that are used with each other should be created close to each other ... so 
			// they should always end up in different banks

			u32 bank = 0;

			switch (params.m_Type)
			{
				case grcrtVolume:
				case grcrtCubeMap:
				case grcrtPermanent:
				case grcrtFrontBuffer:
				case grcrtBackBuffer:
					{
						bank = grcRenderTargetGCM::sm_ColorBank + 1;
						++grcRenderTargetGCM::sm_ColorBank %= 3;

						//
						// color compression is only applicable to 32-bits-per-pixel surfaces!
						//
						// if ROPTiles are compressed, the ROP block can output them at a rate of 1 clock/ROPTile -> for 32bpp, 32 samples / clock
						// if ROPTiles are not compressed, the ROP block can output them at a rate of 4 clocks / ROPTile -> for 32 bpp, 8 samples / clock

						// only works on 32bpp stuff
						if (m_BitDepth == 32)
						{
							// 4AA choose the 2x2 compression mode
							if (m_MSAA == grcDevice::MSAA_Centered4xMS || m_MSAA == grcDevice::MSAA_Rotated4xMS)
							{
								m_Compression = CELL_GCM_COMPMODE_C32_2X2;
							}
							// no super-sampling or 2AA choose the 2x1 compression mode
							else
							{
								m_Compression = CELL_GCM_COMPMODE_C32_2X1;
							}

						}
					}
					break;
				case grcrtDepthBuffer:
				case grcrtShadowMap:
					{
						bank = 0;		// just always use bank 0 for depth, loop on 1-3 (above) for colour

						// this is good for an MS mode CENTERED_1 and SQUARE_CENTERED_4
						// no compression but
						// this is better than CELL_GCM_COMPMODE_DISABLED because it allows to 
						// ignore the stencil value if it is not used ... so we only write 3 bytes
						// instead of 4 bytes

						// TODO: crap, what do we do here? I don't have the depth buffer format info available...
						if (1 /*|| (m_Texture.format & (CELL_GCM_TEXTURE_DEPTH24_D8 | CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT)) != 0*/)
						{
							m_Compression = CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REGULAR;

							if (m_MSAA == grcDevice::MSAA_2xMS)
							{
								// this compression mode is for the multi-sampling mode CENTERED_2
								m_Compression = CELL_GCM_COMPMODE_Z32_SEPSTENCIL_DIAGONAL;
							}
							else if (m_MSAA == grcDevice::MSAA_Rotated4xMS)
							{
								// this compression mode is for the multi-sampling mode ROTATED_4
								m_Compression = CELL_GCM_COMPMODE_Z32_SEPSTENCIL_ROTATED;
							}
						}
					}
					break;
				default:
					Quitf("Render target type not supported in MoveIntoTiledMemory()");
			}
			
			// ignore compression mode if they did not enable it. override it otherwise since it must match tiled compression type
			m_Compression = params.m_Compression ? m_Compression : CELL_GCM_COMPMODE_DISABLED;
		
			u8 location = (params.m_PhysicalMem) ? CELL_GCM_LOCATION_LOCAL : CELL_GCM_LOCATION_MAIN;
			u32 offset = (params.m_PhysicalMem) ? gcm::LocalOffset(m_PoolMemory) : gcm::MainOffset(m_PoolMemory);

			int info = cellGcmSetTileInfo(	m_TiledIndex,		// index of the tiled area to set 0..14
											location,			// vram or main ram
											offset,				// offset in vram or main ram
											m_Size,				// size of area to set as tiled
											m_Pitch,			// pitch size
											m_Compression,		// the compression we want to use.
											grcRenderTargetGCM::sm_CompressionTag,	// starting address of compression tag area
											bank);				// bank offset of tiled area 0-3

			// Note that failure of cellGcmSetTileInfo is considerred a fatal
			// error, since the non-debug libraries can create bad tiling
			// regions which will lead to very painful to diagnose RSX crashes
			// (for example a bad alignment creates a tiling region at the
			// rounded down offset, which can then overlap non-tiled surfaces).
			switch (info)
			{
				case CELL_GCM_ERROR_INVALID_VALUE:
 					if (m_TiledIndex >= 15)
 						Quitf("[%s] Invalid tile index (0 - 14): %d", m_Name, m_TiledIndex);
					if (grcRenderTargetGCM::sm_CompressionTag >= 800)
						Quitf("[%s] Invalid starting address of compression tag area (0x0 - 0x7ff): %d", m_Name, grcRenderTargetGCM::sm_CompressionTag);
					if (bank >= 4)
						Quitf("[%s] Invalid bank offset (0 - 3): %d", m_Name, bank);
					m_Compression = CELL_GCM_COMPMODE_DISABLED;
					m_IsInTiledMem = false;
					break;
				case CELL_GCM_ERROR_INVALID_ALIGNMENT:
					if ((offset & 65535) != 0)
						Quitf("[%s] Address offset of area to set as tiled is not 64kb aligned: %d", m_Name, offset);
					if ((m_Size & 65535) != 0)
						Quitf("[%s] Size of area to set as tiled is not 64kb aligned: %d", m_Name, m_Size);
					if ((m_Pitch & 255) != 0)
						Quitf("[%s] Pitch of area to set as tiled is not 256b aligned: %d", m_Name, m_Pitch);
					m_Compression = CELL_GCM_COMPMODE_DISABLED;
					m_IsInTiledMem = false;
					break;
				case CELL_GCM_ERROR_INVALID_ENUM:
					switch (m_Compression)
					{
						case CELL_GCM_COMPMODE_DISABLED:
						case CELL_GCM_COMPMODE_C32_2X1:
						case CELL_GCM_COMPMODE_C32_2X2:
						case CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REGULAR:
						case CELL_GCM_COMPMODE_Z32_SEPSTENCIL_DIAGONAL:
						case CELL_GCM_COMPMODE_Z32_SEPSTENCIL_ROTATED:
							break;
						default:
							Quitf("[%s] Invalid compression mode for area to set as tiled: %d", m_Name, m_Compression);
							break;
					}

					if (location != CELL_GCM_LOCATION_MAIN || location != CELL_GCM_LOCATION_LOCAL)
						Quitf("[%s] Invalid memory location for area to set as tiled: %d", m_Name, location);

					m_Compression = CELL_GCM_COMPMODE_DISABLED;
					m_IsInTiledMem = false;
					break;

				default:
					if (cellGcmBindTile(m_TiledIndex) == CELL_GCM_ERROR_ADDRESS_OVERWRAP)
					{
						Quitf("[%s] Tiling regions overlap", m_Name);
						m_Compression = CELL_GCM_COMPMODE_DISABLED;
						m_IsInTiledMem = false;
					}
					else
					{
						grcRenderTargetGCM::sm_TiledMemorySize += m_Size;
						if (m_Compression != CELL_GCM_COMPMODE_DISABLED)
						{
							grcRenderTargetGCM::sm_CompressionTag += m_Size / 0x10000;
						}
					}
					break;
			}
		}
		else
		{
			grcErrorf("[%s] No tile index available, disabling tiling for this pool", m_Name);
			m_IsInTiledMem = false;
		}
	}
}

// AllocatePoolMemory() is platform specific
void grcRenderTargetPoolEntry::AllocatePoolMemory(u32 size, bool physical, int alignment, void * buffer)
{
	m_Size = size;

	if( buffer )
	{
		Assert(physical == false || gcm::IsLocalPtr(buffer));
		// should we verify the buffer alignment matches the alignment value?
	
		m_AllocatedMemory = false;
		m_PoolMemory = buffer;
	}
	else
	{
		m_AllocatedMemory = true;

		if (physical)
		{
			m_PoolMemory = physical_new( m_Size, alignment );
			Assert(m_PoolMemory);
		}
		else
		{
			m_PoolMemory = rage_aligned_new (alignment) char[m_Size];
			Assert(m_PoolMemory);
		}
	}
}

void grcRenderTargetPoolEntry::FreePoolMemory()
{
	if (m_AllocatedMemory)
	{
		delete m_PoolMemory;
	}

	m_PoolMemory = NULL;
}

void grcRenderTargetGCM::AllocateNonPooled()
{
	int textureFormat = gcm::StripTextureFormat(m_Texture.format);

	// Tiling places restrictions on the way we allocate memory
	ValidateTiledMemory();

	// Z culling places restrictions on the way we allocate memory
	const bool useZCull = ValidateZCulling(false);
	if (!useZCull)
		m_BooleanTraits.Clear(grcrttUseZCulling);  // we're not pools, so we'll disable the trait.

	const bool inTiledMemory = m_BooleanTraits.IsSet(grcrttInTiledMemory);
	const bool isSwizzled = m_BooleanTraits.IsSet(grcrttIsSwizzled);
	const bool inLocalMemory = m_BooleanTraits.IsSet(grcrttInLocalMemory);
	const bool isCubeMap = m_Type == grcrtCubeMap;

	const u32 memoryAlignment = gcm::GetSurfaceAlignment(inTiledMemory, useZCull);

	// Allocate enough memory to keep a memory offset for the beginning of each layer/mip
	m_MemoryOffsets = rage_new u32[m_Texture.mipmap * GetLayerCount()];
	m_MemoryBaseOffsets = rage_new u32[m_Texture.mipmap * GetLayerCount()];

	// To start with start with the top most layer/mip
	m_LockedLayer = 0;
	m_LockedMip = 0;

	void* result;

	SetPhysicalSize(gcm::GetSurfaceSize(m_Texture.width, m_Texture.height, m_BitsPerPix, m_Texture.mipmap, GetLayerCount(), inLocalMemory, inTiledMemory, isSwizzled, isCubeMap, useZCull, m_MemoryBaseOffsets));
	Assert(GetPhysicalSize() > 0);
	if (inTiledMemory)
	{
		// Validate that this will actually fit in tiled memory
		if (!ValidateTiledMemory(GetPhysicalSize()))
		{
			AllocateNonPooled(); // what is this? I guess Validate Tiled changes the parameters, so it tries again.
			return;
		}
		m_Texture.pitch = gcm::GetSurfaceTiledPitch(m_Texture.width, m_BitsPerPix, isSwizzled);
	}
	else
	{
		m_Texture.pitch = gcm::GetSurfacePitch(m_Texture.width, m_BitsPerPix, isSwizzled);
	}

	if (inLocalMemory)
	{
		result = physical_new(GetPhysicalSize(), memoryAlignment);
		Assertf(result, "Cannot allocate local memory for render target \"%s\"", m_Name);
	}
	else
	{
		result = rage_aligned_new (memoryAlignment) char[GetPhysicalSize()];
		Assertf(result, "Cannot allocate main memory for render target \"%s\"", m_Name);
	}


	if (inLocalMemory)
	{
		m_Texture.offset = m_MemoryOffset = gcm::LocalOffset(result);
		m_Texture.location = CELL_GCM_LOCATION_LOCAL;
	}
	else
	{
		m_Texture.offset = m_MemoryOffset = gcm::MainOffset(result);
		m_Texture.location = CELL_GCM_LOCATION_MAIN;
	}

	// Add base offset
	Assert(m_MemoryBaseOffsets[0] == 0);
	for (u32 layer = 0; layer < GetLayerCount(); ++layer)
	{
		for (u32 mip = 0; mip < m_Texture.mipmap; ++mip)
		{
			m_MemoryOffsets[mip * GetLayerCount() + layer] = m_MemoryBaseOffsets[mip * GetLayerCount() + layer] + m_Texture.offset;
		}
	}

	UpdatePackedTexture();
}

void grcRenderTargetGCM::AllocateInPlace(u32 baseOffset)
{
	int textureFormat = gcm::StripTextureFormat(m_Texture.format);

	// Tiling places restrictions on the way we allocate memory
	ValidateTiledMemory();

	// Z culling places restrictions on the way we allocate memory
	const bool useZCull = ValidateZCulling(false);
	if (!useZCull)
		m_BooleanTraits.Clear(grcrttUseZCulling);  // we're not pools, so we'll disable the trait.

	const bool inTiledMemory = m_BooleanTraits.IsSet(grcrttInTiledMemory);
	const bool isSwizzled = m_BooleanTraits.IsSet(grcrttIsSwizzled);
	const bool inLocalMemory = m_BooleanTraits.IsSet(grcrttInLocalMemory);
	const bool isCubeMap = m_Type == grcrtCubeMap;

	const u32 memoryAlignment = gcm::GetSurfaceAlignment(inTiledMemory, useZCull);

	// Allocate enough memory to keep a memory offset for the beginning of each layer/mip
	m_MemoryOffsets = rage_new u32[m_Texture.mipmap * GetLayerCount()];
	m_MemoryBaseOffsets = rage_new u32[m_Texture.mipmap * GetLayerCount()];

	// To start with start with the top most layer/mip
	m_LockedLayer = 0;
	m_LockedMip = 0;

	SetPhysicalSize(gcm::GetSurfaceSize(m_Texture.width, m_Texture.height, m_BitsPerPix, m_Texture.mipmap, GetLayerCount(), inLocalMemory, inTiledMemory, isSwizzled, isCubeMap, useZCull, m_MemoryBaseOffsets));
	Assert(GetPhysicalSize() > 0);
	if (inTiledMemory)
	{
		// Validate that this will actually fit in tiled memory
		if (!ValidateTiledMemory(GetPhysicalSize()))
		{
			AllocateNonPooled(); // what is this? I guess Validate Tiled changes the parameters, so it tries again.
			return;
		}
		m_Texture.pitch = gcm::GetSurfaceTiledPitch(m_Texture.width, m_BitsPerPix, isSwizzled);
	}
	else
	{
		m_Texture.pitch = gcm::GetSurfacePitch(m_Texture.width, m_BitsPerPix, isSwizzled);
	}


	if (inLocalMemory)
	{
		m_Texture.offset = m_MemoryOffset = baseOffset;
		m_Texture.location = CELL_GCM_LOCATION_LOCAL;
	}
	else
	{
		m_Texture.offset = m_MemoryOffset = baseOffset;
		m_Texture.location = CELL_GCM_LOCATION_MAIN;
	}

	// Add base offset
	Assert(m_MemoryBaseOffsets[0] == 0);
	for (u32 layer = 0; layer < GetLayerCount(); ++layer)
	{
		for (u32 mip = 0; mip < m_Texture.mipmap; ++mip)
		{
			m_MemoryOffsets[mip * GetLayerCount() + layer] = m_MemoryBaseOffsets[mip * GetLayerCount() + layer] + m_Texture.offset;
		}
	}

	UpdatePackedTexture();
}


void grcRenderTargetGCM::AllocateFromPool()
{
	int textureFormat = gcm::StripTextureFormat(m_Texture.format);

// this assumes ValidatePoolAllocation() has already been called to set up pitch, calc m_PhysicalSize, etc.

	// TODO: this needs to be checked more carefully, what is the overhead of changing the zcull regions?
	//       they may have to be permanently allocated.
	const bool inTiledMemory = m_BooleanTraits.IsSet(grcrttInTiledMemory);
	const bool isSwizzled = m_BooleanTraits.IsSet(grcrttIsSwizzled);
	const bool inLocalMemory = m_BooleanTraits.IsSet(grcrttInLocalMemory);
	
	Assertf(inTiledMemory == grcRenderTargetPoolMgr::GetIsMemoryPoolTiled(m_PoolID), "Tiling of target (%s = %stiled) does not match pool (%s= %stiled)",
				m_Name,	inTiledMemory?"":"un", grcRenderTargetPoolMgr::GetName(m_PoolID), grcRenderTargetPoolMgr::GetIsMemoryPoolTiled(m_PoolID)?"":"un");

	Assertf(inLocalMemory == grcRenderTargetPoolMgr::GetIsPhysical(m_PoolID), "Memory location of target (%s=%s) does not match pool (%s=%s)",
				m_Name,	inLocalMemory ? "local":"main", grcRenderTargetPoolMgr::GetName(m_PoolID), grcRenderTargetPoolMgr::GetIsPhysical(m_PoolID)?"local":"main");

	bool useZCull = false;
	
	if (m_BooleanTraits.IsSet(grcrttUseZCulling)) // if this is true, we did not determine eligibility, etc, and need to now (on the first use).
	{											  // we also need to compute m_Physical after that.
		useZCull = ValidateZCulling(false);
		if (!useZCull) // if we could not do it, lets disable it for the future
			 m_BooleanTraits.Clear(grcrttUseZCulling);

		u32 pitch = grcRenderTargetPoolMgr::GetMemoryPoolPitch(m_PoolID);
		const bool isCubeMap = m_Type == grcrtCubeMap;

		SetPhysicalSize(gcm::GetSharedSurfaceSizeForPitch(m_Texture.width, m_Texture.height, pitch, m_BitsPerPix,	m_Texture.mipmap, GetLayerCount(), inLocalMemory, inTiledMemory, isSwizzled, isCubeMap, useZCull, m_MemoryBaseOffsets));
		Assert(m_MemoryBaseOffsets[0] == 0);
	}

	void* result;

	if (inTiledMemory)
	{
		result = grcRenderTargetPoolMgr::AllocateTextureMem(m_PoolID, m_PoolHeap, GetPhysicalSize(), m_OffsetInPool, grcRenderTargetPoolMgr::GetMemoryPoolPitch(m_PoolID) * 8);
	}
	else
	{
		result = grcRenderTargetPoolMgr::AllocateTextureMem(m_PoolID, m_PoolHeap, GetPhysicalSize(), m_OffsetInPool, gcm::GetSurfaceAlignment(false, useZCull));
	}
	Assertf(result, "Cannot allocate from memory pool for render target \"%s\"", m_Name);

	if (inLocalMemory)
		m_Texture.offset = m_MemoryOffset = gcm::LocalOffset(result);
	else
		m_Texture.offset = m_MemoryOffset = gcm::MainOffset(result);

	// Add to base offsets
	for (u32 layer = 0; layer < GetLayerCount(); ++layer)
	{
		for (u32 mip = 0; mip < m_Texture.mipmap; ++mip)
		{
			m_MemoryOffsets[mip * GetLayerCount() + layer] = m_MemoryBaseOffsets[mip * GetLayerCount() + layer] + m_Texture.offset;
		}
	}

	UpdatePackedTexture();
}

void grcRenderTargetGCM::ReleaseFromPool()
{
	// can't release zcull targets without updating the zcull tables, etc, that can only happen when RSX is idle, so we just complain for now
	Assertf(!m_BooleanTraits.IsSet(grcrttUseZCulling), "trying to unlock a rendertarget (\"%s\" in pool \"%s\") that is set for zCull", m_Name, grcRenderTargetPoolMgr::GetName(m_PoolID));
	
	void * ptr  = ( m_BooleanTraits.IsSet(grcrttInLocalMemory)) ?  gcm::LocalPtr(m_MemoryOffset) :  gcm::MainPtr(m_MemoryOffset);

	grcRenderTargetPoolMgr::FreeTextureMem(m_PoolID, m_PoolHeap, ptr);
	
	// should we force m_MemoryOffset invalid here?
}

// NOTE: this function is used for compressing into dxt textures, hence the strangeness.
// it could be tweaked to work for regular textures if needed but won't work like this.
void grcRenderTargetGCM::OverrideTextureOffsets(u32 baseOffset, u16 newWidth, u16 newHeight)
{
	m_Texture.offset = m_MemoryOffset = baseOffset;
	newWidth = newWidth >> 1;
	newHeight = newHeight >> 2;

	// we only need to calculate new offsets if the dimensions are different, otherwise just updating the base offset is enough
	if (newWidth != m_Texture.width || newHeight != m_Texture.height)
	{
		m_Texture.width = newWidth;
		m_Texture.height = newHeight;

		const bool inLocalMemory = m_BooleanTraits.IsSet(grcrttInLocalMemory);
		const bool inTiledMemory = m_BooleanTraits.IsSet(grcrttInTiledMemory);
		const bool isSwizzled = true;

		SetPhysicalSize(gcm::GetSurfaceSize(m_Texture.width, m_Texture.height, m_BitsPerPix, m_Texture.mipmap, GetLayerCount(), inLocalMemory, inTiledMemory, isSwizzled, false, false, m_MemoryBaseOffsets));
	}

	// update offsets
	Assert(m_MemoryBaseOffsets[0] == 0);
	for (u32 layer = 0; layer < GetLayerCount(); ++layer)
	{
		for (u32 mip = 0; mip < m_Texture.mipmap; ++mip)
		{
			m_MemoryOffsets[mip * GetLayerCount() + layer] = m_MemoryBaseOffsets[mip * GetLayerCount() + layer] + m_Texture.offset;
		}
	}

	UpdatePackedTexture();
}

// we'll calc strides, memory size, and compatibility here during create time, so we don't need to every time we AllocateMemoryFromPool() the target.
void grcRenderTargetGCM::ValidatePoolAllocation()
{
	int textureFormat = gcm::StripTextureFormat(m_Texture.format);

	// Z culling places restrictions on the way we allocate memory
	const bool useZCull = m_BooleanTraits.IsSet(grcrttUseZCulling); //ValidateZCulling(false);
	const bool inTiledMemory = m_BooleanTraits.IsSet(grcrttInTiledMemory);
	const bool isSwizzled = m_BooleanTraits.IsSet(grcrttIsSwizzled);
	const bool inLocalMemory = m_BooleanTraits.IsSet(grcrttInLocalMemory);
	const bool isCubeMap = m_Type == grcrtCubeMap;

	// Allocate enough memory to keep a memory offset for the beginning of each layer/mip
	m_MemoryOffsets = rage_new u32[m_Texture.mipmap * GetLayerCount()];
	m_MemoryBaseOffsets = rage_new u32[m_Texture.mipmap * GetLayerCount()];

	// To start with start with the top most layer/mip
	m_LockedLayer = 0;
	m_LockedMip = 0;

	// Calculate the physical size required for this render target in a memory pool
	u32 pitch = grcRenderTargetPoolMgr::GetMemoryPoolPitch(m_PoolID);
	Assert(pitch > 0);

	if (!useZCull)  // can't do this now if we might use zcull
	{
		SetPhysicalSize(gcm::GetSharedSurfaceSizeForPitch(m_Texture.width, m_Texture.height, pitch, m_BitsPerPix,	m_Texture.mipmap, GetLayerCount(), inLocalMemory, inTiledMemory, isSwizzled, isCubeMap, useZCull, m_MemoryBaseOffsets));
		Assert(m_MemoryBaseOffsets[0] == 0);
	}

	m_Texture.pitch = pitch;

	Assertf(!inTiledMemory || pitch >= gcm::GetSurfaceTiledPitch(m_Texture.width, m_BitsPerPix, isSwizzled),"RenderTarget \"%s\" (w=%d, bpp=%d) Requires a pitch of %d while current memory pool is %d", m_Name,m_Texture.width,m_BitsPerPix,gcm::GetSurfaceTiledPitch(m_Texture.width, m_BitsPerPix, isSwizzled),pitch);
	Assertf( inTiledMemory || pitch >= gcm::GetSurfacePitch     (m_Texture.width, m_BitsPerPix, isSwizzled),"RenderTarget \"%s\" (w=%d, bpp=%d) Requires a pitch of %d while current memory pool is %d", m_Name,m_Texture.width,m_BitsPerPix,gcm::GetSurfacePitch(m_Texture.width, m_BitsPerPix, isSwizzled),pitch);
	Assertf(!inTiledMemory || grcRenderTargetPoolMgr::GetIsMemoryPoolTiled(m_PoolID),"RenderTarget \"%s\" Requires a tiled memory pool (%s is not)",m_Name,grcRenderTargetPoolMgr::GetName(m_PoolID));

	u32 physSize = GetPhysicalSize();
	if (inTiledMemory || (m_OffsetInPool%pitch) == 0) // only check this if we have not been tweaking the offset inside the stride
	{
		Assertf(m_OffsetInPool + physSize <= grcRenderTargetPoolMgr::GetAllocated(m_PoolID), "Rendertarget \"%s\" (%d bytes) is too big to fit in RenderTarget pool \"%s\" (%d bytes) at offset %d.",m_Name,GetPhysicalSize(), grcRenderTargetPoolMgr::GetName(m_PoolID),grcRenderTargetPoolMgr::GetAllocated(m_PoolID), m_OffsetInPool);
	}

	if (m_PoolHeap==kRTPoolHeapInvalid && inTiledMemory)// verify that the offset is a safe alignment if tiled (8*pitch)
	{
		Assertf((m_OffsetInPool%(pitch*8))==0, "Rendertarget \"%s\" specified a direct offset (%d) that is not properly aligned for the tiled rendertarget pool \"%s\" (should be %d, i.e. 8*pitch)", m_Name, m_OffsetInPool, grcRenderTargetPoolMgr::GetName(m_PoolID), pitch*8);
	}

	m_BooleanTraits.Set(grcrttInMemoryPool);

	m_Texture.offset = m_MemoryOffset = 0;
	m_Texture.location = (inLocalMemory) ? CELL_GCM_LOCATION_LOCAL : CELL_GCM_LOCATION_MAIN;

}

bool grcRenderTargetGCM::ValidateTiledMemory(u32 memorySize /*= 0*/)
{
	if (m_BooleanTraits.IsClear(grcrttInTiledMemory))
	{
		return false;
	}

	if (sm_BitFieldTiledMemory.CountBits(false) == 0)
	{
		grcWarningf("Only %d slots available for tiled memory, \"%s\" will not be tiled.", sm_BitFieldTiledMemory.GetNumBits(), m_Name);
		m_BooleanTraits.Clear(grcrttInTiledMemory);
		if( m_PoolID != kRTPoolIDInvalid )
		{
			grcWarningf("RT mempool \"%s\" will not be tiled.", grcRenderTargetPoolMgr::GetName(m_PoolID));
		}
		
		return false;
	}

	if ((sm_TiledMemorySize + memorySize) > 0x800 * 0x10000)
	{
		grcWarningf("We have a maximum of 128MB for tiled render targets, \"%s\" will not be tiled.", m_Name);
		m_BooleanTraits.Clear(grcrttInTiledMemory);
		return false;
	}

	if (m_BooleanTraits.IsSet(grcrttIsSwizzled))
	{
		grcWarningf("You can't have a swizzled render target in tiled memory");
		m_BooleanTraits.Clear(grcrttInTiledMemory);
		return false;
	}

	return true;
}

bool grcRenderTargetGCM::ValidateZCulling(bool validateAlignment /*= true*/)
{
	if (m_BooleanTraits.IsClear(grcrttUseZCulling))
	{
		return false;
	}

	// if it is not a depth buffer or a shadow map there should not be Z culling involved
	if (m_Type != grcrtDepthBuffer && m_Type != grcrtShadowMap)
	{
//		m_BooleanTraits.Clear(grcrttUseZCulling);  // don't clear anymore, we'' do that above if we want to.
		return false;
	}

	if (validateAlignment && ((m_Texture.offset + 4095) & ~4095) != m_Texture.offset)
	{
		grcWarningf("Texture address must be 4k aligned for Z culling to be enabled");
//		m_BooleanTraits.Clear(grcrttUseZCulling);
		return false;
	}

	if (!m_BooleanTraits.IsSet(grcrttInLocalMemory))
	{
		grcWarningf("You can't use Z culling for render targets in main memory, only VRAM");
//		m_BooleanTraits.Clear(grcrttUseZCulling);
		return false;
	}

#if 0
	// if there is no slot open for Z culling show the error message
	if (sm_BitFieldZCulling.CountBits(false) == 0)
	{
		grcWarningf("Z culling is only available for %d render targets, \"%s\" will not use Z culling.", sm_BitFieldZCulling.GetNumBits(), m_Name);
//		m_BooleanTraits.Clear(grcrttUseZCulling);
		return false;
	}
#endif // 0

	if (m_PoolID == kRTPoolIDInvalid) // not in a pool
	{
		if (sm_BitFieldTiledMemory.CountBits(false) == 0)
		{
			grcWarningf("Z culling is only available for tiled render targets and there are no tiling regions free, \"%s\" will not use Z culling.", m_Name);
//			m_BooleanTraits.Clear(grcrttUseZCulling);
			return false;
		}
		else
		{
			m_BooleanTraits.Set(grcrttInTiledMemory);
		}
	}

	return true;
}

void grcRenderTargetGCM::FreeRenderTarget()
{
	if (m_BooleanTraits.IsClear(grcrttInMemoryPool) && m_BooleanTraits.IsClear(grcrttUseTextureMemory))
	{
		if (m_Texture.location == CELL_GCM_LOCATION_LOCAL)
		{
			physical_delete(gcm::LocalPtr(m_Texture.offset));
		}
		else
		{
			Assert(m_Texture.location == CELL_GCM_LOCATION_MAIN);
			delete [] reinterpret_cast<char*>(gcm::MainPtr(m_Texture.offset));
		}
	}
}

bool grcRenderTargetGCM::LockRect(int layer, int mipLevel, grcTextureLock &lock, u32 ASSERT_ONLY(uLockFlags) ) const
{ 
	Assert(layer >= 0 && layer < static_cast<int>(GetLayerCount()));
	Assert(mipLevel >= 0 && mipLevel < m_Texture.mipmap);
	Assert(m_Allocated);
	Assertf(m_Texture.location != CELL_GCM_LOCATION_LOCAL || (uLockFlags & grcsAllowVRAMLock), "Trying to lock VRAM texture '%s' - really slow!", GetName());
	lock.Base = gcm::GetPtr(m_MemoryOffsets[mipLevel * GetLayerCount() + layer],m_Texture.location == CELL_GCM_LOCATION_LOCAL);
	lock.BitsPerPixel = m_BitsPerPix; 
	lock.Width = Max<int>(m_Texture.width >> mipLevel, 1);
	lock.Height = Max<int>(m_Texture.height >> mipLevel, 1);

	if ((m_Texture.format & CELL_GCM_TEXTURE_LN) == 0)
	{
		lock.Pitch = lock.Width * lock.BitsPerPixel / 8;
	}
	else
	{
		lock.Pitch = m_Texture.pitch;
	}

	lock.RawFormat = m_Texture.format;
	lock.MipLevel = mipLevel;
	lock.Layer = layer;

	return true;
}

grcTextureFactory *grcTextureFactory::CreatePagedTextureFactory(bool bMakeActive) 
#else

bool grcRenderTargetGCM::LockRect(int /*layer*/, int /*mipLevel*/,grcTextureLock &/*lock*/, u32 /*uLockFlags*/) const 
{ 
	return false;
}

grcTextureFactory *grcTextureFactoryGCM::CreatePagedTextureFactory(bool bMakeActive) 
#endif
{
	grcTextureFactory *pFactory = rage_new grcTextureFactoryGCM;

	if	(bMakeActive)
		SetInstance(*pFactory);

	return(pFactory);
}

static void PatchShadowToDepthBuffer(grcRenderTargetGCM *target, bool patchSurfaceFormat)
{
	CellGcmTexture &gcmTexture = target->GetGcmTexture();

	u8 colorFormat = gcm::StripTextureFormat(gcmTexture.format);
	u8 colorFlags = gcmTexture.format & ~colorFormat;

	grcAssertf((colorFormat==CELL_GCM_TEXTURE_DEPTH24_D8) || (colorFormat==CELL_GCM_TEXTURE_DEPTH16) ,"invalid format?");

	bool is16BitDepth = (colorFormat==CELL_GCM_TEXTURE_DEPTH16);

	if(patchSurfaceFormat)		// I think this is only used for debugging anyway
	{
		grcAssertf(!is16BitDepth, "cannot patch the 16 bit depth buffers");
		target->SetSurfaceFormat(CELL_GCM_SURFACE_A8R8G8B8);
	}

	gcmTexture.format = static_cast<u8>(((is16BitDepth) ? CELL_GCM_TEXTURE_X16 : CELL_GCM_TEXTURE_A8R8G8B8) | colorFlags );
	if (!is16BitDepth)
	{
		gcmTexture.remap =  CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
							CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
							CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
							CELL_GCM_TEXTURE_REMAP_REMAP << 8  |
							CELL_GCM_TEXTURE_REMAP_FROM_G << 6 |
							CELL_GCM_TEXTURE_REMAP_FROM_R << 4 |
							CELL_GCM_TEXTURE_REMAP_FROM_A << 2 |
							CELL_GCM_TEXTURE_REMAP_FROM_B;
	}

	target->UpdatePackedTexture();
}

grcTextureFactoryGCM::grcTextureFactoryGCM()
{
#if __PPU
	// Reset some static stuff
	grcRenderTargetGCM::sm_BitFieldTiledMemory.Reset();
	grcRenderTargetGCM::sm_BitFieldZCulling.Reset();
	grcRenderTargetGCM::sm_TiledMemorySize = 0;
	grcRenderTargetGCM::sm_ZCullStart = 0;
	for (u32 i = 0; i < grcmrtCount; ++i)
	{
		sm_DefaultRenderTargets[i].Resize(1);
		sm_DefaultRenderTargets[i].End() = NULL;
	}
#if HACK_GTA4
	g_DepthBackBuffer = NULL;
	g_DepthBuffer = NULL;
#endif // HACK_GTA4

	grcRenderTargetPoolMgr::Init();

	grcTextureFactory::CreateParams params;
	// Don't explicitly create a seperate non-MSAAed buffer for these render targets
	params.CreateAABuffer = false;
	params.Multisample = GRCDEVICE.GetMSAA();

	int width = g_WindowWidth;
	int height = g_WindowHeight;

#if HACK_MC4
	if (width == 1280 && PARAM_bb720pwidth.Get(width))
	{
		Assert(width <= 1280);
	}
	if (height == 720 && PARAM_bb720pheight.Get(height))
	{
		Assert(height <= 720);
	}
#endif // HACK_MC4

	//
	// let's call this prefab render targets
	//
	u32 bitsPerPixel;
	// Choose appropriate pixel format and bit depth
	if (!PARAM_hdr.Get())
	{
		params.Format = grctfA8R8G8B8;
		bitsPerPixel = 32;
	}
	else
	{
		params.Format =  grctfA16B16G16R16F;
		bitsPerPixel = 64;
	}

	// We never want the double buffered front buffers to be multisampled.
	params.Multisample = grcDevice::MSAA_None;
	params.IsSRGB = PARAM_srgb.Get();

	if (PARAM_fbcount.Get(g_FrontBufferCount))
	{
		Assert(g_FrontBufferCount <= 8);
	}

	grcRTPoolCreateParams poolParams;
	poolParams.m_Size = gcm::GetSharedSurfaceSize(
		g_WindowWidth,
		g_WindowHeight,
		bitsPerPixel,
		1U, // Mip count
		1U, // Face count
		params.InLocalMemory,
		params.InTiledMemory,
		params.IsSwizzled,
		false, // Is a cube map?
		params.UseHierZ,
		g_FrontBufferCount, // Number of surfaces (ie. 2 for double buffered)
		NULL);

	poolParams.m_HeapCount = 1;
	poolParams.m_InitOnCreate = false;  // calc pitch,etc from first target that uses the pool			
	poolParams.m_Alignment = gcm::GetSurfaceAlignment(params.InTiledMemory, params.UseHierZ);	
	poolParams.m_Tiled = params.InTiledMemory;
	poolParams.m_Zculled = params.UseHierZ;
	poolParams.m_PhysicalMem = params.InLocalMemory;

	m_FrontBufferMemoryPoolID = grcRenderTargetPoolMgr::CreatePool("FrontBufferPool", poolParams);

	for (u32 i = 0; i < g_FrontBufferCount; ++i)
	{
		char frontBufferName[16];
		formatf(frontBufferName, "Front Buffer %d", i);
		params.PoolID = m_FrontBufferMemoryPoolID;
		params.AllocateFromPoolOnCreate = true;
		g_FrontBuffer[i] = static_cast<grcRenderTargetGCM*>(CreateRenderTarget(frontBufferName, grcrtFrontBuffer, g_WindowWidth, g_WindowHeight, 32, &params));
	}   
	
	params.PoolID = kRTPoolIDInvalid;	// the depth targets aren't in a pool

	//
	// if the user wants 2x or 4x super-sampling create the right render target
	//
	switch (GRCDEVICE.GetMSAA())
	{
	case grcDevice::MSAA_Centered4xMS:
	case grcDevice::MSAA_Rotated4xMS:
	case grcDevice::MSAA_2xMS:
		{
			params.InTiledMemory = true;
			params.UseHierZ = true;
			params.Multisample = GRCDEVICE.GetMSAA();

			if (PARAM_nodepth.Get())
			{
				g_DepthBackBuffer = NULL;
			}
			else
			{
				g_DepthBackBuffer = static_cast<grcRenderTargetGCM*>(CreateRenderTarget("Depth Back Buffer", grcrtDepthBuffer, width, height, 32, &params));
				PatchShadowToDepthBuffer(g_DepthBackBuffer, false);
				SetDefaultRenderTarget(grcmrtDepthStencil, g_DepthBackBuffer);
			}
			g_DepthBuffer = NULL;

			// create back buffer texture
			g_BackBuffer = static_cast<grcRenderTargetGCM*>(CreateRenderTarget("Back Buffer", grcrtBackBuffer, width, height, 32, &params));

			SetDefaultRenderTarget(grcmrtColor0, g_BackBuffer);
		}
		break;
	default:
		{
			// create depth buffer texture
			if (PARAM_nodepth.Get())
			{
				g_DepthBuffer = NULL;
			}
			else
			{
				params.Multisample = grcDevice::MSAA_None;
				params.InTiledMemory = true;
				params.UseHierZ = true;

				g_DepthBuffer = static_cast<grcRenderTargetGCM*>(CreateRenderTarget("Depth Front Buffer", grcrtDepthBuffer, g_WindowWidth, g_WindowHeight, 32, &params));
				PatchShadowToDepthBuffer(g_DepthBuffer, false);

				if (GRCDEVICE.GetMSAA() == grcDevice::MSAA_None)
				{
					SetDefaultRenderTarget(grcmrtDepthStencil, g_DepthBuffer);
				}
			}
		}
		break;
	}
#endif // __PPU
}

grcTextureFactoryGCM::~grcTextureFactoryGCM()
{
#if __PPU
	for (u32 i = 0; i < g_FrontBufferCount; ++i)
	{
		SafeRelease(g_FrontBuffer[i]);
	}
	SafeRelease(g_BackBuffer);
	SafeRelease(g_DepthBuffer);
	SafeRelease(g_DepthBackBuffer);

	grcRenderTargetPoolMgr::DestroyPool(m_FrontBufferMemoryPoolID);
#endif // __PPU
}

void grcTextureFactoryGCM::BindDefaultRenderTargets()
{
#if __PPU
	for (u32 mrt = 0; mrt < grcmrtColorCount; ++mrt)
	{
		Assert(!sm_DefaultRenderTargets[mrt].IsEmpty());
		m_LockColorRenderTargets[mrt] = sm_DefaultRenderTargets[mrt].End();
	}
	Assert(!sm_DefaultRenderTargets[grcmrtDepthStencil].IsEmpty());
	m_LockDepthRenderTarget = sm_DefaultRenderTargets[grcmrtDepthStencil].End();

	GRCDEVICE.SetRenderTargets(m_LockColorRenderTargets, m_LockDepthRenderTarget);
#endif // __PPU
}

grcTextureGCM::grcTextureGCM(const char *pFilename,grcTextureFactory::TextureCreateParams *params)
{
	grcImage* pImage        = NULL;
	bool      bImageCreated = false;
#if __RESOURCECOMPILER
	void*     pProcessProxy = NULL;
#endif

#if __RESOURCECOMPILER
	sysMemStartTemp();		
#endif
	if (strcmp(pFilename, "none") && strcmp(pFilename, "nonresident"))
	{
#if __RESOURCECOMPILER
		grcImage::ImageList images;
		bool result = grcImage::RunCustomLoadFunc( pFilename, images, &pProcessProxy );
		if ( result )
			pImage = images[1];

		if (!result || pImage == NULL) // try loading normally
#endif // __RESOURCECOMPILER
		{
			pImage = grcImage::Load(pFilename);
			bImageCreated = true;
		}
	}
#if __RESOURCECOMPILER
	sysMemEndTemp();		
#endif

	if (pImage)
	{
		const int w = pImage->GetWidth();
		const int h = pImage->GetHeight();

		if ((w|h) & 3)
		{
			grcErrorf("grcTextureGCM - Texture '%s' - invalid resolution %d by %d", pFilename, w, h);

			if (bImageCreated)
			{
				sysMemStartTemp();
				pImage->Release();
				bImageCreated = false;
				sysMemEndTemp();
			}

			pImage = NULL;
#if __RESOURCECOMPILER
			pProcessProxy = NULL;
#endif
		}
	}

	if (!pImage)
	{
		const u32 texel = strcmp(pFilename, "nonresident") ? 0xFFFFFFFF : 0x40FFFFFF;
		const int dummySize = 4;

		sysMemStartTemp();
		pImage = grcImage::Create(dummySize, dummySize, 1, grcImage::A8R8G8B8, grcImage::STANDARD, 0, 0);
		bImageCreated = true;
		sysMemEndTemp();

		u32 *texels = (u32*) pImage->GetBits();

		for (int i = 0; i < dummySize*dummySize; i++)
		{
			texels[i] = texel;
		}
	}

	Init(pFilename, pImage, params);
#if __RESOURCECOMPILER
	RunCustomProcessFunc( pProcessProxy );
#endif // __RESOURCECOMPILER

	if (bImageCreated)
	{
		sysMemStartTemp();
		pImage->Release();
		bImageCreated = false;
		sysMemEndTemp();
	}
}

grcTextureGCM::grcTextureGCM(grcImage *pImage,grcTextureFactory::TextureCreateParams *params)
{
	const char* name = "image";
#if __BANK || __RESOURCECOMPILER
	name = grcTexture::GetCustomLoadName(name);
#endif // __BANK || __RESOURCECOMPILER
	Init(name,pImage,params);
}

grcTextureGCM::grcTextureGCM(u32 width,u32 height,u32 format,void *pBuffer, u32 mipLevels, grcTextureFactory::TextureCreateParams *params)
{
	Init(width, height, format, pBuffer, mipLevels, params);
}


grcTextureGCM::~grcTextureGCM()
{
	delete [] m_MemoryOffsets;
	gcm::FreePtr(GetBits());
}

class Swizzler 
{
public:

	// Dimensions of the texture
	u32 m_Width;
	u32 m_Height;
	u32 m_Depth; 

	// Internal mask for each coordinate
	u32 m_MaskU;
	u32 m_MaskV;
	u32 m_MaskW; 

	// Swizzled texture coordinates
	u32 m_u;
	u32 m_v;
	u32 m_w;     

	Swizzler(): m_Width(0), m_Height(0), m_Depth(0),
		m_MaskU(0), m_MaskV(0), m_MaskW(0),
		m_u(0), m_v(0), m_w(0)
	{ }

	// Initializes the swizzler
	Swizzler(
		u32 width, 
		u32 height, 
		u32 depth
		)
	{ 
		Init(width, height, depth);
	}

	void Init(
		u32 width,
		u32 height,
		u32 depth
		)
	{
		m_Width = width; 
		m_Height = height; 
		m_Depth = depth;
		m_MaskU = 0;
		m_MaskV = 0;
		m_MaskW = 0;
		m_u = 0;
		m_v = 0;
		m_w = 0;

		u32 i = 1;
		u32 j = 1;
		u32 k;

		do 
		{
			k = 0;
			if (i < width)   
			{ 
				m_MaskU |= j;   
				k = (j<<=1);  
			}

			if (i < height)  
			{ 
				m_MaskV |= j;   
				k = (j<<=1);  
			}

			if (i < depth)   
			{
				m_MaskW |= j;   
				k = (j<<=1);  
			}

			i <<= 1;
		} 
		while (k);
	}

	// Swizzles a texture coordinate
	u32 SwizzleU( 
		u32 num 
		)
	{
		u32 r = 0;

		for (u32 i = 1; i <= m_MaskU; i <<= 1) 
		{
			if (m_MaskU & i) 
			{
				r |= (num & i);
			}
			else
			{
				num <<= 1;
			}
		}

		return r;
	}

	u32 SwizzleV( 
		u32 num 
		)
	{
		u32 r = 0;

		for (u32 i = 1; i <= m_MaskV; i <<= 1) 
		{
			if (m_MaskV & i)
			{
				r |= (num & i);
			}
			else
			{
				num <<= 1;
			}
		}

		return r;
	}

	u32 SwizzleW( 
		u32 num 
		)
	{
		u32 r = 0;

		for (u32 i = 1; i <= m_MaskW; i <<= 1) 
		{
			if (m_MaskW & i)
			{
				r |= (num & i);
			}
			else
			{
				num <<= 1;
			}
		}

		return r;
	}

	u32 Swizzle(
		u32 u, 
		u32 v, 
		u32 w
		)
	{
		return SwizzleU(u) | SwizzleV(v) | SwizzleW(w);
	}
};

template <typename _Type> void SwizzleType(_Type* dest,const _Type *src,int width,int height,int depth) {
	Swizzler S(width,height,depth);
#if __ASSERT
	int size = width*height*depth;
#endif
	for (int z=0; z<depth; z++) {
		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				int offset = S.Swizzle(x,y,z);
				Assert(offset>=0 && offset <size);
				dest[offset] = *src++;
			}
		}
	}
}

static void SwizzleTexture(void *dest,const void *src,int width,int height,int depth,int bpp) {
	// Swizzling is a recursive Z-stroke algorithm
	// 00 01 04 05 10 11 14 15
	// 02 03 06 07 12 13 16 17
	// 08 09 0c 0d 18 19 1c 1d
	// 0a 0b 0e 0f 1a 1b 1e 1f
	// 20 21 24 25 30 31 34 35
	// 22 23 26 27 32 33 36 37
	// 28 29 2c 2d 38 39 3c 3d
	// 2a 2b 2e 2f 3a 3b 3e 3f
	if (bpp == 16)
		SwizzleType<u128>((u128*)dest,(u128*)src,width,height,depth);
	else if (bpp == 8)
		SwizzleType<u64>((u64*)dest,(u64*)src,width,height,depth);
	else if (bpp == 4)
		SwizzleType<u32>((u32*)dest,(u32*)src,width,height,depth);
	else if (bpp == 2)
		SwizzleType<u16>((u16*)dest,(u16*)src,width,height,depth);
	else if (bpp == 1)
		SwizzleType<u8>((u8*)dest,(u8*)src,width,height,depth);
	else
		Assert(0);
}

void grcTextureGCM::Resize(u32 width, u32 height)
{
	AssertMsg(m_Texture.mipmap == 1 && GetLayerCount() == 1 && m_Texture.dimension == CELL_GCM_TEXTURE_DIMENSION_2,
		"Currently this func does not support multiple layers or mipmaps");

	m_Texture.width = (u16) width;
	m_Texture.height = (u16) height;

	RecalculatePitch();
}

// ================================================================================================

static u32 GetGCMTextureRemapFromImageFormat(grcImage::Format format)
{
	switch (format)
	{
#if 0 && __RESOURCECOMPILER // why is this only for the resource compiler?
	case grcImage::A8R8G8B8 : return CELL_GCM_TEXTURE_REMAP(A,R,G,B);
#endif
	case grcImage::A8B8G8R8 : return CELL_GCM_TEXTURE_REMAP(R,G,B,A);
	case grcImage::L8       : return CELL_GCM_TEXTURE_REMAP(B,B,B,1);
	case grcImage::A8       : return CELL_GCM_TEXTURE_REMAP(0,0,0,B);
	case grcImage::R8       : return CELL_GCM_TEXTURE_REMAP(0,0,B,1);
	case grcImage::A8L8     : return CELL_GCM_TEXTURE_REMAP(B,B,B,G);
	case grcImage::G8R8     : return CELL_GCM_TEXTURE_REMAP(0,G,B,1);
	case grcImage::L16      : return CELL_GCM_TEXTURE_REMAP(B,G,R,A) | (CELL_GCM_TEXTURE_REMAP_ORDER_XXXY << 16);
	default                 : return CELL_GCM_TEXTURE_REMAP(B,G,R,A);
	}

	//i don't understand this ... the c'tor grcRenderTargetGCM uses other texture remapping ..
	//{
	//	switch (textureFormat)
	//	{
	//	case CELL_GCM_TEXTURE_A8R8G8B8  : RGBA
	//	case CELL_GCM_TEXTURE_X32_FLOAT : BGR1
	//	case CELL_GCM_TEXTURE_B8        : BBB1
	//	default                         : BGRA
	//	}
	//}
}

static u32 GetGCMTextureFormatFromImageFormat(grcImage::Format format)
{
	switch (format)
	{
	case grcImage::UNKNOWN                     : return 0                                     ;
	case grcImage::DXT1                        : return CELL_GCM_TEXTURE_COMPRESSED_DXT1      ;
	case grcImage::DXT3                        : return CELL_GCM_TEXTURE_COMPRESSED_DXT23     ;
	case grcImage::DXT5                        : return CELL_GCM_TEXTURE_COMPRESSED_DXT45     ;
	case grcImage::CTX1                        : return 0                                     ;
	case grcImage::DXT3A                       : return 0                                     ;
	case grcImage::DXT3A_1111                  : return 0                                     ;
	case grcImage::DXT5A                       : return 0                                     ;
	case grcImage::DXN                         : return 0                                     ;
	case grcImage::BC6                         : return 0                                     ;
	case grcImage::BC7                         : return 0                                     ;
	case grcImage::A8R8G8B8                    : return CELL_GCM_TEXTURE_A8R8G8B8             ;
	case grcImage::A8B8G8R8                    : return CELL_GCM_TEXTURE_A8R8G8B8             ;
	case grcImage::A8                          : return CELL_GCM_TEXTURE_B8                   ;
	case grcImage::L8                          : return CELL_GCM_TEXTURE_B8                   ;
	case grcImage::A8L8                        : return CELL_GCM_TEXTURE_G8B8                 ;
	case grcImage::A4R4G4B4                    : return CELL_GCM_TEXTURE_A4R4G4B4             ;
	case grcImage::A1R5G5B5                    : return CELL_GCM_TEXTURE_A1R5G5B5             ;
	case grcImage::R5G6B5                      : return CELL_GCM_TEXTURE_R5G6B5               ;
	case grcImage::R3G3B2                      : return 0                                     ;
	case grcImage::A8R3G3B2                    : return 0                                     ;
	case grcImage::A4L4                        : return 0                                     ;
	case grcImage::A2R10G10B10                 : return 0                                     ;
	case grcImage::A2B10G10R10                 : return 0                                     ;
	case grcImage::A16B16G16R16                : return 0                                     ;
	case grcImage::G16R16                      : return CELL_GCM_TEXTURE_Y16_X16              ;
	case grcImage::L16                         : return CELL_GCM_TEXTURE_X16                  ;
	case grcImage::A16B16G16R16F               : return CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT;
	case grcImage::G16R16F                     : return CELL_GCM_TEXTURE_Y16_X16_FLOAT        ;
	case grcImage::R16F                        : return 0                                     ;
	case grcImage::A32B32G32R32F               : return CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT;
	case grcImage::G32R32F                     : return 0                                     ;
	case grcImage::R32F                        : return CELL_GCM_TEXTURE_X32_FLOAT            ;
	case grcImage::D15S1                       : return 0                                     ;
	case grcImage::D24S8                       : return 0                                     ;
	case grcImage::D24FS8                      : return 0                                     ;
	case grcImage::P4                          : return 0                                     ;
	case grcImage::P8                          : return 0                                     ;
	case grcImage::A8P8                        : return 0                                     ;
	case grcImage::R8                          : return CELL_GCM_TEXTURE_B8                   ;
	case grcImage::R16                         : return CELL_GCM_TEXTURE_X16                  ;
	case grcImage::G8R8                        : return CELL_GCM_TEXTURE_G8B8                 ;
	case grcImage::LINA32B32G32R32F_DEPRECATED : return CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT;
	case grcImage::LINA8R8G8B8_DEPRECATED      : return CELL_GCM_TEXTURE_A8R8G8B8             ;
	case grcImage::LIN8_DEPRECATED             : return CELL_GCM_TEXTURE_B8                   ;
	case grcImage::RGBE                        : return CELL_GCM_TEXTURE_A8R8G8B8             ;
	}

	// not used:
	// CELL_GCM_TEXTURE_R6G5B5
	// CELL_GCM_TEXTURE_R5G5B5A1
	// CELL_GCM_TEXTURE_D1R5G5B5
	// CELL_GCM_TEXTURE_D8R8G8B8
	// CELL_GCM_TEXTURE_DEPTH16
	// CELL_GCM_TEXTURE_DEPTH16_FLOAT
	// CELL_GCM_TEXTURE_DEPTH24_D8
	// CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT
	// CELL_GCM_TEXTURE_COMPRESSED_HILO8
	// CELL_GCM_TEXTURE_COMPRESSED_HILO_S8
	// CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8
	// CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8

	return 0;
}

u32 grcTextureGCM::GetImageFormat() const
{
	u32 currentGPUFormat = m_Texture.format & 0x9F;

	// maybe handle these eventually ..
	if ((m_Texture.format & 0xBF) == CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8 ||
		(m_Texture.format & 0xBF) == CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8)
	{
		currentGPUFormat = (m_Texture.format & 0xBF);
	}

	if (currentGPUFormat == CELL_GCM_TEXTURE_B8) // handle ambiguous cases which can only be resolved by swizzle
	{
		switch (m_Texture.remap & 0xffff)
		{
		case CELL_GCM_TEXTURE_REMAP(B,B,B,1): return grcImage::L8;
		case CELL_GCM_TEXTURE_REMAP(0,0,0,B): return grcImage::A8;
		case CELL_GCM_TEXTURE_REMAP(0,0,B,1): return grcImage::R8;
		}
	}
	else if (currentGPUFormat == CELL_GCM_TEXTURE_G8B8)
	{
		switch (m_Texture.remap & 0xffff)
		{
		case CELL_GCM_TEXTURE_REMAP(B,B,B,G): return grcImage::A8L8;
		case CELL_GCM_TEXTURE_REMAP(0,G,B,1): return grcImage::G8R8;
		}
	}
	else if (currentGPUFormat == CELL_GCM_TEXTURE_X16)
	{
		switch (m_Texture.remap >> 16)
		{
		case CELL_GCM_TEXTURE_REMAP_ORDER_XYXY: return grcImage::R16;
		case CELL_GCM_TEXTURE_REMAP_ORDER_XXXY: return grcImage::L16;
		}
	}

	switch (currentGPUFormat)
	{
//	case 0                                      : return grcImage::UNKNOWN                    ;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1       : return grcImage::DXT1                       ;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23      : return grcImage::DXT3                       ;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45      : return grcImage::DXT5                       ;
//	case 0                                      : return grcImage::CTX1                       ;
//	case 0                                      : return grcImage::DXT3A                      ;
//	case 0                                      : return grcImage::DXT3A_1111                 ;
//	case 0                                      : return grcImage::DXT5A                      ;
//	case 0                                      : return grcImage::DXN                        ;
	case CELL_GCM_TEXTURE_A8R8G8B8              : return grcImage::A8R8G8B8                   ;
//	case CELL_GCM_TEXTURE_A8R8G8B8              : return grcImage::A8B8G8R8                   ;
//	case CELL_GCM_TEXTURE_B8                    : return grcImage::A8                         ;
	case CELL_GCM_TEXTURE_B8                    : return grcImage::L8                         ; // .. could be L8, A8 or R8
	case CELL_GCM_TEXTURE_G8B8                  : return grcImage::A8L8                       ; // .. could be A8L8 or G8R8
	case CELL_GCM_TEXTURE_A4R4G4B4              : return grcImage::A4R4G4B4                   ;
	case CELL_GCM_TEXTURE_A1R5G5B5              : return grcImage::A1R5G5B5                   ;
	case CELL_GCM_TEXTURE_R5G6B5                : return grcImage::R5G6B5                     ;
//	case 0                                      : return grcImage::R3G3B2                     ;
//	case 0                                      : return grcImage::A8R3G3B2                   ;
//	case 0                                      : return grcImage::A4L4                       ;
//	case 0                                      : return grcImage::A2R10G10B10                ;
//	case 0                                      : return grcImage::A2B10G10R10                ;
//	case 0                                      : return grcImage::A16B16G16R16               ;
	case CELL_GCM_TEXTURE_Y16_X16               : return grcImage::G16R16                     ;
	case CELL_GCM_TEXTURE_X16                   : return grcImage::L16                        ; // .. could be L16 or R16
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT : return grcImage::A16B16G16R16F              ;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT         : return grcImage::G16R16F                    ;
//	case 0                                      : return grcImage::R16F                       ;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT : return grcImage::A32B32G32R32F              ;
//	case 0                                      : return grcImage::G32R32F                    ;
	case CELL_GCM_TEXTURE_X32_FLOAT             : return grcImage::R32F                       ;
//	case 0                                      : return grcImage::D15S1                      ;
//	case 0                                      : return grcImage::D24S8                      ;
//	case 0                                      : return grcImage::D24FS8                     ;
//	case 0                                      : return grcImage::P4                         ;
//	case 0                                      : return grcImage::P8                         ;
//	case 0                                      : return grcImage::A8P8                       ;
//	case CELL_GCM_TEXTURE_B8                    : return grcImage::R8                         ;
//	case CELL_GCM_TEXTURE_X16                   : return grcImage::R16                        ;
//	case CELL_GCM_TEXTURE_G8B8                  : return grcImage::G8R8                       ;
//	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT : return grcImage::LINA32B32G32R32F_DEPRECATED;
//	case CELL_GCM_TEXTURE_A8R8G8B8              : return grcImage::LINA8R8G8B8_DEPRECATED     ;
//	case CELL_GCM_TEXTURE_B8                    : return grcImage::LIN8_DEPRECATED            ;
//	case CELL_GCM_TEXTURE_A8R8G8B8              : return grcImage::RGBE                       ;
	}

	return grcImage::UNKNOWN;
}

void grcTextureGCM::Init(const char * filename,grcImage *image,const grcTextureFactory::TextureCreateParams* params)
{
	m_Name = grcSaveTextureNames ? StringDuplicate(filename) : NULL;
	m_CachedTexturePtr = &m_Texture;

	// format filled out below
	m_Texture.mipmap = static_cast<u8>(image->GetExtraMipCount() + 1);
	switch (image->GetType())
	{
	case grcImage::CUBE:
		m_Texture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
		m_LayerCount = 5;
		m_Texture.cubemap = CELL_GCM_TRUE;
		break;
	case grcImage::STANDARD:
	case grcImage::DEPTH:
		m_Texture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
		m_LayerCount = 0;
		m_Texture.cubemap = CELL_GCM_FALSE;
		break;
	case grcImage::VOLUME:
		m_Texture.dimension = CELL_GCM_TEXTURE_DIMENSION_3;
		m_LayerCount = 0;
		m_Texture.cubemap = CELL_GCM_FALSE;
		break;
	default:
		grcErrorf("Invalid image type");
		m_Texture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
		m_LayerCount = 0;
		m_Texture.cubemap = CELL_GCM_FALSE;
		break;
	}
	m_MemoryOffsets = rage_new u32[m_Texture.mipmap * GetLayerCount()];
	m_Texture.width = (u16) image->GetWidth();
	m_Texture.height = (u16) image->GetHeight();
	m_Texture.depth = (u16) image->GetDepth();

	// ============================================================================================

	bool bDynamic  = (params && params->Memory == grcTextureFactory::TextureCreateParams::SYSTEM) || image->IsSysMem() || PARAM_texturesinsystemmemory.Get();
	bool bLinear   = (params && params->Format == grcTextureFactory::TextureCreateParams::LINEAR) || image->IsLinear();
	bool bPowerOf2 = _IsPowerOfTwo(m_Texture.width) && _IsPowerOfTwo(m_Texture.height) && _IsPowerOfTwo(m_Texture.depth);

	grcImage::Format format = image->GetFormat();

	if (grcImage::IsFormatDXTBlockCompressed(format) && !bPowerOf2)
	{
		bLinear = true;
	}

	switch ((u32)format) // handle funky formats .. these should go away soon maybe
	{
	case grcImage::LINA32B32G32R32F_DEPRECATED : format = grcImage::A32B32G32R32F; bLinear            = true; break;
	case grcImage::LINA8R8G8B8_DEPRECATED      : format = grcImage::A8R8G8B8     ; bLinear = bDynamic = true; break;
	case grcImage::LIN8_DEPRECATED             : format = grcImage::L8           ; bLinear            = true; break;
	}

	m_Texture.remap   = GetGCMTextureRemapFromImageFormat(format);
	m_Texture.format  = (u8)GetGCMTextureFormatFromImageFormat(format);
	m_Texture.format |= (bLinear ? CELL_GCM_TEXTURE_LN : CELL_GCM_TEXTURE_SZ);
	m_Texture.format |= CELL_GCM_TEXTURE_NR; // All textures currently use normalised texture coordinates

	RecalculatePitch();

#if __BANK_TEXTURE_CONTROL
	if (gTextureControl.m_enabled)
	{
		if (1)
		{
			gTextureControl.DisplayFormat(image, m_Texture.format, m_Texture.remap);
		}

		u32 temp_format = m_Texture.format;
		u32 temp_remap  = m_Texture.remap;

		if (gTextureControl.UpdateFormat(image, temp_format, temp_remap))
		{
			m_Texture.format = (u8)temp_format;
			m_Texture.remap  = temp_remap;
		}
	}
#endif // __BANK_TEXTURE_CONTROL

	// ============================================================================================

	SetPhysicalSize(0);
	
	int memSize = gcm::GetTextureSize(	m_Texture.width, 
										m_Texture.height, 
										m_Texture.pitch, 
										GetBitsPerPixel(), 
										m_Texture.mipmap, 
										m_LayerCount+1, 
										!bDynamic, 
										!bLinear, 
										m_Texture.cubemap == CELL_GCM_TRUE, 
										m_MemoryOffsets,
										GetLinesPerPitch());
	SetPhysicalSize(memSize);
	
	
	if (bDynamic)
	{
		// dynamic textures might be vertex textures, which must always be 128b aligned.
		void* bits;
		if (params && params->Buffer)
		{
			//use client/caller managed memory
			bits = params->Buffer;
		}
		else
		{
			//allocate new memory
			bits = rage_aligned_new (128) char[GetPhysicalSize()];
		}

		Assertf(bits, "Could not allocate main memory for texture \"%s\"", filename);
#if __PPU
		Assertf(gcm::IsMainPtr(bits), "Invalid main pointer- %d bytes @ 0x%p", GetPhysicalSize(), bits);
		m_Texture.offset = gcm::MainOffset(bits);
#else
		m_Texture.offset = reinterpret_cast<u32>(bits);
#endif // __PPU
		m_Texture.location = CELL_GCM_LOCATION_MAIN;
	}
	else
	{
		void* bits;
		if (params && params->Buffer)
		{
			//use client/caller managed memory
			bits = params->Buffer;			
		}
		else
		{
			//allocate new memory
			bits = physical_new(GetPhysicalSize(), 128);
		}

		Assertf(bits, "Could not allocate local memory for texture \"%s\"", filename);
#if __PPU
		Assertf(gcm::IsLocalPtr(bits), "Invalid local pointer- %d bytes @ 0x%p", GetPhysicalSize(), bits);
		m_Texture.offset = gcm::LocalOffset(bits);
#else
		m_Texture.offset = reinterpret_cast<u32>(bits);
#endif // __PPU
		m_Texture.location = CELL_GCM_LOCATION_LOCAL;
	}

#if __PPU
	// Add base memory offset
	for (u32 mipIdx = 0; mipIdx < m_Texture.mipmap; ++mipIdx)
		for (u32 layerIdx = 0; layerIdx < GetLayerCount(); ++layerIdx)
			m_MemoryOffsets[mipIdx * GetLayerCount() + layerIdx] += m_Texture.offset;
#endif // __PPU

	SetGammaEnabled(image->IsSRGB());

	Copy(image);

	UpdatePackedTexture();
}

void grcTextureGCM::Init(u32 width, u32 height, u32 imgFormat, void* pBuffer, u32 mipLevels, grcTextureFactory::TextureCreateParams * params)
{
	Assertf(pBuffer, "Buffer cannot be NULL");

	m_Name = NULL;
	m_CachedTexturePtr = &m_Texture;

	// format filled out below
	
	m_Texture.mipmap = (u8)mipLevels;
	m_Texture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	m_LayerCount = 0;
	m_Texture.cubemap = CELL_GCM_FALSE;

	m_MemoryOffsets = rage_new u32[m_Texture.mipmap * GetLayerCount()];
	m_Texture.width = (u16) width;
	m_Texture.height = (u16) height;
	m_Texture.depth = (u16) 1;

	// ============================================================================================

	bool bDynamic  = (params && params->Memory == grcTextureFactory::TextureCreateParams::SYSTEM);
	bool bLinear   = (params && params->Format == grcTextureFactory::TextureCreateParams::LINEAR);
	bool bPowerOf2 = _IsPowerOfTwo(m_Texture.width) && _IsPowerOfTwo(m_Texture.height) && _IsPowerOfTwo(m_Texture.depth);

	grcImage::Format format = (grcImage::Format)imgFormat;

	if (grcImage::IsFormatDXTBlockCompressed(format) && !bPowerOf2)
	{
		bLinear = true;
	}

	switch ((u32)format) // handle funky formats .. these should go away soon maybe
	{
	case grcImage::LINA32B32G32R32F_DEPRECATED : format = grcImage::A32B32G32R32F; bLinear            = true; break;
	case grcImage::LINA8R8G8B8_DEPRECATED      : format = grcImage::A8R8G8B8     ; bLinear = bDynamic = true; break;
	case grcImage::LIN8_DEPRECATED             : format = grcImage::L8           ; bLinear            = true; break;
	}

	m_Texture.remap   = GetGCMTextureRemapFromImageFormat(format);
	m_Texture.format  = (u8)GetGCMTextureFormatFromImageFormat(format);
	m_Texture.format |= (bLinear ? CELL_GCM_TEXTURE_LN : CELL_GCM_TEXTURE_SZ);
	m_Texture.format |= CELL_GCM_TEXTURE_NR; // All textures currently use normalised texture coordinates

	RecalculatePitch();

	// ============================================================================================

	SetPhysicalSize(0);

	int memSize = gcm::GetTextureSize(	m_Texture.width, 
		m_Texture.height, 
		m_Texture.pitch, 
		GetBitsPerPixel(), 
		m_Texture.mipmap, 
		m_LayerCount+1, 
		!bDynamic, 
		!bLinear, 
		m_Texture.cubemap == CELL_GCM_TRUE, 
		m_MemoryOffsets,
		GetLinesPerPitch());
	SetPhysicalSize(memSize);


	// dynamic textures might be vertex textures, which must always be 128b aligned.
	void* bits = pBuffer;

	if (bDynamic)
	{
#if __PPU
		Assertf(gcm::IsMainPtr(bits), "Invalid main pointer- %d bytes @ 0x%p", GetPhysicalSize(), bits);
		m_Texture.offset = gcm::MainOffset(bits);
#else
		m_Texture.offset = reinterpret_cast<u32>(bits);
#endif // __PPU
		m_Texture.location = CELL_GCM_LOCATION_MAIN;
	}
	else
	{
#if __PPU
		Assertf(gcm::IsLocalPtr(bits), "Invalid local pointer- %d bytes @ 0x%p", GetPhysicalSize(), bits);
		m_Texture.offset = gcm::LocalOffset(bits);
#else
		m_Texture.offset = reinterpret_cast<u32>(bits);
#endif // __PPU
		m_Texture.location = CELL_GCM_LOCATION_LOCAL;
	}

#if __PPU
	// Add base memory offset
	for (u32 mipIdx = 0; mipIdx < m_Texture.mipmap; ++mipIdx)
		for (u32 layerIdx = 0; layerIdx < GetLayerCount(); ++layerIdx)
			m_MemoryOffsets[mipIdx * GetLayerCount() + layerIdx] += m_Texture.offset;
#endif // __PPU

	SetGammaEnabled(false);

	UpdatePackedTexture();
}

static inline u32 Pad_uint(u32 x, u32 pad)
{
	Assert(pad != 0);
	return (x + pad - 1) / pad * pad;
}

// Code to perform Volume Texture Compression (VTC) adapted from the Cell SDK-
// \usr\local\cell\samples\gtf\dds2gtf\libdds2gtf
static u32 CopyS3TCVolumeTexture(void* destinationBuffer, const grcImage* image)
{
	Assert(image->GetType() == grcImage::VOLUME);

	u32 offset = 0;
	u8* destinationBufferAsU8 = reinterpret_cast<u8*>(destinationBuffer);

	u32 width  = image->GetWidth();
	u32 height = image->GetHeight();
	u32 depth  = image->GetDepth();
	const u32 mipCount = static_cast<u32>(image->GetExtraMipCount()) + 1;

	// Should we perform VTC?
	const bool isDxt1 = image->GetFormat() == grcImage::DXT1;
	Assert(isDxt1 || image->GetFormat() == grcImage::DXT3 || image->GetFormat() == grcImage::DXT5);

	const bool isDepthAPowerOf2 = _IsPowerOfTwo(image->GetDepth());

	if (_IsPowerOfTwo(image->GetWidth()) && _IsPowerOfTwo(image->GetHeight()) && isDepthAPowerOf2)
	{
		u32 VtcOuterLoopCount = Pad_uint(depth, 4) / 4;
		u32 VtcInnerLoopCount = depth % 4;
		if (VtcInnerLoopCount == 0)
		{
			VtcInnerLoopCount = 4;
		}	

		const u32 blockSize = isDxt1 ? 8 : 16;

		for (u32 mip = 0; mip < mipCount; ++mip)
		{
			// S3TC compressed texture 
			// VTC is either: 4x4x1 or 4x4x2 or 4x4x4.
			Assert(VtcInnerLoopCount == 1 || VtcInnerLoopCount == 2 || VtcInnerLoopCount == 4);

			for (u32 zOuter = 0; zOuter < VtcOuterLoopCount; ++zOuter)
			{
				// NOTE: blockSize copy
				//
				//     If DXT1, copy 8 bytes/block.
				//     If DXT3/5, copy 16 bytes/block.
				//
				const u32 DxtcWidth  = (Pad_uint(width, 4) + 3) / 4;
				const u32 DxtcHeight = (Pad_uint(height, 4) + 3) / 4;

				for (u32 y = 0; y < DxtcHeight; ++y)
				{
					for (u32 x = 0; x < DxtcWidth; ++x)
					{
						for (u32 zInner = 0; zInner < VtcInnerLoopCount; ++zInner)
						{
							u8* sourceBufferAsU8 = image->GetBits(zInner + zOuter * VtcInnerLoopCount, mip);
							sourceBufferAsU8 += blockSize * (x + y * DxtcWidth);

							memcpy(&destinationBufferAsU8[offset], sourceBufferAsU8, blockSize);
							offset += blockSize;
						}
					}
				}
			}

			if (width != 1) 
			{
				width /= 2;
			}
			if (height != 1)
			{
				height /= 2;
			}
			if (depth != 1)
			{
				depth /= 2;
				VtcOuterLoopCount = Pad_uint(depth, 4) / 4;
				VtcInnerLoopCount = depth % 4;
				if (VtcInnerLoopCount == 0)
				{
					VtcInnerLoopCount = 4;
				}	
			}
		}

		offset = Pad_uint(offset, 128); 
	}
	else if (isDepthAPowerOf2)
	{
		// NOTE: blockSize copy 
		//
		//     If DXT1, copy 8 bytes/block.
		//     If DXT3/5, copy 16 bytes/block.
		//
		const u32 stride = image->GetStride();
		const u32 blockSize = isDxt1 ? 8 : 16;
		const u32 pitchSize = (Pad_uint(stride, 4) + 3) / 4 * blockSize;

		for (u32 mip = 0; mip < mipCount; ++mip)
		{
			for (u32 z = 0; z < depth; ++z)
			{
				// get texture for current depth
				u8* sourceBufferAsU8 = image->GetBits(z, mip);

				u32 lineSize  = (Pad_uint(width, 4) + 3) / 4 * blockSize;
				u32 DxtcHeight = (Pad_uint(height, 4) + 3) / 4;

				for (u32 y = 0; y < DxtcHeight; ++y)
				{
					memcpy(&destinationBufferAsU8[offset], sourceBufferAsU8, lineSize);

					sourceBufferAsU8 += lineSize;
					offset += pitchSize;
				}

				// may not be necessary...
				offset = Pad_uint(offset, pitchSize);
			}

			// divide dimension to get to next mip level
			if (width != 1) 
			{
				width /= 2;
			}
			if (height != 1)
			{
				height /= 2;
			}
			if (depth != 1) 
			{
				depth /= 2;
			}
		} 

		offset = Pad_uint(offset, 128);
	}
	else
	{
		Quitf("Depth must be a power of 2 for DXT volume textures");
	}

	Assert((offset & 127) == 0);

	return offset;
}

bool grcTextureGCM::Copy(const grcImage *image)
{
	if (m_Texture.width != image->GetWidth()
		|| m_Texture.height != image->GetHeight()
		|| m_Texture.depth != image->GetDepth()
		|| m_Texture.mipmap != image->GetExtraMipCount()+1)
		return false;

	u8* bitsAsU8 = reinterpret_cast<u8*>(GetBits());
	const grcImage* layer = image;
	u32 size = 0;

	bool isS3tc = image->GetFormat() == grcImage::DXT1 ||
		image->GetFormat() == grcImage::DXT3 ||
		image->GetFormat() == grcImage::DXT5;

	if (image->GetType() == grcImage::VOLUME && isS3tc)
	{
		// Volume texture compression (VTC) for S3TC 3D textures
		CopyS3TCVolumeTexture(bitsAsU8, image);
	}
	else
	{
		bool isAPowerOf2 = _IsPowerOfTwo(m_Texture.width) && _IsPowerOfTwo(m_Texture.height) && _IsPowerOfTwo(m_Texture.depth);
		bool isLinear = (m_Texture.format & CELL_GCM_TEXTURE_LN) != 0;
		bool needsSwizzling = isAPowerOf2 && !isLinear && !isS3tc;
		// Note that this will be 0 if the texture is DXT - we don't use it anyway
		u32 bytesPerPixel = gcm::TextureFormatBitsPerPixel(m_Texture.format) >> 3;

		while (layer)
		{
			int level = 0;
			const grcImage* mip = layer;
			while (mip)
			{
				if (needsSwizzling)
				{
#if 1
					Assert(!isS3tc || bytesPerPixel == 0);
					SwizzleTexture(bitsAsU8+size,mip->GetBits(),mip->GetWidth(),mip->GetHeight(),mip->GetDepth(),bytesPerPixel);
#else
					cellGcmConvertSwizzleFormat(bitsAsU8 + size,
						mip->GetBits(),
						0,0,0,
						mip->GetWidth(),mip->GetHeight(),mip->GetDepth(),
						0,0,0,
						mip->GetWidth(),mip->GetHeight(),mip->GetDepth(),
						mip->GetWidth(),mip->GetHeight(),mip->GetDepth(),
						2,
						4,
						false,
						2,
						NULL);
#endif
				}
				else
				{
					/// SSSLLLOOOWWW
					memcpy(bitsAsU8 + size, mip->GetBits(), mip->GetSize());
				}

#if __RESOURCECOMPILER
				if (!grcImage::IsFormatDXTBlockCompressed(image->GetFormat()))
				{
					grcImage::ByteSwapData(bitsAsU8 + size, mip->GetSize(), bytesPerPixel);
				}
#endif // __RESOURCECOMPILER

				size += mip->GetSize();
				mip = mip->GetNext();
				++level;
			}
			if (m_Texture.cubemap)
			{
				size = (size+127) & ~127;
			}

			layer = layer->GetNextLayer();
		}

		if (needsSwizzling)
		{
			m_Texture.pitch = 0;
		}
	}

	return true;
}

bool grcTextureGCM::Copy2D(const void* pSrc, u32 imgFormat, u32 width, u32 height, u32 numMips)
{
	grcImage::Format format = (grcImage::Format)imgFormat;

	if (GetLayerCount() != 1U || format != (grcImage::Format)GetImageFormat() || m_Texture.width != width|| m_Texture.height != height || m_Texture.mipmap != numMips)
		return false;

	const u32 bpp		= grcImage::GetFormatBitsPerPixel(format);
	const u32 blockSize	= grcImage::IsFormatDXTBlockCompressed(format) ? 4 : 1;
	const bool isS3tc	= (format == grcImage::DXT1 || format == grcImage::DXT3 || format == grcImage::DXT5);

	bool isAPowerOf2 = _IsPowerOfTwo(m_Texture.width) && _IsPowerOfTwo(m_Texture.height) && _IsPowerOfTwo(m_Texture.depth);
	bool isLinear = (m_Texture.format & CELL_GCM_TEXTURE_LN) != 0;
	bool needsSwizzling = isAPowerOf2 && !isLinear && !isS3tc;

	// Note that this will be 0 if the texture is DXT - we don't use it anyway
	u32 bytesPerPixel = gcm::TextureFormatBitsPerPixel(m_Texture.format) >> 3;

	u32 mipWidth = Max<u32>(blockSize, width);
	u32 mipHeight = Max<u32>(blockSize, height);

	u32 mipPitch = mipWidth*bpp/8;

	u32 mipSizeInBytes = (mipHeight*mipPitch);
	u32 curMipOffset = 0;

	u8* bitsAsU8 = reinterpret_cast<u8*>(GetBits());

	const char* pSrcMip = static_cast<const char*>(pSrc);

	for (u32 i = 0; i < numMips; i++)
	{

		if (needsSwizzling)
		{
			Assert(!isS3tc || bytesPerPixel == 0);
			SwizzleTexture(bitsAsU8+curMipOffset, pSrcMip, mipWidth, mipHeight, 1, bytesPerPixel);
		}
		else
		{
			/// SSSLLLOOOWWW
			memcpy(bitsAsU8 + curMipOffset, pSrcMip, mipSizeInBytes);
		}

		mipWidth = Max<u32>(blockSize, mipWidth/2);
		mipHeight = Max<u32>(blockSize, mipHeight/2);

		mipPitch = mipWidth*bpp/8;

		curMipOffset += mipSizeInBytes;
		pSrcMip += mipSizeInBytes;
		mipSizeInBytes = (mipHeight*mipPitch);

	}

	if (needsSwizzling)
	{
		m_Texture.pitch = 0;
	}

	return true;

}

bool grcTextureGCM::LockRect(int layer, int mipLevel, grcTextureLock &lock, u32 ASSERT_ONLY(uLockFlags) ) const {
	Assert(layer >= 0 && (u32)layer < GetLayerCount());
	Assertf(m_Texture.location != CELL_GCM_LOCATION_LOCAL || (uLockFlags & grcsAllowVRAMLock), "Trying to lock VRAM texture '%s' - really slow!", GetName());
	
	bool isLocal = m_Texture.location == CELL_GCM_LOCATION_LOCAL;
	
	if (m_MemoryOffsets)
	{
		Assert(mipLevel >= 0 && mipLevel < m_Texture.mipmap);
		lock.Base = gcm::GetPtr(m_MemoryOffsets[mipLevel * GetLayerCount() + layer],isLocal);
	}
	else
	{
		Assert(mipLevel == 0);
		lock.Base = gcm::GetPtr(m_Texture.offset,isLocal);
	}

	lock.Width = Max<int>(m_Texture.width >> mipLevel, 1);
	lock.Height = Max<int>(m_Texture.height >> mipLevel, 1);
	lock.BitsPerPixel = gcm::TextureFormatBitsPerPixel(m_Texture.format);

	if ((m_Texture.format & CELL_GCM_TEXTURE_LN) == 0)
	{
		// Note that CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8 and CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8
		// are impossible here because CELL_GCM_TEXTURE_LN is always implied for them.
		u8 internalFormat = m_Texture.format & 0x9F;

		if (internalFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT1 || internalFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT23 || internalFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT45)
		{
			int blockSize = (internalFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16;
			lock.Pitch = (lock.Width + 3) / 4 * blockSize;
		}
		else
		{
			lock.Pitch = lock.Width * lock.BitsPerPixel / 8;
		}
	}
	else
	{
		lock.Pitch = m_Texture.pitch;
	}

	lock.RawFormat = m_Texture.format;
	lock.MipLevel = mipLevel;
	lock.Layer = layer;

	return true;
}

int grcTextureGCM::GetBitsPerPixel() const {
	return gcm::TextureFormatBitsPerPixel(m_Texture.format);
}

u32 grcTextureGCM::GetLinesPerPitch() const {
	return gcm::TextureFormatLinesPerPitch(m_Texture.format);
}

int grcRenderTargetGCM::GetBitsPerPixel() const {
	return gcm::TextureFormatBitsPerPixel(m_Texture.format);
}

void  grcTextureGCM::UnlockRect(const grcTextureLock &) const {
}

grcTextureGCM::grcTextureGCM(datResource &rsc) : grcTexture(rsc) {
	rsc.PointerFixup(m_Name);
	rsc.PointerFixup(m_MemoryOffsets);

	if (m_Texture.offset == 0)
	{
		if (m_Texture.location == CELL_GCM_LOCATION_LOCAL)
			m_Texture.offset = gcm::LocalOffset(m_MemoryOffsets);
		else
			m_Texture.offset = gcm::MainOffset(m_MemoryOffsets);

		m_MemoryOffsets = NULL;
	}
	else
	{
		// There's probably a more clever way of dealing with this, but it'll do for now 

		// Reset to a known state
		if (datResource_IsDefragmentation)
		{
			// Remove base offset
			for (u32 mipIdx = 0; mipIdx < m_Texture.mipmap; ++mipIdx)
				for (u32 layerIdx = 0; layerIdx < GetLayerCount(); ++layerIdx)
					m_MemoryOffsets[mipIdx * GetLayerCount() + layerIdx] -= m_Texture.offset;

			// Restore pointer
			if (m_Texture.location == CELL_GCM_LOCATION_LOCAL)
				m_Texture.offset = (u32)gcm::LocalPtr(m_Texture.offset);
			else
				m_Texture.offset = (u32)gcm::MainPtr(m_Texture.offset);
		}

		void* bits = reinterpret_cast<void*>(m_Texture.offset);
		rsc.PointerFixup(bits);

		if (m_Texture.location == CELL_GCM_LOCATION_LOCAL)
			m_Texture.offset = gcm::LocalOffset(bits);
		else
			m_Texture.offset = gcm::MainOffset(bits);

		// Add base offset
		for (u32 mipIdx = 0; mipIdx < m_Texture.mipmap; ++mipIdx)
			for (u32 layerIdx = 0; layerIdx < GetLayerCount(); ++layerIdx)
				m_MemoryOffsets[mipIdx * GetLayerCount() + layerIdx] += m_Texture.offset;
	}

	if (!datResource_IsDefragmentation)
		m_Texture._padding &= 15;			// clear off the damned texture conversion flags that didn't belong here in the first place

	m_CachedTexturePtr = GetTexturePtr();

	UpdatePackedTexture();
}

#if __DECLARESTRUCT && !__PPU
void grcTextureGCM::DeclareStruct(datTypeStruct &s) {
	// Make sure we precalculate this before we byte swap the CellGcmTexture member 'm_Texture' and grcTexture base class
	const u32 offsetCount = GetLayerCount() * m_Texture.mipmap;

	grcTexture::DeclareStruct(s);
	STRUCT_BEGIN(grcTextureGCM);
	STRUCT_DYNAMIC_ARRAY_NOCOUNT(m_MemoryOffsets, offsetCount);
	STRUCT_END();
}
#endif


void grcTexture::UpdatePackedTexture()
{
#if USE_PACKED_GCMTEX
	// This function is now mis-named!
	s_spuGcmState.PackedTextureArray[pgHandleBase::RegisterIndex(this)].PackFrom(GetGcmTexture());
#else
	const CellGcmTexture &src = GetGcmTexture();
	PackedCellGcmTexture p;
	p.PackFrom(src);
	CellGcmTexture test;
	p.UnpackTo(test);
	Assert(!memcmp(&src,&test,24));
#endif
}

void* grcTextureGCM::GetBits()
{
#if __PPU
	return gcm::GetTextureAddress(GetTexturePtr());
#else
	return reinterpret_cast<void*>(m_Texture.offset);
#endif // __GCM
}

#if __PPU
static void GpuMemCpy(void* dst, const void* src, size_t size)
{
	bool isDstLocal = gcm::IsLocalPtr(dst);
	bool isSrcLocal = gcm::IsLocalPtr(src);
	CellGcmEnum location;
	if (isDstLocal)
	{
		if (isSrcLocal)
		{
			location = CELL_GCM_TRANSFER_LOCAL_TO_LOCAL;
		}
		else
		{
			location = CELL_GCM_TRANSFER_MAIN_TO_LOCAL;
		}
	}
	else
	{
		if (isSrcLocal)
		{
			location = CELL_GCM_TRANSFER_LOCAL_TO_MAIN;
		}
		else
		{
			location = CELL_GCM_TRANSFER_MAIN_TO_MAIN;
		}
	}

	gcm::SetTransferData(GCM_CONTEXT,
		location, // mode
		gcm::GetOffset(dst), // dstOffset
		size, // dstPitch
		gcm::GetOffset(src), // srcOffset
		size, // srcPitch
		size, // bytesPerRow
		1 // rowCount
		);
}

void grcTextureGCM::RemoveTopMip()
{
	Assert(GetLayerCount() == 1);

	if (GetMipMapCount() == 1)
		return;

	u32 mipTailOffset = GetMemoryOffset(0, 1);
	u32 topLevelMipSize = mipTailOffset - GetMemoryOffset(0, 0);
	u32 mipTailSize = GetPhysicalSize() - topLevelMipSize;
	void* mipTail = (u8*)GetBits() + topLevelMipSize;

	int heapIndex = m_Texture.location == CELL_GCM_LOCATION_LOCAL ? MEMTYPE_RESOURCE_PHYSICAL : MEMTYPE_RESOURCE_VIRTUAL;
	sysMemAllocator* resourceAllocator = sysMemAllocator::GetCurrent().GetAllocator(heapIndex);
	void* newMipTail;
	if (resourceAllocator && resourceAllocator->IsValidPointer(GetBits()))
	{
		newMipTail = resourceAllocator->Allocate(mipTailSize, 128);
	}
	else
	{
		newMipTail = gcm::AllocatePtr(mipTailSize, 128);
	}

	GpuMemCpy(newMipTail, mipTail, mipTailSize);

	GRCDEVICE.BlockOnFence(GRCDEVICE.InsertFence());

	gcm::FreePtr(GetBits());

	for (u32 mipIdx = 0; mipIdx < m_Texture.mipmap - 1; ++mipIdx)
	{
		m_MemoryOffsets[mipIdx] = m_MemoryOffsets[mipIdx + 1] - m_Texture.offset;
	}

	m_Texture.offset = gcm::GetOffset(newMipTail);
	m_Texture.width = Max(m_Texture.width >> 1, 1);
	m_Texture.height = Max(m_Texture.height >> 1, 1);
	m_MemoryOffsets[--m_Texture.mipmap] = 0;

	for (u32 mipIdx = 0; mipIdx < m_Texture.mipmap; ++mipIdx)
	{
		m_MemoryOffsets[mipIdx] += m_Texture.offset;
	}

	UpdatePackedTexture();
}
#endif // __PPU

void grcTextureGCM::RecalculatePitch()
{
	if ((m_Texture.format & CELL_GCM_TEXTURE_LN) != 0)
	{
		switch (gcm::StripTextureFormat(m_Texture.format))
		{
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			m_Texture.pitch = ((m_Texture.width+1)/2) * 4;
			break;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
			m_Texture.pitch = ((m_Texture.width+3)/4) * 8;
			break;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
			m_Texture.pitch = ((m_Texture.width+3)/4) * 16;
			break;
		default:
			m_Texture.pitch = m_Texture.width * gcm::TextureFormatBitsPerPixel(m_Texture.format) / 8;
			break;
		}

		// pitch must be >= 16 bytes, other wise speed of the texture L2 cache
		// will decelerate and the mapping speed of all textures will deteriorate:
		// https://ps3.scedev.net/technotes/view/1075
		Assertf(m_Texture.pitch >= 16, "%s: Texture.pitch less than 16 bytes!", m_Name);
	}
	else
	{
		m_Texture.pitch = 0; 
	}
}

grcTexture::ChannelBits FindUsedChannelFromGcmTexture(const CellGcmTexture& tex)
{
	grcTexture::ChannelBits bits(false);

	u32 remapOutB = (tex.remap >> 14) & 0x3;
	u32 remapOutG = (tex.remap >> 12) & 0x3;
	u32 remapOutR = (tex.remap >> 10) & 0x3;
	u32 remapOutA = (tex.remap >> 8) & 0x3;

	bits.Set(grcTexture::CHANNEL_RED,	remapOutR == CELL_GCM_TEXTURE_REMAP_REMAP);
	bits.Set(grcTexture::CHANNEL_GREEN,	remapOutG == CELL_GCM_TEXTURE_REMAP_REMAP);
	bits.Set(grcTexture::CHANNEL_BLUE,	remapOutB == CELL_GCM_TEXTURE_REMAP_REMAP);
	bits.Set(grcTexture::CHANNEL_ALPHA,	remapOutA == CELL_GCM_TEXTURE_REMAP_REMAP);

	switch(gcm::StripTextureFormat(tex.format))
	{
		// handle some special cases here
	case CELL_GCM_TEXTURE_DEPTH16:
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		bits.Reset();
		bits.Set(grcTexture::CHANNEL_DEPTH);
		break;

		// handle some special cases here
	case CELL_GCM_TEXTURE_DEPTH24_D8:
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		bits.Reset();
		bits.Set(grcTexture::CHANNEL_DEPTH);
		bits.Set(grcTexture::CHANNEL_STENCIL);
		break;

	default:
		break;
	}

	return bits;
}

grcTexture::ChannelBits grcTextureGCM::FindUsedChannels() const
{
	return FindUsedChannelFromGcmTexture(m_Texture);
}

static u32 _SetTextureSwizzle(grcTexture::eTextureSwizzle swizzle)
{
	switch (swizzle)
	{
	case grcTexture::TEXTURE_SWIZZLE_R : return CELL_GCM_TEXTURE_REMAP_R;
	case grcTexture::TEXTURE_SWIZZLE_G : return CELL_GCM_TEXTURE_REMAP_G;
	case grcTexture::TEXTURE_SWIZZLE_B : return CELL_GCM_TEXTURE_REMAP_B;
	case grcTexture::TEXTURE_SWIZZLE_A : return CELL_GCM_TEXTURE_REMAP_A;
	case grcTexture::TEXTURE_SWIZZLE_0 : return CELL_GCM_TEXTURE_REMAP_0;
	case grcTexture::TEXTURE_SWIZZLE_1 : return CELL_GCM_TEXTURE_REMAP_1;
	}

	return 0;
}

void grcTextureGCM::SetTextureSwizzle(eTextureSwizzle r, eTextureSwizzle g, eTextureSwizzle b, eTextureSwizzle a, bool bApplyToExistingSwizzle)
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

	// TODO -- support remap order XXXY/XYXY for certain texture formats
	m_Texture.remap =
	(
		(_SetTextureSwizzle(b) << 6) | // BGRA
		(_SetTextureSwizzle(g) << 4) |
		(_SetTextureSwizzle(r) << 2) |
		(_SetTextureSwizzle(a) << 0)
	);
}

static grcTexture::eTextureSwizzle _GetTextureSwizzle(u32 swizzle)
{
	switch (swizzle)
	{
	case CELL_GCM_TEXTURE_REMAP_R : return grcTexture::TEXTURE_SWIZZLE_R;
	case CELL_GCM_TEXTURE_REMAP_G : return grcTexture::TEXTURE_SWIZZLE_G;
	case CELL_GCM_TEXTURE_REMAP_B : return grcTexture::TEXTURE_SWIZZLE_B;
	case CELL_GCM_TEXTURE_REMAP_A : return grcTexture::TEXTURE_SWIZZLE_A;
	case CELL_GCM_TEXTURE_REMAP_0 : return grcTexture::TEXTURE_SWIZZLE_0;
	case CELL_GCM_TEXTURE_REMAP_1 : return grcTexture::TEXTURE_SWIZZLE_1;
	}

	return grcTexture::TEXTURE_SWIZZLE_0;
}

void grcTextureGCM::GetTextureSwizzle(eTextureSwizzle& r, eTextureSwizzle& g, eTextureSwizzle& b, eTextureSwizzle& a) const
{
	// TODO -- support remap order XXXY/XYXY for certain texture formats
	b = _GetTextureSwizzle((m_Texture.remap >> 6) & CELL_GCM_TEXTURE_REMAP_MASK);
	g = _GetTextureSwizzle((m_Texture.remap >> 4) & CELL_GCM_TEXTURE_REMAP_MASK);
	r = _GetTextureSwizzle((m_Texture.remap >> 2) & CELL_GCM_TEXTURE_REMAP_MASK);
	a = _GetTextureSwizzle((m_Texture.remap >> 0) & CELL_GCM_TEXTURE_REMAP_MASK);
}

grcTexture* grcTextureFactoryGCM::Create(const char *filename,TextureCreateParams *params) {
	char	buffer[RAGE_MAX_PATH];

	sysMemUseMemoryBucket TEXTURES(grcTexture::sm_MemoryBucket);

	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);
	RAGE_TRACK_NAME( filename );

	StringNormalize(buffer, filename, sizeof(buffer));

	grcTexture *tex = LookupTextureReference(buffer);
	if (tex)
		return tex;

	tex = rage_aligned_new(16) grcTextureGCM(buffer,params);

	// Did the creation work?
	if (!tex->GetTexturePtr())
	{
		// Nope.
		tex->Release();
		return NULL;
	}

	return tex;
}

grcTexture* grcTextureFactoryGCM::Create(grcImage *image,TextureCreateParams *params) {
	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);

	grcTexture *tex = rage_aligned_new(16) grcTextureGCM(image,params);

	// Did the creation work?
	if (!tex->GetTexturePtr())
	{
		// Nope.
		tex->Release();
		return NULL;
	}

	return(tex);
}

grcTexture* grcTextureFactoryGCM::Create(u32 width, u32 height, u32 format, void* pBuffer, u32 numMips, TextureCreateParams * params) {
	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);

	grcTexture *tex = rage_aligned_new(16) grcTextureGCM(width, height, format, pBuffer, numMips, params);

	// Did the creation work?
	if (!tex->GetTexturePtr())
	{
		// Nope.
		tex->Release();
		return NULL;
	}

	return tex;
}

grcRenderTargetGCM::grcRenderTargetGCM(const char *name,
	grcRenderTargetType type,
	int width,
	int height,
	int bitsPerPixel,
	grcTextureFactory::CreateParams * _params
	)
: m_TiledIndex(0)
{
#if __GCM

	grcTextureFactory::CreateParams params;
	if (_params)
		params = *_params; 

	bool preallocated = params.basePtr != 0;

	m_CachedTexturePtr = &m_Texture;

	int depth = 1; // TODO: Promote this to a parameter, hardcode in the meantime

	Assert(width > 0 && width <= 4096);
	Assert(height > 0 && height <= 4096);
	Assert(depth > 0 && depth <= 512);
	Assert(width != 1 || height == 1);

	m_Name = StringDuplicate(name);
	m_Type = type;
	m_Format = params.Format;
	m_BitsPerPix = bitsPerPixel;
	m_PoolID = params.PoolID;
	m_PoolHeap = params.PoolHeap;
	m_OffsetInPool = params.PoolOffset;
	m_AABuffer = NULL;

	if (params.IsSwizzled && (!_IsPowerOfTwo(width) || !_IsPowerOfTwo(height) || !_IsPowerOfTwo(depth)))
	{
		grcWarningf("\"%s\" has dimensions %dx%dx%d which are not a power of 2, it will not be swizzled as requested", GetName(), width, height, depth);
		params.IsSwizzled = false;
	}
	m_BooleanTraits.Set(grcrttIsSwizzled, params.IsSwizzled);
	m_BooleanTraits.Set(grcrttInTiledMemory, params.InTiledMemory);
	m_BooleanTraits.Set(grcrttInLocalMemory, params.InLocalMemory);
	if (params.MipLevels == 0)
	{
		//m_Texture.mipmap = _Log2(_RoundUpPowerOf2(Max(width, height, depth))) + 1;
		m_Texture.mipmap = gcm::GetSurfaceMipMapCount(width, height, params.IsSwizzled);
	}
	else
	{
		m_Texture.mipmap = params.MipLevels;
	}
	m_MSAA = params.Multisample;

	if (type == grcrtDepthBuffer || type == grcrtShadowMap)
	{
		// ZCulling allowed only for tiled and VRAM:
		m_BooleanTraits.Set(grcrttUseZCulling,	params.UseHierZ	&&
												m_BooleanTraits.IsSet(grcrttInTiledMemory) &&
												m_BooleanTraits.IsSet(grcrttInLocalMemory) );

		m_BooleanTraits.Set( grcrttCustomZCullAllocation, m_BooleanTraits.IsSet(grcrttUseZCulling) &&
														  params.UseCustomZCullAllocation );
	}

	AssertMsg(m_BooleanTraits.IsClear(grcrttIsSwizzled) || (_IsPowerOfTwo(width) && _IsPowerOfTwo(height) && _IsPowerOfTwo(depth)), "Swizzled render targets must be a power of 2");
	AssertMsg(m_Type != grcrtCubeMap || (_IsPowerOfTwo(width) && _IsPowerOfTwo(height) && _IsPowerOfTwo(depth) && width == height), "Cube map render targets must be a power of 2 and the width and height must be the same");

	// size of the texture atlas
	int atlasWidth = width;
	int atlasHeight = height;

	// grcTextureFormat_REFERENCE
	static const CellGcmEnum texFormat[] = 
	{
		CELL_GCM_ENUM_INVALID,
		CELL_GCM_TEXTURE_R5G6B5, // is the only texture format that supports dithering on the PS3 
		CELL_GCM_TEXTURE_A8R8G8B8,
		CELL_GCM_ENUM_INVALID,
		CELL_GCM_TEXTURE_X32_FLOAT, // does not support filtering but the Z buffer compression still works
		CELL_GCM_ENUM_INVALID,
		CELL_GCM_ENUM_INVALID,		
		CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT,
		CELL_GCM_TEXTURE_Y16_X16,
		CELL_GCM_TEXTURE_Y16_X16_FLOAT,  // does not support filtering
		CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT, // does not support filtering and the Z buffer compression does not work anymore!
		CELL_GCM_ENUM_INVALID,
		CELL_GCM_ENUM_INVALID,
		CELL_GCM_TEXTURE_B8, // Doesn't support stencil testing, depth testing or depth bounds testing
		CELL_GCM_ENUM_INVALID,
		CELL_GCM_ENUM_INVALID,
		CELL_GCM_ENUM_INVALID,
		CELL_GCM_TEXTURE_A1R5G5B5,
		CELL_GCM_TEXTURE_DEPTH24_D8, //? -- was CELL_GCM_ENUM_INVALID, // intended for impostors to save memory
		CELL_GCM_ENUM_INVALID,
		CELL_GCM_ENUM_INVALID,
		CELL_GCM_ENUM_INVALID,
		CELL_GCM_TEXTURE_DEPTH16,
		CELL_GCM_TEXTURE_G8B8,
		CELL_GCM_ENUM_INVALID,		// PC D32F
		CELL_GCM_ENUM_INVALID,		// PC X8R8G8B8
		CELL_GCM_ENUM_INVALID,		// PC NULL, whatever that is
		CELL_GCM_ENUM_INVALID,		// PC X24G8
		CELL_GCM_ENUM_INVALID,		// PC A8
		CELL_GCM_ENUM_INVALID,		// PC R11G11B10F
		CELL_GCM_ENUM_INVALID,		// PC D32S8
		CELL_GCM_ENUM_INVALID,		// PC X32S8
		CELL_GCM_ENUM_INVALID,		// PC DXT1
		CELL_GCM_ENUM_INVALID,		// PC DXT3
		CELL_GCM_ENUM_INVALID,		// PC DXT5
		CELL_GCM_ENUM_INVALID,		// PC DXT5A
		CELL_GCM_ENUM_INVALID,		// PC DXN
		CELL_GCM_ENUM_INVALID,		// grctfA8B8G8R8_SNORM not supported on PS3
		CELL_GCM_ENUM_INVALID,		// PC B8G8R8A8 (LoL).
	};
	CompileTimeAssert(NELEM(texFormat) == grctfCount);

	int textureFormat = 0;
	m_LayerCount = 0;

	// The most common settings...
	m_Texture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	m_Texture.cubemap = false;

	// if it is a color texture check for 16-bit, 32-bit and 64-bit
	//
	// front and back buffers are pre-fab render targets ... so they should not show up here as 	
	// grcrtFrontBuffer or grcrtBackBuffer
	//
	switch (type)
	{
	case grcrtBackBuffer:
	case grcrtFrontBuffer:
	case grcrtCubeMap:
	case grcrtVolume:
	case grcrtPermanent:
		{
			// if the user did not bother to specify a grctf* format ... check out the bits per pixel
			if (params.Format == grctfNone)
			{
				if (bitsPerPixel == 8 || bitsPerPixel == 16)
				{
					Quitf("Specify a 8-bit or 16-bit render target format with grctf*");
				}
				else
				{
					textureFormat = m_BitsPerPix == 32 ? CELL_GCM_TEXTURE_A8R8G8B8 : CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT;
				}
			}
			else // pick format from our cross-platform list
			{
				Assert(texFormat[params.Format] != CELL_GCM_TEXTURE_DEPTH24_D8); // just wondering, this wasn't valid before, does it ever come up?
				if(texFormat[params.Format] != CELL_GCM_ENUM_INVALID)
				{
					textureFormat = texFormat[params.Format];
				}
				else
				{
					Quitf("Render target format %d is not supported on PS3", params.Format);
				}
			}

			m_BitsPerPix = gcm::TextureFormatBitsPerPixel(textureFormat);
			Assert(m_BitsPerPix == 8 || m_BitsPerPix == 16 || m_BitsPerPix == 32 || m_BitsPerPix == 64 || m_BitsPerPix == 128);

			bool breakOut = true;

			switch (type)
			{
			case grcrtCubeMap:
				{
					m_Texture.cubemap = true;
					m_LayerCount = 5;

					// We don't texture atlas cube maps so early out of switch statement here
				}
				break;
			case grcrtVolume:
				{
					m_Texture.dimension = CELL_GCM_TEXTURE_DIMENSION_3;
					m_LayerCount = depth - 1;

					// We don't texture atlas volume textures so early out of switch statement here
				}
				break;
#if __ASSERT
			case grcrtBackBuffer:
				{
					AssertMsg(!g_BackBuffer, "You can only create one back buffer");
					AssertMsg(GRCDEVICE.GetMSAA() || !g_BackBuffer, "The back buffer is only for use when we need multisampling");
					breakOut = false;
				}
				break;
			case grcrtFrontBuffer:
				{
					bool freeFrontBuffer = false;
					for (u32 i = 0; i < g_FrontBufferCount && !freeFrontBuffer; ++i)
					{
						freeFrontBuffer = g_FrontBuffer[i] == NULL;
					}
					Assertf(freeFrontBuffer, "You can only create %d front buffers", g_FrontBufferCount);
					breakOut = false;
				}
				break;
#endif // __ASSERT
			default:
				{
					breakOut = false;
				}
				break;
			}

			if (breakOut)
			{
				break;
			}
		}
		break;
	// if it is a depth texture check out those
	case grcrtDepthBuffer:
	case grcrtShadowMap:
		{
			// note we have four different depth buffer formats
			// - CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT
			// - CELL_GCM_TEXTURE_DEPTH24_D8
			// - CELL_GCM_TEXTURE_DEPTH16_FLOAT
			// - CELL_GCM_TEXTURE_DEPTH16
			// but we only care about two of them
// 			if (m_BitsPerPix == 32)
// 			{
// 				textureFormat = g_grcDepthFormat;
// 			}
// 
// 			// if it is 32-bit it should have been catched above ... if it is not someone made a typo
// 			else if (g_grcDepthFormat == CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT)
// 			{
// 				textureFormat = CELL_GCM_TEXTURE_DEPTH16_FLOAT;
// 			}
// 			else
// 			{
// 				textureFormat = CELL_GCM_TEXTURE_DEPTH16;
// 			}

			// is the user asking for a integer D24S8?
			if(params.Format == grctfD24S8)
 				textureFormat = CELL_GCM_TEXTURE_DEPTH24_D8;
			// if it is not a 24-bit integer depth buffer then it might be a fp depth buffer
			// default to whatever is considered the default here
			else if(m_BitsPerPix == 32)
				textureFormat = g_grcDepthFormat;
			// is he asking for a integer 16-bit
			else if(params.Format == grctfD16)
				textureFormat = CELL_GCM_TEXTURE_DEPTH16;
			// there is not much support in the engine for this so ...
			else if(m_BitsPerPix == 16)
				textureFormat = CELL_GCM_TEXTURE_DEPTH16_FLOAT;

			// test if it is one of the supported texture formats 
			Assert(textureFormat == CELL_GCM_TEXTURE_DEPTH24_D8 || 
				textureFormat == CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT ||
				textureFormat == CELL_GCM_TEXTURE_DEPTH16_FLOAT ||
				textureFormat == CELL_GCM_TEXTURE_DEPTH16);

			m_BitsPerPix = gcm::TextureFormatBitsPerPixel(textureFormat);
//			Assert(m_BitsPerPix == 16 || m_BitsPerPix == 32);

			// test if someone made a typo
			Assert(((textureFormat == CELL_GCM_TEXTURE_DEPTH24_D8 || CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT) && m_BitsPerPix == 32) ||
				((textureFormat == CELL_GCM_TEXTURE_DEPTH16_FLOAT || CELL_GCM_TEXTURE_DEPTH16) && m_BitsPerPix == 16));
		}
		break;
	default:
		{
			Quitf("type not supported");
		}
		break;
	}

	// Sanity checks
	Assert(m_BitsPerPix == gcm::TextureFormatBitsPerPixel(textureFormat));

	// get corresponding surface format
	m_SurfaceFormat = static_cast<u8>(gcm::TextureToSurfaceFormat(textureFormat));
	if (!m_SurfaceFormat)
	{
		Quitf("texture format (%d) is not supported", textureFormat);
	}

	// Set up appropriate remappings
	switch (textureFormat)
	{
	case CELL_GCM_TEXTURE_A8R8G8B8:
		{
			// fill-up a bit mask to remap color input and output values
			//
			// color remapping for 
			// CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR
			// this remapping seems to be specific for RAGE ... 
			m_Texture.remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
				CELL_GCM_TEXTURE_REMAP_FROM_R << 6 |
				CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
				CELL_GCM_TEXTURE_REMAP_FROM_B << 2 |
				CELL_GCM_TEXTURE_REMAP_FROM_A;
		}
		break;
	case CELL_GCM_TEXTURE_X32_FLOAT:
		{
			// color remapping for 
			// CELL_GCM_TEXTURE_X32_FLOAT | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR
			// alpha is opaque
			m_Texture.remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
				CELL_GCM_TEXTURE_REMAP_ONE << 8 |
				CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
				CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
				CELL_GCM_TEXTURE_REMAP_FROM_R << 2;
		}
		break;
	case CELL_GCM_TEXTURE_B8:
		{
			// color remapping for 
			// CELL_GCM_TEXTURE_B8 | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR
			m_Texture.remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
				CELL_GCM_TEXTURE_REMAP_ONE << 8 |
				CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
				CELL_GCM_TEXTURE_REMAP_FROM_B << 4 |
				CELL_GCM_TEXTURE_REMAP_FROM_B << 2;
		}
		break;
	default:
		{
			// CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR
			// CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR
			// ??
			m_Texture.remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
				CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
				CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
				CELL_GCM_TEXTURE_REMAP_FROM_R << 2 |
				CELL_GCM_TEXTURE_REMAP_FROM_A;
		}
		break;
	}

	// Set texture dimensions
	m_Texture.width = static_cast<u16>(atlasWidth);
	m_Texture.height = static_cast<u16>(atlasHeight);
	m_Texture.depth = static_cast<u16>(depth);
	m_Texture.format = static_cast<u8>(textureFormat | (m_BooleanTraits.IsSet(grcrttIsSwizzled) ? CELL_GCM_TEXTURE_SZ : CELL_GCM_TEXTURE_LN) | CELL_GCM_TEXTURE_NR);

#if __ASSERT
	bool isColorTarget = m_Type != grcrtDepthBuffer && m_Type != grcrtShadowMap;
#endif
	SetGammaEnabled(params.IsSRGB);

	if (m_PoolID!=kRTPoolIDInvalid)
	{
		if (m_PoolID == kRTPoolIDAuto)	// we'll make a pool just for this target...
		{	
			SetPhysicalSize(gcm::GetSurfaceSize(	m_Texture.width, m_Texture.height, m_BitsPerPix, m_Texture.mipmap, GetLayerCount(),
													m_BooleanTraits.IsSet(grcrttInLocalMemory), m_BooleanTraits.IsSet(grcrttInTiledMemory),
													m_BooleanTraits.IsSet(grcrttIsSwizzled),  m_Type == grcrtCubeMap,
													m_BooleanTraits.IsSet(grcrttUseZCulling), NULL));

			m_PoolID = grcTextureFactory::CreateAutoRTPool(name, params, GetPhysicalSize());
		}

		// Is the first allocation in the memory pool?
		if (!grcRenderTargetPoolMgr::GetIsInitialised(m_PoolID))
		{
			// this is from old style usage, when the pool was initialized on first use, based on that rendertarget's creation params

			// make a param set based on the current target.
			grcRTPoolCreateParams poolParams;
			poolParams.m_Type = m_Type;
			poolParams.m_BitDepth = m_BitsPerPix;
			poolParams.m_Swizzled = m_BooleanTraits.IsSet(grcrttIsSwizzled);
			poolParams.m_Tiled = m_BooleanTraits.IsSet(grcrttInTiledMemory);
			poolParams.m_Compression = params.EnableCompression ? 0xff : 0x00;  // set to 0xff, so we calculate it
			poolParams.m_PhysicalMem = params.InLocalMemory;
			poolParams.m_Pitch = grcRenderTargetPoolMgr::GetMemoryPoolPitch(m_PoolID);

			// if they did not specify a pitch when creating the pool, the first texture will do it for them. (yuck!)
			if (grcRenderTargetPoolMgr::GetMemoryPoolPitch(m_PoolID)==0)
			{
				if (grcRenderTargetPoolMgr::GetIsMemoryPoolTiled(m_PoolID))
					poolParams.m_Pitch =  gcm::GetSurfaceTiledPitch( m_Texture.width, poolParams.m_BitDepth, poolParams.m_Swizzled);
				else
					poolParams.m_Pitch =  gcm::GetSurfacePitch( m_Texture.width, poolParams.m_BitDepth, poolParams.m_Swizzled);
			}
		
			grcRenderTargetPoolMgr::InitializeMemory(m_PoolID, poolParams);

			if (!grcRenderTargetPoolMgr::GetIsMemoryPoolTiled(m_PoolID))
				m_BooleanTraits.Clear(grcrttInTiledMemory);
		}
 		
		Assertf(!grcRenderTargetPoolMgr::GetIsMemoryPoolTiled(m_PoolID) || grcRenderTargetPoolMgr::GetIsColorTarget(m_PoolID) == isColorTarget || grcRenderTargetPoolMgr::GetCompression(m_PoolID) == CELL_GCM_COMPMODE_DISABLED, "All render targets within this *compressed* *tiled* memory pool must have a %s format", isColorTarget ? "color" : "depth");
		m_BooleanTraits.Set(grcrttInTiledMemory, grcRenderTargetPoolMgr::GetIsMemoryPoolTiled(m_PoolID));

		if (!grcRenderTargetPoolMgr::GetIsMemoryPoolTiled(m_PoolID) && params.InTiledMemory)
		{
			grcWarningf("Memory pool is not tiled, but tiling was specified for \"%s\"", GetName());
		}

		ValidatePoolAllocation(); // not really allocating yet, just verifying and calculating some values

		m_TiledIndex = grcRenderTargetPoolMgr::GetTiledIndex(m_PoolID);
		Assertf(m_Texture.pitch <=grcRenderTargetPoolMgr::GetMemoryPoolPitch(m_PoolID) , "All render targets within a single memory pool must have at most a %d byte pitch", grcRenderTargetPoolMgr::GetMemoryPoolPitch(m_PoolID));

// 		if (m_BooleanTraits.IsSet(grcrttUseZCulling)) // don't do this until actual allocation
// 		{
// 			AttachZCulling();
// 		}
	}
	else if (!preallocated)
	{
		// If this render target does not belong to a memory pool, just allocate the render target
		// and move if it into a tiling region, if requested
		AllocateNonPooled();

		if (m_BooleanTraits.IsSet(grcrttUseZCulling) && m_BooleanTraits.IsClear(grcrttCustomZCullAllocation))
		{
			AttachZCulling();
		}

		if (m_BooleanTraits.IsSet(grcrttInTiledMemory))
		{
			MoveIntoTiledMemory(GetPhysicalSize(), params.EnableCompression);
		}
	}

	if (!(m_BooleanTraits.IsClear(grcrttUseZCulling) || m_BooleanTraits.IsSet(grcrttInTiledMemory)))
	{
		m_BooleanTraits.Clear(grcrttUseZCulling);
		grcWarningf("Z culling is only available for tiled render targets and there are no tiling regions free, \"%s\" will not use Z culling.", m_Name);
	}

	if (m_PoolID==kRTPoolIDInvalid)
	{
		m_Allocated = true;

		if (preallocated)
		{
			u32 baseOffset = m_BooleanTraits.IsSet(grcrttInLocalMemory) ? gcm::LocalOffset((void*)params.basePtr) : gcm::MainOffset((void*)params.basePtr);
			AllocateInPlace(baseOffset);
		}
	}
	else if (params.AllocateFromPoolOnCreate)
	{
		Assert(!preallocated);
		AllocateMemoryFromPool();
	}

	UpdatePackedTexture();

#else // !__GCM
	// Unused parameters
	name = name;
	type = type;
	width = width;
	height = height;
	bitsPerPixel = bitsPerPixel;
	_params = _params;
#endif
}


grcRenderTargetGCM::grcRenderTargetGCM(const char *name, const grcTextureObject *pTexture)
{
#if __GCM
	int width = pTexture->width;
	int height = pTexture->height;

	m_CachedTexturePtr = &m_Texture;

	Assert(width > 0 && width <= 4096);
	Assert(height > 0 && height <= 4096);
	Assert(width != 1 || height == 1);

	m_Name = StringDuplicate(name);
	m_Type = grcrtPermanent;
	
	u8 texFormat = gcm::StripTextureFormat(pTexture->format);
	// grcTextureFormat_REFERENCE
	switch( texFormat )
	{
		case CELL_GCM_TEXTURE_R5G6B5:
			m_Format = grctfR5G6B5;
			break;
		case CELL_GCM_TEXTURE_A8R8G8B8:
			m_Format = grctfA8R8G8B8;
			break;
		case CELL_GCM_TEXTURE_X32_FLOAT:
			m_Format = grctfR32F;
			break;
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
			m_Format = grctfA16B16G16R16F;
			break;
		case CELL_GCM_TEXTURE_Y16_X16:
			m_Format = grctfG16R16;
			break;
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			m_Format = grctfG16R16F;
			break;
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
			m_Format = grctfA32B32G32R32F;
			break;
		case CELL_GCM_TEXTURE_B8:
			m_Format = grctfL8;
			break;
		case CELL_GCM_TEXTURE_A1R5G5B5:
			m_Format = grctfA1R5G5B5;
			break;
		case CELL_GCM_TEXTURE_DEPTH24_D8:
			m_Format = grctfD24S8;
			break;
		case CELL_GCM_TEXTURE_DEPTH16:
			m_Format = grctfD16;
			break;
		case CELL_GCM_TEXTURE_G8B8:
			m_Format = grctfG8B8;
			break;
        case CELL_GCM_TEXTURE_COMPRESSED_DXT1: // NOTE: used for gpu compression where rt aliases a resource texture
            m_Format = grctfG16R16;
            break;
		default:
			Quitf("texture format (%d) is not supported", texFormat);
			break;
		}

	m_BitsPerPix = gcm::TextureFormatBitsPerPixel(pTexture->format);
	m_PoolID = kRTPoolIDInvalid;
	m_PoolHeap = 0;
	m_OffsetInPool = 0;
	m_AABuffer = NULL;

	m_BooleanTraits.Set(grcrttIsSwizzled, (pTexture->format & CELL_GCM_TEXTURE_LN) == 0);
	m_BooleanTraits.Set(grcrttInTiledMemory, false);
	m_BooleanTraits.Set(grcrttInLocalMemory, (pTexture->location == CELL_GCM_LOCATION_LOCAL));

	m_Texture.mipmap = pTexture->mipmap;
	
	m_MSAA = grcDevice::MSAA_None;

	AssertMsg(m_BooleanTraits.IsClear(grcrttIsSwizzled) || (_IsPowerOfTwo(width) && _IsPowerOfTwo(height)), "Swizzled render targets must be a power of 2");

	m_LayerCount = 0;

	// The most common settings...
	m_Texture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	m_Texture.cubemap = false;


	// Sanity checks
	Assert(m_BitsPerPix == gcm::TextureFormatBitsPerPixel(pTexture->format));

    if (texFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT1)
    {
		// setup our arcane settings here
		width = width >> 1;
		height = height >> 2;
        m_BitsPerPix = 32;
        m_SurfaceFormat = CELL_GCM_SURFACE_A8B8G8R8;

        Assert(pTexture->location == CELL_GCM_LOCATION_LOCAL);
        m_BooleanTraits.Set(grcrttInTiledMemory, false);
        m_BooleanTraits.Set(grcrttIsSwizzled, true);
    }
    else if( gcm::StripTextureFormat(pTexture->format) != CELL_GCM_TEXTURE_A8R8G8B8 )
	{
        // get corresponding surface format
		m_SurfaceFormat = static_cast<u8>(gcm::TextureToSurfaceFormat(pTexture->format));

		if (!m_SurfaceFormat)
		{
			Quitf("texture format (%d) is not supported", pTexture->format);
		}
	}
	else
	{
		m_SurfaceFormat = CELL_GCM_SURFACE_A8R8G8B8;
	}

	// Set up appropriate remappings
	switch (pTexture->format)
	{
	//case CELL_GCM_TEXTURE_A8R8G8B8:
	//	{
	//		// fill-up a bit mask to remap color input and output values
	//		//
	//		// color remapping for 
	//		// CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR
	//		// this remapping seems to be specific for RAGE ... 
	//		.
	//		m_Texture.remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
	//			CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
	//			CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
	//			CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
	//			CELL_GCM_TEXTURE_REMAP_FROM_R << 6 |
	//			CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
	//			CELL_GCM_TEXTURE_REMAP_FROM_B << 2 |
	//			CELL_GCM_TEXTURE_REMAP_FROM_A;
	//	}
	//	break;
	case CELL_GCM_TEXTURE_X32_FLOAT:
		{
			// color remapping for 
			// CELL_GCM_TEXTURE_X32_FLOAT | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR
			// alpha is opaque
			m_Texture.remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
				CELL_GCM_TEXTURE_REMAP_ONE << 8 |
				CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
				CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
				CELL_GCM_TEXTURE_REMAP_FROM_R << 2;
		}
		break;
	case CELL_GCM_TEXTURE_B8:
		{
			// color remapping for 
			// CELL_GCM_TEXTURE_B8 | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR
			m_Texture.remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
				CELL_GCM_TEXTURE_REMAP_ONE << 8 |
				CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
				CELL_GCM_TEXTURE_REMAP_FROM_B << 4 |
				CELL_GCM_TEXTURE_REMAP_FROM_B << 2;
		}
		break;
	default:
		{
			// CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR
			// CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR
			// ??
			m_Texture.remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
				CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
				CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
				CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
				CELL_GCM_TEXTURE_REMAP_FROM_R << 2 |
				CELL_GCM_TEXTURE_REMAP_FROM_A;
		}
		break;
	}

	// Set texture dimensions
	m_Texture.width = static_cast<u16>(width);
	m_Texture.height = static_cast<u16>(height);
	m_Texture.depth = 1;
	m_Texture.format = static_cast<u8>(pTexture->format | (m_BooleanTraits.IsSet(grcrttIsSwizzled) ? CELL_GCM_TEXTURE_SZ : CELL_GCM_TEXTURE_LN) | CELL_GCM_TEXTURE_NR);
	m_Texture._padding = pTexture->_padding;
	m_Texture.pitch = pTexture->pitch;
	m_Texture.offset = pTexture->offset;

	AllocateInPlace(pTexture->offset);
	m_BooleanTraits.Set(grcrttUseTextureMemory,true);

	if (texFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT1)
	{
		// swizzled is set to true so we get correct mip offsets for the dxt packing, but we need to turn it off
		// again, the RT shouldn't be swizzled. this has also set the pitch to 0 but you should set it for each mip
		// when rendering anyway
        m_BooleanTraits.Set(grcrttIsSwizzled, false);
	}

	m_Allocated = true;

	UpdatePackedTexture();

#else
	(void)sizeof(name);
	(void)sizeof(pTexture);
#endif
}

grcRenderTargetGCM::~grcRenderTargetGCM()
{
	grcTextureFactory::UnregisterRenderTarget(this);

	if (m_PoolID!=kRTPoolIDInvalid)
	{
		if (IsAllocated())
			ReleaseMemoryToPool();
	}

	delete [] m_MemoryOffsets;
	delete [] m_MemoryBaseOffsets;

	// release the AA buffer in case we have one
	if (m_AABuffer != NULL)
	{
		m_AABuffer->Release();
	}
#if __GCM
	// Last minute GTA5 bug fix for B*1569948: We should only do this if we aren't pooled (if we're pooled, other render targets may
	// be using this tile index)! So, I guess if we're pooled... we're just going to leave the tile index (and potentially leak it).
	// Really what we should do is keep a table that keeps track of the number of references on each tile index and release the index
	// when there are no more references to it.
	if ( (m_PoolID==kRTPoolIDInvalid) && m_BooleanTraits.IsSet(grcrttInTiledMemory))
	{
		cellGcmUnbindTile(m_TiledIndex);
	}

	FreeRenderTarget();
#endif // __GCM
}

void grcRenderTargetGCM::AllocateMemoryFromPool()
{
#if __GCM
	Assertf(!m_Allocated,"Attempting to lock an already locked target \"%s\"",m_Name);

	if (m_PoolID!=kRTPoolIDInvalid)
	{
		grcRenderTarget::LogTexture("Lock",this);

		AllocateFromPool();

		if (m_BooleanTraits.IsSet(grcrttUseZCulling) && m_BooleanTraits.IsClear(grcrttCustomZCullAllocation))  
		{
			AttachZCulling();
		}

		// and lock the AA buffer too
		if (m_AABuffer)
			m_AABuffer->AllocateMemoryFromPool();

		m_Allocated = true;
	}
#endif
}

void grcRenderTargetGCM::ReleaseMemoryToPool()
{
#if __GCM
	Assertf(m_Allocated,"Attempting to unlock an already unlocked target \"%s\"",m_Name);

	// NOTE: don't allow zCull targets to be unlocked. we cannot keep updating the zCull records during rendering
	// AJH: I'm sure we can find a way of sharing the record...
	if (Verifyf(!m_BooleanTraits.IsSet(grcrttUseZCulling),"Sorry, you can't unlock a zCull rendertarget (\"%s\")",m_Name))  
	{
		if (m_PoolID!=kRTPoolIDInvalid)
		{
			grcRenderTarget::LogTexture("Unlock",this);
			ReleaseFromPool();

			// deal with the aa buffer too if it exists
			if (m_AABuffer)
				m_AABuffer->ReleaseMemoryToPool();

			m_Allocated = false;
		}
	}
#endif
}

void grcRenderTargetGCM::UpdateMemoryLocation(const grcTextureObject *pTexture)
{
#if __GCM
	Assertf(m_Allocated,"Attempting to update a non allocated target \"%s\"",m_Name);
	Assertf(m_BooleanTraits.IsSet(grcrttUseTextureMemory),"Attempting to update a texture based target \"%s\"",m_Name);

	m_Texture.offset = m_MemoryOffset = pTexture->offset;
	UpdatePackedTexture();

	// Add base offset
	Assert(m_MemoryBaseOffsets[0] == 0);
	for (u32 layer = 0; layer < GetLayerCount(); ++layer)
	{
		for (u32 mip = 0; mip < m_Texture.mipmap; ++mip)
		{
			m_MemoryOffsets[mip * GetLayerCount() + layer] = m_MemoryBaseOffsets[mip * GetLayerCount() + layer] + m_Texture.offset;
		}
	}
#else // __GCM
	(void)sizeof(pTexture);
#endif // __GCM	
}


grcRenderTarget *grcTextureFactoryGCM::CreateRenderTarget(const char *name, grcRenderTargetType type, int width, int height, int bitsPerPixel, CreateParams *_params WIN32PC_ONLY(, grcRenderTarget* /*originalTarget*/)) 
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(grcRenderTarget);
	RAGE_TRACK_NAME(name);

	CreateParams params;
	if (_params)
		params = *_params;
	

	// check for old style memory pool allocation, etc.
	if ( params.PoolID==kRTPoolIDInvalid)
	{
		if (grcRenderTargetMemPool * oldStylePool = grcRenderTargetMemPool::GetActiveMemPool())
		{ 
			params.AllocateFromPoolOnCreate = true;
			params.PoolID = oldStylePool->GetPoolID();
			params.PoolHeap = oldStylePool->GetActiveHeap();
		}
	}

	// embedding the super-sampling render target in the main render target
	// helps us to keep everything very similar looking to the other platforms
	// we only support super-sampling in the render target class, because it does not make sense
	// to switch on multi-sampling while blitting between render targets ... it does not work

	// holds a temp pointer to the the super-sampling buffer
	grcRenderTargetGCM* AABuffer = NULL;	
	char concatenatedName[256];
	memset(concatenatedName, 0, 256);

	const u32 mipLevels = params.MipLevels; // Store temporary as we don't want the AA buffer to have mip maps
	params.MipLevels = 1;

	// depending on if the user choose 2x or 4x AA allocate a MSAA back buffer
	switch (params.Multisample)
	{
	case grcDevice::MSAA_2xMS:
		{
			strcat(concatenatedName, "2xAABuffer ");
			strcat(concatenatedName, name);		
			AABuffer = rage_aligned_new(16) grcRenderTargetGCM( concatenatedName,type,2 * width,height,bitsPerPixel,&params);	
		}
		break;
	case grcDevice::MSAA_Centered4xMS:
	case grcDevice::MSAA_Rotated4xMS:
		{
			strcat(concatenatedName, "4xAABuffer ");
			strcat(concatenatedName, name);
			AABuffer = rage_aligned_new(16) grcRenderTargetGCM( concatenatedName,type, 2 * width, 2 * height,bitsPerPixel,&params);
		}
		break;
	default:
		{
			params.MipLevels = mipLevels;
			grcRenderTargetGCM* parent = rage_aligned_new(16) grcRenderTargetGCM(name, type, width, height, bitsPerPixel, &params);
			parent->m_AABuffer = NULL;

			RegisterRenderTarget(parent);

			return parent;
		}
		// break;
	}

	// Only multisampled render targets make it this far
	Assert(AABuffer);
	RegisterRenderTarget(AABuffer);

	if (params.CreateAABuffer)
	{
		// create the main render target
		params.MipLevels = mipLevels;
		params.Multisample = grcDevice::MSAA_None;
		grcRenderTargetGCM* parent = rage_aligned_new(16) grcRenderTargetGCM(name, type, width, height, bitsPerPixel, &params);
		parent->m_AABuffer = AABuffer;
		AABuffer->m_AABuffer = parent; // Create a double link between the AA buffer and it's parent

		RegisterRenderTarget(parent);

		return parent;
	}
	else
	{
		AABuffer->m_AABuffer = NULL;
		return AABuffer;
	}
}


grcRenderTarget* grcTextureFactoryGCM::CreateRenderTarget(const char *pName, const grcTextureObject *pTexture WIN32PC_ONLY(, grcRenderTarget* /*originalTarget*/))
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(RenderTarget);
	RAGE_TRACK_NAME(pName);

	grcRenderTargetGCM* target = rage_aligned_new(16) grcRenderTargetGCM(pName, pTexture);

	RegisterRenderTarget(target);
	return target;
}


#if __PPU
bool grcRenderTargetGCM::zCullRecord::operator==(zCullRecord a) const
{
	// SimonB: this is the hardware check for zcull equivalence 
	// see https://ps3.scedev.net/forums/thread/35871
	return (	(offset==a.offset) &&
				(zFormat==a.zFormat) &&
				(aaFormat==a.aaFormat) );
}

// Create a zcull record for this target
grcRenderTargetGCM::zCullRecord::zCullRecord(const grcRenderTargetGCM* pTarget)
{
	Assert(pTarget!=NULL);

	width = pTarget->m_Texture.width;
	height = pTarget->m_Texture.height;

	// pick multi-sampling support
	switch (pTarget->m_MSAA)
	{
	case grcDevice::MSAA_2xMS:
		{
			aaFormat = CELL_GCM_SURFACE_DIAGONAL_CENTERED_2;
			width /= 2;
		}
		break;
	case grcDevice::MSAA_Centered4xMS:
		{
			aaFormat = CELL_GCM_SURFACE_SQUARE_CENTERED_4;
			width /= 2;
			height /= 2;
		}
		break;
	case grcDevice::MSAA_Rotated4xMS:
		{
			aaFormat = CELL_GCM_SURFACE_SQUARE_ROTATED_4;
			width /= 2;
			height /= 2;
		}
		break;
	default:
		{
			aaFormat = CELL_GCM_SURFACE_CENTER_1;
		}
		break;
	}

	switch (pTarget->m_SurfaceFormat)
	{
	case CELL_GCM_SURFACE_Z16:
		{
			zFormat = CELL_GCM_ZCULL_Z16;
		}
		break;
	case CELL_GCM_SURFACE_Z24S8:
		{
			zFormat = CELL_GCM_ZCULL_Z24S8;
		}
		break;
	default:
		{
			Quitf("Unknown depth format");
			zFormat = 0;
		}
		break;
	}
	//
	// Z culling functionality: 
	// we can bind Z culling functionality to 8 render targets
	// (this naturally aligns the size to 4k)
	width = (width + 63) & ~63;
	height = (height + 63) & ~63;

	// If the full range of the depth buffer is used and the depth test direction is LESS, 
	// then the LONES format should be used, otherwise the MSB format should be used.
	zcullFormat = CELL_GCM_ZCULL_LONES;
	zcullDir = CELL_GCM_ZCULL_LESS;
	sFunc = CELL_GCM_SCULL_SFUNC_NOTEQUAL;
#if HACK_GTA4
	sRef = 7; // DEFERRED_MATERIAL_CLEAR
#else
	sRef = 0x00;
#endif
	sMask = 0xff;
	zCullStart = 0;
	offset = pTarget->m_Texture.offset;
}

// Search for an existing zcull record that matches this one. Returns the record index if it finds one, or -1 if it doesn't.
int grcRenderTargetGCM::FindMatchingZCullRecord(const zCullRecord& cullRecord, bool enableLogging)
{
	for(int i = 0; i < sm_BitFieldZCulling.GetNumBits(); ++i)
	{
		if( sm_ZCullRegionsRecord[i] == cullRecord )
		{
			if( sm_ZCullRegionsRecord[i].width < cullRecord.width ||
				sm_ZCullRegionsRecord[i].height < cullRecord.height )
			{
				// We have to return an valid record index here even though the target doesn't actually end up sharing the record. This preserves the previous behavior.
				if (enableLogging)
				{
					grcWarningf("%s failed to share ZCull. Matching record is %dx%d while current target is %dx%d",
						m_Name,sm_ZCullRegionsRecord[i].width,sm_ZCullRegionsRecord[i].height,cullRecord.width,cullRecord.height);
				}
			}
			else
			{
				if (enableLogging)
				{
					grcDisplayf("\"%s\" is Sharing ZCull [%d].", m_Name,i);
				}
			}
			return i;
		}
	}

	return -1;
}

// Be careful that your width and height of the zcull region matches the 
// width and height of the buffer (usually in pixels).  There seems to be 
// no need to pad them out to a multiple of 64. 
// Also ensure that the multi-sample settings match your multi-sample 
// settings at the time of rendering.

// If anything doesn't match then zcull seems to be silently disabled. 
// Also you need to properly prime zcull using a hardware clear (in your 
// case on the near plane) before rendering.	

void grcRenderTargetGCM::AttachZCulling()
{
	// If a custom zcull allocation was requested then we shouldn't be here
	if ( !AssertVerify( m_BooleanTraits.IsClear(grcrttCustomZCullAllocation) ) )
	{
		grcWarningf("Attempting to use default zcull allocation on target requesting custom zcull allocation!");
		return;
	}

	if (!ValidateZCulling())
	{
		return;
	}

	// Make a record for this target
	zCullRecord cullRecord(this);
								
	// As we're sharing the depth surface, there's a chance that this surface is already bound
	if ( HasMatchingZCullRecord(cullRecord,true) )
	{
		return;
	}

	// if there is no slot open for Z culling show the error message
	if (sm_BitFieldZCulling.CountBits(false) == 0)
	{
		grcWarningf("Z culling is only available for %d render targets, \"%s\" will not use Z culling.", sm_BitFieldZCulling.GetNumBits(), m_Name);
		m_BooleanTraits.Clear(grcrttUseZCulling);
		return;
	}
		

	{
		// Allocate a unique region for this buffer (no overlap!)
		u32 regionEnd = sm_ZCullStart + cullRecord.width*cullRecord.height;
		if( regionEnd > 3145728u )
		{
			grcWarningf("Out of Z cull RAM, \"%s\" will not use Z culling.", m_Name);
			return;
		}
		cullRecord.zCullStart = sm_ZCullStart;	
		sm_ZCullStart = regionEnd;
	}

	// .. so we have free slots in the bit field, search the next possible slot and set the flag
	// moved until after region alloc
	int recordIndex;
	for(recordIndex = 0; recordIndex < sm_BitFieldZCulling.GetNumBits(); ++recordIndex)
	{
		if(sm_BitFieldZCulling.IsClear(recordIndex))
		{
			sm_BitFieldZCulling.Set(recordIndex);
			break;
		}
	}

	grcDisplayf("Allocating ZCull record %d for \"%s\" (%d left)",recordIndex,m_Name,sm_BitFieldZCulling.CountBits(false));

	// Update our records
	sm_ZCullRegionsRecord[recordIndex] = cullRecord;
	GCM_DEBUG(cellGcmSetZcull(recordIndex, m_Texture.offset, cullRecord.width, cullRecord.height, cullRecord.zCullStart, cullRecord.zFormat, cullRecord.aaFormat, cullRecord.zcullDir, cullRecord.zcullFormat, cullRecord.sFunc, cullRecord.sRef, cullRecord.sMask));
}

// Force this render target to share the zcull region of an existing render target (the hosting target). Only call this if you really know what you're doing.
bool grcRenderTargetGCM::ForceZCullSharing( const grcRenderTargetGCM* pZCullHostingTarget )
{
	Assert(pZCullHostingTarget!=NULL);
	
	if ( m_BooleanTraits.IsClear(grcrttCustomZCullAllocation) )
	{
		grcErrorf("Unable to share zcull for \"%s\", the render target wasn't created with grcrttCustomZCullAllocation.", m_Name);
		return false;
	}

	// It's technically possible to share zcull between different sized targets, but we don't have any safety checks in place to
	// prevent this target from overflowing the hosting target's zcull memory (and thus corrupting memory of neighboring zcull records).
	// Just to be safe, we'll prevent sharing zcull between targets of different sizes.
	if ( pZCullHostingTarget->m_Texture.width != m_Texture.width ||
		 pZCullHostingTarget->m_Texture.height != m_Texture.height ||
		 pZCullHostingTarget->m_Texture.format != m_Texture.format )
	{
		grcErrorf("We don't support sharing zcull memory between targets of different sizes!");
		return false;
	}

	if (!ValidateZCulling())
	{
		return false;
	}

	// If there is no slot open for Z culling then we can't force share the hosting target's zcull memory
	if (sm_BitFieldZCulling.CountBits(false) == 0)
	{
		grcWarningf("Z culling is only available for %d render targets. We've run out of slots and can't force-share zcull. \"%s\" will not use Z culling.", sm_BitFieldZCulling.GetNumBits(), m_Name);
		m_BooleanTraits.Clear(grcrttUseZCulling);
		return false;
	}

	// Find the record for the hosting target
	int hostRecordIndex = FindMatchingZCullRecord(zCullRecord(pZCullHostingTarget),false);
	if ( !AssertVerify(0 <= hostRecordIndex) )
	{
		grcErrorf("The hosting render target doesn't have a ZCull record so it has nothing to share!");
		return false;
	}

	// Make a record for our target and force it to overlap the hosting render target's zcull memory
	zCullRecord cullRecord(this);
	cullRecord.zCullStart = sm_ZCullRegionsRecord[hostRecordIndex].zCullStart;
	if (HasMatchingZCullRecord(cullRecord,false))
	{
		grcErrorf("This target already has a matching ZCull record, why are you trying to force sharing?!");
		return false;
	}
	
	// We have free slots in the bit field (verified above), search the next possible slot and set the flag moved until after region alloc
	int newRecordIndex;
	for(newRecordIndex = 0; newRecordIndex < sm_BitFieldZCulling.GetNumBits(); ++newRecordIndex)
	{
		if(sm_BitFieldZCulling.IsClear(newRecordIndex))
		{
			sm_BitFieldZCulling.Set(newRecordIndex);
			break;
		}
	}

	grcDisplayf("Allocating ZCull record %d using forced-sharing for \"%s\" (%d left). This is hosted in the zcull region of \"%s\".",newRecordIndex,m_Name,sm_BitFieldZCulling.CountBits(false),pZCullHostingTarget->m_Name);

	// Update our records and create the GCM zcull slot
	sm_ZCullRegionsRecord[newRecordIndex] = cullRecord;
	GCM_DEBUG(cellGcmSetZcull(newRecordIndex, m_Texture.offset, cullRecord.width, cullRecord.height, cullRecord.zCullStart, cullRecord.zFormat, cullRecord.aaFormat, cullRecord.zcullDir, cullRecord.zcullFormat, cullRecord.sFunc, cullRecord.sRef, cullRecord.sMask));

	return true;
}

//
// move render target into tiled memory
//
u8 grcRenderTargetGCM::MoveIntoTiledMemory(u32 memorySize, bool enableCompression)
{
	if (!ValidateTiledMemory(memorySize))
	{
		return CELL_GCM_COMPMODE_DISABLED;
	}

	// .. so we have free slots in the bit field, search the next possible slot and set the flag
	for (m_TiledIndex = 0; m_TiledIndex < sm_BitFieldTiledMemory.GetNumBits(); ++m_TiledIndex)
	{
		if(sm_BitFieldTiledMemory.IsClear(m_TiledIndex))
		{
			sm_BitFieldTiledMemory.Set(m_TiledIndex);
			break;
		}
	}

	u8 compressionMode = CELL_GCM_COMPMODE_DISABLED;

	// this determines which DRAM page is accessed
	// if a color and a depth buffer access the same DRAM page simultaneously, a page miss will occur 
	// ... whatever the result of a page miss is will then happen :-)
	// to move as most render targets as possible in different banks, I just count up here and reset to 0 if 3 is reached,
	// because we have four different DRAM banks
	//
	// from my understanding, render targets that are used with each other should be created close to each other ... so 
	// they should always end up in different banks

	u32 bank = 0;

	switch (m_Type)
	{
	case grcrtVolume:
	case grcrtCubeMap:
	case grcrtPermanent:
	case grcrtFrontBuffer:
	case grcrtBackBuffer:
		{
			bank = sm_ColorBank + 1;
			++sm_ColorBank %= 3;

			//
			// color compression is only applicable to 32-bits-per-pixel surfaces!
			//
			// if ROPTiles are compressed, the ROP block can output them at a rate of 1 clock/ROPTile -> for 32bpp, 32 samples / clock
			// if ROPTiles are not compressed, the ROP block can output them at a rate of 4 clocks / ROPTile -> for 32 bpp, 8 samples / clock
			compressionMode = CELL_GCM_COMPMODE_DISABLED;

			// only works on 32bpp stuff
			if (m_BitsPerPix == 32)
			{
				// 4AA choose the 2x2 compression mode
				if (m_MSAA == grcDevice::MSAA_Centered4xMS || m_MSAA == grcDevice::MSAA_Rotated4xMS)
				{
					compressionMode = CELL_GCM_COMPMODE_C32_2X2;
				}
				// no super-sampling or 2AA choose the 2x1 compression mode
				else
				{
					compressionMode = CELL_GCM_COMPMODE_C32_2X1;
				}

			}
		}
		break;
	case grcrtDepthBuffer:
	case grcrtShadowMap:
		{
#if HACK_GTA4
			bank = 0;		// just always use bank 0 for depth, loop on 1-3 (above) for colour
#else
			bank = sm_DepthBank + 1;
			++sm_DepthBank %= 3;
#endif

			// this is good for an MS mode CENTERED_1 and SQUARE_CENTERED_4
			// no compression but
			// this is better than CELL_GCM_COMPMODE_DISABLED because it allows to 
			// ignore the stencil value if it is not used ... so we only write 3 bytes
			// instead of 4 bytes
			compressionMode = CELL_GCM_COMPMODE_DISABLED;
			
			if ((m_Texture.format & (CELL_GCM_TEXTURE_DEPTH24_D8 | CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT)) != 0)
			{
				compressionMode = CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REGULAR;
			
				if (m_MSAA == grcDevice::MSAA_2xMS)
				{
					// this compression mode is for the multi-sampling mode CENTERED_2
					compressionMode = CELL_GCM_COMPMODE_Z32_SEPSTENCIL_DIAGONAL;
				}
				else if (m_MSAA == grcDevice::MSAA_Rotated4xMS)
				{
					// this compression mode is for the multi-sampling mode ROTATED_4
					compressionMode = CELL_GCM_COMPMODE_Z32_SEPSTENCIL_ROTATED;
				}
			}
		}
		break;
	default:
		Quitf("Render target type not supported in MoveIntoTiledMemory()");
		return CELL_GCM_COMPMODE_DISABLED;
	}

	compressionMode = enableCompression ? compressionMode : CELL_GCM_COMPMODE_DISABLED;
	if (m_PoolID != kRTPoolIDInvalid &&
		grcRenderTargetPoolMgr::GetIsInitialised(m_PoolID) &&
		grcRenderTargetPoolMgr::GetIsMemoryPoolTiled(m_PoolID) &&
		grcRenderTargetPoolMgr::GetCompression(m_PoolID) != compressionMode)
	{
		grcWarningf("The compression mode for render target \"%s\" does not match the memory pool \"%s\" [%d != %d]", GetName(), grcRenderTargetPoolMgr::GetName(m_PoolID), compressionMode, grcRenderTargetPoolMgr::GetCompression(m_PoolID));
		compressionMode = grcRenderTargetPoolMgr::GetCompression(m_PoolID);
	}

	int info = cellGcmSetTileInfo(m_TiledIndex,		// index of the tiled area to set 0..14
					m_Texture.location,
					m_Texture.offset,	// address offset of area to set as tiled
					memorySize,			// size of area to set as tiled
					m_Texture.pitch,	// pitch size
					compressionMode,
					sm_CompressionTag,	// starting address of compression tag area
					bank);				// bank offset of tiled area 0-3

	switch (info)
	{
	case CELL_GCM_ERROR_INVALID_VALUE:
		if (m_TiledIndex >= 15)
			grcErrorf("[%s] Invalid tile index (0 - 14): %d", GetSurfaceName(), m_TiledIndex);
		if (sm_CompressionTag >= 800)
			grcErrorf("[%s] Invalid starting address of compression tag area (0x0 - 0x7ff): %d", GetSurfaceName(), sm_CompressionTag);
		if (bank >= 4)
			grcErrorf("[%s] Invalid bank offset (0 - 3): %d", GetSurfaceName(), bank);
		return CELL_GCM_COMPMODE_DISABLED;
	case CELL_GCM_ERROR_INVALID_ALIGNMENT:
		if ((m_Texture.offset & 65535) != 0)
			grcErrorf("[%s] Address offset of area to set as tiled is not 64kb aligned: %d", GetSurfaceName(), m_Texture.offset);
		if ((memorySize & 65535) != 0)
			grcErrorf("[%s] Size of area to set as tiled is not 64kb aligned: %d", GetSurfaceName(), memorySize);
		if ((m_Texture.pitch & 255) != 0)
			grcErrorf("[%s] Pitch of area to set as tiled is not 256b aligned: %d", GetSurfaceName(), m_Texture.pitch);
		return CELL_GCM_COMPMODE_DISABLED;
	case CELL_GCM_ERROR_INVALID_ENUM:
		switch (compressionMode)
		{
		case CELL_GCM_COMPMODE_DISABLED:
		case CELL_GCM_COMPMODE_C32_2X1:
		case CELL_GCM_COMPMODE_C32_2X2:
		case CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REGULAR:
		case CELL_GCM_COMPMODE_Z32_SEPSTENCIL_DIAGONAL:
		case CELL_GCM_COMPMODE_Z32_SEPSTENCIL_ROTATED:
			break;
		default:
			grcWarningf("[%s] Invalid compression mode for area to set as tiled: %d", GetSurfaceName(), compressionMode);
			break;
		}
		
		if (m_Texture.location != CELL_GCM_LOCATION_MAIN || m_Texture.location != CELL_GCM_LOCATION_LOCAL)
			grcErrorf("[%s] Invalid memory location for area to set as tiled: %d", GetSurfaceName(), m_Texture.location);
		return CELL_GCM_COMPMODE_DISABLED;
	default:
		if (cellGcmBindTile(m_TiledIndex) == CELL_GCM_ERROR_ADDRESS_OVERWRAP)
		{
			grcErrorf("[%s] Tiling regions overlap", GetSurfaceName());
			return CELL_GCM_COMPMODE_DISABLED;
		}

		if (compressionMode != CELL_GCM_COMPMODE_DISABLED)
		{
			sm_CompressionTag += memorySize / 0x10000;
		}
		sm_TiledMemorySize += memorySize;
		break;
	}

	return compressionMode;
}

void grcRenderTargetGCM::CreateMipMaps(const grcResolveFlags* /*resolveFlags*/, int index)
{
	// Only generate mips if we just rendered into the top level
	if (m_LockedMip > 0)
	{
		return;
	}

	Assert(m_Texture.mipmap > 0);
	Assert(index >=0 && index <= grcmrtColorCount);

	Vector2 sourceOffset(0.0f, 0.0f);
	Vector2 sourceSize(1.0f, 1.0f);

	u32 oldLockedMip = m_LockedMip;
	for (int i = 1; i < m_Texture.mipmap; ++i)
	{
		LockSurface(m_LockedLayer, i);
		if (m_Type == grcrtDepthBuffer || m_Type == grcrtShadowMap)
		{
			GRCDEVICE.SetRenderTarget(NULL, this);
		}
		else 
		{
			GRCDEVICE.SetRenderTarget(this, NULL);
		}

		sm_FastMipMapper.DownSample(this, index, i, sourceOffset, sourceSize, m_Type);
	}

	// Return us back to happy land
	LockSurface(m_LockedLayer, oldLockedMip);

	// TODO: Clear implementation
}

void grcRenderTargetGCM::CreateMipMaps(grcRenderTarget** _mipMaps, u32 mipMapCount, int index)
{
	if (mipMapCount == 0)
		return;

	Assert(index >=0 && index <= grcmrtColorCount);

	Vector2 sourceOffset(0.0f, 0.0f);
	Vector2 sourceSize(1.0f, 1.0f);

	for (u32 i = 1; i < mipMapCount; ++i)
	{
		grcRenderTargetGCM* src = static_cast<grcRenderTargetGCM*>(_mipMaps[i - 1]);
		grcRenderTargetGCM* dst = static_cast<grcRenderTargetGCM*>(_mipMaps[i]);
		Assert(src->GetType() == dst->GetType());

		if (dst->GetType() == grcrtDepthBuffer || dst->GetType() == grcrtShadowMap)
		{
			GRCDEVICE.SetRenderTarget(NULL, dst);
		}
		else 
		{
			GRCDEVICE.SetRenderTarget(dst, NULL);
		}

		sm_FastMipMapper.DownSample(src, index, 0, sourceOffset, sourceSize, src->GetType());
	}
}

void grcRenderTargetGCM::Blur(const grcResolveFlags* resolveFlags)
{
	Vector2 sourceOffset(0.0f, 0.0f);
	Vector2 sourceSize(1.0f, 1.0f);

	GRCDEVICE.SetRenderTarget(this, NULL);
	sm_FastMipMapper.Blur(this, resolveFlags->BlurKernelSize, sourceOffset, sourceSize);
}

//
// lock the render target -> tell the renderer where to render stuff
//
void grcTextureFactoryGCM::LockRenderTarget(int index, const grcRenderTarget *color,const grcRenderTarget *depth, u32 layer, bool lockDepth, u32 D3D11_OR_ORBIS_ONLY(mipToLock))
{
	const u32 mipLevel = 0;

	Assert(index >= 0);
	Assert(!color || (layer < color->GetLayerCount() && mipLevel < (u32)color->GetMipMapCount()));
	Assert(!depth || depth->GetMipMapCount() == 1);
	Assert(color == NULL || color->IsAllocated());
	Assert(depth == NULL || depth->IsAllocated());

#if __ASSERT
	for (int i = 0; i < index; ++i)
	{
		AssertMsg(m_LockColorRenderTargets[i], "MRTs must be locked in increasing order, ie. LockRenderTarget(0, ...), LockRenderTarget(1, ...), LockRenderTarget(2, ...)");
	}
#endif // __ASSERT

	if (color)
	{
		// Sanity checks since we don't yet support volume render targets and this is going to require a bit of change of interface
		Assert(color->GetType() != grcrtVolume);

		m_LockColorRenderTargets[index] = color->GetAABufferPtr() ? color->GetAABufferPtr() : color;

		if (m_LockColorRenderTargets[index]->GetType() == grcrtCubeMap)
		{
			static_cast<const grcRenderTargetGCM*>(m_LockColorRenderTargets[index])->LockSurface(layer, mipLevel);
		}
	}
	else
	{
		AssertMsg(index == 0, "If you are just locking a depth buffer it needs to go into slot 0");
		m_LockColorRenderTargets[index] = NULL;
	}

	if (depth )
	{
		// Sanity checks since we don't yet support volume render targets and this is going to require a bit of change of interface
		Assert(depth->GetType() != grcrtVolume);

		m_LockDepthRenderTarget = depth->GetAABufferPtr() ? depth->GetAABufferPtr() : depth;

		if (m_LockDepthRenderTarget->GetType() == grcrtCubeMap && lockDepth)
		{
			static_cast<const grcRenderTargetGCM*>(m_LockDepthRenderTarget)->LockSurface(layer, mipLevel);
		}
	}
	else
	{
		m_LockDepthRenderTarget = NULL;
	}

	// To make this more MRT compatible
	// if the index is 0 set the other render targets to NULL
	// why is this necessary? You can not leave render target 1 laying around while exchanging render target 0
	// there is a high chance that the new render target 0 does not have the same size and format as render target 1
	// and it crashes then
	// so if you set a new render target 0 you also have to set render target 1 again
	for (u32 i = static_cast<u32>(index) + 1; i < grcmrtColorCount; ++i)
	{
		m_LockColorRenderTargets[i] = NULL;
	}

	// sets the rendering buffer in cellGcmSetSurface()
	GRCDEVICE.SetRenderTargets(m_LockColorRenderTargets, m_LockDepthRenderTarget);
}

//
// lock several render targets -> tell the renderer where to render stuff
// this is meant for 2D Multiple render targets
//
void grcTextureFactoryGCM::LockMRT(const grcRenderTarget *color[grcmrtColorCount],const grcRenderTarget *depth, const u32* D3D11_ONLY(mipsToLock)) 
{
	//
	// This function allows to set the render targets in any order ... which is not allowed in LockRenderTarget()
	// Texture atlas and AA is not supported here also cube maps are naturally excluded... this would be part of our customer's 
	// choice program (please call 800 RAGE and ask for Samantha. She will welcome your requirement list)
	// 
	u32 colorExists = 0;
	for (u32 i = 0; i < grcmrtColorCount; ++i)
	{
		Assert(color[i] == NULL || color[i]->IsAllocated());
		m_LockColorRenderTargets[i] = color[i] ? (color[i]->GetAABufferPtr() ? color[i]->GetAABufferPtr() : color[i]) : NULL;
		colorExists |= reinterpret_cast<u32>(m_LockColorRenderTargets[i]);
	}
	Assert(depth == NULL || depth->IsAllocated());
	m_LockDepthRenderTarget = depth ? (depth->GetAABufferPtr() ? depth->GetAABufferPtr() : depth) : NULL; 

	if (!colorExists)
	{
		return;
	}

	// sets the rendering buffer in cellGcmSetSurface()
	GRCDEVICE.SetRenderTargets(m_LockColorRenderTargets, m_LockDepthRenderTarget);
}

void grcTextureFactoryGCM::UnlockRenderTarget(int index, const grcResolveFlags* resolveFlags) 
{
	Assert(index >= 0);


#if __ASSERT
	for (int i = 0; i < index; ++i)
	{
		AssertMsg(m_LockColorRenderTargets[i], "MRTs must be unlocked in decreasing order, ie. UnlockRenderTarget(2, ...), UnlockRenderTarget(1, ...), UnlockRenderTarget(0, ...)");
	}
#endif // __ASSERT
	
	// 
	// if the user specified super-sampling for this render target
	// do the AA pass here
	// in other words this is support for render target AA only ... contrary to 
	// MSAA support for front and back-buffers
	//
	// Very very dirty const casting!!!
	grcRenderTarget*& target = const_cast<grcRenderTarget*&>(m_LockColorRenderTargets[index]);
	if (target)
	{
		Assert(target->IsAllocated());	
		if (target->GetAABufferPtr())
		{
			grcRenderTarget* source = target;
			target = source->GetAABufferPtr();
			Assert(source->GetMSAA() && !target->GetMSAA());
			GRCDEVICE.MsaaResolve(target, source);
		}

		if (resolveFlags && resolveFlags->BlurResult)
		{
			target->Blur(resolveFlags);
		}

		if (target->GetMipMapCount() > 1 && (!resolveFlags || resolveFlags->MipMap))
		{
			target->CreateMipMaps(NULL, index);
		}
	}

	if (index == 0)
	{
		if (m_LockDepthRenderTarget && m_LockDepthRenderTarget->GetMipMapCount() > 1 && (resolveFlags == NULL || resolveFlags->MipMap))
		{
			Assert(m_LockDepthRenderTarget->IsAllocated());	

			// Very very dirty const casting!!!
			const_cast<grcRenderTarget*>(m_LockDepthRenderTarget)->CreateMipMaps(NULL, index);
		}

		//GRCDEVICE.SetRenderTargets(m_LockColorRenderTargets, m_LockDepthRenderTarget);
		m_LockDepthRenderTarget = NULL;
		target = NULL;
		BANK_ONLY(g_AreRenderTargetsBound = false);
		//takes 1.2% of the frame to do this, before if you didn't bind your own target it would go to the front buffer, now it just goes to the previously bound target
	}
	else
	{
		target = NULL;
		GRCDEVICE.SetRenderTargets(m_LockColorRenderTargets, m_LockDepthRenderTarget);
	}
}


void grcTextureFactoryGCM::UnlockMRT(const grcResolveFlagsMrt* resolveFlags /*= NULL*/) 
{		  
	// if it would be AA'ed we would have to set a AA'ed MRT here and do the AA blit from one MRT to the other

	// without AA we assume that the user wants to go back rendering into the front or back buffer
	// if the front buffer is super-sampled, we need to set the back buffer here

	// set all the MRT render targets and the depth buffer to NULL
	for (u32 i = 0; i < grcmrtColorCount; ++i)
	{
		// 
		// if the user specified super-sampling for this render target
		// do the AA pass here
		// in other words this is support for render target AA only ... contrary to 
		// MSAA support for front and back-buffers
		//
		// Very very dirty const casting!!!
		grcRenderTarget*& target = const_cast<grcRenderTarget*&>(m_LockColorRenderTargets[i]);
		if (target)
		{
			if (target->GetAABufferPtr())
			{
				grcRenderTarget* source = target;
				target = source->GetAABufferPtr();
				Assert(source->GetMSAA() && !target->GetMSAA());
				GRCDEVICE.MsaaResolve(target, source, -1);
			}

			if (target->GetMipMapCount() > 1 && (!resolveFlags || !(*resolveFlags)[i] || (*resolveFlags)[i]->MipMap))
			{
				target->CreateMipMaps(NULL,0);
			}
		}

		target = NULL;
	}
}

#else
void grcTextureFactoryGCM::LockRenderTarget	(int /*index*/,const grcRenderTarget *,const grcRenderTarget *,u32 /*layer*/,bool /*lockDepth*/, u32 D3D11_OR_ORBIS_ONLY(mipsToLock)) {}
void grcTextureFactoryGCM::UnlockRenderTarget(int /*index*/,const grcResolveFlags *) {}
void grcTextureFactoryGCM::LockMRT(const grcRenderTarget *[grcmrtColorCount],const grcRenderTarget *,const u32*) {}
void grcTextureFactoryGCM::UnlockMRT(const grcResolveFlagsMrt* /*resolveFlags*/ /*= NULL*/) {}
u8 grcRenderTargetGCM::MoveIntoTiledMemory(u32 /*memorySize*/, bool /*enableCompression*/) { return 0; }
void grcRenderTargetGCM::AttachZCulling() {}
void grcRenderTargetGCM::CreateMipMaps(const grcResolveFlags* /*resolveFlags*/, int /*index*/) {}
void grcRenderTargetGCM::Blur(const grcResolveFlags* /*resolveFlags*/) {}
#endif

grcTexture::ChannelBits grcRenderTargetGCM::FindUsedChannels() const
{
	return FindUsedChannelFromGcmTexture(m_Texture);
}

u32 grcTextureFactoryGCM::GetTextureDataSize(u32 width, u32 height, u32 format, u32 mipLevels, u32 numLayers, bool bIsCubeMap, bool bIsLinear, bool bLocalMemory)
{
	grcImage::Format imgFormat	= (grcImage::Format)format;
	int bpp         			= grcImage::GetFormatBitsPerPixel(imgFormat);
	bool bLinear				= bIsLinear;
	bool bDynamic				= !bLocalMemory;
	u8 internalFormat			= (u8)GetGCMTextureFormatFromImageFormat(imgFormat);
	int pitch;

	if (internalFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT1)
	{
		pitch = ((width+3)/4) * 8;
	}
	else if (internalFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT23 || internalFormat == CELL_GCM_TEXTURE_COMPRESSED_DXT45)
	{
		pitch = ((width+3)/4) * 16;
	}
	else 
	{
		pitch = width * gcm::TextureFormatBitsPerPixel(internalFormat) / 8;
	}

	switch ((u32)imgFormat) // handle funky formats .. these should go away soon maybe
	{
		case grcImage::LINA32B32G32R32F_DEPRECATED : imgFormat = grcImage::A32B32G32R32F; bLinear            = true; break;
		case grcImage::LINA8R8G8B8_DEPRECATED      : imgFormat = grcImage::A8R8G8B8     ; bLinear = bDynamic = true; break;
		case grcImage::LIN8_DEPRECATED             : imgFormat = grcImage::L8           ; bLinear            = true; break;
	}



	internalFormat		|= (bLinear ? CELL_GCM_TEXTURE_LN : CELL_GCM_TEXTURE_SZ);
	internalFormat		|= CELL_GCM_TEXTURE_NR; // All textures currently use normalised texture coordinates


	u32 linesPerPitch = gcm::TextureFormatLinesPerPitch(internalFormat);

	u32 memSize = gcm::GetTextureSize(width, height, pitch,
		bpp, 
		mipLevels, 
		numLayers+1, 
		!bDynamic, 
		!bLinear, 
		bIsCubeMap, 
		NULL,
		linesPerPitch);

	return memSize;
}

void grcTextureFactoryGCM::PlaceTexture(class datResource &rsc,grcTexture &tex) {
	switch (tex.GetResourceType()) {
		case grcTexture::NORMAL: ::new (&tex) grcTextureGCM(rsc); break;
		case grcTexture::RENDERTARGET: AssertMsg(0 , "unsafe to reference a rendertarget"); break;
		case grcTexture::REFERENCE: ::new (&tex) grcTextureReference(rsc); break;
		default: Quitf("Bad resource type %d in grcTextureFactoryGCM::PlaceTexture",tex.GetResourceType());
	}
}

grcRenderTargetGCM *grcRenderTargetGCM::CreatePrePatchedTarget(bool patchSurface)
{
	grcRenderTargetGCM *newRT = rage_aligned_new(16) grcRenderTargetGCM(this);

	PatchShadowToDepthBuffer(newRT, patchSurface);

	return newRT;
}

#if __PPU

grcRenderTargetGCM *grcRenderTargetGCM::DuplicateTarget(const char *name)
{
	grcRenderTargetGCM *newRT = rage_aligned_new(16) grcRenderTargetGCM(this);
	newRT->m_Name = __FINAL ? NULL : StringDuplicate(name);
	return newRT;
#else

grcRenderTargetGCM *grcRenderTargetGCM::DuplicateTarget(const char *)
{
	return NULL;
#endif
}

grcRenderTargetGCM::grcRenderTargetGCM(grcRenderTargetGCM *rtGCM)
{
	m_Texture = rtGCM->m_Texture;
	m_AABuffer = rtGCM->m_AABuffer;
	m_Name = rtGCM->m_Name;
	m_MSAA = rtGCM->m_MSAA;
	m_MemoryOffsets = rtGCM->m_MemoryOffsets;
	m_MemoryBaseOffsets = rtGCM->m_MemoryBaseOffsets;
	m_MemoryOffset = rtGCM->m_MemoryOffset;
	m_LockedMip = rtGCM->m_LockedMip;
	m_BooleanTraits = rtGCM->m_BooleanTraits;
	m_SurfaceFormat = rtGCM->m_SurfaceFormat;
	m_TiledIndex = rtGCM->m_TiledIndex;
	m_Format = rtGCM->m_Format;
	m_BitsPerPix = rtGCM->m_BitsPerPix;
	m_Type = rtGCM->m_Type;
	m_ResourceTypeAndConversionFlags= rtGCM->m_ResourceTypeAndConversionFlags;
	m_LayerCount = rtGCM->m_LayerCount;
	
	m_PhysicalSizeAndTemplateType = rtGCM->m_PhysicalSizeAndTemplateType;
	m_HandleIndex = 0;

	UpdatePackedTexture();

#if HACK_GTA4
	// Because the gcmTexture is a local structure included as a flat structure within the rendertarget,
	// it got moved around (this is a shallow copy after all). So, we need to GetTexturePtr() rather than 
	// just get the original cached pointer, and make a copy of the CellGcmTexture data for good measure.
	m_CachedTexturePtr = GetTexturePtr();
#else
	m_CachedTexturePtr = rtGCM->m_CachedTexturePtr;
#endif
}

#if __PPU
// grab the back buffer 
// this is the full-screen back buffer the application uses
grcRenderTarget *grcTextureFactoryGCM::GetBackBuffer(bool /*realize*/) 
{
	if (g_BackBuffer)
	{
		// return the back buffer
		return g_BackBuffer;
	}
	else
	{
		return GetFrontBuffer();
	}
}

const grcRenderTarget *grcTextureFactoryGCM::GetBackBuffer(bool realize) const
{
	return GetBackBuffer(realize);
}

// grab the back buffer 
// this is the full-screen back buffer the application uses
grcRenderTarget *grcTextureFactoryGCM::GetFrontBuffer(bool nextBuffer) 
{
	// return the front buffer
	if (nextBuffer)
		return g_FrontBuffer[(GRCDEVICE.GetFrameCounter() + 1) % g_FrontBufferCount];
	else
		return g_FrontBuffer[GRCDEVICE.GetFrameCounter() % g_FrontBufferCount];
}

const grcRenderTarget *grcTextureFactoryGCM::GetFrontBuffer(bool nextBuffer) const
{
	return GetFrontBuffer(nextBuffer);
}

grcRenderTarget *grcTextureFactoryGCM::GetFrontBufferDepth(bool /*realize=true*/)
{
	return g_DepthBuffer;
}

const grcRenderTarget *grcTextureFactoryGCM::GetFrontBufferDepth(bool /*realize=true*/) const
{
	return g_DepthBuffer;
}

// grab the depth buffer 
// this is the full-screen depth buffer the application uses
grcRenderTarget *grcTextureFactoryGCM::GetBackBufferDepth(bool /*realize*/) 
{
	if (g_DepthBackBuffer)
	{
		// return the depth buffer.
		return g_DepthBackBuffer;
	}
	else
	{
		return g_DepthBuffer;
	}
}

const grcRenderTarget *grcTextureFactoryGCM::GetBackBufferDepth(bool realize) const
{
	return GetBackBufferDepth(realize);
}

#else
// grab the back buffer 
// this is the full-screen back buffer the application uses
grcRenderTarget *grcTextureFactoryGCM::GetBackBuffer(bool /*realize*/) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryGCM::GetBackBuffer(bool realize) const {
	return GetBackBuffer(realize);
}

grcRenderTarget *grcTextureFactoryGCM::GetFrontBuffer(bool) 
{
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryGCM::GetFrontBuffer(bool nextBuffer) const
{
	return GetFrontBuffer(nextBuffer);
}

// grab the depth buffer 
// this is the full-screen depth buffer the application uses
grcRenderTarget *grcTextureFactoryGCM::GetFrontBufferDepth(bool /*realize*/) 
{
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryGCM::GetFrontBufferDepth(bool realize) const
{
	return GetFrontBufferDepth(realize);
}

// grab the depth buffer 
// this is the full-screen depth buffer the application uses
grcRenderTarget *grcTextureFactoryGCM::GetBackBufferDepth(bool /*realize*/) 
{
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryGCM::GetBackBufferDepth(bool realize) const
{
	return GetBackBufferDepth(realize);
}

#endif

void grcTextureFactoryGCM::SetBufferDepth(grcRenderTargetGCM *PPU_ONLY(depthBuffer))
{
#if __PPU
	AssertMsg(NULL == g_DepthBackBuffer,"Default MSAA depth buffer already set. This is a one off operation");
	AssertMsg(NULL == g_DepthBuffer,"Default depth buffer already set. This is a one off operation");
	AssertMsg(HACK_MC4 || GRCDEVICE.GetMSAA() == grcDevice::MSAA_None,"Setting normal depth buffer while using MSAA, use SetBackBufferDepth when using MSAA");

	g_DepthBuffer = depthBuffer;
	SetDefaultRenderTarget(grcmrtDepthStencil, g_DepthBackBuffer);
#endif
}

void grcTextureFactoryGCM::SetBackBufferDepth(grcRenderTargetGCM *PPU_ONLY(depthBuffer))
{
#if __PPU
	AssertMsg(NULL == g_DepthBackBuffer,"Default MSAA depth buffer already set. This is a one off operation");
	AssertMsg(HACK_MC4 || NULL == g_DepthBuffer,"Default depth buffer already set. This is a one off operation");
	AssertMsg(GRCDEVICE.GetMSAA() != grcDevice::MSAA_None,"Setting MSAA depth buffer while not using MSAA, use SetBufferDepth when using MSAA");
	g_DepthBackBuffer = depthBuffer;
	SetDefaultRenderTarget(grcmrtDepthStencil, g_DepthBackBuffer);
#endif
}
	
}	// namespace rage

inline rage::u32 l2f(rage::u32 x)
{
	rage::u32 r = 0;
	while (x >>= 1)
		++r;
	return r;
}

void PackedCellGcmTexture::PackFrom(const CellGcmTexture &t)
{
	// ((location) + 1) | ((cubemap) << 2) | ((border) << 3) | ((dimension) << 4) | ((format) << 8) | ((mipmap) << 16) )
	format = (t.location + 1) | (t.cubemap << 2) | (1<<3) | (t.dimension << 4) | (t.format << 8) | (t.mipmap << 16) | ((t._padding & 0xF) << 20) | (l2f(t.depth) << 24) | ((t._padding & 0xF0) << 24);
	Assert((1 << ((format >> 24) & 0xF)) == t.depth);
	imagerect = t.height | (t.width << 16);
	Assign(remap,t.remap & 0xFFFEFFFF);
	// Observed pitch values: 32, 5120, 10240, otherwise zero or a multiple of 64.  So we have several bits free here if necessary, only 9 are actually significant
	Assign(pitch,t.pitch | ((t.remap >> 16) & 1));
	offset = t.offset;
}

void PackedCellGcmTexture::UnpackTo(CellGcmTexture &t)
{
	t.format = (format >> 8) & 0xFF;
	t.mipmap = (format >> 16) & 0xF;
	t.dimension = (format >> 4) & 3;
	t.cubemap = (format >> 2) & 1;
	t.location = (format >> 1) & 1;
	t._padding = ((format >> 24) & 0xF0) | ((format >> 20) & 0xF);
	t.depth = 1 << ((format >> 24) & 0xF);
	t.height = imagerect & 0xFFFF;
	t.width = imagerect >> 16;
	t.remap = remap | ((rage::u32)(pitch & 1) << 16);
	t.pitch = pitch & 0xFFFE;
	t.offset = offset;
}


#endif
