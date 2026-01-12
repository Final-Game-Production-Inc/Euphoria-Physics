//  
// phsolver/UpdateContactsFixedPointArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsfixedpointartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsfixedpointartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsfixedpointartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsFixedPointArtAndMov(manifold, globals);
}
 
