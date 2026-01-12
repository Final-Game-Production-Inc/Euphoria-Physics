//  
// phsolver/ApplyImpulseAndPushArtAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushartandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushartandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushartandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushArtAndArt(manifold, globals);
}
 
