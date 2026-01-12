//  
// phsolver/PreResponseFixedPointArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsefixedpointartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsefixedpointartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsefixedpointartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseFixedPointArtAndArt(manifold, globals);
}
 
