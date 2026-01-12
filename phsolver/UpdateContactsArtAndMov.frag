//  
// phsolver/UpdateContactsArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsArtAndMov(manifold, globals);
}
 
