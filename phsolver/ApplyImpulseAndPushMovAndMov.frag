//  
// phsolver/ApplyImpulseAndPushMovAndMov.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushmovandmov.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushmovandmov, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushmovandmov, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushMovAndMov(manifold, globals);
}
 
