//  
// phsolver/ApplyImpulseFixedRotationArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedrotationartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedrotationartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedrotationartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedRotationArtAndFix(manifold, globals);
}
 
