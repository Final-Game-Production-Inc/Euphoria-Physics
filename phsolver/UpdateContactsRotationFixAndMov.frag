//  
// phsolver/UpdateContactsRotationFixAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "updatecontactsrotationfixandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, updatecontactsrotationfixandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, updatecontactsrotationfixandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	UpdateContactsRotationFixAndMov(manifold, globals);
}
 
