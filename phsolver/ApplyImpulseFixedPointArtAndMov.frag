//  
// phsolver/ApplyImpulseFixedPointArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedpointartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedpointartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedpointartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedPointArtAndMov(manifold, globals);
}
 
