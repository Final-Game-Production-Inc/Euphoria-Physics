//  
// phsolver/PreResponseFixedPointFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsefixedpointfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsefixedpointfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsefixedpointfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseFixedPointFixAndArt(manifold, globals);
}
 
