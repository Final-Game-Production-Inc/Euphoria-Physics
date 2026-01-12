// 
// phsolver/updatecontactsrotationfixandmov.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

#if __SPU
#include "updatecontactsrotationmovandfix.cpp"
#include "physics/manifold.cpp"
#else
#include "forcesolverconstraints.h"
#endif

SOLVER_OPTIMISATIONS()

namespace rage {

void UpdateContactsRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	manifold.Exchange();
	UpdateContactsRotationMovAndFix(manifold, globals);
	manifold.Exchange();
}

} // namespace rage
