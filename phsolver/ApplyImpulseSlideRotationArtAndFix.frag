//  
// phsolver/ApplyImpulseSlideRotationArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsesliderotationartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsesliderotationartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsesliderotationartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseSlideRotationArtAndFix(manifold, globals);
}
 
