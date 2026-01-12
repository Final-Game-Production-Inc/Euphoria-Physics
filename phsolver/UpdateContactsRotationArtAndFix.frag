//  
// phsolver/UpdateContactsRotationArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsrotationartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsrotationartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsrotationartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsRotationArtAndFix(manifold, globals);
}
 
