// 
// ik/solvers/QuadLegSolverProxy.h
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#ifndef QUADLEGSOLVERPROXY_H
#define QUADLEGSOLVERPROXY_H

#include "ik/IkConfig.h"

#if IK_QUAD_LEG_SOLVER_ENABLE

#include "SolverProxy.h"

#include "fwtl/pool.h"
#include "ik/IkConfig.h"
#include "ik/IkRequest.h"

#define QUAD_LEG_SOLVER_PROXY_POOL_MAX 16

class CPed;

class CQuadLegIkSolverProxy : public CIkSolverProxy
{
	friend class CQuadLegIkSolver;

public:
	CQuadLegIkSolverProxy();
	~CQuadLegIkSolverProxy();

#if IK_QUAD_LEG_SOLVER_USE_POOL
	FW_REGISTER_CLASS_POOL(CQuadLegIkSolverProxy);
#endif // IK_QUAD_LEG_SOLVER_USE_POOL

	void Reset();
	void Iterate(crSkeleton& skeleton);

	void PreIkUpdate(float deltaTime);
	void PostIkUpdate();

	bool AllocateSolver(CPed* pPed);

	void Request(const CIkRequestLeg& request);

#if __IK_DEV
	void DebugRender();
#endif //__IK_DEV

#if !IK_QUAD_LEG_SOLVER_USE_POOL
	static s8			ms_NoOfFreeSpaces;
#endif // IK_QUAD_LEG_SOLVER_USE_POOL

private:
	CIkRequestLeg		m_Request;
	bool				m_bActive;
};

#endif // IK_QUAD_LEG_SOLVER_ENABLE

#endif // QUADLEGSOLVERPROXY_H
