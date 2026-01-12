//  
// phsolver/PreResponseFixedPointMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsefixedpointmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsefixedpointmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsefixedpointmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseFixedPointMovAndArt(manifold, globals);
}
 
