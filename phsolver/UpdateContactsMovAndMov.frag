//  
// phsolver/UpdateContactsMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsMovAndMov(manifold, globals);
}
 
