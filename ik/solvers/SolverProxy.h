// 
// ik/solvers/SolverProxy.h
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#ifndef SOLVERPROXY_H
#define SOLVERPROXY_H

#include "crbody/iksolverbase.h"

class CPed;

class CIkSolverProxy : public crIKSolverBase
{
public:
	CIkSolverProxy();
	virtual ~CIkSolverProxy();

	virtual void Iterate(float, crSkeleton&)	{}
	virtual bool IsDead() const					{ return m_bComplete != 0; }

	virtual bool AllocateSolver(CPed*)			{ return false; }
	virtual bool FreeSolver();

	crIKSolverBase* GetSolver() const			{ return m_pSolver; }
	bool IsRunning() const						{ return m_pSolver != NULL; }

	void SetComplete(bool bComplete)			{ m_bComplete = (u8)bComplete; }

	void SetBlocked(bool bBlocked)				{ m_bBlocked = (u8)bBlocked; }
	bool IsBlocked() const						{ return m_bBlocked != 0; }

protected:
	crIKSolverBase* m_pSolver;

	u8 m_bBlocked;
	u8 m_bComplete;
};

#endif // SOLVERPROXY_H
