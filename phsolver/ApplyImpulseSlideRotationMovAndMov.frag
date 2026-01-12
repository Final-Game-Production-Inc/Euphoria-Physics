//  
// phsolver/ApplyImpulseSlideRotationMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsesliderotationmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsesliderotationmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsesliderotationmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseSlideRotationMovAndMov(manifold, globals);
}
 
