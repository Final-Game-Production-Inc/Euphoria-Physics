//  
// phsolver/UpdateContactsMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsMovAndFix(manifold, globals);
}
 
