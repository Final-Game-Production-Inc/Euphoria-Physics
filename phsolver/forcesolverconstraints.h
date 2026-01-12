// 
// physics/forcesolverconstraints.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_FORCESOLVERCONSTRAINTS_H 
#define PHYSICS_FORCESOLVERCONSTRAINTS_H 

namespace rage {

struct phForceSolverGlobals;
struct phTaskCollisionPair;
class phManifold;

void UpdateContactsFixedPointFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsFixedPointMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsFixedPointMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsFixedPointFixAndFix(phManifold& manifold, const phForceSolverGlobals& globals);

void PreResponseFixedPointMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseFixedPointMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseFixedPointFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseFixedPointMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedPointMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedPointFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseAndPushFixedPointMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedPointMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedPointFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void UpdateContactsRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void UpdateContactsRotationFixAndFix(phManifold& manifold, const phForceSolverGlobals& globals);

void PreResponseRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void PreResponseRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseFixedRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseFixedRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseAndPushFixedRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushFixedRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseSlideRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseSlideRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseSlideRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseAndPushSlideRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushSlideRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushSlideRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulsePivotRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulsePivotRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulsePivotRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

void ApplyImpulseAndPushPivotRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushPivotRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals);
void ApplyImpulseAndPushPivotRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals);

} // namespace rage

#endif // PHYSICS_FORCESOLVERCONSTRAINTS_H 
