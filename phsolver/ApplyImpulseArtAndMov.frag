//  
// phsolver/ApplyImpulseArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseArtAndMov(manifold, globals);
}
 
