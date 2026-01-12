// 
// physics/forcesolverartconstraints.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_FORCESOLVERARTCONSTRAINTS_H 
#define PHYSICS_FORCESOLVERARTCONSTRAINTS_H 

namespace rage {

struct phForceSolverGlobals;
struct phTaskCollisionPair;
class phManifold;

void UpdateContactsFixedPointFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsFixedPointArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsFixedPointMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsFixedPointArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsFixedPointArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void PreResponseFixedPointArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseFixedPointArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseFixedPointArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseFixedPointFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseFixedPointMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseFixedPointArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedPointArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedPointArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedPointFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedPointMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseAndPushFixedPointArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedPointArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedPointArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedPointFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedPointMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void UpdateContactsRotationFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsRotationArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsRotationMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void PreResponseRotationArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseRotationFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseRotationMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseFixedRotationArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedRotationFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedRotationMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseAndPushFixedRotationArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedRotationFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedRotationMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseSlideRotationArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseSlideRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseSlideRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseSlideRotationFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseSlideRotationMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseAndPushSlideRotationArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushSlideRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushSlideRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushSlideRotationFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushSlideRotationMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulsePivotRotationArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulsePivotRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulsePivotRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulsePivotRotationFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulsePivotRotationMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseAndPushPivotRotationArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushPivotRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushPivotRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushPivotRotationFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushPivotRotationMovAndArt(phManifold& manifold, const phForceSolverGlobals& globals);


} // namespace rage

#endif // PHYSICS_FORCESOLVERARTCONSTRAINTS_H 
