// 
// phsolver/updatecontactsmovandart.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

#if __SPU
#include "updatecontactsartandmov.cpp"
#include "physics/manifold.cpp"
#else
#include "forcesolverartcontacts.h"
#endif

SOLVER_OPTIMISATIONS()

namespace rage {

void UpdateContactsMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	manifold.Exchange();
	UpdateContactsArtAndMov(manifold, globals);
	manifold.Exchange();
}

} // namespace rage
