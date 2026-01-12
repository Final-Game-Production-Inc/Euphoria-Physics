//  
// phsolver/ApplyImpulseAndPushFixedRotationFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushfixedrotationfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushfixedrotationfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushfixedrotationfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushFixedRotationFixAndArt(manifold, globals);
}
 
