// 
// physics/forcesolverartjoints.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_FORCESOLVERARTJOINTS_H 
#define PHYSICS_FORCESOLVERARTJOINTS_H 

namespace rage {

struct phForceSolverGlobals;
struct phTaskCollisionPair;
class phManifold;

void UpdateContactsArtJoint(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseArtJoint(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseArtJoint(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushArtJoint(phManifold& manifold, const phForceSolverGlobals& globals);


} // namespace rage

#endif // PHYSICS_FORCESOLVERARTJOINTS_H 
