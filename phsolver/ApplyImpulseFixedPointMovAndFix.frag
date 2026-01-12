//  
// phsolver/ApplyImpulseFixedPointMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedpointmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedpointmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedpointmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedPointMovAndFix(manifold, globals);
}
 
