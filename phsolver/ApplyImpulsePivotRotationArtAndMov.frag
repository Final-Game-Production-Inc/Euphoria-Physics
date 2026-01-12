//  
// phsolver/ApplyImpulsePivotRotationArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsePivotrotationartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsePivotrotationartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsePivotrotationartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulsePivotRotationArtAndMov(manifold, globals);
}
 
