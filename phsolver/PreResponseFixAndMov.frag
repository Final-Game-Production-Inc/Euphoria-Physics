//  
// phsolver/PreResponseFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsefixandMov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsefixandMov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsefixandMov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseFixAndMov(manifold, globals);
}
 
