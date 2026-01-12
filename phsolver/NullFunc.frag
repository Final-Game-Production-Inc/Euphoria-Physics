//  
// phsolver/NullFunc.frag  
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.  
//  
 
#include "nullfunc.cpp"
 
using namespace rage;
 
SPUFRAG_DECL(void, nullfunc, phManifold&, const phForceSolverGlobals&);
SPUFRAG_IMPL(void, nullfunc, phManifold& manifold, const phForceSolverGlobals& globals)
{
	NullFunc(manifold, globals);
}
 
