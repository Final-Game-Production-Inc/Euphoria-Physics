//
// grcore/effect_load.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "effect.h"
#include "effect_config.h"
#include "effect_internal.h"
#include "stateblock_internal.h"

#include "texture.h"
#include "texturereference.h"

#include "system/cache.h"
#include "diag/art_channel.h"
#include "diag/output.h"
#include "file/asset.h"
#include "file/device.h"
#include "file/limits.h"
#include "file/token.h"
#include "system/magicnumber.h"
#include "system/task.h"
#include "system/rsg_algorithm.h" //for lower bound

#if __WIN32
	#include "device.h"
	#include "system/xtl.h"
	#include "system/d3d9.h"
	#include "grcore/wrapper_d3d.h"

	#if RSG_PC || RSG_DURANGO
		#if (__D3D11)
			#define INITGUID
			#include <Guiddef.h>
			#include "system/d3d11.h"
			#include "texture_d3d11.h"
			#if !__D3D11_1
				#include <D3D11Shader.h>
			#endif // !__D3D11_1
			#if !__D3D11_MONO_DRIVER
				#include <D3DCompiler.h>
			#else
				#include <D3DCompiler_x.h>
			#endif
			#if RSG_DURANGO
				#pragma comment(lib,"dxguid.lib")
			#else
				#if !__D3D11_1
					#pragma comment(lib,"d3dx11.lib")
				#endif // !__D3D11_1
			#endif
			#pragma comment(lib,"d3dcompiler.lib")
		#endif // __D3D11
	#else	// __XENON
		#include "system/xgraphics.h"
	#endif // RSG_PC || RSG_DURANGO
#elif RSG_ORBIS
	#include "device.h"
	#include "grcore/gnmx.h"
	/// #include <gnm/shader.h>
	#if ENABLE_GPU_DEBUGGER
		#if GPU_DEBUGGER_MANUAL_LOAD
			#include "gpu_debugger.h"	//local version
		#else 
			#include <gpu_debugger.h>	//SDK version
		#endif
	#endif //ENABLE_GPU_DEBUGGER
	#if SCE_ORBIS_SDK_VERSION >= (0x00990020u)
		#include "grcore/gnmx/shader_parser.h"
		namespace SHADERPARSER = sce::Gnmx;
	#else
		#include <shader/shader_parser.h>
		namespace SHADERPARSER = sce::Shader::Binary;
	#endif	//SCE_ORBIS_SDK_VERSION
	#include <shader/binary.h>
	#include "fvf.h"
	#include "wrapper_gnm.h"
#endif	// __WIN32, RSG_ORBIS

#if !__TOOL && !__RESOURCECOMPILER && RSG_PC
	#include <d3dx11async.h>
	#define D3DCompileFromMemory D3DX11CompileFromMemory
#endif

// Jimmy's terrain shader requires that parameter order is unchanged, sigh.
// PS3 and any resource paths always require TEXTURES_FIRST
#define TEXTURES_FIRST		(!(HACK_GTA4 && __TOOL))

// Make sure the program refers to the sampler variable instead of the texture object.
// It is compatible with D3D9 and console texture bindings, so using texture objects
// does not require your to change the client code.
#define DX10_TEXTURE_GLUE	(1 && (__D3D11 || RSG_ORBIS))

#if 0 && RSG_PC && __BANK && !__RESOURCECOMPILER
	#define ALLOCATE_PROGRAM(x) sysMemVirtualAllocate(x);
	#define DEALLOCATE_PROGRAM(x) sysMemVirtualFree(x); x = NULL;
#else
	#define ALLOCATE_PROGRAM(x) rage_new u8[x];
	#define DEALLOCATE_PROGRAM(x) delete[] x; x = NULL;
#endif

#if ENABLE_GPU_DEBUGGER
template<typename T>
static void RegisterForDebugger(const T *registers, const sce::Gnmx::ShaderCommonData *common, const rage::grcProgram *highProgram)
{
	if (rage::GRCDEVICE.IsGpuDebugged())
	{
		const char *name = highProgram->GetEntryName();
		ASSERT_ONLY(SceGpuDebuggerErrorCode ret=) sceGpuDebuggerRegisterShaderCode(registers, common->m_shaderSize, name);
		grcAssertf(ret == SCE_OK, "Register shader (%s) failed with code %x", name, ret);
	}
}
#else
static void RegisterForDebugger(const void*, const void*, const void*)	{}
#endif // ENABLE_GPU_DEBUGGER

namespace rage {

using namespace grcRSV;
using namespace grcSSV;

PARAM(useDebugShaders, "Force the usage of debug shaders for easier pixel debugging");
PARAM(featurelevel, "Force a certain shader model in technique remapping code (1000, 1010, 1100)");
NOSTRIP_PARAM(useFinalShaders, "Enable the use of FINAL shaders"); 
PARAM(useSourceShaders, "Force the usage of .hlsl files for easier graphics debugging");

#if __PS3
NOSTRIP_PARAM(useOptimizedShaders, "Enable the use of Optimized shaders"); 
#endif

#if __DEV
PARAM(PrintShaderCacheEntries, "Print shader cache entries");
static char s_CurrentlyLoadingEntry[RAGE_MAX_PATH] = "";
#endif // __DEV

bool grcEffect::sm_MatchError;

#if GRCORE_ON_SPU
extern spuGcmState s_spuGcmState;
#endif

atFixedArray<grcParameter,grcProgram::c_MaxGlobals> grcEffect::sm_Globals ;

grcCBuffer *g_SkinningBase = NULL, *g_MatrixBase = NULL;
#if RAGE_INSTANCED_TECH
grcCBuffer *g_InstBase = NULL, *g_InstUpdateBase = NULL;
#endif
//grcCBuffer *grcEffect::CBInstancing_base = NULL;
//grcCBuffer *grcEffect::CBInstancingUpdate_base = NULL;
atFixedArray<grcCBuffer,grcProgram::c_MaxGlobals> grcEffect::sm_GlobalsCBuf ;

atFixedArray<atHashString[grcEffect::RMC_COUNT],grcEffect::MAX_TECHNIQUE_GROUPS> grcEffect::sm_TechniqueGroupHashes;
#if EFFECT_PRESERVE_STRINGS
atFixedArray<atString[grcEffect::RMC_COUNT],grcEffect::MAX_TECHNIQUE_GROUPS> grcEffect::sm_TechniqueGroupNames;
#endif

u16 grcEffect::sm_NextOrdinal;
ASSERT_ONLY(bool grcEffect::sm_AllowShaderLoading = true;);
atFixedArray<grcEffect*,grcEffect::c_MaxEffects> grcEffect::sm_Effects;
grcEffect *s_FallbackEffect;
bool s_EnableFallback = true;

#if THREADED_SHADER_LOAD
atArray<sysIpcThreadId>	grcEffect::sm_WorkerThreads;
atArray<char*>			grcEffect::sm_LoadList;
sysCriticalSectionToken	grcEffect::sm_CsToken;
sysCriticalSectionToken	grcEffect::sm_EffectToken;
static int g_LoadListCount = 0;
static int g_JobListCount = 0;

PARAM(NoThreadedLoad, "Disable DX11 threaded loading of shaders");
#endif // THREADED_SHADER_LOAD

#define INVALID_RENDER_STATE	0xCDCDCDCD
#define INVALID_SAMPLER_STATE	0xCDCDCDCD
#define INVALID_OFFSET			0xFFFF

#if RSG_PC
u32 grcEffect::sm_ShaderQuality = 3;
#endif // RSG_PC

#if RSG_ORBIS
//TODO: clean up after we all move to 1.7 SDK
static __inline void ParseShader(SHADERPARSER::ShaderInfo* si, const void* data, sce::Gnmx::ShaderType type)
{
#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
	(void)type;
	SHADERPARSER::parseShader(si, data);
#else
	SHADERPARSER::parseShader(si, data, type);
#endif
}
#endif //RSG_ORBIS

#if EFFECT_PRESERVE_STRINGS
static void ReadString(fiStream &S,ConstString &string)
{
	USE_DEBUG_MEMORY();
	char buffer[256];
	int count = S.GetCh();
	S.Read(buffer,count);
	string = buffer;
}
#endif

static void ReadString(fiStream &S,const char *&string)
{
	USE_DEBUG_MEMORY();
	int count = S.GetCh();
	string = rage_new char[count];
	S.Read((char*)string,count);
}

static u32 ReadHashedString(fiStream &S)
{
	int count = S.GetCh();
	char temp[256];
	S.Read(temp,count);
	grcDebugf3("  ReadHashedString[%s]",temp);
	return atStringHash(temp);
}

#if (__D3D11 && 0)
u32 HashShaderInputSignature(D3D11_SIGNATURE_PARAMETER_DESC& oInputDesc, u32 uHash)
{
	u32 newHash = uHash;

	// Hash the string.
	newHash = atStringHash( oInputDesc.SemanticName, newHash );

	// Hash the rest of the structure except for the string pointer.
	newHash = atDataHash( (const unsigned int*)(&oInputDesc.SemanticIndex), sizeof(D3D11_SIGNATURE_PARAMETER_DESC)-sizeof(char*), newHash );

	return newHash;
}
#endif // (__D3D11 && 0)

//This check is needed for EFFECT_CHECK_ENTRY_TYPES
CompileTimeAssert(&reinterpret_cast<grcInstanceData::Entry*>(0)->SamplerStateSet == reinterpret_cast<u8*>(2));


// enum VarType { VT_NONE, VT_INT, VT_FLOAT, VT_VECTOR2, VT_VECTOR3, VT_VECTOR4, 
//	VT_TEXTURE, VT_BOOL, VT_MATRIX34, VT_MATRIX44, VT_STRING,
//#if RSG_PC || RSG_DURANGO || RSG_ORBIS
//VT_INT, VT_INT2, VT_INT3, VT_INT4, VT_STRUCTUREDBUFFER, VT_SAMPLERSTATE,
//#endif
//VT_UNUSED1, VT_UNUSED2, VT_UNUSED3, VT_UNUSED4, //these are to be used for the particle system to add custom vertex buffer values
//#if RSG_PC || RSG_DURANGO || RSG_ORBIS
//VT_UAV_STRUCTURED, VT_UAV_TEXTURE,
//VT_BYTEADDRESSBUFFER, VT_UAV_BYTEADDRESS,
//#endif
 //VT_COUNT /*Must be the last one*/};
u8 g_Float4SizeByType[] = { 
	0,	// NONE
	1,	// INT
	1,	// FLOAT
	1,	// VECTOR2
	1,	// VECTOR3
	1,	// VECTOR4
	0,	// TEXTURE
	1,	// BOOL
	4,	// MATRIX43
	4,	// MATRIX44
	0,	// STRING
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	1,	// INT
	1,	// INT2
	1,	// INT3
	1,	// INT4
	0,  // STRUCTUREDBUFFER
	0,  // SAMPLERSTATE
#endif
	1,	// UNUSED1
	1,	// UNUSED2
	1,	// UNUSED3
	1,	// UNUSED4
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	0,  // UAV_STRUCTURED
	0,  // UAV_TEXTURE
	0,	// BYTEADDRESSBUFFER
	0,	// UAV_BYTEADDRESS
#endif
};
CompileTimeAssert(NELEM(g_Float4SizeByType) == grcEffect::VT_COUNT);

#if !__PS3
u8 g_FloatSizeByType[] = { 
	0,	// NONE
	1,	// INT
	1,	// FLOAT
	2,	// VECTOR2
	3,	// VECTOR3
	4,	// VECTOR4
	0,	// TEXTURE
	1,	// BOOL
	16,	// MATRIX43
	16,	// MATRIX44
	0,	// STRING
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	1,	// INT
	2,	// INT2
	3,	// INT3
	4,	// INT4
	0,  // STRUCTUREDBUFFER
	0,   // SAMPLERSTATE
#endif
	1,	// UNUSED1
	1,	// UNUSED2
	1,	// UNUSED3
	1,	// UNUSED4
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	0,  // UAV_STRUCTURED
	0,  // UAV_TEXTURE
	0,	// BYTEADDRESSBUFFER
	0,	// UAV_BYTEADDRESS
#endif
};
CompileTimeAssert(NELEM(g_FloatSizeByType) == grcEffect::VT_COUNT);
#endif	//!_PS3

PARAM(debugtechniques,"[grcore] Enable loading of shader programs ending in _debug suffix");

#if __PS3
static __THREAD bool isRecordable;
#endif

#if EFFECT_CACHE_PROGRAM
grcProgram::CacheMap grcProgram::sm_ShaderCache[ssCount];
BANK_ONLY(ShaderCacheStat grcProgram::sm_Stats[ssCount+1];)
BANK_ONLY(grcProgram::CacheNameMap grcProgram::sm_ShaderNameCache[ssCount];)
#endif // EFFECT_CACHE_PROGRAM

#if EFFECT_CACHE_LOCALCBS
grcCBuffer::CBufCacheMap grcCBuffer::sm_CBuffer;
#endif // EFFECT_CACHE_LOCALCBS

#if EFFECT_CACHE_PROGRAM
grcShader* grcProgram::FindShader(u32 key, u32 BANK_ONLY(length), ShaderStage eStage)
{
	static bool bEnabled = true;
	if (!bEnabled)
		return NULL;

	Assert(eStage < ssCount);
	CacheMap::iterator it = sm_ShaderCache[eStage].find(key);
	
	if (it != sm_ShaderCache[eStage].end())
	{
		grcShader* pShader = it->second;
#if __BANK
		sm_Stats[eStage].Hits++;
		sm_Stats[eStage].HitBytes += length;
		sm_Stats[ssCount].Hits++;
		sm_Stats[ssCount].HitBytes += length;
#endif // __BANK
		return pShader;
	}
#if __BANK
	sm_Stats[eStage].Misses++;
	sm_Stats[eStage].MissBytes += length;
	sm_Stats[ssCount].Misses++;
	sm_Stats[ssCount].MissBytes += length;
#endif // __BANK
	return NULL;
}

#if __BANK
ShaderCacheStat *grcProgram::GetShaderCacheStats() {
	return sm_Stats;
}
#endif // __BANK
#endif // EFFECT_CACHE_PROGRAM

grcCBuffer* grcCBuffer::LoadConstantBuffer(fiStream &S)
{
	grcCBuffer* pTemp = rage_new grcCBuffer();
	pTemp->Load(S);

#if EFFECT_CACHE_LOCALCBS
	static bool bEnabled = true;
	u32 uKey = ComputeFingerprint(*pTemp);
	CBufCacheMap::iterator it = sm_CBuffer.find(uKey);

	if (bEnabled && it != sm_CBuffer.end())
	{
		// Found match, compare to loaded
		grcCBuffer* pCached = it->second;
		Assertf(pCached->GetSize() == pTemp->GetSize(), "Size Mismatch %s size %d and %d", pCached->GetName(), pCached->GetSize(), pTemp->GetSize());
		delete pTemp;
		return pCached;
	}
	sm_CBuffer[uKey] = pTemp;
#endif // EFFECT_CACHE_LOCALCBS
	pTemp->Init();
	return pTemp;
}

#if EFFECT_CACHE_LOCALCBS
u32 grcCBuffer::ComputeFingerprint(grcCBuffer& oSource)
{
	char szHashName[1024];
	formatf(szHashName, sizeof(szHashName) - 1, "%s.%d.%d.%d.%d.%d.%d.%d", oSource.GetName(), oSource.GetSize(),
		oSource.GetRegister(VS_TYPE), oSource.GetRegister(PS_TYPE), oSource.GetRegister(CS_TYPE), oSource.GetRegister(DS_TYPE), oSource.GetRegister(GS_TYPE), oSource.GetRegister(HS_TYPE));
	u32 uKey = atHashString(szHashName);
	return uKey;
}
#endif // EFFECT_CACHE_LOCALCBS

bool grcProgram::Load(fiStream &S, const char* currentLoadingEffect, bool &isDebugName)
{
	int namelen = S.GetCh();
	char name[RAGE_MAX_PATH];
	S.Read(name,namelen);
	grcDebugf2("Loading program [%s]",name);
	// Build a suitable label name to avoid excessive labels
	char temp[RAGE_MAX_PATH];
	formatf(temp,sizeof(temp),"%s:%s",currentLoadingEffect,name);
	grcDebugf3("grcProgram::Load entry [%s]",temp);
#if __DEV
	safecpy(s_CurrentlyLoadingEntry,temp);
#endif // __DEV
#if EFFECT_PRESERVE_STRINGS
	m_EntryName = temp;
#endif

	isDebugName = strstr(name,"_debug") || strstr(name,"_BANK_ONLY");
	bool isDebug = ((strstr(name,"_debug") && !PARAM_debugtechniques.Get()) || (strstr(name,"_BANK_ONLY") && !__BANK));
#if __PS3
	isRecordable = strstr(name,"_REC");
#endif

	// Read the constants this program references.
	int constantCount = S.GetCh();
	bool result = true;
	if (isDebug) {
		while (constantCount--)
			ReadHashedString(S);
		result = false;
	}
	else
	{
		if (constantCount > 0)
		{
			m_Constants.Reserve(constantCount);
			for (int i=0; i<constantCount; i++)
				m_Constants.Append() = ReadHashedString(S);
		}		
	}

#if __D3D11 || RSG_ORBIS
	int cbCount = S.GetCh();
	while (cbCount--) {
		ReadHashedString(S);
		S.GetCh(); S.GetCh();
	}
#endif

	return result;
}

grcVertexProgram::grcVertexProgram() 
#if __D3D11
: ProgramData(NULL), FirstDecl(NULL)
#elif __WIN32PC
: ProgramData(NULL)
#elif RSG_ORBIS
: FirstDecl(NULL), ProgramVS(NULL), bIsEsShader(false), bIsLsShader(false)
#elif __XENON
: ConstantBase(0), ConstantCount(0), Constants(NULL)
#elif RSG_PS3
: ProgramSize(0), ProgramOffset(0), DefaultConstantRegisters(NULL), DefaultConstantValues(NULL)
#endif // platforms
# if EFFECT_TRACK_INSTANCING_ERRORS
, bIsPerInstance(false)
# endif
{}

#if __PPU

size_t CompressMicrocode(u32 *dest,const u32 *src,size_t sizeInWords)
{
	// Reminder: Inputs are word-swapped.
	u32 destSize = 0;
	while (sizeInWords) {
		u32 constants[4];
		u32 constantCount = 0;
		u32 step = 4;
		u32 insn[4] = {src[0],src[1],src[2],src[3]};
		if (!(insn[2] & 0x8000)) {	// flow control cannot have constants.
			for (int i=1; i<=3; i++) {
				if ((insn[i] & 0x30000) == 0x20000) {	// Constant?
					u32 control = 0;
					for (int j=0; j<4; j++, control<<=1) {
						if (src[4+j]) {
							constants[constantCount++] = src[4+j];
							control |= 0x20000;
						}
					}
					Assert(!(insn[i] & control));
					insn[i] |= control;
					step = 8;
					break;
				}
			}
		}
		// These are extremely common values for insns with only two or three operands.
		// Baseline on gta5, mapping insn[3] 0xC80000001 and 0xC8003FE1 to 0xC8000001: 1572704 bytes in cached shaders
		// Don't map 3FE1 to 0001: 1591504 bytes in cached shaders.
		// Don't handle 0001 in insn[3]: 1926948 bytes in cached shaders
		u32 second = ((insn[2] == 0xC8000001)? 0x8000 : 0) | (insn[3] == 0xC8000001 /*|| insn[3] == 0xC8003FE1*/? 0x4000 : 0);
		dest[destSize++] = insn[0];
		Assert(!(insn[1] & second));
		dest[destSize++] = insn[1] | second;
		if (!(second & 0x8000))
			dest[destSize++] = insn[2];
		if (!(second & 0x4000))
			dest[destSize++] = insn[3];
		for (u32 c=0; c<constantCount; c++)
			dest[destSize++] = constants[c];
		src += step;
		sizeInWords -= step;
	}
	return destSize;
}

const int MaxCachedCount = 1750;
int CachedCount;
struct CacheEntry { u32 Hash, RefCount:8, IsDebug:1, Size:23; void *Data; };
atRangeArray<CacheEntry,MaxCachedCount> s_UcodeCache;
u32 TotalUcode, TotalUcodeWithoutCache, TotalUcodeInDebugHeap;
// Copies the data into reference-counted cache, returning the address of the copy.
void *AllocUcode(void *ucode,u32 ucodeSize,bool mainMemory,bool debugMemory,int* cacheIndex,u32 initialHash)
{
	TotalUcodeWithoutCache += ucodeSize;
	u32 thisHash = atDataHash((const char*)ucode,ucodeSize,initialHash);
	for (int i=0; i<CachedCount; i++)
		if (thisHash == s_UcodeCache[i].Hash && ucodeSize == s_UcodeCache[i].Size) {
			s_UcodeCache[i].RefCount++;
			if (cacheIndex) { *cacheIndex = i; }
			return s_UcodeCache[i].Data;
		}

	// Wasn't in cache.
	if (CachedCount == MaxCachedCount)
		Quitf("Increase MaxCachedCount");

	if (debugMemory)
		TotalUcodeInDebugHeap += ucodeSize;
	else
		TotalUcode += ucodeSize;
	grcDebugf2("...%u bytes total ucode, %u progs",TotalUcode,CachedCount+1);
	s_UcodeCache[CachedCount].Hash = thisHash;
	s_UcodeCache[CachedCount].Size = ucodeSize;
	s_UcodeCache[CachedCount].RefCount = 1;
	s_UcodeCache[CachedCount].IsDebug = debugMemory;
	s_UcodeCache[CachedCount].Data = debugMemory? (sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_DEBUG_VIRTUAL)->RAGE_LOG_ALLOCATE((ucodeSize+15)&~15,16)) : mainMemory? rage_aligned_new(16) u8[ucodeSize] : (u8*)physical_new((ucodeSize + 63) & ~63,64);
	memcpy(s_UcodeCache[CachedCount].Data,ucode,ucodeSize);
	if (cacheIndex) { *cacheIndex = CachedCount; }
	return s_UcodeCache[CachedCount++].Data;
}
void FreeUcode(void *ucode)
{
	for (int i=0; i<CachedCount; i++)
		if (ucode == s_UcodeCache[i].Data) {
			s_UcodeCache[i].RefCount--;
			TotalUcodeWithoutCache -= s_UcodeCache[i].Size;
			if (s_UcodeCache[i].RefCount)
				return;
			if (s_UcodeCache[i].IsDebug)
				TotalUcodeInDebugHeap -= s_UcodeCache[i].Size;
			else
				TotalUcode -= s_UcodeCache[i].Size;
			if (gcm::IsLocalPtr(s_UcodeCache[i].Data))
				physical_delete(s_UcodeCache[i].Data);
			else
				delete[] (char*) s_UcodeCache[i].Data;
			s_UcodeCache[i] = s_UcodeCache[--CachedCount];
			return;
		}
	Assertf(false,"FreeUcode of unknown address %p",ucode);
}
#endif

#define CACHE_SHADERS_ON_XENON (__XENON)

#if CACHE_SHADERS_ON_XENON
class XenonShaderCache 
{
public:
	XenonShaderCache(u32 maxCount_)
	{
		maxCount = maxCount_;
		hashes = rage_new u32[maxCount];
		programs = rage_new D3DResource*[maxCount];
		count = 0;

	}
	D3DResource *Lookup(u32 d,int* cacheIndex = NULL)
	{
		for (u32 i=0; i<count; i++)
			if (hashes[i] == d)
			{
				if (cacheIndex)
					*cacheIndex = (int)i;
				return programs[i];
			}
		return NULL;
	}
	int Insert(u32 d,D3DResource* p)
	{
		if (count == maxCount)
			Quitf("XenonShaderCache needs to be bigger, now %u",maxCount);
		Assert(!Lookup(d));
		hashes[count] = d;
		programs[count] = p;
		count++;
		return (int)count - 1;
	}
	void Release(D3DResource* r)
	{
		for (u32 i=0; i<count; i++) 
		{
			if (programs[i] == r) 
			{
				if (r->Release() == 0) 
				{
					--count;
					hashes[i] = hashes[count];
					programs[i] = programs[count];
				}
				return;
			}
		}
		Errorf("Error in XenonShaderCache::Release?");
	}
private:
	u32 count, maxCount;
	u32 *hashes;
	D3DResource** programs;
};

#define XENON_VERTEXSHADER_CACHE_SIZE 880
#define XENON_PIXELSHADER_CACHE_SIZE 1500

static XenonShaderCache s_VertexShaderCache(XENON_VERTEXSHADER_CACHE_SIZE);
static XenonShaderCache s_PixelShaderCache(XENON_PIXELSHADER_CACHE_SIZE);
#endif	//CACHE_SHADERS_ON_XENON

void grcVertexProgram::Load(fiStream &S, const char* currentLoadingEffect)
{
#if RSG_PC || RSG_DURANGO
	bool createShader = true;
#endif // RSG_PC || RSG_DURANGO
	bool isDebug = false;
	bool saveProgram = grcProgram::Load(S,currentLoadingEffect,isDebug);
#if RSG_ORBIS
	ProgramVS = NULL; //ProgramES = ProgramLS = NULL;
#else
	Program = NULL;
#endif

	u32 programSize;
	S.ReadInt(&programSize,1);
	if (!programSize)
		return;

	u8 *programData = (u8*)ALLOCATE_PROGRAM(programSize);

#if __ASSERT
	int bytesRead = 
#endif
	S.Read(programData, programSize);
	grcAssertf((u32)bytesRead == programSize, "Couldn't read full vertex program from stream. Program size = %d, bytes read = %d", programSize, bytesRead);

#if (RSG_PC && __D3D11) || RSG_DURANGO
	char szShaderModel[16];
	if (programSize) {
		u8 major, minor;
		S.ReadByte(&major,1);
		S.ReadByte(&minor,1);
		createShader = GRCDEVICE.CanLoadShader(major, minor);
		formatf(szShaderModel, sizeof(szShaderModel), "vs_%d_%d", major, minor);
	}
#endif // (RSG_PC && __D3D11) || RSG_DURANGO

#if __WIN32
	// This allows us to read shaders in offline tools that haven't created a D3D device.
	// (and also allows us to read Xenon or PS3 shaders on a PC)
#if RSG_PC || RSG_DURANGO
	ProgramData = programData;

#if !__TOOL && !__RESOURCECOMPILER && RSG_PC && __D3D11
	bool bLoadSourceShader = grcEffect::UseSourceShader(currentLoadingEffect);
	if (bLoadSourceShader)
		grcEffect::LoadSourceShader(currentLoadingEffect, ProgramData, ProgramSize, m_EntryName.c_str(), szShaderModel);
#endif

	if (GRCDEVICE.IsCreated() && saveProgram && createShader)
	{
#if !__RESOURCECOMPILER && !__TOOL
		Program = (ProgramType)CreateShader(GetEntryName(), programData, programSize, ssVertexStage, m_HashKey);
#else
		GRCDEVICE.CreateVertexShader(programData,programSize,(grcVertexShader**)&Program);
#endif
		ProgramSize = programSize;
	}
#else
	if (GRCDEVICE.GetCurrent() && saveProgram)
	{
		CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateVertexShader((DWORD*)programData,&Program));
#if CACHE_SHADERS_ON_XENON
		UINT sizeData;
		Program->GetFunction(NULL,&sizeData);
		char *temp = rage_new char[sizeData];
		Program->GetFunction(temp,&sizeData);
		u32 progHash = XGMicrocodeHashShader(temp,XGMCS_IGNORE_DEBUG_INFO);
		
		int cacheIndex = -1;
		D3DVertexShader *cached = static_cast<D3DVertexShader*>(s_VertexShaderCache.Lookup(progHash,&cacheIndex));

		if(cached)
		{
			// verify if vs programs are really identical:
			UINT cachedSizeData;
			cached->GetFunction(NULL,&cachedSizeData);
			char *cachedTemp = rage_new char[cachedSizeData];
			cached->GetFunction(cachedTemp,&cachedSizeData);
			if(XGMicrocodeCompareShaders(temp,cachedTemp, XGMCS_IGNORE_DEBUG_INFO) != 0)
			{
			#if	__DEV
				if(PARAM_PrintShaderCacheEntries.Get())
				{
					Displayf("Hash clash on SHADERPROGRAM %s,%d,%d", s_CurrentlyLoadingEntry, cacheIndex, cachedSizeData);
				}
			#endif
				// generate new unique hash:
				cached = NULL;
				do
				{
					progHash += u32(XENON_VERTEXSHADER_CACHE_SIZE);
				} while(s_VertexShaderCache.Lookup(progHash) != NULL);
			}

			delete[] cachedTemp;
		}

		delete[] temp;
		
		if (cached)
		{
			cached->AddRef();
			Program->Release();
			Program = cached;
		}
		else
		{
			cacheIndex = s_VertexShaderCache.Insert(progHash,Program);
		}
#if __DEV
		if (PARAM_PrintShaderCacheEntries.Get())
		{
			Displayf("SHADERPROGRAM %s,%d,%d", s_CurrentlyLoadingEntry, cacheIndex, sizeData);
		}
#endif // __DEV
#endif // CACHE_SHADERS_ON_XENON

#if __XENON
		struct header {
			u32 magic;	// 16, 42, 17, 1
			u32 debugLength;
			u32 ucodeLength;
			u32 unknownCount;
			u32 headerSize;
		};
		header *h = (header*) programData;
		u8 *constantTable = programData + h->headerSize + 4;	// skip the length
		struct constantTableHeader {
			u32 _0x1c;
			u32 length1;
			u32 shaderVersion;	// 0xFFFF0300
			u32 constantCount;
			u32 _0x1c_again;
			u32 _0x00;
			u32 someOtherLength;
		};
		struct constantDef {
			u32 nameOffset;		// relative to startOfConstantTable
			u16 registerSet;		// D3DXREGISTER_SET
			u16 registerIndex;
			u16 registerCount;
			u16 unknown;			// usually 0x2?
			u32 typeOffset;		// these can be shared between entries with same type
			u32 defaultValueOffset;
		};
		constantTableHeader *ch = (constantTableHeader*)constantTable;
		constantDef *constants = (constantDef*)(ch+1);
		for (u32 i=0; i<ch->constantCount; i++) {
			const constantDef &c = constants[i];
			if (c.defaultValueOffset) {
				AssertMsg(!ConstantCount,"Vertex program already has a table");
				Assign(ConstantBase,c.registerIndex);
				Assign(ConstantCount,c.registerCount);
				Constants = rage_new float[c.registerCount * 4];
				memcpy(Constants,constantTable + c.defaultValueOffset,c.registerCount * 16);
			}
		}
#endif // __XENON
	}
#endif

#if (RSG_PC || RSG_DURANGO) && !__RESOURCECOMPILER && (__D3D11)

	if (__D3D11)
	{
#if 0
		ID3D11ShaderReflection* pShader = NULL;
		HRESULT hRes =( D3DReflect( programData, programSize, IID_ID3D11ShaderReflection, (void**) &pShader) );
		if (FAILED(hRes))
		{
			Quitf("Cannot load D3D11ReflectShader entry point");
		}		
		D3D11_SHADER_DESC desc;
		pShader->GetDesc(&desc);

		Displayf("SHADER %s",m_EntryName.c_str());
		for (u32 iIndex = 0; iIndex < desc.InputParameters; iIndex++)
		{
			D3D11_SIGNATURE_PARAMETER_DESC oInputDesc;
			AssertVerify(SUCCEEDED(pShader->GetInputParameterDesc(iIndex, &oInputDesc)));
			m_uInputSignatureHash = HashShaderInputSignature(oInputDesc, m_uInputSignatureHash);
			Displayf("Name %s Index %u Reg %u Sys Type %d Comp Type %d Mask %d, RW Mask %d, ", 
				oInputDesc.SemanticName, oInputDesc.SemanticIndex, oInputDesc.Register, oInputDesc.SystemValueType,
				oInputDesc.ComponentType, oInputDesc.Mask, oInputDesc.ReadWriteMask);
		}
		Displayf("constant buffers");
		for (u32 i = 0; i < desc.ConstantBuffers; i++)
		{
			ID3D11ShaderReflectionConstantBuffer *cb = pShader->GetConstantBufferByIndex(i);
			D3D11_SHADER_BUFFER_DESC desc;
			cb->GetDesc(&desc);
			Displayf("%d. [%s] %d vars, %d bytes, flags = %x",i,desc.Name,desc.Variables,desc.Size,desc.uFlags);

			for (u32 j=0; j<desc.Variables; j++) 
			{
				ID3D11ShaderReflectionVariable *v = cb->GetVariableByIndex(j);
				D3D11_SHADER_VARIABLE_DESC desc;
				v->GetDesc(&desc);
				Displayf("  %d. %s offset %u size %u",j,desc.Name,desc.StartOffset,desc.Size);
			}

		}
		Displayf("%d bound resources",desc.BoundResources);
		for (u32 j=0; j<desc.BoundResources; j++)
		{
			D3D11_SHADER_INPUT_BIND_DESC oBindDesc;
			pShader->GetResourceBindingDesc(j, &oBindDesc);
			Displayf(" %d. %s/%d",j,oBindDesc.Name,oBindDesc.BindPoint);
		}
		AssertVerify(pShader->Release() == 0);
#endif
	}
	else
#endif // RSG_PC || RSG_DURANGO
	{
		DEALLOCATE_PROGRAM(programData);
#if RSG_PC || RSG_DURANGO
		ProgramData = NULL;
#endif // RSG_PC || RSG_DURANGO
	}

#elif __PPU
	if (saveProgram)
		Program = (ProgramType) programData;
	else {
		delete[] programData;
		programSize = 0;
	}
	ProgramSize = programSize;
#elif RSG_ORBIS

	SHADERPARSER::ShaderInfo si;
	sce::Shader::Binary::Header *const h = (sce::Shader::Binary::Header*)programData;
	bIsEsShader = bIsLsShader = false;

	switch (h->m_shaderTypeInfo.m_vsInfo.m_vertexShaderVariant)
	{
		case sce::Shader::Binary::kVertexVariantVertex:
			ParseShader(&si, programData, sce::Gnmx::kVertexShader);
			break;
		case sce::Shader::Binary::kVertexVariantExport:
			ParseShader(&si, programData, sce::Gnmx::kExportShader);
			bIsEsShader = true;
			break;
		case sce::Shader::Binary::kVertexVariantLocal:
			ParseShader(&si, programData, sce::Gnmx::kLocalShader);
			bIsLsShader = true;
			break;
		default:
			Assertf(false,"Vertex Program: only vertex/export/local variants are supported.\n");
	}

	(void)saveProgram;
	ProgramSize = programSize;

#if EFFECT_CACHE_PROGRAM
	m_HashKey = ComputeFingerprint(programData, ProgramSize);
	grcShader* pShader = FindShader(m_HashKey, programSize, bIsEsShader ? ssExportStage : (bIsLsShader ? ssGeometryStage : ssVertexStage));
#else
	m_HashKey = 0;
	grcShader* pShader = NULL;
#endif
	if (pShader == NULL)
	{
		void *gpuData = allocateVideoPrivateMemory(si.m_gpuShaderCodeSize,sce::Gnm::kAlignmentOfShaderInBytes);
		copyToVideoMemory(gpuData, si.m_gpuShaderCode, si.m_gpuShaderCodeSize);

		if (bIsEsShader)
		{
			// export shader
			ProgramES = si.m_esShader;
			ProgramES->patchShaderGpuAddress(gpuData);
			GRCDEVICE.SetEsGsRingBufferData(ProgramES->m_memExportVertexSizeInDWord);
			RegisterForDebugger(&ProgramES->m_esStageRegisters, &ProgramES->m_common, this);
#if EFFECT_CACHE_PROGRAM
			sm_ShaderCache[ssExportStage][m_HashKey] = (grcShader*)ProgramES;
#endif // #if EFFECT_CACHE_PROGRAM
		}
		else if (bIsLsShader)
		{
			// export shader
			ProgramLS = si.m_lsShader;
			ProgramLS->patchShaderGpuAddress(gpuData);
			RegisterForDebugger(&ProgramLS->m_lsStageRegisters, &ProgramLS->m_common, this);
#if EFFECT_CACHE_PROGRAM
			sm_ShaderCache[ssGeometryStage][m_HashKey] = (grcShader*)ProgramLS;
#endif // #if EFFECT_CACHE_PROGRAM
		}
		else
		{
			// vertex shader
			ProgramVS = si.m_vsShader;
			ProgramVS->patchShaderGpuAddress(gpuData);
			RegisterForDebugger(&ProgramVS->m_vsStageRegisters, &ProgramVS->m_common, this);
#if EFFECT_CACHE_PROGRAM
			sm_ShaderCache[ssVertexStage][m_HashKey] = (grcShader*)ProgramVS;
#endif // EFFECT_CACHE_PROGRAM
		}
	}
	else
	{
		if (bIsEsShader)
		{
			ProgramES = (sce::Gnmx::EsShader*)pShader;
		}
		else if (bIsLsShader)
		{
			ProgramLS = (sce::Gnmx::LsShader*)pShader;
		}
		else
		{
			ProgramVS = (sce::Gnmx::VsShader*)pShader;
		}
	}

	int vstCount = bIsLsShader ? ProgramLS->m_numInputSemantics : bIsEsShader ? ProgramES->m_numInputSemantics : ProgramVS->m_numInputSemantics;
	const sce::Gnm::VertexInputSemantic *vst = bIsLsShader ? ProgramLS->getInputSemanticTable() : bIsEsShader ? ProgramES->getInputSemanticTable() : ProgramVS->getInputSemanticTable();

#if !__NO_OUTPUT
	for (int i=0; i<vstCount; i++)
	{
		grcDebugf2("vst %d nelem %d semantic %d VGPR %d",i, vst[i].m_sizeInElements, vst[i].m_semantic, vst[i].m_vgpr);
	}
#endif // !__NO_OUTPUT

	sce::Shader::Binary::Program binary;
	binary.loadFromMemory(programData,programSize);
	FVF = 0; ImmFVF = 0; InputCount = binary.m_numInputAttributes;
	memset(SemanticRemap,0xFF,sizeof(SemanticRemap));
	memset(VsharpRemap,0xFF,sizeof(VsharpRemap));
#if EFFECT_TRACK_INSTANCING_ERRORS
	bIsPerInstance = false;
#endif

	for (int i=0; i<binary.m_numInputAttributes; i++) {
		sce::Shader::Binary::Attribute &a = binary.m_inputAttributes[i];
		const char *sem = a.getSemanticName();
		int bit = -1;
		grcDebugf2("ia %d %s:%s (%s) semanticIndex %d resourceIndex %d",i,a.getName(),sem,sce::Shader::Binary::getPsslSemanticString((sce::Shader::Binary::PsslSemantic)a.m_psslSemantic),a.m_semanticIndex,a.m_resourceIndex);
		// TODO: Use hashcodes, or compute this offline
		if (a.m_psslSemantic == sce::Shader::Binary::kSemanticSPosition || !strcmp(sem,"POSITION"))
			bit = grcFvf::grcfcPosition;
		else if (a.m_psslSemantic == sce::Shader::Binary::kSemanticSVertexId)
			;//skip
		else if (a.m_psslSemantic == sce::Shader::Binary::kSemanticSInstanceId)
#if EFFECT_TRACK_INSTANCING_ERRORS
			bIsPerInstance=true
#endif
			;//skip
		else if (!strcmp(sem,"NORMAL"))
			bit = grcFvf::grcfcNormal, ImmFVF |= 1;
		else if (!strcmp(sem,"COLOR"))
			bit = grcFvf::grcfcDiffuse + a.m_semanticIndex, ImmFVF |= 2;
		else if (!strcmp(sem,"TEXCOORD"))
			bit = grcFvf::grcfcTexture0 + a.m_semanticIndex, ImmFVF |= 4;
		else if (!strcmp(sem,"TANGENT"))
			bit = grcFvf::grcfcTangent0 + a.m_semanticIndex;
		else if (!strcmp(sem,"BINORMAL"))
			bit = grcFvf::grcfcBinormal0 +a.m_semanticIndex;
		else if (!strcmp(sem,"BLENDWEIGHT"))
			bit = grcFvf::grcfcWeight;
		else if (!strcmp(sem,"BLENDINDICES"))
			bit = grcFvf::grcfcBinding;
		else
			Errorf("Orbis: Unrecognized semantic '%s' in shader",sem);
		if (bit != -1) {
			TrapGE(bit, NELEM(SemanticRemap));
			CompileTimeAssert(NELEM(SemanticRemap) == NELEM(VsharpRemap));
			SemanticRemap[bit] = a.m_resourceIndex;
			for (int j=0; j<vstCount; j++) {
				if (a.m_resourceIndex == vst[j].m_semantic) {
					VsharpRemap[bit] = j;
					break;
				}
			}
			grcDebugf2("rage semantic %d remapped to PSSL semantic slot %d / V#%d",bit,SemanticRemap[bit],VsharpRemap[bit]);
			FVF |= (1 << bit);
		}
	}

#if !__NO_OUTPUT
	for (int i=0; i<binary.m_numBuffers; i++) {
		sce::Shader::Binary::Buffer &b = binary.m_buffers[i];
		grcDebugf2("buffer %d, '%s' ri %d, stride %d, %d elements.",i,b.getName(),b.m_resourceIndex,b.m_strideSize,b.m_numElements);
	}

	for (int i=0; i<binary.m_numOutputAttributes; i++) {
		sce::Shader::Binary::Attribute &a = binary.m_outputAttributes[i];
		grcDebugf2("oa %d %s:%s",i,a.getName(),a.getSemanticName());
		Assertf(strncmp(a.getSemanticName(),"POSITION",8),"Likely incorrect vertex output semantic, need to use DECLARE_POSITION in %s",s_CurrentlyLoadingEntry);
	}
#endif // !__NO_OUTPUT

#if ENABLE_LCUE
	if (saveProgram) {
		SRO = rage_new sce::LCUE::ShaderResourceOffsets;
		sce::LCUE::generateSROTable(SRO, GetGnmStage(), GetProgram() );
	}
#endif
#endif

#if __RESOURCECOMPILER || __PS3
#if __RESOURCECOMPILER
	bool isRecordableShader = false;
#else
	bool isRecordableShader = isRecordable;
#endif

	if (g_sysPlatform == platform::PS3) {
		ProgramOffset = 0;
		// Read vertex program configuration
		S.Read(&Configuration,sizeof(Configuration));
		Assign(DefaultConstantCount,S.GetCh());
		u16 *defaultConstantRegisters;
		float *defaultConstantValues;
		if (isRecordableShader) {
			DefaultConstantRegisters = rage_new u16[DefaultConstantCount];
			defaultConstantRegisters = DefaultConstantRegisters;
			DefaultConstantValues = rage_new float[DefaultConstantCount*4];
			defaultConstantValues = DefaultConstantValues;
		}
		else {
			defaultConstantRegisters = Alloca(u16,DefaultConstantCount);
			defaultConstantValues = Alloca(float,DefaultConstantCount*4);
			DefaultConstantRegisters = NULL;
			DefaultConstantValues = NULL;
		}
		S.ReadShort(defaultConstantRegisters,DefaultConstantCount);
		S.Read(defaultConstantValues,DefaultConstantCount*16);	// already byte-swapped on-disc
#if __PS3

		if (saveProgram) {
			const int listSize = 8192;
			uint32_t tempList[listSize] ;
			CellGcmContextData tempData = { tempList, tempList + listSize, tempList, NULL };
			// grcDisplayf("VP slot %d count %d inputmask %x regcount %d",Configuration.instructionSlot,Configuration.instructionCount,Configuration.attributeInputMask,Configuration.registerCount);
			// grcDisplayf("%d default constants, Program=%p",DefaultConstantCount,Program);
			cellGcmSetVertexProgramLoad(&tempData,&Configuration,Program);
			for (u32 i=0; i<DefaultConstantCount; i++)
				cellGcmSetVertexProgramConstants(&tempData,defaultConstantRegisters[i],4,&defaultConstantValues[i<<2]);

#if INLINE_SMALL_VERTEX_PROGRAM_LIMIT
			bool shouldInline = !isRecordableShader && ((char*)tempData.current - (char*)tempData.begin) < INLINE_SMALL_VERTEX_PROGRAM_LIMIT;
#else
			const bool shouldInline = false;
#endif
			if (shouldInline) {
				while ((uint32_t)tempData.current & 15)
					*tempData.current++ = 0;
			}
			else
				cellGcmSetReturnCommand(&tempData);
			u32 sizeData = (char*)tempData.current - (char*)tempData.begin;
			// Only inlined debug programs can use the debug heap (which RSX cannot see)
			int cacheIndex = -1;
			void *program = AllocUcode(tempList, sizeData,true,shouldInline && isDebug,&cacheIndex,0);
#if __DEV
			if (PARAM_PrintShaderCacheEntries.Get())
			{
				Displayf("SHADERPROGRAM %s,%d,%d", s_CurrentlyLoadingEntry, cacheIndex, sizeData);
			}
#endif // __DEV

			ProgramOffset = shouldInline? (u32) program : gcm::MainOffset(program);
			if (!isRecordableShader)
			{
				delete[] Program;
				Program = NULL;
				if (shouldInline)
					ProgramSize = sizeData;
				else
					ProgramSize = 0;
			}
			else
				ProgramSize = 0;
		}
#endif
	}
#endif
}

grcVertexProgram::~grcVertexProgram()
{
#if __WIN32
	if (Program)
	{
#if  __WIN32PC && __RESOURCECOMPILER
		((IUnknown*)Program)->Release();
#else
#if CACHE_SHADERS_ON_XENON
		s_VertexShaderCache.Release(Program);
#else
		Program->Release();
#endif
#endif 			
	}
#if RSG_PC || RSG_DURANGO
	if (ProgramData)
	{
		DEALLOCATE_PROGRAM(ProgramData);
		ProgramData = NULL;
	}
#if (__D3D11)
	DeclSetup *ds = FirstDecl;
	while (ds)
	{
		DeclSetup *next = ds->Next;
		ds->FetchDecl->Release();
		ds->InputLayout->Release();
		delete ds;
		ds = next;
	}
#endif // __D3D11
#endif // __WIN32PC

#elif __PPU
	delete[] DefaultConstantRegisters;
	delete[] DefaultConstantValues;
	delete[] (char*) Program;
	if (ProgramOffset)
		FreeUcode(ProgramSize? (void*)ProgramOffset : gcm::MainPtr(ProgramOffset));
#elif __XENON
	delete[] Constants;
#elif RSG_ORBIS
	freeVideoPrivateMemory(GetProgram());
	bIsEsShader = bIsLsShader = false;
	DeclSetup *ds = FirstDecl;
	while (ds)
	{
		DeclSetup *next = ds->Next;
		delete ds;
		ds = next;
	}
#endif
}

grcFragmentProgram::grcFragmentProgram()
#if RSG_ORBIS
: IsPerSample(false)
#if __ASSERT
, ColorOutputFormat(sce::Gnm::kPsTargetOutputModeNoExports)
#endif
#endif
{
#if __PS3 || __RESOURCECOMPILER
	Configuration.offset = 0;
	CompressedPatchTable = NULL;
	PS3_ONLY( ProgramFlags = 0x0 );
#endif
}


NOTFINAL_ONLY(PARAM(nocompressucode,"Disable microcode compression"));

void grcFragmentProgram::Load(fiStream &S, const char* currentLoadingEffect)
{
	bool isDebug, saveProgram = grcProgram::Load(S,currentLoadingEffect,isDebug);
	u32 programSize;
	S.ReadInt(&programSize,1);
	if (!programSize) {
		Program = NULL;
		return;
	}
	u8 *programData = (u8*)ALLOCATE_PROGRAM(programSize);
#if __ASSERT
	int bytesRead = 
#endif
	S.Read(programData, programSize);
	grcAssertf((u32)bytesRead == programSize, "Couldn't read full fragment program from stream. Program size = %d, bytes read = %d", programSize, bytesRead);
	Program = NULL;

#if __WIN32
	bool createShader = true;
#endif // __WIN32

#if ( RSG_PC && __D3D11 ) || RSG_DURANGO
	if (programSize) {
		u8 major, minor;
		S.ReadByte(&major,1);
		S.ReadByte(&minor,1);
		createShader = GRCDEVICE.CanLoadShader(major,minor);
	}
#endif // ( RSG_PC && __D3D11 ) || RSG_DURANGO

#if __RESOURCECOMPILER || __PS3
	u16 patchTableCount = 0, *patchTable = 0;
	u32 initialHash = 0;
	if (g_sysPlatform == platform::PS3) {
		// Read configuration and patch table
		S.Read(&Configuration,sizeof(Configuration));
		S.ReadShort(&patchTableCount,1);
		patchTable = patchTableCount? Alloca(u16,patchTableCount) : NULL;
		// grcDisplayf("%s: %u bytes in patch table",currentLoadingEffect,patchTableCount*2);
		S.ReadShort(patchTable,patchTableCount);
		/* compress the patch table */
		u8 compressedTable[4096], *ct = compressedTable;
		u16 *st = patchTable;
		int count = patchTableCount;
		while (count) {
			*ct++ = u8(*st);						// register address
			if (u8(*st) == 255) Quitf("can't patch address 255, that's our terminator");
			int repCount = u8(*st++ >> 8);			// repeat count
			std::sort(st,st+repCount);				// sort into ascending order
			if (!repCount || repCount > 32) Quitf("this shouldn't happen %d",repCount);
			u16 nextOffset = *st++;
			if (nextOffset > 0x7FFF) Quitf("this shouldn't happen either %x",nextOffset);
			*ct++ = u8(repCount-1) | u8((nextOffset >> 12) << 5);
			*ct++ = u8(nextOffset >> 4);
			count -= repCount + 1;
			nextOffset += 16;
			while (repCount-- > 1) {
				u16 np = *st++;
				u16 dist = u16((np - nextOffset) >> 4);
				if (dist >= 0xF0) {
					*ct++ = u8(dist >> 8) | 0xF0;
					*ct++ = u8(dist);
				}
				else
					*ct++ = u8(dist);
				nextOffset = np + 16;
			}
		}
		if (patchTableCount) {
			// Terminate the patch table
			*ct++ = 255;
			if (ct > compressedTable + sizeof(compressedTable))
				Quitf("Make compressedTable bigger");
			// static int accumSavings;
			// accumSavings += patchTableCount* 2 - (ct - compressedTable);
			// grcDisplayf("%s: %u bytes compressed down to %u in patch table (%d accum)",currentLoadingEffect,patchTableCount*2,ct - compressedTable,accumSavings);
			size_t ctSize = ct - compressedTable;
			CompressedPatchTable = rage_contained_new u8[ctSize];
			memcpy(CompressedPatchTable, compressedTable, ctSize);
			initialHash = atDataHash((char*)CompressedPatchTable,ctSize);
			CompileTimeAssert(sizeof(Configuration) == 24);
		}
		else
			CompressedPatchTable = NULL;
	}
#endif

#if __WIN32
	// This allows us to read shaders in offline tools that haven't created a D3D device.
	// (and also allows us to read Xenon or PS3 shaders on a PC)
	if (GRCDEVICE.IsCreated() && saveProgram && createShader)
	{
#if RSG_PC || RSG_DURANGO
#if !__RESOURCECOMPILER && !__TOOL
		Program = (ProgramType)CreateShader(GetEntryName(), programData, programSize, ssPixelStage, m_HashKey);
#else
		GRCDEVICE.CreatePixelShader(programData,programSize,(grcPixelShader**)&Program);
#endif
		ProgramSize = programSize;
#else
		CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreatePixelShader((DWORD*)programData,&Program));
#if CACHE_SHADERS_ON_XENON
		UINT sizeData;
		Program->GetFunction(NULL,&sizeData);
		char *temp = rage_new char[sizeData];
		Program->GetFunction(temp,&sizeData);
		u32 progHash = XGMicrocodeHashShader(temp,XGMCS_IGNORE_DEBUG_INFO);

		int cacheIndex = -1;
		D3DPixelShader *cached = static_cast<D3DPixelShader*>(s_PixelShaderCache.Lookup(progHash,&cacheIndex));

		if(cached)
		{
			// verify if ps programs are really identical:
			UINT cachedSizeData;
			cached->GetFunction(NULL,&cachedSizeData);
			char *cachedTemp = rage_new char[cachedSizeData];
			cached->GetFunction(cachedTemp,&cachedSizeData);
			if(XGMicrocodeCompareShaders(temp,cachedTemp, XGMCS_IGNORE_DEBUG_INFO) != 0)
			{
			#if	__DEV
				if(PARAM_PrintShaderCacheEntries.Get())
				{
					Displayf("Hash clash on SHADERPROGRAM %s,%d,%d", s_CurrentlyLoadingEntry, cacheIndex, cachedSizeData);
				}
			#endif

				// generate new unique hash:
				cached = NULL;
				do
				{
					progHash += u32(XENON_PIXELSHADER_CACHE_SIZE);
				} while(s_PixelShaderCache.Lookup(progHash) != NULL);
			}

			delete[] cachedTemp;
		}

		delete[] temp;

		if (cached)
		{
			cached->AddRef();
			Program->Release();
			Program = cached;
		}
		else
		{
			cacheIndex = s_PixelShaderCache.Insert(progHash,Program);
		}
#if __DEV
		if (PARAM_PrintShaderCacheEntries.Get())
		{
			Displayf("SHADERPROGRAM %s,%d,%d", s_CurrentlyLoadingEntry, cacheIndex, sizeData);
		}
#endif // __DEV
#endif // CACHE_SHADERS_ON_XENON
#endif // RSG_PC || RSG_DURANGO
	}
	DEALLOCATE_PROGRAM(programData);
#elif __PPU

	// Look for branches that are eligible for runtime branch elimination, must be done before ucode compression
	// though we could fold it into the compression routine if we want to speed up shader load times.
	// If this becomes a problem we could move this to shader compile-time.
	if ( CanStripBranches(programData,programSize) )
		ProgramFlags |= FP_PROGFLAG_STRIP_BRANCHES;
	else
		ProgramFlags &= ~FP_PROGFLAG_STRIP_BRANCHES;
	
	// Compress ucode
	u32 compBuf[16384/4];
	if (saveProgram) {
		int cacheIndex = -1;
		DEV_ONLY(u32 sizeData = 0;)
		if (NOTFINAL_ONLY(!PARAM_nocompressucode.Get() &&) patchTableCount) {
			// TODO - We do in-place decompression by loading the compressed code as high as possible in the decomp buffer
			// and hoping for the best.  In practice we probably want to either make the decompression buffer an extra
			// kilobyte or two larger than we'd actually support, or refuse to compress ucode above a certain size.
			if (programSize > sizeof(compBuf))
				Quitf("Program in '%s' is over compBuf limit.",S.GetName());
			size_t compSize = CompressMicrocode(compBuf,(u32*)programData,programSize>>2);
			programSize = (compSize << 2) | 1;		// Get size in bytes, mark it compressed
			DEV_ONLY(sizeData = compSize << 2;)
			Program = (ProgramType) AllocUcode(compBuf,compSize << 2,true,isDebug,&cacheIndex,initialHash);
		}
		else
		{
			DEV_ONLY(sizeData = programSize;)
			Program = (ProgramType) AllocUcode(programData,programSize,patchTableCount!=0,patchTableCount!=0 && isDebug,&cacheIndex,initialHash);
		}
		delete[] programData;
#if __DEV
		if (PARAM_PrintShaderCacheEntries.Get())
		{
			Displayf("SHADERPROGRAM %s,%d,%d", s_CurrentlyLoadingEntry, cacheIndex, sizeData);
		}
#endif // __DEV
	}
	else {
		delete[] programData;
		programSize = 0;
	}

	if(programSize > 65535)
		Quitf("Fragment program code is too big (%d) and will overflow grcFragmentProgram::ProgramSize, steal bits from grcFragmentProgram::__Padding byte?",programSize);

	ProgramSize = programSize;

#elif RSG_ORBIS
	SHADERPARSER::ShaderInfo si;
	ParseShader(&si, programData, sce::Gnmx::kPixelShader);
	ProgramSize = programSize;
#if EFFECT_CACHE_PROGRAM
	m_HashKey = ComputeFingerprint(programData, ProgramSize);
	grcShader* pShader = FindShader(m_HashKey, programSize, ssPixelStage);
#else
	m_HashKey = 0;
	grcShader* pShader = NULL;
#endif // #if EFFECT_CACHE_PROGRAM
	if (pShader == NULL)
	{
		void *gpuData = allocateVideoPrivateMemory(si.m_gpuShaderCodeSize,sce::Gnm::kAlignmentOfShaderInBytes);
		copyToVideoMemory(gpuData, si.m_gpuShaderCode, si.m_gpuShaderCodeSize);
		Program = si.m_psShader;
		Program->patchShaderGpuAddress(gpuData);
		(void)saveProgram;

		RegisterForDebugger(&Program->m_psStageRegisters, &Program->m_common, this);
#if EFFECT_CACHE_PROGRAM
		sm_ShaderCache[ssPixelStage][m_HashKey] = (grcShader*)Program;
#endif // EFFECT_CACHE_PROGRAM
	}
	else
	{
		Program = (sce::Gnmx::PsShader*)pShader;
	}

	sce::Shader::Binary::Program binary;
	binary.loadFromMemory(programData,programSize);
	IsPerSample = false;
#if __ASSERT
	ColorOutputFormat = Program->m_psStageRegisters.getTargetOutputMode(0);
#endif // __ASSERT

	for (int i=0; i<binary.m_numInputAttributes; i++) {
		sce::Shader::Binary::Attribute &a = binary.m_inputAttributes[i];
		grcAssertf(strcmp(a.getSemanticName(), "VPOS") && strcmp(a.getSemanticName(), "POSITION") &&
			(strcmp(a.getName(), "pos") || a.m_psslSemantic == sce::Shader::Binary::kSemanticSPosition),
			"Pixel shader (%s) input attribute (%s) should have S_POSITION semantics",
			GetEntryName(), a.getName());
		if (a.m_psslSemantic == sce::Shader::Binary::kSemanticSSampleIndex)
		// || a.m_interpType == sce::Shader::Binary::kFragmentInterpTypeSample	//this enum seems broken on Orbis, value is always 0
		{
			IsPerSample = true;
			//Program->m_psStageRegisters.m_spiBarycCntl |= 1<<16;
		}
#if !__NO_OUTPUT
		const char *sem = a.getSemanticName();
		grcDebugf2("ia %d %s:%s",i,a.getName(),sem);
		Assertf(strncmp(a.getSemanticName(),"POSITION",8),"Likely incorrect pixel input semantic, need to use DECLARE_POSITION in %s",s_CurrentlyLoadingEntry);
	}
	for (int i=0; i<binary.m_numBuffers; i++) {
		sce::Shader::Binary::Buffer &b = binary.m_buffers[i];
		grcDebugf2("buffer %d, '%s' ri %d, stride %d, %d elements.",i,b.getName(),b.m_resourceIndex,b.m_strideSize,b.m_numElements);
	}
#else // !__NO_OUTPUT
	}
#endif // !__NO_OUTPUT

#if ENABLE_LCUE
	if (saveProgram) {
		SRO = rage_new sce::LCUE::ShaderResourceOffsets;
		sce::LCUE::generateSROTable(SRO,sce::Gnm::kShaderStagePs,si.m_psShader);
	}
#endif

#endif

#if __PPU
#if SPU_GCM_FIFO
	Configuration.fragmentControl &= 0xffff7fff; // TXP->TEX demotion
#endif // SPU_GCM_FIFO

	// If we don't need patching, remember the offset permanently.
	if (!patchTableCount && saveProgram) {
		Configuration.offset = gcm::LocalOffset(Program);
		Assert(Configuration.offset);
	}
	else {
		Configuration.offset = 0;
	}
#endif

#if 0
	Printf("Outputs: ");
	static const char *outputs[22] = { "frontdiffuse","frontspecular","backdiffuse","backspecular",
		"fog", "pointsize", "userclip0", "userclip1",
		"userclip2", "userclip3", "userclip4", "userclip5",
		"tex8", "tex9", "tex0", "tex1",
		"tex2", "tex3", "tex4", "tex5",
		"tex6", "tex7" };
	for (int i=0; i<=21; i++) {
		if (attribOutputMask & (1<<i))
			Printf("%s ",outputs[i]);
	}
	Printf("\n");
#endif
}

#if __PPU
// Search for branch instructions in the ucode that are eligible for runtime stripping (cellGcmCgStripBranchesFromFragmentUCode) and make sure there will be room to do the stripping
bool grcFragmentProgram::CanStripBranches(u8 const *pUCode, const u32 nUCodeSize) const
{

	u32 nEQBranchCount=0, nNEQBranchCount=0;
	u32 nNumProgWords = nUCodeSize>>2; //nUCodeSize is in bytes
	u32* pProgramWords = (u32*)pUCode;

	for ( u32 nWordOffset=0; nWordOffset<nNumProgWords; nWordOffset+=4 ) // FP ucode instruction lines are each 16 bytes (as far as I can tell)
	{
		// Based on guess work from looking at GPAD ucode asm and binary
		static const u32 RSX_FPOP_IFBRANCH = 0x00004240; // [branch] if(...)
		static const u32 RSX_FPCONDITION_NEQ0 = 0x4;     // [branch] if(x!=0) or if(x)
		static const u32 RSX_FPCONDITION_EQ0 = 0x8;      // [branch] if(x==0) or if(!x)

		if ( Unlikely(pProgramWords[nWordOffset] == RSX_FPOP_IFBRANCH) ) // found an 'if' branch
		{
			u32 nOpModifier = pProgramWords[nWordOffset+1] & 0xf;
			switch (nOpModifier)
			{
			case RSX_FPCONDITION_NEQ0 : nNEQBranchCount++; break;
			case RSX_FPCONDITION_EQ0  : nEQBranchCount++; break;
			}

			#if __NO_OUTPUT || !EFFECT_PRESERVE_STRINGS
				// Faster branch detection during startup when we won't be outputting the branch count
				if ( nNEQBranchCount || nEQBranchCount ) 
					break;
			#endif
		}
	}


	// If we found any branches...
	if ( nNEQBranchCount || nEQBranchCount )
	{	
		#if !__NO_OUTPUT && EFFECT_PRESERVE_STRINGS
			Displayf( "[RSX Branch Detection] %d branches (NEQ0=%d, EQ0=%d) detected in: \"%s\"", nNEQBranchCount+nEQBranchCount, nNEQBranchCount, nEQBranchCount, m_EntryName.c_str() );
		#endif

		// If we want to strip branches, we need enough room in the fragment program cache for both the stripped and non-stripped ucode. Because
		// we don't know how big the stripped code will be, we'll assume the worst... that it will be as big as the non-stripped code. 
		const u32 nAlignedProgramSize = ((nUCodeSize + 15) & ~15);
		const u32 nCombinedProgramsSize = nUCodeSize + nAlignedProgramSize; // Original + Worst-case-stripped (which must be aligned)
		const  u32 nTotalCacheSize = SPU_FRAGMENT_PROGRAM_CACHE_SIZE;
		if ( Unlikely(nCombinedProgramsSize > nTotalCacheSize) )
		{
			// If you get here, you can generally safely ignore this though performance will suffer. Possible work arounds: refactor the
			// shader so that it's smaller or make the cache (SPU_FRAGMENT_PROGRAM_CACHE_SIZE) bigger.
			#if !__NO_OUTPUT && EFFECT_PRESERVE_STRINGS
				Warningf( "[RSX Branch Detection] ucode is too big for branch stripping (shader=%d, required=%d, cache=%d); stripping disabled for: \"%s\"", nUCodeSize, nCombinedProgramsSize, nTotalCacheSize, m_EntryName.c_str() );
			#endif
			return false;
		}

		return true;
	}

	return false;
}
#endif // __PPU

grcFragmentProgram::~grcFragmentProgram()
{
#if __WIN32
	if (Program)
	{
#if  __WIN32PC && __RESOURCECOMPILER
		((IUnknown*)Program)->Release();
#elif CACHE_SHADERS_ON_XENON
		s_PixelShaderCache.Release(Program);
#else
		Program->Release();
#endif 			
	}	
#elif __PPU
	if (Program)
		FreeUcode(Program);
#endif
#if __PS3 || __RESOURCECOMPILER
	delete[] CompressedPatchTable;
#endif
#if RSG_ORBIS
	freeVideoPrivateMemory(Program);
#endif
}

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
grcComputeProgram::grcComputeProgram() 
{
}

void grcComputeProgram::Load(fiStream &S, const char* currentLoadingEffect)
{
	bool isDebug, saveProgram = grcProgram::Load(S,currentLoadingEffect,isDebug);
	u32 programSize;
	S.ReadInt(&programSize,1);
	if (!programSize) {
		Program = NULL;
		return;
	}
	u8 *programData = (u8*)ALLOCATE_PROGRAM(programSize);
#if __ASSERT
	int bytesRead = 
#endif
	S.Read(programData, programSize);
	grcAssertf((u32)bytesRead == programSize, "Couldn't read full vertex program from stream. Program size = %d, bytes read = %d", programSize, bytesRead);
	Program = NULL;
	// This allows us to read shaders in offline tools that haven't created a D3D device.
	// (and also allows us to read Xenon or PS3 shaders on a PC)
#if RSG_ORBIS
	if (saveProgram)
	{
		SHADERPARSER::ShaderInfo si;
		ParseShader(&si, programData, sce::Gnmx::kComputeShader);
#if EFFECT_CACHE_PROGRAM
		m_HashKey = ComputeFingerprint(programData, ProgramSize);
		grcShader* pShader = FindShader(m_HashKey, programSize, ssComputeStage);
#else
		m_HashKey = 0;
		grcShader* pShader = NULL;
#endif // EFFECT_CACHE_PROGRAM
		if (pShader == NULL)
		{
			ProgramSize = programSize;
			void *gpuData = allocateVideoPrivateMemory(si.m_gpuShaderCodeSize,sce::Gnm::kAlignmentOfShaderInBytes);
			copyToVideoMemory(gpuData, si.m_gpuShaderCode, si.m_gpuShaderCodeSize);
			Program = si.m_csShader;
			Program->patchShaderGpuAddress(gpuData);

			RegisterForDebugger(&Program->m_csStageRegisters, &Program->m_common, this);
	#if ENABLE_LCUE
			SRO = rage_new sce::LCUE::ShaderResourceOffsets;
			sce::LCUE::generateSROTable(SRO,sce::Gnm::kShaderStageCs,si.m_csShader);
	#endif
#if EFFECT_CACHE_PROGRAM
			sm_ShaderCache[ssComputeStage][m_HashKey] = (grcShader*)Program;
#endif // EFFECT_CACHE_PROGRAM
		}
		else
		{
			Program = (sce::Gnmx::CsShader*)pShader;
		}
	}
#else	//platforms
	if (GRCDEVICE.IsCreated() && saveProgram)
	{
#if RSG_PC
		if (GRCDEVICE.SupportsFeature(COMPUTE_SHADER_50))
#endif // RSG_PC
		{
#if !__RESOURCECOMPILER  && !__TOOL
			Program = (ProgramType)CreateShader(GetEntryName(), programData, programSize, ssComputeStage, m_HashKey);
#else
			GRCDEVICE.CreateComputeShader(programData, programSize,(grcComputeShader**)&Program);
#endif
			ProgramSize = programSize;
		}
#if RSG_PC
		else 
		{
			Warningf("Hardware does not support Compute Shader 5.0 - Program %s", currentLoadingEffect);
			Program = NULL;
			ProgramSize = 0;
		}
#endif // RSG_PC
	}
	DEALLOCATE_PROGRAM(programData);
#endif	//platforms
}

grcComputeProgram::~grcComputeProgram()
{
#if __WIN32
	if (Program)
	{
#if  __WIN32PC && __RESOURCECOMPILER
		((IUnknown*)Program)->Release();
#else
		Program->Release();
#endif 			
	}	
#endif
}


grcDomainProgram::grcDomainProgram() 
#if RSG_ORBIS
: ProgramVS(NULL), bIsEsShader(false)
#endif
{
}

void grcDomainProgram::Load(fiStream &S, const char* currentLoadingEffect)
{
	bool isDebug, saveProgram = grcProgram::Load(S,currentLoadingEffect,isDebug);
#if RSG_ORBIS
	ProgramVS = NULL; //ProgramES = NULL;
#else
	Program = NULL;
#endif

	u32 programSize;
	S.ReadInt(&programSize,1);
	if (!programSize)
		return;

	bool createShader=true;
	D3D11_ONLY(createShader = GRCDEVICE.CanLoadShader(5,0));
	if (!createShader)
	{
		int newPos = S.Tell();
		S.Seek(newPos+programSize);
		return;
	}

	u8 *programData = (u8*)ALLOCATE_PROGRAM(programSize);
#if __ASSERT
	int bytesRead = 
#endif
		S.Read(programData, programSize);
	grcAssertf((u32)bytesRead == programSize, "Couldn't read full domain program from stream. Program size = %d, bytes read = %d", programSize, bytesRead);
	// This allows us to read shaders in offline tools that haven't created a D3D device.
	// (and also allows us to read Xenon or PS3 shaders on a PC)
#if RSG_ORBIS
	if (saveProgram)
	{
		SHADERPARSER::ShaderInfo si;
		sce::Shader::Binary::Header *const h = (sce::Shader::Binary::Header*)programData;
		bIsEsShader = false;

		switch (h->m_shaderTypeInfo.m_dsInfo.m_domainShaderVariant)
		{
		case sce::Shader::Binary::kDomainVariantVertex:
			ParseShader(&si, programData, sce::Gnmx::kVertexShader);
			break;
		case sce::Shader::Binary::kDomainVariantExport:
			ParseShader(&si, programData, sce::Gnmx::kExportShader);
			bIsEsShader = true;
			break;
		default:
			Assertf(false,"Domain Program: only vertex/export variants are supported.\n");
		}

		ProgramSize = programSize;
#if EFFECT_CACHE_PROGRAM
		m_HashKey = ComputeFingerprint(programData, ProgramSize);
		grcShader* pShader = FindShader(m_HashKey, programSize, ssDomainStage);
#else
		m_HashKey = 0;
		grcShader* pShader = NULL;
#endif // EFFECT_CACHE_PROGRAM
		if (pShader == NULL)
		{
			void *gpuData = allocateVideoPrivateMemory(si.m_gpuShaderCodeSize,sce::Gnm::kAlignmentOfShaderInBytes);
			copyToVideoMemory(gpuData, si.m_gpuShaderCode, si.m_gpuShaderCodeSize);

			if (bIsEsShader)
			{
				ProgramES = si.m_esShader;
				ProgramES->patchShaderGpuAddress(gpuData);
				GRCDEVICE.SetEsGsRingBufferData(ProgramES->m_memExportVertexSizeInDWord);
				RegisterForDebugger(&ProgramES->m_esStageRegisters, &ProgramES->m_common, this);
#if EFFECT_CACHE_PROGRAM
				sm_ShaderCache[ssDomainStage][m_HashKey] = (grcShader*)ProgramES;
#endif // #if EFFECT_CACHE_PROGRAM
			}else
			{
				ProgramVS = si.m_vsShader;
				ProgramVS->patchShaderGpuAddress(gpuData);
				RegisterForDebugger(&ProgramVS->m_vsStageRegisters, &ProgramVS->m_common, this);
#if EFFECT_CACHE_PROGRAM
				sm_ShaderCache[ssDomainStage][m_HashKey] = (grcShader*)ProgramVS;
#endif // #if EFFECT_CACHE_PROGRAM
			}
		
# if ENABLE_LCUE
			SRO = rage_new sce::LCUE::ShaderResourceOffsets;
			sce::LCUE::generateSROTable(SRO,GetGnmStage(),GetProgram());
# endif
		}
		else
		{
			if (bIsEsShader)
			{
				ProgramES = (sce::Gnmx::EsShader*)pShader;
			}
			else
			{
				ProgramVS = (sce::Gnmx::VsShader*)pShader;
			}
		}
	}
#else	//platforms
	if (GRCDEVICE.IsCreated() && saveProgram)
	{
#if !__RESOURCECOMPILER && !__TOOL
		Program = (ProgramType)CreateShader(GetEntryName(), programData, programSize, ssDomainStage, m_HashKey);
#else
		GRCDEVICE.CreateDomainShader(programData, programSize,(grcDomainShader**)&Program);
#endif
		ProgramSize = programSize;
	}
	DEALLOCATE_PROGRAM(programData);
#endif	//platforms
}

grcDomainProgram::~grcDomainProgram()
{
#if __WIN32
	if (Program)
	{
# if  __WIN32PC && __RESOURCECOMPILER
		((IUnknown*)Program)->Release();
# else
		Program->Release();
# endif 			
	}	
#endif	//__WIN32
}


grcGeometryProgram::grcGeometryProgram() 
{
}

void grcGeometryProgram::Load(fiStream &S, const char* currentLoadingEffect)
{
	bool isDebug, saveProgram = grcProgram::Load(S,currentLoadingEffect,isDebug);
	StreamCount = S.GetCh();//mStreamCount
	Streams = rage_new grcStream[StreamCount];
	for(unsigned int i = 0; i < StreamCount; ++i)
	{
		int namelen = S.GetCh();
		grcAssertf(namelen < 16, "geometry program stream out semantic name should be shorter than 16 charcaters long..."); // maximum semantic name number
		S.Read(Streams[i].mSemanticName, namelen);
		grcDebugf1("Loading stream semantic: [%s]", Streams[i].mSemanticName);
		Streams[i].mSemanticIndex = static_cast<unsigned char>(S.GetCh());
		Streams[i].mStartComponent = static_cast<unsigned char>(S.GetCh());
		Streams[i].mComponentCount = static_cast<unsigned char>(S.GetCh());
		Streams[i].mOutputSlot = static_cast<unsigned char>(S.GetCh());
	}

	u32 programSize;
	S.ReadInt(&programSize,1);
	if (!programSize) {
		Program = NULL;
		return;
	}

	u8 *programData = (u8*)ALLOCATE_PROGRAM(programSize);
#if __ASSERT
	int bytesRead = 
#endif
		S.Read(programData, programSize);
	grcAssertf((u32)bytesRead == programSize, "Couldn't read full geometry program from stream. Program size = %d, bytes read = %d", programSize, bytesRead);
	Program = NULL;

	bool createShader=true;
#if ( RSG_PC && __D3D11 ) || RSG_DURANGO
	if (programSize) {
		u8 major, minor;
		S.ReadByte(&major,1);
		S.ReadByte(&minor,1);
		createShader = GRCDEVICE.CanLoadShader(major,minor);
	}
#endif // ( RSG_PC && __D3D11 ) || RSG_DURANGO
	// This allows us to read shaders in offline tools that haven't created a D3D device.
	// (and also allows us to read Xenon or PS3 shaders on a PC)
#if RSG_ORBIS
	if (saveProgram && createShader)
	{
		Assert(StreamCount == 0);
		SHADERPARSER::ShaderInfo gs_si,vs_si;
		SHADERPARSER::parseGsShader(&gs_si, &vs_si, programData);

		void *gs_gpuData = allocateVideoPrivateMemory(gs_si.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes);
		copyToVideoMemory(gs_gpuData, gs_si.m_gpuShaderCode, gs_si.m_gpuShaderCodeSize);

		void *vs_gpuData = allocateVideoPrivateMemory(vs_si.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes);
		copyToVideoMemory(vs_gpuData, vs_si.m_gpuShaderCode, vs_si.m_gpuShaderCodeSize);

		ProgramSize = programSize;
		Program = gs_si.m_gsShader;
		Program->patchShaderGpuAddresses(gs_gpuData, vs_gpuData);

		GRCDEVICE.SetGsVsRingBufferData(Program->m_memExportVertexSizeInDWord, Program->m_maxOutputVertexCount);
		RegisterForDebugger(&Program->m_gsStageRegisters, &Program->m_common, this);
	}
#else	//platforms
	if (GRCDEVICE.IsCreated() && saveProgram && createShader)
	{
		if (StreamCount == 0)
		{
#if !__RESOURCECOMPILER && !__TOOL
			Program = (ProgramType)CreateShader(GetEntryName(), programData, programSize, ssGeometryStage, m_HashKey);
#else
			GRCDEVICE.CreateGeometryShader(programData, programSize,(grcGeometryShader**)&Program);
#endif
			ProgramSize = programSize;
		} 
		else
		{
			//GRCDEVICE.CreateGeometryShaderWithStreamOutput(programData, programSize, (grcGeometryShader**)&Program);
			ProgramSize = programSize;
		}
	}
	DEALLOCATE_PROGRAM(programData);
#endif	//platforms

}

grcGeometryProgram::~grcGeometryProgram()
{
#if __WIN32
	if (Program)
	{
#if  __WIN32PC && __RESOURCECOMPILER
		((IUnknown*)Program)->Release();
#else
		Program->Release();
#endif 			
	}	
#endif
}







void grcHullProgram::Load(fiStream &S, const char* currentLoadingEffect)
{
	bool isDebug, saveProgram = grcProgram::Load(S,currentLoadingEffect,isDebug);

	u32 programSize;
	S.ReadInt(&programSize,1);
	if (!programSize) {
		Program = NULL;
		return;
	}

	Program = NULL;
	bool createShader=true;
	D3D11_ONLY(createShader = GRCDEVICE.CanLoadShader(5,0));
	if (!createShader)
	{
		int newPos = S.Tell();
		S.Seek(newPos+programSize);
		return;
	}

	u8 *programData = (u8*)ALLOCATE_PROGRAM(programSize);
#if __ASSERT
	int bytesRead = 
#endif
		S.Read(programData, programSize);
	grcAssertf((u32)bytesRead == programSize, "Couldn't read full vertex program from stream. Program size = %d, bytes read = %d", programSize, bytesRead);
	// This allows us to read shaders in offline tools that haven't created a D3D device.
	// (and also allows us to read Xenon or PS3 shaders on a PC)
#if RSG_ORBIS
	if (saveProgram)
	{
		SHADERPARSER::ShaderInfo si;
		ParseShader(&si, programData, sce::Gnmx::kHullShader);
	
		ProgramSize = programSize;
#if EFFECT_CACHE_PROGRAM
		m_HashKey = ComputeFingerprint(programData, ProgramSize);
		grcShader* pShader = FindShader(m_HashKey, programSize, ssHullStage);
#else
		m_HashKey = 0;
		grcShader* pShader = NULL;
#endif // EFFECT_CACHE_PROGRAM
		if (pShader == NULL)
		{
			void *gpuData = allocateVideoPrivateMemory(si.m_gpuShaderCodeSize,sce::Gnm::kAlignmentOfShaderInBytes);
			copyToVideoMemory(gpuData, si.m_gpuShaderCode, si.m_gpuShaderCodeSize);
			Program = si.m_hsShader;
			Program->patchShaderGpuAddress(gpuData);

			RegisterForDebugger(&Program->m_hsStageRegisters, &Program->m_common, this);
#if EFFECT_CACHE_PROGRAM
			sm_ShaderCache[ssHullStage][m_HashKey] = (grcShader*)Program;
#endif // #if EFFECT_CACHE_PROGRAM
		}
		else
		{
			Program = (sce::Gnmx::HsShader*)pShader;
		}
	}
#else	//platforms
	if (GRCDEVICE.IsCreated() && saveProgram)
	{
#if !__RESOURCECOMPILER && !__TOOL
		Program = (ProgramType)CreateShader(GetEntryName(), programData, programSize, ssHullStage, m_HashKey);
#else
		GRCDEVICE.CreateHullShader(programData, programSize,(grcHullShader**)&Program);
#endif
		ProgramSize = programSize;
	}
	DEALLOCATE_PROGRAM(programData);
#endif	//platforms

}

grcHullProgram::~grcHullProgram()
{
#if __WIN32
	if (Program)
	{
#if  __WIN32PC && __RESOURCECOMPILER
		((IUnknown*)Program)->Release();
#else
		Program->Release();
#endif 			
	}	
#endif
}

#else //RSG_PC || RSG_DURANGO || RSG_ORBIS

template<bool HasStreamOut>
void grcPlaceHolderProgram<HasStreamOut>::Load(fiStream &S,const char*loading)
{
	bool isDebug;
	grcProgram::Load(S,loading,isDebug);
	if (HasStreamOut)
	{
		int streamCount = S.GetCh();
		while(--streamCount >= 0)
		{
			const int nameSize = S.GetCh();
			S.Seek( S.Tell() + nameSize + 4 );
		}
	}
	int programSize = 0;
	S.ReadInt(&programSize,1);
	S.Seek( S.Tell() + programSize );
}

#endif // __WIN32PC

grcEffect::grcEffect()
{
#if EFFECT_TRACK_TEXTURE_ERRORS
	memset(m_GlobalSubTypes, 0, sizeof(m_GlobalSubTypes));
#endif
}

#if RSG_PC
void grcEffect::SetShaderQuality(int iQuality)
{
	sm_ShaderQuality = Max(Min(3, iQuality), 0);
}
#endif // RSG_PC


struct LookupByHash {
	bool operator()(grcEffect* const &a,const u32 &b) const {
		return a->GetHashCode() < b;
	}	
};

grcEffect* grcEffect::LookupEffect(u32 hashCode)
{
	// std::binary_search only tells us if the item is present.
	grcEffect **i = std::lower_boundRSG(sm_Effects.begin(),sm_Effects.end(),hashCode,LookupByHash());
	if (i != sm_Effects.end()) {
		if ((*i)->GetHashCode() == hashCode)
			return *i;
	}
	return NULL;
}

grcMaterialLibrary* grcMaterialLibrary::sm_CurrentLibrary;

grcMaterialLibrary* grcMaterialLibrary::Preload(const char *preloadDir, const char* preloadFile /* = "preload" */)
{
	ASSET.PushFolder(preloadDir);
	fiStream *S = ASSET.Open(preloadFile,"list");
	if (S==NULL || (S=fiStream::PreLoad(S))==NULL) {
		ASSET.PopFolder();
		return NULL;
	}
	int count = 0;
	char buf[RAGE_MAX_PATH];
	while (fgetline(buf,sizeof(buf),S))
		if (buf[0] && buf[0] != ';')
		++count;
	S->Seek(0);
	grcMaterialLibrary *lib = rage_new grcMaterialLibrary(count);
	lib->m_RootPath = preloadDir;
	while (fgetline(buf,sizeof(buf),S)) {
		if (!buf[0] || buf[0] == ';')
			continue;
		char *equals = strchr(buf,'=');
		fiStream *S2;
		if (equals) {
			char temp[128];
			*equals++ = 0;
			fiDevice::MakeMemoryFileName(temp,sizeof(temp),equals,strlen(equals),false,buf);
			S2 = fiStream::Open(temp);
		}
		else
			S2 = ASSET.Open(buf,"sps");
		if (S2) {
			fiTokenizer T;
			T.Init(buf,S2);
			grcInstanceData *mtl = rage_new grcInstanceData;
			mtl->Load(buf,T,true);
			mtl->MaterialName = StringDuplicate(buf);
			lib->AddEntry(buf,mtl);
			S2->Close();
		}
		else {
			lib->AddEntry(buf,NULL);
		}
	}
	S->Close();
	ASSET.PopFolder();
	return lib;
}

#if __TOOL
bool grcMaterialLibrary::SaveToPreloadFile(const char* presetName, const char* preloadFile) const
{
	ASSET.PushFolder(m_RootPath);
	
	fiStream *S = ASSET.Open(preloadFile,"list",false,false);
	if (S==NULL) 
	{
		ASSET.PopFolder();
		return NULL;
	}
	S->Seek(S->Size()-1);
	if('\n'!=(char)(S->GetCh()))
		S->Write("\n", 1);
	int written = S->Write(presetName, strlen(presetName));
	S->Close();
	ASSET.PopFolder();
	return written>0;
}
bool grcMaterialLibrary::EraseFromPreloadFile(const char* presetName, const char* preloadFile) const
{
	ASSET.PushFolder(m_RootPath);

	fiStream *S = ASSET.Open(preloadFile,"list",false,false);
	if (S==NULL) 
	{
		ASSET.PopFolder();
		return NULL;
	}
	char buf[RAGE_MAX_PATH];
	atString contentBuffer;
	while (fgetline(buf,sizeof(buf),S)) {
		if (!buf[0] || NULL!=strstr(buf, presetName))
			continue;
		contentBuffer += buf;
		contentBuffer += "\n";
	}
	S->Close();
	S = ASSET.Create(preloadFile,"list");
	bool success = true;
	if (S) {
		int charsWritten = S->WriteTChar(contentBuffer.c_str(), contentBuffer.length());
		success = contentBuffer.length() == (unsigned)charsWritten;
		S->Close();
	}
	ASSET.PopFolder();
	return success;
}
#endif

bool grcMaterialLibrary::Save() const
{
	bool allGood = true;
	ASSET.PushFolder(m_RootPath);
	for (int i=0; i<GetCount(); i++) {
		grcInstanceData *m = GetEntry(i);
		fiStream *S = ASSET.Create(m->GetMaterialName(),"sps");
		if (S) {
			m->Save(S);
			S->Close();
		}
		else
			allGood = false;
	}
	ASSET.PopFolder();
	return allGood;
}

bool grcMaterialLibrary::Save(const char *entry) const
{
	bool allGood = false;
	grcInstanceData *m = LookupLocal(entry);
	if (m) {
		ASSET.PushFolder(m_RootPath);
		fiStream *S = ASSET.Create(m->GetMaterialName(),"sps");
		if (S) {
			m->Save(S);
			S->Close();
			allGood = true;
		}
		ASSET.PopFolder();
	}
	return allGood;
}

bool grcMaterialLibrary::Reload(const char *entry) const
{
	bool allGood = false;
	grcInstanceData *m = LookupLocal(entry);
	if (m) {
		ASSET.PushFolder(m_RootPath);
		fiStream *S = ASSET.Open(m->GetMaterialName(),"sps");
		if (S) {
			fiTokenizer T;
			T.Init(m->GetMaterialName(),S);
			m->Load(m->GetMaterialName(),T,true);
			S->Close();
			allGood = true;
		}
		ASSET.PopFolder();
	}
	return allGood;
}


#if __TOOL
bool grcMaterialLibrary::Destroy(const char * /*presetName*/)
{
	return false;
}


grcInstanceData* grcMaterialLibrary::Create(const char * presetName,const char *effectName) 
{
	// lookup effect...
	// expand the dictionary (not really supposed to happen at runtime but this is for tools -- should mark __TOOL-only)
	grcInstanceData *mtl = rage_new grcInstanceData;
	grcEffect *e = grcEffect::LookupEffect(atStringHash(effectName));
	if(!e)
		return NULL;

	e->Clone(*mtl);
	mtl->MaterialName = StringDuplicate(presetName);
	mtl->Flags |= grcInstanceData::FLAG_MATERIAL;
	return mtl;
}
#endif


grcInstanceData* grcEffect::LookupMaterial(u32 hashCode)
{
	grcInstanceData *result = NULL;
	if (grcMaterialLibrary::GetCurrent())
		result = grcMaterialLibrary::GetCurrent()->Lookup(hashCode);
	return result;
}

grcEffect::~grcEffect()
{
	Shutdown();

	// Find self in table (can't remember our own index because we might have moved)
	grcEffect **i = std::lower_boundRSG(sm_Effects.begin(),sm_Effects.end(),m_NameHash,LookupByHash());
	// Remove self from effect table (but table is kept in sorted order)
	// (check result in case we failed the load and aren't actually in the table yet)
	if (i >= sm_Effects.begin() && i < sm_Effects.end() && *i == this)
		sm_Effects.Delete((int)(i-sm_Effects.begin()));

	if (this == s_FallbackEffect)
		s_FallbackEffect = NULL;
}

grcProgram::grcProgram()
{
#if EFFECT_USE_CONSTANTBUFFERS
	m_numCBuffers = 0;
	memset(m_pCBuffers, 0, sizeof(grcCBuffer*)*EFFECT_CONSTANT_BUFFER_COUNT);

	m_numTexContainers = 0;
	for(int i=0; i<EFFECT_TEXTURE_COUNT + EFFECT_UAV_BUFFER_COUNT; ++i)
		m_pTexContainers[i] = NULL;
#endif // EFFECT_USE_CONSTANTBUFFERS

#if EFFECT_CACHE_PROGRAMRESOURCES
	m_CBufferFingerprint = 0;
	m_ppDeviceCBuffers = NULL;
#endif // EFFECT_CACHE_PROGRAMRESOURCES

#if RSG_ORBIS && ENABLE_LCUE
	SRO = NULL;
#endif
#if EFFECT_TRACK_GLOBALS
	memset(m_GlobalByTextureRegister, 0xFF, sizeof(m_GlobalByTextureRegister));
#endif
}

grcProgram::~grcProgram()
{
#if EFFECT_CACHE_PROGRAMRESOURCES
	delete m_ppDeviceCBuffers;
#endif // EFFECT_USE_CONSTANTBUFFERS

#if RSG_ORBIS && ENABLE_LCUE
	delete SRO;
#endif

#if EFFECT_CACHE_PROGRAM
	// Lazy delete search for all of them - Rare event
	for(u32 uStage = 0; uStage < ssCount; uStage++)
	{
		CacheMap::iterator it = sm_ShaderCache[uStage].find(m_HashKey);
		if (it != sm_ShaderCache[uStage].end())
		{
			sm_ShaderCache[uStage].erase(it);
		}
	}
#endif // EFFECT_CACHE_PROGRAM
}

grcEffect::Technique::Technique() : NameHash(0)
{
}

grcEffect::Technique::~Technique()
{
}

grcParameter::grcParameter() : Annotations(NULL), Data(NULL), Register(0), Usage(0), SamplerStateSet(0)
{
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	CBufferOffset = 0;
	CBufferNameHash = 0;
	CBuf = 0;
#if TRACK_CONSTANT_BUFFER_CHANGES
	m_UsageCount = 0;
#endif
#endif
}

grcParameter::~grcParameter()
{
	if (Annotations != NULL) { delete[] Annotations; Annotations = NULL; }
	if (DataSize > 0) { delete[] (char*) Data; Data = NULL; }
}

grcParameter::Annotation::Annotation() : Type(AT_INT)
{
	String = 0;
}

grcParameter::Annotation::~Annotation()
{
	if (Type==AT_STRING) 
		StringFree(String); 
}



struct SortByHash {
	bool operator()( grcEffect * const &a, grcEffect * const &b ) const {
		return a->GetHashCode() < b->GetHashCode();
	}
};

grcEffect* grcEffect::AssignToSlot(grcEffect *effect)
{
#if THREADED_SHADER_LOAD
	sysCriticalSection cs(sm_CsToken);
#endif

	sm_Effects.Append() = effect;
	std::sort(sm_Effects.begin(),sm_Effects.end(),SortByHash());
	return effect;
}

grcEffect* grcEffect::Create(const char *name)
{
	grcDebugf2("Loading effect '%s'...",name);
	grcEffect *that = LookupEffect(atStringHash(name));
	if (that) {
		return that;
	}

	const bool isFull=sm_Effects.IsFull();
	if (isFull) {
		Quitf(ERR_GFX_EFFECT_1,"Too many effects resident, increase grcEffect:c_MaxEffects");
		return NULL;
	}

	that = rage_new grcEffect;
	Assign(that->m_Ordinal,sm_NextOrdinal);
	if (that->Init(name)) {
		return AssignToSlot(that);
	}
#if __RESOURCECOMPILER
	grcErrorf("Attempting to access shader '%s' failed, data is probably bad.",name);
#endif
	delete that;
	return NULL;
}


grcEffect* grcEffect::CreateWithFallback(const char *name)
{
	grcEffect *result = Create(name);
	if (!result) {
		if( s_EnableFallback ) {
			grcErrorf("YOUR ASSETS SUCK, using a fallback for missing shader '%s'...",name);
			result = s_FallbackEffect;
		}
		else {
			Quitf(ERR_GFX_EFFECT_2,"Failed to create effect %s, did the shader load ok ?",name);
		}
	}
	return result;
}

void grcEffect::SetEnableFallback(bool value)
{
	s_EnableFallback = value;
}

bool grcEffect::Preload(const char *preloadDir, const char* preloadFileName /* = "preload" */)
{
#if THREADED_SHADER_LOAD
	bool doShaderLoad = !PARAM_NoThreadedLoad.Get();
#if RSG_PC
	doShaderLoad = doShaderLoad && GRCDEVICE.GetManufacturer() != NVIDIA;
#endif // RSG_PC
#endif // THREADED_SHADER_LOAD

	ASSET.PushFolder(preloadDir);
	fiStream *S = ASSET.Open(preloadFileName,"list");
	if (S==NULL || (S=fiStream::PreLoad(S))==NULL) {
		ASSET.PopFolder();
		grcErrorf("Failed to read file '%s' from %s",preloadFileName,preloadDir);
		return false;
	}
	char buf[64];
	while (fgetline(buf,sizeof(buf),S)) {
		if (!buf[0] || buf[0] == ';')
			continue;
		if (buf[0] == '@') {
			if (!Preload(buf+1))
				grcErrorf("Unable to process nested file '%s'",buf+1);
			continue;
		}
		if (strrchr(buf,'.'))
			*strrchr(buf,'.') = '\0';

#if THREADED_SHADER_LOAD
		if (doShaderLoad)
		{
			char* shaderEntry = rage_new char[64];
			strncpy(shaderEntry, buf, 63);
			sm_LoadList.PushAndGrow(shaderEntry);
		}
		else
#endif // THREADED_SHADER_LOAD
		{
			grcEffect *effect = Create(buf);
			if (effect) {
				if (!s_FallbackEffect)
					s_FallbackEffect = effect;
			}
			else
			{
				grcErrorf("Failed to load preload shader '%s'.",buf);
			}
		}
	}
	S->Close();

#if THREADED_SHADER_LOAD
	if (doShaderLoad)
	{
		ULONG_PTR processMask = 63;
		ULONG_PTR systemMask = 0;
#if RSG_PC
		GetProcessAffinityMask(GetCurrentProcess(),&processMask,&systemMask);
#elif RSG_DURANGO
		processMask = 63;
		systemMask = 63;
#endif // RSG_PC

		int workerThreadCount = sysTaskManager::GetNumThreads();
		sysIpcPriority workerThreadPriority = PRIO_HIGHEST;
		int workerThreadStackSize = 64*1024;
		s32 cpuIdx = -1;

		g_LoadListCount = sm_LoadList.GetCount();
		g_JobListCount = 0;

		ShaderThreadPacket *packets = (ShaderThreadPacket*)malloc(sizeof(ShaderThreadPacket) * workerThreadCount);

		Assertf(packets, "Could not allocate memory for shader packets.");

		sm_WorkerThreads.Resize(workerThreadCount);

		u16 numJobs = (u16)(g_LoadListCount / workerThreadCount);
		u16 extraJobs = (u16)(g_LoadListCount % workerThreadCount);
		u16 offset = 0;

		for (u16 i = 0; i < workerThreadCount; i++)
		{
			const int maxThreadNameLen = 64;
			char threadName[maxThreadNameLen];
			formatf(threadName, maxThreadNameLen, "[RAGE] D3D10EffectScheduler[%d]", i);

			do
			{
				cpuIdx = (cpuIdx+1)&31;
			}
			while(!((u32)processMask&(1<<u32(cpuIdx))));

			// Tack on the extra jobs to the first packet.
			if (i == 0)
				packets[i].jobCount = numJobs + extraJobs;
			else
				packets[i].jobCount = numJobs;

			packets[i].offset = offset;
			offset += packets[i].jobCount;

			sysIpcThreadFunc workerThreadFunc = grcEffect::Worker;
			sm_WorkerThreads[i] = sysIpcCreateThread(workerThreadFunc, (void*)&packets[i], workerThreadStackSize, workerThreadPriority, threadName, cpuIdx);

			Assertf(sm_WorkerThreads[i] != sysIpcThreadIdInvalid, "Couldn't create worker threads to load D3D10/11 effects!");
		}

		while(true)
		{
			sysCriticalSection cs(sm_CsToken);

			// Break out once all jobs have completed.
			if (g_JobListCount == g_LoadListCount)
				break;
		}

		free(packets);

		for (int i = 0; i < sm_LoadList.GetCount(); i++)
		{
			if (sm_LoadList[i])
				delete [] sm_LoadList[i];
		}

		sm_LoadList.Reset();
		sm_WorkerThreads.Reset();
	}
#endif // __D3D11

	ASSET.PopFolder();
	return true;
}

#if THREADED_SHADER_LOAD
void grcEffect::Worker(void* jobStruct)
{
	u16 jobNum = 0;
	ShaderThreadPacket* packet = (ShaderThreadPacket*)jobStruct;

	while (jobNum != packet->jobCount)
	{
		sysCriticalSection cs(sm_CsToken);
		char* shaderName = sm_LoadList[packet->offset + jobNum];
		cs.Exit();

		grcEffect* effect = Create(shaderName);
		if (effect)
		{
			cs.Enter();
			if (!s_FallbackEffect)
				s_FallbackEffect = effect;
			cs.Exit();
		}
		else
		{
			Assertf(0, "Failed to preload shader %s - Rebuild Shaders", shaderName);
		}

		jobNum++;
	}

	sysCriticalSection cs(sm_CsToken);
	g_JobListCount += jobNum;
}
#endif

void grcEffect::ReloadAll()
{
	for (int i=0; i<sm_Effects.GetCount(); i++)
		if (sm_Effects[i]->IsOutOfDate()) {
			grcDisplayf("Reloading effect program '%s'",sm_Effects[i]->m_EffectPath.c_str());
			sm_Effects[i]->Init(NULL);
		}
}

void grcEffect::UnloadAll()
{
	while(sm_Effects.GetCount())
	{
		delete sm_Effects[sm_Effects.GetCount()-1];
	}
	sm_Globals.clear();
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	grcCBuffer Temp;
	for (s32 iCount = 0; iCount < sm_GlobalsCBuf.GetCount(); iCount++)
	{
		sm_GlobalsCBuf[iCount] = Temp;
	}
#endif // RSG_PC || RSG_DURANGO
}

bool grcEffect::GetFullName(const char* pszShaderName, char* pszShaderDestination, int iLength, bool bSourceShader)
{
	if (pszShaderName) 
	{
		Assertf(!strchr(pszShaderName,'.'),"Don't want an extension in effect name: '%s'",pszShaderName);
		char fullpath[RAGE_MAX_PATH];
		const char *ext = bSourceShader ? "hlsl" : (g_sysPlatform==platform::PS3?"cgx":"fxc");
		ASSET.FullPath(fullpath,sizeof(fullpath),pszShaderName,ext);
#if __WIN32PC && !__RESOURCECOMPILER && 0
		// Figure out version directory to load from - Game specific probably but we should cover most of the cases
		const char *aszName[] =
		{
			"win32_30",			// 0
			"win32_30_atidx9",	// 1
			"win32_30_nvdx9",	// 2
			"win32_30_atidx10", // 3
			"win32_30_nvdx10",	// 4
			"win32_40",			// 5
			"win32_40_atidx10",	// 6
			"win32_40_nvdx10",  // 7
			"win32_41",			// 8
			"win32_41_atidx10", // 9
			"win32_41_nvdx10",  // 10
			"win32_50"			// 11
		};
		u32 uVersion = GRCDEVICE.GetDxFeatureLevel(); //GRCDEVICE.GetDxVersion();
		u32 uIndex = 0;
		// PC TODO - More configurations needed
		if (uVersion < 1000)
		{
			uIndex = 0;

			if (GRCDEVICE.GetManufacturer() == ATI)
			{
				if (!GRCDEVICE.SupportsFeature(VERTEX_TEXTURE_SAMPLING))
				{
					uIndex = 1;
				}
				else
				{
					uIndex = 3;
				}
			}
			else if (GRCDEVICE.GetManufacturer() == NVIDIA)
			{
				if (!GRCDEVICE.SupportsFeature(PROPER_ZSAMPLE))
				{
					uIndex = 2;
				}
				else // if (!GRCDEVICE.SupportsFeature(ZSAMPLE_FROM_VERTEX_SHADER))
				{
					uIndex = 4;
				}
				// PC TODO - Probably need a DX9 - DX11 Path as everything should support this by then
			}
		}
		else if ((uVersion >= 1000) && (uVersion < 1100))
		{
			uIndex = 5;

			if (GRCDEVICE.GetManufacturer() == ATI)
			{
				uIndex = 6;
			}
			else if (GRCDEVICE.GetManufacturer() == NVIDIA)
			{
				uIndex = 7;
			}
		}
		else
		{
			// Assume DX11 only hardware for now
			uIndex = 6; //10; // PC TODO - Shader 5.0 support
		}
#if HACK_GTA4
	uIndex = 0;
#endif // HACK_GTA4
		if (PARAM_useDebugShaders.Get())
		{ // PC TODO - We can add embedded once it works with Terry's tool
			char szTempBuf[256];
			char szDebug[] = "_debug";
			strcpy_s(szTempBuf, sizeof(szTempBuf), aszName[uIndex]);
			strcat_s(szTempBuf, sizeof(szTempBuf), szDebug);
			if (!ASSET.InsertPathElement(fullpath,sizeof(fullpath),szTempBuf))
				return false;
		}
		else
		{
			if (!ASSET.InsertPathElement(fullpath,sizeof(fullpath),aszName[uIndex]))
				return false;
		}
#else
		bool bShaderVersion50 = (strstr(fullpath,"_50") != NULL);

		{
			const char *platformString = NULL;
			if (g_sysPlatform == platform::XENON)
				platformString = "fxl_final";
			else if (g_sysPlatform == platform::PS3)
				platformString = "psn";
			else if (RSG_ORBIS)
				platformString = "orbis";
			else if (RSG_DURANGO)
				platformString = "durango";
			else if (__D3D11)
			{
#if RSG_PC
				if (GRCDEVICE.IsStereoEnabled())
					platformString = "win32_nvstereo";
				else if (GetShaderQuality() == 0)
					platformString = "win32_40_lq";
				else
#endif // RSG_PC
				if (bShaderVersion50)
					platformString = "win32_50";
				else
					platformString = "win32_40";
			}
			else if(__D3D9)
			{
				platformString = "win32_30";
			}

			Assert( platformString != NULL ); 

			// Optionally load "final" shaders. We take this path by default in __FINAL builds but it can
			// always be forced on/off on the command line (regardless of build configuration).
			bool bUseFinalShaders = __FINAL ? true : false;
			if ( PARAM_useFinalShaders.Get() )
			{
				int nParam = 0;
				PARAM_useFinalShaders.Get(nParam);
				bUseFinalShaders = nParam ? true : false;
			}

#if __PS3
			// Optionally load "optimized" shaders. We take this path by default in __FINAL builds but it can
			// always be forced on/off on the command line (regardless of build configuration).
			bool bUseOptimizedShaders = false; /*__FINAL ? true : false;*/ 
			if ( PARAM_useOptimizedShaders.Get() )
			{
				int nParam = 0;
				PARAM_useOptimizedShaders.Get(nParam);
				bUseOptimizedShaders = nParam ? true : false;
			}
#else
			const bool bUseOptimizedShaders = false;
#endif
			char pStrPlatformFolder[32];
			if ( Likely(bUseFinalShaders || bUseOptimizedShaders) )
			{
				Assert( (strlen(platformString) + sizeof("_final") PS3_ONLY(+ sizeof("_optimized"))) <= sizeof(pStrPlatformFolder) );
				strncpy( pStrPlatformFolder, platformString, sizeof(pStrPlatformFolder) );

#if __PS3
				if ( Likely(bUseOptimizedShaders) )
					strncat( pStrPlatformFolder, "_optimized", sizeof("_optimized") );
#endif // __PS3

				if ( Likely(bUseFinalShaders) )
					strncat( pStrPlatformFolder, "_final", sizeof("_final") );

				platformString = &pStrPlatformFolder[0];
			}
#if RSG_PC && !__RESOURCECOMPILER
			if (PARAM_useDebugShaders.Get())
			{
				Assert((strlen(platformString) + sizeof("_debug")) <= sizeof(pStrPlatformFolder));
				strncpy( pStrPlatformFolder, platformString, sizeof(pStrPlatformFolder) );
				strncat( pStrPlatformFolder, "_debug", sizeof("_debug") );

				platformString = &pStrPlatformFolder[0];
			}
			if (bSourceShader && PARAM_useSourceShaders.Get())
			{
				Assert((strlen(platformString) + sizeof("_source")) <= sizeof(pStrPlatformFolder) );
				strncpy( pStrPlatformFolder, platformString, sizeof(pStrPlatformFolder) );

				strncat( pStrPlatformFolder, "_source", sizeof("_source") );
				platformString = &pStrPlatformFolder[0];
			}

#endif // RSG_PC && !__RESOURCECOMPILER
			if (!ASSET.InsertPathElement(fullpath,sizeof(fullpath),platformString))
				return false;
		}
#endif
		formatf(pszShaderDestination, iLength, "%s", fullpath);
		return true;
	}
	return false;
}

bool grcEffect::Init(const char * filename)
{
	Shutdown();

	const char* currentLoadingEffect;

#if THREADED_SHADER_LOAD
	sm_EffectToken.Lock();
#endif

	// Load effect for desired target platform.
	if (filename) 
	{
		char fullpath[RAGE_MAX_PATH];
		if (GetFullName(filename, fullpath, RAGE_MAX_PATH - 1, false))
		{
			m_EffectPath = fullpath;
			const char *srcName = ASSET.FileName(filename); 
			Assertf(strlen(srcName) < sizeof(m_EffectName), "Effect name '%s' is too long for current code (%" SIZETFMT "u).", srcName, sizeof(m_EffectName));
			safecpy(m_EffectName, srcName);
			m_NameHash = atStringHash(m_EffectName);
		}
	}

#if __ASSERT	
	// Debug shaders get pass, so they don't polute the preload list.
	if( strstr(m_EffectName,"debug") == NULL )
	{
		Assertf(sm_AllowShaderLoading,"Loading effect %s while loading is unauthorized",m_EffectName);
	}
#endif // __ASSERT
	fiStream *S = fiStream::Open(m_EffectPath);
	DIAG_CONTEXT_MESSAGE("Loading effect '%s'",m_EffectPath.c_str());
	if (S) 
	{
		if ((S=fiStream::PreLoad(S))==NULL)
		{
#if THREADED_SHADER_LOAD
			sm_EffectToken.Unlock();
#endif
			return false;
		}

		m_EffectTimeStamp = fiDevice::GetDevice(m_EffectPath)->GetFileTime(m_EffectPath);
		int magic = 0x20202020;
		S->ReadInt(&magic,1);
		if (magic != MAKE_MAGIC_NUMBER('r','g','x','e') && magic != MAKE_MAGIC_NUMBER('r','g','x','f'))
		{
			S->Close();
#if THREADED_SHADER_LOAD
			sm_EffectToken.Unlock();
#endif
			Quitf(ERR_GFX_INVALID_SHADER,"%s: Old version of rage effect found (%c%c%c%c).  You need to recompile your shaders!",m_EffectPath.c_str(),EXPAND_MAGIC_NUMBER(magic));
			// return false;
		}
		ASSERT_ONLY(sm_CurrentEffect = this);
		currentLoadingEffect = filename;		// intentionally don't want full path here
		magic >>= 24;		// retain subversion number
#if __DEV
		if (currentLoadingEffect == NULL)		// if the filename was NULL, we can still get the effect name
		{
			static char effectNameBuf[128] = "";
			const char* effectPath = m_EffectPath.c_str();
			const char* effectName = strrchr(effectPath, '/');
			if (effectName == NULL)
				effectName = effectPath; // no '/', just use the full path
			else
				effectName++; // skip past '/'
			strcpy(effectNameBuf, effectName);
			char* effectExt = strrchr(effectNameBuf, '.');
			if (effectExt)
				effectExt[0] = '\0'; // strip extension
			currentLoadingEffect = effectNameBuf;
		}
#endif // __DEV
#if !__TOOL
		sysMemContainer container(m_Container);

		container.Init(EFFECT_CONTAINER_SIZE); // magic number, limits size of SPU transfer area
#endif
#if THREADED_SHADER_LOAD
		sm_EffectToken.Unlock();
#endif
		Load(*S,magic,currentLoadingEffect);
#if THREADED_SHADER_LOAD
		sm_EffectToken.Lock();
#endif
#if !__TOOL
# if !__WIN32PC
		if (container.GetMemoryUsed(-1) >= 8192)
			grcWarningf("%s: %" SIZETFMT "u bytes used in effect container (%u FP's, %u inst data)",m_EffectPath.c_str(),container.GetMemoryUsed(-1),m_FragmentPrograms.GetCount(),m_InstanceData.TotalSize);
# endif
		container.Finalize();
#endif
		currentLoadingEffect = NULL;
		ASSERT_ONLY(sm_CurrentEffect = 0);
		S->Close();

#if THREADED_SHADER_LOAD
		sm_EffectToken.Unlock();
#endif
		return true;
	}
	else 
	{
#if THREADED_SHADER_LOAD
		sm_EffectToken.Unlock();
#endif
		return false;
	}
}


void grcEffect::Technique::Pass::Load(fiStream &S)
{
#if !RSG_PC && !RSG_DURANGO && !RSG_ORBIS
	u8 ComputeProgramIndex;
	u8 DomainProgramIndex;
	u8 GeometryProgramIndex;
	u8 HullProgramIndex;
#endif
	Assign(VertexProgramIndex,S.GetCh());
	Assign(FragmentProgramIndex,S.GetCh());
	Assign(ComputeProgramIndex, S.GetCh());
	Assign(DomainProgramIndex, S.GetCh());
	Assign(GeometryProgramIndex, S.GetCh());
	Assign(HullProgramIndex, S.GetCh());
	int count = S.GetCh();
	grcRenderState *rs = Alloca(grcRenderState,count);
	S.ReadInt((int*)rs,count*2);
	std::sort(rs,rs+count);
	/* Idea: Synthesize a named state block based on its contents if it doesn't already have a name.
		Higher-level code can then "fix" the state block at runtime if necessary to match current inherited state. */
	grcRasterizerStateDesc rasterizer;
	grcDepthStencilStateDesc depthStencil;
	grcBlendStateDesc blend;
	bool separateAlphaBlendEnable = false, twoSidedStencil = false;
	bool anyDSS = false, anyRS = false, anyBS = false;
	depthStencil.DepthFunc = grcRSV::CMP_LESS;
	for (int i=0; i<count; i++) {
		switch(rs[i].State) {
			case grcersZENABLE: anyDSS = true; depthStencil.DepthEnable = rs[i].Int; break;
			case grcersFILLMODE: anyRS = true; rasterizer.FillMode = rs[i].Int; break;
			case grcersZWRITEENABLE: anyDSS = true; depthStencil.DepthWriteMask = rs[i].Int; break;
			case grcersALPHATESTENABLE: break;
			case grcersSRCBLEND: anyBS = true; blend.BlendRTDesc[0].SrcBlend = rs[i].Int; break;
			case grcersDESTBLEND: anyBS = true; blend.BlendRTDesc[0].DestBlend = rs[i].Int; break;
			case grcersCULLMODE: anyRS = true; rasterizer.CullMode = rs[i].Int; break; 
			case grcersZFUNC: anyDSS = true; depthStencil.DepthFunc = rs[i].Int; break;
			case grcersALPHAREF: D3D9_ONLY(anyBS = true; Assign(AlphaRef,rs[i].Int);) break;
			case grcersALPHAFUNC: break;
			case grcersALPHABLENDENABLE: anyBS = true; blend.BlendRTDesc[0].BlendEnable = rs[i].Int; break;
			case grcersSTENCILENABLE: anyDSS = true; depthStencil.StencilEnable = rs[i].Int; break;
			case grcersSTENCILFAIL: anyDSS = true; depthStencil.FrontFace.StencilFailOp = rs[i].Int; break;
			case grcersSTENCILZFAIL: anyDSS = true; depthStencil.FrontFace.StencilDepthFailOp = rs[i].Int; break;
			case grcersSTENCILPASS: anyDSS = true; depthStencil.FrontFace.StencilPassOp = rs[i].Int; break;
			case grcersSTENCILFUNC: anyDSS = true; depthStencil.FrontFace.StencilFunc = rs[i].Int; break;
			case grcersSTENCILREF: anyDSS = true; Assign(StencilRef,rs[i].Int); break;
			case grcersSTENCILMASK: anyDSS = true; Assign(depthStencil.StencilReadMask,rs[i].Int); break;
			case grcersSTENCILWRITEMASK: anyDSS = true; Assign(depthStencil.StencilWriteMask,rs[i].Int); break;
			case grcersCOLORWRITEENABLE: anyBS = true; Assign(blend.BlendRTDesc[0].RenderTargetWriteMask,rs[i].Int); break;
			case grcersCOLORWRITEENABLE1: anyBS = true; Assign(blend.BlendRTDesc[1].RenderTargetWriteMask,rs[i].Int); blend.IndependentBlendEnable = true; break;
			case grcersCOLORWRITEENABLE2: anyBS = true; Assign(blend.BlendRTDesc[2].RenderTargetWriteMask,rs[i].Int); blend.IndependentBlendEnable = true; break;
			case grcersCOLORWRITEENABLE3: anyBS = true; Assign(blend.BlendRTDesc[3].RenderTargetWriteMask,rs[i].Int); blend.IndependentBlendEnable = true; break;
			case grcersBLENDOP: anyBS = true; blend.BlendRTDesc[0].BlendOp = rs[i].Int; break;
			case grcersBLENDOPALPHA: anyBS = true; blend.BlendRTDesc[0].BlendOpAlpha = rs[i].Int; break;
			case grcersSEPARATEALPHABLENDENABLE: anyBS = true; separateAlphaBlendEnable = rs[i].Int != 0; break;
			case grcersSRCBLENDALPHA: anyBS = true; blend.BlendRTDesc[0].SrcBlendAlpha = rs[i].Int; break;
			case grcersDESTBLENDALPHA: anyBS = true; blend.BlendRTDesc[0].DestBlendAlpha = rs[i].Int; break;
			case grcersHIGHPRECISIONBLENDENABLE: break;
			case grcersSLOPESCALEDEPTHBIAS: anyRS = true; rasterizer.SlopeScaledDepthBias = rs[i].Float; break;
			case grcersDEPTHBIAS: 
#if __D3D9 || __D3D11
					anyRS = true; rasterizer.DepthBiasDX9 = rs[i].Float; 
#endif
					break;
				/* grcersBLENDFACTOR,			// 31 */
			case grcersALPHATOMASK: anyBS = true; blend.AlphaToCoverageEnable = rs[i].Int != 0; break;
			case grcersALPHATOMASKOFFSETS: D3D9_ONLY(anyBS = true; blend.AlphaToMaskOffsets = u8(rs[i].Int)); break;
			case grcersHALFPIXELOFFSET: D3D9_ONLY(anyRS = true; rasterizer.HalfPixelOffset = rs[i].Int != 0); break;
			case grcersTWOSIDEDSTENCIL: anyDSS = true; twoSidedStencil = rs[i].Int != 0; break;
			case grcersBACKSTENCILFAIL: anyDSS = true; depthStencil.BackFace.StencilFailOp = rs[i].Int; break;
			case grcersBACKSTENCILZFAIL: anyDSS = true; depthStencil.BackFace.StencilDepthFailOp = rs[i].Int; break;
			case grcersBACKSTENCILPASS: anyDSS = true; depthStencil.BackFace.StencilPassOp = rs[i].Int; break;
			case grcersBACKSTENCILFUNC: anyDSS = true; depthStencil.BackFace.StencilFunc = rs[i].Int; break;
			case grcersBACKSTENCILREF: Quitf(ERR_GFX_EFFECT_INVALID_STATE_1,"BackStencilRef cannot work any longer!"); break;
			case grcersBACKSTENCILMASK: Quitf(ERR_GFX_EFFECT_INVALID_STATE_2,"BackStencilMask cannot work any longer!"); break;
			case grcersBACKSTENCILWRITEMASK: Quitf(ERR_GFX_EFFECT_INVALID_STATE_3,"BackStencilWriteMask cannot work any longer!"); break;

			default: Quitf(ERR_GFX_EFFECT_INVALID_STATE_4,"danger, unhandled state %u value %u",rs[i].State,rs[i].Int);
		}
	}
	if (!separateAlphaBlendEnable) {
		// If separate alpha blend enable isn't set, make sure these were left at defaults
		Assert(blend.BlendRTDesc[0].SrcBlendAlpha == grcRSV::BLEND_ONE);
		Assert(blend.BlendRTDesc[0].DestBlendAlpha == grcRSV::BLEND_ZERO);
		Assert(blend.BlendRTDesc[0].BlendOpAlpha == grcRSV::BLENDOP_ADD);
		blend.BlendRTDesc[0].SrcBlendAlpha = blend.BlendRTDesc[0].SrcBlend;
		blend.BlendRTDesc[0].BlendOpAlpha = blend.BlendRTDesc[0].BlendOp;
		blend.BlendRTDesc[0].DestBlendAlpha = blend.BlendRTDesc[0].DestBlend;
	}
	if (!twoSidedStencil) {
		// If two-sided stencil isn't set, make sure these were left at defaults
		Assert(depthStencil.BackFace.StencilFailOp == grcRSV::STENCILOP_KEEP);
		Assert(depthStencil.BackFace.StencilDepthFailOp == grcRSV::STENCILOP_KEEP);
		Assert(depthStencil.BackFace.StencilPassOp == grcRSV::STENCILOP_KEEP);
		Assert(depthStencil.BackFace.StencilFunc == grcRSV::CMP_ALWAYS);
		depthStencil.BackFace = depthStencil.FrontFace;
	}
	// Translate separate color write enables in to DX11-style format.
	if (blend.IndependentBlendEnable) {
		// If all four write masks ended up being the same, turn the flag back off
		if (blend.BlendRTDesc[0].RenderTargetWriteMask==blend.BlendRTDesc[1].RenderTargetWriteMask&&
			blend.BlendRTDesc[0].RenderTargetWriteMask==blend.BlendRTDesc[2].RenderTargetWriteMask&&
			blend.BlendRTDesc[0].RenderTargetWriteMask==blend.BlendRTDesc[3].RenderTargetWriteMask) {
			blend.IndependentBlendEnable = false;
			blend.BlendRTDesc[1].RenderTargetWriteMask=blend.BlendRTDesc[2].RenderTargetWriteMask=
				blend.BlendRTDesc[3].RenderTargetWriteMask=15;
		}
		// Otherwise copy all the other blend states across to avoid sanity checking assertions
		else for (int i=1; i<=3; i++) {
			blend.BlendRTDesc[i].BlendEnable = blend.BlendRTDesc[0].BlendEnable;
			blend.BlendRTDesc[i].SrcBlend = blend.BlendRTDesc[0].SrcBlend;
			blend.BlendRTDesc[i].DestBlend = blend.BlendRTDesc[0].DestBlend;
			blend.BlendRTDesc[i].BlendOp = blend.BlendRTDesc[0].BlendOp;
			blend.BlendRTDesc[i].SrcBlendAlpha = blend.BlendRTDesc[0].SrcBlendAlpha;
			blend.BlendRTDesc[i].DestBlendAlpha = blend.BlendRTDesc[0].DestBlendAlpha;
			blend.BlendRTDesc[i].BlendOpAlpha = blend.BlendRTDesc[0].BlendOpAlpha;
		}
	}

	// If we explicitly specified any states of a certain type in the shader, we will always create
	// a state block, even if it happens to match default state.
	RasterizerStateHandle = anyRS? (u8)grcStateBlock::CreateRasterizerState(rasterizer) : INVALID_STATEBLOCK;
	DepthStencilStateHandle = anyDSS? (u8)grcStateBlock::CreateDepthStencilState(depthStencil) : INVALID_STATEBLOCK;
	BlendStateHandle = anyBS? (u8)grcStateBlock::CreateBlendState(blend) : INVALID_STATEBLOCK;
}

void grcEffect::Technique::Load(fiStream &S)
{
#if EFFECT_PRESERVE_STRINGS
	ReadString(S,Name);
	NameHash = atStringHash(Name);
#else
	NameHash = ReadHashedString(S);
#endif
	int count = S.GetCh();
	if (count > 0)
	{
		Passes.Reserve(count);
		for (int i=0; i<count; i++)
			Passes.Append().Load(S);
	}	
}

void grcParameter::Annotation::Load(fiStream &S)
{
	NameHash = ReadHashedString(S);
	Type = (AnnoType) S.GetCh();
	if (Type == AT_STRING) {
#if EFFECT_PRESERVE_STRINGS_DCC
		ReadString(S,String);
#elif EFFECT_PRESERVE_STRINGS
		// skip a few var to try and save a few bytes..
		if (NameHash == ATSTRINGHASH("TCPTemplate",0x2310a15b) || NameHash == ATSTRINGHASH("TCPTemplateRelative",0x86aa042e))
			ReadHashedString(S);
		else		
			ReadString(S,String);
#else
		// Only preserve a few specific strings in the final game, everything else is for DCC stuff.
		if (NameHash == ATSTRINGHASH("UIHint",0xcddc9ea3) || NameHash == ATSTRINGHASH("UIAssetName",0xe6c32dcd) || NameHash == ATSTRINGHASH("ResourceName",0x4f5ce147))
			ReadString(S,String);
		else
			ReadHashedString(S);
#endif
	}
	else		// Works for either ints or floats.
		S.ReadInt(&Int,1);
}


void grcParameter::Load(int type, fiStream &S, u32 textureNameHash)
{
	Type = (u8) type;
	Count = (u8) S.GetCh();
#if EFFECT_TEXTURE_SUBTYPING
	const u8 regAndTextureType = static_cast<u8>(S.GetCh());
	Register = regAndTextureType & ((1<<TEXTURE_REGISTER_BITS)-1);
	TextureType = regAndTextureType >> TEXTURE_REGISTER_BITS;
	const u8 usageAndComparison = static_cast<u8>(S.GetCh());
	Usage = usageAndComparison & ((1<<TEXTURE_USAGE_BITS)-1);
	ComparisonFilter = 0;
#else
	Assign(Register,S.GetCh());
	Assign(Usage,S.GetCh());
#endif // EFFECT_TEXTURE_SUBTYPING
#if __WIN32PC
	if (!GRCDEVICE.GetCurrent())
		Usage = 0;
#endif
#if EFFECT_PRESERVE_STRINGS
	ReadString(S,Name);
	NameHash = atStringHash(Name);
	ReadString(S,Semantic);
	SemanticHash = atStringHash(Semantic);
#else
	NameHash = ReadHashedString(S);
	SemanticHash = ReadHashedString(S);
#endif

	switch (Type)
	{
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	case grcEffect::VT_UAV_STRUCTURED:
		SamplerStateSet = Count;
		Count = 1; break;
#endif
#if EFFECT_TEXTURE_SUBTYPING
	case grcEffect::VT_TEXTURE:
		if (TextureType == TEXTURE_REGULAR && textureNameHash != NameHash)
		{
			TextureType = TEXTURE_PURE;
		}
		if (TextureType == TEXTURE_SAMPLER)
		{
			ComparisonFilter = Count;
		}
		Count = 0; break;
#else
	(void)textureNameHash;
#endif
	default:
		if (!Count)
			Count = 1;
	}

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
#if __RESOURCECOMPILER
	if (g_sysPlatform == platform::WIN32PC || g_sysPlatform == platform::WIN64PC || g_sysPlatform == platform::DURANGO || g_sysPlatform == platform::ORBIS)
#endif // __RESOURCECOMPILER
	{
		S.Read(&CBufferOffset,sizeof(CBufferOffset));
		S.Read(&CBufferNameHash,sizeof(CBufferNameHash));
	}
#endif

	AnnotationCount = (u8) S.GetCh();
	delete[] Annotations;
	Annotations = AnnotationCount? rage_new Annotation[AnnotationCount] : NULL;

	// Load all annotations; if IsMaterial is present, use that to override
	// the computed "is this a per-material or per-instance" flag.
	// (the default is a material parameter for everything but textures)
	u32 IsMaterial_hash = ATSTRINGHASH("IsMaterial",0x90ad794b);
	int IsMaterial_value = (Type != grcEffect::VT_TEXTURE);
	for (int i=0; i<AnnotationCount; i++)
	{
		Annotations[i].Load(S);
		if (Annotations[i].NameHash == IsMaterial_hash && Annotations[i].Type == Annotation::AT_INT)
			IsMaterial_value = Annotations[i].Int;
	}
	if (IsMaterial_value)
		Usage |= USAGE_MATERIAL;

	DataSize = (u8) S.GetCh();
	delete[] (char*) Data;
	Data = NULL;
#if EFFECT_TEXTURE_SUBTYPING
	if (Type == grcEffect::VT_TEXTURE && TextureType != TEXTURE_SAMPLER) {
		S.Seek(S.Tell() + DataSize*4);	//skip sampler description
		DataSize = 0;
		SamplerStateSet = INVALID_STATEBLOCK;
	}else
#endif // EFFECT_TEXTURE_SUBTYPING
	if (Type == grcEffect::VT_TEXTURE) {
		u32 *temp = Alloca(u32,DataSize);
		S.ReadInt((int*)temp,DataSize);
#if !__RESOURCECOMPILER	// effect_values doesn't match between platforms (and isn't supposed to) so skip this.
		grcSamplerStateDesc ss;
		u32 magFilter = TEXF_POINT, minFilter = TEXF_POINT, mipFilter = TEXF_POINT;
		for (int i=0; i<(DataSize>>1); i++) {
			union { float f; unsigned u; int i; } x;
			x.u = temp[i+i+1];
			// Note that DX11 nomenclature is opposite of DX9 nomenclature!
			// D3DSAMP_MAXMIPLEVEL - LOD index of largest map to use. Values range from 0 to (n-1) where 0 is the largest. The default value is zero.  
			// D3DSAMP_MINMIPLEVEL - DWORD value that specifies the level-of-detail index of the smallest mipmap level to sample. If this index is greater or equal to the 
			// number of mip levels in a set texture, an index of (mip levels - 1) will be used instead. The default value is 13.  
			switch (temp[i+i]) {
				case grcessADDRESSU: ss.AddressU = x.u; break;
				case grcessADDRESSV: ss.AddressV = x.u; break;
				case grcessADDRESSW: ss.AddressW = x.u; break;
				case grcessBORDERCOLOR: ss.BorderColorRed = ss.BorderColorGreen = ss.BorderColorBlue = ss.BorderColorAlpha = x.u != 0? 1.0f : 0.0f; break;
				case grcessMAGFILTER: magFilter = x.u; break;
				case grcessMINFILTER: minFilter = x.u; break;
				case grcessMIPFILTER: mipFilter = x.u; break;
				case grcessMIPMAPLODBIAS: ss.MipLodBias = x.f; break;
				case grcessMAXMIPLEVEL: ss.MinLod = float(x.i); break;
				case grcessMAXANISOTROPY: ss.MaxAnisotropy = x.u; break;
				case grcessTRILINEARTHRESHOLD: D3D9_ONLY(ss.TrilinearThresh = x.u;) break;
				case grcessMINMIPLEVEL: ss.MaxLod = float(x.i); break;
				case grcessTEXTUREZFUNC: D3D9_ONLY(ss.TextureZFunc = x.u;) break;
				case grcessALPHAKILL: D3D9_ONLY(ss.AlphaKill = x.i != 0;) break;
#if RSG_PC || RSG_DURANGO
				case grcessCOMPAREFUNC: ss.ComparisonFunc = x.u; break;
#elif RSG_ORBIS
				case grcessCOMPAREFUNC: ss.ComparisonFunc = x.u - 1; break; // Orbis is offset by one, but fx2cg values is PC versions, so we fudge. yolo.
#endif
				default: Quitf(ERR_GFX_EFFECT_INVALID_STATE_5,"invalid sampler state %d in shader file?",temp[i+i]); break;
			}
		}
		// Translate old-style filter modes into DX11-style.
		if (minFilter==TEXF_ANISOTROPIC||magFilter==TEXF_ANISOTROPIC) {
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
			ss.Filter = ss.ComparisonFunc == grcRSV::CMP_NEVER ? FILTER_ANISOTROPIC : FILTER_COMPARISON_ANISOTROPIC;
#else
			ss.Filter = FILTER_ANISOTROPIC;
			Assertf(ss.MaxAnisotropy > 1 && ss.MaxAnisotropy <= 16,"Bogus max anisotropy %u specified in sampler %s when using anisotropic filtering %s",ss.MaxAnisotropy,Name.c_str(),ss.MaxAnisotropy==1?"(using 1 essentially disables it, just specify LINEAR for the filter instead)":"");
#endif

		}
		else if (minFilter==TEXF_GAUSSIAN||magFilter==TEXF_GAUSSIAN) {
			ss.Filter = FILTER_GAUSSIAN;
			Assertf(ss.MaxAnisotropy == 1,"Bogus max anisotropy %u specified in sampler %s when using non-anisotropic filtering",ss.MaxAnisotropy,Name.c_str());
		}
		else if (mipFilter==TEXF_LINEAR) {
			if (minFilter==TEXF_LINEAR)
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
				ss.Filter = ss.ComparisonFunc == grcRSV::CMP_NEVER ? (magFilter==TEXF_LINEAR? FILTER_MIN_MAG_MIP_LINEAR : FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR) :
																	 (magFilter==TEXF_LINEAR? FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
#else
				ss.Filter = magFilter==TEXF_LINEAR? FILTER_MIN_MAG_MIP_LINEAR : FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
#endif
			else // MinFilter = POINT
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
				ss.Filter = ss.ComparisonFunc == grcRSV::CMP_NEVER ? (magFilter==TEXF_LINEAR? FILTER_MIN_POINT_MAG_MIP_LINEAR : FILTER_MIN_MAG_POINT_MIP_LINEAR) :
																	 (magFilter==TEXF_LINEAR? FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR : FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR);
#else
				ss.Filter = magFilter==TEXF_LINEAR? FILTER_MIN_POINT_MAG_MIP_LINEAR : FILTER_MIN_MAG_POINT_MIP_LINEAR;
#endif
			Assertf(ss.MaxAnisotropy == 1 || RSG_PC, "Bogus max anisotropy %u specified in sampler %s when using non-anisotropic filtering",ss.MaxAnisotropy,Name.c_str());
		}
		else { // MipFilter==TEXF_POINT
			if (minFilter==TEXF_LINEAR)
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
				ss.Filter = ss.ComparisonFunc == grcRSV::CMP_NEVER ? (magFilter==TEXF_LINEAR? FILTER_MIN_MAG_LINEAR_MIP_POINT: FILTER_MIN_LINEAR_MAG_MIP_POINT) :
																	 (magFilter==TEXF_LINEAR? FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT: FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT);
#else
				ss.Filter = magFilter==TEXF_LINEAR? FILTER_MIN_MAG_LINEAR_MIP_POINT: FILTER_MIN_LINEAR_MAG_MIP_POINT;
#endif
			else // MinFilter = POINT
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
				ss.Filter = ss.ComparisonFunc == grcRSV::CMP_NEVER ? (magFilter==TEXF_LINEAR? FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT: FILTER_MIN_MAG_MIP_POINT) :
																	 (magFilter==TEXF_LINEAR? FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT: FILTER_COMPARISON_MIN_MAG_MIP_POINT);
#else
				ss.Filter = magFilter==TEXF_LINEAR? FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT: FILTER_MIN_MAG_MIP_POINT;
#endif
			Assertf(ss.MaxAnisotropy == 1,"Bogus max anisotropy %u specified in sampler %s when using non-anisotropic filtering",ss.MaxAnisotropy,Name.c_str());
			// If NONE is supplied as a mip filter, and MaxLod is the default value, force it to MinLod instead.
			if (mipFilter==TEXF_NONE && ss.MaxLod == 12.0f)
				ss.MaxLod = ss.MinLod;
		}
		Assign(SamplerStateSet,grcStateBlock::CreateSamplerState(ss));
#if EFFECT_TEXTURE_SUBTYPING
		grcAssertf(ComparisonFilter == (ss.Filter >= FILTER_COMPARISON_MIN_MAG_MIP_POINT),
			"Loaded sampler (%s) state has a comparison flag mismatch", Name.c_str());
#endif // EFFECT_TEXTURE_SUBTYPING
#else	// __RESOURCECOMPILER
		SamplerStateSet = INVALID_STATEBLOCK;	// Bogus value, never actually used
#endif	// __RESOURCECOMPILER
		ASSERT_ONLY(SavedSamplerStateSet = INVALID_STATEBLOCK);
		DataSize = 0;
	}
	else {
		int allocSize = Count * 4 * g_Float4SizeByType[Type];
		/// grcDisplayf("DataSize=%d allocSize = %d, Count=%d, sizeByType=%d",DataSize,allocSize,Count,g_Float4SizeByType[Type]);
		Assertf(allocSize >= DataSize,"Allocated %d bytes, but reserved %d -- maybe too many initializers in an array?",allocSize,DataSize);
		if (DataSize) {
			int srcStride = (int)DataSize / (int)Count;	// in words
			int destStride = allocSize / Count;	// in words
			Data = (void*) (rage_new int[allocSize]);
			Assign(DataSize,allocSize>>2);
			memset(Data, 0, destStride * Count * sizeof(int));
			int *dest = (int*) Data;
			for (int i=0; i<Count; i++, dest += destStride)
				S.ReadInt(dest, srcStride);
		}
	}
}

struct SortByNotTexture {
	bool operator()( grcParameter const &a, grcParameter const &b ) const {
		return a.GetCount() < b.GetCount();
	}
};

#if RSG_ORBIS
#if !__NO_OUTPUT
static const char* Type2Str(u8 type)
{
	return sce::Shader::Binary::getPsslTypeString( static_cast<sce::Shader::Binary::PsslType>(type) );
}
#endif

template <class PA,class PB>
static void OrbisMatchPrograms(const char *const descriptor, const PA &progA, const PB &progB)
{
	const char *const vpBinaryLoc = (char*)progA.GetProgram() - 0x34;
	sce::Shader::Binary::Program vpBinary;
	vpBinary.loadFromMemory(vpBinaryLoc,progA.GetProgramSize());

	const char *const fpBinaryLoc = (char*)progB.GetProgram() - 0x34;
	sce::Shader::Binary::Program fpBinary;
	fpBinary.loadFromMemory(fpBinaryLoc,progB.GetProgramSize());

#if !__NO_OUTPUT
#if EFFECT_PRESERVE_STRINGS
	const char *const vpName = progA.GetEntryName(), *const fpName = progB.GetEntryName();
	grcDebugf2("%s - %d vp outputs, %d fp inputs", descriptor, vpBinary.m_numOutputAttributes, fpBinary.m_numInputAttributes );
#else
	const char vpName[] = "VS", fpName[] = "PS";	//TODO: fix this for general case
#endif
	// Make sure every FP input has a matching VP output.
	for (int k=0; k<fpBinary.m_numInputAttributes; k++) {
		sce::Shader::Binary::Attribute &ia = fpBinary.m_inputAttributes[k];
		if (ia.m_psslSemantic == sce::Shader::Binary::kSemanticSFrontFace ||	// This is supplied by the rasterizer hardware, not the vertex shader.
			ia.m_psslSemantic == sce::Shader::Binary::kSemanticSSampleIndex)	
			continue;
		bool found = false;
		for (int l=0; l<vpBinary.m_numOutputAttributes; l++) {
			sce::Shader::Binary::Attribute &oa = vpBinary.m_outputAttributes[l];
			if (!strcmp(oa.getSemanticName(), ia.getSemanticName()) && oa.m_semanticIndex == ia.m_semanticIndex && oa.m_psslSemantic == ia.m_psslSemantic) {
				if (oa.m_type == ia.m_type)
					found = true;
#if EFFECT_PRESERVE_STRINGS
				else
					grcErrorf("%s - %s output attribute type (%s) doesn't match %s input type (%s) even though semantics '%s' match; (out/in:%s/%s)",
						descriptor, vpName, Type2Str(oa.m_type), fpName, Type2Str(ia.m_type), oa.getSemanticName(), oa.getName(), ia.getName() );
#endif
			}
		}
		if (!found) {
			for (int l=0; l<vpBinary.m_numOutputAttributes; l++) {
				sce::Shader::Binary::Attribute &oa = vpBinary.m_outputAttributes[l];
				grcErrorf("%s output %d: %s:%s (%d/%d), type %s", vpName, l,
					oa.getName(), oa.getSemanticName(), oa.m_semanticIndex, oa.m_resourceIndex, Type2Str(oa.m_type) );
			}
			for (int k=0; k<fpBinary.m_numInputAttributes; k++) {
				sce::Shader::Binary::Attribute &ia = fpBinary.m_inputAttributes[k];
				grcErrorf("%s  input %d: %s:%s (%d/%d), type %s", fpName, k,
					ia.getName(), ia.getSemanticName(), ia.m_semanticIndex, ia.m_resourceIndex,	Type2Str(ia.m_type) );
			}
#if EFFECT_PRESERVE_STRINGS
			grcErrorf("%s - %s input attribute %s:%s (see above) doesn't have matching %s output.",
				descriptor,	fpName, ia.getName(),ia.getSemanticName(), vpName );
#endif
		}
	}
#endif // !__NO_OUTPUT
}
#endif	//RSG_ORBIS

#if !__TOOL && !__RESOURCECOMPILER && RSG_PC
bool grcEffect::UseSourceShader(const char* pszShader)
{
	const char* pSpecifiedShader = NULL;
	bool bLoadSourceShader = false;
	
	// Check to see if there is a source file specified, otherwise compile all source files.
	if (PARAM_useSourceShaders.Get(pSpecifiedShader) && strcmp(pSpecifiedShader, "") != 0)
	{
		atString specifiedShaderName = atString(pSpecifiedShader);
		atString currentSourceEffect = atString(pszShader);
		atString finalSpecifiedName = atString("/");

		specifiedShaderName.Lowercase(); currentSourceEffect.Lowercase();

		// Add a '/' so it doesn't accidentally take an improper shader.
		// Ex. default.hlsl could still load ped_default.hlsl
		finalSpecifiedName += specifiedShaderName;

		if (currentSourceEffect.EndsWith(finalSpecifiedName))
			bLoadSourceShader = true;
	}
	return bLoadSourceShader;
}

void grcEffect::LoadSourceShader(const char* pszShader, void* &ProgramData, u32 &ProgramSize, const char* pzEntryName, const char* pzShaderModel)
{
	LPD3D10BLOB code;
	LPD3D10BLOB errors;
	HRESULT		result;

	if (pszShader)
	{
		char fullpath[RAGE_MAX_PATH];
		if (!GetFullName(pszShader, fullpath, RAGE_MAX_PATH - 1, true))
		{
			return;
		}

		fiStream* pStream = fiStream::Open(fullpath);
		if ( pStream != NULL)
		{
			u32 size = pStream->Size();

			char* pTempData = rage_new char[size];
			pStream->Read( pTempData, size );

			const char* entry = strrchr(pzEntryName, ':') + 1;

			u32 uCompileFlags = D3D10_SHADER_PACK_MATRIX_ROW_MAJOR | D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY;

			if (PARAM_useDebugShaders.Get())
				uCompileFlags |= D3D10_SHADER_PREFER_FLOW_CONTROL | D3D10_SHADER_SKIP_OPTIMIZATION | D3D10_SHADER_DEBUG;

			if (FAILED(D3DCompileFromMemory(pTempData, size, NULL, NULL, NULL, entry, pzShaderModel, uCompileFlags, NULL, NULL, &code, &errors, &result)))
			{
				// We can do something here to spit out the errors that were thrown during the compile.
				char* errorListing = rage_new char[errors->GetBufferSize()];
				strcpy(errorListing, (char*)errors->GetBufferPointer());

				Warningf("\nCould not compile shader from memory: %s\n", pszShader);
				Errorf("%s\n", errorListing);

				delete [] errorListing;
			}

			delete[] pTempData;
			pStream->Close();

			if (code != NULL)
			{
				size_t programSize = code->GetBufferSize();
				void* programData = NULL;

				// Re-allocate the programData memory to write the program too.
#if !__FINAL && __WIN32PC
				if (PARAM_useDebugShaders.Get())
				{
					DEALLOCATE_PROGRAM(programData);
					programData = ALLOCATE_PROGRAM(programSize);
				}
				else
#endif
				{
					DEALLOCATE_PROGRAM(programData);
					programData = ALLOCATE_PROGRAM(programSize);
				}

				// Copy the newly compiled code into the programData.
				memcpy(programData, code->GetBufferPointer(), programSize);
				ProgramData = programData;
				ProgramSize = (u32)programSize;
			}
		}
	}
}
#endif // !__TOOL && !__RESOURCECOMPILER && RSG_PC

void grcEffect::Load(fiStream &S,int magic, const char* currentLoadingEffect)
{
	/* File format:
			STRING is a byte length followed by the characters; \0 is included in the length and the data.
			For example, "draw" would be stored as 5, 'd', 'r', 'a', 'w', '\0'.

			'rgxe' magic number (now 'rgxf')
			byte propertyCount
			propertyCount Annotation's (see below)
			byte VertexProgramCount
			VertexProgramCount programs:
				STRING EntryName
				byte constantCount
				constantCount Constant's: (only used so we can see which passes use which parameters)
					STRING ConstantName
				little-endian short ProgramSize
				ProgramSize bytes of program data (already in native byte order)
				PS3-specific (not on fragment programs)
					CellCgbVertexProgramConfiguration
					byte vertexConstantCount
					vertexConstantCount indices:
						little-endian-short registerIndex
					vertexConstantCount values:
						float4 initialValue
			byte FragmentProgramCount
			FragmentProgramCount programs (as above)
				PS3-specific:
					CellCgbFragmentProgramConfiguration
					little-endian short patchCount
					patchCount patches:
						byte registerIndex
						byte count
						count Patches:
							little-endian-short patchAddress
			byte GeometrytProgramCount
			GeometryProgramCount programs (as above)
				Stream Out Specification
			byte HullProgramCount
			HullProgramCount programs (as above)
			byte DomainProgramCount
			DomainProgramCount programs (as above)
			byte ComputeProgramCount
			ComputeProgramCount programs (as above)
			byte globalCount
			globalCount Parameter's:
				byte type (grcEffect::VT_... enumerant)
				byte arraySize (0 if scalar and not an array)
				byte Register
				byte Usage (bitmask of USAGE_VERTEXPROGRAM and USAGE_FRAGMENTPROGRAM)
					Note this is across all techniques and passes in the shader
				STRING ParameterName (the parameter as it appears in the tranlated cg source)
				STRING SemanticName (the semantic name that all higher-level code will actually be using)
				byte annotationCount
				annotationCount Annotation's:
					STRING AnnotationName
					byte type
					case type of:
						AT_INT: little-endian int
						AT_FLOAT: little-endian float
						AT_STRING: STRING
				byte initialValueSize (in words)
				initialValueSize little-endian int's or float's
			byte localCount
			localCount Parameter's (as above)
			byte techniqueCount
			techniqueCount Technique's:
				STRING TechniqueName
				byte passCount
				passCount Pass'es:
					byte VertexProgramIndex
					byte FragmentProgramIndex
					byte stateCount
					stateCount States:
						little-endian int (as per effect_typedefs.h)
						little-endian int or float value
	*/

	// Magic number has already been validated by now.
	u32 dcl;
	S.ReadInt(&dcl,1);

	Assign(m_Dcl, dcl);

	int propCount = S.GetCh();
	if (propCount > 0)
	{
		m_Properties.Reserve(propCount);
		for (int i=0; i<propCount; i++)
			m_Properties.Append().Load(S);
	}

	int count = S.GetCh();
	if (count > 0)
	{
		m_VertexPrograms.Reserve(count);
		for (int i=0; i<count; i++) {
			// grcDisplayf("VP %d:",i);
			m_VertexPrograms.Append().Load(S,currentLoadingEffect);
		}
	}

	count = S.GetCh();
	if (count > 0)
	{
		m_FragmentPrograms.Reserve(count);
		for (int i=0; i<count; i++) {
			// grcDisplayf("FP %d:",i);
			m_FragmentPrograms.Append().Load(S,currentLoadingEffect);
		}
	}

	count = S.GetCh();
	if (count > 0)
	{
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
#if (__D3D9)
		{
			ASSERT_ONLY(char message[1024]);
			ASSERT_ONLY(sprintf(message, "%s %d compute program is defined for platform(%c)", GetEffectName(), count, rage::g_sysPlatform));
			AssertMsg(count == ((rage::g_sysPlatform != rage::platform::WIN32PC && rage::g_sysPlatform != rage::platform::WIN64PC && rage::g_sysPlatform != rage::platform::DURANGO && rage::g_sysPlatform != rage::platform::ORBIS) ? 0 : 1), message);
				// testing against 1 for non Dx11 win32 platforms as we always write out nullprogram for win32pc platform
		// testing against 0 for non PC builds as we dont write anything for them...
		}
#endif

		m_ComputePrograms.Reserve(count);
		for (int i=0; i<count; i++)
		{
			// grcDisplayf("FP %d:",i);
			m_ComputePrograms.Append().Load(S,currentLoadingEffect);
		}
#else
	Assert(!count);
#endif // RSG_PC || RSG_DURANGO
	}

	count = S.GetCh();
	if (count > 0)
	{
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
#if (__D3D9)
		{
			ASSERT_ONLY(char message[1024]);
			ASSERT_ONLY(sprintf(message, "%s %d domain program is defined for platform(%c)", GetEffectName(), count, rage::g_sysPlatform));
			AssertMsg(count == ((rage::g_sysPlatform != rage::platform::WIN32PC && rage::g_sysPlatform != rage::platform::WIN64PC && rage::g_sysPlatform != rage::platform::DURANGO && rage::g_sysPlatform != rage::platform::ORBIS) ? 0 : 1), message);
			// testing against 1 for non Dx11 win32 platforms as we always write out nullprogram for win32pc platform
			// testing against 0 for non PC builds as we dont write anything for them...
		}
#endif
		m_DomainPrograms.Reserve(count);
		for (int i=0; i<count; i++) {
			// grcDisplayf("FP %d:",i);
			m_DomainPrograms.Append().Load(S,currentLoadingEffect);
		}
#else
	Assert(!count);
#endif // RSG_PC || RSG_DURANGO
	}

	count = S.GetCh();
	if (count > 0)
	{
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
#if (__D3D9)
		{
			ASSERT_ONLY(char message[1024]);
			ASSERT_ONLY(sprintf(message, "%s %d geometry program is defined for platform(%c)", GetEffectName(), count, rage::g_sysPlatform));
			AssertMsg(count == ((rage::g_sysPlatform != rage::platform::WIN32PC && rage::g_sysPlatform != rage::platform::WIN64PC && rage::g_sysPlatform != rage::platform::DURANGO && rage::g_sysPlatform != rage::platform::ORBIS) ? 0 : 1), message);
			// testing against 1 for non Dx11 win32 platforms as we always write out nullprogram for win32pc platform
			// testing against 0 for non PC builds as we dont write anything for them...
		}
#endif
		m_GeometryPrograms.Reserve(count);
		for (int i=0; i<count; i++)
		{
			// grcDisplayf("FP %d:",i);
			m_GeometryPrograms.Append().Load(S,currentLoadingEffect);
		}
#else
	Assert(!count);
#endif // RSG_PC || RSG_DURANGO
	}

	count = S.GetCh();
	if (count > 0)
	{
	
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
#if (__D3D9)
		{
			ASSERT_ONLY(char message[1024]);
			ASSERT_ONLY(sprintf(message, "%s %d hull program is defined for platform(%c)", GetEffectName(), count, rage::g_sysPlatform));
			AssertMsg(count == ((rage::g_sysPlatform != rage::platform::WIN32PC && rage::g_sysPlatform != rage::platform::WIN64PC && rage::g_sysPlatform != rage::platform::DURANGO && rage::g_sysPlatform != rage::platform::ORBIS) ? 0 : 1), message);
			// testing against 1 for non Dx11 win32 platforms as we always write out nullprogram for win32pc platform
			// testing against 0 for non PC builds as we dont write anything for them...
		}
#endif
		m_HullPrograms.Reserve(count);
		for (int i=0; i<count; i++)
		{
			// grcDisplayf("FP %d:",i);
			m_HullPrograms.Append().Load(S,currentLoadingEffect);
		}
#else
	Assert(!count);
#endif	// RSG_PC || RSG_DURANGO
	}

#if THREADED_SHADER_LOAD
	sm_EffectToken.Lock();
#endif

	// Turn off any active container
	sysMemAllocator &prev = sysMemAllocator::SetContainer(sysMemAllocator::GetCurrent());

#if RSG_DURANGO
	(void*) magic;
#endif
	if (g_sysPlatform == platform::WIN32PC || g_sysPlatform == platform::WIN64PC || g_sysPlatform == platform::DURANGO || rage::g_sysPlatform == rage::platform::ORBIS || magic >= 'f')
	{
		count = S.GetCh();
		for (int i=0; i < count; i++)
		{
			grcCBuffer temp;
			temp.Load(S);
			bool duplicate = false;
			for (int j = 0; j < sm_GlobalsCBuf.GetCount(); j++)
			{
				if (sm_GlobalsCBuf[j].GetNameHash() == temp.GetNameHash())
				{
					FatalAssertf((sm_GlobalsCBuf[j].GetSize() == temp.GetSize()),
						"Mismatching global buffer size - Must always be the same size for all shaders - %s %d, %d",
						sm_GlobalsCBuf[j].GetName(), sm_GlobalsCBuf[j].GetSize(), temp.GetSize());
					FatalAssertf(memcmp(sm_GlobalsCBuf[j].GetRegisters(), temp.GetRegisters(), NUM_TYPES*sizeof(u16)) == 0,
						"Mismatching global buffer registers - Must always be assigned to the same registers for all shaders - %s %d, %d",
						sm_GlobalsCBuf[j].GetName(), sm_GlobalsCBuf[j].GetSize(), temp.GetSize());
					duplicate = true;
					break;
				}
			}

			if (!duplicate || (temp.GetSize() == 0))
			{
				sm_GlobalsCBuf.Push(temp);
				grcCBuffer *topmost = &sm_GlobalsCBuf.Top();
				topmost->Init(true);
			}
		}
	}

	u32 lastSemanticHash = 0;
	count = S.GetCh();
	for (int i=0; i<count; i++) {
		grcParameter temp;
		const VarType varType = static_cast<VarType>( S.GetCh() );
		Assert(varType != VT_NONE);
		temp.Load(varType, S, lastSemanticHash);

		// and check any previous globals for a match.
		// if found, delete the one we just loaded, it's redundant.
		bool duplicate = false;
		int j = -1;
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
		if (!temp.GetCBufferNameHash()) {
			// For shader constants that have a zero cbuffer hash (happens when
			// nostrip is specified), treat these as a dup so that no constant
			// buffer will be registered, or error reported.  Other effect
			// parameter types (textures etc) will also have a zero cbuffer
			// hash, but these still need to be handled normally.
#			define IGNORE_TYPES_AS_DUP(FUNC)                                   \
				FUNC(0x00, VT_NONE,                 false)                     \
				FUNC(0x01, VT_INT_DONTUSE,          false)                     \
				FUNC(0x02, VT_FLOAT,                true)                      \
				FUNC(0x03, VT_VECTOR2,              true)                      \
				FUNC(0x04, VT_VECTOR3,              true)                      \
				FUNC(0x05, VT_VECTOR4,              true)                      \
				FUNC(0x06, VT_TEXTURE,              false)                     \
				FUNC(0x07, VT_BOOL_DONTUSE,         false)                     \
				FUNC(0x08, VT_MATRIX43,             true)                      \
				FUNC(0x09, VT_MATRIX44,             true)                      \
				FUNC(0x0a, VT_STRING,               false)                     \
				FUNC(0x0b, VT_INT,                  true)                      \
				FUNC(0x0c, VT_INT2,                 true)                      \
				FUNC(0x0d, VT_INT3,                 true)                      \
				FUNC(0x0e, VT_INT4,                 true)                      \
				FUNC(0x0f, VT_STRUCTUREDBUFFER,     false)                     \
				FUNC(0x10, VT_SAMPLERSTATE,         false)                     \
				FUNC(0x11, VT_UNUSED1,              true)                      \
				FUNC(0x12, VT_UNUSED2,              true)                      \
				FUNC(0x13, VT_UNUSED3,              true)                      \
				FUNC(0x14, VT_UNUSED4,              true)                      \
				FUNC(0x15, VT_UAV_STRUCTURED,       false)                     \
				FUNC(0x16, VT_UAV_TEXTURE,          false)                     \
				FUNC(0x17, VT_BYTEADDRESSBUFFER,    false)                     \
				FUNC(0x18, VT_UAV_BYTEADDRESS,      false)
			CompileTimeAssert(0x19 == VT_COUNT);
#			define CHECK_VALUES(INDEX, ENUM, DUP)   CompileTimeAssert(INDEX == ENUM);
#			define BUILD_LUT(INDEX, ENUM, DUP)      DUP,
			IGNORE_TYPES_AS_DUP(CHECK_VALUES)
			static const bool lut[VT_COUNT] = {IGNORE_TYPES_AS_DUP(BUILD_LUT)};
			duplicate = lut[temp.GetType()];
#			undef BUILD_LUT
#			undef CHECK_VALUES
#			undef IGNORE_TYPES_AS_DUP
		}
		if (!duplicate)
#endif
		for (j=0; j<sm_Globals.GetCount(); j++) {
			if (sm_Globals[j].NameHash == temp.NameHash) {
				// Catch the case where the global was used only in one type of program before but now
				// is used in both.  But don't blow away information that was already there.
#if EFFECT_PRESERVE_STRINGS
#	if EFFECT_TEXTURE_SUBTYPING && 0	// it is a valid use case
				grcAssertf(sm_Globals[j].IsMsaaTexture() == temp.IsMsaaTexture(),
					"Global texture (%s) is re-used with a different subtype (old=%d, new=%d)",
					temp.Name, sm_Globals[j].TextureType, temp.TextureType);
#	endif
				if (sm_Globals[j].Register != temp.Register && !(__D3D11 || RSG_ORBIS))
					grcErrorf("Global '%s' previously seen at addr %d now at addr %d, probably need recompile.",sm_Globals[j].Name.c_str(),sm_Globals[j].Register,temp.Register);
				if ((sm_Globals[j].Usage | temp.Usage) != sm_Globals[j].Usage)
					grcDebugf2("DUPLICATE Global variable '%s' usage now %d",sm_Globals[j].Name.c_str(),sm_Globals[j].Usage | temp.Usage);
#endif
				sm_Globals[j].Usage |= temp.Usage;

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
				FatalAssertf(sm_Globals[j].GetCBufferNameHash() == temp.GetCBufferNameHash(),
					"Global variable '%s' belongs to more than one constant buffer", sm_Globals[j].Name.c_str());

				// special case: global sampler loaded up earlier had invalid CBufferOffset, so fix it now if valid one arrives (but is dupe):
				if(	(sm_Globals[j].GetType()		== VT_TEXTURE)		&&
					(sm_Globals[j].CBufferOffset	== INVALID_OFFSET)	&&
					(sm_Globals[j].CBufferNameHash	== 0)				&&
					(temp.CBufferOffset				!= INVALID_OFFSET)	&&
					(temp.Usage						!=0)				)
				{
					Assert(sm_Globals[j].Register == temp.Register);				// this should never trigger
					Assert(sm_Globals[j].SamplerStateSet == temp.SamplerStateSet);

					sm_Globals[j].CBufferOffset = temp.CBufferOffset;	// fix buffer offset of the sampler
				}
#endif

				duplicate = true;
				break;
			}
		}
		//if (!duplicate)
		//	grcDisplayf("NEW global %s:%s addr vp %d/ fp %d",temp.Name.c_str(),temp.Semantic.c_str(),temp.VertexAddress,temp.FragmentAddress);
		if (!duplicate) {
			// sizeof(Parameter) is a multiple of 16 so this is never unaligned.
			// Move the parameter onto the global data heap
			if( sm_Globals.IsFull() == false )
			{
				sm_Globals.Push(temp);
				temp.Data = NULL;			// HACK so that the copied version still owns the array.
				temp.Annotations = NULL;	// HACK so that the copied version still owns the array.
				// grcDisplayf("Global %d data at %p size=%u",sm_Globals.GetCount()-1,global.Data,allocSize);
#if EFFECT_PRESERVE_STRINGS
				grcDebugf2("Global variable '%s' addr %x (usage %d)",temp.Name.c_str(),temp.Register,temp.Usage);
#endif	
			}
#if __ASSERT
			else
			{
				static int missingGlobals = 0;
				missingGlobals++;

				char Msg[256];
				sprintf(Msg, "Shader globals array full (needs to be at least %d\n", missingGlobals);
				AssertMsg(false, Msg);
				
				Warningf("%d: Trying to add Global variable '%s' addr %x (usage %d)",missingGlobals, temp.Name.c_str(), temp.Register, temp.Usage);
			}
#endif // __ASSERT			
		}
#if EFFECT_TRACK_MSAA_ERRORS
		if (varType == VT_TEXTURE || varType == VT_SAMPLERSTATE)
		{
			// j == actual index in sm_Globals that our "temp" is associated with
			grcAssertf(!strcmp(sm_Globals[j].GetName(), temp.GetName()), "Incorrect global association with %s", temp.GetName());
			int value = temp.TextureType | (temp.ComparisonFilter * SPECIAL_TEMP_SUBTYPE_SAMPLER_BIT);
			grcAssertf(value>=0 && value<=0xFF, "Invalid global subtype detected");
			m_GlobalSubTypes[j] = static_cast<u8>(value);
		}
#endif //EFFECT_TRACK_MSAA_ERRORS
		if( sm_Globals.GetCount() )
			lastSemanticHash = j>=0 ? sm_Globals[j].GetSemanticHash() : 0;
	}
	// Restore any active memory container
	sysMemAllocator::SetContainer(prev);

#if __D3D11 || RSG_ORBIS
	{
		bool bFound;
		for (int i = 0; i < sm_Globals.GetCount(); i++)
		{
			if (sm_Globals[i].GetCBufferNameHash() == 0)
				continue;
				
			bFound = false;
			for (int j = 0; j < sm_GlobalsCBuf.GetCount(); j++)
			{
				if (sm_Globals[i].GetCBufferNameHash() == sm_GlobalsCBuf[j].GetNameHash())
				{
					bFound = true;
					u32 hash = sm_GlobalsCBuf[j].GetNameHash();

					if (hash == ATSTRINGHASH("rage_matrices",0x3d8e4e7f))	// MATRIX_BASE
						g_MatrixBase = &sm_GlobalsCBuf[j];
					else if (hash == ATSTRINGHASH("rage_bonemtx",0xb7df3c3b)) // BONE_MTX
					{
						g_SkinningBase = &sm_GlobalsCBuf[j];
#if RSG_DURANGO || RSG_ORBIS
						g_SkinningBase->SetIsExplicitBind(true);
#endif
					}
#if RAGE_INSTANCED_TECH
					else if (hash == ATSTRINGHASH("rage_cbinst_matrices",0xe526ea0d)) // INSTANCING MTX
						g_InstBase = &sm_GlobalsCBuf[j];
					else if (hash == ATSTRINGHASH("rage_cbinst_update",0xc8d18e00))
						g_InstUpdateBase = &sm_GlobalsCBuf[j];
#endif
					else if (hash == ATSTRINGHASH("rage_clipplanes",0x2a8dd94a))
						 GRCDEVICE.SetClipPlanesConstBuffer(&sm_GlobalsCBuf[j]);

					sm_Globals[i].SetParentCBuf(&sm_GlobalsCBuf[j]);
					if (sm_Globals[i].Data != NULL)
					{
						SetGlobalFloatCommon((grcEffectGlobalVar)(i+1), (float*)sm_Globals[i].Data, sm_Globals[i].Count, (grcEffect::VarType)sm_Globals[i].GetType());
					}
					break;
				}
			}
			Assertf(bFound,"Global variable '%s' (defined in %s) probably needs to be wrapped into a constant buffer",sm_Globals[i].Name.c_str(),GetEffectName());
		}
	}
#endif

	if (g_sysPlatform == platform::WIN32PC || g_sysPlatform == platform::WIN64PC || g_sysPlatform == platform::DURANGO || rage::g_sysPlatform == rage::platform::ORBIS || magic >= 'f')
	{
		count = S.GetCh();
		m_LocalsCBuf.Resize(count);
		for (int i = 0; i < count ; i++)
		{
			m_LocalsCBuf[i] = grcCBuffer::LoadConstantBuffer(S);
			//m_LocalsCBuf[i].Load(S);
			//m_LocalsCBuf[i].Init();
		}
	}

	// Load textures at bottom of parameter list
	// and constants from the top so that textures are all first.
	count = S.GetCh();
	if (count > 0)
	{
		m_Locals.Resize(count);

#if TEXTURES_FIRST
		int ti = 0, ci = count;
		while (count--) {
			int type = S.GetCh();
			if (type == VT_TEXTURE)
			{
				m_Locals[ti].Load(type, S, ti ? m_Locals[ti-1].GetSemanticHash() : 0);
				++ti;
			}
			else
			{
				--ci;
				m_Locals[ci].Load(type, S, 0);
			}
		}
		Assert(ti == ci);
#else
		for (int i=0; i<count; i++)
		{
			int type = S.GetCh();
			m_Locals[i].Load(type, S, type==VT_TEXTURE && i ? m_Locals[i-1].GetSemanticHash() : 0);
		}
#endif // TEXTURES_FIRST
	}	

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	{
		for (int i = 0; i < m_Locals.GetCount(); i++)
		{
			if (m_Locals[i].GetCBufferNameHash() == 0)
			{	// This means that the variable isn't used by any entry points in this effect.
				continue;
			}

			const VarType varType = static_cast<VarType>( m_Locals[i].GetType() );
			if ((	varType == VT_TEXTURE		|| varType == VT_STRUCTUREDBUFFER	|| varType == VT_BYTEADDRESSBUFFER	||
					varType == VT_UAV_TEXTURE	|| varType == VT_UAV_STRUCTURED		|| varType == VT_UAV_BYTEADDRESS	||
					varType == VT_SAMPLERSTATE) && (m_Locals[i].GetCBufferNameHash() == 0)) 
			{
				// textures/structured buffers and samplerstates do not need to be in constant buffers
				m_Locals[i].SetParentCBuf(NULL);
				continue;
			}

			int j;
			for (j = 0; j < m_LocalsCBuf.GetCount(); j++)
			{
				if (m_Locals[i].GetCBufferNameHash() == m_LocalsCBuf[j]->GetNameHash())
				{
					//Assert to check that no shader constants are getting put in $Globals constant buffer
					Assertf(RSG_ORBIS || strcmp(m_LocalsCBuf[j]->GetName(), "$Globals"), "Constant %s is in $Globals, it needs adding to its own buffer.", m_Locals[i].GetName());
					m_Locals[i].SetParentCBuf(m_LocalsCBuf[j]);
					break;
				}
			}	
		}
	}
#endif	//PC,DURANGO,ORBIS

	count = S.GetCh();
	if (count > 0)
	{
		m_Techniques.Reserve(count);
		for (int i=0; i<count; i++)
			m_Techniques.Append().Load(S);
	}	

#if (RSG_PC || RSG_DURANGO || RSG_ORBIS) && EFFECT_PRESERVE_STRINGS && !RSG_RSC		// Currently the latter is always true but let's not assume that
	// Scan the technique list for _sm40, _sm41, or _sm50 entries.
	// If we find any, strip the suffix from the one that is the best match for our feature level.
	// Also, make sure the original is stripped away if and only if it was replaced.
	u32 level = GRCDEVICE.GetDxFeatureLevel();
	// If -featurelevel is on the command line and has a value, grab that instead.
	PARAM_featurelevel.Get(level);

	for (int i=0; i<count; i++) {
		const char *base = m_Techniques[i].Name;
		const char *suffix = strrchr(base,'_');
		if (suffix && suffix[1]=='s' && suffix[2]=='m' && (suffix[3]=='4'||suffix[3]=='5') && suffix[4] && !suffix[5]) {
			u32 bestLevel = 0;
			int bestIndex = 0;
			suffix += 3;
			ptrdiff_t len = suffix - base;
			for (int j=i; j<count; j++) {
				const char *cur = m_Techniques[j].Name;
				if (!strncmp(base,cur,len)) {
					cur += len;
					u32 thisLevel = (cur[0]=='5'&&cur[1]=='0')? 1100 : (cur[0]=='4'&&cur[1]=='1')? 1010 : (cur[0]=='4'&&cur[1]=='0')? 1000 : 0;
					// If this level is the highest we've seen, but is not so high we don't support it, remember it.
					if (thisLevel > bestLevel && thisLevel <= level) {
						bestLevel = thisLevel;
						bestIndex = j;
					}
				}
			}
			// Catch bad data
			if (!bestLevel)
				Warningf("Cannot find appropriate feature-specific technique in '%s' in shader %s (must have _sm40, _sm41, or _sm50 suffix)", base, GetEffectName());
			// Otherwise, replace all of the suffixes and recompute all of the hashes.
			// We need to replace the ones that didn't match so we don't get confused and try to fix them again
			// since we place no restriction on the order of the techniques in the file.
			else {
				for (int j=i; j<count; j++) {
					char *cur = const_cast<char*>(m_Techniques[j].Name.m_String);
					if (!strncmp(base,cur,len)) {
						if (j==bestIndex) {
							cur[len-3] = 0;	// Strip _smXX suffix off and recompute hash.
							m_Techniques[j].NameHash = atStringHash(cur);
							// Rescan the entire table looking for an unsuffixed version and rename that out of the way if found
							for (int k=0; k<count; k++) {
								if (k != j && m_Techniques[k].NameHash == m_Techniques[j].NameHash) {
									*const_cast<char*>(m_Techniques[k].Name.m_String) = '-';
									m_Techniques[k].NameHash = atStringHash(m_Techniques[k].Name.m_String);
								}
							}
						}
						else {
							cur[len-3] = '-';		// prevent this from matching again (leave hash code alone)
						}
					}
				}
			}
		}
	}
#endif	//(RSG_PC || RSG_DURANGO || RSG_ORBIS) && EFFECT_PRESERVE_STRINGS && !RSG_RSC

#if RSG_ORBIS
	for (int i=0; i<m_Techniques.GetCount(); i++) {
		Technique &t = m_Techniques[i];
		for (int j=0; j<t.Passes.GetCount(); j++) {
			Technique::Pass &p = t.Passes[j];
			grcVertexProgram	&vp = m_VertexPrograms[		p.VertexProgramIndex	];
			grcHullProgram		&hp = m_HullPrograms[		p.HullProgramIndex		];
			grcDomainProgram	&dp = m_DomainPrograms[		p.DomainProgramIndex	];
			grcGeometryProgram	&gp = m_GeometryPrograms[	p.GeometryProgramIndex	];
			grcFragmentProgram	&fp = m_FragmentPrograms[	p.FragmentProgramIndex	];
			grcComputeProgram	&cp = m_ComputePrograms[	p.ComputeProgramIndex	];
			
			// vp only is legal : depth only code path, used for shadows and depth stuff.
			if( vp.GetProgram() && !fp.GetProgram())
				continue;

			// no bitching on compute shader eithers
			if( cp.GetProgram() )
				continue;

			// Now, this one _is_ weird.
			if (!vp.GetProgram()) {
#if EFFECT_PRESERVE_STRINGS
				grcDebugf1("%s - Technique %s pass %d - missing VP?",currentLoadingEffect,t.Name.c_str(),j);
#endif
				continue;
			}

			char descriptor[64];
#if EFFECT_PRESERVE_STRINGS
			formatf( descriptor, sizeof(descriptor), "%s - Technique %s pass %d", currentLoadingEffect, t.Name.c_str(), j );
#else
			formatf( descriptor, sizeof(descriptor), "%s - Technique 0x%x pass %d", currentLoadingEffect, t.NameHash, j );
#endif
			if (hp.GetProgram())
			{
				if(!dp.GetProgram())
				{
#if EFFECT_PRESERVE_STRINGS
					grcDebugf1("%s - Technique %s pass %d - missing DP?",currentLoadingEffect,t.Name.c_str(),j);
#endif
					continue;
				}

				if (gp.GetProgram())
				{
					//FIXME
				}else
				{
					//FIXME:
					//OrbisMatchPrograms( descriptor, vp, hp );
					//OrbisMatchPrograms( descriptor, hp, dp );
					OrbisMatchPrograms( descriptor, dp, fp );
				}
			}else
			{
				if (gp.GetProgram())
				{
					// this isn't reliable since vp is really export shader
					// needs to find a way to get actual vs 
					//OrbisMatchPrograms( descriptor, vp, gp );
					OrbisMatchPrograms( descriptor, gp, fp );
				}else
				{
					OrbisMatchPrograms( descriptor, vp, fp );
				}
			}
		}
	}
#endif	//RSG_ORBIS

	// for (int i=0; i<sm_Globals.GetCount(); i++)
	//	Assert(!sm_Globals[i].Address || grcConstant::SizeOf(sm_Globals[i].Address));

	ConnectParametersToPasses();

	// Every effect has an implicit material associated with it which is based on whatever
	// defaults are compiled into the shader itself.
	ConstructReferenceInstanceData(m_InstanceData);

	// Count up variables with UI elements that are NOT materials.
	int instCount = 0;
	for (int i=0; i<m_Locals.GetCount(); i++)
	{
		if (HasAnnotation(GetVarByIndex(i),ATSTRINGHASH("UIName",0xf6202a0)))
			++instCount;
	}

#if EFFECT_PRESERVE_STRINGS
	{
		USE_DEBUG_MEMORY();
		if (instCount > 0)
		{
			m_VarInfo.Reserve(instCount);
		}		
		
		for (int i=0; i<m_Locals.GetCount(); i++)
		{
			grcEffectVar v = GetVarByIndex(i);
			if (!HasAnnotation(v,ATSTRINGHASH("UIName",0xf6202a0)))
				continue;
			int annoCount;
			bool isGlobal;
			VariableInfo &info = m_VarInfo.Append();
			GetVarDesc(v, info.m_Name, info.m_Type, annoCount, isGlobal, &info.m_ActualName);

			info.m_TypeName = grcEffect::GetTypeName(info.m_Type);
			info.m_UiName = GetAnnotationData(v,"UIName",info.m_Name);
			info.m_AssetName = GetAnnotationData(v,"UIAssetName",(char*)0);
			info.m_UiWidget = GetAnnotationData(v,"UIWidget",(char*)0);
			info.m_UiHelp = GetAnnotationData(v,"UIHelp",(char*)0);
			info.m_UiHint = GetAnnotationData(v,"UIHint",(char*)0);
			info.m_UiMin = GetAnnotationData(v,"UIMin",0.0f);
			info.m_UiMax = GetAnnotationData(v,"UIMax",1.0f);
			info.m_UiStep = GetAnnotationData(v,"UIStep",0.001f);
			info.m_UiHidden = GetAnnotationData(v,"UIHidden",0)==0?false:true;
			info.m_UvSetIndex = GetAnnotationData(v,"UvSetIndex",0);
			info.m_UvSetName = GetAnnotationData(v,"UvSetName",(char*)0);
			info.m_TextureOutputFormats = GetAnnotationData(v,"TextureOutputFormats",(char*)0);
			info.m_iNoOfMipLevels = GetAnnotationData(v,"NoOfMipLevels",0);
			info.m_MaterialDataUvSetIndex = GetAnnotationData(v,"MaterialDataUvSetIndex",-1);
			info.m_GeoValueSource = GetAnnotationData(v,"GeoValueSource",(char*)0);
			info.m_ArrayCount = 1;
			info.m_IsMaterial = (m_Locals[i].Usage & USAGE_MATERIAL) != 0;
			info.m_TCPTemplate = GetAnnotationData(v,"TCPTemplate","");
			info.m_TCPTemplateRelative = GetAnnotationData(v,"TCPTemplateRelative","");
			info.m_TCPAllowOverride = GetAnnotationData(v,"TCPAllowOverride",1);
		}
	}
#endif

	// Assign technique groups (note this is after we've possibly renamed some techniques on PC)
	// Displayf("Shader %s", currentLoadingEffect);
	memset(m_TechniqueMap, 0, sizeof(m_TechniqueMap));
	for (int i=0; i<sm_TechniqueGroupHashes.GetCount(); i++) {
		for (int j=0; j<RMC_COUNT; j++)
		{
			m_TechniqueMap[i][j] = (u8) LookupTechniqueByHash(sm_TechniqueGroupHashes[i][j].GetHash());
			/*
			if (m_TechniqueMap[i][j] != 0)
				Displayf("Technique %d %d - %s", i, j, GetTechniqueName((grcEffectTechnique)m_TechniqueMap[i][j]));
			*/
		}
	}

#if __BANK && EFFECT_PRESERVE_STRINGS && __D3D11
	// Set label in PIX for DX11 debugging.
	SetPIXLabel();
#endif //__BANK && EFFECT_PRESERVE_STRINGS && __D3D11

#if THREADED_SHADER_LOAD
	sm_EffectToken.Unlock();
#endif // THREADED_SHADER_LOAD
}


#if __BANK && EFFECT_PRESERVE_STRINGS && __D3D11

void grcEffect::SetPIXLabel()
{
	SetPIXLabelInPrograms( m_VertexPrograms );
	SetPIXLabelInPrograms( m_FragmentPrograms );

#if __WIN32PC
	// DX11 TODO Should this define be __DX11 or similar,
	SetPIXLabelInPrograms( m_ComputePrograms);
	SetPIXLabelInPrograms( m_DomainPrograms );
	SetPIXLabelInPrograms( m_GeometryPrograms );
	SetPIXLabelInPrograms( m_HullPrograms );
#endif //__WIN32PC
}

template <class T>
void grcEffect::SetPIXLabelInPrograms(grcArray <T> &ProgramList)
{
#if !__D3D11_MONO_DRIVER
	int i;

	for( i=0; i<ProgramList.GetCount(); i++)
	{
		// Collect the program entry point(this already has the effect name/path pre-appended).
		const char *pEntryName = ProgramList[i].GetEntryName();

		// Set is as the debug object name, PIX will display this.
		if( ProgramList[i].GetProgram() )
		{
			((ID3D11DeviceChild *)ProgramList[i].GetProgram())->SetPrivateData(WKPDID_D3DDebugObjectName, StringLength(pEntryName), (const void *)pEntryName);
		}
	}
#else // !__D3D11_MONO_DRIVER
	ProgramList;
#endif	// !__D3D11_MONO_DRIVER
}

#endif //__BANK && EFFECT_PRESERVE_STRINGS && __D3D11

grcEffect::VariableInfo::VariableInfo()
	: m_Name(0), m_ActualName(0), m_Type(grcEffect::VT_FLOAT), 
	m_TypeName("float"), m_UiName(0), m_UiWidget(0), m_UiHelp(0), m_UiHint(0), m_AssetName(0), m_GeoValueSource(0), 
	m_UiMin(0.0f), m_UiMax(1.0f), m_UiStep(0.001f), m_UiHidden(false), m_UvSetIndex(0), 
	m_UvSetName(0), m_TextureOutputFormats(0), m_iNoOfMipLevels(0), m_MaterialDataUvSetIndex(-1), 
	m_ArrayCount(1), m_IsMaterial(false)
{
	// EMPTY
}

int grcEffect::FindTechniqueGroupId(const char *name) {
	char buf[64];
	if (strcmp(name,"default")) {
		safecpy(buf, name);
		safecat(buf,"_draw");
	}
	else
		safecpy(buf,"draw");
	u32 hash = atStringHash(buf);
	for (int i=0; i<sm_TechniqueGroupHashes.GetCount(); i++)
		if (sm_TechniqueGroupHashes[i][RMC_DRAW].GetHash() == hash)
			return i;
	return -1;
}

#if EFFECT_PRESERVE_STRINGS
const char* grcEffect::GetTechniqueGroupName(int techGroupId, eDrawType type)
#else
const char* grcEffect::GetTechniqueGroupName(int UNUSED_PARAM(techGroupId), eDrawType UNUSED_PARAM(type))
#endif
{
#if EFFECT_PRESERVE_STRINGS
	return sm_TechniqueGroupNames[techGroupId][type].c_str();
#else
	return NULL;
#endif
}

int grcEffect::RegisterTechniqueGroup(const char *name) {
	int result = FindTechniqueGroupId(name);
	if (result < 0) {
		result = sm_TechniqueGroupHashes.GetCount();
		atHashString *nameHash = &sm_TechniqueGroupHashes.Append()[0];

#if EFFECT_PRESERVE_STRINGS
		atString *astrName = &sm_TechniqueGroupNames.Append()[0];
#endif

		char prefix[128];
		if (!strcmp(name,"default"))
			prefix[0] = 0;
		else {
			strcpy(prefix,name);
			strcat(prefix,"_");
		}
		int len = StringLength(prefix);
		strcpy(prefix + len,"draw");
		nameHash[RMC_DRAW].SetFromString(prefix);
#if EFFECT_PRESERVE_STRINGS
		astrName[RMC_DRAW] = prefix;
#endif
		strcpy(prefix + len,"drawskinned");
		nameHash[RMC_DRAWSKINNED].SetFromString(prefix);
#if EFFECT_PRESERVE_STRINGS
		astrName[RMC_DRAWSKINNED] = prefix;
#endif

#if RAGE_SUPPORT_TESSELLATION_TECHNIQUES	
		strcpy(prefix + len,"drawtessellated");
		nameHash[RMC_DRAW_TESSELLATED].SetFromString(prefix);
# if EFFECT_PRESERVE_STRINGS
		astrName[RMC_DRAW_TESSELLATED] = prefix;
# endif
		strcpy(prefix + len,"drawskinnedtessellated");
		nameHash[RMC_DRAWSKINNED_TESSELLATED].SetFromString(prefix);
# if EFFECT_PRESERVE_STRINGS
		astrName[RMC_DRAWSKINNED_TESSELLATED] = prefix;
# endif
#endif // RAGE_SUPPORT_TESSELLATION_TECHNIQUES
#if RAGE_INSTANCED_TECH
		strcpy(prefix + len,"drawinstanced");
		nameHash[RMC_DRAW_INSTANCED].SetFromString(prefix);
# if EFFECT_PRESERVE_STRINGS
		astrName[RMC_DRAW_INSTANCED] = prefix;
# endif
		strcpy(prefix + len,"drawskinnedinstanced");
		nameHash[RMC_DRAWSKINNED_INSTANCED].SetFromString(prefix);
# if EFFECT_PRESERVE_STRINGS
		astrName[RMC_DRAWSKINNED_INSTANCED] = prefix;
# endif

# if RAGE_SUPPORT_TESSELLATION_TECHNIQUES	
		strcpy(prefix + len,"drawinstancedtessellated");
		nameHash[RMC_DRAW_INSTANCED_TESSELLATED].SetFromString(prefix);
#  if EFFECT_PRESERVE_STRINGS
		astrName[RMC_DRAW_INSTANCED_TESSELLATED] = prefix;
#  endif
		strcpy(prefix + len,"drawskinnedinstancedtessellated");
		nameHash[RMC_DRAWSKINNED_INSTANCED_TESSELLATED].SetFromString(prefix);
#  if EFFECT_PRESERVE_STRINGS
		astrName[RMC_DRAWSKINNED_INSTANCED_TESSELLATED] = prefix;
#  endif
# endif
#endif	//RAGE_INSTANCED_TECH
	}
	return result;
}



void grcEffect::Shutdown()
{
#if !__TOOL
	sysMemContainer container(m_Container);
	container.BeginShutdown();
#endif
	m_Techniques.Reset();
	m_Locals.Reset();

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	m_LocalsCBuf.Reset();
#endif

	m_Properties.Reset();
	m_VertexPrograms.Reset();
	m_FragmentPrograms.Reset();

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	m_ComputePrograms.Reset();
	m_DomainPrograms.Reset();
	m_GeometryPrograms.Reset();
	m_HullPrograms.Reset();
#endif // RSG_PC || RSG_DURANGO

#if !__TOOL
	container.EndShutdown();
#endif
	m_VarInfo.Reset();

	m_InstanceData.Shutdown();
}

#if EFFECT_USE_CONSTANTBUFFERS
static int SortCounters(grcCBuffer* const* lhs, grcCBuffer* const* rhs)
{	
	u32 lhs_value = (*lhs)->GetRegister(0);
	u32 rhs_value = (*rhs)->GetRegister(0);

	if (lhs_value < rhs_value)
		return 1;
	else if (lhs_value > rhs_value)
		return -1;
	else
		return 0;
}

/* static int SortCounters1(grcParameter* const* lhs, grcParameter* const* rhs)
{	
	u32 lhs_value = (*lhs)->GetCBufOffset();
	u32 rhs_value = (*rhs)->GetCBufOffset();

	if (lhs_value > rhs_value)
		return 1;
	else if (lhs_value < rhs_value)
		return -1;
	else
		return 0;
} */

void grcProgram::ConnectCBuffer(atArray<grcParameter> & locals,ShaderType type)
{
	atArray<grcCBuffer *>	CBufferContainer;
	atArray<grcParameter *>	TexContainer;

	// figure out the max index for tex container
	u32 numTextures = 0, numUnorderedViews = 0;
	for (int i=0; i<m_Constants.GetCount(); i++)
	{
		u32 thisConstant = m_Constants[i];

		// connect local cbuffer 
		for (int j = 0; j < locals.GetCount(); j++)
		{
			if (thisConstant == locals[j].GetNameHash())
			{
				if (locals[j].GetCBufOffset() < INVALID_OFFSET)
				{
					switch( locals[j].GetType() )
					{
					case grcEffect::VT_TEXTURE:
					case grcEffect::VT_STRUCTUREDBUFFER:
					case grcEffect::VT_BYTEADDRESSBUFFER:
						numTextures = Max(locals[j].GetCBufOffset()+1,numTextures);
						break;
					case grcEffect::VT_UAV_TEXTURE:
					case grcEffect::VT_UAV_STRUCTURED:
					case grcEffect::VT_UAV_BYTEADDRESS:
						numUnorderedViews = Max(locals[j].GetCBufOffset()+1,numUnorderedViews);
						break;
					}
				}
#if EFFECT_PRESERVE_STRINGS
				else
				{
					grcDebugf2("Local Buffer Offset %x Exceeds Max on %s", locals[j].GetCBufOffset(), locals[j].GetName());
				}
#endif
			}
		}


		for (int j = 0; j < grcEffect::GetCBufferGlobalCount(); j++)
		{
			const grcParameter &param = *grcEffect::GetCBufferGlobal(j);
			if (thisConstant == param.GetNameHash())
			{
				switch (param.GetType())
				{
				case grcEffect::VT_TEXTURE:
				case grcEffect::VT_STRUCTUREDBUFFER:
				case grcEffect::VT_BYTEADDRESSBUFFER:
					if (param.GetCBufOffset() < INVALID_OFFSET)
					{
						numTextures = Max( param.GetCBufOffset()+1, numTextures );
					}
	#if EFFECT_PRESERVE_STRINGS
					else
					{
						grcDebugf2( "Global Buffer Offset %x Exceeds Max on %s", param.GetCBufOffset(), param.GetName() );
					}
	#endif
					break;
				default:;
				}
			}
		}
	}

	Assert( numTextures<=EFFECT_TEXTURE_COUNT && numUnorderedViews<=EFFECT_UAV_BUFFER_COUNT );
	TexContainer.Reserve(numTextures+numUnorderedViews);
	TexContainer.Resize(numTextures+numUnorderedViews);
	for (int i = 0; i < TexContainer.GetCount(); i++)
		TexContainer[i] = NULL;

	for (int i=0; i<m_Constants.GetCount(); i++)
	{
		u32 thisConstant = m_Constants[i];
		int thisConstantLocalIdx = -1;

		// connect local cbuffer 
		for (int j = 0; j < locals.GetCount(); j++)
		{
			if (thisConstant == locals[j].GetNameHash())
			{
				switch(locals[j].GetType())
				{
				case grcEffect::VT_UAV_TEXTURE:
				case grcEffect::VT_UAV_STRUCTURED:
				case grcEffect::VT_UAV_BYTEADDRESS:
					if (locals[j].GetCBufOffset() < EFFECT_UAV_BUFFER_COUNT)
					{
						TexContainer[numTextures + locals[j].GetCBufOffset()] = &locals[j];
						thisConstantLocalIdx = j;
					}
#if EFFECT_PRESERVE_STRINGS
					else
					{
						grcDebugf2("Not connecting %s - %d to texture container", locals[j].GetName(), j);
					}
#endif
					break;
				case grcEffect::VT_TEXTURE:
				case grcEffect::VT_STRUCTUREDBUFFER:
				case grcEffect::VT_BYTEADDRESSBUFFER:
					if (locals[j].GetCBufOffset() < EFFECT_TEXTURE_COUNT)
					{
						grcParameter* &param = TexContainer[ locals[j].GetCBufOffset() ];
#if EFFECT_PRESERVE_STRINGS
						Assertf( param == NULL || param == &locals[j],
								"Local Offset %d - %s Texture Offset %d - %s ", 
								locals[j].GetCBufOffset(),	locals[j].GetName(), 
								param->GetCBufOffset(),		param->GetName() );
#endif // EFFECT_PRESERVE_STRINGS
						param = &locals[j];
						thisConstantLocalIdx = j;
					}
#if EFFECT_PRESERVE_STRINGS
					else
					{
						grcDebugf2("Not connecting %s - %d to texture container", locals[j].GetName(), j);
					}
#endif
					break;
				case grcEffect::VT_UNUSED1:
				case grcEffect::VT_UNUSED2:
				case grcEffect::VT_UNUSED3:
				case grcEffect::VT_UNUSED4:
					Assert( !locals[j].GetParentCBuf() );
					break;
				default:
					int count = CBufferContainer.GetCount();
					bool duplicate = false;
					for (int k = 0; k < count; k++)
					{
						if (CBufferContainer[k]->GetNameHash() == locals[j].GetParentCBuf()->GetNameHash())
						{
							duplicate = true;
							break;
						}
					}
					if (!duplicate)
					{
						CBufferContainer.Grow() = locals[j].GetParentCBuf();
						Assert(CBufferContainer.Top());
					}
				}
				break;	//terminate the loop
			}
		}

		if(thisConstantLocalIdx != -1)
		{
		#if !__FINAL && EFFECT_PRESERVE_STRINGS
			for (int j = 0; j < grcEffect::GetCBufferGlobalCount(); j++)
			{
				grcParameter &param = *grcEffect::GetCBufferGlobal(j);
				if ((thisConstant == param.GetNameHash()) || (thisConstant == param.GetSemanticHash()))
				{
					grcWarningf("grcProgram::ConnectCBuffer()...local %s clashes with a global.\n", locals[thisConstantLocalIdx].GetName());
					break;
				}
			}
		#endif // !__FINAL && EFFECT_PRESERVE_STRINGS
			// Move onto the next constant.
			continue;
		}

		// connect global cbuffer
		for (int j = 0; j < grcEffect::GetCBufferGlobalCount(); j++)
		{
			grcParameter &param = *grcEffect::GetCBufferGlobal(j);
			if ((thisConstant == param.GetNameHash()) || (thisConstant == param.GetSemanticHash()))
			{
				switch( param.GetType() )
				{
				case grcEffect::VT_TEXTURE:
				case grcEffect::VT_STRUCTUREDBUFFER:
				case grcEffect::VT_BYTEADDRESSBUFFER:
					if (param.GetCBufOffset() < INVALID_OFFSET)
					{
						grcParameter * &localParam = TexContainer[ param.GetCBufOffset() ];
						Assert( localParam == NULL || localParam==&param );
						localParam = &param;
					}
#if EFFECT_PRESERVE_STRINGS
					else
					{
						grcDebugf2("Global Texture %s has invalid offset %d", param.GetName(), param.GetCBufOffset());
					}
#endif
					break;
				default:
					{
						int count = CBufferContainer.GetCount();
						bool duplicate = false;
						for (int k = 0; k < count; k++)
						{
							if (CBufferContainer[k]->GetNameHash() == param.GetParentCBuf()->GetNameHash())
							{
								duplicate = true;
								break;
							}
						}
						if (!duplicate)
						{
							CBufferContainer.Grow() = param.GetParentCBuf();
							Assert(CBufferContainer.Top());
						}
					}
				}
				break;	//terminate the loop
			}
		}
	}

	// sort cbuffer container based on register value (slot value)
	CBufferContainer.QSort(0,CBufferContainer.size(),&SortCounters);

	m_numCBuffers = CBufferContainer.GetCount();
	memset((void*)&(m_pCBuffers[0]),0,sizeof(grcCBuffer*)*m_numCBuffers);

	for (int Slot=0;Slot<m_numCBuffers; ++Slot)
	{
		m_pCBuffers[Slot] = CBufferContainer[Slot];
	}

	const int count = CBufferContainer.GetCount();
	Assert(count <= EFFECT_CONSTANT_BUFFER_COUNT);

	m_CBufStartSlot = m_CBufEndSlot = 0;
	
#if EFFECT_CACHE_PROGRAMRESOURCES
	m_ppDeviceCBuffers = NULL;
#endif // EFFECT_CACHE_PROGRAMRESOURCES

	if (count > 0)
	{
		m_CBufStartSlot = EFFECT_CONSTANT_BUFFER_COUNT-1;
		m_CBufEndSlot = 0;

		for (u8 i = 0; i < (u8)count; i++)
		{
			if (CBufferContainer[i]->GetBuffer() || RSG_DURANGO || RSG_ORBIS)
			{
				u8 Slot = (u8)CBufferContainer[i]->GetRegister(type);

				m_CBufStartSlot = Min(Slot, m_CBufStartSlot);
				m_CBufEndSlot   = Max(Slot, m_CBufEndSlot);
			}
		}
		Assert(m_CBufStartSlot <= m_CBufEndSlot);

#if EFFECT_CACHE_PROGRAMRESOURCES
		m_ppDeviceCBuffers = (void**)rage_new grcBuffer*[EFFECT_CONSTANT_BUFFER_COUNT];
		memset(m_ppDeviceCBuffers, 0, sizeof(grcBuffer*)*EFFECT_CONSTANT_BUFFER_COUNT);

		for (int i = 0; i < m_numCBuffers; i++)
		{
			grcCBuffer *	pCBuffer = m_pCBuffers[i];
			const u32		slot = pCBuffer->GetRegister(type);
			
#if EFFECT_CACHE_PROGRAMRESOURCES
			m_ppDeviceCBuffers[slot] = (ID3D11Buffer*)(pCBuffer->GetBuffer());
			Assert(m_ppDeviceCBuffers[slot] != NULL);
#endif // EFFECT_USE_OFFSETCBS
		}
		m_CBufferFingerprint = atDataHash((const char*)m_ppDeviceCBuffers, sizeof(void *)*EFFECT_CONSTANT_BUFFER_COUNT);

#endif // EFFECT_CACHE_PROGRAMRESOURCES
	}

	// Determine the slots that need to be set.
	Assert(numTextures <= EFFECT_TEXTURE_COUNT);

	m_TexStartSlot = m_TexEndSlot = EFFECT_TEXTURE_COUNT;

	if (numTextures > 0)
	{
		m_TexStartSlot = EFFECT_TEXTURE_COUNT-1;
		m_TexEndSlot   = 0;

		for (u8 i = 0; i < (u8)numTextures; i++)
		{
			if(TexContainer[i] != NULL)
			{
				s8 Slot = (s8)TexContainer[i]->GetCBufOffset();

				m_TexStartSlot = Min(Slot, m_TexStartSlot);
				m_TexEndSlot   = Max(Slot, m_TexEndSlot);
			}
		}
		Assert(m_TexStartSlot <= m_TexEndSlot);
	}
	else
	{
		//When we have no textures set the slots to -1 so when we use this for UAVS it looks at element 0.
		m_TexStartSlot = m_TexEndSlot = -1;
	}

	for (int Slot=0; Slot<TexContainer.GetCount(); ++Slot)
	{
		m_pTexContainers[Slot] = TexContainer[Slot];
	}
	m_numTexContainers = TexContainer.GetCount();
}

#if __ASSERT
bool grcProgram::DoesUseCBuffer(grcCBuffer *pBuffer)
{
	int i;

	for(i=0; i<m_numCBuffers; i++)
	{
		if(m_pCBuffers[i] == pBuffer)
		{
			return true;
		}
	}
	return false;
}
#endif //__ASSERT

#endif // EFFECT_USE_CONSTANTBUFFERS

#if __D3D11 || (RSG_ORBIS && ENABLE_LCUE)
static bool IsInLocalsList(u32 thisConstant, const atArray<grcParameter> & locals)
{
	for(int i=0; i<locals.GetCount(); i++) {
		if(thisConstant == locals[i].GetNameHash())	{
			return true;
		}
	}
	return false;
}
#endif

void grcProgram::Connect(const atArray<grcParameter> & locals EFFECT_TRACK_ERRORS_ONLY(,const GlobalSubTypesArray &globalSubTypes))
{
#if __D3D11 || (RSG_ORBIS && ENABLE_LCUE)
	u16 globalTypes[EFFECT_TEXTURE_COUNT] = { 0 };
	u32 textureSlotsUsed = 0;

	// Visit the constants building up the set of globals textures/structured buffers which are used.
	for (int i=0; i<m_Constants.GetCount(); i++) {
		u32 thisConstant = m_Constants[i];
		for (int j = 0; j < grcEffect::GetCBufferGlobalCount(); j++) {
			grcParameter &param = *grcEffect::GetCBufferGlobal(j);
			if ((thisConstant == param.GetNameHash()) || (thisConstant == param.GetSemanticHash())) {
				if (param.GetType() ==  grcEffect::VT_TEXTURE) {
					// Some textures have invalid slots...Not sure why yet.
					if (param.GetCBufOffset() < INVALID_OFFSET) {
						// If a global has the same name as a local, then the local takes precedent.
						if (IsInLocalsList(thisConstant, locals) == false) {
#if RSG_ORBIS && ENABLE_LCUE
							// Skip samplers that aren't ultimately used by the shader (although why are they in the constants list in the first place?)
							if (!SRO || SRO->samplerOffset[param.GetRegister()] != 0xFFFF) 
#endif
							{
								u16 currentType = SPECIAL_GLOBAL_VT_TEXTURE;
#if EFFECT_TRACK_COMPARISON_ERRORS
								currentType |= (globalSubTypes[j] & SPECIAL_TEMP_SUBTYPE_SAMPLER_BIT) << SPECIAL_GLOBAL_SUBTYPE_SHIFT;
#endif //EFFECT_TRACK_COMPARISON_ERRORS
#if EFFECT_TRACK_MSAA_ERRORS
								int exactId = j;
								if (thisConstant != param.GetNameHash())
								{
									for (exactId=j+1; exactId < grcEffect::GetCBufferGlobalCount() &&
										thisConstant != grcEffect::GetCBufferGlobal(exactId)->GetNameHash();
										++exactId);
								}
								if (AssertVerify(exactId < grcEffect::GetCBufferGlobalCount()))
								{
									currentType |= globalSubTypes[exactId] << SPECIAL_GLOBAL_SUBTYPE_SHIFT;
								}
#endif //EFFECT_TRACK_MSAA_ERRORS
#if EFFECT_TRACK_GLOBALS
								if (AssertVerify(j<0x100))
								{
									m_GlobalByTextureRegister[param.GetRegister()] = static_cast<u8>(j);
								}
#endif //EFFECT_TRACK_GLOBALS
								globalTypes[param.GetRegister()] = currentType;
								textureSlotsUsed |= (1 << param.GetRegister());
							}

						}
					}
				}
				else if (param.GetType() ==  grcEffect::VT_STRUCTUREDBUFFER) {
					// ORBIS TODO: Probably need a test similar to above, but for something other than samplers?
					globalTypes[param.GetCBufOffset()] = SPECIAL_GLOBAL_VT_STRUCTUREDBUFFER;
				}
				/*
				else if (param.GetType() ==  grcEffect::VT_SAMPLERSTATE){
					globalTypes[param.GetRegister()] = SPECIAL_GLOBAL_VT_TEXTURE;
				}
				*/
				break;
			}
		}
	}
#endif // __D3D11 || (RSG_ORBIS && ENABLE_LCUE)

	int stored = 0;
	for (int i=0; i<m_Constants.GetCount(); i++) {
		u32 thisConstant = m_Constants[i];
		for (int j=0; j<locals.GetCount(); j++) {
			if (thisConstant != locals[j].GetNameHash())
				continue;
			int targetId = j;
#if DX10_TEXTURE_GLUE
			// The sampler variable is followed by the texture object one
			// Check the previous variable to be the same register texture and use it
			if (j>0 && locals[j-1].IsSamplerFor(locals[j]))
			{
# if EFFECT_PRESERVE_STRINGS
				Assertf(!locals[j].IsMsaaTexture(), "Detected a sampler (%s) used for MSAA texture (%s) by the shader (%s)",
					locals[j-1].GetName(), locals[j].GetName(), GetEntryName());
# endif
				--targetId;
			}
#endif	//DX10_TEXTURE_GLUE
#if RSG_ORBIS && ENABLE_LCUE
			// Skip samplers that aren't ultimately used by the shader (although why are they in the constants list in the first place?)
			if (locals[targetId].GetType() == grcEffect::VT_TEXTURE && SRO && SRO->samplerOffset[locals[targetId].GetRegister()] == 0xFFFF) 
				continue;
#endif
			m_Constants[stored++] = targetId;
#if EFFECT_PRESERVE_STRINGS
			grcDebugf3("  Connecting local '%s'(%d) to constant %d",locals[j].GetName(),targetId,i);
#endif
#if __D3D11 || (RSG_ORBIS && ENABLE_LCUE)
			if (locals[targetId].GetType() == grcEffect::VT_TEXTURE)
				textureSlotsUsed |= (1 << locals[targetId].GetRegister());
#endif
			break;
		}
	}
	Assert(stored <= m_Constants.GetCount());

#if __D3D11 || (RSG_ORBIS && ENABLE_LCUE)
	// Append the locals with the specially formed indices which indicate global textures/structured buffers.
	for (int i=0; i<EFFECT_TEXTURE_COUNT; i++) {
		if (globalTypes[i] != 0)
			m_Constants[stored++] = globalTypes[i] | i;
# if RSG_ORBIS
		// Last-ditch attempt to find globals that weren't in the constant table for some reason (assume it's a texture)
		else if (SRO && !(textureSlotsUsed & (1 << i)) && SRO->resourceOffset[i] != 0xFFFF) {
			m_Constants[stored++] = SPECIAL_GLOBAL_VT_TEXTURE | i;
		}
# endif
	}
#endif //__D3D11 || (RSG_ORBIS && ENABLE_LCUE)

	// Shrink the array to remove globals.
	m_Constants.SetCount(stored);
}

void grcEffect::ConnectParametersToPasses()
{
	for (int i=0; i<m_VertexPrograms.GetCount(); i++) {
#if EFFECT_PRESERVE_STRINGS
		grcDebugf3("Processing VP %s",m_VertexPrograms[i].GetEntryName());
#endif

#if EFFECT_USE_CONSTANTBUFFERS
		m_VertexPrograms[i].ConnectCBuffer(m_Locals,VS_TYPE);
#endif
		m_VertexPrograms[i].Connect(m_Locals EFFECT_TRACK_ERRORS_ONLY(,m_GlobalSubTypes));
	}
	for (int i=0; i<m_FragmentPrograms.GetCount(); i++) {
#if EFFECT_PRESERVE_STRINGS
		grcDebugf3("Processing FP %s",m_FragmentPrograms[i].GetEntryName());
#endif
#if EFFECT_USE_CONSTANTBUFFERS
		m_FragmentPrograms[i].ConnectCBuffer(m_Locals,PS_TYPE);
#endif
		m_FragmentPrograms[i].Connect(m_Locals EFFECT_TRACK_ERRORS_ONLY(,m_GlobalSubTypes));
	}
	
#if EFFECT_USE_CONSTANTBUFFERS
	for (int i=0; i<m_GeometryPrograms.GetCount(); i++) {
#if EFFECT_PRESERVE_STRINGS
		grcDebugf3("Processing GP %s",m_GeometryPrograms[i].GetEntryName());
#endif
		m_GeometryPrograms[i].ConnectCBuffer(m_Locals,GS_TYPE);
		m_GeometryPrograms[i].Connect(m_Locals EFFECT_TRACK_ERRORS_ONLY(,m_GlobalSubTypes));
	}

	for (int i=0; i<m_ComputePrograms.GetCount(); i++) {
#if EFFECT_PRESERVE_STRINGS
		grcDebugf3("Processing CP %s",m_ComputePrograms[i].GetEntryName());
#endif
		m_ComputePrograms[i].ConnectCBuffer(m_Locals,CS_TYPE);
		m_ComputePrograms[i].Connect(m_Locals EFFECT_TRACK_ERRORS_ONLY(,m_GlobalSubTypes));
	}

	for (int i=0; i<m_DomainPrograms.GetCount(); i++) {
#if EFFECT_PRESERVE_STRINGS
		grcDebugf3("Processing DP %s",m_DomainPrograms[i].GetEntryName());
#endif
		m_DomainPrograms[i].ConnectCBuffer(m_Locals,DS_TYPE);
		m_DomainPrograms[i].Connect(m_Locals EFFECT_TRACK_ERRORS_ONLY(,m_GlobalSubTypes));
	}

	for (int i=0; i<m_HullPrograms.GetCount(); i++) {
#if EFFECT_PRESERVE_STRINGS
		grcDebugf3("Processing DP %s",m_HullPrograms[i].GetEntryName());
#endif
		m_HullPrograms[i].ConnectCBuffer(m_Locals,HS_TYPE);
		m_HullPrograms[i].Connect(m_Locals EFFECT_TRACK_ERRORS_ONLY(,m_GlobalSubTypes));
	}
#endif // __D3D11 || RSG_ORBIS
}


u32 TotalReferenceInstanceData;

void grcEffect::ConstructReferenceInstanceData(grcInstanceData &instanceData)
{
	Assert(instanceData.MaterialName == NULL);

	// Construct the reference instance data.
	int count = m_Locals.GetCount();
	instanceData.pad = instanceData.TextureCount = 0;
	instanceData.Basis = this;
	// instanceData.BasisHash = m_NameHash; // In a union together now.
	Assign(instanceData.Count,count);
	size_t offset = count * sizeof(grcInstanceData::Entry);
	offset = (offset + 15) & ~15;
	Assign(instanceData.TotalSize,offset);
	for (int i=0; i<count; i++)
		instanceData.TotalSize += ((g_Float4SizeByType[m_Locals[i].Type]) * m_Locals[i].Count) << 4;
	for (int i=0; i<count; i++)
		if (m_Locals[i].Type == grcEffect::VT_TEXTURE)
			++instanceData.TextureCount;
	instanceData.SpuSize = instanceData.TotalSize;
	instanceData.TotalSize += 32 + (((instanceData.Count + 3) & ~3) << 2);		// NameHash entries; include some slop for future expansion too.
	if (instanceData.TotalSize >= 512)
        grcDebugf1("Unusually large instance data in shader %s, %u bytes",GetEffectName(),instanceData.TotalSize);
	TotalReferenceInstanceData += instanceData.TotalSize;
	char *base = rage_new char[instanceData.TotalSize];
	instanceData.Entries = (grcInstanceData::Entry*) base;
	u32 *NameHash = (u32*)(base + (instanceData.SpuSize));		// overlaps Parameters
	memset(instanceData.Entries, 0, instanceData.TotalSize);
	for (int i=0; i<count; i++) {
		int thisSize = (g_Float4SizeByType[m_Locals[i].Type] << 4) * m_Locals[i].Count;
		if (thisSize) {
			Assert(m_Locals[i].Count);
			u32 uSize = g_Float4SizeByType[m_Locals[i].Type];
			uSize *= m_Locals[i].Count;
			Assign(instanceData.Entries[i].Count, uSize);
#if EFFECT_CHECK_ENTRY_TYPES
			instanceData.Entries[i].Type = grcInstanceData::ET_FLOAT;
#endif //EFFECT_CHECK_ENTRY_TYPES
			instanceData.Entries[i].Any = base + offset;
			if (m_Locals[i].Data)
				memcpy(instanceData.Entries[i].Any, m_Locals[i].Data, thisSize);
			// else
			//	grcWarningf("Local var '%s' in effect has no default value (will be zero)",m_Locals[i].Name.c_str());
			offset += thisSize;
		}
		else {
			instanceData.Entries[i].Any = NULL;
			instanceData.Entries[i].Count = 0;
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
			ASSERT_ONLY( const VarType varType = static_cast<VarType>( m_Locals[i].Type ); )
			Assert(	varType == grcEffect::VT_TEXTURE	|| varType == grcEffect::VT_STRUCTUREDBUFFER	|| varType == grcEffect::VT_BYTEADDRESSBUFFER	||
					varType == grcEffect::VT_UAV_TEXTURE|| varType == grcEffect::VT_UAV_STRUCTURED		|| varType == grcEffect::VT_UAV_BYTEADDRESS		||
					varType == grcEffect::VT_SAMPLERSTATE );
# if EFFECT_CHECK_ENTRY_TYPES && 0
			// no need to set the Type here, since the value of Any is still NULL
			instanceData.Entries[i].Type = varType == grcEffect::VT_TEXTURE || varType == grcEffect::VT_UAV_TEXTURE || varType == grcEffect::VT_SAMPLERSTATE ?
				grcInstanceData::ET_TEXTURE : grcInstanceData::ET_BUFFER;
# endif
#else
			Assert(m_Locals[i].Type == grcEffect::VT_TEXTURE);
#endif //RSG_PC || RSG_DURANGO || RSG_ORBIS
		}
#if EFFECT_CHECK_ENTRY_TYPES
		grcAssert((m_Locals[i].Register >> 6) == 0);
#endif // EFFECT_CHECK_ENTRY_TYPES
		instanceData.Entries[i].Register = m_Locals[i].Register;
		instanceData.Entries[i].SamplerStateSet = m_Locals[i].SamplerStateSet;
		ASSERT_ONLY(instanceData.Entries[i].SavedSamplerStateSet = INVALID_STATEBLOCK);
		NameHash[i] = m_Locals[i].NameHash;
	}

	// TODO: "drawBucket" is no longer used by any macros or shaders, drop support for it in a week or so.
	Assign(instanceData.DrawBucket,GetPropertyValue("__rage_drawbucket",GetPropertyValue("drawBucket",(int)0)));
	u32 defaultMask = BUCKETMASK_GENERATE(instanceData.DrawBucket);
	instanceData.DrawBucketMask = GetPropertyValue("__rage_drawbucketmask",(int)defaultMask);
	instanceData.IsInstanced = GetPropertyValue("__rage_instanced", 0) != 0;

#if MULTIPLE_RENDER_THREADS > 1
	instanceData.ExpandForMultipleThreads(true);
#endif
}

#if MULTIPLE_RENDER_THREADS
void grcInstanceData::ExpandForMultipleThreads(bool fromShaderLoad)
{
	Entry *old = Entries;
	const int mrt = NUMBER_OF_RENDER_THREADS;
	int allocated = ((Flags & FLAG_EXTRA_DATA) != 0) && !fromShaderLoad;

	// We might have gotten the array the right size already with new resources, so check the flag first to see if we really need to reallocate.
	if (!allocated)
		Entries = (Entry*) rage_new char[TotalSize * mrt];
	
	for (int i=allocated; i<mrt; i++)
	{
		Entry *ne = (Entry*)((char*)Entries + TotalSize * i);
		ptrdiff_t fixup = (char*)ne - (char*)old;
		memcpy(ne,old,TotalSize);
		// Textures are already correct after the memcpy; float data
		// needs to be fixed up.  The payload will already be correct.
		for (int j=0; j<Count; j++)
			if (ne[j].Count)
				ne[j].Any = (char*)ne[j].Any + fixup;
#		if PGHANDLE_REF_COUNT
			// Since we only memcpy'd the data, make sure we inc refs when ref
			// counting is enabled.
			for (int j=0; j<TextureCount; j++)
				ne[j].Texture.IncRef();
#		endif
	}

	if (!allocated) 
	{
		delete[] (char*) old;
		Flags |= FLAG_EXTRA_DATA;
	}
}

void grcInstanceData::CopyDataToMultipleThreads()
{
	if (!(Flags & FLAG_EXTRA_DATA))
	{
		return;
	}
	grcInstanceData::Entry * src = DataForThread(0);
	const int mrt = NUMBER_OF_RENDER_THREADS;
	for (u32 n = 1; n < mrt; n++)
	{
		grcInstanceData::Entry * dst = DataForThread(n);
		memcpy(dst,src,TotalSize);
	}
}
#endif

void grcInstanceData::Shutdown()
{
	if ((Flags & FLAG_MATERIAL) && MaterialName) {
		StringFree(MaterialName);
		MaterialName = NULL;
	}

#if PGHANDLE_REF_COUNT
	for(int dataIndex=0; dataIndex<TextureCount; dataIndex++) {
		Assert(IsTexture(dataIndex));
		Entries[dataIndex].Texture = NULL;
	}
#endif

#if __TOOL
	StringFree(Comment);
	Comment = NULL;
	for(int dataIndex=0; dataIndex<Count; dataIndex++)
		if(IsTexture(dataIndex))
		{
			if(Entries[dataIndex].Texture)
				delete Entries[dataIndex].Texture;
		}
#endif

	TextureCount = 0;
}

grcInstanceData::grcInstanceData() : Entries(NULL), Basis(NULL), Material(NULL), Count(0), TotalSize(0), SpuSize(0), DrawBucket(0), PhysMtl_DEPRECATED(0), Flags(0),
	DrawBucketMask(0), IsInstanced(false), UserFlags(0), pad(0), TextureCount(0)
#if __TOOL
	,Comment(0)
	,mPresetFlags(0)
#else
	,SortKey_DEPRECATED(0)
#endif
{
}


grcInstanceData::~grcInstanceData()
{
	Shutdown();
	delete (char*) Entries;
}


void grcInstanceData::Save(fiStream *S) const
{
	grcEffect &e = GetBasis();
	const char *effectName = e.m_EffectName;
	fprintf(S,"shader \"%s\"\r\n",effectName);
#if __TOOL
	fprintf(S,"comment \"%s\"\r\n",Comment? Comment : "");
#else
	fprintf(S,"comment \"%s\"\r\n",GetMaterialName());
#endif
	fprintf(S,"__rage_drawbucket {\r\n\tint %d\r\n}\r\n",DrawBucket);
	fprintf(S,"__rage_drawbucketmask {\r\n\tint %d\r\n}\r\n",DrawBucketMask);
	if(IsInstanced) fprintf(S,"__rage_instanced {\r\n\tint %d\r\n}\r\n",IsInstanced);
	// u32 *NameHash = (u32*)((char*)Data + (SpuSize));
	bool isMaterial = (Flags & FLAG_MATERIAL) != 0;
	for (int i=0; i<Count; i++) 
	{
		grcEffectVar v = (grcEffectVar) (i + 1);
		if (!e.HasAnnotation(v,ATSTRINGHASH("UIName",0xf6202a0)))
			continue;
		// Is this item a texture?
		bool thisIsTexture = e.m_Locals[i].Type == grcEffect::VT_TEXTURE;
		bool thisIsMaterialParameter = 
#if __TOOL
			GetPresetFlag((grcEffectVar)(i+1)) != 0;
#else
			(e.m_Locals[i].Usage & USAGE_MATERIAL) != 0;
#endif
		// If the object we're saving shouldn't be serialized, skip it.
		if (isMaterial != thisIsMaterialParameter)
			continue;

#if EFFECT_PRESERVE_STRINGS
		fprintf(S,"%s {\r\n\t%s",e.m_Locals[i].Semantic.c_str(),grcEffect::GetTypeName((grcEffect::VarType)e.m_Locals[i].Type));
#else
		fprintf(S,"hash_%x {\r\n\t%s",e.m_Locals[i].SemanticHash,grcEffect::GetTypeName((grcEffect::VarType)e.m_Locals[i].Type));
#endif
		int count = 1;
		if(g_Float4SizeByType[e.m_Locals[i].Type])
		{
			count = Entries[i].Count / g_Float4SizeByType[e.m_Locals[i].Type];
			Assert(count);
			if (count > 1)
				fprintf(S," Count %d",count);
		}
		if (thisIsTexture)
		{
			Assert(count == 1);
			const char *texName = Data()[i].Texture?Data()[i].Texture->GetName():"none";
			fprintf(S,"\t\"%s\"\n",texName);
		}
		else
		{
			float *f = Data()[i].Float;
			switch(e.m_Locals[i].Type) 
			{
				case grcEffect::VT_FLOAT: do fprintf(S,"\t%.8g\r\n",f[0]); while (f+=4, --count); break;
				case grcEffect::VT_VECTOR2: do fprintf(S,"\t%.8g %.8g\r\n",f[0],f[1]); while (f+=4, --count); break;
				case grcEffect::VT_VECTOR3: do fprintf(S,"\t%.8g %.8g %.8g\r\n",f[0],f[1],f[2]); while (f+=4, --count); break;
				case grcEffect::VT_VECTOR4: do fprintf(S,"\t%.8g %.8g %.8g %.8g\r\n",f[0],f[1],f[2],f[3]); while (f+=4, --count); break;
				case grcEffect::VT_MATRIX43: count*=3; do fprintf(S,"\t%.8g %.8g %.8g %.8g\r\n",f[0],f[1],f[2],f[3]); while (f+=3, --count); break;
				case grcEffect::VT_MATRIX44: count<<=2; do fprintf(S,"\t%.8g %.8g %.8g %.8g\r\n",f[0],f[1],f[2],f[3]); while (f+=4, --count); break;
				default: Assert(false);
			}
		}
		fprintf(S,"}\r\n");
	}
}

#if __TOOL
void grcInstanceData::TryMapVariablesFrom(grcInstanceData& oldData)
{
	DrawBucket = oldData.DrawBucket;
	DrawBucketMask = oldData.DrawBucketMask;
	Flags = oldData.Flags;

	grcEffect &oldBasis = oldData.GetBasis();
	grcEffect &basis = GetBasis();

	for (int i=0; i<oldData.GetCount(); i++) 
	{
//		grcEffectVar v = (grcEffectVar) (i + 1);
		// Is this item a texture?
		bool thisIsTexture = oldBasis.m_Locals[i].Type == grcEffect::VT_TEXTURE;
// 		bool thisIsMaterialParameter = 
// #if __TOOL
// 			oldData.GetPresetFlag((grcEffectVar)(i+1)) != 0;
// #else
// 			(e.m_Locals[i].Usage & USAGE_MATERIAL) != 0;
// #endif


		grcEffectVar newVar = basis.LookupVar(oldBasis.m_Locals[i].Semantic.c_str());
		int localIndex = ((int)newVar)-1;
		if(newVar!=grcevNONE && 
			basis.m_Locals[localIndex].Type == oldBasis.m_Locals[i].Type)
		{
			int count = 1;
			if(g_FloatSizeByType[oldBasis.m_Locals[i].Type])
			{
				count = g_FloatSizeByType[oldBasis.m_Locals[i].Type];
				Assert(count);
			}
			Data()[localIndex] = oldData.Data()[i];
			if (thisIsTexture)
			{
				Assert(count == 1);
				if(oldData.Data()[i].Texture)
				{
					//basis.SetVar(*this, newVar, (grcTexture*)&oldData.Data()[i].Texture);
					const char *texName = oldData.Data()[i].Texture->GetName();
					Data()[localIndex].Texture = grcTextureFactory::GetInstance().Create(texName,NULL);
				}
			}
			else
			{
				Data()[localIndex].Float = rage_new float[count];
				memcpy(Data()[localIndex].Float, oldData.Data()[i].Float, sizeof(float)*count);
			}

			if(oldData.mPresetFlags & (1 << (i+1)))
				mPresetFlags |= (1 << (localIndex+1));
		}
	}
}
#endif

bool grcInstanceData::Load(const char *OUTPUT_ONLY(filename),fiTokenizer &T,bool isPreset)
{
	char buf[RAGE_MAX_PATH];
	grcEffect *e = Basis;
	if (isPreset) {
		T.MatchToken("shader");
		T.GetToken(buf,sizeof(buf));
		// Note that to avoid getting the input stream way of of sync, we need to deal with e being NULL.
		e = grcEffect::LookupEffect(atStringHash(buf));
	}
	// u32 *NameHash = (u32*)((char*)Data + (SpuSize));
	if (!e)
		grcErrorf("grcInstanceData::LoadMaterial(%s) cannot find parent effect '%s'",filename,buf);
	else if (!Entries)			// Clone the data now if caller didn't already do it.
	{
		// Clones the data FROM the preloaded shader onto (this) the instanced material!
		e->Clone(*this);
	}
	if (isPreset) {
		if (T.CheckToken("comment")) 
		{
#if __TOOL
			if (Comment)
				StringFree(Comment);
			char buf[128];
			T.GetLine(buf,sizeof(buf));

			// remove quotes
			char *pBuf = &buf[0];
			if(pBuf[0]=='"')
				pBuf++;
			int buglen = strlen(pBuf);
			if(buglen>0 && pBuf[buglen-1]=='"')
				pBuf[buglen-1] = '\0';

			Comment = StringDuplicate(pBuf);
#else
			T.SkipToEndOfLine();
#endif
		}
	}
	// Cloning resets the material flag, so reinstate it.
	if (isPreset)
		Flags = FLAG_MATERIAL;

	// Default drawbucket still should come from shader.
	if (e && isPreset) 
	{
		Assign(DrawBucket, e->GetDrawBucket_Deprecated());
		Assign(DrawBucketMask, e->GetDrawBucketMask());
	}

	if (T.CheckToken("__rage_drawbucket")) // Might not be present in old files.
	{
		T.MatchToken("{"); T.MatchToken("int"); Assign(DrawBucket,T.GetInt()); T.MatchToken("}");
		DrawBucketMask = BUCKETMASK_GENERATE(DrawBucket);
	}
	if (T.CheckToken("__rage_drawbucketmask")) // Might not be present in old files.
	{
		T.MatchToken("{"); T.MatchToken("int"); DrawBucketMask = T.GetInt(); T.MatchToken("}");
	}
	if (T.CheckToken("__rage_instanced")) // Might not be present in old files.
	{
		T.MatchToken("{"); T.MatchToken("int"); IsInstanced = T.GetInt() != 0; T.MatchToken("}");
	}
	if (T.CheckToken("__rage_physmtl")) // Might not be present in old files.
	{
		T.MatchToken("{"); T.MatchToken("int"); T.GetInt(); T.MatchToken("}");
	}

	// Stop parsing on closing brace or EOF.  Allows this to also handle inline sva data.
	while (!T.CheckToken("}",false) && T.GetToken(buf,sizeof(buf)))
	{
		u32 bufHash = strncmp(buf,"hash_",5)? atStringHash(buf) : strtoul(buf+5,NULL,16);
		// Failing to find an index shouldn't trigger an error; this can happen when variables are removed
		// from effects but they still exist in .sps files.  They'll be silently ignored instead.
		int localIndex = e? e->LookupVarByHash(bufHash) : 0;
		// If we're not loading a material, and the parameter is marked as being a per-material parameter,
		// pretend we didn't find it so we don't accidentally replace the parent version.
		// TODO: Revisit this -- it breaks existing data.
		// if (!isPreset && localIndex && e && (e->m_Locals[localIndex-1].Usage & USAGE_MATERIAL))
		//	localIndex = 0;

#if __TOOL
		// GunnarD: All sps file defined values should be locked for now.
		if(localIndex)
			mPresetFlags |= 1 << localIndex;
#endif
		T.MatchToken("{");
		char typeBuf[16];
		T.GetToken(typeBuf,sizeof(typeBuf));
		grcEffect::VarType type = grcEffect::GetType(typeBuf);
		int count = 1;
		if (T.CheckToken("Count"))
			count = T.GetInt();
		if (localIndex && e && e->m_Locals[localIndex-1].Type != type) {
			/*grcErrorf*/grcWarningf("Variable '%s' has type '%s' in sps file but type '%s' in code (ignored).",
				buf,typeBuf,grcEffect::GetTypeName((grcEffect::VarType)e->m_Locals[localIndex-1].Type));
			static u8 countByType[] = { 0, 1, 1, 2, 3, 4, 1, 1, 12, 16, 1 };
			count *= countByType[type];
			while (count--)
				T.IgnoreToken();

		}
		else if (type == grcEffect::VT_TEXTURE)
		{
			char texName[256];

			// handle missing texture file
			if (!T.CheckToken("}", false))
			{
				T.GetToken(texName,sizeof(texName));

				if (localIndex)
				{
#if __TOOL
					if(0==strcmp(texName, "none"))
						mPresetFlags &= ~(1 << localIndex);
#endif
#if EFFECT_CHECK_ENTRY_TYPES
					Data()[localIndex-1].Type = ET_TEXTURE;
#endif //EFFECT_CHECK_ENTRY_TYPES
					Data()[localIndex-1].Texture = grcTextureFactory::GetInstance().LookupTextureReference(texName);

					if (!Data()[localIndex-1].Texture) {
#if __RESOURCECOMPILER
						if (!isPreset)
							Data()[localIndex-1].Texture = rage_new grcTextureReference(texName,NULL);
#elif __TOOL
						Data()[localIndex-1].Texture = grcTextureFactory::GetInstance().Create(texName,NULL);
#else
						grcErrorf("Variable '%s' texture '%s' not found resident (should be registered reference or in a current texture dictionary).",buf,texName);
						Data()[localIndex-1].Texture = const_cast<grcTexture*>(grcTexture::None);
#endif
					}
					// If LookupTextureReference worked, adjust the refcount back down one.
					else if (Data()[localIndex-1].Texture->Release() == 0)
						grcErrorf("Variable '%s' texture '%s' refcount just unexpectedly went to zero.",buf,texName);

#if MULTIPLE_RENDER_THREADS > 1
					if (Flags & FLAG_EXTRA_DATA)
						for (int i=1; i<NUMBER_OF_RENDER_THREADS; i++)
							DataForThread(i)[localIndex-1].Texture = DataForThread(0)[localIndex-1].Texture;
#endif
				}
			}
		}
		else
		{
#if EFFECT_CHECK_ENTRY_TYPES
			if (localIndex)
				Data()[localIndex-1].Type = ET_FLOAT;
#endif //EFFECT_CHECK_ENTRY_TYPES
			float dummy[64];		// scratch area in case it didn't parse
			float *f = localIndex? Data()[localIndex-1].Float : dummy;
			switch (type) 
			{
				case grcEffect::VT_FLOAT: do { f[0] = T.GetFloat(); f[1] = f[2] = f[3] = 0.0f; } while (f+=4, --count); break;
				case grcEffect::VT_VECTOR2: do { f[0] = T.GetFloat(); f[1] = T.GetFloat(); f[2] = f[3] = 0.0f; } while (f+=4, --count); break;
				case grcEffect::VT_VECTOR3: do { f[0] = T.GetFloat(); f[1] = T.GetFloat(); f[2] = T.GetFloat(); f[3] = 0.0f; } while (f+=4, --count); break;
				case grcEffect::VT_VECTOR4: do { f[0] = T.GetFloat(); f[1] = T.GetFloat(); f[2] = T.GetFloat(); f[3] = T.GetFloat(); } while (f+=4, --count); break;
				case grcEffect::VT_MATRIX43: count *= 3; do { f[0] = T.GetFloat(); f[1] = T.GetFloat(); f[2] = T.GetFloat(); f[3] = T.GetFloat(); } while (f+=4, --count); break;
				case grcEffect::VT_MATRIX44: count <<= 2; do { f[0] = T.GetFloat(); f[1] = T.GetFloat(); f[2] = T.GetFloat(); f[3] = T.GetFloat(); } while (f+=4, --count); break;
				// Assert on unknown types if they somehow still existed.  Skip them.
				default: Assertf(!localIndex,"Unknown type '%s' in %s",typeBuf,filename); T.IgnoreToken(); break;
			}
#if MULTIPLE_RENDER_THREADS > 1
			if (localIndex && (Flags & FLAG_EXTRA_DATA))
				for (int i=1; i<NUMBER_OF_RENDER_THREADS; i++)
				{
# if EFFECT_CHECK_ENTRY_TYPES
					DataForThread(i)[localIndex-1].Type = ET_FLOAT;
# endif //EFFECT_CHECK_ENTRY_TYPES
					memcpy(DataForThread(i)[localIndex-1].Float, DataForThread(0)[localIndex-1].Float, DataForThread(0)[localIndex-1].Count * 16);
				}
#endif //MULTIPLE_RENDER_THREADS
		}
		T.MatchToken("}");
	}

	if (e)
		e->UpdateTextureReferences(*this);
#if !__TOOL
	// UpdateSortKey();
#endif

	return e != 0;
}


const char *grcInstanceData::GetShaderName() const
{
	return GetBasis().GetEffectName();
}


#if __TOOL
void grcInstanceData::SetComment(const char *c)
{
	if (Comment != c) 
	{
		StringFree(Comment);
		Comment = StringDuplicate(c);
	}
}


const char *grcInstanceData::GetComment() const
{
	return Comment;
}
#endif		// __TOOL

void grcInstanceData::Prefetch() const 
{
	bool doneFloat = false;
	for (int i=0; i<Count; i++) {
		if (IsTexture(i)) {
			if (Data()[i].Texture) {
				PrefetchDC(Data()[i].Texture);
			}
			else
			{
				if ( !doneFloat )
				{
					PrefetchDC( Data()[i].Float);
					doneFloat = true;
				}
			}
		}
	}
}


void grcEffect::Clone(grcInstanceData &outClone) const
{
	m_InstanceData.Clone(outClone);
}


void grcInstanceData::Clone(grcInstanceData &outClone) const
{
	outClone.Basis = Basis;
	if (Flags & FLAG_MATERIAL)
		outClone.Material = const_cast<grcInstanceData*>(this);
	else
		outClone.Material = NULL;
	outClone.Count = Count;
	outClone.TotalSize = TotalSize;
	outClone.SpuSize = SpuSize;
	outClone.DrawBucket = DrawBucket;
	outClone.DrawBucketMask = DrawBucketMask;
	outClone.PhysMtl_DEPRECATED = PhysMtl_DEPRECATED;
	outClone.pad = pad;
	outClone.TextureCount = TextureCount;
	outClone.IsInstanced = IsInstanced;
	outClone.UserFlags = UserFlags;
	Assign(outClone.Flags,Flags & ~FLAG_MATERIAL);
#if MULTIPLE_RENDER_THREADS || RSG_RSC
#if RSG_RSC
	CompileTimeAssert(MULTIPLE_RENDER_THREADS_ORBIS && MULTIPLE_RENDER_THREADS_DURANGO);
	// The +1 is from MULTIPLE_RENDER_THREADS_ALLOW_DRAW_LISTS_ON_RENDER_THREAD, which is 1 for all platforms but Orbis
	const int mrt = (g_sysPlatform==platform::WIN64PC || g_sysPlatform==platform::WIN32PC) ? (MULTIPLE_RENDER_THREADS + 1) : (g_sysPlatform==platform::ORBIS? MULTIPLE_RENDER_THREADS_ORBIS : (MULTIPLE_RENDER_THREADS_DURANGO + 1));
#else
	const int mrt = NUMBER_OF_RENDER_THREADS;
#endif
	char *buffer = (TotalSize? rage_new char[TotalSize * mrt] : NULL);
	// Remember that we've already allocated the correct amount of memory.  We still need to call ExpandForMultipleThreads elsewhere
	// in the code though because the cloned data is changed after this point, possibly by the rsc ctor due to type changes.
	outClone.Flags |= FLAG_EXTRA_DATA;
#else
	char *buffer = (TotalSize? rage_contained_new char[TotalSize] : NULL);
#endif
	outClone.Entries = (grcInstanceData::Entry*)buffer;
	memcpy(buffer, Entries, TotalSize);
#if !__TOOL
	ptrdiff_t fixup = buffer - (char*)Entries;
	// Just copy the handle when doing a clone operation.
#if TEXTURES_FIRST
	// Do the textures:
	for (int i=0; i<TextureCount; i++) {
# if USE_PACKED_GCMTEX
		outClone.Data()[i].Texture.index = Data()[i].Texture.index;
# elif ENABLE_DEFRAGMENTATION
		// Since we memcpy'd the data across, we don't want to decrement the
		// current texture handle, so copy the .ptr element instead and manually
		// inc the ref count.
		outClone.Data()[i].Texture.ptr = Data()[i].Texture.ptr;
		outClone.Data()[i].Texture.IncRef();
# else
		outClone.Data()[i].Texture = Data()[i].Texture;
# endif
# if EFFECT_CHECK_ENTRY_TYPES
		outClone.Data()[i].Type = ET_TEXTURE;
# endif
	}

	// Do the constants:
	for (int i=TextureCount; i<Count; i++)
	{
		outClone.Data()[i].Any = (void*)((char*)Data()[i].Any + fixup);
# if EFFECT_CHECK_ENTRY_TYPES
		outClone.Data()[i].Type = ET_FLOAT;
# endif
	}
#else		// !TEXTURES_FIRST
	for (int i=0; i<Count; i++) {
		if (IsTexture(i)) {
			outClone.Data()[i].Texture.ptr = Data()[i].Texture.ptr;
			outClone.Data()[i].Texture.IncRef();
			outClone.Data()[i].SavedSamplerStateSet = Data()[i].SavedSamplerStateSet;
		}
		else {
			outClone.Data()[i].Any = (void*)((char*)Data()[i].Any + fixup);
		}
# if EFFECT_CHECK_ENTRY_TYPES
		outClone.Data()[i].Type = Data()[i].Type;
# endif
	}
#endif		// !TEXTURES_FIRST
#else // __TOOL
	if (Comment)
		outClone.Comment = StringDuplicate(Comment);
	else
		outClone.Comment = NULL;
#endif

#if MULTIPLE_RENDER_THREADS
	outClone.ExpandForMultipleThreads(false);
#endif
}

#if RSG_XENON || RSG_PS3

grcCBuffer::grcCBuffer()
{
	Size = 0;
}

grcCBuffer::~grcCBuffer()
{
}

void grcCBuffer::Init(bool)
{
}

void grcCBuffer::Load(fiStream &S)
{
	//S.ReadInt(&Count, 1);
	S.ReadInt(&Size,1);
	S.ReadShort(Registers,6);

	char buffer[256];
	int count = S.GetCh();
	S.Read(buffer,count);
	Name = buffer;

	grcDebugf2("CBuffer::Load - %s, register VS %d PS %d, size %d", buffer, Registers[0], Registers[1], Size);
	NameHash = atStringHash(Name);
}

void grcCBuffer::operator=(const grcCBuffer &rhs)
{
	Size = rhs.Size;
	memcpy(Registers,rhs.Registers,sizeof(Registers));
	NameHash = rhs.NameHash;
	Name = rhs.Name;
}

void *grcCBuffer::GetDataPtr()
{
	return NULL;
}

grcBuffer *grcCBuffer::GetBuffer(DEV_ONLY(u8))
{
	return NULL;
}

#endif

}	// namespace rage
