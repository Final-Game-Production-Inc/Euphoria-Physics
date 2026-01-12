//  
// phsolver/UpdateContactsMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsMovAndArt(manifold, globals);
}
 
