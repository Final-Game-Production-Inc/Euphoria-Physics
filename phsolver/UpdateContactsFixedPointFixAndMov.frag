//  
// phsolver/UpdateContactsFixedPointFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsfixedpointfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsfixedpointfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsfixedpointfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsFixedPointFixAndMov(manifold, globals);
}
 
