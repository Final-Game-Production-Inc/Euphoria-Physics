// 
// physics/forcesolver.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_FORCESOLVER_H 
#define PHYSICS_FORCESOLVER_H 

// Tasks should only depend on taskheader.h, not task.h (which is the dispatching system)
#include "../system/taskheader.h"

#include "physics/contact.h"
#include "physics/manifold.h"

#include "system/ipc.h"
#include "vectormath/classes.h"
#include "system/messagequeue.h"

#include <profile/cellspurstrace.h>

// When enabled, this will cause the target velocity of a contact point to be based on it's relative velocity from *before* external forces/impulses
//   were applied.  
#define FORCESOLVER_USE_NEW_BOUNCE_VELOCITY	1

namespace rage {

DECLARE_TASK_INTERFACE(SolveConstraintTask);
void SolveConstraintTask( ::rage::sysTaskParameters & );

int phIpcQuerySema(sysIpcSema sema);

class sysBarrier;
class phManifold;
struct phOverlappingPairArray;
struct phTaskCollisionPair;

// PURPOSE: There are 3 force solver table rows for each of 5 constraint types, plus the rows listed here.
#define FORCE_SOLVER_TABLE_ROWS		3*phManifold::NUM_CONSTRAINT_TYPES

// PURPOSE:	There are 3 force solver table columns, for fixed, rigid and articulated bodies.
#define FORCE_SOLVER_TABLE_COLS		3

#define FORCESOLVER_LOWER_STACKSIZE (3 * 1024)

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD

#define MAX_NUM_FORCE_SOLVER_DEBUG_RECORDS 128

class phForceSolverDebugRecord
{
public:

	phForceSolverDebugRecord()
	{
	}
	phForceSolverDebugRecord(const phForceSolverDebugRecord& src)
	{
		From(src);
	}
	~phForceSolverDebugRecord()
	{
	}

	phForceSolverDebugRecord& operator=(const phForceSolverDebugRecord& src)
	{
		From(src);
		return *this;
	}

	void SetColliders(const phCollider* pBodyA, const phCollider* pBodyB)
	{
		m_pBodyA=pBodyA;
		m_pBodyB=pBodyB;
	}
	void SetNormal(const Vector3 vNormal)
	{
		m_vNormal=vNormal;
	}

	void SetNormal(const Vec3V vNormal)
	{
		m_vNormal=RCC_VECTOR3(vNormal);
	}
	void SetStartVelocities(const Vector3& vVelA, const Vector3& vVelB, const Vector3& vOmegaA, const Vector3& vOmegaB)
	{
		m_vVelStartA=vVelA;
		m_vVelStartB=vVelB;
		m_vOmegaStartA=vOmegaA;
		m_vOmegaStartB=vOmegaB;
	}
	void SetEndVelocities(const Vector3& vVelA, const Vector3& vVelB, const Vector3& vOmegaA, const Vector3& vOmegaB)
	{
		m_vVelEndA=vVelA;
		m_vVelEndB=vVelB;
		m_vOmegaEndA=vOmegaA;
		m_vOmegaEndB=vOmegaB;
	}
	void SetFunctionName(const char* pName)
	{
		strcpy(&m_functionName[0],pName);
	}


	const phCollider* GetColliderA() const
	{
		return m_pBodyA;
	}
	const phCollider* GetColliderB() const
	{
		return m_pBodyB;
	}
	const Vector3& GetNormal() const
	{
		return m_vNormal;
	}
	const char* GetFunctionNamePtr() const
	{
		return m_functionName;
	}
	const Vector3& GetPreApplyVelA() const
	{
		return m_vVelStartA;
	}
	const Vector3& GetPreApplyVelB() const
	{
		return m_vVelStartB;
	}
	const Vector3& GetPreApplyOmegaA() const
	{
		return m_vOmegaStartA;
	}
	const Vector3& GetPreApplyOmegaB() const
	{
		return m_vOmegaStartB;
	}
	const Vector3& GetPostApplyVelA() const
	{
		return m_vVelEndA;
	}
	const Vector3& GetPostApplyVelB() const
	{
		return m_vVelEndB;
	}
	const Vector3& GetPostApplyOmegaA() const
	{
		return m_vOmegaEndA;
	}
	const Vector3& GetPostApplyOmegaB() const
	{
		return m_vOmegaEndB;
	}


private:

	const phCollider* m_pBodyA;
	const phCollider* m_pBodyB;
	Vector3 m_vNormal;
	Vector3 m_vVelStartA,m_vVelStartB;		//Velocities at start of fuction
	Vector3 m_vVelEndA,m_vVelEndB;			//Velocities at end of fucntion.
	Vector3 m_vOmegaStartA,m_vOmegaStartB;	//Angular velocities at start of function.
	Vector3 m_vOmegaEndA,m_vOmegaEndB;		//Angular velocities at end of function.
	char m_functionName[64];

	void From(const phForceSolverDebugRecord& src)
	{
		SetColliders(src.m_pBodyA,src.m_pBodyB);
		SetNormal(src.m_vNormal);
		SetStartVelocities(src.m_vVelStartA,src.m_vVelStartB,src.m_vOmegaStartA,src.m_vOmegaStartB);
		SetEndVelocities(src.m_vVelEndA,src.m_vVelEndB,src.m_vOmegaEndA,src.m_vOmegaEndB);
		SetFunctionName(src.m_functionName);
	}
};
#endif

#if __SPU
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(globals.ptrToMember))
#else
#define CALL_MEMBER_FN(object,ptrToMember)  (object).ptrToMember
#endif

class phArticulatedBody;
class phArticulatedCollider;
class phPhaseSpaceVector;

typedef void (phArticulatedCollider::*GetInverseMassMatrixPtr)(Mat33V_InOut, Vec::V3Param128, int) const;
typedef void (phArticulatedCollider::*GetInverseMassMatrixSelfPtr)(Mat33V_InOut, Vec::V3Param128, Vec::V3Param128, int, int ) const;
typedef void (*GetInverseInertiaMatrixPtr)(Mat33V_In, Vec::V3Param128, Mat33V_InOut);
typedef void (phManifold::*ManifoldExchangePtr)();

typedef Vec3V_Out (phArticulatedBody::*GetLocalVelocityPtr)(int, Vec::V3Param128);
typedef Vec3V_Out (phArticulatedBody::*GetLocalVelocityNoPropPtr)(int, Vec::V3Param128) const;
typedef Vec3V_Out (phArticulatedBody::*GetAngularVelocityPtr)(int);
typedef Vec3V_Out (phArticulatedBody::*GetAngularVelocityNoPropPtr)(int) const;
typedef void (phArticulatedBody::*ApplyImpulsePtr)(int, Vec3V_In, Vec3V_In);
typedef void (phArticulatedBody::*ApplyAngImpulsePtr)(int, Vec::V3Param128);

#if __WIN32
#pragma warning(push)
#pragma warning(disable: 4324)
#endif

struct phForceSolverGlobals
{
	BoolV calculateBounceAndTangent;
	ScalarV separateBias;
	ScalarV invTimeStep;
	ScalarV allowedPenetration;
	ScalarV halfAllowedPenetration;
	ScalarV allowedAnglePenetration;
	ScalarV minBounce;

#if TURN_CLAMPING
	ScalarV maxTurn;
#endif

	GetInverseMassMatrixPtr GetInverseMassMatrix;
	GetInverseMassMatrixSelfPtr GetInverseMassMatrixSelf;
	GetInverseInertiaMatrixPtr GetInverseInertiaMatrix;
	ManifoldExchangePtr Exchange;

	GetAngularVelocityPtr GetAngularVelocity;
	GetLocalVelocityPtr GetLocalVelocity;
	GetLocalVelocityNoPropPtr GetLocalVelocityNoProp;
	ApplyImpulsePtr ApplyImpulse;
	ApplyAngImpulsePtr ApplyAngImpulse;

	bool applyWarmStart;
	bool clearWarmStart;
	bool baumgarteJointLimitTerm;
};

#if __WIN32
#pragma warning(pop)
#endif

class phForceSolver
{
public:
	static const u32 MAX_BATCHES_NUM = 20;
	static const u32 MAX_TASKS_NUM = 32;

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	static phForceSolverDebugRecord sm_aForceSolverRecords[MAX_NUM_FORCE_SOLVER_DEBUG_RECORDS];
	static int sm_iNumForceSolverRecords;
	static bool sm_bActivateDebugRecords;
	static void AddForceSolverDebugRecord(const phForceSolverDebugRecord& record);
	static void ActivateDebugRecords()
	{
		sm_bActivateDebugRecords=true;
	}
	static void DeactivateDebugRecords()
	{
		sm_bActivateDebugRecords=false;
	}
	static void ClearForceSolverDebugRecords()
	{
		sm_iNumForceSolverRecords=0;
	}
	static int GetNumForceSolverDebugRecords()
	{
		return sm_iNumForceSolverRecords;
	}
	static const phForceSolverDebugRecord& GetForceSolverDebugRecord(const int index)
	{
		Assert(index<sm_iNumForceSolverRecords);
		return sm_aForceSolverRecords[index];
	}
#endif

	struct ParallelGroup {
		phManifold* manifolds[MAX_BATCHES_NUM];

		u32 waitTime;
		u32 solveTime;
		u16 numManifolds;
		u16 phasesToRelease;

#if PER_MANIFOLD_SOLVER_METRICS
		__forceinline void AddWaitTime(utimer_t waitTimeParam)
		{
#if __SPU
			sysInterlockedAdd(&waitTime, waitTimeParam);
#else
			utimer_t newWaitTime = waitTime + waitTimeParam;

			if (newWaitTime < 0xffffffff)
			{
				waitTime = (u32)newWaitTime;
			}
			else
			{
				waitTime = 0xffffffff;
			}
#endif
		}

		__forceinline void AddSolveTime(utimer_t solveTimeParam)
		{
#if __SPU
			sysInterlockedAdd(&solveTime, solveTimeParam);
#else
			utimer_t newSolveTime = solveTime + solveTimeParam;

			if (newSolveTime < 0xffffffff)
			{
				solveTime = (u32)newSolveTime;
			}
			else
			{
				solveTime = 0xffffffff;
			}
#endif
		}
#endif // __BANK

	} ;

	static const u32 REUSE_COLLIDER = 0xffffffff;

	typedef void (*ConstraintFunc)(phManifold& manifold, const phForceSolverGlobals& global);
	typedef ConstraintFunc ConstraintTable[FORCE_SOLVER_TABLE_ROWS][FORCE_SOLVER_TABLE_COLS];

	int AllocatePhase();
	int ReleasePhase(u32 phase);
	void IterateOneConstraint(phManifold& manifold, ConstraintTable functionTable);
	void IterateConstraints(u32 phase, bool preResponse);

	void InitGlobals();
	void Solve();
	void SolveSingleThreaded();

	void Sync();

	struct CompletedParaGroup
	{
		u16 index;
		u16 iteration;
	};

	static phForceSolverGlobals sm_Globals;

	u32 m_NumPhases;
	u32 m_NumIterations;
	u32 m_ExtraReleasesAtEnd;
	float m_TimeStep;
	float m_SeparateBias;
	float m_AllowedPenetration;
	float m_AllowedAnglePenetration;
	float m_MinBounce;
#if FORCESOLVER_USE_NEW_BOUNCE_VELOCITY
	Vec3V m_Gravity;
#endif	// !FORCESOLVER_USE_NEW_BOUNCE_VELOCITY
#if TURN_CLAMPING
	float m_MaxTurn;
#else
	float m_Padding;
#endif
	bool m_ApplyWarmStart;
	bool m_ClearWarmStart;
	bool m_PreResponse;
	bool m_DmaPlanAsList;
	u32* m_PhaseAllocateAtom;
	u64* m_PhaseReleaseAtom;
	sysIpcSema m_PhaseReleaseSema;
	u32* m_OutOfOrderCompletionBuffer;
	u32 m_LastPhase;
	u32 m_MaxFragSize;

	static const u32 FIRST_PHASE_OF_ITERATION_SO_NO_LAST_PHASE = 0xffffffff;
	static const size_t COLLIDER_HEAP_SIZE;

	// PURPOSE: flag to tell whether to compute pushes in the force solver
	// NOTES:	When this is off, penetrations are resolved with impulses.
	bool m_ComputePushesAndTurns;

	bool m_BaumgarteJointLimitTerm;

	ParallelGroup* m_ParallelGroups;
	phOverlappingPairArray* m_OverlappingPairs;

#if __64BIT
#define PH_FORCE_SOLVER_PADDING_NEEDED 0
#else
#define PH_FORCE_SOLVER_PADDING_NEEDED 4
#endif

#if PH_FORCE_SOLVER_PADDING_NEEDED > 0
	u8 pad[PH_FORCE_SOLVER_PADDING_NEEDED];
#endif
} ;


inline ScalarV_Out ComputeImpulseDenomInv (Vec3V_In position, Vec3V_In centerOfMass, ScalarV_In massInv, Mat33V_In inertiaInv, Vec3V_In worldNormal)
{
	Vec3V localPosA = Subtract(position,centerOfMass);
	Mat33V scaleMatrix;
	Mat33VFromScale(scaleMatrix,massInv,massInv,massInv);
	Mat33V cross;
	CrossProduct(cross,localPosA);
	Mat33V term;
	Multiply(term,cross,inertiaInv);
	Multiply(term,term,cross);

	Mat33V K(scaleMatrix - term);
	return InvertSafe(Dot(Multiply(K,worldNormal),worldNormal));
}

inline ScalarV_Out ComputeImpulseDenomInv (Vec3V_In positionA, Vec3V_In positionB, Vec3V_In centerOfMassA, Vec3V_In centerOfMassB, ScalarV_In massInvA,
	ScalarV_In massInvB, Mat33V_In inertiaInvA, Mat33V_In inertiaInvB, Vec3V_In normal)
{
	Vec3V localPosA = positionA - centerOfMassA;
	Vec3V localPosB = positionB - centerOfMassB;
	Mat33V scaleMatrix;
	Mat33VFromScale(scaleMatrix,Vec3V(massInvA)+Vec3V(massInvB));
	Mat33V crossA, crossB;
	CrossProduct(crossA,localPosA);
	CrossProduct(crossB,localPosB);
	Mat33V termA, termB;
	Multiply(termA,crossA,inertiaInvA);
	Multiply(termA,termA,crossA);
	Multiply(termB,crossB,inertiaInvB);
	Multiply(termB,termB,crossB);

	Mat33V K(scaleMatrix - termA - termB);
	return InvertSafe(Dot(Multiply(K,normal),normal));
}


} // namespace rage

#endif // PHYSICS_FORCESOLVER_H 