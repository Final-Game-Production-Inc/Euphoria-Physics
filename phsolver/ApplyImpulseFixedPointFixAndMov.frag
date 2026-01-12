//  
// phsolver/ApplyImpulseFixedPointFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedpointfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedpointfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedpointfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedPointFixAndMov(manifold, globals);
}
 
