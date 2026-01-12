//  
// phsolver/ApplyImpulseAndPushFixedRotationArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushfixedrotationartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushfixedrotationartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushfixedrotationartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushFixedRotationArtAndArt(manifold, globals);
}
 
