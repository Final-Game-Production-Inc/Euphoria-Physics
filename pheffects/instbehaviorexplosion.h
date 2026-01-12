//
// pheffects/instbehaviorexplosion.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_INSTBEHAVIOREXPLOSION_H
#define PHEFFECTS_INSTBEHAVIOREXPLOSION_H


#include "explosiontype.h"

#include "phbound/boundsphere.h"
#include "physics/archetype.h"
#include "physics/collider.h"
#include "physics/inst.h"
#include "physics/instbehavior.h"
#include "physics/levelnew.h"
#include "physics/shapetest.h"
#include "physics/simulator.h"
#include "vectormath/classes.h"

namespace rage {

////////////////////////////////////////////////////////////////
// phInstBehaviorExplosion

// PURPOSE
//   Derivation of phInstBehavior that alters an instance's behavior 
// NOTES
//   

class phExplosionCollisionInfo
{
public:
	phInst *m_OtherInst;
	Vector3 m_ForceDir;
	float m_ForceMag;
	int m_Component;
};

class phInstBehaviorExplosion : public phInstBehavior
{
public:
	phInstBehaviorExplosion();
	virtual ~phInstBehaviorExplosion();

	virtual void Reset();

	virtual bool IsActive() const;

	virtual void Update(float TimeStep);

    virtual bool CollideObjects(Vec::V3Param128 timeStep, phInst* myInst, phCollider* myCollider, phInst* otherInst, phCollider* otherCollider, phInstBehavior* otherInstBehavior);

	virtual bool ActivateWhenHit() const;

	// Explosion specific accessors.
	float GetRadius() const;
	float GetRadiusSpeed() const;

	void SetExplosionType(const phExplosionType *ExplosionType);

	// <COMBINE: phInstBehavior::IsForceField>
	virtual bool IsForceField () const { return true; }

protected:
	// Derived classes might want to modify or adjust the force before it gets applied.
	virtual void PreApplyForce (phExplosionCollisionInfo& collisionInfo) const;

	// Derived classes might want to use this to do game-specific things like trigger damage.
	virtual void PostCollision (phExplosionCollisionInfo& collisionInfo) const;

	// Derived classes can override this to customize the place where the force is applied
	virtual bool FindIntersection(phInst* otherInst, phIntersection& iSect);

	float m_Radius, m_RadiusSpeed;

	const phExplosionType *m_ExplosionType;
};


inline phInstBehaviorExplosion::phInstBehaviorExplosion() : phInstBehavior()
{
	m_ExplosionType = NULL;
}


inline phInstBehaviorExplosion::~phInstBehaviorExplosion()
{
}


inline void phInstBehaviorExplosion::Reset()
{
	// Let's set our behavior back to an initial state.
	FastAssert(m_ExplosionType != NULL);
	m_Radius = SMALL_FLOAT;
	m_RadiusSpeed = m_ExplosionType->m_InitialRadiusSpeed;
	FastAssert(m_Instance->GetArchetype()->GetBound()->GetType() == phBound::SPHERE);
	phBoundSphere *BoundSphere = static_cast<phBoundSphere *>(m_Instance->GetArchetype()->GetBound());
	BoundSphere->SetSphereRadius(m_Radius);
}


inline bool phInstBehaviorExplosion::IsActive() const
{
	if(!m_ExplosionType)
	{
		// It's probably not even valid to call this function in this
		// situation, but returning false seems like the safe thing
		// to do. /FF
		return false;
	}
	return m_RadiusSpeed > m_ExplosionType->m_DeactivationRadiusSpeed;
}


inline void phInstBehaviorExplosion::Update(float TimeStep)
{
	FastAssert(IsActive());
	FastAssert(m_ExplosionType != NULL);
	// If this asserts, then the radius of the instance being used isn't matching with what we 
	FastAssert(m_Radius == m_Instance->GetArchetype()->GetBound()->GetRadiusAroundCentroid());

	// Update the instance radius.
	m_RadiusSpeed += m_ExplosionType->m_DecayFactor * m_RadiusSpeed * TimeStep;
	if(IsActive())
	{
		m_Radius += m_RadiusSpeed * TimeStep;
		FastAssert(m_Instance->GetArchetype()->GetBound()->GetType() == phBound::SPHERE);
		phBoundSphere *BoundSphere = static_cast<phBoundSphere *>(m_Instance->GetArchetype()->GetBound());
		BoundSphere->SetSphereRadius(m_Radius);

		// Tell the physics level that the instance has changed, and also tell the simulator that, even though it is inactive, it could still be colliding with other inactives.
		PHLEVEL->UpdateObjectLocationAndRadius(m_Instance->GetLevelIndex(), (Mat34V_Ptr)(NULL));
	}
}


inline bool phInstBehaviorExplosion::CollideObjects(Vec::V3Param128 timeStep, phInst* /*myInst*/, phCollider* /*myCollider*/, phInst* OtherInst, phCollider* /*otherCollider*/, phInstBehavior* /*otherInstBehavior*/)
{
	FastAssert(m_ExplosionType != NULL);

	if(m_RadiusSpeed > 1.0f)
	{
        FastAssert(OtherInst != NULL);

		FastAssert(m_Instance->GetArchetype()->GetBound()->GetType() == phBound::SPHERE);
		phBound &OtherBound = *OtherInst->GetArchetype()->GetBound();

		Mat34V temp = m_Instance->GetMatrix();
		Vector3 ThisInstCenter = VEC3V_TO_VECTOR3(m_Instance->GetArchetype()->GetBound()->GetWorldCentroid( temp ));

		// Find the position on the other bound whereat to apply the force.
		phIntersection ISect;
		if( !FindIntersection( OtherInst, ISect ) )
		{
			// The explosion missed the other object.
			return false;
		}

		// We're probably going to use this later on so let's save it off before we transform into world space.
		Vector3 Normal_OtherInstSpace(RCC_VECTOR3(ISect.GetNormal()));

		// Transform the intersection information into world space.
		Mat34V temp2 = OtherInst->GetMatrix();
		ISect.Transform(temp2);

		phSegment probeSeg;
		probeSeg.A.Set(ThisInstCenter);
		probeSeg.B.Set(RCC_VECTOR3(ISect.GetPosition()));
		phIntersection probeISect;
		if(PHLEVEL->TestProbe(probeSeg, &probeISect, m_Instance) && probeISect.GetInstance() != OtherInst)
		{
			// There's something in the way.
			return false;
		}

		// Find the velocity of the other object at the point of force application.
		phCollider *OtherCollider = PHLEVEL->IsActive(OtherInst->GetLevelIndex()) ? PHSIM->GetCollider(OtherInst->GetLevelIndex()) : NULL;
		Vector3 OtherInstLocalVelocity;
		if(OtherCollider != NULL)
		{
			OtherInstLocalVelocity = VEC3V_TO_VECTOR3(OtherCollider->GetLocalVelocity(RCC_VECTOR3(ISect.GetPosition())));
		}
		else
		{
			OtherInstLocalVelocity = VEC3V_TO_VECTOR3(OtherInst->GetExternallyControlledLocalVelocity(ISect.GetPosition().GetIntrin128()));
		}

		Vector3 unitContactPosToExplosionCenter(ThisInstCenter);
		unitContactPosToExplosionCenter.Subtract(RCC_VECTOR3(ISect.GetPosition()));
		unitContactPosToExplosionCenter.Normalize();
		float OtherInstRelativeSpeed = m_RadiusSpeed + OtherInstLocalVelocity.Dot(unitContactPosToExplosionCenter);
		// Here we linearly interpolate between 0.0f and m_RadiusSpeed here on the distance from the center of the explosion.
		// I tried this, but it didn't work very well for objects that were very close to explosion center because they would rarely get pushed very hard.
		//float ExplosionSpeedAtPointOfContact = m_RadiusSpeed * ThisInstCenter.Dist(ISect.Position) / m_Radius;
		//float OtherInstRelativeSpeed =  ExplosionSpeedAtPointOfContact + OtherInstLocalVelocity.Dot(ISect.Normal);
		if(OtherInstRelativeSpeed > 0.0f)
		{
			// In order to determine the force to apply on the object, we need to know the surface area of the object that is facing the blast.  We do this
			//   by approximating the object as either a sphere or a box, the choice of which is determined by which is a tighter fit (which is smaller).
			float ApproxAreaFacing;

			// Calculate the volume of the bounding sphere.
			float OtherInstRadius = OtherBound.GetRadiusAroundCentroid();
			float VolSphere = (4.0f / 3.0f) * PI * power3(OtherInstRadius);

			// Calculate the volume of the bounding box.
			Vector3 BoxExtents(VEC3V_TO_VECTOR3(OtherBound.GetBoundingBoxMax()));
			BoxExtents.Subtract(VEC3V_TO_VECTOR3(OtherBound.GetBoundingBoxMin()));
			float VolBox = BoxExtents.x * BoxExtents.y * BoxExtents.z;

			if(VolSphere > VolBox)
			{
				// The box is going to be the better approximation for this bound.
				ApproxAreaFacing = fabs(Normal_OtherInstSpace.x * BoxExtents.y * BoxExtents.z) + fabs(Normal_OtherInstSpace.y * BoxExtents.x * BoxExtents.z) + fabs(Normal_OtherInstSpace.z * BoxExtents.x * BoxExtents.y);
			}
			else
			{
				// Wow, the sphere actually won and is the tighter bounding volume.
				ApproxAreaFacing = 2.0f * PI * square(OtherInstRadius);
			}

			// At long last, here's our force!
			Vector3 ForceDir(RCC_VECTOR3(ISect.GetNormal()));
			ForceDir.Negate();							// Remember, the normal in the intersection points out from the object that was intersected.
//				ForceDir.y += 0.1f;							// This is a little cheap hack to give objects a little extra upward force, but we don't need it, we're better than that.
			const float kForceMag = m_ExplosionType->m_ForceFactor * 0.20f * ApproxAreaFacing * square(OtherInstRelativeSpeed);

			// Because the impetus that is stored in the phImpactData is always the force that should be applied to object *A*, if we're object A then we need to invert the force.
			phExplosionCollisionInfo collisionInfo;
			collisionInfo.m_OtherInst = OtherInst;
			collisionInfo.m_ForceDir.Set(ForceDir);
			collisionInfo.m_ForceMag = kForceMag;
			collisionInfo.m_Component = ISect.GetComponent();

			// Give derived classes a chance to modify the force if they want to.
			PreApplyForce(collisionInfo);

			Vector3 Force(collisionInfo.m_ForceDir);
			Force.Scale(collisionInfo.m_ForceMag);

            PHSIM->ApplyForce(timeStep, OtherInst->GetLevelIndex(), Force);

            PostCollision(collisionInfo);
		}
	}

	return false;
}


inline bool phInstBehaviorExplosion::ActivateWhenHit() const
{
	return false;
}


inline float phInstBehaviorExplosion::GetRadius() const
{
	return m_Radius;
}


inline float phInstBehaviorExplosion::GetRadiusSpeed() const
{
	return m_RadiusSpeed;
}


inline void phInstBehaviorExplosion::SetExplosionType(const phExplosionType *ExplosionType)
{
	m_ExplosionType = ExplosionType;
}


inline void phInstBehaviorExplosion::PreApplyForce (phExplosionCollisionInfo& UNUSED_PARAM(collisionInfo)) const
{
}


inline void phInstBehaviorExplosion::PostCollision (phExplosionCollisionInfo& UNUSED_PARAM(collisionInfo)) const
{
}


inline bool phInstBehaviorExplosion::FindIntersection(phInst* otherInst, phIntersection& iSect)
{
	FastAssert(otherInst != NULL);
	FastAssert(m_Instance->GetArchetype()->GetBound()->GetType() == phBound::SPHERE);
	phBound& otherBound = *otherInst->GetArchetype()->GetBound();
	Vector3 thisInstCenter;
    Vector3 ExplosionCenter_OtherInstSpace;

	Mat34V temp = m_Instance->GetMatrix();
	thisInstCenter = VEC3V_TO_VECTOR3(m_Instance->GetArchetype()->GetBound()->GetWorldCentroid(temp));
	Mat34V temp2 = otherInst->GetMatrix();
	(reinterpret_cast<Matrix34*>(&temp))->UnTransform(thisInstCenter, ExplosionCenter_OtherInstSpace);
	phShapeTest<phShapeSphere> sphereTest;
	sphereTest.InitSphere(ExplosionCenter_OtherInstSpace, m_Radius, &iSect);
	if (!sphereTest.TestOneObject(otherBound))
	{
		// The explosion missed the other object.
		return false;
	}

	return true;
}


} // namespace rage

#endif
