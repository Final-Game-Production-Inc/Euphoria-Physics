//  
// phsolver/PreResponseFixedPointArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsefixedpointartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsefixedpointartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsefixedpointartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseFixedPointArtAndMov(manifold, globals);
}
 
