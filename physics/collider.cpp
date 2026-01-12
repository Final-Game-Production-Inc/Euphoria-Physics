//
// physics/collider.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "collider.h"
#include "colliderdispatch.h"

#include "system/dma.h"
#include "archetype.h"
#include "contact.h"
#include "simulator.h"
#include "sleep.h"

#include "data/resourcehelpers.h"
#include "grcore/debugdraw.h"
#include "grcore/font.h"
#include "math/random.h"
#include "math/simplemath.h"
#include "phbound/boundcomposite.h"
#include "phbound/boundgeom.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "vector/colors.h"
#include "vector/matrix33.h"
#include "system/spinlock.h" // for sys_lwsync
#include "system/stack.h"

#if __BANK
#include "bank/bank.h"
#endif

#define COLLIDER_CLAMP_ROTATION_INSTEAD_OF_ANG_VELOCITY	0

// Set DEBUG_COLLIDERS to 1 to enable code that will check the collider against
// a static g_debugColliderIndex and generate spam and/or trigger breakpoints when
// values being applied exceed a tolerance specified by g_debugColliderTolerance.
//
// Note that this code can only be enabled in __PFDRAW && !__SPU builds.
//
#define DEBUG_COLLIDERS (__BANK && __PFDRAW && !__SPU)

namespace rage {


#define MAX_PUSH_CLAMP 100.0f
#define MAX_PUSH_CLAMP_HEX 0x42C80000 // MAX_PUSH_CLAMP value in hex. Change this whenever you change 'MAX_PUSH_CLAMP'!

EXT_PFD_DECLARE_GROUP(Bounds);
#if TRACK_COLLISION_TIME
EXT_PFD_DECLARE_ITEM(CollisionTime);
#endif
EXT_PFD_DECLARE_ITEM(Forces);
EXT_PFD_DECLARE_ITEM(Impulses);
EXT_PFD_DECLARE_ITEM(Pushes);
EXT_PFD_DECLARE_ITEM(Torques);
EXT_PFD_DECLARE_ITEM(AngularImpulses);
EXT_PFD_DECLARE_ITEM(Turns);
EXT_PFD_DECLARE_ITEM(PreviousFrame);
EXT_PFD_DECLARE_ITEM(PreviousSafeFrame);
EXT_PFD_DECLARE_ITEM(DrawBoundMaterials);
EXT_PFD_DECLARE_ITEM(Solid);
EXT_PFD_DECLARE_GROUP(Colliders);
EXT_PFD_DECLARE_ITEM(ColliderMassText);
EXT_PFD_DECLARE_ITEM(ColliderAngInertiaText);
EXT_PFD_DECLARE_ITEM(ColliderExtraAllowedPenetrationText);

#if __DEV
phCollider* phCollider::sm_DebugColliderUpdate;
#endif

#if DEBUG_COLLIDERS

static int g_debugColliderIndex = 65535;
static float g_debugColliderTolerance = 0.001f;
static bool g_debugColliderSpam = true;
static bool g_debugColliderBreak = false;
static bool g_debugColliderStack = true;

void phCollider::AddWidgets(bkBank& bank)
{
	bank.PushGroup("Debug Collider", false);
	bank.AddSlider("Collider Index", &g_debugColliderIndex, 0, 65535,1);
	bank.AddSlider("Collider Tolerance", &g_debugColliderTolerance, 0.0f, 100000.0f,0.001f);
	bank.AddToggle("Collider Spam", &g_debugColliderSpam);
	bank.AddToggle("Collider Stacks", &g_debugColliderStack);
	bank.AddToggle("Collider Breakpoint", &g_debugColliderBreak);
	bank.PopGroup(); 
}

#define DEBUG_COLLIDER(x,y) DebugCollider(this,x,y)
void DebugCollider(phCollider* pCollider, const char* label, const Vector3& val)
{
	if((g_debugColliderIndex == 65535) || !pCollider || !pCollider->GetInstance() || (pCollider->GetInstance()->GetLevelIndex() == phInst::INVALID_INDEX) || pCollider->GetInstance()->GetLevelIndex() != g_debugColliderIndex)
	{
		return;
	}

	if(val.Mag() < g_debugColliderTolerance)
	{
		return;
	}
	if(g_debugColliderSpam)
	{
		Displayf("DebugCollider: %s %f %f %f", label, val.x, val.y, val.z);
	}

	if(g_debugColliderStack)
	{
		size_t callstack[32];
		sysStack::CaptureStackTrace(callstack, 32, 1);
		if(!g_debugColliderSpam)
		{
			Displayf("DebugCollider Stack: %s", label);
		}
		sysStack::PrintCapturedStackTrace(callstack,32);
	}

	if(g_debugColliderBreak)
	{
		__debugbreak();
	}
}
#else
void phCollider::AddWidgets(bkBank& /*bank*/)
{
}
#define DEBUG_COLLIDER(x,y)
#endif

void phCollider::SetType (int iType)
{
//#if __BANK
//	if (iType)
//		Displayf("Setting collider type to TYPE_ARTICULATED_BODY");
//	else
//		Displayf("Setting collider type to TYPE_RIGID_BODY");
//#endif

	AssertMsg(m_ColliderType != iType, "Attempted to set a collider's type to it's current type");

	m_ColliderType = iType;

	if (m_Instance)
	{
		InitInertia();

		// Get the maximum speed and angular speed from the instance's physics archetype.
		phArchetype* archetype = m_Instance->GetArchetype();
		m_MaxSpeed = archetype->GetMaxSpeed();
		m_MaxAngSpeed = archetype->GetMaxAngSpeed();
		m_GravityFactor = archetype->GetGravityFactor();

		// Set the velocity and angular velocity.
		SetVelocity(GetVelocity().GetIntrin128());
		SetAngVelocity(GetAngVelocity().GetIntrin128());

		ResetSolverInvMass();
		ResetSolverInvAngInertia();
		ResetRotationConstraintInvMass();
		ResetRotationConstraintInvAngInertia();
		ResetTranslationConstraintInvMass();
		ResetTranslationConstraintInvAngInertia();
		SetNeedsUpdateBeforeFinalIterations(false);
	}
}


phCollider::phCollider ()
{
//	m_Mass = 1.0f;
//	m_InvMass = 1.0f;
	m_AngInertiaXYZMassW = Vec4V(V_ONE);
//	phMathInertia::ClampAngInertia(RC_VECTOR3(m_AngInertiaXYZMassW));
	m_InvAngInertiaXYZInvMassW = InvertSafe(m_AngInertiaXYZMassW);
	m_Matrix = Mat34V(V_IDENTITY);

	m_Instance = NULL;
	m_ColliderType = TYPE_RIGID_BODY;
	m_Sleep = NULL;

	m_MaxSpeed = DEFAULT_MAX_SPEED;
	m_MaxAngSpeed = DEFAULT_MAX_ANG_SPEED;

	m_Push = m_AppliedPush = Vec3V(V_ZERO);

	m_CurrentlyPenetrating = false;
	m_CurrentlyPenetratingCount = 0;
	m_NonPenetratingAfterTeleport = false;

	m_CausesPushCollisions = true;
	m_PreventsPushCollisions = false;

	TRACK_PUSH_COLLIDERS_ONLY(m_IsInPushPair = false;)

	m_DampingEnabled = true;
	m_DoubleDampingEnabled = false;

	m_SolverInvAngInertiaXYZSolverInvMassW = Vec4V(V_ZERO);

	m_SolverInvAngInertiaResetOverride = Vec3V(V_ZERO);
	m_UseSolverInvAngInertiaResetOverride = false;
	m_UseSolverInvAngInertiaResetOverrideInWheelIntegrator = false;

	m_TranslationConstraintInvAngInertiaXYZInvMassZ = Vec4V(V_ZERO);
	m_RotationConstraintInvAngInertiaXYZInvMassZ = Vec4V(V_ZERO);

	SetReferenceFrameVelocityDampingRate(0.0f);
	SetReferenceFrameAngularVelocityDampingRate(0.0f);

	m_ExtraAllowedPenetration = 0.0f;

	m_GravityFactor = 1.0f;

	Freeze();

#if TRACK_COLLISION_TIME
	m_CollisionTime = 1.0f;
#endif

#if __PS3
	m_DmaPlan = NULL;
#endif
}


phCollider::~phCollider ()
{
}


void phCollider::Init (phInst* instance, phSleep* sleep)
{
	SetInstanceAndReset(instance);

	SetSleep(sleep);
}


void phCollider::SetInstanceAndReset (phInst* instance)
{
	m_Instance = instance;

	if (m_Instance)
	{
		// Set the mass and angular inertia and their inverses from the instance's physics archetype.
		InitInertia();

		// Get the maximum speed and angular speed from the instance's physics archetype.
		phArchetype* archetype = m_Instance->GetArchetype();
		m_MaxSpeed = archetype->GetMaxSpeed();
		m_MaxAngSpeed = archetype->GetMaxAngSpeed();
		m_GravityFactor = archetype->GetGravityFactor();
		m_ApproximateRadius = archetype->GetBound()->GetRadiusAroundCentroid();
	}
	else
	{
//		m_Mass = 1.0f;
//		m_InvMass = 1.0f;
		m_AngInertiaXYZMassW = Vec4V(V_ONE);
//		phMathInertia::ClampAngInertia(RC_VECTOR3(m_AngInertia));
		m_InvAngInertiaXYZInvMassW = InvertSafe( m_AngInertiaXYZMassW );
	}

	// Do this after setting the mass, inertia, etc. above as it resets other variables which need these to be set.
	Reset();

	if(m_Instance)
	{
		// Set the velocity and angular velocity from the instance's motion.
		GetMotionFromInstance();
	}
}


void phCollider::InitInertia ()
{
	Assert(m_Instance);
	phArchetype* archetype = m_Instance->GetArchetype();
	m_AngInertiaXYZMassW.SetXYZ(VECTOR3_TO_VEC3V(archetype->GetAngInertia()));
	m_InvAngInertiaXYZInvMassW.SetXYZ(VECTOR3_TO_VEC3V(archetype->GetInvAngInertia()));
	m_AngInertiaXYZMassW.SetWf(archetype->GetMass());
	m_InvAngInertiaXYZInvMassW.SetWf(archetype->GetInvMass());
	ASSERT_ONLY(const char *pName = (GetInstance() && GetInstance()->GetArchetype() && GetInstance()->GetArchetype()->GetFilename()) ? GetInstance()->GetArchetype()->GetFilename() : "unknown");
	Assertf(IsGreaterThanAll(m_AngInertiaXYZMassW, Vec4V(V_ZERO)), "This object %s has negative inertia <%f, %f, %f, %f>", pName, m_AngInertiaXYZMassW.GetXf(), m_AngInertiaXYZMassW.GetYf(), m_AngInertiaXYZMassW.GetZf(), m_AngInertiaXYZMassW.GetWf());
}

#if !__SPU
void phCollider::GetMotionFromInstance ()
{
	// Get the velocity from the instance, and clamp it.
	Assert(m_Instance);
	Vec3V velocity = m_Instance->GetExternallyControlledVelocity();
	SetVelocity(velocity.GetIntrin128());
	ClampVelocity();

	// Get the angular velocity from the instance, and clamp it.
	velocity = m_Instance->GetExternallyControlledAngVelocity();
	SetAngVelocity(velocity.GetIntrin128());
	ClampAngularVelocity();

	// Get the previous instance matrix from the instance.
	SetLastInstanceMatrix(PHLEVEL->GetLastInstanceMatrix(m_Instance));
}
#endif

void phCollider::SetInertia (Vec::V3Param128 mass, Vec::V3Param128 angInertia)
{
	Vec3V vAngInertia = Vec3V(angInertia);
	phMathInertia::ClampAngInertia(RC_VECTOR3(vAngInertia));
	m_AngInertiaXYZMassW = Vec4V(vAngInertia, ScalarV(mass));
	ASSERT_ONLY(const char *pName = (GetInstance() && GetInstance()->GetArchetype() && GetInstance()->GetArchetype()->GetFilename()) ? GetInstance()->GetArchetype()->GetFilename() : "unknown");
	Assertf(IsGreaterThanAll(m_AngInertiaXYZMassW, Vec4V(V_ZERO)), "This object %s has negative inertia <%f, %f, %f, %f>", pName, m_AngInertiaXYZMassW.GetXf(), m_AngInertiaXYZMassW.GetYf(), m_AngInertiaXYZMassW.GetZf(), m_AngInertiaXYZMassW.GetWf());
//	m_Mass = mass;
//	m_InvMass = (mass>VERY_SMALL_FLOAT ? 1.0f/m_Mass : FLT_MAX);
//	m_AngInertia = Vec3V(angInertia);
//	phMathInertia::ClampAngInertia(RC_VECTOR3(m_AngInertia));
	m_InvAngInertiaXYZInvMassW = InvertSafe(m_AngInertiaXYZMassW);
}


#if !__SPU
void phCollider::Reset ()
{
	m_Matrix = Mat34V(V_IDENTITY);

	Freeze();

	if (m_Instance)
	{
		SetLastInstanceMatrix(PHLEVEL->GetLastInstanceMatrix(m_Instance));
		SetColliderMatrixFromInstance();
	}
	else
	{
		m_LastInstanceMatrix = Mat34V(V_IDENTITY);
	}


	m_RejuvenateCount = 0;

	if (m_Sleep)
	{
		m_Sleep->Reset();
	}

	m_CurrentlyPenetrating = false;
	m_CurrentlyPenetratingCount = 0;
	m_NonPenetratingAfterTeleport = false;

	m_CausesPushCollisions = true;
	m_PreventsPushCollisions = false;

	m_DampingEnabled = true;
	m_DoubleDampingEnabled = false;

	TRACK_PUSH_COLLIDERS_ONLY(m_IsInPushPair = false;)

	m_ExtraAllowedPenetration = 0.0f;

	m_SolverInvAngInertiaResetOverride = Vec3V(V_ZERO);
	m_UseSolverInvAngInertiaResetOverride = false;
	m_UseSolverInvAngInertiaResetOverrideInWheelIntegrator = false;

	ResetSolverInvMass();
	ResetSolverInvAngInertia();
	ResetRotationConstraintInvMass();
	ResetRotationConstraintInvAngInertia();
	ResetTranslationConstraintInvMass();
	ResetTranslationConstraintInvAngInertia();
	SetNeedsUpdateBeforeFinalIterations(false);

#if TRACK_COLLISION_TIME
	m_CollisionTime = 1.0f;
#endif
}
#endif

void phCollider::Freeze ()
{
	const Vec3V vZero(V_ZERO);
	m_LastVelocity =
		m_LastAngVelocity =
		m_Velocity =
		m_AngVelocity =
		vZero;
	PDR_ONLY(debugPlayback::RecordSetVelocity(*GetInstance(), m_Velocity.GetIntrin128()));
	PDR_ONLY(debugPlayback::RecordSetAngularVelocity(*GetInstance(), m_AngVelocity.GetIntrin128()));

	SetReferenceFrameVelocity(vZero.GetIntrin128());
	SetReferenceFrameAngularVelocity(vZero.GetIntrin128());
	ZeroForces();
}


void phCollider::ZeroForces ()
{
	m_Force = 
		m_Torque =
		m_Impulse =
		m_AngImpulse =
		m_AppliedPush =
		m_AppliedTurn =
		m_Push =
		m_Turn =
		m_VelocityBeforeForce =
		m_AngVelocityBeforeForce =
		Vec3V(V_ZERO);
}


void phCollider::ClampVelocity ()
{
/*
	Vector3 ms, ms2;
	ms.Set(m_MaxSpeed);
	Vector3 speed2 = m_Velocity.Mag2V();
	ms2.Multiply(ms,ms);

	Vector3 comparitor = ms2.IsGreaterThanV( ms2 );
	Vector3 clampor = speed2.RecipSqrtV();
	clampor.Multiply( ms );

	Vector3 clampScale = comparitor.Select( VEC3_IDENTITY, clampor );
	m_Velocity.Multiply( clampScale );
	m_Momentum.Multiply( clampScale );

*/
	// TODO: Vectorized really isn't better?

	Vec3f s_veloc;
	LoadAsScalar( s_veloc, m_Velocity );
	float speed2 = MagSquared( s_veloc );

	if (speed2 > m_MaxSpeed*m_MaxSpeed)
	{
		float clampScale = m_MaxSpeed*FPInvSqrt(speed2);
		Vec3f s_clampScale = Vec3FromF32(clampScale);
		s_veloc = Scale(s_veloc, s_clampScale);

		StoreAsScalar( m_Velocity, s_veloc );
	}
}


void phCollider::ClampAngularVelocity ()
{
	// SCALAR VERSION
	Vec3f s_AngVelocity;
	LoadAsScalar( s_AngVelocity, m_AngVelocity );

	float angSpeed2 = MagSquared( s_AngVelocity );
	if(angSpeed2 > m_MaxAngSpeed*m_MaxAngSpeed)
	{
		float clampScale;

		clampScale = m_MaxAngSpeed*FPInvSqrt(angSpeed2);		
		s_AngVelocity = Scale(s_AngVelocity, clampScale);

		StoreAsScalar( m_AngVelocity, s_AngVelocity );
	}

	// NO-BRANCH VECTORIZED VERSION
	// Clamp the angular velocity (and angular momentum) if they're too large.
	//Vec3V v_AngVelocity = RCC_VEC3V(m_AngVelocity);
	//Vec3V v_AngMomentum = RCC_VEC3V(m_AngMomentum);
	//ScalarV angSpeed2 = MagSquared( v_AngVelocity );
	//ScalarV v_MaxAngSpeed = ScalarV( m_MaxAngSpeed );
	//ScalarV v_MaxAngSpeed2 = Multiply( v_MaxAngSpeed, v_MaxAngSpeed );
	//VecBoolV isGt = IsGreaterThan( angSpeed2, v_MaxAngSpeed2 );
	//ScalarV clampScale = Multiply( v_MaxAngSpeed, InvSqrt( angSpeed2 ) );
	//RC_VEC3V(m_AngVelocity) = SelectFT( isGt, v_AngVelocity, Scale( Vec3V(clampScale), v_AngVelocity ) );
	//RC_VEC3V(m_AngMomentum) = SelectFT( isGt, v_AngMomentum, Scale( Vec3V(clampScale), v_AngMomentum ) );
}

#if __PFDRAW
void phCollider::ProfileDraw() const
{
	if(PFDGROUP_Colliders.Begin())
	{
		int lines = 0;
		if(PFD_ColliderMassText.Begin())
		{
			char massBuffer[128];
			sprintf(massBuffer,"Mass: %.1f",GetMass());
			grcDebugDraw::Text(GetPosition(),Color_pink,0,grcFont::GetCurrent().GetHeight()*lines,massBuffer,true);
			++lines;
			PFD_ColliderMassText.End();
		}
		if(PFD_ColliderAngInertiaText.Begin())
		{
			char angInertiaBuffer[128];
			sprintf(angInertiaBuffer,"Angular Inertia: %.1f, %.1f, %.1f",GetAngInertia().GetXf(), GetAngInertia().GetYf(), GetAngInertia().GetZf());
			grcDebugDraw::Text(GetPosition(),Color_pink,0,grcFont::GetCurrent().GetHeight()*lines,angInertiaBuffer,true);
			++lines;
			PFD_ColliderAngInertiaText.End();
		}
		if(PFD_ColliderExtraAllowedPenetrationText.Begin())
		{
			char extraAllowedPenetrationTextBuffer[128];
			sprintf(extraAllowedPenetrationTextBuffer,"Extra Allowed Penetration: %.3f",GetExtraAllowedPenetration());
			grcDebugDraw::Text(GetPosition(),Color_pink,0,grcFont::GetCurrent().GetHeight()*lines,extraAllowedPenetrationTextBuffer,true);
			++lines;
			PFD_ColliderExtraAllowedPenetrationText.End();
		}

		PFDGROUP_Colliders.End();
	}

	if (PFDGROUP_Bounds.Begin(false))
	{
		if (PFD_PreviousFrame.GetEnabled())
		{
			grcColor(PFD_PreviousFrame.GetBaseColor());
			m_Instance->GetArchetype()->GetBound()->DrawLast(m_LastInstanceMatrix, PFD_DrawBoundMaterials.WillDraw(), PFD_Solid.GetEnabled());
		}

		if (PFD_PreviousSafeFrame.GetEnabled())
		{
			grcColor(PFD_PreviousSafeFrame.GetBaseColor());
			m_Instance->GetArchetype()->GetBound()->DrawLast(m_LastSafeInstanceMatrix, PFD_DrawBoundMaterials.WillDraw(), PFD_Solid.GetEnabled());
		}

		PFDGROUP_Bounds.End();
	}

#if TRACK_COLLISION_TIME
	if (PFD_CollisionTime.Begin())
	{
		grcDrawLabelf(RCC_VECTOR3(GetPosition()), "%f", m_CollisionTime);

		PFD_CollisionTime.End();
	}
#endif

	if (m_Sleep)
	{
		m_Sleep->ProfileDraw();
	}
}
#endif // __PFDRAW


void phCollider::DrawForce (Vec3V_In PF_DRAW_ONLY(force), Vec3V_In PF_DRAW_ONLY(position))
{
#if __PFDRAW
	DEBUG_COLLIDER("Force ", RCC_VECTOR3(force));
	// Try to begin the drawing group.
	if (PFD_Forces.GetEnabled())
	{
		// Set the force vector end point to make the arrow have a length of 1 m for a force that gives it an acceleration of 10 m/s^2
		Vec3V end = position;
		end = AddScaled(end, force, Vec3VFromF32(0.1f*GetInvMass()));

		// Draw the arrow and end the drawing group.
		PFD_Forces.DrawArrow(RCC_VECTOR3(position), RCC_VECTOR3(end));
	}
#endif
}


void phCollider::DrawImpulse (Vec3V_In PF_DRAW_ONLY(impulse), Vec3V_In PF_DRAW_ONLY(position))
{
#if __PFDRAW
	DEBUG_COLLIDER("Impulse ", RCC_VECTOR3(impulse));
	// Try to begin the drawing group.
	if(PFD_Impulses.GetEnabled())
	{
		// Set the impulse vector end point to make the arrow have a length of 1 m for an impulse that gives it a velocity change of 10 m/s
		Vec3V end = position;
		end = AddScaled(end, impulse, Vec3VFromF32(GetInvMass()));

		// Draw the arrow and end the drawing group.
		PFD_Impulses.DrawArrow(RCC_VECTOR3(position), RCC_VECTOR3(end));
	}
#endif
}


void phCollider::DrawPush (Vec3V_In PF_DRAW_ONLY(push), Vec3V_In PF_DRAW_ONLY(position))
{
#if __PFDRAW
	DEBUG_COLLIDER("Push ", RCC_VECTOR3(push));
	// Try to begin the drawing group.
	if(PFD_Pushes.GetEnabled())
	{
		// Draw the arrow and end the drawing group.
		Vec3V end = position;
		end = Add(end, push);
		PFD_Pushes.DrawArrow(RCC_VECTOR3(position), RCC_VECTOR3(end));
	}
#endif
}


void phCollider::DrawTorque (Vec3V_In PF_DRAW_ONLY(torque), Vec3V_In PF_DRAW_ONLY(position))
{
#if __PFDRAW
	DEBUG_COLLIDER("Torque ", RCC_VECTOR3(torque));
	// Try to begin the drawing group.
	if(PFD_Torques.Begin())
	{
		Vec3f s_torque;
		LoadAsScalar( s_torque, torque );

		// Set the torque vector end point to make the arrow have a length of 1 m for a torque that gives it an angular acceleration of of 1 radian/s^2
		float torqueLength = Mag(s_torque);
		if (torqueLength > SMALL_FLOAT)
		{
			float logTorque = log10(torqueLength + 1.0f) * 0.25f;
			Vec3V end = torque;
			end = AddScaled(position, end, Vec3VFromF32(logTorque/torqueLength));

			// Draw the arrow and end the drawing group.
			bool oldLighting = grcLighting(false);
			grcWorldIdentity();
			grcDrawSpiral(RCC_VECTOR3(position),RCC_VECTOR3(end),0.0f,logTorque*0.125f,10.0f,0.5f,logTorque*0.05f);
			grcLighting(oldLighting);
		}

		PFD_Torques.End();
	}
#endif
}


void phCollider::DrawAngularImpulse (Vec3V_In PF_DRAW_ONLY(angImpulse), Vec3V_In PF_DRAW_ONLY(position))
{
#if __PFDRAW
	DEBUG_COLLIDER("AngularImpulse ", RCC_VECTOR3(angImpulse));
	// Try to begin the drawing group.
	if(PFD_AngularImpulses.GetEnabled())
	{
		// Set the angular impulse vector end point to make the arrow have a length of 1 m for an impulse that gives it an angular velocity change of 0.1 radian/s
		Vec3V end = position;
		end = AddScaled(	end,
							angImpulse,
							Vec3VFromF32(10.0f*Dot(angImpulse, GetInvAngInertia()).Getf() * FPInvSqrtSafe( MagSquared(angImpulse).Getf()) )
						);

		// Draw the arrow and end the drawing group.
		PFD_AngularImpulses.DrawArrow(RCC_VECTOR3(position), RCC_VECTOR3(end));
	}
#endif
}


void phCollider::DrawTurn (Vec3V_In PF_DRAW_ONLY(turn), Vec3V_In PF_DRAW_ONLY(position))
{
#if __PFDRAW
	DEBUG_COLLIDER("Turn ", RCC_VECTOR3(turn));
	// Try to begin the drawing group.
	if(PFD_Turns.GetEnabled())
	{
		// Draw the arrow and end the drawing group.
		Vec3V end = position;
		end = Add(end, turn);
		PFD_Turns.DrawArrow(RCC_VECTOR3(position), RCC_VECTOR3(end));
	}
#endif
}


void phCollider::UpdateImp (Vec::V3Param128 timeStep, Vec::V3Param128 gravity)
{
#if __DEV && !__SPU
	if (sm_DebugColliderUpdate == this)
	{
		__debugbreak();
	}
#endif

#if TRACK_COLLISION_TIME
	m_CollisionTime = 1.0f;
#endif

	// See if it's time to verify the orthonormality of the matrix.
	if (IncrementAndCheckRejuvenation())
	{
		// See if the matrix decayed from roundoff errors, and make it orthonormal again if it has.
		Rejuvenate();
	}

	// Update the velocity from the impulse and the angular velocity from the angular impulse, and clamp them.
	UpdateVelocityFromImpulse(timeStep);

	// Update the position and orientation from the velocity and angular velocity.
	UpdatePositionFromVelocity(timeStep);

	// Find and apply the gravity force.
	ApplyGravity(gravity, timeStep);

	// Find and apply damping forces.
	DampMotion(timeStep);

	// Update the velocity now so that the solver can react to it. Note that the position will not be affected by
	// this velocity until after the solver has had a look at it.
	UpdateVelocityFromExternal(timeStep);

	// Set the new matrix of the instance from the collider's matrix.
	SetInstanceMatrixFromCollider();

	ResetSolverInvMass();
	ResetSolverInvAngInertia();
	ResetRotationConstraintInvMass();
	ResetRotationConstraintInvAngInertia();
	ResetTranslationConstraintInvMass();
	ResetTranslationConstraintInvAngInertia();
	SetNeedsUpdateBeforeFinalIterations(false);
}

void phCollider::UpdatePositionFromVelocityImp (Vec::V3Param128 timeStep)
{
	// Move the collider's position from its velocity.
	Vec3V v_timeStep = RCC_VEC3V(timeStep);

	Vec3V setPos;

#if __SPU
	double timeStepD = INTRIN_TO_VEC3V(timeStep).GetXf();
	setPos.SetXf(float(double(m_Matrix.GetCol3().GetXf()) + double(m_Velocity.GetXf()) * timeStepD));
	setPos.SetYf(float(double(m_Matrix.GetCol3().GetYf()) + double(m_Velocity.GetYf()) * timeStepD));
	setPos.SetZf(float(double(m_Matrix.GetCol3().GetZf()) + double(m_Velocity.GetZf()) * timeStepD));
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
	Assertf(IsLessThanAll(Abs(setPos), Vec3V(V_FLT_LARGE_6)), "phCollider::UpdatePositionFromVelocityImp - setting matrix to a large number.  Curr mat pos = %f, %f, %f.  New mat pos  = %f, %f, %f",
		VEC3V_ARGS(m_Matrix.GetCol3()), 
		VEC3V_ARGS(setPos));
#endif
	m_Matrix.GetCol3Ref().SetXf(setPos.GetXf());
	m_Matrix.GetCol3Ref().SetYf(setPos.GetYf());
	m_Matrix.GetCol3Ref().SetZf(setPos.GetZf());
#else
	setPos = AddScaled(m_Matrix.GetCol3(), m_Velocity, v_timeStep);
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
	Assertf(IsLessThanAll(Abs(setPos), Vec3V(V_FLT_LARGE_6)), "phCollider::UpdatePositionFromVelocityImp - setting matrix to a large number.  Curr mat pos = %f, %f, %f.  New mat pos  = %f, %f, %f",
		VEC3V_ARGS(m_Matrix.GetCol3()), 
		VEC3V_ARGS(setPos));
#endif
	m_Matrix.SetCol3(setPos);
#endif

	// Clamp rotation BEFORE doing anything to position to make sure that nothing ever moves more in a frame than our expected limits
	ClampAngularVelocity();
	// Find the rotation and change the matrix orientation.
	Vec3V rotation(m_AngVelocity);
	rotation = Scale(rotation, v_timeStep);

#if COLLIDER_CLAMP_ROTATION_INSTEAD_OF_ANG_VELOCITY
	const ScalarV kvsRotationMagSq = MagSquared(rotation);
	// This is a fairly arbitrarily picked value but it happens to correspond to a single-frame rotation of about 81 degrees.
	const ScalarV vsMaxRotationMagSq(V_TWO);

	rotation = SelectFT(IsGreaterThan(kvsRotationMagSq, vsMaxRotationMagSq), rotation, NormalizeFast(rotation) * vsMaxRotationMagSq);
#endif

	RotateCollider(rotation.GetIntrin128());

	EARLY_FORCE_SOLVE_ONLY(SetInstanceMatrixFromCollider();)

	EARLY_FORCE_SOLVE_ONLY(m_NeedsCollision = true;)
}

void phCollider::UpdatePositionFromVelocityIncludingForce (Vec::V3Param128 timeStep)
{
	ScalarV v_timeStep(timeStep);
	/// ScalarV v_InvMass = GetInvMassV();

	// Find the velocity change resulting from the force.
	/// Vec3V forceVelocity = Scale( m_Force, Scale(v_InvMass, v_timeStep) );
	/// Vec3V totalVelocity = m_Velocity + forceVelocity;

	Vec3V setPos(AddScaled(m_Matrix.GetCol3(), m_Velocity, v_timeStep));
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
	Assertf(IsLessThanAll(Abs(setPos), Vec3V(V_FLT_LARGE_6)), "phCollider::UpdatePositionFromVelocityIncludingForce - setting matrix to a large number.  Curr mat pos = %f, %f, %f.  New mat pos  = %f, %f, %f",
		VEC3V_ARGS(m_Matrix.GetCol3()), 
		VEC3V_ARGS(setPos));
#endif
	// Move the collider's position from its velocity.
	m_Matrix.SetCol3( setPos );

	Vec3V delAngMomentum = m_Torque;
	delAngMomentum = Scale(delAngMomentum, v_timeStep);

	// Find the change in angular velocity from the change in angular momentum.
	Vec3V delAngVel = UnTransform3x3Ortho( m_Matrix, delAngMomentum );
	delAngVel = Scale( delAngVel, GetInvAngInertia() );
	delAngVel = Transform3x3( m_Matrix, delAngVel );

	// Find the rotation and change the matrix orientation.
	Vec3V rotation(m_AngVelocity);
	rotation = Scale(rotation, v_timeStep);
	Vec3V totalAngVelocity = delAngVel + rotation;
	RotateCollider(totalAngVelocity.GetIntrin128());
}


void phCollider::UpdateSleep (float timeStep)
{
	Assert(m_Sleep);
	m_Sleep->Update(timeStep);
}


void phCollider::RotateCollider (Vec::V3Param128 rotation)
{
	Vec3V v_rotation(rotation);

	ScalarV v_verysmallfloat = ScalarV(V_FLT_SMALL_12);
	ScalarV rotMag2 = MagSquared( v_rotation );
	if ( IsGreaterThanAll(rotMag2, v_verysmallfloat) != 0 )
	{
		// Calculate the angular momentum, it stays consistent during the rotation
		Vec3V angMomentum = CalculateAngMomentumRigid();

		// Rotate the current rotation matrix
		ScalarV mag = ScalarV( Sqrt( rotMag2 ) );
		Vec3V unitRotation = InvScale(v_rotation, mag);	

		Mat34V matRotate;
		Mat34VFromAxisAngle(matRotate, unitRotation, mag);

		Transform3x3(m_Matrix, matRotate, m_Matrix);

		// Calculate the new angular velocity using the current angular momentum and new rotation matrix
		Mat33V invInertiaMtx;
		GetInverseInertiaMatrixRigid(invInertiaMtx);
		SetAngVelocity(Multiply(invInertiaMtx, angMomentum).GetIntrin128());

		// Clamp the angular velocity (don't let things rotate too quickly).
		ClampAngularVelocity();
	}
}

#if EARLY_FORCE_SOLVE
float g_PushCollisionTolerance = 0.04f;
float g_TurnCollisionTolerance = 0.01f;

float g_PushRadiasBias = 0.1f;
float g_TurnRadiasBias = 0.1f;

#if __PFDRAW
extern int g_PushCollisionCount;
extern int g_TurnCollisionCount;
#endif

#if __BANK
extern bool g_PushCollisionTTY;
#endif // __BANK
#endif // EARLY_FORCE_SOLVE

#if __BANK && !__PS3
extern bool g_AlwaysPushCollisions;
#endif // __BANK && !__PS3

void phCollider::UpdateFromPushAndTurn ()
{
	ScalarV v_verysmallfloat = ScalarV(V_FLT_SMALL_12);

#if EARLY_FORCE_SOLVE
#if __BANK && !__PS3
	m_NeedsCollision = g_AlwaysPushCollisions;
#else // __BANK && !__PS3
	m_NeedsCollision = false;
#endif // __BANK && !__PS3
#endif // EARLY_FORCE_SOLVE

	ScalarV objectRadius(m_ApproximateRadius);

	// Move from the push (not from the velocity).
	ScalarV pushMag2 = MagSquared( m_Push );
	if ( IsGreaterThanAll( pushMag2, v_verysmallfloat ) )
	{
		Vec3V setPos;
#if __SPU
		setPos.SetXf(float(double(m_Matrix.GetCol3().GetXf()) + double(m_Push.GetXf())));
		setPos.SetYf(float(double(m_Matrix.GetCol3().GetYf()) + double(m_Push.GetYf())));
		setPos.SetZf(float(double(m_Matrix.GetCol3().GetZf()) + double(m_Push.GetZf())));
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
		Assertf(IsLessThanAll(Abs(setPos), Vec3V(V_FLT_LARGE_6)), "phCollider::UpdateFromPushAndTurn - setting matrix to a large number.  Curr mat pos = %f, %f, %f.  New mat pos  = %f, %f, %f",
			VEC3V_ARGS(m_Matrix.GetCol3()), 
			VEC3V_ARGS(setPos));
#endif
		m_Matrix.GetCol3Ref().SetXf(setPos.GetXf());
		m_Matrix.GetCol3Ref().SetYf(setPos.GetYf());
		m_Matrix.GetCol3Ref().SetZf(setPos.GetZf());
#else
		setPos = m_Matrix.GetCol3() + m_Push;
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
		Assertf(IsLessThanAll(Abs(setPos), Vec3V(V_FLT_LARGE_6)), "phCollider::UpdateFromPushAndTurn - setting matrix to a large number.  Curr mat pos = %f, %f, %f.  New mat pos  = %f, %f, %f",
			VEC3V_ARGS(m_Matrix.GetCol3()), 
			VEC3V_ARGS(setPos));
#endif
		m_Matrix.SetCol3(setPos);
#endif

		m_AppliedPush = m_Push;
		m_Push = Vec3V(V_ZERO);

#if EARLY_FORCE_SOLVE
		ScalarV pushTolerance2 = ScalarV(g_PushCollisionTolerance) + ScalarV(g_PushRadiasBias) * objectRadius;
		pushTolerance2 *= pushTolerance2;
		if (IsGreaterThanAll(pushMag2, pushTolerance2))
		{
#if __BANK
			if (g_PushCollisionTTY && m_CausesPushCollisions)
			{
				Displayf("Push %f greater than tolerance %f for %s",
					sqrtf(pushMag2.Getf()),
					sqrtf(pushTolerance2.Getf()),
					m_Instance->GetArchetype()->GetFilename());
			}
#endif // __BANK

			m_NeedsCollision = true;
			PF_DRAW_ONLY(g_PushCollisionCount++;)

			SetCurrentlyPenetrating();
		}
#endif // EARLY_FORCE_SOLVE
	}
	else
	{
		m_AppliedPush = Vec3V(V_ZERO);
	}
	
	// Find the turn magnitude and see if it is not nearly zero.
	Vec3V v_turn = m_Turn;
	ScalarV turnMag2 = MagSquared( v_turn );
	if ( IsGreaterThanAll( turnMag2, v_verysmallfloat) )
	{
		// Save the final applied turn per frame for use by phSleep.
		m_AppliedTurn = m_Turn;

		// The turn is not nearly zero, so rotate the collider's matrix.
		ScalarV mag = Sqrt(turnMag2);
		Vec3V turnAxis = InvScale( m_Turn, mag );
		Mat34V rotateMatrix;
		Mat34VFromAxisAngle(rotateMatrix, turnAxis, mag);
		Transform3x3(m_Matrix, rotateMatrix, m_Matrix);

		// Reset the turn
		m_Turn = Vec3V(V_ZERO);

#if EARLY_FORCE_SOLVE
		ScalarV turnTolerance2 = ScalarV(g_TurnCollisionTolerance) + ScalarV(g_TurnRadiasBias) * objectRadius;
		turnTolerance2 *= turnTolerance2;
		if (IsGreaterThanAll(turnMag2, turnTolerance2))
		{
#if __BANK
			if (g_PushCollisionTTY && m_CausesPushCollisions)
			{
				Displayf("Turn %f greater than tolerance %f for %s",
					sqrtf(turnMag2.Getf()),
					sqrtf(turnTolerance2.Getf()),
					m_Instance->GetArchetype()->GetFilename());
			}
#endif // __BANK

			m_NeedsCollision = true;
			PF_DRAW_ONLY(g_TurnCollisionCount++;)
			
			SetCurrentlyPenetrating();
		}
#endif // EARLY_FORCE_SOLVE
	}
	else
	{
		// No turn was applied. Any nearly-zero turn will be saved for next time.
		m_AppliedTurn = Vec3V(V_ZERO);
	}

	if (!m_CausesPushCollisions)
	{
		m_NeedsCollision = false;
	}
}

// stop using original LastSafeMatrix after a certain number of frames
	bank_u8 s_MaxPenetratingFrames = 8;
//
void phCollider::UpdateLastMatrixFromCurrent (bool updateLastSafeMatrixToo)
{
	// Remember the instance matrix just computed, as a starting point for next frame's continuous collisions
	Mat34V instanceMatrix;
#if __SPU && !EARLY_FORCE_SOLVE
	static const int tag = 1;
	sysDmaGet(&instanceMatrix, (uint64_t)&m_Instance->GetMatrix(), sizeof(Matrix34), tag);
	sysDmaWaitTagStatusAll(1<<tag);
#else
	instanceMatrix = m_Instance->GetMatrix();
#endif

	m_LastInstanceMatrix = instanceMatrix;

	if (!updateLastSafeMatrixToo)
	{
		return;
	}

	bool inTheClear = false;
	if (!m_CurrentlyPenetrating)
	{
		m_LastSafeInstanceMatrix = instanceMatrix;
		m_NonPenetratingAfterTeleport = true;
		inTheClear = true;
	}
	else if(!m_NonPenetratingAfterTeleport || m_CurrentlyPenetratingCount > s_MaxPenetratingFrames)
	{
#if __DEV
		if (m_CurrentlyPenetratingCount > s_MaxPenetratingFrames)
			Displayf("Physics Warning: Collider Max Penetrating Frame Count Exceeded! Reseting The Safe Last Matrix!");
#endif
		m_LastSafeInstanceMatrix = instanceMatrix;
		inTheClear = true;
	}

	if (m_CurrentlyPenetrating)
	{
		m_CurrentlyPenetrating = false;
	}
	else
	{
		m_CurrentlyPenetratingCount = 0;
	}

	if (IsArticulated() && inTheClear)
	{
		phBound* bound = GetInstance()->GetArchetype()->GetBound();
		Assert(bound->GetType() == phBound::COMPOSITE);
		static_cast<phBoundComposite*>(bound)->UpdateLastMatricesFromCurrent();
		static_cast<phArticulatedCollider*>(this)->DecrementSelfCollisionTemporaryFrames();
	}
}

void phCollider::MoveImp (Vec::V3Param128 UNUSED_PARAM(timeStep), bool UNUSED_PARAM(usePushes))
{
	//EARLY_FORCE_SOLVE_ONLY(UpdateLastMatrixFromCurrent();)

	// Use the push and turn to translate and rotate the collider's matrix.
	UpdateFromPushAndTurn();

	// Set the instance matrix from the collider's new matrix.
	SetInstanceMatrixFromCollider();

	NOT_EARLY_FORCE_SOLVE_ONLY(UpdateLastMatrixFromCurrent();)
}


void phCollider::UpdateVelocityImp (Vec::V3Param128 timeStep)
{
	// Add the change in velocity from impulses to the velocity
	Vec3V deltaVelocity = m_Impulse * GetInvMassV();
	Vec3V newVelocity = m_Velocity + deltaVelocity;
	newVelocity = ClampVelocity(newVelocity.GetIntrin128());

	// Record the velocity before we add in forces
	m_VelocityBeforeForce = newVelocity;

	// Add the change in angular velocity from angular impulses the angular velocity
	Mat33V inverseInertiaMtx;
	GetInverseInertiaMatrix(inverseInertiaMtx);
	Vec3V deltaAngVelocity = Multiply(inverseInertiaMtx, m_AngImpulse);
	Vec3V newAngVelocity = m_AngVelocity + deltaAngVelocity;
	newAngVelocity = ClampAngularVelocity(newAngVelocity.GetIntrin128());

	// Record the angular velocity before we add in forces
	m_AngVelocityBeforeForce = newAngVelocity;

	// Add the force applied over the given time step to the impulse.
	Vec3V forceImpulse = m_Force * ScalarV(timeStep);
	Vec3V forceDeltaVelocity = forceImpulse * GetInvMassV();
	newVelocity = newVelocity + forceDeltaVelocity;
	newVelocity = ClampVelocity(newVelocity.GetIntrin128());

	// Record the new actual linear velocity
	Assert(IsFiniteAll(newVelocity));
	m_Velocity = newVelocity;
	PDR_ONLY(debugPlayback::RecordSetVelocity(*GetInstance(), m_Velocity.GetIntrin128()));

	// Add the torque applied over the given time step to the angular impulse.
	Vec3V forceAngImpulse = m_Torque * ScalarV(timeStep);
	Vec3V forceDeltaAngVelocity = Multiply(inverseInertiaMtx, forceAngImpulse);
	newAngVelocity = newAngVelocity + forceDeltaAngVelocity;
	newAngVelocity = ClampAngularVelocity(newAngVelocity.GetIntrin128());

	// Record the new actual angular velocity
	Assert(IsFiniteAll(newAngVelocity));
	m_AngVelocity = newAngVelocity;
	PDR_ONLY(debugPlayback::RecordSetAngularVelocity(*GetInstance(), m_AngVelocity.GetIntrin128()));

	// Reset the impulses and forces.
	m_Impulse = m_AngImpulse = m_Force = m_Torque = Vec3V(V_ZERO);

	// Have to zero these out somewhere since we don't always call Move any more
	m_Turn = m_AppliedTurn = m_AppliedPush = Vec3V(V_ZERO);
}


void phCollider::UpdateVelocityFromExternalImp (Vec::V3Param128 timeStep)
{
	phCollider::UpdateVelocityImp(timeStep);
}


void phCollider::UpdateVelocityFromImpulseImp (Vec::V3Param128 UNUSED_PARAM(timeStep))
{
	if ( IsZeroAll(m_Impulse) == 0 )
	{
		// Apply the impulse to get the new momentum, and find and clamp the new velocity.
		AddMomentum(m_Impulse.GetIntrin128());

		// Reset the impulse.
		m_Impulse = Vec3V(V_ZERO);
	}

	if ( IsZeroAll(m_AngImpulse) == 0 )
	{
		// Apply the angular impulse to get the new angular momentum, and find and clamp the new angular velocity.
		AddAngMomentum(m_AngImpulse.GetIntrin128());

		// Reset the angular impulse.
		m_AngImpulse = Vec3V(V_ZERO);
	}
}


#if __ASSERT
void phCollider::ValidateInstanceMatrixAlignedWithCollider(ScalarV_In tolerance) const
{
	Mat34V currentInstanceMatrix = m_Instance->GetMatrix();
	Mat34V expectedInstanceMatrix;
	ComputeInstanceMatrixFromCollider(expectedInstanceMatrix);
	Assertf(IsCloseAll(currentInstanceMatrix,expectedInstanceMatrix,Vec3V(tolerance)),	"Instance matrix on '%s' is not lined up with collider matrix"
		"\nActual Instance Matrix:"
		"\n\t %5.2f, %5.2f, %5.2f, %5.2f"
		"\n\t %5.2f, %5.2f, %5.2f, %5.2f"
		"\n\t %5.2f, %5.2f, %5.2f, %5.2f"
		"\nExpected Instance Matrix:"
		"\n\t %5.2f, %5.2f, %5.2f, %5.2f"
		"\n\t %5.2f, %5.2f, %5.2f, %5.2f"
		"\n\t %5.2f, %5.2f, %5.2f, %5.2f",
		m_Instance->GetArchetype()->GetFilename(),
		MAT34V_ARG_FLOAT_RC(currentInstanceMatrix),MAT34V_ARG_FLOAT_RC(expectedInstanceMatrix));
}
#endif // __ASSERT

void phCollider::ComputeInstanceMatrixFromCollider(Mat34V_InOut instanceMtx) const
{
	// Get the matrix.
	instanceMtx = m_Matrix;
	if (GetType() == 1 && static_cast<const phArticulatedCollider*>(this)->IsUsingInstanceToArtColliderOffsetMat())
	{
		Matrix34 tempMat, tempMat2;
		tempMat.Transpose3x4(static_cast<const phArticulatedCollider*>(this)->GetOffsetMatrix());
		tempMat2.Dot(tempMat, MAT34V_TO_MATRIX34(m_Matrix));
		instanceMtx = MATRIX34_TO_MAT34V(tempMat2);
	}


	// See if the bound has a center of gravity offset.
#if __SPU
	Vector3 cgOffset;
	
	if ((int)m_Instance < 256*1024)
	{
		// If the instance is in local store already, the archetype will be too
		Assert((int)m_Instance->GetArchetype() < 256*1024);
		phBound* boundMm = m_Instance->GetArchetype()->GetBound();
		sysDmaGet(&cgOffset, (uint64_t)boundMm->GetCGOffsetPtr(), sizeof(Vector3), DMA_TAG(10));
	}
	else
	{
		// The instance is in main memory, so get the archetype and the cg offset from there
		phArchetype* archMm = NULL;
		archMm = (phArchetype*)sysDmaGetUInt32((uint64_t)m_Instance->GetArchetypePtr(), DMA_TAG(10));
		phBound* boundMm = (phBound*)sysDmaGetUInt32((uint64_t)archMm->GetBoundPtr(), DMA_TAG(10));
		sysDmaGet(&cgOffset, (uint64_t)boundMm->GetCGOffsetPtr(), sizeof(Vector3), DMA_TAG(10));
	}
	sysDmaWaitTagStatusAll(DMA_MASK(10));
#else
	FastAssert(GetInstance());
	FastAssert(GetInstance()->GetArchetype());
	FastAssert(GetInstance()->GetArchetype()->GetBound());
	const phBound& bound = *GetInstance()->GetArchetype()->GetBound();
	Vector3 cgOffset = VEC3V_TO_VECTOR3(bound.GetCGOffset());
#endif
	// The bound's center of gravity offset is the position of the collider in the instance's coordinate
	// system. Translate the instance matrix from the matrix by the negated center of gravity offset
	// in world coordinates, to get the correct instance location.
	Vector3 cgWorldOffset;
	RCC_MATRIX34(instanceMtx).Transform3x3(cgOffset,cgWorldOffset);
	cgWorldOffset.Negate();
	RC_MATRIX34(instanceMtx).Translate(cgWorldOffset);
}


void phCollider::SetInstanceMatrixFromCollider ()
{
	Mat34V instanceMtx;
	ComputeInstanceMatrixFromCollider(instanceMtx);

	// Set the instance's updated matrix.
#if __SPU
	if ((int)m_Instance < 256*1024)
	{
		m_Instance->SetMatrix(instanceMtx);
	}
	else
	{
		sysDmaPut(&instanceMtx, (uint64_t)&m_Instance->GetMatrix(), sizeof(Matrix34), DMA_TAG(10));
		sysDmaWaitTagStatusAll(DMA_MASK(10));
	}
#else
	m_Instance->SetMatrix(instanceMtx);
#endif
}

void phCollider::SetColliderMatrixFromInstanceRigid ()
{
	// Copy the instance's current matrix into the collider.
	Assert(m_Instance);
	SPU_ONLY(Assert((u32)m_Instance < 256 * 1024));
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
	Assertf(IsLessThanAll(Abs(m_Instance->GetMatrix().GetCol3()), Vec3V(V_FLT_LARGE_6)), "phCollider::SetColliderMatrixFromInstanceRigid - setting matrix to a large number.  Curr mat pos = %f, %f, %f.  New mat pos  = %f, %f, %f",
		VEC3V_ARGS(m_Matrix.GetCol3()), 
		VEC3V_ARGS(m_Instance->GetMatrix().GetCol3()));
#endif
	m_Matrix = m_Instance->GetMatrix();

	if (GetType() == 1 && static_cast<phArticulatedCollider*>(this)->IsUsingInstanceToArtColliderOffsetMat())
	{
		Matrix34 newColliderMat;
		newColliderMat.Dot(static_cast<phArticulatedCollider*>(this)->GetOffsetMatrix(), MAT34V_TO_MATRIX34(m_Instance->GetMatrix()));
		m_Matrix = MATRIX34_TO_MAT34V(newColliderMat);
	}

	// See if this object has a center-of-gravity offset, which means that the instance and the collider are not at the same location.

#if __SPU
	Vector3 cgOffset;

	if ((int)m_Instance < 256*1024)
	{
		// If the instance is in local store already, the archetype will be too
		Assert((int)m_Instance->GetArchetype() < 256*1024);
		phBound* boundMm = m_Instance->GetArchetype()->GetBound();
		sysDmaGet(&cgOffset, (uint64_t)boundMm->GetCGOffsetPtr(), sizeof(Vector3), DMA_TAG(10));
	}
	else
	{
		// The instance is in main memory, so get the archetype and the cg offset from there
		phArchetype* archMm = NULL;
		archMm = (phArchetype*)sysDmaGetUInt32((uint64_t)m_Instance->GetArchetypePtr(), DMA_TAG(10));
		phBound* boundMm = (phBound*)sysDmaGetUInt32((uint64_t)archMm->GetBoundPtr(), DMA_TAG(10));
		sysDmaGet(&cgOffset, (uint64_t)boundMm->GetCGOffsetPtr(), sizeof(Vector3), DMA_TAG(10));
	}
	sysDmaWaitTagStatusAll(DMA_MASK(10));
#else
	FastAssert(GetInstance());
	FastAssert(GetInstance()->GetArchetype());
	FastAssert(GetInstance()->GetArchetype()->GetBound());
	const phBound& bound = *GetInstance()->GetArchetype()->GetBound();
	Vector3 cgOffset = VEC3V_TO_VECTOR3(bound.GetCGOffset());
#endif
	// The bound has a center of gravity offset, so translate the collider's matrix to the center of gravity.
	Vec3V cgGlobalPos = Transform( m_Matrix, RCC_VEC3V(cgOffset) );
	m_Matrix.SetCol3( cgGlobalPos );
}

float phCollider::GetTotalInternalMotionRigid() const
{
	return 0.0f;
}


void phCollider::GetInertiaMatrix (Mat33V_InOut inertia, Vec::V3Param128 offset) const
{
	GetInertiaMatrix(inertia);
	ScalarV v_mass = GetMassV();

	Mat33V tempInertia = inertia;
	Mat33V offsetInertia;
	Vec3V v_offset = RCC_VEC3V(offset);
	Vec3V v_offset2 = Scale(v_offset, v_offset);

	// v_offset2Sums = { x^2+z^2, y^2+x^2m z^2+y^2 }
	Vec3V v_permutedOffset2s = v_offset2.Get<Vec::Z, Vec::X, Vec::Y>();
	Vec3V v_offset2Sums = Add(v_offset2, v_permutedOffset2s);

	// v_negatedScaledOffsets = { -x*y, -y*z, -z*x }
	Vec3V v_zero(V_ZERO);
	Vec3V v_permutedOffsets = v_offset.Get<Vec::Y, Vec::Z, Vec::X>();
	Vec3V v_negatedScaledOffsets = SubtractScaled( v_zero, v_offset, v_permutedOffsets );

	Vec3V col0 = AddScaled( tempInertia.GetCol0(), GetFromTwo<Vec::X1, Vec::X2, Vec::Z2>( v_offset2Sums, v_negatedScaledOffsets ), v_mass );
	Vec3V col1 = AddScaled( tempInertia.GetCol1(), GetFromTwo<Vec::X2, Vec::Z1, Vec::Y2>( v_offset2Sums, v_negatedScaledOffsets ), v_mass );
	Vec3V col2 = AddScaled( tempInertia.GetCol2(), GetFromTwo<Vec::Z2, Vec::Y2, Vec::Y1>( v_offset2Sums, v_negatedScaledOffsets ), v_mass );

	inertia.SetCols( col0, col1, col2 );
}

void phCollider::GetInverseInertiaMatrixRigid (Mat33V_InOut invInertia) const
{
	phMathInertia::GetInverseInertiaMatrix(m_Matrix.GetMat33(),GetInvAngInertia().GetIntrin128(),invInertia);
}


Vec3V_Out phCollider::ComputeInertiaAboutAxis (Vec::V3Param128 worldAxis) const
{
	// Compute the inverse inertia matrix.
	Mat33V inertia;
	GetInertiaMatrix(inertia);

	// Compute the inverse inertia for rotations about the given axis.
	Vec3V inertiaAboutAxis = Multiply( inertia, Vec3V(worldAxis) );
	return inertiaAboutAxis;
}


Vec3V_Out phCollider::ComputeInverseInertiaAboutAxis (Vec::V3Param128 worldAxis) const
{
	// Compute the inverse inertia matrix.
	Mat33V inverseInertia;
	GetInverseInertiaMatrix(inverseInertia);

	// Compute the inverse inertia for rotations about the given axis.
	Vec3V invInertiaAboutAxis = Multiply( inverseInertia, Vec3V(worldAxis) );
	return invInertiaAboutAxis;
}


void phCollider::GetInvTorqueMassMatrix (Mat33V_InOut outMtx, Vec::V3Param128 position) const
{
	Mat33V outTempMat;
	GetInverseInertiaMatrix( outTempMat );
	Vec3V relPos(position);
	relPos = Subtract(relPos, m_Matrix.GetCol3());

	DotCrossProduct( outTempMat, outTempMat, relPos );
	outMtx = outTempMat;
}

void phCollider::GetInvMassMatrixRigid (Mat33V_InOut invMassMatrix, Vec::V3Param128 sourcePos, const Vec3V* responsePos) const
{
	Mat33V tempInvMassMatrix;

	// Get the relative response position.
	Vec3V relPos = (responsePos ? *responsePos : Vec3V(sourcePos));
	relPos = Subtract(relPos, m_Matrix.GetCol3());

	// Dot the matrix with the cross product matrix of the response position.
	Mat33V dotCross = m_Matrix.GetMat33();
	DotCrossProduct( dotCross, dotCross, relPos );

	// Dot the inverse angular inertia with the previous matrix.
	Mat33V tranInvAngInertia;
	tranInvAngInertia.SetCol0( Scale(dotCross.GetCol0(), GetInvAngInertia().GetX()) );
	tranInvAngInertia.SetCol1( Scale(dotCross.GetCol1(), GetInvAngInertia().GetY()) );
	tranInvAngInertia.SetCol2( Scale(dotCross.GetCol2(), GetInvAngInertia().GetZ()) );

	if (responsePos)
	{
		// Get the relative impulse application position.
		relPos = Subtract(Vec3V(sourcePos), m_Matrix.GetCol3());

		// Dot the matrix with the cross product matrix of the application position.
		dotCross = m_Matrix.GetMat33();
		DotCrossProduct( dotCross, dotCross, relPos );
	}

	// Transpose the previous matrix and dot it with the transformed inverse angular inertia.
	Transpose(dotCross, dotCross);
	Multiply(tempInvMassMatrix, tranInvAngInertia, dotCross);

	// Add the inverse mass to the diagonal elements and zero the position vector.
	Vec3V v_InvMass = Vec3V(GetInvMassV());
	Mat33V invMassIdent(V_IDENTITY);
	AddScaled( tempInvMassMatrix, tempInvMassMatrix, invMassIdent, v_InvMass );
	
	invMassMatrix = tempInvMassMatrix;
}


void phCollider::GetInvMassMatrixTranslation (Mat33V_InOut invMassMatrixTranslation, Vec::V3Param128 UNUSED_PARAM(sourceNormal)) const
{
	Scale(invMassMatrixTranslation,Mat33V(V_IDENTITY),Vec3V(GetInvMassV()));
}



Vec3V_Out phCollider::GetLocalVelocityRigid (Vec::V3Param128 position) const
{
	Vec3V velocity;

	velocity = Subtract(Vec3V(position), m_Matrix.GetCol3());
	velocity = Cross(m_AngVelocity, velocity);
	velocity = Add(velocity, m_Velocity);

	return velocity;
}


Vec3V_Out phCollider::GetLocalAcceleration (Vec::V3Param128 position) const
{
	Vec3V tempAccel;
	ScalarV v_InvMass = GetInvMassV();

	// Get the relative position.
	Vec3V relPos(position);
	relPos = Subtract(relPos, m_Matrix.GetCol3());

	// Find the centripetal acceleration (AngVelocity cross (AngVelocity cross relPos)).
	tempAccel = Cross(m_AngVelocity, relPos);
	tempAccel = Cross(m_AngVelocity, tempAccel);

	// Add the acceleration from the force.
	tempAccel = AddScaled(tempAccel, m_Force, v_InvMass);

	// Add the acceleration from torque and wobble.
	Vec3V wobble = CalculateAngMomentum();
	wobble = AddCrossed(m_Torque, wobble, m_AngVelocity);
	Vec3V localAngAccel;
	localAngAccel = UnTransform3x3Ortho( m_Matrix, wobble );
	localAngAccel = Scale(localAngAccel, GetInvAngInertia());
	wobble = Transform3x3( m_Matrix, localAngAccel );

	tempAccel = AddCrossed( tempAccel, wobble, relPos );

	return tempAccel;
}


void phCollider::ApplyGravityImp (Vec::V3Param128 gravity, Vec::V3Param128 timeStep)
{
	if (!m_Instance->GetInstFlag(phInst::FLAG_NO_GRAVITY))
	{
		// Make sure this collider has an instance with an archetype. Failures tend to happen here because it's the
		// first collider access in the simulator update after the game managers update.
		AssertMsg(m_Instance , "A collider has no instance.");
		AssertMsg(m_Instance->GetArchetype() , "An instance in a collider has no archetype.");
		Vec3V scaledGravity(gravity);
		scaledGravity = Scale(scaledGravity, ScalarVFromF32(GetGravityFactor()));
		ApplyAccel(scaledGravity.GetIntrin128(), timeStep);
	}
}

void phCollider::DampMotionImp (Vec::V3Param128 timeStep)
{
	// Get the physics archetype and see if it is damped (a phArchetypeDamp).
	Assert(m_Instance && m_Instance->GetArchetype());
	const phArchetype* archetype = m_Instance->GetArchetype();
	if (archetype->IsDamped() && IsDampingEnabled())
	{
		// This is a damped archetype, so apply damping to the object's motion.
		const phArchetypeDamp* dampedArch = static_cast<const phArchetypeDamp*>(archetype);

		// Get the velocity.

		Vec3V localVel = UnTransform3x3Ortho( m_Matrix, m_Velocity - GetReferenceFrameVelocity()); //Compute damping force based on relative velocity instead of exact velocity

		// Get the linear damping parameters and compute the linear damping acceleration.
		Vec3V invTimeStep = InvertSafe( Vec3V(timeStep) );
		Vec3V dampC = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::LINEAR_C));
		Vec3V dampV = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::LINEAR_V));
		Vec3V dampV2 = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::LINEAR_V2));
		Vec3V force( ComputeDampingAccel(localVel.GetIntrin128(),timeStep,invTimeStep.GetIntrin128(),dampC.GetIntrin128(),dampV.GetIntrin128(),dampV2.GetIntrin128()) );

		// Make the force proportional to the mass.
		force = Scale(force, GetMassV());

		// Put the force in world coordinates and apply it to the collider.
		force = Transform3x3( m_Matrix, force );
		ApplyForceCenterOfMass(force.GetIntrin128(), timeStep);

		// Get the angular velocity.
		Vec3V localAngVel = UnTransform3x3Ortho( m_Matrix, m_AngVelocity - GetReferenceFrameAngularVelocity());

		// Get the angular damping parameters and compute the angular damping acceleration.
		dampC = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::ANGULAR_C));
		dampV = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::ANGULAR_V));
		dampV2 = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::ANGULAR_V2));
		Vec3V torque( ComputeDampingAccel(localAngVel.GetIntrin128(),timeStep,invTimeStep.GetIntrin128(),dampC.GetIntrin128(),dampV.GetIntrin128(),dampV2.GetIntrin128()) );

		// Make the torque proportional to the angular inertia.
		torque = Scale(torque, GetAngInertia());

		// Put the torque in world coordinates and apply it to the collider.
		torque = Transform3x3( m_Matrix, torque );
		ApplyTorque(torque.GetIntrin128(), timeStep);
	}

	// Damp based on the game-controlled damping rate
	DampReferenceFrameVelocities(timeStep);
}


void phCollider::ApplyTorqueAndForce (Vec::V3Param128 torque, Vec::V3Param128 worldPosition, Vec::V3Param128 timestep)
{
	// Apply the given torque about the center of mass.
	ApplyTorque(torque, timestep);

	// Get the inverse inertia matrix, to find the acceleration at the given world position resulting from the torque.
	Mat33V invInertiaMatrix;
	GetInverseInertiaMatrix(invInertiaMatrix);

	// Find the angular acceleration resulting from the torque.
	Vec3V force = Multiply( invInertiaMatrix, Vec3V(torque) );

	// Find the offset from the position to the given world position.
	Vec3V offset = Subtract(Vec3V(worldPosition), m_Matrix.GetCol3());

	// Find and apply the force needed to keep the given world position from accelerating from the torque.
	ScalarV v_Mass = GetMassV();
	force = Cross(force, offset);
	force = Scale(force, -v_Mass);
	ApplyForceCenterOfMass(force.GetIntrin128(), timestep);
}


void phCollider::ApplyImpulseChangeMotion (Vec::V3Param128 impulse, Vec::V3Param128 worldPosition, int UNUSED_PARAM(component))
{
	Vec3V tempImpulse(impulse);
	Vec3V tempWorldPosition(worldPosition);

	// Add the impulse to the momentum, and use it to find the new velocity.
	AddMomentum(tempImpulse.GetIntrin128());

	// Find the angular impulse and add it to the angular momentum.
	Vec3V angImpulse = tempWorldPosition;
	angImpulse = Subtract(angImpulse, m_Matrix.GetCol3());
	angImpulse = Cross(angImpulse, tempImpulse);

	// Find and clamp the new angular velocity.
	AddAngMomentum(angImpulse.GetIntrin128());

	DrawImpulse(tempImpulse, tempWorldPosition);
}


void phCollider::ApplyAirResistance(Vec::V3Param128 timestep, float airDensity, Vec::V3Param128 windVelocity, const phBound* bound)
{
	const phBound& useBound = (bound ? *bound : *GetInstance()->GetArchetype()->GetBound());
	Vec3V airVelocity, force, position;
	ScalarV dot;
	int i,j;
	switch (useBound.GetType())
	{
		case phBound::GEOMETRY:
		USE_GEOMETRY_CURVED_ONLY(case phBound::GEOMETRY_CURVED:)
		case phBound::BOX:
		{
			const phBoundPolyhedron& polyhedronBound = *static_cast<const phBoundPolyhedron*>(&useBound);
			for(i=polyhedronBound.GetNumPolygons()-1;i>=0;i--)
			{
				const phPolygon& polygon = polyhedronBound.GetPolygon(i);
				airVelocity = Vec3V(V_ZERO);		// using airVelocity as a temp here
				for(j=POLY_MAX_VERTICES - 1;j>=0;j--)
				{
					airVelocity = Add(airVelocity, polyhedronBound.GetVertex(polygon.GetVertexIndex(j)));
				}
				float fNumVerts = (float)POLY_MAX_VERTICES;
				airVelocity = InvScale(airVelocity, Vec3VFromF32(fNumVerts));
				position = Transform(m_Matrix, airVelocity);
				airVelocity = GetLocalVelocity(position.GetIntrin128(), 0);
				airVelocity = Subtract(airVelocity, Vec3V(windVelocity));
				Vec3V polyUnitNormal = polyhedronBound.GetPolygonUnitNormal(i);
				dot = Dot(airVelocity, polyUnitNormal);
				if( IsLessThanOrEqualAll(dot, ScalarV(V_ZERO)) != 0 )
				{
					continue;
				}
				force = Negate(polyUnitNormal);
				float fProd = polygon.GetArea()*airDensity;
				force = Scale(force, Scale( Vec3V(Scale(dot,dot)), Vec3VFromF32(fProd) ));
				ApplyForce(force.GetIntrin128(), position.GetIntrin128(), timestep);
			}
			break;
		}
		case phBound::SPHERE:
		{
			// @@@@@ NOT YET IMPLEMENTED @@@@@
			break;
		}

		case phBound::CAPSULE:
		{
			// @@@@@ NOT YET IMPLEMENTED @@@@@
			break;
		}

		case phBound::COMPOSITE:
		{
			const phBoundComposite& compositeBound = *static_cast<const phBoundComposite*>(&useBound);
			for(i=compositeBound.GetNumBounds()-1;i>=0;i--)
			{
				ApplyAirResistance(timestep, airDensity,windVelocity,compositeBound.GetBound(i));
			}
			break;
		}
	}
}


void phCollider::ApplyPushAndTurn (Vec::V3Param128 push, Vec::V3Param128 worldPosition, Vec::V3Param128 worldRotationCenter)
{
	// TODO: Convert this.
	// This could potentially be a huge savings. It seems like the if's are usually taken, so we woudln't lose much (on average) by vectorizing
	// everything and using vsel at the end.

	// Get the push position and the destination of the push position relative to the rotation center.
	Vector3 relPosition(worldPosition);
	relPosition.Subtract(Vector3(worldRotationCenter));
	float relPosMag2 = relPosition.Mag2();
	if (relPosMag2>SMALL_FLOAT)
	{
		Vector3 relDestination(relPosition);
		relDestination.Add(Vector3(push));
		float relDestMag2 = relDestination.Mag2();
		if (relDestMag2>SMALL_FLOAT)
		{
			// Get the axis about which to make a turn, scaled by the sine of the turn angle.
			Vector3 turn(relPosition);
			turn.Cross(relDestination);
			turn.InvScale(sqrtf(relPosMag2*relDestMag2));

			// Get the square of the sine of the turn angle.
			float sine2 = turn.Mag2();
			if (sine2>SMALL_FLOAT)
			{
				// Get the turn angle and the turn, and apply the turn.
				float sine = sqrtf(sine2);
				float turnAngle = Asinf(sine);
				turn.Scale(turnAngle/sine);
				ApplyTurn(turn);

				// Get the offset of the pushed position resulting from the turn.
				Vector3 offset(worldPosition);
				offset.Subtract(VEC3V_TO_VECTOR3(m_Matrix.GetCol3()));
				offset.CrossNegate(turn);

				// Find and apply the remaining push needed for the pushed position.
				Vector3 remainingPush(push);
				remainingPush.Subtract(offset);
				ApplyPush(remainingPush);
				return;
			}
		}
	}

	// The push is parallel to the offset from the rotation center and the pushed position, so don't apply a turn.
	ApplyPush(push);
}



void phCollider::ApplyPush (Vec::V3Param128 ipush, Vec::V3Param128 UNUSED_PARAM(position))
{
	//VALIDATE_PHYSICS_ASSERT(push.Mag2()<LARGE_FLOAT);
	Vec3V push(ipush);
	push = ClampMag( push, ScalarV(V_ZERO), ScalarV(Vec::V4VConstant<MAX_PUSH_CLAMP_HEX,MAX_PUSH_CLAMP_HEX,MAX_PUSH_CLAMP_HEX,MAX_PUSH_CLAMP_HEX>()) );

	Vec3V setPush = AddNet(m_Push, push);
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
	Assertf(IsLessThanAll(Abs(setPush), Vec3V(V_FLT_LARGE_6)), "phCollider::ApplyPushRigid - setting pushes to a large number.  Curr pushes = %f, %f, %f.  New pushes = %f, %f, %f",
		VEC3V_ARGS(m_Push), 
		VEC3V_ARGS(setPush));
#endif
	m_Push = setPush;

	DrawPush(push, m_Matrix.GetCol3Ref());	// m_Matrix's column 3 is used for drawing instead of position because the default position is ORIGIN
}


void phCollider::ApplyTurn (Vec::V3Param128 iturn)
{
	//VALIDATE_PHYSICS_ASSERT(turn.Mag2()<LARGE_FLOAT);
	Vec3V turn(iturn);
	turn = ClampMag( turn, ScalarV(V_ZERO), ScalarV(Vec::V4VConstant<MAX_PUSH_CLAMP_HEX,MAX_PUSH_CLAMP_HEX,MAX_PUSH_CLAMP_HEX,MAX_PUSH_CLAMP_HEX>()) );
	m_Turn = AddNet(m_Turn, turn);

	DrawTurn(turn, m_Matrix.GetCol3Ref());
}


void phCollider::ApplyAccel (Vec::V3Param128 acceleration, Vec::V3Param128 timestep)
{ 
	Vec3V force(acceleration);
	force = Scale(force, GetMassV());
	ApplyForceCenterOfMass(force.GetIntrin128(), timestep);
}


void phCollider::ApplyAccelOnAxis (float accel, int axisIndex, Vec::V3Param128 timestep)
{
	Vec3V force(V_ZERO);
	force[axisIndex] = accel*GetMass();
	ApplyForceCenterOfMass(force.GetIntrin128(), timestep);
}



void phCollider::ApplyAngAccelRigid (Vec::V3Param128 angAccel, Vec::V3Param128 timeStep)
{
	Mat33V inertia;
	GetInertiaMatrix(inertia);
	Vec3V torque;
	torque = Multiply( inertia, Vec3V(angAccel) );
	ApplyTorque(torque.GetIntrin128(), timeStep);
}



void phCollider::ApplyImpulseRigid (Vec::V3Param128 impulse, Vec::V3Param128 position)
{
	PDR_ONLY(debugPlayback::RecordApplyImpulse(*GetInstance(), impulse, position));

	m_Impulse = Add(m_Impulse, RCC_VEC3V(impulse));
	Vec3V angImpulse = RCC_VEC3V(position);
	angImpulse = Subtract(angImpulse, m_Matrix.GetCol3());
	angImpulse = Cross(angImpulse, RCC_VEC3V(impulse));
	m_AngImpulse = Add(m_AngImpulse, angImpulse);


	// Inform the instance that it received an impulse.
#if __PFDRAW
	DrawImpulse(Vec3V(impulse),Vec3V(position));
#endif
}


void phCollider::ApplyImpulseCenterOfMassRigid (Vec::V3Param128 impulse)
{
	PDR_ONLY(debugPlayback::RecordApplyImpulseCG(*GetInstance(), impulse));
	m_Impulse = Add(m_Impulse, Vec3V(impulse));

	DrawImpulse(Vec3V(impulse), m_Matrix.GetCol3());
}

void phCollider::ApplyAngImpulseRigid (Vec::V3Param128 angImpulse)
{
	m_AngImpulse = Add(m_AngImpulse, Vec3V(angImpulse));


	DrawAngularImpulse(Vec3V(angImpulse), m_Matrix.GetCol3());
}


void phCollider::ApplyJointAngImpulseRigid (Vec::V3Param128 angImpulse)
{
	m_AngImpulse = Add(m_AngImpulse, Vec3V(angImpulse));

	DrawAngularImpulse(Vec3V(angImpulse), m_Matrix.GetCol3());
}

void phCollider::ApplyImpetus ( Vec3V_In positionA, Vec3V_In impulseA, const int iElementA, const int iComponentA, const bool isForce, Vec::V3Param128 timestep, float breakScale)
{
	if (isForce)
	{
		// Apply a force.
		ApplyForce(impulseA.GetIntrin128(),positionA.GetIntrin128(), timestep,iComponentA);

		Assert(m_Instance);
		m_Instance->NotifyForce(impulseA.GetIntrin128(),positionA.GetIntrin128(),iComponentA,iElementA,breakScale);
	}
	else
	{
		// Apply an impulse.
		ApplyImpulse(impulseA.GetIntrin128(),positionA.GetIntrin128(),iComponentA,breakScale);

		Assert(m_Instance);
		m_Instance->NotifyImpulse(impulseA.GetIntrin128(),positionA.GetIntrin128(),iComponentA,iElementA,breakScale);
	}
}

#if HACK_GTA4_FRAG_BREAKANDDAMAGE

void phCollider::ApplyImpetusAndDamageImpetus ( const bool isForce, const int component, Vec3V_In impulse, Vec3V_In position, Vec3V_In damageImpetus, Vec::V3Param128 timestep)
{
	if (isForce)
	{
		// Apply a force.
		ApplyForce(impulse.GetIntrin128(),position.GetIntrin128(), timestep,component);
	}
	else
	{
		// Apply an impulse.
		ApplyImpulse(impulse.GetIntrin128(),position.GetIntrin128(),component);

		Assert(m_Instance);
		m_Instance->NotifyImpulse(damageImpetus.GetIntrin128(),position.GetIntrin128(),component);
	}
}
#endif // HACK_GTA4_FRAG_BREAKANDDAMAGE

void phCollider::ApplyForceImp (Vec::V3Param128 force, Vec::V3Param128 position, int component)
{
	PDR_ONLY(debugPlayback::RecordApplyForce(*GetInstance(), force, position, component));
	// Add the force.
	m_Force = Add(m_Force, Vec3V(force));

	// Calculate and add the torque.
	Vec3V torque(position);
	torque = Subtract(torque, m_Matrix.GetCol3());
	m_Torque = AddCrossed(m_Torque, torque, Vec3V(force));

	// Inform the instance that it received a force.
	// Commented this out for gta4 - but think the change should be made on the rage\dev branch as well.
	// 
	// By default NotifyForce isn't implemented anywhere, so this call usually does nothing
	// For gta4 we had to implement fragInst::NotifyForce because otherwise fragment objects wouldn't break apart when
	// hit by continuous forces as opposed to impulses, e.g. from explosions.  However this caused us problems because
	// applying continuous forces directly to the collider (e.g. car suspension) wasn't expected to produce any breaking.
	//
	// phCollider::ApplyImpulse doesn't call m_Instance->NotifyImpulse, and I believe that for consistency this should
	// be the default behaviour, and NotifyForce/Impulse calls should be handled through the phSimulator
	(void)component;

	DrawForce(Vec3V(force), Vec3V(position));
}


void phCollider::ApplyForceCenterOfMassImp (Vec::V3Param128 force)
{
	PDR_ONLY(debugPlayback::RecordApplyForceCG(*GetInstance(), force));

	m_Force = Add(m_Force, Vec3V(force));

	DrawForce(Vec3V(force), m_Matrix.GetCol3());
}


void phCollider::ApplyTorqueImp (Vec::V3Param128 torque)
{
	PDR_ONLY(debugPlayback::RecordApplyTorque(*GetInstance(), torque));
	m_Torque = Add(m_Torque, Vec3V(torque));

	DrawTorque(Vec3V(torque), m_Matrix.GetCol3());
}


void phCollider::SetVelocityImp (Vec::V3Param128 velocity)
{
	PDR_ONLY(debugPlayback::RecordSetVelocity(*GetInstance(), velocity));
	DEBUG_COLLIDER("SetVelocity", velocity);
	m_Velocity = RCC_VEC3V(velocity);
}

void phCollider::SetReferenceFrameVelocity (Vec::V3Param128 velocity) 
{
	DEBUG_COLLIDER("SetReferenceFrameVelocity", velocity);
	m_ReferenceFrameVelocityXYZDampingRateW.SetXYZ(RCC_VEC3V(velocity));
}

void phCollider::SetReferenceFrameAngularVelocity (Vec::V3Param128 angVelocity) 
{
	DEBUG_COLLIDER("SetReferenceFrameAngularVelocity", angVelocity);
	m_ReferenceFrameAngularVelocityXYZDampingRateW.SetXYZ(RCC_VEC3V(angVelocity));
}

void phCollider::SetAngVelocityImp (Vec::V3Param128 angVelocity)
{
	PDR_ONLY(debugPlayback::RecordSetAngularVelocity(*GetInstance(), angVelocity));
	m_AngVelocity = RCC_VEC3V(angVelocity);
}

void phCollider::AddMomentum(Vec::V3Param128 deltaMomentum)
{
	// Add the change in velocity to the current velocity
	Vec3V deltaVelocity = Scale(Vec3V(deltaMomentum),GetInvMassV());
	SetVelocity(Add(GetVelocity(), deltaVelocity).GetIntrin128());

	// Clamp the velocity
	ClampVelocity();

	PDR_ONLY(debugPlayback::RecordSetVelocity(*GetInstance(), m_Velocity.GetIntrin128()));
}

void phCollider::AddAngMomentum(Vec::V3Param128 deltaAngMomentum)
{
	// Add the change in angular velocity to the current angular velocity
	Mat33V inverseInertiaMtx;
	GetInverseInertiaMatrix(inverseInertiaMtx);
	Vec3V deltaAngVelocity = Multiply(inverseInertiaMtx, Vec3V(deltaAngMomentum));
	SetAngVelocity(Add(GetAngVelocity(),deltaAngVelocity).GetIntrin128());

	// Clamp the angular velocity
	ClampAngularVelocity();

	PDR_ONLY(debugPlayback::RecordSetAngularVelocity(*GetInstance(), m_AngVelocity.GetIntrin128()));
}

Vec3V_Out phCollider::CalculateAngMomentum () const
{
	Mat33V inertiaMtx;
	GetInertiaMatrix(inertiaMtx);
	return Multiply(inertiaMtx, m_AngVelocity);
}

bool phCollider::IsConstrained () const
{
	return false;
}

#if TRACK_COLLISION_TIME
void phCollider::DecreaseCollisionTimeSafe(float time)
{
	bool success = false;
	u32 stopper = 8;

#if __SPU
	Assert((u32)this > 256 * 1024); // should be called on a PPU pointer to work correctly
#endif

	while (!success)
	{
		unsigned colliderTimeInt = sysInterlockedRead((unsigned*)&m_CollisionTime);
		float colliderTime = *(float*)&colliderTimeInt;

		if (colliderTime > time)
		{
			unsigned newColliderTimeInt = *(unsigned*)&time;

			if (sysInterlockedCompareExchange((unsigned*)&m_CollisionTime, newColliderTimeInt, colliderTimeInt) == colliderTimeInt)
			{
				sys_lwsync();

				success = true;
			}
		}
		else
		{
			success = true;
		}

		if (!success)
		{
			volatile u32 spinner = 0;
			while (spinner < stopper)
				++spinner;
			if (stopper < 1024)
				stopper <<= 1;
		}
	}
}
#endif

#if __PS3
void phCollider::GenerateDmaPlan(DMA_PLAN_ARGS(phCollider))
{
	DMA_BEGIN_PARTIAL(this, &m_AngInertiaXYZMassW);

	m_DmaPlan = &dmaPlan;
}
#endif

void phCollider::RevertImpulsesRigid()
{
	m_Velocity = m_LastVelocity;
	m_AngVelocity = m_LastAngVelocity;

	PDR_ONLY(debugPlayback::RecordSetVelocity(*GetInstance(), m_Velocity.GetIntrin128()));
	PDR_ONLY(debugPlayback::RecordSetAngularVelocity(*GetInstance(), m_AngVelocity.GetIntrin128()));
}

bool phCollider::RejuvenateImp ()
{
	bool errorLimitReached = true;

#if REJUVENATE_TEST
	// See how far away from normal and orthogonal the matrix gets.
	// When rejuvenate testing is turned on, this will only change the matrix if the error
	// limit is reached, to find out how necessary this is.
	errorLimitReached = !GetMatrix().IsOrthonormal(REJUVENATE_ERROR);
	if (errorLimitReached)
#endif
	{
		if (RC_MATRIX34(m_Matrix).a.IsNonZero() && RC_MATRIX34(m_Matrix).b.IsNonZero() && RC_MATRIX34(m_Matrix).c.IsNonZero())
		{
			RC_MATRIX34(m_Matrix).b.Normalize();
			RC_MATRIX34(m_Matrix).c.Cross(RC_MATRIX34(m_Matrix).a, RC_MATRIX34(m_Matrix).b);
			RC_MATRIX34(m_Matrix).c.Normalize();
			RC_MATRIX34(m_Matrix).a.Cross(RC_MATRIX34(m_Matrix).b, RC_MATRIX34(m_Matrix).c);
		}
		else
		{
			// protection against very bad matrices...
			RC_MATRIX34(m_Matrix).Identity3x3();
		}

		SetInstanceMatrixFromCollider();
	}

	m_RejuvenateCount = 0;
	return errorLimitReached;
}

#if __DEV
void phCollider::SetDebugColliderUpdate(phCollider* debugCollider)
{
	sm_DebugColliderUpdate = debugCollider;
}
#endif

#if __DEBUGLOG
void phCollider::DebugReplay(void) const
{
	if (!diagDebugLogIsActive(diagDebugLogPhysicsSlow))
		return;

	if (m_Instance)
		m_Instance->DebugReplay();

	if (m_Sleep)
		m_Sleep->DebugReplay();

	diagDebugLog(diagDebugLogPhysics, 'pCLI', &m_LastInstanceMatrix);

	diagDebugLog(diagDebugLogPhysics, 'pCMM', &m_Mass);
	diagDebugLog(diagDebugLogPhysics, 'pCIM', &m_InvMass);
	diagDebugLog(diagDebugLogPhysics, 'pCAI', &m_AngInertia);
	diagDebugLog(diagDebugLogPhysics, 'pCIA', &m_InvAngInertia);
	diagDebugLog(diagDebugLogPhysics, 'pCMS', &m_MaxSpeed);
	diagDebugLog(diagDebugLogPhysics, 'pCMA', &m_MaxAngSpeed);

	diagDebugLog(diagDebugLogPhysics, 'pCMX', &m_Matrix);
	diagDebugLog(diagDebugLogPhysics, 'pCVV', &m_Velocity);
	diagDebugLog(diagDebugLogPhysics, 'pCAV', &m_AngVelocity);
	diagDebugLog(diagDebugLogPhysics, 'pCMT', &m_Momentum);
	diagDebugLog(diagDebugLogPhysics, 'pCAM', &m_AngMomentum);

	diagDebugLog(diagDebugLogPhysics, 'pCFF', &m_Force);
	diagDebugLog(diagDebugLogPhysics, 'pCTT', &m_Torque);
	diagDebugLog(diagDebugLogPhysics, 'pCII', &m_Impulse);
	diagDebugLog(diagDebugLogPhysics, 'pCAI', &m_AngImpulse);
	diagDebugLog(diagDebugLogPhysics, 'pCPH', &m_Push);
	diagDebugLog(diagDebugLogPhysics, 'pCTU', &m_Turn);
	diagDebugLog(diagDebugLogPhysics, 'pCAP', &m_AppliedPush);
	diagDebugLog(diagDebugLogPhysics, 'pCAT', &m_AppliedTurn);

	diagDebugLog(diagDebugLogPhysics, 'pCNS', &m_NetSpringDampMat);
	diagDebugLog(diagDebugLogPhysics, 'pCSE', &m_SumErFdMat);
	diagDebugLog(diagDebugLogPhysics, 'pCSF', &m_SumFdErSq);
	diagDebugLog(diagDebugLogPhysics, 'pCRC', &m_RejuvenateCount);
}
#endif


////////////////
// RESOURCING
////////////////


phCollider::phCollider(class datResource &rsc)
{
#if __SPU
	rsc.PointerFixup(m_Instance);
#endif
	rsc.PointerFixup(m_Sleep);
}

} // namespace rage
