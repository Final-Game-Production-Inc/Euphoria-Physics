// 
// physics/forcesolvercontacts.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_FORCESOLVERCONTACTS_H 
#define PHYSICS_FORCESOLVERCONTACTS_H 

namespace rage {

struct phForceSolverGlobals;
struct phTaskCollisionPair;
class phManifold;

void UpdateContactsFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsFixAndFix(phManifold& manifold, const phForceSolverGlobals& globals);

void PreResponseMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseAndPushMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void AssertFunc(phManifold& manifold, const phForceSolverGlobals& globals);
void NullFunc(phManifold& manifold, const phForceSolverGlobals& globals);

} // namespace rage

#endif // PHYSICS_FORCESOLVERCONTACTS_H 
