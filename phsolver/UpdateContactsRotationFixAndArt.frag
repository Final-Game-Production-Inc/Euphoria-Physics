//  
// phsolver/UpdateContactsRotationFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsrotationfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsrotationfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsrotationfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsRotationFixAndArt(manifold, globals);
}
 
