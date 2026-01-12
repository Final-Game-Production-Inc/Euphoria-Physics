//  
// phsolver/PreResponseFixedPointArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsefixedpointartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsefixedpointartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsefixedpointartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseFixedPointArtAndFix(manifold, globals);
}
 
