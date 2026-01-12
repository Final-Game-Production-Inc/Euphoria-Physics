//  
// phsolver/ApplyImpulseFixedPointMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedpointmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedpointmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedpointmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedPointMovAndMov(manifold, globals);
}
 
