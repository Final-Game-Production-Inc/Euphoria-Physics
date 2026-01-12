//  
// phsolver/ApplyImpulseAndPushFixedPointArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushfixedpointartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushfixedpointartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushfixedpointartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushFixedPointArtAndMov(manifold, globals);
}
 
