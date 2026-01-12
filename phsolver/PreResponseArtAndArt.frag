//  
// phsolver/PreResponseArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponseartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponseartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponseartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseArtAndArt(manifold, globals);
}
 
