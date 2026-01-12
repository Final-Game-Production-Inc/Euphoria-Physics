//  
// phsolver/ApplyImpulseAndPushPivotRotationArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushPivotrotationartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushPivotrotationartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushPivotrotationartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushPivotRotationArtAndFix(manifold, globals);
}
 
