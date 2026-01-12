//  
// phsolver/ApplyImpulseSlideRotationMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsesliderotationmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsesliderotationmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsesliderotationmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseSlideRotationMovAndArt(manifold, globals);
}
 
