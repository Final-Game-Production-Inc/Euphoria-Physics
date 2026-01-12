//  
// phsolver/ApplyImpulseAndPushFixedPointMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushfixedpointmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushfixedpointmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushfixedpointmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushFixedPointMovAndMov(manifold, globals);
}
 
