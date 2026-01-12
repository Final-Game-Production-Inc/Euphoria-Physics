//  
// phsolver/PreResponseFixedPointFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsefixedpointfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsefixedpointfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsefixedpointfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseFixedPointFixAndMov(manifold, globals);
}
 
