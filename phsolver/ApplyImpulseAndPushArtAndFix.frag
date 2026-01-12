//  
// phsolver/ApplyImpulseAndPushArtAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushartandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushartandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushartandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushArtAndFix(manifold, globals);
}
 
