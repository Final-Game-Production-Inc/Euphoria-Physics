//  
// phsolver/PreResponseFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsefixandArt.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsefixandArt, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsefixandArt, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseFixAndArt(manifold, globals);
}
 
