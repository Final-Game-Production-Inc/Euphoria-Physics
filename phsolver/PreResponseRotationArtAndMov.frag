//  
// phsolver/PreResponseRotationArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponserotationartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponserotationartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponserotationartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseRotationArtAndMov(manifold, globals);
}
 
