//  
// phsolver/PreResponseFixedPointMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsefixedpointmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsefixedpointmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsefixedpointmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseFixedPointMovAndMov(manifold, globals);
}
 
