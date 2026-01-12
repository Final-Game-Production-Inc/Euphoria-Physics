//
// physics/constraintprismatic.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "constraintprismatic.h"

#include "collider.h"
#include "inst.h"
#include "phbound/boundcomposite.h"
#include "simulator.h"

#include "phcore/pool.h"
#include "vector/colors.h"

namespace rage {

phConstraintPrismatic::phConstraintPrismatic(const Params& params)
	:	phConstraintBase(params), 
		m_MinLimits(params.minDisplacement, 0.0f, 0.0f), 
		m_MaxLimits(params.maxDisplacement, 0.0f, 0.0f)
{
	phInst* instanceA = params.instanceA;
	phInst* instanceB = params.instanceB;

	phConstraintMgr* constraintMgr = ::rage::phSimulator::GetActiveInstance()->GetConstraintMgr();

	m_RotationConstraint = static_cast<phConstraintFixedRotation*>( constraintMgr->AllocateAndConstruct( phConstraintFixedRotation::Params(params) ) );
	if(m_RotationConstraint == NULL)
	{
		FlagForDestruction();
		return;
	}

	for(int i = 0 ; i < ePCM_COUNT ; i++)
	{
		m_Manifolds[i] = AllocateManifold(instanceA, instanceB, &m_ConstraintMatrices[i]);
	}

	//Check whether there were any allocation failures
	if( m_Manifolds[ePCM_COUNT - 1] == NULL )
	{
		FlagForDestruction();
		return;
	}

	for(int i = 0 ; i < ePCM_COUNT ; i++)
	{
		m_Manifolds[i]->SetConstraintType(phManifold::SLIDING_POINT);
	}

	Mat34V componentTransformA, componentTransformB;
	GetWorldMatrix(instanceA, params.componentA, componentTransformA);
	GetWorldMatrix(instanceB, params.componentB, componentTransformB);

	Vec3V originA = componentTransformA.GetCol3(); 
	originA = UnTransformOrtho(componentTransformB, originA);

	Vec3V perpAxis1LocalB, perpAxis2LocalB;
	MakeOrthonormals(params.slideAxisLocalB, perpAxis1LocalB, perpAxis2LocalB);
	m_SlideCoordinateSystem_LocalSpaceB.SetCols(params.slideAxisLocalB, perpAxis1LocalB, perpAxis2LocalB, originA);
}

phConstraintPrismatic::~phConstraintPrismatic()
{
	phConstraintMgr* constraintMgr = ::rage::phSimulator::GetActiveInstance()->GetConstraintMgr();
	constraintMgr->DestructAndFree( m_RotationConstraint );

	for(int i = 0 ; i < ePCM_COUNT ; i++)
	{
		if( m_Manifolds[i] )
		{
			PHMANIFOLD->Release( m_Manifolds[i] );
		}
	}
}

void phConstraintPrismatic::VirtualEnforceWorldConstraints(phCollider* collider)
{
	collider->SetNeedsUpdateBeforeFinalIterations(true);
	collider->SetSolverInvMass(0);
	m_RotationConstraint->phConstraintFixedRotation::VirtualEnforceWorldConstraints(collider);
}

Vec3V_Out phConstraintPrismatic::GetComponentCentreOfMassInLocalSpace(phInst* instance, int component)
{
	if (instance)
	{
		const phBound* bound = instance->GetArchetype()->GetBound();

		if (bound->GetType()==phBound::COMPOSITE)
		{
			const phBoundComposite* compositeBound = static_cast<const phBoundComposite*>(bound);
			bound = compositeBound->GetBound(component);
		}

		return bound->GetCGOffset();

	}
	else
	{
		return Vec3V(V_ZERO);
	}
}

void phConstraintPrismatic::VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver)
{
	m_RotationConstraint->phConstraintFixedRotation::VirtualUpdate(invTimeStep, instanceA, colliderA, instanceB, colliderB, addToSolver);

	Mat34V componentTransformA, componentTransformB;
	GetWorldMatrix(instanceA, m_ComponentA, componentTransformA);
	GetWorldMatrix(instanceB, m_ComponentB, componentTransformB);

	Mat34V slideAxisSpaceToWorldSpace;
	Transform(slideAxisSpaceToWorldSpace, componentTransformB, m_SlideCoordinateSystem_LocalSpaceB);

	Vec3V originAPosition_SlideCoords = UnTransformOrtho(slideAxisSpaceToWorldSpace, componentTransformA.GetCol3());

	Vec3V maxLimitError = originAPosition_SlideCoords - m_MaxLimits - Vec3V(V_FLT_SMALL_5); //small number subtracted to prevent both contacts in a pair being at a 
	Vec3V minLimitError = m_MinLimits - originAPosition_SlideCoords - Vec3V(V_FLT_SMALL_5); //	positive depth simultaneously due to numerical errors (I haven't observed it happen though)
	
	if( m_Swapped )
	{
		//What we think is B is actually inst A of the contact, so the normal has to be inverted
		slideAxisSpaceToWorldSpace = -slideAxisSpaceToWorldSpace;
	}

	phContact*	contact = GetContactPoint(m_Manifolds[ePCM_SLIDEAXIS_MINLIMIT]);
				contact->SetWorldNormal(slideAxisSpaceToWorldSpace.GetCol0());
				contact->SetDepth(minLimitError.GetX());

				contact = GetContactPoint(m_Manifolds[ePCM_SLIDEAXIS_MAXLIMIT]);
				contact->SetWorldNormal(-slideAxisSpaceToWorldSpace.GetCol0());
				contact->SetDepth(maxLimitError.GetX());

				contact = GetContactPoint(m_Manifolds[ePCM_PERPAXIS1_MINLIMIT]);
				contact->SetWorldNormal(slideAxisSpaceToWorldSpace.GetCol1());
				contact->SetDepth(minLimitError.GetY());

				contact = GetContactPoint(m_Manifolds[ePCM_PERPAXIS1_MAXLIMIT]);
				contact->SetWorldNormal(-slideAxisSpaceToWorldSpace.GetCol1());
				contact->SetDepth(maxLimitError.GetY());

				contact = GetContactPoint(m_Manifolds[ePCM_PERPAXIS2_MINLIMIT]);
				contact->SetWorldNormal(slideAxisSpaceToWorldSpace.GetCol2());
				contact->SetDepth(minLimitError.GetZ());

				contact = GetContactPoint(m_Manifolds[ePCM_PERPAXIS2_MAXLIMIT]);
				contact->SetWorldNormal(-slideAxisSpaceToWorldSpace.GetCol2());
				contact->SetDepth(maxLimitError.GetZ());


	Vec3V localCentreOfMassPosA = GetComponentCentreOfMassInLocalSpace(m_Swapped ? instanceB : instanceA, m_Swapped ? m_ComponentB : m_ComponentA);
	Vec3V localCentreOfMassPosB = GetComponentCentreOfMassInLocalSpace(m_Swapped ? instanceA : instanceB, m_Swapped ? m_ComponentA : m_ComponentB);

	for(int i = 0 ; i < ePCM_COUNT ; i++)
	{
		phContact* contact = GetContactPoint(m_Manifolds[i]);

		//Work out where the centre of mass of the component is in and get the solver to apply forces there.
		//It's not obvious what the best position to apply forces is in general when objects are rotating; at least for the 
		//sliding doors attached to the world the centre of mass should give good results
		contact->SetLocalPosA( localCentreOfMassPosA );
		contact->SetLocalPosB( localCentreOfMassPosB );

		m_Manifolds[i]->RefreshContactPoints(-1);
		contact->SetTargetRelVelocity(Vec3V(V_ZERO));

		if (addToSolver)
		{
			AddManifoldToSolver(m_Manifolds[i], instanceA, colliderA, instanceB, colliderB);
		}
	}

	RequestWorldLimitEnforcement();
}

void phConstraintPrismatic::SetBreakable(bool breakable, float breakingStrength)
{
	m_Breakable = breakable;
	m_RotationConstraint->phConstraintFixedRotation::SetBreakable(breakable, breakingStrength);
}

void phConstraintPrismatic::SetBroken(bool broken)
{
	m_Broken = broken;
	m_RotationConstraint->phConstraintFixedRotation::SetBroken(broken);
}

bool phConstraintPrismatic::UpdateBreaking()
{
	if(m_Broken || !m_Breakable) 
		return false;

	for(int i = 0 ; i < ePCM_COUNT ; i++)
	{
		if( BreakIfImpulseSufficient(m_Manifolds[i]) )
		{
			phConstraintPrismatic::SetBroken(true);
			return true;
		}
	}

	if( m_RotationConstraint->UpdateBreaking() )
	{
		phConstraintPrismatic::SetBroken(true);
		return true;
	}

	return false;
}

#if __PFDRAW
void phConstraintPrismatic::ProfileDraw()
{
}
#endif

} // namespace rage
