//  
// phsolver/PreResponseMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsemovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsemovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsemovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseMovAndMov(manifold, globals);
}
 
