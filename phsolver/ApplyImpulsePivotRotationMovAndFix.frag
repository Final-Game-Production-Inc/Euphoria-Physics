//  
// phsolver/ApplyImpulsePivotRotationMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsePivotrotationmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsePivotrotationmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsePivotrotationmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulsePivotRotationMovAndFix(manifold, globals);
}
 
