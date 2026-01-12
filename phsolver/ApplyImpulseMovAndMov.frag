//  
// phsolver/ApplyImpulseMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsemovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsemovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsemovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseMovAndMov(manifold, globals);
}
 
