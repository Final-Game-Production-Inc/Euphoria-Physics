//  
// phsolver/PreResponseRotationMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponserotationmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponserotationmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponserotationmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseRotationMovAndFix(manifold, globals);
}
 
