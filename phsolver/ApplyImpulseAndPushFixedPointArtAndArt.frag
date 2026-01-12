//  
// phsolver/ApplyImpulseAndPushFixedPointArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushfixedpointartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushfixedpointartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushfixedpointartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushFixedPointArtAndArt(manifold, globals);
}
 
