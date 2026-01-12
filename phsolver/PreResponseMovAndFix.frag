//  
// phsolver/PreResponseMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponsemovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponsemovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponsemovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseMovAndFix(manifold, globals);
}
 
