// 
// phsolver/updatecontactsfixedpointfixandart.cpp
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

#if __SPU
#include "updatecontactsfixedpointartandfix.cpp"
#include "physics/manifold.cpp"
#else
#include "forcesolverartconstraints.h"
#endif

SOLVER_OPTIMISATIONS()

namespace rage {

void UpdateContactsFixedPointFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	manifold.Exchange();
	UpdateContactsFixedPointArtAndFix(manifold, globals);
	manifold.Exchange();
}

} // namespace rage
