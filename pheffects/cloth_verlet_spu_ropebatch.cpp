#if __SPU
#define		TASK_SPU_2			1
#define		ENTRYPOINT			UpdateRope
#define		__SPU_SCRATCHPAD	1

#include "system/taskheaderspu.h"
#include "system/task_spu.h"
#include "system/spuscratch.h"

#include "physics/inst.cpp"
#include "phbound/bound.cpp"
#include "phbound/boundcapsule.cpp"
#include "phbound/boundbvh.cpp"
#include "clothdata.cpp"
#include "cloth_verlet_col.cpp"
#include "cloth_verlet_update.cpp"
#include "cloth_verlet_spu_common.cpp"

#include "atl/dlist.h"

using namespace rage;

void UpdateRope(sysTaskContext& c)
{
#if TRAP_VIRTUAL_CALLS
	for(u32 i=0; i<sizeof(g_pVTable)/sizeof(g_pVTable[0]); ++i)
		g_pVTable[i] = &VirtualCall;
#endif
	const_cast<Matrix34&>(M34_IDENTITY).Identity();

	phVerletRopeUpdate*	gpRopeBatchUpdate = c.GetUserDataAs<phVerletRopeUpdate>();

	sysScratchInit(c.GetScratch(c.ScratchSize()), c.ScratchSize());

	phVerletRopeUpdate& u = *gpRopeBatchUpdate;

	g_instLvlIdxToMtxAddrMM = u.instLastMtxIdxAddrMM;
	g_instLastMtxAddrMM = u.instLastMatricsAddrMM;

	atDNode<phVerletCloth*>* head = (atDNode<phVerletCloth*>*)c.GetInput((sizeof(atDNode<phVerletCloth*>)+15)&~15);
	Assert( head );

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

	const int nodeSize = (sizeof(atDNode<phVerletCloth*>)+15)&(~15);
	u8 nodeBuffer[nodeSize] ;

	atDNode<phVerletCloth*>* node = head;
	while( node )
	{
		sysScratchInfo* pVerletInfo = sysScratchGetAndSave( node->Data, sizeof(phVerletCloth), 0 );
		sysDmaWait(1<<0);

		phVerletCloth* pVerlet = node->Data;
		Assert( pVerlet );

		sysScratchGetAndSave(pVerlet->m_ClothData.m_TypeData );
		sysScratchGetAndSave(pVerlet->m_CollisionInst);
		sysScratchGetAndSave(pVerlet->m_DynamicPinList);

		sysScratchInfo* m_VertexPositions = sysScratchGetAndSave(pVerlet->m_ClothData.m_VertexPositions);
		sysScratchInfo* m_VertexPrevPositions = sysScratchGetAndSave(pVerlet->m_ClothData.m_VertexPrevPositions);
		sysScratchGetAndSave(pVerlet->m_ClothData.m_VertexInitialNormals);

		sysDmaWait(1<<0);

		ScalarV timeStep;
		timeStep.Set( u.m_Gravity.w );

		sysScratchGetAndSave(pVerlet->m_EdgeData);

		sysDmaWait(1<<0);

		pVerlet->UpdateEnvironmentRope( VECTOR3_TO_VEC3V(u.m_Gravity), timeStep, pVerlet->m_CollisionInst, 0 /*u.clothBound*/, ScalarV(V_ONE) );

		m_VertexPrevPositions->Writeback();
		m_VertexPositions->Writeback();
		sysDmaWait(1<<0);

		sysScratchReset( (u8*)pVerletInfo->ls );

		if( node->GetNext() )
		{
			sysDmaLargeGet(nodeBuffer, (u32)node->GetNext(), sizeof(atDNode<phVerletCloth*>), 0);
			sysDmaWait(1<<0);
			node = (atDNode<phVerletCloth*>*)nodeBuffer;
		}
		else
		{
			node = NULL;
		}
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

}

#endif // __SPU
