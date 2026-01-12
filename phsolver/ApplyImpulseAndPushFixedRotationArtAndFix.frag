//  
// phsolver/ApplyImpulseAndPushFixedRotationArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushfixedrotationartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushfixedrotationartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushfixedrotationartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushFixedRotationArtAndFix(manifold, globals);
}
 
