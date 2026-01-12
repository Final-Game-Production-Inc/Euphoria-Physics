//  
// phsolver/UpdateContactsRotationMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsrotationmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsrotationmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsrotationmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsRotationMovAndArt(manifold, globals);
}
 
