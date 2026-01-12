//  
// phsolver/ApplyImpulseAndPushSlideRotationMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushsliderotationmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushsliderotationmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushsliderotationmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushSlideRotationMovAndFix(manifold, globals);
}
 
