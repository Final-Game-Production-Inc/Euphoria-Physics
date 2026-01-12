//  
// phsolver/ApplyImpulsePivotRotationFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsePivotrotationfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsePivotrotationfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsePivotrotationfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulsePivotRotationFixAndMov(manifold, globals);
}
 
