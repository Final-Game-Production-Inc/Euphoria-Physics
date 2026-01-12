// 
// pheffects/spring.cpp
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

// TITLE: Springs
// PURPOSE:
//		This class allows you to attach spring behaviour to physics instances.

#include "springanimatedattachment.h"

#include "physics/collider.h"
#include "physics/colliderdispatch.h"
#include "physics/inst.h"
#include "physics/levelnew.h"
#include "physics/simulator.h"
#include "vectormath/classes.h"


using namespace rage;


// Called once per frame to give you an opportunity to make any changes that you wish to make.
void phSpringAninmatedAttachment::Update (float TimeStep) 
{
	{
		Vector3 vel0;
		Vector3 pt = GetAttachedPositionThis();
		Vector3 po = GetAttachedPositionOther();
		Vector3 springVector = pt - po;							//vector between the two masses

		float r = springVector.Mag();											//distance between the two masses
		
		Vector3 vatt = (m_OtherAttachmentPosition - m_PrevOtherAttachmentPosition)*1.0f/TimeStep;
		m_PrevOtherAttachmentPosition = m_OtherAttachmentPosition;

		if( fabsf(r) < SMALL_FLOAT )
		{
			return;
		}

		if( PHLEVEL->IsActive( m_Instance->GetLevelIndex() ) == false )
		{
			PHSIM->ActivateObject( m_Instance->GetLevelIndex() );
		}


		vel0.Set(RCC_VECTOR3(PHSIM->GetActiveCollider(m_Instance->GetLevelIndex())->GetVelocity()) - vatt);

		springVector.Scale(1.0f/r);
		r-=m_SpringLength;

		Vector3 f, vel1;
		if(0) {  // explicit spring formulation
			f.Zero();
			{
				r *= -m_SpringConstant;
				f.Scale(springVector,r);
			}

			vel0.Scale( springVector, vel0.Dot(springVector) );

			vel1.Scale( vel0,m_SpringDampening);
			f.Subtract(vel1);
			PHSIM->GetActiveCollider(m_Instance->GetLevelIndex())->ApplyForce(f, pt, ScalarVFromF32(TimeStep).GetIntrin128ConstRef());

		} else {  // implicit spring formulation
			float mvel0 = vel0.Dot(springVector);
			float masst = PHSIM->GetActiveCollider(m_Instance->GetLevelIndex())->GetMass();
			float mass = masst;

			float constovermass = m_SpringConstant/mass;
			float mvel1 = (mvel0 - (constovermass) * r * TimeStep) /
				(1.0f+(m_SpringDampening/(2.0f*mass)) * TimeStep + constovermass*TimeStep*TimeStep);

			f.Scale( springVector,(mvel1-mvel0)*masst );
			PHSIM->GetActiveCollider(m_Instance->GetLevelIndex())->ApplyImpulse(f, pt);

		}

	}
}

Vector3 phSpringAninmatedAttachment::GetAttachedPositionOther()
{ 
	return m_OtherAttachmentPosition;
}


Vector3 phSpringAninmatedAttachment::GetAttachedPositionThis()
{ 
	Vec3V r = *(reinterpret_cast<Vec3V*>(&m_ThisAttachmentPosition));
	Mat34V transformMat = m_Instance->GetMatrix();
	r = Transform( transformMat, r );
	return *(reinterpret_cast<Vector3*>(&r));
}

