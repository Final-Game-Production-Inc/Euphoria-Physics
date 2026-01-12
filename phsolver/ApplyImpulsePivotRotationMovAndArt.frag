//  
// phsolver/ApplyImpulsePivotRotationMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsePivotrotationmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsePivotrotationmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsePivotrotationmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulsePivotRotationMovAndArt(manifold, globals);
}
 
