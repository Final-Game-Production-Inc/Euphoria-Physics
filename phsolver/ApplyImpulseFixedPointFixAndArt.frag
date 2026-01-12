//  
// phsolver/ApplyImpulseFixedPointFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedpointfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedpointfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedpointfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedPointFixAndArt(manifold, globals);
}
 
