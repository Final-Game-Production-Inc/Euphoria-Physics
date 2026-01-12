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

using namespace rage;


void UpdateRope(sysTaskContext& c)
{
#if TRAP_VIRTUAL_CALLS
	for(u32 i=0; i<sizeof(g_pVTable)/sizeof(g_pVTable[0]); ++i)
		g_pVTable[i] = &VirtualCall;
#endif
	const_cast<Matrix34&>(M34_IDENTITY).Identity();

	phVerletClothUpdate* gpVerletUpdate = c.GetUserDataAs<phVerletClothUpdate>();

	sysScratchInit(c.GetScratch(c.ScratchSize()), c.ScratchSize());

	phVerletClothUpdate& u	= *gpVerletUpdate;

	g_instLvlIdxToMtxAddrMM = u.instLastMtxIdxAddrMM;
	g_instLastMtxAddrMM = u.instLastMatricsAddrMM;

	phVerletCloth* pVerlet = (phVerletCloth*)c.GetInput((sizeof(phVerletCloth)+15)&~15);
	Assert( pVerlet );

	sysScratchGetAndSave(pVerlet->m_EdgeData);
	sysScratchGetAndSave(pVerlet->m_CollisionInst);
	sysScratchGetAndSave(pVerlet->m_DynamicPinList);

	sysScratchInfo* m_VertexPositions = sysScratchGetAndSave(pVerlet->m_ClothData.m_VertexPositions);
	sysScratchInfo* m_VertexPrevPositions = sysScratchGetAndSave(pVerlet->m_ClothData.m_VertexPrevPositions);
// TODO: why is this here ??
//	sysScratchGetAndSave(pVerlet->m_ClothData.m_VertexInitialNormals);

	sysDmaWait(1<<0);

	ScalarV timeStep;
	timeStep.Set(u.m_Gravity.w);

	pVerlet->UpdateEnvironmentRope( VECTOR3_TO_VEC3V(u.m_Gravity), timeStep, pVerlet->m_CollisionInst, ScalarV(V_ONE) );

	m_VertexPrevPositions->Writeback();
	m_VertexPositions->Writeback();
	sysDmaWait(1<<0);
}

#endif // __SPU
