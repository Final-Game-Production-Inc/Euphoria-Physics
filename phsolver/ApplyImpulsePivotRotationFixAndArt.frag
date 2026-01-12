//  
// phsolver/ApplyImpulsePivotRotationFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsePivotrotationfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsePivotrotationfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsePivotrotationfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulsePivotRotationFixAndArt(manifold, globals);
}
 
