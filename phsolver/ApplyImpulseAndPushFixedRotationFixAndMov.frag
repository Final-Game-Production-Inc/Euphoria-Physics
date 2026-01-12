//  
// phsolver/ApplyImpulseAndPushFixedRotationFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushfixedrotationfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushfixedrotationfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushfixedrotationfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushFixedRotationFixAndMov(manifold, globals);
}
 
