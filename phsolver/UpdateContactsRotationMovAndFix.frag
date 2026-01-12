//  
// phsolver/UpdateContactsRotationMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsrotationmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsrotationmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsrotationmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsRotationMovAndFix(manifold, globals);
}
 
