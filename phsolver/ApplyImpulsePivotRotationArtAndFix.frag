//  
// phsolver/ApplyImpulsePivotRotationArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsePivotrotationartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsePivotrotationartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsePivotrotationartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulsePivotRotationArtAndFix(manifold, globals);
}
 
