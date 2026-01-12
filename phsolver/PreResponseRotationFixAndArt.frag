//  
// phsolver/PreResponseRotationFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponserotationfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponserotationfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponserotationfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseRotationFixAndArt(manifold, globals);
}
 
