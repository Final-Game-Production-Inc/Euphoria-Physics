// 
// phsolver/applyimpulseandpushfixandart.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"
#include "forcesolverartcontacts.h"

#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

#if __SPU
#include "applyimpulseandpushartandfix.cpp"
#include "physics/manifold.cpp"
#endif // __SPU

namespace rage {

void ApplyImpulseAndPushFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	CALL_MEMBER_FN(manifold, Exchange)();
	ApplyImpulseAndPushArtAndFix(manifold, globals);
	CALL_MEMBER_FN(manifold, Exchange)();
}

} // namespace rage
