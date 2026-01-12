// 
// ik/solvers/LegSolverProxy.cpp
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#include "LegSolverProxy.h"

#include "ik/solvers/LegSolver.h"
#include "Peds/ped.h"

#define LEG_SOLVER_PROXY_POOL_MAX 40

FW_INSTANTIATE_CLASS_POOL(CLegIkSolverProxy, LEG_SOLVER_PROXY_POOL_MAX, atHashString("CLegIkSolverProxy",0xd64a10b7));

ANIM_OPTIMISATIONS();

CLegIkSolverProxy::CLegIkSolverProxy()
: CIkSolverProxy()
, m_fBlendTime(0.0f)
, m_bActive(false)
{
	SetPrority(6);
}

CLegIkSolverProxy::~CLegIkSolverProxy()
{
}

void CLegIkSolverProxy::Reset()
{
	// Forward to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CLegIkSolver*>(m_pSolver)->Reset();
	}

	m_fBlendTime = 0.0f;
	m_bActive = false;
}

void CLegIkSolverProxy::Iterate(float UNUSED_PARAM(dt), crSkeleton& UNUSED_PARAM(skeleton))
{
}

void CLegIkSolverProxy::PreIkUpdate(float deltaTime)
{
	// Forward the update to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CLegIkSolver*>(m_pSolver)->PreIkUpdate(deltaTime);
	}
}

void CLegIkSolverProxy::PostIkUpdate(float deltaTime)
{
	// Forward the update to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CLegIkSolver*>(m_pSolver)->PostIkUpdate(deltaTime);
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

bool CLegIkSolverProxy::AllocateSolver(CPed* pPed)
{
	ikAssert(!IsBlocked());

	if (m_pSolver == NULL)
	{
		if (CLegIkSolver::GetPool()->GetNoOfFreeSpaces() != 0)
		{
			m_pSolver = rage_new CLegIkSolver(pPed, this);

			// Forward stored request
			ikAssert(m_pSolver);
			static_cast<CLegIkSolver*>(m_pSolver)->Request(m_Request);

			return true;
		}
	}

	return false;
}

void CLegIkSolverProxy::Request(const CIkRequestLeg& request)
{
	// Forward the request to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CLegIkSolver*>(m_pSolver)->Request(request);
	}

	// Store the request regardless
	m_Request = request;

	m_bActive = true;
}

#if __IK_DEV
void CLegIkSolverProxy::DebugRender()
{
	if (m_pSolver)
	{
		m_pSolver->DebugRender();
	}
}
#endif //__IK_DEV
