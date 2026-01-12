//  
// phsolver/ApplyImpulseAndPushSlideRotationMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushsliderotationmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushsliderotationmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushsliderotationmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushSlideRotationMovAndArt(manifold, globals);
}
 
