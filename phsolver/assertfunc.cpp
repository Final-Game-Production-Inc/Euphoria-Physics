// 
// phsolver/assertfunc.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#if __SPU
#include "system/spu_library.cpp"
#endif

SOLVER_OPTIMISATIONS()

namespace rage {

class phManifold;
struct phForceSolverGlobals;

void AssertFunc(phManifold& UNUSED_PARAM(manifold), const phForceSolverGlobals& UNUSED_PARAM(globals))
{ 
	Assert(0);
}

} // namespace rage
