//  
// phsolver/UpdateContactsFixedPointArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsfixedpointartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsfixedpointartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsfixedpointartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsFixedPointArtAndArt(manifold, globals);
}
 
