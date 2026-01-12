//  
// phsolver/PreResponseRotationArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponserotationartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponserotationartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponserotationartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseRotationArtAndArt(manifold, globals);
}
 
