//
// physics/inst.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_INST_H
#define PHYSICS_INST_H

#include "archetype.h"

#include "contactiterator.h"

#include "debugevents.h"

#include "paging/ref.h"

#include "vectormath/classes.h"

// DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK, when enabled, will cause all calls made to phInst.SetMatrix() to have the current callstack stored in an
// array which can be accessed via phInst.GetLastSetMatrixCallstack() for use with sysStack::PrintCapturedStackTrace().
// For an example of this, please refer to phLevelNew::CreateChild() which detects this #define and emits the callstack
// to the TTY when it encounters a problem with a specific phInst.
// This will add a widget for controlling whether or not to track the stacks at run time (default is on if compiled in via
// this #define).
#define DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK 0 

// DEBUG_PHINST can be enabled whenever the need for detecting or monitoring all SetMatrix calls on a phInst are needed.
// Note that it requires __BANK to be enabled for it to work due to the widgets it adds.
#define DEBUG_PHINST ((0 || DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK) && __BANK)

#define PHINST_USER_DATA_IN_MATRIX_W_COMPONENT 1

#define PHINST_VALIDATE_POSITION (__BANK)
#if PHINST_VALIDATE_POSITION
#include "system/stack.h"
#define PHINST_VALIDATE_POSITION_ONLY(X) X
#if !__SPU
#define PHINST_SCOPED_DISABLE_VALIDATE_POSITION phInst::ScopedDisableValidatePosition disableInstValidatePosition
#else // !__SPU
#define PHINST_SCOPED_DISABLE_VALIDATE_POSITION 
#endif // !__SPU
#else // PHINST_VALIDATE_POSITION
#define PHINST_VALIDATE_POSITION_ONLY(X)
#define PHINST_SCOPED_DISABLE_VALIDATE_POSITION 
#endif //  PHINST_VALIDATE_POSITION

namespace rage {

class bkBank;
class datResource;
class phCollider;
class phConstraintBase;
class crSkeleton;

////////////////////////////////////////////////////////////////
// phInst

// PURPOSE
//   Basic physics instance for representing any physical object.  It contains a Matrix34 for orientation and
//   position in the world, flags (some used by the physics simulator and some available for project use) and a pointer
//   to a phArchetype, which contains physical information.
// NOTES
//   - Physics instance classes that can break, change physical parameters, or add other functionality are derived from
//     this class.
//   - Most game projects derive their own physics instances from phInst or one of its derived classes in the physics project.
//   - The data members AlighmentPad0 and AlignmentPad1 are slack space forced by the 16-byte alignment of the matrix data
//     and are named here so they can be accessed by higher level code if desired.  They are not used by RAGE code.
// <FLAG Component>
class phInst: public pgBase
{
public:
	////////////////////////////////////////////////////////////
	// instance type info
	enum																// RAGE-level phInst object enums
	{
		PH_INST,														// phInst, RAGE base phInst object
		PH_INST_BREAKABLE,												// phInstBreakable, base-type for "static" objects
		PH_INST_SWAPABLE,												// phInstSwapable, an instance with two bounds, one active
		PH_INST_GLASS,													// phGlassInst, an already broken shard from a piece of glass
		PH_INST_RAGE_LAST = PH_INST_GLASS								// the last RAGE type, higher level code defines from here
	};
	PH_NON_SPU_VIRTUAL int GetClassType () const									{ return PH_INST; }

	////////////////////////////////////////////////////////////
#if !__SPU
	phInst ();															// constructor
	phInst (datResource & rsc);											// constructor
	PH_NON_SPU_VIRTUAL ~phInst ();													// destructor
#endif // !__SPU

	DECLARE_PLACE(phInst);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct& s);
#endif

#if !__SPU
	static void SetDtorCallback(void (*callback)(phInst *inst));
	static void SetArchetypeChangeCallback(void (*callback)(phInst *inst, phArchetype *newArchetype));
#endif

	// PURPOSE: Set the phArchetype pointer and add a reference count to it, and set the matrix for this instance.
	// PARAMS:
	//	archetype	- the physics archetype for this instance
	//	mtx			- the world position and orientation for this instance
	void Init (phArchetype & archetype, const Matrix34 & mtx);			// initialize the instance

	////////////////////////////////////////////////////////////
	// accessors
	phArchetype * GetArchetype ()										{ return m_Archetype; }
	const phArchetype * GetArchetype () const							{ return m_Archetype; }

#if __SPU
	phArchetype ** GetArchetypePtr ()									{ return &m_Archetype.ptr; }
	const phArchetype * const* GetArchetypePtr () const					{ return &m_Archetype.ptr; }
#endif

	// PURPOSE: Set the phArchetype pointer and add a reference count to it.
	// PARAMS:
	//	archetype - phArchetype with physical information.
	//	deleteAtZero - optional to tell whether to delete the existing archetype if the new one is NULL and this is the last reference to the existing one
	// NOTES:
	//	- A NULL archetype pointer argument will remove the current archetype.
	void SetArchetype (phArchetype* archetype, bool deleteAtZero=true);

	// PURPOSE: Get the position and orientation matrix for this instance.
	// RETURN:	a reference to the position and orientation matrix for this instance
	Mat34V_ConstRef GetMatrix () const;

	// PURPOSE: Set the position and orientation matrix for this instance.
	// PARAMS:
	//	mtx - the new position and orientation matrix for this instance
	void SetMatrix (Mat34V_In mtx);
	void SetMatrixNoZeroAssert (Mat34V_In mtx);

	// PURPOSE: Get the position of this instance in world coordinates.
	// RETURN:	the position of this instance in world coordinates
	Vec3V_ConstRef GetPosition () const;

	// PURPOSE: Set the position of this instance in world coordinates.
	// PARAMS:
	//	position - the new position for this instance
	void SetPosition (Vec3V_In position);

	// PURPOSE: Get this physics instance's level index.
	// RETURN:	the index number of this instance in the physics level
	// NOTES:	A level index of phInst::INVALID_INDEX (65535) means that this instance is not in the physics level.
	u16 GetLevelIndex () const;

	// PURPOSE: Sett his physics instance's level index.
	// PARAMS:
	//	levelIndex - the new index for this instance in the physics level
	// NOTES:	A level index of phInst::INVALID_INDEX (65535) means that this instance is not in the physics level.
	void SetLevelIndex (u16 levelIndex);

	// PURPOSE: Get the user data for this physics instance.
	// RETURN:	the user data for this physics instance
	// NOTES:	The user data can be any pointer.
	void* GetUserData () const;

	// PURPOSE: Set the user data for this physics instance.
	// PARAMS:
	//	userData - the user data for this physics instance
	// NOTES:	The user data can be any pointer.
	void SetUserData (void* userData);

	// PURPOSE:	Get the flags for this instance.
	// RETURN:	this instance's flags
	// NOTES:	The instance flags are eInstFlags and they are used for controlling ways in which the object collides. Some are available for game use.
	u32 GetInstFlags () const;

	// PURPOSE:	Set the flags for this instance.
	// PARAMS:
	//	flags - this instance's flags
	// NOTES:	The instance flags are eInstFlags and they are used for controlling ways in which the object collides. Some are available for game use.
	void SetInstFlags (u16 flags);

	// PURPOSE:	Get one or more of the flags for this instance.
	// PARAMS:
	//	mask - set of bits to determine which flags to get
	// RETURN:	this instance's flags within the given mask
	// NOTES:	The instance flags are eInstFlags and they are used for controlling ways in which the object collides. Some are available for game use.
	u32 GetInstFlag (u16 mask) const;

	// PURPOSE: Set the instance flags.
	// PARAMS:
	//	mask - a u16 with bits turned on for the particular flag(s) to be modified
	//	value - boolean for the value to be given to the flag bits specified in mask
	void SetInstFlag (u16 mask, bool value);

	bool HasLastMatrix() const;

	void Copy (const phInst & inst);

#if __PFDRAW
	// No skeleton - generally found from a fragInst
	PH_NON_SPU_VIRTUAL crSkeleton* GetSkeleton() const
	{
		return NULL;
	}
#endif

	// PURPOSE: Get the matrix on the instance on the previous frame, for classes derived from phInst
	//			that store the last matrix.
	// RETURN:	A reference to the instance's current matrix; this is overridden in derived classes to return
	//			a reference to the matrix on the previous frame.
	// NOTES:
	//	- This virtual function is the same as ReadMatrix() phInst.
	//PH_NON_SPU_VIRTUAL Mat34V_ConstRef GetLastMatrix () const;

	//PH_NON_SPU_VIRTUAL void SetLastMatrix (Mat34V_In UNUSED_PARAM(last))		{}
	//void UpdateLastMatrix ()											{ SetLastMatrix(m_Matrix); }

	// PURPOSE: Get the velocity of this object, for classes derived from phInst that can move.
	// RETURNS:
	//	the velocity of the instance as controlled by external forces
	// NOTES:
	//	1. This function is intended to be overridden by a derived class that implements moving
	//	inactive objects. For example, traffic has been implemented using this, where the cars
	//	move along a rail and are not physically simulated until they become active by colliding
	//	with something.
	//	2. If this function is not correct in a derived class, you will see strange behavior after
	//	colliding with an inactive object, such as the object suddenly stopping or the collision
	//	not being realistic.
	PH_NON_SPU_VIRTUAL Vec3V_Out GetExternallyControlledVelocity () const;

	// PURPOSE: Calculate the angular velocity of this object, for classes derived from phInst that can move.
	// RETURNS:
	//	the angular velocity of the instance as controlled by external forces
	// Notes:
	//	1. This function is intended to be overridden by a derived class that implements moving
	//	inactive objects. For example, traffic has been implemented using this, where the cars
	//	move along a rail and are not physically simulated until they become active by colliding
	//	with something.
	//	2. If this function is not correct in a derived class, you will see strange behavior after
	//	colliding with an inactive object, such as the object suddenly stopping or the collision
	//	not being realistic.
	PH_NON_SPU_VIRTUAL Vec3V_Out GetExternallyControlledAngVelocity () const;

	// PURPOSE: Get the externally controlled angular velocity of the given component in world space
	// PARAMS: 
	//  component - index of component to get angular velocity of
	// RETURN: The world space angular velocity of the given component (including the instances external velocity)
	Vec3V_Out GetExternallyControlledAngVelocity (int component) const;

	// PURPOSE: Get the externally controlled velocity of a point on a given component
	// PARAMS:
	//	position - world space position, required to compute the local velocity on component
	//	component - index of component the position is on
	// RETURN: The world space velocity of the given position NOT including the angular velocity of the instance
	Vec3V_Out GetExternallyControlledVelocity (Vec3V_In position, int component) const;

	// PURPOSE: If this returns a non-zero value we will add the internal motion of components to the external velocity we compute
	// RETURNS: The time it took components to move from their last matrices to their current matrices. 
	// NOTE: This is a bit hacky to use this as a bool. Maybe we could have a phInst flag for it but there are none available. 
	PH_NON_SPU_VIRTUAL ScalarV_Out GetInvTimeStepForComponentExternalVelocity () const;

	// PURPOSE: Calculate the velocity of this object at the given world location, for classes derived
	//			from phInst that can move.
	// PARAMS:
	//	position - the world position at which to get the object's velocity
	//	component - optional bound part index for use by instances with composite bounds with moving parts,
	//				such as animating creatures or props
	// RETURNS:
	//  the local velocity of the object at the specified point
	// NOTES:
	//	This function is intended to be overridden by a derived class that implements inactive
	//	objects with internal motion. For instance, a character with animating limbs could be
	//	created using a composite bound. You would then want to override this function to ensure
	//	that collisions with its limbs respect their motion.
	PH_NON_SPU_VIRTUAL Vec3V_Out GetExternallyControlledLocalVelocity (Vec::V3Param128 position, int component=0) const;

	// PURPOSE: Tells whether m_Matrix and LastMatrix have the same memory address, for classes derived
	//			from phInst that store a previous matrix.
	// RETURN: true if ReadLastMatrix and GetMatrix return references to different matrices, false if
	//			they return references to the same matrix.
	// Notes:
	//	- This method always returns false in phInst.
	bool CanMove () const;

	PH_NON_SPU_VIRTUAL bool ClassTypeBreakable () const								{ return false; }

	// PURPOSE: Callback to control whether this object can break.
	// RETURNS: true if the object can break, false if it cannot.
	// PARAMS: 
	//     otherInst - The other object hitting the inst to cause it to break, or NULL if it is being broken by an impulse.
	PH_NON_SPU_VIRTUAL bool IsBreakable (phInst* UNUSED_PARAM(otherInst)) const		{ return false; }
	bool IsInLevel () const															{ return (m_LevelIndex!=INVALID_INDEX); }

	// PURPOSE: Apply the given impulse to the instance.
	// PARAMETERS:
	//	impulse		- the impulse to apply
	//	position	- the position in world space at which to apply the impulse
	//	component	- optional composite bound part number (only used when the instance has a composite bound)
	//	push		- optional push to apply
	// NOTES:
	//	1.	This does nothing in phInst. It is intended for use in derived instance classes that can react to impulses.
	PH_NON_SPU_VIRTUAL void ApplyImpulse (const Vector3& impulse, const Vector3& position, int component=0, int element=0, const Vector3* push=NULL, float breakScale=1.0f);

	// PURPOSE: Apply the given impulse to the instance at the location of the instance.
	// PARAMETERS:
	//	impulse		- the impulse to apply
	// NOTES:
	//	1.	This does nothing in phInst. It is intended for use in derived instance classes that can react to impulses.
	//	2.	This calls ApplyImpulse with m_Matrix.d as the world position.
	PH_NON_SPU_VIRTUAL void ApplyImpulse (const Vector3& impulse, float breakScale=1.0f);

	// PURPOSE: Apply the given force to the instance.
	// PARAMETERS:
	//	force		- the force to apply
	//	position	- the position in world space at which to apply the force
	//	component	- optional composite bound part number (only used when the instance has a composite bound)
	// NOTES:
	//	1.	This does nothing in phInst. It is intended for use in derived instance classes that can react to forces.
	PH_NON_SPU_VIRTUAL void ApplyForce (const Vector3& force, const Vector3& position, int component=0, int element=0, float breakScale=1.0f);

	// PURPOSE: Apply the given angular impulse to the instance.
	// PARAMETERS:
	//	angImpulse		- the angular impulse to apply
	// NOTES:
	//	1.	This does nothing in phInst. It is intended for use in derived instance classes that can react to angular impulses.
	PH_NON_SPU_VIRTUAL void ApplyAngImpulse (const Vector3& UNUSED_PARAM(angImpulse)) {}

	// PURPOSE: Apply the given torque to the instance.
	// PARAMETERS:
	//	torque		- the torque to apply
	// NOTES:
	//	1.	This does nothing in phInst. It is intended for use in derived instance classes that can react to torques.
	PH_NON_SPU_VIRTUAL void ApplyTorque (const Vector3& UNUSED_PARAM(torque)) {}

	// PURPOSE: Apply the given impulse to the instance and change its motion immediately (instead of accumulating impulse).
	// PARAMETERS:
	//	impulse		- the impulse to apply
	//	position	- the position in world space at which to apply the impulse
	//	component	- optional composite bound part number (only used when the instance has a composite bound)
	// NOTES:
	//	1.	This does nothing in phInst. It is intended for use in derived instance classes that can react to impulses.
	PH_NON_SPU_VIRTUAL void ApplyImpulseChangeMotion (const Vector3& impulse, const Vector3& position, int component=0, int element=0, float breakScale=1.0f);

	// PURPOSE: Apply the force or impulse in the given impact data to the instance.
	// PARAMETERS:
	//	impactData	- the phImpactData with the impetus, position and component in object A
	void ApplyImpetus( Vec3V_In worldPosA, Vec3V_In impulseA, const int iElementA, const int iComponentA, const bool isForce, float breakScale=1.0f);

	// PURPOSE: Notify the instance that an impulse was applied.
	// PARAMETERS:
	//	impulse		- the impulse that was applied
	//	position	- the position in world space at which the impulse was applied
	//	component	- optional composite bound part number (only used when the instance has a composite bound)
	// Notes:
	//	1.	This does nothing in phInst; it is intended for use in derived instance classes that can react to impulses,
	//		such as by accumulating damage.
	//	2.	This is called on every impulse application on this instance or on its collider (if it is active).
	PH_NON_SPU_VIRTUAL void NotifyImpulse (const Vector3& UNUSED_PARAM(impulse), const Vector3& UNUSED_PARAM(position), int UNUSED_PARAM(component)=0, int UNUSED_PARAM(element)=0, float UNUSED_PARAM(breakScale)=1.0f) {}

	// PURPOSE: Notify the instance that a force was applied.
	// PARAMETERS:
	//	force		- the force that was applied
	//	position	- the position in world space at which the force was applied
	//	component	- optional composite bound part number (only used when the instance has a composite bound)
	// Notes:
	//	1.	This does nothing in phInst; it is intended for use in derived instance classes that can react to forces,
	//		such as by accumulating damage.
	//	2.	This is called on every force application on this instance or on its collider (if it is active).
	PH_NON_SPU_VIRTUAL void NotifyForce (const Vector3& UNUSED_PARAM(force), const Vector3& UNUSED_PARAM(position), int UNUSED_PARAM(component)=0, int UNUSED_PARAM(element)=0, float UNUSED_PARAM(breakScale)=1.0f) {}

	PH_NON_SPU_VIRTUAL void NotifyOutOfWorld () {}

	PH_NON_SPU_VIRTUAL int GetNMAgentID() const { return -1; }

	enum {INVALID_INDEX=0xFFFF};										// magic number indicating not in a level

	enum eInstFlags
	{
		FLAG_OPTIONAL_ITERATIONS = (1 << 0),						// optional force solver iterations reqested by game
		FLAG_DONT_WAKE_DUE_TO_ISLAND = (1 << 1),					// Don't wake up if your sleep island wakes up
		FLAG_HIGH_PRIORITY = (1 << 2),								// manifolds are allocated with "high priority"
		FLAG_NEVER_ACTIVATE = (1 << 3),								// this inst should never activate (return NULL from PrepareForActivation)
		FLAG_NO_GRAVITY = (1 << 4),	
		// Added a gta specific flag - would like this move across to rage\dev if possible
		// Don't think it would cause any problems
		// Another new flag has already been added on the main branch, so might need to add some extra flag bits
		// in the future?
		FLAG_NO_GRAVITY_ON_ROOT_LINK = (1 << 5),					// don't apply gravity to the root link of articulated collider (gta specific)
		FLAG_QUERY_EXTERN_VEL = (1<<6),								// A derived class of this inst overrides one of the GetExternallyControlled_xxx_Velocity() functions and wants it to be used by the impulse solver. This can be changed at runtime if an inst goes to or from active/inactive.
		FLAG_INTERNAL_USE_ONLY_LAST_MTX = (1<<7),					// Has a last matrix when in the level. Only to be set by the level.
		FLAG_SLEEPS_ALONE = (1 << 8),								// Doesn't go to sleep using islands
		FLAG_USER_0     = (1 << 9)									// first user defined flag bit
	};

	////////////////////////////////////////////////////////////
	// physics interfaces

	// Purpose: General-purpose virtual function for classes derived from phInst.  This is called by the physics
	//  simulator whenever an intersection of its bounding sphere with the bounding sphere of another instance
	//  is detected, before it calculates any geometric intersection information, and it gives either instance a
	//  chance to abort the collision process.
	//  No action should be taken in this function other than simply determining whether to abort the collision
	//  or not.
	PH_NON_SPU_VIRTUAL bool ShouldFindImpacts(const phInst* UNUSED_PARAM(otherInst)) const
	{
		return true;
	}
	DEPRECATED PH_NON_SPU_VIRTUAL bool DoCollision() const
	{
		return true;
	}

	DEPRECATED PH_NON_SPU_VIRTUAL bool PreImpactCheck() 
	{
		return true;
	}

	// Purpose: General-purpose virtual function for classes derived from phInst.  This is called by the
	//	physics simulator whenever a collision is detected with another instance, before it calculates any
	//  response for the collision.  Intent is that derived instances might want to do something before
	//  a collision response is calculated but after they know that it is going to take place and the geometric
	//  intersection information is available.
	// Parameters:
	//	impactList - the collision information
	//	hitInst - the other instance in the collision
	// Return: None.
	// Notes:
	//	- This virtual function takes no action in phInst.
	PH_NON_SPU_VIRTUAL void PreComputeImpacts (phContactIterator UNUSED_PARAM(impacts))
	{
	}
	DEPRECATED PH_NON_SPU_VIRTUAL void PreCalcImpact ()
	{
	}

	PH_NON_SPU_VIRTUAL void ReportMovedBySim () {}									// called when a phInst has had its matrix updated by the simulator

#if !__SPU
	// PURPOSE: Callback called prior to activation, also allowing cancellation of activation.
	// PARAMS:
	//     colliderToUse - Put your collider pointer in here if you wish to ask for it to be used instead of a managed collider.
	//     otherInst - The other inst that is colliding with this inst to activate it, or NULL if the activation is not due to collision.
	// NOTES:
	//   - This will be called prior to activation.  It should do any necessary prep work such as providing a new instance to use,
	//     and creating/initializing/supplying a specific collider to use if necessary.
	//   - If activation is not possible or not permitted, it should return NULL.
	PH_NON_SPU_VIRTUAL phInst* PrepareForActivation(phCollider** UNUSED_PARAM(colliderToUse), phInst* UNUSED_PARAM(otherInst), const phConstraintBase * UNUSED_PARAM(constraint))
	{
		if(GetInstFlag(FLAG_NEVER_ACTIVATE))
		{
			return NULL;
		}
		return this;
	}
#endif

	PH_NON_SPU_VIRTUAL void OnActivate(phInst* UNUSED_PARAM(otherInst)) {}

	// PURPOSE: Give instances notice that they're going to be deactivated for any reason.
	// PARAMS:
	//  colliderManagedBySim	- whether or not the collider for this instance is managed by the simulator (if not, it is probably a specialized derived collider)
	// RETURN: whether or not the deactivation should be permitted
	// NOTES:
	//  Sometimes instances will have special things that they need to do when they become inactive, such as return a collider to pool.  This allows
	//		them to reliably take care of those things.
	//  If you did get have to do anything special to get your collider then you don't need to do anything special with it in here.
	//	This returns true when the instance's collider is managed by the simulator, so that managed colliders can be recycled and the instance can be deactivated.
	//		it returns false when the instance's collider is not managed by the simulator, because otherwise the derived collider would likely be lost, and the
	//		instance could later be reactivated with a regular collider.
	PH_NON_SPU_VIRTUAL bool PrepareForDeactivation(bool colliderManagedBySim, bool UNUSED_PARAM(forceDeactivate))
	{
		return colliderManagedBySim;
	}
	
	PH_NON_SPU_VIRTUAL void OnDeactivate() {}

	// PURPOSE: Get the inverse mass matrix for the instance.
	// PARAMS:
	//	sourcePos	- the application position
	//	outMtx		- the inverse mass matrix, filled in by this method
	//	responsePos	- optional pointer to the response position
	// NOTES:
	//	1.	The inverse mass matrix is the matrix by which one multiplies an impulse to get the resulting change in
	//		velocity at the same position or at the response position.
	//	2.	This is intended for overriding in derived instance classes that can react to collisions with or without becoming
	//		active (such as animating creatures with physical reactions or otherwise derformable objects).
	PH_NON_SPU_VIRTUAL void GetInvMassMatrix (Matrix33& outMtx, const Vector3& sourcePos, const Vector3* responsePos=NULL,
		int sourceComponent=0, int responseComponent=0, const Vector3& sourceNormal=XAXIS) const;

#if HACK_GTA4
	// PURPOSE: phContact stores a contact response impulse that is applied in the force solver.
	//			This impulse can be used directly in the fragment breaking code to determine if a fragment will 
	//			break apart due to a contact response.  Alternatively, we might want to modify the impulse used to 
	//			determine if a fragment breaks apart in order to satisfy game designers.  
	//			This functions allows us to modify the impulse that is used to determine if a fragment breaks.
	// PARAMS:	
	//  contact - the contact whose response impulse we will be analysing.
	//	otherInst - the other instance of the contact (we've got a contact between "this" phInst and otherInst).
	// RETURN:	the magnitude of the impulse that is used for the fragment breaking test (the impulse used in the 
	//			fragment breaking test will be contact.GetImpulse()*returnValue/|contact.GetImpulse()|. A return value
	//			of -1 denotes that the impulse used for the fragment breaking test will be contact.GetImpulse().
	PH_NON_SPU_VIRTUAL ScalarV_Out ModifyImpulseMag (int UNUSED_PARAM(myComponent), int UNUSED_PARAM(otherComponent), int UNUSED_PARAM(numComponentImpulses), ScalarV_In UNUSED_PARAM(impulseMagSquared), const phInst* UNUSED_PARAM(otherInst)) const
	{
		return ScalarV(V_NEGONE);
	}
#endif

	// PURPOSE: Get the center of mass of the instance in world coordinates
	// RETURN: the center of mass of the instance in world coordinates
	// NOTES:	Center of mass and center of gravity are the same thing.
	Vec3V_Out GetCenterOfMass () const;

	// PURPOSE: Get the centroid of the instance's bound in world coordinates
	// RETURN: the centroid of the instance's bound in world coordinates
	Vec3V_Out GetWorldCentroid () const;

	SPU_ONLY(void SetLSArchetype(phArchetype* archetype) { m_Archetype = archetype; })

#if __DEV
		PH_NON_SPU_VIRTUAL void Validate () { }										// called by physics level validation function, can also be called elsewhere
		PH_NON_SPU_VIRTUAL void InvalidStateDump() const { }
#endif

#if __DEBUGLOG
	virtual void DebugReplay() const;
#endif

#if DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK
	// PURPOSE: Return the array of u32 values for use by sysStack::PrintCapturedStackTrace() for 
	//	displaying the callstack that last caused the matrix to be set.
	const u32* GetLastSetMatrixCallstack() const;
#endif

	static void AddWidgets(bkBank& bank);

#if !__FINAL
	static void AcquireDeletionLock()
	{
		sm_DeletionLockCount++;
	}

	static void ReleaseDeletionLock()
	{
		Assert(sm_DeletionLockCount > 0);
		sm_DeletionLockCount--;
	}

	static bool AreDeletionsLocked()
	{
		return (sm_DeletionLockCount != 0);
	}
#else // !__FINAL
	static void AcquireDeletionLock() {};
	static void ReleaseDeletionLock() {};
	static bool AreDeletionsLocked() { return false; }
#endif // !__FINAL

#if PHINST_VALIDATE_POSITION
#if !__SPU
	struct ScopedDisableValidatePosition
	{
		ScopedDisableValidatePosition() { phInst::sm_DisableValidatePositionCount++; }
		~ScopedDisableValidatePosition() { phInst::sm_DisableValidatePositionCount--; }
	};
	static int sm_DisableValidatePositionCount;
#endif // !__SPU
	static bool IsPositionValid(Vec3V_In instancePosition);
	void ValidateNewInstancePosition(Vec3V_In newPosition, bool assertOnInvalid) const;
#endif // PHINST_VALIDATE_POSITION


private:
	pgRef<phArchetype> m_Archetype;											// +8 archetype for this instance
	u16 m_LevelIndex;													// +12 the "handle" for this instance in the level
	u16 m_Flags;													// flags for this instance (2 reserved bits, 14 user defined)

	// PURPOSE: orientation (3x3 part) and position (d-vector) for the physics instance
	Mat34V m_Matrix;	// +16

#if !PHINST_USER_DATA_IN_MATRIX_W_COMPONENT
	void* m_UserData;
	u8 pad[__64BIT? 8 : 12];																// +88
#endif

#if !__SPU
	static void (*sm_DtorCallback)(phInst *inst);
	static void (*sm_ArchetypeChangeCallback)(phInst *inst, phArchetype *newArchetype);
#endif

	void SetArchetypeInternal(phArchetype* archetype, bool deleteAtZero);

#if DEBUG_PHINST
	static u16 sm_DebugLevelIndex;
	static bool sm_DebugAllSetMatrix;
	static bool sm_DebugSetMatrixSpam;
	static float sm_SetMatrixCallstackTolerance;
	static bool sm_DebugSetMatrixBreak;
#if DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK
	static bool sm_DebugSetMatrixCallstack;
#endif
#endif

	// For debugging - if >0, fragments may not be deleted.
#if !__FINAL
	static int sm_DeletionLockCount;
#endif // !__FINAL
};


inline Mat34V_ConstRef phInst::GetMatrix () const
{
	return m_Matrix;
}

#if PHINST_VALIDATE_POSITION
// Need to be in .h file for SPU
inline bool phInst::IsPositionValid(Vec3V_In instancePosition)
{
	const Vec3V minimumPosition(V_TEN);
	const Vec3V maximumPosition(V_FLT_LARGE_6);
	const Vec3V absPosition = Abs(instancePosition);
	return IsLessThanAll(absPosition,maximumPosition) && !IsLessThanAll(absPosition,minimumPosition);
}
inline void phInst::ValidateNewInstancePosition(Vec3V_In newPosition, bool ASSERT_ONLY(assertOnInvalid)) const
{
#if !__SPU
	if(sm_DisableValidatePositionCount == 0)
#endif // !__SPU
	{
		if(IsInLevel() && !IsPositionValid(newPosition) && IsPositionValid(m_Matrix.GetCol3()))
		{
			const char* fileName = NULL;
#if !__SPU
			fileName = GetArchetype() ? GetArchetype()->GetFilename() : NULL;
#endif // !__SPU
#if __ASSERT
			if(assertOnInvalid)
			{
				Assertf(false,"phInst::ValidateNewInstancePosition - '%s' - Setting invalid position on instance in level. Curr pos = %f, %f, %f.  New pos  = %f, %f, %f", fileName, VEC3V_ARGS(m_Matrix.GetCol3()), VEC3V_ARGS(newPosition));
			}
			else
#endif // __ASSERT
			{
#if !__SPU
				sysStack::PrintStackTrace();
#endif // !__SPU
				Warningf("phInst::ValidateNewInstancePosition - '%s' - Setting invalid position on instance in level. Curr pos = %f, %f, %f.  New pos  = %f, %f, %f", fileName, VEC3V_ARGS(m_Matrix.GetCol3()), VEC3V_ARGS(newPosition));
			}
		}
	}
}
#endif // PHINST_VALIDATE_POSITION

#if !DEBUG_PHINST
// If we are debugging the calls to SetMatrix, this needs to be implemented in the .cpp where there is additional diagnostics code.
inline void phInst::SetMatrix (Mat34V_In mtx)
{
	PDR_ONLY(debugPlayback::RecordSetMatrix(*this, mtx));
	// Set the matrix.

#if __ASSERT
	const char* fileName = NULL;
#if !__SPU
	fileName = GetArchetype() ? GetArchetype()->GetFilename() : NULL;
#endif // !__SPU
	Assertf(mtx.IsOrthonormal3x3(ScalarVFromF32(REJUVENATE_ERROR_NEW_VEC)),	"Setting non-orthonormal instance matrix on %s:"
																			"\n\t[%5.3f %5.3f %5.3f %5.3f]"
																			"\n\t[%5.3f %5.3f %5.3f %5.3f]"
																			"\n\t[%5.3f %5.3f %5.3f %5.3f]",
																			fileName,MAT34V_ARG_FLOAT_RC(mtx));
#endif // __ASSERT
	PHINST_VALIDATE_POSITION_ONLY(ValidateNewInstancePosition(mtx.GetCol3(),false));

#if PHINST_USER_DATA_IN_MATRIX_W_COMPONENT
#if __64BIT
	Vec4V userData1InW = Vec4V(m_Matrix.GetCol3());
	Vec4V userData2InW = Vec4V(m_Matrix.GetCol2());
	m_Matrix.SetCol0(mtx.GetCol0ConstRef());
	m_Matrix.SetCol1(mtx.GetCol1ConstRef());
	m_Matrix.SetCol2(Vec3V(GetFromTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::W2>(Vec4V(mtx.GetCol2()), userData2InW).GetIntrin128()));
	m_Matrix.SetCol3(Vec3V(GetFromTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::W2>(Vec4V(mtx.GetCol3()), userData1InW).GetIntrin128()));
#else
	Vec4V userDataInW = Vec4V(m_Matrix.GetCol3());
	m_Matrix.Set3x3(mtx.GetMat33ConstRef());
	m_Matrix.SetCol3(Vec3V(GetFromTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::W2>(Vec4V(mtx.GetCol3()), userDataInW).GetIntrin128()));
#endif
#else
	m_Matrix = mtx;
#endif
}

inline void phInst::SetMatrixNoZeroAssert (Mat34V_In mtx)
{
	PDR_ONLY(debugPlayback::RecordSetMatrix(*this, mtx));
	// Set the matrix.

#if __ASSERT
	const char* fileName = NULL;
#if !__SPU
	fileName = GetArchetype() ? GetArchetype()->GetFilename() : NULL;
#endif // !__SPU
	Assertf(mtx.IsOrthonormal3x3(ScalarVFromF32(2.0f*REJUVENATE_ERROR)),	"Setting non-orthonormal instance matrix on %s:"
		"\n\t[%5.3f %5.3f %5.3f %5.3f]"
		"\n\t[%5.3f %5.3f %5.3f %5.3f]"
		"\n\t[%5.3f %5.3f %5.3f %5.3f]",
		fileName,MAT34V_ARG_FLOAT_RC(mtx));
#endif // __ASSERT

	PHINST_VALIDATE_POSITION_ONLY(ValidateNewInstancePosition(mtx.GetCol3(),false));

#if PHINST_USER_DATA_IN_MATRIX_W_COMPONENT
#if __64BIT
	Vec4V userData1InW = Vec4V(m_Matrix.GetCol3());
	Vec4V userData2InW = Vec4V(m_Matrix.GetCol2());
	m_Matrix.SetCol0(mtx.GetCol0ConstRef());
	m_Matrix.SetCol1(mtx.GetCol1ConstRef());
	m_Matrix.SetCol2(Vec3V(GetFromTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::W2>(Vec4V(mtx.GetCol2()), userData2InW).GetIntrin128()));
	m_Matrix.SetCol3(Vec3V(GetFromTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::W2>(Vec4V(mtx.GetCol3()), userData1InW).GetIntrin128()));
#else
	Vec4V userDataInW = Vec4V(m_Matrix.GetCol3());
	m_Matrix.Set3x3(mtx.GetMat33ConstRef());
	m_Matrix.SetCol3(Vec3V(GetFromTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::W2>(Vec4V(mtx.GetCol3()), userDataInW).GetIntrin128()));
#endif
#else
	m_Matrix = mtx;
#endif
}
#endif

inline Vec3V_ConstRef phInst::GetPosition () const
{
	return (*reinterpret_cast<const Vec3V*>((&(m_Matrix.GetCol3Intrin128ConstRef()))));
}

inline void phInst::SetPosition (Vec3V_In position)
{
	PHINST_VALIDATE_POSITION_ONLY(ValidateNewInstancePosition(position,false));

#if PHINST_USER_DATA_IN_MATRIX_W_COMPONENT
	Vec4V userDataInW = Vec4V(m_Matrix.GetCol3());
	m_Matrix.SetCol3(Vec3V(GetFromTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::W2>(Vec4V(position), userDataInW).GetIntrin128()));
#else
	m_Matrix.SetCol3(position);
#endif
}

inline u16 phInst::GetLevelIndex () const
{
	return m_LevelIndex;
}

#if !DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK
// If we are debugging the calls to SetMatrix, this needs to be implemented in the .cpp where there is additional diagnostics code.
inline void phInst::SetLevelIndex (u16 levelIndex)
{
	m_LevelIndex = levelIndex;
}
#endif

inline void* phInst::GetUserData () const
{
#if PHINST_USER_DATA_IN_MATRIX_W_COMPONENT
#if __64BIT
	return (void*)((u64)(u32)m_Matrix.GetCol3ConstRef().GetWi() | (((u64)(u32)m_Matrix.GetCol2ConstRef().GetWi()) << 32));
#else
	return (void*)m_Matrix.GetCol3ConstRef().GetWi();
#endif
#else
	return m_UserData;
#endif
}

inline void phInst::SetUserData (void* userData)
{
#if PHINST_USER_DATA_IN_MATRIX_W_COMPONENT
#if __64BIT
	m_Matrix.GetCol3Ref().SetWi(u32(u64(userData) & 0xffffffffLL));
	m_Matrix.GetCol2Ref().SetWi(u32((u64(userData) & 0xffffffff00000000LL) >> 32));
#else
	m_Matrix.GetCol3Ref().SetWi((u32)userData);
#endif
#else
	m_UserData = userData;
#endif
}

inline u32 phInst::GetInstFlags () const
{
	return m_Flags;
}

inline void phInst::SetInstFlags (u16 flags)
{
	m_Flags = flags;
}

inline u32 phInst::GetInstFlag (u16 mask) const
{
	return (m_Flags & mask);
}

inline bool phInst::HasLastMatrix() const
{
	return (GetInstFlag(FLAG_INTERNAL_USE_ONLY_LAST_MTX) != 0);
}

inline void phInst::Copy (const phInst& inst)
{
	SetMatrix(inst.GetMatrix());
	SetArchetype( inst.m_Archetype );
	m_LevelIndex = inst.m_LevelIndex;
	m_Flags = inst.m_Flags;
}


inline Vec3V_Out phInst::GetCenterOfMass () const
{
	FastAssert(GetArchetype() && GetArchetype()->GetBound());
	return GetArchetype()->GetBound()->GetCenterOfMass( m_Matrix );
}


inline Vec3V_Out phInst::GetWorldCentroid () const
{
	FastAssert(GetArchetype() && GetArchetype()->GetBound());
	return GetArchetype()->GetBound()->GetWorldCentroid(m_Matrix);
}

#if DR_ENABLED
namespace debugPlayback {
		//------------------------------------------------------------------//
		//	  Helper function to make adding events to the system easier	//
		//    Defined here to avoid cyclic dependency on debugevents.h      //
		//------------------------------------------------------------------//
		template<typename _TEvent>
		_TEvent* AddPhysicsEvent(const phInst *pInst)
		{
			if (DR_EVENT_ENABLED(_TEvent) && pInst)
			{
				DebugRecorder *pRecorder = DebugRecorder::GetInstance();
				if (pRecorder && (pRecorder->IsRecording() || CallstackHelper::sm_bTrapCallstack))
				{
					if (pInst->IsInLevel() && PhysicsEvent::IsSelected(*pInst))
					{
						CallstackHelper ch;
						if (pRecorder->IsRecording())
						{
							DR_MEMORY_HEAP();
							_TEvent *pEvent = rage_new _TEvent;
							pEvent->Init(ch, pInst);
							pRecorder->AddEvent( *pEvent );
							return pEvent;
						}
					}
				}
			}
			return 0;
		}
}	// namespace debugPlayback
#endif // DR_ENABLED


} // namespace rage

#endif
