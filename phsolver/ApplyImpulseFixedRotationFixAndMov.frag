//  
// phsolver/ApplyImpulseFixedRotationFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedrotationfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedrotationfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedrotationfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedRotationFixAndMov(manifold, globals);
}
 
