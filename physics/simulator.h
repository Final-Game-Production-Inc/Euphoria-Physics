//
// physics/simulator.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_SIMULATOR_H
#define PHYSICS_SIMULATOR_H

////////////////////////////////////////////////////////////////
// external defines

#if !__SPU
#include "constraintmgr.h"
#endif

#include "collider.h"
#include "inst.h"
#include "levelnew.h"

#if __PS3 
#include <cell/spurs/task_types.h>
#endif // __PS3 

#include "atl/functor.h"

#define USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK	0

#if (__WIN32 || ENABLE_UNUSED_PHYSICS_CODE)
class btStackAlloc;
#endif // __WIN32 || ENABLE_UNUSED_PHYSICS_CODE

namespace rage {

class bkBank;
struct btBroadphasePair;
class phBreakData;
class phCollider;
class phCompositePointers;
class phConstraintMgr;
class phContactMgr;
class phImpact;
class phInst;
class phInstBehavior;
class phInstBreakable;
struct phOverlappingPairArray;
template <class T> class phPool;
class phSleep;
class phSleepMgr;
class phWorkerThread;
struct phTaskCollisionPair;
struct sysTaskParameters;
struct phCollisionMemory;
class GJKCacheSystem;

// PURPOSE:	If set, phSimulator::m_InstBehaviors will be sorted, and
//			phSimulator::GetInstBehavior() will use a binary search
//			instead of a linear search.
// NOTES:	There is probably no good reason to turn this off, except for
//			testing its impact on performance or if any bugs are suspected.
#define PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX 1

// PURPOSE:	If set (together with PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX),
//			an array of level indices will be allocated in parallel with
//			phSimulator::m_InstBehaviors. This increases the memory cost by
//			sizeof(u16)*m_MaxInstBehaviors, but improves performance by
//			more efficient data cache utilization and better optimization
//			of the binary search code.
// NOTES:	It is probably worth leaving this on, because the memory cost
//			is fairly small compared to the performance gain. To take an
//			example, in a project with up to 128 instance behaviors, the memory
//			cost is just 256 bytes, but a gain of almost 0.1 ms per frame was
//			measured.
#define PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE (1 && PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX)

// PURPOSE:	This can be enabled in combination with PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX,
//			to enable extra checks to make sure nothing goes wrong with the data structures.
// NOTES:	There is some performance overhead here, so it is recommended to leave this
//			off except for if a problem is suspected.
#define PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SANITY_CHECK (0 && __ASSERT && PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX)

#define PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY 1

// PURPOSE:	Defines the min number of updates that a contact point will go through 
//          before being considered for removal due to orthogonal movement relative 
//          to the contact normal.
#define PHSIM_DEFAULT_MIN_POINT_LIFETIME	4

// PURPOSE: Used only with LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE. Will commit
//          all deferred octree updates in the simulator's PreCollide if set to 1.
//          If 0, the application is responsible for calling
//          PHLEVEL->CommitDeferredOctreeUpdates() once per frame.
#define PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR			(!HACK_RDR2)


////////////////////////////////////////////////////////////////
// phSimulator


// singletons (there can be only one simulator running at a time)

#if __SPU
// "this" pointer not needed since all methods are static on SPU
#define PHMANIFOLD ((::rage::phPool< ::rage::phManifold>*)NULL)
#define PHCONTACT ((::rage::phPool< ::rage::phContact>*)NULL)
#define PHCOMPOSITEPOINTERS ((::rage::phPool< ::rage::phCompositePointers>*)NULL)
#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE
extern GJKCacheSystem * g_gjkCacheSystem_EA;
#define GJKCACHESYSTEM g_gjkCacheSystem_EA
#else // USE_SPU_FRAME_PERSISTENT_GJK_CACHE
#define GJKCACHESYSTEM NULL
#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE
#else
#define PHMANIFOLD (::rage::phSimulator::GetActiveInstance()->GetManifoldPool())
#define PHCONTACT (::rage::phSimulator::GetActiveInstance()->GetContactPool())
#define PHCOMPOSITEPOINTERS (::rage::phSimulator::GetActiveInstance()->GetCompositePointerPool())
#if __FINAL
#define PHSIM (::rage::phSimulator::GetActiveInstance())
#define PHCONSTRAINT (::rage::phSimulator::GetActiveInstance()->GetConstraintMgr())
#else
#define PHSIM (::rage::phSimulator::GetActiveInstance()->GetConstraintMgr()->SetOwner(__FILE__, __LINE__), ::rage::phSimulator::GetActiveInstance())
#define PHCONSTRAINT (::rage::phSimulator::GetActiveInstance()->GetConstraintMgr()->SetOwner(__FILE__, __LINE__), ::rage::phSimulator::GetActiveInstance()->GetConstraintMgr())
#endif
#define PHSLEEP (::rage::phSimulator::GetActiveInstance()->GetSleepMgr())
#define GJKCACHESYSTEM (::rage::phSimulator::GetActiveInstance()->GetGJKCacheSystem())
#endif

extern int g_MaxPushCollisionIterations;

// PURPOSE
//   The physics simulator updates and calculates collisions for all physically active objects.
// <FLAG Component>
class phSimulator
{
public:
	// singleton management

	// PURPOSE: Set the currently active simulator.
	// PARAMS:
	//	instance -	the simulator to set as the currently active simulator
	// NOTES:
	//	1.	There can be only one simulator running at a time.
	static void SetActiveInstance (phSimulator* instance);

	// PURPOSE: Set this simulator as the currently active simulator.
	// NOTES:
	//	1.	There can be only one simulator running at a time.
	void SetActiveInstance ()											{ SetActiveInstance(this); }

	// PURPOSE: Get the currently active simulator.
	// RETURN: the currently active simulator
	// NOTES:
	//	1.	There can be only one simulator running at a time.
	static phSimulator * GetActiveInstance ()							{ return sm_ActiveInstance; }

	phSimulator();
	virtual ~phSimulator();

	struct InitParams
	{
		// The number of active objects of normal type (that don't have their own special colliders, articulated bodies)
		int maxManagedColliders;
		
		// The number of bytes in the contact manager for allocating data for SplitPairs and breaking operations
		int scratchpadSize;

		// The number of manifolds to allocate in the manifold pool
		int maxManifolds;

		// The number of manifolds to reserve for collisions involving insts marked with the high priority flag
		int highPriorityManifolds;

		// The number of manifolds that can be root composite manifolds
		int maxCompositeManifolds;

		// The number of contacts to allocate in the contact pool
		int maxContacts;

		// The number of manifolds that can involve objects that report externally controlled velocities (kinematically controlled)
		int maxExternVelManifolds;

		// The number of inst behaviors that can be added to the simulator
		u16 maxInstBehaviors;

		// The number of user constraints (phConstraints) that can be allocated by the user
		int maxConstraints;

		// The number of constraints of the larger types (currently hinges)
		// NOTE: use zero to mean, allocate all constraints from the game heap
		int maxBigConstraints;

		// The number of constraints of the smaller types
		// NOTE: use zero to mean, allocate all constraints from the game heap
		int maxLittleConstraints;

		// The number of bytes to allocate for storing information about sleeping insts
		size_t sleepBufferSize;

		// The maximum number of islands for which we track information while they are asleep
		int maxSleepIslands;

		InitParams()
			: maxManagedColliders(512)
			, scratchpadSize(6*1024*1024)
			, maxManifolds(1024)
			, highPriorityManifolds(0)
			, maxCompositeManifolds(256)
			, maxContacts(4096)
			, maxExternVelManifolds(256)
			, maxInstBehaviors(0)
			, maxConstraints(256)
			, maxBigConstraints(256)
			, maxLittleConstraints(256)
#if __WIN32
			, sleepBufferSize(64 * 1024)
#else
			, sleepBufferSize(16 * 1024)
#endif // #if __WIN32
			, maxSleepIslands(256)
		{
		}
	};

	void Init(phLevelNew *pNewLevel, const InitParams& userParams);

	static void InitClass();
	static void ShutdownClass();

	void Reset ();
	void ResetInstanceBehaviors ();

	void Clear ();

	void ValidateArchetypes ();

	void SortUnusedManagedColliderIndices ();

	// Global callbacks, called once per frame each with info about all insts
	typedef Functor0 AllCollisionsDoneCallback;
	typedef Functor2 <phContact**, int> PreComputeAllImpactsFunc;
	typedef Functor2 <phContact**, int> PreApplyAllImpactsFunc;
	typedef Functor0Ret<bool> AllowBreakingCallback;

	void SetAllCollisionsDoneCallback(AllCollisionsDoneCallback func);
	void SetPreComputeAllImpactsFunc(PreComputeAllImpactsFunc func);
	void SetPreApplyAllImpactsFunc(PreApplyAllImpactsFunc func);
	void SetAllowBreakingCallback(AllowBreakingCallback func);

	AllCollisionsDoneCallback		GetAllCollisionsDoneCallback() const;
	PreComputeAllImpactsFunc		GetPreComputeAllImpactsFunc() const;
	PreApplyAllImpactsFunc			GetPreApplyAllImpactsFunc() const;
	AllowBreakingCallback			GetAllowBreakingCallback() const;

	static void DefaultAllCollisionsDoneCallback();
	static void DefaultPreComputeAllImpacts(phContact**, int numContacts);
	static void DefaultPreApplyAllImpacts(phContact**, int numContacts);
	static bool DefaultAllowBreakingCallback();

    // PURPOSE: Step the physics forward by one frame
    // NOTES: Normally you call this once per game loop. You can call it multiple times by dividing the time step into parts.
	virtual void Update (float timeStep, bool finalUpdate);

#if __PFDRAW
	void ProfileDraw() const;
	void SimColorChoice(const phInst* inst) const;
#endif

	// This should be called before phSimulator::Update() and before and instance-behavior managers so as to allow the managers to remove
	//   instance behaviors that have ended before get used.
	void PreUpdateInstanceBehaviors (float timeStep);
	void UpdateInstanceBehaviors (float timeStep);

	// PURPOSE: Update and do collision detection and response on one collider.
	// PARAMS:
	//	collider - the collider to update
	//	timeStep - the frame time interval
	//	doCollisions - optional boolean to compute collision detection and response
	// NOTES:	This is used when to locally advance a collider that is behind because of network latency.
	void UpdateOneCollider (phCollider* collider, float timeStep, bool doCollisions=false) const;

	int AddActiveObject(phInst *pInst, bool bAlwaysActive=false);
	int AddActiveObject(phCollider *pCollider);

	// add a batch of objects all at once.  This is more efficient compared to a bunch of atomic adds; O(nlogn) rather than O(n^2)
	void AddActiveObjects(phInst **pInst, int nInst, int *levelIndex, bool bAlwaysActive=false);
	void AddFixedObjects( phInst **pInst, int nInst, int *levelIndex );
	void AddInactiveObjects( phInst **pInst, int nInst, int *levelIndex );

	int AddInactiveObject(phInst *pInst);
	int AddFixedObject(phInst *pInst);

	// add any objects to sweep-and-prune space that were added with delayedSAP
	void CommitDelayedObjectsSAP();

#if __PS3
	phManifold::DmaPlan* GetDmaPlan(phManifold* manifold);
	phManifold* GetManifold(phManifold::DmaPlan* dmaPlan);

	phCollider::DmaPlan* GetDmaPlan(phCollider* collider);
	phCollider* GetManifold(phCollider::DmaPlan* dmaPlan);
#endif

	// PURPOSE: Activate the specified phInst.
	phCollider* ActivateObject (phInst* pInst, phCollider* colliderToActivate=NULL, phInst* otherInst=NULL, const phConstraintBase * constraint=NULL);

	// PURPOSE: Activate the object specified by the levelIndex.
	// NOTES: Use the other version above that takes a phInst* when possible; that version does not incur
	//	a lookup into the PHLEVEL's instance table in non-assert builds.
	phCollider* ActivateObject (int levelIndex, phCollider* colliderToActivate=NULL, phInst* otherInst=NULL, const phConstraintBase * constraint=NULL);

	// PURPOSE: Activate the object specified by the levelIndex and phInst pointer.
	// NOTES: This version expected to be used only in very specialized cases such as when iterating over 
	//	a collection that already has both the level index and the inst pointer and the caller wishes to 
	//	avoid additional lookups.
	phCollider* ActivateObject(int levelIndex, phInst* pInst, phCollider* colliderToActivate=NULL, phInst* otherInst=NULL, const phConstraintBase * constraint=NULL);

	// PURPOSE: Get the linear velocity of the given object.
	// PARAMS:
	//	instance - the physics instance for which to get the velocity
	//	velocity - reference into which to put the velocity
	// RETURN:	true if the given instance is in the physics level, or false if it is not
	bool GetObjectVelocity (const phInst& instance, Vector3& velocity) const;

	// PURPOSE: Get the angular velocity of the given object.
	// PARAMS:
	//	instance - the physics instance for which to get the angular velocity
	//	angVelocity - reference into which to put the angular velocity
	// RETURN:	true if the given instance is in the physics level, or false if it is not
	bool GetObjectAngVelocity (const phInst& instance, Vector3& angVelocity) const;

	// PURPOSE: Get the velocity at a particular point on the given object.
	// PARAMS:
	//	localVelocity - reference into which to put the angular velocity
	//	instance - the physics instance for which to get the angular velocity
	//  position - the position on the object at which the velocity is needed
	//  component - the component on which the point is located, or zero if you don't care
	// RETURN:	true if the given instance is in the physics level, or false if it is not
	bool GetObjectLocalVelocity(Vec3V_InOut localVelocity, const phInst& instance, Vec3V_In position, int component = 0) const;
	bool GetObjectLocalVelocity(Vector3& localVelocity, const phInst& instance, const Vector3& position, int component = 0) const;

	// PURPOSE: Move the given instance to the given position and orientation.  Also clears out all contacts that this instance may have had with other
	//   instances.
	// PARAMS:
	//	instance - the physics instance to teleport
	//	newMatrix - the new position and orientation of the given instance
	//	newLastMatrix - optional new position and orientation of the given instance specified for the previous frame (default is NULL)
	// RETURN:	true if the given instance is in the physics level, or false if it is not
	// NOTES:	If newLastMatrix is NULL then the new matrix is put into the new last matrix, and the object collides as if it had no motion.
	//			If newLastMatrix is specified to be the current (non-teleported) matrix, then the object will try to collide with any other
	//			objects along the teleport path.
	//			Due to the fact that this clears out existing contacts, this function should be used when motion is discontinuous (eg, a car being
	//			  teleported across the world) rather than when motion is continuous (eg, a car being moved smoothly along a path).
	// SEE ALSO: MoveObject().
	bool TeleportObject (phInst& instance, const Matrix34& newMatrix, const Matrix34* newLastMatrix=NULL);


	// PURPOSE: Move the given instance to the given position and orientation, maintaining existing contact points.
	// PARAMS:
	//	instance - the physics instance to teleport
	//	newMatrix - the new position and orientation of the given instance
	//  newLastMatrix - the new last matrix that this instance will have - typically you'll want to pass in whatever the current 'last' matrix is
	// RETURN:	true if the given instance is in the physics level, or false if it is not
	// NOTES:	If newLastMatrix is NULL then the new matrix is put into the new last matrix, and the object collides as if it had no motion.
	//			If newLastMatrix is specified to be the current (non-teleported) matrix, then the object will try to collide with any other
	//			objects along the teleport path.
	//			Due to the fact that this maintains existing contacts, this function should be used when motion is continuous (eg, a car being moved 
	//			  smoothly along a path) rather than when motion is continuous (eg, a car being teleported across the world).
	// SEE ALSO: TeleportObject().
	bool MoveObject (phInst& instance, Mat34V_In newMatrix, Mat34V_In newLastMatrix);

	// PURPOSE: Delete the object from the world.
	// PARAMS:
	//	pInst - the instance to delete
	// NOTES:	This calls DeleteObject in the contact manager and in the physics level. Any object that is removed from the world (such as during streaming)
	//			should have this called. If it is not, or if it is called in the physics level but not here, the simulator can crash on bad instance pointers
	//			when colliding objects are removed from memory.
	void DeleteObject(phInst* pInst, bool forceImmediateDelete = false);

	// PURPOSE: Delete the object from the world.
	// PARAMS:
	//	levelIndex - the level index of the object to delete
	// NOTES:	This calls DeleteObject in the contact manager and in the physics level. Any object that is removed from the world (such as during streaming)
	//			should have this called. If it is not, or if it is called in the physics level but not here, the simulator can crash on bad instance pointers
	//			when colliding objects are removed from memory.
	void DeleteObject(int levelIndex, bool forceImmediateDelete = false);

	// delete a bunch of objects at once
	void DeleteObjects(int *levelIndex, int nLevelIndex);

	// PURPOSE: Deactivate the object.
	// PARAMS:
	//	pInst - the object to deactivate. 
	//  forceDeactivate - if set to true the collider will be deactivated regardless of what phInst::PrepareForDeactivation returns
	void DeactivateObject(phInst* pInst, bool forceDeactivate = false);

	// PURPOSE: Deactivate the object.
	// PARAMS:
	//	knLevelIndex - the level index for the object to deactivate. 
	//  forceDeactivate - if set to true the collider will be deactivated regardless of what phInst::PrepareForDeactivation returns
	// NOTES: Use the other version above that takes a phInst* when possible; that version does not incur
	//	a lookup into the PHLEVEL's instance table in non-assert builds.
	void DeactivateObject(const int knLevelIndex, bool forceDeactivate = false);

	// PURPOSE: Deactivate the object.
	// PARAMS:
	//	knLevelIndex - the level index for the object to deactivate.
	//	pInst - the object to deactivate
	//  forceDeactivate - if set to true the collider will be deactivated regardless of what phInst::PrepareForDeactivation returns
	// NOTES: This version is not expected to be used except for very specialized cases for instance where
	//	iterating over a collection that contains both level index and inst pointers.
	void DeactivateObject(const int knLevelIndex, phInst* pInst, bool forceDeactivate = false);

	void ActivateAll();

	phCollider *GetCollider(const int knLevelIndex) const;
	phCollider *GetCollider(const phInst* instance) const;
	phCollider *GetActiveCollider(int levelIndex) const { return static_cast<phCollider*>(m_Level->GetActiveObjectUserData(levelIndex)); }

	// This interface has been removed. Instead, call -
	//	phLevelNew::SetInactiveCollidesAgainstFixed(levelIndex,true) to make an inactive object collide with fixed objects, and call
	//	phLevelNew::SetInactiveCollidesAgainstInactive(levelIndex,true) to make an inactive object collide with other inactive objects, and then call
	//	phLevelNew::FindAndAddOverlappingPairs(levelIndex)
	DEPRECATED bool AddMovingInstance (int levelIndex);
	DEPRECATED bool AddMovingInstance (const phInst* instance);
	DEPRECATED bool AddActiveInstance (int levelIndex);
	DEPRECATED bool AddActiveInstance (const phInst* instance);

	// PURPOSE: Add an instance behavior to the simulator.
	// PARAMS:
	//	instBehavior - the instance behavior to add
	// NOTES:	Instance behaviors have their own updates and collision handling, for special objects like water and explosions.
	bool AddInstBehavior (phInstBehavior& instBehavior);

	// PURPOSE: Remove an instance behavior and its instance from the simulator.
	// PARAMS:
	//	instBehavior - the instance behavior to remove
	//	forceImmediateDelete - optional flag to skip delayed object removal (default is false)
	// NOTES:	Instance behaviors have their own updates and collision handling, for special objects like water and explosions.
	void RemoveInstanceAndBehavior (phInstBehavior& instBehavior, bool forceImmediateDelete=false);

	// PURPOSE: Remove an instance behavior from the simulator.
	// PARAMS:
	//	instBehavior - the instance behavior to remove
	// NOTES:	Instance behaviors have their own updates and collision handling, for special objects like water and explosions.
	void RemoveInstBehavior (phInstBehavior& instBehavior);

	// PURPOSE: Get the instance behavior for the instance with the given level index.
	// PARAMS:
	//	levelIndex - the level index of the instance behavior's instance
	// RETURN:	a pointer to the instance behavior for the object with the given level index
	phInstBehavior * GetInstBehavior(int LevelIndex, bool needsLock = true) const;
	phInstBehavior * GetInstBehaviorBySearch(int LevelIndex, bool needsLock = true) const;

	bool IsInstBehaviorInArray(phInstBehavior *instBehavior);

    void AddSelfCollision(phInst* instance);

#if HACK_GTA4_FRAG_BREAKANDDAMAGE
	bool ApplyImpetusAndBreakingImpetus(Vec::V3Param128 timeStep, int levelIndex, const Vector3& impetus, const Vector3& position, int component, int element, float breakingImpetus, bool scaleImpulseByMassRatio = false);
	bool ApplyForceAndBreakingForce(Vec::V3Param128 timeStep, int levelIndex, const Vector3& force, const Vector3& position, int component, int element, float breakingForce, bool scaleForceByMassRatio = false);
#endif

	// PURPOSE: Apply the given impulse or force to the object with the given level index.
	// PARAMS:
	//	levelIndex - the physics level index of the object to which to apply the given impetus
	//	impetus - the impulse or force to apply to the object with the given level index
	//	position - the world position at which to apply the impetus
	//	component - optional composite bound part to which to apply the impulse
	//  scaleForceByMassRatio - if a break occurs the force applied to the new objects will be scaled based on their masses compared to the original mass
	//	breakScale - optional scale factor for the impetus to use when computing which parts break
	// RETURN:	true if the impetus applies to a collider (an active object) or a breaking instance, false if it applies to an inactive, non-breaking instance
	bool ApplyImpetus (Vec::V3Param128 timeStep, int levelIndex, const Vector3& impetus, const Vector3& position, int component=0, int element=0, bool isImpulse=true, bool scaleForceByMassRatio=false, float breakScale=1.0f);

	// PURPOSE: Apply the given impulse to the object with the given level index.
	// PARAMS:
	//	levelIndex - the physics level index of the object to which to apply the given impulse
	//	impulse - the impulse to apply to the object with the given level index
	//	position - the world position at which to apply the impetus
	//	component - optional composite bound part to which to apply the impulse
	//	breakScale - optional scale factor for the impulse to use when computing which parts break
	// RETURN:	true if the impulse applies to a collider (an active object) or a breaking instance, false if it applies to an inactive, non-breaking instance
	bool ApplyImpulse (int levelIndex, const Vector3& impulse, const Vector3& position, int component=0, int element=0, float breakScale=1.0f);

	// PURPOSE: Compute and apply the impulse from the given bullet mass and velocity to the object with the given level index.
	// PARAMS:
	//	levelIndex - the physics level index of the object to which to apply the given impulse
	//	bulletMass - the mass of the bullet, from which to compute the impulse
	//	bulletSpeed - the speed of the bullet, from which to compute the impulse
	//	bulletWorldUnitDir - the direction of the bullet in world coordinates
	//	position - the world position at which to apply the impetus
	//	component - optional composite bound part to which to apply the impulse
	//	breakScale - optional scale factor for the impulse to use when computing which parts break
	// RETURN:	true if the impulse applies to a collider (an active object) or a breaking instance, false if it applies to an inactive, non-breaking instance
	bool ApplyBulletImpulse (int levelIndex, ScalarV_In bulletMass, ScalarV_In bulletSpeed, Vec3V_In bulletWorldUnitDir, Vec3V_In worldPosition, int component=0, int element=0, ScalarV_In breakScale=ScalarV(V_ONE));

	// PURPOSE: Compute and apply the impulse from the given bullet mass and velocity to the object with the given level index.
	// PARAMS:
	//	levelIndex - the physics level index of the object to which to apply the given impulse
	//	impulse - the impulse to apply to the object with the given level index
	//	bulletSpeed - the speed of the bullet, from which to compute the impulse
	//	position - the world position at which to apply the impetus
	//	component - optional composite bound part to which to apply the impulse
	//	breakScale - optional scale factor for the impulse to use when computing which parts break
	// RETURN:	true if the impulse applies to a collider (an active object) or a breaking instance, false if it applies to an inactive, non-breaking instance
	bool ApplyBulletImpulse (int levelIndex, Vec3V_In impulse, ScalarV_In bulletSpeed, Vec3V_In worldPosition, int component=0, int element=0, ScalarV_In breakScale=ScalarV(V_ONE));

	// PURPOSE: Apply the given force to the object with the given level index.
	// PARAMS:
	//	levelIndex - the physics level index of the object to which to apply the given force
	//	force - the force to apply to the object with the given level index
	//	position - the world position at which to apply the force
	//	component - optional composite bound part to which to apply the force
	// RETURN:	true if the force applies to a collider (an active object) or a breaking instance, false if it applies to an inactive, non-breaking instance
	bool ApplyForce(Vec::V3Param128 timeStep, int levelIndex, const Vector3& force, const Vector3& position, int component=0, int element=0, float breakScale=1.0f);

	bool ApplyForceScaled(Vec::V3Param128 timeStep, int levelIndex, Vector3& force, const Vector3& position, float mass, int component=0);
	bool ApplyImpetus(Vec::V3Param128 timeStep, int levelIndex, const Vector3& impetus, bool isImpulse=true);
	bool ApplyImpulse(int levelIndex, const Vector3& impulse) { return ApplyImpetus(ScalarV(V_ZERO).GetIntrin128ConstRef(), levelIndex,impulse); }
	bool ApplyForce(Vec::V3Param128 timeStep, int levelIndex, const Vector3& force) { return ApplyImpetus(timeStep, levelIndex,force,false); }
	bool ApplyAngImpulse(int levelIndex, const Vector3& angImpulse);
	bool ApplyTorque(Vec::V3Param128 timeStep, int levelIndex, Vec::V3Param128 torque);
	bool ApplyForceCenterOfMass (int levelIndex, Vec::V3Param128 force, Vec::V3Param128 timestep, float breakScale=1.0f);

	// PURPOSE: Promote the given instance from inactive to active, if it is inactive.
	// PARAMS:
	//	instance - a physics instance to activate
	// RETURN:	the collider for the instance, whether it was already active or newly activated (or NULL if the instance is fixed).
	phCollider* TestPromoteInstance (phInst* instance);

	phContactMgr* GetContactMgr ()										{ return m_ContactMgr; }
	phPool<phManifold>* GetManifoldPool ()								{ return m_ManifoldPool; }
	phPool<phContact>* GetContactPool ()								{ return m_ContactPool; }
	phPool<phCompositePointers>* GetCompositePointerPool ()				{ return m_CompositePointerPool; }
	phConstraintMgr* GetConstraintMgr ()								{ return m_ConstraintMgr; }
	phSleepMgr* GetSleepMgr ()											{ return m_SleepMgr; }
    phOverlappingPairArray* GetOverlappingPairArray ()                  { return m_OverlappingPairArray; }

	// PURPOSE: Set the acceleration due to gravity.
	// PARAMS:
	//	gravity - the acceleration due to gravity
	static void SetGravity (Vec3V_In gravity);
	static void SetGravity (const Vector3& gravity);

	// PURPOSE: Get the acceleration due to gravity.
	// RETURN: the acceleration due to gravity
	static Vec3V_Out GetGravityV ();
	static const Vector3& GetGravity ();

	// PURPOSE: Enable or disable sleep.
	// PARAMS:
	//	enabled - whether to enable or disable sleep
	static void SetSleepEnabled (bool enabled);
	static bool GetSleepEnabled ();

	static void SetDelaySAPAddRemoveEnabled(bool enabled);
	static bool GetDelaySAPAddRemoveEnabled();

	// PURPOSE: Set the minimum thickness that concave bounds are built with in your game
	// NOTES: This is used to disable collisions with the backside of bounds when moving at high speeds. Setting it too large
	//        can result in spurious collisions, while setting it too small can cause fall-throughs.
	static void SetMinimumConcaveThickness(float minThickness);
	static Vector3 GetMinimumConcaveThickness();
	static void UpdateMinConcaveThickness();

    static int GetNumCollisionsPerTask ();

	static void SetShouldFindImpactsEnabled(bool enabled) { sm_ShouldFindImpacts = enabled; }
	static void SetPreComputeImpactsEnabled(bool enabled) { sm_PreComputeImpacts = enabled; }
	static void SetReportMovedBySimEnabled(bool enabled)  { sm_ReportMovedBySim = enabled; }
	static void SetManifoldBoxTestEnabled(bool enabled)   { sm_ManifoldBoxTest = enabled; }
	static void SetSortPairsByCost(bool enabled)		  { sm_SortPairsByCost = enabled; }
	static void SetMaintainLooseOctree(bool enabled);

	static bool GetShouldFindImpactsEnabled() { return sm_ShouldFindImpacts; }
	static bool GetPreComputeImpactsEnabled() { return sm_PreComputeImpacts; }
	static bool GetReportMovedBySimEnabled()  { return sm_ReportMovedBySim; }
	static bool GetManifoldBoxTestEnabled()   { return sm_ManifoldBoxTest; }
	static bool GetMaintainLooseOctree();
	static bool GetSelfCollisionsEnabled()    { return sm_SelfCollisionsEnabled; }

	static int GetMinManifoldPointLifetime() 
	{ 
#if __BANK		
		return sm_MinManifoldPointLifetime;
#else
		return PHSIM_DEFAULT_MIN_POINT_LIFETIME;
#endif
	}
    static bool GetCompositePartSphereTest() { return sm_CompositePartSphereTest; }
    static bool GetCompositePartOBBTest() { return sm_CompositePartOBBTest; }
    static bool GetCompositePartAABBTest() { return sm_CompositePartAABBTest; }

	static void SetColliderUpdateIndex(int value) { sm_ColliderUpdateIndex = value; }

    enum
    {
        Penetration_Minkowski,
        Penetration_GJK_EPA,
        Penetration_Triangle,
        Penetration_Count
    };

    static int GetConcavePenetration() { return sm_ConcavePenetration; }
	static int GetConvexPenetration() { return sm_ConvexPenetration; }	
	
	// PURPOSE: Set the allowed penetration in collisions.
	// PARAMS:	penetration - the penetration to be allowed in collisions
	// NOTES:	Colliding objects are permitted to penetrate up to this amount without being forced or pushed apart, so that collisions will persist across frames.
	static void  SetAllowedPenetration (float penetration)  { sm_AllowedPenetration = penetration; }

	// PURPOSE: Get the allowed penetration in collisions.
	// NOTES:	Colliding objects are permitted to penetrate up to this amount without being forced or pushed apart, so that collisions will persist across frames.
	static float GetAllowedPenetration ()  { return sm_AllowedPenetration; }
	static ScalarV_Out GetAllowedPenetrationV ()  { return ScalarV(sm_AllowedPenetration); }

	// PURPOSE: Set the allowed angle penetration for joint limits.
	// PARAMS:	penetration - the angle penetration to be allowed for joints at limits
	// NOTES:	Joints are permitted to penetrate their limits up to this amount without being forced or pushed back, so that joint limits will persist across frames.
	static void  SetAllowedAnglePenetration (float penetration)  { s_AllowedAnglePenetration = penetration; }

	// PURPOSE: Get the allowed angle penetration for joint limits.
	// NOTES:	Joints are permitted to penetrate their limits up to this amount without being forced or pushed back, so that joint limits will persist across frames.
	static float GetAllowedAnglePenetration ()  { return s_AllowedAnglePenetration; }
	static ScalarV_Out GetAllowedAnglePenetrationV ()  { return ScalarV(s_AllowedAnglePenetration); }

#if (__WIN32 || ENABLE_UNUSED_PHYSICS_CODE)
	static btStackAlloc* GetPerThreadStackAlloc ();
#endif // __WIN32 || ENABLE_UNUSED_PHYSICS_CODE
	void ComputeMaxStackAlloc ();

	// public static math functions (used by the simulator and by contacts)
	static void GetInvMassMatrix (const phInst* instance, const phCollider* collider, const Vector3& sourcePos, Matrix33& outMtx,
									const Vector3* responsePos=NULL, Vector3* localVelocity=NULL, Vector3* angVelocity=NULL);
	static void GetCompositeInvMassMatrix (const phInst* instanceA, const phCollider* colliderA, const phInst* instanceB,
											const phCollider* colliderB, const Vector3& sourcePosA, const Vector3& sourcePosB,
											Matrix33& mtx, const Vector3* responsePosA=NULL, const Vector3* responsePosB=NULL,
											Vector3* localDelVel=NULL);

#if __BANK
	static void ReloadShapeTestELF();
	static void AddWidgets (bkBank& bank);
#endif

	// PURPOSE: Get the matrix of the instance with the given level index on the previous update.
	// PARAMS:
	//	levelIndex - the index of the object in the level
	// RETURN: the matrix of the instance with the given level index on the previous update
	// NOTES:	This gets the last matrix from the collider if the object is active. Otherwise it calls GetLastInstanceMatrix.
	Mat34V_ConstRef  GetLastInstanceMatrix (const phInst* instance) const;

	void SetLastInstanceMatrix(const phInst* inst, Mat34V_In lastMtx);

	bool ColliderIsManagedBySimulator (const phCollider* collider) const;
    bool ColliderIsPermanentlyActive (const phCollider* collider) const;

	// PURPOSE: Called from the force solver when all calcuations are done.
	void ForceSolverDone();

	// PURPOSE: True if phSimulator::Update is in progress
	// NOTES:
	//     - This is used by callbacks e.g. to determine if it is safe to delete objects.
	bool IsUpdateRunning() const;

	// PURPOSE: Can objects be deleted safely at this time?
	bool IsSafeToDelete() const;

	// PURPOSE: control whether objects can be deleted, should not be called from user code
	void SetSafeToDelete(bool safeToDelete);

	// PURPOSE: Is it safe to delete information on manifolds
	bool IsSafeToModifyManifolds() const;

	// PURPOSE: Is breaking allowed
	bool AllowBreaking();

	// PURPOSE: Tell the simulator that this is the last update of the frame.
	// NOTES:	This is only used for multi-stepping physics. For single-stepped physics it is always true.
	void SetLastUpdateThisFrame (bool lastUpdateThisFrame);

	// PURPOSE: Tell if the simulator is running its last update of the frame.
	// RETURN:	true if this is the last of multiple updates in the same frame, false if it is not
	// NOTES:	This is only used for multi-stepping physics. For single-stepped physics it is always true.
	bool LastUpdateThisFrame () const;

	void CollisionActivations();

	void AddActivatingPair(phTaskCollisionPair* pair);

	GJKCacheSystem * GetGJKCacheSystem() { return m_gjkCacheSystem; }

#if __DEV
	void CheckForManifoldReferencesToInst(const phInst* pInst, bool allowEmptyManifolds, bool allowDeactivatedConstraints);
	void DumpManifoldsWithInstance(const phInst* pInst);
#endif // __DEV

protected:

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// UPDATE ROUTINES
#if EARLY_FORCE_SOLVE
	void IntegrateVelocities(ScalarV_In timeStep);
	void RefreshInstAndColliderPointers(float timeStep);
	void VelocitySolve(ScalarV_In timeStep);
	void IntegratePositions(ScalarV_In timeStep);
	void PushSolve(ScalarV_In timeStep, phOverlappingPairArray* pairArray);
	void ApplyPushes(ScalarV_In timeStep);
	phOverlappingPairArray* FindPushPairs(phOverlappingPairArray* pairArray, bool careAboutNeedsCollision);
#else
	virtual void PreCollide (float timeStep);				// returns true on the first frame
	virtual void Collide (float timeStep);
	virtual void PostCollide (float timeStep);
#endif // EARLY_FORCE_SOLVE

	void CoreUpdate(float timeStep, bool finalUpdate);

	void ProcessOverlaps(phOverlappingPairArray* pairArray, btBroadphasePair* prunedPairs, u16* sortList, int numPrunedPairs, Vec::V3Param128 timestep);
	// CollideActive: Returns the next broad phase pair index that needs to be processed. CollideActive can create new broad phase pairs.
	int CollideActive(phOverlappingPairArray* pairArray, u32 startingBroadphasePairIndex, float timeStep);
    void InitiateCollisionJobs(phOverlappingPairArray& pairList, float timeStep);

	void InitiateCommitDeferredOctreeUpdatesTask();
	void FlushCommitDeferredOctreeUpdatesTask();

	void CheckBroadphaseOverlaps(phInst* inst);
	void CheckAllBroadphaseOverlaps();

	void BreakObject (phInstBreakable* pInstance, Vec::V3Param128 timeStep, 
		Vec3V_In worldPosition,	Vec3V_In impulseV, const int iElement, const int iComponent, 
		const bool isForce, bool scaleForceByMassRatio = false, float breakScale=1.0f) const;

	phInstBreakable* FindWeakestInst (phInstBreakable** breakInstList, int numBreakInsts,float* breakStrength, 
		Vec3V_In worldPosA, Vec3V_In worldPosB, Vec3V_In impulseA,const phInst* pInstanceA, const int iComponentA,const phInst* pInstanceB, const int iComponentB,
		phBreakData** breakData, phBreakData** testBreakData) const;

	void ApplyAirResistance(Vec::V3Param128 timeStep, float airDensity=1.0f, const Vector3& windVelocity=Vector3(0.0f,0.0f,0.0f));

	void DeleteObjectHelper (int levelIndex);

protected:

	// PURPOSE: control whether manifolds can be modified or not
	void SetSafeToModifyManifolds(bool safeToModifyManifolds);

	void SetMaxInstBehaviors(u16 maxInstBehaviors);
	void AllocateInstBehaviors();

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SANITY_CHECK
	void SanityCheckInstanceBehaviorArrays() const;
#endif

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX
	int FindInstanceBehaviorInternal(int levelIndex) const;
	void SortInstanceBehaviors() const;
#endif

	void InitPhysicsEffects();
	void CopyDataToPhysicsEffects();
	void PhysicsEffectsStep(float timeStep);
	void CopyDataFromPhysicsEffects();

	void FillInColliderUpdateParams(sysTaskParameters& params, int updateType, float timeStep);

	static phSimulator * sm_ActiveInstance;								// the currently active simulator

	phLevelNew * m_Level;

	phContactMgr * m_ContactMgr;										// the contact manager used by this simulator

	phPool<phManifold> * m_ManifoldPool;

#if __PS3
	phManifold::DmaPlan* m_ManifoldDmaPlanPool;
	phCollider::DmaPlan* m_ColliderDmaPlanPool;
#endif

	phPool<phContact> * m_ContactPool;

	phPool<phCompositePointers> * m_CompositePointerPool;

	phConstraintMgr * m_ConstraintMgr;

	phSleepMgr * m_SleepMgr;

	phOverlappingPairArray* m_OverlappingPairArray;

#if EARLY_FORCE_SOLVE
	phOverlappingPairArray* m_PushPairsA;
	phOverlappingPairArray* m_PushPairsB;
#endif // EARLY_FORCE_SOLVE

	static const int MAX_NUM_ACTIVATING_MANIFOLDS = 256;
	phTaskCollisionPair* m_ActivatingPairs[MAX_NUM_ACTIVATING_MANIFOLDS];
	int m_NumActivatingPairs;

	bool m_IsSafeToModifyManifolds;

	// PURPOSE: True if phSimulator::Update is in progress
	bool m_UpdateRunning;

	// PURPOSE: Can objects be deleted safely at this time?
	bool m_SafeToDelete;

	// PURPOSE: true if the simulator's last update of the frame is in progress
	// NOTES:	This is used when there is more than one physics update per frame. For single-stepped physics this is always true.
	bool m_LastUpdateThisFrame;

	// PURPOSE: the acceleration due to gravity
	static Vector3 sm_Gravity;

	// PURPOSE: whether or not sleep is enabled (default is true)
	static bool sm_SleepEnabled;
	static bool sm_EnablePushes;
	static bool sm_AlwaysPush;
	static bool sm_ValidateArchetypes;
    static bool sm_ColliderUpdateEnabled;
	static bool sm_ComputeForces;
#if __BANK
	static bool sm_UseOctreeUpdateTask;
#endif	// __BANK
	static bool sm_PhysicsEffectsSolver;
	static bool sm_PhysicsEffectsSPU;
	static bool sm_DelayCollisionActivations;
	static bool sm_ShouldFindImpacts;
	static bool sm_PreComputeImpacts;
	static bool sm_ReportMovedBySim;
	static bool sm_CheckAllPairs;
	static bool sm_ManifoldBoxTest;
	static bool sm_SortPairsByCost;
	static bool sm_delaySAPAddRemove;
	static Vector3 sm_MinConcaveThickness;
	static bool sm_SelfCollisionsEnabled;
	static bool sm_InitClassCalled;
	static float sm_AllowedPenetration;
	static float s_AllowedAnglePenetration;

#if PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY && __BANK
	static bool sm_GetInstBehaviorFromArray;
#endif // PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY

    static int sm_NumCollisionsPerTask;

	static int sm_ColliderUpdateIndex;

#if __BANK
	static int sm_MinManifoldPointLifetime;
#endif
    static bool sm_CompositePartSphereTest;
    static bool sm_CompositePartOBBTest;
    static bool sm_CompositePartAABBTest;
	static int sm_ConcavePenetration;
    static int sm_ConvexPenetration;
    static bool sm_SOLIDPenetrationLicenceWarning;

	enum eColliderFlags
	{
		COLLIDERFLAG_PERMANENTLYACTIVE	= 1 << 0,
	};

	int m_MaxManagedColliders;											// The maximum number of colliders that will be managed by the physics simulator.  Note that this cannot be greater
																		//   than the maximum number of active objects allowed in the level, but it can be less.
	int m_NumUsedManagedColliders;										// The number of colliders that are in actual use in the physics level right now.  Note that this may be less than
																		//   the number of active objects in the physics level right now because colliders that are not from the simulator
																		//   may also be in the level too.
	phCollider * m_ManagedColliders;
	u8 * m_ManagedColliderFlags;										// Right now these flags are only used to track whether a given collider is permanently active or not, and this is
																		//   only tracked for colliders that we are managing.  Any non-managed collider is automatically not eligible for
																		//   automatic deactivation.
	phSleep * m_ManagedSleeps;
	u16 * m_AvailableColliderIndices;

	phInstBehavior ** m_InstBehaviors;									// An array of pointers to instance behaviors that are supplied by clients.  The first m_NumInstBehaviors of these are

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE
	u16*	m_OptInstBehaviorLevelIndices;									// Array parallel to m_InstBehaviors, with m_MaxInstBehaviors elements, to store level indices of the instance behaviors.
#endif	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX
	mutable bool m_OptInstBehaviorsNeedSort;								// If true, m_InstBehaviors is currently not (known to be) sorted.
#endif	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX
																		// valid and they are sorted by level index.
	u16 m_NumInstBehaviors;												// The number of instance behaviors pointed to in m_InstBehaviors.
	u16 m_MaxInstBehaviors;												// The maximum number of instance behaviors.

#if PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY
	phInstBehavior** m_InstBehaviorByLevelIndex;
#endif // PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY

	AllCollisionsDoneCallback m_AllCollisionsDoneCallback;
	PreComputeAllImpactsFunc m_PreComputeAllImpactsFunc;
	PreApplyAllImpactsFunc m_PreApplyAllImpactsFunc;
	AllowBreakingCallback m_AllowBreakingCallback;

	atArray< phInst *> m_AddObjectDelayedSAPInst;
	atArray< int > m_AddObjectDelayedSAPLevelIndex;

	atArray< int > m_RemoveObjectDelayedSAPLevelIndex;

	static const int MAX_ARTICULATED_COLLIDERS = 256;

	phWorkerThread* m_CollisionTasks;
	phWorkerThread* m_UpdateCollidersTasks;
	phWorkerThread* m_UpdateCollidersPostTasks;
#if USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
	phWorkerThread* m_CommitDeferredOctreeUpdatesTask;
#endif	// USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK

	GJKCacheSystem * m_gjkCacheSystem;

#if __BANK
	void UpdateTestForces(float timeStep);
	void DumpActivatingPairs();

	static bool sm_ApplyTestForce;
	static int	sm_TestForceLevelIndex;
	static int sm_TestForceComponent;
	static float sm_TestForceMassMultiplier;
	static bool sm_IsTestForceModelSpace;
	static bool sm_ApplyTestForceToCenter;
	static float sm_fTestForceRange;
	static Vector3 sm_TestForceScale;
	static Vector3 sm_TestForceLoc;
#endif

#if __DEV
	static const int sm_DebugLevel;
#endif

public:
	void CollisionIntersection (phManifold* rootManifold, phCollisionMemory * collisionMemory);

    void CollisionSelfIntersection(phInst *pInst1, phManifold* manifold);

private:
    static void ProcessPairListTask(sysTaskParameters &p);
#if USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
    static void CommitDeferredOctreeUpdatesTask(sysTaskParameters &p);
#endif	// USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
    void FlushCollisionJobs(phOverlappingPairArray* pairArray, float timeStep);
};


inline phInstBehavior* phSimulator::GetInstBehavior(int levelIndex, bool BANK_ONLY(needsLock) /* = true */) const
{
#if PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY
#if __BANK
	if (sm_GetInstBehaviorFromArray)
#endif	// !__FINAL
	{
		return m_InstBehaviorByLevelIndex[levelIndex];
	}
#endif // PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY

#if __BANK
	return GetInstBehaviorBySearch(levelIndex, needsLock);
#endif	// !__FINAL
}


inline bool phSimulator::IsInstBehaviorInArray(phInstBehavior *instBehavior)
{
	for (int InstBehaviorIndex = 0; InstBehaviorIndex < m_NumInstBehaviors; ++InstBehaviorIndex)
	{
		if(m_InstBehaviors[InstBehaviorIndex] == instBehavior)
			return true;
	}

	return false;
}

inline phSimulator::AllCollisionsDoneCallback phSimulator::GetAllCollisionsDoneCallback() const
{
	return m_AllCollisionsDoneCallback;
}


inline phSimulator::PreComputeAllImpactsFunc phSimulator::GetPreComputeAllImpactsFunc() const
{
	return m_PreComputeAllImpactsFunc;
}


inline phSimulator::PreApplyAllImpactsFunc phSimulator::GetPreApplyAllImpactsFunc() const
{
	return m_PreApplyAllImpactsFunc;
}

inline phSimulator::AllowBreakingCallback phSimulator::GetAllowBreakingCallback() const
{
	return m_AllowBreakingCallback;
}

// Get a collider by level index.
__forceinline phCollider *phSimulator::GetCollider(const int knLevelIndex) const
{
	// I'd much rather require valid level indices to be passed in here, but that will require a lot of fixes in a lot of code...
	// FastAssert(knLevelIndex != phInst::INVALID_INDEX);
	if (knLevelIndex == phInst::INVALID_INDEX)
	{
		return NULL;
	}

	phCollider* collider = static_cast<phCollider*>(m_Level->GetActiveObjectUserData(knLevelIndex));
	Assert(m_Level->IsActive(knLevelIndex) && collider || !m_Level->IsActive(knLevelIndex) && !collider);
	return collider;
}

// Get a collider for the given instance.
__forceinline phCollider *phSimulator::GetCollider(const phInst* instance) const
{
	return GetCollider(instance->GetLevelIndex());
}

inline void phSimulator::ForceSolverDone()
{
	m_UpdateRunning = false;
	m_SafeToDelete = true;
}

inline bool phSimulator::IsUpdateRunning() const
{
	return m_UpdateRunning;
}

inline bool phSimulator::IsSafeToDelete() const
{
	return m_SafeToDelete;
}

inline void phSimulator::SetSafeToDelete(bool safeToDelete)
{
	m_SafeToDelete = safeToDelete;
}

inline bool phSimulator::IsSafeToModifyManifolds() const
{ 
	return m_IsSafeToModifyManifolds; 
}

inline void phSimulator::SetSafeToModifyManifolds(bool isSafeToModifyManifolds)
{ 
	m_IsSafeToModifyManifolds = isSafeToModifyManifolds; 
}

inline bool phSimulator::AllowBreaking()
{
	return m_AllowBreakingCallback();
}

inline void phSimulator::SetLastUpdateThisFrame (bool lastUpdateThisFrame)
{
	m_LastUpdateThisFrame = lastUpdateThisFrame;
}

inline bool phSimulator::LastUpdateThisFrame () const
{
	return m_LastUpdateThisFrame;
}

inline void phSimulator::AddActivatingPair(phTaskCollisionPair* pair)
{
	if (Verifyf(m_NumActivatingPairs < MAX_NUM_ACTIVATING_MANIFOLDS, "Ran out of activating pairs!"))
	{
		m_ActivatingPairs[m_NumActivatingPairs++] = pair;
	}
#if __BANK
	else
	{
		DumpActivatingPairs();
	}
#endif // __BANK
}


} // namespace rage

#endif
