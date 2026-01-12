//  
// phsolver/ApplyImpulseAndPushSlideRotationFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushsliderotationfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushsliderotationfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushsliderotationfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushSlideRotationFixAndArt(manifold, globals);
}
 
