//  
// phsolver/PreResponseArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponseartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponseartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponseartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseArtAndMov(manifold, globals);
}
 
