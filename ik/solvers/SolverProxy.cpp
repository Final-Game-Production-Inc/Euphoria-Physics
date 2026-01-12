// 
// ik/solvers/SolverProxy.cpp
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#include "SolverProxy.h"

CIkSolverProxy::CIkSolverProxy()
: crIKSolverBase(IK_SOLVER_TYPE_PROXY)
, m_pSolver(NULL)
, m_bBlocked(1)
, m_bComplete(0)
{
}

CIkSolverProxy::~CIkSolverProxy()
{
	CIkSolverProxy::FreeSolver();
}

bool CIkSolverProxy::FreeSolver()
{
	if (m_pSolver)
	{
		delete m_pSolver;
		m_pSolver = NULL;

		return true;
	}

	return false;
}
