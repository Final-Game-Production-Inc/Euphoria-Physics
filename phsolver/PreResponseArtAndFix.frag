//  
// phsolver/PreResponseArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponseartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponseartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponseartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseArtAndFix(manifold, globals);
}
 
