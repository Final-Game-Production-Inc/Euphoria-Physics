//  
// phsolver/AssertFunc.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "assertfunc.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, assertfunc, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, assertfunc, phManifold& manifold, const phForceSolverGlobals& globals)
{
	AssertFunc(manifold, globals);
}
 
