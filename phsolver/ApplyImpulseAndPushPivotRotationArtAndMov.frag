//  
// phsolver/ApplyImpulseAndPushPivotRotationArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushPivotrotationartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushPivotrotationartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushPivotrotationartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushPivotRotationArtAndMov(manifold, globals);
}
 
