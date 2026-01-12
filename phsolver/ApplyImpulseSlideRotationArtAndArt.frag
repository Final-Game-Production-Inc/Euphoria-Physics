//  
// phsolver/ApplyImpulseSlideRotationArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsesliderotationartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsesliderotationartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsesliderotationartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseSlideRotationArtAndArt(manifold, globals);
}
 
