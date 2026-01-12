//  
// phsolver/ApplyImpulsePivotRotationArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsePivotrotationartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsePivotrotationartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsePivotrotationartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulsePivotRotationArtAndArt(manifold, globals);
}
 
