//  
// phsolver/UpdateContactsFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsFixAndArt(manifold, globals);
}
 
