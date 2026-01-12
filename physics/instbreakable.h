//
// physics/instbreakable.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_INST_BREAKABLE_H
#define PHYSICS_INST_BREAKABLE_H

#include "inst.h"

namespace rage {

class phBreakData;
class phCollider;
class phImpact;


////////////////////////////////////////////////////////////////
// phInstBreakable
//   A phInst that provides a Break() function for changing
//   from a static state to an active state.

// PURPOSE
//   Basic physics instance for representing a physical object that can break into another object.
//   In addition to what is contained in its base class phInst, it contains UnbrokenLevelIndex, which is used
//   by the physics level to reset the instance to its unbroken state.
// NOTES
//   - The Break function in this class is called when the physics simulator is requesting that this object break,
//     and it controls whether the break will occur and what the object will break into.
//   - This class does not contain a matrix for resetting to its unbroken state.  Most games or testers use their
//     own instance class derived from this class, which either contains an initial matrix and other reset
//     information, or makes this instance break into other instances instead of using itself as the broken instance. 
// <FLAG Component>
class phInstBreakable : public phInst
{
public:
	// These are used by the simulator to create arrays of objects that can be filled in by breaking instances.
	enum { MAX_NUM_BROKEN_INSTS = 64, MAX_NUM_BREAKABLE_INSTS = 64, MAX_NUM_BREAKABLE_COMPONENTS = 128 };
	enum { BREAK_DATA_MAX_NUM_BYTES = 1024 };

	// Constructors
	phInstBreakable ();							// constructor
	phInstBreakable (datResource &rsc);			// resource constructor

	// Class type accessors
	int GetClassType () const { return PH_INST_BREAKABLE; }				// for game-level use to identify derived classes
	bool ClassTypeBreakable () const { return true; }				// used to check for breakability

	// Impulse limits for breaking
	float GetImpulseLimit (bool active) const;
	virtual float GetInactiveImpulseLimit () const;
	virtual float GetActiveImpulseLimit () const;

	// Deciding whether or not to break
	virtual bool IsBreakable (phInst* otherInst) const;
    virtual phInst* PrepareForActivation(phCollider** colliderToUse, phInst* otherInst, const phConstraintBase * constraint);

	// PURPOSE: Find the fraction of the given collision that it will take to break this instance.
	// PARAMS:
	//	componentImpulses - pointer to an array of total impulses, with one element for each component of the instance
	//	breakStrength - pointer to the fraction of the collision needed to cause breaking, to be filled in here
	//	breakData - pointer to a data class provided by the simulator for recording breaking info
	// RETURN: true if this object can break as a result of the given collision or impetus application
	// NOTES:
	//	1.	This method can be overridden in derived classes to specify different criteria for breaking.
	//	2.	The behavior used here is to use GetImpulseLimit()/(total impulse) as the breaking strength
	virtual bool FindBreakStrength (const Vector3* componentImpulses, const Vector4* componentPositions, float* breakStrength, phBreakData* breakData) const;

#if !__SPU
	// Code dissabled on Spu for gta4 - should ask to move this to rage\dev?
	// I'm guessing we needed to compile fragment/instance.h on spu
	// Think it was so we can do the skeleton update, and PoseBoundsFromSkeleton, in spu task
#define COMPILE_BREAKING_CODE 1
#else
#define COMPILE_BREAKING_CODE 0
#endif 

#if COMPILE_BREAKING_CODE
	// Breaking

	virtual int BreakApart (phInstBreakable** breakableInstList, int* numBreakInsts, phInst** brokenInstList, 
		int componentA, int componentB, phInst*& pInstanceA, phInst*& pInstanceB,
		const phBreakData& breakData);


    virtual phInst* BreakApart (const phBreakData& breakData);
	int BreakIntoActiveSelf (phInstBreakable** breakableInstList, int* numBreakInsts,
								phInst** brokenInstList, bool breakableAgain=false,
								phCollider* collider=NULL);

	void FindPostBreakMotion (Vector3& centerOfMass, Vector3& velocity, Vector3& angVelocity) const;
	virtual int GetNumCompositeParts () const;
	virtual const Matrix34& GetBrokenPartMatrix (int partIndex) const;
	virtual phInst* GetBrokenPartInstance (int partIndex);

	int BreakIntoSelf (phInst** UNUSED_PARAM(ppBrokenInstances), phCollider** UNUSED_PARAM(ppBrokenColliders)) { return 0; }

	static void RemoveFromInstList (phInstBreakable** breakableInstList, phInstBreakable* inst, int* numBreakInsts);
	static void ThrowBrokenPart (const phInst* partInst, const Vector3& centerOfMass, const Vector3& velocity,
									const Vector3& angVelocity);
#endif
};


// This is an empty class for use by breakable instances to record how they will break in FindBreakStrength(), and then
// reuse that information to break in BreakApart(). Two of these are kept on the stack by the simulator during breaking,
// one for the next object to break, and one for checking other objects to see if they will be the next to break.
class ALIGNAS(16) phBreakData
{
public:
	u8 BreakData[phInstBreakable::BREAK_DATA_MAX_NUM_BYTES];
} ;

} // namespace rage

#endif

