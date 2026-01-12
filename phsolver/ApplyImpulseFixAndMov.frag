//  
// phsolver/ApplyImpulseFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixAndMov(manifold, globals);
}
 
