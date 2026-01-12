//  
// phsolver/ApplyImpulseFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixAndArt(manifold, globals);
}
 
