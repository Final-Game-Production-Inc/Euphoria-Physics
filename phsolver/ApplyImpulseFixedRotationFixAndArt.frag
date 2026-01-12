//  
// phsolver/ApplyImpulseFixedRotationFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedrotationfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedrotationfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedrotationfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedRotationFixAndArt(manifold, globals);
}
 
