//  
// phsolver/ApplyImpulseFixedRotationArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedrotationartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedrotationartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedrotationartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedRotationArtAndMov(manifold, globals);
}
 
