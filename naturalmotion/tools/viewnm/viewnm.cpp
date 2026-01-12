// 
// viewnm\viewnm.cpp
//
// Copyright (C) Rockstar Games.  All Rights Reserved.
//
// Suggested command lines:
// -file=$\fragments\piranha\entity.type
// -file=$\fragments\detectivedc\entity.type
// -file=$\fragments\hotdogred_rex\entity.type
// -file=$\fragments\env_sheet\env_sheet.type
// -file=$\rage_male_cloth\rage_male_cloth.type
//
// View GTA4 props:
// -zup -archive x:/gta/build/xbox360/data/maps/props/roadside/traffic.img -fragrsc -rscfile x:/gta/build/xbox360/data/maps/props/roadside/bm_nytraflite5a -shaderlib x:/gta/build/common/shaders -shaderdb x:/gta/build/common/shaders/db


#include "sample_physics/sample_physics.h"
#include "sample_fragment/fragworld.h"

#include "fragment/animation.h"
#include "fragment/cache.h"
#include "fragment/cachemanager.h"
#include "fragment/drawable.h"
#include "fragment/tune.h"
#include "fragment/typechild.h"

#include "art/messageparams.h"
#include "fragmentnm/instance.h"
#include "fragmentnm/manager.h"

#include "rockstar/NmRsCharacter.h"

#include "bank/bkmgr.h"
#include "cranimation/animation.h"
#include "cranimation/frame.h"
#include "crskeleton/skeleton.h"
#include "crskeleton/skeletondata.h"
#include "math/random.h"
#include "paging/dictionary.h"
#include "pharticulated/articulatedcollider.h"
#include "phcore/phmath.h"
#include "physics/levelnew.h"
#include "physics/simulator.h"
#include "physics/asyncshapetestmgr.h"
#include "grprofile/drawmanager.h"
#include "rmptfx/ptxmanager.h"
#include "shaderlib\skinning_method_config.h"
#include "system/main.h"
#include "system/param.h"
#include "system/timemgr.h"	 
#include "vectormath/vec3v.h"

#include "cloth/charactercloth.h"
#include "cloth/environmentcloth.h"
#include "pheffects/cloth_verlet.h"
#include "phcore/materialmgrflag.h"
#include "grmodel/geometry.h"

PARAM(file,		 "[viewnm] Fragment type to load");
PARAM(tdfile,	 "[viewnm] File to load texture dictionary from (when using fragrsc)");
PARAM(nmxmlfile, "[viewnm] Natural Motion xml file to load from in addition to the type file");
PARAM(anim,      "[viewnm] The animation to play on the character");
PARAM(artfile,   "[viewnm] The binary file to load (NaturalMotion asset)");
PARAM(physRig,   "[viewnm] The .XML physics rig");

namespace rage {

EXT_PFD_DECLARE_ITEM(Solid);
EXT_PFD_DECLARE_ITEM(Wireframe);
EXT_PFD_DECLARE_ITEM(Models);

#if (__BANK && !__RESOURCECOMPILER)
datCallback	g_ClothManagerCallback = NullCB;
#endif 

extern bool g_FixRagdollStuckInGeometry;
extern bool g_FixRagdollStuckInGeometryElementMatch;
extern float g_FixRagdollStuckInGeometryDepth;

}
namespace ragesamples {

using namespace rage;

	phAsyncShapeTestMgr* ms_pRageAsyncShapeTestMgr;

class viewInstNM : public fragInstNM
{
public:
	viewInstNM(const fragType* type, const Matrix34& matrix, u32 guid = 0)
		: fragInstNM(type, matrix, guid)
	{
	}

	phInst* PrepareForActivation(phCollider** collider, phInst* otherInst, const phConstraintBase * constraint)
	{
		phInst* result = fragInstNM::PrepareForActivation(collider, otherInst, constraint);

		ART::MessageParams msg;
		msg.addBool("start", true);
		msg.addBool("useBodyTurn", false);
		PostARTMessage("bodyBalance", &msg);

		return result;
	}
};

////////////////////////////////////////////////////////////////
// 

class nmSampleManager : public physicsSampleManager
{
public:
	nmSampleManager()
		: m_Type(NULL)
		, m_TextureDictionary(NULL)
		, m_Inst(NULL)
		, m_LoadedAnimation(NULL)
		, m_Animation(NULL)
		, m_AnimFrame(NULL)
		, m_AnimPhase(0.0f)
		, m_FrameRate(1.0f)
		, m_AnimBank(NULL)
		, m_AnimIndex(0)
		, m_LoopWidget(true)
	{
		m_UseZUp = true;
	}

	virtual void InitClient()
	{
#if __XENON
		AssertMsg(MTX_IN_VB == 0, "Turn off MTX_IN_VB in shaderlib\\skinning_method_config.h to fix rendering problems. These problems are due to not double buffering matrices so it does no affect games, only samples.");
#endif

		physicsSampleManager::InitClient();

#if __PFDRAW
        PFD_Solid.SetEnabled(true);
		PFD_Wireframe.SetEnabled(true);
        PFD_Models.SetEnabled(false);
#endif

		m_Mapper.Map(IOMS_KEYBOARD, KEY_F4, m_Reset);
		m_Mapper.Map(IOMS_KEYBOARD, KEY_C, m_PutIntoCache);

		//RMPTFX Initialization
		ptxManager::InitClass(); 
		RMPTFXMGR.CreatePointPool(1000); 
		RMPTFXMGR.CreateEffectInstPool(1000); 

		bkBank& ragdollBank = BANKMGR.CreateBank("Fix Stuck Ragdolls");
		ragdollBank.AddToggle("Fix Stuck Ragdolls", &g_FixRagdollStuckInGeometry);
		ragdollBank.AddToggle("Element Match", &g_FixRagdollStuckInGeometryElementMatch);
		ragdollBank.AddSlider("Fake Depth", &g_FixRagdollStuckInGeometryDepth, 0.0f, 1.0f, 0.01f);

		m_Demos.AddDemo(rage_new fragWorld("viewnm"));

		// Set the pose with a rotation and offset.
		Vector3 position(0.0f,0.0f,0.0f);
		Vector3 rotation(0.0f,0.0f,0.0f);
		m_ObjectMatrix.Set(CreateRotatedMatrix(position,rotation));

		m_Demos.GetCurrentWorld()->Init(1000, 128, 10000, Vec3V(-10000.0f, -10000.0f, -10000.0f),  Vec3V(10000.0f, 10000.0f, 10000.0f), 128);
		m_Demos.GetCurrentWorld()->Activate();
		if (m_UseZUp)
		{
			m_Demos.GetCurrentWorld()->GetSimulator()->SetGravity(Vector3(0.0f, 0.0f, GRAVITY));
		}

		const bool triangulated = true;
		Vector3 height(0.0f, 0.0f, 1.0f);
		m_Demos.GetCurrentWorld()->ConstructTerrainPlane(triangulated, INCLUDE_FLAGS_ALL, GetZUp(), m_ObjectMatrix.d - height);
		//m_Demos.GetCurrentWorld()->CreateFixedBox(Vector3(10.0f, 0.01f, 10.0f), position - Vector3(0.0f, 10.0f, 0.0f));
		m_Demos.GetCurrentWorld()->SetUpdateCallbacks(phDemoWorld::UpdateCallback().Reset<nmSampleManager, &nmSampleManager::PreSimUpdate>(this),
			phDemoWorld::UpdateCallback().Reset<nmSampleManager, &nmSampleManager::PostSimUpdate>(this));

		const char* artFile = "$\\naturalmotion\\fred_nm_load_data";
		PARAM_artfile.Get(artFile);
		const char* physRig = "$\\naturalmotion\\fred_physics_rig";
		PARAM_physRig.Get(physRig);

		fragNMAssetManager::Create();
		FRAGNMASSETMGR->Load(artFile);

		const char* file = "$\\fragments\\GTA5_male_NM_actor\\entity.type";
		PARAM_file.Get(file);

		m_Type = rage_new fragType;
		m_Type->LoadWithXML(file, physRig);

		// set bound friction
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(0)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("buttocks"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(1)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("thigh_left"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(2)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("shin_left"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(3)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("foot_left"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(4)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("thigh_right"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(5)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("shin_right"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(6)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("foot_right"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(7)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("spine0"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(8)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("spine1"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(9)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("spine2"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(10)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("spine3"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(11)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("clavicle_left"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(12)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("upper_arm_left"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(13)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("lower_arm_left"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(14)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("hand_left"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(15)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("clavicle_right"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(16)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("upper_arm_right"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(17)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("lower_arm_right"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(18)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("hand_right"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(19)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("neck"));
		m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(20)->SetMaterial(MATERIALMGRFLAG.FindMaterialId("head"));

#if __DEV
		int numBounds = m_Type->GetPhysics(0)->GetCompositeBounds()->GetNumBounds();		
		for (int iPart = 0; iPart < numBounds; iPart++)
		{
			Printf("\n friction for part %d is %f. Elasticity is %f. Material name is %s.", iPart, 
				m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(iPart)->GetMaterial(0).GetFriction(),
				m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(iPart)->GetMaterial(0).GetElasticity(),
				m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(iPart)->GetMaterial(0).GetName());
		}
		Printf("\n\n");
#endif

		ms_pRageAsyncShapeTestMgr = rage_new phAsyncShapeTestMgr;
		const int asyncProbeSchedulerIndex = 0;
		ms_pRageAsyncShapeTestMgr->Init(asyncProbeSchedulerIndex);
		ART::NmRsCharacter::SetAsyncShapeTestMgr(ms_pRageAsyncShapeTestMgr);

		m_Inst = rage_new viewInstNM(m_Type, m_ObjectMatrix);
		m_Inst->Insert(false);

		// Refresh the fragment type list in the widgets. The new type was added to the list in Load() above.
		BANK_ONLY(FRAGTUNE->RefreshTypeList());

		if (!m_Inst)
		{
			Quitf("Fragment type missing: %s", file);
		}

        m_DemoObject = m_Demos.GetCurrentWorld()->RequestObject();
        m_DemoObject->SetPhysInst(m_Inst);
        m_DemoObject->SetResetMatrix(m_ObjectMatrix);
        m_DemoObject->SetDrawSolidIfNoModel(false);
        m_Demos.GetCurrentWorld()->RegisterObject(m_DemoObject);

		int startingDemo = 0;

		m_Demos.SetCurrentDemo(startingDemo);

		if (PARAM_anim.Get())
		{
			const char* animName;
			PARAM_anim.Get(animName);

			m_LoadedAnimation = m_Animation = crAnimation::AllocateAndLoad(animName);
		}
	}

	virtual void InitCamera()
	{
		Vector3 lookFrom(m_ObjectMatrix.d);
		lookFrom += Vector3(2.0f, -4.0f, 2.0f);
		Vector3 lookTo(m_ObjectMatrix.d);

		m_CamMgr.SetUseZUp(true);
		InitSampleCamera(lookFrom,lookTo);
	}

	virtual void Update()
	{
		//EnableNanSignal(true);

		m_Mapper.Update();
		if(m_Reset.IsPressed())
		{
			//someone else is resetting the particles
			//RMPTFXMGR.Reset();
		}

		if (m_PutIntoCache.IsPressed() && m_Inst->GetType() && !m_Inst->GetCached())
		{
			m_Inst->PutIntoCache();
		}

		FRAGMGR->SetInterestFrustum(*m_Viewport, GetCameraMatrix());

		if (g_ptxManager::IsInstantiated())
		{
			RMPTFXMGR.BeginFrame();
//			RMPTFXMGR.Update();
		}		

		if (m_Animation && m_Inst->GetType() && !m_Inst->GetCached() && PHLEVEL->IsInactive(m_Inst->GetLevelIndex()))
		{
			// Just for the heck of it let's get the instance into the cache as quickly as possible.  It's not strictly necessary to do so unless we animating it, though.
			m_Inst->PutIntoCache();
			Assert(m_Inst->GetCached());
		}

#if __BANK
		if (m_Inst->GetType() && m_AnimBank == NULL)
		{
			m_AnimBank = &BANKMGR.CreateBank("Animations");

			m_AnimBank->AddToggle("Loop", &m_LoopWidget);
			m_AnimBank->AddButton("Play", datCallback(MFA(nmSampleManager::BankPlayAnimation), this));
			m_AnimBank->AddSlider("Speed", &m_FrameRate, 0.0f, 10.0f, 0.1f);
			m_AnimBank->AddButton("Load from file", datCallback(MFA(nmSampleManager::LoadAnimation), this));

			m_AnimNames[0] = "<Loaded from file>";
			int numNames = 1;

			Assert(m_Inst->GetType());
			if (fragDrawable* drawable = m_Inst->GetType()->GetCommonDrawable())
			{
				for (int animIndex = 0; animIndex < drawable->GetAnimCount() && numNames < MAX_NUM_ANIMS; ++animIndex, ++numNames)
				{
					fragAnimation* anim = drawable->GetAnimation(animIndex);
					m_AnimNames[numNames] = anim->Name;
				}
			}

			m_AnimBank->AddCombo("Animation Name", &m_AnimIndex, numNames, m_AnimNames, datCallback(MFA(nmSampleManager::BankSelectAnimation),this));

			if (&m_Inst->GetType()->GetCommonDrawable()->GetShaderGroup())
			{
				m_Inst->GetType()->GetCommonDrawable()->GetShaderGroup().AddWidgets(BANKMGR.CreateBank("Shaders"));
			}
		}
#endif

		if (m_Animation && m_Demos.GetCurrentWorld()->ShouldUpdate())
		{
			// We have loaded an animation.
			if (!m_AnimFrame)
			{
				if (crSkeleton* skeleton = m_Inst->GetSkeleton())
				{
					m_AnimFrame = rage_new crFrame;
					const crSkeletonData& skeletonData = skeleton->GetSkeletonData();
					m_AnimFrame->InitCreateBoneAndMoverDofs(skeletonData, false);
				}
			}

			if (m_AnimFrame)
			{
				if (m_Inst->IsInLevel() && !PHLEVEL->IsActive(m_Inst->GetLevelIndex()) && m_Inst->GetCached())
				{
					// Have the animation pose the skeleton.
					// Note that this is only done if the instance is no longer active.
					m_Animation->CompositeFrame(m_AnimPhase, *m_AnimFrame, false, false);
					crSkeleton* skeleton = m_Inst->GetSkeleton();
					m_AnimFrame->Pose(*skeleton);

					fragHierarchyInst* hierInst = fragCacheManager::GetEntryFromInst(m_Inst)->GetHierInst();
					Assert(hierInst);
					Matrix34 linkWorld;
					const Mat34V* parentMatrix = &m_Inst->GetMatrix();

					if(hierInst->body != NULL && PHLEVEL->IsActive(m_Inst->GetLevelIndex()))
					{
						int linkIndex = hierInst->articulatedCollider->GetLinkFromComponent(0);
						phArticulatedBody* body = hierInst->body;

						linkWorld.Transpose(MAT34V_TO_MATRIX34(body->GetLink(linkIndex).GetMatrix()));
						linkWorld.d.Set(body->GetLink(linkIndex).GetPosition());
						linkWorld.Dot(RCC_MATRIX34(m_Inst->GetMatrix()));
						linkWorld.DotTranspose(m_Inst->GetTypePhysics()->GetLinkAttachmentMatrices()[0]);

						parentMatrix = &RCC_MAT34V(linkWorld);
					}

					skeleton->SetParentMtx(parentMatrix);
					skeleton->Update();
					skeleton->SetParentMtx(&m_Inst->GetMatrix());
					//m_Inst->SkeletonWasUpdated(reinterpret_cast<Matrix34*>(skeleton->GetObjectMtxs()), skeleton->GetSkeletonData().GetNumBones());
					// ^This used to be called, is this the correct replacement?
					m_Inst->PoseBoundsFromSkeleton(false, false);
				}

				m_AnimPhase += TIME.GetSeconds() * m_FrameRate * m_Demos.GetCurrentWorld()->GetTimeWarp();

				if (m_AnimPhase > m_Animation->GetDuration())
				{
					if (m_LoopWidget)
					{
						m_AnimPhase -= m_Animation->GetDuration();
					}
					else
					{
						m_AnimPhase = m_Animation->GetDuration();
					}
				}
			}
		}

		physicsSampleManager::Update();
	}

	void PreSimUpdate(float deltaTime)
	{
		FRAGNMASSETMGR->StepPhase1(deltaTime);
	}

	void PostSimUpdate(float deltaTime)
	{
		FRAGNMASSETMGR->StepPhase2(deltaTime);
	}

	virtual void DrawClient()
	{
		physicsSampleManager::DrawClient();
	}

	virtual void ShutdownClient()
	{
		m_Demos.GetCurrentWorld()->RemoveObject(m_DemoObject);

		FRAGMGR->Reset();

		delete m_AnimFrame;
		delete m_LoadedAnimation;

		m_Inst->Remove();
		delete m_Inst;
		delete m_Type;

		physicsSampleManager::ShutdownClient();

		m_DemoObject->SetPhysInst(NULL);
		delete m_DemoObject;

		if (g_ptxManager::IsInstantiated())
		{
			ptxManager::ShutdownClass();
		}
	}

	void DrawHelpClient()
	{
		physicsSampleManager::DrawHelpClient();

		char tmpString[100];
		formatf(tmpString, sizeof(tmpString), "broken: %s", m_Inst->GetBroken() ? "true" : "false");
		DrawString(tmpString, false);
		formatf(tmpString, sizeof(tmpString), "part broken: %s", m_Inst->GetPartBroken() ? "true" : "false");
		DrawString(tmpString, false);
	}

protected:
	virtual const char* GetSampleName() const
	{
		return "viewnm";
	}

private:
	void BankSelectAnimation()
	{
		if (m_AnimIndex == 0)
		{
			m_Animation = m_LoadedAnimation;
		}
		else if (fragDrawable* drawable = m_Inst->GetType()->GetCommonDrawable())
		{
			m_Animation = drawable->GetAnimation(m_AnimIndex - 1)->Animation;
		}

		m_AnimPhase = 0.0f;
	}

	void BankPlayAnimation()
	{
		m_AnimPhase = 0.0f;
	}

	void LoadAnimation()
	{
#if __BANK
		if (const char* animFile = BANKMGR.OpenFile("*.anim", false, "Animation (*.anim)"))
		{
			delete m_LoadedAnimation;

			m_LoadedAnimation = m_Animation = crAnimation::AllocateAndLoad(animFile);

			m_AnimIndex = 0;
			m_AnimPhase = 0.0f;
		}
#endif
	}

	fragType*					m_Type;
	pgDictionary<grcTexture>*	m_TextureDictionary;
	fragInst*					m_Inst;
    phDemoObject*               m_DemoObject;
	Matrix34 m_ObjectMatrix;

	// Animation related
	static const int			MAX_NUM_ANIMS = 256;
	crAnimation*				m_LoadedAnimation;
	crAnimation*				m_Animation;
	crFrame*				m_AnimFrame;
	float						m_AnimPhase;
	float						m_FrameRate;
	bkBank*						m_AnimBank;
	const char*					m_AnimNames[MAX_NUM_ANIMS];
	int							m_AnimIndex;
	bool						m_LoopWidget;

	ioMapper					m_Mapper;
	ioValue						m_Reset;
	ioValue						m_PutIntoCache;
};

} // namespace ragesamples

// main application
int Main()
{
	{
		ragesamples::nmSampleManager samplePhysics;

		samplePhysics.SetFullAssetPath(RAGE_ASSET_ROOT);

		samplePhysics.Init();

		samplePhysics.UpdateLoop();

		samplePhysics.Shutdown();
	}

	return 0;
}





