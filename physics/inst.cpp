//
// physics/inst.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#include "inst.h"
#include "data/safestruct.h"
#include "lockconfig.h"
#include "phbound/boundcomposite.h"
#include "phcore/phmath.h"
#include "vector/matrix33.h"
#include "vectormath/classes.h"

#if DEBUG_PHINST
#include "bank/bank.h"
#include "system/stack.h"
#endif

#if __ASSERT
#include "system/timemgr.h"
#endif

using namespace rage;

#if !__SPU
void (*phInst::sm_DtorCallback)(phInst *inst) = NULL;
void (*phInst::sm_ArchetypeChangeCallback)(phInst *inst, phArchetype *newArchetype) = NULL;
#endif

#if PHINST_VALIDATE_POSITION && !__SPU
int phInst::sm_DisableValidatePositionCount = 0;
#endif // PHINST_VALIDATE_POSITION && __SPU

#if DEBUG_PHINST
u16 phInst::sm_DebugLevelIndex = (u16) -1;
bool phInst::sm_DebugAllSetMatrix = false;
bool phInst::sm_DebugSetMatrixBreak = false;
#if DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK
bool phInst::sm_DebugSetMatrixCallstack = false;
static u32 s_LastSetMatrixCallstack[65536][32];
#endif
bool phInst::sm_DebugSetMatrixSpam = false;
float phInst::sm_SetMatrixCallstackTolerance = 0.1f;
#endif

// For debugging - if >0, fragments may not be deleted.
#if !__FINAL
int	phInst::sm_DeletionLockCount = 0;
#endif // !__FINAL


////////////////////////////////////////////////////////////////



#if !__SPU
phInst::phInst ()
    : m_Archetype(NULL)
	, m_LevelIndex(INVALID_INDEX)
	, m_Flags(0)
{
	SetUserData(NULL);
}

phInst::~phInst ()
{
#if !__FINAL
#if HACK_GTA4 && !__TOOL
	// Cloth is deleted on the render thread, this assert isn't of much use if it always fires in that case. At least now
	//   it can be used to track down mid-simulator or callback deletes. 
	if(sysThreadType::IsUpdateThread())
#endif // HACK_GTA4 && !__TOOL
	{
		AssertMsg(!AreDeletionsLocked(), "A phInst is being deleted outside the safe zone - this can SO crash the game.");
	}
#endif // !__FINAL

	if (sm_DtorCallback)
	{
		sm_DtorCallback(this);
	}

	const bool deleteAtZero = true;
	SetArchetypeInternal(NULL, deleteAtZero);
}


void phInst::SetDtorCallback(void (*callback)(phInst *inst))
{
	sm_DtorCallback = callback;
}


void phInst::SetArchetypeChangeCallback(void (*callback)(phInst *inst, phArchetype *newArchetype))
{
	sm_ArchetypeChangeCallback = callback;
}


void phInst::Init (phArchetype & archetype, const Matrix34 & mtx)
{
	const bool deleteAtZero = true;
	SetArchetypeInternal(&archetype, deleteAtZero);
	SetMatrix(RCC_MAT34V(mtx));
}
#endif // !__SPU

void phInst::SetArchetypeInternal (phArchetype* archetype, bool deleteAtZero)
{
	if (archetype!=NULL)
	{
		archetype->AddRef();
	}

	if (m_Archetype!=NULL)
	{
		m_Archetype->Release(deleteAtZero);
	}

	// If we're getting our archetype cleared, we'd better either not be in the level, or we need to have a physics write lock
	ASSERT_ONLY(bool archetypeIsNotNullOrNotIsInLevelOrWeHaveAWriteLock = archetype != NULL || !IsInLevel();)
#if !__SPU && ENABLE_PHYSICS_LOCK
	ASSERT_ONLY(archetypeIsNotNullOrNotIsInLevelOrWeHaveAWriteLock = archetypeIsNotNullOrNotIsInLevelOrWeHaveAWriteLock || g_PerThreadWriteLockCount > 0;)
#endif // !__SPU
	Assert(archetypeIsNotNullOrNotIsInLevelOrWeHaveAWriteLock);

	m_Archetype = archetype;

#if __RESOURCECOMPILER
	//flag only needs to be set during resourcing.
	// if it cant become active then mark it as never able to become active.
	SetInstFlag(phInst::FLAG_NEVER_ACTIVATE,false);
	if (m_Archetype)
	{
		if (m_Archetype->GetBound())
		{
			if (!m_Archetype->GetBound()->CanBecomeActive())
			{
				SetInstFlag(phInst::FLAG_NEVER_ACTIVATE, true);
			}
		}
	}
#endif
}

void phInst::SetArchetype (phArchetype* archetype, bool deleteAtZero)
{
#if !__SPU
	if (sm_ArchetypeChangeCallback)
	{
		sm_ArchetypeChangeCallback(this, archetype);
	}
#endif
	SetArchetypeInternal(archetype, deleteAtZero);
}

void phInst::SetInstFlag (u16 mask, bool value)
{
#if 0
	if (value)
	{
		m_Flags |= mask;
	}
	else
	{
		m_Flags &= ~mask;
	}
#else
	// Need to know who is setting FLAG_NEVER_ACTIVATE on ragdolls...
	Assertf(!(value && ((mask & FLAG_NEVER_ACTIVATE) != 0) && GetClassType() == 16), "phInst::SetInstFlag: [%d] Setting FLAG_NEVER_ACTIVATE on ragdoll instance %s - Please create bug for Stephane Conde with the callstack", 
		TIME.GetFrameCount(), GetArchetype() != NULL ? GetArchetype()->GetFilename() : "(none)");
	m_Flags = (m_Flags & ~mask) | (-(s32)value & mask);
#endif
}


bool phInst::CanMove () const
{
	return !HasLastMatrix();
}


void phInst::ApplyImpulse (const Vector3& impulse, const Vector3& position, int component, int element, const Vector3* UNUSED_PARAM(push), float breakScale)
{
	NotifyImpulse(impulse,position,component,element,breakScale);
}


void phInst::ApplyImpulse (const Vector3& impulse, float breakScale)
{
	ApplyImpulse(impulse,*(reinterpret_cast<const Vector3*>(&GetPosition())),0,0,NULL,breakScale);
}


void phInst::ApplyForce (const Vector3& force, const Vector3& position, int component, int element, float breakScale)
{
	NotifyForce(force,position,component,element,breakScale);
}


void phInst::ApplyImpulseChangeMotion (const Vector3& impulse, const Vector3& position, int component, int element, float breakScale)
{
	ApplyImpulse(impulse,position,component,element,NULL,breakScale);
}

void phInst::ApplyImpetus( Vec3V_In worldPosA, Vec3V_In impulseA, const int iElementA, const int iComponentA, const bool isForce, float breakScale)
{
	if (isForce)
	{
		// Apply a force.
		ApplyForce(VEC3V_TO_VECTOR3(impulseA),VEC3V_TO_VECTOR3(worldPosA),iComponentA,iElementA,breakScale);
	}
	else
	{
		// Apply an impulse.
		ApplyImpulse(VEC3V_TO_VECTOR3(impulseA),VEC3V_TO_VECTOR3(worldPosA),iComponentA,iElementA,NULL,breakScale);
	}
}

Vec3V_Out phInst::GetExternallyControlledVelocity () const
{
	return Vec3V(V_ZERO);
}


Vec3V_Out phInst::GetExternallyControlledAngVelocity () const
{
	return Vec3V(V_ZERO);
}

Vec3V_Out phInst::GetExternallyControlledVelocity (Vec3V_In position, int component) const
{
	Vec3V velocity = GetExternallyControlledVelocity();

	// Add in the internal velocity of the components if the user wants
	// NOTE: This is a bit hacky to use a ScalarV as a boolean but the alternative would be having 2 virtual functions or 
	//         return a bool and pass in a ScalarV_InOut, no option seems very elegant. We could move the component-velocity
	//         calculation to a derived class but this allows everyone to access it. 
	const ScalarV componentVelocityInvTimeStep = GetInvTimeStepForComponentExternalVelocity();
	if(!IsZeroAll(componentVelocityInvTimeStep))
	{
		const phBound* pBound = GetArchetype()->GetBound();
		if(phBound::IsTypeComposite(pBound->GetType()))
		{
			const phBoundComposite* pBoundComposite = static_cast<const phBoundComposite*>(pBound);
			if(AssertVerify(component < pBoundComposite->GetNumBounds()))
			{
				// Assuming the given position is relative to the current matrix, compute that point's internal translation from where it was with the last matrix
				const Vec3V positionInComposite = UnTransformOrtho(GetMatrix(),Vec3V(position));
				const Vec3V positionInComponent = UnTransformOrtho(pBoundComposite->GetCurrentMatrix(component),positionInComposite);
				const Vec3V positionDelta = Subtract(positionInComposite,Transform(pBoundComposite->GetLastMatrix(component),positionInComponent));
				const Vec3V internalVelocity = Scale(positionDelta,componentVelocityInvTimeStep);
				velocity = Add(velocity,Transform3x3(GetMatrix(),internalVelocity));
			}
		}
	}

	return velocity;
}

Vec3V_Out phInst::GetExternallyControlledAngVelocity (int component) const
{
	Vec3V angVelocity = GetExternallyControlledAngVelocity();

	// Add in the given component's internal angular velocity if the user wants
	// NOTE: This is a bit hacky to use a ScalarV as a boolean but the alternative would be having 2 virtual functions or 
	//         return a bool and pass in a ScalarV_InOut, no option seems very elegant. We could move the component-velocity
	//         calculation to a derived class but this allows everyone to access it. 
	const ScalarV componentVelocityInvTimeStep = GetInvTimeStepForComponentExternalVelocity();
	if(!IsZeroAll(componentVelocityInvTimeStep))
	{
		const phBound* pBound = GetArchetype()->GetBound();
		if(phBound::IsTypeComposite(pBound->GetType()))
		{
			const phBoundComposite* pBoundComposite = static_cast<const phBoundComposite*>(pBound);
			if(AssertVerify(component < pBoundComposite->GetNumBounds()))
			{
				// Calculate the internal rotation of this component over the given timestep
				const Vec3V axisAngleDelta = ComputeAxisAnglePrecise(pBoundComposite->GetCurrentMatrix(component).GetMat33ConstRef(),pBoundComposite->GetLastMatrix(component).GetMat33ConstRef());
				const Vec3V internalAngVelocity = Scale(axisAngleDelta,componentVelocityInvTimeStep);
				angVelocity = Add(angVelocity, Transform3x3(GetMatrix(),internalAngVelocity));
			}
		}
	}

	return angVelocity;
}

ScalarV_Out phInst::GetInvTimeStepForComponentExternalVelocity () const
{
	return ScalarV(V_ZERO);
}

Vec3V_Out phInst::GetExternallyControlledLocalVelocity (Vec::V3Param128 position, int component) const
{
	// Get the instance's velocity.
	Vec3V velocity = GetExternallyControlledVelocity(Vec3V(position), component);

	// Get the instance's angular velocity.
	Vec3V angVelocity = GetExternallyControlledAngVelocity();
	if (!IsZeroAll(angVelocity))
	{
		// The instance's angular velocity is not zero, so add the rotational velocity at the given point.
		Vec3V relPos = Vec3V(position) - m_Matrix.GetCol3();
		velocity = AddCrossed(velocity, angVelocity,relPos);
	}

	return velocity;
}


void phInst::GetInvMassMatrix (Matrix33& outMtx, const Vector3& UNUSED_PARAM(sourcePos), const Vector3* UNUSED_PARAM(responsePos),
								int UNUSED_PARAM(sourceComponent), int UNUSED_PARAM(responseComponent),
								const Vector3& UNUSED_PARAM(sourceNormal)) const
{
	outMtx.Zero();
}


#if __DEBUGLOG
void phInst::DebugReplay() const
{
	diagDebugLog(diagDebugLogPhysics, 'pILI', &m_LevelIndex);
	diagDebugLog(diagDebugLogPhysics, 'pIFl', &m_Flags);
	diagDebugLog(diagDebugLogPhysics, 'pIMa', &m_Matrix);
}
#endif

#if DEBUG_PHINST

void phInst::SetMatrix (Mat34V_In mtx)
{
	PDR_ONLY(debugPlayback::RecordSetMatrix(*this, mtx));
#if DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK
	if(sm_DebugSetMatrixCallstack)
	{
		sysStack::CaptureStackTrace(s_LastSetMatrixCallstack[m_LevelIndex], 32, 1);
	}
#endif

	if(sm_DebugAllSetMatrix || (m_LevelIndex == sm_DebugLevelIndex))
	{
		Vec3V old = m_Matrix.GetCol3();
		Vec3V newp = mtx.GetCol3();
		const ScalarV distSqFromLastPos = DistSquared(old, newp);
		ScalarV thresholdSqV;
		thresholdSqV.Set(sm_SetMatrixCallstackTolerance);
		if (IsLessThanAll(thresholdSqV, distSqFromLastPos))
		{
			if(sm_DebugSetMatrixSpam)
			{
				u32 callstack[32];
				sysStack::CaptureStackTrace(callstack, 32, 1);
				Displayf("phInst[%d] SetMatrix %f %f %f -> %f %f %f", m_LevelIndex, old.GetXf(), old.GetYf(), old.GetZf(), newp.GetXf(), newp.GetYf(), newp.GetZf());
				sysStack::PrintCapturedStackTrace(callstack,32);
			}

			if(sm_DebugSetMatrixBreak)
			{
				__debugbreak();
			}
		}

	}
	SetMatrix(mtx);
}

void phInst::SetMatrixNoZeroAssert (Mat34V_In mtx)
{
	PDR_ONLY(debugPlayback::RecordSetMatrix(*this, mtx));
#if DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK
	if(sm_DebugSetMatrixCallstack)
	{
		sysStack::CaptureStackTrace(s_LastSetMatrixCallstack[m_LevelIndex], 32, 1);
	}
#endif

	if(sm_DebugAllSetMatrix || (m_LevelIndex == sm_DebugLevelIndex))
	{
		Vec3V old = m_Matrix.GetCol3();
		Vec3V newp = mtx.GetCol3();
		const ScalarV distSqFromLastPos = DistSquared(old, newp);
		ScalarV thresholdSqV;
		thresholdSqV.Set(sm_SetMatrixCallstackTolerance);
		if (IsLessThanAll(thresholdSqV, distSqFromLastPos))
		{
			if(sm_DebugSetMatrixSpam)
			{
				u32 callstack[32];
				sysStack::CaptureStackTrace(callstack, 32, 1);
				Displayf("phInst[%d] SetMatrix %f %f %f -> %f %f %f", m_LevelIndex, old.GetXf(), old.GetYf(), old.GetZf(), newp.GetXf(), newp.GetYf(), newp.GetZf());
				sysStack::PrintCapturedStackTrace(callstack,32);
			}

			if(sm_DebugSetMatrixBreak)
			{
				__debugbreak();
			}
		}

	}

	SetMatrixNoZeroAssert(mtx);
}

#if DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK
const u32* phInst::GetLastSetMatrixCallstack() const
{
	return s_LastSetMatrixCallstack[m_LevelIndex];
}

void phInst::SetLevelIndex (u16 levelIndex)
{
	m_LevelIndex = levelIndex;
	memset(s_LastSetMatrixCallstack[m_LevelIndex], 0, sizeof(u32) * 32);
}
#endif

void phInst::AddWidgets(bkBank& bank)
{
	bank.PushGroup("Debug phInst", false);
	bank.AddSlider("Level Index", &sm_DebugLevelIndex, 0, 65535,1);
	bank.AddToggle("SetMatrix Spam", &sm_DebugSetMatrixSpam);
	bank.AddSlider("SetMatrix Spam Tolerance", &sm_SetMatrixCallstackTolerance, 0.0f, 1000.0f, 0.01f);
	bank.AddToggle("SetMatrix Breakpoint", &sm_DebugSetMatrixBreak);
	bank.PopGroup(); 
}
#else
void phInst::AddWidgets(bkBank& /*bank*/)
{
}
#endif

#if !__SPU
//
// Resource fixup
//
#include "data/resourcehelpers.h"

IMPLEMENT_PLACE(phInst);

#if PHINST_USER_DATA_IN_MATRIX_W_COMPONENT
phInst::phInst (datResource & )
{
}
#else // PHINST_USER_DATA_IN_MATRIX_W_COMPONENT
phInst::phInst (datResource &rsc )
{
	Assert( !m_UserData );
// TODO: I don't think we need this ptr fixup here, it is confusing - svetli
	rsc.PointerFixup(m_UserData);
}
#endif // PHINST_USER_DATA_IN_MATRIX_W_COMPONENT

#if __DECLARESTRUCT
void phInst::DeclareStruct(datTypeStruct& s)
{
	pgBase::DeclareStruct(s);
	SSTRUCT_BEGIN_BASE(phInst, pgBase)
		SSTRUCT_FIELD(phInst, m_Archetype)
		SSTRUCT_FIELD(phInst, m_LevelIndex)
		SSTRUCT_FIELD(phInst, m_Flags)
		SSTRUCT_FIELD(phInst, m_Matrix)
#if !PHINST_USER_DATA_IN_MATRIX_W_COMPONENT
		SSTRUCT_IGNORE(phInst, m_UserData)
		SSTRUCT_CONTAINED_ARRAY(phInst, pad)
#endif // !PHINST_USER_DATA_IN_MATRIX_W_COMPONENT
	SSTRUCT_END(phInst)
}
#endif // __DECLARESTRUCT

#endif // !__SPU

