//  
// phsolver/ApplyImpulseFixedPointArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedpointartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedpointartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedpointartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedPointArtAndFix(manifold, globals);
}
 
