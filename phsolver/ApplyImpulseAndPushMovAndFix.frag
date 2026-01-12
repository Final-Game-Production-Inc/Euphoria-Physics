//  
// phsolver/ApplyImpulseAndPushMovAndFix.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "applyimpulseandpushmovandfix.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, applyimpulseandpushmovandfix, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, applyimpulseandpushmovandfix, phManifold& manifold, const phForceSolverGlobals& globals)
{
	ApplyImpulseAndPushMovAndFix(manifold, globals);
}
 
