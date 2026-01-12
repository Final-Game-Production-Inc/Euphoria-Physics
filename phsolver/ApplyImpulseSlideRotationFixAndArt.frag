//  
// phsolver/ApplyImpulseSlideRotationFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsesliderotationfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsesliderotationfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsesliderotationfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseSlideRotationFixAndArt(manifold, globals);
}
 
