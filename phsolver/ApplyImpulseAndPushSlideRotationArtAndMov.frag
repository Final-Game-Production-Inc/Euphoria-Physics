//  
// phsolver/ApplyImpulseAndPushSlideRotationArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushsliderotationartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushsliderotationartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushsliderotationartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushSlideRotationArtAndMov(manifold, globals);
}
 
