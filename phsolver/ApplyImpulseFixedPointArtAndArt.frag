//  
// phsolver/ApplyImpulseFixedPointArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedpointartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedpointartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedpointartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedPointArtAndArt(manifold, globals);
}
 
