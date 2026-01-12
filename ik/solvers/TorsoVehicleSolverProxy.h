// 
// ik/solvers/TorsoVehicleSolverProxy.h
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#ifndef TORSOVEHICLESOLVERPROXY_H
#define TORSOVEHICLESOLVERPROXY_H

#include "SolverProxy.h"

#include "fwtl/pool.h"
#include "ik/IkRequest.h"

class CPed;

class CTorsoVehicleIkSolverProxy : public CIkSolverProxy
{
	friend class CTorsoVehicleIkSolver;

public:
	CTorsoVehicleIkSolverProxy();
	~CTorsoVehicleIkSolverProxy();

	FW_REGISTER_CLASS_POOL(CTorsoVehicleIkSolverProxy);

	void Reset();
	void Iterate(float dt, crSkeleton& skeleton);

	void PreIkUpdate(float deltaTime);
	void PostIkUpdate(float deltaTime);

	bool AllocateSolver(CPed* pPed);

	void Request(const CIkRequestTorsoVehicle& request);

#if __IK_DEV
	void DebugRender();
#endif //__IK_DEV

private:
	CIkRequestTorsoVehicle	m_Request;
	bool					m_bActive;
};

#endif // TORSOVEHICLESOLVERPROXY_H
