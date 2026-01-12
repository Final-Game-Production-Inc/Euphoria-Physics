//  
// phsolver/ApplyImpulseAndPushMovAndArt.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushmovandart.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushmovandart, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushmovandart, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushMovAndArt(manifold, globals);
}
 
