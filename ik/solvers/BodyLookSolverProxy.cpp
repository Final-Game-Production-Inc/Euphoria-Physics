// 
// ik/solvers/BodyLookSolverProxy.cpp
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#include "BodyLookSolverProxy.h"

#include "ik/solvers/BodyLookSolver.h"
#include "camera/CamInterface.h"
#include "Peds/ped.h"

#define BODYLOOK_SOLVER_PROXY_POOL_MAX	40

FW_INSTANTIATE_CLASS_POOL_SPILLOVER(CBodyLookIkSolverProxy, BODYLOOK_SOLVER_PROXY_POOL_MAX, 0.53f, atHashString("CBodyLookIkSolverProxy",0xd866a500));

ANIM_OPTIMISATIONS();

CBodyLookIkSolverProxy::CBodyLookIkSolverProxy(CPed* pPed)
: CIkSolverProxy()
, m_vTargetPosition(V_ZERO_WONE)
, m_pPed(pPed)
, m_uCurrTime(0)
, m_bActive(false)
, m_bAbort(false)
, m_bTargetEntityValid(false)
{
	SetPrority(8);
}

CBodyLookIkSolverProxy::~CBodyLookIkSolverProxy()
{
}

void CBodyLookIkSolverProxy::Reset()
{
	m_vTargetPosition.ZeroComponents();
	m_uCurrTime = 0;

	m_bActive = false;
	m_bAbort = false;

	if (m_pSolver)
	{
		static_cast<CBodyLookIkSolver*>(m_pSolver)->Reset();
	}
}

void CBodyLookIkSolverProxy::Iterate(float UNUSED_PARAM(dt), crSkeleton& UNUSED_PARAM(skeleton))
{
}

void CBodyLookIkSolverProxy::PreIkUpdate(float deltaTime)
{
	// Forward the update to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CBodyLookIkSolver*>(m_pSolver)->PreIkUpdate(deltaTime);
	}
}

void CBodyLookIkSolverProxy::PostIkUpdate(float deltaTime)
{
	UpdateTargetPosition();

	// Time
	const u16 uHoldTime = GetHoldTime();
	if ((uHoldTime > 0) && (uHoldTime != (u16)-1))
	{
		m_uCurrTime += (u16)(deltaTime * 1000.0f);
		m_uCurrTime = Min(m_uCurrTime, uHoldTime);
	}

	// Forward the update to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CBodyLookIkSolver*>(m_pSolver)->PostIkUpdate(deltaTime);
	}
	else
	{
		UpdateActiveState();

		fwAnimDirector* pAnimDirector = m_pPed->GetAnimDirector();
		if (pAnimDirector && ((GetFlags() & LOOKIK_BLOCK_ANIM_TAGS) == 0))
		{
			// Check for block tag
			const CClipEventTags::CLookIKControlEventTag* pPropControl = static_cast<const CClipEventTags::CLookIKControlEventTag*>(pAnimDirector->FindPropertyInMoveNetworks(CClipEventTags::LookIkControl));

			if (pPropControl && !pPropControl->GetAllowed())
			{
				IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_BLOCKED_BY_TAG, "CBodyLookIkSolverProxy::PostIkUpdate");
				m_bActive = false;
			}
		}

		if ((!m_bActive && (uHoldTime != (u16)-1)) || IsAborted())
		{
			SetComplete(true);
		}
	}

	if (uHoldTime == 0)
	{
		m_bActive = false;
	}
}

bool CBodyLookIkSolverProxy::AllocateSolver(CPed* pPed)
{
	ikAssert(!IsBlocked());
	ikAssert(pPed == m_pPed);

	if (m_pSolver == NULL)
	{
		if (CBodyLookIkSolver::GetPool()->GetNoOfFreeSpaces() != 0)
		{
			m_pSolver = rage_new CBodyLookIkSolver(pPed, this);

			// Forward stored request
			ikAssert(m_pSolver);
			static_cast<CBodyLookIkSolver*>(m_pSolver)->Request(m_Request);

			return true;
		}
	}

	return false;
}

void CBodyLookIkSolverProxy::Request(const CIkRequestBodyLook& request)
{
	// Store look target information
	m_pTargetEntity = request.GetTargetEntity();
	m_bTargetEntityValid = (m_pTargetEntity != NULL);
	ikAssertf(((u16)request.GetTargetBoneTag() == (u16)-1) || m_bTargetEntityValid, "Invalid target entity");

	m_uCurrTime = 0;

	// Forward the request to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CBodyLookIkSolver*>(m_pSolver)->Request(request);
	}

	// Store the request regardless
	m_Request = request;

	m_bActive = true;
	m_bAbort = false;
}

void CBodyLookIkSolverProxy::Abort()
{
	m_bAbort = true;

	// Clear hold time if current request is set to be infinite so that the solver 
	// will still time out after a frame if the abort fails to get processed
	if (GetHoldTime() == (u16)-1)
	{
		m_Request.SetHoldTime(0);
	}
}

u8 CBodyLookIkSolverProxy::GetTurnRate() const
{
	if (m_pSolver)
	{
		// Get turn rate directly from solver in case it has been overridden by a LookIk tag
		return static_cast<CBodyLookIkSolver*>(m_pSolver)->GetTurnRate();
	}

	return (u8)m_Request.GetTurnRate();
}

u8 CBodyLookIkSolverProxy::GetHeadRotLim(LookIkRotationAngle rotAngle) const
{
	if (m_pSolver)
	{
		// Get head rotation limit directly from solver in case it has been overridden by a LookIk tag
		return static_cast<CBodyLookIkSolver*>(m_pSolver)->GetHeadRotLim(rotAngle);
	}

	return (u8)m_Request.GetRotLimHead(rotAngle);
}

void CBodyLookIkSolverProxy::UpdateTargetPosition()
{
	if (GetFlags() & LOOKIK_USE_CAMERA_FOCUS)
	{
		m_vTargetPosition = AddScaled(RCC_VEC3V(camInterface::GetPos()), RCC_VEC3V(camInterface::GetFront()), ScalarV(V_TEN));
	}
	else
	{
		const Vec3V& vTargetOffset = m_Request.GetTargetOffset();

		if (m_bTargetEntityValid && m_pTargetEntity)
		{
			const crSkeleton* pSkeleton = m_pTargetEntity->GetSkeleton();

			if (pSkeleton && (m_Request.GetTargetBoneTag() != BONETAG_INVALID))
			{
				Mat34V mtxTargetBone(V_IDENTITY);

				s32 targetBoneIdx = -1;
				pSkeleton->GetSkeletonData().ConvertBoneIdToIndex((u16)m_Request.GetTargetBoneTag(), targetBoneIdx);

				if (targetBoneIdx >= 0)
				{
					pSkeleton->GetGlobalMtx(targetBoneIdx, mtxTargetBone);
				}

				m_vTargetPosition = Add(mtxTargetBone.GetCol3(), Transform3x3(mtxTargetBone, vTargetOffset));
			}
			else
			{
				// Target offset relative to target entity
				m_vTargetPosition = m_pTargetEntity->GetTransform().Transform(vTargetOffset);
			}
		}
		else
		{
			// Target offset is in world space
			m_vTargetPosition = vTargetOffset;
		}
	}
}

void CBodyLookIkSolverProxy::UpdateActiveState()
{
	const u16 uHoldTime = GetHoldTime();

	if (uHoldTime == (u16)-1)
	{
		// Always active until aborted
		m_bActive = true;
	}

	if (IsTargetEntityValid() && (GetTargetEntity() == NULL))
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_INVALID_TARGET, "CBodyLookIkSolverProxy::UpdateActiveState");
		m_bActive = false;
	}

	if (m_pPed->ShouldBeDead())
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_CHAR_DEAD, "CBodyLookIkSolverProxy::UpdateActiveState");
		m_bActive = false;
	}

	if (m_pPed->GetHeadIkBlocked())
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_BLOCKED, "CBodyLookIkSolverProxy::UpdateActiveState");
		m_bActive = false;
	}

	if (m_pPed->GetCodeHeadIkBlocked() && !(GetFlags() & LOOKIK_SCRIPT_REQUEST))
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_BLOCKED, "CBodyLookIkSolverProxy::UpdateActiveState");
		m_bActive = false;
	}

	if (m_pPed->GetMovePed().GetBlendHelper().IsFlagSet(APF_BLOCK_IK | APF_BLOCK_HEAD_IK))
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_BLOCKED_BY_ANIM, "CBodyLookIkSolverProxy::UpdateActiveState");
		m_bActive = false;
	}

	if ((uHoldTime > 0) && (uHoldTime != (u16)-1) && (m_uCurrTime >= uHoldTime))
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_TIMED_OUT, "CBodyLookIkSolverProxy::UpdateActiveState");
		m_bActive = false;
	}
}

#if __IK_DEV
void CBodyLookIkSolverProxy::DebugRender()
{
	if (m_pSolver)
	{
		m_pSolver->DebugRender();
	}
}
#endif //__IK_DEV
