// 
// physics/forcesolverartcontacts.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_FORCESOLVERARTCONTACTS_H 
#define PHYSICS_FORCESOLVERARTCONTACTS_H 

namespace rage {

struct phForceSolverGlobals;
struct phTaskCollisionPair;
class phManifold;

void UpdateContactsFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void PreResponseArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseAndPushArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

} // namespace rage

#endif // PHYSICS_FORCESOLVERARTCONTACTS_H 
