//  
// phsolver/UpdateContactsRotationArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsrotationartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsrotationartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsrotationartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsRotationArtAndArt(manifold, globals);
}
 
