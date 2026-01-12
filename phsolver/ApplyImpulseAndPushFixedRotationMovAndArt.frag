//  
// phsolver/ApplyImpulseAndPushFixedRotationMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushfixedrotationmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushfixedrotationmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushfixedrotationmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushFixedRotationMovAndArt(manifold, globals);
}
 
