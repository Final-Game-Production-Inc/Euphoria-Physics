// 
// ik/solvers/LegSolverProxy.h
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#ifndef LEGSOLVERPROXY_H
#define LEGSOLVERPROXY_H

#include "SolverProxy.h"

#include "fwtl/pool.h"
#include "ik/IkRequest.h"

class CPed;

class CLegIkSolverProxy : public CIkSolverProxy
{
	friend class CLegIkSolver;

public:
	CLegIkSolverProxy();
	~CLegIkSolverProxy();

	FW_REGISTER_CLASS_POOL(CLegIkSolverProxy);

	void Reset();
	void Iterate(float dt, crSkeleton& skeleton);

	void PreIkUpdate(float deltaTime);
	void PostIkUpdate(float deltaTime);

	bool AllocateSolver(CPed* pPed);

	void Request(const CIkRequestLeg& request);

	void OverrideBlendTime(float fBlendTime)	{ m_fBlendTime = fBlendTime; }
	float GetOverriddenBlendTime() const		{ return m_fBlendTime; }

#if __IK_DEV
	void DebugRender();
#endif //__IK_DEV

private:
	CIkRequestLeg	m_Request;
	float			m_fBlendTime;
	bool			m_bActive;
};

#endif // LEGSOLVERPROXY_H
