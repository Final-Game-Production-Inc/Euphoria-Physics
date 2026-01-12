//  
// phsolver/UpdateContactsRotationArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsrotationartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsrotationartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsrotationartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsRotationArtAndMov(manifold, globals);
}
 
