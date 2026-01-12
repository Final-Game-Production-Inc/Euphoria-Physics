#if __SPU
#define TASK_SPU_2 1
#define ENTRYPOINT UpdateCloth
#define		__SPU_SCRATCHPAD	1
#include "system/taskheaderspu.h"
#include "system/task_spu.h"
#include "system/spuscratch.h"
#include "physics/inst.cpp"
#include "phbound/bound.cpp"
#include "phbound/boundcapsule.cpp"
#include "phbound/boundbvh.cpp"
#include "clothdata.cpp"
#include "cloth/characterclothcontroller.h"
#include "cloth_verlet_col.cpp"
#include "cloth_verlet_col_cloth.cpp"
#include "cloth_verlet_update.cpp"
#include "crskeleton/skeleton.h"
#include "crskeleton/skeletondata.h"

using namespace rage;

bool sysLsCheckPointer(const void* start, const void* end, bool forWrite);
template<class T> __forceinline void sysValidateWrite(T* addr, u32 count = 1)
{
	if (!sysLsCheckPointer(addr, addr + count, true)) 
		__debugbreak();
}
template<> __forceinline void sysValidateWrite(void* addr, u32 count)
{
	sysValidateWrite((u8*)addr, count);
}
template<class T> __forceinline void sysValidateRead(const T* addr, u32 count = 1)
{
	if (!sysLsCheckPointer(addr, addr + count, false)) 
		__debugbreak();
}
template<> __forceinline void sysValidateRead(const void* addr, u32 count)
{
	sysValidateRead((u8*)addr, count);
}

#if 0 // __ASSERT
// force pointers to be reset when scratch objects go out of scope
#define sysScratchGet sysScratchGetAndSave
#endif

const Matrix34 rage::M34_IDENTITY;

namespace rage {

#define TRAP_VIRTUAL_CALLS (!__FINAL)
#define dprintf(...) //Printf(__VA_ARGS__)

#if TRAP_VIRTUAL_CALLS
	void VirtualCall(void* pClass)
	{
		Quitf("Virtual function called on class instance %p", pClass);
	}
	void (*g_pVTable[32])(void*);
#endif

	template <typename _T>
	struct atSpuArray : public atArray<_T>
	{
		atSpuArray(int count, _T* elements)
		{
			atArray<_T>::m_Count = count;
			atArray<_T>::m_Capacity = count;
			atArray<_T>::m_Elements = elements;
		}
	};


	class crSkeletonDataSpu : public crSkeletonData
	{
	public:
		void GetForSkinning()
		{
			const int numBones = GetNumBones();
			sysScratchGet( m_CumulativeInverseTransforms.ptr, numBones );

			sysDmaWait(1<<0);
		}
	};

	class crSkeletonSpu : public crSkeleton
	{
	public:
		void GetForSkinning()
		{
			sysScratchGet( m_SkeletonData );
			sysScratchGet( m_Parent );
			sysScratchGetAndSave( m_Objects, m_NumBones, 0 );

			sysDmaWait(1<<0);

			((crSkeletonDataSpu&)*m_SkeletonData).GetForSkinning();
		}
	} ;

	class characterClothControllerSpu : public characterClothController
	{
	public:
		void Init(phVerletCloth* pClothSim, int simLodIndex, crSkeletonSpu* skeleton ) 
		{
			m_Cloth[simLodIndex] = pClothSim;

			sysScratchGet(m_Bridge);
			sysDmaWait(1<<0);

			skeleton->GetForSkinning();
		}
	};


} // namespace rage

#include "cloth/characterclothcontrollershared.h"

namespace rage 
{

	void crSkeleton::GetGlobalMtx(u32 boneIdx, Mat34V_InOut outMtx) const
	{
		if(m_Parent)
		{
			rage::Transform(outMtx, *m_Parent, m_Objects[boneIdx]);
		}
		else
		{
			outMtx = m_Objects[boneIdx];
		}
	}
} // namespace rage


u8*     g_instLvlIdxToMtxAddrMM = NULL;
Mat34V* g_instLastMtxAddrMM = NULL;

void UpdateCloth(sysTaskContext& c)
{

#if TRAP_VIRTUAL_CALLS
	for(u32 i=0; i<sizeof(g_pVTable)/sizeof(g_pVTable[0]); ++i)
		g_pVTable[i] = &VirtualCall;
#endif

	const_cast<Matrix34&>(M34_IDENTITY).Identity();

	phVerletCharacterClothUpdate& u = *(c.GetUserDataAs<phVerletCharacterClothUpdate>());

	sysScratchInit(c.GetScratch(c.ScratchSize()), c.ScratchSize());

#if ENABLE_SPU_COMPARE
	gpCompareBuf	= u.compareBuf;
#endif

#if ENABLE_SPU_DEBUG_DRAW

	const int verletDebugSpuSize = (sizeof(phVerletSPUDebug)+15)&(~15);
	u8* verletSpuNodeBuffer = sysScratchAllocObj<u8>( verletDebugSpuSize );
	phVerletSPUDebug* verletSpuDebug = NULL;
	if( u.verletSpuDebugAddr != 0 )
	{
		sysDmaLargeGet(verletSpuNodeBuffer, u.verletSpuDebugAddr, verletDebugSpuSize, 0);
		sysDmaWait(1<<0);
		verletSpuDebug = (phVerletSPUDebug*)verletSpuNodeBuffer;
		Assert( verletSpuDebug );
		gpDebugDrawBuf	= verletSpuDebug->debugDrawBuf;
	}
	else
	{
		gpDebugDrawBuf	= NULL;
	}
	
	gSPUDebugDrawSpheresCount	= 0;
	gSPUDebugDrawTrianglesCount	= 0;
	gSPUDebugDrawBoxesCount		= 0;
	gSPUDebugDrawCapsulesCount	= 0;

#endif

	gSPU_PauseClothSimulation = u.pauseSimulation;

	g_instLvlIdxToMtxAddrMM = u.instLastMtxIdxAddrMM;
	g_instLastMtxAddrMM = u.instLastMatricsAddrMM;

	const int sizeOfCharacterClothController = ((sizeof(characterClothController)+15)&~15);
	characterClothControllerSpu* pCClothController = (characterClothControllerSpu*)c.GetInput(sizeOfCharacterClothController);
	Assert( pCClothController );

	const int sizeOfCloth = ((sizeof(phVerletCloth)+15)&~15);
	phVerletCloth* pCloth = (phVerletCloth*)c.GetInput( sizeOfCloth );
	Assert( pCloth );

//	sysScratchScope s;

	sysScratchGetAndSave( pCloth->m_VerletClothType, 1, 1);

// TODO: this is not really needed, char cloth collide only with custom bounds
//	sysScratchGetAndSave( pCloth->m_CollisionInst );

	datRef<crSkeletonSpu> spu_skeleton;
	spu_skeleton = (crSkeletonSpu*)u.spu_skeleton;
	Assert( spu_skeleton.ptr );
	sysScratchGetAndSave( spu_skeleton );

	sysScratchGetAndSave( pCClothController->m_ControllerType, 1, 1 );

#if !NO_BONE_MATRICES
	sysScratchGetAndSave( pCClothController->m_PoseMatrixSet );
#endif
	sysScratchGetAndSave( pCClothController->m_BoneIndexMap );

// TODO: need 4 bytes to store somewhere pin radius sets threshold
// m_OriginalPos is not used in the instance, but is used in the type
	sysScratchGetAndSave( pCClothController->m_OriginalPos );
		
	sysScratchInfo* pSavePositions = sysScratchGetAndSave(pCloth->m_ClothData.m_VertexPositions);
	
	sysDmaWait(1<<1);

	sysDmaWait(1<<0);

	characterClothControllerSpu* pControllerType = pCClothController->m_ControllerType.ptr? (characterClothControllerSpu*)pCClothController->m_ControllerType.ptr: pCClothController;
	Assert( pControllerType );
	sysScratchGetAndSave( pControllerType->m_BindingInfo );
	sysScratchGetAndSave( pControllerType->m_OriginalPos );
	sysScratchGetAndSave( pControllerType->m_TriIndices );

	const int lodIndex = 0;
	pCClothController->Init( pCloth, lodIndex, spu_skeleton );
	sysScratchInfo* pSaveNormals = sysScratchGetAndSave( pCloth->m_ClothData.m_VertexInitialNormals );	

	sysDmaWait(1<<0);

	Vec3V* pNormals = (Vec3V*)pCloth->m_ClothData.m_VertexInitialNormals.GetElements();
	Assert( pNormals );

	clothBridgeSimGfx* pClothBridge = pCClothController->GetBridge();
	Assert( pClothBridge );

	sysScratchGet( pClothBridge->GetVertexWeightsRef(lodIndex), pClothBridge->GetMeshVertCount(lodIndex) );
	sysScratchGet( pClothBridge->GetInflationScaleRef(lodIndex), pClothBridge->GetMeshVertCount(lodIndex) );

	sysDmaWait(1<<0);		// wait for tuning data

	const float* pVertWeights = pClothBridge->GetVertexWeights(lodIndex);
	Assert( pVertWeights );
	const float* pInflationScale = pClothBridge->GetInflationScale(lodIndex);
	Assert( pInflationScale );

	pCloth->ApplyAirResistance( pInflationScale, pVertWeights, (Vec3V*)pNormals, 1, u.m_Force );

	phVerletCloth* pVerletClothType = const_cast<phVerletCloth*>(pCloth->m_VerletClothType.ptr ? pCloth->m_VerletClothType.ptr: pCloth);
	Assert( pVerletClothType );
	sysScratchGetAndSave(pVerletClothType->m_EdgeData);
	sysScratchGetAndSave(pVerletClothType->m_CustomEdgeData);

	sysDmaWait(1<<0);		// wait for the edges

	pCClothController->Update( VECTOR3_TO_VEC3V(u.m_Gravity), u.m_Force.w, spu_skeleton, u.m_Gravity.w /*timeStep*/, u.customBound, u.bonesIndices );

#if ENABLE_SPU_COMPARE
	if( !gpCompareBuf )
#endif
	{
		Assert( pSavePositions );
		pSavePositions->Writeback();
		Assert( pSaveNormals );
		pSaveNormals->Writeback();

		// Note: DMA back whatever changed
		sysDmaLargePut((void*)&pCClothController->m_ForcePin, u.flagsOffsetAddress, 16, 0);

		if( u.entityVertexBufferAddress )
		{
			sysDmaLargePut((void*)pCloth->m_ClothData.GetVertexPointer(), u.entityVertexBufferAddress, pCloth->GetNumVertices()*16, 0);
		}

#if ENABLE_SPU_DEBUG_DRAW
		if( gpDebugDrawBuf )
		{
			Vec4V countersOut;
			countersOut.SetXi( gSPUDebugDrawSpheresCount );
			countersOut.SetYi( gSPUDebugDrawBoxesCount );
			countersOut.SetZi( gSPUDebugDrawTrianglesCount );
			countersOut.SetWi( gSPUDebugDrawCapsulesCount );

			Assert( verletSpuDebug );
			sysDmaLargePut((void*)&countersOut,	verletSpuDebug->debugDrawCountOut, 16, 0);
			sysDmaWait(1<<0);
		}
#endif
		sysDmaWait(1<<0);
	}
}


#endif // __SPU
