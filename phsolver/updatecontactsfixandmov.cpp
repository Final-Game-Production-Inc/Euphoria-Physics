// 
// phsolver/updatecontactsfixandmov.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

#if __SPU
#include "updatecontactsmovandfix.cpp"
#include "physics/manifold.cpp"
#else
#include "forcesolvercontacts.h"
#endif

SOLVER_OPTIMISATIONS()

namespace rage {

void UpdateContactsFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	manifold.Exchange();
	UpdateContactsMovAndFix(manifold, globals);
	manifold.Exchange();
}

} // namespace rage
