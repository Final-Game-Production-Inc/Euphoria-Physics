//  
// phsolver/UpdateContactsFixedPointMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsfixedpointmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsfixedpointmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsfixedpointmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsFixedPointMovAndMov(manifold, globals);
}
 
