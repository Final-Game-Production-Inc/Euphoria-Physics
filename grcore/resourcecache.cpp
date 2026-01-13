//
// grcore/resourcecache.cpp
//
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved.
//

#include "resourcecache.h"

#include "bank\bkmgr.h"
#include "bank\bank.h"
#include "channel.h"
#include "file\stream.h"
#include "grcore\debugdraw.h"
#include "image.h"
#include "profile\timebars.h"
#include "system\param.h"
#include "system\timer.h"
#include "texturepc.h"
#include "wrapper_d3d.h"

#if (__D3D11)
#include "texture_d3d11.h"
#include "texturefactory_d3d11.h"
#include "rendertarget_d3d11.h"
#else
#include "texture_d3d9.h"
#endif // __D3D11

#if __WIN32PC
#if defined(NV_SUPPORT) && NV_SUPPORT
#include "../../3rdParty/NVidia/nvapi.h"
#endif
#endif // __WIN32PC

#if USE_RESOURCE_CACHE

PARAM(noprecache, "[MEMORY] Do not precache resources");
PARAM(nomemrestrict, "[MEMORY] Do not restrict the amount of available memory for managed resources");
PARAM(memrestrict, "[MEMORY] Set the restriction the amount of available memory for managed resources");
NOSTRIP_PARAM(reserve, "[MEMORY] Amount of memory to set aside for other applications");
PARAM(no_3GB, "[MEMORY] Disable 32bit OS with /3GB");
NOSTRIP_PARAM(reservedApp, "[MEMORY] Amount of memory to leave available within application space");
PARAM(nocache, "[MEMORY] Do not cache resources");
PARAM(lazydelete, "[MEMORY] Take your time deleting stuff");
PARAM(extravidmem, "[MEMORY] Set amount of extra video memmory to report in MB");
PARAM(allowgrowth, "[MEMORY] Allow video memory usage to grow beyond limits");
PARAM(forcefailedresources, "[MEMORY] Fail all allocations to see that we remain stable");
PARAM(displayStats, "[MEMORY] Displays on screen memory statistics reports on resources");
PARAM(disableCooldownTimer, "[MEMORY] Disable over memory budget cool down timer");
NOSTRIP_PARAM(percentageForStreamer, "[MEMORY] Percentage of memory available for streamer");

using namespace rage;

//-----------------------------------------------------------------------------
#define DEBUGINFO(x) grcDisplayf(x)
#define DEBUG_RESOURCE(x) // x
#define HEAVY_VALIDATE (0)

#if __WIN32PC
#define AlignTo(Addr, align) (((u32)Addr + align-1) & (~(align-1)))
#else
#define AlignTo(Addr, align) (((u32)Addr + align) & (~(align-1)))
#endif

#if (__D3D11)
	#define LARGEST_ITEM oResource
#else
	#define LARGEST_ITEM oTexture
#endif


//-----------------------------------------------------------------------------
#if __BANK
bool grcResourceCache::m_bDisplayStats = false;
bool grcResourceCache::m_bDisplaySystemMemStats = false;
bool grcResourceCache::m_bResetStats = false;
bool grcResourceCache::m_bDumpMemoryStats = false;
bool grcResourceCache::m_bDumpRenderTargetDependencies = false;
s64 grcResourceCache::sm_iWasteSystemMemory = 256 * 1024 * 1024;
s64 grcResourceCache::sm_iWasteVideoMemory = 64 * 1024 * 1024;
#endif // __BANK

const DWORDLONG cuKiloBytes = 1024;
const DWORDLONG cuMegaBytes = 1024 * 1024;

#define MAX_DATA_RESOURCES (320 * 1024)
#define MAX_DATA_RESORUCES_PRIME (327707)

bool grcResourceCache::sm_bManagerResources = true;
float grcResourceCache::sm_fPercentageForCache = REUSE_RESOURCE ? 0.10f : 0.0f;

float g_fPercentAvailablePostFragmentation = 0.99f;
float g_fPercentForStreamer = 0.85f; // Percentage of Available Video memory set aside for GTA Streamer

s64 grcResourceCache::m_aiTotalAvailableMemory[MT_Last] = { 0, 0 };
s64 grcResourceCache::m_aiTotalPhysicalMemory[MT_Last] = { 0, 0 };
s64 grcResourceCache::sm_aiReservedMemory[MT_Last] = { 0, 24 * 1024 * 1024 };
s64 grcResourceCache::sm_aiReservedApplicationMemory[MT_Last] = { 0, 288 * 1024 * 1024 };
s64 grcResourceCache::sm_aiMemoryRestriction[MT_Last] = { 0, 0 };
s64 grcResourceCache::sm_aiUnloadMemory[MT_Last] = { 0, 0 };
s64 grcResourceCache::sm_aiMaxCacheMemory[MT_Last] = { 256 * 1024 * 1024, 128 * 1024 * 1024 };

s64  grcResourceCache::sm_aiExtraAvailableMemory[MT_Last] = { 0, 0 };
bool grcResourceCache::sm_bExtraCooldownTimer = true;
u32  grcResourceCache::sm_uCooldownDelayMS = 10000;	 // 10 second wait before reducing the memory
u32  grcResourceCache::sm_uExtraUpdateTime = 0;

float grcResourceCache::sm_fMemoryReductionRate = 0.9999f; // Reduce extra memory down by this per frame after cool down delay has expired

sysIpcCurrentThreadId grcResourceCache::sm_iPlacementThreadId = NULL;

#if __ASSERT && RSG_PC
bool grcResourceCache::sm_SafeCreate = false;
#endif

#if USE_CREATION_THREAD
sysIpcThreadId grcResourceCache::s_CreationWorkerThread = sysIpcThreadIdInvalid;
MUTEX_TYPE grcResourceCache::m_MutexResourcingQueue;
atFixedArray<grcIndexBuffer*, 4096> grcResourceCache::m_queueForResourcingIndexBuffers;
atFixedArray<grcVertexBuffer*, 4096> grcResourceCache::m_queueForResourcingVertexBuffers;
atFixedArray<grcTexture*, 4096> grcResourceCache::m_queueForResourcingTextures;
#endif // USE_CREATION_THREAD

OSVersion grcResourceCache::sm_eOS = OS_XP;
bool grcResourceCache::sm_bUseLargeMemory = false;

#if __BANK
bool grcResourceCache::m_bSaveResourceData = false;
bool grcResourceCache::m_bSaveRTs = false;
#endif // __BANK

#if CONTROL_RATE_OF_CREATIONS
float	grcResourceCache::m_fDataCreationRate = (512.0f * 1024.0f * 1024.0f) / (33.0f / 1000.0f);
float	grcResourceCache::m_fCreateRate = 100;
float   grcResourceCache::m_fFalloffFromPreviousFrame = 0.97f;
float	grcResourceCache::m_fMaxWaitTime = 33.0f / 1000.0f;

float	grcResourceCache::m_fCurrentDataCreateRate = 0.0f;
float	grcResourceCache::m_fCurrentCreateRate = 0.0f;
#endif

#if !__TOOL
extern sysIpcCurrentThreadId g_MainThreadId = NULL;
#endif


GameRunningCallback grcResourceCache::sm_fnGameRunning = NULL;

enum TextureQuality
{
	LOW,
	MEDIUM,
	HIGH,
	VERY_HIGH,
	TQ_LAST
};

const u32 auMatchSize[RT_LAST] =
{
	0,		// RT_Texture
	0,		// RT_VolumeTexture
	0,		// RT_CubeTexture
	0,		// RT_RenderTarget
	0,		// RT_DepthStencil
	0,		// RT_ConstantBuffer
	4096,	// RT_VertexBuffer
	4096,	// RT_IndexBuffer
	4096,	// RT_StreamOutput
	4096,	// RT_BufferStaging
	4096	// RT_UnorderedAccess
};

const char astrManufacturer[DM_LAST][16] =
{
	{ "ATI" }, 
	{ "NVidia" },
	{ "Intel" },
	{ "Other" },
};
CompileTimeAssert(NELEM(astrManufacturer)==DM_LAST);

const char astrTextureLevel[TQ_LAST][16] =
{
	{ "Low" }, 
	{ "Medium"},
	{ "High" }
};

const char astrResourceNames[RT_LAST+1][24] = 
{
	"Texture",
	"Volume",
	"Cube",
	"Render Target",
	"Depth Target",
	"Constant Buf",
	"Vertex",
	"Index",
	"Stream Out",
	"Indirect Buf",
	"Staging",
	"Unordered Buf",
	"Depth View",
	"Render View",
	"Shader View",
	"Unorder View",
	"Total"
};

const char astrMemoryNames[MT_Last][16] =
{
	"Video",
	"System"
};

s64 aiTextureMaxMem[OS_LAST * 2][DM_LAST + 1][TQ_LAST] =
{
	{ // XP 32
		{ // ATI
			900 * 1024 * 1024,
			1100 * 1024 * 1024,
			1600 * 1024 * 1024,
			1600 * 1024 * 1024
		},
		{ // NVidia
			900 * 1024 * 1024,
			1100 * 1024 * 1024,
			1600 * 1024 * 1024,
			1600 * 1024 * 1024
		},
		{ // Other
			900 * 1024 * 1024,
			1100 * 1024 * 1024,
			1600 * 1024 * 1024,
			1600 * 1024 * 1024
		},
		{ // Max
			900 * 1024 * 1024,
			1100 * 1024 * 1024,
			1600 * 1024 * 1024,
			1600 * 1024 * 1024
		}
	},
	{ // Vista/Win7 32
		{ // ATI
			900 * 1024 * 1024,
			1100 * 1024 * 1024,
			1600 * 1024 * 1024,
			1600 * 1024 * 1024
		},
		{ // NVidia
			900 * 1024 * 1024,
			1100 * 1024 * 1024,
			1600 * 1024 * 1024,
			1600 * 1024 * 1024
		},
		{ // Other
			900 * 1024 * 1024,
			1100 * 1024 * 1024,
			1600 * 1024 * 1024,
			1600 * 1024 * 1024
		},
		{ // Max
			900 * 1024 * 1024,
			1100 * 1024 * 1024,
			1600 * 1024 * 1024,
			1600 * 1024 * 1024
		}
	},
	{ // XP 64
		{ // ATI
			2000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL
		},
		{ // NVidia
			2000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
		},
		{ // Other
			2000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
		},
		{ // Max
			4000LL * 1024LL * 1024LL,
			4000LL * 1024LL * 1024LL,
			4000LL * 1024LL * 1024LL,
			4000LL * 1024LL * 1024LL,
		}
	},
	{ // Vista/Win7 64
		{ // ATI
			2000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
		},
		{ // NVidia
			2000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
		},
		{ // Other
			2000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
			3000LL * 1024LL * 1024LL,
		},
		{ // Max
			4000LL * 1024LL * 1024LL,
			4000LL * 1024LL * 1024LL,
			4000LL * 1024LL * 1024LL,
			4000LL * 1024LL * 1024LL,
		}
	}
};

const u32 keyTable[] =
{
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
	0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
	0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
	0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
	0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,

	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
	0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
	0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
	0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
	0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
	0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
	0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,

	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
	0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
	0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
	0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
	0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,

	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
	0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
	0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
	0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
	0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

typedef struct _TEXTUREDATA {
	u32 uUsage;
	u32 uPool;
	union {
		u32 uWidth;
		u32 uEdgeLength;
	};
	u32 uHeight;
	u32 uDepth;
	u32 uLevels;
	u32 uFormat;
	u32 uMultiSample;
	u32 uMultiSampleQuality;
	union {
		u32 uLockable;
		u32 uDiscard;
	};
} TEXTUREDATA;

typedef struct _GEOMETRYDATA
{
	u32 uUsage;
	u32 uPool;
	u32 uSize;
	union {
		u32 uFVF;
		u32 uFormat;
	};
} GEOMETRYDATA;

#if (__D3D11)
typedef struct _grcResourceDesc {
	union 
	{
		D3D10_TEXTURE1D_DESC oTex1D_10;
		D3D10_TEXTURE2D_DESC oTex2D_10;
		D3D10_TEXTURE3D_DESC oTex3D_10;
		D3D10_BUFFER_DESC	 oBuffer_10;
		/* Identical to 10
		D3D11_TEXTURE1D_DESC oTex1D_11;
		D3D11_TEXTURE2D_DESC oTex2D_11;
		D3D11_TEXTURE3D_DESC oTex3D_11;
		*/
		D3D11_BUFFER_DESC	 oBuffer_11;
		D3D11_DEPTH_STENCIL_VIEW_DESC oDepthStencilView;
		D3D11_RENDER_TARGET_VIEW_DESC oRenderTargetView;
		D3D11_SHADER_RESOURCE_VIEW_DESC oShaderResourceView;
		D3D11_UNORDERED_ACCESS_VIEW_DESC oUnorderAccessView;
	};
} grcResourceDesc;
#endif // (__D3D11)

CompileTimeAssert(sizeof(GEOMETRYDATA) <= sizeof(TEXTUREDATA));


namespace rage
{
extern GUID TextureBackPointerGuid;

class ResourceData
{
public:
	ResourceData();
	virtual ~ResourceData() {}
    void clear();

	ResourceType	eType;
	u32				uSize;
	union 
	{
		TEXTUREDATA  oTexture;
		GEOMETRYDATA oGeometry;
#if (__D3D11)
		grcResourceDesc	oResource;
#endif // (__D3D11)
	};

	u32				auMemory[MT_Last];
	u32				uCRC;
	void*			pvResource;
	void*			pvRetResource;
	u32				uRequestSize;
	atDNode<ResourceData*, rage::datBase> Node;
};
}

//-----------------------------------------------------------------------------
ResourceData::ResourceData()
{
	clear();
}

void ResourceData::clear()
{
	eType = RT_LAST;
	memset(&LARGEST_ITEM,0,sizeof(LARGEST_ITEM));

	uCRC = 0;
	uSize = 0;
	auMemory[MT_Video] = auMemory[MT_System] = 0;
	pvResource = NULL;
	pvRetResource = NULL;
}

//-----------------------------------------------------------------------------
void grcResourceCache::InitClass()
{
	GetInstance().ResetStats();
}

void grcResourceCache::ShutdownClass()
{
	GetInstance().UnloadCache();

#if __BANK
	// Report Leaks Asserts
	if (grcResourceCache::GetInstance().GetActiveCount(MT_Video, RT_Totals) > 0)
	{
		grcResourceCache::GetInstance().SaveResourceData(true);
		grcResourceCache::GetInstance().SaveRenderTargets(true);

	}
#endif // __FINAL
}

grcResourceCache& grcResourceCache::GetInstance()
{
	static grcResourceCache oResCache;
	return oResCache;
}

const char* grcResourceCache::GetResourceName(ResourceType eResType)
{
	Assert(eResType <= RT_Totals);
	return astrResourceNames[eResType];
}

u32 grcResourceCache::GetCRCForResource(const ResourceData &oResource)
{
	const char *data = (const char*)&oResource.eType;
	s32 len = sizeof(ResourceType) + oResource.uSize;
	u32 crc = 0xffffffff;

	for( ; len > 0; len-- )
	{
		crc = (crc >> 8) ^ keyTable[((*data) & 0xff) ^ (crc & 0xff)];
		data++;
	}
	return (crc);
}

OSVersion grcResourceCache::GetOSVersion()
{
	OSVERSIONINFOEX oVersion;
	DWORD dwTypeMask = VER_BUILDNUMBER | VER_MAJORVERSION | VER_MINORVERSION | VER_PLATFORMID | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR | VER_SUITENAME | VER_PRODUCT_TYPE;
	DWORDLONG dwlConditionMask = 0;
	oVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (VerifyVersionInfo(&oVersion, dwTypeMask, dwlConditionMask))
	{
		if (oVersion.dwMajorVersion >= 6 && oVersion.dwMinorVersion >= 0)
		{
			return OS_VISTA;
		}
		else
		{
			return OS_XP;
		}
	}
	else
	{
		return OS_VISTA;
	}
}

void grcResourceCache::DeviceLost()
{
	VIDEOMEMORY_LOCK_RESOURCES;
	GetInstance().UnloadCache();
	VIDEOMEMORY_UNLOCK_RESOURCES;
}

void grcResourceCache::DeviceReset()
{
	VIDEOMEMORY_LOCK_RESOURCES;

	// Account of change in screen size
	sysMemCpy(grcResourceCache::GetInstance().m_aiTotalAvailableMemory, grcResourceCache::GetInstance().m_aiTotalPhysicalMemory, sizeof(grcResourceCache::GetInstance().m_aiTotalPhysicalMemory));

	// Front and Back buffers and Z not accounted for - Should just set reserved memory for this.  No Z created for GTA on device create
	grcResourceCache::GetInstance().UpdateTotalAvailableMemory();
	VIDEOMEMORY_UNLOCK_RESOURCES;
}

grcResourceCache::grcResourceCache()
{
	//Functor0 fnLostCb  = MakeFunctor(grcResourceCache::DeviceLost);
	//Functor0 fnResetCb = MakeFunctor(grcResourceCache::DeviceReset);
	//GRCDEVICE.RegisterDeviceLostCallbacks(fnLostCb, fnResetCb);
	CREATE_MUTEX(m_MutexCachingQueue);
#if USE_CREATION_THREAD
	CREATE_MUTEX(m_MutexResourcingQueue);
#endif // USE_CREATION_THREAD

	m_DeviceCreate = sysIpcCreateMutex();

	for (u32 uMemType = 0; uMemType < MT_Last; uMemType++)
	{
		CREATE_MUTEX(m_aMutex[uMemType]);
		m_auLockCount[uMemType] = 0;
		m_aiTotalAvailableMemory[uMemType] = m_aiTotalPhysicalMemory[uMemType] = 0;
		m_aThreadID[uMemType] = sysIpcCurrentThreadIdInvalid;

		for (u32 uStack = 0; uStack < LOCK_STACK_DEPTH; uStack++)
		{
			m_apszFile[uMemType][uStack] = NULL;
			m_auLineNumber[uMemType][uStack] = 0;
		}
	}
	
	m_bWarmedCache = false;
	m_bLazyDelete = false;
	if (PARAM_lazydelete.Get())
		m_bLazyDelete = true;

	m_ResDataPool = rage_new atPool<ResourceData>(MAX_DATA_RESOURCES);

	Assert(m_mapActiveResources.max_size() > MAX_DATA_RESORUCES_PRIME);
#if defined(_MSC_VER) && (_MSC_VER <= 1600) // VS2010 and down
	// nop
#else
	m_mapActiveResources.reserve(MAX_DATA_RESORUCES_PRIME);
#endif

	for (u32 uIndex = 0; uIndex < RT_LAST + 1; uIndex++)
	{
		for (u32 uMemType = 0; uMemType < MT_Last; uMemType++)
		{
			m_auTypeTotalActiveMemory[uMemType][uIndex] = 0;
			m_auTotalActiveItems[uMemType][uIndex] = 0;
			m_auNewAddItems[uMemType][uIndex] = 0;

#if REUSE_RESOURCE
			m_auTypeTotalCachedMemory[uMemType][uIndex] = 0;
			m_auTotalCacheItems[uMemType][uIndex] = 0;
			m_auNewCacheItems[uMemType][uIndex] = 0;	
			m_auNewClearCacheItems[uMemType][uIndex] = 0;
			m_auNewUsedCacheItems[uMemType][uIndex] = 0;
#endif // REUSE_RESOURCE
		}
	}
	sm_eOS = GetOSVersion();

	if (sysMemTotalMemory() > (2.2f * 1024 * 1024 * 1024))
	{
		if (sm_eOS)
			sm_aiReservedMemory[MT_System] = 24 * 1024 * 1024;
		else
			sm_aiReservedMemory[MT_System] = 288 * 1024 * 1024;
	}
	u32 uReserve = 0;
	if (PARAM_reserve.Get(uReserve))
	{
		sm_aiReservedMemory[MT_System] = Max(1U, uReserve);
	}

	if (PARAM_reservedApp.Get(uReserve))
	{
		sm_aiReservedApplicationMemory[MT_System] = Max(1U, uReserve);
	}

	if (PARAM_extravidmem.Get(uReserve))
	{
		sm_aiExtraAvailableMemory[MT_Video] = Max(0U, 1024 * 1024 * uReserve);
	}

	// We check if the OS is 64 Bit
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

	LPFN_ISWOW64PROCESS 
		fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle("kernel32"),"IsWow64Process");

	BOOL b64BitOS = false;
	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(),&b64BitOS))
		{
			// handle error
		}
	}
	sm_bUseLargeMemory = (b64BitOS) ? true : false;

	if (!b64BitOS)
	{
		MEMORYSTATUSEX oData;
		oData.dwLength = sizeof(MEMORYSTATUSEX);

		if (GlobalMemoryStatusEx(&oData))
		{
			if (oData.ullAvailVirtual > (2*1024*1024*1024UL))
			{
				sm_bUseLargeMemory = true;
			}
		}
		else
		{
			Errorf("GlobalMemoryStatusEx Failed - %u", GetLastError());
		}
	}

	if (PARAM_no_3GB.Get())
	{
		sm_bUseLargeMemory = false;
	}

	ResetStats();

	UpdateTotalAvailableMemory();
#if USE_CREATION_THREAD
	s_CreationWorkerThread = sysIpcCreateThread(CreationWorker,0,sysIpcMinThreadStackSize,PRIO_ABOVE_NORMAL,"[RAGE] Resource Creation Worker",true);
#endif // USE_CREATION_THREAD

#if !__FINAL
	m_bForceFailures = false;
	if (PARAM_forcefailedresources.Get())
		m_bForceFailures = true;
#endif // !__FINAL

#if __BANK
	if (PARAM_displayStats.Get())
		m_bDisplayStats = true;
#endif // __BANK

	if (PARAM_disableCooldownTimer.Get())
		sm_bExtraCooldownTimer = false;

#if HACK_GTA4
	const s64 vram = GRCDEVICE.GetAvailableVideoMemory();
	const s64 limit = static_cast<s64>(2) << 30;
	if (vram < limit)
		g_fPercentForStreamer = 0.95f;
	else if (GRCDEVICE.GetManufacturer() == ATI)
		g_fPercentForStreamer = 0.8f; // Until they fix their shit 0.6 would be the true value to use
#endif // HACK_GTA

	PARAM_percentageForStreamer.Get(g_fPercentForStreamer);
}

#if USE_CREATION_THREAD
void grcResourceCache::CreationWorker(void*)
{
	while(1)
	{
		if (m_MutexResourcingQueue.TryLock())
		{
			bool bSleep = true;
			// Process one file at a time to minimize lock time
			if (!m_queueForResourcingIndexBuffers.empty())
			{
				grcIndexBufferD3D* poIndexBuffer = (grcIndexBufferD3D*)m_queueForResourcingIndexBuffers.Pop();
				if (poIndexBuffer != NULL)
				{
					Assert(poIndexBuffer->IsDirty());
					poIndexBuffer->Update(true);
					bSleep = false;
				}
			}
			else if (!m_queueForResourcingVertexBuffers.empty())
			{
				grcVertexBufferD3D* poVertexBuffer = (grcVertexBufferD3D*)m_queueForResourcingVertexBuffers.Pop();
				if (poVertexBuffer != NULL)
				{
					Assert(poVertexBuffer->IsDirty());
					poVertexBuffer->Update(true);
					bSleep = false;
				}
			}
			else if (!m_queueForResourcingTextures.empty())
			{
				grcTexturePC* poTexture = (grcTexturePC*)m_queueForResourcingTextures.Pop();
				if (poTexture != NULL)
				{
					Assert(poTexture->IsDirty());
					poTexture->Update(true);
					bSleep = false;
				}
			}
			m_MutexResourcingQueue.Unlock();
			if (bSleep)
			{
				sysIpcSleep(1);
			}
		}
		else
		{
			sysIpcSleep(1);
		}
	}
}

bool grcResourceCache::QueueForCreation(grcIndexBuffer* pvResource)
{
	m_MutexResourcingQueue.Lock();
	Assert(pvResource != NULL);
	m_queueForResourcingIndexBuffers.Push(pvResource);
	m_MutexResourcingQueue.Unlock();
	return true;
}

bool grcResourceCache::QueueForCreation(grcVertexBuffer* pvResource)
{
	m_MutexResourcingQueue.Lock();
	Assert(pvResource != NULL);
	m_queueForResourcingVertexBuffers.Push(pvResource);
	m_MutexResourcingQueue.Unlock();
	return true;
}


bool grcResourceCache::QueueForCreation(grcTexture* pvResource)
{
	m_MutexResourcingQueue.Lock();
	Assert(pvResource != NULL);
	m_queueForResourcingTextures.Push(pvResource);
	m_MutexResourcingQueue.Unlock();
	return true;
}


bool grcResourceCache::ClearFromCreationQueue(void* pvResource)
{
	m_MutexResourcingQueue.Lock();
	s32 iIndex;
	for (iIndex = 0; iIndex < m_queueForResourcingIndexBuffers.GetCount(); iIndex++)
	{
		if (m_queueForResourcingIndexBuffers[iIndex] == pvResource)
		{ // Found remove
			m_queueForResourcingIndexBuffers[iIndex] = NULL; // Check for NULL in worker thread
			m_MutexResourcingQueue.Unlock();
			return true;
		}
	}
	for (iIndex = 0; iIndex < m_queueForResourcingVertexBuffers.GetCount(); iIndex++)
	{
		if (m_queueForResourcingVertexBuffers[iIndex] == pvResource)
		{ // Found remove
			m_queueForResourcingVertexBuffers[iIndex] = NULL; // Check for NULL in worker thread
			m_MutexResourcingQueue.Unlock();
			return true;
		}
	}
	for (iIndex = 0; iIndex < m_queueForResourcingTextures.GetCount(); iIndex++)
	{
		if (m_queueForResourcingTextures[iIndex] == pvResource)
		{ // Found remove
			m_queueForResourcingTextures[iIndex] = NULL; // Check for NULL in worker thread
			m_MutexResourcingQueue.Unlock();
			return true;
		}
	}
	Warningf("Failed to find item in creation queue");
	m_MutexResourcingQueue.Unlock();
	return false;
}
#endif // USE_CREATION_THREAD


bool grcResourceCache::UpdateTotalAvailableMemory()
{
	m_aiTotalAvailableMemory[MT_Video] = GRCDEVICE.GetAvailableVideoMemory();
	bool bRet = false;
	if (m_aiTotalAvailableMemory[MT_Video] > 0)
	{
		m_aiTotalAvailableMemory[MT_Video] = (s64)((float)m_aiTotalAvailableMemory[MT_Video] * g_fPercentAvailablePostFragmentation);
		m_aiTotalPhysicalMemory[MT_Video]  = m_aiTotalAvailableMemory[MT_Video];

		m_aiTotalAvailableMemory[MT_Video] -= GRCDEVICE.GetWidth() * GRCDEVICE.GetHeight() * sizeof(DWORD) * 3;
		//m_aiTotalAvailableMemory[MT_Video] += GetTotalUsedMemory(MT_Video); // PC TODO - Depends if you are able to get the free memory properly

		D3D11_ONLY(grcDisplayf("Using %" I64FMT "d bytes video memory\n",m_aiTotalAvailableMemory[MT_Video]));

		m_aiTotalAvailableMemory[MT_System] = (s64)Max(sysMemTotalMemory(), (u64)0x7FFFFFFF);
		bRet = true;
	}
	/* SLI reports 500MB missing on 1.5GB card.  Still use this for free memory report.
	if (GetVideoCardFreeMemory() > 0)
	{
		m_aiTotalAvailableMemory[MT_Video] = GetVideoCardFreeMemory() + GetTotalUsedMemory(MT_Video);
		bRet = true;
	}
	*/
	return true;
}

float grcResourceCache::GetPercentageForStreaming() const
{
	return g_fPercentAvailablePostFragmentation;
}

s64 grcResourceCache::GetStreamingMemory() const
{
	return GetStreamingMemory(g_fPercentForStreamer);
}

void grcResourceCache::SetExtraMemory(MemoryType eType, s64 iExtraMemory)
{ 
	Assert(eType < MT_Last); 
	sm_aiExtraAvailableMemory[eType] = iExtraMemory; 
	sm_uExtraUpdateTime = sysTimer::GetSystemMsTime() + sm_uCooldownDelayMS;
}

s64 grcResourceCache::GetStreamingMemory(float fPercentageOfMemoryToUse) const
{
	if (sm_bExtraCooldownTimer)
	{
		if (sm_uExtraUpdateTime < sysTimer::GetSystemMsTime())
		{
			for (u32 uType = 0; uType < MT_Last; uType++)
			{
				sm_aiExtraAvailableMemory[uType] = (s64)(sm_aiExtraAvailableMemory[uType] * sm_fMemoryReductionRate);
			}
		}
	}

	s64 iAvailable = GetTotalFreeMemory(rage::MT_Video);
	if (iAvailable > 0)
	{
		s64 iPhysicalAvailable = (s64)((float)(GetTotalMemory(rage::MT_Video) + GetExtraMemory(rage::MT_Video)) * fPercentageOfMemoryToUse) - GetActiveMemory(rage::MT_Video, rage::RT_Totals) - GetActiveMemory(MT_System, RT_Texture);
		iAvailable = Min(iAvailable, iPhysicalAvailable);
	}
	iAvailable += GetQueuedMemoryForDeletion(rage::MT_Video);

	if (GetTotalMemory(rage::MT_System) < ((u64)2560 * (u64)1024 * (u64)1024)) // Lower end machines need to worry about system memory
	{
		iAvailable = Min(iAvailable, GetTotalFreeMemory(rage::MT_System)); // This function is bloody slow
	}
	iAvailable = Min(iAvailable,(s64)0x7FFFFFFF);
	return iAvailable;
}

s64 grcResourceCache::GetQueuedMemoryForDeletion(MemoryType eType) const
{ 
	Assert(eType < MT_Last); 
	s64 iAmount = sm_aiUnloadMemory[eType];

	if ( m_lstItemToFree.GetCount() <= 0)
	{
		return iAmount;
	}
	LOCK_DELETION_QUEUE

	for (s32 iIndex = 0; iIndex < m_lstItemToFree.GetCount(); iIndex++)
	{
		void* pvResource = m_lstItemToFree[iIndex];
		Assert(pvResource != NULL);

		MapActive::const_iterator itActive = m_mapActiveResources.find(pvResource);
		Assertf(itActive != m_mapActiveResources.end(), "Unable to find resource %p type %d for deletion", pvResource, eType);
		if (itActive != m_mapActiveResources.end())
		{
			const ResourceData* poData = NULL;
			poData = itActive->second;
			for (u32 uMemory = 0; uMemory < MT_Last; uMemory++)
			{
				iAmount += poData->auMemory[uMemory];
			}
		}
	}
	UNLOCK_DELETION_QUEUE
	return iAmount;
}

grcResourceCache::~grcResourceCache()
{
	DESTROY_MUTEX(m_MutexCachingQueue);
#if USE_CREATION_THREAD
	DESTROY_MUTEX(m_MutexResourcingQueue);
#endif // USE_CREATION_THREAD
	for (u32 uMemType = 0; uMemType < MT_Last; uMemType++)
	{
		DESTROY_MUTEX(m_aMutex[uMemType]);
	}
}

bool grcResourceCache::Lock(MemoryType eMemory 
#if !(__FINAL || __NO_OUTPUT)
							, const char* pszFile, u32 uLine
#endif // !__FINAL
							)
{
	Assert(eMemory < MT_Last);
	m_aMutex[eMemory].Lock();
#if !(__FINAL || __NO_OUTPUT)
	if (m_auLockCount[eMemory] == 0)
	{
		m_aThreadID[eMemory] = sysIpcGetCurrentThreadId();
	}
	else
	{
		Assertf((sysIpcGetCurrentThreadId() == m_aThreadID[eMemory]), "Last Lock came from a different thread Old %s %d - New %s %d", 
			m_apszFile[m_auLockCount[eMemory]], m_auLineNumber[m_auLockCount[eMemory]], pszFile, uLine);
		m_aThreadID[eMemory] = sysIpcGetCurrentThreadId();
	}
	m_apszFile[eMemory][m_auLockCount[eMemory]] = pszFile;
	m_auLineNumber[eMemory][m_auLockCount[eMemory]] = uLine;

	m_auLockCount[eMemory]++;
	Assert(m_auLockCount[eMemory] < LOCK_STACK_DEPTH);
#endif // !__FINAL
	return true; //bReturn;
}

bool grcResourceCache::Unlock(MemoryType eMemory 
#if !(__FINAL || __NO_OUTPUT)
							, const char* pszFile, u32 OUTPUT_ONLY(uLine)
#endif // !__FINAL
							)
{
#if !(__FINAL || __NO_OUTPUT)
	Assert(eMemory < MT_Last);
	Assertf((m_auLockCount[eMemory] > 0), "Lock/Unlock Mismatch %d", m_auLockCount[eMemory]);
	if (strcmp(pszFile, m_apszFile[eMemory][m_auLockCount[eMemory] - 1]))
	{
		Warningf("Lock/Unlock came from different items on the stack %s - %d - %s - %d", 
			m_apszFile[m_auLockCount[eMemory]], m_auLineNumber[m_auLockCount[eMemory]], pszFile, uLine);
	}
	m_apszFile[eMemory][m_auLockCount[eMemory]] = NULL;
	m_auLineNumber[eMemory][m_auLockCount[eMemory]] = 0;
	m_auLockCount[eMemory]--;
#endif
	m_aMutex[eMemory].Unlock();
	return true;
}

bool grcResourceCache::LockDeleteQueue(
#if !(__FINAL || __NO_OUTPUT)
	const char* pszFile, u32 uLine
#endif // !__FINAL
	)
{
	m_MutexCachingQueue.Lock();
#if !(__FINAL || __NO_OUTPUT)
	if (m_uDeleteLockCount == 0)
	{
		m_aDeleteThreadID = sysIpcGetCurrentThreadId();
	}
	else
	{
		Assertf((sysIpcGetCurrentThreadId() == m_aDeleteThreadID), "Last Lock came from a different thread Old %s %d - New %s %d", 
			m_apszDeleteFile[m_uDeleteLockCount], m_auDeleteLineNumber[m_uDeleteLockCount], pszFile, uLine);
		m_aDeleteThreadID = sysIpcGetCurrentThreadId();
	}
	Assert(m_uDeleteLockCount < LOCK_STACK_DEPTH);
	m_apszDeleteFile[m_uDeleteLockCount] = pszFile;
	m_auDeleteLineNumber[m_uDeleteLockCount] = uLine;
	m_uDeleteLockCount++;

#endif // !__FINAL
	return true;
}

bool grcResourceCache::UnlockDeleteQueue(
#if !(__FINAL || __NO_OUTPUT)
	const char* pszFile, u32 uLine
#endif // !__FINAL
	)
{
#if !(__FINAL || __NO_OUTPUT)
	Assertf((m_uDeleteLockCount > 0), "Lock/Unlock Mismatch %d", m_uDeleteLockCount);
	if (strcmp(pszFile, m_apszDeleteFile[m_uDeleteLockCount - 1]))
	{
		Warningf("Lock/Unlock came from different items on the stack %s - %d - %s - %d", 
			m_apszDeleteFile[m_uDeleteLockCount], m_auDeleteLineNumber[m_uDeleteLockCount], pszFile, uLine);
	}
	m_uDeleteLockCount--;
	m_apszDeleteFile[m_uDeleteLockCount] = NULL;
	m_auDeleteLineNumber[m_uDeleteLockCount] = 0;
#endif
	m_MutexCachingQueue.Unlock();
	return true;
}

bool grcResourceCache::IsLockedDeleteQueue(int ASSERT_ONLY(iExpectedStatus))
{
	Assertf((iExpectedStatus == -1 || (u32)iExpectedStatus == m_uDeleteLockCount), "%s - %d Locked last Expected %d Current Count %d", m_apszDeleteFile[m_uDeleteLockCount ? m_uDeleteLockCount - 1 : 0], m_auDeleteLineNumber[m_uDeleteLockCount ? m_uDeleteLockCount - 1 : 0], iExpectedStatus, m_uDeleteLockCount);
	// Not reliable - Use for assertions only
	return (m_uDeleteLockCount != 0) ? true : false;
}


#define USE_HEAP_ALLOCATION 1

#if USE_HEAP_ALLOCATION
	typedef struct _MemoryTracker
	{
		void* pvAddress;
		u32   uSize;
		u32   pad[2];
	} MemoryTracker;

	#define ALIGNMENT_SIZE 256
	#define DATA_SIZE (sizeof(MemoryTracker))
	#define EXTRA_TO_ALIGN (128 + DATA_SIZE * 2) // 16 byte alignment to data (original pointer and size)

	#define HEAP_ONLY(x) x
#else
	#define HEAP_ONLY(x)
	#define ALIGNMENT_SIZE 4096
	#define EXTRA_TO_ALIGN 0
#endif // USE_HEAP_ALLOCATION

void* grcResourceCache::Allocate(ResourceType eType, u32 uSize)
{
	PF_AUTO_PUSH_DETAIL("Allocate Sys Resource");
	// PC TODO - Probably caching and heaps can be used to reduce the memory foot print and save on calling the OS
	Assert(eType < RT_LAST);
	void* pvResource = NULL;

	// Search Cache for a match
#if ENABLE_SYSTEMCACHE
	SYSTEMMEMORY_LOCK_RESOURCES;
	uSize = AlignTo(uSize + EXTRA_TO_ALIGN, ALIGNMENT_SIZE);
	SystemMemorySizes::iterator itSysMem = m_mmapCacheSysSize.find(uSize);
	if (itSysMem != m_mmapCacheSysSize.end())
	{
		Assert(uSize <= itSysMem->first);
		SysFreeContainer* poListEntry = (SysFreeContainer*)itSysMem->second;
		// Found Matching Size - Use it
		// Remove from cache
		m_listSysMemCache.erase(poListEntry);

		m_auTotalCacheItems[MT_System][RT_Totals]--;
		m_auTypeTotalCachedMemory[MT_System][RT_Totals] -= itSysMem->first;
		m_auNewUsedCacheItems[MT_System][RT_Totals]++;
		m_mmapCacheSysSize.erase(itSysMem);
		pvResource = poListEntry;
	}
	else
	{
		SYSTEMMEMORY_UNLOCK_RESOURCES
#endif // ENABLE_SYSTEMCACHE
		pvResource = NULL;

		while(pvResource == NULL)
		{
#if USE_HEAP_ALLOCATION
			pvResource = sysMemHeapAllocate(uSize);
#else
			pvResource = sysMemVirtualAllocate(uSize);
#endif
			if (pvResource == NULL)
			{
				if (!ManageSystemMemoryCache(true))
				{
					LOCK_DELETION_QUEUE
					VIDEOMEMORY_LOCK_RESOURCES
					if (!CleanCache(MT_System))
					{
						if (!CleanCache(MT_Video))
						{
							Warningf("Failed to allocate %d bytes of system memory", uSize);
							Warningf("System(MB) - Free Memory %u Total %u Used %u", (u32)(GetTotalFreeMemory(MT_System) / cuMegaBytes, false), (u32)(GetTotalMemory(MT_System) / cuMegaBytes), (u32)(GetTotalUsedMemory(MT_System) / cuMegaBytes));
							Warningf("Mem Total(MB) %u Total Free %u Largest Free %u Free App %u Free Kernel %u", 
									(u32)(sysMemTotalMemory() / cuMegaBytes), (u32)(sysMemTotalFreeMemory() / cuMegaBytes), (u32)(sysMemLargestFreeBlock() / cuMegaBytes), (u32)(sysMemTotalFreeAppMemory() / cuMegaBytes), (u32)(sysMemTotalFreeKernelMemory() / cuMegaBytes));

							for (u32 uMemory = 0; uMemory < MT_Last; uMemory++)
							{
								for (u32 uResType = 0; uResType < RT_LAST; uResType++)
								{
									Warningf("Type %s - %u MB\n", astrResourceNames[uResType], (u32)(GetActiveMemory((MemoryType)uMemory, (ResourceType)uResType) / cuMegaBytes));
								}
							}

							Quitf(ERR_MEM_SYSTEM,"Failed to allocate system memory");
						}
					}
					VIDEOMEMORY_UNLOCK_RESOURCES
					UNLOCK_DELETION_QUEUE
				}
			}
		}
		//Displayf("Alloc %p %d total %d", pvResource, uSize, m_auTypeTotalCachedMemory[MT_System][RT_Totals] + m_auTypeTotalActiveMemory[MT_System][RT_Totals]);
		
	SYSTEMMEMORY_LOCK_RESOURCES;
		m_auNewAddItems[MT_System][eType]++;
#if ENABLE_SYSTEMCACHE
	}
#endif // !ENABLE_SYSTEMCACHE

	if (pvResource != NULL)
	{
		memset(pvResource, 0xAA, uSize);
#if USE_HEAP_ALLOCATION
		MemoryTracker *poAlignedResource = (MemoryTracker*)pvResource;
		poAlignedResource = (MemoryTracker*)AlignTo((size_t)poAlignedResource, 16);
		poAlignedResource->pvAddress = pvResource;
		poAlignedResource->uSize = uSize;
		poAlignedResource++;
		pvResource = (void*)poAlignedResource;		// Aligned Address
		poAlignedResource--;						// Move back to place data

		size_t iActualSize = poAlignedResource->uSize;
#else
		MEMORY_BASIC_INFORMATION oMemInfo;
		ASSERT_ONLY(u32 uMemSize = ) VirtualQuery(pvResource, &oMemInfo, sizeof(oMemInfo));
		Assert(uMemSize > 0);
		Assertf((uSize <= oMemInfo.RegionSize), "Request Size %d Query Size %d", uSize, uMemSize);
		size_t iActualSize = oMemInfo.RegionSize;
#endif

		m_auTypeTotalActiveMemory[MT_System][eType] += iActualSize; // (uSize + (D3DRUNTIME_MEMORYALIGNMENT - 1)) & ~(D3DRUNTIME_MEMORYALIGNMENT - 1);
		m_auTypeTotalActiveMemory[MT_System][RT_Totals] += iActualSize; // (uSize + (D3DRUNTIME_MEMORYALIGNMENT - 1)) & ~(D3DRUNTIME_MEMORYALIGNMENT - 1);
		m_auTotalActiveItems[MT_System][eType]++;
		m_auTotalActiveItems[MT_System][RT_Totals]++;
	}
	else
	{
		Assertf(0, "Failed to allocate memory for resource %s - Size %d", astrResourceNames[eType], uSize);
		// PC TODO - Need to resolve this somehow
	}
	SYSTEMMEMORY_UNLOCK_RESOURCES;
	return pvResource;
}

void grcResourceCache::Free(ResourceType eType, void* pvResource)
{
	// PC TODO - Probably caching and heaps can be used to reduce the memory foot print and save on calling the OS
	Assert(eType < RT_LAST);
	Assert(pvResource != NULL);
	if (pvResource == NULL)
		return;

	PF_PUSH_TIMEBAR("Free Sys Resource");
	size_t iSize = 0;

#if USE_HEAP_ALLOCATION
	MemoryTracker* poAlignedResource = (MemoryTracker*)pvResource;
	poAlignedResource--;
	iSize = poAlignedResource->uSize;
	pvResource = poAlignedResource->pvAddress;
	Assert(pvResource != NULL);
#else
	MEMORY_BASIC_INFORMATION oMemInfo;
	iSize = VirtualQuery(pvResource, &oMemInfo, sizeof(oMemInfo));
	Assert(iSize > 0);
	iSize = oMemInfo.RegionSize;
#endif

#if ENABLE_SYSTEMCACHE
	// Add to system cache
	SYSTEMMEMORY_LOCK_RESOURCES;
	SysFreeContainer* poListEntry = rage_placement_new(pvResource) SysFreeContainer;

	// Insert into Containers
	m_mmapCacheSysSize.insert(std::pair<size_t, rage::SysFreeContainer*>(iSize, poListEntry));
	m_listSysMemCache.push_front(poListEntry);

	m_auTotalCacheItems[MT_System][RT_Totals]++;
	m_auTypeTotalCachedMemory[MT_System][RT_Totals] += iSize;
	m_auNewCacheItems[MT_System][RT_Totals]++;
#else // !ENABLE_SYSTEMCACHE
	#if USE_HEAP_ALLOCATION
	if (sysMemHeapFree(pvResource))
	#else
	if (sysMemVirtualFree(pvResource))
	#endif // USE_HEAP_ALLOCATION
	{
		SYSTEMMEMORY_LOCK_RESOURCES;
#endif // ENABLE_SYSTEMCACHE
		m_auTypeTotalActiveMemory[MT_System][eType] -= iSize; //(iSize + (D3DRUNTIME_MEMORYALIGNMENT - 1)) & ~(D3DRUNTIME_MEMORYALIGNMENT - 1);
		m_auTypeTotalActiveMemory[MT_System][RT_Totals] -= iSize; //(iSize + (D3DRUNTIME_MEMORYALIGNMENT - 1)) & ~(D3DRUNTIME_MEMORYALIGNMENT - 1);
		m_auTotalActiveItems[MT_System][eType]--;
		m_auTotalActiveItems[MT_System][RT_Totals]--;
		SYSTEMMEMORY_UNLOCK_RESOURCES;
#if !ENABLE_SYSTEMCACHE
	}
	else
	{
		Assertf(0, "Failed to deallocate memory for resource %s", astrResourceNames[eType]);
	}
#endif // !ENABLE_SYSTEMCACHE

	PF_POP_TIMEBAR();
}

s64 grcResourceCache::GetVideoCardFreeMemory() const
{
#if defined(NV_SUPPORT) && NV_SUPPORT && 0 // New API no longer supports this
	if (GRCDEVICE.GetManufacturer() == NVIDIA)
	{
		// PC TODO - Figure out how to match index to actual adapter/display we are using
		NvDisplayHandle oDisplayHandle;
		NvAPI_Status eRet = NvAPI_EnumNvidiaDisplayHandle((NvU32)0, &oDisplayHandle);
		if (eRet == NVAPI_OK)
		{
			NV_DISPLAY_DRIVER_MEMORY_INFO oMemInfo;
			oMemInfo.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER;
			eRet = NvAPI_GetDisplayDriverMemoryInfo(oDisplayHandle, &oMemInfo);
			if (eRet == NVAPI_OK)
			{
				Assert(oMemInfo.version == NV_DISPLAY_DRIVER_MEMORY_INFO_VER_2);
				static bool bSpamMemory = false;
				if (bSpamMemory)
				{
					Displayf("Vid Mem - Dedicated: %u Avail Dedicated: %u Sys Vid: %u Shared Sys: %u Cur Avail Dedicated: %u",
						oMemInfo.dedicatedVideoMemory, 
						oMemInfo.availableDedicatedVideoMemory,
						oMemInfo.systemVideoMemory,
						oMemInfo.sharedSystemMemory,
						oMemInfo.curAvailableDedicatedVideoMemory);
				}
				return Max(oMemInfo.systemVideoMemory, oMemInfo.curAvailableDedicatedVideoMemory) * 1024;
			}
		}
	}
#endif // NV_SUPPORT
	return 0;
}

s64 grcResourceCache::GetTotalFreeMemory(MemoryType eType, bool bIgnoreCache) const
{ 
	if (eType == MT_Video)
	{
		/*
		s64 iFreeMemory = GetVideoCardFreeMemory();
		if (iFreeMemory > 0)
		{
			iFreeMemory += GetCachedMemory(eType, RT_Totals);
		}
		*/

		// Don't report cached memory as that can be freed on demand
		s64 iVideoMemoryFree = GetTotalMemory(MT_Video) + sm_aiExtraAvailableMemory[eType] 
							   - GetActiveMemory(eType, RT_Totals) /*GetTotalUsedMemory(eType)*/ 
							   - GetMemoryRestriction(eType) 
							   - (!__D3D11 ? GetActiveMemory(MT_System, RT_Texture) : (s64)0) 
							   - (bIgnoreCache ? 0 : GetCachedMemory(MT_Video, RT_Totals));

		s64 iTotalMem = (s64)iVideoMemoryFree;
		int iLimit = 0;

		// Reeanble nomemrestrict below if you want memory restrictions back on
		//if (!PARAM_nomemrestrict.Get()) // && UseRestrictions() )
		{
			if (!PARAM_memrestrict.Get(iLimit))
			{
				iTotalMem = Min(GetTotalMemory(MT_Video) + sm_aiExtraAvailableMemory[eType], 
								aiTextureMaxMem[sm_eOS + OS_LAST * sm_bUseLargeMemory][GRCDEVICE.GetManufacturer()][grcTexturePC::GetTextureQuality()] + grcRenderTargetPC::GetTotalMemory() + (bIgnoreCache ? 0 : (s64)(GetTotalMemory(MT_Video) * sm_fPercentageForCache))); // GetCachedMemory(MT_Video, RT_Totals)));
			}
			else
			{
				s64 iAmount = iLimit;
				iAmount *= 1024 * 1024;

				iTotalMem = Min(GetTotalMemory(MT_Video) + sm_aiExtraAvailableMemory[eType], 
								iAmount + grcRenderTargetPC::GetTotalMemory() + (bIgnoreCache ? 0 : (s64)(GetTotalMemory(MT_Video) * sm_fPercentageForCache))); //GetCachedMemory(MT_Video, RT_Totals)));
			}
		}
		/*
		else
		{
			iTotalMem = Min(GetTotalMemory(MT_Video), aiTextureMaxMem[sm_eOS + OS_LAST * sm_bUseLargeMemory][DM_LAST][grcTexturePC::GetTextureQuality()] + grcRenderTargetPC::GetTotalMemory());
		}
		*/

		iTotalMem = iTotalMem - GetActiveMemory(eType, RT_Totals) - (bIgnoreCache ? 0 : GetCachedMemory(MT_Video, RT_Totals)); //, GetActiveMemory(MT_System, RT_Totals))  /*GetTotalUsedMemory(eType)*/ /*- GetMemoryRestriction(eType) - GetActiveMemory(MT_System, RT_Texture)*/;

		// iTotalMem = (PARAM_allowgrowth.Get() && (iTotalMem == iVideoMemoryFree)) ? iTotalMem : iTotalMem);
		// iTotalMem =  Min(iTotalMem : 0x7FFFFFFF));
		return iTotalMem; // iVideoMemoryFree;
	}
	else
	{
		const s64 auUpdateRate[2] = { 133, 1333 };
		const s64 auMemoryLimits[2] = { 2 * 1024 * 1024, 32 * 1024 * 1024 };

		static u64 uNextUpdate = 0;
		static u32 uLastTime = 0;
		static s64 iTotalFreeAppMemory = 0;
		static s64 iTotalFreeMemory = 0;

		u32 uCurTime = sysTimer::GetSystemMsTime();
		if ((uCurTime < (uLastTime + uNextUpdate)) && (iTotalFreeAppMemory != 0))
		{
			// Do not update info
		}
		else
		{
			uLastTime = uCurTime;
			iTotalFreeAppMemory = sysMemTotalFreeAppMemory();
			iTotalFreeMemory = sysMemTotalFreeMemory();

			uNextUpdate = min(max(iTotalFreeAppMemory, auMemoryLimits[0]), auMemoryLimits[1]);
			uNextUpdate = (u64)(((uNextUpdate - auMemoryLimits[0]) / (float)(auMemoryLimits[1] - auMemoryLimits[0])) * (auUpdateRate[1] - auUpdateRate[0]) + auUpdateRate[0]);
		}

		// PC TODO - sysMemTotalFreeMemory/sysMemTotalFreeAppMemory are expensive calls should only update rarely
		s64 iFreeMemory = Max((s64)(iTotalFreeAppMemory - (s64)sm_aiReservedApplicationMemory[MT_System]), (s64)(iTotalFreeMemory) - (s64)sm_aiReservedMemory[MT_System]);
		iFreeMemory = Max((s64)0, iFreeMemory);
		return Min((s64)(0xFFFFFFFF), iFreeMemory);
	}
}

#if !__D3D11
HRESULT grcResourceCache::CreateTexture( UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE*)
{ 
	ResourceData oResource;
	oResource.eType = RT_Texture;
	oResource.uSize = sizeof(TEXTUREDATA);
	oResource.oTexture.uWidth = Width;
	oResource.oTexture.uHeight = Height;
	oResource.oTexture.uLevels = Levels;
	oResource.oTexture.uUsage = Usage;
	oResource.oTexture.uFormat = (u32)Format;
	oResource.oTexture.uPool = (u32)Pool;

	oResource.auMemory[MT_Video] = Width * Height * grcTextureFactoryDX9::GetBitsPerPixelStaticDX9((u32)Format) / 8; 
	oResource.auMemory[MT_Video] = (u32)((float)oResource.auMemory[MT_Video] * ((Levels > 1) ? 1.4f : 1.0f));

	const u32 RequiredAlignment =  (Width < 256 || Height < 256) ? D3DRUNTIME_MEMORYALIGNMENT : D3D_AMD_LARGEALIGNMENT;
	oResource.auMemory[MT_Video] = (oResource.auMemory[MT_Video] + (RequiredAlignment - 1)) & ~(RequiredAlignment - 1);
	if ((GRCDEVICE.SupportsFeature(AUTOSTEREO) && (GRCDEVICE.GetGPUCount() <= 1)) &&
		((Usage == D3DUSAGE_DEPTHSTENCIL) ||
		 (Usage == D3DUSAGE_RENDERTARGET)) &&
		(Width != Height) &&
		(Width >= (UINT)GRCDEVICE.GetWidth()) && 
		(Height >= (UINT)GRCDEVICE.GetHeight()))
	{
		oResource.auMemory[MT_Video] *= 2;
	}
	
	if (Pool != D3DPOOL_DEFAULT)
	{
		oResource.auMemory[MT_System] = oResource.auMemory[MT_Video];
	}

	if ((Pool == D3DPOOL_SYSTEMMEM) || (Pool == D3DPOOL_SCRATCH))
	{
		oResource.auMemory[MT_Video] = 0;
	}

	HRESULT hr = Get(oResource);
	AssertMsg(hr == D3D_OK, "ResourceCache: Could not allocate DX9 texture.");
	*ppTexture = (IDirect3DTexture9*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateVolumeTexture( UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE*)
{
	ResourceData oResource;
	oResource.eType = RT_VolumeTexture;
	oResource.uSize = sizeof(TEXTUREDATA);
	oResource.oTexture.uWidth = Width;
	oResource.oTexture.uHeight = Height;
	oResource.oTexture.uDepth = Depth;
	oResource.oTexture.uLevels = Levels;
	oResource.oTexture.uUsage = Usage;
	oResource.oTexture.uFormat = (u32)Format;
	oResource.oTexture.uPool = (u32)Pool;

	oResource.auMemory[MT_Video] = Width * Height * Depth * grcTextureFactoryDX9::GetBitsPerPixelStaticDX9((u32)Format) / 8; 
	oResource.auMemory[MT_Video] = (u32)((float)oResource.auMemory[MT_Video] * ((Levels > 1) ? 1.25f : 1.0f));

	const u32 RequiredAlignment =  (Width < 256 || Height < 256) ? D3DRUNTIME_MEMORYALIGNMENT : D3D_AMD_LARGEALIGNMENT;
	oResource.auMemory[MT_Video] = (oResource.auMemory[MT_Video] + (RequiredAlignment - 1)) & ~(RequiredAlignment - 1);

	if (Pool != D3DPOOL_DEFAULT)
	{
		oResource.auMemory[MT_System] = oResource.auMemory[MT_Video];
	}

	if ((Pool == D3DPOOL_SYSTEMMEM) || (Pool == D3DPOOL_SCRATCH))
	{
		oResource.auMemory[MT_Video] = 0;
	}

	HRESULT hr = Get(oResource);
	AssertMsg(hr == D3D_OK, "ResourceCache: Could not allocate DX9 volume texture.");
	*ppVolumeTexture = (IDirect3DVolumeTexture9*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateCubeTexture( UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE*)
{
	ResourceData oResource;
	oResource.eType = RT_CubeTexture;
	oResource.uSize = sizeof(TEXTUREDATA);
	oResource.oTexture.uEdgeLength = EdgeLength;
	oResource.oTexture.uLevels = Levels;
	oResource.oTexture.uUsage = Usage;
	oResource.oTexture.uFormat = (u32)Format;
	oResource.oTexture.uPool = (u32)Pool;

	oResource.auMemory[MT_Video] = 6 * EdgeLength * EdgeLength * grcTextureFactoryDX9::GetBitsPerPixelStaticDX9((u32)Format) / 8; 
	oResource.auMemory[MT_Video] = (u32)((float)oResource.auMemory[MT_Video] * ((Levels > 1) ? 1.4f : 1.0f));

	const u32 RequiredAlignment =  D3D_AMD_LARGEALIGNMENT;
	oResource.auMemory[MT_Video] = (oResource.auMemory[MT_Video] + (RequiredAlignment - 1)) & ~(RequiredAlignment - 1);

	if (Pool != D3DPOOL_DEFAULT)
	{
		oResource.auMemory[MT_System] = oResource.auMemory[MT_Video];
	}

	if ((Pool == D3DPOOL_SYSTEMMEM) || (Pool == D3DPOOL_SCRATCH))
	{
		oResource.auMemory[MT_Video] = 0;
	}

	HRESULT hr = Get(oResource);
	AssertMsg(hr == D3D_OK, "ResourceCache: Could not allocate DX9 cube texture.");
	*ppCubeTexture = (IDirect3DCubeTexture9*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateRenderTarget( UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* )
{
	ResourceData oResource;
	oResource.eType = RT_RenderTarget;
	oResource.uSize = sizeof(TEXTUREDATA);
	oResource.oTexture.uWidth = Width;
	oResource.oTexture.uHeight = Height;
	oResource.oTexture.uFormat = (u32)Format;
	oResource.oTexture.uMultiSample = MultiSample;
	oResource.oTexture.uMultiSampleQuality = MultisampleQuality;
	oResource.oTexture.uLockable = (u32)Lockable;

	// PC TODO - Add Multisample Support
	oResource.auMemory[MT_Video] = (MultiSample ? MultiSample : 1) * Width * Height * grcTextureFactoryDX9::GetBitsPerPixelStaticDX9((u32)Format) / 8; 

	const u32 RequiredAlignment =  (Width < 256 || Height < 256) ? D3DRUNTIME_MEMORYALIGNMENT : D3D_AMD_LARGEALIGNMENT;
	oResource.auMemory[MT_Video] = (oResource.auMemory[MT_Video] + (RequiredAlignment - 1)) & ~(RequiredAlignment - 1);

	if ((GRCDEVICE.SupportsFeature(AUTOSTEREO) && (GRCDEVICE.GetGPUCount() <= 1)) &&
		(Width != Height) &&
		(Width >= (UINT)GRCDEVICE.GetWidth()) && 
		(Height >= (UINT)GRCDEVICE.GetHeight()))
	{
		oResource.auMemory[MT_Video] *= 2;
	}

	HRESULT hr = Get(oResource);
	AssertMsg(hr == D3D_OK, "ResourceCache: Could not allocate DX9 render target.");
	*ppSurface = (IDirect3DSurface9*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateDepthStencilSurface( UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultiSampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* )
{
	ResourceData oResource;
	oResource.eType = RT_DepthStencil;
	oResource.uSize = sizeof(TEXTUREDATA);
	oResource.oTexture.uWidth = Width;
	oResource.oTexture.uHeight = Height;
	oResource.oTexture.uFormat = (u32)Format;
	oResource.oTexture.uMultiSample = MultiSample;
	oResource.oTexture.uMultiSampleQuality = MultiSampleQuality;
	oResource.oTexture.uDiscard = (u32)Discard;

	// PC TODO - Add Multisample Support
	oResource.auMemory[MT_Video] = (MultiSample ? MultiSample : 1) * Width * Height * grcTextureFactoryDX9::GetBitsPerPixelStaticDX9((u32)Format) / 8; 

	const u32 RequiredAlignment =  (Width < 256 || Height < 256) ? D3DRUNTIME_MEMORYALIGNMENT : D3D_AMD_LARGEALIGNMENT;
	oResource.auMemory[MT_Video] = (oResource.auMemory[MT_Video] + (RequiredAlignment - 1)) & ~(RequiredAlignment - 1);

	if ((GRCDEVICE.SupportsFeature(AUTOSTEREO) && (GRCDEVICE.GetGPUCount() <= 1)) &&
		(Width != Height) &&
		(Width >= (UINT)GRCDEVICE.GetWidth()) && 
		(Height >= (UINT)GRCDEVICE.GetHeight()))
	{
		oResource.auMemory[MT_Video] *= 2;
	}

	HRESULT hr = Get(oResource);
	AssertMsg(hr == D3D_OK, "ResourceCache: Could not allocate DX9 depth stencil surface.");
	*ppSurface = (IDirect3DSurface9*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateVertexBuffer( UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE*)
{
	ResourceData oResource;
	oResource.eType = RT_VertexBuffer;
	oResource.uSize = sizeof(GEOMETRYDATA);
	oResource.oGeometry.uUsage = Usage;
	oResource.oGeometry.uFVF = FVF;
	oResource.oGeometry.uPool = (u32)Pool;
	oResource.auMemory[MT_Video] = (Length + (D3DRUNTIME_MEMORYALIGNMENT - 1)) & ~(D3DRUNTIME_MEMORYALIGNMENT - 1);
	oResource.oGeometry.uSize = oResource.auMemory[MT_Video];

	if (Pool != D3DPOOL_DEFAULT)
	{
		oResource.auMemory[MT_System] = oResource.auMemory[MT_Video];
	}

	if ((Pool == D3DPOOL_SYSTEMMEM) || (Pool == D3DPOOL_SCRATCH))
	{
		oResource.auMemory[MT_Video] = 0;
	}

	HRESULT hr = Get(oResource);
	AssertMsg(hr == D3D_OK, "ResourceCache: Could not allocate DX9 vertex buffer.");
	*ppVertexBuffer = (IDirect3DVertexBuffer9*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateIndexBuffer( UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* )
{
	ResourceData oResource;
	oResource.eType = RT_IndexBuffer;
	oResource.uSize = sizeof(GEOMETRYDATA);
	oResource.oGeometry.uUsage = Usage;
	oResource.oGeometry.uFormat = (DWORD)Format;
	oResource.oGeometry.uPool = (u32)Pool;
	oResource.auMemory[MT_Video] = (Length + (D3DRUNTIME_MEMORYALIGNMENT - 1)) & ~(D3DRUNTIME_MEMORYALIGNMENT - 1);
	oResource.oGeometry.uSize = oResource.auMemory[MT_Video];

	if (Pool != D3DPOOL_DEFAULT)
	{
		oResource.auMemory[MT_System] = oResource.auMemory[MT_Video];
	}

	if ((Pool == D3DPOOL_SYSTEMMEM) || (Pool == D3DPOOL_SCRATCH))
	{
		oResource.auMemory[MT_Video] = 0;
	}

	HRESULT hr = Get(oResource);
	AssertMsg(hr == D3D_OK, "ResourceCache: Could not allocate DX9 index buffer.");
	*ppIndexBuffer = (IDirect3DIndexBuffer9*)oResource.pvRetResource;
	return hr;
}

#else // =========== D3D11 ==============

HRESULT grcResourceCache::CreateTexture1D(const D3D11_TEXTURE1D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture1D **ppTexture)
{
	Assert(pDesc != NULL);
	ResourceData oResource;
	oResource.eType = RT_Texture;
	sysMemCpy(&oResource.oResource.oTex1D_10, pDesc, sizeof(D3D11_TEXTURE1D_DESC));
	CompileTimeAssert(sizeof(TEXTUREDATA) != sizeof(D3D11_TEXTURE1D_DESC));
	CompileTimeAssert(sizeof(D3D11_TEXTURE1D_DESC) != sizeof(D3D11_TEXTURE2D_DESC));
	oResource.uSize = sizeof(D3D11_TEXTURE1D_DESC);
	oResource.pvResource = (void*)pInitialData;

	oResource.auMemory[MT_Video] = pDesc->ArraySize * pDesc->Width * grcTextureFactoryDX11::GetD3D10BitsPerPixel((u32)pDesc->Format) / 8; 
	oResource.auMemory[MT_Video] = (u32)((float)oResource.auMemory[MT_Video] * ((pDesc->MipLevels > 1) ? 2.0f : 1.0f));
	oResource.auMemory[MT_Video] = (oResource.auMemory[MT_Video] + (D3DRUNTIME_MEMORYALIGNMENT - 1)) & ~(D3DRUNTIME_MEMORYALIGNMENT - 1);

	if (pDesc->Usage == D3D11_USAGE_DYNAMIC)
	{
		oResource.auMemory[MT_System] = oResource.auMemory[MT_Video];
	}

	if (pDesc->Usage == D3D11_USAGE_STAGING)
	{
		oResource.auMemory[MT_Video] = 0;
	}
	HRESULT hr = Get(oResource);
	AssertMsg(SUCCEEDED(hr), "ResourceCache: Could not allocate DX11 1D texture.");
	*ppTexture = (ID3D11Texture1D*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateTexture2D(const D3D11_TEXTURE2D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture2D **ppTexture)
{
	Assert(pDesc != NULL);
	ResourceData oResource;
	oResource.eType = RT_Texture;
	sysMemCpy(&oResource.oResource.oTex2D_10, pDesc, sizeof(D3D11_TEXTURE2D_DESC));
	CompileTimeAssert(sizeof(TEXTUREDATA) != sizeof(D3D11_TEXTURE2D_DESC));
	oResource.uSize = sizeof(D3D11_TEXTURE2D_DESC);
	oResource.pvResource = (void*)pInitialData;

	oResource.auMemory[MT_Video] = pDesc->ArraySize * pDesc->Width * pDesc->Height * grcTextureFactoryDX11::GetD3D10BitsPerPixel((u32)pDesc->Format) / 8; 
	oResource.auMemory[MT_Video] = (u32)((float)oResource.auMemory[MT_Video] * ((pDesc->MipLevels > 1) ? 1.4f : 1.0f));
	oResource.auMemory[MT_Video] *= pDesc->SampleDesc.Count; // Multi-sampling

	const u32 RequiredAlignment =  (pDesc->Width < 256 || pDesc->Height < 256) ? D3DRUNTIME_MEMORYALIGNMENT : D3D_AMD_LARGEALIGNMENT;
	oResource.auMemory[MT_Video] = (oResource.auMemory[MT_Video] + (RequiredAlignment - 1)) & ~(RequiredAlignment - 1);

	if (pDesc->Usage == D3D11_USAGE_DYNAMIC)
	{
		oResource.auMemory[MT_System] = oResource.auMemory[MT_Video];
	}

	if (pDesc->Usage == D3D11_USAGE_STAGING)
	{
		oResource.auMemory[MT_Video] = 0;
	}
	HRESULT hr = Get(oResource);
	Assertf(SUCCEEDED(hr), "ResourceCache: Could not allocate DX11 2D texture Err %x - Width %d Height %d Format %x Mips %d Sample Count %d Usage %x Bind %x CPU %x Misc %x",
		hr, pDesc->Width, pDesc->Height, pDesc->Format, pDesc->MipLevels, pDesc->SampleDesc.Count, pDesc->Usage, pDesc->BindFlags, pDesc->CPUAccessFlags, pDesc->MiscFlags);
	*ppTexture = (ID3D11Texture2D*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateTexture3D(const D3D11_TEXTURE3D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture3D **ppTexture)
{
	Assert(pDesc != NULL);
	ResourceData oResource;
	oResource.eType = RT_Texture;
	sysMemCpy(&oResource.oResource.oTex3D_10, pDesc, sizeof(D3D11_TEXTURE3D_DESC));
	CompileTimeAssert(sizeof(TEXTUREDATA) != sizeof(D3D11_TEXTURE3D_DESC));
	CompileTimeAssert(sizeof(D3D11_TEXTURE1D_DESC) != sizeof(D3D11_TEXTURE3D_DESC));
	CompileTimeAssert(sizeof(D3D11_TEXTURE2D_DESC) != sizeof(D3D11_TEXTURE3D_DESC));
	oResource.uSize = sizeof(D3D11_TEXTURE3D_DESC);
	oResource.pvResource = (void*)pInitialData;

	oResource.auMemory[MT_Video] = pDesc->Width * pDesc->Height * pDesc->Depth * grcTextureFactoryDX11::GetD3D10BitsPerPixel((u32)pDesc->Format) / 8; 
	oResource.auMemory[MT_Video] = (u32)((float)oResource.auMemory[MT_Video] * ((pDesc->MipLevels > 1) ? 1.2f : 1.0f));

	const u32 RequiredAlignment =  (pDesc->Width < 256 || pDesc->Height < 256) ? D3DRUNTIME_MEMORYALIGNMENT : D3D_AMD_LARGEALIGNMENT;
	oResource.auMemory[MT_Video] = (oResource.auMemory[MT_Video] + (RequiredAlignment - 1)) & ~(RequiredAlignment - 1);

	if (pDesc->Usage == D3D11_USAGE_DYNAMIC)
	{
		oResource.auMemory[MT_System] = oResource.auMemory[MT_Video];
	}

	if (pDesc->Usage == D3D11_USAGE_STAGING)
	{
		oResource.auMemory[MT_Video] = 0;
	}
	HRESULT hr = Get(oResource);
	AssertMsg(SUCCEEDED(hr), "ResourceCache: Could not allocate DX11 3D texture.");
	*ppTexture = (ID3D11Texture3D*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateBuffer(const D3D11_BUFFER_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Buffer **ppBuffer)
{
	Assert(pDesc != NULL);
	ResourceData oResource;
	if (pDesc->BindFlags & D3D11_BIND_VERTEX_BUFFER)
		oResource.eType = RT_VertexBuffer;
	else if (pDesc->BindFlags & D3D11_BIND_INDEX_BUFFER)
		oResource.eType = RT_IndexBuffer;
	else if (pDesc->BindFlags & D3D11_BIND_CONSTANT_BUFFER)
		oResource.eType = RT_ConstantBuffer;
	else if (pDesc->BindFlags & D3D11_BIND_STREAM_OUTPUT)
		oResource.eType = RT_StreamOutput;
	else if (pDesc->BindFlags & D3D11_BIND_UNORDERED_ACCESS)
		oResource.eType = RT_UnorderedAccess;
	else if (pDesc->BindFlags == 0)
	{
		if (pDesc->MiscFlags & D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS)
		{
			oResource.eType = RT_IndirectBuffer;
		}
		else
		{
			oResource.eType = RT_Staging;	
		}
	}
	else
	{
		Assertf(0, "Unsupport Bind Flag %x", oResource.eType);
	}
	CompileTimeAssert(sizeof(GEOMETRYDATA) != sizeof(D3D11_BUFFER_DESC));
	sysMemCpy(&oResource.oResource.oBuffer_11, pDesc, sizeof(D3D11_BUFFER_DESC));
	oResource.uSize = sizeof(D3D11_BUFFER_DESC);
	oResource.pvResource = (void*)pInitialData;

	oResource.auMemory[MT_Video] = pDesc->ByteWidth;
	if (oResource.eType != RT_ConstantBuffer) // Do not realign constant buffers
	{
		oResource.auMemory[MT_Video] = (oResource.auMemory[MT_Video] + (D3DRUNTIME_MEMORYALIGNMENT - 1)) & ~(D3DRUNTIME_MEMORYALIGNMENT - 1);
	}

	if (pDesc->Usage == D3D11_USAGE_DYNAMIC)
	{
		oResource.auMemory[MT_System] = oResource.auMemory[MT_Video];
	}

	if (pDesc->Usage == D3D11_USAGE_STAGING)
	{
		oResource.auMemory[MT_Video] = 0;
	}
	HRESULT hr = Get(oResource);
	Assertf(SUCCEEDED(hr), "ResourceCache: Could not allocate DX11 buffer. Size %u Usage %x Bind %x Access %x Misc %x Stride %x", 
		oResource.oResource.oBuffer_11.ByteWidth,
		oResource.oResource.oBuffer_11.Usage,
		oResource.oResource.oBuffer_11.BindFlags,
		oResource.oResource.oBuffer_11.CPUAccessFlags,
		oResource.oResource.oBuffer_11.MiscFlags,
		oResource.oResource.oBuffer_11.StructureByteStride
		);
	*ppBuffer = (ID3D11Buffer*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateDepthStencilView(ID3D11Resource *pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, ID3D11DepthStencilView **ppDepthStencilView)
{
	Assert(pDesc != NULL);
	ResourceData oResource;
	oResource.eType = RT_DSView;

	sysMemCpy(&oResource.oResource.oDepthStencilView, pDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	oResource.uSize = sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC);
	oResource.pvResource = (void*)pResource;

	oResource.auMemory[MT_Video] = oResource.auMemory[MT_System] = 0;
	HRESULT hr = Get(oResource);
	Assertf(SUCCEEDED(hr), "ResourceCache: Could not Create DepthStencilView. Format %x View Dim %x", 
		oResource.oResource.oDepthStencilView.Format,
		oResource.oResource.oDepthStencilView.ViewDimension
		);
	*ppDepthStencilView = (ID3D11DepthStencilView*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateRenderTargetView(ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC *pDesc, ID3D11RenderTargetView **ppRTView)
{
	ResourceData oResource;
	oResource.eType = RT_RTView;

	if (pDesc != NULL)
	{
		sysMemCpy(&oResource.oResource.oRenderTargetView, pDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	} 
	else
	{
		return D3DERR_INVALIDCALL;
	}
	oResource.uSize = sizeof(D3D11_RENDER_TARGET_VIEW_DESC);
	oResource.pvResource = (void*)pResource;

	oResource.auMemory[MT_Video] = oResource.auMemory[MT_System] = 0;
	HRESULT hr = Get(oResource);
	Assertf(SUCCEEDED(hr), "ResourceCache: Could not Create RenderTargetView. Format %x View Dim %x", 
		oResource.oResource.oRenderTargetView.Format,
		oResource.oResource.oRenderTargetView.ViewDimension
		);
	*ppRTView = (ID3D11RenderTargetView*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateShaderResourceView(ID3D11Resource *pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, ID3D11ShaderResourceView **ppSRView)
{
	ResourceData oResource;
	oResource.eType = RT_SRView;

	if (pDesc != NULL)
	{
		sysMemCpy(&oResource.oResource.oShaderResourceView, pDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	}
	else
	{
		return D3DERR_INVALIDCALL;
	}
	oResource.uSize = sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC);
	oResource.pvResource = (void*)pResource;

	oResource.auMemory[MT_Video] = oResource.auMemory[MT_System] = 0;
	HRESULT hr = Get(oResource);
	Assertf(SUCCEEDED(hr), "ResourceCache: Could not Create ShaderResourceView. Format %x View Dim %x", 
		oResource.oResource.oShaderResourceView.Format,
		oResource.oResource.oShaderResourceView.ViewDimension
		);
	*ppSRView = (ID3D11ShaderResourceView*)oResource.pvRetResource;
	return hr;
}

HRESULT grcResourceCache::CreateUnorderedAccessView(ID3D11Resource *pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, ID3D11UnorderedAccessView **ppUAView)
{
	Assert(pDesc != NULL);
	ResourceData oResource;
	oResource.eType = RT_UNVIew;

	sysMemCpy(&oResource.oResource.oUnorderAccessView, pDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	oResource.uSize = sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC);
	oResource.pvResource = (void*)pResource;

	oResource.auMemory[MT_Video] = oResource.auMemory[MT_System] = 0;
	HRESULT hr = Get(oResource);
	Assertf(SUCCEEDED(hr), "ResourceCache: Could not Create ShaderResourceView. Format %x View Dim %x", 
		oResource.oResource.oUnorderAccessView.Format,
		oResource.oResource.oUnorderAccessView.ViewDimension
		);
	*ppUAView = (ID3D11UnorderedAccessView*)oResource.pvRetResource;
	return hr;
}

#endif // (__D3D11)

HRESULT grcResourceCache::Get(ResourceData &oResource)
{
#if CONTROL_RATE_OF_CREATIONS
	if ((sm_fnGameRunning != NULL) && (sm_fnGameRunning()) && IsOkToCreateResources())
	{
		static sysTimer UpdateTimer;
		static u32 uFrame = GRCDEVICE.GetFrameCounter();

		if (uFrame != GRCDEVICE.GetFrameCounter())
		{
			float fFrames = (float)(uFrame - GRCDEVICE.GetFrameCounter());
			uFrame = GRCDEVICE.GetFrameCounter();
			float fPreviousFrames = powf(m_fFalloffFromPreviousFrame, fFrames);
			m_fCurrentDataCreateRate = (m_fCurrentDataCreateRate * fPreviousFrames);
			m_fCurrentCreateRate = (m_fCurrentCreateRate * fPreviousFrames);
		}

		m_fCurrentDataCreateRate = (float)oResource.auMemory[MT_Video];
		m_fCurrentCreateRate += 1.0f;

		if (((m_fCurrentDataCreateRate > m_fDataCreationRate) || (m_fCurrentCreateRate > m_fCreateRate)) &&
			(uFrame == GRCDEVICE.GetFrameCounter()))
		{
			PF_PUSH_TIMEBAR("Wait - Data rate too high");

			while (((m_fCurrentDataCreateRate > m_fDataCreationRate) || (m_fCurrentCreateRate > m_fCreateRate)) &&
				   (uFrame == GRCDEVICE.GetFrameCounter()) &&
				   (UpdateTimer.GetTime() < m_fMaxWaitTime))
			{
				sysIpcSleep(0);
			}
			PF_POP_TIMEBAR(); // Wait - Data rate too high
		}
		UpdateTimer.Reset();
	}
#endif // CONTROL_RATE_OF_CREATIONS
	// PF_AUTO_PUSH_TIMEBAR("Creating Resource"); // Enable to see creation time in telemetry
	VIDEOMEMORY_LOCK_RESOURCES;
	Assertf((__D3D11) || GRCDEVICE.CheckThreadOwnership(),"Attempted to create resource outside of render thread type %d", oResource.eType );
#if __BANK
	if(m_bResetStats)
	{
		ResetStats();
		m_bResetStats = false;
	}
#endif // __BANK

	if ((GetTotalMemory(MT_Video) == 0) && (grcTextureFactoryPC::HasInstance()))
	{
		UpdateTotalAvailableMemory();
	}

	if (!m_bLazyDelete)
	{
		ClearCacheQueue();
		ManageSystemMemoryCache();
	}
	else
	{
		if(((s64)GetTotalFreeMemory(MT_Video, false) <= 0) && (GetTotalMemory(MT_Video) > 0))
		{
			ClearCacheQueue();
			ManageSystemMemoryCache();
		}
	}

#if REUSE_RESOURCE
	// Multimap method of search cache
	MultiMapCache::iterator itCache = m_ammapCachedResources[oResource.eType].lower_bound(Max(oResource.auMemory[MT_Video], oResource.auMemory[MT_System]));

	u32 uCRC = oResource.uCRC = GetCRCForResource(oResource);
	u32 uMatchSize = auMatchSize[oResource.eType];

	while((itCache != m_ammapCachedResources[oResource.eType].end()) &&
		  (((oResource.auMemory[MT_Video] + uMatchSize) >= itCache->first) ||
		  ((oResource.auMemory[MT_System] + uMatchSize) >= itCache->first)))
	{
		MapSubClass *pmapSubClass = itCache->second;
#if REUSE_ALLOW_RESIZING // Do not allow resizing for now
		if ((oResource.eType == RT_VertexBuffer) ||
			(oResource.eType == RT_IndexBuffer) ||
			(oResource.eType == RT_StreamOutput) ||
			(oResource.eType == RT_Staging) ||
			(oResource.eType == RT_IndirectBuffer))
		{
			oResource.uRequestSize = Resource.oResource.oBuffer_11.ByteWidth;
			// Tweak vertex / index buffer so that it still hit with mismatching sizes
#if (__D3D11)
			if (sizeof(D3D11_BUFFER_DESC) == oResource.uSize)
				oResource.oResource.oBuffer_11.ByteWidth = itCache->first; // 10 and 11 share same location
			else
#elif (__D3D11)
			if (sizeof(D3D10_BUFFER_DESC) == oResource.uSize)
				oResource.oResource.oBuffer_10.ByteWidth = itCache->first; // 10 and 11 share same location
			else
#endif
				oResource.oGeometry.uSize = itCache->first;

			uCRC = oResource.uCRC = GetCRCForResource(oResource);
		}
#endif // REUSE_ALLOW_RESIZING

		MapSubClass::iterator itCRC = pmapSubClass->find(uCRC);
		if (itCRC != pmapSubClass->end())
		{
			sysMemStartTemp();
			// Found - Should always be the best match
			MapResource *pmapResources = itCRC->second;
			MapResource::iterator itResource = pmapResources->begin();
			Assert(itResource != pmapResources->end());
			ResourceData* poData = itResource->second;

			// Remove from cache list
			pmapResources->erase(itResource);
			if (pmapResources->empty())
			{
				// Remove map and delete of CRC type
				pmapSubClass->erase(itCRC);
				delete pmapResources;
			}
			if (pmapSubClass->empty())
			{
				// Remove map and delete of Size type
				m_ammapCachedResources[oResource.eType].erase(itCache);
				delete pmapSubClass;
			}
	
			m_lstFreeCache.PopNode(poData->Node);
			m_mapActiveResources[poData->pvRetResource] = poData;
			oResource = *poData;
			sysMemEndTemp();

			m_auTotalActiveItems[MT_Video][RT_Totals]++;
			m_auTotalActiveItems[MT_Video][poData->eType]++;

			m_auTypeTotalActiveMemory[MT_Video][poData->eType] += poData->auMemory[MT_Video];
			m_auTypeTotalActiveMemory[MT_System][poData->eType] += poData->auMemory[MT_System];

			m_auTypeTotalActiveMemory[MT_Video][RT_Totals] += poData->auMemory[MT_Video];
			m_auTypeTotalActiveMemory[MT_System][RT_Totals] += poData->auMemory[MT_System];

			m_auTotalCacheItems[MT_Video][RT_Totals]--;
			m_auTotalCacheItems[MT_Video][poData->eType]--;

			m_auTypeTotalCachedMemory[MT_Video][poData->eType] -= poData->auMemory[MT_Video];
			m_auTypeTotalCachedMemory[MT_System][poData->eType] -= poData->auMemory[MT_System];

			m_auTypeTotalCachedMemory[MT_Video][RT_Totals] -= poData->auMemory[MT_Video];
			m_auTypeTotalCachedMemory[MT_System][RT_Totals] -= poData->auMemory[MT_System];

			m_auNewUsedCacheItems[MT_Video][poData->eType]++;
			m_auNewUsedCacheItems[MT_Video][RT_Totals]++;

			DEBUG_RESOURCE(char szName[256];)
			DEBUG_RESOURCE(GetName(static_cast<ID3D11Resource*>(poData->pvRetResource), szName, 254);)
			DEBUG_RESOURCE(grcDisplayf("Reusing %p - %s", poData->pvRetResource, szName);)
			VIDEOMEMORY_UNLOCK_RESOURCES;
			return S_OK;
		}
		itCache++;
	}

	/*
	if (oResource.eType == RT_VertexBuffer)
	{
		if (itCache == m_ammapCachedResources[oResource.eType].end())
		{
			grcWarningf("No match in size %d - No lower bound found", oResource.uRequiredMemory, oResource.eType);
		}
		else
		{
			grcWarningf("Not found Size %d - %d", oResource.uRequiredMemory, itCache->first);
		}
	}
	*/
#if REUSE_ALLOW_RESIZING
	// Restore original requested size of buffer
	if ((oResource.eType == RT_VertexBuffer) ||
		(oResource.eType == RT_IndexBuffer) ||
		(oResource.eType == RT_StreamOutput) ||
		(oResource.eType == RT_Staging) ||
		(oResource.eType == RT_IndirectBuffer))
	{
		// Tweak vertex / index buffer so that it still hit with mismatching sizes
		if (sizeof(GEOMETRYDATA) == oResource.uSize)
		{
			oResource.uRequestSize = oResource.oGeometry.uSize;
			oResource.oGeometry.uSize = Max(oResource.auMemory[MT_Video], oResource.auMemory[MT_System]);
		}
#if (__D3D11)
		else
		{
			// No cache so no need to align it - Should not get corrupted as it will never be changed by caching system
			//((D3D10_BUFFER_DESC*)&oResource.oResource)->ByteWidth = Max(oResource.auMemory[MT_Video], oResource.auMemory[MT_System]);			
		}
#endif // __D3D11
	}
#endif // REUSE_ALLOW_RESIZING
#endif // REUSE_RESOURCE

	// Nothing found
	// Allocate new resource
	while (true)
	{
		for (u32 uIndex = 0; uIndex < MT_System; uIndex++)
		{
			MemoryType eType = static_cast<MemoryType>(uIndex);
			while(((s64)GetTotalFreeMemory(eType, false) < 0) && (GetTotalMemory(MT_Video) > 0))
			{
				ClearCacheQueue();
				ManageSystemMemoryCache(true);

				LOCK_DELETION_QUEUE
				if (!CleanCache(eType))
				{
					grcDebugf1("Warning - %s memory has exceeded limit %" I64FMT "d", astrMemoryNames[eType], - ((s64)GetTotalFreeMemory(eType, false)));
#if REUSE_RESOURCE
					grcDebugf1("Active %d Cached %d Total Mem %" I64FMT "d Active %" I64FMT "d Cached %" I64FMT "d", 
						 GetActiveCount(MT_Video, RT_Totals), 
						 GetCachedCount(MT_Video, RT_Totals),
						 GetTotalUsedMemory(eType), 
						 GetActiveMemory(eType, RT_Totals), 
						 GetCachedMemory(eType, RT_Totals)
						);
#endif // REUSE_RESOURCE
					UNLOCK_DELETION_QUEUE
					break;
				}
				UNLOCK_DELETION_QUEUE
			}
		}

		HRESULT hRes = Add(oResource);
		if (hRes != S_OK)
		{
			hRes = Add(oResource);
		}
		VIDEOMEMORY_UNLOCK_RESOURCES;
		return hRes;
	}
	//Unlock();
}

bool grcResourceCache::IsOkToCreateResources()
{
	bool bOkay = (sm_fnGameRunning == NULL) || !sm_fnGameRunning() ASSERT_ONLY( || sm_SafeCreate);
	bool bNotGameMainThread = !GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread();
	bNotGameMainThread &= g_MainThreadId != sysIpcGetCurrentThreadId();
	bOkay |= bNotGameMainThread;
	return bOkay;
}

HRESULT grcResourceCache::Add(ResourceData &oResource)
{
	//PF_AUTO_PUSH_DETAIL("grcResourceCache::Add");
	HRESULT hResult = D3DERR_INVALIDCALL;
	Assertf(IsOkToCreateResources(), "Creation of Resource should not occur on main/render thread as it will cause a stall, Render Thread %d Main Thread %d", GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), g_MainThreadId == sysIpcGetCurrentThreadId());
#if !__FINAL
	if (m_bForceFailures && (oResource.eType != RT_RenderTarget) && !((oResource.eType >= RT_ConstantBuffer) && (oResource.eType <= RT_IndexBuffer)))
		return hResult;
#endif

	sysIpcLockMutex(m_DeviceCreate);
#if !__D3D11
	if (!__D3D11)
	{
		// DX9
		switch (oResource.eType)
		{
		case RT_Texture:
			hResult = static_cast<RageDirect3DDevice9*>(GRCDEVICE.GetCurrent())->m_Inner->CreateTexture(
				oResource.oTexture.uWidth,
				oResource.oTexture.uHeight,
				oResource.oTexture.uLevels,
				oResource.oTexture.uUsage,
				(D3DFORMAT)oResource.oTexture.uFormat,
				(D3DPOOL)oResource.oTexture.uPool,
				(IDirect3DTexture9**)&oResource.pvRetResource,
				NULL);
#if !__NO_OUTPUT
			if (hResult != D3D_OK)
			{
				Errorf("Err %x Failed to Create Texture Width %d Height %d Format 0x%x Mip Level %d", 
					hResult,
					oResource.oTexture.uWidth,
					oResource.oTexture.uHeight,
					oResource.oTexture.uFormat,
					oResource.oTexture.uLevels);
			}
#endif // !__NO_OUTPUT
			break;

		case RT_VolumeTexture:
			hResult = static_cast<RageDirect3DDevice9*>(GRCDEVICE.GetCurrent())->m_Inner->CreateVolumeTexture(
				oResource.oTexture.uWidth,
				oResource.oTexture.uHeight,
				oResource.oTexture.uDepth,
				oResource.oTexture.uLevels,
				oResource.oTexture.uUsage,
				(D3DFORMAT)oResource.oTexture.uFormat,
				(D3DPOOL)oResource.oTexture.uPool,
				(IDirect3DVolumeTexture9**)&oResource.pvRetResource,
				NULL);
			break;

		case RT_CubeTexture:
			hResult = static_cast<RageDirect3DDevice9*>(GRCDEVICE.GetCurrent())->m_Inner->CreateCubeTexture(
				oResource.oTexture.uEdgeLength,
				oResource.oTexture.uLevels,
				oResource.oTexture.uUsage,
				(D3DFORMAT)oResource.oTexture.uFormat,
				(D3DPOOL)oResource.oTexture.uPool,
				(IDirect3DCubeTexture9**)&oResource.pvRetResource,
				NULL);
			break;

		case RT_VertexBuffer:
			hResult = static_cast<RageDirect3DDevice9*>(GRCDEVICE.GetCurrent())->m_Inner->CreateVertexBuffer(
				oResource.oGeometry.uSize,
				oResource.oGeometry.uUsage,
				oResource.oGeometry.uFVF,
				(D3DPOOL)oResource.oGeometry.uPool,
				(IDirect3DVertexBuffer9**)&oResource.pvRetResource,
				NULL);
			break;

		case RT_IndexBuffer:
			hResult = static_cast<RageDirect3DDevice9*>(GRCDEVICE.GetCurrent())->m_Inner->CreateIndexBuffer(
				oResource.oGeometry.uSize,
				oResource.oGeometry.uUsage,
				(D3DFORMAT)oResource.oGeometry.uFormat,
				(D3DPOOL)oResource.oGeometry.uPool,
				(IDirect3DIndexBuffer9**)&oResource.pvRetResource,
				NULL);
			break;

		case RT_RenderTarget:
			hResult = static_cast<RageDirect3DDevice9*>(GRCDEVICE.GetCurrent())->m_Inner->CreateRenderTarget(
					oResource.oTexture.uWidth,
					oResource.oTexture.uHeight,
					(D3DFORMAT)oResource.oTexture.uFormat,
					(D3DMULTISAMPLE_TYPE)oResource.oTexture.uMultiSample,
					oResource.oTexture.uMultiSampleQuality,
					(BOOL)oResource.oTexture.uLockable,
					(IDirect3DSurface9**)&oResource.pvRetResource,
					NULL);
#if !__NO_OUTPUT
			if (hResult != D3D_OK)
			{
				Errorf("Err %x Failed to Create Render Target Width %d Height %d Format 0x%x Mip Level %d", 
					hResult,
					oResource.oTexture.uWidth,
					oResource.oTexture.uHeight,
					oResource.oTexture.uFormat,
					1);
			}
#endif // !__NO_OUTPUT
			break;

		case RT_DepthStencil:
			hResult = static_cast<RageDirect3DDevice9*>(GRCDEVICE.GetCurrent())->m_Inner->CreateDepthStencilSurface(
					oResource.oTexture.uWidth,
					oResource.oTexture.uHeight,
					(D3DFORMAT)oResource.oTexture.uFormat,
					(D3DMULTISAMPLE_TYPE)oResource.oTexture.uMultiSample,
					oResource.oTexture.uMultiSampleQuality,
					(BOOL)oResource.oTexture.uDiscard,
					(IDirect3DSurface9**)&oResource.pvRetResource,
					NULL);
#if !__NO_OUTPUT
			if (hResult != D3D_OK)
			{
				Errorf("Err %x Failed to Create Render Target Width %d Height %d Format 0x%x Mip Level %d", 
					hResult,
					oResource.oTexture.uWidth,
					oResource.oTexture.uHeight,
					oResource.oTexture.uFormat,
					1);
			}
#endif // !__NO_OUTPUT
			break;

		default:
			grcWarningf("Unknown Resource Type Being Added");
			break;
		}
	}
#else
	if (__D3D11)
	{
		// DX11
		switch (oResource.eType)
		{
		case RT_Texture:
			switch(oResource.uSize)
			{
			case (sizeof(D3D11_TEXTURE1D_DESC)):
				hResult = static_cast<RageDirect3DDevice11*>(GRCDEVICE.GetCurrent())->m_Inner->CreateTexture1D((D3D11_TEXTURE1D_DESC*)&oResource.oResource, (const D3D11_SUBRESOURCE_DATA *)oResource.pvResource, (ID3D11Texture1D**)&oResource.pvRetResource);
				break;
			case (sizeof(D3D11_TEXTURE2D_DESC)):
				hResult = static_cast<RageDirect3DDevice11*>(GRCDEVICE.GetCurrent())->m_Inner->CreateTexture2D((D3D11_TEXTURE2D_DESC*)&oResource.oResource, (const D3D11_SUBRESOURCE_DATA *)oResource.pvResource, (ID3D11Texture2D**)&oResource.pvRetResource);
#if !__NO_OUTPUT
				if (hResult != D3D_OK)
				{
					Errorf("Err %x Failed to Create Texture Width %d Height %d Format 0x%x Mip Level %d Bind %x Usage %x", 
						hResult,
						oResource.oResource.oTex2D_10.Width,
						oResource.oResource.oTex2D_10.Height,
						oResource.oResource.oTex2D_10.Format,
						oResource.oResource.oTex2D_10.MipLevels,
						oResource.oResource.oTex2D_10.BindFlags,
						oResource.oResource.oTex2D_10.Usage);
				}
#endif // !__NO_OUTPUT
				break;
			case (sizeof(D3D11_TEXTURE3D_DESC)):
				hResult = static_cast<RageDirect3DDevice11*>(GRCDEVICE.GetCurrent())->m_Inner->CreateTexture3D((D3D11_TEXTURE3D_DESC*)&oResource.oResource, (const D3D11_SUBRESOURCE_DATA *)oResource.pvResource, (ID3D11Texture3D**)&oResource.pvRetResource);
				break;
			default:
				AssertMsg(0, ("Unknown DX11 Texture Type %d", oResource.uSize));
			}
			break;
		case RT_ConstantBuffer:
		case RT_VertexBuffer:
		case RT_IndexBuffer:
		case RT_StreamOutput:
		case RT_Staging:
		case RT_UnorderedAccess:
		case RT_IndirectBuffer:
			hResult = static_cast<RageDirect3DDevice11*>(GRCDEVICE.GetCurrent())->m_Inner->CreateBuffer((D3D11_BUFFER_DESC*)&oResource.oResource, (const D3D11_SUBRESOURCE_DATA *)oResource.pvResource, (ID3D11Buffer**)&oResource.pvRetResource);
			break;
		case RT_DSView:
			hResult = static_cast<RageDirect3DDevice11*>(GRCDEVICE.GetCurrent())->m_Inner->CreateDepthStencilView((ID3D11Resource*)oResource.pvResource, (const D3D11_DEPTH_STENCIL_VIEW_DESC *)&oResource.oResource, (ID3D11DepthStencilView**)&oResource.pvRetResource);
			break;
		case RT_RTView:
			hResult = static_cast<RageDirect3DDevice11*>(GRCDEVICE.GetCurrent())->m_Inner->CreateRenderTargetView((ID3D11Resource*)oResource.pvResource, (const D3D11_RENDER_TARGET_VIEW_DESC *)&oResource.oResource, (ID3D11RenderTargetView**)&oResource.pvRetResource);
			break;
		case RT_SRView:
			hResult = static_cast<RageDirect3DDevice11*>(GRCDEVICE.GetCurrent())->m_Inner->CreateShaderResourceView((ID3D11Resource*)oResource.pvResource, (const D3D11_SHADER_RESOURCE_VIEW_DESC *)&oResource.oResource, (ID3D11ShaderResourceView**)&oResource.pvRetResource);
			break;
		case RT_UNVIew:
			hResult = static_cast<RageDirect3DDevice11*>(GRCDEVICE.GetCurrent())->m_Inner->CreateUnorderedAccessView((ID3D11Resource*)oResource.pvResource, (const D3D11_UNORDERED_ACCESS_VIEW_DESC *)&oResource.oResource, (ID3D11UnorderedAccessView**)&oResource.pvRetResource);
			break;
		default:
			grcWarningf("Unknown Resource Type Being Added");
			break;
		}
#if __BANK
		DumpMemoryStats();
		DumpRenderTargetDependencies();
		SaveResourceData(false);
		SaveRenderTargets(false);
#endif // __BANK
	}
#endif //(!__D3D11)

	else
	{
		sysIpcUnlockMutex(m_DeviceCreate);
		Quitf(ERR_GFX_D3D_VER_1,"Unsupported DirectX Version");
	}
	sysIpcUnlockMutex(m_DeviceCreate);

	if (SUCCEEDED(hResult))
	{
		DEBUG_RESOURCE(static u32 uCount = 0;)
		DEBUG_RESOURCE(grcDisplayf("Created %d Resource %p", uCount++, oResource.pvRetResource);)

		m_auNewAddItems[MT_Video][oResource.eType]++;
		m_auNewAddItems[MT_Video][RT_Totals]++;

		m_auTotalActiveItems[MT_Video][oResource.eType]++;
		m_auTotalActiveItems[MT_Video][RT_Totals]++;

		for (u32 uIndex = 0; uIndex < MT_Last; uIndex++)
		{
			m_auTypeTotalActiveMemory[uIndex][oResource.eType] += oResource.auMemory[uIndex];
			m_auTypeTotalActiveMemory[uIndex][RT_Totals] += oResource.auMemory[uIndex];
		}

		s64 iExceeded = GetTotalFreeMemory(MT_Video, false);
		s64 iAllowedOverflow = (s64)(GetTotalMemory(MT_Video) * (1.0f - GetPercentageForStreaming()));
		if (REUSE_RESOURCE_ONLY((GetCachedMemory(MT_Video, RT_Totals) <= 0) && ) PARAM_allowgrowth.Get())
		{
			if (iExceeded < 0)
			{
				sm_aiExtraAvailableMemory[MT_Video] -= iExceeded;
				grcDebugf1("Increased System Memory by %d to %d bytes", (s32)-iExceeded, (s32)sm_aiExtraAvailableMemory[MT_Video]);
			}
			else if ((GetExtraMemory(MT_Video) > 0) && (iExceeded > iAllowedOverflow))
			{
				iExceeded -= iAllowedOverflow;
				if (iExceeded < GetExtraMemory(MT_Video))
				{
					grcDebugf1("Reducing Extra Memory of %d to %d bytes", (s32)GetExtraMemory(MT_Video), Max(GetExtraMemory(MT_Video) - iExceeded, (s64)0));
					SetExtraMemory(MT_Video, Max(GetExtraMemory(MT_Video) - iExceeded, (s64)0));
				}
				else
				{
					grcDebugf1("Eliminating Extra Memory by %d bytes", (s32)GetExtraMemory(MT_Video));
					SetExtraMemory(MT_Video, 0);
				}
			}
		}

		sysMemStartTemp();
		// Add new resource to active list
		ResourceData* poResource = static_cast<ResourceData*>(m_ResDataPool->New());
		FatalAssert(poResource != NULL);
		*poResource = oResource;
		poResource->uCRC = GetCRCForResource(oResource);
		m_mapActiveResources[oResource.pvRetResource] = poResource;
		sysMemEndTemp();
		return hResult;
	}
	else
	{
		// Clamp max to current limit hit
		if (hResult == D3DERR_OUTOFVIDEOMEMORY)
		{
			for (u32 uIndex = 0; uIndex < MT_Last; uIndex++)
			{
				if (oResource.auMemory[uIndex] > 0)
				{
					m_aiTotalAvailableMemory[uIndex] = Min(GetTotalMemory((MemoryType)uIndex), GetActiveMemory((MemoryType)uIndex, RT_Totals) + (s64)(0.9f * (float)GetCachedMemory((MemoryType)uIndex, RT_Totals)));
				}
			}

			for (u32 uIndex = 0; uIndex < MT_Last; uIndex++)
			{
				while(((s64)GetTotalFreeMemory((MemoryType)uIndex, false) <= 0) && (GetTotalMemory(MT_Video) > 0))
				{
					LOCK_DELETION_QUEUE
					if (!CleanCache((MemoryType)uIndex))
					{
						UNLOCK_DELETION_QUEUE
						break;
					}
					UNLOCK_DELETION_QUEUE
				}
			}
			grcWarningf("OUT OF VIDEO MEMORY");
			grcWarningf("Active %d Cached %d", GetActiveCount(MT_Video, RT_Totals), GetCachedCount(MT_Video, RT_Totals));
			grcWarningf("Total Vid Mem %u MB Active %u MB Cached %u MB", (u32)(GetTotalUsedMemory(MT_Video) / cuMegaBytes), (u32)(GetActiveMemory(MT_Video, RT_Totals) / cuMegaBytes), (u32)(GetCachedMemory(MT_Video, RT_Totals) / cuMegaBytes));
			grcWarningf("Total Sys Mem %u MB Active %u MB Cached %u MB", (u32)(GetTotalUsedMemory(MT_System) / cuMegaBytes), (u32)(GetActiveMemory(MT_System, RT_Totals) / cuMegaBytes), (u32)(GetCachedMemory(MT_System, RT_Totals) / cuMegaBytes));
			grcWarningf("Sys Free %u MB App Free %u MB Largest Block %u MB Kernal Free %u MB", (u32)(sysMemTotalFreeMemory() / cuMegaBytes), (u32)(sysMemTotalFreeAppMemory() / cuMegaBytes), (u32)(sysMemLargestFreeBlock() / cuMegaBytes), (u32)(sysMemTotalFreeKernelMemory() / cuMegaBytes));
		}
		else if (hResult == E_OUTOFMEMORY)
		{
			for (u32 uIndex = MT_System; uIndex < MT_Last; uIndex++)
			{
				if (oResource.auMemory[uIndex] > 0)
				{
					m_aiTotalAvailableMemory[uIndex] = Min(GetTotalMemory((MemoryType)uIndex), GetActiveMemory((MemoryType)uIndex, RT_Totals) + (s64)(0.9f * (float)GetCachedMemory((MemoryType)uIndex, RT_Totals)));
				}
			}

			for (u32 uIndex = MT_System; uIndex < MT_Last; uIndex++)
			{
				while(((s64)GetTotalFreeMemory((MemoryType)uIndex, false) <= 0) && (GetTotalMemory(MT_Video) > 0))
				{
					LOCK_DELETION_QUEUE
					if (!CleanCache((MemoryType)uIndex))
					{
						UNLOCK_DELETION_QUEUE
						break;
					}
					UNLOCK_DELETION_QUEUE
				}
			}
			while(ManageSystemMemoryCache(true)) {}
			grcWarningf("OUT OF SYSTEM MEMORY");
			grcWarningf("Active %d Cached %d", GetActiveCount(MT_Video, RT_Totals), GetCachedCount(MT_Video, RT_Totals));
			grcWarningf("Total Vid Mem %u MB Active %u MB Cached %u MB", (u32)(GetTotalUsedMemory(MT_Video) / cuMegaBytes), (u32)(GetActiveMemory(MT_Video, RT_Totals) / cuMegaBytes), (u32)(GetCachedMemory(MT_Video, RT_Totals) / cuMegaBytes));
			grcWarningf("Total Sys Mem %u MB Active %u MB Cached %u MB", (u32)(GetTotalUsedMemory(MT_System) / cuMegaBytes), (u32)(GetActiveMemory(MT_System, RT_Totals) / cuMegaBytes), (u32)(GetCachedMemory(MT_System, RT_Totals) / cuMegaBytes));
			grcWarningf("Sys Free %u MB App Free %u MB Largest Block %u MB Kernal Free %u MB", (u32)(sysMemTotalFreeMemory() / cuMegaBytes), (u32)(sysMemTotalFreeAppMemory() / cuMegaBytes), (u32)(sysMemLargestFreeBlock() / cuMegaBytes), (u32)(sysMemTotalFreeKernelMemory() / cuMegaBytes));
		}
		else
		{
			Assertf(0, "Failed to Create Resource Error Code %x - Setting to NULL", hResult);
			CheckDxHresultFatal(hResult);
			oResource.pvRetResource = NULL;
			return hResult;
		}
#if __DEV
		sysIpcSleep(1);
#endif
	}

	return hResult;
}

bool grcResourceCache::Cache(void* pvResource, bool 
#if REUSE_RESOURCE
							 bCache
#endif // REUSE_RESOURCE
							 )
{
#if __DEV && HEAVY_VALIDATE
	if (!AssertVerify(Validate(pvResource)))
		return false;
#endif // __DEV

	//PF_AUTO_PUSH_DETAIL("grcResourceCache::Cache");
	// Remove from Active List
	ResourceData* poData = NULL;
	MapActive::iterator itActive = m_mapActiveResources.find(pvResource);
	if (itActive != m_mapActiveResources.end())
	{
		sysMemStartTemp();
		poData = itActive->second;
		m_mapActiveResources.erase(itActive);
		sysMemEndTemp();
	}
	else
	{
		return false;
	}

#if REUSE_RESOURCE
	bool bReuseable = REUSE_RESOURCE ? bCache : false;
#endif

#if (!REUSE_IMMUTABLE_RESOURCES && __D3D11 && REUSE_RESOURCE)
	bool bReuseable = REUSE_RESOURCE ? bCache : false;
	{
		switch (poData->eType)
		{
		case RT_VolumeTexture:
		case RT_CubeTexture:
		case RT_RenderTarget:
		case RT_DepthStencil:
			bReuseable = false; // TODO - Could reuse in theory
			break;

		case RT_Texture:
			{
				switch (poData->uSize)
				{
					case (sizeof(D3D10_TEXTURE1D_DESC)):
						if (poData->oResource.oTex1D_10.Usage == D3D10_USAGE_IMMUTABLE)
						{
							bReuseable = false;
						}
						break;
					case (sizeof(D3D10_TEXTURE2D_DESC)):
						if (poData->oResource.oTex2D_10.Usage == D3D10_USAGE_IMMUTABLE)
						{
							bReuseable = false;
						}
						break;
					case (sizeof(D3D10_TEXTURE3D_DESC)):
						if (poData->oResource.oTex3D_10.Usage == D3D10_USAGE_IMMUTABLE)
						{
							bReuseable = false;
						}
						break;
					default:
						FatalAssertf(0, "Unknown Texture Type - Unhandled");
						break;
				}
			}
			break;

		case RT_ConstantBuffer:
			bReuseable = false; // TODO - Could reuse in theory
			break;

		case RT_VertexBuffer:
		case RT_IndexBuffer:
		case RT_StreamOutput:
		case RT_IndirectBuffer:
		case RT_UnorderedAccess:
			if (poData->oResource.oBuffer_10.Usage == D3D10_USAGE_IMMUTABLE)
			{
				bReuseable = false;
			}
			break;

		case RT_DSView:
		case RT_RTView:
		case RT_SRView:
		case RT_UNVIew:
			bReuseable = false;
			break;

		case RT_Staging:
			bReuseable = false; // TODO - Could reuse in theory
			break;

		default:
			Quitf("Unknown Resource Type");
			return false;
		}
	}
#endif // (!REUSE_IMMUTABLE_RESOURCES && __D3D11)

	// Add to Cache List
	for (u32 uIndex = 0; uIndex < MT_Last; uIndex++)
	{
		m_auTypeTotalActiveMemory[uIndex][poData->eType] -= poData->auMemory[uIndex];
		m_auTypeTotalActiveMemory[uIndex][RT_Totals] -= poData->auMemory[uIndex];
#if REUSE_RESOURCE
		if (bReuseable)
		{
			m_auTypeTotalCachedMemory[uIndex][poData->eType] += poData->auMemory[uIndex];
			m_auTypeTotalCachedMemory[uIndex][RT_Totals] += poData->auMemory[uIndex];
		}
#endif // REUSE_RESOURCE
	}

	m_auTotalActiveItems[MT_Video][poData->eType]--;
	m_auTotalActiveItems[MT_Video][RT_Totals]--;
#if REUSE_RESOURCE
	if (bReuseable)
	{
		m_auTotalCacheItems[MT_Video][poData->eType]++;
		m_auTotalCacheItems[MT_Video][RT_Totals]++;

		m_auNewCacheItems[MT_Video][poData->eType]++;
		m_auNewCacheItems[MT_Video][RT_Totals]++;
	}
#endif // REUSE_RESOURCE	
	poData->Node.Data = poData;
#ifndef FINAL
	poData->Node.SetNext(NULL);
	poData->Node.SetPrev(NULL);
#endif // FINAL

#if REUSE_RESOURCE
	if (bReuseable)
	{
		m_lstFreeCache.Append(poData->Node);

		// Start of Add to multimap/map/map list
		MultiMapCache::iterator itCache = m_ammapCachedResources[poData->eType].find(Max(poData->auMemory[MT_Video], poData->auMemory[MT_System]));
		if (itCache == m_ammapCachedResources[poData->eType].end())
		{
			sysMemStartTemp();
			// This size is not in the list already
			// Add new map to track this size in multimap
			MapSubClass *pmapSubClass = rage_new MapSubClass();
			itCache = m_ammapCachedResources[poData->eType].insert(std::pair<u32, MapSubClass*>(Max(poData->auMemory[MT_Video], poData->auMemory[MT_System]), pmapSubClass));
			Assert(itCache != m_ammapCachedResources[poData->eType].end());
			sysMemEndTemp();
		}
		u32 uCRC = poData->uCRC;

		MapSubClass *pmapSubClass = itCache->second;
		MapSubClass::iterator itCRC = pmapSubClass->find(uCRC);		
		if (itCRC == pmapSubClass->end())
		{
			sysMemStartTemp();
			MapResource *pmapResources = rage_new MapResource();
			(*pmapSubClass)[uCRC] = pmapResources;
			itCRC = pmapSubClass->find(uCRC);
			Assert(itCRC != pmapSubClass->end());
			sysMemEndTemp();
		}

		sysMemStartTemp();
		MapResource *pmapResources = itCRC->second;
		Assert(pmapResources->find(poData->pvRetResource) == pmapResources->end());
		pmapResources->insert(std::pair< void*, ResourceData* >(poData->pvRetResource, poData));
		sysMemEndTemp();
		// End of Add to multimap/map/map list
	}
#endif // REUSE_RESOURCE

#if REUSE_RESOURCE
	if (!bReuseable)
	{
		m_lstFreeCache.PopNode(poData->Node);
		AssertVerify(ReleaseCacheItem(&poData->Node));
	}
	else
	{
		DEBUG_RESOURCE(char szName[256];)
		DEBUG_RESOURCE(GetName(static_cast<ID3D11Resource*>(poData->pvRetResource), szName, 254);)
		DEBUG_RESOURCE(grcDisplayf("Caching %p - %s", pvResource, szName);)
	}
#else
	AssertVerify(ReleaseCacheItem(&poData->Node));
#endif // REUSE_RESOURCE

	// Free up space in cache in case of overflow
	if (grcTextureFactoryPC::HasInstance())
	{
		for (u32 uIndex = 0; uIndex < MT_System; uIndex++)
		{
			while(((s64)GetTotalFreeMemory((MemoryType)uIndex, false) <= 0) && (GetTotalMemory(MT_Video) > 0))
			{
				Assert(IS_LOCKED_DELETION_QUEUE(1));
				if (!CleanCache((MemoryType)uIndex))
				{
					break;
				}
			}
		}
	}
#if REUSE_RESOURCE
	if (PARAM_nocache.Get())
	{
		Assert(IS_LOCKED_DELETION_QUEUE(1));
		while (CleanCache(MT_Last)) {}
	}
#endif // REUSE_RESOURCE
	return true;
}

bool grcResourceCache::CleanCache(MemoryType REUSE_RESOURCE_ONLY(eType))
{
#if REUSE_RESOURCE
	PF_AUTO_PUSH_DETAIL("grcResourceCache::CleanCache");
	CacheListNode* poNode = NULL;
	if (eType >= MT_Last)
	{
		poNode = m_lstFreeCache.PopHead();
		if (poNode == NULL)
			return false;
	}
	else
	{
		// Search for matching resource type we want to free up.
		poNode = m_lstFreeCache.GetHead();
		while (poNode != NULL)
		{
			ResourceData* poData = poNode->Data;
			if (poData->auMemory[eType] > 0) // Found Node
			{
				m_lstFreeCache.PopNode(*poNode);
				break;
			}
				
			poNode = poNode->GetNext();
		}
	}

	return (poNode == NULL) ? false : ClearCacheItem(poNode);
#else
	return false;
#endif // REUSE_RESOURCE
}

bool grcResourceCache::ClearCacheItem(CacheListNode* poNode)
{
	Assertf(IS_LOCKED_DELETION_QUEUE(1), "Deletion queue should be locked before calling ClearCacheItem - For performance reasons");
	AssertMsg((__D3D11) || GRCDEVICE.CheckThreadOwnership(),"Attempted to clear cache item outside of render thread");
	Assert(poNode != NULL);
	Assert(poNode->Data != NULL);

#if REUSE_RESOURCE
	ResourceData* poData = poNode->Data;
	ResourceType eType = poData->eType;

	for (u32 uIndex = 0; uIndex < MT_Last; uIndex++)
	{
		m_auTypeTotalCachedMemory[uIndex][RT_Totals] -= poData->auMemory[uIndex];
		m_auTypeTotalCachedMemory[uIndex][eType] -= poData->auMemory[uIndex];
	}
	m_auNewClearCacheItems[MT_Video][eType]++;
	m_auNewClearCacheItems[MT_Video][RT_Totals]++;
	m_auTotalCacheItems[MT_Video][RT_Totals]--;
	m_auTotalCacheItems[MT_Video][eType]--;
#endif // REUSE_RESOURCE

#if REUSE_RESOURCE
	MultiMapCache::iterator itCache = m_ammapCachedResources[eType].find(Max(poData->auMemory[MT_Video], poData->auMemory[MT_System]));
	Assert(itCache != m_ammapCachedResources[eType].end());
	if (itCache != m_ammapCachedResources[eType].end())
	{
		u32 uCRC = poData->uCRC;

		MapSubClass *pmapSubClass = itCache->second;
		MapSubClass::iterator itCRC = pmapSubClass->find(uCRC);
		Assert(itCRC != pmapSubClass->end());
		if (itCRC != pmapSubClass->end())
		{
			// Found - Should always be the best match
			MapResource *pmapResources = itCRC->second;
			MapResource::iterator itResource = pmapResources->find(poData->pvRetResource);
			Assert(itResource != pmapResources->end());
			if (itResource != pmapResources->end())
			{
				sysMemStartTemp();
				// Found
				// ResourceData* poData = itResource->second;
				// Now remove from list and clear issue caused by the removal of the resource
				pmapResources->erase(itResource);
				if (pmapResources->empty())
				{
					// Remove map and delete of CRC type
					pmapSubClass->erase(itCRC);
					delete pmapResources;
				}
				if (pmapSubClass->empty())
				{
					// Remove map and delete of Size type
					m_ammapCachedResources[eType].erase(itCache);
					delete pmapSubClass;
				}
				sysMemEndTemp();
			}
		}
	}
	sysMemStartTemp();
	m_lstFreeCache.PopNode(poData->Node);
	sysMemEndTemp();
#endif // REUSE_RESOURCE
	return ReleaseCacheItem(poNode);
}

bool grcResourceCache::ReleaseCacheItem(CacheListNode* poNode)
{
	Assertf(IS_LOCKED_DELETION_QUEUE(1), "Deletion queue should be locked before calling ClearCacheItem - For performance reasons");
	AssertMsg((__D3D11) || GRCDEVICE.CheckThreadOwnership(),"Attempted to clear cache item outside of render thread");
	Assert(poNode != NULL);
	Assert(poNode->Data != NULL);

	ResourceData* poData = poNode->Data;
	ASSERT_ONLY(ResourceType eType = poData->eType;)
	IUnknown* pResource = static_cast<IUnknown*>(poData->pvRetResource);

	m_ResDataPool->Delete(poData);
	poData = NULL;

	DEBUG_RESOURCE(char szName[256];)
	DEBUG_RESOURCE(GetName(static_cast<ID3D11Resource*>(pResource), szName, 254);)
	DEBUG_RESOURCE(grcDisplayf("Releasing %p - %s", pResource, szName);)

	UNLOCK_DELETION_QUEUE
	ASSERT_ONLY(ULONG uRefCnt =) pResource->Release();
#if __ASSERT
	if (uRefCnt != EXPECT_REF_COUNT)
	{
		if ((!__D3D11) || (uRefCnt > 2))
		{
			Assert(eType < RT_LAST);
			grcWarningf("%s still has %u reference counts when attempting to delete - Expected on DX10/DX11 Immutable Resources", astrResourceNames[eType], uRefCnt);
		}
	}
#endif // __ASSERT
	LOCK_DELETION_QUEUE
	return true;
}

bool grcResourceCache::IsQueuedForCaching(void* pvResource)
{
	for (int item = 0; item < m_lstItemToFree.GetCount(); ++item)
	{
		if (m_lstItemToFree[item] && (m_lstItemToFree[item] == pvResource))
		{
			u32 uRefCount = ((IUnknown*)pvResource)->AddRef();
			uRefCount = ((IUnknown*)pvResource)->Release();

			if (uRefCount == 1)
			{
				Assertf(false, "Resource already queued for deletion");
				UNLOCK_DELETION_QUEUE
				return true;
			}
		}
	}
	return false;
}

bool grcResourceCache::QueueForCaching(void* pvResource)
{
	//PF_AUTO_PUSH_DETAIL("grcResourceCache::QueueForCaching"); // Overflow during flushes that don't update time bars.  Too many resources tossed out.
	LOCK_DELETION_QUEUE

#if __DEV
	if (IsQueuedForCaching(pvResource))
		return false;

	ASSERT_ONLY(MapActive::iterator itActive = m_mapActiveResources.find(pvResource));
	Assertf(itActive != m_mapActiveResources.end(), "Attempting to delete resource not in resource manager %p - If display surfaces in DX10 or higher this is expected", pvResource);

	u32 uRefCount = ((IUnknown*)pvResource)->AddRef();
	uRefCount = ((IUnknown*)pvResource)->Release();
	if (uRefCount != 1)
	{
		Warningf("%p reference count %d", pvResource, uRefCount);
	}
#if HEAVY_VALIDATE
	char szName[256];
	ID3D11DeviceChild *const deviceChild = static_cast<ID3D11DeviceChild*>(pvResource);
	UINT iNameSize = sizeof(szName)-1;

	deviceChild->GetPrivateData(WKPDID_D3DDebugObjectName, &iNameSize, szName);

	DEBUG_RESOURCE(static u32 uCount = 0;)
	if (!iNameSize)
	{
		DEBUG_RESOURCE(grcDisplayf("Queue %d for Deletion %p", uCount++, pvResource);)
	}
	else
	{
		DEBUG_RESOURCE(grcDisplayf("Queue %d - %s for Deletion %p", uCount++, szName, pvResource);)
	}
#endif // HEAVY_VALIDATE
#endif // __DEV

	if (m_lstItemToFree.GetCount() + 16 >= FREELIST_BUFFER_SIZE)
	{
		// Force Flush the list unfortunately - This will cause a heavy stalls
		UNLOCK_DELETION_QUEUE
		ClearCacheQueue();
		LOCK_DELETION_QUEUE
	}
	// Store for render thread safe deletion
	m_lstItemToFree.Push /*AndGrow*/(pvResource);
	UNLOCK_DELETION_QUEUE

	return true;
}

bool grcResourceCache::ClearCacheQueue()
{
	//PF_AUTO_PUSH_DETAIL("grcResourceCache::ClearCacheQueue");
#if __NO_OUTPUT
	if (!IsOkToCreateResources())
	{
		Warningf("ClearCacheQueue should not occur on main/render thread as it will cause a stall, Render Thread %d Main Thread %d", GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), g_MainThreadId == sysIpcGetCurrentThreadId());
	}
#endif // !__NO_OUTPUT

	LOCK_DELETION_QUEUE

	while(!m_lstItemToFree.empty())
	{
		void* pvResource = m_lstItemToFree.Pop();
		Assert(pvResource != NULL);

		MapActive::iterator itActive = m_mapActiveResources.find(pvResource);
		if (itActive != m_mapActiveResources.end())
		{
			// About to be deleted put into cache
			if (!Cache(pvResource))
			{
				if (0) // GRCDEVICE.GetResourcePool() == RESOURCE_MANAGED)
				{
					grcWarningf("Not in Active List");
				}
			}
		}
		else
		{
			Warningf("This should be the matching \"Release Issued that didn't dereference the resource properly\"");
		}
	}
	UNLOCK_DELETION_QUEUE
	return true;
}

bool grcResourceCache::ManageSystemMemoryCache(bool 
#if ENABLE_SYSTEMCACHE
												   bFailedAllocation
#endif // ENABLE_SYSTEMCACHE
											   )
{
#if ENABLE_SYSTEMCACHE
	if (!bFailedAllocation REUSE_RESOURCE_ONLY( && GetCachedMemory(MT_System, RT_Totals) < sm_aiMaxCacheMemory[MT_System]))
		return true;

	SYSTEMMEMORY_LOCK_RESOURCES
	// Too much is allocated.  Free up some system memory
	// Oldest is in the tail of list remove from there
	while (bFailedAllocation REUSE_RESOURCE_ONLY( || GetCachedMemory(MT_System, RT_Totals) > sm_aiMaxCacheMemory[MT_System]))
	{
		if (m_listSysMemCache.empty())
		{
			break;
		}
		else
		{
			bFailedAllocation = false;
		}
		
		SysFreeContainer *poListEntry = m_listSysMemCache.back();
		m_listSysMemCache.pop_back();
		void* pvResource = poListEntry;

#if USE_HEAP_ALLOCATION
		size_t iActualSize = poListEntry->m_SizeSort.m_key;
#else
		MEMORY_BASIC_INFORMATION oMemInfo;
		ASSERT_ONLY(u32 uMemSize = ) VirtualQuery(pvResource, &oMemInfo, sizeof(oMemInfo));
		Assert(uMemSize > 0);
		size_t iActualSize = oMemInfo.RegionSize;
#endif

		SystemMemorySizes::iterator itSysMem = m_mmapCacheSysSize.find(iActualSize);
		while((itSysMem->second != pvResource) && (itSysMem != m_mmapCacheSysSize.end()))
		{
			itSysMem++;
			if (itSysMem->first > iActualSize)
			{
				Assertf(0, "Failed to find system memory allocation %x - Size %d - Memory Leak", pvResource, iActualSize);
				break;
			}
		}
		Assert(itSysMem->second == pvResource);
		if (itSysMem->second == pvResource)
		{
			m_mmapCacheSysSize.erase(itSysMem);
#if REUSE_RESOURCE
			m_auTotalCacheItems[MT_System][RT_Totals]--;
			m_auTypeTotalCachedMemory[MT_System][RT_Totals] -= itSysMem->first;
			m_auNewClearCacheItems[MT_System][RT_Totals]++;
			//Displayf("Free %p %d - %d", pvResource, itSysMem->first, m_auTypeTotalCachedMemory[MT_System][RT_Totals] + m_auTypeTotalActiveMemory[MT_System][RT_Totals]);
#endif // #if REUSE_RESOURCE
		}
#if USE_HEAP_ALLOCATION
		sysMemHeapFree(pvResource);
#else
		sysMemVirtualFree(pvResource);
#endif // USE_HEAP_ALLOCATION
	}

	SYSTEMMEMORY_UNLOCK_RESOURCES;
	return !bFailedAllocation;
#else
	return true;
#endif // ENABLE_SYSTEMCACHE
}

u32 grcResourceCache::Match(const ResourceData &oSrc1, const ResourceData &oSrc2)
{
	// NOTE - Assume Src1 is the type we are looking for

	if (oSrc1.eType != oSrc2.eType)
		return false;

	if (memcmp(&oSrc1, &oSrc2, sizeof(ResourceType) + sizeof(DDSURFACEDESC2)) == 0)
	{
		return true;
	}
	// PC TODO - Add support for close matches that can be used 
	// Ex. A vertex buffer that is bigger than the requested size
	if (oSrc1.eType == RT_VertexBuffer)
	{
		// Make sure they match everything that matters
		if ((oSrc1.oGeometry.uFVF != oSrc2.oGeometry.uFVF) ||
			(oSrc1.oGeometry.uPool != oSrc2.oGeometry.uPool) ||
			(oSrc1.oGeometry.uUsage != oSrc2.oGeometry.uUsage))
				return false;

		if (oSrc1.oGeometry.uSize < oSrc2.oGeometry.uSize)
		{
			const u32 uMaxLostMemory = auMatchSize[RT_VertexBuffer];
			u32 uLostMemory = oSrc2.oGeometry.uSize - oSrc1.oGeometry.uSize;
			if (uLostMemory < uMaxLostMemory)
			{
				return uLostMemory;
			}
		}
		return false;
	}
	else if (oSrc1.eType == RT_IndexBuffer)
	{
		// Make sure they match everything that matters
		if ((oSrc1.oGeometry.uFormat != oSrc2.oGeometry.uFormat) ||
			(oSrc1.oGeometry.uPool != oSrc2.oGeometry.uPool) ||
			(oSrc1.oGeometry.uUsage != oSrc2.oGeometry.uUsage))
				return false;

		if (oSrc1.oGeometry.uSize < oSrc2.oGeometry.uSize)
		{
			const u32 uMaxLostMemory = auMatchSize[RT_IndexBuffer];
			u32 uLostMemory = oSrc2.oGeometry.uSize - oSrc1.oGeometry.uSize;
			if (uLostMemory < uMaxLostMemory)
			{
				return uLostMemory;
			}
		}
		return false;
	}
	return false;
}

bool grcResourceCache::UnloadCache()
{
	VIDEOMEMORY_LOCK_RESOURCES;
	ClearCacheQueue();
	LOCK_DELETION_QUEUE
	while (CleanCache(MT_Last)) {}
	UNLOCK_DELETION_QUEUE
	VIDEOMEMORY_UNLOCK_RESOURCES;
	Displayf("ResourceCache - Deleted Cache");
	return true;
}

// Application Specific Data
//#include "grcore/precache.h"

bool grcResourceCache::Precache()
{
	VIDEOMEMORY_LOCK_RESOURCES;
	SYSTEMMEMORY_LOCK_RESOURCES;

	if (PARAM_noprecache.Get())
	{
		SYSTEMMEMORY_UNLOCK_RESOURCES;
		VIDEOMEMORY_UNLOCK_RESOURCES;
		return false;
	}
	static bool bOneTime = false;
	if (bOneTime)
	{
		SYSTEMMEMORY_UNLOCK_RESOURCES;
		VIDEOMEMORY_UNLOCK_RESOURCES;
		return true;
	}

	bOneTime = true;

	/*
	float afAvailableMemoryPerType[RT_LAST] = 
	{
		0.62f,  // RT_Texture - Assuming High Textures
		0.0f,  // RT_VolumeTexture
		0.0f,  // RT_CubeTexture
		0.325f, // RT_VertexBuffer
		0.035f, // RT_IndexBuffer
		0.0f,  // RT_RenderTarget
		0.0f   // RT_DepthStencil
	};

	// Rebalance based on texture quality settings
	if (grcTexturePC::GetTextureQuality() != grcTexturePC::HIGH)
	{
		afAvailableMemoryPerType[RT_Texture] /= powf((float)(1 << (grcTexturePC::HIGH - grcTexturePC::GetTextureQuality())), 2.0f);

		float fSum = 0.0f;

		for (u32 uIndex = 0; uIndex < RT_LAST; uIndex++)
		{
			fSum += afAvailableMemoryPerType[uIndex];
		}
		// Renormalize
		for (u32 uIndex = 0; uIndex < RT_LAST; uIndex++)
		{
			afAvailableMemoryPerType[uIndex] /= fSum;
		}
	}
	
	float fTotalAvailableMemory = (float)GetTotalAvailableMemory() - grcRenderTargetPC::GetTotalMemory();


	float fAvailableMemory;
	u32 uItemCnt;
	ResourceData oResource;

	// Vertex Buffers
	fAvailableMemory = afAvailableMemoryPerType[RT_VertexBuffer] * fTotalAvailableMemory;
	uItemCnt = sizeof(aoPrecacheVertexData) / sizeof(BufferCacheItem);

	oResource.clear();
	oResource.eType = RT_VertexBuffer;
	oResource.oData.ddpfPixelFormat.dwFlags = grcDevice::GetResourcePool();
	oResource.oData.dwFlags = (grcDevice::GetResourcePool() == RESOURCE_MANAGED) ? 0 : D3DUSAGE_WRITEONLY;

	for (u32 uItem = 0; uItem < uItemCnt; uItem++)
	{
		oResource.oData.dwSize = oResource.uRequiredMemory = (aoPrecacheVertexData[uItem].uLength + (D3DRUNTIME_MEMORYALIGNMENT - 1)) & ~(D3DRUNTIME_MEMORYALIGNMENT - 1);
		const u32 uQuantity = (u32)(aoPrecacheVertexData[uItem].fPercentage * fAvailableMemory / (float)oResource.uRequiredMemory);
		for (u32 uCnt = 0; uCnt < uQuantity; uCnt++)
		{
			if (Add(oResource))
			{
				AssertVerify(Cache(oResource.pvRetResource));
			}
			else
			{
				Unlock();
				return false;
			}
		}
	}

	// Index Buffers
	fAvailableMemory = afAvailableMemoryPerType[RT_IndexBuffer] * fTotalAvailableMemory;
	uItemCnt = sizeof(aoPrecacheIndexData) / sizeof(BufferCacheItem);

	oResource.clear();
	oResource.eType = RT_IndexBuffer;
	oResource.oData.ddpfPixelFormat.dwFourCC = D3DFMT_INDEX16;
	oResource.oData.ddpfPixelFormat.dwFlags = grcDevice::GetResourcePool();
	oResource.oData.dwFlags = (grcDevice::GetResourcePool() == RESOURCE_MANAGED) ? 0 : D3DUSAGE_WRITEONLY;

	for (u32 uItem = 0; uItem < uItemCnt; uItem++)
	{
		oResource.oData.dwSize = oResource.uRequiredMemory = (aoPrecacheVertexData[uItem].uLength + (D3DRUNTIME_MEMORYALIGNMENT - 1)) & ~(D3DRUNTIME_MEMORYALIGNMENT - 1);
		const u32 uQuantity = (u32)(aoPrecacheIndexData[uItem].fPercentage * fAvailableMemory / (float)oResource.uRequiredMemory);
		for (u32 uCnt = 0; uCnt < uQuantity; uCnt++)
		{
			if (Add(oResource))
			{
				AssertVerify(Cache(oResource.pvRetResource));
			}
			else
			{
				Unlock();
				return false;
			}
		}
	}

	// Textures
	fAvailableMemory = afAvailableMemoryPerType[RT_Texture] * fTotalAvailableMemory;
	uItemCnt = sizeof(aoPrecacheTextureData) / sizeof(TexBufferCacheItem);

	oResource.clear();
	oResource.eType = RT_Texture;
	oResource.oData.ddpfPixelFormat.dwFlags = grcDevice::GetResourcePool();

	for (u32 uItem = 0; uItem < uItemCnt; uItem++)
	{
		const TexBufferCacheItem &oItem = aoPrecacheTextureData[uItem];
		u32 uShiftScale = grcTexturePC::GetMipLevelScaleQuality(grcImage::STANDARD, oItem.uWidth, oItem.uHeight, oItem.uMipCount, oItem.uFormat);
		oResource.oData.dwWidth = oItem.uWidth >> uShiftScale;
		oResource.oData.dwHeight = oItem.uHeight >> uShiftScale;
		if (sysMemTotalMemory() > (u64)(1.4f * 1024 * 1024 * 1024))
		{
			oResource.oData.dwMipMapCount = oItem.uMipCount - uShiftScale;
		}
		else
		{
			oResource.oData.dwMipMapCount = 1;
		}
		oResource.oData.ddpfPixelFormat.dwFourCC = oItem.uFormat;
		// Clamp to dimension of 4 for NVidia driver issue
		if ((( oItem.uFormat == D3DFMT_DXT1 ) || ( oItem.uFormat == D3DFMT_DXT5 )) && (oResource.oData.dwMipMapCount > 1))
		{
			s32 iMaxMips = _FloorLog2(Min(oResource.oData.dwWidth, oResource.oData.dwHeight));
			if (iMaxMips > 1)
			{
				oResource.oData.dwMipMapCount = (u8)(((s32)oResource.oData.dwMipMapCount > (iMaxMips - 2)) ? Max((iMaxMips - 2),1) : oResource.oData.dwMipMapCount);
			}
		}

		u32 uMemory = oResource.oData.dwWidth * oResource.oData.dwHeight * grcTexturePC::GetD3DBitsPerPixel((u32)oResource.oData.ddpfPixelFormat.dwFourCC) / 8; 
		oResource.uRequiredMemory = (u32)((float)uMemory * ((oResource.oData.dwMipMapCount > 1) ? 1.4f : 1.0f));
		oResource.uRequiredMemory = (oResource.uRequiredMemory + (D3DRUNTIME_MEMORYALIGNMENT - 1)) & ~(D3DRUNTIME_MEMORYALIGNMENT - 1);

		const u32 uQuantity = (u32)(aoPrecacheTextureData[uItem].fPercentage * fAvailableMemory / (float)oResource.uRequiredMemory);
		for (u32 uCnt = 0; uCnt < uQuantity; uCnt++)
		{
			if (Add(oResource))
			{
				AssertVerify(Cache(oResource.pvRetResource));
			}
			else
			{
				Unlock();
				return false;
			}
		}
	}
	ResetStats();
	*/
	SYSTEMMEMORY_UNLOCK_RESOURCES;
	VIDEOMEMORY_UNLOCK_RESOURCES;
	return true;
}

void grcResourceCache::ResetStats()
{

	for (u32 uIndex = 0; uIndex <= RT_Totals; uIndex++)
	{
		m_auNewAddItems[MT_Video][uIndex] = 0;
#if REUSE_RESOURCE
		m_auNewCacheItems[MT_Video][uIndex] = 0;
		m_auNewClearCacheItems[MT_Video][uIndex] = 0;
		m_auNewUsedCacheItems[MT_Video][uIndex] = 0;
#endif // REUSE_RESOURCE
	}
}

bool grcResourceCache::GetName(ID3D11Resource *pResource, char* pszName, int maxLen)
{
	char szName[256] = { "" };
	if (pResource)
	{
		ID3D11DeviceChild *const deviceChild = static_cast<ID3D11DeviceChild*>(pResource);
		UINT iOldNameSize = sizeof(szName)-1;

		deviceChild->GetPrivateData(WKPDID_D3DDebugObjectName, &iOldNameSize, szName);
		if (iOldNameSize)
		{
			formatf(pszName, maxLen, szName);
			return true;
		}

		/*
		if (!iOldNameSize)
		{
			grcTexture* poTexture = NULL;
			UINT iSize = sizeof(poTexture);
			deviceChild->GetPrivateData(TextureBackPointerGuid, &iSize ,&poTexture);
			if (iSize)
			{
				if (poTexture->GetName())
					formatf(pszName, "%s", poTexture->GetName());
				else
					formatf(pszName, "No name in texture");
			}
			else
			{
				if (IsQueuedForCaching(poData->pvRetResource))
					formatf(szName, "Texture queued for deleteion");
				else
					formatf(szName, "Leaked Texture no longer exists");
			}
		}
		*/
	}
	formatf(pszName, maxLen, "Unknown");
	return false;
}

#if __BANK

void grcResourceCache::SaveResource(fiStream* fid, ResourceData* poData, u32 uResType)
{
	if ((size_t)poData->pvRetResource == 0xEEEEEEEE)
	{
		Errorf("Bad Resource %p", poData->pvRetResource);
		return;
	}
#if !__D3D11
	if (!__D3D11)
	{
		if (poData->eType == (s32)uResType)
		{
			switch (uResType)
			{
			case RT_Texture:
			{
				rage::grcTexture* pTexture = NULL;
				UINT uSize = sizeof(pTexture);

				IDirect3DTexture9* pD3DTexture = (IDirect3DTexture9*)poData->pvRetResource;
				pD3DTexture->GetPrivateData(TextureBackPointerGuid, &pTexture, (DWORD*)&uSize);

#if 0	// We want to crash here
				if (*(u32*)pTexture == 0xEEEEEEEE)
				{
					Warningf("Texture has been deleted: %p", pTexture);
					break;
				}
#endif	//0
				fprintf(fid, "%u, %u, %u, %u, %u, %u, %u, %u, %s\r\n",
					poData->oTexture.uWidth,
					poData->oTexture.uHeight,
					poData->oTexture.uLevels,
					poData->oTexture.uUsage,
					poData->oTexture.uFormat,
					poData->oTexture.uPool,
					poData->auMemory[MT_Video],
					poData->auMemory[MT_System],
					// NOTE: ejanderson - pTexture is invalid during the shutdown process. So.... don't try 
					// to invoke virtual functions on an object without a valid vtable!
					(pTexture && !grcResourceCache::GetInstance().IsCacheEmpty()) ? pTexture->GetName() : "");
				break;
			}
			case RT_VolumeTexture:
			{
				rage::grcTexture* pTexture = NULL;
				UINT uSize = sizeof(pTexture);

				IDirect3DVolumeTexture9* pD3DTexture = (IDirect3DVolumeTexture9*)poData->pvRetResource;
				pD3DTexture->GetPrivateData(TextureBackPointerGuid, &pTexture, (DWORD*)&uSize);

				fprintf(fid, "%u, %u, %u, %u, %u, %u, %u, %u, %u, %s\r\n",
					poData->oTexture.uWidth,
					poData->oTexture.uHeight,
					poData->oTexture.uDepth,
					poData->oTexture.uLevels,
					poData->oTexture.uUsage,
					poData->oTexture.uFormat,
					poData->oTexture.uPool,
					poData->auMemory[MT_Video],
					poData->auMemory[MT_System],
					// NOTE: ejanderson - pTexture is invalid during the shutdown process. So.... don't try 
					// to invoke virtual functions on an object without a valid vtable!
					(pTexture && !grcResourceCache::GetInstance().IsCacheEmpty()) ? pTexture->GetName() : "");
				break;
			}
			case RT_CubeTexture:
				fprintf(fid, "%u, %u, %u, %u, %u, %u, %u\r\n",
					poData->oTexture.uEdgeLength,
					poData->oTexture.uLevels,
					poData->oTexture.uUsage,
					poData->oTexture.uFormat,
					poData->oTexture.uPool,
					poData->auMemory[MT_Video],
					poData->auMemory[MT_System]);
				break;
			case RT_VertexBuffer:
				fprintf(fid, "%u, %u, %u, %u, %u, %u\r\n",
					poData->oGeometry.uSize,
					poData->oGeometry.uUsage,
					poData->oGeometry.uFVF,
					poData->oGeometry.uPool,
					poData->auMemory[MT_Video],
					poData->auMemory[MT_System]);
				break;
			case RT_IndexBuffer:
				fprintf(fid, "%u, %u, %u, %u, %u, %u\r\n",
					poData->oGeometry.uSize,
					poData->oGeometry.uUsage,
					poData->oGeometry.uFormat,
					poData->oGeometry.uPool,
					poData->auMemory[MT_Video],
					poData->auMemory[MT_System]);
				break;
			case RT_RenderTarget:
				fprintf(fid, "%u, %u, %u, %u, %u, %u, %u, %u\r\n",
					poData->oTexture.uWidth,
					poData->oTexture.uHeight,
					poData->oTexture.uFormat,
					poData->oTexture.uMultiSample,
					poData->oTexture.uMultiSampleQuality,
					poData->oTexture.uLockable,
					poData->auMemory[MT_Video],
					poData->auMemory[MT_System]);
				break;
			case RT_DepthStencil:
				fprintf(fid, "%u, %u, %u, %u, %u, %u, %u, %u\r\n",
					poData->oTexture.uWidth,
					poData->oTexture.uHeight,
					poData->oTexture.uFormat,
					poData->oTexture.uMultiSample,
					poData->oTexture.uMultiSampleQuality,
					poData->oTexture.uDiscard,
					poData->auMemory[MT_Video],
					poData->auMemory[MT_System]);
				break;
			default:
				break;
			}
		}
	}
#else
	// else
	{
		switch (uResType)
		{
		case RT_Texture:
			{
				char szName[256] = { "" };
				if (poData->pvRetResource)
				{
					ID3D11DeviceChild *const deviceChild = static_cast<ID3D11DeviceChild*>(poData->pvRetResource);
					UINT iOldNameSize = sizeof(szName)-1;

					deviceChild->GetPrivateData(WKPDID_D3DDebugObjectName, &iOldNameSize, szName);
					if (!iOldNameSize)
					{
						grcTexture* poTexture = NULL;
						UINT iSize = sizeof(poTexture);
						deviceChild->GetPrivateData(TextureBackPointerGuid, &iSize ,&poTexture);
						if (iSize)
						{
							if (poTexture->GetName())
								formatf(szName, "%s", poTexture->GetName());
							else
								formatf(szName, "No name in texture");
						}
						else
						{
							if (IsQueuedForCaching(poData->pvRetResource))
								formatf(szName, "Texture queued for deleteion");
							else
								formatf(szName, "Leaked Texture no longer exists");
						}
					}
				}

				switch(poData->uSize)
				{
				case (sizeof(D3D10_TEXTURE1D_DESC)):
					fprintf(fid, "%s, 1D,          %5u, %6u, %7u, %5u, %6u, %5x, %4u, %u, %u\r\n",
						szName,
						poData->oResource.oTex1D_10.Width,
						0,
						0,
						poData->oResource.oTex1D_10.ArraySize,
						poData->oResource.oTex1D_10.Format,
						poData->oResource.oTex1D_10.Usage,
						poData->oResource.oTex1D_10.BindFlags,
						poData->auMemory[MT_Video],
						poData->auMemory[MT_System]);
					break;
				case (sizeof(D3D10_TEXTURE2D_DESC)):
					fprintf(fid, "%s, 2D,          %5u, %6u, %7u, %5u, %6u, %5x, %4u, %u, %u\r\n",
						szName,
						poData->oResource.oTex2D_10.Width,
						poData->oResource.oTex2D_10.Height,
						poData->oResource.oTex2D_10.MipLevels,
						poData->oResource.oTex2D_10.ArraySize,
						poData->oResource.oTex2D_10.Format,
						poData->oResource.oTex2D_10.Usage,
						poData->oResource.oTex2D_10.BindFlags,
						poData->auMemory[MT_Video],
						poData->auMemory[MT_System]);
					break;
				case (sizeof(D3D10_TEXTURE3D_DESC)):
					fprintf(fid, "%s, 3D,          %5u, %6u, %7u, %5u, %6u, %5x, %4u, %u, %u\r\n",
						szName,
						poData->oResource.oTex3D_10.Width,
						poData->oResource.oTex3D_10.Height,
						poData->oResource.oTex3D_10.MipLevels,
						poData->oResource.oTex3D_10.Depth,
						poData->oResource.oTex3D_10.Format,
						poData->oResource.oTex3D_10.Usage,
						poData->oResource.oTex3D_10.BindFlags,
						poData->auMemory[MT_Video],
						poData->auMemory[MT_System]);
					break;
				default:
					break;
				}
			}
			break;
		case RT_VertexBuffer:
		case RT_IndexBuffer:
		case RT_ConstantBuffer:
		case RT_StreamOutput:
		case RT_Staging:
		case RT_IndirectBuffer:
			fprintf(fid, "%u, %u, %x, %x, %u, %u\r\n",
				poData->oResource.oBuffer_10.ByteWidth,
				poData->oResource.oBuffer_10.Usage,
				poData->oResource.oBuffer_10.BindFlags,
				poData->oResource.oBuffer_10.CPUAccessFlags,
				poData->auMemory[MT_Video],
				poData->auMemory[MT_System]);
			break;
		default:
			break;
		}
	}
#endif // (!__D3D11)
}

void grcResourceCache::SaveRenderTargets(bool bEnable, const char* poFileName)
{
	if (!bEnable && !m_bSaveRTs)
		return;

	m_bSaveRTs = false;
	DumpMemoryStats();

	char szFileName[2048] = { "RenderTargets.txt" };
	if (poFileName != NULL)
	{
		strncpy(szFileName, poFileName, 2047);
		szFileName[2047] = 0;
	}

	fiStream* fid;
	fid = fiStream::Create(szFileName);  // open for writing
	if (!fid)
	{
		grcWarningf("Fail to Open %s for write", szFileName);
		return;
	}

	fprintf(fid, "Name, Dimensions, Width, Height, MipCount, Array, Format, Usage, Bind, Memory(V/S), Count\r\n");
	
	MapActive::iterator itActiveResource = m_mapActiveResources.begin();
	while(itActiveResource != m_mapActiveResources.end())
	{
		ResourceData* poData = itActiveResource->second;
		if (poData->eType == (s32)RT_Texture)
		{
			u32 flags = 0;

			switch(poData->uSize)
			{
				case (sizeof(D3D10_TEXTURE1D_DESC)):
					flags = poData->oResource.oTex1D_10.BindFlags;
				break;

				case (sizeof(D3D10_TEXTURE2D_DESC)):
					flags = poData->oResource.oTex2D_10.BindFlags;
				break;

				case (sizeof(D3D10_TEXTURE3D_DESC)):
					flags = poData->oResource.oTex3D_10.BindFlags;
				break;

				default:
				break;
			}

			if ((flags & (grcBindRenderTarget | grcBindDepthStencil)) != 0)
			{
				SaveResource(fid, poData, RT_Texture);
			}
		}
		++itActiveResource;
	}

	fid->Close();
}

void grcResourceCache::SaveResourceData(bool bEnable, const char* poFileName)
{
	if (!bEnable && !m_bSaveResourceData)
		return;

	m_bSaveResourceData = false;
	DumpMemoryStats();

	fiStream* fid;

	char szFileName[2048] = { "ResourceStats.txt" };
	if (poFileName != NULL)
	{
		strncpy(szFileName, poFileName, 2047);
		szFileName[2047] = 0;
	}
		
	fid = fiStream::Create(szFileName);  // open for writing
	if (!fid)
	{
		grcWarningf("Fail to Open ResourceStats.txt for write");
		return;
	}

	if (!__D3D11)
	{
		for (u32 uResType = 0; uResType < RT_LAST; uResType++)
		{
			fprintf(fid, "Type %s\r\n", astrResourceNames[uResType]);

			switch (uResType)
			{
				case RT_Texture:
					fprintf(fid, "Width, Height, MipCount, Usage, Format, Pool, Memory, Count, Name\r\n");
					break;
				case RT_VolumeTexture:
					fprintf(fid, "Width, Height, Depth, MipCount, Usage, Format, Pool, Memory, Count\r\n");
					break;
				case RT_CubeTexture:
					fprintf(fid, "Edge, MipCount, Usage, Format, Pool, Memory, Count\r\n");
					break;
				case RT_VertexBuffer:
					fprintf(fid, "Length, Usage, FVF, Pool, Memory, Count\r\n");
					break;
				case RT_IndexBuffer:
					fprintf(fid, "Length, Usage, Format, Pool, Memory, Count\r\n");
					break;
				case RT_RenderTarget:
					fprintf(fid, "Width, Height, Format, Multisample, Multisample Quality, Lockable, Memory, Count\r\n");
					break;
				case RT_DepthStencil:
					fprintf(fid, "Width, Height, Format, Multisample, Multisample Quality, Discard, Memory, Count\r\n");
					break;
				default:
					break;
			}

			MapActive::iterator itActiveResource = m_mapActiveResources.begin();
			while(itActiveResource != m_mapActiveResources.end())
			{
				ResourceData* poData = itActiveResource->second;
				if (poData->eType == (s32)uResType)
				{
					SaveResource(fid, poData, uResType);
				}
				++itActiveResource;
			}

#if REUSE_RESOURCE
			// Dump out cache list as well
			MultiMapCache::iterator itCache = m_ammapCachedResources[uResType].begin();
			while (itCache != m_ammapCachedResources[uResType].end())
			{
				MapSubClass::iterator itCRC = itCache->second->begin();
				while (itCRC != itCache->second->end())
				{
					MapResource::iterator itResource = itCRC->second->begin();
					ResourceData* poData = itResource->second;
					if (poData->eType == (s32)uResType)
					{
						SaveResource(fid, poData, uResType);
					}
					itCRC++;
				}
				itCache++;
			}
#endif // REUSE_RESOURCE
		}
	}
	else
	{
		for (u32 uResType = 0; uResType < RT_LAST; uResType++)
		{
			fprintf(fid, "Type %s\r\n", astrResourceNames[uResType]);
			switch (uResType)
			{
				case RT_Texture:
					fprintf(fid, "Name, Dimensions, Width, Height, MipCount, Array, Format, Usage, Bind, Memory(V/S), Count\r\n");
					break;
				case RT_VertexBuffer:
				case RT_IndexBuffer:
				case RT_ConstantBuffer:
				case RT_StreamOutput:
				case RT_Staging:
				case RT_IndirectBuffer:
					fprintf(fid, "Size, Usage, Bind, CPU, Misc, Count\r\n");
					break;
				default:
					break;
			}

			MapActive::iterator itActiveResource = m_mapActiveResources.begin();
			while(itActiveResource != m_mapActiveResources.end())
			{
				ResourceData* poData = itActiveResource->second;
				if (poData->eType == (s32)uResType)
				{
					SaveResource(fid, poData, uResType);
				}
				++itActiveResource;
			}

#if REUSE_RESOURCE
			// Dump out cache list as well
			MultiMapCache::iterator itCache = m_ammapCachedResources[uResType].begin();
			while (itCache != m_ammapCachedResources[uResType].end())
			{
				MapSubClass::iterator itCRC = itCache->second->begin();
				while (itCRC != itCache->second->end())
				{
					MapResource::iterator itResource = itCRC->second->begin();
					ResourceData* poData = itResource->second;
					if (poData->eType == (s32)uResType)
					{
						SaveResource(fid, poData, uResType);
					}
					itCRC++;
				}
				itCache++;
			}
#endif // REUSE_RESOURCE
		}
	}				
	fid->Close();
}

void grcResourceCache::DisplayStats()
{
	if (!m_bDisplayStats)
		return;

	const u32 iScale = 1024 * 1024;
	static u32 uLastTime = 0;
	static u64 uTotalFreeAppMemory = 0;
	static u64 uLargestFreeBlock = 0;

	u32 uCurTime = sysTimer::GetSystemMsTime();

	grcDebugDraw::AddDebugOutputEx(false, "Video Memory (MB)");
	grcDebugDraw::AddDebugOutputEx(false, "Total %u Active %u Cached %u Used %u Free %d Vid Free %u Streaming %d Extra %u Render Target %d"
		, (u32)(GetTotalMemory(MT_Video) / iScale)
		, (u32)(GetActiveMemory(MT_Video, RT_Totals) / iScale)
		, (u32)(GetCachedMemory(MT_Video, RT_Totals) / iScale)
		, (u32)(GetTotalUsedMemory(MT_Video) / iScale)
		, (s32)(GetTotalFreeMemory(MT_Video, false) / iScale)
		, (u32)(GetVideoCardFreeMemory() / iScale)
		, (s32)(GetStreamingMemory() / iScale)
		, (u32)(GetExtraMemory(MT_Video) / iScale)
		, (u32)(grcRenderTargetPC::GetTotalMemory() / iScale)
		);

	grcDebugDraw::AddDebugOutputEx(false, "Count Active %u Cached %u Used %u", (u32)GetActiveCount(MT_Video, RT_Totals)
		, (u32)GetCachedCount(MT_Video, RT_Totals)
		, (u32)GetCachedCount(MT_Video, RT_Totals) + GetActiveCount(MT_Video, RT_Totals));

	if (m_bDisplaySystemMemStats)
	{
		if (uCurTime > uLastTime)
		{
			uTotalFreeAppMemory = sysMemTotalFreeAppMemory();
			uLargestFreeBlock = sysMemLargestFreeBlock();

		}

		uLastTime = uCurTime + 1000;

		grcDebugDraw::AddDebugOutputEx(false, "System Memory (MB)");
		grcDebugDraw::AddDebugOutputEx(false, "Total %u Active %u Cached %u Used %u Free %u", (u32)(GetTotalMemory(MT_System) / iScale)
			, (u32)(GetActiveMemory(MT_System, RT_Totals) / iScale)
			, (u32)(GetCachedMemory(MT_System, RT_Totals) / iScale)
			, (u32)(GetTotalUsedMemory(MT_System) / iScale)
			, (u32)(GetTotalFreeMemory(MT_System, false) / iScale));

		grcDebugDraw::AddDebugOutputEx(false, "Largest Free Block %uMB App Free %uMB Free Kermem %uMB", /*(u32)sysMemTotalMemory(), (u32)sysMemTotalFreeMemory(),*/ (u32)(uLargestFreeBlock / iScale), (u32)(uTotalFreeAppMemory / iScale), (u32)(sysMemTotalFreeKernelMemory() / iScale));
	}

	{
		grcDebugDraw::AddDebugOutputEx(false, "Attributes %11s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s", 
			GetResourceName(RT_Totals),
			GetResourceName(RT_Texture),
			GetResourceName(RT_ConstantBuffer),
			GetResourceName(RT_VertexBuffer),
			GetResourceName(RT_IndexBuffer),
			GetResourceName(RT_StreamOutput),
			GetResourceName(RT_Staging),
			GetResourceName(RT_IndirectBuffer),
			GetResourceName(RT_UnorderedAccess),
			GetResourceName(RT_DSView),
			GetResourceName(RT_RTView),
			GetResourceName(RT_SRView),
			GetResourceName(RT_UNVIew)
			);

		grcDebugDraw::AddDebugOutputEx(false, "Tot Activ Cnt %8d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d", 
			GetActiveCount(MT_Video, RT_Totals),
			GetActiveCount(MT_Video, RT_Texture),
			GetActiveCount(MT_Video, RT_ConstantBuffer),
			GetActiveCount(MT_Video, RT_VertexBuffer),
			GetActiveCount(MT_Video, RT_IndexBuffer),
			GetActiveCount(MT_Video, RT_StreamOutput),
			GetActiveCount(MT_Video, RT_Staging),
			GetActiveCount(MT_Video, RT_IndirectBuffer),
			GetActiveCount(MT_Video, RT_UnorderedAccess),
			GetActiveCount(MT_Video, RT_DSView),
			GetActiveCount(MT_Video, RT_RTView),
			GetActiveCount(MT_Video, RT_SRView),
			GetActiveCount(MT_Video, RT_UNVIew)
			);
		grcDebugDraw::AddDebugOutputEx(false, "Tot Cache Cnt %8d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d", 
			GetCachedCount(MT_Video, RT_Totals),
			GetCachedCount(MT_Video, RT_Texture),
			GetCachedCount(MT_Video, RT_ConstantBuffer),
			GetCachedCount(MT_Video, RT_VertexBuffer),
			GetCachedCount(MT_Video, RT_IndexBuffer),
			GetCachedCount(MT_Video, RT_StreamOutput),
			GetCachedCount(MT_Video, RT_Staging),
			GetCachedCount(MT_Video, RT_IndirectBuffer),
			GetCachedCount(MT_Video, RT_UnorderedAccess),
			GetCachedCount(MT_Video, RT_DSView),
			GetCachedCount(MT_Video, RT_RTView),
			GetCachedCount(MT_Video, RT_SRView),
			GetActiveCount(MT_Video, RT_UNVIew)
			);


		grcDebugDraw::AddDebugOutputEx(false, "Mem Vid Activ %8d %12d %12d %12d %12d %12d %12d %12d %12d", 
			(u32)(GetActiveMemory(MT_Video, RT_Totals) / iScale),
			(u32)(GetActiveMemory(MT_Video, RT_Texture) / iScale),
			(u32)(GetActiveMemory(MT_Video, RT_ConstantBuffer) / iScale),
			(u32)(GetActiveMemory(MT_Video, RT_VertexBuffer) / iScale),
			(u32)(GetActiveMemory(MT_Video, RT_IndexBuffer) / iScale),
			(u32)(GetActiveMemory(MT_Video, RT_StreamOutput) / iScale),
			(u32)(GetActiveMemory(MT_Video, RT_Staging) / iScale),
			(u32)(GetActiveMemory(MT_Video, RT_IndirectBuffer) / iScale),
			(u32)(GetActiveMemory(MT_Video, RT_UnorderedAccess) / iScale));

		grcDebugDraw::AddDebugOutputEx(false, "Mem Sys Activ %8d %12d %12d %12d %12d %12d %12d %12d %12d", 
			(u32)(GetActiveMemory(MT_System, RT_Totals) / iScale),
			(u32)(GetActiveMemory(MT_System, RT_Texture) / iScale),
			(u32)(GetActiveMemory(MT_System, RT_ConstantBuffer) / iScale),
			(u32)(GetActiveMemory(MT_System, RT_VertexBuffer) / iScale),
			(u32)(GetActiveMemory(MT_System, RT_IndexBuffer) / iScale),
			(u32)(GetActiveMemory(MT_System, RT_StreamOutput) / iScale),
			(u32)(GetActiveMemory(MT_System, RT_Staging) / iScale),
			(u32)(GetActiveMemory(MT_System, RT_IndirectBuffer) / iScale),
			(u32)(GetActiveMemory(MT_System, RT_UnorderedAccess) / iScale));

#if 0 // System Cache
		grcDebugDraw::AddDebugOutputEx(false, "Mem Sys Cach  %8d %12d %12d %12d %12d %12d %12d %12d %12d", 
			(u32)(GetCachedMemory(MT_System, RT_Totals) / iScale),
			(u32)(GetCachedMemory(MT_System, RT_Texture) / iScale),
			(u32)(GetCachedMemory(MT_System, RT_ConstantBuffer) / iScale),
			(u32)(GetCachedMemory(MT_System, RT_VertexBuffer) / iScale),
			(u32)(GetCachedMemory(MT_System, RT_IndexBuffer) / iScale),
			(u32)(GetCachedMemory(MT_System, RT_StreamOutput) / iScale),
			(u32)(GetCachedMemory(MT_System, RT_Staging) / iScale),
			(u32)(GetCachedMemory(MT_System, RT_IndirectBuffer) / iScale),
			(u32)(GetCachedMemory(MT_System, RT_UnorderedAccess) / iScale));
#endif // System Cache

		grcDebugDraw::AddDebugOutputEx(false, "Resource Usage");

		for (u32 MemType = MT_Video; MemType < MT_System; MemType++)
		{
			const char* pszName = (MemType == MT_System) ? "Sys" : "Vid";
			MemoryType uType = (MemoryType)MemType;

			grcDebugDraw::AddDebugOutputEx(false, "Add %s   %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d", 
				pszName,
				GetNewAddItemsCount(uType, RT_Totals),
				GetNewAddItemsCount(uType, RT_Texture),
				GetNewAddItemsCount(uType, RT_ConstantBuffer),
				GetNewAddItemsCount(uType, RT_VertexBuffer),
				GetNewAddItemsCount(uType, RT_IndexBuffer),
				GetNewAddItemsCount(uType, RT_StreamOutput),
				GetNewAddItemsCount(uType, RT_Staging),
				GetNewAddItemsCount(uType, RT_IndirectBuffer),
				GetNewAddItemsCount(uType, RT_UnorderedAccess),
				GetNewAddItemsCount(uType, RT_DSView),
				GetNewAddItemsCount(uType, RT_RTView),
				GetNewAddItemsCount(uType, RT_SRView),
				GetNewAddItemsCount(uType, RT_UNVIew));

			grcDebugDraw::AddDebugOutputEx(false, "Cache %s %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d", 
				pszName,
				GetNewCacheItemsCount(uType, RT_Totals),
				GetNewCacheItemsCount(uType, RT_Texture),
				GetNewCacheItemsCount(uType, RT_ConstantBuffer),
				GetNewCacheItemsCount(uType, RT_VertexBuffer),
				GetNewCacheItemsCount(uType, RT_IndexBuffer),
				GetNewCacheItemsCount(uType, RT_StreamOutput),
				GetNewCacheItemsCount(uType, RT_Staging),
				GetNewCacheItemsCount(uType, RT_IndirectBuffer),
				GetNewCacheItemsCount(uType, RT_UnorderedAccess),
				GetNewCacheItemsCount(uType, RT_DSView),
				GetNewCacheItemsCount(uType, RT_RTView),
				GetNewCacheItemsCount(uType, RT_SRView),
				GetNewCacheItemsCount(uType, RT_UNVIew));

			grcDebugDraw::AddDebugOutputEx(false, "Clear %s %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d", 
				pszName,
				GetNewClearCacheItemsCount(uType, RT_Totals),
				GetNewClearCacheItemsCount(uType, RT_Texture),
				GetNewClearCacheItemsCount(uType, RT_ConstantBuffer),
				GetNewClearCacheItemsCount(uType, RT_VertexBuffer),
				GetNewClearCacheItemsCount(uType, RT_IndexBuffer),
				GetNewClearCacheItemsCount(uType, RT_StreamOutput),
				GetNewClearCacheItemsCount(uType, RT_Staging),
				GetNewClearCacheItemsCount(uType, RT_IndirectBuffer),
				GetNewClearCacheItemsCount(uType, RT_UnorderedAccess),
				GetNewClearCacheItemsCount(uType, RT_DSView),
				GetNewClearCacheItemsCount(uType, RT_RTView),
				GetNewClearCacheItemsCount(uType, RT_SRView),
				GetNewClearCacheItemsCount(uType, RT_UNVIew));

			grcDebugDraw::AddDebugOutputEx(false, "Reuse %s %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d %12d", 
				pszName,
				GetNewUsedCacheItemsCount(uType, RT_Totals),
				GetNewUsedCacheItemsCount(uType, RT_Texture),
				GetNewUsedCacheItemsCount(uType, RT_ConstantBuffer),
				GetNewUsedCacheItemsCount(uType, RT_VertexBuffer),
				GetNewUsedCacheItemsCount(uType, RT_IndexBuffer),
				GetNewUsedCacheItemsCount(uType, RT_StreamOutput),
				GetNewUsedCacheItemsCount(uType, RT_Staging),
				GetNewUsedCacheItemsCount(uType, RT_IndirectBuffer),
				GetNewUsedCacheItemsCount(uType, RT_UnorderedAccess),
				GetNewUsedCacheItemsCount(uType, RT_DSView),
				GetNewUsedCacheItemsCount(uType, RT_RTView),
				GetNewUsedCacheItemsCount(uType, RT_SRView),
				GetNewUsedCacheItemsCount(uType, RT_UNVIew));
		}
	}
}

void grcResourceCache::DumpMemoryStats()
{
	if (!m_bDumpMemoryStats)
		return;

	static bool bOpen = false;
	static HANDLE process = NULL;
	const u32 uUpdateRate = 5000;
	static u32 uNextUpdate = uUpdateRate;
	static u32 uLastTime = 0;

	u32 uCurTime = sysTimer::GetSystemMsTime();
	if (uCurTime < uLastTime)
		return;

	uLastTime = uCurTime + uNextUpdate;
	if (!bOpen)
	{
		process = OpenProcess(
			PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 
			false,
			GetCurrentProcessId());
	}

	unsigned char *p = NULL;
	MEMORY_BASIC_INFORMATION info;

	u64 uCommited = 0;
	u64 uLargestCommitted = 0;
	u64 uFree = 0;
	u64 uLargestFree = 0;
	u64 uReserved = 0;
	u64 uLargestReserved = 0;

	for ( p = NULL;	VirtualQueryEx(process, p, &info, sizeof(info)) == sizeof(info); p += info.RegionSize ) 
	{
		if (info.State == MEM_FREE) 
		{	
			uFree += info.RegionSize;
			uLargestFree = max(info.RegionSize, uLargestFree);
		}
		else if (info.State == MEM_COMMIT)
		{
			uCommited += info.RegionSize;
			uLargestCommitted = max(info.RegionSize, uLargestCommitted);
		}
		else if (info.State == MEM_RESERVE)
		{
			uReserved += info.RegionSize;
			uLargestReserved = max(info.RegionSize, uLargestReserved);
		}
	}

	Displayf("Comitted %u MB, Reserved %u MB, Free %u MB", uCommited / cuMegaBytes, uReserved / cuMegaBytes, uFree / cuMegaBytes);
	Displayf("Largest - Comitted %u KB, Reserved %u KB, Free %u KB", uLargestCommitted / cuKiloBytes, uLargestReserved / cuKiloBytes, uLargestFree / cuKiloBytes);

	MEMORYSTATUSEX oData;
	oData.dwLength = sizeof(oData);
	if (GlobalMemoryStatusEx(&oData))
	{
		Displayf("Percentage Memory In Use - %u %", oData.dwMemoryLoad);
		Displayf("Total     Physical Memory %u MB", (u32)(oData.ullTotalPhys / cuMegaBytes));
		Displayf("Available Physical Memory %u MB", (u32)(oData.ullAvailPhys / cuMegaBytes));
		Displayf("Size of Commited Memory Limit %u MB", (u32)(oData.ullTotalPageFile / cuMegaBytes));
		Displayf("Size of Available Memory to Commit %u MB", (u32)(oData.ullAvailPageFile / cuMegaBytes));
		Displayf("Total Virtual Memory %u MB", (u32)(oData.ullTotalVirtual / cuMegaBytes));
		Displayf("Available Virtual Memory %u MB", (u32)(oData.ullAvailVirtual / cuMegaBytes));
		Displayf("Availabel Extended Virtual %u MB", (u32)(oData.ullAvailExtendedVirtual / cuMegaBytes));
	}
}

void grcResourceCache::DumpRenderTargetDependencies()
{
	if (!m_bDumpRenderTargetDependencies)
		return;

	m_bDumpRenderTargetDependencies = false;

#if SRC_DST_TRACKING
	grcTextureFactoryDX11::MultiMapTargetTextureSources::iterator itRT;
	const grcRenderTargetDX11* pLast = NULL;

	Printf("\nName, Width, Height, Format, MSAA, Bytes, ---- ");
	sysCriticalSection cs(grcTextureFactoryDX11::sm_Lock);

	for (itRT = grcTextureFactoryDX11::sm_mmapSourceTextureList.begin(); itRT != grcTextureFactoryDX11::sm_mmapSourceTextureList.end(); ++itRT)
	{
		if (!itRT->first->IsValid())
			continue;

		if (itRT->first != pLast)
		{
			pLast = static_cast<const grcRenderTargetDX11*>(itRT->first);
			const grcRenderTargetDX11* pRT = static_cast<const grcRenderTargetDX11*>(pLast);
			int iBytes = (pRT->GetMSAA() ? pRT->GetMSAA() : 1) * pRT->GetWidth() * pRT->GetHeight() * pRT->GetBitsPerPixel() / 8;
			Printf("\n%s, %d, %d, %d, %d, %d, ---- ", 
				pRT->GetName(), pRT->GetWidth(), pRT->GetHeight(), pRT->GetFormat(), pRT->GetMSAA(), iBytes);			
		}
		if (itRT->second != NULL)
			Printf(", %s", itRT->second->GetName());
	}

	typedef std::map< const grcRenderTarget*, const grcRenderTarget* > MapTargetSharedTextures;
	MapTargetSharedTextures mapTargetSharedCandidates;
	MapTargetSharedTextures::iterator itCandidates;

	// Search for candidates that have matching attributes but are not in targets dependency list
	Printf("\nName, Width, Height, Format, MSAA, Bytes, Share Target Candidates ");
	for (itRT = grcTextureFactoryDX11::sm_mmapSourceTextureList.begin(); itRT != grcTextureFactoryDX11::sm_mmapSourceTextureList.end(); ++itRT)
	{
		if (!itRT->first->IsValid())
			continue;

		if (itRT->first != pLast)
		{
			// Dump out the candidate list
			for (itCandidates = mapTargetSharedCandidates.begin(); itCandidates != mapTargetSharedCandidates.end(); itCandidates++)
			{
				const grcRenderTargetDX11* pCanTex = static_cast<const grcRenderTargetDX11*>(itCandidates->first);
				Printf(", %s", pCanTex->GetName());
			}
			mapTargetSharedCandidates.clear();

			pLast = static_cast<const grcRenderTargetDX11*>(itRT->first);
			const grcRenderTargetDX11* pRT = static_cast<const grcRenderTargetDX11*>(pLast);
			int iBytes = (pRT->GetMSAA() ? pRT->GetMSAA() : 1) * pRT->GetWidth() * pRT->GetHeight() * pRT->GetBitsPerPixel() / 8;
			Printf("\n%s, %d, %d, %x, %d, %d, ---- ", 
				pRT->GetName(), pRT->GetWidth(), pRT->GetHeight(), pRT->GetFormat(), pRT->GetMSAA(), iBytes);

			grcTextureFactoryDX11::MultiMapTargetTextureSources::iterator itDstLower, itDstUpper, it, itMatch;
			itDstLower = grcTextureFactoryDX11::sm_mmapSourceTextureList.lower_bound(pLast);
			itDstUpper = grcTextureFactoryDX11::sm_mmapSourceTextureList.upper_bound(pLast);
			
			for (it = grcTextureFactoryDX11::sm_mmapSourceTextureList.begin(); it != grcTextureFactoryDX11::sm_mmapSourceTextureList.end(); ++it)
			{
				const grcRenderTargetDX11* pMatch = static_cast<const grcRenderTargetDX11*>(it->first);

				if (!pMatch->IsValid())
					continue;

				if (pLast != pMatch)
				{
					// Unique Texture - Check to see if it matches
					if (pMatch->IsValid() &&
						(pLast->GetMSAA() == pMatch->GetMSAA()) &&
						(pLast->GetWidth() == pMatch->GetWidth()) &&
						(pLast->GetHeight() == pMatch->GetHeight()) &&
						(pLast->GetBitsPerPixel() == pMatch->GetBitsPerPixel()) &&
						(pLast->GetFormat() == pMatch->GetFormat()))
					{
						// Match - See if item is in our dependency list
						grcTextureFactoryDX11::MultiMapTargetTextureSources::iterator itLower, itUpper;
						itLower = itDstLower;
						itUpper = itDstUpper;
						
						bool bFound = false;
						for (itMatch=itDstLower; itMatch!=itDstUpper; ++itMatch)
						{
							const grcRenderTargetDX11 *pListItem = static_cast<const grcRenderTargetDX11*>(itMatch->second);
							if (pListItem == pMatch)
							{
								bFound = true;
								break;
							}
						}
						if (!bFound)
						{
							mapTargetSharedCandidates[pMatch] = pMatch;
							//Printf(", %s", pMatch->GetName());
						}
					}
				}
			}

		}
	}


#endif // SRC_DST_TRACKING
}

void grcResourceCache::InitWidgets()
{
	bkBank& bank = BANKMGR.CreateBank("rage - Resource Cache");
	bank.AddToggle("Display Resource Cache", &m_bDisplayStats);
	bank.AddToggle("Display System Memory Stats", &m_bDisplaySystemMemStats);

	bank.AddSlider("% Vid Mem For Fragmentation", &g_fPercentAvailablePostFragmentation, 0.0f, 1.0f, 0.01f);
	bank.AddSlider("% Vid Mem For Streamer", &g_fPercentForStreamer, 0.0f, 1.0f, 0.01f);

	bank.AddToggle("Reset Cache Stats", &m_bResetStats);
	bank.AddToggle("Save Out Resource Stats", &m_bSaveResourceData);
	bank.AddToggle("Save Out Render Targets", &m_bSaveRTs);
	bank.AddToggle("Report System Memory Stats", &m_bDumpMemoryStats);
	bank.AddToggle("Dump Render Target Dependencies", &m_bDumpRenderTargetDependencies);
	bank.AddToggle("Use Video Memory Limiter for Streamer", &sm_bManagerResources);
	// bank.AddButton("Report Loaded Unused Textures", datCallback(CFA(grcTextureFactoryPC::ReportUnusedTextures)));

	bank.AddSlider("Total Video Mem", (u32*)&m_aiTotalAvailableMemory[MT_Video], 0U, (u32)(0xFFFFFFFF), (u32)(1024 * 1024));
	bank.AddSlider("Total Sys Mem", (u32*)&m_aiTotalAvailableMemory[MT_System], 0U, (u32)(0xFFFFFFFF), (u32)(1024 * 1024));

	bank.AddSlider("Vid Mem Restrict", (u32*)&sm_aiMemoryRestriction[MT_Video], 0U, (u32)(1<<31), (u32)(1024 * 1024));
	bank.AddSlider("Sys Mem Restrict", (u32*)&sm_aiMemoryRestriction[MT_System], 0U, (u32)(1<<31), (u32)(1024 * 1024));

	bank.AddSlider("Reserved Video Memory", (u32*)&sm_aiReservedMemory[MT_Video], 0U, (u32)(1<<31), (u32)(1024 * 1024));
	bank.AddSlider("Reserved System Memory", (u32*)&sm_aiReservedMemory[MT_System], 0U, (u32)(1<<31), (u32)(1024 * 1024));

	bank.AddSlider("Reserved Application Free Memory", (u32*)&sm_aiReservedApplicationMemory[MT_System], 0U, (u32)(1<<31), 1024 * 1024);

	bank.AddSlider("Total Active Video Memory", (u32*)&m_auTypeTotalActiveMemory[MT_Video][RT_Totals], 0U, (u32)(1<<31), (u32)(1024 * 1024));
#if REUSE_RESOURCE	
	bank.AddSlider("Total Cached Video Memory", (u32*)&m_auTypeTotalCachedMemory[MT_Video][RT_Totals], 0U, (u32)(1<<31), (u32)(1024 * 1024));
#endif // #if REUSE_RESOURCE
	bank.AddSlider("Total Active System Memory", (u32*)&m_auTypeTotalActiveMemory[MT_System][RT_Totals], 0U, (u32)(1<<31), (u32)(1024 * 1024));
#if REUSE_RESOURCE
	bank.AddSlider("Total Cached System Memory", (u32*)&m_auTypeTotalCachedMemory[MT_System][RT_Totals], 0U, (u32)(1<<31), (u32)(1024 * 1024));
#endif // REUSE_RESOURCE
	bank.AddSlider("Extra Video Memory", (u32*)&sm_aiExtraAvailableMemory[MT_Video], 0U, (u32)(1 << 31), (u32)1024* 1024);
	bank.AddSlider("Extra System Memory", (u32*)&sm_aiExtraAvailableMemory[MT_System], 0U, (u32)(1 << 31), (u32)1024* 1024);

	bank.AddToggle("Extra Memory Cool Down Timer", &sm_bExtraCooldownTimer);	
	bank.AddSlider("Cool Down Delay Time", &sm_uCooldownDelayMS, 0U, 1U << 31U, 1000U);
	bank.AddSlider("Extra Update Time", &sm_uExtraUpdateTime, 0U, 1U << 31U, 1000U);
	bank.AddSlider("Memory Reduction Rate", &sm_fMemoryReductionRate, 0.0f, 1.0f, 0.0001f);

	bank.AddSlider("Total Active Video Count", &m_auTotalActiveItems[MT_Video][RT_Totals], 0U, (u32)(1<<31), (u32)(1024 * 1024));
#if REUSE_RESOURCE
	bank.AddSlider("Total Cached Video Count", &m_auTotalCacheItems[MT_Video][RT_Totals], 0U, (u32)(1<<31), (u32)(1024 * 1024));
#endif // REUSE_RESOURCE
	bank.AddSlider("New", &m_auNewAddItems[MT_Video][RT_Totals], 0U, (u32)(1<<31), (u32)(1024 * 1024));
#if REUSE_RESOURCE
	bank.AddSlider("Cached", &m_auNewCacheItems[MT_Video][RT_Totals], 0U, (u32)(1<<31), (u32)(1024 * 1024));
	bank.AddSlider("Cleared Cache", &m_auNewClearCacheItems[MT_Video][RT_Totals], 0U, (u32)(1<<31), (u32)(1024 * 1024));
	bank.AddSlider("Reused Cached", &m_auNewUsedCacheItems[MT_Video][RT_Totals], 0U, (u32)(1<<31), (u32)(1024 * 1024));
#endif // REUSE_RESOURCE

#if CONTROL_RATE_OF_CREATIONS
	bank.AddSlider("Data Creation Rate (Per Second)", &m_fDataCreationRate, 1.0f * 1024.0f * 1024.0f, 16.0f * 1024.0f * 1024.0f * 1024, 1024.0f * 1024.0f);
	bank.AddSlider("Creation Rate", &m_fCreateRate, 1.0f, 1024.0f * 1024.0f * 1024.0f, 32.0f);
	bank.AddSlider("Fall off from previous frame", &m_fFalloffFromPreviousFrame, 0.0f, 1.0f, 0.01f);
	bank.AddSlider("Max Wait Time", &m_fMaxWaitTime, 0.0f, 1.0f, 0.001f);

	bank.AddSlider("Current Data Rate", &m_fCurrentDataCreateRate, 0.0f, 1024.0f * 1024.0f * 1024.0f, 0.0f);
	bank.AddSlider("Current Create Rate", &m_fCurrentCreateRate, 0.0f, 1024.0f * 1024.0f * 1024.0f, 0.0f);
#endif

	bank.AddSlider("Waste Video Memory", (u32*)&sm_iWasteVideoMemory, 0U, (u32)(0xFFFFFFFF), (u32)(1024 * 1024), datCallback(CFA(WasteVideoMemory)));
	bank.AddSlider("Waste System Memory", (u32*)&sm_iWasteSystemMemory, 0U, (u32)(0xFFFFFFFF), (u32)(1024 * 1024), datCallback(CFA(WasteSystemMemory)));

	bank.AddToggle("Force Failures", &m_bForceFailures);

	bank.PushGroup("Memory Restrictions", true);
	for (u32 uIndex = 0; uIndex < DM_LAST; uIndex++)
	{
		bank.PushGroup(astrManufacturer[uIndex], true);
		for (u32 uTexQual = 0; uTexQual < TQ_LAST; uTexQual++)
		{
			bank.AddSlider(astrTextureLevel[uTexQual], (u32*)&aiTextureMaxMem[sm_eOS][uIndex][uTexQual], 1000000U,3000U * 1024U * 1024U, 1000000U);
		}
		bank.PopGroup();
	}
	bank.PopGroup();
}

void grcResourceCache::WasteSystemMemory()
{
	void* ptr = sysMemHeapAllocate((size_t)sm_iWasteSystemMemory);
	if (ptr == NULL)
	{
		Warningf("Failed to allocate wasted system memory");
	}
	else
	{
		Displayf("Wasting %d of Video memory", sm_iWasteSystemMemory);
	}
}

void grcResourceCache::WasteVideoMemory()
{
	HRESULT uReturnCode = S_OK;

#if (!__D3D11)
	if (!__D3D11)
	{
		IDirect3DVertexBuffer9* m_D3DBuffer;
		uReturnCode = GRCDEVICE.GetCurrent()->CreateVertexBuffer((UINT)sm_iWasteVideoMemory, D3DUSAGE_WRITEONLY, 0, (D3DPOOL)RESOURCE_UNMANAGED, &m_D3DBuffer, 0);
	}
	else
#endif // (!__D3D11)
	{
#if __D3D11
		D3D11_BUFFER_DESC oDesc = {0};
		oDesc.ByteWidth = (UINT)sm_iWasteVideoMemory;
		oDesc.BindFlags = grcBindVertexBuffer;					
		oDesc.MiscFlags = 0;
		oDesc.CPUAccessFlags = grcCPUNoAccess; 
		oDesc.Usage = static_cast<D3D11_USAGE>(grcUsageDefault);
		oDesc.StructureByteStride = 0;

		ID3D11Buffer* m_D3DBuffer;
		uReturnCode = GRCDEVICE.GetCurrent()->CreateBuffer(&oDesc, NULL, (ID3D11Buffer**)&m_D3DBuffer);
#endif // __D3D11
	}

	if (uReturnCode == S_OK)
	{
		Displayf("Wasting %u of Video memory", sm_iWasteVideoMemory);
	}
	else
	{
		Warningf("Failed to allocate wasted video memory");
	}
}

#endif // __BANK

#endif // USE_RESOURCE_CACHE

