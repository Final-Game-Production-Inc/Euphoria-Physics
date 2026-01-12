//  
// phsolver/PreResponseRotationMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponserotationmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponserotationmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponserotationmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseRotationMovAndMov(manifold, globals);
}
 
