//  
// phsolver/PreResponseRotationArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "preresponserotationartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, preresponserotationartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, preresponserotationartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	PreResponseRotationArtAndFix(manifold, globals);
}
 
