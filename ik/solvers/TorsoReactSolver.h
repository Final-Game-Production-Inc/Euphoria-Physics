// 
// ik/solvers/TorsoReactSolver.h
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef TORSOREACTSOLVER_H
#define TORSOREACTSOLVER_H

///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//
// IK solver that applies procedural impulse react to torso. Derives from
// crIKSolverBase base class. 
//
///////////////////////////////////////////////////////////////////////////////

#include "crbody/iksolverbase.h"

#include "fwtl/pool.h"
#include "vector/vector3.h"

#include "Animation/AnimBones.h"
#include "Animation/debug/AnimDebug.h"
#include "ik/IkRequest.h"
#include "Peds/pedDefines.h"
#include "scene/RegdRefTypes.h"

namespace rage
{
	class Matrix34;
};

class CTorsoReactIkSolver : public crIKSolverBase
{
public:
	CTorsoReactIkSolver(CPed* pPed);
	~CTorsoReactIkSolver();

	FW_REGISTER_CLASS_POOL(CTorsoReactIkSolver);

	// PURPOSE: 
	virtual void Reset();

	// PURPOSE: 
	virtual bool IsDead() const;

	// PURPOSE: 
	virtual void Iterate(float dt, crSkeleton& skeleton);

	// PURPOSE:
	void Request(const CIkRequestBodyReact& request);

	// PURPOSE:
	//void PreIkUpdate();

	// PURPOSE:
	void Update(float deltaTime);

	// PURPOSE:
	void Apply(crSkeleton& skeleton);

	// PURPOSE:
	void PostIkUpdate(float deltaTime);

	// PURPOSE:
	bool IsActive() const;

#if __IK_DEV
	virtual void DebugRender();
	bool CaptureDebugRender();
	static void AddWidgets(bkBank *pBank);
	static void TestReact();
#endif //__IK_DEV

	static const int MaxReacts = 6;

	enum eSpinePart { SPINE0, SPINE1, SPINE2, SPINE3, NUM_SPINE_PARTS };
	enum eArmType { LEFT_ARM, RIGHT_ARM, NUM_ARMS };
	enum eLegType { LEFT_LEG, RIGHT_LEG, NUM_LEGS };
	enum eLegPart { THIGH, CALF, FOOT, TOE, NUM_LEG_PARTS };

	struct BoneSituation
	{
		Vector3 vAxis;			// Rotation axis in object space based on approximated torque vector
		float fMaxAngle;		// Max angle of rotation about axis
		float fAngle;			// Current angle of rotation about axis

		void Reset()
		{
			vAxis.Zero();
			fMaxAngle = 0.0f;
			fAngle = 0.0f;
		}
	};

	struct ReactSituation
	{
		BoneSituation aoSpineBones[NUM_SPINE_PARTS];
		BoneSituation oNeckBone;
		BoneSituation oPelvisBone;
		BoneSituation oRootBone;

		float fDuration;
		float fNeckDuration;

		u32 bActive : 1;
		u32 bLocalInflictor : 1;

		ReactSituation()
		{
			Reset();
		}

		void Reset()
		{
			for (int spineBoneIndex = 0; spineBoneIndex < NUM_SPINE_PARTS; ++spineBoneIndex)
				aoSpineBones[spineBoneIndex].Reset();

			oNeckBone.Reset();
			oPelvisBone.Reset();
			oRootBone.Reset();

			fDuration = 0.0f;
			fNeckDuration = 0.0f;
			bActive = false;
			bLocalInflictor = true;
		}
	};

	struct Params
	{
		enum Type
		{
			AI,
			LOCAL_PLAYER,
			REMOTE_PLAYER,
			REMOTE_REMOTE,
			NUM_TYPES
		};

		Vector3 avSpineLimits[NUM_SPINE_PARTS][2];	// [Min|Max]
		Vector3 avPelvisLimits[2];					// [Min|Max]
		float afRootLimit[NUM_TYPES];

		float afMaxDeflectionAngle[NUM_TYPES];
		float fMaxDeflectionAngleLimit;
		float fMaxCameraDistance;

		float afSpineStiffness[NUM_TYPES][NUM_SPINE_PARTS];
		float afPelvisStiffness[NUM_TYPES];
		float afRootStiffness[NUM_TYPES];

		float afReactMagnitude[NUM_TYPES];

		float afReactMagnitudeNetModifier[NUM_TYPES];

		float afReactSpeedModifier[NUM_TYPES];
		float fMaxCharacterSpeed;

		float fReactVehicleModifier;
		float fReactFirstPersonVehicleModifier;
		float fReactVehicleLimit;
		float fReactMeleeModifierLocalPlayer;
		float fReactMeleeModifier;
		float fReactEnteringVehicleModifier;
		float fReactLowCoverModifier;

		float fReactAimingModifier;
#if FPS_MODE_SUPPORTED
		float fReactFPSModifier;
#endif // FPS_MODE_SUPPORTED

		float fReactDistanceModifier;
		float afReactDistanceLimits[2];

		float afMinDistFromCenterThreshold[NUM_TYPES];
		float afMaxDistFromCenterThreshold[NUM_TYPES];

		float fNeckDelay;
		u32 auReactDurationMS[NUM_TYPES];

		float afLowerBodyDeflectionAngleModifier[NUM_TYPES];
		float afLowerBodyMinDistFromCenterThreshold[NUM_TYPES];
		float afLowerBodyMaxDistFromCenterThreshold[NUM_TYPES];

		float afWeaponGroupModifierSniper[NUM_TYPES];
		float afWeaponGroupModifierShotgun[NUM_TYPES];

#if __IK_DEV
		Vector3 vTestPosition;
		Vector3 vTestDirection;
		bool bDrawTest;
#endif // __IK_DEV
	};
	static Params ms_Params;

private:
	bool IsValid();
	bool IsBlocked();
	float SampleResponseCurve(float fInterval);
	float GetReactDistanceModifier();
	float GetWeaponGroupModifier(int type, u32 weaponGroup);
	int GetBoneIndex(int component);
	int GetTypeIndex(const CIkRequestBodyReact& request);
	int GetTypeIndex() const;
	void ClampAngles(Vector3& eulers, const Vector3& min, const Vector3& max);
	void ClampAngles(Matrix34& m, const Vector3& min, const Vector3& max);

	ReactSituation m_aoReacts[MaxReacts];
	CPed* m_pPed;

	u16 m_auSpineBoneIdx[NUM_SPINE_PARTS];
	u16 m_auArmBoneIdx[NUM_ARMS];
	u16 m_auLegBoneIdx[NUM_LEGS][NUM_LEG_PARTS];
	u16 m_uPelvisBoneIdx;
	u16 m_uRootBoneIdx;
	u16 m_uNeckBoneIdx;

	u32 m_removeSolver	: 1;
	u32 m_bonesValid	: 1;

#if __IK_DEV
	static CDebugRenderer ms_debugHelper;
	static bool ms_bRenderDebug;
	static int ms_iNoOfTexts;
#endif //__IK_DEV
};

#endif // TORSOREACTSOLVER_H
