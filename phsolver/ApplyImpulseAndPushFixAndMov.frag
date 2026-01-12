//  
// phsolver/ApplyImpulseAndPushFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushFixAndMov(manifold, globals);
}
 
