//  
// phsolver/ApplyImpulseAndPushFixAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushfixandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushfixandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushfixandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushFixAndArt(manifold, globals);
}
 
