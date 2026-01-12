// 
// pheffects/spring.h
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
// 

// TITLE: Springs
// PURPOSE:
//		This class allows you to attach spring behaviour to physics instances.


#ifndef PHEFFECTS_SPRING_H
#define PHEFFECTS_SPRING_H

#include "data/base.h"
#include "physics/instbehavior.h"
#include "vector/vector3.h"

namespace rage {



class phSpring	: public phInstBehavior										
{
public:

	// PURPOSE: Resource constructor
	phSpring(datResource& rsc) : phInstBehavior(rsc) { rsc.PointerFixup(m_OtherBody); }

	// PURPOSE: Constructor, pass in a pointer to a position, it automagically keeps track of it. Also pass in spring parameters.
	phSpring( phInst* otherBody, const Vector3 &thisAttachmentPoint, const Vector3 &otherAttachmentPoint, float springConstant, float springLength, float springDampening)	: 
		m_OtherBody(otherBody),
		m_ThisAttachmentPosition(thisAttachmentPoint),
		m_OtherAttachmentPosition(otherAttachmentPoint),
		m_SpringConstant(springConstant),									//set the spring constant
		m_SpringLength(springLength),										//set the spring length
		m_SpringDampening(springDampening)								//set the friction constant
	{
	}

	// PURPOSE: Change what we are attached to.
	void AttachToThis(const Vector3& pos) { m_ThisAttachmentPosition = pos; }
	void AttachToOther(const Vector3& pos) { m_OtherAttachmentPosition = pos; }

	// Accessors:
	float* GetSpringConstantPtr() { return &m_SpringConstant; }
	float GetSpringConstant() const { return m_SpringConstant; }
	void  SetSpringConstant(float f) { m_SpringConstant = f; }

	float* GetSpringLengthPtr() { return &m_SpringLength; }
	float GetSpringLength() const { return m_SpringLength; }
	void  SetSpringLength(float f) { m_SpringLength = f; }

	float* GetSpringDampeningPtr() { return &m_SpringDampening; }
	float GetSpringDampening() const { return m_SpringDampening; }
	void  SetSpringDampening(float f) { m_SpringDampening = f; }

	Vector3* GetThisAttachmentPtr() { return &m_ThisAttachmentPosition; }
	Vector3 GetThisAttachment() const { return m_ThisAttachmentPosition; }
	void  SetThisAttachment(Vector3 &x) { m_ThisAttachmentPosition = x; }

	Vector3* GetOtherAttachmentPtr() { return &m_OtherAttachmentPosition; }
	Vector3 GetOtherAttachment() const { return m_OtherAttachmentPosition; }
	void  SetOtherAttachment(Vector3 &x) { m_OtherAttachmentPosition = x; }

	// PURPOSE: Get the world position of one end of the spring.
	Vector3 GetAttachedPositionThis();

	// PURPOSE: Get the world position of the other end of the spring.
	Vector3 GetAttachedPositionOther();

	// PURPOSE: Reset the spring to some sane state.
	virtual void Reset(){}

	// If this returns false, that means that this instance behavior is done, and won't do anything further without any prompting.
	//   This will usually be used by a manager class to determine when to remove the behavior from the phSimulator.
	virtual bool IsActive() const {return true;}

	// Called once per frame to give you an opportunity to make any changes that you wish to make.
	virtual void PreUpdate (float UNUSED_PARAM(TimeStep)) {}

	// Called when your instance's cull sphere is intersecting another instance's cull sphere to give you a chance to cause an alternate response.
    virtual bool CollideObjects(Vec::V3Param128 UNUSED_PARAM(timeStep), phInst* UNUSED_PARAM(myInst), phCollider* UNUSED_PARAM(myCollider), phInst* UNUSED_PARAM(otherInst), phCollider* UNUSED_PARAM(otherCollider), phInstBehavior* UNUSED_PARAM(otherInstBehavior))
    {
        return false;
    }

	// Called before the instance's ActivateWhenHit() is called.  Return false to disallow activation.  Return true to let the phInst
	//   proceed as normal.
	virtual bool ActivateWhenHit() const {return false;}

	// Called once per frame to give you an opportunity to make any changes that you wish to make.
	virtual void Update (float UNUSED_PARAM(TimeStep));


protected:
	phInst *m_OtherBody;
	Vector3 m_ThisAttachmentPosition;
	Vector3 m_OtherAttachmentPosition;

	// PURPOSE: A constant to represent the stiffness of the spring
	float m_SpringConstant;													

	// PURPOSE: The length that the spring does not exert any force
	float m_SpringLength;													

	// PURPOSE: A spring dampening constant
	float m_SpringDampening;												
};

} // namespace rage
#endif
