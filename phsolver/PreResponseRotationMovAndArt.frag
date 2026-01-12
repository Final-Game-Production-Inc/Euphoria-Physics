//  
// phsolver/PreResponseRotationMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponserotationmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponserotationmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponserotationmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseRotationMovAndArt(manifold, globals);
}
 
