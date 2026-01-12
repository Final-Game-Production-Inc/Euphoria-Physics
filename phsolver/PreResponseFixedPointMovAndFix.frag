//  
// phsolver/PreResponseFixedPointMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsefixedpointmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsefixedpointmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsefixedpointmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseFixedPointMovAndFix(manifold, globals);
}
 
