//  
// phsolver/ApplyImpulseArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseArtAndFix(manifold, globals);
}
 
