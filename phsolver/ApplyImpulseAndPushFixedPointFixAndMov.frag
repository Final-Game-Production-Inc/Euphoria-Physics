//  
// phsolver/ApplyImpulseAndPushFixedPointFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushfixedpointfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushfixedpointfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushfixedpointfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushFixedPointFixAndMov(manifold, globals);
}
 
