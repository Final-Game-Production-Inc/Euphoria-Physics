//  
// phsolver/PreResponseMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsemovandArt.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsemovandArt, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsemovandArt, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseMovAndArt(manifold, globals);
}
 
