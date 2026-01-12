//  
// phsolver/ApplyImpulseFixedRotationMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedrotationmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedrotationmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedrotationmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedRotationMovAndMov(manifold, globals);
}
 
