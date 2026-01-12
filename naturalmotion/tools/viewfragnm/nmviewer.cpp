// 
// nmviewer/nmviewer.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "nmviewer.h"

#include "bank/bank.h"
#include "bank/bkmgr.h"
#include "bank/combo.h"
#include "cranimation/animation.h"
#include "crskeleton/skeleton.h"
#include "crskeleton/skeletondata.h"
#include "fragment/cachemanager.h"
#include "fragment/instance.h"
#include "fragment/tune.h"
#include "fragment/typechild.h"
#include "fragment/animation.h"
#include "fragment/drawable.h"
#include "fragmentnm/instance.h"
#include "fragmentnm/manager.h"
#include "fragmentnm/messageparams.h"
#include "fragmentnm/nmbehavior.h"
#include "grcore/texture.h"
#include "parser/manager.h"
#include "pharticulated/articulatedcollider.h"
#include "phcore/phmath.h"
#include "physics/levelnew.h"
#include "physics/simulator.h"
#include "rmptfx/ptxmanager.h"
#include "system/param.h"
#include "system/timemgr.h"

#include "nminst.h"

#define DEBUG_ALT_UPDATE_ORDER 0

using namespace rage;

XPARAM(file);   //PARAM(file,		 "[viewfragnm] Fragment type to load");
PARAM(rscfile,	 "[viewfragnm] File to use to save resources (when using fragrsc)");
PARAM(tdfile,	 "[viewfragnm] File to load texture dictionary from (when using fragrsc)");
XPARAM(anim);   //PARAM(anim,      "[viewfragnm] The animation to play on the character");
PARAM(artfile,   "[viewfragnm] The binary file to load (NaturalMotion asset)");
PARAM(physRig,   "[viewfragnm] The .XML physics rig");

namespace rage {

EXT_PFD_DECLARE_GROUP(Physics);

}
namespace ragesamples
{

NMViewer::NMViewer()
: rmcSampleSimpleWorld()
, m_UseRageActivePose(false)
, m_ActivePoseInst(NULL)
, m_UseNMActivePose(false)
, m_Type(NULL)
, m_TextureDictionary(NULL)
, m_Inst(NULL)
, m_LoadedAnimation(NULL)
, m_Animation(NULL)
, m_AnimFrame(NULL)
, m_ActivePoseStiffness(0.8f)
, m_AnimPhase(0.0f)
, m_FrameRate(1.0f)
#if __BANK
, m_AnimBank(NULL)
#endif // __BANK
, m_AnimIndex(0)
, m_LoopWidget(true)
#if __BANK
, m_BehaviorBank(NULL)
, m_BehaviorListComboBox(NULL)
, m_currentNMBehavior(-1)
, m_previousNMBehavior(-1)
#endif // __BANK
{
	m_UseZUp = true;

    m_MaxOctreeNodes = 1000;
    m_MaxActivePhysicsObjects = 128;
    m_MaxPhysicsObjects = 128;
}

static const u32 DOESNT_COLLIDE_WITH_TERRAIN = 1 << 1;

NMViewer::~NMViewer()
{
}

void NMViewer::InitClient()
{
	PFDGROUP_Physics.SetEnabled(true);

    m_constructTerrainPlane = false;

    rmcSampleSimpleWorld::InitClient();

    const char* artFile = "$\\naturalmotion\\fred_nm_load_data";
    PARAM_artfile.Get(artFile);
	const char* physRig = "$\\naturalmotion\\fred_physics_rig";
	PARAM_physRig.Get(physRig);

	phSimulator::SetSleepEnabled(false);

    m_pFragWorld->ConstructTerrainPlane(false, ~DOESNT_COLLIDE_WITH_TERRAIN, m_UseZUp);
    m_pFragWorld->SetUpdateCallbacks(phDemoWorld::UpdateCallback().Reset<NMViewer, &NMViewer::PreSimUpdate>(this),
        phDemoWorld::UpdateCallback().Reset<NMViewer, &NMViewer::PostSimUpdate>(this));

    if (m_UseZUp)
    {
        m_pFragWorld->GetSimulator()->SetGravity(Vector3(0.0f, 0.0f, GRAVITY));
    }

    //m_pFragWorld->CreateBox(Vector3(1.0f, 1.0f, 1.0f), 1.0f);

    fragNMAssetManager::Create();
    FRAGNMASSETMGR->Load(artFile, 3);

    //Matrix34 objectMatrix(M34_IDENTITY);
    Matrix34 objectMatrix(CreateRotatedMatrix(Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f)));

    const char* file = "$/fragments/GTA5_male_NM_actor/entity.type";
    PARAM_file.Get(file);

	m_Type = rage_new fragType;
    m_Type->LoadWithXML(file, physRig);

	// set bound friction
	int numBounds = m_Type->GetPhysics(0)->GetCompositeBounds()->GetNumBounds();
	for (int i = 0; i < numBounds; i++)
	{
		phMaterial &material = const_cast<phMaterial&>(m_Type->GetPhysics(0)->GetCompositeBounds()->GetBound(i)->GetMaterial(0));
		material.SetFriction(10.0f);
	}

    m_Inst = rage_new sampleFragInstNM(m_Type, objectMatrix);
    m_Inst->Insert(false);
	if (!m_Inst->GetCached())
	{
        m_Inst->PutIntoCache();
	}

#if EXTRA_INSTS > 0
    for (int extraGuy = 1; extraGuy < EXTRA_INSTS; ++extraGuy)
    {
        objectMatrix.d.x += 1.2f;
        objectMatrix.RotateX(0.05f);
        m_Insts[extraGuy] = rage_new sampleFragInstNM(m_Type, objectMatrix);
        m_Insts[extraGuy]->Insert(false);
        m_Insts[extraGuy]->PutIntoCache();
    }
#endif // EXTRA_INSTS

    //m_Inst2= new sampleFragInstNM(m_Type, M34_IDENTITY);
    //m_Inst2->Insert(false);
    BANK_ONLY(FRAGTUNE->AddArchetypeWidgets(m_Type));

    if (!m_Inst)
    {
        Quitf("Fragment type missing: %s", file);
    }

	const char* animName = "$/animation/GTA5-Ped/Get_Up_Slow.anim";
    PARAM_anim.Get(animName);
    m_LoadedAnimation = m_Animation = crAnimation::AllocateAndLoad(animName);

    NMBEHAVIORPOOL::Instantiate();

    // now load the behavior pool
    if ( NMBEHAVIORPOOL::GetInstance().LoadFile("$/naturalmotion/Behaviours.xml") )
    {
        Displayf( "Behavior Pool loaded %d behaviors", NMBEHAVIORPOOL::GetInstance().GetNumBehaviors() );
    }
    else
    {
        Quitf( "Behavior Pool load failure." );
    }
}

void NMViewer::ShutdownClient()
{
    delete m_AnimFrame;
    delete m_LoadedAnimation;

	if (m_Pin.IsValid())
	{
		PHCONSTRAINT->Remove(m_Pin);
		m_Pin.Reset();
	}

    m_Inst->Remove();
    delete m_Inst;
    delete m_Type;

    fragNMAssetManager::Destroy();

#if __BANK
    int count = m_NMBehaviorInstances.GetCount();
    for (int i = 0; i < count; ++i)
    {
        delete m_NMBehaviorInstances[i];
    }
    m_NMBehaviorInstances.clear();
#endif // __BANK

    NMBEHAVIORPOOL::Destroy();

    rmcSampleSimpleWorld::ShutdownClient();
}

void NMViewer::InitCamera()
{
	m_CamMgr.SetUseZUp(true);
    InitSampleCamera(Vector3(3.0f, -3.0f, 3.0f), ORIGIN);
}

#if __BANK
void NMViewer::AddWidgetsClient()
{
    rmcSampleSimpleWorld::AddWidgetsClient();

    m_BehaviorBank = &BANKMGR.CreateBank( "NaturalMotion" );
    {
        m_BehaviorBank->AddButton( "Load Behavior Instance", datCallback(MFA(NMViewer::BankLoadBehaviorInstance),this) );
        m_BehaviorBank->AddButton( "Save Behavior Instance", datCallback(MFA(NMViewer::BankSaveBehaviorInstance),this) );
        m_BehaviorBank->AddButton( "Load All Behavior Instances", datCallback(MFA(NMViewer::BankLoadBehaviorInstances),this) );
        m_BehaviorBank->AddButton( "Save All Behavior Instances", datCallback(MFA(NMViewer::BankSaveBehaviorInstances),this) );
        
        m_BehaviorBank->AddButton( "Trigger Behavior", datCallback(MFA(NMViewer::TriggerBehavior),this) );
		m_BehaviorBank->AddToggle( "Use Rage Active Pose", &m_UseRageActivePose );
        m_BehaviorBank->AddToggle( "Use NM Active Pose", &m_UseNMActivePose );
		m_BehaviorBank->AddSlider( "Active Pose Stiffness", &m_ActivePoseStiffness, 0.0f, 16.0f, 1.0f );

        int count = NMBEHAVIORPOOL::GetInstance().GetNumBehaviors();
        if ( count > 0 )
        {
            // create the NM Behavior Instances, one for each Behavior in the pool
            for (int i = 0; i < count; ++i)
            {            
                const NMBehavior *b = NMBEHAVIORPOOL::GetInstance().GetBehavior( i );
                NMBehaviorInst *bI = rage_new NMBehaviorInst( b->GetName(), b, b->GetDescription() );

                // This will provide us with initial values
                bI->VerifyParamValues();

                if ( strcmp(b->GetName(),"bodyBalance") == 0 )
                {
                    m_currentNMBehavior = i;
                }

                m_NMBehaviorInstances.Grow() = bI;
            }
            
            AddBehaviorListComboWidget();
            
            AddCurrentBehaviorWidgets();
        }
    }

    if ( m_Inst->GetType() )
    {
        m_AnimBank = &BANKMGR.CreateBank( "Animations" );

        m_AnimBank->AddToggle("Loop", &m_LoopWidget);
        m_AnimBank->AddButton("Play", datCallback(MFA(NMViewer::BankPlayAnimation), this));
        m_AnimBank->AddSlider("Speed", &m_FrameRate, 0.0f, 10.0f, 0.1f);
        m_AnimBank->AddButton("Load from file", datCallback(MFA(NMViewer::LoadAnimation), this));

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

        m_AnimBank->AddCombo( "Animation Name", &m_AnimIndex, numNames, m_AnimNames, datCallback(MFA(NMViewer::BankSelectAnimation),this));

        if (&m_Inst->GetType()->GetCommonDrawable()->GetShaderGroup())
        {
            m_Inst->GetType()->GetCommonDrawable()->GetShaderGroup().AddWidgets(BANKMGR.CreateBank("Shaders"));
        }
    }
}
#endif // __BANK

void NMViewer::Update()
{
    rmcSampleSimpleWorld::Update();

    if (m_Animation && m_pFragWorld->ShouldUpdate())
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
            if (!m_Inst->GetCached())
            {
                m_Inst->PutIntoCache();
            }

			// Initialize the active pose inst if needed
			if (m_UseRageActivePose || m_UseNMActivePose)
			{
				// Create the model inst if haven't done so
				if (!m_ActivePoseInst)
				{
					Matrix34 objectMatrix(CreateRotatedMatrix(Vector3(2.0f, 0.0f, 1.0f), Vector3(0.0f, 0.0f, PI)));
					m_ActivePoseInst = rage_new sampleFragInstNM(m_Type, objectMatrix);
					m_ActivePoseInst->SetDisableActivation(true);
					//m_Inst->SetBlockNMActivation(true);
					m_ActivePoseInst->Insert(false);
					if (!m_ActivePoseInst->GetCached())
						m_ActivePoseInst->PutIntoCache();
				}
				else if (!m_ActivePoseInst->GetCached())
				{
					m_ActivePoseInst->SetDisableActivation(true);
					m_ActivePoseInst->PutIntoCache();
				}

				// Have the animation pose the skeleton.
				// Note that this is only done if the instance is no longer active.
				if (m_ActivePoseInst->IsInLevel() && !PHLEVEL->IsActive(m_ActivePoseInst->GetLevelIndex()) && m_ActivePoseInst->GetCached())
				{
					m_Animation->CompositeFrame(m_AnimPhase, *m_AnimFrame, false, false);
					crSkeleton* skeleton = m_ActivePoseInst->GetSkeleton();
					m_AnimFrame->Pose(*skeleton);
					skeleton->Update();
					m_ActivePoseInst->PoseBoundsFromSkeleton(false, true);
				}
			}

			// Rage Active Pose
			if (m_UseRageActivePose)
			{
				// Advance the model's anim phase and use that frame to active pose the agent
				if (m_ActivePoseInst && m_Inst->GetCacheEntry()->GetHierInst()->body)
				{
					// Bump the ragdoll out of NM if haven't already
					if (!m_Inst->IsSimulatingAsNonNMRagdoll())
					{
						m_Inst->SetBlockNMActivation(true);
						PHSIM->DeactivateObject(m_Inst->GetLevelIndex());
						PHSIM->ActivateObject(m_Inst->GetLevelIndex());
					}

					// Add world constraints so that the actor doesn't fall over
					if (!m_Pin.IsValid())
					{
						phConstraintFixed::Params pinConstraint;
						pinConstraint.instanceA = m_Inst;
						pinConstraint.componentA = 10;
						m_Pin = PHCONSTRAINT->Insert(pinConstraint);
					}

					m_Inst->GetCacheEntry()->GetHierInst()->body->SetStiffness(m_ActivePoseStiffness);

					// allocate temp frame
					const crSkeletonData &rSkelData = m_Inst->GetSkeleton()->GetSkeletonData();
					crFrame *savedAnimPose = rage_new crFramePacked;
					savedAnimPose->InitCreateBoneAndMoverDofs(rSkelData, false);

					m_Inst->SetMusclesFromFrame(*m_AnimFrame, *savedAnimPose);

					// deallocate temp frame
					delete savedAnimPose;
				}
			}
			else if (m_UseNMActivePose)
			{
				// Advance the model's anim phase and use that frame to active pose the agent
				if (m_ActivePoseInst && m_Inst->GetCacheEntry()->GetHierInst()->body)
				{
					// Add world constraints so that the actor doesn't fall over
					if (!m_Pin.IsValid())
					{
						phConstraintFixed::Params pinConstraint;
						pinConstraint.instanceA = m_Inst;
						pinConstraint.componentA = 10;
						m_Pin = PHCONSTRAINT->Insert(pinConstraint);
					}

					m_Inst->SetComponentTMsFromSkeleton(*m_ActivePoseInst->GetSkeleton());

					ART::MessageParams params;
					params.addFloat("stiffness", m_ActivePoseStiffness);
					m_Inst->PostARTMessage("activePose", &params);
					m_Inst->ReportMovedBySim();
				}
			}
			else if (m_ActivePoseInst)
			{
				m_ActivePoseInst->Remove();
				delete m_ActivePoseInst;
				m_ActivePoseInst = NULL;
			}

			// Remove Pins if no longer needed (not active or not doing active pose)
			if (m_Pin.IsValid() && ((!m_UseRageActivePose && !m_UseNMActivePose) || 
				!m_Inst->GetCacheEntry()->GetHierInst()->body))
			{
				PHCONSTRAINT->Remove(m_Pin);
				m_Pin.Reset();
			}

			// Bump Non-NM ragdolls back into NM if rage active pose isn't active
			if (!m_UseRageActivePose && !m_UseNMActivePose && m_Inst->IsNMActivationBlocked() &&
				m_Inst->GetCacheEntry()->GetHierInst()->body)
			{
				m_Inst->SetBlockNMActivation(false);
				PHSIM->DeactivateObject(m_Inst->GetLevelIndex());
				PHSIM->ActivateObject(m_Inst->GetLevelIndex());
			}

#if EXTRA_INSTS > 0
            for (int extraInst = 1; extraInst < EXTRA_INSTS; ++extraInst)
            {
                if (!m_Insts[extraInst]->GetCached())
                {
                    m_Insts[extraInst]->PutIntoCache();
                }
                if (m_Insts[extraInst]->IsInLevel() && m_Insts[extraInst]->GetCached() &&
                    (m_UseNMActivePose || !PHLEVEL->IsActive(m_Insts[extraInst]->GetLevelIndex())))
                {
                    // Have the animation pose the skeleton.
                    // Note that this is only done if the instance is no longer active.
                    m_Animation->CompositeFrame(m_AnimPhase, *m_AnimFrame, false, false);
                    crSkeleton* skeleton = m_Insts[extraInst]->GetSkeleton();
                    m_AnimFrame->Pose(*skeleton);

                    fragHierarchyInst* hierInst = fragCacheManager::GetEntryFromInst(m_Insts[extraInst])->GetHierInst();
                    Assert(hierInst);
                    Matrix34 linkWorld;
                    const Mat34V* parentMatrix = &m_Insts[extraInst]->GetMatrix();

                    if(hierInst->body != NULL && PHLEVEL->IsActive(m_Insts[extraInst]->GetLevelIndex()))
                    {
                        int linkIndex = hierInst->articulatedCollider->GetLinkFromComponent(0);
                        phArticulatedBody* body = hierInst->body;

                        const fragType* type = m_Insts[extraInst]->GetType();
                        Assert(type);

                        linkWorld.Transpose(MAT34V_TO_MATRIX34(body->GetLink(linkIndex).GetMatrix()));
                        linkWorld.d.Set(body->GetLink(linkIndex).GetPosition());
                        linkWorld.Dot(RCC_MATRIX34(m_Insts[extraInst]->GetMatrix()));
                        linkWorld.DotTranspose(m_Inst->GetTypePhysics()->GetAllChildren()[0]->GetLinkAttachment());

                        parentMatrix = &RCC_MAT34V(linkWorld);
                    }

                    skeleton->SetParentMtx(parentMatrix);
                    skeleton->Update();
                    skeleton->SetParentMtx(&m_Insts[extraInst]->GetMatrix());

                    if (!PHLEVEL->IsActive(m_Insts[extraInst]->GetLevelIndex()))
                    {
                        m_Insts[extraInst]->PoseBoundsFromSkeleton(false, true);
                    }
                    else if (m_UseNMActivePose)
                    {
                        m_Inst->SetComponentTMsFromSkeleton(*skeleton);

                        ART::MessageParams params;
						params.addFloat("stiffness", m_ActivePoseStiffness);
                        m_Inst->PostARTMessage("activePose", &params);
                    }
                    m_Insts[extraInst]->ReportMovedBySim();
                }
            }
#endif // EXTRA_INSTS

            m_AnimPhase += TIME.GetSeconds() * m_FrameRate * m_pFragWorld->GetTimeWarp();

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
}

void NMViewer::DrawHelpClient()
{
    rmcSampleSimpleWorld::DrawHelpClient();

    char tmpString[100];
    formatf(tmpString, sizeof(tmpString), "broken: %s", m_Inst->GetBroken() ? "true" : "false");
    DrawString(tmpString, false);
    formatf(tmpString, sizeof(tmpString), "part broken: %s", m_Inst->GetPartBroken() ? "true" : "false");
    DrawString(tmpString, false);
}

void NMViewer::PreSimUpdate(float deltaTime)
{
#if DEBUG_ALT_UPDATE_ORDER
	static int count = 0;

	if (m_Inst->GetCached() && m_Inst->GetCacheEntry()->GetHierInst()->body)
	{
		++count;

		if (count == 5)
		{
			ART::MessageParams msg;
			msg.addBool("start", true);
			msg.addBool("useBodyTurn", false);
			m_Inst->PostARTMessage("bodyBalance", &msg);	
		}
		if (count == 50)
		{
			static float Xf = 0.0f;
			static float Yf = -130.0f;
			static float Zf = 0.0f;
			Vector3 impulse(Xf, Yf, Zf);

			ART::MessageParams msg;
			msg.addBool("start", true);
			msg.addInt("partIndex", 10);
			msg.addVector3("impulse", impulse.x, impulse.y, impulse.z);
			msg.addBool("localHitPointInfo", true);
			m_Inst->PostARTMessage("applyBulletImpulse", &msg);
		}
	}
	else
		count = 0;
#endif

	FRAGNMASSETMGR->StepPhase1(deltaTime);
}

void NMViewer::PostSimUpdate(float deltaTime)
{
    FRAGNMASSETMGR->StepPhase2(deltaTime);
}

#if __BANK

void NMViewer::BankSelectAnimation()
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

void NMViewer::BankPlayAnimation()
{
    m_AnimPhase = 0.0f;
}

void NMViewer::LoadAnimation()
{
    if (const char* animFile = BANKMGR.OpenFile("*.anim", false, "Animation (*.anim)"))
    {
        delete m_LoadedAnimation;

        m_LoadedAnimation = m_Animation = crAnimation::AllocateAndLoad(animFile);

        m_AnimIndex = 0;
        m_AnimPhase = 0.0f;
    }
}

void NMViewer::TriggerBehavior()
{
    NMBehaviorInst *bI = NULL;
    if ( m_currentNMBehavior > -1 )
    {
        bI = m_NMBehaviorInstances[m_currentNMBehavior];
    }

    if (m_Inst->IsInLevel() && PHLEVEL->IsActive(m_Inst->GetLevelIndex()) && m_Inst->GetCached())
    {
        sampleFragInstNM *pFragInstNM = dynamic_cast<sampleFragInstNM *>( m_Inst );
        if ( pFragInstNM != NULL )
        {
            pFragInstNM->SetBehaviorInst( bI );
            pFragInstNM->CreateAndPostARTMessage();
        }
    }
    else if ( bI )
    {
        Displayf( "Unable to trigger %s at this time.", bI->GetName() );
    }

#if EXTRA_INSTS > 0
    for (int extraInst = 1; extraInst < EXTRA_INSTS; ++extraInst)
    {
        if (m_Insts[extraInst]->IsInLevel() && PHLEVEL->IsActive(m_Insts[extraInst]->GetLevelIndex()) && m_Insts[extraInst]->GetCached())
        {
            sampleFragInstNM *pFragInstNM = dynamic_cast<sampleFragInstNM *>( m_Insts[extraInst] );
            if ( pFragInstNM != NULL )
            {
                pFragInstNM->SetBehaviorInst( bI );
                pFragInstNM->CreateAndPostARTMessage();
            }
        }
    }
#endif // EXTRA_INSTS
}

void NMViewer::AddCurrentBehaviorWidgets()
{
    if ( m_BehaviorBank == NULL )
    {
        return;
    }

    // remove old widgets
    if ( m_previousNMBehavior > -1 )
    {
        NMBehaviorInst *bI = m_NMBehaviorInstances[m_previousNMBehavior];
        bI->RemoveWidgets( *m_BehaviorBank );
    }

    // add new widgets
    if ( m_currentNMBehavior > -1 )
    {
        NMBehaviorInst *bI = m_NMBehaviorInstances[m_currentNMBehavior];
        bI->AddWidgets( *m_BehaviorBank );

        // set current behavior on our fragInstNMs
        sampleFragInstNM *pFragInstNM = dynamic_cast<sampleFragInstNM *>( m_Inst );
        if ( pFragInstNM != NULL )
        {
            pFragInstNM->SetBehaviorInst( bI );
        }

#if EXTRA_INSTS > 0
        for (int extraInst = 1; extraInst < EXTRA_INSTS; ++extraInst)
        {
            sampleFragInstNM *pFragInstNM = dynamic_cast<sampleFragInstNM *>( m_Insts[extraInst] );
            if ( pFragInstNM != NULL )
            {
                pFragInstNM->SetBehaviorInst( bI );
            }
        }
#endif // EXTRA_INSTS        
    }

    m_previousNMBehavior = m_currentNMBehavior;
}

void NMViewer::BankSaveBehaviorInstance()
{
    char file[256];
    memset(file, 0, 256);

    if ( BANKMGR.OpenFile(file,256,"*.xml",true,"Behavior Instance (*.xml)") )
    {
        NMBehaviorInst *bI = m_NMBehaviorInstances[m_currentNMBehavior];

        // save the parTree
        const char *ext = ASSET.FindExtensionInPath( file );
        if ( ext != NULL )
        {
            PARSER.SaveObject( file, "", bI, parManager::XML );
        }
        else
        {
            PARSER.SaveObject( file, "xml", bI, parManager::XML );
        }

        // save as new one at the end of our list
        m_NMBehaviorInstances.Grow() = bI;

        // replace it's position in the array with a fresh one
        m_NMBehaviorInstances[m_currentNMBehavior] = rage_new NMBehaviorInst( bI->GetBehavior()->GetName(), 
            bI->GetBehavior(), bI->GetBehavior()->GetDescription() );
        m_NMBehaviorInstances[m_currentNMBehavior]->VerifyParamValues();

        // remove current Behavior group
        m_currentNMBehavior = -1;
        AddCurrentBehaviorWidgets();

        // re-build combo widget
        AddBehaviorListComboWidget();

        // re-add current behavior group
        m_currentNMBehavior = m_NMBehaviorInstances.GetCount() - 1;
        AddCurrentBehaviorWidgets();
    }
}

void NMViewer::BankLoadBehaviorInstance()
{
    char file[256];
    memset(file, 0, 256);

    if ( BANKMGR.OpenFile(file,256,"*.xml",false,"Behavior Instance (*.xml)") )
    {
        NMBehaviorInst *bI = NULL;
        const char *ext = ASSET.FindExtensionInPath( file );
        if ( ext != NULL )
        {
            PARSER.LoadObjectPtr( file, "", bI );
        }
        else
        {
            PARSER.LoadObjectPtr( file, "xml", bI );
        }

        if ( bI == NULL )
        {
            return;
        }
         
        m_NMBehaviorInstances.Grow() = bI;

        // remove current Behavior group
        m_currentNMBehavior = -1;
        AddCurrentBehaviorWidgets();

        // re-build combo widget
        AddBehaviorListComboWidget();

        // re-add current behavior group
        m_currentNMBehavior = m_NMBehaviorInstances.GetCount() - 1;
        AddCurrentBehaviorWidgets();
    }
}

void NMViewer::BankSaveBehaviorInstances()
{
    char file[256];
    memset(file, 0, 256);

    if ( BANKMGR.OpenFile(file,256,"*.xml",true,"Behavior Instances (*.xml)") )
    {
        // build the parTree
        parTree *pTree = rage_new parTree();
        parTreeNode *pRoot = pTree->CreateRoot();
        pRoot->GetElement().SetName( "BehaviorInstances" );

        int count = m_NMBehaviorInstances.GetCount();
        for (int i = 0; i < count; ++i)
        {
            parTreeNode *pNode = PARSER.BuildTreeNode( m_NMBehaviorInstances[i] );
            if ( pNode != NULL )
            {
                pNode->AppendAsChildOf( pRoot );
            }
        }

        // save the parTree
        const char *ext = ASSET.FindExtensionInPath( file );
        if ( ext != NULL )
        {
            PARSER.SaveTree( file, "", pTree, parManager::XML );
        }
        else
        {
            PARSER.SaveTree( file, "xml", pTree, parManager::XML );
        }
    }
}

void NMViewer::BankLoadBehaviorInstances()
{
    char file[256];
    memset(file, 0, 256);

    if ( BANKMGR.OpenFile(file,256,"*.xml",false,"Behavior Instances (*.xml)") )
    {
        parTree *pTree = NULL;
        const char *ext = ASSET.FindExtensionInPath( file );
        if ( ext != NULL )
        {
            pTree = PARSER.LoadTree( file, "" );
        }
        else
        {
            pTree = PARSER.LoadTree( file, "xml" );
        }

        if ( pTree == NULL )
        {
            return;
        }

        parTreeNode *pRoot = pTree->GetRoot();
        if ( strcmp("BehaviorInstances",pRoot->GetElement().GetName()) != 0 )
        {
            return;
        }

        parTreeNode *pNode = pRoot->GetChild();
        while ( pNode != NULL )
        {
            NMBehaviorInst *bI = NULL;
            if ( PARSER.LoadObjectPtr(pNode,bI) )
            {
                m_NMBehaviorInstances.Grow() = bI;
            }

            pNode = pNode->GetSibling();
        }

        // remove current Behavior group
        int curr = m_currentNMBehavior;
        m_currentNMBehavior = -1;
        AddCurrentBehaviorWidgets();

        // re-build combo widget
        AddBehaviorListComboWidget();

        // re-add current behavior group
        m_currentNMBehavior = curr;
        AddCurrentBehaviorWidgets();
    }
}

void NMViewer::AddBehaviorListComboWidget()
{
    if ( m_BehaviorBank == NULL )
    {
        return;
    }

    if ( m_BehaviorListComboBox != NULL )
    {
        m_BehaviorBank->Remove( *m_BehaviorListComboBox );
    }

    int count = m_NMBehaviorInstances.GetCount();
    if ( count > 0 )
    {
        const char** behaviorNames = rage_new const char*[count];
        for (int i = 0; i < count; ++i)
        {
            behaviorNames[i] = m_NMBehaviorInstances[i]->GetName();
        }        

        m_BehaviorListComboBox = m_BehaviorBank->AddCombo( "Behaviors", &m_currentNMBehavior, count, behaviorNames, datCallback(MFA(NMViewer::AddCurrentBehaviorWidgets),this) );
    }
}
#endif // __BANK

void NMViewer::PartsBrokeOff(fragInst* oldInst,
                          atFixedBitSet<phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS>& brokenParts,
                          fragInst* newInst)
{
    phDemoWorld::GetActiveDemo()->FixGrabForBreak(oldInst, brokenParts, newInst);
}

} // namespace ragesamples
