//  
// phsolver/ApplyImpulseFixedRotationArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedrotationartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedrotationartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedrotationartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedRotationArtAndArt(manifold, globals);
}
 
