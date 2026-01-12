//  
// phsolver/ApplyImpulseArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseArtAndArt(manifold, globals);
}
 
