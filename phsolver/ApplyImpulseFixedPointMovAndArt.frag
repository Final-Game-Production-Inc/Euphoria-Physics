//  
// phsolver/ApplyImpulseFixedPointMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedpointmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedpointmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedpointmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedPointMovAndArt(manifold, globals);
}
 
