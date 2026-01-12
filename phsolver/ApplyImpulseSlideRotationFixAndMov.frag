//  
// phsolver/ApplyImpulseSlideRotationFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsesliderotationfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsesliderotationfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsesliderotationfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseSlideRotationFixAndMov(manifold, globals);
}
 
