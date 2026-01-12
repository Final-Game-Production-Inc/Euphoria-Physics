//  
// phsolver/ApplyImpulseFixedRotationMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedrotationmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedrotationmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedrotationmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedRotationMovAndArt(manifold, globals);
}
 
