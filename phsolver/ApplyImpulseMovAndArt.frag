//  
// phsolver/ApplyImpulseMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsemovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsemovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsemovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseMovAndArt(manifold, globals);
}
 
