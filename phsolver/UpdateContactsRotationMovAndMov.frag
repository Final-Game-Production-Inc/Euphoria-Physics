//  
// phsolver/UpdateContactsRotationMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsrotationmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsrotationmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsrotationmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsRotationMovAndMov(manifold, globals);
}
 
