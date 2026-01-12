//  
// phsolver/ApplyImpulseFixedRotationMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulsefixedrotationmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulsefixedrotationmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulsefixedrotationmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseFixedRotationMovAndFix(manifold, globals);
}
 
