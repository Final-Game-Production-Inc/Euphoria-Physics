//  
// phsolver/ApplyImpulseSlideRotationMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsesliderotationmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsesliderotationmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsesliderotationmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseSlideRotationMovAndFix(manifold, globals);
}
 
