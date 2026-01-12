//  
// phsolver/UpdateContactsArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsArtAndFix(manifold, globals);
}
 
