// 
// phsolver/nullfunc.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

SOLVER_OPTIMISATIONS()

namespace rage {

class phManifold;
struct phForceSolverGlobals;

void NullFunc(phManifold& UNUSED_PARAM(manifold), const phForceSolverGlobals& UNUSED_PARAM(globals))
{ 
}

} // namespace rage
