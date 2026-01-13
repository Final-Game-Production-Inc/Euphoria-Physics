// 
// grcore/edge_jobs.h
// 
// Copyright (C) 2007-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_EDGE_JOBS_H 
#define GRCORE_EDGE_JOBS_H 

#include "grcore/effect_config.h"
#include "vector/vector3.h"

// GCM forward declarations
struct CellGcmContextData;

// SPURS forward declarations
struct CellSpurs;
struct CellSpursJobChain;
struct CellSpursJobGuard;
struct CellSpursJob256;
struct CellSpursEventFlag;

// Edge forward declarations
struct EdgeGeomPpuConfigInfo;
struct EdgeGeomLocalToWorldMatrix;
struct EdgeGeomOutputBufferInfo;
struct EdgeGeomViewportInfo;
struct EdgeGeomSpuProfileData;

// Rage forward declarations
namespace rage
{
	class Vector4;
	struct Matrix43;
	class Matrix44;
	class grcViewport;
	struct spuGcmState;
#if HACK_GTA4_MODELINFOIDX_ON_SPU
	struct CGta4DbgSpuInfoStruct;
#endif

	struct __sysTaskHandle;

	namespace edgegeomspujob
	{
		struct CellSpursEdgeJob; // custom job descriptor in edgegeomspu.h
	} // namespace edgegeomspujob
} // namespace rage

namespace rage
{
	// Segmented job ring buffer management.
	// 
	// A "job segment" is the unit used to track ring buffer progress.
	// Each segment starts with a guard and ends with a notification 
	// job.
	//
	// Jobchains are fully live. A job can be picked up by an available 
	// as soon as it's added. It doesn't require the segment to be current 
	// active segment to be flushed.
	//
	// Because viewports are not duplicated in the job header, we only store
	// one per segment. A viewport change will cause a segment flush.
	//
	// Edge geometry jobs contain RSX synchronisation code that can
	// - wait for RSX (ring buffer allocation, if full)
	// - block RSX execution (JTS patching)
	// Therefore, deadlocks are possible if it shares the same SPU 
	// with another job having similar synchronisation, and execute 
	// in reverse command buffer order (the second job, preallocated, will 
	// never have a change to run to unblock the first one, since the first
	// one is waiting actively). To avoid this kind of deadlock, make sure
	// these jobs run on different SPUs (by using the priority array).
	//
	// This class will soon be usable on the SPU-side.
	// - it must be 128 byte aligned to potentially allow SL1/ATO access 
	// - it is not thread safe 
	//		- only one PPU thread *OR* one SPU can access it
	// - Init/Shutdown functions are only available on the SPU
	class grcGeometryJobs
	{
	public:
		enum {			
			// Multiple segments
			// - We currently can't have more than 16 segments at the moment 
			//   since we're using 1 bit per segment in the event flag. 
			// - When we need to flush a segment (because of viewport change)
			//	 we waste the other jobs in the segment. This can be further
			//	 optimised, but it's considered not to be the mainline case at 
			//	 the moment
			EDGE_JOB_EVENT_START = 0,
			EDGE_MAX_SPU = 5
		};

		grcGeometryJobs()
		: m_eaOutputBufferInfo(NULL)
		, m_eaSharedOutputBuffer(NULL)
		, m_eaRingOutputBuffer(NULL)
		, m_eaJobChainCommandArray(NULL)
		, m_eaJobChain(NULL)
		, m_Initialized(false)
		{
		}

#if !__SPU
		int Initialize(CellSpurs* spursInstance, unsigned maxJobs,
					   unsigned sizeOutputRingBuffer, unsigned int sizeOutputSharedBuffer,
					   u8 outputBufferLocation, u8 spuPriorities[8]);
		int Shutdown(void);
		void BeginFrame(void);
		void EndFrame(void);			
		void ProfilerDraw(int x, int y);	
#endif // !__SPU

		void AddJob(
			CellGcmContextData* ctx,
			const EdgeGeomPpuConfigInfo* lsPpuConfigInfo, 
			const void* eaSkinningMatrices, 
			const Vector3& offset,
			u32 vertexShaderInputMask,
			spuGcmState& gcmState);

		void AddOtherJob(
			CellGcmContextData *ctx,
			char* binaryEa,
			char* binarySize,
			size_t InputSize,
			void *InputData,
			size_t ScratchSize,
			size_t SpuStackSize,
			size_t UserDataCount,
			int *UserData);

#if HACK_GTA4
#if USE_EDGE && __PPU
		bool AddExtractJob(CellGcmContextData*	ctx,
			const EdgeGeomPpuConfigInfo*		ppuConfigInfo, 
			const EdgeGeomLocalToWorldMatrix*	localToWorldMatrix,
			const Vector4**						pClipPlanes,
			int									ClipPlaneCount,
			const u8*							boneRemapLut,
			unsigned							boneRemapLutSize,
			void*								verts,
			Vector4*							EndOfOutputArray,
			void*								pDstVertsPtrs,
			u16*								indexes,
			u32									vertexOffsetForBatch,
			u32									totalverts,
			void*								damageTexture,
			float								boundsRadius,
			const Vector3*						pBoneNormals,
			float								dotProdThreshold
#if HACK_GTA4_MODELINFOIDX_ON_SPU
			,CGta4DbgSpuInfoStruct				*gta4SpuInfoStruct
#endif		
			,Matrix43*						ms,
			u8									extractMask);

		bool AddExtractJobAsync(CellGcmContextData*	ctx,
			u32**								handle,
			const EdgeGeomPpuConfigInfo*		ppuConfigInfo, 
			const EdgeGeomLocalToWorldMatrix*	localToWorldMatrix,
			const Vector4**						pClipPlanes,
			int									ClipPlaneCount,
			const u8*							boneRemapLut,
			unsigned							boneRemapLutSize,
			void*								verts,
			Vector4*							EndOfOutputArray,
			void*								pDstVertsPtrs,
			u16*								indexes,
			u32									vertexOffsetForBatch,
			u32									totalverts,
			void*								damageTexture,
			float								boundsRadius,
			const Vector3*						pBoneNormals,
			float								dotProdThreshold
#if HACK_GTA4_MODELINFOIDX_ON_SPU
			,CGta4DbgSpuInfoStruct				*gta4SpuInfoStruct
#endif		
			,Matrix43*						ms,
			u8									extractMask);

		bool FinalizeExtractJobAsync(CellGcmContextData* ctx, u32* handle);
#endif
#endif


		void SetOccluders(void* quads, u32 quadCount);

		// Static functions
		static void* SetLocalStallHole(CellGcmContextData *ctx, u32 holeSize, u32 extraHoleSize = 0, const spuGcmState* gcmState = NULL);

#if __PPU
		inline u32 GetOutputBufferInfoEa() const { return reinterpret_cast<u32>(m_eaOutputBufferInfo); }
#endif // __PPU

#if !__SPU
		bool IsRingBufferFull(u32 size) const;
#endif // !__SPU

	protected:
		// Output buffers
		EdgeGeomOutputBufferInfo	*m_eaOutputBufferInfo;
		u8							*m_eaSharedOutputBuffer;
		u8							*m_eaRingOutputBuffer;	
		u32							 m_FirstRsxLabelIndex;			// RSX label
		u32							 m_OutputLocation;				// CELL_GCM_LOCATION_MAIN || CELL_GCM_LOCATION_LOCAL
		
		// Segmented job buffer
		u32							 m_MaxJobs, m_NextJob;
		u64							*m_eaJobChainCommandArray;		// Commands (pre built with holes)

		// Job mangement data
		CellSpursJobChain*			 m_eaJobChain;
		bool						 m_Initialized;
	};
} // namespace rage

// TEMPORARY HACK: lives in effect_gcm.cpp until it's properly moved to/shared with SPUs
#if SPU_GCM_FIFO
# if __PPU
#include "grcore/grcorespu.h"
namespace rage { extern spuGcmState s_spuGcmState; }
#define GEOMETRY_JOBS	s_spuGcmState.EdgeJobs
# else
#define GEOMETRY_JOBS	pSpuGcmState->EdgeJobs
# endif
#else
namespace rage {
	extern grcGeometryJobs g_GeometryJobs;
} //namespace rage 
#define GEOMETRY_JOBS	g_GeometryJobs
#endif

#endif // GRCORE_EDGE_JOBS_H 
