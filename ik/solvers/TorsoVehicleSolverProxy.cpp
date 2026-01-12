// 
// ik/solvers/TorsoVehicleSolverProxy.cpp
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#include "TorsoVehicleSolverProxy.h"

#include "ik/solvers/TorsoVehicleSolver.h"
#include "Peds/ped.h"

#define TORSOVEHICLE_SOLVER_PROXY_POOL_MAX	32

FW_INSTANTIATE_CLASS_POOL_SPILLOVER(CTorsoVehicleIkSolverProxy, TORSOVEHICLE_SOLVER_PROXY_POOL_MAX, 0.68f, atHashString("CTorsoVehicleIkSolverProxy",0x8cae847e));

ANIM_OPTIMISATIONS();

CTorsoVehicleIkSolverProxy::CTorsoVehicleIkSolverProxy()
: CIkSolverProxy()
, m_bActive(false)
{
	SetPrority(5);
}

CTorsoVehicleIkSolverProxy::~CTorsoVehicleIkSolverProxy()
{
}

void CTorsoVehicleIkSolverProxy::Reset()
{
	// Forward to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CTorsoVehicleIkSolver*>(m_pSolver)->Reset();
	}

	m_bActive = false;
}

void CTorsoVehicleIkSolverProxy::Iterate(float UNUSED_PARAM(dt), crSkeleton& UNUSED_PARAM(skeleton))
{
}

void CTorsoVehicleIkSolverProxy::PreIkUpdate(float deltaTime)
{
	// Forward the update to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CTorsoVehicleIkSolver*>(m_pSolver)->PreIkUpdate(deltaTime);
	}
}

void CTorsoVehicleIkSolverProxy::PostIkUpdate(float deltaTime)
{
	// Forward the update to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CTorsoVehicleIkSolver*>(m_pSolver)->PostIkUpdate(deltaTime);
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

bool CTorsoVehicleIkSolverProxy::AllocateSolver(CPed* pPed)
{
	ikAssert(!IsBlocked());

	if (m_pSolver == NULL)
	{
		if (CTorsoVehicleIkSolver::GetPool()->GetNoOfFreeSpaces() != 0)
		{
			m_pSolver = rage_new CTorsoVehicleIkSolver(pPed, this);

			// Forward stored request
			ikAssert(m_pSolver);
			static_cast<CTorsoVehicleIkSolver*>(m_pSolver)->Request(m_Request);

			return true;
		}
	}

	return false;
}

void CTorsoVehicleIkSolverProxy::Request(const CIkRequestTorsoVehicle& request)
{
	// Forward the request to the real solver if allocated
	if (m_pSolver)
	{
		static_cast<CTorsoVehicleIkSolver*>(m_pSolver)->Request(request);
	}

	// Store the request regardless
	m_Request = request;

	m_bActive = true;
}

#if __IK_DEV
void CTorsoVehicleIkSolverProxy::DebugRender()
{
	if (m_pSolver)
	{
		m_pSolver->DebugRender();
	}
}
#endif //__IK_DEV
