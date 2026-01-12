// 
// phsolver/updatecontactsfixedpointmovandart.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

#if __SPU
#include "updatecontactsfixedpointartandmov.cpp"
#include "physics/manifold.cpp"
#else
#include "forcesolverartconstraints.h"
#endif

SOLVER_OPTIMISATIONS()

namespace rage {

void UpdateContactsFixedPointMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	manifold.Exchange();
	UpdateContactsFixedPointArtAndMov(manifold, globals);
	manifold.Exchange();
}

} // namespace rage
