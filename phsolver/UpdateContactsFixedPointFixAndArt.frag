//  
// phsolver/UpdateContactsFixedPointFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsfixedpointfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsfixedpointfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsfixedpointfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsFixedPointFixAndArt(manifold, globals);
}
 
