//  
// phsolver/UpdateContactsFixedPointMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsfixedpointmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsfixedpointmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsfixedpointmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsFixedPointMovAndFix(manifold, globals);
}
 
