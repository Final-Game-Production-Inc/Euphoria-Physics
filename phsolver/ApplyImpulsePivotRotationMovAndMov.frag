//  
// phsolver/ApplyImpulsePivotRotationMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsePivotrotationmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsePivotrotationmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsePivotrotationmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulsePivotRotationMovAndMov(manifold, globals);
}
 
