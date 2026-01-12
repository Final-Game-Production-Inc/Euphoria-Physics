//  
// phsolver/UpdateContactsFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsFixAndMov(manifold, globals);
}
 
