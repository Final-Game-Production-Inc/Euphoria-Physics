//
// grcore/effect.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_EFFECT_H 
#define GRCORE_EFFECT_H 

#include    <map>

#include "atl/array.h"
#include "data/struct.h"
#include "grcore/config.h"
#include "grcore/effect_typedefs.h"
#include "grcore/stateblock.h"
#include "string/string.h"
#include "grcore/array.h"
#include "grcore/fvf.h"
#include "system/container.h"
#include "system/tls.h"
#include "atl/string.h"
#include "paging/handle.h"
#include "paging/ref.h"
#include "atl/hashstring.h"
#include "grcore/effect_mrt_config.h"

#define THREADED_SHADER_LOAD (0 && __D3D11)

#define EFFECT_USE_CONSTANTBUFFERS			(__D3D11 || RSG_ORBIS)
#define EFFECT_CACHE_PROGRAMRESOURCES		( EFFECT_USE_CONSTANTBUFFERS && (RSG_PC && __D3D11) )
#define EFFECT_OFFSETCB_ALIGNMENT			( 64 )
#define EFFECT_BIND_UNORDERED_RESOURCES		(SUPPORT_UAV && RSG_ORBIS)
#define EFFECT_TEXTURE_SUBTYPING			(__D3D11 || RSG_ORBIS)
#define EFFECT_CACHE_PROGRAM				(1 && !__RESOURCECOMPILER)
#define EFFECT_CACHE_LOCALCBS				(1 && !__RESOURCECOMPILER)
#define EFFECT_STENCIL_REF_MASK				(1 && RSG_PC)

#define EFFECT_TRACK_INSTANCING_ERRORS		( 0 && RSG_ORBIS && !__FINAL )
#define EFFECT_TRACK_MSAA_ERRORS			(EFFECT_TEXTURE_SUBTYPING && RSG_PC && __ASSERT)
// enable manually to catch the CMP mismatches versus regular samplers
#define EFFECT_TRACK_COMPARISON_ERRORS		(0 && EFFECT_TEXTURE_SUBTYPING && __ASSERT)
#define EFFECT_TRACK_TEXTURE_ERRORS			(EFFECT_TRACK_MSAA_ERRORS || EFFECT_TRACK_COMPARISON_ERRORS)
#define EFFECT_TRACK_GLOBALS				(1 && __ASSERT)
// make sure that the type of the resource bound to a variable matches shader's expectations
#define EFFECT_CHECK_ENTRY_TYPES			(0 && __ASSERT)


#if RSG_DURANGO
#define FIXED_PLACEMENT_BASE                (256)
#endif

#if __PS3
#include "grcorespu.h"
#endif

#include "grcore/effect_config.h"		// TEMPORARY, until USE_PACKED_GCMTEX is on by default

#if __SPU
#include "system/dma.h"
#else
#include "paging/dictionary.h"
#endif

#define EFFECT_PRESERVE_STRINGS		((__PPU && __PROFILE) || __BANK || __WIN32PC || RSG_DURANGO || RSG_ORBIS)
#define EFFECT_PRESERVE_STRINGS_DCC (EFFECT_PRESERVE_STRINGS && __TOOL)

// Enable complimentary GPU Debuffer on Orbis
#define ENABLE_GPU_DEBUGGER			(1 && RSG_ORBIS && EFFECT_PRESERVE_STRINGS && __DEV && SCE_ORBIS_SDK_VERSION >= 0x01600071u)
#define GPU_DEBUGGER_MANUAL_LOAD	(ENABLE_GPU_DEBUGGER && SCE_ORBIS_SDK_VERSION < 0x01700602u)
//#define GPU_DEBUGGER_MANUAL_LOAD	(ENABLE_GPU_DEBUGGER && SCE_ORBIS_SDK_VERSION < 0x01700000u)

#define TRACK_CONSTANT_BUFFER_CHANGES (__BANK && RSG_PC && __D3D11)
#if TRACK_CONSTANT_BUFFER_CHANGES
#	define TRACK_CONSTANT_BUFFER_CHANGES_ONLY(...)   __VA_ARGS__
#else
#	define TRACK_CONSTANT_BUFFER_CHANGES_ONLY(...)
#endif

#if EFFECT_TRACK_TEXTURE_ERRORS
#	define EFFECT_TRACK_ERRORS_ONLY(...)	__VA_ARGS__
#else
#	define EFFECT_TRACK_ERRORS_ONLY(...)
#endif

#if EFFECT_TRACK_GLOBALS
#	define EFFECT_TRACK_GLOBALS_ONLY(...)	__VA_ARGS__
#else
#	define EFFECT_TRACK_GLOBALS_ONLY(...)
#endif

#define CHECK_GPU_DATA	(__DEV && (RSG_PC || RSG_DURANGO || RSG_ORBIS))

#define TRAP_UNMARKED_TEXTURES    (__BANK && __PS3)
#define BUCKETMASK_MODELMATCH(model,passed) (((((passed) >> 8) & 0xFF) & (model)) == 0)

#define BUCKETMASK_GENERATE(baseBucket) ((0xff << 8) | ((1U << (baseBucket)) & 0xff))
#define BUCKETMASK_GENERATE_WITH_SUB(baseBucket, subBucketMask)  (((subBucketMask) << 8) & 0xff00) | ((1U << (baseBucket)) & 0xff)

// Kept so we don`t break shader hair sort. Should probably be BUCKETMASK_MATCH_SHADER.
#define BUCKETMASK_MATCH(shader,passed) ((((shader) & (passed)) >> 8) && (((shader) & (passed))&0xff))
#define BUCKETMASK_MATCH_LOD(shader,passed) ((((shader) & (passed)) >> 8) && (((shader) & (passed))&0xff))
#define BUCKETMASK_MATCH_SHADER(shader,passed) ((((shader) & (passed)) >> 8) && (((shader) & (passed))&0xff))

#define BUCKETMASK_TESSELLATION_BIT (0x1 << 15)
#define BUCKETMASK_CLEAR_TESSELLATION_BIT(mask) ((mask) & ~BUCKETMASK_TESSELLATION_BIT)
#define BUCKETMASK_SET_TESSELLATION_BIT(mask) ((mask) | BUCKETMASK_TESSELLATION_BIT)

#define GRC_CBUFFER_ALIGN(x, y)	(((x) + (y - 1)) & ~(y - 1))

#define SPECIAL_GLOBAL_TYPE_MASK			0x0f00
#define SPECIAL_GLOBAL_INDEX_MASK			0x00ff
#define SPECIAL_GLOBAL_VT_TEXTURE			0x0100
#define SPECIAL_GLOBAL_VT_STRUCTUREDBUFFER	0x0200
#define SPECIAL_GLOBAL_SUBTYPE_SHIFT		12
#define SPECIAL_GLOBAL_SUBTYPE_MASK			(0x3<<SPECIAL_GLOBAL_SUBTYPE_SHIFT)
// Temporary sampler bit for the m_GlobalSubTypes, helps to avoid ambiguety
// of the texture/sampler sub-type when connecting global parameters
#define SPECIAL_TEMP_SUBTYPE_SAMPLER_BIT	(0x08)


#if __WIN32PC
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;
struct grcCommandBuffer;
#elif __XENON
struct D3DVertexShader;
struct D3DPixelShader;
struct D3DConstantBuffer;
namespace rage {
struct grcVertexDeclaration;
}
struct D3DCommandBuffer;
#elif __PS3 || __PSP2
struct _CGprogram;
struct grcCommandBuffer;
#endif

#if __OPTIMIZED
#define FXFORCEINLINE __forceinline
#elif __PS3
#define FXFORCEINLINE inline
#else
#define FXFORCEINLINE
#endif

#if __PS3
#include <cell/cgb/cgb_struct.h>
#elif __RESOURCECOMPILER
typedef struct CellCgbVertexProgramConfiguration{
	unsigned short instructionSlot;
	unsigned short instructionCount;
	unsigned short attributeInputMask;
	unsigned char registerCount;
	unsigned char clipMask;		// R* Extension!
}CellCgbVertexProgramConfiguration;
typedef struct CellCgbFragmentProgramConfiguration{
	unsigned offset;
	unsigned attributeInputMask;
	unsigned short texCoordsInputMask;
	unsigned short texCoords2D;
	unsigned short texCoordsCentroid;
	unsigned fragmentControl;
	unsigned char registerCount;
}CellCgbFragmentProgramConfiguration;
#endif

#if __RESOURCECOMPILER
#define grcBuffer void
#define grcComputeShader void
#define grcDeviceSurface void
#define grcDeviceTexture void
#define grcDeviceView void
#define grcDomainShader void
#define grcGeometryShader void
#define grcHullShader void
#define grcPixelShader void
#define grcResource void
#define grcShader void
#define grcStreamOutputDeclaration void
#define grcTextureObject void
#define grcVertexShader void
struct ID3D11InputLayout;
#elif __XENON
struct D3DBaseTexture;
#define grcTextureObject D3DBaseTexture
typedef struct D3DPixelShader grcPixelShader;
typedef struct D3DVertexShader grcVertexShader;
typedef void *grcBuffer;
#elif __D3D11
// Abstract wrapper for a Direct3D device
struct ID3D11Buffer;
struct ID3D11ComputeShader;
struct ID3D11DeviceChild;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11HullShader;
struct ID3D11PixelShader;
struct ID3D11RenderTargetView;
struct ID3D11Resource;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11VertexShader;
struct IUnknown;

typedef ID3D11Buffer			    grcBuffer;
typedef ID3D11ComputeShader			grcComputeShader;
typedef ID3D11DeviceChild		    grcShader;
typedef ID3D11DomainShader			grcDomainShader;
typedef ID3D11GeometryShader		grcGeometryShader;
typedef ID3D11HullShader			grcHullShader;
typedef ID3D11PixelShader		    grcPixelShader;
typedef ID3D11Resource				grcDeviceTexture;
typedef ID3D11Resource			    grcTextureObject;
typedef ID3D11ShaderResourceView	grcDeviceView;
typedef ID3D11VertexShader		    grcVertexShader;
typedef IUnknown					grcDeviceSurface;
typedef IUnknown					grcResource;
typedef IUnknown				    grcBlob;
typedef void						grcStreamOutputDeclaration;

struct ID3D11InputLayout;
struct ID3D11Buffer;
#elif __WIN32PC
// Abstract wrapper for a Direct3D device
struct IUnknown;

typedef IUnknown grcBlob;
typedef IUnknown grcBuffer;
typedef IUnknown grcComputeShader;
typedef IUnknown grcDeviceSurface;
typedef IUnknown grcDeviceTexture;
typedef IUnknown grcDeviceView;
typedef IUnknown grcDomainShader;
typedef IUnknown grcGeometryShader;
typedef IUnknown grcHullShader;
typedef IUnknown grcPixelShader;
typedef IUnknown grcResource;
typedef IUnknown grcShader;
typedef IUnknown grcTextureObject;
typedef IUnknown grcVertexShader;
typedef void	 grcStreamOutputDeclaration;

struct ID3D11InputLayout;
struct ID3D11Buffer;
#elif __PS3
struct CellGcmTexture;
struct PackedCellGcmTexture;
#if __PPU || !USE_PACKED_GCMTEX
#define grcTextureObject CellGcmTexture
#else
#define grcTextureObject PackedCellGcmTexture
#endif
typedef void *grcBuffer;
#elif __PSP2
struct SceGxmTexture;
struct SceGxmProgram;
#define grcTextureObject SceGxmTexture
#elif RSG_ORBIS
#include <gnm/buffer.h>
namespace sce {
	namespace Gnm { class Texture; }
	namespace Gnmx { class LsShader; class VsShader; class PsShader; class EsShader; class CsShader; class GsShader; class HsShader; }
	namespace LCUE { struct ShaderResourceOffsets; }
}
typedef sce::Gnm::Texture   grcTextureObject;
typedef sce::Gnm::Buffer    grcBuffer;
typedef sce::Gnmx::PsShader grcPixelShader;
typedef sce::Gnmx::VsShader grcVertexShader;
typedef sce::Gnmx::CsShader grcComputeShader;
typedef sce::Gnmx::LsShader grcLocalShader;
typedef sce::Gnmx::HsShader grcHullShader;
typedef sce::Gnmx::GsShader grcGeometryShader;
typedef sce::Gnmx::EsShader grcExportShader;
typedef void grcShader;
typedef void grcDeviceView;
struct grcCommandBuffer;
#else
typedef void grcShader;
typedef void grcDeviceView;
#endif

#if RSG_DURANGO
namespace rage { class grcGfxContextD3dWrapper;  }
namespace rage { class grcGfxContextCommandList; }
typedef struct ID3D11DeviceX					grcDeviceHandle;
typedef class  rage::grcGfxContextD3dWrapper	grcContextHandle;
typedef class  rage::grcGfxContextCommandList	grcContextCommandList;
struct ID3D11DmaEngineContextX;
#elif __D3D11_1
typedef struct ID3D11Device1					grcDeviceHandle;
typedef struct ID3D11DeviceContext1				grcContextHandle;
typedef struct ID3D11CommandList				grcContextCommandList;
#elif __D3D11
typedef struct ID3D11Device						grcDeviceHandle;
typedef struct ID3D11DeviceContext				grcContextHandle;
typedef struct ID3D11CommandList				grcContextCommandList;
#elif __D3D9
typedef struct IDirect3DDevice9					grcDeviceHandle;
typedef struct IDirect3DDevice9					grcContextHandle;
#endif // __D3D11_1

namespace rage {

#if MULTIPLE_RENDER_THREADS
extern __THREAD u8 g_RenderThreadIndex;
extern __THREAD bool g_IsSubRenderThread;
#else
// Use enums here (rather than #defines, etc) so that these are compile time
// constants, and are part of the rage namespace.
enum { g_RenderThreadIndex = 0 };
enum { g_IsSubRenderThread = false };
#endif

#if __D3D11
static const int s_MissingInputsVertexBufferStream = 4;
#endif

#if __BANK
class bkBank;
class bkText;
#endif
class datResource;
class grcTexture;
struct grcVertexDeclaration;
class Vector2;
class Vector3;
class Vector4;
class Vec2f;
class Vec2V;
class Vec3V;
class Vec4V;
class Vec4f;
struct Matrix43;
class Matrix34;
class Matrix44;
class Mat34V;
class Mat44V;
class fiStream;
class fiTokenizer;
class Color32;

#if __SPU
typedef const char *grcString;
#else
typedef ConstString grcString;
#endif

typedef int IntVector[4];

#define SUPPORT_UAV	(__D3D11 || RSG_ORBIS)
#if SUPPORT_UAV
#if RSG_PC &&__D3D11
class grcBufferD3D11;
class grcBufferD3D11Resource;
class grcRenderTargetDX11;
typedef grcBufferD3D11			grcBufferBasic;
typedef grcBufferD3D11Resource	grcBufferUAV;
typedef grcRenderTargetDX11		grcTextureUAV;
#elif RSG_DURANGO
class grcBufferDurango;
class grcBufferResourceDurango;
class grcRenderTargetDurango;
typedef grcBufferDurango			grcBufferBasic;
typedef grcBufferResourceDurango	grcBufferUAV;
typedef grcRenderTargetDurango		grcTextureUAV;
#elif RSG_ORBIS
class grcBufferGNM;
class grcRenderTargetGNM;
typedef grcBufferGNM			grcBufferBasic;
typedef grcBufferGNM			grcBufferUAV;
typedef grcRenderTargetGNM		grcTextureUAV;
#endif
#endif	//SUPPORT_UAV

#if EFFECT_CACHE_PROGRAM
struct ShaderCacheStat {
	u32 Hits, HitBytes, Misses, MissBytes, MissMicrocodeBytes, MissSymbolBytes;
};
#endif // EFFECT_CACHE_PROGRAM

#if EFFECT_CACHE_PROGRAM || EFFECT_CACHE_LOCALCBS
template<typename T>
struct CacheAllocator : public std::allocator<T> {
	CacheAllocator() {}
	template<class Other> CacheAllocator( const CacheAllocator<Other>& /*_Right*/ ) {}

	inline typename std::allocator<T>::pointer allocate(typename std::allocator<T>::size_type n, typename std::allocator<void>::const_pointer = 0) 
	{	
		return reinterpret_cast<typename std::allocator<T>::pointer>( ::operator rage_heap_new(n * sizeof(T))  ); 
	}

	inline void deallocate(typename std::allocator<T>::pointer p, typename std::allocator<T>::size_type ) {
		::operator delete(p);
	}

	template<typename U>
	struct rebind {
		typedef CacheAllocator<U> other;
	};
};
#endif // EFFECT_CACHE_PROGRAM || EFFECT_CACHE_LOCALCBS

struct grcTextureHandleUnionable: public pgHandleUnionable<grcTexture> {
#if !__SPU
	// Assignment doesn't inherit, must respecify
	void PointerFixup(datResource&);
	void operator=(grcTexture* that)
#if ENABLE_DEFRAGMENTATION
	{ grcTexture **oldPtr=ptr; ptr = (grcTexture**)Register((pgBase*)that); IncRef(); DecRef(oldPtr); }
#else
	{ ptr = that; }
#endif
#endif
};

#define GRCTEXTUREHANDLE_IS_PGREF	(!ENABLE_DEFRAGMENTATION)	// Should be safe to turn this on for anything other than PS3

#if GRCTEXTUREHANDLE_IS_PGREF
typedef pgRef<grcTexture> grcTextureHandle;
typedef pgRef<grcTexture> grcTextureIndex;
#else
struct grcTextureHandle: public grcTextureHandleUnionable {
#if !__SPU && ENABLE_DEFRAGMENTATION
#	if PGHANDLE_REF_COUNT
		grcTextureHandle() { ptr=NULL; }
		grcTextureHandle(const grcTextureHandle& that) { ptr=that.ptr; IncRef(); }
		grcTextureHandle& operator=(const grcTextureHandle& that) { ptr=that.ptr; IncRef(); that.DecRef(); return *this; }
		~grcTextureHandle() { DecRef(); ptr=NULL; }
#	endif
	void operator=(grcTexture* that) { grcTextureHandleUnionable::operator=(that); }
#endif
};

#if !__SPU
typedef pgHandleIndex<grcTexture> grcTextureIndex;
#endif
#endif	// __64BIT

#if USE_PACKED_GCMTEX && !__SPU
struct grcTextureIndex32 {
	grcTexture& operator*() const { return *(grcTexture*)g_pgHandleArray[index]; }
	grcTexture* operator->() const { return (grcTexture*)g_pgHandleArray[index]; }
	operator grcTexture*() const { return (grcTexture*)g_pgHandleArray[index]; }
	void operator=(grcTexture* that) { index = pgHandleBase::RegisterIndex((pgBase*)that); }
	u32 index;
	void PointerFixup(datResource&);
};
#endif

#if EFFECT_TEXTURE_SUBTYPING
// A sub-typing enum for textures, helps us to manage associated samplers
// and assert on incompatible bindings on rendering
enum TextureSubtype
{
	TEXTURE_SAMPLER,// texture IS a sampler (TODO: use VT_SAMPLERSTATE)
	TEXTURE_REGULAR,// texture with an associated sampler
	TEXTURE_PURE,	// texture with no sampler
	TEXTURE_MSAA,	// MSAA texture (with no sampler)
};

enum
{
	TEXTURE_REGISTER_BITS	= 6,
	TEXTURE_USAGE_BITS		= 7,
};
#endif // EFFECT_TEXTURE_SUBTYPING

// Maps VT_... into element size, in words.
extern u8 g_FloatSizeByType[];
// Maps VT_... into element size, in quadwords.
extern u8 g_Float4SizeByType[];

enum {
	USAGE_VERTEXPROGRAM		= 1,
	USAGE_FRAGMENTPROGRAM	= 2,
	USAGE_GEOMETRYPROGRAM	= 4,		// DX10
	USAGE_HULLPROGRAM		= 8,		// DX11
	USAGE_DOMAINPROGRAM		= 16,		// DX11
	USAGE_COMPUTEPROGRAM	= 32,		// DX11

	USAGE_ANYPROGRAM		= 63,
	USAGE_MATERIAL			= 64,		// Parameter is per-material, not per-instance
	USAGE_COMPARISON		= 128,		// Parameter is sampler with comparison filter
};

enum ShaderStage { // Match with gnmx kInvalidShader
	ssVertexStage,
	ssPixelStage,
	ssComputeStage,
	ssGeometryStage,
	ssHullStage,
	ssDomainStage,
	ssExportStage,
	ssCount
};

class  grcEffect;
struct grcEffect_Technique_Pass;
class  grcInstanceData;
class  grcFragmentProgram;
class  grcParameter;
class  grcVertexProgram;
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
class grcComputeProgram;
class grcHullProgram;
#endif

enum ShaderType
{
	VS_TYPE = 0,
	PS_TYPE,
	CS_TYPE,
	DS_TYPE,
	GS_TYPE,
	HS_TYPE,
	NUM_TYPES,
	NONE_TYPE = NUM_TYPES
};

struct grcBufferInfo
{
	u8 Dirty;
	bool bLocked;
	grcBuffer *Buffer;
	char *pbySysMemBuf;
	void *pvLockPtr;
#if __DEV
	int threadId;
#endif
};

#define GRC_CBUFFER_CACHE_LINE_SIZE 64
#define GRC_BUFFER_ELEMENT_ALIGN 16

class grcCBuffer
{
public:
	grcCBuffer();
	grcCBuffer(u32 size, bool directLockOnly = false);
	~grcCBuffer();
	void operator=(const grcCBuffer &rhs);

	static grcCBuffer* LoadConstantBuffer(fiStream &S);
#if EFFECT_CACHE_LOCALCBS
	static u32 ComputeFingerprint(grcCBuffer& pSource);
#endif // EFFECT_CACHE_LOCALCBS
	void Load(fiStream &S);
	void Init(bool someBool = false);
#if RSG_PC && __D3D11
	void Init_Internal(bool directLockOnly);
#endif
	void Destroy();
	const char *GetName() const { return Name; }
	u32 GetNameHash() const { return NameHash; }
	u32 GetRegister(int unit) const	{ return Registers[unit]; }
	const u16 *GetRegisters() const { return Registers; }
#if RSG_DURANGO || RSG_ORBIS
	void SetBackingStoreThread(void *backingStore, u32 threadIdx=g_RenderThreadIndex);
	void SetBackingStore(void *backingStore);
	void SetDirtySingleThread();
	void SetDirtyAllThreads();
	void SetDirty();
	bool GetDirty();
	void ResetDirty();
	bool GetIsExplicitBind() const { return ExplicitBind; }
	void SetIsExplicitBind(bool explicitBind) { ExplicitBind = explicitBind; }
	u32 GetSize() const { return CurrSize[g_RenderThreadIndex]; }
	u32 GetMaxSize() const { return MaxSize; }
#if RSG_DURANGO
	void* GetBuffer() { return Buffer[g_RenderThreadIndex]; }
	void SetBuffer(void *buf) { Buffer[g_RenderThreadIndex] = buf; }
#elif RSG_ORBIS
	grcBuffer* GetBuffer() { return &Buffer[g_RenderThreadIndex]; }
#endif
#else
	void SetDirty(u8 uFlag);	//{ Dirty |= uFlag; }
	u8 GetDirty();			//{ return Dirty; }
	void ResetDirty();		//{ Dirty = 0; }
	u32 GetSize() const { return Size; }
	grcBuffer* GetBuffer(DEV_ONLY(u8 lockflag = 2));
#endif
	bool IsLocked(u32 threadIdx=g_RenderThreadIndex)	const;
#if MULTIPLE_RENDER_THREADS
	void* GetDataPtrForThread(u32);
	__forceinline void* GetDataPtr() { return GetDataPtrForThread(g_RenderThreadIndex); }
#else
	void* GetDataPtr();
#endif

#if RSG_PC && __D3D11
	u32 GetBackingStoreSize() { return m_BufferStride*NUMBER_OF_RENDER_THREADS; }
#elif RSG_DURANGO || RSG_ORBIS
	u32 GetBackingStoreSize() { return MaxSize*NUMBER_OF_RENDER_THREADS; }
#else
	u32 GetBackingStoreSize() { return Size*NUMBER_OF_RENDER_THREADS; }
#endif

	bool Unlock();

#if RSG_PC && __D3D11
	void* GetDataPtr_ReadOnly();
	void *LockDirect(u32 numBytes);
	void UnlockDirect();
#endif

	#if TRACK_CONSTANT_BUFFER_CHANGES
	void ResetLockCount() { m_LockCount = 0; }
	u32 GetLockCount() { return m_LockCount; }
	#endif

#if RSG_PC && __D3D11
	void *BeginUpdate(u32 sizeBytes)  { return LockDirect(sizeBytes); }
	void EndUpdate() { UnlockDirect(); }

	// LDS DMC TEMP:-
	void PrintData();
#else
	// Returns pointer to raw constant buffer storage as seen by the final device
	// WARNING: Data becomes invalid as soon as current alloc scope is closed.
	void *BeginUpdate(u32 sizeBytes);
	void EndUpdate();

	// Data is copied from 'data' into temporary memory.  Copy is performed when
	// grcCBuffer is first bound to a shader stage.  The temporary memory copy
	// will then be reused if the alloc scope that was open when the data was
	// copied is still valid, otherwise new temporary memory will be allocated,
	// and 'data' will be copied again.
	//
	// This means that 'data' should point to valid memory until all the command
	// buffer for all drawcalls that use it have been created.  'data' could be
	// destroyed earlier if it can be garunteed that the alloc scope is not
	// closed, but generally this is fragile and should be avoided.
	//
	void SetDataIndirect(const void *data, u32 sizeBytes);

	// GPU access 'data' directly.  Contents of data must stay valid until GPU
	// is guarunteed to have finished accessing it.
	void SetDataDirect(const void *data, u32 sizeBytes);
#endif
	template <class _Type> _Type* BeginTypedUpdate() { return (_Type*) BeginUpdate((u32)sizeof(_Type)); }

	void SetRegisters(u16 registers[NONE_TYPE]) { for(int i=0; i<NONE_TYPE; i++) { Registers[i] = registers[i]; } }

private:
	bool Lock();
	int GetDataIndex() const;

#if RSG_DURANGO || RSG_ORBIS
	u32 MaxSize;
	u32	CurrSize[NUMBER_OF_RENDER_THREADS];
#else
	u32	Size;		// total size of cbuf
#endif
	u16 Registers[NONE_TYPE];	// slot #, per functional unit
	u32 NameHash;	// atStringHash(m_Name)

	grcString Name;	// actual name of cbuf struct

#if RSG_PC && __D3D11
	#define GRC_CBUFFER_DIRTY_FLAG		0
	#define GRC_CBUFFER_LOCKED_DIRECT	1
	#define GRC_CBUFFER_FIRST_USE		2
	typedef struct BUFFER_HEADER
	{
		// Last deferred context this buffer was used in.
		u32 m_LastDeferredContextCount;
		char m_Flags[3];
	} BUFFER_HEADER;

	u8 GetFlagThreadIdx(u32 threadIdx, int flag) const;
	void SetFlagThreadIdx(u32 threadIdx, int flag);
	void ResetFlagThreadIdx(u32 threadIdx, int flag);

	void* GetDataPtrForThread_Write(u32);
	void* GetDataPtrForThread_ReadOnly(u32);
	bool SetParameter_ThreadIdx(int threadIdx, const float *data, int count, int offset, int size);
	bool SetParameter(const float *data, int count, int offset, int size);

	void CalculateBufferStride(bool directLockOnly);
	void AllocateBackingStore();
	void FreeBackingStore();

	BUFFER_HEADER *GetBufferHeader(u32 threadIdx);
	BUFFER_HEADER *GetBufferHeader(u32 threadIdx) const;

	static void OnBeginDeferredContext();
	static void OnEndDeferredContext();
	bool DoesUnlockNeedToBeCalled();
	void UpdateD3DBuffer();

#if !(MULTIPLE_RENDER_THREADS > 1)
	void* GetDataPtrForThread(u32);
#endif

	grcBuffer* m_pBuffer;

	char *m_pAlignedBackingStore;
	char *m_pAllocatedBackingStore;
	u32 m_BufferStride;
	// Count of the current deferred context.
	static u32 DECLARE_MTR_THREAD sm_DeferredContextCount;

	friend class grcProgram;
	friend class grcDevice;
# if TRACK_CONSTANT_BUFFER_CHANGES
	u32 m_LockCount;
# endif
#elif RSG_DURANGO
	void *Buffer[NUMBER_OF_RENDER_THREADS];
	char *BackingStore[NUMBER_OF_RENDER_THREADS];
	u32 AllocScopeId[NUMBER_OF_RENDER_THREADS];
	bool ExplicitBind; // because the code is a rat's nest, and skinning matrices are tied to geometry via skinning palette, and .: cannot use the standard bind code path
#elif RSG_ORBIS
	grcBuffer Buffer[NUMBER_OF_RENDER_THREADS];
	char *BackingStore[NUMBER_OF_RENDER_THREADS];
	u32 AllocScopeId[NUMBER_OF_RENDER_THREADS];
	bool ExplicitBind;
#else
	grcBuffer *Data;
	char *m_pbySysMemBuf;
	u8 Dirty;		// 0 = not updated; 0x01 = vs update; 0x10 = ps update; 0x11 = vs/ps udpate
	bool  m_bLocked;
	void *m_pvLockPtr;
#endif

#if EFFECT_CACHE_LOCALCBS
	typedef std::map< u32, grcCBuffer*, std::less<u32>, CacheAllocator<std::pair<u32, grcCBuffer*>> > CBufCacheMap;
	static CBufCacheMap sm_CBuffer;
#endif // EFFECT_CACHE_LOCALCBS
};

#if RSG_SM_50
extern grcCBuffer *g_SkinningBase, *g_MatrixBase;
#if RAGE_INSTANCED_TECH
extern grcCBuffer *g_InstBase, *g_InstUpdateBase;
#define INST_CBUFFER			g_InstBase;
#define INSTUPDATE_CBUFFER		g_InstUpdateBase;
#endif
#define SKINNING_CBUFFER		g_SkinningBase
#define SKINNING_CBUFFER_TYPE	grcCBuffer*
#define MATRIX_CBUFFER			g_MatrixBase
#define MATRIX_CBUFFER_TYPE		grcCBuffer*
#else
#define SKINNING_CBUFFER		SKINNING_BASE
#define SKINNING_CBUFFER_TYPE	int
#define MATRIX_CBUFFER			MATRIX_BASE
#define MATRIX_CBUFFER_TYPE		int
#endif

#if RSG_ORBIS
#define EFFECT_CONSTANT_BUFFER_COUNT	( sce::Gnm::kSlotCountConstantBuffer - 1 )
#define EFFECT_UAV_BUFFER_COUNT			( sce::Gnm::kSlotCountRwResource )
#define EFFECT_SAMPLER_COUNT			( 16 )
#define EFFECT_TEXTURE_COUNT			( 42 )
#elif RSG_DURANGO
#define EFFECT_CONSTANT_BUFFER_COUNT	( 14 ) //D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT
#define EFFECT_UAV_BUFFER_COUNT			( 16 ) //( 64 ) //D3D11_1_UAV_SLOT_COUNT	//If too large, you get: Fatal Error: <RenderThread> sysMemContainer(xxx) overflowed, give larger number to Init call...
#define EFFECT_SAMPLER_COUNT			( 16 ) //D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT
#define EFFECT_TEXTURE_COUNT			( 42 )
#elif __D3D11 || RSG_PC
#define EFFECT_CONSTANT_BUFFER_COUNT	( 14 )
#define EFFECT_UAV_BUFFER_COUNT			( 4 )
#define EFFECT_SAMPLER_COUNT			( 16 )
#define EFFECT_TEXTURE_COUNT			( 42 )
#else
#define EFFECT_CONSTANT_BUFFER_COUNT	( 1 )
#define EFFECT_UAV_BUFFER_COUNT			( 0 )
#define EFFECT_SAMPLER_COUNT			( 16 )
#define EFFECT_TEXTURE_COUNT			( 16 )
#endif // platforms

/*
	grcProgram is an abstraction of a vertex shader or pixel shader (vertex program or fragment program).
	It contains a constant table which is used to map effect parameters to actual shader parameters.
*/
class grcProgram {
	friend class grcEffect;
	friend class grcInstanceData;
public:
	static const int c_MaxGlobals = (__XENON || __PPU) ? 196 : 256;
#if EFFECT_TRACK_TEXTURE_ERRORS
	typedef u8 GlobalSubTypesArray[c_MaxGlobals];
#endif
#if EFFECT_TRACK_GLOBALS
	typedef u8 RegisterToGlobalTable[EFFECT_TEXTURE_COUNT];
#endif

	grcProgram();
#if __WIN32
	virtual ~grcProgram();
#else
	~grcProgram();
#endif

#if __SPU
	grcProgram(datResource &rsc) : m_Constants(rsc) { }
	IMPLEMENT_PLACE_INLINE(grcProgram)
#endif

#if EFFECT_PRESERVE_STRINGS
	const char *GetEntryName() const { return m_EntryName; }
#endif

	void Connect(const atArray<grcParameter> &locals EFFECT_TRACK_ERRORS_ONLY(,const GlobalSubTypesArray &globalSubTypes));

// Note, several of these tests should really be against __D3D11, but __RESOURCECOMPILER complicates things
#if EFFECT_USE_CONSTANTBUFFERS
	void ConnectCBuffer(atArray<grcParameter> &locals,ShaderType type);
	void SetTextureResourcesUsingVectorDXAPICall(ShaderType eType) const;
#if RSG_PC
	bool SetConstantBufferData(ShaderType eType) const;
	template<class T> void SetConstantBuffer() const;
	template<class T> void SetConstantBuffer(grcCBuffer **ppOverrides, u32 overrideOffset, u32 numOverrides) const;
	static bool SetLocalParameterInConstantBuffer(float *data, int count, u32 offset, grcCBuffer *pConstBuffer, u8 type);
#else
	static void SetLocalParameterInConstantBuffer(float *data, int count, u32 offset, grcCBuffer *pConstBuffer, u8 type);
#endif
#if __ASSERT
	bool DoesUseCBuffer(grcCBuffer *pBuffer);
#endif //__ASSERT

#if !RSG_ORBIS
	virtual u32 GetProgramSize() const = 0;
#endif // !RSG_ORBIS
#endif // EFFECT_USE_CONSTANTBUFFERS

#if TRACK_CONSTANT_BUFFER_CHANGES
	void PrintConstantBufferUsage(ShaderType type, bool printUsage);
#endif

#if (RSG_ORBIS && ENABLE_LCUE) || __D3D11
	static void SetGlobalTexture(int index, void *pTexture, int stateHandle);
#if RSG_ORBIS
	static void UpdateGlobalTexture(int stage, int index, const grcTextureObject *data, u16 stateHandle);
#endif
#endif

#if EFFECT_TRACK_GLOBALS
	const RegisterToGlobalTable& GetRegisterToGlobalTable() const { return m_GlobalByTextureRegister; }
#endif //EFFECT_TRACK_GLOBALS

protected:
#if __WIN32
#if __WIN32PC || RSG_DURANGO
	static void SetParameterCommon(u32 sampler,const /*grcTextureObject*/grcTexture *data,u16 stateHandle);
#else
	static void SetParameterCommon(u32 sampler,const grcTextureObject *data,u16 stateHandle);
#endif
#endif
#if RSG_DURANGO
	template<ShaderType SHADER_TYPE> static void SetTextureCommon(int address, ID3D11ShaderResourceView *srv, u16 stateHandle);
	template<ShaderType SHADER_TYPE> static void SetTextureCommon(int address, const grcTexture*, u16 stateHandle);
	template<ShaderType SHADER_TYPE> static void SetShaderResourceViewCommon(int address, ID3D11ShaderResourceView *srv);
	template<ShaderType SHADER_TYPE> static void SetShaderResourceViewCommon(int address, const grcBufferUAV *buf);
	template<ShaderType SHADER_TYPE> void BindCommon() const;
#endif
#if RSG_ORBIS
	void BindCommon(sce::Gnm::ShaderStage stage, int stageType) const;
	static void SetTextureCommon(sce::Gnm::ShaderStage stage,int,const grcTextureObject*,u16);
#endif
#if RSG_ORBIS || __D3D11
	static void SetParameter(int address, const float *data,int count,u32 offset = 0, grcCBuffer *pEffectVar = NULL);
	static void SetParameterW(int address, const float *data,int wordcount,u32 offset = 0, grcCBuffer *pEffectVar = NULL TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, grcParameter* global = NULL));
	static void SetParameterI(int address,const int* data, int count, u32 offset= 0, grcCBuffer *pEffectVar = NULL TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, grcParameter* global = NULL));
	static void SetFlag(int address, int value, u32 offset= 0, grcCBuffer *pEffectVar = NULL TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, grcParameter* global = NULL));
#endif
#if RSG_DURANGO || RSG_ORBIS
	static void SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type);
#else
	static bool SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type);
#endif
#if __D3D11
	static void SetLocalFlag(int address,bool value, u32 offset= 0, grcCBuffer *grcCBuffer = NULL);

	static void RecordProgramResourceForVectorDXAPICall(ShaderType unit, int slot, void *data);
	static void RecordComputeProgramResourceForVectorDXAPICall(int slot, void *data, int initCount=-1);

	/*
	bool SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type) const;
	void SetLocalFlag(int address,bool value, u32 offset= 0, grcCBuffer *grcCBuffer = NULL) const;
	*/
#endif
	bool Load(fiStream &S, const char* currentLoadingEffect, bool &isDebugName);

#if EFFECT_BIND_UNORDERED_RESOURCES
	static void SetUnorderedTextureUAV	(sce::Gnm::ShaderStage stage, int address, const grcTextureUAV *pTexture);
	static void SetStructuredBuffer		(sce::Gnm::ShaderStage stage, int address, const grcBufferUAV *pBuffer);
	static void SetStructuredBufferUAV	(sce::Gnm::ShaderStage stage, int address, const grcBufferUAV *pBuffer, int unorderedCount = -1);
	static void SetDataBuffer			(sce::Gnm::ShaderStage stage, int address, const grcBufferUAV *pBuffer);
	static void SetDataBufferUAV		(sce::Gnm::ShaderStage stage, int address, const grcBufferUAV *pBuffer);
	static void SetNumAppendConsumeCounters(sce::Gnm::ShaderStage stage, u32 base, u32 numCounters);
#endif // EFFECT_BIND_UNORDERED_RESOURCES

#if EFFECT_CACHE_PROGRAM
	static u32			ComputeFingerprint(const void* code, u32 length);
	static grcShader*	FindShader(u32 key, u32 length, ShaderStage eStage);

	BANK_ONLY(static ShaderCacheStat *GetShaderCacheStats();)	// Returns pointer to array of ssCount+1 ShaderCacheStat objects; last one is totals.
#endif // EFFECT_CACHE_PROGRAM

	static grcShader* CreateShader(const char* pszName, const u8* code, u32 length, ShaderStage eStage, u32 &uKey);
#if EFFECT_PRESERVE_STRINGS
	grcString m_EntryName;
#else
	void *m_UnusedEntryName;
#endif
	grcArray<u32> m_Constants;		// hash codes of all constants used by this program.
									// this is replaced by an array of indices into the owning local parameter array
#if EFFECT_CACHE_PROGRAMRESOURCES
	void **			m_ppDeviceCBuffers;
	u32				m_CBufferFingerprint;	// A hash of all constant buffers used. Many use the same 
											// buffers so can do a very early-out.
#endif // EFFECT_CACHE_PROGRAMRESOURCES

#if EFFECT_USE_CONSTANTBUFFERS
	u8				m_CBufStartSlot, m_CBufEndSlot;
	s8				m_TexStartSlot,	 m_TexEndSlot;

	// Source data use to maintain all the associated constant buffer at the moment
 	int				m_numCBuffers;
 	grcCBuffer*		m_pCBuffers[EFFECT_CONSTANT_BUFFER_COUNT];

	int				m_numTexContainers;
	grcParameter*	m_pTexContainers[EFFECT_TEXTURE_COUNT + EFFECT_UAV_BUFFER_COUNT];
#endif // EFFECT_USE_CONSTANTBUFFERS

#if RSG_ORBIS && ENABLE_LCUE
	sce::LCUE::ShaderResourceOffsets *SRO;
#endif

	u32 m_HashKey;
#if EFFECT_CACHE_PROGRAM
	typedef std::map< u32, grcShader*, std::less<u32>, CacheAllocator<std::pair<u32, grcShader*>> > CacheMap;
	static CacheMap sm_ShaderCache[ssCount];
#if __BANK
	typedef std::map< u32, char*, std::less<u32>, CacheAllocator<std::pair<u32, char*>> > CacheNameMap;
	static CacheNameMap sm_ShaderNameCache[ssCount];

	static ShaderCacheStat sm_Stats[ssCount+1];
#endif // __BANK
#endif // EFFECT_CACHE_PROGRAM
#if EFFECT_TRACK_GLOBALS
	RegisterToGlobalTable m_GlobalByTextureRegister;
#endif //EFFECT_TRACK_GLOBALS
};


#if !__SPU
class grcParameter {
	friend class grcEffect;
	friend class grcInstanceData;
public:
	grcParameter();
	~grcParameter();
#if __SPU
	grcParameter(datResource &) { }
	IMPLEMENT_PLACE_INLINE(grcParameter)
#endif
	void Load(int type, fiStream &S, u32 textureNameHash);
#if EFFECT_PRESERVE_STRINGS
	const char *GetName() const { return Name; }
#endif
	u32 GetNameHash() const { return NameHash; }
	u32 GetSemanticHash() const { return SemanticHash; }
	int GetCount() const { return Count; }
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	void SetParentCBuf(grcCBuffer *pGrcCbuf)					{ FastAssert(pGrcCbuf); CBuf = pGrcCbuf; }
	grcCBuffer *GetParentCBuf()	const							{ return CBuf;	}
	u32 GetCBufferNameHash() const								{ return CBufferNameHash; }
	u8 GetType() const											{ return Type; }
	inline void SetTexture(grcTexture *pTex);
	inline const grcTexture *GetTexPtr() const;
#if SUPPORT_UAV
	inline const grcBufferUAV	*GetReadBufferPtr()	const;
	inline grcBufferUAV			*GetWriteBufferPtr(int *initCount=NULL)	const;
	inline const grcTextureUAV	*GetUnorderedTexturePtr()	const;
	inline int					GetStructuredBufferInitCount() const;
	inline bool IsStackBuffer() const;
#endif	//SUPPORT_UAV
#if EFFECT_TEXTURE_SUBTYPING
	inline bool IsPureTexture() const;
	inline bool	IsMsaaTexture() const;
	inline bool IsComparisonFilter() const;
#endif // EFFECT_TEXTURE_SUBTYPING
	u8 GetRegister()	const									{ return Register; }
	u32	GetCBufOffset()	const									{ return CBufferOffset; }
	inline bool IsSamplerFor(const grcParameter &param) const;
#endif	//platforms

	struct Annotation {
		Annotation();
		~Annotation();
		void Load(fiStream &S);
		u32 NameHash;											// Annotation name
		enum AnnoType { AT_INT, AT_FLOAT, AT_STRING } Type;		// Discriminator for payload
		union {						// Payload (String is allocated on heap)
			int Int;
			float Float;
			const char *String;
		};
	};

#if TRACK_CONSTANT_BUFFER_CHANGES
	void IncrementUsageCount() const { m_UsageCount++; }
	void ResetUsageCount() { m_UsageCount = 0; }
	u32 GetUsageCount() { return m_UsageCount; }
#endif

#if !__SPU
private:
#endif
	u8 Type;				// +0 grcConstant::VarType
	u8 Count;				// +1 nonzero if it's an array (count of sampler states if it's a sampler)
	u8 DataSize;			// +2 count in doublewords if texture (samplerstate/data pairs), or quadwords if floating-point
	u8 AnnotationCount;		// +3 number of attached annotations.
#if EFFECT_PRESERVE_STRINGS
	grcString Name;			// +4 Actual name of the variable
	grcString Semantic;		// +8 Semantic name of the variable
#else
	void *UnusedName, *UnusedSemantic;
#endif
	u32 NameHash;			// +12 atStringHash(m_Name) (matches the grcConstant hash code)

	u32 SemanticHash;		// +16 atStringHash(m_Semantic) (what code typically uses to look things up; may be zero)
	Annotation *Annotations;// +20
	void *Data;				// +24 Default value of the parameter (string name of the texture if it's a texture)
#if EFFECT_TEXTURE_SUBTYPING
	u16 Register			: TEXTURE_REGISTER_BITS;
	u16 TextureType			: 8 - TEXTURE_REGISTER_BITS;
	u16 Usage				: TEXTURE_USAGE_BITS;
	u16 ComparisonFilter	: 8 - TEXTURE_USAGE_BITS;
#else
	u8 Register, Usage;		// +28 High 2 bits store texture sub-type, if needed
#endif
	u8 SamplerStateSet;		// Sampler state set the effect was originally compiled with
	u8 SavedSamplerStateSet;	// Saved sampler state set, only used by global variables.
#if __WIN32PC || RSG_DURANGO || RSG_ORBIS
	u32 CBufferOffset;		// Byte offset 
	u32 CBufferNameHash;
#if TRACK_CONSTANT_BUFFER_CHANGES
	mutable u32 m_UsageCount;
#endif
	union
	{
		grcCBuffer *CBuf;
		const grcTexture *Tex;
#if SUPPORT_UAV
		const grcBufferUAV	*RO_Buffer;
		grcBufferUAV		*RW_Buffer;
		const grcTextureUAV	*RW_Texture;
#endif	//SUPPORT_UAV
	};
#endif	//platforms
};

#if __D3D11 || RSG_ORBIS
struct grcProgramResource
{
	const grcTexture *GetTexPtr()
	{
		return Tex;
	}
#if SUPPORT_UAV
	const grcBufferUAV *GetStructuredBufferPtr()
	{
		return StructuredBuffer;
	}
#endif	//SUPPORT_UAV
	union
	{
		void *pAny;
		const grcTexture *Tex;	
#if SUPPORT_UAV
		const grcBufferUAV	*StructuredBuffer;
#endif	//SUPPORT_UAV
	};
};

struct grcComputeProgramResource
{
#if SUPPORT_UAV
	const grcBufferUAV *GetStructuredBufferPtr(int *initCount=NULL)
	{
		if(initCount)
		{
			*initCount = unorderedCount;
		}
		return StructuredBuffer;
	}
	const grcTextureUAV *GetUnorderedTexturePtr(int *initCount=NULL)
	{
		if(initCount)
		{
			*initCount = unorderedCount;
		}
		return UnorderedTexture;
	}
	const grcBufferUAV *GetDataBufferPtr()
	{
		return DataBuffer;
	}
#endif	// SUPPORT_UAV
	union
	{
		void *pAny;
	#if SUPPORT_UAV
		const grcBufferUAV	*StructuredBuffer;
		const grcTextureUAV	*UnorderedTexture;
		const grcBufferUAV	*DataBuffer;
	#endif	// SUPPORT_UAV
	};
	int unorderedCount;
};

#endif // __D3D11 || RSG_ORBIS

#if __WIN32PC || RSG_DURANGO || RSG_ORBIS
#if TRACK_CONSTANT_BUFFER_CHANGES
CompileTimeAssertSize(grcParameter,48,72);
#else
CompileTimeAssertSize(grcParameter,44,72);
#endif
#else
CompileTimeAssertSize(grcParameter,32,56);	// Multiple of 16 to simplify DMA from SPU
#endif
#endif



#if !__SPU
class grcMaterialLibrary: public pgDictionary<grcInstanceData>
{
	void Push();		// Not safe to use
	void Pop();			// Not safe to use
public:
	grcMaterialLibrary(int memberCount) : pgDictionary<grcInstanceData>(memberCount) { }

	grcMaterialLibrary(datResource &rsc) : pgDictionary<grcInstanceData>(rsc) { }

	// PURPOSE:	Loads material library from a preload list name
	// PARAMS:	preloadList - Directory containing the preload.list file
	// NOTES:	Depending on operation mode, the file actually loaded
	//			will be either a .list file with the material .sps files
	//			listed therein, or a platform-specific resource
	static grcMaterialLibrary *Preload(const char *preloadDir, const char* preloadFile="preload");

	// PURPOSE:	Saves material library loaded via Preload
	// NOTES:	Uses the path the material library was originally loaded from,
	//			although we could override that if desired.
	bool Save() const;
#if __TOOL
	bool SaveToPreloadFile(const char* presetName, const char* preloadFile="preload") const;
	bool EraseFromPreloadFile(const char* presetName, const char* preloadFile="preload") const;
#endif

	// PURPOSE:	Save the single named shader
	bool Save(const char *presetName) const;

#if __TOOL
	// PURPOSE:	Creates a new material based on the named effect.
	// PARAMS:	presetName - Name to give the new material
	//			effectName - Name of the effect to base the shader on.
	// RETURNS:	Pointer to material
	static grcInstanceData *Create(const char *presetName,const char *effectName);

	// PURPOSE:	Remove a named material from the library.
	bool Destroy(const char *presetName);
#endif

	// PURPOSE: Reloads a named material
	bool Reload(const char *presetName) const;

	static grcMaterialLibrary* GetCurrent() { return sm_CurrentLibrary; }
	static grcMaterialLibrary* SetCurrent(grcMaterialLibrary*lib) { 
		grcMaterialLibrary *prev = sm_CurrentLibrary;
		sm_CurrentLibrary = lib; 
		return prev;
	}

	// PURPOSE:	Returns the path this library was loaded from (ie what Preload was called with)
	const char *GetRootPath() const { return m_RootPath; }
#if __BANK
	void AddWidgets(bkBank &bank) const;
#endif
private:
	void Load(fiStream *S);
	grcString m_RootPath;
	static grcMaterialLibrary *sm_CurrentLibrary;
};
#endif

//Forward Declare
#if __BANK
	class grcTexChangeData;
	class grcSamplerStateEditData;
#endif

class grcInstanceData
{
	friend class grcEffect;
	friend class grcEffectInstanceLoaderData;
public:
	grcInstanceData();
	explicit grcInstanceData(datResource &rsc);
#if !__SPU
	// PURPOSE: Clone an instance data.
	// PARAMS:	outClone - Location of main structure.
	void Clone(grcInstanceData &outClone) const;
#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif
	IMPLEMENT_PLACE_INLINE(grcInstanceData);
#else	// __SPU
	void FetchMarkedTextures(datResource &rsc,unsigned maskMarked,unsigned maskUse);
	void MarkUsedTextures(const grcProgram *program);
	void MarkUsedTextures(const grcVertexProgram *vertProgs,const grcFragmentProgram *fragProgs,const grcEffect_Technique_Pass *pass);
	void MarkUsedTextures(const grcEffect *effect,unsigned technique);
	void FetchTextures(datResource &rsc);
	grcInstanceData(datResource &rsc,const grcEffect *effect,unsigned technique,unsigned passIndex);
	grcInstanceData(datResource &rsc,unsigned techniqueGroupId,unsigned drawTypeMask);
	void SpuGet();
	void SpuGet(const grcEffect *effect,unsigned technique,unsigned passIndex);
#endif	// __SPU
	void Shutdown();
	~grcInstanceData();

	// Dummy crap for pgDictionary.
	int Release() const { delete this; return 0; }
	void SetHandleIndex(u32) { }

	void Prefetch() const ;
	// Textures are stored as direct pointers to the grcTexture object
	// and are never cloned.
	bool IsTexture(int idx) const { return Data()[idx].Count == 0; }

	// PURPOSE:	Saves out the instancedata in the legacy .sps format.
	void Save(fiStream *S) const;

	// PURPOSE:	Loads the material from the legacy .sps format or .sva format (inline or separate file)
	bool Load(const char *filename,fiTokenizer &T,bool isPreset);

	int GetDrawBucket_Deprecated() const { return DrawBucket; }
#if __TOOL
	void TryMapVariablesFrom(grcInstanceData& oldData);
	int GetDrawBucket() const { return DrawBucket; }
#endif

#if __WIN32PC || RSG_DURANGO || RSG_ORBIS
	template <class Derived,int mask> void Bind	(const Derived &,const atArray<grcParameter> &locals) const;
	void BindCompute	(const grcComputeProgram &,const atArray<grcParameter> &locals) const;
#else
	template <class Derived,int mask> void Bind(const Derived &) const;
#endif
	template <class Derived> void CopyData(const Derived &, const grcInstanceData &) const;

#if __BANK
	void AddWidgets(bkBank &B) const;
	const char* GetVariableName(int varNum) const;
	bool HasTexture(int varNum, const char* previousTextureName) const;
	int GetNumTextures() const;
	void AddWidgets_WithLoad(bkBank &B, atArray<grcTexChangeData> &texArray, int userData=-1, atArray<grcSamplerStateEditData> *samplerEditData=NULL); // userData is a modelinfo (archetype) index
#endif

	grcEffect& GetBasis() const { return *Basis; }

	const char *GetShaderName() const;

#if __TOOL
	const char *GetComment() const;

	void SetComment(const char*);

	bool GetPresetFlag(grcEffectVar var) const
	{
		return (mPresetFlags & 1 << ((int)var)) != 0;
	}
	void SetPresetFlag(grcEffectVar var, bool onOff)
	{
		if(onOff)
			mPresetFlags |= 1 << ((int)var);
		else
			mPresetFlags &= ~(1 << (int)var);
	}
#endif

	const grcInstanceData* GetMaterial() const { return (Flags & FLAG_MATERIAL)? this : Material; }

	const char* GetMaterialName() const { return (Flags & FLAG_MATERIAL)? MaterialName : Material? Material->MaterialName : NULL; }

	int GetCount() const { return Count; }

	void Optimize();			// Sets CONSTANTS_ALL_INHERITED, CONSTANTS_NONE_INHERITED, Vertex/FragmentConstantBase/Count.

	void UpdateSortKey();

#if EFFECT_CHECK_ENTRY_TYPES
	enum EntryType {
		ET_UNKNOWN,
		ET_FLOAT,
		ET_TEXTURE,
		ET_BUFFER,
	};
#endif //EFFECT_CHECK_ENTRY_TYPES

	struct Entry {
		u8 Count;
#if EFFECT_CHECK_ENTRY_TYPES
		u8 Register : 6;
		u8 Type     : 2;
#else
		u8 Register;
#endif //EFFECT_CHECK_ENTRY_TYPES
		u8 SamplerStateSet,		// Handle for sampler states; must be in even byte address for SPU access
			SavedSamplerStateSet;	// One-element stack for saved sampler state set 
		union {
			float *Float;						// aligned to 16 byte boundary
#if USE_PACKED_GCMTEX
			u32 TextureIndex;
# if __SPU
			grcTextureObject *Texture;
# else
			grcTextureIndex32 Texture;
# endif
#else
# if __SPU || __TOOL || !ENABLE_DEFRAGMENTATION
			grcTexture *Texture;
# else
			grcTextureHandleUnionable Texture;
# endif
# if SUPPORT_UAV
			const grcBufferUAV	*RO_Buffer;
			grcBufferUAV		*RW_Buffer;
			const grcTextureUAV	*RW_Texture;
# endif	//SUPPORT_UAV
#endif	//USE_PACKED_GCMTEX
			void *Any;							// either
		};
	};
	// The actual memory block is owned by Data itself.
	// All constants come before all textures.
	Entry *Entries;			// +0
#if MULTIPLE_RENDER_THREADS
	__forceinline Entry *DataForThread(u32 idx) const {
		Assert(!idx || (Flags & FLAG_EXTRA_DATA)); 
		return (Entry*)((char*)Entries + TotalSize * idx); 
	}
	__forceinline Entry *Data() const { 
		return DataForThread(g_RenderThreadIndex);
	}
	void ExpandForMultipleThreads(bool fromShaderLoad);
	void CopyDataToMultipleThreads();
#else
	__forceinline Entry *Data() const { return Entries; }
#endif
	union {					// +4
		grcEffect *Basis;
		u32 BasisHashCode;
	};
	enum {										// For 'Flags' immediately below
		FLAG_MATERIAL = 0x01,					// This instanceData is really a material
		FLAG_EXTRA_DATA = 0x80,					// This instanceData already has its MTR-expanded buffers allocated (if relevant)
	};
	u8 Count, DrawBucket, PhysMtl_DEPRECATED, Flags;			// +8
	u16 SpuSize, TotalSize;							// +12
	union {											// +16
		grcInstanceData *Material;					// valid at runtime if we inherited from material
		u32 MaterialHashCode;						// valid at resource time if we inherited from material
		const char *MaterialName;					// valid if we are a material (for saving materials back out)
	};

	u32 DrawBucketMask;

	bool IsInstanced;
	u8 UserFlags;					// grmShader steals some of these; only used at runtime, never resourced.
	u8 pad, TextureCount;
#if __TOOL
	union { u32 SortKey_DEPRECATED; const char *Comment; };
	u32 mPresetFlags; 
#else
	u32 SortKey_DEPRECATED;
#endif

	static void HandleUserData(void** ppReference,void* userData);		// For defragmentation on PS3.
};
#if __TOOL
CompileTimeAssertSize(grcInstanceData,36,56);
#elif __WIN32PC || RSG_DURANGO
CompileTimeAssertSize(grcInstanceData,32,48);
#else
CompileTimeAssertSize(grcInstanceData,32,48);
#endif

#if __BANK
class grcTexChangeData : public datBase
{
public:
	grcTexChangeData();
	grcTexChangeData(grcInstanceData* instance, int varNum, const atString& filename, int userData);
	~grcTexChangeData();

	typedef void (*UpdateTextureFuncType)(const grcTexture* texture, const char* name, int userData);
	static void SetUpdateTextureFunc(UpdateTextureFuncType func) { s_UpdateTextureFunc = func; }
	static UpdateTextureFuncType s_UpdateTextureFunc;

	void ReloadTextureFile();
	void LoadTextureFile();
	void UpdateTextureParams();

	grcInstanceData* m_instance;
	int m_varNum;
	atString m_filename;
	int m_userData;
	bool m_ready;
	bkText* m_textBox;
	char m_textBoxString[256];
};

class grcSamplerStateEditData
{
public:
	grcSamplerStateDesc	m_desc;
	grcInstanceData*	m_data;
	grcEffectVar		m_var;
	bool				m_enabled;
};
#endif // __BANK

class grcVertexProgram: public grcProgram {
	friend class grcEffect;
	friend class spuGeometryDma;
public:
	grcVertexProgram();
	~grcVertexProgram();
	void Load(fiStream &S, const char* currentLoadingEffect);

	void Bind() const;
#if __WIN32PC || RSG_DURANGO || RSG_ORBIS
	void Bind(const grcInstanceData &data,const atArray<grcParameter>&locals) const;
#else
	void Bind(const grcInstanceData &data) const;
#endif
#if __PPU
	void RecordBind(const grcInstanceData &data) const;
	void RecordBind() const;
	static void RecordSetParameter(int address,const float *data,int count);
#endif
#if RSG_DURANGO
	void SetTexture(int address,const grcTexture *data,u16 stateHandle) const;
	static void SetGlobalTexture(int address,const grcTextureObject *data,u16 stateHandle);
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { grcProgram::SetTextureCommon<VS_TYPE>(index, data, stateHandle); }
	static void SetStructuredBuffer(int address, const grcBufferUAV *pBuffer);
#elif RSG_ORBIS
	void SetTexture(int address,const grcTextureObject *data,u16 stateHandle) const;
	static void SetGlobalTexture(int address,const grcTextureObject *data,u16 stateHandle);
# if ENABLE_LCUE
	static void UpdateGlobalTexture(int index, const grcTextureObject *data, u16 stateHandle) { grcProgram::UpdateGlobalTexture(sce::Gnm::kShaderStageVs /*2*/, index, data, stateHandle); }
	void BindConstants() const { }	// We do the real work in Bind(const grcVertexDeclaration*) once we know the final vertex formats
# endif
#elif RSG_PC && __D3D11
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { SetTexture(index, data, stateHandle); }
	static void SetStructuredBuffer(int address, const grcBufferUAV *pBuffer);
	static void SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers);
	static void ClearConstantBuffers();
#elif (__XENON || __PS3)
	static void SetParameter(int address, const float *data,int count);
	static void SetTexture(int address,const grcTextureObject *data,u16 stateHandle);
	static void SetFlag(int address,bool value);
	static void SetFlag(int address,int value);
#else
	static void SetParameter(int address, const float *data,int count, u32 offset, grcCBuffer *pEffectVar);
	static void SetParameterW(int address, const float *data,int wordcount,u32 offset = 0, grcCBuffer *pEffectVar = NULL);
	static void SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle);
	static void SetFlag(int address,bool value, u32 offset= 0, grcCBuffer *grcCBuffer = NULL);
	static void SetFlag(int address,int value, u32 offset= 0, grcCBuffer *grcCBuffer = NULL);
	static bool SetLocalParameter(int address, float *data,int count, u32 offset, grcCBuffer *pEffectVar, u8 type);
	static void SetLocalFlag(int address,bool value, u32 offset= 0, grcCBuffer *grcCBuffer = NULL);
#endif

#if __WIN32PC || RSG_DURANGO
	typedef grcVertexShader *ProgramType;
#elif __XENON
	typedef D3DVertexShader *ProgramType;
#elif __PS3
	typedef u128 *ProgramType;
#elif __PSP2
	typedef SceGxmProgram *ProgramType;
#endif
#if RSG_ORBIS
	void Bind(const grcVertexDeclaration*) const;		// finalizes the vertex shader, making sure the fetch shader is correct for active decl.
	u32 GetProgramSize() const { return ProgramSize; }
	static const grcVertexProgram *GetCurrent() { return CurrentProgram; }
#endif

#if RSG_ORBIS
	void* GetProgram() const { return bIsLsShader ? (void*)ProgramLS : bIsEsShader ? (void*)ProgramES : (void*)ProgramVS; }
	bool IsEsShader() const { return bIsEsShader; } 
	bool IsLsShader() const { return bIsLsShader; }
	sce::Gnm::ShaderStage GetGnmStage() const	{ return bIsLsShader ? sce::Gnm::kShaderStageLs : bIsEsShader ? sce::Gnm::kShaderStageEs : sce::Gnm::kShaderStageVs; }
#else
	ProgramType GetProgram() { return Program; }
	ProgramType GetProgram() const { return Program; }
#endif

#if EFFECT_TRACK_INSTANCING_ERRORS
	bool IsPerInstance() const	{ return bIsPerInstance; }
#endif

#if __WIN32PC || RSG_DURANGO || __RESOURCECOMPILER
	void* GetProgramData() { return ProgramData; }
	void* GetProgramData() const { return ProgramData; }
	u32	  GetProgramSize() const { return ProgramSize; }
	void Bind(const grcVertexDeclaration*) const;		// finalizes the vertex shader, making sure the fetch shader is correct for active decl.
#endif

	static ShaderType	GetShaderType() { return VS_TYPE; };


#if __SPU
	grcVertexProgram(datResource &rsc) : grcProgram(rsc) { }
	IMPLEMENT_PLACE_INLINE(grcVertexProgram)
#endif

#if RSG_ORBIS
	struct DeclSetup {
		DeclSetup() { }
		~DeclSetup();
		void *FetchShader;
		const grcVertexDeclaration *FetchDecl;
		u8 InstanceStepRates[2];
		u8 InputCount;
		u8 RemapSemanticTable[13];
		u8 RemapTable[13];
		u32 Modifier;
		bool IsFetchShaderRemapped;
		DeclSetup *Next;
	};
#elif __D3D11
	struct DeclSetup {
		const grcVertexDeclaration *FetchDecl;
		ID3D11InputLayout *InputLayout;
		DeclSetup *Next;
	};
#endif

private:
#if RSG_ORBIS
	union {
		grcVertexShader *ProgramVS;
		grcExportShader	*ProgramES;
		grcLocalShader	*ProgramLS;
	};
#else
	ProgramType Program;
#endif
	u32 ProgramSize;

#if __RESOURCECOMPILER || __PS3
	u16 DefaultConstantCount;
	u32 ProgramOffset;
	CellCgbVertexProgramConfiguration Configuration; // 	uint16_t instructionSlot; uint16_t instructionCount; uint16_t attributeInputMask; uint8_t registerCount;
	u16 *DefaultConstantRegisters;	// [DefaultConstantCount]
	float *DefaultConstantValues; // [DefaultConstantCount*4];
#endif 
#if __XENON
	u8 ConstantBase, ConstantCount;
	float *Constants;
#endif
#if __D3D11
	void* ProgramData;
	mutable DeclSetup *FirstDecl;
#elif __WIN32PC
	void* ProgramData;
#endif
#if RSG_ORBIS
	mutable DeclSetup *FirstDecl;
	u16 FVF; u8 ImmFVF, InputCount;
	u8 SemanticRemap[grcFvf::grcfcCount];		// contains the IA slot of the given channel if present, or 0xFF if not present.
	u8 VsharpRemap[grcFvf::grcfcCount];
	static __THREAD const grcVertexProgram *CurrentProgram;
	bool bIsEsShader, bIsLsShader;
#endif
#if EFFECT_TRACK_INSTANCING_ERRORS
	bool bIsPerInstance;
#endif
};

class grcFragmentProgram: public grcProgram {
	friend class grcEffect;
	friend class spuGeometryDma;
public:
	grcFragmentProgram();
	~grcFragmentProgram();

	void Load(fiStream &S, const char* currentLoadingEffect);

	void Bind() const;
	void Bind(u32 offset,bool isStatic) const;
#if __WIN32PC || RSG_DURANGO || RSG_ORBIS
	void Bind(const grcInstanceData &data,const atArray<grcParameter>&locals) const;
#else
	void Bind(const grcInstanceData &data) const;
#endif
#if __PPU
	void RecordBind(const grcInstanceData &data) const;
	void RecordBind() const;
	static void RecordSetTexture(int address,const grcTextureObject *data,u16 stateHandle);
#endif
#if RSG_DURANGO
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { grcProgram::SetTextureCommon<PS_TYPE>(index, data, stateHandle); }
	static void SetStructuredBuffer(int address, const grcBufferUAV *pBuffer);
#elif __D3D11
	static void SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers);
	static void ClearConstantBuffers();
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { SetTexture(index, data, stateHandle); }
	static void SetStructuredBuffer(int address, const grcBufferUAV *pBuffer);
#elif RSG_ORBIS
	sce::Gnm::ShaderStage GetGnmStage() const	{ return sce::Gnm::kShaderStagePs; }
	static void SetParameter(int index, const float *data,int count, u32 offset = 0, grcCBuffer *pEffectVar = NULL);
	static void SetTexture(int address,const grcTextureObject *data,u16 stateHandle);
# if ENABLE_LCUE
	static void UpdateGlobalTexture(int index, const grcTextureObject *data, u16 stateHandle) { grcProgram::UpdateGlobalTexture(sce::Gnm::kShaderStagePs /*1*/, index, data, stateHandle); }
	void BindConstants() const { BindCommon(GetGnmStage(), PS_TYPE); }
# endif
	u32 GetProgramSize() const { return ProgramSize; }
#elif (__XENON || __PS3)
	static void SetParameter(int address, const float *data,int count);
	static void SetTexture(int address,const grcTextureObject *data,u16 stateHandle);
	static void SetFlag(int address,bool value);
	static void SetFlag(int address,int value);
#else
	static void SetParameter(int address, const float *data,int count,u32 offset, grcCBuffer *pEffectVar);
	static void SetParameterW(int address, const float *data,int count,u32 offset, grcCBuffer *pEffectVar TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, grcParameter *global));
	static void SetFlag(int address,bool value, u32 offset = 0,grcCBuffer *pEffectVar = NULL);
	static void SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle);
	static bool SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type);
	static void SetLocalFlag(int address,bool value, u32 offset= 0, grcCBuffer *grcCBuffer = NULL);
#endif

#if __PS3
	static void InitCache(u128 *cachePtr,unsigned cacheSizeBytes);
	u32 Cache() const;
	u32 GetOffset() const { return Configuration.offset; }
	u8 GetInitialRegisterCount() const { return Configuration.registerCount; }
	void SetInitialRegisterCount(u8 registerCount) { Configuration.registerCount = registerCount; }
	static void StripBranches(u128 const *pOriginalUCode, u32 nOriginalUCodeSize, u32 nTotalBufferSize, u128** ppUCodeOut, u32* pUCodeSizeOut);
#if !__SPU
	// Search for branch instructions in the ucode that are eligible for runtime stripping and make sure we have enough cache size to do stripping.
	bool CanStripBranches(u8 const *pUCode, const u32 nUCodeSize) const;
#endif // !__SPU
#endif

#if __WIN32PC || RSG_DURANGO || RSG_ORBIS
	typedef grcPixelShader *ProgramType;
#elif __XENON
	typedef D3DPixelShader *ProgramType;
#elif __PS3
	typedef u128 *ProgramType;
#elif __PSP2
	typedef SceGxmProgram *ProgramType;
#endif

#if __SPU
	grcFragmentProgram(datResource &rsc) : grcProgram(rsc) { 
		rsc.PointerFixup(CompressedPatchTable);
	}
	IMPLEMENT_PLACE_INLINE(grcFragmentProgram)
#endif

	ProgramType GetProgram() { return Program; }
	ProgramType GetProgram() const { return Program; }
#if __RESOURCECOMPILER || __PS3 || __WIN32PC || RSG_DURANGO
	u32 GetProgramSize() const { return ProgramSize; }
#endif

	static ShaderType	GetShaderType() { return PS_TYPE; };

#if !__SPU
private:
#endif
	ProgramType Program;
#if !__PS3
	u32 ProgramSize;
#else // !__PS3
	enum 
	{
		FP_PROGFLAG_STRIP_BRANCHES = 0x1
	};
	
	u16 ProgramSize;
	u8 ProgramFlags;  // Currently only using a single bit for FP_PROGFLAG_STRIP_BRANCHES
	u8 __Padding;
#endif // !__PS3

#if __RESOURCECOMPILER || __PS3
	u8 *CompressedPatchTable;		// Register index (255 is terminator of entire list), 
			// followed by 12:4 split of (count-1) and first offset, followed by (count-1)
			// additional deltas from original offset.  If delta is 0xF0 or greater, it's
			// really a two-byte offset (rare) with the upper 4 bits in the lower nibble of the 0xF0.
			// All offsets are in quadwords, giving us a limit of 64k of ucode (same as before).
	CellCgbFragmentProgramConfiguration Configuration;
#if __SPU
	static u128 *sm_Constants;
#else
	static u128 sm_Constants[256];
#endif
#endif
#if RSG_ORBIS
	bool IsPerSample;
#if __ASSERT
	uint8_t ColorOutputFormat;
#endif
#endif
};

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
class grcComputeProgram: public grcProgram {
	friend class grcEffect;
	friend class spuGeometryDma;
public:
	grcComputeProgram();
	~grcComputeProgram();

	void Load(fiStream &S, const char* currentLoadingEffect);

	void Bind() const;
	void Bind(u32 offset) const;
	void Bind(const grcInstanceData &data,const atArray<grcParameter> &locals) const;

#if RSG_DURANGO
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { grcProgram::SetTextureCommon<CS_TYPE>(index, data, stateHandle); }
#elif RSG_ORBIS
	sce::Gnm::ShaderStage GetGnmStage() const	{ return sce::Gnm::kShaderStageCs; }
	static void SetParameter(int index, const float *data,int count, u32 offset = 0, grcCBuffer *pEffectVar = NULL);
	static void SetTexture(int address,const grcTextureObject *data,u16 stateHandle);
#elif __D3D11
	static void SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers);
	static void ClearConstantBuffers();
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle, bool bSetSamplerOnly = false);
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { SetTexture(index, data, stateHandle); }
#else
	static void SetParameter(int address, float *data,int count,u32 offset, grcCBuffer *pEffectVar);
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle, bool bSetSamplerOnly = false);
	static void SetFlag(int address,bool value, u32 offset = 0,grcCBuffer *pEffectVar = NULL);

	bool SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type) const;
	void SetLocalFlag(int address,bool value,u32 offset,grcCBuffer *pEffectVar) const;
#endif

#if SUPPORT_UAV && !EFFECT_BIND_UNORDERED_RESOURCES
	static void SetUnorderedTextureUAV	(int address, const grcTextureUAV *pTexture);
	static void SetStructuredBuffer		(int address, const grcBufferUAV *pBuffer);
	static void SetStructuredBufferUAV	(int address, const grcBufferUAV *pBuffer, int unorderedCount = -1);
	static void SetDataBuffer			(int address, const grcBufferUAV *pBuffer);
	static void SetDataBufferUAV		(int address, const grcBufferUAV *pBuffer);
#endif	//SUPPORT_UAV && !EFFECT_BIND_UNORDERED_RESOURCES
	
#if __D3D11
	void SetUnorderedResource() const;
	static void ResetUnorderedResource();
#endif
	
	typedef grcComputeShader *ProgramType;

	ProgramType GetProgram() { return Program; }
	ProgramType GetProgram() const { return Program; }
	u32 GetProgramSize() const { return ProgramSize; }

	static ShaderType	GetShaderType() { return CS_TYPE; };

private:
	ProgramType Program;
	u32 ProgramSize;
};


class grcDomainProgram: public grcProgram {
	friend class grcEffect;
	friend class spuGeometryDma;
public:
	
	grcDomainProgram();
	~grcDomainProgram();

	void Load(fiStream &S, const char* currentLoadingEffect);

	void Bind() const;
	void Bind(u32 offset) const;
	void Bind(const grcInstanceData &data,const atArray<grcParameter> &locals) const;

#if RSG_DURANGO
	void SetTexture(int address,const grcTexture *data,u16 stateHandle) const;
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { grcProgram::SetTextureCommon<DS_TYPE>(index, data, stateHandle); }
	static void SetStructuredBuffer(int address, const grcBufferUAV *pBuffer);
#elif RSG_ORBIS
	static void SetParameter(int index, const float *data,int count, u32 offset = 0, grcCBuffer *pEffectVar = NULL);
	void SetTexture(int address,const grcTextureObject *data,u16 stateHandle) const;
# if ENABLE_LCUE
	static void UpdateGlobalTexture(int index, const grcTextureObject *data, u16 stateHandle) { grcProgram::UpdateGlobalTexture(sce::Gnm::kShaderStageLs /*1*/, index, data, stateHandle); }
	void BindConstants() const { BindCommon(GetGnmStage(), DS_TYPE); }
# endif	//ENABLE_LCUE
#elif __D3D11
	static void SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers);
	static void ClearConstantBuffers();
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { SetTexture(index, data, stateHandle); }
	static void SetStructuredBuffer(int address, const grcBufferUAV *pBuffer);
#else
	static void SetParameter(int index, float *data,int count, u32 offset = 0, grcCBuffer *pEffectVar = NULL);
	static void SetFlag(int address,bool value, u32 offset = 0,grcCBuffer *pEffectVar = NULL);
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	bool SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type) const;
	void SetLocalFlag(int address,bool value, u32 offset= 0, grcCBuffer *grcCBuffer = NULL) const;
#endif

	u32 GetProgramSize() const { return ProgramSize; }

#if RSG_ORBIS
	void* GetProgram() const { return bIsEsShader ? (void*)ProgramES : (void*)ProgramVS; }
	bool IsEsShader() const { return bIsEsShader; } 
	sce::Gnm::ShaderStage GetGnmStage() const	{ return bIsEsShader ? sce::Gnm::kShaderStageEs : sce::Gnm::kShaderStageVs; }
	static const grcDomainProgram *GetCurrent() { return CurrentProgram; }
#else
	typedef grcDomainShader *ProgramType;
	ProgramType GetProgram() { return Program; }
	ProgramType GetProgram() const { return Program; }
#endif	//RSG_ORBIS

	static ShaderType	GetShaderType() { return DS_TYPE; };

private:
#if RSG_ORBIS
	static __THREAD const grcDomainProgram *CurrentProgram;
	union {
		grcVertexShader *ProgramVS;
		grcExportShader	*ProgramES;
	};
	bool bIsEsShader;
#else
	ProgramType Program;
#endif	//RSG_ORBIS
	u32 ProgramSize;
};


class grcGeometryProgram: public grcProgram {
	friend class grcEffect;
	friend class spuGeometryDma;
public:
	struct grcStream
	{
		char			mSemanticName[16];	
		unsigned char	mSemanticIndex;							
		unsigned char	mStartComponent;						
		unsigned char	mComponentCount;						
		unsigned char	mOutputSlot;							
	};

	grcGeometryProgram();
	~grcGeometryProgram();

	void Load(fiStream &S, const char* currentLoadingEffect);

	void Bind() const;
	void Bind(u32 offset) const;
	void Bind(const grcInstanceData &data,const atArray<grcParameter> &locals) const;

#if RSG_DURANGO
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { grcProgram::SetTextureCommon<GS_TYPE>(index, data, stateHandle); }
	static void SetStructuredBuffer(int address, const grcBufferUAV *pBuffer);
#elif RSG_ORBIS
	sce::Gnm::ShaderStage GetGnmStage() const	{ return sce::Gnm::kShaderStageGs; }
	static void SetParameter(int index, const float *data,int count, u32 offset = 0, grcCBuffer *pEffectVar = NULL);
	static void SetTexture(int address,const grcTextureObject *data,u16 stateHandle);
# if ENABLE_LCUE
	static void UpdateGlobalTexture(int index, const grcTextureObject *data, u16 stateHandle) { grcProgram::UpdateGlobalTexture(sce::Gnm::kShaderStageGs /*3*/, index, data, stateHandle); }
	void BindConstants() const { BindCommon(GetGnmStage(), GS_TYPE); }
# endif
#elif __D3D11
	static void SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers);
	static void ClearConstantBuffers();
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { SetTexture(index, data, stateHandle); }
	static void SetStructuredBuffer(int address, const grcBufferUAV *pBuffer);
#else
	static void SetParameter(int index, float *data,int count, u32 offset = 0, grcCBuffer *pEffectVar = NULL);
	static void SetFlag(int address,bool value, u32 offset = 0,grcCBuffer *pEffectVar = NULL);
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	bool SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type) const;
	void SetLocalFlag(int address,bool value, u32 offset= 0, grcCBuffer *grcCBuffer = NULL) const;
#endif	//RSG_ORBIS

	typedef grcGeometryShader *ProgramType;

	ProgramType GetProgram() { return Program; }
	ProgramType GetProgram() const { return Program; }
	u32 GetProgramSize() const { return ProgramSize; }

	static ShaderType	GetShaderType() { return GS_TYPE; };

private:
	ProgramType Program;
	u32 ProgramSize;
	grcStream*	Streams;
	u32 StreamCount;
};

class grcHullProgram: public grcProgram {
	friend class grcEffect;
	friend class spuGeometryDma;
public:
	grcHullProgram() {}
	~grcHullProgram();

	void Load(fiStream &S, const char* currentLoadingEffect);

	void Bind() const;
	void Bind(u32 offset) const;
	void Bind(const grcInstanceData &data,const atArray<grcParameter> &locals) const;

#if RSG_DURANGO
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { grcProgram::SetTextureCommon<HS_TYPE>(index, data, stateHandle); }
	static void SetStructuredBuffer(int address, const grcBufferUAV *pBuffer);
#elif RSG_ORBIS
	sce::Gnm::ShaderStage GetGnmStage() const	{ return sce::Gnm::kShaderStageHs; }
	static void SetParameter(int index, const float *data,int count, u32 offset = 0, grcCBuffer *pEffectVar = NULL);
	static void SetTexture(int address,const grcTextureObject *data,u16 stateHandle);
	static const grcHullProgram* GetCurrent()	{ return CurrentProgram; }
# if ENABLE_LCUE
	static void UpdateGlobalTexture(int index, const grcTextureObject *data, u16 stateHandle) { grcProgram::UpdateGlobalTexture(sce::Gnm::kShaderStageHs /*5*/, index, data, stateHandle); }
	void BindConstants() const { BindCommon(GetGnmStage(), HS_TYPE); }
# endif
#elif __D3D11
	static void SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers);
	static void ClearConstantBuffers();
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	static void UpdateGlobalTexture(int index, const grcTexture *data, u16 stateHandle) { SetTexture(index, data, stateHandle); }
	static void SetStructuredBuffer(int address, const grcBufferUAV *pBuffer);
#else
	static void SetParameter(int index, float *data,int count, u32 offset = 0, grcCBuffer *pEffectVar = NULL);
	static void SetFlag(int address,bool value, u32 offset = 0,grcCBuffer *pEffectVar = NULL);
	static void SetTexture(int address,const grcTexture *data,u16 stateHandle);
	bool SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type) const;
	void SetLocalFlag(int address,bool value, u32 offset= 0, grcCBuffer *grcCBuffer = NULL) const;
#endif	//RSG_ORBIS

	typedef grcHullShader *ProgramType;

	ProgramType GetProgram() { return Program; }
	ProgramType GetProgram() const { return Program; }
	u32 GetProgramSize() const { return ProgramSize; }

	static ShaderType	GetShaderType() { return HS_TYPE; };

private:
	ProgramType Program;
	u32 ProgramSize;
#if RSG_ORBIS
	static __THREAD const grcHullProgram *CurrentProgram;
#endif	//RSG_ORBIS
};

#else	// RSG_PC || RSG_DURANGO || RSG_ORBIS

template <bool HasStreamOut>
class grcPlaceHolderProgram: public grcProgram {
	friend class grcEffect;
	friend class spuGeometryDma;
public:
	grcPlaceHolderProgram()		{ ; }
	~grcPlaceHolderProgram()	{ ; }

	void Load(fiStream &,const char*);
private:
};
typedef grcPlaceHolderProgram<false>	grcComputeProgram;
typedef grcPlaceHolderProgram<false>	grcDomainProgram;
typedef grcPlaceHolderProgram<true>		grcGeometryProgram;
typedef grcPlaceHolderProgram<false>	grcHullProgram;		
#endif //RSG_PC || RSG_DURANGO
/*
	A grcEffect collects one or more techniques	together along with parameters used by programs within passes of those techniques.  

	Modify per-instance parameters with the SetVar overloads.  Modify global parameters with the SetGlobalVar overloads.

	The standard way to use an effect is the following:

	int passCount = pEffect->BeginDraw(tech,restoreState)
	for (int i=0; i < passCount; i++) {
		pEffect->BeginPass(i);
		...draw something using the shader...
		pEffect->EndPass();
	}
	pEffect->EndDraw();

	Note that if restoreState is true, any states modified by BeginPass are undone by EndPass.  Otherwise, any states are inherited
	by the next pass.  (PS3 does not inherit sampler states across passes).

	The other way to use an effect is to call pEffect->Bind directly.  This can only be done outside of a BeginDraw/EndDraw block,
	and any BeginDraw cancels a Bind.  It's intended for immediate mode use.  Note that BeginPass and Bind both call the same
	functions internally.

	Calls to SetGlobalVar always take effect immediately.  Calls to SetVar also take effect immediately if they are called on the
	effect that is currently active, no additional call is necessary.

	For performance reasons, we cache fragment programs in VRAM.  In the current implementation, changing fragment parameters
	on an effect doesn't update the cached copy, it must be flushed from the cache first.  This needs to be fixed.  Ideally updating
	parameters, particularly globals, should only flush what is necessary.
*/
struct grcEffect_Technique_Pass {
	void Load(fiStream &S);
	u8 VertexProgramIndex, FragmentProgramIndex;
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	u8 GeometryProgramIndex, ComputeProgramIndex, DomainProgramIndex, HullProgramIndex;
#endif			
	u8 RasterizerStateHandle, DepthStencilStateHandle, BlendStateHandle, AlphaRef, StencilRef, pad;
	DECLARE_DUMMY_PLACE(grcEffect_Technique_Pass);
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	void Bind(const grcVertexProgram& vp,const grcFragmentProgram& fp,const grcComputeProgram &cp, const grcDomainProgram &dp, const grcGeometryProgram &gp, const grcHullProgram &hp, const grcInstanceData &instanceData, const atArray<grcParameter> &locals) const;
#else
	void Bind(const grcVertexProgram& vp,const grcFragmentProgram& fp, const grcInstanceData &instanceData) const;
#endif
#if __PPU
	void RecordBind(const grcVertexProgram& vp,const grcFragmentProgram& fp, const grcInstanceData &instanceData) const;
#endif
	void UnBind(bool restoreState = true) const;

#if __BANK
	static DECLARE_MTR_THREAD bool ms_bEnableShaderRS;
#endif
};
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
CompileTimeAssertSize(grcEffect_Technique_Pass,12,12);
#else
CompileTimeAssertSize(grcEffect_Technique_Pass,8,8);
#endif		

class grcEffect {
	friend class spuGeometryDma;
	friend class grcInstanceData;

	grcEffect();		// ctor is private

	// PURPOSE:	Initialize an effect from a file on disc
	// PARAMS:	filename - Name of the effect file, .fx is assumed.
	// RETURNS:	True on success, false on failure	
	bool Init(const char *filename);
public:
	static const int RORC_VERSION = 13;
	static const int c_MaxEffects = (__XENON || __PPU) ? 512 : 512;

	struct Technique {
		Technique();
		~Technique();
		void Load(fiStream &S);

		typedef grcEffect_Technique_Pass Pass;

		u32 NameHash;				// hashed name of the technique
#if EFFECT_PRESERVE_STRINGS
		grcString Name;			// human-readable name of the technique
#else
		void *UnusedName;
#endif
		grcArray<Pass> Passes;		// array of passes for the technique.
#if __SPU
		explicit Technique(datResource &rsc) : Passes(GRCARRAY_PLACEELEMENTS,rsc) { }
		IMPLEMENT_PLACE_INLINE(Technique)
#endif
	};
	CompileTimeAssertSize(Technique,16,32);

	void GetLocalCommon(const grcInstanceData &instanceData,grcEffectVar handle,void *dest,size_t destSize) const;

#if !__SPU
	const grcParameter::Annotation* GetAnnotation(grcEffectVar var,const char *name) const;
	const grcParameter::Annotation* GetAnnotation(grcEffectVar var,u32 hashcode) const;

	static const grcParameter::Annotation* GetGlobalAnnotation(int idx,const char *name);
#endif

	void ConnectParametersToPasses();

	void ConstructReferenceInstanceData(grcInstanceData &instanceData);

	void Load(fiStream &S,int magic, const char* currentLoadingEffect);

#if __BANK && EFFECT_PRESERVE_STRINGS && __D3D11
	void SetPIXLabel();
	template <class T>
	void SetPIXLabelInPrograms(grcArray <T> &ProgramList);
#endif //__BANK && EFFECT_PRESERVE_STRINGS && __D3D11

public:
	enum VarType { 
		VT_NONE, VT_INT_DONTUSE, VT_FLOAT, VT_VECTOR2,				// 0-3
		VT_VECTOR3, VT_VECTOR4, VT_TEXTURE, VT_BOOL_DONTUSE,		// 4-7
		VT_MATRIX43, VT_MATRIX44, VT_STRING,						// 8-A
#if RSG_PC || RSG_DURANGO || RSG_ORBIS	// && DX10 or above...
		VT_INT, VT_INT2, VT_INT3, VT_INT4, VT_STRUCTUREDBUFFER, VT_SAMPLERSTATE,
#endif	//__D3D11
		VT_UNUSED1, VT_UNUSED2, VT_UNUSED3, VT_UNUSED4, //these are to be used for the particle system to add custom vertex buffer values
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
		VT_UAV_STRUCTURED, VT_UAV_TEXTURE,	// these only affect Compute Shaders
		VT_BYTEADDRESSBUFFER, VT_UAV_BYTEADDRESS,
#endif	//__D3D11
		VT_COUNT /*Must be the last one*/};

	// NOTE, all strings here are just pointer copies of the attributes and
	// will become invalid if the effect is unloaded.
	struct VariableInfo {
		VariableInfo();

		const char *				m_Name;			// Semantic name
		const char *				m_ActualName;	// Actual underlying parameter name, only on __WIN32PC builds.
		VarType						m_Type;
		const char *				m_TypeName;
		const char *				m_UiName;		// Name to display in widgets
		const char *				m_UiWidget;		// Used to describe what type of widget to display
		const char *				m_UiHelp;		// Extra info for widget tool tips
		const char *				m_UiHint;
		const char *				m_AssetName;	// Primarily useful for hardcoding texture filenames
		const char *				m_GeoValueSource;//The source of the value ("instance","type","template") "instance" is the assumed default
		float						m_UiMin;		// Minimum allowed value for a widget
		float						m_UiMax;		// Maximum allowed value for a widget
		float						m_UiStep;		// Step size
		bool						m_UiHidden;		// Flag for if this variable should be hidden from the UI.
		int							m_UvSetIndex;	// Used for texture variables to formally declare which UV set index the shader uses
		const char *				m_UvSetName;	// Used for texture variables to formally declare which UV set name the exporter should use
		const char *				m_TextureOutputFormats;		// The output formats of the texture, this is a ; separated list of 'platform'='format' pairs
		int							m_iNoOfMipLevels;			// No of mip map levels to create (If this value is zero, a complete mipmap chain is created.  This also the default.)
		int							m_MaterialDataUvSetIndex; // Used for misc. material data that needs to get pushed into the vertex buffer. -1 means not using
		int							m_ArrayCount;	// Number of elements in the array
		bool						m_IsMaterial;

		const char *				m_TCPTemplate;				// Default texture processing template
		const char *				m_TCPTemplateRelative;		// Default texture processing template as relative path
		int							m_TCPAllowOverride;		// Whether the default texture processing template can be overidden
	};

	// PURPOSE:	Create an effect based on and loaded from a filename.  Checks for duplicate first.
	static grcEffect* Create(const char *name);

	// PURPOSE:	Create an effect based on and loaded from a filename.  Checks for duplicate first.
	//			Returns a fallback effect (the first effect we ever preload) if the desired effect
	//			isn't found.
	static grcEffect* CreateWithFallback(const char *name);

	// PURPOSE:	Destructor
	~grcEffect();

	// PURPOSE:	Initialize the entire system (called automatically by grcDevice::InitClass)
	static void InitClass();

	// PURPOSE: Enable/Disable the use of a fallback effect. When disabled, the game will quit on failure.
	static void SetEnableFallback(bool value);
	
	// PURPOSE:	Preload one or more effects listed in preload.list in the specified directory
	//			The effects in question are allocated directly on the heap and
	//			the toplevel memory is NOT currently reclaimed.  Attempts to preload
	//			an effect that is already known (via LookupEffect) are ignored.
	static bool Preload(const char *preloadDir, const char* preloadFileName = "preload");
#if RSG_PC
	static void SetShaderQuality(int iQuality);
	static u32  GetShaderQuality() { return sm_ShaderQuality; }
#endif // RSG_PC
	// PURPOSE:	Reload all effect files from their original paths, checking date stamps.
	static void ReloadAll();

	// PURPOSE:	Unload the effect files.
	static void UnloadAll();

	// PURPOSE:	Given the hash code of an effect (as stored in an grcInstanceData typically)
	//			find the original effect, if available.  This is how resourced objects reconnect
	//			to their original effects (which are not resourced).
	static grcEffect *LookupEffect(u32 hashCode);

	// PURPOSE:	Given the hash name of either an effect or a material, return the associated
	//			material.  We search material libraries first, and then loaded effects, returning
	//			the effect's instance data in that case.
	static grcInstanceData *LookupMaterial(u32 hashCode);

	// PURPOSE:	Free memory associated with effect.  Shouldn't ever need to call this externally.
	void Shutdown();

	// PURPOSE:	Shutdown the entire system (called automatically by grcDevice::ShutdownClass)
	static void ShutdownClass();

	// PURPOSE:	Clear any cached state we may have; useful when recording command
	//			buffers so that we don't assume anything about previously executed
	//			effects.  In particular, on some platforms we remember the last vertex
	//			and pixel (fragment) shader we bound to avoid the overhead of sending the
	//			same one down again.
	// NOTES:	There should be no reason to call this directly, it's used internally
	//			by other basic rage graphics components.
	static void ClearCachedState();

	// PURPOSE:	Clear any cached sampler state we may have; it needs to be called when
	//			modifying any sampler state (e.g.: PushDefaultSamplerState/PushDefaultSamplerState),
	//			otherwise changes are not going to be visible due to the cache showing
	//			that a particular sampler state set hasn't changed.
	// NOTES:	There should be no reason to call this directly, it's used internally
	//			by other basic rage graphics components.
#if __WIN32
	static void ClearCachedSamplerState();
#endif

	// PURPOSE: Force sets graphics context state again.
	//          On XB1, the RestoreDeferredContextState flag to
	//          ID3D11DeviceContext::FinishCommandList doesn't work, so this
	//          function is part of what is required to manually implement it.
	// PARAMS   ctx - graphics context
#if RSG_DURANGO
	static void ForceSetContextShaders(grcContextHandle *ctx);
#endif

	// PURPOSE:	Returns human-readable name associated with a type.
	static const char *GetTypeName(VarType vt);

	// PURPOSE:	Given a type name, return its VT_... value
	static VarType GetType(const char *type);

	// PURPOSE:	Begin rendering the specified pass
	// PARAMS:	passIndex - Pass number to begin rendering
	//			instanceData - Instance data to use for rendering instead of the data built into the effect
	// NOTES:	See class description for typical program flow
	void BeginPass(int passIndex,const grcInstanceData &instanceData) const;
#if __PPU
	void RecordBeginPass(int passIndex,const grcInstanceData &instanceData) const;
#endif

#if !__SPU
	// PURPOSE:	Begin rendering the specified pass
	// PARAMS:	passIndex - Pass number to begin rendering
	// NOTES:	See class description for typical program flow
	FXFORCEINLINE void BeginPass(int passIndex) const {
		BeginPass(passIndex,m_InstanceData);
	}
#endif

	void EndPass() const;
#if __PPU
	void RecordEndPass() const;
#endif

	void CopyPassData(grcEffectTechnique techHandle, int passIndex, const grcInstanceData &instanceData, const grcInstanceData &sourceData) const;

	// PURPOSE:	Begin rendering the specified pass
	// PARAMS:	passIndex - Pass number to begin rendering
	// NOTES:	See class description for typical program flow
	void BeginIncrementalPass(int passIndex,const grcInstanceData &instanceData) const;

#if __PS3
	FXFORCEINLINE u8 GetInitialRegisterCount(int passIndex) const {
		FastAssert(sm_CurrentTechnique);
		return m_FragmentPrograms[sm_CurrentTechnique->Passes[passIndex].FragmentProgramIndex].GetInitialRegisterCount();
	}

	FXFORCEINLINE void SetInitialRegisterCount(grcEffectTechnique tech, int passIndex, int registerCount) {
		m_FragmentPrograms[m_Techniques[tech-1].Passes[passIndex].FragmentProgramIndex].SetInitialRegisterCount(registerCount);
	}
	
#endif

	// PURPOSE:	Make the technique current.  This function is designed for use by
	//			immediate-mode code which doesn't want the Begin/End overhead.
	//			You are explicitly allowed to Bind a new effect while a previous
	//			effect is already bound.  BeginDraw will cancel any Bind.
	// PARAMS:	tech - Technique to make current
	void Bind(grcEffectTechnique tech) const;

#if __SPU
	void Bind(grcEffectTechnique tech,int passIndex,const grcInstanceData &instanceData) const 
	{
			const Technique::Pass &p = m_Techniques[tech-1].Passes[passIndex];
			p.Bind(m_VertexPrograms[p.VertexProgramIndex],m_FragmentPrograms[p.FragmentProgramIndex],instanceData);
	}
	/* void IncrementalBind(grcEffectTechnique tech,int passIndex,const grcInstanceData &instanceData) const 
	{
		m_Techniques[tech-1].Passes[passIndex].IncrementalBind(m_VertexPrograms,m_FragmentPrograms,instanceData,m_Locals);
	} */
	void UnBind(grcEffectTechnique tech,int passIndex) const
	{
		m_Techniques[tech-1].Passes[passIndex].UnBind();
	}

	u32 GetFragmentProgramSize(grcEffectTechnique tech, int passIndex) const
	{
		u32 fpIndex = m_Techniques[tech-1].Passes[passIndex].FragmentProgramIndex;
		return m_FragmentPrograms[fpIndex].GetProgramSize();
	}
#endif

	static FXFORCEINLINE void UnBind() 
	{
		sm_CurrentPass->UnBind();
		sm_CurrentPass = sm_CurrentBind = NULL;
	}

	// PURPOSE:	Begin rendering using the specified technique.
	// PARAMS:	tech - Technique to use
	//			restoreState - If true, any render or sampler state touched by the effect
	//				will be restored to default state (as per SetDefaultRenderState and
	//				SetDefaultSamplerState) in EndDraw.  There is some overhead to this,
	//				so don't use it unless necessary.
	// RETURNS:	Number of passes in the technique
	// NOTES:	See class description for typical program flow
	FXFORCEINLINE int BeginDraw(grcEffectTechnique tech,bool restoreState) const {
		sm_CurrentTechniqueHandle = tech;
		if (tech) {
			sm_CurrentTechnique = &m_Techniques[tech-1];
			int passCount = sm_CurrentTechnique->Passes.GetCount();
			sm_RestoreState = restoreState || passCount > 1;
			FastAssert(passCount);
			return passCount;
		}
		else
			return 0;
	}

	// RETURNS:	True if an effect is active.  (So that lower-level code knows not to set up a default effect)
	static FXFORCEINLINE bool IsInDraw()
	{
		return sm_CurrentPass || sm_CurrentBind;
	}

	// PURPOSE: Prepare an effect for rendering in the near future.  Several Configure calls should
	//			be batched together if possible before any BeginDraw blocks.
	// FXFORCEINLINE void Configure(grcEffectTechnique tech) const { Configure(tech,m_InstanceData); }

	// PURPOSE:	Ends the current technique
	FXFORCEINLINE void EndDraw() { sm_CurrentTechnique = NULL; }

	void Clone(grcInstanceData &outClone) const;

#if !__SPU
	int GetDrawBucket_Deprecated() const { return m_InstanceData.DrawBucket; }
#if __TOOL	// TEMPORARY FIX
	int GetDrawBucket() const { return m_InstanceData.DrawBucket; }
#endif
	u32 GetDrawBucketMask() const { return m_InstanceData.DrawBucketMask; }
#endif

	// PURPOSE:	Look up a global variable by name (one declared 'shared' in the fx file)
	// PARAMS:	name - Name of the variable to look up (case-sensitive)
	//			mustExist - If true, issue an error if the variable is not found
	// RETURNS:	Variable handle if found, or grcegvNONE (zero) if not found.
	// NOTES:	We compare names based on the semantic first, then the actual name.
	static grcEffectGlobalVar LookupGlobalVar(const char *name,bool mustExist = true);

	static grcCBuffer *LookupGlobalConstantBufferByHash(u32 hash);

	// PURPOSE:	Set a global variable's value (many overloads)
	// PARAMS:	handle - Variable handle
	//			value - Value of the variable to set
	//			count - Number of elements in the array (for array overloads)
	// NOTES:	It is undefined behavior if you pass down
	//			a value that doesn't match the actual underlying type.
	static void SetGlobalVar(grcEffectGlobalVar handle,const grcTexture *value);
	static void SetGlobalVar(grcEffectGlobalVar handle,bool value);
	static void SetGlobalVar(grcEffectGlobalVar handle,int value);
	static void SetGlobalVar(grcEffectGlobalVar handle,const IntVector &value);
	static FXFORCEINLINE void SetGlobalVar(grcEffectGlobalVar handle,float value);
	static void SetGlobalVar(grcEffectGlobalVar handle,const float *value,int count);
	static FXFORCEINLINE void SetGlobalVar(grcEffectGlobalVar handle,const Vec2V &value);
	static void SetGlobalVar(grcEffectGlobalVar handle,const Vec2f *value,int count);
	static FXFORCEINLINE void SetGlobalVar(grcEffectGlobalVar handle,const Vec3V &value);
	static FXFORCEINLINE void SetGlobalVar(grcEffectGlobalVar handle,const Vec3V *value,int count);
	static FXFORCEINLINE void SetGlobalVar(grcEffectGlobalVar handle,const Vec4V &value);
	static FXFORCEINLINE void SetGlobalVar(grcEffectGlobalVar handle,const Vec4f &value) { SetGlobalVar(handle,(Vec4V&)value); }
	static FXFORCEINLINE void SetGlobalVar(grcEffectGlobalVar handle,const Vec4V *value,int count);
	static FXFORCEINLINE void SetGlobalVar(grcEffectGlobalVar handle,const Vec4f *value,int count) { SetGlobalVar(handle,(Vec4V*)value,count); }
	static FXFORCEINLINE void SetGlobalVar(grcEffectGlobalVar handle,const Mat34V &value);
	static FXFORCEINLINE void SetGlobalVar__As4x4(grcEffectGlobalVar handle,const Matrix43 *value,int count);
	static FXFORCEINLINE void SetGlobalVar(grcEffectGlobalVar handle,const Mat44V &value);
	static FXFORCEINLINE void SetGlobalVar(grcEffectGlobalVar handle,const Mat44V *value,int count);

#if !__SPU
	// PURPOSE: Get the actual register of a global variable.
	// PARAMS:  handle - Variable handle
	// RETURNS: The register index, like 48 for "c48".
	static int GetGlobalVarRegister(grcEffectGlobalVar handle);

	// RETURNS: TRUE if the variables are adjacent to each other, for example
	//          handle1 is c48, handle2 is c49, handle3 is c50. The variables must be
	//          in ascending order (c50, c49, c48 will return false).
	static bool AreGlobalVarsAdjacent(grcEffectGlobalVar handle1, grcEffectGlobalVar handle2);
	static bool AreGlobalVarsAdjacent(grcEffectGlobalVar handle1, grcEffectGlobalVar handle2, grcEffectGlobalVar handle3);

	// PURPOSE: Get the actual register of a local variable.
	// PARAMS:  handle - Variable handle
	// RETURNS: The register index, like 48 for "c48" or 4 for "s4"
	u8 GetVarRegister(grcEffectVar handle) const;

#if __WIN32PC || RSG_DURANGO || RSG_ORBIS
	// PURPOSE: Get the constant buffer that contains a local variable.
	// PARAMS:  handle - Variable handle
	// RETURNS: Constant buffer pointer
	grcCBuffer *GetParentCBuf(grcEffectVar handle) const;

	// PURPOSE: Get the offset within the constant buffer that contains a local variable.
	// PARAMS:  handle - Variable handle
	// RETURNS: Constant buffer offset
	u32 GetCBufOffset(grcEffectVar handle) const;

	// PURPOSE: Get the constant buffer that contains a global variable.
	// PARAMS:  handle - Variable handle
	// RETURNS: Constant buffer pointer
	static grcCBuffer *GetGlobalParentCBuf(grcEffectGlobalVar handle);

	// PURPOSE: Get the offset within the constant buffer that contains a global variable.
	// PARAMS:  handle - Variable handle
	// RETURNS: Constant buffer offset
	static u32 GetGlobalCBufOffset(grcEffectGlobalVar handle);
#endif // __WIN32PC || RSG_DURANGO

#endif // !__SPU

	// PURPOSE:	Look up a local variable by name (one NOT declared 'shared' in the fx file)
	// PARAMS:	name - Name of the variable to look up (case-sensitive)
	//			mustExist - If true, issue an error if the variable is not found
	// RETURNS:	Variable handle if found, or grcevNONE (zero) if not found.
	// NOTES:	We compare names based on the semantic first, then the actual name.
	grcEffectVar LookupVar(const char *name,bool mustExist = true) const;
	grcEffectVar LookupVarByHash(u32 hashCode) const;

	// PURPOSE:	Look up a local variable by id
	// RETURNS:	Whether the local variable is found.
	bool IsVar(grcEffectVar var) const;

	// PURPOSE:	Set a local variable's value (many overloads)
	// PARAMS:	instanceData - instance data object to fill
	//			handle - Variable handle
	//			value - Value of the variable to set
	//			count - Number of elements in the array (for array overloads)
	// NOTES:	It is undefined behavior (currently fatal on PS3) if you pass down
	//			a value that doesn't match the actual underlying type.
	void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const grcTexture *value);
#if SUPPORT_UAV
	void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const grcBufferUAV *value);
	void SetVarUAV(grcInstanceData& instanceData,grcEffectVar handle,grcBufferUAV *value, int initCount=-1);
	void SetVarUAV(grcInstanceData& instanceData,grcEffectVar handle,const grcTextureUAV *value);
#endif	//SUPPORT_UAV
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,int value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const IntVector &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,float value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const float *value,int count) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vector2 &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vector2 *value,int count) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vector3 &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vector3 *value,int count) const;
	void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Color32 &value) const;
	void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Color32 *value,int count) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vector4 &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vector4 *value,int count) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Matrix34 &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Matrix34 *value,int count) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Matrix44 &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Matrix44 *value,int count) const;

	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vec2V &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vec2V *value,int count) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vec3V &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vec3V *value,int count) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vec4V &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vec4V *value,int count) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vec4f &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vec4f *value,int count) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Mat34V &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Mat34V *value,int count) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Mat44V &value) const;
	inline void SetVar(grcInstanceData& instanceData,grcEffectVar handle,const Mat44V *value,int count) const;

	void RecordSetVar(grcInstanceData& instanceData,grcEffectVar handle,const grcTexture *value);
	void RecordSetVar(grcInstanceData& instanceData,grcEffectVar handle,float value) const;
	void RecordSetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vector2 &value) const;
	void RecordSetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vector3 &value) const;
	void RecordSetVar(grcInstanceData& instanceData,grcEffectVar handle,const Vector4 &value) const;

	// PURPOSE: Set a local variable's value by reference (many overloads)
	// PARAMS:	instanceData - instance data object to fill
	//			handle - Variable handle
	//			value - Value of the variable to set
	//			count - Number of elements in the array (for array overloads)
	// NOTES:	Only types that are never internally translated are supported.  Vector2 arrays are
	//			not supported because their stride doesn't match the final storage method.
	//			Source data must be 16-byte-aligned for DMA on PS3.
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vector2 &value) const { SetLocalCommonByRef(instanceData,handle,&value,8,1); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vector3 &value) const { SetLocalCommonByRef(instanceData,handle,&value,16,1); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vector3 *value,int count) const  { SetLocalCommonByRef(instanceData,handle,value,16,count); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vector4 &value) const { SetLocalCommonByRef(instanceData,handle,&value,16,1); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vector4 *value,int count) const { SetLocalCommonByRef(instanceData,handle,value,16,count); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Matrix34 &value) const { SetLocalCommonByRef(instanceData,handle,&value,64,1); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Matrix34 *value,int count) const { SetLocalCommonByRef(instanceData,handle,value,64,count); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Matrix44 &value) const  { SetLocalCommonByRef(instanceData,handle,&value,64,1); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Matrix44 *value,int count) const { SetLocalCommonByRef(instanceData,handle,value,64,count); }

	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vec2V &value) const { SetLocalCommonByRef(instanceData,handle,&value,16,1); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vec3V &value) const { SetLocalCommonByRef(instanceData,handle,&value,16,1); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vec3V *value,int count) const  { SetLocalCommonByRef(instanceData,handle,value,16,count); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vec4V &value) const { SetLocalCommonByRef(instanceData,handle,&value,16,1); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vec4V *value,int count) const { SetLocalCommonByRef(instanceData,handle,value,16,count); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vec4f &value) const { SetLocalCommonByRef(instanceData,handle,&value,16,1); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Vec4f *value,int count) const { SetLocalCommonByRef(instanceData,handle,value,16,count); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Mat34V &value) const { SetLocalCommonByRef(instanceData,handle,&value,64,1); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Mat34V *value,int count) const { SetLocalCommonByRef(instanceData,handle,value,64,count); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Mat44V &value) const  { SetLocalCommonByRef(instanceData,handle,&value,64,1); }
	void SetVarByRef(grcInstanceData& instanceData,grcEffectVar handle,const Mat44V *value,int count) const { SetLocalCommonByRef(instanceData,handle,value,64,count); }


	// PURPOSE:	Set a local variable's value (many overloads)
	// PARAMS:	handle - Variable handle
	//			value - Value of the variable to set
	//			count - Number of elements in the array (for array overloads)
	// NOTES:	It is undefined behavior (currently fatal on PS3) if you pass down
	//			a value that doesn't match the actual underlying type.
	//			This version uses the instance data object associated with the actual effect.
#if !__SPU
	inline void SetVar(grcEffectVar handle,const grcTexture *value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,float value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const float *value,int count) { SetVar(m_InstanceData,handle,value,count); }
	inline void SetVar(grcEffectVar handle,const Vector2 &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Vector2 *value,int count) { SetVar(m_InstanceData,handle,value,count); }
	inline void SetVar(grcEffectVar handle,const Vector3 &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Vector3 *value,int count) { SetVar(m_InstanceData,handle,value,count); }
	inline void SetVar(grcEffectVar handle,const Color32 &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Color32 *value,int count) { SetVar(m_InstanceData,handle,value,count); }
	inline void SetVar(grcEffectVar handle,const Vector4 &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Vector4 *value,int count) { SetVar(m_InstanceData,handle,value,count); }
	inline void SetVar(grcEffectVar handle,const Matrix34 &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Matrix34 *value,int count) { SetVar(m_InstanceData,handle,value,count); }
	inline void SetVar(grcEffectVar handle,const Matrix44 &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Matrix44 *value,int count) { SetVar(m_InstanceData,handle,value,count); }

	inline void SetVar(grcEffectVar handle,const Vec2V &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Vec2V *value,int count) { SetVar(m_InstanceData,handle,value,count); }
	inline void SetVar(grcEffectVar handle,const Vec3V &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Vec3V *value,int count) { SetVar(m_InstanceData,handle,value,count); }
	inline void SetVar(grcEffectVar handle,const Vec4V &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Vec4V *value,int count) { SetVar(m_InstanceData,handle,value,count); }
	inline void SetVar(grcEffectVar handle,const Vec4f &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Vec4f *value,int count) { SetVar(m_InstanceData,handle,value,count); }
	inline void SetVar(grcEffectVar handle,const Mat34V &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Mat34V *value,int count) { SetVar(m_InstanceData,handle,value,count); }
	inline void SetVar(grcEffectVar handle,const Mat44V &value) { SetVar(m_InstanceData,handle,value); }
	inline void SetVar(grcEffectVar handle,const Mat44V *value,int count) { SetVar(m_InstanceData,handle,value,count); }

	inline void SetVarByRef(grcEffectVar handle,const Vector2 &value) { SetVarByRef(m_InstanceData,handle,value); }
	inline void SetVarByRef(grcEffectVar handle,const Vector3 &value) { SetVarByRef(m_InstanceData,handle,value); }
	inline void SetVarByRef(grcEffectVar handle,const Vector3 *value,int count) { SetVarByRef(m_InstanceData,handle,value,count); }
	inline void SetVarByRef(grcEffectVar handle,const Vector4 &value) { SetVarByRef(m_InstanceData,handle,value); }
	inline void SetVarByRef(grcEffectVar handle,const Vector4 *value,int count) { SetVarByRef(m_InstanceData,handle,value,count); }
	inline void SetVarByRef(grcEffectVar handle,const Matrix34 &value) { SetVarByRef(m_InstanceData,handle,value); }
	inline void SetVarByRef(grcEffectVar handle,const Matrix34 *value,int count) { SetVarByRef(m_InstanceData,handle,value,count); }
	inline void SetVarByRef(grcEffectVar handle,const Matrix44 &value) { SetVarByRef(m_InstanceData,handle,value); }
	inline void SetVarByRef(grcEffectVar handle,const Matrix44 *value,int count) { SetVarByRef(m_InstanceData,handle,value,count); }

	inline void SetVarByRef(grcEffectVar handle,const Vec2V &value) { SetVarByRef(m_InstanceData,handle,value); }
	inline void SetVarByRef(grcEffectVar handle,const Vec3V &value) { SetVarByRef(m_InstanceData,handle,value); }
	inline void SetVarByRef(grcEffectVar handle,const Vec3V *value,int count) { SetVarByRef(m_InstanceData,handle,value,count); }
	inline void SetVarByRef(grcEffectVar handle,const Vec4V &value) { SetVarByRef(m_InstanceData,handle,value); }
	inline void SetVarByRef(grcEffectVar handle,const Vec4V *value,int count) { SetVarByRef(m_InstanceData,handle,value,count); }
	inline void SetVarByRef(grcEffectVar handle,const Vec4f &value) { SetVarByRef(m_InstanceData,handle,value); }
	inline void SetVarByRef(grcEffectVar handle,const Vec4f *value,int count) { SetVarByRef(m_InstanceData,handle,value,count); }
	inline void SetVarByRef(grcEffectVar handle,const Mat34V &value) { SetVarByRef(m_InstanceData,handle,value); }
	inline void SetVarByRef(grcEffectVar handle,const Mat34V *value,int count) { SetVarByRef(m_InstanceData,handle,value,count); }
	inline void SetVarByRef(grcEffectVar handle,const Mat44V &value) { SetVarByRef(m_InstanceData,handle,value); }
	inline void SetVarByRef(grcEffectVar handle,const Mat44V *value,int count) { SetVarByRef(m_InstanceData,handle,value,count); }
#endif

	// PURPOSE:	Retrieve a local variable's value (many overloads)
	// PARAMS:	handle - Variable handle
	//			value - Value of the variable to set
	//			count - Number of elements in the array (for array overloads)
	// NOTES:	It is undefined behavior (currently fatal on PS3) if you pass down
	//			a value that doesn't match the actual underlying type.
	inline void GetVar(const grcInstanceData &instanceData,grcEffectVar handle,float &value) const;
	inline void GetVar(const grcInstanceData &instanceData,grcEffectVar handle,Vector2 &value) const;
	inline void GetVar(const grcInstanceData &instanceData,grcEffectVar handle,Vector3 &value) const;
	inline void GetVar(const grcInstanceData &instanceData,grcEffectVar handle,Vector4 &value) const;
	inline void GetVar(const grcInstanceData &instanceData,grcEffectVar handle,Matrix34 &value) const;
	inline void GetVar(const grcInstanceData &instanceData,grcEffectVar handle,Matrix44 &value) const;
	// inline void GetVar(const grcInstanceData &instanceData,grcEffectVar handle,void *&value) const;
	void GetVar(const grcInstanceData &instanceData,grcEffectVar handle,grcTexture*&value) const;

#if !__SPU
	// PURPOSE:	Retrieve a local variable's value (many overloads)
	// PARAMS:	handle - Variable handle
	//			value - Value of the variable to set
	//			count - Number of elements in the array (for array overloads)
	// NOTES:	It is undefined behavior (currently fatal on PS3) if you pass down
	//			a value that doesn't match the actual underlying type.
	inline void GetVar(grcEffectVar handle,float &value) const { GetVar(m_InstanceData,handle,value); }
	inline void GetVar(grcEffectVar handle,Vector2 &value) const { GetVar(m_InstanceData,handle,value); }
	inline void GetVar(grcEffectVar handle,Vector3 &value) const { GetVar(m_InstanceData,handle,value); }
	inline void GetVar(grcEffectVar handle,Vector4 &value) const { GetVar(m_InstanceData,handle,value); }
	inline void GetVar(grcEffectVar handle,Matrix34 &value) const { GetVar(m_InstanceData,handle,value); }
	inline void GetVar(grcEffectVar handle,Matrix44 &value) const { GetVar(m_InstanceData,handle,value); }
	// inline void GetVar(grcEffectVar handle,void *&value) const { GetVar(m_InstanceData,handle,value); }

	// RETURNS: Number of local variables in the current effect
	int GetVarCount() const { return m_Locals.GetCount(); }

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	const atArray<grcParameter> &GetLocalVar() const { return m_Locals; }
#endif

	// PURPOSE:	Returns a local variable handle by index (useful for iterating over all variables)
	// PARAM:	idx - Index, must be between zero and GetVarCount()-1.
	// RETURNS:	Variable handle
	static grcEffectVar GetVarByIndex(int idx) { return (grcEffectVar)(idx+1); }

	// PURPOSE:	Retrieves a description of the specified local variable
	// PARAMS:	var - Variable handle
	//			name - Returns the (semantic) name of the variable
	//			type - Returns the type of the variable
	//			annotationCount - Returns the number of annotations on the variable
	//			isGlobal - True if the variable is actually global (not really used)
	//			actualName - If not null, returns the actual name of the variable in the shader code
	//				(this now works properly on all platforms)
	// NOTES:	Strings returned to the caller are static and read-only and should not be freed by the caller.
#if EFFECT_PRESERVE_STRINGS
	void GetVarDesc(grcEffectVar var,const char *&name,VarType &type,int &annotationCount,bool &isGlobal,const char **actualName = NULL) const;
#endif
	void GetVarDesc(grcEffectVar var,u32 &nameHash,VarType &type,u32 *actualNameHash = NULL) const;

	// PURPOSE:	Determine if an annotation exists
	// PARAMS:	var - variable to check
	//			name - Name of annotation to look for
	// RETURNS:	True if annotation with that name exists, else false.
	bool HasAnnotation(grcEffectVar var,const char *name) const;
	bool HasAnnotation(grcEffectVar var,u32 hashcode) const;

	// PURPOSE: Returns data associated with an annotation
	// PARAMS:	var - Local variable containing the annotation we wish to search for
	//			name - Name of the annotation to retrieve the data from
	//			defaultValue - Default value to return if the annoation doesn't exist
	// RETURNS:	Actual data associated with the annotation, or defaultValue if it wasn't found.
	// NOTES:	It is undefined behavior to query data from an annotation that has a different type than you expect.
	//			Strings returned to the caller are static and read-only and should not be freed by the caller.
	const char *GetAnnotationData(grcEffectVar var,const char *name,const char *defaultValue) const;
	const char *GetAnnotationData(grcEffectVar var,u32 hashcode,const char *defaultValue) const;

	// PURPOSE: Returns data associated with an annotation
	// PARAMS:	var - Local variable containing the annotation we wish to search for
	//			name - Name of the annotation to retrieve the data from
	//			defaultValue - Default value to return if the annoation doesn't exist
	// RETURNS:	Actual data associated with the annotation, or defaultValue if it wasn't found.
	// NOTES:	It is undefined behavior to query data from an annotation that has a different type than you expect.
	float GetAnnotationData(grcEffectVar var,const char *name,float defaultValue) const;

	// PURPOSE: Returns data associated with an annotation
	// PARAMS:	var - Local variable containing the annotation we wish to search for
	//			name - Name of the annotation to retrieve the data from
	//			defaultValue - Default value to return if the annoation doesn't exist
	// RETURNS:	Actual data associated with the annotation, or defaultValue if it wasn't found.
	// NOTES:	It is undefined behavior to query data from an annotation that has a different type than you expect.
	int GetAnnotationData(grcEffectVar var,const char *name,int defaultValue) const;

	// PURPOSE:	Sets the default path for all effect files.  Must call this before grcDevice::InitClass.
	// PARAMS:	path - Path to all effect files (default to $/tune/shaders/lib).
	static void SetDefaultPath(const char *path);

	// RETURNS:	Current default path for all effect files.
	static const char *GetDefaultPath() { return sm_DefaultPath; }

	// PURPOSE:	No operation, for backward compatibility
	static void ComputeDefaultGlobals() { }

	// PURPOSE:	Make all globals (that haven't already been changed) reset to their default values.
	static void ResetAllGlobalsToDefaults();

	// PURPOSE: Set all global textures to None in order to avoid GPU crashes on invalid textures
	static void ResetGlobalTextures();

	// PURPOSE: Returns property value, or default if not found.
	int GetPropertyValue(const char *propName,int defaultValue) const;
	float GetPropertyValue(const char *propName,float defaultValue) const;
	const char *GetPropertyValue(const char *propName,const char *defaultValue) const;

	// PURPOSE:	Returns the number of global variables; only valid after call to ComputeDefaultGlobals
	static int GetGlobalVarCount() { return sm_Globals.GetCount(); }

	// PURPOSE: Get sampler state handle associated with a local variable.  Will not work if state is currently pushed.
	grcSamplerStateHandle GetSamplerState(const grcInstanceData &data,grcEffectVar var) const;
	grcSamplerStateHandle GetSamplerState(grcEffectVar var) const { return GetSamplerState(m_InstanceData,var); }

	// PURPOSE: Set new sampler state into effect local variable.
	void SetSamplerState(grcInstanceData &data,grcEffectVar var,grcSamplerStateHandle h);
	void SetSamplerState(grcEffectVar var,grcSamplerStateHandle h) { SetSamplerState(m_InstanceData,var,h); }

	// PURPOSE: Push new sampler state into effect local variable.  Stack is only one element deep.
	void PushSamplerState(grcInstanceData &data,grcEffectVar var,grcSamplerStateHandle h);
	void PushSamplerState(grcEffectVar var,grcSamplerStateHandle h) { PushSamplerState(m_InstanceData,var,h); }

	// PURPOSE: Pop previously pushed sampler state into effect local variable.
	void PopSamplerState(grcInstanceData &data,grcEffectVar var);
	void PopSamplerState(grcEffectVar var) { PopSamplerState(m_InstanceData,var); }

	// PURPOSE: Get sampler state handle associated with a global variable.  Will not work if state is currently pushed.
	grcSamplerStateHandle GetGlobalSamplerState(grcEffectGlobalVar var) const;

	// PURPOSE: Set new sampler state into effect global variable.
	void SetGlobalSamplerState(grcEffectGlobalVar var,grcSamplerStateHandle h);

	// PURPOSE: Push new sampler state into effect global variable.  Stack is only one element deep.
	void PushGlobalSamplerState(grcEffectGlobalVar var,grcSamplerStateHandle h);

	// PURPOSE: Pop previously pushed sampler state into effect global variable.
	void PopGlobalSamplerState(grcEffectGlobalVar var);
#endif

	// PURPOSE:	Retrieves information about the specified global variable
	// PARAMS:	idx - Variable index, between 0 and GetGlobalVarCount-1
	//			name - Pointer into static storage of the variable name
	//			type - variable type
	static void GetGlobalVarDesc(int idx,const char *&name,VarType &type);

	// PURPOSE: Looks up an annotation associated with the specified global variable
	// PARAMS:	idx - Variable index, between 0 and GetGlobalVarCount-1
	//			name - Annotation name to look up
	//			defaultValue - Default value to return if annotation wasn't found
	// RETURNS:	Value associated with the annoation, or the default value if the annotation wasn't found.
	static const char *GetGlobalVarAnnotationData(int idx,const char *name,const char *defaultValue);

	// PURPOSE: Looks up an annotation associated with the specified global variable
	// PARAMS:	idx - Variable index, between 0 and GetGlobalVarCount-1
	//			name - Annotation name to look up
	//			defaultValue - Default value to return if annotation wasn't found
	// RETURNS:	Value associated with the annoation, or the default value if the annotation wasn't found.
	static float GetGlobalVarAnnotationData(int idx,const char *name,float defaultValue);

	// PURPOSE: Looks up an annotation associated with the specified global variable
	// PARAMS:	idx - Variable index, between 0 and GetGlobalVarCount-1
	//			name - Annotation name to look up
	//			defaultValue - Default value to return if annotation wasn't found
	// RETURNS:	Value associated with the annoation, or the default value if the annotation wasn't found.
	static int GetGlobalVarAnnotationData(int idx,const char *name,int defaultValue);

	// RETURNS:	Current time stamp of the effect (timestamp of the file when it was last loaded)
	u64 GetCurrentTimeStamp() const;

#if !__SPU
	// RETURNS:	True if the current timestamp on disc is newer than when we last loaded.
	bool IsOutOfDate() const { return GetCurrentTimeStamp() > m_EffectTimeStamp; }
#endif

	// PURPOSE:	Sets a default render state.  This render state is both immediately made current
	//			and also modifies the default state that an effect will restore to when restoreFlag
	//			is true in BeginDraw().
	// PARAMS:	state - Default state index to set (based on the D3DRS_... constants)
	//			value - Value to set
	// NOTES:	This state management is completely independent of the grcState class (as the effect-based
	//			shaders have always been) so avoid mixing grcState calls with these.
	static void SetDefaultFloatRenderState(grceRenderState state,float value);

	// PURPOSE:	Lookup a technique by name
	// PARAMS:	name - Name of the technique to search for
	//			mustExist - If true, issue a fatal error if the technique is not found
	// RETURNS:	Technique handle, or grcetNONE (zero) if not found.
	grcEffectTechnique LookupTechnique(const char *name,bool mustExist = true) const;

	// PURPOSE:	Lookup a technique by hash code
	// PARAMS:	hash - Hash of the technique to search for
	// RETURNS:	Technique handle, or grcetNONE (zero) if not found.
	grcEffectTechnique LookupTechniqueByHash(u32 hash) const;

	// PURPOSE:	Retrieve a technique by index
	// PARAMS:	idx - Index of the technique to retrieve
	// RETURNS:	Technique handle
	// NOTES:	This is only used as a fallback when setting up technique bindings in the
	//			grmShader code -- if all other searches fail we simply default to the
	//			first technique defined in the file.
	grcEffectTechnique GetTechniqueByIndex(int idx) const { return (grcEffectTechnique) (idx+1); }

	// RETURNS:	Number of techniques in the effect
	int GetTechniqueCount() const { return m_Techniques.GetCount(); }

	// RETURNS:	Name of the technique
#if EFFECT_PRESERVE_STRINGS
	const char *GetTechniqueName(grcEffectTechnique tech) const { return m_Techniques[tech-1].Name; }
#endif

	// RETURNS:	Current bound technique (if any; if zero, means that last BeginDraw returned zero)
	static grcEffectTechnique GetCurrentTechnique() { return sm_CurrentTechniqueHandle; }

#if !__SPU && EFFECT_PRESERVE_STRINGS
	// RETURNS: Count of instanced variables
	int GetInstancedVariableCount() const { return m_VarInfo.GetCount(); }

	// RETURNS:	Instanced variable information for specified variable
	void GetInstancedVariableInfo(int idx,VariableInfo &outInfo) const {  outInfo = m_VarInfo[idx]; }
#if __TOOL
	void GetInstancedVariableInfo(int idx, VariableInfo *&outInfo) {  outInfo = &m_VarInfo[idx]; }
#endif
#endif

	static const int MAX_TECHNIQUE_GROUPS = 64; // Has to be multiple of 4 but not 8 for alignment.

	enum eDrawType
	{ 
		RMC_DRAW, 
		RMC_DRAWSKINNED, 
	#if RAGE_SUPPORT_TESSELLATION_TECHNIQUES
		RMC_DRAW_TESSELLATED, 
		RMC_DRAWSKINNED_TESSELLATED, 
	#endif // RAGE_SUPPORT_TESSELLATION_TECHNIQUES
	#if RAGE_INSTANCED_TECH
		RMC_DRAW_INSTANCED,
		RMC_DRAWSKINNED_INSTANCED,
	#if RAGE_SUPPORT_TESSELLATION_TECHNIQUES
		RMC_DRAW_INSTANCED_TESSELLATED,
		RMC_DRAWSKINNED_INSTANCED_TESSELLATED,
	#endif
	#endif	// instanced enum types must be placed as the last ones
		RMC_COUNT 
	};

	// PURPOSE: Register user defined technique groups.
	// PARAMS:	name - The name of the technique group to use
	// RETURNS: An ID to use as the desired technique group
	// NOTES:	This must be called BEFORE any shaders are loaded/preloaded
	static int RegisterTechniqueGroup(const char *name);

	// RETURNS: An ID of the technique group matching the name, or -1 if no match is found -- SLOW
	static int FindTechniqueGroupId(const char *name);

	static const char* GetTechniqueGroupName(int techGroupId, eDrawType type);

	// RETURNS:	Technique group
	// NOTES:	Invalid technique groups are ignored; so that attempting to use a technique group
	//			registered after an effect was loaded won't crash.  TODO: Are we better off crashing?
	grcEffectTechnique GetDrawTechnique(int group,int type) const { return (grcEffectTechnique) m_TechniqueMap[group][type]; }

	// RETURNS: The total number of technique groups available
	static int GetTechniqueGroupCount() { return sm_TechniqueGroupHashes.GetCount(); }

	// PURPOSE:	Returns the number of render passes used by a particular technique
	// PARAMS:	tech - Technique handle
	// RETURNS:	Number of passes in the associated technique
	int GetPassCount(grcEffectTechnique tech) const { return m_Techniques[tech-1].Passes.GetCount(); }

#if __XENON
	// PURPOSE:	Bind all vertex shaders in the current technique to this vertex declaration.
	//			This improves processing speed and eliminates the possibility of shader patching.
	//			Currently it applies the declaration to all techniques, which is clearly wrong
	//			although we don't often mix skinned and unskinned use on the same shader.
	// PARAMS:	decl - Vertex declaration to use.
	//			stride - Vertex stride to use
	void SetDeclaration(grcVertexDeclaration *decl,int stride,bool isSkinned);

#endif

#if !__SPU
	u32 GetHashCode() const { return m_NameHash; }

	// PURPOSE: Returns vertex declaration mask, indicating format and which channels should be present.
	int GetDcl() const { return m_Dcl; }

#else
	u32 GetHashCode() const
	{
		qword buf;
		u32 ea = (u32)this + OFFSETOF_NAME_HASH;
		sysDmaSmallGetAndWait(&buf, ea&~15, 16, 0);
		return si_to_uint(si_rotqby(buf, si_from_uint(ea)));
	}
#endif

	// PURPOSE:	Makes all default render states current on the device
	static void ApplyDefaultRenderStates();

	// PURPOSE:	Makes all default sampler states current on the device
	static void ApplyDefaultSamplerStates();

	// PURPOSE:	Generic entry point for per-frame bookkeeping
	static void BeginFrame();

	// PURPOSE:	Generic entry point for per-frame bookkeeping
	static void EndFrame();

	static void ResetMatchError() {sm_MatchError = false;}

	static bool GetMatchError() {return sm_MatchError;}

#if !__SPU
	// PURPOSE:	This is a stable small number that can be used for sorting.
	int GetOrdinal() const { return m_Ordinal; }

#if EFFECT_PRESERVE_STRINGS
	static const char* GetCurrentTechniqueName() { return sm_CurrentTechnique ? sm_CurrentTechnique->Name.c_str() : NULL; }
#endif

	const char *GetEffectPath() { return m_EffectPath; }

	const char *GetEffectName() { return m_EffectName; }

	// RETURNS: The specified effect pointer, between 0 and GetEffectCount()-1.
	static grcEffect* GetEffect(int index) { return sm_Effects[index]; }

	// RETURNS:	Number of loaded effect files.
	static int GetEffectCount() { return sm_Effects.GetCount(); }
#endif

#if RSG_PC && !__RESOURCECOMPILER
	static bool UseSourceShader(const char* pszShader);
	static void LoadSourceShader(const char* pszShader, void* &programData, u32 &programSize, const char* pzEntryName, const char* pzShaderModel);
#endif // RSG_PC && !__RESOURCECOMPILER
	static bool GetFullName(const char* pszShaderName, char* pszShaderDestination, int iLength, bool bSourceShader);


	// For glue to device_gcm.cpp
	static u32 GetSpuStateCounts();

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	static grcParameter *GetCBufferGlobal(int i)		{ return &sm_Globals[i]; }
	static int GetCBufferGlobalCount()					{ return sm_Globals.GetCount(); }

	//see ShaderModel50 class constructor for the reason we do +1 to the program index
	const grcComputeProgram* GetComputeProgram(int computeProgramId) const	{ return (m_ComputePrograms.GetCount() > computeProgramId+1) ? &m_ComputePrograms[computeProgramId+1] : NULL; }
	grcComputeProgram* GetComputeProgram(int computeProgramId)				{ return (m_ComputePrograms.GetCount() > computeProgramId+1) ? &m_ComputePrograms[computeProgramId+1] : NULL; }
#endif

#if __ASSERT && !__SPU
	static const char* GetCurrentEffectPath() { return sm_CurrentEffect ? sm_CurrentEffect->m_EffectPath.c_str() : NULL; }

	static bool GetAllowShaderLoading() { return sm_AllowShaderLoading; }
	static void SetAllowShaderLoading(bool b) { sm_AllowShaderLoading = b; }

#else
	static const char* GetCurrentEffectPath() { return NULL; }
#endif // __ASSERT

#if __BANK && !__SPU
	void AddWidgets(bkBank &B) { m_InstanceData.AddWidgets(B); }
#endif

	void SetVarUsageFlag(grcEffectVar var, u32 offset, bool isOnlyMtl);
	bool GetVarUsageFlag(grcEffectVar var, u32 offset);

#if __PPU
	// PURPOSE: enable/disable edge viewport frustum culling
	// RETURNS: Previous enable value
	// NOTES: backfacing/frontfacing is controlled by the normal render state.
	static bool SetEdgeViewportCullEnable(bool enable);  
#endif

#if __SPU
	static grcEffect* spuGet_(grcEffect *&ptr);
#endif

#if EFFECT_STENCIL_REF_MASK
	static u8 GetStencilRefMask();
	static void SetStencilRefMask(u8 mask);
#endif //EFFECT_STENCIL_REF_MASK

	static void SetGlobalFloatCommon(int ci,/*const*/ float *value,int count,grcEffect::VarType type);
	void SetLocalCommon(grcInstanceData &instanceData,grcEffectVar handle,const void *value,int stride,int count,bool isFloat=true) const;
	void SetLocalCommonByRef(grcInstanceData &instanceData,grcEffectVar handle,const void *value,int stride,int count) const;
#if (__D3D11 || RSG_ORBIS)
	grcCBuffer *GetCBufferAndOffset(grcEffectVar handle, int &Offset);
#if TRACK_CONSTANT_BUFFER_CHANGES
	static void PrintConstantBufferUsage(bool printUsage);
#endif
#endif // (__D3D11 || RSG_ORBIS)

protected:
	template<class T>
	static void SetGlobalInternal(grcParameter &global, const float *value, int count);

private:
	grcArray<Technique> m_Techniques;					// +0
#if __SPU
	u32 m_Locals_pad[4];
#else
	atArray<grcParameter> m_Locals;						// +8
	atArray<grcCBuffer*>  m_LocalsCBuf;
#endif
	grcArray<grcVertexProgram> m_VertexPrograms;		// +16
	grcArray<grcFragmentProgram> m_FragmentPrograms;	// +24

	u8 m_TechniqueMap[MAX_TECHNIQUE_GROUPS][RMC_COUNT];	// +32

#if EFFECT_TRACK_TEXTURE_ERRORS
	grcProgram::GlobalSubTypesArray m_GlobalSubTypes;
#endif

#if __SPU
	explicit grcEffect(datResource &rsc) : m_Techniques(GRCARRAY_PLACEELEMENTS,rsc), /*m_Locals(GRCARRAY_PLACEELEMENTS,rsc),*/ m_VertexPrograms(GRCARRAY_PLACEELEMENTS,rsc), m_FragmentPrograms(GRCARRAY_PLACEELEMENTS,rsc) { }
	IMPLEMENT_PLACE_INLINE(grcEffect)
#endif

#if __SPU
public:
#endif
	sysMemContainerData m_Container;
	char m_EffectName[40];

#if __PS3
	enum { OFFSETOF_NAME_HASH = 132 + MAX_TECHNIQUE_GROUPS * RMC_COUNT };
#endif

#if !__SPU	// None of this data is necessary on the SPU, so don't transfer it
	grcInstanceData m_InstanceData;
	u64 m_EffectTimeStamp;
	grcString m_EffectPath;
	u32 m_NameHash;
	atArray<grcParameter::Annotation> m_Properties;
	u32 m_Dcl;
	u16 m_Ordinal;
	atArray<VariableInfo> m_VarInfo;

#if __WIN32PC || RSG_DURANGO || RSG_ORBIS
	grcArray<grcComputeProgram>		m_ComputePrograms;
	grcArray<grcDomainProgram>		m_DomainPrograms;
	grcArray<grcGeometryProgram>	m_GeometryPrograms;
	grcArray<grcHullProgram>		m_HullPrograms;
#endif

#if __PS3
	static inline void OffsetofNameHashCompileTimeAssert()
	{
		// If this assert fires, simply adjust OFFSETOF_NAME_HASH to fix it
		CompileTimeAssert(offsetof(grcEffect, m_NameHash) == OFFSETOF_NAME_HASH);
	}
#endif // __PS3
#endif // !__SPU


#if __SPU
public:
#endif
#if !__SPU
	static atFixedArray<grcParameter,grcProgram::c_MaxGlobals> sm_Globals ;
	static atFixedArray<grcCBuffer,grcProgram::c_MaxGlobals> sm_GlobalsCBuf ;

	static atFixedArray<grcEffect*,c_MaxEffects> sm_Effects;
	static grcEffect* AssignToSlot(grcEffect*);
	void UpdateTextureReferences(grcInstanceData &instanceData);
#endif
	static char sm_DefaultPath[128];
#if RSG_PC
	static u32 sm_ShaderQuality;
#endif // RSG_PC

	static DECLARE_MTR_THREAD const Technique *sm_CurrentTechnique;
	static DECLARE_MTR_THREAD grcEffectTechnique sm_CurrentTechniqueHandle;
	static DECLARE_MTR_THREAD int sm_CurrentPassIndex;
	static DECLARE_MTR_THREAD const Technique::Pass *sm_CurrentPass;
	static DECLARE_MTR_THREAD const Technique::Pass *sm_CurrentBind;
	ASSERT_ONLY(static DECLARE_MTR_THREAD const grcEffect *sm_CurrentEffect);
	static DECLARE_MTR_THREAD bool sm_RestoreState;
	static bool sm_MatchError;
	static u16 sm_NextOrdinal;
	ASSERT_ONLY(static bool sm_AllowShaderLoading;);
	static atFixedArray<atHashString[RMC_COUNT], MAX_TECHNIQUE_GROUPS> sm_TechniqueGroupHashes;
#if EFFECT_PRESERVE_STRINGS
	static atFixedArray<atString[RMC_COUNT], MAX_TECHNIQUE_GROUPS> sm_TechniqueGroupNames;
#endif

#if THREADED_SHADER_LOAD
	static void Worker(void*);

	static atArray<sysIpcThreadId>	sm_WorkerThreads;
	static atArray<char*>			sm_LoadList;

public:
	static sysCriticalSectionToken	sm_CsToken;
	static sysCriticalSectionToken	sm_EffectToken;

	struct ShaderThreadPacket 
	{
		u16 offset;
		u16 jobCount;
	};

private:
#endif

public:
	static int GetNumRegisteredTechniqueGroups()
	{
		return sm_TechniqueGroupHashes.GetCount();
	}

#if TRACK_CONSTANT_BUFFER_CHANGES
	static bool sm_TrackConstantBufferUsage;
	static bool sm_PrintConstantBufferUsage;
	static bool sm_TrackConstantBufferResetFrame;
	static bool sm_PrintConstantBufferUsageOnlyChanged;
#endif

#if RSG_PC
	static bool sm_SetUAVNull;
#endif // RSG_PC
};

#if __SPU
CompileTimeAssertSize(grcEffect,160,0);
#elif __PPU
CompileTimeAssertSize(grcEffect,240,0);
#endif

inline void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,float value)
{
	SetGlobalFloatCommon((u32)handle,&value,1,VT_FLOAT);
}

inline void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const Vec2V &value)
{
	SetGlobalFloatCommon((u32)handle,(float*)&value,1,VT_VECTOR2);
}

inline void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const Vec3V &value)
{
	SetGlobalFloatCommon((u32)handle,(float*)&value,1,VT_VECTOR3);
}

inline void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const Vec3V *value,int count)
{
	SetGlobalFloatCommon((u32)handle,(float*)value,count,VT_VECTOR3);
}

inline void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const Vec4V &value)
{
#if __PPU
	if (handle) {
		grcParameter &global = sm_Globals[handle-1];
		Assert(global.Usage);
		SPU_COMMAND(grcEffect__SetGlobalFloatCommon,0,sizeof(Vector4));
		cmd->Register = global.Register;
		cmd->Usage = global.Usage;
		cmd->qwCount = sizeof(value)>>4;
		*(u128*)cmd->alignedPayload = *(u128*)&value;
	}
#else
	SetGlobalFloatCommon((u32)handle,(float*)&value,1,VT_VECTOR4);
#endif
}

inline void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const Vec4V *value,int count)
{
	SetGlobalFloatCommon((u32)handle,(float*)value,count,VT_VECTOR4);
}

inline void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const Mat34V &value)
{
	SetGlobalFloatCommon((u32)handle,(float*)&value,1,VT_MATRIX44);
}

inline void grcEffect::SetGlobalVar__As4x4(grcEffectGlobalVar handle,const Matrix43 *value,int count)
{
	SetGlobalFloatCommon((u32)handle,(float*)value,count,VT_MATRIX43);
}

inline void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const Mat44V &value)
{
	SetGlobalFloatCommon((u32)handle,(float*)&value,1,VT_MATRIX44);
}

inline void grcEffect::SetGlobalVar(grcEffectGlobalVar handle,const Mat44V *value,int count)
{
	SetGlobalFloatCommon((u32)handle,(float*)value,count,VT_MATRIX44);
}

#if !__SPU
inline int grcEffect::GetGlobalVarRegister(grcEffectGlobalVar handle)
{
	return sm_Globals[handle-1].Register;
}

inline bool grcEffect::AreGlobalVarsAdjacent(grcEffectGlobalVar handle1, grcEffectGlobalVar handle2)
{
	return handle1 && handle2 && GetGlobalVarRegister(handle1) + 1 == GetGlobalVarRegister(handle2);
}

inline bool grcEffect::AreGlobalVarsAdjacent(grcEffectGlobalVar handle1, grcEffectGlobalVar handle2, grcEffectGlobalVar handle3)
{
	if (!handle1 || !handle2 || !handle3)
		return false;
	int reg2 = GetGlobalVarRegister(handle2);
	return GetGlobalVarRegister(handle1) + 1 == reg2 && reg2 + 1 == GetGlobalVarRegister(handle3);
}

inline u8 grcEffect::GetVarRegister(grcEffectVar handle) const
{
	return m_Locals[handle-1].Register;
}

#if __WIN32PC || RSG_DURANGO || RSG_ORBIS
inline grcCBuffer *grcEffect::GetParentCBuf(grcEffectVar handle) const
{
	return m_Locals[handle-1].GetParentCBuf();
}

inline u32 grcEffect::GetCBufOffset(grcEffectVar handle) const
{
	return m_Locals[handle-1].GetCBufOffset();
}

inline grcCBuffer *grcEffect::GetGlobalParentCBuf(grcEffectGlobalVar handle)
{
	return sm_Globals[handle-1].GetParentCBuf();

}

inline u32 grcEffect::GetGlobalCBufOffset(grcEffectGlobalVar handle)
{
	return sm_Globals[handle-1].GetCBufOffset();
}

#endif // __WIN32PC || RSG_DURANGO || RSG_ORBIS

#endif // !__SPU


inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,int value) const
{
	SetLocalCommon(instanceData,handle,&value,4,1,false);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const IntVector &value) const
{
	SetLocalCommon(instanceData,handle,value,16,1,false);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,float value) const
{
	SetLocalCommon(instanceData,handle,&value,4,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const float *value,int count) const
{
	SetLocalCommon(instanceData,handle,value,4,count);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vector2 &value) const
{
	SetLocalCommon(instanceData,handle,&value,8,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vector2 *value,int count) const
{
	SetLocalCommon(instanceData,handle,value,8,count);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vector3 &value) const
{
	SetVar(instanceData,handle,&value,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vector3 *value,int count) const
{
	SetLocalCommon(instanceData,handle,value,16,count);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vector4 &value) const
{
	SetLocalCommon(instanceData,handle,&value,16,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vector4 *value,int count) const
{
	SetLocalCommon(instanceData,handle,value,16,count);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Matrix34 &value) const
{
	SetLocalCommon(instanceData,handle,&value,64,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Matrix34 *value,int count) const
{
	SetLocalCommon(instanceData,handle,&value,64,count);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Matrix44 &value) const
{
	SetLocalCommon(instanceData,handle,&value,64,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Matrix44 *value,int count) const
{
	SetLocalCommon(instanceData,handle,value,64,count);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vec2V &value) const
{
	SetLocalCommon(instanceData,handle,&value,16,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vec2V *value,int count) const
{
	SetLocalCommon(instanceData,handle,value,16,count);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vec3V &value) const
{
	SetVar(instanceData,handle,&value,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vec3V *value,int count) const
{
	SetLocalCommon(instanceData,handle,value,16,count);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vec4V &value) const
{
	SetLocalCommon(instanceData,handle,&value,16,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vec4V *value,int count) const
{
	SetLocalCommon(instanceData,handle,value,16,count);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vec4f &value) const
{
	SetLocalCommon(instanceData,handle,&value,16,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Vec4f *value,int count) const
{
	SetLocalCommon(instanceData,handle,value,16,count);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Mat34V &value) const
{
	SetLocalCommon(instanceData,handle,&value,64,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Mat34V *value,int count) const
{
	SetLocalCommon(instanceData,handle,&value,64,count);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Mat44V &value) const
{
	SetLocalCommon(instanceData,handle,&value,64,1);
}

inline void grcEffect::SetVar(grcInstanceData &instanceData,grcEffectVar handle,const Mat44V *value,int count) const
{
	SetLocalCommon(instanceData,handle,value,64,count);
}

inline void grcEffect::GetVar(const grcInstanceData &instanceData,grcEffectVar handle,float &value) const 
{ 
	GetLocalCommon(instanceData,handle,&value,sizeof(value)); 
}

inline void grcEffect::GetVar(const grcInstanceData &instanceData,grcEffectVar handle,Vector2 &value) const 
{ 
	GetLocalCommon(instanceData,handle,&value,8); 
}

inline void grcEffect::GetVar(const grcInstanceData &instanceData,grcEffectVar handle,Vector3 &value) const 
{ 
	GetLocalCommon(instanceData,handle,&value,16); 
}

inline void grcEffect::GetVar(const grcInstanceData &instanceData,grcEffectVar handle,Vector4 &value) const 
{ 
	GetLocalCommon(instanceData,handle,&value,16); 
}

inline void grcEffect::GetVar(const grcInstanceData &instanceData,grcEffectVar handle,Matrix34 &value) const 
{ 
	GetLocalCommon(instanceData,handle,&value,64); 
}

inline void grcEffect::GetVar(const grcInstanceData &instanceData,grcEffectVar handle,Matrix44 &value) const 
{ 
	GetLocalCommon(instanceData,handle,&value,64); 
}

#if __SPU
inline grcEffect* spuGet_(grcEffect*& ptr) { return ptr = grcEffect::spuGet_(ptr); }
#endif


// Implementing grcParameter accessors
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
void grcParameter::SetTexture(grcTexture *pTex)
{
	Assert(Type==grcEffect::VT_TEXTURE);
	Tex = pTex;
}
const grcTexture *grcParameter::GetTexPtr() const
{
	Assert(Type==grcEffect::VT_TEXTURE);
	return Tex;
}

#if SUPPORT_UAV
const grcBufferUAV *grcParameter::GetReadBufferPtr()	const
{
	Assert(Type==grcEffect::VT_STRUCTUREDBUFFER || Type==grcEffect::VT_BYTEADDRESSBUFFER);

	return RO_Buffer;
}

grcBufferUAV *grcParameter::GetWriteBufferPtr(int *initCount)	const
{
	Assert(Type==grcEffect::VT_UAV_STRUCTURED || Type==grcEffect::VT_UAV_BYTEADDRESS);
	if (initCount)
	{
		*initCount = static_cast<int>( reinterpret_cast<size_t>(Data) );
	}
	return RW_Buffer;
}

const grcTextureUAV *grcParameter::GetUnorderedTexturePtr()	const
{
	Assert(Type==grcEffect::VT_UAV_TEXTURE);
	return RW_Texture;
}

int grcParameter::GetStructuredBufferInitCount() const
{
	if (this)
	{
		Assert(Type==grcEffect::VT_STRUCTUREDBUFFER || Type==grcEffect::VT_UAV_STRUCTURED);
		return static_cast<int>( reinterpret_cast<size_t>(Data) );
	}else
	{
		Assertf(this,"UAV parameter not assigned");
		return -1;
	}
}

bool grcParameter::IsStackBuffer() const
{
	return Type==grcEffect::VT_UAV_STRUCTURED && SamplerStateSet;
}

#endif	//SUPPORT_UAV

#if EFFECT_TEXTURE_SUBTYPING
bool grcParameter::IsPureTexture() const
{
#if EFFECT_TEXTURE_SUBTYPING
	return Type == grcEffect::VT_TEXTURE && TextureType == TEXTURE_PURE;
#else
	return false;
#endif
}

bool grcParameter::IsMsaaTexture() const
{
#if EFFECT_TEXTURE_SUBTYPING
	return Type == grcEffect::VT_TEXTURE && TextureType == TEXTURE_MSAA;
#else
	return false;
#endif
}

bool grcParameter::IsComparisonFilter() const
{
#if EFFECT_TEXTURE_SUBTYPING
	if (Type==grcEffect::VT_TEXTURE && SamplerStateSet!=INVALID_STATEBLOCK)
	{
		return ComparisonFilter;
	}
#endif
	return false;
}

#endif // #if EFFECT_TEXTURE_SUBTYPING

bool grcParameter::IsSamplerFor(const grcParameter &param) const
{
	return Type == grcEffect::VT_TEXTURE &&
		Type == param.Type && 
		Register == param.Register &&
		SemanticHash == param.NameHash;
}
#endif	//RSG_PC || RSG_DURANGO || RSG_ORBIS

#if EFFECT_BIND_UNORDERED_RESOURCES
struct BufferCounterTracker
{
	u32 m_Base, m_Count;
	BufferCounterTracker(): m_Base(0),m_Count(0)	{}
	void Update(const u32 slot, grcBufferUAV *const buf, int initCount);
};
#endif	//EFFECT_BIND_UNORDERED_RESOURCES

} // namespace rage

#undef FXFORCEINLINE

#endif // GRCORE_PROGRAM_H 
