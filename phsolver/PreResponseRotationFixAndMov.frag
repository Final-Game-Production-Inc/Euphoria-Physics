//  
// phsolver/PreResponseRotationFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponserotationfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponserotationfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponserotationfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseRotationFixAndMov(manifold, globals);
}
 
