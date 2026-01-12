//  
// phsolver/UpdateContactsFixedPointArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsfixedpointartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsfixedpointartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsfixedpointartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsFixedPointArtAndFix(manifold, globals);
}
 
