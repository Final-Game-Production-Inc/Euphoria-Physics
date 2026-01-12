//  
// phsolver/UpdateContactsArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsArtAndArt(manifold, globals);
}
 
