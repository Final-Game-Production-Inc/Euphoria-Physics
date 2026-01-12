//  
// phsolver/UpdateContactsFixedPointMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsfixedpointmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsfixedpointmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsfixedpointmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsFixedPointMovAndArt(manifold, globals);
}
 
