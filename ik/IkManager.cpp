// 
// ik/IkManager.cpp
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
//

#include "IkManager.h"

// Rage headers
#include "creature/componentextradofs.h"
#include "crskeleton/skeleton.h"
#include "crskeleton/skeletondata.h"
#include "crbody/iksolver.h"
#include "entity/camerarelativeextension.h"
#include "fwanimation/directorcomponentmotiontree.h"
#include "fwdebug/picker.h"

// Game headers
#include "animation/anim_channel.h"
#include "animation/AnimBones.h"
#include "animation/MovePed.h"
#include "camera/CamInterface.h"
#include "debug/DebugScene.h"
#include "ik/IkConfig.h"
#include "ik/solvers/ArmIkSolver.h"
#include "ik/solvers/BodyLookSolverProxy.h"
#include "ik/solvers/BodyLookSolver.h"
#include "ik/solvers/BodyRecoilSolver.h"
#include "ik/solvers/HeadSolver.h"
#include "ik/solvers/QuadLegSolverProxy.h"
#include "ik/solvers/QuadLegSolver.h"
#include "ik/solvers/LegSolverProxy.h"
#include "ik/solvers/LegSolver.h"
#include "ik/solvers/TorsoSolver.h"
#include "ik/solvers/QuadrupedReactSolver.h"
#include "ik/solvers/TorsoReactSolver.h"
#include "ik/solvers/TorsoVehicleSolverProxy.h"
#include "ik/solvers/TorsoVehicleSolver.h"
#include "ik/solvers/RootSlopeFixupSolver.h"
#include "Peds/Horse/horseComponent.h"
#include "Peds/Ped.h"
#include "Peds/PedTuning.h"
#include "Peds/PedIKSettings.h"
#include "Peds/PedIntelligence.h"
#include "scene/world/GameWorld.h"
#include "Task/System/Tuning.h"
#include "Task/Vehicle/TaskInVehicle.h"

#if __IK_DEV
#include "scene/world/GameWorld.h"
#endif // __IK_DEV

#include "ik/Metadata_parser.h"

//////////////////////////////////////////////////////////////////////////

RAGE_DEFINE_CHANNEL(ik)

//////////////////////////////////////////////////////////////////////////

#define REGISTER_SOLVER_POOL_SIZE(_T, _P)	CBaseIkManager::ms_SolverPoolSizes[(int)_T] = (u16)(_P->GetSize())

ANIM_OPTIMISATIONS();

#if __IK_DEV
CDebugRenderer CBaseIkManager::ms_DebugHelper;
bool CBaseIkManager::ms_DisplayMemory = false;
bool CBaseIkManager::ms_DisplaySolvers = false;
#endif // _DEV

#if __IK_DEV || __BANK
extern const char* parser_IKManagerSolverTypes__Flags_Strings[];
bool CBaseIkManager::ms_DisableAllSolvers = false;
bool CBaseIkManager::ms_DisableSolvers[] = {0};
#endif // __IK_DEV || __BANK

u16 CBaseIkManager::ms_SolverPoolSizes[] = {0};
const CPed* CBaseIkManager::ms_pPedDeletingSolver = NULL;
CBaseIkManager::DebugInfo CBaseIkManager::ms_DebugInfo;

bool CIkManager::ms_bSampleHighHeelHeight = false;

using namespace IKManagerSolverTypes;

////////////////////////////////////////////////////////////////////////////////

#if __IK_DEV
const char *eLookIkReturnCodeToString(eLookIkReturnCode eReturnCode)
{
	switch(eReturnCode)
	{
	case LOOKIK_RC_NONE:			return "LOOKIK_RC_NONE";
	case LOOKIK_RC_DISABLED:		return "LOOKIK_RC_DISABLED";
	case LOOKIK_RC_INVALID_PARAM:	return "LOOKIK_RC_INVALID_PARAM";
	case LOOKIK_RC_CHAR_DEAD:		return "LOOKIK_RC_CHAR_DEAD";
	case LOOKIK_RC_BLOCKED:			return "LOOKIK_RC_BLOCKED";
	case LOOKIK_RC_BLOCKED_BY_ANIM:	return "LOOKIK_RC_BLOCKED_BY_ANIM";
	case LOOKIK_RC_BLOCKED_BY_TAG:	return "LOOKIK_RC_BLOCKED_BY_TAG";
	case LOOKIK_RC_OFFSCREEN:		return "LOOKIK_RC_OFFSCREEN";
	case LOOKIK_RC_RANGE:			return "LOOKIK_RC_RANGE";
	case LOOKIK_RC_FOV:				return "LOOKIK_RC_FOV";
	case LOOKIK_RC_PRIORITY:		return "LOOKIK_RC_PRIORITY";
	case LOOKIK_RC_INVALID_TARGET:	return "LOOKIK_RC_INVALID_TARGET";
	case LOOKIK_RC_TIMED_OUT:		return "LOOKIK_RC_TIMED_OUT";
	case LOOKIK_RC_ABORT:			return "LOOKIK_RC_ABORT";
	default:						return "UNKNOWN";
	}
}
#endif // __IK_DEV

////////////////////////////////////////////////////////////////////////////////

CBaseIkManager::CBaseIkManager() :
m_pPed(NULL),
m_flags(0)
{
	for(u32 i=0; i<ikSolverTypeCount; i++)
	{
		m_Solvers[i]=NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

CBaseIkManager::~CBaseIkManager()
{
	RemoveAllSolvers();
}

////////////////////////////////////////////////////////////////////////////////

void CBaseIkManager::InitClass()
{
	ms_DebugInfo.infoAsUInt = 0;
}

////////////////////////////////////////////////////////////////////////////////

bool CBaseIkManager::Init(CPed* pPed)
{
	ikAssert(pPed);
	m_pPed = pPed;
	m_Kinematics.Init(*m_pPed->GetSkeleton());
	return true;
}

////////////////////////////////////////////////////////////////////////////////

u32 CBaseIkManager::GetNumSolvers() const
{
	u32 count=0;

	for(u32 i=0; i<ikSolverTypeCount; i++)
	{
		if (m_Solvers[i])
		{
			count++;
		}
	}

	return count;
}

////////////////////////////////////////////////////////////////////////////////

const crIKSolverBase* CBaseIkManager::GetSolver(eIkSolverType solverType) const
{
	return m_Solvers[solverType];
}

////////////////////////////////////////////////////////////////////////////////

crIKSolverBase* CBaseIkManager::GetSolver(eIkSolverType solverType)
{
	return m_Solvers[solverType];
}

////////////////////////////////////////////////////////////////////////////////

crIKSolverBase* CBaseIkManager::GetOrAddSolver(eIkSolverType solverType)
{
	ikAssertf(!(m_pPed && m_pPed->GetIKSettings().IsIKFlagDisabled(solverType)) IK_QUAD_LEG_SOLVER_ONLY( || (solverType == ikSolverTypeQuadLeg) ) IK_QUAD_REACT_SOLVER_ONLY( || (solverType == ikSolverTypeQuadrupedReact) ), "Modelname = (%s) Trying to get or add an ik solver that has been disabled for this ped type!", m_pPed ? m_pPed->GetModelName() : "Unknown");

	if (!HasSolver(solverType))
	{
		AddSolver(solverType);
	}

	return m_Solvers[solverType];
}

////////////////////////////////////////////////////////////////////////////////

bool CBaseIkManager::HasSolver(eIkSolverType solverType) const
{
	return m_Solvers[solverType]!=NULL;
}

////////////////////////////////////////////////////////////////////////////////

bool CBaseIkManager::AddSolver(eIkSolverType solverType)
{
#if __IK_DEV || __BANK
	if (!CBaseIkManager::ms_DisableAllSolvers && !ms_DisableSolvers[solverType])
	{
#endif //__IK_DEV || __BANK
		ikAssertf(!(m_pPed && m_pPed->GetIKSettings().IsIKFlagDisabled(solverType)) IK_QUAD_LEG_SOLVER_ONLY( || (solverType == ikSolverTypeQuadLeg) ) IK_QUAD_REACT_SOLVER_ONLY( || (solverType == ikSolverTypeQuadrupedReact) ), "Modelname = (%s) Trying to add an ik solver that has been disabled for this ped type!", m_pPed ? m_pPed->GetModelName() : "Unknown");		
		ikAssertf(!m_Solvers[solverType], "Trying to add a duplicate Ik solver!");
		if (!m_Solvers[solverType])
		{
			switch(solverType)
			{
			case ikSolverTypeRootSlopeFixup:
				if(CRootSlopeFixupIkSolver::GetPool()->GetNoOfFreeSpaces() != 0)
				{
					m_Solvers[solverType] = rage_new CRootSlopeFixupIkSolver(m_pPed);
				}
				break;

			case ikSolverTypeTorso:
				if(CTorsoIkSolver::GetPool()->GetNoOfFreeSpaces() != 0)
				{
					m_Solvers[solverType] = rage_new CTorsoIkSolver(m_pPed);
				}
				break;

			case ikSolverTypeTorsoReact:
				if(CTorsoReactIkSolver::GetPool()->GetNoOfFreeSpaces() != 0)
				{
					m_Solvers[solverType] = rage_new CTorsoReactIkSolver(m_pPed);
				}
				break;

#if IK_QUAD_REACT_SOLVER_ENABLE
			case ikSolverTypeQuadrupedReact:
#if IK_QUAD_REACT_SOLVER_USE_POOL
				if(CQuadrupedReactSolver::GetPool()->GetNoOfFreeSpaces() != 0)
				{
					m_Solvers[solverType] = rage_new CQuadrupedReactSolver(m_pPed);
				}
#else
				if(CQuadrupedReactSolver::ms_NoOfFreeSpaces > 0)
				{
					m_Solvers[solverType] = rage_new CQuadrupedReactSolver(m_pPed);

					if (m_Solvers[solverType])
					{
						CQuadrupedReactSolver::ms_NoOfFreeSpaces--;
					}
				}
#endif // IK_QUAD_REACT_SOLVER_USE_POOL
				break;
#endif // IK_QUAD_REACT_SOLVER_ENABLE

			case ikSolverTypeTorsoVehicle:
				if(CTorsoVehicleIkSolverProxy::GetPool()->GetNoOfFreeSpaces() != 0)
				{
					m_Solvers[solverType] = rage_new CTorsoVehicleIkSolverProxy();
				}
				break;

			case ikSolverTypeHead:
#if ENABLE_HEAD_SOLVER
				if(CHeadIkSolver::GetPool()->GetNoOfFreeSpaces() != 0)
				{
					m_Solvers[solverType] = rage_new CHeadIkSolver(m_pPed);
				}
#endif // ENABLE_HEAD_SOLVER
				break;

			case ikSolverTypeBodyLook:
				if(CBodyLookIkSolverProxy::GetPool()->GetNoOfFreeSpaces() != 0)
				{
					m_Solvers[solverType] = rage_new CBodyLookIkSolverProxy(m_pPed);
				}
				break;

			case ikSolverTypeBodyRecoil:
				if(CBodyRecoilIkSolver::GetPool()->GetNoOfFreeSpaces() != 0)
				{
					m_Solvers[solverType] = rage_new CBodyRecoilIkSolver(m_pPed);
				}
				break;

			case ikSolverTypeLeg:
				if(CLegIkSolverProxy::GetPool()->GetNoOfFreeSpaces() != 0)
				{
					m_Solvers[solverType] = rage_new CLegIkSolverProxy();
				}
				break;

#if IK_QUAD_LEG_SOLVER_ENABLE
			case ikSolverTypeQuadLeg:
#if IK_QUAD_LEG_SOLVER_USE_POOL
				if(CQuadLegIkSolverProxy::GetPool()->GetNoOfFreeSpaces() != 0)
				{
					m_Solvers[solverType] = rage_new CQuadLegIkSolverProxy();
				}
#else
				if(CQuadLegIkSolverProxy::ms_NoOfFreeSpaces > 0)
				{
					m_Solvers[solverType] = rage_new CQuadLegIkSolverProxy();

					if (m_Solvers[solverType])
					{
						CQuadLegIkSolverProxy::ms_NoOfFreeSpaces--;
					}
				}
#endif // IK_QUAD_LEG_SOLVER_USE_POOL
				break;
#endif // IK_QUAD_LEG_SOLVER_ENABLE

			case ikSolverTypeArm:
				if(CArmIkSolver::GetPool()->GetNoOfFreeSpaces() != 0)
				{
					m_Solvers[solverType] = rage_new CArmIkSolver(m_pPed);
				}
				break;

			default:
				ikAssertf(0, "Trying to create an invalid Ik solver type. Have you remembered to add the solver type to the Ik manager AddSolver method?");
				return false;
			}

			if (m_Solvers[solverType])
			{
				RefreshKinematics();
			}
			else
			{
//#if __IK_DEV || __BANK
// 				ikWarningf("Unable to create IK solver of type %d (%s). Solver pool reached new peak or solver not supported", (int)solverType, GetSolverName(solverType));
//#else
// 				ikWarningf("Unable to create IK solver of type %d. Solver pool reached new peak or solver not supported", (int)solverType);
//#endif // #if __IK_DEV || __BANK
				return false;
			}

			return true;
		}
#if __IK_DEV || __BANK
	}
#endif //__IK_DEV || __BANK

	return false;
}


////////////////////////////////////////////////////////////////////////////////

bool CBaseIkManager::RemoveSolver(eIkSolverType solverType)
{
	ikAssert(sysThreadType::IsUpdateThread());

	if (m_Solvers[solverType])
	{
#if IK_QUAD_LEG_SOLVER_ENABLE && !IK_QUAD_LEG_SOLVER_USE_POOL
		if (solverType == ikSolverTypeQuadLeg)
		{
			CQuadLegIkSolverProxy* pProxy = static_cast<CQuadLegIkSolverProxy*>(m_Solvers[solverType]);
			if (pProxy->FreeSolver())
			{
				CQuadLegIkSolver::ms_NoOfFreeSpaces++;
			}

			CQuadLegIkSolverProxy::ms_NoOfFreeSpaces++;
		}
#endif // !IK_QUAD_LEG_SOLVER_USE_POOL
#if IK_QUAD_REACT_SOLVER_ENABLE && !IK_QUAD_REACT_SOLVER_USE_POOL
		if (solverType == ikSolverTypeQuadrupedReact)
		{
			CQuadrupedReactSolver::ms_NoOfFreeSpaces++;
		}
#endif // !IK_QUAD_REACT_SOLVER_USE_POOL

		ms_pPedDeletingSolver = m_pPed;
		delete m_Solvers[solverType];
		m_Solvers[solverType] = NULL;
		ms_pPedDeletingSolver = NULL;

		RefreshKinematics();
		return true;
	}
	else
	{
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////

void CBaseIkManager::RefreshKinematics()
{
	m_Kinematics.RemoveAllSolvers();

	for (int i=ikSolverTypeCount-1; i >= 0; --i)
	{
		if (m_Solvers[i])
		{
			crIKSolverBase* pSolverToAdd = m_Solvers[i];

			if (m_Solvers[i]->GetType() == crIKSolverBase::IK_SOLVER_TYPE_PROXY)
			{
				crIKSolverBase* pRealSolver = static_cast<CIkSolverProxy*>(m_Solvers[i])->GetSolver();

				// Add the real solver if it exists. Otherwise, let the proxy run in its place.
				if (pRealSolver)
				{
					pSolverToAdd = pRealSolver;
				}
			}

			m_Kinematics.AddSolver(pSolverToAdd);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void CBaseIkManager::InitPools()
{
	CArmIkSolver::InitPool( MEMBUCKET_ANIMATION );
#if ENABLE_HEAD_SOLVER
	CHeadIkSolver::InitPool( MEMBUCKET_ANIMATION );
#endif // ENABLE_HEAD_SOLVER
	CLegIkSolver::InitPool( MEMBUCKET_ANIMATION );
#if IK_QUAD_LEG_SOLVER_ENABLE && IK_QUAD_LEG_SOLVER_USE_POOL
	CQuadLegIkSolver::InitPool( MEMBUCKET_ANIMATION );
#endif // IK_QUAD_LEG_SOLVER_USE_POOL
#if IK_QUAD_REACT_SOLVER_ENABLE && IK_QUAD_REACT_SOLVER_USE_POOL
	CQuadrupedReactSolver::InitPool( MEMBUCKET_ANIMATION );
#endif // IK_QUAD_REACT_SOLVER_USE_POOL
	CTorsoIkSolver::InitPool( MEMBUCKET_ANIMATION );
	CTorsoReactIkSolver::InitPool( MEMBUCKET_ANIMATION );
	CRootSlopeFixupIkSolver::InitPool( MEMBUCKET_ANIMATION );
	CBodyLookIkSolver::InitPool( MEMBUCKET_ANIMATION );
	CBodyRecoilIkSolver::InitPool( MEMBUCKET_ANIMATION );
	CTorsoVehicleIkSolver::InitPool( MEMBUCKET_ANIMATION );

	CBodyLookIkSolverProxy::InitPool( MEMBUCKET_ANIMATION );
	CLegIkSolverProxy::InitPool( MEMBUCKET_ANIMATION );
#if IK_QUAD_LEG_SOLVER_ENABLE && IK_QUAD_LEG_SOLVER_USE_POOL
	CQuadLegIkSolverProxy::InitPool( MEMBUCKET_ANIMATION );
#endif // IK_QUAD_LEG_SOLVER_USE_POOL
	CTorsoVehicleIkSolverProxy::InitPool( MEMBUCKET_ANIMATION );

	// Register pool size for solvers with proxies
	REGISTER_SOLVER_POOL_SIZE(ikSolverTypeBodyLook, CBodyLookIkSolver::GetPool());
	REGISTER_SOLVER_POOL_SIZE(ikSolverTypeLeg, CLegIkSolver::GetPool());
#if IK_QUAD_LEG_SOLVER_ENABLE
#if IK_QUAD_LEG_SOLVER_USE_POOL
	REGISTER_SOLVER_POOL_SIZE(ikSolverTypeQuadLeg, CQuadLegIkSolver::GetPool());
#else
	CBaseIkManager::ms_SolverPoolSizes[(int)ikSolverTypeQuadLeg] = QUAD_LEG_SOLVER_PROXY_POOL_MAX;
#endif // IK_QUAD_LEG_SOLVER_USE_POOL
#endif // IK_QUAD_LEG_SOLVER_ENABLE
	REGISTER_SOLVER_POOL_SIZE(ikSolverTypeTorsoVehicle, CTorsoVehicleIkSolver::GetPool());
}

////////////////////////////////////////////////////////////////////////////////

void CBaseIkManager::ShutdownPools()
{
	CArmIkSolver::ShutdownPool();
#if ENABLE_HEAD_SOLVER
	CHeadIkSolver::ShutdownPool();
#endif // ENABLE_HEAD_SOLVER
	CLegIkSolver::ShutdownPool();
#if IK_QUAD_LEG_SOLVER_ENABLE && IK_QUAD_LEG_SOLVER_USE_POOL
	CQuadLegIkSolver::ShutdownPool();
#endif // IK_QUAD_LEG_SOLVER_USE_POOL
#if IK_QUAD_REACT_SOLVER_ENABLE && IK_QUAD_REACT_SOLVER_USE_POOL
	CQuadrupedReactSolver::ShutdownPool();
#endif // IK_QUAD_REACT_SOLVER_USE_POOL
	CTorsoIkSolver::ShutdownPool();
	CTorsoReactIkSolver::ShutdownPool();
	CRootSlopeFixupIkSolver::ShutdownPool();
	CBodyLookIkSolver::ShutdownPool();
	CBodyRecoilIkSolver::ShutdownPool();
	CTorsoVehicleIkSolver::ShutdownPool();

	CBodyLookIkSolverProxy::ShutdownPool();
	CLegIkSolverProxy::ShutdownPool();
#if IK_QUAD_LEG_SOLVER_ENABLE && IK_QUAD_LEG_SOLVER_USE_POOL
	CQuadLegIkSolverProxy::ShutdownPool();
#endif // IK_QUAD_LEG_SOLVER_USE_POOL
	CTorsoVehicleIkSolverProxy::ShutdownPool();
}

////////////////////////////////////////////////////////////////////////////////

void CBaseIkManager::ProcessSolvers()
{
	// Matched to CPedAILodManager::kMaxPeds
	const int kMaxPeds = 256;

	PedIkPriority aPedIkPriority[kMaxPeds];

	// Reconstruct ped scoring information calculated by CPedAILodManager earlier
	const CSpatialArray& spatialArray = *CPed::ms_spatialArray;
	const int nbPeds = spatialArray.GetNumObjects();
	ikAssert(nbPeds <= kMaxPeds);

	for (int pedIndex = 0; pedIndex < nbPeds; ++pedIndex)
	{
		PedIkPriority& ikPriority = aPedIkPriority[pedIndex];
		CPed* pPed = CPed::GetPedFromSpatialArrayNode(spatialArray.GetNodePointer(pedIndex));
		ikPriority.m_pPed	= pPed;
		ikPriority.m_Order	= (u16)pPed->GetPedAiLod().GetAiScoreOrder();
		ikPriority.m_Dirty	= 0;
	}

	std::sort(aPedIkPriority, aPedIkPriority + nbPeds, PedIkPriority::OrderLessThan);

	// Assign solvers to highest scoring peds.
	int aiNumSolversAvailable[IKManagerSolverTypes::ikSolverTypeCount];
	for (int solverType = 0; solverType < IKManagerSolverTypes::ikSolverTypeCount; ++solverType)
	{
		aiNumSolversAvailable[solverType] = ms_SolverPoolSizes[solverType];
	}

	for (int pedIndex = 0; pedIndex < nbPeds; ++pedIndex)
	{
		PedIkPriority& ikPriority = aPedIkPriority[pedIndex];
		CIkManager& ikManager = ikPriority.m_pPed->GetIkManager();

		ikManager.RemoveCompletedSolvers();

		for (int solverType = 0; solverType < IKManagerSolverTypes::ikSolverTypeCount; ++solverType)
		{
			if (ikManager.m_Solvers[solverType] && (ikManager.m_Solvers[solverType]->GetType() == crIKSolverBase::IK_SOLVER_TYPE_PROXY))
			{
				CIkSolverProxy* pProxy = static_cast<CIkSolverProxy*>(ikManager.m_Solvers[solverType]);
				pProxy->SetBlocked(aiNumSolversAvailable[solverType] <= 0);
				aiNumSolversAvailable[solverType]--;
			}
		}
	}

	// Allocate solvers for proxies if necessary
	for (int pedIndex = 0; pedIndex < nbPeds; ++pedIndex)
	{
		PedIkPriority& ikPriority = aPedIkPriority[pedIndex];
		CIkManager& ikManager = ikPriority.m_pPed->GetIkManager();

		for (int solverType = 0; solverType < IKManagerSolverTypes::ikSolverTypeCount; ++solverType)
		{
			if (ikManager.m_Solvers[solverType] && (ikManager.m_Solvers[solverType]->GetType() == crIKSolverBase::IK_SOLVER_TYPE_PROXY))
			{
				CIkSolverProxy* pProxy = static_cast<CIkSolverProxy*>(ikManager.m_Solvers[solverType]);
				if (!pProxy->IsBlocked() && (pProxy->GetSolver() == NULL))
				{
					ikPriority.m_Dirty |= (u16)pProxy->AllocateSolver(ikPriority.m_pPed);
				}
			}
		}

		if (ikPriority.m_Dirty != 0)
		{
			ikManager.RefreshKinematics();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void CBaseIkManager::RemoveCompletedSolvers()
{
	ikAssert(sysThreadType::IsUpdateThread());

	for (int i = 0; i < ikSolverTypeCount; i++)
	{
		crIKSolverBase* solver = GetSolver((eIkSolverType)i);

		if (solver && ((eIkSolverType)i == ikSolverTypeTorso) && !CTorsoIkSolver::GetPool()->IsValidPtr(solver))
		{
			ikWarningf("Deleted torso solver being tracked by IK manager for ped %p", m_pPed);
			Assertf(false, "Deleted torso solver being tracked by IK manager for ped %p", m_pPed);
			m_Solvers[i] = NULL;
			RefreshKinematics();
			continue;
		}

#if __IK_DEV || __BANK
		if (solver && (solver->IsDead() || CBaseIkManager::ms_DisableSolvers[i] || CBaseIkManager::ms_DisableAllSolvers))
#else
		if (solver && solver->IsDead())
#endif // __IK_DEV || __BANK
		{
			RemoveSolver((eIkSolverType)i);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void CBaseIkManager::RemoveAllSolvers()
{
	ikAssert(sysThreadType::IsUpdateThread());

	ms_pPedDeletingSolver = m_pPed;

	for (int i = 0; i < ikSolverTypeCount; i++)
	{
		if (m_Solvers[i])
		{
#if IK_QUAD_LEG_SOLVER_ENABLE && !IK_QUAD_LEG_SOLVER_USE_POOL
			if (i == (int)ikSolverTypeQuadLeg)
			{
				CQuadLegIkSolverProxy* pProxy = static_cast<CQuadLegIkSolverProxy*>(m_Solvers[i]);
				if (pProxy->FreeSolver())
				{
					CQuadLegIkSolver::ms_NoOfFreeSpaces++;
				}

				CQuadLegIkSolverProxy::ms_NoOfFreeSpaces++;
			}
#endif // !IK_QUAD_LEG_SOLVER_USE_POOL
#if IK_QUAD_REACT_SOLVER_ENABLE && !IK_QUAD_REACT_SOLVER_USE_POOL
			if (i == (int)ikSolverTypeQuadrupedReact)
			{
				CQuadrupedReactSolver::ms_NoOfFreeSpaces++;
			}
#endif // !IK_QUAD_REACT_SOLVER_USE_POOL

			delete m_Solvers[i];
			m_Solvers[i] = NULL;
		}
	}

	ms_pPedDeletingSolver = NULL;

	RefreshKinematics();
}

////////////////////////////////////////////////////////////////////////////////

void CBaseIkManager::ValidateSolverDeletion(CPed* pPed, eIkSolverType solverType)
{
	Assertf(GetPedDeletingSolver() && (GetPedDeletingSolver() == pPed), "Solver type %d being deleted by (%p) instead of owning ped (%p)", (int)solverType, GetPedDeletingSolver(), pPed);
	if (!GetPedDeletingSolver() || (GetPedDeletingSolver() != pPed))
	{
		u8 ownedBy = 0xFF; // Unknown
		u8 deletedBy = 0xFF; // Unknown

		if (pPed)
		{
			ownedBy = (u8)CPed::GetPool()->GetIndex(pPed);
		}

		if (GetPedDeletingSolver())
		{
			deletedBy = (u8)CPed::GetPool()->GetIndex(GetPedDeletingSolver());
		}

		ms_DebugInfo.info.ownedBy = ownedBy;
		ms_DebugInfo.info.deletedBy = deletedBy;
		ms_DebugInfo.info.deletedType = (u8)solverType;
	}
}

////////////////////////////////////////////////////////////////////////////////

#if __IK_DEV
void CBaseIkManager::DebugRender()
{
	if (ms_DisplayMemory)
	{
		DisplayMemory();
	}

	if (ms_DisplaySolvers)
	{
		DisplaySolvers();
	}

	CEntity* pSelectedEntity = (CEntity*) g_PickerManager.GetSelectedEntity();
	if (pSelectedEntity && pSelectedEntity->GetIsTypePed())
	{
		CPed* pSelectedPed = static_cast<CPed*>(pSelectedEntity);
		if (pSelectedPed)
		{
			pSelectedPed->GetIkManager().DebugRenderPed();
		}
	}
}

void CBaseIkManager::DisplayMemory()
{
	char szText[128];
	s32 lineNb = 0;
	size_t usedMem = 0;
	size_t totalMem = 0;

#define DisplaySolverMemory(_T, _P)																		\
	formatf(szText, "%-28s: Used(%3d) Free(%3d) Total(%3d) sizeof(%4" SIZETFMT "d) UsedMem(%6" SIZETFMT "d) TotalMem (%6" SIZETFMT "d)\n",	\
		GetSolverName(_T),																				\
		_P->GetNoOfUsedSpaces(),																		\
		_P->GetNoOfFreeSpaces(),																		\
		_P->GetSize(),																					\
		_P->GetStorageSize(),																			\
		_P->GetNoOfUsedSpaces() * _P->GetStorageSize(),													\
		_P->GetSize() * _P->GetStorageSize());															\
	usedMem += _P->GetNoOfUsedSpaces() * _P->GetStorageSize();											\
	totalMem += _P->GetSize() * _P->GetStorageSize();													\
	ms_DebugHelper.Text2d(lineNb++, Color_white, szText);

	ms_DebugHelper.Reset();

	// Solvers
	formatf(szText, "Solvers");
	ms_DebugHelper.Text2d(lineNb++, Color_white, szText);

	DisplaySolverMemory(ikSolverTypeArm, CArmIkSolver::GetPool());
	DisplaySolverMemory(ikSolverTypeBodyLook, CBodyLookIkSolver::GetPool());
	DisplaySolverMemory(ikSolverTypeBodyRecoil, CBodyRecoilIkSolver::GetPool());
#if ENABLE_HEAD_SOLVER
	DisplaySolverMemory(ikSolverTypeHead, CHeadIkSolver::GetPool());
#endif // ENABLE_HEAD_SOLVER
	DisplaySolverMemory(ikSolverTypeLeg, CLegIkSolver::GetPool());
#if IK_QUAD_LEG_SOLVER_ENABLE && IK_QUAD_LEG_SOLVER_USE_POOL
	DisplaySolverMemory(ikSolverTypeQuadLeg, CQuadLegIkSolver::GetPool());
#endif // IK_QUAD_LEG_SOLVER_USE_POOL
#if IK_QUAD_REACT_SOLVER_ENABLE && IK_QUAD_REACT_SOLVER_USE_POOL
	DisplaySolverMemory(ikSolverTypeQuadrupedReact, CQuadrupedReactSolver::GetPool());
#endif // IK_QUAD_REACT_SOLVER_USE_POOL
	DisplaySolverMemory(ikSolverTypeTorso, CTorsoIkSolver::GetPool());
	DisplaySolverMemory(ikSolverTypeTorsoReact, CTorsoReactIkSolver::GetPool());
	DisplaySolverMemory(ikSolverTypeTorsoVehicle, CTorsoVehicleIkSolver::GetPool());
	DisplaySolverMemory(ikSolverTypeRootSlopeFixup, CRootSlopeFixupIkSolver::GetPool());

	lineNb++;

	// Solver Proxies
	formatf(szText, "Proxies");
	ms_DebugHelper.Text2d(lineNb++, Color_white, szText);

	DisplaySolverMemory(ikSolverTypeBodyLook, CBodyLookIkSolverProxy::GetPool());
	DisplaySolverMemory(ikSolverTypeLeg, CLegIkSolverProxy::GetPool());
#if IK_QUAD_LEG_SOLVER_ENABLE && IK_QUAD_LEG_SOLVER_USE_POOL
	DisplaySolverMemory(ikSolverTypeQuadLeg, CQuadLegIkSolverProxy::GetPool());
#endif // IK_QUAD_LEG_SOLVER_USE_POOL
	DisplaySolverMemory(ikSolverTypeTorsoVehicle, CTorsoVehicleIkSolverProxy::GetPool());

	lineNb++;

	// Non-Pooled Solvers
	formatf(szText, "Non-Pooled Solvers");
	ms_DebugHelper.Text2d(lineNb++, Color_white, szText);

#if IK_QUAD_LEG_SOLVER_ENABLE && !IK_QUAD_LEG_SOLVER_USE_POOL
	formatf(szText, "%-28s: Used(%3d) Free(%3d) Total(%3d) sizeof(%4" SIZETFMT "d) UsedMem(%6" SIZETFMT "d) TotalMem (%6" SIZETFMT "d)\n",	\
		GetSolverName(ikSolverTypeQuadLeg),																\
		(QUAD_LEG_SOLVER_POOL_MAX - CQuadLegIkSolver::ms_NoOfFreeSpaces),								\
		CQuadLegIkSolver::ms_NoOfFreeSpaces,															\
		QUAD_LEG_SOLVER_POOL_MAX,																		\
		sizeof(CQuadLegIkSolver),																		\
		(QUAD_LEG_SOLVER_POOL_MAX - CQuadLegIkSolver::ms_NoOfFreeSpaces) * sizeof(CQuadLegIkSolver),	\
		(QUAD_LEG_SOLVER_POOL_MAX - CQuadLegIkSolver::ms_NoOfFreeSpaces) * sizeof(CQuadLegIkSolver));	\
	usedMem += (QUAD_LEG_SOLVER_POOL_MAX - CQuadLegIkSolver::ms_NoOfFreeSpaces) * sizeof(CQuadLegIkSolver);		\
	totalMem += (QUAD_LEG_SOLVER_POOL_MAX - CQuadLegIkSolver::ms_NoOfFreeSpaces) * sizeof(CQuadLegIkSolver);	\
	ms_DebugHelper.Text2d(lineNb++, Color_white, szText);
#endif // IK_QUAD_LEG_SOLVER_USE_POOL
#if IK_QUAD_REACT_SOLVER_ENABLE && !IK_QUAD_REACT_SOLVER_USE_POOL
	formatf(szText, "%-28s: Used(%3d) Free(%3d) Total(%3d) sizeof(%4" SIZETFMT "d) UsedMem(%6" SIZETFMT "d) TotalMem (%6" SIZETFMT "d)\n",	\
		GetSolverName(ikSolverTypeQuadrupedReact),														\
		(QUAD_REACT_SOLVER_POOL_MAX - CQuadrupedReactSolver::ms_NoOfFreeSpaces),						\
		CQuadrupedReactSolver::ms_NoOfFreeSpaces,														\
		QUAD_REACT_SOLVER_POOL_MAX,																		\
		sizeof(CQuadrupedReactSolver),																	\
		(QUAD_REACT_SOLVER_POOL_MAX - CQuadrupedReactSolver::ms_NoOfFreeSpaces) * sizeof(CQuadrupedReactSolver),	\
		(QUAD_REACT_SOLVER_POOL_MAX - CQuadrupedReactSolver::ms_NoOfFreeSpaces) * sizeof(CQuadrupedReactSolver));	\
	usedMem += (QUAD_REACT_SOLVER_POOL_MAX - CQuadrupedReactSolver::ms_NoOfFreeSpaces) * sizeof(CQuadrupedReactSolver);		\
	totalMem += (QUAD_REACT_SOLVER_POOL_MAX - CQuadrupedReactSolver::ms_NoOfFreeSpaces) * sizeof(CQuadrupedReactSolver);	\
	ms_DebugHelper.Text2d(lineNb++, Color_white, szText);
#endif // IK_QUAD_REACT_SOLVER_USE_POOL

	lineNb++;

	// Non-Pooled Proxies
	formatf(szText, "Non-Pooled Proxies");
	ms_DebugHelper.Text2d(lineNb++, Color_white, szText);

#if IK_QUAD_LEG_SOLVER_ENABLE && !IK_QUAD_LEG_SOLVER_USE_POOL
	formatf(szText, "%-28s: Used(%3d) Free(%3d) Total(%3d) sizeof(%4" SIZETFMT "d) UsedMem(%6" SIZETFMT "d) TotalMem (%6" SIZETFMT "d)\n",	\
		GetSolverName(ikSolverTypeQuadLeg),																\
		(QUAD_LEG_SOLVER_PROXY_POOL_MAX - CQuadLegIkSolverProxy::ms_NoOfFreeSpaces),					\
		CQuadLegIkSolverProxy::ms_NoOfFreeSpaces,														\
		QUAD_LEG_SOLVER_PROXY_POOL_MAX,																	\
		sizeof(CQuadLegIkSolverProxy),																	\
		(QUAD_LEG_SOLVER_PROXY_POOL_MAX - CQuadLegIkSolverProxy::ms_NoOfFreeSpaces) * sizeof(CQuadLegIkSolverProxy),	\
		(QUAD_LEG_SOLVER_PROXY_POOL_MAX - CQuadLegIkSolverProxy::ms_NoOfFreeSpaces) * sizeof(CQuadLegIkSolverProxy));	\
	usedMem += (QUAD_LEG_SOLVER_PROXY_POOL_MAX - CQuadLegIkSolverProxy::ms_NoOfFreeSpaces) * sizeof(CQuadLegIkSolverProxy);		\
	totalMem += (QUAD_LEG_SOLVER_PROXY_POOL_MAX - CQuadLegIkSolverProxy::ms_NoOfFreeSpaces) * sizeof(CQuadLegIkSolverProxy);	\
	ms_DebugHelper.Text2d(lineNb++, Color_white, szText);
#endif // IK_QUAD_LEG_SOLVER_USE_POOL

	lineNb++;

	// Totals
	formatf(szText, "                                                                                (%7" SIZETFMT "d)         (%7" SIZETFMT "d)  ", usedMem, totalMem);
	ms_DebugHelper.Text2d(lineNb++, Color_white, szText);

	ms_DebugHelper.Render();
}

void CBaseIkManager::DisplaySolvers()
{
	const Vec3V vSourcePosition(VECTOR3_TO_VEC3V(CGameWorld::FindLocalPlayerCoors()));

	const CSpatialArray& spatialArray = *CPed::ms_spatialArray;
	const int nbPeds = spatialArray.GetNumObjects();

	for (int pedIndex = 0; pedIndex < nbPeds; ++pedIndex)
	{
		CPed* pPed = CPed::GetPedFromSpatialArrayNode(spatialArray.GetNodePointer(pedIndex));
		CIkManager& ikManager = pPed->GetIkManager();
		Vec3V vPedPosition(pPed->GetTransform().GetPosition());
		float fDisplaySolversOffset = 0.0f;

		for (int solverType = 0; solverType < IKManagerSolverTypes::ikSolverTypeCount; ++solverType)
		{
			if (ikManager.m_Solvers[solverType] && (ikManager.m_Solvers[solverType]->GetType() == crIKSolverBase::IK_SOLVER_TYPE_PROXY))
			{
				CIkSolverProxy* pProxy = static_cast<CIkSolverProxy*>(ikManager.m_Solvers[solverType]);

				Color32 aColours[2] = { Color_green4, Color_green1 };
				if (solverType == ikSolverTypeBodyLook) { aColours[0] = Color_maroon4; aColours[1] = Color_maroon1; }
#if IK_QUAD_LEG_SOLVER_ENABLE
				if (solverType == ikSolverTypeQuadLeg) { aColours[0] = Color_orange4; aColours[1] = Color_orange1; }
#endif // IK_QUAD_LEG_SOLVER_ENABLE
				if (solverType == ikSolverTypeTorsoVehicle) { aColours[0] = Color_cyan4; aColours[1] = Color_cyan1; }

				grcDebugDraw::Line(vSourcePosition, Add(vPedPosition, Vec3V(0.0f, 0.0f, fDisplaySolversOffset)), aColours[!pProxy->IsBlocked() && pProxy->GetSolver() ? 1 : 0]);
				fDisplaySolversOffset += 0.075f;
			}
		}
	}
}
#endif //__IK_DEV

////////////////////////////////////////////////////////////////////////////////

#if __IK_DEV || __BANK
const char* CBaseIkManager::GetSolverName(eIkSolverType solverType)
{
	return parser_IKManagerSolverTypes__Flags_Strings[solverType];
}
#endif // __IK_DEV || __BANK

///////////////////////////////////////////////////////////////////////////////

CIkManager::~CIkManager()
{
#if FPS_MODE_SUPPORTED

	if (m_pFirstPersonRequests)
	{
		delete m_pFirstPersonRequests;
		m_pFirstPersonRequests = NULL;
	}

	if (m_pThirdPersonSkeleton)
	{
		delete m_pThirdPersonSkeleton;
		m_pThirdPersonSkeleton = NULL;
	}

	if (m_pThirdPersonInputFrame)
	{
		ikAssert(m_pPed);
		ikAssert(m_pPed->GetAnimDirector());
		m_pPed->GetAnimDirector()->GetFrameBuffer().ReleaseFrame(m_pThirdPersonInputFrame);
		m_pThirdPersonInputFrame = NULL;
	}

#endif // FPS_MODE_SUPPORTED
}


bool CIkManager::Init(CPed* pPed)
{
	ikAssert(pPed);

	CBaseIkManager::Init(pPed);

	InitHighHeelData();

	//make sure fade defaults to on
	ClearFlag(PEDIK_LEGS_AND_PELVIS_FADE_OFF);  

	//make sure leg ik defaults to on
	ClearFlag(PEDIK_LEGS_AND_PELVIS_OFF);       

#if FPS_MODE_SUPPORTED
	SetExternallyDrivenPelvisOffset(0.0f);
	m_pFirstPersonRequests = NULL;
	m_pThirdPersonSkeleton = NULL;
	m_pThirdPersonInputFrame = NULL;

	// Initialise requests
	FirstPersonPassRequests& requests = GetFpsRequests();
	requests.Reset();

	m_fDeltaTime = 0.0f;
	m_bFPSUpdatePass = false;
#endif // FPS_MODE_SUPPORTED

	IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_NONE, "CIkManager::Init");
	return true;
}

//////////////////////////////////////////////////////////////////////////

#if __BANK
void CIkManager::InitWidgets()
{
	bkBank* pBank = &BANKMGR.CreateBank("IK Manager");

	pBank->PushGroup("Disable solvers", false);
	pBank->AddToggle("Disable all solvers", &CBaseIkManager::ms_DisableAllSolvers);
	for (int i = 0; i < ikSolverTypeCount; ++i)
	{
		pBank->AddToggle(CBaseIkManager::GetSolverName(eIkSolverType(i)), &ms_DisableSolvers[i]);
	}
	pBank->PopGroup();

#if __IK_DEV
	pBank->AddToggle("Display memory", &CBaseIkManager::ms_DisplayMemory);
	pBank->AddToggle("Display solvers", &CBaseIkManager::ms_DisplaySolvers);

	CArmIkSolver::AddWidgets(pBank);
#if ENABLE_HEAD_SOLVER
	CHeadIkSolver::AddWidgets(pBank);
#endif // ENABLE_HEAD_SOLVER
	CLegIkSolver::AddWidgets(pBank);
#if IK_QUAD_LEG_SOLVER_ENABLE
	CQuadLegIkSolver::AddWidgets(pBank);
#endif // IK_QUAD_LEG_SOLVER_ENABLE
#if IK_QUAD_REACT_SOLVER_ENABLE
	CQuadrupedReactSolver::AddWidgets(pBank);
#endif // IK_QUAD_REACT_SOLVER_ENABLE
	CTorsoIkSolver::AddWidgets(pBank);
	CTorsoReactIkSolver::AddWidgets(pBank);
	CTorsoVehicleIkSolver::AddWidgets(pBank);
	CRootSlopeFixupIkSolver::AddWidgets(pBank);
	CBodyLookIkSolver::AddWidgets(pBank);
	CBodyRecoilIkSolver::AddWidgets(pBank);
#endif //__IK_DEV
}
#endif // _BANK

//////////////////////////////////////////////////////////////////////////

void CIkManager::PreTaskUpdate()
{
	// Leg ik is on by default
	ClearFlag(PEDIK_LEGS_AND_PELVIS_OFF);

	// Blending out slowly (rather than instantly) is on by default
	ClearFlag(PEDIK_LEGS_AND_PELVIS_FADE_OFF);
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::PreIkUpdate(float deltaTime)
{
#if __ASSERT
	static bool bPreIkAlreadyAsserted = false;
	if (!bPreIkAlreadyAsserted)
	{
		s32 foreArmIdx = m_pPed->GetBoneIndexFromBoneTag(BONETAG_L_FOREARM);
		if (foreArmIdx!=-1)
		{		
			const Matrix34 &localForeArmMtx = RCC_MATRIX34(m_pPed->GetSkeleton()->GetLocalMtx(foreArmIdx));
			if (fabsf(localForeArmMtx.d.x) > 1.0f)
			{
				bPreIkAlreadyAsserted = true;

				// Find out what animations are running at the time of the bad matrix
				m_pPed->GetAnimDirector()->DebugDump();
				
				// Find out what tasks are running at the time of the bad matrix
				if (m_pPed->GetPedIntelligence())
				{
					m_pPed->GetPedIntelligence()->PrintTasks();
				}

				// Find if the left arm solver is running and what its target is at the time of the bad matrix
				if (HasSolver(ikSolverTypeArm))
				{
					Printf("CIkManager::PreIkUpdate() : Left Arm IK running target = \n");
					static_cast<const CArmIkSolver*>(GetSolver(ikSolverTypeArm))->GetArmGoal(crIKSolverArms::LEFT_ARM).m_Target.Print();
				}
				else
				{
					Printf("CIkManager::PreIkUpdate() : No Left Arm IK running\n");
				}

				Assertf(0, "CIkManager::PreIkUpdate() : The x component of the local left forearm matrix is > 1.0f. (please include TTY)");
			}
		}

		s32 footIdx = m_pPed->GetBoneIndexFromBoneTag(BONETAG_R_FOOT);
		if (footIdx!=-1)
		{
			Matrix34 globalFootMtx;
			m_pPed->GetSkeleton()->GetGlobalMtx(footIdx, RC_MAT34V(globalFootMtx));

			// Use error of 0.05 to allow for slightly non orthonormal matrices
			if(!globalFootMtx.IsOrthonormal(0.05f))
			{
				// Find out what animations are running at the time of the bad matrix
				m_pPed->GetAnimDirector()->DebugDump();

				// Find out what tasks are running at the time of the bad matrix
				if (m_pPed->GetPedIntelligence())
				{
					m_pPed->GetPedIntelligence()->PrintTasks();
				}

				Assertf(0, "CIkManager::PreIkUpdate() : The matrix of the global right foot matrix is not orthonormal (0.05) for model - %s. (please include TTY)", m_pPed->GetModelName());
			}
		}

	}
#endif //__ASSERT	

#if FPS_MODE_SUPPORTED
	if (deltaTime > 0.0f)
	{
		m_fDeltaTime = deltaTime;
		m_bFPSUpdatePass = false;
	}
#endif // FPS_MODE_SUPPORTED

	// Dampen the root motion. Used when camera is in close to stop bobbing
	if (m_pPed->IsLocalPlayer() && m_pPed->GetPlayerInfo())
	{
		m_pPed->GetPlayerInfo()->ProcessDampenRoot();
	}

	BeginHighHeelData(deltaTime);

	// TODO: virtualize PreIkUpdate as PreIterate in crIKSolverBase to simplify below?
	crIKSolverBase* pSolver = GetSolver(ikSolverTypeBodyRecoil);
	if (pSolver)
	{
		static_cast<CBodyRecoilIkSolver*>(pSolver)->PreIkUpdate(deltaTime);
	}

	pSolver = GetSolver(ikSolverTypeArm);
	if (pSolver)
	{
		static_cast<CArmIkSolver*>(pSolver)->PreIkUpdate(deltaTime);
	}

	pSolver = GetSolver(ikSolverTypeBodyLook);
	if (pSolver)
	{
		static_cast<CBodyLookIkSolverProxy*>(pSolver)->PreIkUpdate(deltaTime);
	}

	pSolver = GetSolver(ikSolverTypeRootSlopeFixup);
	if (pSolver)
	{
		static_cast<CRootSlopeFixupIkSolver*>(pSolver)->PreIkUpdate(deltaTime);
	}

	pSolver = GetSolver(ikSolverTypeLeg);
	if (pSolver)
	{
		static_cast<CLegIkSolverProxy*>(pSolver)->PreIkUpdate(deltaTime);
	}

#if FPS_MODE_SUPPORTED
	pSolver = GetSolver(ikSolverTypeTorso);
	if (pSolver)
	{
		ms_DebugInfo.info.nonOwnedSolverPre |= (m_pPed != static_cast<CTorsoIkSolver*>(pSolver)->GetPed());
		static_cast<CTorsoIkSolver*>(pSolver)->PreIkUpdate(deltaTime); 
	}
#endif // FPS_MODE_SUPPORTED

#if IK_QUAD_LEG_SOLVER_ENABLE
	pSolver = GetSolver(ikSolverTypeQuadLeg);
	if (pSolver)
	{
		static_cast<CQuadLegIkSolverProxy*>(pSolver)->PreIkUpdate(deltaTime);
	}
#endif // IK_QUAD_LEG_SOLVER_ENABLE

	pSolver = GetSolver(ikSolverTypeTorsoVehicle);
	if (pSolver)
	{
		static_cast<CTorsoVehicleIkSolverProxy*>(pSolver)->PreIkUpdate(deltaTime);
	}
}


///////////////////////////////////////////////////////////////////////////////

void CIkManager::PostIkUpdate(float deltaTime)
{
	EndHighHeelData(deltaTime);

	// TODO: virtualize PostIkUpdate as PostSolve in crIKSolverBase to simplify below?

	if (HasSolver(ikSolverTypeTorso))
	{
		ms_DebugInfo.info.nonOwnedSolverPost |= (m_pPed != static_cast<CTorsoIkSolver*>(GetSolver(ikSolverTypeTorso))->GetPed());
	}

	// Now the pre-render motion trees have all finished we can update the head solver target without blocking the pre-render motion tree
#if ENABLE_HEAD_SOLVER
	if (HasSolver(ikSolverTypeHead))
	{
		static_cast<CHeadIkSolver*>(GetSolver(ikSolverTypeHead))->PostIkUpdate(deltaTime);
	}
#endif // ENABLE_HEAD_SOLVER

	if (HasSolver(ikSolverTypeArm))
	{
		static_cast<CArmIkSolver*>(GetSolver(ikSolverTypeArm))->PostIkUpdate(deltaTime);
	}

	if (HasSolver(ikSolverTypeBodyLook))
	{
		float fSolverDeltaTime = deltaTime;

#if FPS_MODE_SUPPORTED
		if (m_bFPSUpdatePass)
		{
			fSolverDeltaTime = (deltaTime == 0.0f) ? m_fDeltaTime : 0.0f;
		}
#endif // FPS_MODE_SUPPORTED

		static_cast<CBodyLookIkSolverProxy*>(GetSolver(ikSolverTypeBodyLook))->PostIkUpdate(fSolverDeltaTime);
	}

	if (HasSolver(ikSolverTypeBodyRecoil))
	{
		static_cast<CBodyRecoilIkSolver*>(GetSolver(ikSolverTypeBodyRecoil))->PostIkUpdate(deltaTime);
	}

	if (HasSolver(ikSolverTypeLeg))
	{
		static_cast<CLegIkSolverProxy*>(GetSolver(ikSolverTypeLeg))->PostIkUpdate(deltaTime);
	}

#if IK_QUAD_LEG_SOLVER_ENABLE
	if (HasSolver(ikSolverTypeQuadLeg))
	{
		static_cast<CQuadLegIkSolverProxy*>(GetSolver(ikSolverTypeQuadLeg))->PostIkUpdate(deltaTime);
	}
#endif // IK_QUAD_LEG_SOLVER_ENABLE

#if IK_QUAD_REACT_SOLVER_ENABLE
	if (HasSolver(ikSolverTypeQuadrupedReact))
	{
		static_cast<CQuadrupedReactSolver*>(GetSolver(ikSolverTypeQuadrupedReact))->PostIkUpdate(deltaTime);
	}
#endif // IK_QUAD_REACT_SOLVER_ENABLE

	if (HasSolver(ikSolverTypeTorsoReact))
	{
		static_cast<CTorsoReactIkSolver*>(GetSolver(ikSolverTypeTorsoReact))->PostIkUpdate(deltaTime);
	}

	if (HasSolver(ikSolverTypeTorsoVehicle))
	{
		static_cast<CTorsoVehicleIkSolverProxy*>(GetSolver(ikSolverTypeTorsoVehicle))->PostIkUpdate(deltaTime);
	}

	if (deltaTime > 0.0f)
	{
		RemoveCompletedSolvers();

#if FPS_MODE_SUPPORTED
		if (m_pFirstPersonRequests)
		{
			m_pFirstPersonRequests->Reset();
		}
#endif // FPS_MODE_SUPPORTED
	}

#if __ASSERT
	static bool bPostIKAlreadyAsserted = false;
	if (!bPostIKAlreadyAsserted)
	{
		s32 foreArmIdx = m_pPed->GetBoneIndexFromBoneTag(BONETAG_L_FOREARM);
		if (foreArmIdx!=-1)
		{
			const Matrix34 &localForeArmMtx = RCC_MATRIX34(m_pPed->GetSkeleton()->GetLocalMtx(foreArmIdx));
			if (fabsf(localForeArmMtx.d.x) > 1.0f)
			{
				bPostIKAlreadyAsserted = true;

				// Find out what animations are running at the time of the bad matrix
				m_pPed->GetAnimDirector()->DebugDump();

				// Find out what tasks are running at the time of the bad matrix
				if (m_pPed->GetPedIntelligence())
				{
					m_pPed->GetPedIntelligence()->PrintTasks();
				}

				// Find if the left arm solver is running and what its target is at the time of the bad matrix
				if (HasSolver(ikSolverTypeArm))
				{
					Printf("CIkManager::PostIkUpdate() : Left Arm IK running target = \n");
					static_cast<const CArmIkSolver*>(GetSolver(ikSolverTypeArm))->GetArmGoal(crIKSolverArms::LEFT_ARM).m_Target.Print();
				}
				else
				{
					Printf("CIkManager::PostIkUpdate() : No Left Arm IK running\n");
				}

				Assertf(0, "CIkManager::PostIkUpdate() : The x component of the local left forearm matrix is > 1.0f for model - %s. (please include TTY)", m_pPed->GetModelName());
			}
		}

		s32 footIdx = m_pPed->GetBoneIndexFromBoneTag(BONETAG_R_FOOT);
		if (footIdx!=-1)
		{
			Matrix34 globalFootMtx;
			m_pPed->GetSkeleton()->GetGlobalMtx(footIdx, RC_MAT34V(globalFootMtx));

			// Use error of 0.05 to allow for slightly non orthonormal matrices
			if(!globalFootMtx.IsOrthonormal(0.05f))
			{
				// Find out what animations are running at the time of the bad matrix
				m_pPed->GetAnimDirector()->GetMove().Dump();

				// Find out what tasks are running at the time of the bad matrix
				if (m_pPed->GetPedIntelligence())
				{
					m_pPed->GetPedIntelligence()->PrintTasks();
				}

				Assertf(0, "CIkManager::PostIkUpdate() : The matrix of the global right foot matrix is not orthonormal (0.05) for model - %s. (please include TTY)", m_pPed->GetModelName());
			}
		}
	}
#endif //__ASSERT
}

///////////////////////////////////////////////////////////////////////////////

void CIkManager::ResetAllSolvers()
{
	for(u32 i=0; i<ikSolverTypeCount; i++)
	{
		crIKSolverBase* pSolver = GetSolver((eIkSolverType)i);
		if (pSolver)
		{
			pSolver->Reset();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void CIkManager::ResetAllNonLegSolvers()
{
	for (u32 i = 0; i < ikSolverTypeCount; i++)
	{
		if ((i != ikSolverTypeLeg) IK_QUAD_LEG_SOLVER_ONLY( && (i != ikSolverTypeQuadLeg) ))
		{
			crIKSolverBase* pSolver = GetSolver((eIkSolverType)i);
			if (pSolver)
			{
				pSolver->Reset();
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

bool CIkManager::IsActive(eIkSolverType solverType) const
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(solverType) IK_QUAD_LEG_SOLVER_ONLY( && (solverType != ikSolverTypeQuadLeg) ) IK_QUAD_REACT_SOLVER_ONLY( && (solverType != ikSolverTypeQuadrupedReact) ))
		return false;

	const crIKSolverBase* pSolver = GetSolver(solverType);

	if (pSolver)
	{
		switch (solverType)
		{
			case ikSolverTypeArm:
			case ikSolverTypeBodyLook:
			case ikSolverTypeBodyRecoil:
			case ikSolverTypeHead:
			case ikSolverTypeTorso:
			{
				return true;
			}
			case ikSolverTypeLeg:
			{
				const CLegIkSolver* pLegSolver = static_cast<const CLegIkSolver*>(static_cast<const CLegIkSolverProxy*>(pSolver)->GetSolver());

				if (pLegSolver)
				{
					return (pLegSolver->GetLegBlend(CLegIkSolver::LEFT_LEG) == 1.0f) && (pLegSolver->GetLegBlend(CLegIkSolver::RIGHT_LEG) == 1.0f);
				}

				break;
			}
#if IK_QUAD_LEG_SOLVER_ENABLE
			case ikSolverTypeQuadLeg:
			{
				const CQuadLegIkSolver* pLegSolver = static_cast<const CQuadLegIkSolver*>(static_cast<const CQuadLegIkSolverProxy*>(pSolver)->GetSolver());

				if (pLegSolver)
				{
					return true;
				}

				break;
			}
#endif // IK_QUAD_LEG_SOLVER_ENABLE
#if IK_QUAD_REACT_SOLVER_ENABLE
			case ikSolverTypeQuadrupedReact:
			{
				const CQuadrupedReactSolver* pReactSolver = static_cast<const CQuadrupedReactSolver*>(pSolver);
				return pReactSolver->IsActive();
			}
#endif // IK_QUAD_REACT_SOLVER_ENABLE
			case ikSolverTypeTorsoReact:
			{
				const CTorsoReactIkSolver* pTorsoReactSolver = static_cast<const CTorsoReactIkSolver*>(pSolver);
				return pTorsoReactSolver->IsActive();
			}
			case ikSolverTypeTorsoVehicle:
			{
				const CTorsoVehicleIkSolver* pTorsoVehicleSolver = static_cast<const CTorsoVehicleIkSolver*>(static_cast<const CTorsoVehicleIkSolverProxy*>(pSolver)->GetSolver());

				if (pTorsoVehicleSolver)
				{
					return true;
				}

				break;
			}
			case ikSolverTypeRootSlopeFixup:
			{
				const CRootSlopeFixupIkSolver* pRootSlopeFixupSolver = static_cast<const CRootSlopeFixupIkSolver*>(pSolver);
				return pRootSlopeFixupSolver->IsActive();
			}
			default:
			{
				ikAssertf(0, "CIkManager::IsActive - Solver type not handled");
				break;
			}
		};
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool CIkManager::IsSupported(eIkSolverType solverType) const
{
#if IK_QUAD_LEG_SOLVER_ENABLE
	// Ideally quad leg ik support would be read from PedIKSettings (peds.meta) only, but since 
	// this is strictly to support horse DLC, using horse component existence to define usage.
	if (solverType == ikSolverTypeQuadLeg)
	{
		if ((m_pPed->GetHorseComponent() == NULL) && m_pPed->GetIKSettings().IsIKFlagDisabled(solverType))
		{
			return false;
		}
	}
	else
#endif // IK_QUAD_LEG_SOLVER_ENABLE
#if IK_QUAD_REACT_SOLVER_ENABLE
	// Ideally quad react ik support would be read from PedIKSettings (peds.meta) only, but since 
	// this is strictly to support horse DLC, using horse component existence to define usage.
	if (solverType == ikSolverTypeQuadrupedReact)
	{
		if ((m_pPed->GetHorseComponent() == NULL) && m_pPed->GetIKSettings().IsIKFlagDisabled(solverType))
		{
			return false;
		}

		Vec3V vPedToCamera(VECTOR3_TO_VEC3V(camInterface::GetPos()) - m_pPed->GetTransform().GetPosition());
		return (MagSquared(vPedToCamera).Getf() < square(CQuadrupedReactSolver::ms_Params.fMaxCameraDistance));
	}
	else
#endif // IK_QUAD_REACT_SOLVER_ENABLE
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(solverType))
	{
		return false;
	}

	if (solverType == ikSolverTypeTorsoReact)
	{
		if (!m_pPed->GetCapsuleInfo()->IsBiped())
		{
			return false;
		}

		if (!m_pPed->IsDead() && m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_InVehicle))
		{
			const CVehicle* pVehicle = m_pPed->GetMyVehicle();

			bool bFirstPerson = m_pPed->IsInFirstPersonVehicleCamera();
			if (!bFirstPerson && pVehicle && !pVehicle->InheritsFromBike() && !pVehicle->InheritsFromQuadBike() && !pVehicle->InheritsFromAmphibiousQuadBike() && !pVehicle->GetIsJetSki())
			{
				return false;
			}
		}

		Vector3 vPedToCamera(camInterface::GetPos() - VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition()));
		return (vPedToCamera.Mag2() < square(CTorsoReactIkSolver::ms_Params.fMaxCameraDistance));
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

void CIkManager::SetFlag(u32 flag)
{
	CBaseIkManager::SetFlag(flag);
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::LookAt(u32 hashKey, const CEntity* pEntity, s32 time,
						eAnimBoneTag offsetBoneTag, const Vector3* pOffsetPos, u32 flags,
						s32 UNUSED_PARAM(blendInTime), s32 UNUSED_PARAM(blendOutTime), eLookAtPriority priority)
{
	ikAssert(m_pPed);

	// The entity does not support head ik
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeHead) || !IsSupported(IKManagerSolverTypes::ikSolverTypeBodyLook))
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_DISABLED, "CIkManager::LookAt");
		return;
	}

	// If a valid bonetag was specified
	if (offsetBoneTag != -1)
	{
		// A bonetag was specified but the entity is NULL
		if ( pEntity == NULL )
		{
			ikAssertf(0, "Boneid : %d specified but no target entity", (u16)offsetBoneTag);
			IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_INVALID_PARAM, "CIkManager::LookAt");
			return;
		}

		// A bonetag was specified but the entities skeleton is NULL
		if (pEntity->GetSkeleton() == NULL)
		{
			ikAssertf(0, "Boneid : %d specified but target entity : %s has no skeleton", (u16)offsetBoneTag, pEntity->GetModelName());
			IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_INVALID_PARAM, "CIkManager::LookAt");
			return;
		}

		// A bonetag was specified that is invalid for the skeleton
		s32 offsetBoneIdx = -1;
		pEntity->GetSkeletonData().ConvertBoneIdToIndex((u16)offsetBoneTag, offsetBoneIdx);		
		if (offsetBoneIdx == -1)
		{
			ikAssertf(0, "Unknown boneid : %d specified for target entity : %s", (u16)offsetBoneTag, pEntity->GetModelName());
			IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_INVALID_PARAM, "CIkManager::LookAt");
			return;
		}
	}

	// Is the ped alive?
	if (m_pPed->ShouldBeDead())
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_CHAR_DEAD, "CIkManager::LookAt");
		return;
	}

	// Is the head blocked using the ped reset flags (i.e via script or code)
	if (m_pPed->GetHeadIkBlocked())
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_BLOCKED, "CIkManager::LookAt");
		return;
	}

	// Is the head blocked using the ped reset flags specifically for code driven head ik
	if (m_pPed->GetCodeHeadIkBlocked() && !(flags&LF_FROM_SCRIPT))
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_BLOCKED, "CIkManager::LookAt");
		return;
	}

	// Is the ped playing an animation that is incompatible with the ik?
	int iInvalidFlags = APF_BLOCK_IK|APF_BLOCK_HEAD_IK;
	if (m_pPed->GetMovePed().GetBlendHelper().IsFlagSet(iInvalidFlags))
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_BLOCKED_BY_ANIM, "CIkManager::LookAt");
		return;
	}

	// Is the call not from script?
	bool fromScript = (flags & LF_FROM_SCRIPT) != 0;
	if (!fromScript)
	{
		if (m_pPed->GetIsVisibleInSomeViewportThisFrame() == false)
		{
			IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_OFFSCREEN, "CIkManager::LookAt");
			return;
		}

		if (IsGreaterThanAll(MagSquared(Subtract(VECTOR3_TO_VEC3V(camInterface::GetPos()), m_pPed->GetTransform().GetPosition())), ScalarV(square(20.0f))))
		{
			IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_RANGE, "CIkManager::LookAt");
			return;
		}
	}

	// Does the IK task exist?
	if (HasSolver(ikSolverTypeBodyLook))
	{
		CBodyLookIkSolverProxy* pSolver = static_cast<CBodyLookIkSolverProxy*>(GetSolver(ikSolverTypeBodyLook));

		if ((s8)priority < pSolver->GetPriority())
		{
			IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_PRIORITY, "CIkManager::LookAt");
			return;
		}
	}

	CBodyLookIkSolverProxy* pSolver = static_cast<CBodyLookIkSolverProxy*>(GetOrAddSolver(ikSolverTypeBodyLook));

	if (pSolver)
	{
		// Build request
		Vec3V vTargetOffset(V_ZERO);

		if (pOffsetPos)
		{
			vTargetOffset = VECTOR3_TO_VEC3V(*pOffsetPos);
		}

		CIkRequestBodyLook request(pEntity, offsetBoneTag, vTargetOffset, !(flags & LF_WHILE_NOT_IN_FOV) ? LOOKIK_USE_FOV : 0, (s32)priority, hashKey);
		request.SetLookAtFlags((s32)flags);

		if (flags > 0)
		{
			ConvertLookAtFlagsToLookIk(flags, request);
		}

		if (!(flags & LF_USE_EYES_ONLY))
		{
			// Enable neck component by default
			request.SetRotLimNeck(LOOKIK_ROT_ANGLE_YAW, Max(request.GetRotLimHead(LOOKIK_ROT_ANGLE_YAW), LOOKIK_ROT_LIM_WIDE));
			request.SetRotLimNeck(LOOKIK_ROT_ANGLE_PITCH, (m_pPed->GetPedType() != PEDTYPE_ANIMAL) ? LOOKIK_ROT_LIM_NARROWEST : LOOKIK_ROT_LIM_WIDE);

			if (!(flags & LF_USE_REF_DIR_ABSOLUTE))
			{
				// Set facing dir mode
				request.SetRefDirHead(LOOKIK_REF_DIR_FACING);
				request.SetRefDirNeck(LOOKIK_REF_DIR_FACING);
			}
		}

		request.SetHoldTime(time >= 0 ? (u16)time : (u16)-1);

		pSolver->Request(request);
	}
}

//////////////////////////////////////////////////////////////////////////

bool CIkManager::IsLooking()
{
	return HasSolver(ikSolverTypeBodyLook);
}

//////////////////////////////////////////////////////////////////////////

bool CIkManager::IsLooking(u32 hashKey)
{
	if(HasSolver(ikSolverTypeBodyLook))
	{
		CBodyLookIkSolverProxy* pSolver = static_cast<CBodyLookIkSolverProxy*>(GetSolver(ikSolverTypeBodyLook));

		if (pSolver->GetHashKey() == hashKey)
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::AbortLookAt(s32 UNUSED_PARAM(blendTime))
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeBodyLook))
	{
		//return;
	}

	//check if the solver exists
	if (HasSolver(ikSolverTypeBodyLook))
	{
		static_cast<CBodyLookIkSolverProxy*>(GetSolver(ikSolverTypeBodyLook))->Abort();
	}
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::AbortLookAt(s32 UNUSED_PARAM(blendTime), u32 hashKey)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeBodyLook))
	{
		//return;
	}

	//check if the solver exists
	if (HasSolver(ikSolverTypeBodyLook))
	{
		CBodyLookIkSolverProxy* pSolver = static_cast<CBodyLookIkSolverProxy*>(GetSolver(ikSolverTypeBodyLook));

		if (pSolver->GetHashKey() == hashKey)
		{
			pSolver->Abort();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

const CEntity* CIkManager::GetLookAtEntity() const
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeBodyLook))
	{
		//return NULL; 
	}

	if (HasSolver(ikSolverTypeBodyLook))
	{
		return static_cast<const CBodyLookIkSolverProxy*>(GetSolver(ikSolverTypeBodyLook))->GetTargetEntity();
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////

const u32 CIkManager::GetLookAtFlags() const
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeBodyLook))
	{
		//return NULL; 
	}

	if (HasSolver(ikSolverTypeBodyLook))
	{
		const CBodyLookIkSolverProxy* pSolver = static_cast<const CBodyLookIkSolverProxy*>(GetSolver(ikSolverTypeBodyLook));

		s32 sLookAtFlags = pSolver->GetLookAtFlags();

		if (sLookAtFlags == -1)
		{
			// LookIk was requested directly instead of through LookAt(), so generate an equivalent set of LookAt flags for now
			return ConvertLookIkToLookAtFlags(pSolver);
		}

		return (u32)sLookAtFlags;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
#if __BANK
const u32 CIkManager::GetLookAtHashKey() const
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeBodyLook))
	{
		//return 0; 
	}

	if (HasSolver(ikSolverTypeBodyLook))
	{
		const CBodyLookIkSolverProxy* pSolver = static_cast<const CBodyLookIkSolverProxy*>(GetSolver(ikSolverTypeBodyLook));
		return pSolver->GetHashKey();
	}

	return 0;
}
#endif

//////////////////////////////////////////////////////////////////////////

bool CIkManager::GetLookAtTargetPosition(Vector3& pos)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeBodyLook))
	{
		//return NULL; 
	}

	if (HasSolver(ikSolverTypeBodyLook))
	{
		CBodyLookIkSolverProxy* pSolver = static_cast<CBodyLookIkSolverProxy*>(GetSolver(ikSolverTypeBodyLook));
		
		if (pSolver)
		{
			pos = VEC3V_TO_VECTOR3(pSolver->GetTargetPosition());
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::ConvertLookAtFlagsToLookIk(u32 uFlags, CIkRequestBodyLook& request)
{
	LookIkTurnRate eTurnRate = LOOKIK_TURN_RATE_NORMAL;
	LookIkRotationLimit eYawLimit = LOOKIK_ROT_LIM_WIDE;
	LookIkRotationLimit ePitchLimit = LOOKIK_ROT_LIM_WIDE;
	u32 uRequestFlags = 0;

	if (uFlags & LF_FROM_SCRIPT)
		uRequestFlags |= LOOKIK_SCRIPT_REQUEST;

	if (uFlags & LF_USE_CAMERA_FOCUS)
		uRequestFlags |= LOOKIK_USE_CAMERA_FOCUS;

	if (uFlags & LF_FAST_TURN_RATE)
		eTurnRate = LOOKIK_TURN_RATE_FAST;

	if (uFlags & LF_SLOW_TURN_RATE)
		eTurnRate = LOOKIK_TURN_RATE_SLOW;

	if (!(uFlags & LF_USE_EYES_ONLY))
	{
		if (uFlags & LF_WIDEST_YAW_LIMIT)
		{
			eYawLimit = LOOKIK_ROT_LIM_WIDEST;
		}
		else if (uFlags & LF_NARROW_YAW_LIMIT)
		{
			eYawLimit = LOOKIK_ROT_LIM_NARROW;
		}
		else if (uFlags & LF_NARROWEST_YAW_LIMIT)
		{
			eYawLimit = LOOKIK_ROT_LIM_NARROWEST;
		}

		if (uFlags & LF_WIDEST_PITCH_LIMIT)
		{
			ePitchLimit = LOOKIK_ROT_LIM_WIDEST;
		}
		else if (uFlags & LF_NARROW_PITCH_LIMIT)
		{
			ePitchLimit = LOOKIK_ROT_LIM_NARROW;
		}
		else if (uFlags & LF_NARROWEST_PITCH_LIMIT)
		{
			ePitchLimit = LOOKIK_ROT_LIM_NARROWEST;
		}
	}
	else
	{
		// Disable head
		eYawLimit = LOOKIK_ROT_LIM_OFF;
		ePitchLimit = LOOKIK_ROT_LIM_OFF;
	}

	request.SetTurnRate(eTurnRate);
	request.SetRotLimHead(LOOKIK_ROT_ANGLE_YAW, eYawLimit);
	request.SetRotLimHead(LOOKIK_ROT_ANGLE_PITCH, ePitchLimit);
	request.SetFlags(request.GetFlags() | uRequestFlags);
}

//////////////////////////////////////////////////////////////////////////

u32 CIkManager::ConvertLookIkToLookAtFlags(const CBodyLookIkSolverProxy* pSolver) const
{
	ikAssert(pSolver);

	u32 uLookAtFlags = 0;
	u32 uFlags = pSolver->GetFlags();

	if (uFlags & LOOKIK_USE_CAMERA_FOCUS)
		uLookAtFlags |= LF_USE_CAMERA_FOCUS;

	if (uFlags & LOOKIK_SCRIPT_REQUEST)
		uLookAtFlags |= LF_FROM_SCRIPT;

	if (!(uFlags & LOOKIK_USE_FOV))
		uLookAtFlags |= LF_WHILE_NOT_IN_FOV;

	LookIkTurnRate eTurnRate = (LookIkTurnRate)pSolver->GetTurnRate();

	if (eTurnRate == LOOKIK_TURN_RATE_SLOW)
	{
		uLookAtFlags |= LF_SLOW_TURN_RATE;
	}
	else if (eTurnRate == LOOKIK_TURN_RATE_FAST)
	{
		uLookAtFlags |= LF_FAST_TURN_RATE;
	}

	LookIkRotationLimit eYawLimit = (LookIkRotationLimit)pSolver->GetHeadRotLim(LOOKIK_ROT_ANGLE_YAW);
	LookIkRotationLimit ePitchLimit = (LookIkRotationLimit)pSolver->GetHeadRotLim(LOOKIK_ROT_ANGLE_PITCH);

	if ((eYawLimit == LOOKIK_ROT_LIM_OFF) && (ePitchLimit == LOOKIK_ROT_LIM_OFF))
	{
		uLookAtFlags |= LF_USE_EYES_ONLY;
	}
	else
	{
		const u32 aLookAtYawLimitMapping[LOOKIK_ROT_LIM_NUM] =
		{
			0, LF_NARROWEST_YAW_LIMIT, LF_NARROW_YAW_LIMIT, LF_WIDE_YAW_LIMIT, LF_WIDEST_YAW_LIMIT
		};

		const u32 aLookAtPitchLimitMapping[LOOKIK_ROT_LIM_NUM] =
		{
			0, LF_NARROWEST_PITCH_LIMIT, LF_NARROW_PITCH_LIMIT, LF_WIDE_PITCH_LIMIT, LF_WIDEST_PITCH_LIMIT
		};

		uLookAtFlags |= aLookAtYawLimitMapping[eYawLimit];
		uLookAtFlags |= aLookAtPitchLimitMapping[ePitchLimit];
	}

	return uLookAtFlags;
}

//////////////////////////////////////////////////////////////////////////

bool CIkManager::IsTargetWithinFOV(const Vector3& target, const Vector3 &min, const Vector3 &max)
{
	ikAssert(m_pPed);
	s32 headBoneIndex = m_pPed->GetBoneIndexFromBoneTag(BONETAG_HEAD);
	Matrix34 globalHeadBoneMat;
	m_pPed->GetGlobalMtx(headBoneIndex, globalHeadBoneMat);

	s32 neckBoneIndex = m_pPed->GetBoneIndexFromBoneTag(BONETAG_NECK);
	Matrix34 globalNeckBoneMat;
	m_pPed->GetGlobalMtx(neckBoneIndex, globalNeckBoneMat);

	Matrix34 globalNeckRotHeadPosMat(globalNeckBoneMat);
	globalNeckRotHeadPosMat.d = globalHeadBoneMat.d;

	// Find the goal relative to the neck bones rotation and the head bones position
	Vector3 localTarget(VEC3_ZERO);
	globalNeckRotHeadPosMat.UnTransform(target, localTarget);

	// Avoid gimbal lock
	static dev_float fEpsilon = 0.001f;
	if (rage::Abs(localTarget.y) < fEpsilon &&  rage::Abs(localTarget.z) < fEpsilon)
	{
		localTarget.y = fEpsilon;
		localTarget.z = 0.0f;
	}

	// Calculate the target yaw and the target pitch
	float targetYaw = rage::Atan2f(localTarget.z, localTarget.y);
	float targetPitch = rage::Atan2f(-localTarget.x, sqrtf(localTarget.y*localTarget.y+localTarget.z*localTarget.z));

	bool validYaw = true;
	if(targetYaw > max.x)
	{
		validYaw = false;
	}
	else if(targetYaw < min.x)
	{
		validYaw = false;
	}

	bool validPitch = true;
	if(targetPitch > max.z)
	{
		validPitch = false;
	}
	else if(targetPitch < min.z)
	{
		validPitch = false;
	}

	return (validYaw && validPitch);
}

//////////////////////////////////////////////////////////////////////////

bool CIkManager::Request(const CIkRequestArm& request FPS_MODE_SUPPORTED_ONLY(, s32 pass))
{
#if FPS_MODE_SUPPORTED
	bool bFirstPersonRequestAdded = false;
	if ((pass&IK_PASS_FPS)!=0)
	{
		// lazy init the request structure if need be:
		FirstPersonPassRequests& requests = GetFpsRequests();
		s32 arm = (request.GetArm() == IK_ARM_LEFT) ? CArmIkSolver::LEFT_ARM : CArmIkSolver::RIGHT_ARM;
		requests.m_armRequests[arm] = request;
		requests.m_hasRequests[(arm == CArmIkSolver::LEFT_ARM ? kRequestArmLeft : kRequestArmRight)] = true;
		bFirstPersonRequestAdded = true;
	}

	if ((pass&IK_PASS_MAIN)==0)
	{
		return bFirstPersonRequestAdded;
	}
#endif // FPS_MODE_SUPPORTED

	if (!IsSupported(IKManagerSolverTypes::ikSolverTypeArm))
	{
		return false;
	}

	CArmIkSolver* pSolver = static_cast<CArmIkSolver*>(GetOrAddSolver(ikSolverTypeArm));

	if (pSolver)
	{
		pSolver->Request(request);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CIkManager::Request(const CIkRequestResetArm& request)
{
	if (!IsSupported(IKManagerSolverTypes::ikSolverTypeArm))
	{
		return false;
	}

	CArmIkSolver* pSolver = static_cast<CArmIkSolver*>(GetSolver(ikSolverTypeArm));

	if (pSolver)
	{
		pSolver->ResetArm((request.GetArm() == IK_ARM_RIGHT) ? crIKSolverArms::RIGHT_ARM : crIKSolverArms::LEFT_ARM);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CIkManager::Request(const CIkRequestBodyLook& request FPS_MODE_SUPPORTED_ONLY(, s32 pass))
{
#if FPS_MODE_SUPPORTED
	bool bFirstPersonRequestAdded = false;

	if ((pass & IK_PASS_FPS) != 0)
	{
		// lazy init the request structure if need be:
		FirstPersonPassRequests& requests = GetFpsRequests();

		requests.m_lookRequest = request;
		requests.m_pLookTarget = request.GetTargetEntity();
		requests.m_bLookTargetParam = requests.m_pLookTarget != NULL;
		requests.m_hasRequests[kRequestLook] = true;

		bFirstPersonRequestAdded = true;
	}

	if ((pass & IK_PASS_MAIN) == 0)
	{
		return bFirstPersonRequestAdded;
	}
#endif // FPS_MODE_SUPPORTED

	if (!IsSupported(IKManagerSolverTypes::ikSolverTypeBodyLook))
	{
		return false;
	}

	if (HasSolver(ikSolverTypeBodyLook))
	{
		if (request.GetRequestPriority() < (s32)static_cast<CBodyLookIkSolverProxy*>(GetSolver(ikSolverTypeBodyLook))->GetPriority())
		{
			return false;
		}
	}

	if ((request.GetFlags() & LOOKIK_SCRIPT_REQUEST) == 0)
	{
		if (m_pPed->GetIsVisibleInSomeViewportThisFrame() == false)
		{
			return false;
		}

		if (IsGreaterThanAll(MagSquared(Subtract(VECTOR3_TO_VEC3V(camInterface::GetPos()), m_pPed->GetTransform().GetPosition())), ScalarV(square(60.0f))))
		{
			return false;
		}
	}

	CBodyLookIkSolverProxy* pSolver = static_cast<CBodyLookIkSolverProxy*>(GetOrAddSolver(ikSolverTypeBodyLook));

	if (pSolver)
	{
		pSolver->Request(request);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::SetAbsoluteArmIKTarget(crIKSolverArms::eArms arm, const Vector3& target, s32 controlFlags, float blendInTime, float blendOutTime, float blendInRange, float blendOutRange)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeArm))
	{
		//return;
	}

	CArmIkSolver* pSolver = static_cast<CArmIkSolver*>(GetOrAddSolver(ikSolverTypeArm));

	if (pSolver)
	{
		pSolver->SetAbsoluteTarget(arm, RCC_VEC3V(target), controlFlags, blendInTime, blendOutTime, blendInRange, blendOutRange);
	}
}

///////////////////////////////////////////////////////////////////////////////

void CIkManager::SetRelativeArmIKTarget(crIKSolverArms::eArms arm, const CDynamicEntity* entity, int boneIdx, const Vector3& offset, s32 controlFlags, float blendInTime, float blendOutTime, float blendInRange, float blendOutRange)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeArm))
	{
		//return;
	}

	CArmIkSolver* pSolver = static_cast<CArmIkSolver*>(GetOrAddSolver(ikSolverTypeArm));

	if (pSolver)
	{
		pSolver->SetRelativeTarget(arm, entity, boneIdx, RCC_VEC3V(offset), controlFlags, blendInTime, blendOutTime, blendInRange, blendOutRange);
	}
}

///////////////////////////////////////////////////////////////////////////////

#if FPS_MODE_SUPPORTED
void CIkManager::SetRelativeArmIKTarget(crIKSolverArms::eArms arm, const CDynamicEntity* targetEntity, int targetBoneIdx, const Vector3& targetOffset, const Quaternion& targetRotation, s32 flags, float blendInTime, float blendOutTime, float blendInRange, float blendOutRange)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeArm))
	{
		//return;
	}

	CArmIkSolver* pSolver = static_cast<CArmIkSolver*>(GetOrAddSolver(ikSolverTypeArm));

	if (pSolver)
	{
		pSolver->SetRelativeTarget(arm, targetEntity, targetBoneIdx, RCC_VEC3V(targetOffset), RCC_QUATV(targetRotation), flags, blendInTime, blendOutTime, blendInRange, blendOutRange);
	}
}
#endif // FPS_MODE_SUPPORTED

///////////////////////////////////////////////////////////////////////////////

void CIkManager::AimWeapon(crIKSolverArms::eArms eArm, s32 sFlags)
{
	if (m_pPed->GetWeaponManager() == NULL)
		return;

	const CWeapon* pWeapon = m_pPed->GetWeaponManager()->GetEquippedWeapon();

	if (pWeapon == NULL)
		return;

	eAnimBoneTag eBoneTag = (eArm == crIKSolverArms::RIGHT_ARM) ? BONETAG_R_HAND : BONETAG_L_HAND;

	SetRelativeArmIKTarget(eArm, m_pPed, m_pPed->GetBoneIndexFromBoneTag(eBoneTag), Vector3(Vector3::ZeroType), AIK_TARGET_WRT_HANDBONE | sFlags, PEDIK_ARMS_FADEIN_TIME_QUICK, PEDIK_ARMS_FADEOUT_TIME_QUICK);
}

///////////////////////////////////////////////////////////////////////////////

bool CIkManager::PointGunAtPosition(const Vector3& pos, float fBlendAmount)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return false;
	}

	CTorsoIkSolver *gunSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (gunSolver)
	{
		return gunSolver->PointGunAtPosition(pos,fBlendAmount);
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool CIkManager::PointGunInDirection(float yaw, float pitch, bool bRollPitch, float fBlendAmount)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return false;
	}

	CTorsoIkSolver *gunSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (gunSolver)
	{
		return gunSolver->PointGunInDirection(yaw,pitch,bRollPitch,fBlendAmount);
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

void CIkManager::GetGunAimAnglesFromPosition(const Vector3 &pos, float &yaw, float &pitch)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return;
	}

	CTorsoIkSolver *gunSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (gunSolver)
	{
		gunSolver->GetGunAimAnglesFromPosition(pos,yaw,pitch);
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
	}
}

///////////////////////////////////////////////////////////////////////////////

void CIkManager::GetGunAimPositionFromAngles(float yaw, float pitch, Vector3 &pos)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return;
	}

	CTorsoIkSolver *gunSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));
	if (gunSolver)
	{
		gunSolver->GetGunAimPositionFromAngles(yaw,pitch,pos);
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
	}
}

///////////////////////////////////////////////////////////////////////////////

void CIkManager::GetGunAimDirFromGlobalTarget(const Vector3 &globalTarget, Vector3 &normAimDir)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return;
	}

	CTorsoIkSolver *gunSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));
	if (gunSolver)
	{
		gunSolver->GetGunAimDirFromGlobalTarget(globalTarget,normAimDir);
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
	}
}

///////////////////////////////////////////////////////////////////////////////

void CIkManager::GetGunGlobalTargetFromAimDir(Vector3 &globalTarget, const Vector3 &normAimDir)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return;
	}

	CTorsoIkSolver *gunSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));
	if (gunSolver)
	{
		gunSolver->GetGunGlobalTargetFromAimDir(globalTarget, normAimDir);
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
	}
}

//////////////////////////////////////////////////////////////////////////

u32 CIkManager::GetTorsoSolverStatus()
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		return 0;
	}

	CTorsoIkSolver* pSolver =  static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		return pSolver->GetTorsoSolverStatus();
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
		return 0;
	}
}

//////////////////////////////////////////////////////////////////////////

float CIkManager::GetTorsoMinYaw()
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return 0.0f;
	}

	CTorsoIkSolver* pSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		return pSolver->GetTorsoMinYaw();
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
		return 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////

float CIkManager::GetTorsoMaxYaw()
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return 0.0f;
	}

	CTorsoIkSolver* pSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		return pSolver->GetTorsoMaxYaw();
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
		return 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////

float CIkManager::GetTorsoMinPitch()
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return 0.0f;
	}

	CTorsoIkSolver* pSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		return pSolver->GetTorsoMinPitch();
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
		return 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////

float CIkManager::GetTorsoMaxPitch()
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return 0.0f;
	}

	CTorsoIkSolver* pSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		return pSolver->GetTorsoMaxPitch();
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
		return 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////

float CIkManager::GetTorsoOffsetYaw() const
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return 0.0f;
	}

	const CTorsoIkSolver* pSolver = static_cast<const CTorsoIkSolver*>(GetSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		return pSolver->GetTorsoOffsetYaw();
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
		return 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////

float CIkManager::GetTorsoBlend() const
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return 0.0f;
	}

	const CTorsoIkSolver* pSolver = static_cast<const CTorsoIkSolver*>(GetSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		return pSolver->GetTorsoBlend();
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
		return 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////

const Vector3& CIkManager::GetTorsoTarget()
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return VEC3_ZERO;
	}

	CTorsoIkSolver* pSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (pSolver)	
	{
		return pSolver->GetTorsoTarget(); //need fatal assert here
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
		return VEC3_ZERO;
	}

}

//////////////////////////////////////////////////////////////////////////

void CIkManager::SetTorsoMinYaw(float minYaw)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return;
	}

	CTorsoIkSolver* pSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		pSolver->SetTorsoMinYaw(minYaw);
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
	}
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::SetTorsoMaxYaw(float maxYaw)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return;
	}

	CTorsoIkSolver* pSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		pSolver->SetTorsoMaxYaw(maxYaw);
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
	}
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::SetTorsoMinPitch( float minPitch )
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return;
	}

	CTorsoIkSolver *gunSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (gunSolver)
	{
		gunSolver->SetTorsoMinPitch( minPitch );
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
	}

}

//////////////////////////////////////////////////////////////////////////

void CIkManager::SetTorsoMaxPitch( float maxPitch )
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return;
	}

	CTorsoIkSolver *gunSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (gunSolver)
	{
		gunSolver->SetTorsoMaxPitch( maxPitch );
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
	}
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::SetTorsoOffsetYaw(float torsoOffsetYaw)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return;
	}

	CTorsoIkSolver* pSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		pSolver->SetTorsoOffsetYaw(torsoOffsetYaw);
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
	}
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::SetTorsoYawDeltaLimits(float yawDelta, float yawDeltaSmoothRate, float yawDeltaSmoothEase)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return;
	}

	CTorsoIkSolver* pSolver = static_cast<CTorsoIkSolver*>(GetOrAddSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		pSolver->SetTorsoYawDeltaLimits(yawDelta, yawDeltaSmoothRate, yawDeltaSmoothEase);
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
	}
}

//////////////////////////////////////////////////////////////////////////

float CIkManager::GetTorsoYaw() const
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return 0.0f;
	}

	const CTorsoIkSolver* pSolver = static_cast<const CTorsoIkSolver*>(GetSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		return pSolver->GetTorsoYaw();
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
		return 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////

float CIkManager::GetTorsoYawDelta() const
{
	const CTorsoIkSolver* pSolver = static_cast<const CTorsoIkSolver*>(GetSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		return pSolver->GetYawDelta();
	}

	return 0.0f;
}

//////////////////////////////////////////////////////////////////////////

float CIkManager::GetTorsoPitch() const
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return 0.0f;
	}

	const CTorsoIkSolver* pSolver = static_cast<const CTorsoIkSolver*>(GetSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		return pSolver->GetTorsoPitch();
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
		return 0.0f;
	}
}

///////////////////////////////////////////////////////////////////////////////

void CIkManager::SetDisablePitchFixUp(bool disablePitchFixUp)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeTorso))
	{
		//return;
	}

	CTorsoIkSolver* pSolver = static_cast<CTorsoIkSolver*>(GetSolver(ikSolverTypeTorso));

	if (pSolver)
	{
		pSolver->SetDisablePitchFixUp(disablePitchFixUp);
	}
	else
	{
		//ikAssertf(0, "This ped's IK manager does not contain an torso solver.");
	}
}

//////////////////////////////////////////////////////////////////////////

bool CIkManager::CanUseLegIK(bool& bCanUsePelvis, bool& bCanUseFade)
{
	if (IsFlagSet(PEDIK_LEGS_AND_PELVIS_OFF))
	{
		return false;
	}

	if (m_pPed->GetMovePed().GetBlendHelper().IsFlagSet(APF_BLOCK_IK | APF_BLOCK_LEG_IK))
	{
		return false;
	}

	// Allows script to turn off leg IK
	if (m_pPed->m_PedConfigFlags.GetPedLegIkMode() == LEG_IK_MODE_OFF)
	{
		if (!IsFlagSet(PEDIK_LEGS_AND_PELVIS_FADE_ON))
		{
			// Blend out instantly
			bCanUseFade = false;
		}

		return false;
	}

	// Don't use Leg Ik when in ragdoll or in water
	if (m_pPed->GetUsingRagdoll() || (m_pPed->GetRagdollState() > RAGDOLL_STATE_PHYS_ACTIVATE) || m_pPed->GetIsInWater())
	{
		// Blend out instantly
		bCanUseFade = false;
		return false;
	}

	bCanUsePelvis = true;

	// Don't use Leg Ik when attached (unless attached to the ground or still on a bike).
	bool bIsValid = false;
	if (m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsVaulting))
	{
		if (m_pPed->GetPedResetFlag(CPED_RESET_FLAG_RequiresLegIk))
		{
			bCanUsePelvis = false;
		}
	}
	else if (m_pPed->GetIsAttached())
	{
		CEntity* pAttachEntity = static_cast<CEntity*>(m_pPed->GetAttachParent());
		if (pAttachEntity && pAttachEntity->GetIsTypeVehicle())
		{
			bIsValid = m_pPed->GetPedResetFlag(CPED_RESET_FLAG_RequiresLegIk);

			if (bIsValid)
			{
				bCanUsePelvis = false;
			}
		}

		if (!bIsValid && !m_pPed->GetIsAttachedToGround())
		{
			if (!m_pPed->GetIkManager().IsFlagSet(PEDIK_LEGS_AND_PELVIS_FADE_ON))
			{
				// Blend out instantly
				bCanUseFade = false;
			}

			return false;
		}
	}

	const bool bInAir = m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_IsInTheAir) && !m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsJumping);
	const bool bInWrithe = m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsInWrithe);
	const bool bIsDivingOutOfVehicle = false; // m_pPed->GetPedIntelligence()->GetQueriableInterface()->GetStateForTaskType(CTaskTypes::TASK_EXIT_VEHICLE_EXIT_SEAT) == CTaskExitVehicleExitSeat::State_JumpOutOfSeat ? true : false;

	bool bIsClimbingLadder = m_pPed->GetPedIntelligence()->IsPedClimbingLadder();
	if (m_pPed->GetPedResetFlag(CPED_RESET_FLAG_RequiresLegIk) && bIsClimbingLadder)
	{
		bCanUseFade = false;
		bIsClimbingLadder = false;
	}

	if ((!bIsValid && m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_InVehicle))
		|| bInAir 
		|| bIsClimbingLadder
		|| bIsDivingOutOfVehicle
		|| bInWrithe)
	{
		if (!IsFlagSet(PEDIK_LEGS_AND_PELVIS_FADE_ON))
		{
			if (m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_InVehicle))
			{
				// Blend out instantly
				bCanUseFade = false;
			}
		}

		// Blend out normally
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

#if FPS_MODE_SUPPORTED
bool CIkManager::IsFirstPersonStealthTransition() const
{
	if (HasSolver(ikSolverTypeLeg))
	{
		const CLegIkSolver* pSolver = static_cast<const CLegIkSolver*>(static_cast<const CLegIkSolverProxy*>(GetSolver(ikSolverTypeLeg))->GetSolver());

		if (pSolver)
		{
			return pSolver->IsFirstPersonStealthTransition();
		}
	}

	return false;
}
#endif // FPS_MODE_SUPPORTED

//////////////////////////////////////////////////////////////////////////

#if FPS_MODE_SUPPORTED
bool CIkManager::IsFirstPersonModeTransition() const
{
	if (HasSolver(ikSolverTypeLeg))
	{
		const CLegIkSolver* pSolver = static_cast<const CLegIkSolver*>(static_cast<const CLegIkSolverProxy*>(GetSolver(ikSolverTypeLeg))->GetSolver());

		if (pSolver)
		{
			return pSolver->IsFirstPersonModeTransition();
		}
	}

	return false;
}
#endif // FPS_MODE_SUPPORTED

//////////////////////////////////////////////////////////////////////////

float CIkManager::GetPelvisDeltaZForCamera(bool shouldIgnoreIkControl) const
{
	if (HasSolver(ikSolverTypeLeg))
	{
		const CLegIkSolver* pSolver = static_cast<const CLegIkSolver*>(static_cast<const CLegIkSolverProxy*>(GetSolver(ikSolverTypeLeg))->GetSolver());

		if (pSolver)
		{
			return pSolver->GetPelvisDeltaZForCamera(shouldIgnoreIkControl);
		}
	}

	return 0.0f;
}

//////////////////////////////////////////////////////////////////////////

u32 CIkManager::GetDefaultLegIkMode(const CPed* pPed)
{
	ikAssert(pPed);
	return pPed->IsLocalPlayer() ? LEG_IK_MODE_FULL : LEG_IK_MODE_PARTIAL;
}

//////////////////////////////////////////////////////////////////////////

bool CIkManager::Request(const CIkRequestBodyReact& request)
{
	eIkSolverType solverType = IKManagerSolverTypes::ikSolverTypeTorsoReact;

#if IK_QUAD_REACT_SOLVER_ENABLE
	const CBaseCapsuleInfo* pCapsuleInfo = m_pPed->GetCapsuleInfo();
	if (pCapsuleInfo && pCapsuleInfo->IsQuadruped())
	{
		solverType = IKManagerSolverTypes::ikSolverTypeQuadrupedReact;
	}
#endif // IK_QUAD_REACT_SOLVER_ENABLE

	if (!IsSupported(solverType))
	{
		return false;
	}

	if (m_pPed->GetPedIntelligence()->IsPedClimbingLadder())
	{
		return false;
	}

	if (m_pPed->GetIsVisibleInSomeViewportThisFrame() == false)
	{
		return false;
	}

	crIKSolverBase* pSolver = GetOrAddSolver(solverType);

	if (pSolver)
	{
		if (solverType == IKManagerSolverTypes::ikSolverTypeTorsoReact)
		{
			CTorsoReactIkSolver* pReactSolver = static_cast<CTorsoReactIkSolver*>(pSolver);
			pReactSolver->Request(request);
		}
#if IK_QUAD_REACT_SOLVER_ENABLE
		else if (solverType == IKManagerSolverTypes::ikSolverTypeQuadrupedReact)
		{
			CQuadrupedReactSolver* pReactSolver = static_cast<CQuadrupedReactSolver*>(pSolver);
			pReactSolver->Request(request);
		}
#endif // IK_QUAD_REACT_SOLVER_ENABLE

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CIkManager::Request(const CIkRequestBodyRecoil& request)
{
	if (!m_pPed->GetCapsuleInfo()->IsBiped())
	{
		return false;
	}

	if (!IsSupported(IKManagerSolverTypes::ikSolverTypeBodyRecoil))
	{
		return false;
	}

	if (!m_pPed->GetIsVisibleInSomeViewportThisFrame())
	{
		return false;
	}

	if (IsGreaterThanAll(MagSquared(Subtract(VECTOR3_TO_VEC3V(camInterface::GetPos()), m_pPed->GetTransform().GetPosition())), ScalarV(square(50.0f))))
	{
		return false;
	}

	CBodyRecoilIkSolver* pSolver = static_cast<CBodyRecoilIkSolver*>(GetOrAddSolver(ikSolverTypeBodyRecoil));
	if (pSolver)
	{
		pSolver->Request(request);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CIkManager::Request(const CIkRequestLeg& request FPS_MODE_SUPPORTED_ONLY(, s32 pass))
{

#if FPS_MODE_SUPPORTED
	bool bFirstPersonRequestAdded = false;
	if ((pass&IK_PASS_FPS)!=0)
	{
		// lazy init the request structure if need be:
		FirstPersonPassRequests& requests = GetFpsRequests();

		requests.m_legRequest = request;
		requests.m_hasRequests[kRequestLeg] = true;
		bFirstPersonRequestAdded = true;
	}

	if ((pass&IK_PASS_MAIN)==0)
	{
		return bFirstPersonRequestAdded;
	}
#endif // FPS_MODE_SUPPORTED


	CIkRequestLeg solverRequest = request;

	bool bCanUsePelvis = true;
	bool bCanUseFade = true;

	eIkSolverType solverType = IKManagerSolverTypes::ikSolverTypeLeg;

#if IK_QUAD_LEG_SOLVER_ENABLE
	const CBaseCapsuleInfo* pCapsuleInfo = m_pPed->GetCapsuleInfo();
	if (pCapsuleInfo && pCapsuleInfo->IsQuadruped())
	{
		// Default is low LOD mode unless the local player is the rider
		const CPed* pRider = m_pPed->GetHorseComponent() ? m_pPed->GetHorseComponent()->GetRider() : NULL;

		if (pRider && pRider->IsLocalPlayer())
		{
			solverRequest.SetMode(LEG_IK_MODE_FULL);
		}

		solverType = IKManagerSolverTypes::ikSolverTypeQuadLeg;
	}
#endif // IK_QUAD_LEG_SOLVER_ENABLE

	if (!IsSupported(solverType))
	{
		return false;
	}

	if (!m_pPed->IsLocalPlayer() && !m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_IgnoreLegIkRestrictions))
	{
		if (m_pPed->GetPedAiLod().IsLodFlagSet(CPedAILod::AL_LodMotionTask))
		{
			return false;
		}

		if (!m_pPed->GetIsVisibleInSomeViewportThisFrame())
		{
			return false;
		}

		if (IsGreaterThanAll(MagSquared(Subtract(VECTOR3_TO_VEC3V(camInterface::GetPos()), m_pPed->GetTransform().GetPosition())), ScalarV(square(25.0f))))
		{
			return false;
		}
	}

	if (!CanUseLegIK(bCanUsePelvis, bCanUseFade))
	{
		return false;
	}

	if (IsFlagSet(PEDIK_LEGS_USE_ANIM_BLOCK_TAGS | PEDIK_LEGS_USE_ANIM_ALLOW_TAGS))
	{
		fwAnimDirector* pAnimDirector = m_pPed->GetAnimDirector();

		if (pAnimDirector)
		{
			const CClipEventTags::CLegsIKEventTag* pProp = static_cast<const CClipEventTags::CLegsIKEventTag*>(pAnimDirector->FindPropertyInMoveNetworks(CClipEventTags::LegsIk));

			if (IsFlagSet(PEDIK_LEGS_USE_ANIM_BLOCK_TAGS) && pProp && !pProp->GetAllowed())
			{
				return false;
			}

			if (IsFlagSet(PEDIK_LEGS_USE_ANIM_ALLOW_TAGS) && ((pProp == NULL) || !pProp->GetAllowed()))
			{
				return false;
			}
		}
	}

	crIKSolverBase* pSolver = GetOrAddSolver(solverType);

	if (pSolver)
	{
		if (solverType == IKManagerSolverTypes::ikSolverTypeLeg)
		{
			CLegIkSolverProxy* pLegSolver = static_cast<CLegIkSolverProxy*>(pSolver);
			pLegSolver->Request(solverRequest);
		}
#if IK_QUAD_LEG_SOLVER_ENABLE
		else if (solverType == IKManagerSolverTypes::ikSolverTypeQuadLeg)
		{
			CQuadLegIkSolverProxy* pQuadLegSolver = static_cast<CQuadLegIkSolverProxy*>(pSolver);
			pQuadLegSolver->Request(solverRequest);
		}
#endif // IK_QUAD_LEG_SOLVER_ENABLE

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CIkManager::Request(const CIkRequestTorsoVehicle& request)
{
	if (!m_pPed->GetCapsuleInfo()->IsBiped())
	{
		return false;
	}

	if (!IsSupported(IKManagerSolverTypes::ikSolverTypeTorsoVehicle))
	{
		return false;
	}

	if (!m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsEnteringVehicle))
	{
		fwAttachmentEntityExtension* pExtension = m_pPed->GetAttachmentExtension();
		if ((pExtension == NULL) || (pExtension->GetAttachParentForced() == NULL))
		{
			return false;
		}
	}

	if (m_pPed->GetMovePed().GetBlendHelper().IsFlagSet(APF_BLOCK_IK) || m_pPed->GetMovePed().GetSecondaryBlendHelper().IsFlagSet(APF_BLOCK_IK))
	{
		return false;
	}

	if (m_pPed->GetIsVisibleInSomeViewportThisFrame() == false)
	{
		return false;
	}

	CTorsoVehicleIkSolverProxy* pSolver = static_cast<CTorsoVehicleIkSolverProxy*>(GetOrAddSolver(ikSolverTypeTorsoVehicle));

	if (pSolver)
	{
		pSolver->Request(request);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::AddRootSlopeFixup(float fBlendRate)
{
	if (m_pPed->GetIKSettings().IsIKFlagDisabled(ikSolverTypeRootSlopeFixup))
	{
		return;
	}

	CRootSlopeFixupIkSolver* pSolver = static_cast<CRootSlopeFixupIkSolver*>(GetOrAddSolver(ikSolverTypeRootSlopeFixup));
	if (pSolver)
	{
		pSolver->AddFixup(fBlendRate);
	}
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::OrientateBoneATowardBoneB(s32 boneAIdx, s32 boneBIdx)
{
	crSkeleton* pSkel = m_pPed->GetSkeleton();
	if (ikVerifyf(pSkel, "No valid skeleton!"))
	{
		// Orientate (the x-axis of) matrix A toward the position of matrix b
		Matrix34 MA;
		pSkel->GetGlobalMtx(boneAIdx, RC_MAT34V(MA));
		Matrix34 MB;
		pSkel->GetGlobalMtx(boneBIdx, RC_MAT34V(MB));

		Vector3 dir= MB.d-MA.d;
		ikAssert(dir.Mag()>0.0f);
		dir.NormalizeSafe();

		Matrix34 res;
		res.a=dir;
		res.c=MA.c;
		res.b.Cross(res.c,res.a);
		res.c.Cross(res.a,res.b);
		res.Normalize();
		MA.a=res.a;
		MA.b=res.b;
		MA.c=res.c;
		ikAssert(rage::FPIsFinite(MA.a.x));
		ikAssert(rage::FPIsFinite(MA.b.y));
		ikAssert(rage::FPIsFinite(MA.c.z));

		pSkel->SetGlobalMtx(boneAIdx, RC_MAT34V(MA));
	}

}

//////////////////////////////////////////////////////////////////////////

void CIkManager::SetOverrideLegIKBlendTime(float fTime)
{
	if (HasSolver(ikSolverTypeLeg))
	{
		static_cast<CLegIkSolverProxy*>(GetSolver(ikSolverTypeLeg))->OverrideBlendTime(fTime);
	}
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::ResetOverrideLegIKBlendTime()
{
	if (HasSolver(ikSolverTypeLeg))
	{
		static_cast<CLegIkSolverProxy*>(GetSolver(ikSolverTypeLeg))->OverrideBlendTime(0.0f);
	}
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::SetIkTarget(eIkPart ePart, const CEntity* pTargetEntity, eAnimBoneTag eTargetBoneTag, const Vector3& vTargetOffset, u32 uFlags, int iBlendInTimeMS, int iBlendOutTimeMS, int iPriority)
{
	switch (ePart)
	{
		case IK_PART_HEAD:
		{
			CIkRequestBodyLook request(pTargetEntity, eTargetBoneTag, RCC_VEC3V(vTargetOffset), uFlags, iPriority);

			// Enable neck component by default to go along with head and eyes
			request.SetRotLimNeck(LOOKIK_ROT_ANGLE_YAW, LOOKIK_ROT_LIM_WIDE);
			request.SetRotLimNeck(LOOKIK_ROT_ANGLE_PITCH, LOOKIK_ROT_LIM_NARROW);

			// Use facing direction mode
			request.SetRefDirHead(LOOKIK_REF_DIR_FACING);
			request.SetRefDirNeck(LOOKIK_REF_DIR_FACING);

			Request(request);
			break;
		}
		case IK_PART_ARM_LEFT:
		case IK_PART_ARM_RIGHT:
		{
			const CDynamicEntity* pDynamicEntity = dynamic_cast<const CDynamicEntity*>(pTargetEntity);
			int iTargetBoneIdx = -1;

			if (pDynamicEntity && (eTargetBoneTag != BONETAG_INVALID))
			{
				crSkeleton* pSkeleton = pDynamicEntity->GetSkeleton();

				if (pSkeleton)
				{
					pSkeleton->GetSkeletonData().ConvertBoneIdToIndex((u16)eTargetBoneTag, iTargetBoneIdx);
				}
			}

			SetRelativeArmIKTarget((ePart == IK_PART_ARM_LEFT) ? crIKSolverArms::LEFT_ARM : crIKSolverArms::RIGHT_ARM, 
								   pDynamicEntity,
								   iTargetBoneIdx,
								   vTargetOffset,
								   (s32)uFlags,
								   (iBlendInTimeMS >= 0) ? (float)iBlendInTimeMS / 1000.0f : PEDIK_ARMS_FADEIN_TIME,
								   (iBlendOutTimeMS >= 0) ? (float)iBlendOutTimeMS / 1000.0f : PEDIK_ARMS_FADEOUT_TIME);
			break;
		}
		case IK_PART_SPINE:
		case IK_PART_LEG_LEFT:
		case IK_PART_LEG_RIGHT:
		{
			ikAssertf(false, "CIkManager::SetIkTarget - Unsupported IK part");
			break;
		}
		case IK_PART_INVALID:
		default:
		{
			ikAssertf(false, "CIkManager::SetIkTarget - Invalid IK part");
		}
	};
}

//////////////////////////////////////////////////////////////////////////

float CIkManager::GetSpringStrengthForStairs()
{
	if (HasSolver(ikSolverTypeLeg))
	{
		CLegIkSolver* pSolver = static_cast<CLegIkSolver*>(static_cast<CLegIkSolverProxy*>(GetSolver(ikSolverTypeLeg))->GetSolver());

		if (pSolver)
		{
			return pSolver->GetSpringStrengthForStairs();
		}
	}

	return 1.0f;
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::InitHighHeelData()
{
	// Offsets taken from high heel expression. Needs to be standardized into a meta value.
	const float kfHighHeelOffset = 0.0864783f;

	m_HighHeelData.m_fHeight = kfHighHeelOffset;
	m_HighHeelData.m_fPreExpressionHeight = 0.0f;
	m_HighHeelData.m_RootBoneIdx = -1;
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::BeginHighHeelData(float deltaTime)
{
	if (ms_bSampleHighHeelHeight && m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_HasHighHeels) && !m_pPed->GetUsingRagdoll() && (deltaTime > 0.0f))
	{
		crSkeleton* pSkeleton = m_pPed->GetSkeleton();

		if (pSkeleton)
		{
			if (m_HighHeelData.m_RootBoneIdx == -1)
			{
				pSkeleton->GetSkeletonData().ConvertBoneIdToIndex((u16)BONETAG_ROOT, m_HighHeelData.m_RootBoneIdx);
			}

			if (m_HighHeelData.m_RootBoneIdx >= 0)
			{
				m_HighHeelData.m_fPreExpressionHeight = pSkeleton->GetObjectMtx(m_HighHeelData.m_RootBoneIdx).GetCol3().GetZf();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::EndHighHeelData(float deltaTime)
{
	if (ms_bSampleHighHeelHeight && m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_HasHighHeels) && !m_pPed->GetUsingRagdoll() && (deltaTime > 0.0f))
	{
		crSkeleton* pSkeleton = m_pPed->GetSkeleton();

		if (pSkeleton)
		{
			if (m_HighHeelData.m_RootBoneIdx >= 0)
			{
				float fPostExpressionHeight = pSkeleton->GetObjectMtx(m_HighHeelData.m_RootBoneIdx).GetCol3().GetZf();

				// Remove leg solver contribution
				if (HasSolver(ikSolverTypeLeg))
				{
					const CLegIkSolver* pLegSolver = static_cast<const CLegIkSolver*>(static_cast<const CLegIkSolverProxy*>(GetSolver(ikSolverTypeLeg))->GetSolver());

					if (pLegSolver)
					{
						fPostExpressionHeight -= pLegSolver->GetPelvisDeltaZForCamera(true);
					}
				}

				fPostExpressionHeight -= m_HighHeelData.m_fPreExpressionHeight;

				const float fHighHeelBlend = m_pPed->GetHighHeelExpressionDOF();
				const float kfTolerance = 0.9999f;

				if (fHighHeelBlend >= kfTolerance)
				{
					fPostExpressionHeight /= fHighHeelBlend;

					TUNE_GROUP_FLOAT(LEG_IK, HighHeelOffsetBlendRate, 8.0f, 0.0f, 20.0f, 0.01f);
					m_HighHeelData.m_fHeight = Lerp(Min(HighHeelOffsetBlendRate * deltaTime, 1.0f), m_HighHeelData.m_fHeight, fabsf(fPostExpressionHeight));
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#if __IK_DEV
void CIkManager::DebugRenderPed()
{
	if (CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}

	for(u32 i=0; i<ikSolverTypeCount; i++)
	{
		crIKSolverBase* solver = GetSolver((eIkSolverType)i);
		if (solver && !solver->IsDead() && !CBaseIkManager::ms_DisableSolvers[i] )
		{
			solver->DebugRender();
		}
	}
}
#endif //__IK_DEV

///////////////////////////////////////////////////////////////////////////////
#if FPS_MODE_SUPPORTED
CIkManager::FirstPersonPassRequests& CIkManager::GetFpsRequests()
{
	if (!m_pFirstPersonRequests) 
	{
		m_pFirstPersonRequests = rage_new FirstPersonPassRequests(); 
	}
	ikAssertf(m_pFirstPersonRequests, "Failed to allocate Fps IK request structure! Is the game out of memory?");
	return *m_pFirstPersonRequests;
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::CacheInputFrameForFpsUpdatePass(CPed& ped)
{
	crSkeleton* pSkel = ped.GetSkeleton();
	if (pSkel)
	{
		if (!m_pThirdPersonInputFrame)
		{
			m_pThirdPersonInputFrame = ped.GetAnimDirector()->GetFrameBuffer().AllocateFrame();
		}
		
		m_pThirdPersonInputFrame->InversePose(*pSkel);

		crCreature *pCreature = ped.GetCreature();
		if(pCreature)
		{
			const crCreatureComponentExtraDofs *pExtraDofs = pCreature->FindComponent<crCreatureComponentExtraDofs>();
			if(pExtraDofs)
			{
				m_pThirdPersonInputFrame->Set(pExtraDofs->GetPoseFrame());
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::PerformFpsUpdatePass(CPed& ped)
{
	crSkeleton* pSkel = ped.GetSkeleton();
	if (pSkel)
	{
		m_bFPSUpdatePass = true;

		ped.GetAnimDirector()->WaitForPreRenderUpdateToComplete();
		PostIkUpdate(0.0f);

		// store the matrix set from the third person update.
		if (!m_pThirdPersonSkeleton)
		{
			m_pThirdPersonSkeleton = rage_new crSkeleton();
			m_pThirdPersonSkeleton->Init(pSkel->GetSkeletonData());
			m_pThirdPersonSkeleton->SetParentMtx(&ped.GetMatrixRef()); // ???
		}

		TUNE_GROUP_BOOL(FPS_CAM_RELATIVE, bUpdatePedAttachmentsThirdPerson, false);

		if (bUpdatePedAttachmentsThirdPerson && ped.GetExtension<fwCameraRelativeExtension>() && ped.GetAttachmentExtension())
		{
			// if we're running a camera relative extension
			// process any attachments now and remove the child
			// attachments from the late attachment update
			CGameWorld::ProcessDelayedAttachmentsForPhysical(&ped);
		}

		m_pThirdPersonSkeleton->CopyFrom(*pSkel);

		// restore the values from before the first pre-render update.
		if (m_pThirdPersonInputFrame)
		{
			m_pThirdPersonInputFrame->Pose(*pSkel);
			crCreature *pCreature = ped.GetCreature();
			if(pCreature)
			{
				crCreatureComponentExtraDofs *pExtraDofs = pCreature->FindComponent<crCreatureComponentExtraDofs>();
				if(pExtraDofs)
				{
					pExtraDofs->GetPoseFrame().Set(*m_pThirdPersonInputFrame);
				}
			}
		}

		// Update the skeleton
		pSkel->Update();

		// apply the cached requests
		FirstPersonPassRequests& requests = GetFpsRequests();

		if (requests.m_hasRequests[kRequestArmLeft]) Request(requests.m_armRequests[IK_ARM_LEFT], IK_PASS_MAIN);
		if (requests.m_hasRequests[kRequestArmRight]) Request(requests.m_armRequests[IK_ARM_RIGHT], IK_PASS_MAIN);
		if (requests.m_hasRequests[kRequestLeg]) Request(requests.m_legRequest, IK_PASS_MAIN);

		if (requests.m_hasRequests[kRequestLook] && (!requests.m_bLookTargetParam || (requests.m_pLookTarget != NULL)))
		{
			Request(requests.m_lookRequest, IK_PASS_MAIN);
		}

		// reset the requests for next frame
		requests.Reset();

		// run the pre-render update again, blocking on the result.
		PreIkUpdate(0.0f);

		camInterface::ApplyPedBoneOverridesForFirstPerson(ped, *pSkel);

		if (ped.GetAnimDirector()->IsUpdated())
		{
			ped.GetAnimDirector()->StartUpdatePreRender(0.0f, NULL, false);	
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CIkManager::FirstPersonPassRequests::Reset()
{
	for (s32 i=0; i<kNumRequests; i++)
	{
		m_hasRequests[i] = false;
	}

	m_pLookTarget = NULL;
	m_bLookTargetParam = false;
}

#endif // FPS_MODE_SUPPORTED

