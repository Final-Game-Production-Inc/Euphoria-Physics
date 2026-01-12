//  
// phsolver/ApplyImpulseAndPushPivotRotationFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushPivotrotationfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushPivotrotationfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushPivotrotationfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushPivotRotationFixAndArt(manifold, globals);
}
 
