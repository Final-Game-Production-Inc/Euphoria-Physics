//  
// phsolver/ApplyImpulseAndPushPivotRotationMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushPivotrotationmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushPivotrotationmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushPivotrotationmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushPivotRotationMovAndArt(manifold, globals);
}
 
