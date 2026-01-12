// 
// ik/solvers/QuadLegSolverProxy.cpp
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#include "QuadLegSolverProxy.h"

#if IK_QUAD_LEG_SOLVER_ENABLE

#include "ik/solvers/QuadLegSolver.h"
#include "Peds/ped.h"

#if IK_QUAD_LEG_SOLVER_USE_POOL
FW_INSTANTIATE_CLASS_POOL(CQuadLegIkSolverProxy, QUAD_LEG_SOLVER_PROXY_POOL_MAX, atHashString("CQuadLegIkSolverProxy",0x68b0dc33));
#else
s8 CQuadLegIkSolverProxy::ms_NoOfFreeSpaces = QUAD_LEG_SOLVER_PROXY_POOL_MAX;
#endif // IK_QUAD_LEG_SOLVER_USE_POOL

CQuadLegIkSolverProxy::CQuadLegIkSolverProxy()
: CIkSolverProxy()
, m_bActive(false)
{
	SetPrority(6);
}

CQuadLegIkSolverProxy::~CQuadLegIkSolverProxy()
{
}

void CQuadLegIkSolverProxy::Reset()
{
	// Forward to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CQuadLegIkSolver*>(m_pSolver)->Reset();
	}

	m_bActive = false;
}

void CQuadLegIkSolverProxy::Iterate(crSkeleton& UNUSED_PARAM(skeleton))
{
}

void CQuadLegIkSolverProxy::PreIkUpdate(float deltaTime)
{
	// Forward the update to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CQuadLegIkSolver*>(m_pSolver)->PreIkUpdate(deltaTime);
	}
}

void CQuadLegIkSolverProxy::PostIkUpdate(float deltaTime)
{
	// Forward the update to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CQuadLegIkSolver*>(m_pSolver)->PostIkUpdate(deltaTime);
	}

	if (deltaTime > 0.0f)
	{
		if (!m_pSolver && !m_bActive)
		{
			SetComplete(true);
		}

		m_bActive = false;
	}
}

bool CQuadLegIkSolverProxy::AllocateSolver(CPed* pPed)
{
	ikAssert(!IsBlocked());

	if (m_pSolver == NULL)
	{
#if IK_QUAD_LEG_SOLVER_USE_POOL
		if (CQuadLegIkSolver::GetPool()->GetNoOfFreeSpaces() != 0)
#else
		if (CQuadLegIkSolver::ms_NoOfFreeSpaces > 0)
#endif // IK_QUAD_LEG_SOLVER_USE_POOL
		{
			m_pSolver = rage_new CQuadLegIkSolver(pPed, this);

			// Forward stored request
			ikAssert(m_pSolver);
			static_cast<CQuadLegIkSolver*>(m_pSolver)->Request(m_Request);

#if !IK_QUAD_LEG_SOLVER_USE_POOL
			if (m_pSolver)
			{
				CQuadLegIkSolver::ms_NoOfFreeSpaces--;
			}
#endif // !IK_QUAD_LEG_SOLVER_USE_POOL

			return true;
		}
	}

	return false;
}

void CQuadLegIkSolverProxy::Request(const CIkRequestLeg& request)
{
	// Forward the request to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CQuadLegIkSolver*>(m_pSolver)->Request(request);
	}

	// Store the request regardless
	m_Request = request;

	m_bActive = true;
}

#if __IK_DEV
void CQuadLegIkSolverProxy::DebugRender()
{
	if (m_pSolver)
	{
		m_pSolver->DebugRender();
	}
}
#endif //__IK_DEV

#endif // IK_QUAD_LEG_SOLVER_ENABLE
