//  
// phsolver/ApplyImpulseSlideRotationArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsesliderotationartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsesliderotationartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsesliderotationartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseSlideRotationArtAndMov(manifold, globals);
}
 
