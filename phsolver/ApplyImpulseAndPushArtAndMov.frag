//  
// phsolver/ApplyImpulseAndPushArtAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushartandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushartandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushartandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushArtAndMov(manifold, globals);
}
 
