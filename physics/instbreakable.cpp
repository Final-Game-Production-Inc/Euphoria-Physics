//
// physics/instbreakable.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "instbreakable.h"

#include "collider.h"
#include "colliderdispatch.h"
#include "levelnew.h"
#include "simulator.h"

#include "phbound/boundcomposite.h"

#include "vectormath/classes.h"

using namespace rage;


/*
Purpose: Constructor */
phInstBreakable::phInstBreakable ()
{
}


/*
Purpose: Resource constructor. */
phInstBreakable::phInstBreakable(datResource &rsc) : phInst(rsc)
{
}


/*
Purpose: Get the minimum impulse magnitude required to break this instance.
Params:	active	- whether or not this instance is in the active state (the breaking limit can be different)
Return: the minimum impulse magnitude required to break this object */
float phInstBreakable::GetImpulseLimit (bool active) const
{
	if (!active)
	{
		return GetInactiveImpulseLimit();
	}
	
	return GetActiveImpulseLimit();
}


/*
Purpose: Get the minimum impulse magnitude required to break this instance when it is not active.
Return: Return 0.0f in phInstBreakable to indicate no breaking resistance; used in derived classes to return
		a stored impulse limit.
Notes:
1.	A negative return value in a derived class will prevent the object from ever breaking when it is not active.
2.	The default value of 0.0f in this base class makes objects always break with no resistance. */
float phInstBreakable::GetInactiveImpulseLimit () const
{
	return 0.0f;
}


/*
Purpose: Get the minimum impulse magnitude required to break this instance when it is active.
Return: Return 0.0f in phInstBreakable to indicate no breaking resistance; used in derived classes to return
		a stored impulse limit.
Notes:
1.	A negative return value in a derived class will prevent the object from ever breaking when it is active.
2.	The default value of 0.0f in this base class makes objects always break with no resistance. */
float phInstBreakable::GetActiveImpulseLimit () const
{
	return 0.0f;
}


bool phInstBreakable::IsBreakable(phInst* UNUSED_PARAM(otherInst)) const
{
	return (GetImpulseLimit(PHLEVEL->IsActive(GetLevelIndex()))>SMALL_FLOAT); 
}


phInst* phInstBreakable::PrepareForActivation(phCollider** colliderToUse, phInst* otherInst, const phConstraintBase * constraint)
{
	return fabsf(GetInactiveImpulseLimit()) < SMALL_FLOAT ? phInst::PrepareForActivation(colliderToUse, otherInst, constraint) : NULL;
}


bool phInstBreakable::FindBreakStrength (const Vector3* componentImpulses, const Vector4* UNUSED_PARAM(componentPositions), float* breakStrength, phBreakData* UNUSED_PARAM(breakData)) const
{
	// Get the impulse limit for breaking.
	float impulseLimit = GetImpulseLimit(PHLEVEL->IsActive(GetLevelIndex()));

	// This instance is breakable. Find the total impulse in the collisions on this instance.
	Vector3 totalImpulse(ORIGIN);
	for (int component = 0; component < MAX_NUM_BREAKABLE_COMPONENTS; ++component)
	{
		totalImpulse += componentImpulses[component];
	}

	// Find the impulse limit to break this instance.
	float totalImpulse2 = totalImpulse.Mag2();
	if (totalImpulse2>SMALL_FLOAT && totalImpulse2>square(impulseLimit))
	{
		// The total impulse is enough to break this instance, so find the fraction of the total impulse that
		// will be used for breaking and return true to indicate that this instance can break in this collision.
		(*breakStrength) = impulseLimit*invsqrtf(totalImpulse2);
		return true;
	}

	// The total impulse is not enough to break this object, so return false to indicate no breaking.
	return false;
}


/*
Purpose: Find the fraction of the given collision that it will take to break this instance.
Params:	impactList			- pointer to the impact list for the current collision, if there is one
		breakableInstList	- an array of instance pointers that can be filled in by this method to identify the
							  instances resulting from this break that are further breakable
		numBreakInsts		- the number of objects put into breakableInstList in this method
		brokenInstList		- an array of instance pointers that can be filled in by this method to identify the
							  instances resulting from this collision that should have impetuses applied to them
		impactData			- pointer to the impact data for the current impetus application, if there is one
		breakData			- pointer to a data class provided by the simulator for recording breaking info
Return: the number of instances in the broken instance list
Notes:
1.	This method can be overridden in derived classes to specify different ways to break.
2.	The default behavior is for this instance to break free from fixed as itself. */
int phInstBreakable::BreakApart (phInstBreakable** breakableInstList, int* numBreakInsts,phInst** brokenInstList, 
	int /*componentA*/, int /*componentB*/, phInst*& /*pInstanceA*/, phInst*& /*pInstanceB*/,
	const phBreakData& UNUSED_PARAM(breakData))
{
	// Set the default behavior to break into an object that is not further breakable, and to get a collider from the
	// physics level, instead of using one's own collider. This method can be derived to change this behavior.
	const bool breakableAgain = false;
	phCollider* collider = NULL;
	return BreakIntoActiveSelf(breakableInstList,numBreakInsts,brokenInstList,breakableAgain,collider);
}


/*
Purpose: Find the fraction of the given collision that it will take to break this instance.
Params:	breakableInstList	- an array of instance pointers that can be filled in by this method to identify the
							  instances resulting from this break that are further breakable
		numBreakInsts		- the number of objects put into breakableInstList in this method
		brokenInstList		- an array of instance pointers that can be filled in by this method to identify the
							  instances resulting from this collision that should have impetuses applied to them
		breakData			- pointer to a data class provided by the simulator for recording breaking info
Return: the number of instances in the broken instance list
Notes:
1.	This method can be overridden in derived classes to specify different ways to break.
2.	The default behavior is for this instance to break free from fixed as itself. */
phInst* phInstBreakable::BreakApart (const phBreakData& UNUSED_PARAM(breakData))
{
	// Set the default behavior to break into an object that is not further breakable, and to get a collider from the
	// physics level, instead of using one's own collider. This method can be derived to change this behavior.
	const bool breakableAgain = false;
	phCollider* collider = NULL;
	BreakIntoActiveSelf(NULL,NULL,NULL,breakableAgain,collider);
	return NULL;
}





/*
Purpose: Break this instance free to make it active.
Params:	impactList			- pointer to the impact list for the current collision, if there is one
		breakableInstList	- an array of instance pointers that can be filled in by this method to identify the
							  instances resulting from this break that are further breakable
		numBreakInsts		- the number of objects put into breakableInstList in this method
		brokenInstList		- an array of instance pointers that can be filled in by this method to identify the
							  instances resulting from this collision that should have impetuses applied to them
		breakableAgain		- boolean to tell whether the broken (active) object will be further breakable
		impactData			- optional pointer to the impact data for the current impetus application, if there is one
		collider			- optional pointer to a collider to use with the newly active object (default is NULL, in
								which cas the physics level will assign a collider)
Return: the number of instances in the broken instance list
Notes:
1.	This is the default way to break, called from BreakApart. */
int phInstBreakable::BreakIntoActiveSelf (phInstBreakable** breakableInstList,
											int* numBreakInsts, phInst** brokenInstList, bool breakableAgain,
											phCollider* collider)
{
	if (collider)
	{
		// Set this instance in the given collider.
		collider->SetInstanceAndReset(this);
	}

	// Activate this instance in the physics level.
	Assert(!PHLEVEL->IsActive(GetLevelIndex()));
	PHSIM->ActivateObject(GetLevelIndex(), collider);

	if (!breakableAgain && breakableInstList && numBreakInsts)
	{
		// Remove this instance from the breakable instance list, since it is not breakable again in this collision.
		RemoveFromInstList(breakableInstList,this,numBreakInsts);
	}

	// Put this instance in the broken instance list, to make the simulator recalculate its impacts, or to apply
	// to post-breaking impetus to it.
	if(brokenInstList)
	{
		brokenInstList[0] = this;
	}

	// Return the number of broken instances.
	return 1;
}

void phInstBreakable::FindPostBreakMotion (Vector3& centerOfMass, Vector3& velocity, Vector3& angVelocity) const
{
	phCollider* collider = PHSIM->GetActiveCollider(GetLevelIndex());
	Assert(collider);
	centerOfMass = VEC3V_TO_VECTOR3(GetArchetype()->GetBound()->GetCenterOfMass(GetMatrix()));

	velocity = RCC_VECTOR3(collider->GetLastVelocity());
	angVelocity = RCC_VECTOR3(collider->GetLastAngVelocity());
}


int phInstBreakable::GetNumCompositeParts () const
{
	Assert(GetArchetype() && GetArchetype()->GetBound() && GetArchetype()->GetBound()->GetType()==phBound::COMPOSITE);
	phBoundComposite* compositeBound = static_cast<phBoundComposite*>(GetArchetype()->GetBound());
	return compositeBound->GetNumActiveBounds();
}


const Matrix34& phInstBreakable::GetBrokenPartMatrix (int partIndex) const
{
	Assert(GetArchetype() && GetArchetype()->GetBound() && GetArchetype()->GetBound()->GetType()==phBound::COMPOSITE);
	phBoundComposite* compositeBound = static_cast<phBoundComposite*>(GetArchetype()->GetBound());
	return RCC_MATRIX34(compositeBound->GetCurrentMatrix(partIndex));
}


// Get the instance for the broken part.
phInst* phInstBreakable::GetBrokenPartInstance (int UNUSED_PARAM(partIndex))
{
	Warningf("phInstBreakable::GetBrokenPartInstance has been called but not derived for this breakable instance.");
	return NULL;
}


void phInstBreakable::ThrowBrokenPart (const phInst* partInst, const Vector3& centerOfMass, const Vector3& velocity,
										const Vector3& angVelocity)
{
	phCollider* partCollider = PHSIM->GetCollider(partInst->GetLevelIndex());
	AssertMsg(partCollider,"Don't throw a broken part that's not active in the physics level.");
	if (partCollider == NULL)
		return;

	// Set the broken part in motion.
	Vector3 partVelocity( *(const Vector3*)(&partInst->GetPosition()) );
	partVelocity.Subtract(centerOfMass);
	partVelocity.CrossNegate(angVelocity);
	partVelocity.Add(velocity);
	partCollider->SetVelocity(partVelocity);
	partCollider->SetAngVelocity(angVelocity);
}


void phInstBreakable::RemoveFromInstList (phInstBreakable** breakableInstList, phInstBreakable* inst, int* numBreakInsts)
{
	Assert(breakableInstList && inst && numBreakInsts);
	for (int instIndex=0; instIndex<(*numBreakInsts); instIndex++)
	{
		if (breakableInstList[instIndex]==inst)
		{
			// The given instance was found in the list, so remove it by moving all later instances down by one.
			(*numBreakInsts)--;
			for (int moveInstIndex=instIndex; moveInstIndex<(*numBreakInsts); moveInstIndex++)
			{
				breakableInstList[moveInstIndex] = breakableInstList[moveInstIndex+1];
			}

			// The given instance is assumed to be in the list only once, so stop looking.
			return;
		}
	}
}
