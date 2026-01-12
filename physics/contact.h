//
// physics/contact.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONTACT_H
#define PHYSICS_CONTACT_H

#include "phsolver/forcesolverconfig.h"

#include "diag/debuglog.h"
#include "phbound/bound.h"
#include "phcore/constants.h"
#include "vector/matrix34.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#define CONTACT_VALIDATION (__DEV && !__SPU)
#if CONTACT_VALIDATION
#define CONTACT_VALIDATION_ONLY(x) x
#else
#define CONTACT_VALIDATION_ONLY(x)
#endif

namespace rage {


class bkBank;
class phCollider;
class phInst;
class phManifold;

// PURPOSE
//   phContact keeps collision information for a single interaction between two objects.
//   The collection of contacts keeps the collision information for all interactions in a
//   single frame, and are used to calculate impulses simultaneously.
// NOTES
//   Contacts are calculated by solving a large matrix equation for all interacting contacts
//   simultaneously.
class phContact
{
public:
	phContact();
	
	phContact (Vec::V3Param128 localPosA, Vec::V3Param128 localPosB, Vec::V3Param128_After3Args worldNormal, Vec::V3Param128_After3Args depth, int elementA, int elementB);

	~phContact();

	static const char* GetClassName()
	{
		return "phContact";
	}

	__forceinline void AboutToBeReleased() { }

	// PURPOSE: Reset the contact.
	void Reset ();

	// PURPOSE: Untransform the world positions to get the contact positions in the objects' coordinate systems.
	void ComputeLocalPointsFromWorld (const phManifold* manifold);
	void ComputeLocalPointAFromWorld (const phManifold* manifold);
	void ComputeLocalPointBFromWorld (const phManifold* manifold);

	// PURPOSE: Transform the local positions to get the world positions.
	void ComputeWorldPointsFromLocal (const phManifold* manifold);
	void ComputeWorldPointAFromLocal (const phManifold* manifold);
	void ComputeWorldPointBFromLocal (const phManifold* manifold);

	// PURPOSE: Compute the world positions from the local positions, get velocities from the objects, and increment the lifetime.
	void RefreshContactPoint (const Matrix34& transformA,const Matrix34& transformB,const phManifold* manifold);

	void IncrementLifetime () { m_Lifetime++; }
	phInst* GetOtherInstance (const phInst* instance);
	const phInst* GetOtherInstance (const phInst* instance) const;
	Vec3V_Out GetWorldPosA () const { return m_WorldPosXYZmaterialFlagsW[0].GetXYZ(); }
	void SetWorldPosA(Vec3V_In worldPos) { m_WorldPosXYZmaterialFlagsW[0].SetXYZ(worldPos); }
	Vec3V_Out GetWorldPosB () const { return m_WorldPosXYZmaterialFlagsW[1].GetXYZ(); }
	void SetWorldPosB(Vec3V_In worldPos) { m_WorldPosXYZmaterialFlagsW[1].SetXYZ(worldPos); }
	Vec3V_Out GetWorldPos (int objectIndex) const { return m_WorldPosXYZmaterialFlagsW[objectIndex].GetXYZ(); }

	Vec3V_Out GetLocalPosA () const { return m_LocalPointAXYZfrictionW.GetXYZ(); }
	void SetLocalPosA(Vec3V_In localPos) { m_LocalPointAXYZfrictionW.SetXYZ(localPos); }
	Vec3V_Out GetLocalPosB () const { return m_LocalPointBXYZelasticityW.GetXYZ(); }
	void SetLocalPosB(Vec3V_In localPos) { m_LocalPointBXYZelasticityW.SetXYZ(localPos); }
	Vec3V_Out GetWorldNormal () const { return m_WorldNormalXYZmaterialWA.GetXYZ(); }
	// Would really prefer to not perform this normalize - It is a hidden cost that callers are unlikely to know about
	// - If ever removed we should reenable some non-normal assert within the collision task (f.ex. The bottom of phPairwiseCollisionProcessor::ProcessBoundVsTriangle)
	void SetWorldNormal (Vec3V_In normal) { m_WorldNormalXYZmaterialWA.SetXYZ(NormalizeSafe(normal, Vec3V(V_Z_AXIS_WZERO))); }
	void SetLocalNormals (Vec3V_In normal, const phManifold* manifold);

	Vec3V_Out GetImpulseA () const { return m_ImpulseAXYZuserDatafW.GetXYZ(); }

	int GetElementA() const { return m_Element[0]; }
	int GetElementB() const { return m_Element[1]; }
	int GetElement(int objectIndex) const { return m_Element[objectIndex]; }

	// PURPOSE: Get the unit normal for the given instance in this contact.
	// PARAMS:
	//	instanceIndex - index number of the instance in this contact to get the normal (0==m_InstanceA, 1==m_InstanceB).
	// RETURN:	a reference to the world normal vector for the given instance
	// NOTES:
	//	1.	The normal vector is in world coordinates, and points into the object from its surface for a collision.
	//	2.	Rope contacts derive this method to return the normal vector for wrapped objects (instanceIndex>1).
	Vec3V_Out GetWorldNormal (int instanceIndex) const { return (instanceIndex==0 ? GetWorldNormal() : -GetWorldNormal()); }

	phMaterialMgr::Id GetMaterialIdA() const;
	phMaterialMgr::Id GetMaterialIdB() const;
	inline phMaterialMgr::Id GetMaterialId(int objectIndex) const { return objectIndex == 0 ? GetMaterialIdA() : GetMaterialIdB(); } 

	void SetMaterialIds(phMaterialMgr::Id materialIdA, phMaterialMgr::Id materialIdB);

	// PURPOSE: Get the contact depth.
	// RETURN:	the amount of penetration of this contact
	inline float GetDepth () const { return m_PrevPushXYZdepthW.GetWf(); }

	// PURPOSE: Get the contact depth.
	// RETURN:	the amount of penetration of this contact in every component of a vector
	// NOTES:	The depth is stored in the w-component of m_PrevSolutionXYZuserDataW.
	inline ScalarV_Out GetDepthV () const { return m_PrevPushXYZdepthW.GetW(); }
	
	// PURPOSE: Set the depth of this contact.
	// PARAMS:
	//	depth - the new depth for this contact
	// NOTES:	The depth is stored in the w-component of m_PrevSolutionXYZuserDataW.
	void SetDepth (ScalarV_In depth);

	// PURPOSE: Set the depth of this contact.
	// PARAMS:
	//	depth - the new depth for this contact
	// NOTES:	The depth is stored in the w-component of m_PrevSolutionXYZuserDataW.
	void SetDepth (float depth);

	// PURPOSE: Get the contact friction.
	// RETURN:	the friction of this contact
	// NOTES:	The friction is stored in the w-component of m_LocalPointAXYZfrictionW.
	inline float GetFriction () const { return m_LocalPointAXYZfrictionW.GetWf(); }
	inline ScalarV_Out GetFrictionV() const { return SplatW(m_LocalPointAXYZfrictionW); }

	// PURPOSE: Set the friction of this contact.
	// PARAMS:
	//	friction - the new friction for this contact
	// NOTES:	The friction is stored in the w-component of m_LocalPointAXYZfrictionW.
	inline void SetFriction (float friction) { m_LocalPointAXYZfrictionW.SetWf(friction); }


	// PURPOSE: Get the contact elasticity.
	// RETURN:	the elasticity of this contact
	// NOTES:	The elasticity is stored in the w-component of m_LocalPointBXYZelasticityW.
	inline float GetElasticity () const { return m_LocalPointBXYZelasticityW.GetWf(); }
	inline ScalarV_Out GetElasticityV () const { return SplatW(m_LocalPointBXYZelasticityW);	}

	// PURPOSE: Set the elasticity of this contact.
	// PARAMS:
	//	elasticity - the new elasticity for this contact
	// NOTES:	The elasticity is stored in the w-component of m_LocalPointBXYZelasticityW.
	inline void SetElasticity (float elasticity) { m_LocalPointBXYZelasticityW.SetWf(elasticity); }

	inline void ActivateContact (bool active = true) { m_ActiveContact = active; }
	inline bool IsContactActive () const { return m_ActiveContact; }

#if POSITIVE_DEPTH_ONLY
	__forceinline const bool& IsPositiveDepth() const { return m_PositiveDepth; }
	__forceinline void SetPositiveDepth(bool positiveDepth) { m_PositiveDepth = positiveDepth; }
#endif // POSITIVE_DEPTH_ONLY

	static Vec3V_Out ComputeRelVelocity(Vec3V_In worldPositionA, Vec3V_In worldPositionB, const phManifold* manifold);

	// PURPOSE: Tell whether this contact is owned and managed by the contact manager.
	// RETURN: true for all phContacts
	// NOTES:	This should be overridden in derived contacts to return false (phClothContact is an example).
	bool IsManaged () const { return true; }

	// PURPOSE: Get a scale factor for the normal vector, for derived contacts.
	// NOTES:	This is used for rope wrapping around objects.
	float GetNormalScale (int instanceIndex) const;

	void ZeroPreviousSolution ();

	inline Vec3V_Out GetPreviousSolution () const { return m_PrevSolutionXYZuserDataW.GetXYZ();	}
	inline void SetPreviousSolution (Vec3V_In ps) { m_PrevSolutionXYZuserDataW = GetFromTwo<Vec::X1,Vec::Y1,Vec::Z1,Vec::W2>(Vec4V(ps), m_PrevSolutionXYZuserDataW); FastAssert(HasValidImpulse());	}

	inline Vec3V_Out GetPreviousPush() const { return m_PrevPushXYZdepthW.GetXYZ();	}
	inline void SetPreviousPush (Vec3V_In pp) {	m_PrevPushXYZdepthW = GetFromTwo<Vec::X1,Vec::Y1,Vec::Z1,Vec::W2>(Vec4V(pp), m_PrevPushXYZdepthW); }
	inline int GetLifetime () const { return m_Lifetime; }
	inline void SetLifetime (u16 newLifetime) { m_Lifetime = newLifetime; }

    void CopyPersistentInformation (const phContact& other);
	void CopyElements (const phContact& other);
	
	inline Vec3V_Out GetTargetRelVelocity () const	{ return m_TargetRelVelocityXYZsepBiasW.GetXYZ(); }
	__forceinline void SetTargetRelVelocity (Vec3V_In targetRelVelocity) { m_TargetRelVelocityXYZsepBiasW.SetXYZ(targetRelVelocity); FastAssert(IsFiniteAll(m_TargetRelVelocityXYZsepBiasW)); }


#if __DEBUGLOG
	void DebugReplay() const;
#endif

	inline void SetUserData(void* userData) { *((void**)&m_PrevSolutionXYZuserDataW[3]) = userData; FastAssert(HasValidImpulse()); }
	inline void* GetUserData() const { return *((void**)&(m_PrevSolutionXYZuserDataW[3]));	}

	// *** New Solver ***
	void Exchange()
	{
		Vec3V temp = GetLocalPosA();
		SetLocalPosA(GetLocalPosB());
		SetLocalPosB(temp);
		temp = GetWorldPosA();
		SetWorldPosA(GetWorldPosB());
		SetWorldPosB(temp);
		SwapEm(m_Element[0], m_Element[1]);
		SetWorldNormal(Negate(GetWorldNormal()));
		SetMaterialIds(GetMaterialIdB(), GetMaterialIdA());
		SetTargetRelVelocity(-GetTargetRelVelocity());
	}


	inline bool HasValidImpulse() const
	{
// 		Vector4 v4=RCC_VECTOR4(m_PrevSolutionXYZuserDataW);
// 		return (v4.x==v4.x && v4.y==v4.y && v4.z==v4.z);
// NOTE: the following is a better
		return IsFiniteAll(m_PrevSolutionXYZuserDataW.GetXYZ());
	}

	Vec3V_Out GetTangent() const { return m_PrevPushXYZdepthW.GetXYZ(); }	
	void SetTangent(Vec3V_In tangent)	{ m_PrevPushXYZdepthW.SetXYZ(tangent); }
	ScalarV_Out GetAccumImpulse() const { return m_PrevSolutionXYZuserDataW.GetX(); }	
	__forceinline void SetAccumImpulse(ScalarV_In accumImpulse)
	{
		m_PrevSolutionXYZuserDataW.SetXInMemory(accumImpulse); 
		FastAssert(HasValidImpulse());
	}
	ScalarV_Out GetAccumFriction() const { return m_PrevSolutionXYZuserDataW.GetY(); }	
	__forceinline void SetAccumFriction(ScalarV_In accumFriction)
	{ 
		m_PrevSolutionXYZuserDataW.SetYInMemory(accumFriction);
		FastAssert(HasValidImpulse());
	}
	ScalarV_Out GetAccumPush() const { return m_PrevSolutionXYZuserDataW.GetZ(); }	
	__forceinline void SetAccumPush(ScalarV_In accumPush)
	{
		m_PrevSolutionXYZuserDataW.SetZInMemory(accumPush); 
		FastAssert(HasValidImpulse());
	}

	ScalarV_Out GetImpulseDen() { return m_ImpulseAXYZuserDatafW.GetX(); }	
	void SetImpulseDen(ScalarV_In impulseDen) { m_ImpulseAXYZuserDatafW.SetXInMemory(impulseDen); }
	ScalarV_Out GetFrictionDen() { return m_ImpulseAXYZuserDatafW.GetY(); }	
	void SetFrictionDen(ScalarV_In frictionDen) { m_ImpulseAXYZuserDatafW.SetYInMemory(frictionDen); }

#if USE_PRECOMPUTE_SEPARATEBIAS
	void SetSeparateBias( ScalarV_In depth, ScalarV_In separateBiasMultiplier, ScalarV_In allowedPenetration, ScalarV_In invTime );
	void UpdateSeparateBias( ScalarV_In separateBiasMultiplier, ScalarV_In allowedPenetration, ScalarV_In invTime );
	void DisableSeparateBias() { m_TargetRelVelocityXYZsepBiasW.SetW(ScalarV(V_ZERO)); }
	ScalarV_Out GetSeparateBias_() { return m_TargetRelVelocityXYZsepBiasW.GetW(); }
#endif // USE_PRECOMPUTE_SEPARATEBIAS

	void SetDinv(Mat33V_In dinv)
	{
		m_DinvCol0materialWB.SetXYZ(dinv.GetCol0());
		m_DinvCol1 = dinv.GetCol1();
		m_ImpulseAXYZuserDatafW.SetXYZ(dinv.GetCol2());
		
		FastAssert( IsFiniteAll( m_DinvCol0materialWB.GetXYZ() ) );
		FastAssert( IsFiniteAll( m_DinvCol1 ) );
		FastAssert( IsFiniteAll( m_ImpulseAXYZuserDatafW.GetXYZ() ) );
	}
	Mat33V_Out GetDinv()
	{
		return Mat33V(m_DinvCol0materialWB.GetXYZ(),
			m_DinvCol1,
			m_ImpulseAXYZuserDatafW.GetXYZ()); 
	}

	void SetFrictionPlaneProjection(Vec3V_In fpp) { m_FrictionPlaneProjection = fpp; }
	Vec3V_Out GetFrictionPlaneProjection() { return m_FrictionPlaneProjection; }

	bool IsStaticFriction(const phManifold* manifold) const;

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
	void SetDistToTopSurface(ScalarV_In distToTopSurface) {m_DistToTopSurface=distToTopSurface;}
	ScalarV GetDistToTopSurface() const {return m_DistToTopSurface;}
#endif

	inline Vec3V_Out ComputeTotalImpulse() const { return Add(Scale(GetWorldNormal(),GetAccumImpulse()), Scale(GetTangent(),GetAccumFriction())); }
	Vec3V_Out ComputeTotalArtFixImpulse() const;

#if CONTACT_VALIDATION
	static void ValidateMassInvScales(float massInvScaleA, float massInvScaleB);
	static void ValidateDepth(float depth);
	static void ValidateFriction(float friction);
	static void ValidateElasticity(float elasticity);
	static void ValidateTargetVelocity(Vec3V_In targetVelocity);
	static void ValidateNormal(Vec3V_In normal);
	static void ValidatePosition(Vec3V_In position);
#endif // CONTACT_VALIDATION

protected:

	Vec4V m_PrevPushXYZdepthW; // Contains the depth in the w component
	
	Vec4V m_LocalPointAXYZfrictionW;
	Vec4V m_LocalPointBXYZelasticityW;

	Vec4V m_WorldPosXYZmaterialFlagsW[2];

	Vec4V m_WorldNormalXYZmaterialWA;
	Vec3V m_DinvCol1;
	Vec4V m_ImpulseAXYZuserDatafW;

	Vec4V m_PrevSolutionXYZuserDataW;

	Vec4V m_DinvCol0materialWB;

	Vec3V m_FrictionPlaneProjection;

	// PURPOSE: the target relative velocity after the collision
	// NOTES:	This is normally zero, and is stored here so that it can be modified as the effective collision velocity before computing the collision response.
	Vec4V m_TargetRelVelocityXYZsepBiasW;

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
	ScalarV m_DistToTopSurface;
#endif

	u16 m_Element[2];

	u16 m_Lifetime;

	bool m_ActiveContact;

#if POSITIVE_DEPTH_ONLY
	bool m_PositiveDepth;
#endif // POSITIVE_DEPTH_ONLY

#if CONTACT_VALIDATION
public:
	static float sm_MaxMassInvScale;
	static float sm_MaxDepth;
	static float sm_MaxFriction;
	static float sm_MaxElasticity;
	static float sm_MaxTargetVelocity;
	static float sm_MaxNormalError;
	static Vec3V sm_MaxPosition;
	static Vec3V sm_MinPosition;
protected:
#endif // CONTACT_VALIDATION
};

inline void phContact::SetMaterialIds(phMaterialMgr::Id materialIdA, phMaterialMgr::Id materialIdB)
{
#if PH_MATERIAL_ID_64BIT
	// Store the material flags
	m_WorldPosXYZmaterialFlagsW[0].SetWi((int)((materialIdA & 0xffffffff00000000LL) >> 32));
	m_WorldPosXYZmaterialFlagsW[1].SetWi((int)((materialIdB & 0xffffffff00000000LL) >> 32));
	// Store the materialID
	m_WorldNormalXYZmaterialWA.SetWi((int)materialIdA);
	m_DinvCol0materialWB.SetWi((int)materialIdB);
#else
	m_WorldNormalXYZmaterialWA.SetWi(materialIdA);
	m_DinvCol0materialWB.SetWi(materialIdB);
#endif // PH_MATERIAL_ID_64BIT
	Assert(materialIdA == GetMaterialIdA());
	Assert(materialIdB == GetMaterialIdB());
}

#if USE_NEGATIVE_DEPTH_TUNABLE
extern bool g_UseNegativeDepth;
extern bool g_UseNegativeDepthForceCancel;
extern bool g_UseNegativeDepthForceCancel1;
extern bool g_UseRemoveContact;
extern float g_NegativeDepthBias0;
extern float g_NegativeDepthBias1;
extern float g_NegativeDepthPenetrationBias;
extern float g_PositiveDepthBias;
extern float g_RemoveContactDistance;
extern float g_AllowedPenetrationOffset;
#endif // USE_NEGATIVE_DEPTH_TUNABLE

#define POSITIVE_DEPTH_ONLY_NEW (POSITIVE_DEPTH_ONLY && (!USE_NEGATIVE_DEPTH || USE_NEGATIVE_DEPTH_TUNABLE))

// TODO: MOVE THESE CONTACT CONSTANTS INTO A SEPARATE HEADER.

// CONTACT_PENETRATION_OFFSET is used to reduce or eliminate allowing penetration.
#if USE_CONTACT_PENETRATION_OFFSET
const float CONTACT_PENETRATION_OFFSET = 0.0035f;			// Set to counteract the current 'Baumgarte Allowed Penetration'.
const ScalarV CONTACT_PENETRATION_OFFSET_V(ScalarVFromF32(CONTACT_PENETRATION_OFFSET));
const ScalarV HALF_CONTACT_PENETRATION_OFFSET_V(ScalarVFromF32(0.5f*CONTACT_PENETRATION_OFFSET));
#else // USE_CONTACT_PENETRATION_OFFSET
const float CONTACT_PENETRATION_OFFSET = 0.0f;
#endif // USE_CONTACT_PENETRATION_OFFSET

// ACTIVE_COLLISION_DISTANCE is the distance at which we will add new contact points.
// Bounding volumes or bounding volumes tests need to be adjusted by this amount.
// CCD Needs to take this distance into account at the end of the translation, especially for objects at or close to rest.
#if USE_ACTIVE_COLLISION_DISTANCE
const float ACTIVE_COLLISION_DISTANCE_EXTRA = 0.001f;		// Epsilon to help ensure contact persistence.
const float ACTIVE_COLLISION_DISTANCE = -POSITIVE_DEPTH_ONLY_THRESHOLD + ACTIVE_COLLISION_DISTANCE_EXTRA;
#define ACTIVE_COLLISION_DISTANCE_V ScalarVFromF32(ACTIVE_COLLISION_DISTANCE)
#define HALF_ACTIVE_COLLISION_DISTANCE_V ScalarVFromF32(0.5f*ACTIVE_COLLISION_DISTANCE)
#endif // USE_ACTIVE_COLLISION_DISTANCE

#if USE_NEGATIVE_DEPTH
const float NEGATIVE_DEPTH_NO_DIST_ZONE_THRESHOLD = -0.0055f + CONTACT_PENETRATION_OFFSET;	// This is the sum of allowedPenetration(0.0035) and POSITIVE_DEPTH_ONLY_THRESHOLD(0.002)
#define NEGATIVE_DEPTH_NO_DIST_ZONE_THRESHOLD_V ScalarVFromF32(NEGATIVE_DEPTH_NO_DIST_ZONE_THRESHOLD)
#endif // USE_NEGATIVE_DEPTH

#if USE_PRECOMPUTE_SEPARATEBIAS
inline void phContact::SetSeparateBias(ScalarV_In depth, ScalarV_In separateBiasMultiplier, ScalarV_In allowedPenetration, ScalarV_In invTime)
{
#if USE_NEGATIVE_DEPTH_TUNABLE
	ScalarV separateBias;
	if (g_UseNegativeDepth)
	{
#if USE_CONTACT_PENETRATION_OFFSET
		const ScalarV dif = depth - (allowedPenetration - ScalarVFromF32(g_AllowedPenetrationOffset)) + CONTACT_PENETRATION_OFFSET_V;
#else // USE_CONTACT_PENETRATION_OFFSET
		const ScalarV dif = depth - (allowedPenetration - ScalarVFromF32(g_AllowedPenetrationOffset));
#endif // USE_CONTACT_PENETRATION_OFFSET
		//const ScalarV NoDistZoneThresh = ScalarV(g_NegativeDepthPenetrationBias) * -allowedPenetration;
		const ScalarV NoDistZoneThresh = ScalarV(g_NegativeDepthPenetrationBias) * (NEGATIVE_DEPTH_NO_DIST_ZONE_THRESHOLD_V - ScalarVFromF32(g_AllowedPenetrationOffset));
		const ScalarV NoDistZone = Max(dif,NoDistZoneThresh);
		const ScalarV DistZone = dif - NoDistZone;
		const ScalarV separateBiasNeg = (ScalarV(g_NegativeDepthBias1) * DistZone + ScalarV(g_NegativeDepthBias0) * NoDistZone) * invTime;
		const ScalarV separateBiasPos = ScalarV(g_PositiveDepthBias) * separateBiasMultiplier * Min(dif, allowedPenetration);
		//const BoolV IsPos = IsGreaterThanOrEqual(depth,allowedPenetration);
		const BoolV IsPos = IsGreaterThanOrEqual(dif,ScalarV(V_ZERO));
		const ScalarV separateBias_ = SelectFT(IsPos,separateBiasNeg,separateBiasPos);
		separateBias = separateBias_;
	}
	else
	{
#if USE_CONTACT_PENETRATION_OFFSET
		const ScalarV separateBias_ = separateBiasMultiplier * Max(ScalarV(V_ZERO), Min(depth - allowedPenetration + CONTACT_PENETRATION_OFFSET_V, allowedPenetration));
#else // USE_CONTACT_PENETRATION_OFFSET
		const ScalarV separateBias_ = separateBiasMultiplier * Max(ScalarV(V_ZERO), Min(depth - allowedPenetration, allowedPenetration));
#endif // USE_CONTACT_PENETRATION_OFFSET
		separateBias = separateBias_;
	}
#elif USE_NEGATIVE_DEPTH
#if USE_CONTACT_PENETRATION_OFFSET
	const ScalarV dif = depth - allowedPenetration + CONTACT_PENETRATION_OFFSET_V;
#else
	const ScalarV dif = depth - allowedPenetration;
#endif
	//const ScalarV NoDistZoneThresh = -allowedPenetration;
	const ScalarV NoDistZoneThresh = NEGATIVE_DEPTH_NO_DIST_ZONE_THRESHOLD_V;
	const ScalarV NoDistZone = Max(dif,NoDistZoneThresh);
	const ScalarV DistZone = dif - NoDistZone;
	const ScalarV separateBiasNeg = DistZone * invTime;
	const ScalarV separateBiasPos = separateBiasMultiplier * Min(dif, allowedPenetration);
	//const BoolV IsPos = IsGreaterThanOrEqual(depth,allowedPenetration);
	const BoolV IsPos = IsGreaterThanOrEqual(dif,ScalarV(V_ZERO));
	const ScalarV separateBias = SelectFT(IsPos,separateBiasNeg,separateBiasPos);
#else
	(void)invTime;
	const ScalarV separateBias = separateBiasMultiplier * Max(ScalarV(V_ZERO), Min(depth - allowedPenetration, allowedPenetration));
#endif // USE_NEGATIVE_DEPTH
	m_TargetRelVelocityXYZsepBiasW.SetWInMemory( separateBias );
}

inline void phContact::UpdateSeparateBias(ScalarV_In separateBiasMultiplier, ScalarV_In allowedPenetration, ScalarV_In invTime)
{
	SetSeparateBias(GetDepthV(),separateBiasMultiplier,allowedPenetration,invTime);
}
#endif // USE_PRECOMPUTE_SEPARATEBIAS

inline void phContact::SetDepth (ScalarV_In depth)
{
	m_PrevPushXYZdepthW = GetFromTwo<Vec::X2,Vec::Y2,Vec::Z2,Vec::X1>(Vec4V(depth), m_PrevPushXYZdepthW );
}

inline void phContact::SetDepth (float depth)
{
	m_PrevPushXYZdepthW.SetWf( depth );
}


inline void phContact::CopyPersistentInformation (const phContact& other)
{
    m_PrevSolutionXYZuserDataW = other.m_PrevSolutionXYZuserDataW;
	FastAssert(HasValidImpulse());

	// Don't set the previous push directly, to avoid overwriting the depth in its w component.
	SetPreviousPush(other.GetPreviousPush());
}

inline void phContact::CopyElements (const phContact& other)
{
	m_Element[0] = other.m_Element[0];
	m_Element[1] = other.m_Element[1];
	m_WorldNormalXYZmaterialWA.SetWf(other.m_WorldNormalXYZmaterialWA.GetWf());
	m_DinvCol0materialWB.SetWf(other.m_DinvCol0materialWB.GetWf());
	m_LocalPointAXYZfrictionW.SetWf(other.m_LocalPointAXYZfrictionW.GetWf());
	m_LocalPointBXYZelasticityW.SetWf(other.m_LocalPointBXYZelasticityW.GetWf());
}

inline phMaterialMgr::Id phContact::GetMaterialIdA() const 
{ 
#if PH_MATERIAL_ID_64BIT
	return (((u64)(u32)m_WorldPosXYZmaterialFlagsW[0].GetWi()) << 32) | (u64)(u32)m_WorldNormalXYZmaterialWA.GetWi(); 
#else
	return phMaterialMgr::Id(m_WorldNormalXYZmaterialWA.GetWi()); 
#endif
}

inline phMaterialMgr::Id phContact::GetMaterialIdB() const 
{ 
#if PH_MATERIAL_ID_64BIT
	return (((u64)(u32)m_WorldPosXYZmaterialFlagsW[1].GetWi()) << 32) | (u64)(u32)m_DinvCol0materialWB.GetWi(); 
#else
	return phMaterialMgr::Id(m_DinvCol0materialWB.GetWi()); 
#endif
}

inline void phContact::ZeroPreviousSolution ()
{
	Vec4V _maskw(V_MASKW);
	m_PrevSolutionXYZuserDataW = And( m_PrevSolutionXYZuserDataW, _maskw );
#if HACK_GTA4
	FastAssert(HasValidImpulse());
#endif
	m_PrevPushXYZdepthW = And( m_PrevPushXYZdepthW, _maskw );
}

inline Vec3V_Out phContact::ComputeTotalArtFixImpulse() const
{
	Mat33V constraintAxis;
	Vec3V worldNormal = GetWorldNormal();
	MakeOrthonormals(worldNormal, constraintAxis.GetCol1Ref(), constraintAxis.GetCol2Ref());
	constraintAxis.SetCol0(worldNormal);
	return Multiply(constraintAxis, Negate(GetPreviousSolution()));
}



} // namespace rage

#endif
