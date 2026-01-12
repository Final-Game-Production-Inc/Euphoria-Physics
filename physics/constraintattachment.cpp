//
// physics/constraintattachment.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "constraintattachment.h"

#include "constraintspherical.h"
#include "constraintdistance.h"
#include "constraintfixedrotation.h"

#include "simulator.h"

namespace rage {

#define	PHCONSTRAINTATTACHMENT_SPHERICAL_MAX_DIST	1.0e-2f

phConstraintAttachment::phConstraintAttachment(const Params& params)
	: phConstraintBase(params)
{
	phConstraintMgr* constraintMgr = ::rage::phSimulator::GetActiveInstance()->GetConstraintMgr();

	m_MaxSeparation = params.maxSeparation;

	//Construct the appropriate translation constraint
	if(params.maxSeparation < PHCONSTRAINTATTACHMENT_SPHERICAL_MAX_DIST)
	{
		phConstraintSpherical::Params	sphericalParams(params);
										sphericalParams.constructUsingLocalPositions = true;
										sphericalParams.localPosA = params.localPosA;
										sphericalParams.localPosB = params.localPosB;

		m_TranslationConstraint = constraintMgr->AllocateAndConstruct(sphericalParams);
	}
	else
	{
		phConstraintDistance::Params			distParams(params);
												distParams.maxDistance = params.maxSeparation;
												distParams.constructUsingLocalPositions = true;
												distParams.localPosA = params.localPosA;
												distParams.localPosB = params.localPosB;

		m_TranslationConstraint = constraintMgr->AllocateAndConstruct(distParams);
	}

	if(m_TranslationConstraint == NULL)
	{
		FlagForDestruction();
		return;
	}

	//If requested, also construct a rotation constraint
	if(params.constrainRotation)
	{
		m_RotationConstraint = static_cast<phConstraintFixedRotation*>( constraintMgr->AllocateAndConstruct( phConstraintFixedRotation::Params(params) ) );
		
		if(m_RotationConstraint == NULL) 
		{
			FlagForDestruction();
			return;
		}
	}
	else
	{
		m_RotationConstraint = NULL;
	}
}

phConstraintAttachment::~phConstraintAttachment()
{
	phConstraintMgr* constraintMgr = ::rage::phSimulator::GetActiveInstance()->GetConstraintMgr();

	constraintMgr->DestructAndFree(m_TranslationConstraint);
	constraintMgr->DestructAndFree(m_RotationConstraint);
}

#if !__FINAL
void phConstraintAttachment::SetOwner(const char* file, int line)
{
	phConstraintBase::SetOwner(file, line);

	m_TranslationConstraint->SetOwner(file, line);

	if(m_RotationConstraint != NULL)
	{
		m_RotationConstraint->SetOwner(file, line);
	}
}
#endif // !__FINAL

void phConstraintAttachment::VirtualEnforceWorldConstraints(phCollider* collider)
{
	m_TranslationConstraint->VirtualEnforceWorldConstraints(collider);

	if(m_RotationConstraint != NULL)
	{
		m_RotationConstraint->phConstraintFixedRotation::VirtualEnforceWorldConstraints(collider);
	}
}

void phConstraintAttachment::VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver)
{
	m_TranslationConstraint->VirtualUpdate(invTimeStep, instanceA, colliderA, instanceB, colliderB, addToSolver);

	if(m_RotationConstraint != NULL)
	{
		m_RotationConstraint->phConstraintFixedRotation::VirtualUpdate(invTimeStep, instanceA, colliderA, instanceB, colliderB, addToSolver);
	}
}

bool phConstraintAttachment::UpdateBreaking()
{
	if( m_Breakable && !m_Broken )
	{
		bool newlyBroken = m_TranslationConstraint->UpdateBreaking();

		if( !newlyBroken && m_RotationConstraint != NULL )
		{
			newlyBroken = m_RotationConstraint->phConstraintFixedRotation::UpdateBreaking();
		}

		if( newlyBroken )
		{
			phConstraintAttachment::SetBroken();
			return true;
		}
	}

	return false;
}

void phConstraintAttachment::SetMaxSeparation(float newMaxSeparation)
{
	if(newMaxSeparation < PHCONSTRAINTATTACHMENT_SPHERICAL_MAX_DIST)
	{
		if( m_TranslationConstraint->GetType() == DISTANCE )
		{
			phConstraintMgr* constraintMgr = ::rage::phSimulator::GetActiveInstance()->GetConstraintMgr();

			//Change to a spherical constraint
			phConstraintSpherical::Params params;
				m_TranslationConstraint->ReconstructBaseParams(params);
				params.constructUsingLocalPositions = true;
				params.localPosA = GetLocalPosA();
				params.localPosB = GetLocalPosB();
			
			constraintMgr->DestructAndFree(m_TranslationConstraint);
			m_TranslationConstraint = constraintMgr->AllocateAndConstruct( params );
			Assert(m_TranslationConstraint); //destruction of one little constraint freed a pool slot for the other
		}
		else
		{
			//Do nothing
		}
	}
	else
	{
		if( m_TranslationConstraint->GetType() == DISTANCE )
		{
			static_cast<phConstraintDistance*>(m_TranslationConstraint)->SetMaxDistance(newMaxSeparation);
		}
		else
		{
			phConstraintMgr* constraintMgr = ::rage::phSimulator::GetActiveInstance()->GetConstraintMgr();

			//Change to a distance constraint
			phConstraintDistance::Params params;
				m_TranslationConstraint->ReconstructBaseParams(params);
				params.maxDistance = newMaxSeparation;
				params.constructUsingLocalPositions = true;
				params.localPosA = GetLocalPosA();
				params.localPosB = GetLocalPosB();

			constraintMgr->DestructAndFree(m_TranslationConstraint);
			m_TranslationConstraint = constraintMgr->AllocateAndConstruct( params );
			Assert(m_TranslationConstraint); //destruction of one little constraint freed a pool slot for the other
		}
	}

	m_MaxSeparation = newMaxSeparation;
}

void phConstraintAttachment::SetBreakable(bool breakable, float breakingStrength)
{
	m_Breakable = breakable;

	m_TranslationConstraint->SetBreakable(breakable, breakingStrength);

	if(m_RotationConstraint != NULL)
	{
		m_RotationConstraint->phConstraintFixedRotation::SetBreakable(breakable, breakingStrength);
	}
}

void phConstraintAttachment::SetBroken(bool broken)
{
	m_Broken = broken;
	
	m_TranslationConstraint->SetBroken(broken);

	if(m_RotationConstraint != NULL)
	{
		m_RotationConstraint->phConstraintFixedRotation::SetBroken(broken);
	}
}

#if __PFDRAW
void phConstraintAttachment::ProfileDraw()
{
	m_TranslationConstraint->ProfileDraw();

	if(m_RotationConstraint != NULL)
	{
		m_RotationConstraint->phConstraintFixedRotation::ProfileDraw();
	}
}
#endif

void phConstraintAttachment::SetWorldPosA(Vec3V_In worldPosA)
{
	if(m_TranslationConstraint->GetType() == SPHERICAL)
	{
		static_cast<phConstraintSpherical*>(m_TranslationConstraint)->phConstraintSpherical::SetWorldPosA(worldPosA);
	}
	else
	{
		static_cast<phConstraintDistance*>(m_TranslationConstraint)->phConstraintDistance::SetWorldPosA(worldPosA);
	}
}

void phConstraintAttachment::SetWorldPosB(Vec3V_In worldPosB)
{
	if(m_TranslationConstraint->GetType() == SPHERICAL)
	{
		static_cast<phConstraintSpherical*>(m_TranslationConstraint)->phConstraintSpherical::SetWorldPosB(worldPosB);
	}
	else
	{
		static_cast<phConstraintDistance*>(m_TranslationConstraint)->phConstraintDistance::SetWorldPosB(worldPosB);
	}
}

Vec3V_Out phConstraintAttachment::GetWorldPosA()
{
	if(m_TranslationConstraint->GetType() == SPHERICAL)
	{
		return static_cast<phConstraintSpherical*>(m_TranslationConstraint)->phConstraintSpherical::GetWorldPosA();
	}
	else
	{
		return static_cast<phConstraintDistance*>(m_TranslationConstraint)->phConstraintDistance::GetWorldPosA();
	}
}

Vec3V_Out phConstraintAttachment::GetWorldPosB()
{
	if(m_TranslationConstraint->GetType() == SPHERICAL)
	{
		return static_cast<phConstraintSpherical*>(m_TranslationConstraint)->phConstraintSpherical::GetWorldPosB();
	}
	else
	{
		return static_cast<phConstraintDistance*>(m_TranslationConstraint)->phConstraintDistance::GetWorldPosB();
	}
}

void phConstraintAttachment::SetLocalPosA(Vec3V_In localPosA)
{	
	if(m_TranslationConstraint->GetType() == SPHERICAL)
	{
		static_cast<phConstraintSpherical*>(m_TranslationConstraint)->phConstraintSpherical::SetLocalPosA(localPosA);
	}
	else
	{
		static_cast<phConstraintDistance*>(m_TranslationConstraint)->phConstraintDistance::SetLocalPosA(localPosA);
	}
}

void phConstraintAttachment::SetLocalPosB(Vec3V_In localPosB)
{	
	if(m_TranslationConstraint->GetType() == SPHERICAL)
	{
		static_cast<phConstraintSpherical*>(m_TranslationConstraint)->phConstraintSpherical::SetLocalPosB(localPosB);
	}
	else
	{
		static_cast<phConstraintDistance*>(m_TranslationConstraint)->phConstraintDistance::SetLocalPosB(localPosB);
	}
}

Vec3V_Out phConstraintAttachment::GetLocalPosA()
{
	if(m_TranslationConstraint->GetType() == SPHERICAL)
	{
		return static_cast<phConstraintSpherical*>(m_TranslationConstraint)->phConstraintSpherical::GetLocalPosA();
	}
	else
	{
		return static_cast<phConstraintDistance*>(m_TranslationConstraint)->phConstraintDistance::GetLocalPosA();
	}
}

Vec3V_Out phConstraintAttachment::GetLocalPosB()
{
	if(m_TranslationConstraint->GetType() == SPHERICAL)
	{
		return static_cast<phConstraintSpherical*>(m_TranslationConstraint)->phConstraintSpherical::GetLocalPosB();
	}
	else
	{
		return static_cast<phConstraintDistance*>(m_TranslationConstraint)->phConstraintDistance::GetLocalPosB();
	}
}

} // namespace rage
