// 
// pheffects/spring.cpp
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

// TITLE: Springs
// PURPOSE:
//		This class allows you to attach spring behaviour to physics instances.

#include "spring.h"

#include "physics/collider.h"
#include "physics/colliderdispatch.h"
#include "physics/levelnew.h"
#include "physics/simulator.h"
#include "vectormath/classes.h"

using namespace rage;


// Called once per frame to give you an opportunity to make any changes that you wish to make.
void phSpring::Update (float TimeStep) 
{
	{
		Vector3 vel0;
		Vector3 pt = GetAttachedPositionThis();
		Vector3 po = GetAttachedPositionOther();
		Vector3 springVector = pt - po;							//vector between the two masses

		float r = springVector.Mag();											//distance between the two masses
		
		if( fabsf(r) < SMALL_FLOAT )
		{
			return;
		}

		if( PHLEVEL->IsActive( m_Instance->GetLevelIndex() ) == false )
		{
			PHSIM->ActivateObject( m_Instance->GetLevelIndex() );
		}
		if( m_OtherBody )
		{
			if( PHLEVEL->IsActive( m_OtherBody->GetLevelIndex() ) == false )
			{
				PHSIM->ActivateObject( m_OtherBody->GetLevelIndex() );
			}
		}

		// get the relative velocity
		if( m_OtherBody )
		{
			vel0.Set(RCC_VECTOR3(PHSIM->GetActiveCollider(m_Instance->GetLevelIndex())->GetVelocity()) - RCC_VECTOR3(PHSIM->GetCollider(m_OtherBody)->GetVelocity()));
		}
		else
		{
			vel0.Set(RCC_VECTOR3(PHSIM->GetActiveCollider(m_Instance->GetLevelIndex())->GetVelocity()));
		}

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
			if( m_OtherBody )
			{
				f.Negate();
				PHSIM->GetActiveCollider(m_OtherBody->GetLevelIndex())->ApplyForce(f, po, ScalarVFromF32(TimeStep).GetIntrin128ConstRef());
			}
		} else {  // implicit spring formulation

			float mvel0 = vel0.Dot(springVector);

//*
			const float masst = PHSIM->GetActiveCollider(m_Instance->GetLevelIndex())->GetMass();
			if( m_OtherBody )
			{
				// solved for two bodies.
				const float masso = PHSIM->GetActiveCollider(m_OtherBody->GetLevelIndex())->GetMass();

				const float k = m_SpringConstant;
				const float b = m_SpringDampening;
				const float dt = TimeStep;
				const float ma = masst;
				const float mb = masso;
				const float ftop = -k*( r + dt*mvel0 ) - b*mvel0;
				const float fbottom = (ma*mb + ( mb - ma )*(dt*dt*k + dt*b))/(ma*mb);
				Vector3 f; 
				f.Scale( springVector, ftop/fbottom );

				PHSIM->GetActiveCollider(m_Instance->GetLevelIndex())->ApplyForce(f, pt, ScalarVFromF32(TimeStep).GetIntrin128ConstRef());
				if( m_OtherBody )
				{
					f.Negate();
					PHSIM->GetActiveCollider(m_OtherBody->GetLevelIndex())->ApplyForce(f, po, ScalarVFromF32(TimeStep).GetIntrin128ConstRef());
				}

			}
			else
			{
				// solved for one body
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
}

Vector3 phSpring::GetAttachedPositionOther()
{
	if( m_OtherBody )
	{
		Mat34V transformMat = m_OtherBody->GetMatrix();
		Vec3V r = RCC_VEC3V(m_OtherAttachmentPosition);
		r = Transform( transformMat, r );
		return RCC_VECTOR3(r);
	}
	else
	{
		return m_OtherAttachmentPosition;
	}
}


Vector3 phSpring::GetAttachedPositionThis()
{
	Mat34V transformMat = m_Instance->GetMatrix();
	Vec3V r = RCC_VEC3V(m_OtherAttachmentPosition);
	r = Transform( transformMat, r );
	return RCC_VECTOR3(r);
}

