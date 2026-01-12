//  
// phsolver/ApplyImpulseAndPushSlideRotationMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushsliderotationmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushsliderotationmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushsliderotationmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushSlideRotationMovAndMov(manifold, globals);
}
 
