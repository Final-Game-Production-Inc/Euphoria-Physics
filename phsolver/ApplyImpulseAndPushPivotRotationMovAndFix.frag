//  
// phsolver/ApplyImpulseAndPushPivotRotationMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushPivotrotationmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushPivotrotationmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushPivotrotationmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushPivotRotationMovAndFix(manifold, globals);
}
 
