// 
// physics/manifold.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_MANIFOLD_H
#define PHYSICS_MANIFOLD_H

#include "compositepointers.h"
#include "handle.h"

#include "diag/debuglog.h"
#include "phcore/constants.h"
#include "phbullet/GjkSimplexCache.h"
#include "phsolver/forcesolverconfig.h"
#include "physics/collider.h"
#include "system/cache.h"
#include "system/timer.h"
#include "vectormath/classes.h"

#if __PS3
#include "system/dmaplan.h"
#endif

#if  __ASSERT && __PPU
struct sys_event;
#endif

struct PairListWorkUnitInput;

namespace rage {

#define PH_MANIFOLD_TRACK_IS_IN_OPA 0

#define PER_MANIFOLD_SOLVER_METRICS 0

#if PER_MANIFOLD_SOLVER_METRICS
#define PER_MANIFOLD_SOLVER_METRICS_ONLY(X) X
#else
#define PER_MANIFOLD_SOLVER_METRICS_ONLY(X)
#endif

#if __ASSERT
extern const float CONTACT_DEPTH_ASSERT_THRESHOLD;
#endif // ASSERT

// TODO: MOVE THESE CONTACT CONSTANTS INTO A SEPARATE HEADER.
#define REMOVE_SEPARATED_CONTACTS 1
const float REMOVE_SEPARATED_CONTACTS_VERTICAL_DIST = 0.04f;
#define REMOVE_SEPARATED_CONTACTS_VERTICAL_DIST_V ScalarVFromF32(REMOVE_SEPARATED_CONTACTS_VERTICAL_DIST)
const float REMOVE_SEPARATED_CONTACTS_HORIZONTAL_DIST = 0.02f;
#define REMOVE_SEPARATED_CONTACTS_HORIZONTAL_DIST_SQ_V ScalarVFromF32(REMOVE_SEPARATED_CONTACTS_HORIZONTAL_DIST*REMOVE_SEPARATED_CONTACTS_HORIZONTAL_DIST)

//const float NEW_CONTACTS_ACTIVE_DIST = 0.02f;
//const ScalarV NEW_CONTACTS_ACTIVE_DIST_V(ScalarVFromF32(NEW_CONTACTS_ACTIVE_DIST));

class phBound;
class phContact;
class phCollider;
class phInst;
struct phTaskCollisionPair;
class phSpuManifoldWrapper;

class phManifold
{
public:

	// <COMBINE phManifold::m_ConstraintType>
	enum eConstraintType {	NO_CONSTRAINT_CONTACT,	// this contact is not a constraint (it's a collision contact)
							FIXED_POINT,			// points on two objects held together (use two of these to make a hinge)
							SLIDING_POINT,			// points on two objects held within a fixed distance of each other (use two of these to make a loose hinge)
							FIXED_ROTATION,			// forces two objects to have constant relative orientation
							SLIDING_ROTATION,		// keeps two objects within a fixed relative orientation of each other
							PIVOTING_ROTATION,		// keeps two objects from pivoting away from an axis between them
							NUM_CONSTRAINT_TYPES
							};

    static const int MANIFOLD_CACHE_SIZE = 6;  // 7 or 8 is better for stable stacks

	phManifold();

	/*virtual*/ ~phManifold()
	{
		FastAssert(__SPU || m_NumCompositeManifolds == 0);
		FastAssert(__SPU || m_CachedPoints == 0);
	}

	static const char* GetClassName()
	{
		return "phManifold";
	}

/*
#if !__SPU
	// PURPOSE: Initialize the manifold with the given instances and colliders.
	// PARAMS:
	//	instanceA - the first instance in the manifold
	//	colliderA - the first collider in the manifold
	//	instanceB - the second instance in the manifold
	//	colliderB - the second collider in the manifold
	void Init (phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB);
#endif
*/

	void EnableCompositeManifolds();
	void DisableCompositeManifolds();

	__forceinline bool CompositeManifoldsEnabled() const
	{
		FastAssert(!m_CompositePointers || m_CachedPoints == 0);
		FastAssert(m_CompositePointers || m_NumCompositeManifolds == 0);
		//return m_CompositeManifold;
		return (m_CompositePointers != NULL);
	}

#if CHECK_FOR_DUPLICATE_MANIFOLDS
	__forceinline const phCompositePointers * GetCompositePointers() const
	{
		return m_CompositePointers;
	}
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS

	__forceinline int GetNumCompositeManifolds() const
	{
		return m_NumCompositeManifolds;
	}

	__forceinline phManifold* GetCompositeManifold(int manifoldIndex) const
	{
		FastAssert(CompositeManifoldsEnabled());
#if __SPU
		return m_CompositePointersLs->GetManifold(manifoldIndex); 
#else
		return m_CompositePointers->GetManifold(manifoldIndex); 
#endif
	}

#if __PS3
	__forceinline sysDmaPlan* GetSecondManifoldDmaPlan() const
	{
		FastAssert(CompositeManifoldsEnabled());
#if __SPU
		return m_CompositePointersLs->GetSecondManifoldDmaPlanRef(); 
#else
		return m_CompositePointers->GetSecondManifoldDmaPlanRef(); 
#endif
	}
#endif

	__forceinline u8 GetCompositePairComponentA(int manifoldIndex) const
	{
		FastAssert(CompositeManifoldsEnabled());
#if __SPU
		return m_CompositePointersLs->GetPairComponentA(manifoldIndex); 
#else
		return m_CompositePointers->GetPairComponentA(manifoldIndex); 
#endif
	}

	__forceinline u8 GetCompositePairComponentB(int manifoldIndex) const
	{
#if __SPU
		return m_CompositePointersLs->GetPairComponentB(manifoldIndex); 
#else
		return m_CompositePointers->GetPairComponentB(manifoldIndex); 
#endif
	}

	__forceinline u8 GetCompositePairComponent(u32 objectIndex, int manifoldIndex) const
	{
		FastAssert(CompositeManifoldsEnabled());
#if __SPU
		return m_CompositePointersLs->GetPairComponent(objectIndex, manifoldIndex); 
#else
		return m_CompositePointers->GetPairComponent(objectIndex, manifoldIndex); 
#endif
	}

	__forceinline void SetNumCompositeManifolds(int numCompositeManifolds)
	{
		Assert(numCompositeManifolds < phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		m_NumCompositeManifolds = static_cast<u8>(numCompositeManifolds);
	}

	__forceinline void SetCompositePairComponentA(int manifoldIndex, u8 component)
	{
		FastAssert(CompositeManifoldsEnabled());
#if __SPU
		return m_CompositePointersLs->SetPairComponentA(manifoldIndex, component); 
#else
		return m_CompositePointers->SetPairComponentA(manifoldIndex, component); 
#endif
	}

	__forceinline void SetCompositePairComponentB(int manifoldIndex, u8 component)
	{
#if __SPU
		return m_CompositePointersLs->SetPairComponentB(manifoldIndex, component); 
#else
		return m_CompositePointers->SetPairComponentB(manifoldIndex, component); 
#endif
	}

	__forceinline void SetManifoldAndPairComponents(int manifoldIndex, phManifold *manifold, u8 componentA, u8 componentB)
	{
#if __SPU
		m_CompositePointersLs->SetManifoldAndPairComponents(manifoldIndex, manifold, componentA, componentB);
#else
		m_CompositePointers->SetManifoldAndPairComponents(manifoldIndex, manifold, componentA, componentB);
#endif
	}

	__forceinline void CopyCompositeArrays(int numManifolds, u8* pairs, phManifold** manifolds)
	{
		FastAssert(CompositeManifoldsEnabled());

		if (m_CompositePointers)
		{
#if __SPU
			m_CompositePointersLs->CopyArrays(numManifolds, pairs, manifolds);
#else
			m_CompositePointers->CopyArrays(numManifolds, pairs, manifolds);
#endif
			m_NumCompositeManifolds = (u8)numManifolds;
		}
		else
		{
			m_NumCompositeManifolds = 0;
		}
	}

	__forceinline int CopyArraysOut(u8* pairs, phManifold** manifolds)
	{
		FastAssert(CompositeManifoldsEnabled());
		const int numManifolds = m_NumCompositeManifolds;

#if __SPU
		m_CompositePointersLs->CopyArraysOut(numManifolds, pairs, manifolds);
#else
		m_CompositePointers->CopyArraysOut(numManifolds, pairs, manifolds);
#endif
		return numManifolds;
	}

	__forceinline void CopyCompositeManifoldArray(int numManifolds, phManifold** manifolds)
	{
		FastAssert(CompositeManifoldsEnabled());

		if (m_CompositePointers)
		{
#if __SPU
			m_CompositePointersLs->CopyManifoldsArray(numManifolds, manifolds);
#else
			m_CompositePointers->CopyManifoldsArray(numManifolds, manifolds);
#endif
			m_NumCompositeManifolds = (u8)numManifolds;
		}
		else
		{
			m_NumCompositeManifolds = 0;
		}
	}

	__forceinline phManifold *FindManifoldByComponentIndices(int componentA, int componentB) const
	{
		FastAssert(CompositeManifoldsEnabled());
		FastAssert(m_CompositePointers);
#if __SPU
		return m_CompositePointersLs->FindManifoldByComponentIndices(componentA, componentB, m_NumCompositeManifolds);
#else
		return m_CompositePointers->FindManifoldByComponentIndices(componentA, componentB, m_NumCompositeManifolds);
#endif
	}

#if __SPU
	__forceinline void SetCompositePointersLs(phCompositePointers* compositePointers)
	{
		m_CompositePointersLs = compositePointers;
	}

	void GetCompositePointersFromMm(u32 tag);
	void PutCompositePointersToMm(u32 tag);

	void GatherContactPointsFromMm(u32 tag);
	void ScatterContactPointsToMm(u32 tag);

	void GetConstraintMatrixFromMm(u32 tag);
	void PutConstraintMatrixToMm(u32 tag);
#endif // __SPU

	__forceinline int GetNumContacts() const
	{
		return m_CachedPoints;
	}

	__forceinline bool ShouldRelease() const
	{
		return (m_CachedPoints <= 0);
	}

	__forceinline const phContact& GetContactPoint(int index) const
	{
		FastAssert(!CompositeManifoldsEnabled());
		TrapGE(index, m_CachedPoints);
		TrapLT(index, 0);

#if __SPU
		return *m_ContactsLs[index];
#else // __SPU
		return *m_Contacts[index];
#endif // __SPU
	}

	bool ObjectsInContact() const;

	bool HasDeepContacts(float depthThreshold, bool ttyOutput) const;

	__forceinline phContact& GetContactPoint(int index)
	{
		FastAssert(!CompositeManifoldsEnabled());
		TrapGE(index, m_CachedPoints);
		TrapLT(index, 0);

#if __SPU
		return *m_ContactsLs[index];
#else // __SPU
		return *m_Contacts[index];
#endif // __SPU
	}

#if __SPU
	__forceinline phContact& GetContactPointMm(int index)
	{
		return *m_Contacts[index];
	}

	__forceinline void SetContactsLs(phContact** contacts)
	{
		m_ContactsLs = contacts;
	}

	__forceinline void SetLsPointersFromMmPointers()
	{
		m_ContactsLs = m_Contacts;
		m_CompositePointersLs = m_CompositePointers;
		m_ConstraintMatrixLs = m_ConstraintMatrix;
	}

	__forceinline void SetConstraintMatrixLs(Mat33V* constraintMatrix)
	{
		m_ConstraintMatrixLs = constraintMatrix;
	}
#endif // __SPU

	__forceinline void SetConstraintMatrix(Mat33V* constraintMatrix)
	{
		m_ConstraintMatrix = constraintMatrix;
	}

	__forceinline Mat33V& GetConstraintMatrix()
	{
		FastAssert(m_ConstraintMatrix);

#if __SPU
		return *m_ConstraintMatrixLs;
#else // __SPU
		return *m_ConstraintMatrix;
#endif // __SPU

	}

	__forceinline void PrefetchContactPoint( int index ) const
	{
		FastAssert(!CompositeManifoldsEnabled());
		PrefetchObject(&GetContactPoint(index));
	}

	__forceinline void PrefetchAllContactPoints() const
	{
		int cachedPoints = m_CachedPoints;
		for (int index = 0; index < cachedPoints; ++index)
		{
			PrefetchObject(&GetContactPoint(index));
		}
	}

    __forceinline void ClearNewestContactPoint()
    {
		FastAssert(!CompositeManifoldsEnabled());
        m_NewestPoint = -1;
    }

    __forceinline int GetNewestContactPoint()
    {
		FastAssert(!CompositeManifoldsEnabled());
        return m_NewestPoint;
    }

	__forceinline u32 GetObjectIndex(phInst* inst) const
	{
		ptrdiff_t object1 = GenerateMaskEq(ptrdiff_t(inst), ptrdiff_t(m_Instance[1]));
		return u32(object1) & 1;
	}

	// PURPOSE: Get the level index of an object in the collision manifold.
	// RETURN:	the level index of the object
	u32 GetLevelIndexA () const;
	u32 GetLevelIndexB () const;
	u32 GetLevelIndex (u32 objectIndex) const;

	// PURPOSE: Get the generation ID of an object in the collision manifold.
	// RETURN:	the generation ID of the object
	u32 GetGenerationIdA () const;
	u32 GetGenerationIdB () const;
	u32 GetGenerationId (u32 objectIndex) const;

	// PURPOSE: Get the handle of an object in the collision manifold.
	// RETURN:	the handle of the object
	phHandle GetHandleA () const;
	phHandle GetHandleB () const;
	phHandle GetHandle (u32 objectIndex) const;

	// PURPOSE: Get the component number of an object in the collision manifold.
	// RETURN:	the component number of the object
	int GetComponentA () const;
	int GetComponentB () const;
	int GetComponent (u32 objectIndex) const;

	// PURPOSE: Get the instance pointer of an object in the collision manifold.
	// RETURN: the instance pointer of the object (can be NULL)
	phInst* GetInstanceA () const;
	phInst* GetInstanceB () const;
	phInst* GetInstance (u32 objectIndex) const;

	// PURPOSE: Get the collider pointer of an object in the collision manifold.
	// RETURN: the collider pointer of the object (can be NULL)
	phCollider* GetColliderA () const;
	phCollider* GetColliderB () const;
	phCollider* GetCollider (u32 objectIndex) const;

	// PURPOSE: Get the bound pointer of an object in the collision manifold.
	// RETURN: the bound pointer of the object (can be NULL)
	const phBound* GetBoundA () const;
	const phBound* GetBoundB () const;
	const phBound* GetBound (u32 objectIndex) const;

	// PURPOSE: Get the level index of object A in this manifold, if it is not fixed.
	// RETURN:	the level index of object A, or INVALID_INDEX if A is fixed
	int GetActiveOrInactiveLevelIndexA () const;

	// PURPOSE: Get the level index of object B in this manifold, if it is not fixed.
	// RETURN:	the level index of object B, or INVALID_INDEX if B is fixed
	int GetActiveOrInactiveLevelIndexB () const;

	/// todo: get this margin from the current physics / collision environment
	static float GetManifoldMargin();
	static ScalarV_Out GetManifoldMarginV();
	static ScalarV_Out GetManifoldMarginSquaredV();
	static bool ShouldAddContactPoint(ScalarV_In v_serpartion);

	// PURPOSE: Get the contact point in this manifold that is closest to the given contact point, if any are closer than the margin.
	// PARAMS:
	//	newPoint - the contact point to test for proximity with the points in this manifold
	// RETURN:	the index number of the contact point in this manifold that is closest to the given point within the manifold margin
	int GetClosestPointWithinMargin (const phContact& newPoint) const;

	bool IsSelfCollision()
	{
		return m_LevelIndex[0] == m_LevelIndex[1] &&
			   m_LevelIndex[0] != 0xFFFF; // phInst::INVALID_INDEX
	}

	bool IsArticulatedFixedCollision() const
	{
		return	(m_Collider[0] && m_Collider[0]->IsArticulated() && !m_Collider[1]) || 
				(m_Collider[1] && m_Collider[1]->IsArticulated() && !m_Collider[0]);
	}

	void AddManifoldPoint(const phContact& newPoint);

	int FindContactPointIndex (phContact *contact) const;

	void RemoveContactPoint (int index);
	void ReplaceContactPoint(const phContact& newPoint,int insertIndex);

	// PURPOSE: Get the matrix to transform from instance A's coordinates to world space.
	// PARAMS:
	//	transformA - reference to write the transform for instance A
	void GetLocalToWorldTransformA (Matrix34& transformA) const;

	// PURPOSE: Get the matrix to transform from instance B's coordinates to world space.
	// PARAMS:
	//	transformB - reference to write the transform for instance B
	void GetLocalToWorldTransformB (Matrix34& transformB) const;

	// PURPOSE: Get the matrix to transforms from instance A and instance B's coordinates to world space.
	// PARAMS:
	//	transformA - reference to write the transform for instance A
	//	transformB - reference to write the transform for instance B
	void GetLocalToWorldTransforms (Matrix34& transformA, Matrix34& transformB) const;

	void IncrementContactLifetimes();

	void RefreshContactPoints (int minManifoldPointLifetime, Mat34V_In transformA, Mat34V_In lastA, Mat34V_In transformB, Mat34V_In lastB, ScalarV_In timeStep = ScalarV(V_ZERO));
	void RefreshContactPoints (int minManifoldPointLifetime, ScalarV_In timeStep = ScalarV(V_ZERO));

#if USE_PRECOMPUTE_SEPARATEBIAS
	bool HasOpposingContacts();
	void UpdateSeparateBias( ScalarV_In separateBiasMultiplier, ScalarV_In halfPenetration, ScalarV_In invTime );
#endif // USE_PRECOMPUTE_SEPARATEBIAS

    void    ProfileDraw();

	// PURPOSE: Release all contacts, reset all members to initialized state
	void Reset();


#if USE_FRAME_PERSISTENT_GJK_CACHE
	void ResetGJKCache();
#endif // USE_FRAME_PERSISTENT_GJK_CACHE

	// PURPOSE: Release all contacts and composite manifolds
	void RemoveAllContacts();

//#if __SPU
//	void SpuReset();
//#endif

	// if it's not static friction it will be in a state of sliding friction
	bool IsStaticFriction () const;

	bool GetInactiveCollidesAgainstInactiveA() const;
	bool GetInactiveCollidesAgainstInactiveB() const;

#if __DEBUGLOG
	void DebugReplay() const;
#endif

#if __ASSERT
	void AssertContactNormalVerbose(const phContact& Contact, const char* file, int line, bool noStack = false) const;
#if __PPU
	static void SpuAssertNormalEventHandler(const sys_event* Event);
#endif // PS3
#endif // ASSERT

	/// sort cached points so most isolated points come first
	int	SortCachedPoints(const phContact& pt);

	int GetConstraintType() const;
	void SetConstraintType(int constraintType);

	void SetIsConstraint();

	// PURPOSE: Tell if this manifold is a constraint.
	// RETURN:	true if this manifold is a constraint, false if it is a collision
	bool IsConstraint () const;

	// PURPOSE: Tell if this manifold is a fixed point constraint.
	// RETURN:	true if this manifold fixes points on two objects together, false if it does not
	bool IsFixedPointConstraint () const;

	// PURPOSE: Tell if this manifold is a fixed rotation constraint.
	// RETURN:	true if this manifold fixes the relative orientation between two objects, false if it does not
	bool IsFixedRotationConstraint () const;

	// PURPOSE: Tell if this manifold is a fixed constraint (if it does not allow separation)
	// RETURN:	true if this manifold is a fixed point or a fixed rotation constraint, false if it is not
	bool IsFixedConstraint () const;

	// PURPOSE: Tell if this manifold is a rotation constraint.
	// RETURN:	true if this manifold is a rotation constraint, false if it is a collision or a translation constraint
	bool IsRotationConstraint () const;

	void SetMassInvA(float massInv);
	const float& GetMassInvA() const;
	void SetMassInvB(float massInv);
	const float& GetMassInvB() const;

#if PH_MANIFOLD_TRACK_IS_IN_OPA
	void SetIsInOPA(bool isInOPA)
	{
		m_IsInOPA = isInOPA;
	}

	void AboutToBeReleased();
#else
	__forceinline void AboutToBeReleased() { }
#endif // PH_MANIFOLD_TRACK_IS_IN_OPA

	Mat33V& GetInertiaInvA();
	Mat33V& GetInertiaInvB();

	// Inverse mass scaling functions.
	// PURPOSE: Get the value used to scale the inverse mass of object A.
	const float& GetMassInvScaleA() const;
	// PURPOSE: Get the value used to scale the inverse mass of object B.
	const float& GetMassInvScaleB() const;
	const float& GetMassInvScale(u32 objectIndex) const;
	// PURPOSE: Set the mass inv scale for one of the instances in the manifold
	// NOTE: You should always explicitly set both mass inv scales since their effect is relative to each other.
	void SetMassInvScale(u32 objectIndex, float scale);
	void SetMassInvScaleA(float scale);
	void SetMassInvScaleB(float scale);

	bool GetUsePushes() const;
	BoolV_Out GetUsePushesV() const;

	void SetCollisionTime(utimer_t collisionTime);
	u32 GetCollisionTime() const;
	const u32& GetCollisionTimeRef() const;
#if PER_MANIFOLD_SOLVER_METRICS
	void SetUpdateContactsTime(utimer_t updateContactsTime);
	SPU_ONLY(void DmaPutContactsTime(utimer_t updateContactsTime, u32 tag);)
	u32 GetUpdateContactsTime() const;
	void SetVelocitySolveTime(utimer_t velocitySolveTime);
	u32 GetVelocitySolveTime() const;
	void SetPushSolveTime(utimer_t pushSolveTime);
	u32 GetPushSolveTime() const;
#endif // PER_MANIFOLD_SOLVER_METRICS

	void Exchange();
	void ExchangeThisManifoldOnly();

	void ResetWarmStart();

	void SetUserData(u32 userData);
	u32 GetUserData() const;
	u32& GetUserDataRef();

#if !__SPU
	void DumpDueToRunningOut();
#endif

#if __PS3
	class DmaPlan : public sysDmaPlan
	{
	public:
		static const int MAX_DMAS = MANIFOLD_CACHE_SIZE + 6;
		static const int MAX_FIXUPS = MANIFOLD_CACHE_SIZE + 5;

		DmaPlan()
			: m_ReadOnlyStorage(m_ReadOnlyBits, MAX_DMAS)
		{
			m_DmaList = m_DmaListStorage;
			m_Fixups = m_FixupStorage;
			m_MaxDmas = MAX_DMAS;
			m_MaxFixups = MAX_FIXUPS;
			m_ReadOnly = &m_ReadOnlyStorage;
#if __PPU
			memset(m_ReadOnlyBits,0,sizeof(m_ReadOnlyBits));
#else
			sysMemZeroBytes<sizeof(m_ReadOnlyBits)>(m_ReadOnlyBits);
#endif
		}

	private:
		CellDmaListElement m_DmaListStorage[MAX_DMAS];
		Fixup m_FixupStorage[MAX_FIXUPS];
		atUserBitSet m_ReadOnlyStorage;
		unsigned m_ReadOnlyBits[(MAX_DMAS+31)>>5];
	} ;

	void GenerateDmaPlan(DMA_PLAN_ARGS(phManifold));
	PPU_ONLY(void RegenerateDmaPlan();)
#endif

	static const float MANIFOLD_MARGIN;
	static const ScalarV MANIFOLD_MARGINV;
	static const ScalarV MANIFOLD_MARGINSQUAREDV;

#if POSITIVE_DEPTH_ONLY
	enum {
		POSITIVE_DEPTH_ONLY_DISABLED	= 0,
		POSITIVE_DEPTH_ONLY_ENABLED		= 1,
		POSITIVE_DEPTH_ONLY_VELOCITY	= 2
	};

	static int sm_PositiveDepthMode;
#endif // POSITIVE_DEPTH_ONLY

#if USE_FRAME_PERSISTENT_GJK_CACHE
	void SetGJKCacheDB(GJKCacheDB * gjkCacheDB)
	{
		m_gjkCacheDB = gjkCacheDB;
	}
	GJKCacheDB * GetGJKCacheDB() const
	{
		return m_gjkCacheDB;
	}
#endif

#if !__SPU
	void RefreshColliderPointers();
#endif // !__SPU
protected:
	void SetLevelIndexA (u32 levelIndex, u32 generationId);
	void SetLevelIndexB (u32 levelIndex, u32 generationId);
	void SetLevelIndex (u32 objectIndex, u32 levelIndex, u32 generationId);

	void SetHandleA (phHandle handle); 
	void SetHandleB (phHandle handle); 
	void SetHandle (u32 objectIndex, phHandle handle);

	void SetComponentA (int component);
	void SetComponentB (int component);
	void SetComponent (u32 objectIndex, int component);

	// PURPOSE: Set one of the manifold's instance pointers.
	// PARAMS:
	//	instance - the manifold's new instance pointer
	void SetInstanceA (phInst* instance);
	void SetInstanceB (phInst* instance);
	void SetInstance (u32 objectIndex, phInst* instance);

	void SetColliderA (phCollider* collider);
	void SetColliderB (phCollider* collider);
	void SetCollider (u32 objectIndex, phCollider* collider);

	void SetBoundA (const phBound* bound);
	void SetBoundB (const phBound* bound);
	void SetBound (u32 objectIndex, const phBound* bound);

	void TransferContactPoints (phManifold &fromManifold);

	void ResetInternal();

	Mat33V m_InertiaInv[2];

	phContact* m_Contacts[MANIFOLD_CACHE_SIZE];
	phCompositePointers* m_CompositePointers;
	Mat33V* m_ConstraintMatrix;

#if __PS3
	phContact** m_ContactsLs;
	phCompositePointers* m_CompositePointersLs;
	Mat33V* m_ConstraintMatrixLs;
#endif // __PS3

	phInst* m_Instance[2];
	phCollider* m_Collider[2];

    const phBound* m_Bound[2];

	float	m_MassInvScale[2];

	float	m_MassInv[2];
	u32 m_UserData;

	u32 m_CollisionTime;
#if PER_MANIFOLD_SOLVER_METRICS
	u32 m_UpdateContactsTime;
	u32 m_VelocitySolveTime;
	u32 m_PushSolveTime;
#endif // PER_MANIFOLD_SOLVER_METRICS

#if USE_FRAME_PERSISTENT_GJK_CACHE
	GJKCacheDB * m_gjkCacheDB;
#endif

	u16 m_LevelIndex[2];
	u16 m_GenerationId[2];
	u8 m_Component[2];

	// PURPOSE: This tells the contact manager whether this contact was created by a constraint, and if so then what type of constraint.
	// NOTES:
	//	NO_CONSTRAINT_CONTACT means the contact was made by a collision, not a constraint. It will be solved with friction and complementarity (allowed to separate)
	//	FIXED_POINT means the contact was created by a constraint that will hold the points on two objects together, and the lcp solver will handle it without
	//				allowing sliding (infinite friction), and without complementarity (no separation allowed).
	//	SLIDING_POINT means the contact was created by a constraint with length and friction, and the lcp solver will treat it the same as a collision contact.
	//	FIXED_ROTATION means the contact was created by a rotational constraint, and the lcp solver will solve it with the inverse inertia matrix instead of
	//					the inverse mass matrix, and with relative angular velocity instead of relative linear velocity.
	//	Holding an object exactly in place requires two constraints - a FIXED_POINT constraint and a FIXED_ROTATION constraint.
	u8 m_ConstraintType;

	// If this manifold controls a pair of bodies of which at least one is composite, then the manifolds related to pairwise part collisions
	// will be held in "composite manifolds" of a special type of manifold, the "root" manifold. The number of composite manifolds this "root"
	// manifold controls is m_NumCompositeManifolds, which are contained in the m_CompositeCollidingManifolds array
	// NOTE: The first element of the m_CompositeCollidingManifolds is always the root manifold itself, so if there are any sub manifolds, this number will be > 1
    u8 m_NumCompositeManifolds;

	u8	m_CachedPoints;

	s8 m_NewestPoint;

	bool m_UsePushes;

	bool m_StaticFriction : 1;
	bool m_InactiveCollidesAgainstInactiveA : 1;
	bool m_InactiveCollidesAgainstInactiveB : 1;
	//bool m_CompositeManifold : 1;
	bool m_IsConstraint : 1;
#if PH_MANIFOLD_TRACK_IS_IN_OPA
	bool m_IsInOPA : 1;
#endif // PH_MANIFOLD_TRACK_IS_IN_OPA

#if __DEBUGLOG_PS3
public:
	mutable u16 m_DebugLogSize;
	static const u16 sm_DebugLogBufferSize = 1024 - sizeof(u16);
	char m_DebugLogBuffer[sm_DebugLogBufferSize];
#endif

#if __DEBUGLOG_SPU
public:
	void StartDebugLogSPU() { diagDebugLogSPUInit(m_DebugLogBuffer, &m_DebugLogSize, sm_DebugLogBufferSize); }
	void StopDebugLogSPU()  { diagDebugLogSPUShutdown(); }
#endif

	friend class btConvexTriangleCallback;
	friend class phArticulatedCollider;
	template <int Size> friend class phCollisionCompositeManifoldMgr;
	friend class phConstraintMgr;
	friend class phContactMgr;
	friend class phShapeObject;
	friend class phSimulator;
	friend class CollisionInFlight;
	friend struct SAP_Callback;
	friend class phMidphase;
	friend class phSpuManifoldWrapper;
	friend void UpdateContactsTask(struct sysTaskParameters& params);
	friend u32 ProcessPair(phTaskCollisionPair *spuPair, const PairListWorkUnitInput &wui, phSpuManifoldWrapper &rootManifoldWrapper, bool selfCollisionsEnabled, int rootManifoldDmaTag, bool sweepFromSafe);
	friend void SetManifoldComponentAFromSPU(phManifold* manifold, int component);
	friend void SetManifoldComponentBFromSPU(phManifold* manifold, int component);
	friend class phConstraintBase;

#if __SPU
	friend class phForceSolver;
#endif
};


inline u32 phManifold::GetLevelIndexA () const
{
	return m_LevelIndex[0];
}

inline u32 phManifold::GetLevelIndexB () const
{
	return m_LevelIndex[1];
}

__forceinline u32 phManifold::GetLevelIndex (u32 objectIndex) const
{
	TrapGE(objectIndex, (u32)2);
	return m_LevelIndex[objectIndex];
}

inline u32 phManifold::GetGenerationIdA () const
{
	return m_GenerationId[0];
}

inline u32 phManifold::GetGenerationIdB () const
{
	return m_GenerationId[1];
}

__forceinline u32 phManifold::GetGenerationId (u32 objectIndex) const
{
	TrapGE(objectIndex, (u32)2);
	return m_GenerationId[objectIndex];
}

__forceinline phHandle phManifold::GetHandleA () const
{
	return phHandle(m_LevelIndex[0],m_GenerationId[0]);
}
__forceinline phHandle phManifold::GetHandleB () const
{
	return phHandle(m_LevelIndex[1],m_GenerationId[1]);
}
__forceinline phHandle phManifold::GetHandle (u32 objectIndex) const
{
	TrapGE(objectIndex, (u32)2);
	return phHandle(m_LevelIndex[objectIndex],m_GenerationId[objectIndex]);
}

inline int phManifold::GetComponentA () const
{
	return m_Component[0];
}

inline int phManifold::GetComponentB () const
{
	return m_Component[1];
}

__forceinline int phManifold::GetComponent (u32 objectIndex) const
{
	TrapGE(objectIndex, (u32)2);
	return m_Component[objectIndex];
}

inline phInst* phManifold::GetInstanceA () const
{
	return m_Instance[0];
}

inline phInst* phManifold::GetInstanceB () const
{
	return m_Instance[1];
}

__forceinline phInst* phManifold::GetInstance (u32 objectIndex) const
{
	TrapGE(objectIndex, (u32)2);
	return m_Instance[objectIndex];
}

inline phCollider* phManifold::GetColliderA () const
{
	return m_Collider[0];
}

inline phCollider* phManifold::GetColliderB () const
{
	return m_Collider[1];
}

__forceinline phCollider* phManifold::GetCollider (u32 objectIndex) const
{
	TrapGE(objectIndex, (u32)2);
	return m_Collider[objectIndex];
}

inline const phBound* phManifold::GetBoundA () const
{
	return m_Bound[0];
}

inline const phBound* phManifold::GetBoundB () const
{
	return m_Bound[1];
}

__forceinline const phBound* phManifold::GetBound (u32 objectIndex) const
{
	TrapGE(objectIndex, (u32)2);
	return m_Bound[objectIndex];
}

inline void phManifold::SetLevelIndexA (u32 levelIndex, u32 generationId)
{
	TrapGE(levelIndex, u32(0x10000));
	TrapGE(generationId, u32(0x10000));
	m_LevelIndex[0] = (u16)levelIndex;
	m_GenerationId[0] = (u16)generationId;
}

inline void phManifold::SetLevelIndexB (u32 levelIndex, u32 generationId)
{
	TrapGE(levelIndex, u32(0x10000));
	TrapGE(generationId, u32(0x10000));
	m_LevelIndex[1] = (u16)levelIndex;
	m_GenerationId[1] = (u16)generationId;
}

__forceinline void phManifold::SetLevelIndex (u32 objectIndex, u32 levelIndex, u32 generationId)
{
	TrapGE(objectIndex, (u32)2);
	TrapGE(levelIndex, u32(0x10000));
	TrapGE(generationId, u32(0x10000));
	m_LevelIndex[objectIndex] = (u16)levelIndex;
	m_GenerationId[objectIndex] = (u16)generationId;
}

__forceinline void phManifold::SetHandleA (phHandle handle)
{
	m_LevelIndex[0] = handle.GetLevelIndex();
	m_GenerationId[0] = handle.GetGenerationId();
}
__forceinline void phManifold::SetHandleB (phHandle handle)
{
	m_LevelIndex[1] = handle.GetLevelIndex();
	m_GenerationId[1] = handle.GetGenerationId();
}
__forceinline void phManifold::SetHandle (u32 objectIndex, phHandle handle)
{
	TrapGE(objectIndex, (u32)2);
	m_LevelIndex[objectIndex] = handle.GetLevelIndex();
	m_GenerationId[objectIndex] = handle.GetGenerationId();
}

inline void phManifold::SetComponentA (int component)
{
	FastAssert(component >= 0 && component < 256);
	m_Component[0] = (u8)component;
}

inline void phManifold::SetComponentB (int component)
{
	FastAssert(component >= 0 && component < 256);
	m_Component[1] = (u8)component;
}

__forceinline void phManifold::SetComponent (u32 objectIndex, int component)
{
	TrapGE(objectIndex, (u32)2);
	FastAssert(component >= 0 && component < 256);
	m_Component[objectIndex] = (u8)component;
}

inline void phManifold::SetInstanceA (phInst* instance)
{
	m_Instance[0] = instance;
}

inline void phManifold::SetInstanceB (phInst* instance)
{
	m_Instance[1] = instance;
}

__forceinline void phManifold::SetInstance (u32 objectIndex, phInst* instance)
{
	TrapGE(objectIndex, (u32)2);
	m_Instance[objectIndex] = instance;
}

inline void phManifold::SetColliderA (phCollider* collider)
{
	m_Collider[0] = collider;
}

inline void phManifold::SetColliderB (phCollider* collider)
{
	m_Collider[1] = collider;
}

__forceinline void phManifold::SetCollider (u32 objectIndex, phCollider* collider)
{
	TrapGE(objectIndex, (u32)2);
	m_Collider[objectIndex] = collider;
}

inline void phManifold::SetBoundA (const phBound* bound)
{
	m_Bound[0] = bound;
}

inline void phManifold::SetBoundB (const phBound* bound)
{
	m_Bound[1] = bound;
}

__forceinline void phManifold::SetBound (u32 objectIndex, const phBound* bound)
{
	TrapGE(objectIndex, (u32)2);
	m_Bound[objectIndex] = bound;
}

inline float phManifold::GetManifoldMargin()
{
	return MANIFOLD_MARGIN;
}

inline ScalarV_Out phManifold::GetManifoldMarginV()
{
	return MANIFOLD_MARGINV;
}

inline ScalarV_Out phManifold::GetManifoldMarginSquaredV()
{
	return MANIFOLD_MARGINSQUAREDV;
}

inline bool phManifold::ShouldAddContactPoint(ScalarV_In v_separation)
{
	return IsGreaterThanAll(GetManifoldMarginV(), v_separation) != 0;
}

inline bool phManifold::IsStaticFriction () const
{
	return m_StaticFriction;
}

inline bool phManifold::GetInactiveCollidesAgainstInactiveA() const
{
	return m_InactiveCollidesAgainstInactiveA;
}

inline bool phManifold::GetInactiveCollidesAgainstInactiveB() const
{
	return m_InactiveCollidesAgainstInactiveB;
}

inline int phManifold::GetConstraintType() const
{
	return m_ConstraintType;
}

inline void phManifold::SetConstraintType(int constraintType)
{
	m_ConstraintType = (u8)constraintType;
}

inline void phManifold::SetIsConstraint()
{
	m_IsConstraint = true;
}

inline bool phManifold::IsConstraint () const
{
	return m_IsConstraint;
}

inline bool phManifold::IsFixedPointConstraint () const
{
	return m_ConstraintType==FIXED_POINT;
}

inline bool phManifold::IsFixedRotationConstraint () const
{
	return m_ConstraintType==FIXED_ROTATION;
}

inline bool phManifold::IsFixedConstraint () const
{
	return (IsFixedPointConstraint() || IsFixedRotationConstraint());
}

inline bool phManifold::IsRotationConstraint () const
{
	return m_ConstraintType==FIXED_ROTATION || m_ConstraintType==SLIDING_ROTATION;
}

inline void phManifold::SetMassInvA(float massInv)
{
	m_MassInv[0] = massInv;
}

inline const float& phManifold::GetMassInvA() const
{
	return m_MassInv[0];
}

inline void phManifold::SetMassInvB(float massInv)
{
	m_MassInv[1] = massInv;
}

inline const float& phManifold::GetMassInvB() const
{
	return m_MassInv[1];
}

inline Mat33V& phManifold::GetInertiaInvA()
{
	return m_InertiaInv[0];
}

inline Mat33V& phManifold::GetInertiaInvB()
{
	return m_InertiaInv[1];
}

inline const float& phManifold::GetMassInvScaleA() const
{
	return m_MassInvScale[0];
}

inline const float& phManifold::GetMassInvScaleB() const
{
	return m_MassInvScale[1];
}

inline const float& phManifold::GetMassInvScale(u32 objectIndex) const
{
	return m_MassInvScale[objectIndex];
}

inline void phManifold::SetMassInvScale(u32 objectIndex, float scale)
{
	m_MassInvScale[objectIndex] = scale;
}

inline void phManifold::SetMassInvScaleA(float scale)
{
	SetMassInvScale(0,scale);
}

inline void phManifold::SetMassInvScaleB(float scale)
{
	SetMassInvScale(1,scale);
}

inline bool phManifold::GetUsePushes() const
{
	return m_UsePushes;
}

inline BoolV_Out phManifold::GetUsePushesV() const
{
	return BoolV(m_UsePushes);
}

__forceinline void  phManifold::SetCollisionTime(utimer_t collisionTime)
{
	if (collisionTime < 0xfffffffe)
	{
		m_CollisionTime = (u32)collisionTime;
	}
	else
	{
		m_CollisionTime = 0xfffffffe;
	}
}

__forceinline u32 phManifold::GetCollisionTime() const
{
	return m_CollisionTime;
}

__forceinline const u32& phManifold::GetCollisionTimeRef() const
{
	return m_CollisionTime;
}

#if PER_MANIFOLD_SOLVER_METRICS
__forceinline void  phManifold::SetUpdateContactsTime(utimer_t updateContactsTime)
{
	if (updateContactsTime < 0xffffffff)
	{
		m_UpdateContactsTime = (u32)updateContactsTime;
	}
	else
	{
		m_UpdateContactsTime = 0xffffffff;
	}
}

#if __SPU
#if 1
void phManifold::DmaPutContactsTime(utimer_t , u32 )
{
	// Disable this temporarily since it seems to be causing semi-rare crashes in smoke tests
}
#else
// Clever way from Luke Hutchinson, to DMA a u32 back to main memory without waiting
void dmaPut32(qword* scratch, u32 ea, u32 data, u32 tag)
{
	const qword shufAAAA = si_ila(0x10203);
	*scratch = si_shufb(si_from_uint(data), si_from_uint(data), shufAAAA);
	sysDmaSmallPut((char*)scratch+(ea&15), ea, 4, tag);
}

void phManifold::DmaPutContactsTime(utimer_t updateContactsTime, u32 tag)
{
	static qword scratch;
	dmaPut32(&scratch, (uint64_t)&m_UpdateContactsTime, updateContactsTime, tag);
}
#endif
#endif

__forceinline u32 phManifold::GetUpdateContactsTime() const
{
	return m_UpdateContactsTime;
}

__forceinline void  phManifold::SetVelocitySolveTime(utimer_t velocitySolveTime)
{
	if (velocitySolveTime < 0xffffffff)
	{
		m_VelocitySolveTime = (u32)velocitySolveTime;
	}
	else
	{
		m_VelocitySolveTime = 0xffffffff;
	}
}

__forceinline u32 phManifold::GetVelocitySolveTime() const
{
	return m_VelocitySolveTime;
}

__forceinline void  phManifold::SetPushSolveTime(utimer_t pushSolveTime)
{
	if (pushSolveTime < 0xffffffff)
	{
		m_PushSolveTime = (u32)pushSolveTime;
	}
	else
	{
		m_PushSolveTime = 0xffffffff;
	}
}

__forceinline u32 phManifold::GetPushSolveTime() const
{
	return m_PushSolveTime;
}
#endif // PER_MANIFOLD_SOLVER_METRICS

__forceinline void phManifold::SetUserData(u32 userData)
{
	m_UserData = userData;
}

__forceinline u32 phManifold::GetUserData() const
{
	return m_UserData;
}

__forceinline u32& phManifold::GetUserDataRef()
{
	return m_UserData;
}

#define CLOTH_MANIFOLD 0

#if CLOTH_MANIFOLD
#define CLOTH_MANIFOLD_ONLY(x) x
#else // CLOTH_MANIFOLD
#define CLOTH_MANIFOLD_ONLY(x)
#endif // CLOTH_MANIFOLD


#if CLOTH_MANIFOLD

#define MAX_CLOTH_WRAPPED_OBJECTS	8

// PURPOSE: A version of phManifold used for cloth and rope wrapping around objects. The cloth manifolds are actually regular manifolds from the manager's pool,
//			and they are cast as phClothManifolds to use the methods here for storing information on the wrapped objects.
class phClothManifold : public phManifold
{
public:
	phClothManifold ();

	int GetNumWrappedObjects () const;
	void SetNumWrappedObjects (int numWrappedObjects);

	const phInst* GetWrappedInstance (int wrappedObjectIndex) const;
	void SetWrappedInstance (int wrappedObjectIndex, const phInst* wrappedInstance);

	phCollider* GetWrappedCollider (int wrappedObjectIndex) const;
	void SetWrappedCollider (int wrappedObjectIndex, const phCollider* wrappedCollider);

	Vec3V_Out GetWrappedWorldPosition (int wrappedObjectIndex) const;
	void SetWrappedWorldPosition (int wrappedObjectIndex, Vec3V_In wrappedWorldPosition);

	ScalarV_Out GetWrappedMassInv (int wrappedObjectIndex) const;
	void SetWrappedMassInv (int wrappedObjectIndex, ScalarV_In massInv);

	Mat33V_Out GetWrappedInertiaInv (int wrappedObjectIndex) const;
	void SetWrappedInertiaInv (int wrappedObjectIndex, Mat33V_In wrappedInertiaInv);

	Vec3V_Out GetWrappedWorldNormal (int wrappedObjectIndex) const;
	void SetWrappedWorldNormal (int wrappedObjectIndex, Vec3V_In wrappedWorldNormal);

	ScalarV_Out GetSineHalfAngle (int wrappedObjectIndex) const;
	void SetSineHalfAngle (int wrappedObjectIndex, ScalarV_In sineHalfAngle);

	void AddWrappedObject (int vertexIndex, Vec3V_In worldPosition, Vec3V_In worldNormal, ScalarV_In sineHalfAngle, const phInst* wrappedInstance, const phCollider* wrappedCollider);

protected:
	// This is how it's stored in phManifold:
	//unsigned char m_PointCacheBytes[ALIGNED_CONTACT_SIZE * MANIFOLD_CACHE_SIZE] ;

	// This is how it's used in phClothManifold, starting after one contact with size ALIGNED_CONTACT_SIZE:
	//phInst* m_WrappedInstances[MAX_CLOTH_WRAPPED_OBJECTS];
	//phCollider* m_WrappedColliders[MAX_CLOTH_WRAPPED_OBJECTS];
	//Vec3V m_WrappedWorldPositions[MAX_CLOTH_WRAPPED_OBJECTS];
	//float m_WrappedMassInv[MAX_CLOTH_WRAPPED_OBJECTS];
	//Mat33V m_WrappedInertiaInv[MAX_CLOTH_WRAPPED_OBJECTS];
	//Vec3V m_WrappedWorldNormals[MAX_CLOTH_WRAPPED_OBJECTS];
	//float m_SineHalfAngle[MAX_CLOTH_WRAPPED_OBJECTS];
	//int m_NumWrappedObjects;
};

#define FIRST_WRAPPED_INSTANCE	ALIGNED_CONTACT_SIZE
#define FIRST_WRAPPED_COLLIDER	FIRST_WRAPPED_INSTANCE + MAX_CLOTH_WRAPPED_OBJECTS*sizeof(phInst*)
#define FIRST_WRAPPED_POSITION	FIRST_WRAPPED_COLLIDER + MAX_CLOTH_WRAPPED_OBJECTS*sizeof(phCollider*)
#define FIRST_WRAPPED_MASS_INV	FIRST_WRAPPED_POSITION + MAX_CLOTH_WRAPPED_OBJECTS*sizeof(Vec3V)
#define FIRST_WRAPPED_INERTIAI	FIRST_WRAPPED_MASS_INV + MAX_CLOTH_WRAPPED_OBJECTS*sizeof(float)
#define FIRST_WRAPPED_W_NORMAL	FIRST_WRAPPED_INERTIAI + MAX_CLOTH_WRAPPED_OBJECTS*sizeof(Mat33V)
#define FIRST_WRAPPED_SINEHALF	FIRST_WRAPPED_W_NORMAL + MAX_CLOTH_WRAPPED_OBJECTS*sizeof(Vec3V)
#define NUM_WRAPPED_OBJECTSPOS	FIRST_WRAPPED_SINEHALF + MAX_CLOTH_WRAPPED_OBJECTS*sizeof(float)


inline int phClothManifold::GetNumWrappedObjects () const
{
	return *reinterpret_cast<const int*>(m_PointCacheBytes+NUM_WRAPPED_OBJECTSPOS);
}

inline void phClothManifold::SetNumWrappedObjects (int numWrappedObjects)
{
	sysMemCpy(m_PointCacheBytes+NUM_WRAPPED_OBJECTSPOS,&numWrappedObjects,sizeof(int));
	FastAssert(GetNumWrappedObjects()==numWrappedObjects);
}

inline const phInst* phClothManifold::GetWrappedInstance (int wrappedObjectIndex) const
{
	FastAssert(wrappedObjectIndex>=0 && wrappedObjectIndex<GetNumWrappedObjects());
	const phInst* instance;
	sysMemCpy(&instance,m_PointCacheBytes+FIRST_WRAPPED_INSTANCE+(wrappedObjectIndex*sizeof(phInst*)),sizeof(phInst*));
	return instance;
}

inline void phClothManifold::SetWrappedInstance (int wrappedObjectIndex, const phInst* wrappedInstance)
{
	sysMemCpy(m_PointCacheBytes+FIRST_WRAPPED_INSTANCE+(wrappedObjectIndex*sizeof(phInst*)),&wrappedInstance,sizeof(phInst*));
	FastAssert(GetWrappedInstance(wrappedObjectIndex)==wrappedInstance);
}

inline phCollider* phClothManifold::GetWrappedCollider (int wrappedObjectIndex) const
{
	FastAssert(wrappedObjectIndex>=0 && wrappedObjectIndex<GetNumWrappedObjects());
	phCollider* collider;
	sysMemCpy(&collider,m_PointCacheBytes+FIRST_WRAPPED_COLLIDER+(wrappedObjectIndex*sizeof(phCollider*)),sizeof(phCollider*));
	return collider;
}

inline void phClothManifold::SetWrappedCollider (int wrappedObjectIndex, const phCollider* wrappedCollider)
{
	sysMemCpy(m_PointCacheBytes+FIRST_WRAPPED_COLLIDER+(wrappedObjectIndex*sizeof(phCollider*)),&wrappedCollider,sizeof(phCollider*));
	FastAssert(GetWrappedCollider(wrappedObjectIndex)==wrappedCollider);
}

inline Vec3V_Out phClothManifold::GetWrappedWorldPosition (int wrappedObjectIndex) const
{
	FastAssert(wrappedObjectIndex>=0 && wrappedObjectIndex<GetNumWrappedObjects());
	return *reinterpret_cast<const Vec3V*>(m_PointCacheBytes+FIRST_WRAPPED_POSITION+(wrappedObjectIndex*sizeof(Vec3V)));
}

inline void phClothManifold::SetWrappedWorldPosition (int wrappedObjectIndex, Vec3V_In wrappedWorldPosition)
{
	sysMemCpy(m_PointCacheBytes+FIRST_WRAPPED_POSITION+(wrappedObjectIndex*sizeof(Vec3V)),&wrappedWorldPosition,sizeof(Vec3V));
	FastAssert(IsEqualAll(GetWrappedWorldPosition(wrappedObjectIndex),wrappedWorldPosition));
}

inline ScalarV_Out phClothManifold::GetWrappedMassInv (int wrappedObjectIndex) const
{
	FastAssert(wrappedObjectIndex>=0 && wrappedObjectIndex<GetNumWrappedObjects());
	return ScalarVFromF32(*reinterpret_cast<const float*>(m_PointCacheBytes+FIRST_WRAPPED_MASS_INV+(wrappedObjectIndex*sizeof(float))));
}

inline void phClothManifold::SetWrappedMassInv (int wrappedObjectIndex, ScalarV_In massInv)
{
	float massInvF = massInv.Getf();
	sysMemCpy(m_PointCacheBytes+FIRST_WRAPPED_MASS_INV+(wrappedObjectIndex*sizeof(float)),&massInvF,sizeof(float));
	FastAssert(IsEqualAll(GetWrappedMassInv(wrappedObjectIndex),massInv));
}

inline Mat33V_Out phClothManifold::GetWrappedInertiaInv (int wrappedObjectIndex) const
{
	FastAssert(wrappedObjectIndex>=0 && wrappedObjectIndex<GetNumWrappedObjects());
	return *reinterpret_cast<const Mat33V*>(m_PointCacheBytes+FIRST_WRAPPED_INERTIAI+(wrappedObjectIndex*sizeof(Mat33V)));
}

inline void phClothManifold::SetWrappedInertiaInv (int wrappedObjectIndex, Mat33V_In wrappedInertiaInv)
{
	sysMemCpy(m_PointCacheBytes+FIRST_WRAPPED_INERTIAI+(wrappedObjectIndex*sizeof(Mat33V)),&wrappedInertiaInv,sizeof(Mat33V));
	FastAssert(IsEqualAll(GetWrappedInertiaInv(wrappedObjectIndex),wrappedInertiaInv));
}

inline Vec3V_Out phClothManifold::GetWrappedWorldNormal (int wrappedObjectIndex) const
{
	FastAssert(wrappedObjectIndex>=0 && wrappedObjectIndex<GetNumWrappedObjects());
	return *reinterpret_cast<const Vec3V*>(m_PointCacheBytes+FIRST_WRAPPED_W_NORMAL+(wrappedObjectIndex*sizeof(Vec3V)));
}

inline void phClothManifold::SetWrappedWorldNormal (int wrappedObjectIndex, Vec3V_In wrappedWorldNormal)
{
	sysMemCpy(m_PointCacheBytes+FIRST_WRAPPED_W_NORMAL+(wrappedObjectIndex*sizeof(Vec3V)),&wrappedWorldNormal,sizeof(Vec3V));
	FastAssert(IsEqualAll(GetWrappedWorldNormal(wrappedObjectIndex),wrappedWorldNormal));
}

inline ScalarV_Out phClothManifold::GetSineHalfAngle (int wrappedObjectIndex) const
{
	FastAssert(wrappedObjectIndex>=0 && wrappedObjectIndex<GetNumWrappedObjects());
	return ScalarVFromF32(*reinterpret_cast<const float*>(m_PointCacheBytes+FIRST_WRAPPED_SINEHALF+(wrappedObjectIndex*sizeof(float))));
}

inline void phClothManifold::SetSineHalfAngle (int wrappedObjectIndex, ScalarV_In sineHalfAngle)
{
	float sineHalfAngleF = sineHalfAngle.Getf();
	sysMemCpy(m_PointCacheBytes+FIRST_WRAPPED_SINEHALF+(wrappedObjectIndex*sizeof(float)),&sineHalfAngleF,sizeof(float));
	FastAssert(IsEqualAll(GetSineHalfAngle(wrappedObjectIndex),sineHalfAngle));
}

inline void phClothManifold::AddWrappedObject (int UNUSED_PARAM(vertexIndex), Vec3V_In worldPosition, Vec3V_In worldNormal, ScalarV_In sineHalfAngle,
												const phInst* wrappedInstance, const phCollider* wrappedCollider)
{
	int wrappedIndex = GetNumWrappedObjects();
	SetNumWrappedObjects(wrappedIndex+1);
	SetWrappedWorldPosition(wrappedIndex,worldPosition);
	SetWrappedWorldNormal(wrappedIndex,worldNormal);
	SetSineHalfAngle(wrappedIndex,sineHalfAngle);
	SetWrappedInstance(wrappedIndex,wrappedInstance);
	SetWrappedCollider(wrappedIndex,wrappedCollider);
}

CompileTimeAssert(sizeof(phClothManifold)==sizeof(phManifold));

#endif // CLOTH_MANIFOLD

} // namespace rage

#endif //PHYSICS_MANIFOLD_H
