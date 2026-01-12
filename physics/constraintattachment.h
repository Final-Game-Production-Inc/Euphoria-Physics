//
// physics/constraintattachment.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_ATTACHMENT_H
#define PHYSICS_CONSTRAINT_ATTACHMENT_H

#include "constraintbase.h"
#include "constraintfixedrotation.h"

namespace rage {

//Either a phConstraintSpherical or a phConstraintDistance and optionally a phConstraintFixedRotation
class phConstraintAttachment : public phConstraintBase
{
public:

	// Pass this into PHCONSTRAINT->Insert to create the constraint
	struct Params : public phConstraintBase::Params
	{
		Vec3V localPosA;  
		Vec3V localPosB;
		float maxSeparation;

		//Whether to make the two objects rotate together as well as have a point of each attached
		bool  constrainRotation;

		Params()
			: phConstraintBase::Params(ATTACHMENT)
			, maxSeparation(0.0f)
			, constrainRotation(true)
			, localPosA(V_ZERO)
			, localPosB(V_ZERO)
		{
		}
	};

	phConstraintAttachment(const Params& params);
	virtual ~phConstraintAttachment();

#if !__FINAL
	virtual void SetOwner(const char* file, int line);
#endif

	virtual const char* GetTypeName()
	{
		return "attachment";
	}

#if __PFDRAW
	virtual void ProfileDraw();
#endif

//interface for users
public:

	virtual void SetBreakable(bool breakable=true, float breakingStrength=10000.0f);
	virtual void SetBroken(bool broken=true);
	
	void SetWorldPosA(Vec3V_In worldPosA);
	void SetWorldPosB(Vec3V_In worldPosB);
	void SetLocalPosA(Vec3V_In localPosA);
	void SetLocalPosB(Vec3V_In localPosB);
	Vec3V_Out GetWorldPosA();
	Vec3V_Out GetWorldPosB();
	Vec3V_Out GetLocalPosA();
	Vec3V_Out GetLocalPosB();

	void SetMaxSeparation(float newMaxSeparation);
	float GetMaxSeparation() { return m_MaxSeparation; }

//interface for simulator
public:

	virtual bool UpdateBreaking();
	virtual void VirtualEnforceWorldConstraints(phCollider* collider);
	virtual void VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver);

protected:
	virtual void DisableManifolds()
	{
		m_TranslationConstraint->DisableManifolds();

		if(m_RotationConstraint != NULL)
		{
			m_RotationConstraint->DisableManifolds();
		}
	}


private:

	float						m_MaxSeparation;
	phConstraintBase*			m_TranslationConstraint; //Either a phConstraintDistance or a phConstraintSpherical
	phConstraintFixedRotation*	m_RotationConstraint;
};


}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_ATTACHMENT_H
