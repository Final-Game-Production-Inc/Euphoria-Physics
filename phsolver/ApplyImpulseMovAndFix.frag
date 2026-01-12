//  
// phsolver/ApplyImpulseMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsemovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsemovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsemovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseMovAndFix(manifold, globals);
}
 
