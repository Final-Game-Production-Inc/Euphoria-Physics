//
// physics/contact.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#if __SPU
#include "vector/vector3_consts_spu.cpp"
#endif // __SPU

#include "contact.h"
#include "phsolver/contactmgr.h"

#if !__SPU
#include "archetype.h"
#include "colliderdispatch.h"
#include "inst.h"
#include "levelnew.h"
#include "manifold.h"
#include "simulator.h"

#include "bank/bkmgr.h"
#include "math/constants.h"
#include "phcore/constants.h"
#include "profile/profiler.h"

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#endif

using namespace rage;

#if CONTACT_VALIDATION
float phContact::sm_MaxMassInvScale = 10.0f;
float phContact::sm_MaxDepth = 25.0f;
float phContact::sm_MaxFriction = 100.0f;
float phContact::sm_MaxElasticity = 100.0f;
float phContact::sm_MaxTargetVelocity = 1000.0f;
float phContact::sm_MaxNormalError = POLYGONMAXNORMALERROR;
Vec3V phContact::sm_MaxPosition = Vec3V(V_FLT_LARGE_6);
Vec3V phContact::sm_MinPosition = Vec3V(V_FLT_LARGE_6);
#endif // CONTACT_VALIDATION

phContact::phContact()
{
	Reset();
}

phContact::phContact (Vec::V3Param128 localPosA, Vec::V3Param128 localPosB, Vec::V3Param128_After3Args worldNormal, Vec::V3Param128_After3Args depth, int elementA, int elementB)
{
    Reset();

    m_Element[0] = (unsigned short )elementA;
    m_Element[1] = (unsigned short )elementB;

	Assert( IsFiniteAll(Vec3V(localPosA)) );
	Assert( IsFiniteAll(Vec3V(localPosB)) );

    m_LocalPointAXYZfrictionW = Vec4V(localPosA);
	m_LocalPointAXYZfrictionW.SetWZero();
    m_LocalPointBXYZelasticityW = Vec4V(localPosB);
	m_LocalPointBXYZelasticityW.SetWZero();

	Vec::Vector_4V v_worldNormal = worldNormal;
    SetWorldNormal(Vec3V(v_worldNormal));

	// Need this splat since the W component is different than the other three.
	// TODO: Can probably fix this by tracing upwards...
    SetDepth( SplatX( Vec3V(depth) ) );

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
	m_DistToTopSurface=ScalarV(V_ZERO);
#endif
}


phContact::~phContact()
{
}


void phContact::Reset ()
{
	m_ActiveContact = true;
	m_LocalPointAXYZfrictionW = Vec4V(V_ZERO);
	m_LocalPointBXYZelasticityW = Vec4V(V_ZERO);
	m_WorldPosXYZmaterialFlagsW[0] = Vec4V(V_ZERO);
	m_WorldPosXYZmaterialFlagsW[1] = Vec4V(V_ZERO);
	m_WorldNormalXYZmaterialWA = Vec4V(V_ZERO);
	m_DinvCol1 = Vec3V(V_ZERO);
	m_ImpulseAXYZuserDatafW = Vec4V(V_ZERO);
    m_PrevSolutionXYZuserDataW = Vec4V(V_ZERO);
	m_DinvCol0materialWB = Vec4V(V_ZERO);
    m_PrevPushXYZdepthW = Vec4V(V_ZERO);
	m_TargetRelVelocityXYZsepBiasW = Vec4V(V_ZERO);
	m_Element[0] = 0;
	m_Element[1] = 0;
    m_Lifetime = 0; 
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
	m_DistToTopSurface=ScalarV(V_ZERO);
#endif
}

#if !__SPU
void phContact::ComputeLocalPointsFromWorld (const phManifold* manifold)
{
	ComputeLocalPointAFromWorld(manifold);
	ComputeLocalPointBFromWorld(manifold);
}
void phContact::ComputeLocalPointAFromWorld (const phManifold* manifold)
{
	Assert(m_WorldPosXYZmaterialFlagsW[0].GetXf() == m_WorldPosXYZmaterialFlagsW[0].GetXf() && m_WorldPosXYZmaterialFlagsW[0].GetYf() == m_WorldPosXYZmaterialFlagsW[0].GetYf() && m_WorldPosXYZmaterialFlagsW[0].GetZf() == m_WorldPosXYZmaterialFlagsW[0].GetZf());

	Mat34V transformA;
	manifold->GetLocalToWorldTransformA(RC_MATRIX34(transformA));
	m_LocalPointAXYZfrictionW.SetXYZ( UnTransformOrtho( transformA, m_WorldPosXYZmaterialFlagsW[0].GetXYZ() ) );

	Assert(m_LocalPointAXYZfrictionW.GetXf() == m_LocalPointAXYZfrictionW.GetXf() && m_LocalPointAXYZfrictionW.GetYf() == m_LocalPointAXYZfrictionW.GetYf() && m_LocalPointAXYZfrictionW.GetZf() == m_LocalPointAXYZfrictionW.GetZf());
}
void phContact::ComputeLocalPointBFromWorld (const phManifold* manifold)
{
	Assert(m_WorldPosXYZmaterialFlagsW[1].GetXf() == m_WorldPosXYZmaterialFlagsW[1].GetXf() && m_WorldPosXYZmaterialFlagsW[1].GetYf() == m_WorldPosXYZmaterialFlagsW[1].GetYf() && m_WorldPosXYZmaterialFlagsW[1].GetZf() == m_WorldPosXYZmaterialFlagsW[1].GetZf());

	Mat34V transformB;
	manifold->GetLocalToWorldTransformB(RC_MATRIX34(transformB));
	m_LocalPointBXYZelasticityW.SetXYZ( UnTransformOrtho( transformB, m_WorldPosXYZmaterialFlagsW[1].GetXYZ() ) );

	Assert(m_LocalPointBXYZelasticityW.GetXf() == m_LocalPointBXYZelasticityW.GetXf() && m_LocalPointBXYZelasticityW.GetYf() == m_LocalPointBXYZelasticityW.GetYf() && m_LocalPointBXYZelasticityW.GetZf() == m_LocalPointBXYZelasticityW.GetZf());
}

void phContact::ComputeWorldPointsFromLocal (const phManifold* manifold)
{
	ComputeWorldPointAFromLocal(manifold);
	ComputeWorldPointBFromLocal(manifold);
}
void phContact::ComputeWorldPointAFromLocal (const phManifold* manifold)
{
	Assert(m_LocalPointAXYZfrictionW.GetXf() == m_LocalPointAXYZfrictionW.GetXf() && m_LocalPointAXYZfrictionW.GetYf() == m_LocalPointAXYZfrictionW.GetYf() && m_LocalPointAXYZfrictionW.GetZf() == m_LocalPointAXYZfrictionW.GetZf());
	
	Mat34V transformA;
	manifold->GetLocalToWorldTransformA(RC_MATRIX34(transformA));
	m_WorldPosXYZmaterialFlagsW[0].SetXYZ(Transform( transformA,  m_LocalPointAXYZfrictionW.GetXYZ() ));

	Assert(m_WorldPosXYZmaterialFlagsW[0].GetXf() == m_WorldPosXYZmaterialFlagsW[0].GetXf() && m_WorldPosXYZmaterialFlagsW[0].GetYf() == m_WorldPosXYZmaterialFlagsW[0].GetYf() && m_WorldPosXYZmaterialFlagsW[0].GetZf() == m_WorldPosXYZmaterialFlagsW[0].GetZf());

}
void phContact::ComputeWorldPointBFromLocal (const phManifold* manifold)
{
	Assert(m_LocalPointBXYZelasticityW.GetXf() == m_LocalPointBXYZelasticityW.GetXf() && m_LocalPointBXYZelasticityW.GetYf() == m_LocalPointBXYZelasticityW.GetYf() && m_LocalPointBXYZelasticityW.GetZf() == m_LocalPointBXYZelasticityW.GetZf());
	
	Mat34V transformB;
	manifold->GetLocalToWorldTransformB(RC_MATRIX34(transformB));
	m_WorldPosXYZmaterialFlagsW[1].SetXYZ(Transform( transformB,  m_LocalPointBXYZelasticityW.GetXYZ() ));

	Assert(m_WorldPosXYZmaterialFlagsW[1].GetXf() == m_WorldPosXYZmaterialFlagsW[1].GetXf() && m_WorldPosXYZmaterialFlagsW[1].GetYf() == m_WorldPosXYZmaterialFlagsW[1].GetYf() && m_WorldPosXYZmaterialFlagsW[1].GetZf() == m_WorldPosXYZmaterialFlagsW[1].GetZf());
}


#endif // !__SPU

void phContact::RefreshContactPoint (const Matrix34& transformA,const Matrix34& transformB,const phManifold* manifold)
{
#if !EARLY_FORCE_SOLVE
	if (m_Lifetime>0)
#endif
	{
		// This point is carried over from the previous frame, so recompute its world positions from its local positions to account for object motion.
		Mat34V _transformA = MATRIX34_TO_MAT34V( transformA );
		Mat34V _transformB = MATRIX34_TO_MAT34V( transformB );
		Vec3V worldPosA, worldPosB;
		// Intentionally separated rotation and translation (Had somehow been affecting precision)
		worldPosA = Transform3x3( _transformA, m_LocalPointAXYZfrictionW.GetXYZ() );
		worldPosA = Add(worldPosA, _transformA.GetCol3());
		worldPosB = Transform3x3( _transformB, m_LocalPointBXYZelasticityW.GetXYZ() );
		worldPosB = Add(worldPosB, _transformB.GetCol3());
		m_WorldPosXYZmaterialFlagsW[0].SetXYZ(worldPosA);
		m_WorldPosXYZmaterialFlagsW[1].SetXYZ(worldPosB);

		if (manifold->GetConstraintType() == phManifold::NO_CONSTRAINT_CONTACT)
		{
			Vec3V delta = Subtract(worldPosA, worldPosB);
			ScalarV depth = Dot(delta, -GetWorldNormal());
			m_PrevPushXYZdepthW.SetWInMemory(depth);
			m_TargetRelVelocityXYZsepBiasW = Vec4V(V_ZERO);
		}
	}
}

#if !__SPU
void phContact::SetLocalNormals (Vec3V_In normal, const phManifold* manifold)
{
    Assert( IsZeroAll(normal) == 0 );
    Mat34V transformA,transformB;
    manifold->GetLocalToWorldTransforms(RC_MATRIX34(transformA),RC_MATRIX34(transformB));
	Vec3V worldNormal = Transform3x3(transformB, normal);
	SetWorldNormal(worldNormal);
}

#endif // !__SPU

#if !__SPU

inline Vec3V_Out GetVelocity (const phCollider* collider, const phInst* instance, Vec3V_In worldPosition, int component)
{
	Vec3V velocity;
	if (collider)
	{
		velocity = collider->GetLocalVelocity(worldPosition.GetIntrin128(), component);
	}
	else if (instance && instance->GetInstFlag(phInst::FLAG_QUERY_EXTERN_VEL))
	{
		velocity = instance->GetExternallyControlledLocalVelocity(worldPosition.GetIntrin128(), component);
	}
	else
	{
		velocity = Vec3V(V_ZERO);
	}

	return velocity;
}


inline Vec3V_Out GetAngVelocity (const phCollider* collider, const phInst* instance, int UNUSED_PARAM(component))
{
	Vec3V angVelocity;
	if (collider)
	{
		angVelocity = collider->GetAngVelocity();
		// TO DO! - make this use the component number to allow rotational constraints on articulated body parts
	}
	else if (instance && instance->GetInstFlag(phInst::FLAG_QUERY_EXTERN_VEL))
	{
		angVelocity = instance->GetExternallyControlledAngVelocity();
	}
	else
	{
		angVelocity = Vec3V(V_ZERO);
	}

	return angVelocity;
}


Vec3V_Out phContact::ComputeRelVelocity (Vec3V_In worldPositionA, Vec3V_In worldPositionB, const phManifold* manifold)
{
	Vec3V relVelocity;
	phCollider* colliderA = manifold->GetColliderA();
	phCollider* colliderB = manifold->GetColliderB();
	if (!manifold->IsRotationConstraint())
	{
		// Find the velocity of object A at the contact point minus the velocity of object B at the contact point.
		relVelocity = Subtract(GetVelocity(colliderA,manifold->GetInstanceA(),worldPositionA,manifold->GetComponentA()), GetVelocity(colliderB,manifold->GetInstanceB(),worldPositionB,manifold->GetComponentB()));
	}
	else
	{
		// Find the angular velocity of object A minus the angular velocity of object B.
		relVelocity = Subtract(GetAngVelocity(colliderA,manifold->GetInstanceA(),manifold->GetComponentA()), GetAngVelocity(colliderB,manifold->GetInstanceB(),manifold->GetComponentB()));
	}

	// Return the relative velocity.
	return relVelocity;
}


#endif // !__SPU


float phContact::GetNormalScale (int UNUSED_PARAM(instanceIndex)) const
{
	return 1.0f;
}


bool phContact::IsStaticFriction(const phManifold* manifold) const
{
	if(manifold->IsConstraint())
		return false;

	ScalarV maxFriction = GetFrictionV() * GetAccumImpulse();
	return IsGreaterThanAll(maxFriction, GetAccumFriction()) ? true : false;
}

#if CONTACT_VALIDATION
void phContact::ValidateMassInvScales(float massInvScaleA, float massInvScaleB)
{
	Assertf(massInvScaleA > 0.0f || massInvScaleB > 0.0f, "Invalid Contact Mass Inv Scales: %f, %f", massInvScaleA, massInvScaleB);
	Assertf(massInvScaleA >= 0.0f && massInvScaleA < sm_MaxMassInvScale, "Invalid Contact Mass Inv Scale A: %f", massInvScaleA);
	Assertf(massInvScaleB >= 0.0f && massInvScaleB < sm_MaxMassInvScale, "Invalid Contact Mass Inv Scale B: %f", massInvScaleB);
}
void phContact::ValidateDepth(float depth)
{
	Assertf(depth < sm_MaxDepth, "Invalid Contact Depth: %f", depth);
}
void phContact::ValidateFriction(float friction)
{
	Assertf(friction >= 0.0f && friction <= sm_MaxFriction, "Invalid Contact Friction: %f", friction);
}
void phContact::ValidateElasticity(float elasticity)
{
	Assertf(elasticity >= 0.0f && elasticity <= sm_MaxElasticity, "Invalid Contact Elasticity: %f", elasticity);
}
void phContact::ValidateTargetVelocity(Vec3V_In targetVelocity)
{
	Assertf(IsLessThanAll(Mag(targetVelocity),ScalarVFromF32(sm_MaxTargetVelocity)),"Invalid Contact Target Velocity: <%f, %f, %f> |%f|", VEC3V_ARGS(targetVelocity), Mag(targetVelocity).Getf());
}
void phContact::ValidateNormal(Vec3V_In normal)
{
	Assertf(IsCloseAll(Mag(normal),ScalarV(V_ONE),ScalarVFromF32(sm_MaxNormalError)),"Invalid Contact Normal: <%f, %f, %f> |%f|", VEC3V_ARGS(normal), Mag(normal).Getf());
}
void phContact::ValidatePosition(Vec3V_In position)
{
	Assertf(IsLessThanAll(position,sm_MaxPosition) && IsGreaterThanAll(position,sm_MinPosition),"Invalid Contact Position: <%f, %f, %f>", VEC3V_ARGS(position));
}
#endif // CONTACT_VALIDATION

#if !__SPU
#if __DEBUGLOG
void phContact::DebugReplay() const
{
	if (!diagDebugLogIsActive(diagDebugLogPhysicsSlow))
		return;

	diagDebugLog(diagDebugLogPhysics, 'pCWA', &m_WorldPosXYZmaterialFlagsW[0]);
	diagDebugLog(diagDebugLogPhysics, 'pCWB', &m_WorldPosXYZmaterialFlagsW[1]);
	diagDebugLog(diagDebugLogPhysics, 'pCNA', &m_WorldNormalXYZmaterialWA);
	diagDebugLog(diagDebugLogPhysics, 'pCNB', &m_DinvCol1);
	diagDebugLog(diagDebugLogPhysics, 'pCIA', &m_ImpulseAXYZuserDatafW);

	diagDebugLog(diagDebugLogPhysics, 'pCPS', &m_PrevSolutionXYZuserDataW);
	diagDebugLog(diagDebugLogPhysics, 'pCPP', &m_PrevPushXYZdepthW);

	diagDebugLog(diagDebugLogPhysics, 'pCEA', &m_ElementA);
	diagDebugLog(diagDebugLogPhysics, 'pCEB', &m_ElementB);

	diagDebugLog(diagDebugLogPhysics, 'pCLT', &m_Lifetime);

	bool temp = m_ActiveContact;
	diagDebugLog(diagDebugLogPhysics, 'pCAc', &temp);
}
#endif

#endif // !__SPU
