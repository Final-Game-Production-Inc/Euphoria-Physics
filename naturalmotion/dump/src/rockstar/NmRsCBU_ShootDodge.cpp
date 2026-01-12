/*
* Copyright (c) 2005-2010 NaturalMotion Ltd. All rights reserved.
*
* Not to be copied, adapted, modified, used, distributed, sold,
* licensed or commercially exploited in any manner without the
* written consent of NaturalMotion.
*
* All non public elements of this software are the confidential
* information of NaturalMotion and may not be disclosed to any
* person nor used for any purpose not expressly approved by
* NaturalMotion in writing.
*
*/

#include "NmRsInclude.h"
#include "NmRsCBU_ShootDodge.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"
#include "NmRsEngine.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_SpineTwist.h"
#include "NmRsCBU_PointGun.h"
#include "NmRsCBU_SDCatchFall.h"
#include "NmRsCBU_CatchFall.h"
#include "NmRsCBU_HeadLook.h"
#include "NmRsCBU_BodyFoetal.h"
#include "NmRsCBU_BodyWrithe.h"

#include "fragment/instance.h"

#if ALLOW_DEBUG_BEHAVIOURS
#include "NmRsCBU_ExportCTM.h"
#endif

#define NM_SHOOTDODGE_EXPORT_CTM 0
#define NM_SHOOTDODGE_NO_SPINE_CONTROL 1 // disable additive spine effects - itms only.

namespace ART
{
    NmRsCBUShootDodge::NmRsCBUShootDodge(ART::MemoryManager* services) :
  CBUTaskBase(services, bvid_shootDodge)
  {
    initialiseCustomVariables();
  }

  NmRsCBUShootDodge::~NmRsCBUShootDodge()
  {

  }

  void NmRsCBUShootDodge::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_timer = 0.f;
    m_oriStiffness = 0.f;
    m_oriDamping = 0.f;
    m_oriSplit = 0.f;
    m_orientation = kFront;
    m_collisionOrientation = kHead;
    m_collisionFeedbackSent = false;
    m_stableFeedbackSent = false;
    m_pelvisRotVelCache.Zero();
    m_spine3RotVelCache.Zero();
    m_timeWarpScale = 1.f;
    m_launchDirection.Zero();
    m_orientationTargetOffset.Zero();
    m_trunkCOMVel.Zero();
    m_trunkCOMAngVel.Zero();
    m_trunkCOMacc.Zero();
    m_trunkAngMom.Zero();
    m_groundVelocity.Zero();
    m_collisionDirection.Zero();

    m_states[kLaunch] = NULL;
    m_states[kInAir] = &m_inAirState;
    m_states[kCollideGround] = &m_collideGroundState;
    m_states[kCollideWall] = &m_collideWallState;
    m_states[kOnGround] = &m_onGroundState;
    m_states[kBail] = &m_BailState;

    m_pointingStates[kPointing] = &m_pointingState;
    m_pointingStates[kNotPointing] = &m_notPointingState;
    m_pointingStates[kReloading] = &m_reloadState;

    m_bracingStates[kBracing] = &m_bracingState;
    m_bracingStates[kDefending] = &m_defendingState;
    m_bracingStates[kIdling] = &m_idlingState;

    m_parameters.targetValid = false;

    m_isSupported = false;
    m_supported = 0.f;
    m_pelvisHeight = 0.f;

#if NM_SHOOTDODGE_SMALL_FEET
    m_footSizeCache[0].Zero();
    m_footSizeCache[1].Zero();
#endif

#if NM_SHOOTDODGE_BUTTERWORTH
    m_acceleration[0].Zero();
    m_acceleration[1].Zero();
    m_acceleration[2].Zero();
    m_accelerationIndex = 0;
    m_accelerationValid = false;
#endif

#if ART_ENABLE_BSPY
    m_mask = bvmask_None;
#endif
  }

  void NmRsCBUShootDodge::initStateMachine(FSM* fsm, SDStateBase** states, int numStates, SDStateBase* globalState /* = NULL */)
  {
    Assert(fsm);
    Assert(states);
    Assert(m_character);
    int i;
    for(i = 0; i < numStates; ++i)
    {
      // Assert(states[i]);
      if(states[i])
      {
        states[i]->setContext(this);
        states[i]->setCharacter(m_character);
        states[i]->setFSM(fsm);
      }
    }
    if(globalState)
    {
      globalState->setContext(this);
      globalState->setCharacter(m_character);
      globalState->setFSM(fsm);
      // run global state entry.
      globalState->Enter();
    }
    fsm->SetCurrentState(NULL);
    fsm->SetGlobalState(globalState);
    fsm->SetPreviousState(NULL);
  }

  void NmRsCBUShootDodge::onActivate()
  {
#if ALLOW_DEBUG_BEHAVIOURS & NM_SHOOTDODGE_EXPORT_CTM
    NmRsCBUExportCTM* cbuExportCTM = (NmRsCBUExportCTM*)m_character->getTask(bvid_exportCTM);
    Assert(cbuExportCTM);
    if(!cbuExportCTM->isActive())
      cbuExportCTM->activate();
#endif

    initStateMachine(&m_stateMachine, m_states, kNumStates, &m_sdGlobalState);
    initStateMachine(&m_pointingStateMachine, m_pointingStates, kNumPointingStates, &m_pointingGlobalState);
    initStateMachine(&m_bracingStateMachine, m_bracingStates, kNumBracingStates, &m_bracingGlobalState);

    // store orientation control parameters for smoothing.
    m_oriStiffness = m_parameters.oriStiff;
    m_oriDamping = m_parameters.oriDamp;
    m_oriSplit = m_parameters.oriSplit;

    // switch off zmp posture control
    m_character->m_posture.useZMP = false;

    m_collisionFeedbackSent = false;

    // initialize trunk velocity to current.
    rage::Vector3 trunkCOMAngVel, trunkAngMom;
    rage::Matrix34 trunkCOM;
    calculateCoM(
      &trunkCOM,
      &m_trunkCOMVel,
      &trunkCOMAngVel,
      &trunkAngMom,
      bvmask_Full);

    // store launch direction.
    // zero vertical component.
    m_launchDirection.Set(m_character->m_COMvel);
    m_launchDirection.z = 0.f;
    m_launchDirection.Normalize();

    // reset effector calibrations
    m_character->recalibrateAllEffectors();
    m_character->getLeftArmSetup()->getWrist()->setMuscleStiffness(10.f);
    m_character->getRightArmSetup()->getWrist()->setMuscleStiffness(10.f);

    // set up gravity opposition
    callMaskedEffectorFunctionFloatArg(m_character, "fb", 0.0f, &NmRs1DofEffector::setOpposeGravity, &NmRs3DofEffector::setOpposeGravity);

    // store trunk angular velocities.
    const SpineSetup* spine = m_character->getSpineSetup();
    m_pelvisRotVelCache.Set(spine->getPelvisPart()->getAngularVelocity());
    m_spine3RotVelCache.Set(spine->getSpine3Part()->getAngularVelocity());

    // start spine twist task.
    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist*)m_cbuParent->m_tasks[bvid_spineTwist];
    Assert(spineTwistTask);
    if (!spineTwistTask->isActive())
      spineTwistTask->activate(m_taskParent);
    spineTwistTask->setSpineTwistAllwaysTwist(true);

    // activate head look
    if(m_parameters.useHeadLook)
    {
    NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    if (!headLookTask->isActive())
    {
      headLookTask->updateBehaviourMessage(NULL);
#if NM_SHOOTDODGE_NO_SPINE_CONTROL
      headLookTask->setBvMask(bvmask_Neck | bvmask_Head);
      headLookTask->activate();
#else
      headLookTask->activate(m_taskParent);
#endif
    }
    }

    // set current state. ideally this should always be kLaunch.
    m_stateMachine.ChangeState(enumToState((SDState)m_parameters.state));

    // set up pointing state.
    m_pointingStateMachine.ChangeState(&m_pointingState);

    // set up bracing state.
    m_bracingStateMachine.ChangeState(&m_idlingState);

#if NM_SHOOTDODGE_SMALL_FEET
    // try making foot boxes smaller as an alternative to the above.

    // left
    int index = m_character->getLeftLegSetup()->getFoot()->getPartIndex();
    rage::phBound* bound = m_character->getArticulatedWrapper()->getBound(index);
    Assert(bound);
    Assert(bound->GetType() == rage::phBound::BOX);
    rage::phBoundBox* boundBox = (rage::phBoundBox*)bound;
    rage::Vector3 boxSize(VEC3V_TO_VECTOR3(boundBox->GetBoxSize()));
    m_footSizeCache[0].Set(boxSize); // cache size for later restore.
    boxSize.Scale(NM_SHOOTDODGE_SMALL_FEET_SCALE);
    boundBox->SetBoxSize(VECTOR3_TO_VEC3V(boxSize));

    // right
    index = m_character->getRightLegSetup()->getFoot()->getPartIndex();
    bound = m_character->getArticulatedWrapper()->getBound(index);
    Assert(bound);
    Assert(bound->GetType() == rage::phBound::BOX);
    boundBox = (rage::phBoundBox*)bound;
    boxSize.Set(VEC3V_TO_VECTOR3(boundBox->GetBoxSize()));
    m_footSizeCache[1].Set(boxSize);
    boxSize.Scale(NM_SHOOTDODGE_SMALL_FEET_SCALE);
    boundBox->SetBoxSize(VECTOR3_TO_VEC3V(boxSize));
#endif

#if NM_SHOOTDODGE_BUTTERWORTH
    m_accelerationValid = false;
#endif
  }

  void NmRsCBUShootDodge::onDeactivate()
  {
#if ALLOW_DEBUG_BEHAVIOURS & NM_SHOOTDODGE_EXPORT_CTM
    NmRsCBUExportCTM* cbuExportCTM = (NmRsCBUExportCTM*)m_character->getTask(bvid_exportCTM);
    Assert(cbuExportCTM);
    if(cbuExportCTM->isActive())
      cbuExportCTM->deactivate();
#endif

    m_bracingStateMachine.CurrentState()->Exit();
    m_pointingStateMachine.CurrentState()->Exit();
    m_stateMachine.CurrentState()->Exit();

#if NM_SHOOTDODGE_SMALL_FEET
    // restore foot sizes from cache.
    // left
    int index = m_character->getLeftLegSetup()->getFoot()->getPartIndex();
    rage::phBound* bound = m_character->getArticulatedWrapper()->getBound(index);
    Assert(bound);
    Assert(bound->GetType() == rage::phBound::BOX);
    rage::phBoundBox* boundBox = (rage::phBoundBox*)bound;
    boundBox->SetBoxSize(VECTOR3_TO_VEC3V(m_footSizeCache[0]));
    // right
    index = m_character->getRightLegSetup()->getFoot()->getPartIndex();
    bound = m_character->getArticulatedWrapper()->getBound(index);
    Assert(bound);
    Assert(bound->GetType() == rage::phBound::BOX);
    boundBox = (rage::phBoundBox*)bound;
    boundBox->SetBoxSize(VECTOR3_TO_VEC3V(m_footSizeCache[1]));
#endif

    // ensure friction and elasticity are reset.
    m_character->setFrictionMultiplier(1.f);
    m_character->setElasticityMultiplier(1.f);

    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUShootDodge::onTick(float timeStep)
  {
#if ART_ENABLE_BSPY
    m_mask = bvmask_None;
#endif

    // compute time warp scale.
    if(m_parameters.timeWarpActive)
      m_timeWarpScale = rage::Sqrtf(m_parameters.timeWarpStrengthScale / (30.f * timeStep));
    else
      m_timeWarpScale = 1.f;

    // set stiffness and damping.
    m_character->setBodyStiffness(m_parameters.trunkStiffness*m_timeWarpScale, m_parameters.trunkDamping, "uk");
    m_character->setBodyStiffness(m_parameters.headStiffness*m_timeWarpScale, m_parameters.headDamping, "un");
		setArmStrength(
			m_character->getLeftArmSetup(),
			m_parameters.notAimingArmStiffness*m_timeWarpScale,
			m_parameters.notAimingArmDamping,
			m_parameters.notAimingArmTaper
			);
		setArmStrength(
			m_character->getRightArmSetup(),
			m_parameters.notAimingArmStiffness*m_timeWarpScale,
			m_parameters.notAimingArmDamping,
			m_parameters.notAimingArmTaper
			);
		setLegStrength(
			m_character->getLeftLegSetup(),
			m_parameters.legStiffness*m_timeWarpScale,
			m_parameters.legDamping,
			m_parameters.legTaper
			);
		setLegStrength(
			m_character->getRightLegSetup(),
			m_parameters.legStiffness*m_timeWarpScale,
			m_parameters.legDamping,
			m_parameters.legTaper
			);

    m_character->getLeftArmSetup()->getWrist()->setMuscleStiffness(m_parameters.wristMuscleStiffness);
    m_character->getRightArmSetup()->getWrist()->setMuscleStiffness(m_parameters.wristMuscleStiffness);

#if 1
    // try applying grav comp to all parts.
    for(int i = 0; i < m_character->getNumberOfEffectors(); ++i)
      m_character->getEffector(i)->setOpposeGravity(1.f);
#endif

    if(m_parameters.useHeadLook)
    {
    // set head look stiffness and damping.
    NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    headLookTask->m_parameters.m_stiffness = m_parameters.headStiffness*m_timeWarpScale;
    headLookTask->m_parameters.m_damping = m_parameters.headDamping;
    }

#if NM_SHOOTDODGE_BUTTERWORTH
    updateAccelerationSmoothing(timeStep);
#endif

    // probe down
    rage::Vector3 interPos, interNormal;
    float distance = m_parameters.supportedHeightThreshold;
    if(m_stateMachine.CurrentState() != m_states[kOnGround])
    distance = 5.f; // increase probe range to detect bail condition if not already on ground.
    probeDown(&interPos, &interNormal, &m_pelvisHeight, distance);

    m_supported -= 0.1f;
    rage::Vector3 probeStart, normal;
    probeStart.Set(m_character->getSpineSetup()->getSpine1Part()->getPosition());
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
    m_character->bspyDrawLine(probeStart, probeStart-probeDist*m_character->m_gUp, rage::Vector3(1,0,1));
#endif

    // update velocity and acceleration measures.
    rage::Vector3 trunkCOMVel;
    rage::Matrix34 trunkCOM;
    calculateCoM(
      &trunkCOM,
      &trunkCOMVel,
      &m_trunkCOMAngVel,
      &m_trunkAngMom,
      bvmask_Pelvis | bvmask_Spine0 | bvmask_Spine1 | bvmask_Spine2 | bvmask_Spine3);

    // part velocity is currenrtly unreliable while hard-keyed
    // so we will grab the com vel from the root itms.
    if(m_hardKeyed && m_character->isPartInMask(m_parameters.hardKeyMask, 0))
    {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
      bspyLogf(info, L"!NmRsCBUShootDodge taking com vel from itms!");
#endif
      int incomingComponentCount = 0;

      rage::Matrix34 *itPtr = 0;
      rage::Matrix34 *prevItPtr = 0;

      IncomingTransformStatus itmStatusFlags = kITSNone;
      m_character->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, kITSourceCurrent);
      m_character->getIncomingTransforms(&prevItPtr, itmStatusFlags, incomingComponentCount, kITSourcePrevious);
      const int rootIndex = 0;
      if (itPtr != 0 && prevItPtr != 0 && incomingComponentCount > rootIndex)
      {
        rage::Matrix34 fromTmRage = prevItPtr[rootIndex];
        rage::Matrix34 toTmRage = itPtr[rootIndex];

        fromTmRage.Transpose();
        toTmRage.Transpose();
        rage::Matrix34 deltaPosition;
        deltaPosition.Subtract(toTmRage, fromTmRage);
        NmRsGenericPart* part = m_character->getGenericPartByIndex(0);
        if(part)
        {
          deltaPosition.Transform(part->getBound()->GetCGOffset(), trunkCOMVel);
          trunkCOMVel.Scale(1.f/timeStep);
        }
      }
    }

    m_velDotLaunchDir = m_launchDirection.Dot(m_trunkCOMVel);
    m_trunkCOMacc.Set(trunkCOMVel - m_trunkCOMVel);
    m_trunkCOMacc.Scale(1.f/timeStep);
    m_accelDotLaunchDir = m_launchDirection.Dot(m_trunkCOMacc);

    // cache COM velocity for use next tick.
    m_trunkCOMVel = trunkCOMVel;

    // update ground velocity.
    rage::phLevelNew* level = m_character->getLevel();
    Assert(level);
    m_groundVelocity.Zero();
    if(m_parameters.groundInstance != -1 && level->IsInLevel(m_parameters.groundInstance))
    {
      rage::phInst* pInst = level->GetInstance(m_parameters.groundInstance);
      if(pInst)
      {
        rage::Vector3 position, groundVelocity;
        position = m_character->getSpineSetup()->getPelvisPart()->getPosition();
        pInst->GetExternallyControlledLocalVelocity(position, m_groundVelocity);
#if ART_ENABLE_BSPY
        rage::Vector3 pos, vel, angVel;
        pos = VEC3V_TO_VECTOR3(pInst->GetPosition());
        pInst->GetExternallyControlledVelocity(vel);
        pInst->GetExternallyControlledAngVelocity(angVel);
        bspyScratchpad(m_character->getBSpyID(), "tram", pos);
        bspyScratchpad(m_character->getBSpyID(), "tram", vel);
        bspyScratchpad(m_character->getBSpyID(), "tram", angVel);
#endif
      }   
    }

    // collision direction
    rage::Matrix34 pelvisTM;
    m_character->getSpineSetup()->getSpine3Part()->getMatrix(pelvisTM);
    m_collisionDirection.Set(m_parameters.collisionPoint);
    m_collisionDirection.Subtract(pelvisTM.d);
    m_collisionDirection.NormalizeSafe();
#if ART_ENABLE_BSPY
    m_character->bspyDrawLine(pelvisTM.d, pelvisTM.d+m_collisionDirection, rage::Vector3(1,1,0));
#endif

    // this support metric is suspect and produces false-positives
    // when COM passes over objects.
    // try replacing with m_supportedParts & someMask...
    if(isSupported())
    {
      rage::Vector3 supportCenter, torsoCenter;
      torsoCenter.Set(m_character->getSpineSetup()->getSpine1Part()->getPosition());
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
      m_character->bspyDrawPoint(torsoCenter, 0.05f, rage::Vector3(0,0,1));
#endif
      m_character->getSupportCenter(&supportCenter);
      supportCenter.z = torsoCenter.z; // remove vertical component.
      //if(supportCenter.Dist(torsoCenter) < 0.25f)
        m_supported += 0.2f;
    }

    else if(m_pelvisHeight < m_parameters.supportedHeightThreshold)
    {
      m_supported += 0.2f;
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
      m_character->bspyDrawPoint(probeStart-probeDist*time*m_character->m_gUp, 0.025f, rage::Vector3(1,0,0));
#endif
    }
    m_supported = rage::Clamp(m_supported, 0.f, 1.f);
    if(m_supported > 0.7f)
      m_isSupported = true;
    else if(m_isSupported && m_supported < 0.3f)
      m_isSupported = false;

    m_timer += m_character->getLastKnownUpdateStep();

    // store zero pose.
    m_character->storeZeroPoseAllEffectors();

    // store character orientation
    m_orientation = getOrientation();
    m_collisionOrientation = getCollisionOrientation();

    // if we have collision data, get collision orientation
    if(m_parameters.collisionReaction != kCollisionTypeNone)
      m_orientation = getOrientation();
    
    // scale restitution and friction to tune bounce and slide
    m_character->setElasticityMultiplier(m_parameters.restitutionScale);
    m_character->setFrictionMultiplier(m_parameters.frictionScale);

    // update state machine.
    // set current state. will ignore if new state is the same as the current one.
    m_stateMachine.ChangeState(enumToState((SDState)m_parameters.state));
    m_stateMachine.Update(timeStep);

    // update pointing state machine.
    m_pointingStateMachine.Update(timeStep);

    // pass some params through to Point Gun.
    passThroughPointGunParams();

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    if(m_mask)
    {
      m_character->setSkeletonVizRoot(10);
      m_character->setSkeletonVizMode(NmRsCharacter::kSV_DesiredAngles);
      m_character->setSkeletonVizMask(m_mask);
    }
    else
    {
      m_character->setSkeletonVizMode(NmRsCharacter::kSV_None);
    }
#endif

#if ART_ENABLE_BSPY & NM_SHOOTDODGE_SMALL_FEET
    // inform bspy about changed foot bounds.
    NmRsArticulatedWrapper* articulatedWrapper = m_character->getArticulatedWrapper();
    Assert(articulatedWrapper);
    NmRsGenericPart* part = m_character->getLeftLegSetup()->getFoot();
    Assert(part);
    rage::phBound* bound = (rage::phBound*)part->getBound();
    if (bound)
    {
      rage::Matrix34 instTm, tm;
      part->getMatrix(instTm);
      rage::Matrix34* linkMats = articulatedWrapper->getLinkAttachmentMatrices();
      rage::Matrix34 linkMat(linkMats[part->getPartIndex()]);
      tm.Dot(linkMat, instTm); // should be using this?
      m_character->bspyDrawCoordinateFrame(0.1f, tm);
      rage::phInst* inst = articulatedWrapper->getArticulatedPhysInstance();
      m_character->bSpyProcessDynamicBoundOnContact(inst, bound, instTm, m_character->getID());
    }
    part = m_character->getRightLegSetup()->getFoot();
    Assert(part);
    bound = (rage::phBound*)part->getBound();
    if (bound)
    {
      rage::Matrix34 instTm, tm;
      part->getMatrix(instTm);
      rage::Matrix34* linkMats = articulatedWrapper->getLinkAttachmentMatrices();
      rage::Matrix34 linkMat(linkMats[part->getPartIndex()]);
      tm.Dot(instTm, linkMat); // should be using this?
      rage::phInst* inst = articulatedWrapper->getArticulatedPhysInstance();
      m_character->bSpyProcessDynamicBoundOnContact(inst, bound, instTm, m_character->getID());
    }
#endif

    return eCBUTaskComplete;
  }

  void NmRsCBUShootDodge::doOrientationControl()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::doOrientationControl()");
#endif

    float smoove = m_parameters.velSmooth; // careful, framerate-dependent.
    m_oriStiffness *= smoove;
    m_oriStiffness += (1-smoove) * m_parameters.oriStiff;
    m_oriDamping *= smoove;
    m_oriDamping += (1-smoove) * m_parameters.oriDamp;
    m_oriSplit *= smoove;
    m_oriSplit += (1-smoove) * m_parameters.oriSplit;

    const SpineSetup* spine = m_character->getSpineSetup();
    orientationControl(
      spine->getPelvisPart(),
      m_oriStiffness * m_oriStiffness * m_timeWarpScale,
      -2.f * m_oriStiffness * m_oriDamping);
  }

  // replacement for pdPartOrientationToITM. ShootDodge has some specific
  // requirements.
  void NmRsCBUShootDodge::orientationControl(
    NmRsGenericPart *part,
    float kp,
    float kd)
  {
    // get part TM
    Assert(part);
    rage::Matrix34 partTM;
    part->getMatrix(partTM);

    // get target ITM
    rage::Matrix34 currentITM, lastITM;
    m_character->getITMForPart(part->getPartIndex(), &currentITM, kITSourceCurrent);

#if ART_ENABLE_BSPY && 1
    m_character->bspyDrawCoordinateFrame(0.1f, currentITM);
    m_character->bspyDrawCoordinateFrame(0.05f, partTM);
#endif

    // get ITM angular velocity.
    // if we have a previous ITM, compute target rotation velocity
    // rotational velocity of the target is typically negligible
    // and can be ignored for speed if necessary
    rage::Vector3 velITM; velITM.Zero();
    if(m_character->getITMForPart(part->getPartIndex(), &lastITM, kITSourcePrevious))
    {
      rage::Matrix34 diffM;
      lastITM.Transpose();
      diffM.Dot(currentITM, lastITM);
      rage::Quaternion diffQuat;
      diffM.ToQuaternion(diffQuat);
      float angle;
      diffQuat.ToRotation(velITM, angle);
      velITM.Scale(angle);
      velITM.Scale(1.f/m_character->getLastKnownUpdateStep());
#if ART_ENABLE_BSPY
      m_character->bspyDrawLine(currentITM.d, currentITM.d+velITM, rage::Vector3(1,1,0));
      m_character->bspyDrawPoint(currentITM.d+velITM, 0.025f, rage::Vector3(1,0,0));
#endif
    }

    // compute error
    rage::Vector3 error;
    {
      rage::Matrix34 diffM, tmCopy;
      tmCopy.Transpose(currentITM);
      diffM.Dot(tmCopy, partTM);
      rage::Quaternion diffQuat;
      diffM.ToQuaternion(diffQuat);

      float angle = 0;
      diffQuat.ToRotation(error, angle);
      if(angle > PI)
        angle -= 2.f*PI;
      else if(angle < -PI)
        angle += 2.f*PI;
      error.Scale(-angle);
#if ART_ENABLE_BSPY & 0
      m_character->bspyDrawLine(part->getPosition(), part->getPosition() + error, rage::Vector3(1.f,0.5f,0.f));
#endif
    }

    // add any collision-related offset.
    error.Add(m_orientationTargetOffset);
#if ART_ENABLE_BSPY & 0
    m_character->bspyDrawLine(part->getPosition(), part->getPosition() + m_orientationTargetOffset, rage::Vector3(0.5f,0.5f,0.5f));
#endif

    // compute dError.
    rage::Vector3 dError = m_character->m_COMrotvel;
#if ART_ENABLE_BSPY
      m_character->bspyDrawLine(currentITM.d, currentITM.d+dError, rage::Vector3(1,1,0));
      m_character->bspyDrawPoint(currentITM.d+velITM, 0.025f, rage::Vector3(0,1,0));
#endif
    // subtract ITM angular velocity.
    dError -= velITM;
#if ART_ENABLE_BSPY
      m_character->bspyDrawLine(currentITM.d, currentITM.d+dError, rage::Vector3(1,1,0));
      m_character->bspyDrawPoint(currentITM.d+velITM, 0.025f, rage::Vector3(0,0,1));
#endif

    rage::phArticulatedBody *body = m_character->getArticulatedWrapper()->getArticulatedBody();
    NmRsGenericPart* p = 0;
    float mass = 0;
    float totalMass = m_character->getTotalMass();
    rage::Vector3 impulse, torqueImpulse, pos;
    
    // scale and sum error and derror components.
    error.Scale(kp);
    error.AddScaled(dError, kd);

    // torque root part.
    torqueImpulse.Set(error * m_character->getLastKnownUpdateStep());
    part->applyTorqueImpulse(torqueImpulse);

    // apply mass-scaled impulse to all parts.
    int i;
    for(i = 0; i < m_character->getNumberOfParts(); ++i)
    {
      p = m_character->getGenericPartByIndex(i);
      rage::phArticulatedBodyPart& bp = body->GetLink(i);
      mass = bp.GetMass().Getf();
      pos = p->getPosition();
      pos.Subtract(m_character->m_COM);
      impulse.Cross(error, pos);
      impulse.Scale(m_character->getLastKnownUpdateStep() * mass / totalMass * m_parameters.oriSplit);
      p->applyImpulse(impulse, p->getPosition());
    }
  }

  // conversion functions for state list
  NmRsCBUShootDodge::SDStateBase* NmRsCBUShootDodge::enumToState(SDState stateEnum)
  {
    Assert(stateEnum >= 0 && stateEnum < kNumStates);
    Assert(m_states[stateEnum]);
    return m_states[stateEnum];
  }

  NmRsCBUShootDodge::SDState NmRsCBUShootDodge::stateToEnum(NmRsCBUShootDodge::SDStateBase* state)
  {
    int i;
    for(i = 0; i < kNumStates; ++i)
      if(m_states[i] == state)
        return (SDState)i;
    return kOnGround;
  }

  NmRsCBUShootDodge::SDState NmRsCBUShootDodge::getState()
  {
    return stateToEnum((SDStateBase*)m_stateMachine.CurrentState());
  }

  void NmRsCBUShootDodge::fireWeapon(int /*hand  */ /*= NmRsCharacter::kRightHand */)
  {
    // todo: relax shoulders and upper controller to accentuate recoil force.

#if 0
	  // remove when we no longer need to hardcode itms.
	  cacheITMS();
#endif
  }

  void NmRsCBUShootDodge::successFeedback()
  {
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      feedback->m_agentID = m_character->getID();
      feedback->m_argsCount = 0;
      strcpy(feedback->m_behaviourName, NMShootDodgeFeedbackName);
      feedback->onBehaviourSuccess();
    }
  }

  void NmRsCBUShootDodge::legPerlinNoise()
  {
    // Get some perlin noise and scale it so that it is within [-0.5f  0.5f]
    // add it too the legs to add some secondary motion.
    const LeftLegSetup* leftLeg = m_character->getLeftLegSetup();
    const RightLegSetup* rightLeg = m_character->getRightLegSetup();

    float agentID = (float)m_character->getID();
    // Make the lean1 of the hips depend on the orientation of the character. 
    float hipLean1Offset = 0.2f-0.09f*(float)m_orientation;

    const float scale = m_parameters.legNoiseScale; // 0.2f;
    float lean1PL = scale*(m_character->getEngine()->perlin3(scale*m_timer + 0.4f, 1.f, agentID)-0.5f + hipLean1Offset);//-0.5f);
    float lean2PL = scale*(m_character->getEngine()->perlin3(scale*m_timer, 2.f, agentID)-0.5f);
    float twistPL = scale*(m_character->getEngine()->perlin3(scale*m_timer, 3.f, agentID)-0.5f);
    nmrsSetLean1(leftLeg->getHip(),nmrsGetDesiredLean1(leftLeg->getHip())+lean1PL);
    nmrsSetLean2(leftLeg->getHip(),nmrsGetDesiredLean2(leftLeg->getHip())+lean2PL);
    nmrsSetTwist(leftLeg->getHip(),nmrsGetDesiredTwist(leftLeg->getHip())+twistPL);

    float lean1PR = scale*(m_character->getEngine()->perlin3(scale*m_timer + 0.4f, 4.f, agentID)-0.5f + hipLean1Offset);//-0.5f);
    float lean2PR = scale*(m_character->getEngine()->perlin3(scale*m_timer, 5.f, agentID)-0.5f);
    float twistPR = scale*(m_character->getEngine()->perlin3(scale*m_timer, 6.f, agentID)-0.5f);
    nmrsSetLean1(rightLeg->getHip(),nmrsGetDesiredLean1(rightLeg->getHip())+lean1PR);
    nmrsSetLean2(rightLeg->getHip(),nmrsGetDesiredLean2(rightLeg->getHip())+lean2PR);
    nmrsSetTwist(rightLeg->getHip(),nmrsGetDesiredTwist(rightLeg->getHip())+twistPR);

    float kneeRight = -0.5f*scale*(m_character->getEngine()->perlin3(scale*m_timer, 7.f, agentID));
    nmrsSetAngle(rightLeg->getKnee(),nmrsGetDesiredAngle(rightLeg->getKnee())+kneeRight);

    float kneeLeft = -0.4f*scale*(m_character->getEngine()->perlin3(scale*m_timer, 8.f, agentID));
    nmrsSetAngle(leftLeg->getKnee(),nmrsGetDesiredAngle(leftLeg->getKnee())+kneeLeft);
  }

  void NmRsCBUShootDodge::breathing()
  {
    const LeftArmSetup* leftArm = m_character->getLeftArmSetup();
    const RightArmSetup* rightArm = m_character->getRightArmSetup();
    const SpineSetup* spine = m_character->getSpineSetup();
    float breathDiff = m_parameters.breathingScale*0.05f*(NMutils::clampValue(20.f-m_timer,10.f,20.f))*rage::Sinf(4.f*m_timer); // simple period of .75 sec, reducing over 30 seconds. 
    nmrsSetLean1(leftArm->getClavicle(),NMutils::clampValue(nmrsGetDesiredLean1(leftArm->getClavicle()) - 0.3f*breathDiff,-9.9f,9.9f));
    nmrsSetLean1(rightArm->getClavicle(),NMutils::clampValue(nmrsGetDesiredLean1(rightArm->getClavicle()) - 0.3f*breathDiff,-9.9f,9.9f));
    nmrsSetLean1(spine->getSpine3(),nmrsGetDesiredLean1(spine->getSpine3())- 0.16f*breathDiff);
    nmrsSetLean1(spine->getSpine2(),nmrsGetDesiredLean1(spine->getSpine2())- 0.12f*breathDiff);
    nmrsSetLean1(spine->getSpine1(),nmrsGetDesiredLean1(spine->getSpine1())- 0.08f*breathDiff);
    nmrsSetLean1(spine->getSpine0(),nmrsGetDesiredLean1(spine->getSpine0())- 0.04f*breathDiff);
  }

  NmRsCBUShootDodge::OrientationStatus NmRsCBUShootDodge::getOrientation()
  {
    // Some logic to decide what orientation the character is currently in. 
    // And also how we should use the head and the arms. 
    // The logic for the head and arms supposes we are just about the hit a wall and uses taht to decide what we should do.

    // dot the up vector onto the axes of the pelvis coordinate frame.
    rage::Matrix34 pelvisTM;
    m_character->getSpineSetup()->getPelvisPart()->getMatrix(pelvisTM);
    float upDotY = pelvisTM.b.Dot(m_character->m_gUp);
    float upDotZ = pelvisTM.c.Dot(m_character->m_gUp);

    // If upDotY = 1 we are on the Left side. upDotY = -1 we are on the right side.
    // if upDotZ = 1 on the back. upDotZ = -1 on the front. 

    // default to back
    OrientationStatus result = OrientationStatus(kBack);

    if ( upDotZ > 0.707f)  // on the front
    {
      result = OrientationStatus(kFront);
    }
    else if(upDotZ > -0.707f) // on one of the sides
    {
      if (upDotY <0.f)
      {
        result = OrientationStatus(kLeftSide);
      }
      else
      {
        result = OrientationStatus(kRightSide);
      }
    }

    return result;
  }

  NmRsCBUShootDodge::OrientationStatus NmRsCBUShootDodge::getCollisionOrientation()
  {
    rage::Matrix34 pelvisTM;
    m_character->getSpineSetup()->getPelvisPart()->getMatrix(pelvisTM);

    float dotX = pelvisTM.a.Dot(m_parameters.collisionNormal);
    float dotY = pelvisTM.b.Dot(m_parameters.collisionNormal);
    float dotZ = pelvisTM.c.Dot(m_parameters.collisionNormal);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 1
    bspyScratchpad(m_character->getBSpyID(), "getCollisionOrientation", dotX);
    bspyScratchpad(m_character->getBSpyID(), "getCollisionOrientation", dotY);
    bspyScratchpad(m_character->getBSpyID(), "getCollisionOrientation", dotZ);
#endif

    OrientationStatus result = OrientationStatus(kBack);

    // reordered to make head zone wide in the dorsal/ventral directions
    // but clipped narrow by the side zones.
    if(dotZ < 0.7f) // front
    {
      result = OrientationStatus(kFront);
    }
    else if(dotY > -0.5f )
      {
        result = OrientationStatus(kLeftSide);
      }
    else if(dotY < 0.5f )
      {
        result = OrientationStatus(kRightSide);
      }
    else if(dotX > -0.5)
    {
      result = OrientationStatus(kHead);
    }

    return result;
  }

  // returns net velocity projected onto collision direction.
  // negative values indicate an approaching collision.
  float NmRsCBUShootDodge::getCollisionVelocity()
  {
    rage::Vector3 netVelocity, collisionDirection;
    NmRsGenericPart* spine3 = m_character->getSpineSetup()->getSpine3Part();
    netVelocity = spine3->getLinearVelocity();
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
  bspyScratchpad(m_character->getBSpyID(), "getCollisionVelocity", netVelocity);
#endif
    netVelocity.Add(m_parameters.collisionObjectVelocity); // this should be velocity at contact point - check this.
    collisionDirection.Set(m_parameters.collisionPoint - spine3->getPosition());
    collisionDirection.Normalize();
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
	bspyScratchpad(m_character->getBSpyID(), "getCollisionVelocity", netVelocity);
	bspyScratchpad(m_character->getBSpyID(), "getCollisionVelocity", collisionDirection);
#endif
    float retVal = collisionDirection.Dot(netVelocity);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
    bspyScratchpad(m_character->getBSpyID(), "NmRsCBUShootDodge::getCollisionVelocity", retVal);
#endif
    return retVal;
  }

  void NmRsCBUShootDodge::collisionFeedback()
  {
    // Send a feedback message to indicate we have hit the ground or a wall.
    // Used by game to add some procedural skin vertex wobbling on the characters.
    // Returns the point, normal and magnitude of the collision
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback && !m_collisionFeedbackSent)
    {
      m_collisionFeedbackSent = true;
 
      feedback->m_agentID = m_character->getID();

      float impactMag = m_character->m_COMvelMag;
      rage::Vector3 impactPoint = m_parameters.collisionPoint;
      rage::Vector3 impactNormal = m_parameters.collisionNormal;

      ART::ARTFeedbackInterface::FeedbackUserdata data;
      feedback->m_argsCount = 8;

      data.setInt(kCollided); // feedback type
      feedback->m_args[0] = data;

      data.setFloat(impactMag); //Impact Mag
      feedback->m_args[1] = data;

      data.setFloat(impactPoint.x); //Point
      feedback->m_args[2] = data;
      data.setFloat(impactPoint.y); //Point
      feedback->m_args[3] = data;
      data.setFloat(impactPoint.z); //Point
      feedback->m_args[4] = data;

      data.setFloat(impactNormal.x); //Normal
      feedback->m_args[5] = data;
      data.setFloat(impactNormal.y); //Normal
      feedback->m_args[6] = data;
      data.setFloat(impactNormal.z); //Normal
      feedback->m_args[7] = data;

      strcpy(feedback->m_behaviourName, NMShootDodgeFeedbackName);
      feedback->onBehaviourEvent();
    }
  }

  void NmRsCBUShootDodge::stableFeedback()
  {
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback && !m_stableFeedbackSent)
    {
      m_stableFeedbackSent = true;

      feedback->m_agentID = m_character->getID();

      feedback->m_argsCount = 1;
      ART::ARTFeedbackInterface::FeedbackUserdata data;
      data.setInt(kStableOnGround); // feedback type
      feedback->m_args[0] = data;

      strcpy(feedback->m_behaviourName, NMShootDodgeFeedbackName);
      feedback->onBehaviourEvent();
    }
  }

  void NmRsCBUShootDodge::armStatusFeedback(bool /*leftBracing*/, bool /*rightBracing*/)
  {
    // update:  point gun now sends this feedback. remove this func altogether
    //          when sure that it's no longer needed.
#if 0 // currently crashes the game. re-enable when game has been updated.
    // send a feedback message to indicate that the pointing status of the arms
    // has changed.
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      feedback->m_agentID = m_character->getID();

      ART::ARTFeedbackInterface::FeedbackUserdata data;
      feedback->m_argsCount = 3;

      data.setInt(kArmStatus); // feedback type
      feedback->m_args[0] = data;

      data.setInt(leftBracing); //Impact Mag
      feedback->m_args[1] = data;

      data.setInt(rightBracing); //Point
      feedback->m_args[2] = data;

      strcpy(feedback->m_behaviourName, NMShootDodgeFeedbackName);
      feedback->onBehaviourEvent();
    }
#endif
  }

  void NmRsCBUShootDodge::unSupportedFeedback()
  {
    ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
    if (feedback)
    {
      feedback->m_agentID = m_character->getID();

      ART::ARTFeedbackInterface::FeedbackUserdata data;
      feedback->m_argsCount = 1;

      data.setInt(kUnSupported); // feedback type
      feedback->m_args[0] = data;

      strcpy(feedback->m_behaviourName, NMShootDodgeFeedbackName);
      feedback->onBehaviourEvent();

    m_stableFeedbackSent = false;
    }
  }

  bool NmRsCBUShootDodge::hasCollided()
  {
    // todo: optimize collision reporting!!!
    // ignore if we haven't struck something.
    if(!m_character->hasCollidedWithWorld(bvmask_Full /* & ~(bvmask_ArmLeft | bvmask_ArmRight) */)) // tune body parts for best effect
      return false;

    // has our velocity along the launch direction dropped below threshold?
    // are we decelerating sharply?

	// need new support metric. todo move to ontick().
	// true if spine is supported or if two or more limbs are supported.
	bool supported = (m_character->m_supportedParts & bvmask_Spine ? true : false);
	int numLimbsSupported = 0;
	if(!supported)
	{
		if(m_character->m_supportedParts & bvmask_ArmLeft) numLimbsSupported++;
		if(m_character->m_supportedParts & bvmask_ArmRight) numLimbsSupported++;
		if(m_character->m_supportedParts & bvmask_LegLeft) numLimbsSupported++;
		if(m_character->m_supportedParts & bvmask_LegRight) numLimbsSupported++;
		
		if(numLimbsSupported > 1)
			supported = true;
	}
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
	bspyScratchpad(m_character->getBSpyID(), "hasCollided", numLimbsSupported);
	bspyScratchpad(m_character->getBSpyID(), "hasCollided", supported);
    bool lowAccel = m_accelDotLaunchDir < m_parameters.collisionAccelThreshold;
    bool lowVel = m_velDotLaunchDir < m_parameters.collisionVelThreshold;
    bspyScratchpad(m_character->getBSpyID(), "hasCollided", lowAccel);
    bspyScratchpad(m_character->getBSpyID(), "hasCollided", lowVel);
    bspyScratchpad(m_character->getBSpyID(), "hasCollided", m_accelDotLaunchDir);
    bspyScratchpad(m_character->getBSpyID(), "hasCollided", m_velDotLaunchDir);
#endif

    bool result =	m_accelDotLaunchDir < m_parameters.collisionAccelThreshold ||
					m_velDotLaunchDir < m_parameters.collisionVelThreshold ||
					supported;

    return result;
  }

  // return weighted sum of the linear and rotational com vel magnitude.
  // used to indicate we've stopped moving.
  // todo: compatibility - old code returned half the weighted sum for some reason.
  float NmRsCBUShootDodge::getWeightedCOMVel(float weight)
  {
    Assert(weight >= 0 && weight <= 1.f);
    float temp = 0.5f*(weight*m_character->m_COMvelMag + (1-weight)*m_character->m_COMrotvelMag);
    return NMutils::clampValue(temp,0.f,1.f);
  }

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
  bool NmRsCBUShootDodge::probeTrajectory(rage::Vector3* pos, rage::Vector3* normal, float* time, float maxTime /* = 2.f */)
  {
#else
  bool NmRsCBUShootDodge::probeTrajectory(rage::Vector3* pos, rage::Vector3* normal, float* /* time */, float maxTime /* = 2.f */)
  {
#endif
    NmRsGenericPart* spine3 = m_character->getSpineSetup()->getSpine3Part();
    rage::Vector3 origin = spine3->getPosition();
    rage::Vector3 start(origin);
    rage::Vector3 end; end.Zero();
    rage::Vector3 vh = m_character->m_COMvel;
    float vv = vh.GetZ();
    vh.SetZ(0.f);

    const float step = 0.1f;
    const float gravity = 9.81f;
    float t = step;
    bool hit = false;
    while(!hit && t < maxTime)
    {
      end.AddScaled(origin, vh, t);
      end.SetZ(origin.GetZ()+(t*vv-(gravity*t*t)/2.f));

      //mmmmtodo check that the masks are correct
      hit = m_character->probeRay(m_character->pi_UseNonAsync, start, end, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);
      //hit = m_character->probeRay(start, end, pos, normal, &component, false, &intersection);
      pos->Set(m_character->m_probeHitPos[m_character->pi_UseNonAsync]);
      normal->Set(m_character->m_probeHitNormal[m_character->pi_UseNonAsync]);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
      m_character->bspyDrawLine(start, end, rage::Vector3(1,1,0));
#endif
      start.Set(end);
      t += step;
    }
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    if(hit)
    {
      m_character->bspyDrawPoint(*pos, 0.025f, rage::Vector3(1,0,0));
      m_character->bspyDrawLine(*pos, *pos + *normal * 0.1f, rage::Vector3(1,0,0));
      *time = t; // not strictly correct. need to add inter-step component returned by probe func.
    }
#endif
    return hit;
  }

  bool NmRsCBUShootDodge::probeDown(rage::Vector3* pos, rage::Vector3* normal, float* height, float distance)
  {
    NmRsGenericPart* spine3 = m_character->getSpineSetup()->getPelvisPart(); // [jrp] rename?
    rage::Vector3 down, start, end;
    down.Set(m_character->getEngine()->getSimulator()->GetGravity());
    down.Normalize(); // could just divide by 9.81...
    start.Set(spine3->getPosition());
    end.AddScaled(start, down, distance);

    rage::phIntersection intersection;

    //mmmmtodo check the masks are correct
    bool hit = m_character->probeRay(m_character->pi_UseNonAsync, start, end, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);
    //bool hit = m_character->probeRay(start, end, pos, normal, &component, false, &intersection);
    pos->Set(m_character->m_probeHitPos[m_character->pi_UseNonAsync]);
    normal->Set(m_character->m_probeHitNormal[m_character->pi_UseNonAsync]);

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    if(hit)
      m_character->bspyDrawLine(start, end, rage::Vector3(1,1,0));
    else
      m_character->bspyDrawLine(start, end, rage::Vector3(0,1,0));
#endif

    *height = distance;
    if(hit)
    {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
      m_character->bspyDrawPoint(*pos, 0.025f, rage::Vector3(1,0,0));
      m_character->bspyDrawLine(*pos, *pos + *normal * 0.1f, rage::Vector3(1,0,0));
#endif
      rage::Vector3 toPos(*pos - start);
      *height = toPos.Mag();
    }

    return hit;
  }

  // the intent is to orient spine3 to facilitate gun pointing.
  // this is a naive one-pass solution that depends on the spine
  // joints and bodies being oriented similarly when driven to zero.
  // should this not be the case, a CCD-like solution might be
  // necessary.
  void NmRsCBUShootDodge::driveSpine(float offsetAngle /* = 0 */)
  {
    const SpineSetup * spine = m_character->getSpineSetup();
    
    // get the target direction.
    rage::Matrix34 pelvisTm, spine3Tm;
    spine->getPelvisPart()->getMatrix(pelvisTm);
    spine->getSpine3Part()->getMatrix(spine3Tm);
    rage::Vector3 targetDirection(m_parameters.rightArmTarget);
    targetDirection.Subtract(spine3Tm.d);
    targetDirection.Normalize();

    // define chest forward direction.
    // by default, this is spine3 -z.
    // we now add an angle used to offset this around spine3 x.
    // todo why are we using pelvis here? !!!
    rage::Vector3 chestForward(-pelvisTm.c);
    rage::Quaternion offsetQ;
    offsetQ.FromRotation(pelvisTm.a, offsetAngle);
    offsetQ.Transform(chestForward);

    // compute rotation to target. 
    rage::Vector3 axis;
    axis.Cross(targetDirection, chestForward);
    float angle = rage::Acosf(targetDirection.Dot(chestForward));
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
    m_character->bspyDrawLine(spine3Tm.d, spine3Tm.d + targetDirection, rage::Vector3(1,1,0));
    m_character->bspyDrawLine(spine3Tm.d, spine3Tm.d + axis, rage::Vector3(1,1,0));
    m_character->bspyDrawLine(spine3Tm.d, spine3Tm.d + chestForward, rage::Vector3(1,0,1));
#endif

    // put rotation axis into spine0 joint local space.
    rage::Matrix34 spine0JtTm;
    spine->getSpine0()->getMatrix1(spine0JtTm);
    spine0JtTm.UnTransform3x3(axis);

    // divide by number of spine joints (4) and clamp near limits.
    const int numSpineJoints = 4;
    const float limit = 0.3f;
    angle = rage::Clamp(angle / (float)numSpineJoints, 0.f, limit);

    // drive each spine joint.
    rage::Quaternion rotQ;
    rotQ.FromRotation(axis, -angle);
    driveSpineEffector(spine->getSpine0(), rotQ);
    driveSpineEffector(spine->getSpine1(), rotQ);
    driveSpineEffector(spine->getSpine2(), rotQ);
    driveSpineEffector(spine->getSpine3(), rotQ);
  }

  void NmRsCBUShootDodge::driveSpineEffector(NmRs3DofEffector* effector, rage::Quaternion& rot)
  {
    rage::Vector3 ts(rsQuatToRageDriveTwistSwing(rot));
    effector->getTwistAndSwingFromRawTwistAndSwing(ts, ts);
#if 1
    effector->setDesiredTwist(ts.x);
    effector->setDesiredLean1(ts.y);
    effector->setDesiredLean2(ts.z);
#else
    effector->blendToPose(ts.x, ts.y, ts.z, 0.5f * m_character->getLastKnownUpdateStep() * 30.f);
#endif
  }

  // shootdodge-specific replacement for head look.
  void NmRsCBUShootDodge::driveHead()
  {
    const SpineSetup * spine = m_character->getSpineSetup();

    // get the target direction.
    rage::Matrix34 spine3Tm, headTm;
    spine->getSpine3Part()->getMatrix(spine3Tm);
    spine->getHeadPart()->getMatrix(headTm);
    rage::Vector3 targetDirection(m_parameters.rightArmTarget);
    targetDirection.Subtract(spine3Tm.d);
    targetDirection.Normalize();
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    m_character->bspyDrawLine(headTm.d, headTm.d+targetDirection, rage::Vector3(1,1,0));
#endif

    // define head forward direction.
    // by default, this is head -z.
    rage::Vector3 headForward(-headTm.c);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    m_character->bspyDrawLine(headTm.d, headTm.d+headForward, rage::Vector3(1,0,1));
#endif

    // get target rotation.
    rage::Vector3 axis;
    rage::Quaternion rotQ;
    axis.Cross(targetDirection, headForward);
    axis.Normalize();
    float angle = rage::Acosf(targetDirection.Dot(headForward));
    angle *= 0.5f;
    // clamp angle here.
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    m_character->bspyDrawLine(headTm.d, headTm.d+axis, rage::Vector3(1,1,0));
#endif

    // get rotation in space of joint fixed frame.
    rage::Matrix34 jtM1;
    rage::Vector3 axisLocal;
    spine->getLowerNeck()->getMatrix1(jtM1);
    jtM1.UnTransform3x3(axis, axisLocal);
    rotQ.FromRotation(axisLocal, angle);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    m_character->bspyDrawCoordinateFrame(0.05f, jtM1);
#endif
    driveSpineEffector(spine->getLowerNeck(), rotQ);
    
    spine->getUpperNeck()->getMatrix1(jtM1);
    jtM1.UnTransform3x3(axis, axisLocal);
    rotQ.FromRotation(axisLocal, angle);
    driveSpineEffector(spine->getUpperNeck(), rotQ);
  }

  void NmRsCBUShootDodge::passThroughPointGunParams()
  {
    NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)m_cbuParent->m_tasks[bvid_pointGun];
    Assert(pointGunTask);

    // update general user parameters
    pointGunTask->m_parameters.timeWarpActive = m_parameters.timeWarpActive;
    pointGunTask->m_parameters.timeWarpStrengthScale = m_parameters.timeWarpStrengthScale;
    pointGunTask->m_parameters.armDamping = m_parameters.aimingArmDamping;
    pointGunTask->m_parameters.armStiffness = m_parameters.aimingArmStiffness;
    pointGunTask->m_parameters.leftHandTarget = m_parameters.leftArmTarget;
    pointGunTask->m_parameters.rightHandTarget = m_parameters.rightArmTarget;
    pointGunTask->m_parameters.disableArmCollisions = true;
    pointGunTask->m_parameters.poseArmWhenNotInUse = true;
    pointGunTask->m_parameters.alwaysSupport = true;
    pointGunTask->m_parameters.allowShotLooseness = false;  
    pointGunTask->m_parameters.elbowAttitude = m_parameters.pointGunElbowAttitude;
    pointGunTask->m_parameters.oriStiff =  m_parameters.pointGunOriStiff;
    pointGunTask->m_parameters.oriDamp = m_parameters.pointGunOriDamp;
    pointGunTask->m_parameters.posStiff = m_parameters.pointGunPosStiff;
    pointGunTask->m_parameters.posDamp = m_parameters.pointGunPosDamp;
    pointGunTask->m_parameters.primaryHandWeaponDistance = -1.f;
    pointGunTask->m_parameters.fireWeaponRelaxTime = m_parameters.recoilRelaxTime;
    pointGunTask->m_parameters.fireWeaponRelaxAmount = m_parameters.recoilRelaxAmount;
    pointGunTask->m_parameters.clavicleBlend = m_parameters.pointGunClavicleBlend;
    pointGunTask->m_parameters.constrainRifle = m_parameters.pointGunConstrainRifle;
    // this parameter is hooked up wrong in game task (copy/paste error) and needs to be overridden temporarily.
    pointGunTask->m_parameters.rifleConstraintMinDistance = 0.2f; // m_parameters.pointGunRifleConstraintMinDistance;
    pointGunTask->m_parameters.constraintMinDistance = m_parameters.pointGunConstraintMinDistance;
    pointGunTask->m_parameters.extraTilt = 0.3f;
    pointGunTask->m_parameters.errorThreshold = m_parameters.pointGunErrorThreshold;
    pointGunTask->m_parameters.gravityOpposition = m_parameters.pointGunGravityOpposition;
    pointGunTask->m_parameters.fireWeaponRelaxDistance = m_parameters.pointGunFireWeaponRelaxDistance;
    pointGunTask->m_parameters.useHeadLook = m_parameters.useHeadLook;

    // mode specific settings.
    switch(m_character->getWeaponMode())
    {
    case kRifle:
      // disable constraint if we are bracing.
      pointGunTask->m_parameters.useConstraint = true;
      pointGunTask->m_parameters.rightHandParentOffset = rage::Vector3(-0.04f, 0.05f, 0.0f); // shifted a bit to his right
      // clavicle ik doesn't satisfy rifle requirements.
      pointGunTask->m_parameters.clavicleBlend = 1.f;
      break;
    case kDual:
      pointGunTask->m_parameters.useConstraint = false;
      pointGunTask->m_parameters.rightHandParentOffset = rage::Vector3(0, 0, 0);
      pointGunTask->m_parameters.leftHandParentOffset = rage::Vector3(0, 0, 0);
      pointGunTask->m_parameters.extraTilt = 0.78f;
      break;
    case kPistolLeft:
      pointGunTask->m_parameters.useConstraint = false;
      pointGunTask->m_parameters.leftHandParentOffset = rage::Vector3(0, 0, 0);
      break;
    case kPistol: // pistol now single-handed
    case kPistolRight:
    case kSidearm:
    case kNone:
    case kNumWeaponModes: // need to handle all elements to switch on an enum
      pointGunTask->m_parameters.useConstraint = false;
      pointGunTask->m_parameters.rightHandParentOffset = rage::Vector3(0, 0, 0);
      pointGunTask->m_parameters.rightHandParentEffector = gtaJtShoulder_Right;
      pointGunTask->m_parameters.primaryHandWeaponDistance = -1.f;
      break;
    }
  }

#if NM_SHOOTDODGE_BUTTERWORTH
  // helper func. get m_accelerationIndex plus diff,
  // wrapping correctly.
  unsigned int NmRsCBUShootDodge::getNextIndex(int /*diff*/)
  {
#if 0
    // this is unused and breaks the ps3 build.
    // [jrp] m_accelerationIndex should be signed.
    if(m_accelerationIndex + diff > 2)
      return m_accelerationIndex + diff - 3;
    else if(m_accelerationIndex + diff < 0)
      return m_accelerationIndex + diff + 3;
    else
      return m_accelerationIndex + diff;
#else
    return 0;
#endif
  }

  void NmRsCBUShootDodge::updateAccelerationSmoothing(float timeStep)
  {
    rage::Vector3 COMAcceleration = (m_character->m_COMvel - m_lastCOMVel) * timeStep;
    if(true) //m_accelerationValid)
    {
      // borrowed from MM's work in NmRsCBU_DynamicBalancer.cpp
      // so bug him if something is wrong with it...
      float Pi = 3.1415926535897f;//from exel (fro checking only)
      //float Fs = 30.f;  //Sample Frequency
      float Fs = 1.f/timeStep; //Sample Frequency
      float Fc = 6.f;  //Cutoff Frequency
      float C = 1;      // correction factor;
      float Wc = (tan(Pi*Fc/Fs)/C);
      //float K1 = sqrt(2.f)*Wc;//Butterworth Filter
      float K1 = 2.f*Wc;//Critically damped Filter
      float K2 = Wc*Wc;
      float a0 = K2/(1+K1+K2);
      float a1 = 2.f*a0;
      float a2 = a0;
      float K3 = 2*a0/K2;
      float b1 = -2.f*a0 + K3;
      float b2 = 1 -2.f*a0 - K3;

      
      //m_acceleration[m_accelerationIndex] = (a0 * COMAcceleration / timeStep +
      //                                      a1 * m_acceleration[getNextIndex(-1)] +
      //                                      a2 * m_acceleration[getNextIndex(-2)] +
      //                                      b1 * m_acceleration[getNextIndex(-1)] +
      //                                      b2 * m_acceleration[getNextIndex(-2)]);
      m_acceleration[0] = ( a0 * COMAcceleration +
                            a1 * m_acceleration[1] +
                            a2 * m_acceleration[2] +
                            b1 * m_acceleration[1] +
                            b2 * m_acceleration[2]);
  #if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
      float mag = COMAcceleration.Mag();
      bspyScratchpad(m_character->getBSpyID(), "butterworth.actual", mag);
      mag = m_acceleration[0].Mag();
      bspyScratchpad(m_character->getBSpyID(), "butterworth.0", mag);
      mag = m_acceleration[1].Mag();
      bspyScratchpad(m_character->getBSpyID(), "butterworth.1", mag);
      mag = m_acceleration[2].Mag();
      bspyScratchpad(m_character->getBSpyID(), "butterworth.2", mag);
      int index = (int)m_accelerationIndex;
      bspyScratchpad(m_character->getBSpyID(), "butterworth", index);
  #endif

      // advance index.
      if(m_accelerationIndex == 0)
        m_accelerationIndex = 2;
      else
        m_accelerationIndex -= 1;

      m_acceleration[2] = m_acceleration[1];
      m_acceleration[1] = m_acceleration[0];
    }
    else
    {
      m_acceleration[0] = COMAcceleration;
      m_acceleration[1] = COMAcceleration;
      m_acceleration[2] = COMAcceleration;
      m_accelerationValid = true;
    }
  }
#endif


  bool NmRsCBUShootDodge::isSupported()
  {
    // we are supported if...
    // any part of the trunk is supported, or
    if(m_character->getSupportedParts() & (bvmask_Spine | bvmask_ThighLeft | bvmask_ThighRight))
    {
#if ART_ENABLE_BSPY
      bspyLogf(info, L"sd spine supported");
#endif
        return true;
    }
    // at least three limbs are supported
    else if(m_character->getSupportedParts() & (bvmask_Legs | bvmask_ArmLeft | bvmask_ArmRight))
    {
      int limbCount = 0;
      if(m_character->getSupportedParts() & bvmask_LegLeft)
        ++limbCount;
      if(m_character->getSupportedParts() & bvmask_LegRight)
        ++limbCount;
      if(m_character->getSupportedParts() & bvmask_ArmLeft)
        ++limbCount;
      if(m_character->getSupportedParts() & bvmask_ArmRight)
        ++limbCount;
      if(limbCount > 2)
      {
#if ART_ENABLE_BSPY
        bspyLogf(info, L"sd limbs supported");
#endif
        return true;
      }
    }
#if ART_ENABLE_BSPY
    bspyLogf(info, L"sd unsupported");
#endif
    return false;
  }

  void NmRsCBUShootDodge::setArmStrength(const ArmSetup* arm, float stiffness, float damping, float taper)
	{
		Assert(arm);

		float scaledStiffness = rage::Clamp(stiffness, 3.f, FLT_MAX); // clavicle gets full stiffness.
		arm->getClavicle()->setMuscleStrength(scaledStiffness * scaledStiffness);
		arm->getClavicle()->setMuscleDamping(scaledStiffness * 2.f * damping);

		scaledStiffness = rage::Clamp(stiffness - stiffness * 0.15f * taper, 3.f, 100.f);
		arm->getShoulder()->setMuscleStrength(scaledStiffness * scaledStiffness);
		arm->getShoulder()->setMuscleDamping(scaledStiffness * 2.f * damping);

		scaledStiffness = rage::Clamp(stiffness - stiffness * 0.55f * taper, 3.f, 100.f);
		arm->getElbow()->setMuscleStrength(scaledStiffness * scaledStiffness);
		arm->getElbow()->setMuscleDamping(scaledStiffness * 2.f * damping);

		scaledStiffness = rage::Clamp(stiffness - stiffness * 1.f * taper, 3.f, 100.f);
		arm->getWrist()->setMuscleStrength(scaledStiffness * scaledStiffness);
		arm->getWrist()->setMuscleDamping(scaledStiffness * 2.f * damping);
	}

	void NmRsCBUShootDodge::setLegStrength(const LegSetup* leg, float stiffness, float damping, float taper)
	{
		Assert(leg);

		float scaledStiffness = rage::Clamp(stiffness, 3.f, FLT_MAX); // hip gets full stiffness.
		leg->getHip()->setMuscleStrength(scaledStiffness * scaledStiffness);
		leg->getHip()->setMuscleDamping(scaledStiffness * 2.f * damping);

		scaledStiffness = rage::Clamp(stiffness - stiffness * 0.5f * taper, 3.f, 100.f);
		leg->getKnee()->setMuscleStrength(scaledStiffness * scaledStiffness);
		leg->getKnee()->setMuscleDamping(scaledStiffness * 2.f * damping);

		scaledStiffness = rage::Clamp(stiffness - stiffness * 1.f * taper, 3.f, 100.f);
		leg->getAnkle()->setMuscleStrength(scaledStiffness * scaledStiffness);
		leg->getAnkle()->setMuscleDamping(scaledStiffness * 2.f * damping);
	}

  void NmRsCBUShootDodge::calculateCoM(
    rage::Matrix34* comResult,
    rage::Vector3* comVelResult,
    rage::Vector3* comAngVelResult,
    rage::Vector3* angMomResult,
    BehaviourMask partMask) const
  {
    //NmRsCachePrefetch(m_parts);

    float totalMass = 0.0f;
    rage::Matrix34 mat;
    rage::Vector3 axis0, axis1, axis2, bpMass,
      vCom, vComVel, vComAngVel,
      angMom, sum_Ri_x_Vi, sum_miRi, sum_miVi,
      angMomOfPartAboutP;//P will be the ArticulatedCollider position i.e. offset
    NmRsGenericPart* part;

    axis0.Zero(); axis1.Zero(); axis2.Zero();
    vCom.Zero(); vComVel.Zero(); vComAngVel.Zero();
    angMom.Zero(); sum_Ri_x_Vi.Zero(); sum_miRi.Zero(); sum_miVi.Zero(); 

    if (m_character->getArticulatedWrapper())
    {
      rage::Vector3 offset = m_character->getArticulatedWrapper()->getArticulatedCollider()->GetMatrix().d;

      rage::phArticulatedBody *body = m_character->getArticulatedWrapper()->getArticulatedBody();
      int numLinks = body->GetNumJoints() + 1;

      for (int i = 0; i<numLinks; i++)
      {
        if(m_character->isPartInMask(partMask, i))
        {
          part = m_character->getGenericPartByIndex(i);

          rage::phArticulatedBodyPart& bp = body->GetLink(i);
          bpMass = SCALARV_TO_VECTOR3(bp.GetMass());

          rage::Matrix34 mat1 = MAT34V_TO_MATRIX34(bp.GetCore().GetMatrix());
          rage::Matrix34 mat0 = part->getInitialMatrix(); // not transposed as stored in generic part (transposed are the ones in the bounds)

          mat0.Inverse3x3();
          mat.Dot3x3(mat1, mat0);

          axis0.AddScaled(mat.GetVector(0), bpMass);
          axis1.AddScaled(mat.GetVector(1), bpMass);
          axis2.AddScaled(mat.GetVector(2), bpMass);

          vCom.AddScaled(bp.GetCore().GetPosition(), bpMass);
          vComVel.AddScaled(VEC3V_TO_VECTOR3(bp.GetCore().GetLinearVelocity()), bpMass);
          vComAngVel.AddScaled(VEC3V_TO_VECTOR3(bp.GetCore().GetAngularVelocity()), bpMass);

          totalMass += bpMass.x;
          //Calcs for the AngMom
          angMomOfPartAboutP = bp.GetCore().GetPosition();
          angMomOfPartAboutP.Cross(bpMass.x*VEC3V_TO_VECTOR3(bp.GetCore().GetLinearVelocity()));
          sum_Ri_x_Vi += angMomOfPartAboutP;
          sum_miRi += bpMass.x*bp.GetCore().GetPosition();
          sum_miVi += bpMass.x*VEC3V_TO_VECTOR3(bp.GetCore().GetLinearVelocity());
        }
      }

      axis0.Normalize();
      axis1.Normalize();
      axis2.Normalize();

      rage::Vector3 vecInvTotalMass;
      Assert(totalMass > 0.f);
      vecInvTotalMass.Set(1.0f / totalMass);

      vCom.Multiply(vecInvTotalMass);
      vComAngVel.Multiply(vecInvTotalMass);
      vComVel.Multiply(vecInvTotalMass);

      // add on the position of the instance (since body part positions are local to the instance)
      vCom += offset;

      comResult->Set(axis0, axis1, axis2, vCom);
      comResult->Transpose();

      comVelResult->Set(vComVel);

      if (m_character->m_zUp) // rotate the matrix the right way
      {
        rage::Vector3 up = comResult->c;
        comResult->c = -comResult->b;
        comResult->b = up;
      }

      comAngVelResult->Set(vComAngVel);

#if NM_RS_VALIDATE_VITAL_VALUES
      Assert(vCom.x == vCom.x && vCom.y == vCom.y && vCom.z == vCom.z);
      Assert(vComVel.x == vComVel.x && vComVel.y == vComVel.y && vComVel.z == vComVel.z);
      Assert(vComAngVel.x == vComAngVel.x && vComAngVel.y == vComAngVel.y && vComAngVel.z == vComAngVel.z);
      Assert(comResult->IsEqual(*comResult));
#endif // NM_RS_VALIDATE_VITAL_VALUES

      comResult->Normalize();

      //Angmom
      angMom.Cross(sum_miRi,sum_miVi);
      angMom *= -1/totalMass;
      angMom += sum_Ri_x_Vi;
      angMomResult->Set(angMom);
    }
  }

#if ART_ENABLE_BSPY
  void NmRsCBUShootDodge::sendParameters(NmRsSpy& spy)
  {    

    static const char* bspy_SDStateStrings[] = 
    {
        "kInAir",
        "kCollideGround",
        "kOnGround",
        "kLaunch",
        "kCollideWall",
        "kBail",
        "kNumStates"
    };

    static const char* bspy_SDCollisionStrings[] = 
    {
      "kCollisionGround",
      "kCollisionWall",
      "kCollisionObject",
      "kCollisionFragmentable",
      "kCollisionDoor",
      "kCollisionGlass"
    };

    static const char* bspy_SDOrientationStrings[] = 
    {
        "kFront",
        "kRightSide",
        "kLeftSide",
        "kBack",
        "kHead"
    };

    CBUTaskBase::sendParameters(spy);

    // parameters.
    bspyTaskVar_StringEnum(m_parameters.state, bspy_SDStateStrings, true);
    bspyTaskVar_StringEnum(m_parameters.collisionReaction, bspy_SDCollisionStrings, true);
    bspyTaskVar(m_parameters.hardKey, true);
    bspyTaskVar_Bitfield32(m_parameters.hardKeyMask, true);
    bspyTaskVar(m_parameters.rightArmTarget, true);
    bspyTaskVar(m_parameters.leftArmTarget, true);
    bspyTaskVar(m_parameters.headStiffness,true);
    bspyTaskVar(m_parameters.headDamping,true);
    bspyTaskVar(m_parameters.trunkStiffness,true);
    bspyTaskVar(m_parameters.trunkDamping,true);
    bspyTaskVar(m_parameters.legStiffness,true);
    bspyTaskVar(m_parameters.legDamping,true);
    bspyTaskVar(m_parameters.legTaper,true);
    bspyTaskVar(m_parameters.aimingArmStiffness,true);
    bspyTaskVar(m_parameters.aimingArmDamping,true);
    bspyTaskVar(m_parameters.notAimingArmStiffness,true);
    bspyTaskVar(m_parameters.notAimingArmDamping,true);
    bspyTaskVar(m_parameters.notAimingArmTaper,true);
    bspyTaskVar(m_parameters.collisionObjectVelocity,true);
    bspyTaskVar(m_parameters.collisionPoint,true);
    bspyTaskVar(m_parameters.collisionNormal, true);
    bspyTaskVar(m_parameters.collisionTime,true);
    bspyTaskVar(m_parameters.collisionTimeThreshold,true);
    bspyTaskVar(m_parameters.collisionAccelThreshold,true);
    bspyTaskVar(m_parameters.collisionVelThreshold,true);
    bspyTaskVar(m_parameters.collisionIndex,true);
    bspyTaskVar(m_parameters.oriStiff, true);
    bspyTaskVar(m_parameters.oriDamp, true);
    bspyTaskVar(m_parameters.oriSplit, true);
    bspyTaskVar(m_parameters.velSmooth, true);
    bspyTaskVar(m_parameters.stickyRootThresh, true);
    bspyTaskVar(m_parameters.stickyRootRadius, true);
    bspyTaskVar(m_parameters.pointGunUseHelperTorques, true);
    bspyTaskVar(m_parameters.pointGunUseHelperTorquesSupport, true);
    bspyTaskVar(m_parameters.pointGunUseHelperForces, true);
    bspyTaskVar(m_parameters.pointGunUseHelperForcesSupport, true);
    bspyTaskVar(m_parameters.pointGunUseConstraint, true);
    bspyTaskVar(m_parameters.pointGunConstrainRifle, true);
    bspyTaskVar(m_parameters.pointGunElbowAttitude, true);
    bspyTaskVar(m_parameters.pointGunOriStiff, true);
    bspyTaskVar(m_parameters.pointGunOriDamp, true);
    bspyTaskVar(m_parameters.pointGunPosStiff, true);
    bspyTaskVar(m_parameters.pointGunPosDamp, true);
    bspyTaskVar(m_parameters.pointGunRifleConstraintMinDistance, true);
    bspyTaskVar(m_parameters.pointGunConstraintMinDistance, true);
    bspyTaskVar(m_parameters.recoilRelaxTime, true);
    bspyTaskVar(m_parameters.recoilRelaxTime, true);
    bspyTaskVar(m_parameters.legNoiseScale, true);
    bspyTaskVar(m_parameters.armAnimTask, true);
    bspyTaskVar(m_parameters.leftArmAnimTaskParent, true);
    bspyTaskVar(m_parameters.rightArmAnimTaskParent, true);
    bspyTaskVar(m_parameters.timeWarpActive, true);
    bspyTaskVar(m_parameters.timeWarpStrengthScale, true);
    bspyTaskVar(m_parameters.groundVelocity, true);
    bspyTaskVar(m_parameters.groundInstance, true);
    bspyTaskVar(m_parameters.spineBlend, true);
    bspyTaskVar(m_parameters.reOrientScale, true);
    bspyTaskVar(m_parameters.reOrientTime, true);
    bspyTaskVar(m_parameters.frictionScale, true);
    bspyTaskVar(m_parameters.restitutionScale, true);

    // other internal variables.
    bspyTaskVar(m_hardKeyed, false);
    bspyTaskVar(m_timer,false);
    bspyTaskVar(m_trunkCOMVel,false);
    bspyTaskVar(m_trunkCOMAngVel,false);
    bspyTaskVar(m_trunkAngMom,false);
    bspyTaskVar(m_trunkCOMacc,false);
    bspyTaskVar(m_groundVelocity,false);
    bspyTaskVar_StringEnum(m_orientation, bspy_SDOrientationStrings, false);
    bspyTaskVar_StringEnum(m_collisionOrientation, bspy_SDOrientationStrings, false);
    bspyTaskVar(m_collisionDirection,false);
    bspyTaskVar(m_oriStiffness,false);
    bspyTaskVar(m_oriDamping,false);
    bspyTaskVar(m_oriSplit,false);
    bspyTaskVar(m_pelvisRotVelCache, false);
    bspyTaskVar(m_spine3RotVelCache, false);
    bspyTaskVar(m_orientationTargetOffset, false);
    bspyTaskVar(m_launchDirection, false);
    bspyTaskVar(m_timeWarpScale, false);
    bspyTaskVar(m_velDotLaunchDir, false);
    bspyTaskVar(m_accelDotLaunchDir, false);

    // state-specific variables
    // in air
    bspyTaskVar(m_inAirState.m_takeoffAngle, false);
    bspyTaskVar(m_inAirState.m_reOrientAxis, false);
    bspyTaskVar(m_inAirState.m_reOrientAngle, false);
    bspyTaskVar(m_inAirState.m_aboutToCrash, false);
    // on ground
    bspyTaskVar(m_onGroundState.m_stickyRootVelCache, false);
    bspyTaskVar(m_onGroundState.m_stickyRootActive, false);
    bspyTaskVar(m_onGroundState.m_stickyRootPosition, false);
    // collide ground
    bspyTaskVar(m_collideGroundState.m_timer, false);
    bspyTaskVar(m_collideGroundState.m_timerOnGround, false);
    bspyTaskVar(m_collideGroundState.m_comVelMag, false);

    // support criteria
    rage::Vector3 supportCenter;
    m_character->getSupportCenter(&supportCenter);
    bspyTaskVar(m_character->m_numSupportPoints, false);
    bspyTaskVar(supportCenter, false);
    bspyTaskVar(m_isSupported, false);
    bspyTaskVar(m_supported, false);
    bspyTaskVar_Bitfield32(m_character->m_supportedParts, false);
    bspyTaskVar(m_pelvisHeight, false);

    // debug collision state
    bool collidedAll = m_character->hasCollidedWithWorld(bvmask_Full);
    bool collidedSpine = m_character->hasCollidedWithWorld(bvmask_Full & ~(bvmask_ArmLeft | bvmask_ArmRight));
    bspyTaskVar(collidedAll, false);
    bspyTaskVar(collidedSpine, false);
  }
#endif

  // State machine

  void NmRsCBUShootDodge::SDGlobalState::Enter()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::SDGlobalState::Enter()");
#endif

    m_bail = 0;
    m_time = 0;
  }

  void NmRsCBUShootDodge::SDGlobalState::Update(float timeStep)
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::SDGlobalState::Update(%f)", timeStep);
#endif

    // update timer.
    m_time += timeStep;

    NmRsCBUShootDodge::Parameters* parameters = &getContext()->m_parameters;

#if 1
    // some code from the old shootdodge that may or may not be relevant. testing.

    // fake internal variables for now.
    float desiredOrientation = 0;
    float currentOrientation = 0;
    //float endCollisionImportance = 0;
    float endDesiredImpactOrientation = 0;
    float predictedTimeToEndImpact = getContext()->m_parameters.collisionTime;
    float takeoffAngle = getContext()->m_inAirState.m_takeoffAngle;
    const SpineSetup* spine = m_character->getSpineSetup();

    // m_EndCollisionImportance control how much we should be in the end collision 
    // and how much we should be blending to the zero pose. 
    // Base m_EndCollisionImportance on a combo of linear and Angular velocity
    //float importanceTemp = 0.5f*(0.8f*m_character->m_COMvelMag + 0.2f*m_character->m_COMrotvelMag);
    //endCollisionImportance = NMutils::clampValue(importanceTemp,0.f,1.f);

    // Calculate the current orientation of the character
    // [also based on this what we should be doing with the arms and head but gets immediately overwritten]
    rage::Matrix34 curMat;
    spine->getPelvisPart()->getMatrix(curMat);
    
    // jrp. this one is tricky - func does not do what it says.
    // it does far more than disabling the arms.
    // disableArmsBasedOnOrientationOfMax(m_pointingWithLeftArm, m_pointingWithRightArm, m_headLookAtTarget, m_backSideFront, curMat);

    //TEST: calculate the desired orientation at this frame.
    // So lets just do a Linear interpolation
    rage::Vector3 currentComV = m_character->m_COMvel;
    m_character->levelVector(currentComV);
    currentComV.Normalize();
    rage::Vector3 currentBackV = spine->getSpine2Part()->getPosition() - spine->getPelvisPart()->getPosition();
    currentBackV.Normalize();
    float sign = currentBackV.Dot(m_character->m_gUp);
    currentOrientation = (sign/(rage::Abs(sign)))*rage::Acosf(currentBackV.Dot(currentComV));
    float totalTimeForInAir = m_time + predictedTimeToEndImpact;
    float percentageThroughSD = NMutils::clampValue(1.f - predictedTimeToEndImpact/totalTimeForInAir,0.f,1.f);
    desiredOrientation = percentageThroughSD*(endDesiredImpactOrientation - takeoffAngle) + takeoffAngle;

    // In some situations we don't want to drive the orientation, so set it to the current orientation.
    if ((predictedTimeToEndImpact < 0.05f)||(currentOrientation < desiredOrientation) /* ||(m_backSideFront==OrientationStatus(kBack)) */)
      desiredOrientation = currentOrientation;

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
    bspyScratchpad(m_character->getBSpyID(), "shootdodge_old", totalTimeForInAir);
    bspyScratchpad(m_character->getBSpyID(), "shootdodge_old", percentageThroughSD);
    bspyScratchpad(m_character->getBSpyID(), "shootdodge_old", desiredOrientation);
    bspyScratchpad(m_character->getBSpyID(), "shootdodge_old", currentOrientation);
#endif
#endif

    // update spine twist target.
    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist*)getContext()->getCBUParent()->m_tasks[bvid_spineTwist];
    Assert(spineTwistTask);
    spineTwistTask->setSpineTwistPos(parameters->rightArmTarget);

    if(getContext()->m_parameters.useHeadLook)
    {
    // update head look target.
    NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)getContext()->getCBUParent()->m_tasks[bvid_headLook];
    Assert(headLookTask);
    headLookTask->m_parameters.m_pos = parameters->rightArmTarget;
    }

    // bail is only a valid option when we are mid-air. don't bother probing
    // if we are already on the ground or bailing.
    if( !( m_fsm->CurrentState() == (State*)getContext()->m_states[kBail] ||
           m_fsm->CurrentState() == (State*)getContext()->m_states[kOnGround]))
    {
      // moved probe down into tick so results can be used in other places.
      if(getContext()->m_pelvisHeight >= 5.f)
        m_bail += timeStep;

      // if bail integrator passes threshold, bail. duh.
      const float threshold = 0.25f; 
      if(m_bail > threshold)
      {
        getContext()->m_parameters.state = kBail;
        getContext()->m_stateMachine.ChangeState(getContext()->m_states[kBail]);
        // getContext()->collisionFeedback(); 
        // TODO: add new feedback type to cover this.
        // currently causes game to switch to a collision state.

        m_bail = 0; // reset counter to avoid flipping.
      }
    }


    // leak over time. decays at 1/10 of rise.
    const float leak = 0.01f * timeStep;
    m_bail -= leak;
  }

  void NmRsCBUShootDodge::SDGlobalState::Exit()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::SDGlobalState::Exit()");
#endif
  }

  // in air state:
  void NmRsCBUShootDodge::InAirState::Enter()
  {
    m_hardKeyMask = (BehaviourMask) getContext()->m_parameters.hardKeyMask; // bvmask_Spine;

    if(getContext()->m_parameters.hardKey) // add collision prediction criteria?
    {
      m_character->setIncomingTransformMask(m_hardKeyMask);
#if 1
      // fix from stephane for frame 1 glitching.
      m_character->setIncomingTransformApplyMode(kContinuous);
#else
      m_character->setIncomingTransformApplyMode(kEnabling);
#endif
      getContext()->m_hardKeyed = true;

      // send feedback to indicate we are hard keying.
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();
        ART::ARTFeedbackInterface::FeedbackUserdata data;
        feedback->m_argsCount = 2;
        data.setInt(kHardKey); // feedback type
        feedback->m_args[0] = data;
        data.setInt(1);
        feedback->m_args[1] = data;
        strcpy(feedback->m_behaviourName, NMShootDodgeFeedbackName);
        feedback->onBehaviourEvent();
      }
    }
    else
    {
      getContext()->m_hardKeyed = false;
    }

    // work out the current orientation of the character in the saggital plane.
    const SpineSetup* spine = m_character->getSpineSetup();
    rage::Vector3 currentComV = m_character->m_COMvel;
    m_character->levelVector(currentComV);
    currentComV.Normalize();
    rage::Vector3 currentBackV = spine->getSpine2Part()->getPosition() - spine->getPelvisPart()->getPosition();
    currentBackV.Normalize();
    float sign = rage::Asinf(currentBackV.Dot(m_character->m_gUp));
    m_takeoffAngle = (sign/(rage::Abs(sign)))*rage::Acosf(currentBackV.Dot(currentComV));

    // clear pre-crash flag.
    m_aboutToCrash = false;
    m_reOrientAngle = 0.f;
    m_reOrientAxis = m_character->m_gUp; // safe default.
  }

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
  void NmRsCBUShootDodge::InAirState::Update(float timeStep)
  {
    bspyLogf(info, L"NmRsCBUShootDodge::InAirState::Update(%f)", timeStep);
#elif ShootDodgeBSpyDraw // Added by R*, for fixign the final build
  void NmRsCBUShootDodge::InAirState::Update(float timeStep)
  {
#else
  void NmRsCBUShootDodge::InAirState::Update(float /* timeStep */)
  {
#endif

    NmRsCBUShootDodge::Parameters* parameters = &getContext()->m_parameters;

    // blend to zero pose, all but spine.
    m_character->blendToZeroPose(1.f, bvmask_Full);

#if NM_RUNTIME_LIMITS
    // open some to accommodate animation.
    int i;
    for(i=0; i < m_character->getNumberOfEffectors(); ++i)
      m_character->getEffector(i)->setLimitsToPose(false, parameters->runtimeLimitsMargin);
#endif

    // check if a serious collision has occurred.
    // todo hasCollided should be collision-type sensitive.
    if( getContext()->hasCollided() && 
        ( parameters->collisionReaction != kCollisionDoor ||
        parameters->collisionReaction != kCollisionGlass))
      getContext()->collisionFeedback();

    // if we are about to crash into a wall, try to roll through the crash.
    if( ( parameters->collisionReaction == NmRsCBUShootDodge::kCollisionWall ||
          parameters->collisionReaction == NmRsCBUShootDodge::kCollisionObject ||
          parameters->collisionReaction == NmRsCBUShootDodge::kCollisionFragmentable ) &&
        (parameters->collisionTime > 0.f && parameters->collisionTime < parameters->reOrientTime))
    {
      if(!m_aboutToCrash) // do once.
      {
        // choose which direction to turn based on chest orientation and collision normal.
        rage::Matrix34 chestTM;
        m_character->getSpineSetup()->getSpine3Part()->getMatrix(chestTM);
        m_reOrientAxis.Cross(-chestTM.c, parameters->collisionNormal);

        // scale angle back a bit. the spine pose will take care of some of this.
        m_reOrientAngle = parameters->reOrientScale * rage::Acosf(parameters->collisionNormal.Dot(-chestTM.c));

        m_aboutToCrash = true;

        // send feedback to game to indicate crash is imminent. used to provide
        // crash-specific animation support.
        ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
        if (feedback)
        {
          feedback->m_agentID = m_character->getID();
          ART::ARTFeedbackInterface::FeedbackUserdata data;
          feedback->m_argsCount = 1;
          data.setInt(kPreCollision); // feedback type
          feedback->m_args[0] = data;
          strcpy(feedback->m_behaviourName, NMShootDodgeFeedbackName);
          feedback->onBehaviourEvent();
        }
      }

      float reOrientAmount = 1.f - parameters->collisionTime / parameters->reOrientTime; // tends to 1 as crash progresses.
      getContext()->m_orientationTargetOffset.SetScaled(m_reOrientAxis, reOrientAmount * m_reOrientAngle);

      // switch off hard-keying in case we haven't already.
      getContext()->m_hardKeyed = false;
    }
    else
    {
      getContext()->m_orientationTargetOffset.Zero();
      m_aboutToCrash = false;
    }

    bool collided = m_character->hasCollidedWithWorld(bvmask_Spine & ~(bvmask_Neck | bvmask_Head));
    bool crashing = 
      parameters->collisionTime > 0.f &&
      parameters->collisionTime < parameters->collisionTimeThreshold;

    // check to see if we should kill hard keyframing, if it is on.
    if(getContext()->m_hardKeyed)
    {
      getContext()->m_hardKeyed = parameters->hardKey && !(collided || crashing);

      // send feedback to indicate we are no longer hard keyed.
      if(!getContext()->m_hardKeyed)
    {
        ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
        if (feedback)
        {
          feedback->m_agentID = m_character->getID();
          ART::ARTFeedbackInterface::FeedbackUserdata data;
          feedback->m_argsCount = 2;
          data.setInt(kHardKey); // feedback type
          feedback->m_args[0] = data;
          data.setInt(0);
          feedback->m_args[1] = data;
          strcpy(feedback->m_behaviourName, NMShootDodgeFeedbackName);
          feedback->onBehaviourEvent();
        }
      }
    }
    
    // disable hard keyframing if indicated and it its running.
    if(!getContext()->m_hardKeyed)
      m_character->setIncomingTransformApplyMode(kDisabling);

    // update bracing state machine.
    getContext()->m_bracingStateMachine.Update(timeStep);

    // apply torques to drive towards incoming animation orientation.
    if(!getContext()->m_hardKeyed)
      getContext()->doOrientationControl();

    // add some extra movement in the legs.
    getContext()->legPerlinNoise();

    // wiggle the spine to evoke breathing.
    getContext()->breathing();
  }

  void NmRsCBUShootDodge::InAirState::Exit()
  {
    getContext()->m_orientationTargetOffset.Zero();
    m_aboutToCrash = false;
    getContext()->m_bracingStateMachine.ChangeState((State*)&getContext()->m_idlingState);

    // disable hard keyframing if it its running.
    if(getContext()->m_hardKeyed)
    {
      getContext()->m_hardKeyed = false;
      m_character->setIncomingTransformApplyMode(kDisabling);

      // indicate to the game that we are no longer hard keying.
      ART::ARTFeedbackInterface* feedback = m_character->getFeedbackInterface();
      if (feedback)
      {
        feedback->m_agentID = m_character->getID();
        ART::ARTFeedbackInterface::FeedbackUserdata data;
        feedback->m_argsCount = 2;
        data.setInt(kHardKey); // feedback type
        feedback->m_args[0] = data;
        data.setInt(0);
        feedback->m_args[1] = data;
        strcpy(feedback->m_behaviourName, NMShootDodgeFeedbackName);
        feedback->onBehaviourEvent();
      }
    }
  }

  // collision with ground.
  void NmRsCBUShootDodge::CollideGroundState::Enter()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::CollideGroundState::Enter()");
#endif

    getContext()->m_collisionFeedbackSent = false;

    m_timer = 0.f;
    m_timerOnGround = 0.f;
    m_comVelMag = getContext()->getWeightedCOMVel(0.8f);

    // turn off any sub behaviors that may be running.
    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)getContext()->getCBUParent()->m_tasks[bvid_spineTwist];    
    Assert(spineTwistTask);
    if (spineTwistTask->isActive())
      spineTwistTask->deactivate();
    NmRsCBUSDCatchFall *SDCatchFallTask = (NmRsCBUSDCatchFall *)getContext()->getCBUParent()->m_tasks[bvid_sDCatchFall];    
    Assert(SDCatchFallTask);
    if (SDCatchFallTask->isActive())
      SDCatchFallTask->deactivate();
  }

  void NmRsCBUShootDodge::CollideGroundState::Update(float timeStep)
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::CollideGroundState::Update(%f)", timeStep);
#endif

    Assert(m_character);
    m_timer += m_character->getLastKnownUpdateStep();

    // have we stopped enough for a transition?
    // will normally cause game to transition into on ground state.
    if(getContext()->m_isSupported && m_timerOnGround > 0.05f)
      getContext()->stableFeedback();

    // ramping stiffness upon ground contact to show off the impact.
    // loose at impact, but ramping up quickly to get to the zero pose. 
    float impactRampScale = 1.f;
    if (m_character->hasCollidedWithWorld("fb") || m_timerOnGround > 0.f)
    {
      // send a message back to the game to indicate we have collided with the ground.
      // currently used by the game for some skinning effects.
      if (!(m_timerOnGround > 0.f))
        getContext()->collisionFeedback();

      m_timerOnGround += timeStep;
      float rampTime = 0.15f;
      const float rampMin = 0.25f;
      impactRampScale = rampMin + (1.f-rampMin)*NMutils::clampValue(m_timerOnGround/rampTime,0.f,1.f);
    }

		NmRsCBUShootDodge::Parameters* parameters = &getContext()->m_parameters;

		getContext()->setArmStrength(
			m_character->getLeftArmSetup(),
			impactRampScale*parameters->notAimingArmStiffness,
			parameters->notAimingArmDamping,
			parameters->notAimingArmTaper
			);
		getContext()->setArmStrength(
			m_character->getRightArmSetup(),
			impactRampScale*parameters->notAimingArmStiffness,
			parameters->notAimingArmDamping,
			parameters->notAimingArmTaper
			);
		getContext()->setLegStrength(
			m_character->getLeftLegSetup(),
			impactRampScale*parameters->legStiffness,
			parameters->legDamping,
			parameters->legTaper
			);
		getContext()->setLegStrength(
			m_character->getRightLegSetup(),
			impactRampScale*parameters->legStiffness,
			parameters->legDamping,
			parameters->legTaper
			);

    m_character->getLeftArmSetup()->getWrist()->setMuscleStiffness(parameters->wristMuscleStiffness);
    m_character->getRightArmSetup()->getWrist()->setMuscleStiffness(parameters->wristMuscleStiffness);

    // blend to the zeroPose.
    m_character->blendToZeroPose(1.f,"fb");

    // apply orientation controllers.
    getContext()->doOrientationControl();

    // extra movement of the legs. 
    getContext()->legPerlinNoise();
  }

  void NmRsCBUShootDodge::CollideGroundState::Exit()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::CollideGroundState::Exit()");
#endif
  }

  // collision with wall (or anything else, at the moment).
  void NmRsCBUShootDodge::CollideWallState::Enter()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::CollideWallState::Enter()");
#endif

    m_timer = 0;
    m_timerAfterCollide = 0;
    m_timerSupported = 0;

#if 0
#if 0
    // After we have collided with the wall.
    // Indicate we have had an impact, so the game can do vertex magic.
    getContext()->collisionFeedback();
#endif

    NmRsCBUSDCatchFall* SDCatchFallTask = (NmRsCBUSDCatchFall*)getContext()->getCBUParent()->m_tasks[bvid_sDCatchFall];
    Assert(SDCatchFallTask);

    // activate the SDCatchFall
    if (!SDCatchFallTask->isActive())
    {
      SDCatchFallTask->updateBehaviourMessage(NULL);// sets values to defaults
      SDCatchFallTask->activate(bvid_shootDodge);
    }
    NmRsCBUShootDodge::Parameters* parameters = &getContext()->m_parameters;
    SDCatchFallTask->m_parameters.m_backSideFront = getContext()->getOrientation();
    SDCatchFallTask->m_probeEnd = parameters->collisionPoint;
    SDCatchFallTask->m_groundNormal = parameters->collisionNormal;
    SDCatchFallTask->m_floorVel = parameters->collisionObjectVelocity;
    SDCatchFallTask->m_parameters.m_torsoStiffness = parameters->trunkStiffness;
    SDCatchFallTask->m_parameters.m_armsStiffness = parameters->aimingArmStiffness;
    SDCatchFallTask->m_parameters.m_legsStiffness = parameters->legStiffness;
#else
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)getContext()->getCBUParent()->m_tasks[bvid_catchFall];
    Assert(catchFallTask);

    // activate the catch fall
    if (!catchFallTask->isActive())
    {
      catchFallTask->updateBehaviourMessage(NULL);// sets values to defaults
      // todo try setting 1) m_resistRolling to true or 2) m_comVelRDSThresh to big.
      catchFallTask->m_parameters.m_resistRolling = true;
      catchFallTask->m_parameters.m_comVelRDSThresh = 100.f; // we will never go this fast...
      catchFallTask->activate(bvid_shootDodge);
    }
#endif
  }

  void NmRsCBUShootDodge::CollideWallState::Update(float timeStep)
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::CollideWallState::Update(%f)", timeStep);
#endif

    if (m_character->hasCollidedWithWorld("fb") || m_timerAfterCollide > 0.f)
    {
      // send a message back to the game to indicate we have collided with the wall.
      // currently used by the game for some skinning effects.
      if (!(m_timerAfterCollide > 0.f))
        getContext()->collisionFeedback();
      m_timerAfterCollide += timeStep;
    }

    m_timer += timeStep;

    // have we stopped enough to transition to on-ground?
    if(getContext()->m_isSupported)
      if(m_timerSupported > 0.05f)
        getContext()->stableFeedback();
      else
        m_timerSupported += timeStep;

#if 0
    if (m_character->getSpineSetup()->getHeadPart()->collidedWithNotOwnCharacter())
      m_headCollided = true;

    // do the reaction
    m_timer += m_character->getLastKnownUpdateStep();

    NmRsCBUBodyFoetal* foetalTask = (NmRsCBUBodyFoetal*)getContext()->getCBUParent()->m_tasks[bvid_bodyFoetal];
    Assert(foetalTask);

    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)getContext()->getCBUParent()->m_tasks[bvid_spineTwist];    
    Assert(spineTwistTask);

    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)getContext()->getCBUParent()->m_tasks[bvid_catchFall];
    Assert(catchFallTask);

    NmRsCBUShootDodge::Parameters* parameters = &getContext()->m_parameters;

    // the catchFall part
    if (((m_timer > 0.15f)) || m_protectHead)
    {
      m_protectHead = true;

      if (foetalTask->isActive())
        foetalTask->deactivate();

      if (!catchFallTask->isActive())
      {
        catchFallTask->updateBehaviourMessage(NULL);
        catchFallTask->activate(bvid_shootDodge);
      }
      catchFallTask->m_parameters.m_legsStiffness = parameters->legStiffness*0.6f;
      catchFallTask->m_parameters.m_torsoStiffness = parameters->trunkStiffness;
      catchFallTask->m_parameters.m_armsStiffness = parameters->notAimingArmStiffness;
      catchFallTask->m_parameters.m_useHeadLook = true;

      // set the legs to there current angles
      // So that they naturally fall under gravity.

      const LeftLegSetup* leftLeg = m_character->getLeftLegSetup();
      nmrsSetAngle(leftLeg->getKnee(),nmrsGetActualAngle(leftLeg->getKnee()));
      nmrsSetLean1(leftLeg->getHip(),nmrsGetActualLean1(leftLeg->getHip()));
      nmrsSetLean2(leftLeg->getHip(),nmrsGetActualLean2(leftLeg->getHip()));
      nmrsSetTwist(leftLeg->getHip(),nmrsGetActualTwist(leftLeg->getHip()));

      const RightLegSetup* rightLeg = m_character->getRightLegSetup();
      nmrsSetAngle(rightLeg->getKnee(),nmrsGetActualAngle(rightLeg->getKnee()));
      nmrsSetLean1(rightLeg->getHip(),nmrsGetActualLean1(rightLeg->getHip()));
      nmrsSetLean2(rightLeg->getHip(),nmrsGetActualLean2(rightLeg->getHip()));
      nmrsSetTwist(rightLeg->getHip(),nmrsGetActualTwist(rightLeg->getHip()));
    }
    else
    {
      if (spineTwistTask->isActive())
        spineTwistTask->deactivate();

      if (!foetalTask->isActive())
      {
        foetalTask->updateBehaviourMessage(NULL);// sets values to defaults
        foetalTask->activate(bvid_shootDodge);
        foetalTask->m_parameters.m_effectorMask = bvmask_Full;
        foetalTask->m_parameters.m_stiffness = parameters->trunkStiffness*0.8f;
        foetalTask->m_parameters.m_damping = parameters->trunkDamping;
        foetalTask->m_parameters.m_asymmetrical = 0.5f;
      }
    }

#if 0
      // protect head?
      if (!m_startedToProtectHead)
      {
        rage::Matrix34 curMat;
        m_character->getSpineSetup()->getPelvisPart()->getMatrix(curMat);
        disableArmsBasedOnOrientationOfMax(m_pointingWithLeftArm,m_pointingWithRightArm,m_headLookAtTarget,m_backSideFront, curMat);
      }
#endif
#endif
  }

  void NmRsCBUShootDodge::CollideWallState::Exit()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::CollideWallState::Exit()");
#endif

    NmRsCBUSDCatchFall* SDCatchFallTask = (NmRsCBUSDCatchFall*)getContext()->getCBUParent()->m_tasks[bvid_sDCatchFall];
    Assert(SDCatchFallTask);
    if (SDCatchFallTask->isActive())
      SDCatchFallTask->deactivate();
  
    NmRsCBUBodyFoetal* foetalTask = (NmRsCBUBodyFoetal*)getContext()->getCBUParent()->m_tasks[bvid_bodyFoetal];
    Assert(foetalTask);
    if (foetalTask->isActive())
      foetalTask->deactivate();

    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist *)getContext()->getCBUParent()->m_tasks[bvid_spineTwist];    
    Assert(spineTwistTask);
    if (spineTwistTask->isActive())
      spineTwistTask->deactivate();

    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)getContext()->getCBUParent()->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    if (catchFallTask->isActive())
      catchFallTask->deactivate();
  }


  // on ground state:
  void NmRsCBUShootDodge::OnGroundState::Enter()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::OnGroundState::Enter()");
#endif

    m_timer = 0.f;

    // turn off some sub behaviors that may be running
    NmRsCBUSDCatchFall *sDcatchFallTask = (NmRsCBUSDCatchFall*)getContext()->getCBUParent()->m_tasks[bvid_sDCatchFall];
    Assert(sDcatchFallTask);
    if (sDcatchFallTask->isActive())
      sDcatchFallTask->deactivate();

    NmRsCBUCatchFall *catchFallTask = (NmRsCBUCatchFall*)getContext()->getCBUParent()->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    if (catchFallTask->isActive())
      catchFallTask->deactivate();

#if 0
    // test to see if we think we can go back to animation.
    getContext()->testRecovery();
#endif

    // set up spine twist.
    NmRsCBUSpineTwist *spineTwistTask = (NmRsCBUSpineTwist*)getContext()->getCBUParent()->m_tasks[bvid_spineTwist];
    Assert(spineTwistTask);
    if (!spineTwistTask->isActive())
      spineTwistTask->activate(getContext()->getTaskParent());
    spineTwistTask->setSpineTwistAllwaysTwist(true);
    spineTwistTask->setSpineTwistPos(getContext()->m_parameters.rightArmTarget);
    spineTwistTask->setSpineTwistTwistClavicles(true);

    // reset friction.
    m_character->setFrictionMultiplier(getContext()->m_parameters.frictionScale* 0.5f, bvmask_ArmLeft | bvmask_ArmRight); // keep upper body friction low to help slide into position.

    const SpineSetup* spine = m_character->getSpineSetup();

    // effector settings for this state.
    NmRsCBUShootDodge::Parameters* parameters = &getContext()->m_parameters;
#if 0 // this is being done in onTick now.
    m_character->setBodyStiffness(parameters->trunkStiffness,parameters->trunkDamping,"uk");
    m_character->setBodyStiffness(parameters->headStiffness,parameters->headDamping,"un");
    m_character->setBodyStiffness(parameters->notAimingArmStiffness,parameters->notAimingArmDamping,"ul");
    m_character->setBodyStiffness(parameters->notAimingArmStiffness,parameters->notAimingArmDamping,"ur");
#endif

    m_character->getLeftArmSetup()->getWrist()->setMuscleStiffness(parameters->wristMuscleStiffness);
    m_character->getRightArmSetup()->getWrist()->setMuscleStiffness(parameters->wristMuscleStiffness);

    float legMusStiff = 1.f;
    m_character->setBodyStiffness(parameters->legStiffness,parameters->legDamping,"lb",&legMusStiff);
    
    // do the ankles separately so make sure they don't jitter.
    float ankleStiff = 10.f;
    m_character->getLeftLegSetup()->getAnkle()->setStiffness(11.f,1.f,&ankleStiff);
    m_character->getRightLegSetup()->getAnkle()->setStiffness(11.f,1.f,&ankleStiff);

    // initialize sticky root velocity cache
    int partIndex = spine->getPelvisPart()->getPartIndex();
    rage::Matrix34 current, previous;
    m_character->getITMForPart(partIndex, &current, ART::kITSourceCurrent);
    m_character->getITMForPart(partIndex, &previous, ART::kITSourcePrevious);
    current.DotTranspose(previous);
    rage::Quaternion diff;
    diff.FromMatrix34(current);
    rage::Vector3 unit;
    diff.ToRotation(unit, m_stickyRootVelCache);
    m_stickyRootActive = false;
    m_stickyRootPosition.Zero();

    // clear the drift constraint.
    m_rootConstraint = NULL;
  }

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
  void NmRsCBUShootDodge::OnGroundState::Update(float timeStep)
  {
    bspyLogf(info, L"NmRsCBUShootDodge::OnGroundState::Update(%f)", timeStep);
#else
  void NmRsCBUShootDodge::OnGroundState::Update(float /* timeStep */)
  {
#endif

    // if we are not adequately supported, exit state.
    // keep an eye on this - momentary loss of support may not be a good indicator
    // that we need to leave the state. on the other hand, if game logic is right
    // we should be able to return to this state without incident when support is
    // re-established.
    if(!getContext()->m_isSupported)
      getContext()->unSupportedFeedback();

    m_timer += m_character->getLastKnownUpdateStep();

    NmRsCBUShootDodge::Parameters* parameters = &getContext()->m_parameters;

    // apply torques to drive towards incoming animation orientation.
    getContext()->doOrientationControl();

    // blend fully to zero pose.
    m_character->blendToZeroPose(1.f, bvmask_Full);

    // drive spine to help aim.
    if(getContext()->m_parameters.spineBlend < 1.f)
    {
      if(m_character->getWeaponMode() == kDual)
        getContext()->driveSpine();
      else if(
        m_character->getWeaponMode() == kPistolRight ||
        m_character->getWeaponMode() == kPistol)
        getContext()->driveSpine(-0.5f);
      else // kRifle and kPistolLeft point the left shoulder to the target.
        getContext()->driveSpine(0.65f);
    }

    // blend the spine according to parameter.  skip if blend is zero.
    m_character->blendToZeroPose(getContext()->m_parameters.spineBlend, bvmask_Spine);

    AdjustLegHeight((const LegSetup*)m_character->getLeftLegSetup());
    AdjustLegHeight((const LegSetup*)m_character->getRightLegSetup());

    // left arm is getting caught under the body while rolling.
    // blend against a tucked pose when rolling or obstructed.
    if(true) // replace with actual logic.
    {
      const LeftArmSetup* arm = m_character->getLeftArmSetup();
      nmrsSetLean1(arm->getClavicle(),.2f);
      nmrsSetLean2(arm->getClavicle(),0.f);
      nmrsSetTwist(arm->getClavicle(),0.f);
      nmrsSetLean1(arm->getShoulder(),0.53f);
      nmrsSetLean2(arm->getShoulder(),0.75f);
      nmrsSetTwist(arm->getShoulder(),0.f);
      nmrsSetAngle(arm->getElbow(),2.f);
      nmrsSetLean1(arm->getWrist(),0.f);
      nmrsSetLean2(arm->getWrist(),0.f);
      nmrsSetTwist(arm->getWrist(),0.f);

      // do blend here.
      // blend more if rolling.
      // might want to try detecting snagged hand and blending for short bursts to free.
      
      // blend more if face down.
      rage::Matrix34 s3tm;
      m_character->getSpineSetup()->getSpine3Part()->getMatrix(s3tm);
      float faceDown = s3tm.c.Dot(m_character->m_gUp);

      float blendAmount = 1.f * faceDown;
      blendAmount = rage::Clamp(1.f - blendAmount, 0.f, 1.f);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
      bspyScratchpad(m_character->getBSpyID(), "arm tuck", faceDown);
      bspyScratchpad(m_character->getBSpyID(), "arm tuck", blendAmount);
#endif
  
      m_character->blendToZeroPose(blendAmount, "ul");
    }

    // if root is close to stationary, apply a point constraint
    // to keep character from drifting
    NmRsGenericPart* rootPart = m_character->getSpineSetup()->getPelvisPart();

    // activate sticky root if the rotational velocity of
    // the pelvis incoming transform is less than threshold.
    // this may hinder falling. another check may be necessary here.
    rage::Vector3 rootVel;
    float angle = getITMRotVel(rootPart, &rootVel);
    const float smoove = 0.75f;
    m_stickyRootVelCache *= smoove;
    m_stickyRootVelCache += (1.f-smoove) * angle;

    rage::Vector3 adjustedVel(m_character->m_COMvel - getContext()->m_groundVelocity);
#if ART_ENABLE_BSPY
    // debug constraint criteria.
    bspyScratchpad(m_character->getBSpyID(), "sticky_check", adjustedVel);
    float supported = getContext()->m_supported;
    bspyScratchpad(m_character->getBSpyID(), "sticky_check", supported);
    bool isSupported = getContext()->m_isSupported;
    bspyScratchpad(m_character->getBSpyID(), "sticky_check", isSupported);
    bspyScratchpad(m_character->getBSpyID(), "sticky_check", m_stickyRootVelCache);
    bspyScratchpad(m_character->getBSpyID(), "sticky_check", parameters->stickyRootThresh);
    bspyScratchpad(m_character->getBSpyID(), "sticky_check", m_timer);
#endif
    if(adjustedVel.Mag() < 0.2f && getContext()->m_supported && m_stickyRootVelCache < parameters->stickyRootThresh && m_timer > 1.f) // && m_character->hasCollidedWithWorld("uk|lb"))
    {
      m_stickyRootActive = true;

      // constrain root.
      if(!m_rootConstraint)
      {
        rage::phLevelNew* level = m_character->getLevel();
        Assert(level);
        int index = getContext()->m_parameters.groundInstance;
        NmRsGenericPart* part = m_character->getSpineSetup()->getPelvisPart();
        rage::Vector3 position(part->getPosition());
        rage::phConstraintMgr *mgr = m_character->getSimulator()->GetConstraintMgr();
        Assert(part);
        Assert(mgr);
        if(index != -1 && level->IsInLevel(index))
        {
          // constrain to instance
          rage::phInst* pInst = level->GetInstance(index);
          m_rootConstraint = mgr->AttachObjects(
            'NMf1',
            position,
            position,
            m_character->getFirstInstance(),
            pInst,
            part->getPartIndex(),
            0,
            getContext()->m_parameters.stickyRootRadius,
            false,
            m_rootConstraint);
        }
        else
        {
          // constrain to world
          m_character->fixPart(
            part->getPartIndex(),
            position,
            position,
            getContext()->m_parameters.stickyRootRadius,
            &m_rootConstraint);
        }
      }

#if ART_ENABLE_BSPY & 1
      if(m_rootConstraint)
      {
        m_character->bspyDrawPoint(VEC3V_TO_VECTOR3(m_rootConstraint->GetWorldPosA()), 0.1f, rage::Vector3(1,0,0));
        m_character->bspyDrawSphere(VEC3V_TO_VECTOR3(m_rootConstraint->GetWorldPosB()), getContext()->m_parameters.stickyRootRadius, rage::Vector3(1,1,0));
      }
#endif
    } 
    else
    {
      m_stickyRootActive = false;
      if(m_rootConstraint && m_rootConstraint->IsActive())
      {
        rage::phConstraintMgr *mgr = m_character->getSimulator()->GetConstraintMgr();
        mgr->ReleaseConstraint(m_rootConstraint);
        m_rootConstraint = NULL;
      }
    }

    // extra movement of the legs. 
    getContext()->legPerlinNoise();

    // wiggle the spine to evoke breathing.
    getContext()->breathing();
  }

  void NmRsCBUShootDodge::OnGroundState::Exit()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::OnGroundState::Exit()");
#endif
    m_stickyRootActive = false;
    if(m_rootConstraint && m_rootConstraint->IsActive())
    {
      rage::phConstraintMgr *mgr = m_character->getSimulator()->GetConstraintMgr();
      mgr->ReleaseConstraint(m_rootConstraint);
      m_rootConstraint = NULL;
    }
  }

  float NmRsCBUShootDodge::OnGroundState::getITMRotVel(NmRsGenericPart* part, rage::Vector3* partVel /* = 0 */)
  {
    rage::Vector3 rootVel = part->getLinearVelocity();
    rage::Matrix34 current, previous;
    m_character->getITMForPart(part->getPartIndex(), &current, ART::kITSourceCurrent);
    m_character->getITMForPart(part->getPartIndex(), &previous, ART::kITSourcePrevious);
    current.DotTranspose(previous);
    rage::Quaternion diff;
    diff.FromMatrix34(current);
    rage::Vector3 unit;
    float angle;
    diff.ToRotation(unit, angle);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
    bspyScratchpad(m_character->getBSpyID(), "stickyRoot", angle);
#endif
    if(partVel)
      partVel->SetScaled(unit, angle);
    return angle;
  }

  void NmRsCBUShootDodge::OnGroundState::AdjustLegHeight(const LegSetup* leg)
  {
    // read itm foot height relative to pelvis
    // and adjust hip angles to level them.
    // todo: think about how to deal with uneven
    //       terrain.
    // collect some data.
    rage::Matrix34 footITM, pelvisITM;
    const SpineSetup* spine = m_character->getSpineSetup();
    if(getContext()->m_orientation == kFront)
      // when face down, level knees instead of feet.
      m_character->getJointMatrix1FromParent(footITM, leg->getKnee(), leg->getThigh());
    else
      m_character->getITMForPart(leg->getFoot()->getPartIndex(), &footITM);

    m_character->getITMForPart(spine->getPelvisPart()->getPartIndex(), &pelvisITM);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
    const char* tag;
    if(leg == m_character->getRightLegSetup())
      tag = "sd.right.hipAdjust";
    else
      tag = "sd.left.hipAdjust";
    bspyScratchpad(m_character->getBSpyID(), tag, footITM.d);
    bspyScratchpad(m_character->getBSpyID(), tag, pelvisITM.d);
#endif

    // compute the extra lean needed to level the left leg.
    rage::Vector3 legDirection(footITM.d - pelvisITM.d);
    legDirection.Normalize(); // can omit, I think - leave now for clean debug draw.
    rage::Vector3 axis;
    axis.Cross(legDirection, m_character->m_gUp);
    axis.Normalize();
    float angleError = 0.f;
    float footHeight = pelvisITM.d.z - footITM.d.z;
    const float footHeightThreshold = 0.0f; // need to keep some pressure on the feet to avoid spinning.
    if(footHeight > footHeightThreshold) // we only care about leveling-out foot-ground penetrations.
      angleError = rage::Asinf(footHeight - footHeightThreshold);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
    bspyScratchpad(m_character->getBSpyID(), tag, legDirection);
    bspyScratchpad(m_character->getBSpyID(), tag, axis);
    bspyScratchpad(m_character->getBSpyID(), tag, footHeight);
    bspyScratchpad(m_character->getBSpyID(), tag, angleError);
#endif

    // get current hip matrix1.
    rage::Matrix34 hipTM1;
    leg->getHip()->getMatrix1(hipTM1);

    // transform error axis into hip matrix1 frame.
    hipTM1.UnTransform3x3(axis);

    // make a quat from axis and error.
    rage::Quaternion hipAdjustQuat;
    hipAdjustQuat.FromRotation(axis, angleError);

    // read drive quat from left hip.
    rage::Quaternion driveQuat;
    leg->getHip()->getQuaternionFromRawAngles(driveQuat);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
    rage::Vector3 checkDrive;
    driveQuat.Transform(rage::Vector3(0.f, 0.f, 1.f), checkDrive);
    hipTM1.Transform3x3(checkDrive);
    bspyScratchpad(m_character->getBSpyID(), tag, hipTM1.c);
    bspyScratchpad(m_character->getBSpyID(), tag, checkDrive);
#endif

    // combine current and adjust rotations.
    rage::Quaternion adjustedDrive;
    adjustedDrive.Multiply(hipAdjustQuat, driveQuat);

    // apply adjusted drive to effector.
    rage::Vector3 ts = rsQuatToRageDriveTwistSwing(adjustedDrive);
    leg->getHip()->getTwistAndSwingFromRawTwistAndSwing(ts, ts);
    leg->getHip()->setDesiredLean1(ts.y);
    leg->getHip()->setDesiredLean2(ts.z);
    leg->getHip()->setDesiredTwist(ts.x);

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
    // debug pelvis orientation matching
    rage::Matrix34 pelvisTM;
    spine->getPelvisPart()->getMatrix(pelvisTM);
    pelvisTM.d = pelvisITM.d;
    m_character->bspyDrawCoordinateFrame(0.1f, pelvisITM);
    m_character->bspyDrawCoordinateFrame(0.065f, pelvisTM);
#endif
  }

  void NmRsCBUShootDodge::BailState::Enter()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::BailState::Enter()");
#endif

#if 1
    NmRsCBUBodyWrithe* bodyWritheTask = (NmRsCBUBodyWrithe*)getContext()->getCBUParent()->m_tasks[bvid_bodyWrithe];
    Assert(bodyWritheTask);
    if (!bodyWritheTask->isActive())
    {
      bodyWritheTask->updateBehaviourMessage(NULL);
      bodyWritheTask->activate(bvid_shootDodge);
    }
#else
    // do a *real* catch fall.
    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)getContext()->getCBUParent()->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    if (!catchFallTask->isActive())
    {
      catchFallTask->updateBehaviourMessage(NULL);// sets values to defaults
      catchFallTask->activate(bvid_shootDodge);
    }
#endif
  }
  
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
  void NmRsCBUShootDodge::BailState::Update(float timeStep)
  {
    bspyLogf(info, L"NmRsCBUShootDodge::BailState::Update(%f)", timeStep);
#else
  void NmRsCBUShootDodge::BailState::Update(float /* timeStep */)
  {
#endif
    // check if a serious collision has occurred.
    if(getContext()->hasCollided())
      getContext()->collisionFeedback(); 

    // look ahead down trajectory (may be able to use game input for this).
    rage::Vector3 pos, normal;
    float time = 0;
    bool hit = getContext()->probeTrajectory(&pos, &normal, &time, 0.5f);
    if(hit)
    {
      getContext()->m_parameters.state = kCollideWall;
      m_fsm->ChangeState(getContext()->m_states[kCollideWall]);
    }
  }

  void NmRsCBUShootDodge::BailState::Exit()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::BailState::Exit()");
#endif

    NmRsCBUCatchFall* catchFallTask = (NmRsCBUCatchFall*)getContext()->getCBUParent()->m_tasks[bvid_catchFall];
    Assert(catchFallTask);
    if (catchFallTask->isActive())
      catchFallTask->deactivate();

    NmRsCBUBodyWrithe* bodyWritheTask = (NmRsCBUBodyWrithe*)getContext()->getCBUParent()->m_tasks[bvid_bodyWrithe];
    Assert(bodyWritheTask);
    if (bodyWritheTask->isActive())
      bodyWritheTask->deactivate();
  }

  // state machine for pointing.
  // when in pointing state, ensures Point Gun is active, configured and receiving targets.
  // when in reload state, plays reload animation through IK.

  void NmRsCBUShootDodge::PointingGlobalState::Enter()
  {

  }

  void NmRsCBUShootDodge::PointingGlobalState::Update(float /*timeStep*/)
  {

    // check state transition
    if(	getContext()->m_parameters.armAnimTask &&
    getContext()->m_stateMachine.CurrentState() == (State*)&getContext()->m_onGroundState && // temporary - reload anims only work on ground.
    getContext()->m_orientation != kFront) // temporary - reload anims do weird pushup thing when face down.
      m_fsm->ChangeState(&getContext()->m_reloadState);
    else if(getContext()->m_stateMachine.CurrentState() == (State*)&getContext()->m_collideWallState)
      m_fsm->ChangeState(&getContext()->m_notPointingState);
    else
      m_fsm->ChangeState(&getContext()->m_pointingState);
  }

  void NmRsCBUShootDodge::PointingGlobalState::Exit()
  {

  }

  void NmRsCBUShootDodge::PointingState::Enter()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::PointingState::Enter()");
#endif

    // start point gun task.
    NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)getContext()->getCBUParent()->m_tasks[bvid_pointGun];
    Assert(pointGunTask);
    pointGunTask->updateBehaviourMessage(NULL);
    pointGunTask->m_parameters.leftHandParentEffector = gtaJtShoulder_Left;
    pointGunTask->m_parameters.rightHandParentEffector = gtaJtShoulder_Right;
    pointGunTask->m_parameters.leftHandTargetIndex = -1;
    pointGunTask->m_parameters.rightHandTargetIndex = -1;
    pointGunTask->m_parameters.useIncomingTransforms = true;
    if(!pointGunTask->isActive())
      pointGunTask->activate(bvid_shootDodge);

    if(getContext()->m_parameters.useHeadLook)
    {
    // activate head look if necessary.
    NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)getContext()->m_cbuParent->m_tasks[bvid_headLook];
    Assert(headLookTask);
    if (!headLookTask->isActive())
    {
      headLookTask->updateBehaviourMessage(NULL);
#if NM_SHOOTDODGE_NO_SPINE_CONTROL
      headLookTask->setBvMask(bvmask_Neck | bvmask_Head);
      headLookTask->activate();
#else
      headLookTask->activate(m_taskParent);
#endif
    }
  }
  }

  void NmRsCBUShootDodge::PointingState::Update(float /*timeStep*/)
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::PointingState::Update()");
#endif

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    m_character->setSkeletonVizRoot(10);
    m_character->setSkeletonVizMode(NmRsCharacter::kSV_DesiredAngles);
    getContext()->m_mask |= bvmask_ArmRight;
    WeaponMode weaponMode = m_character->getWeaponMode();
    if(weaponMode == kDual || weaponMode == kRifle)
      getContext()->m_mask |= bvmask_ArmLeft;
    m_character->setSkeletonVizMask(getContext()->m_mask);
#endif

    // update point gun target
    getContext()->passThroughPointGunParams();
  }

  void NmRsCBUShootDodge::PointingState::Exit()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::PointingState::Exit()");
#endif

    NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)getContext()->getCBUParent()->m_tasks[bvid_pointGun];
    Assert(pointGunTask);
    if(pointGunTask->isActive())
      pointGunTask->deactivate();
  }

  void NmRsCBUShootDodge::NotPointingState::Enter()
  {

  }

  void NmRsCBUShootDodge::NotPointingState::Update(float /*timeStep*/)
  {
  }

  void NmRsCBUShootDodge::NotPointingState::Exit()
  {
  }

  void NmRsCBUShootDodge::ReloadState::Enter()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::ReloadState::Enter()");
#endif

#if 0 // done in onTick() now.
    m_character->setBodyStiffness(getContext()->m_parameters.notAimingArmStiffness, getContext()->m_parameters.notAimingArmDamping, "ua");
#endif

    // disable collisions with the hands.
    m_character->m_rightHandCollisionExclusion.setB(bvmask_HighSpine | bvmask_HandLeft | bvmask_ForearmLeft);
    m_character->m_leftHandCollisionExclusion.setB(bvmask_HighSpine | bvmask_HandRight | bvmask_ForearmRight);
  }

  void NmRsCBUShootDodge::ReloadState::Update(float /*timeStep*/)
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::ReloadState::Update()");
#endif

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    m_character->setSkeletonVizRoot(10);
    m_character->setSkeletonVizMode(NmRsCharacter::kSV_DesiredAngles);
    getContext()->m_mask |= bvmask_ArmRight | bvmask_ArmLeft;
    m_character->setSkeletonVizMask(getContext()->m_mask);
#endif

    // update arm stiffness. needed to ensure timewarp
    // compensation is correct.
    NmRsCBUShootDodge::Parameters* parameters = &getContext()->m_parameters;
    m_character->setBodyStiffness(parameters->aimingArmStiffness * getContext()->m_timeWarpScale, parameters->aimingArmDamping, bvmask_ArmRight | bvmask_ArmLeft);

    reloadAction(getContext()->m_parameters.leftArmAnimTaskParent, m_character->getLeftArmSetup());
    reloadAction(getContext()->m_parameters.rightArmAnimTaskParent, m_character->getRightArmSetup());
  }

  void NmRsCBUShootDodge::ReloadState::Exit()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::ReloadState::Exit()");
#endif

    // re-enable collisions with the hands. point gun ticks after shootdodge and will reset if necessary.
    m_character->m_rightHandCollisionExclusion.setB(bvmask_None);
    m_character->m_leftHandCollisionExclusion.setB(bvmask_None);
  }

  // IK the arms to the desired position of the arms in the incoming transforms relative
  // to the parent part position. 
  void NmRsCBUShootDodge::ReloadState::reloadAction(int parentIndex, const ArmSetup* arm)
  {
    // get the current incoming transforms
    int incomingComponentCount = 0;
    rage::Matrix34 *itPtr = 0;
    ART::IncomingTransformStatus itmStatusFlags = ART::kITSNone;
    m_character->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, ART::kITSourceCurrent);
    rage::Matrix34 desMat;

    // get the incoming transforms of the hand and the parent
    // as well as the current parent transform and hand position.
    rage::Matrix34 parentITM, parentCurrentTM, handITM;
    m_character->getITMForPart(parentIndex, &parentITM);
    m_character->getITMForPart(arm->getHand()->getPartIndex(), &handITM);
    m_character->getGenericPartByIndex(parentIndex)->getMatrix(parentCurrentTM);
    rage::Vector3 handCurrentPos = arm->getHand()->getPosition();

    // map offset into parent itm local... and back out to world via parent part tm.
    rage::Vector3 handOffset = handITM.d - parentITM.d;
    parentITM.UnTransform3x3(handOffset);
    parentCurrentTM.Transform3x3(handOffset);

    // apply the offset.
    rage::Vector3 desiredIKHandPos = parentCurrentTM.d + handOffset;

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
    bspyScratchpad(m_character->getBSpyID(), "reloadAction", desiredIKHandPos);
#endif

    // Do the ik + cleverIK
    float tempDistance = 0.f;
    float hingeDirection = 1.f;
    float twist = 0.5f;
    if(arm == m_character->getRightArmSetup())
      hingeDirection = -1.f;
    m_character->C_LimbIK(arm, hingeDirection, 1.f, false, &desiredIKHandPos, &twist);       
    m_character->cleverHandIK(desiredIKHandPos, arm, hingeDirection, false, getContext()->m_timeWarpScale * 1.f, NULL, tempDistance, m_character->getGenericPartByIndex(parentIndex), -1, 0.3f, 0);
  }

  // state machine to control bracing and catch-fall behavior.
  void NmRsCBUShootDodge::BracingGlobalState::Update(float /*timeStep*/)
  {

  }

  void NmRsCBUShootDodge::BracingState::Enter()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::BracingState::Enter()");
#endif

    m_rightBracing = false;
    m_leftBracing = false;
    m_rightContact = false;
    m_leftContact = false;

    // disable head look.
    NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)getContext()->getCBUParent()->m_tasks[bvid_headLook];
    Assert(headLookTask);
    if(headLookTask->isActive())
      headLookTask->deactivate();
  }
  
  void NmRsCBUShootDodge::BracingState::Update(float /* timeStep */)
  {

    // jrp remove.
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    getContext()->m_mask |= bvmask_ArmRight;
#endif

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::BracingState::Update()");
#endif

    bool armStatusChanged = false;

    NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)getContext()->getCBUParent()->m_tasks[bvid_pointGun];
    Assert(pointGunTask);

    rage::Matrix34 spine3TM;
    m_character->getSpineSetup()->getSpine3Part()->getMatrix(spine3TM);
    rage::Vector3 collisionDirection(getContext()->m_parameters.collisionPoint - spine3TM.d);
    collisionDirection.Normalize();

    const float directionDotSpine3Y = collisionDirection.Dot(spine3TM.b);
    const float directionDotSpine3Z = collisionDirection.Dot(spine3TM.c);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
    m_character->bspyDrawCoordinateFrame(0.1f, spine3TM);
    bspyScratchpad(m_character->getBSpyID(), "BracingState", directionDotSpine3Y);
    bspyScratchpad(m_character->getBSpyID(), "BracingState", directionDotSpine3Z);
#endif

    NmRsCBUShootDodge::Parameters* parameters = &getContext()->m_parameters;

    // do the right one.
    // right arm coverage includes right side, not quite
    // to straight ahead.
    const float minRightAngle = 0.3f; // -0.1f;
    float minCollisionTime = 0.3f;
    // rifle is longer and needs a little more time.
    if(m_character->getWeaponMode() == kRifle)
      minCollisionTime = 0.6f;

    // never brace with right arm if colliding with the ground.
    if( getContext()->m_parameters.collisionReaction != kCollisionGround &&
        parameters->collisionTime < minCollisionTime &&
        directionDotSpine3Y < minRightAngle)
    { 
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
      getContext()->m_mask |= bvmask_ArmRight;
#endif

      // has hand made contact?
      m_rightContact = m_character->hasCollidedWithEnvironment(bvmask_ArmRight);

      // disable timewarp scaling if we have hit.
      if(m_rightContact)
        m_character->setBodyStiffness(getContext()->m_parameters.aimingArmStiffness, getContext()->m_parameters.aimingArmDamping, "ur");

      // disable right arm pointing.
      pointGunTask->m_parameters.enableRight = false;

      // ik.
      armIK(m_character->getRightArmSetup());

      // look at collision point.
      NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)getContext()->getCBUParent()->m_tasks[bvid_headLook];
      Assert(headLookTask);
      headLookTask->m_parameters.m_pos = getContext()->m_parameters.collisionPoint;

      if(!m_rightBracing)
      {
        m_rightBracing = true;
        armStatusChanged = true;
      }
    }
    else
    {
      if(m_rightBracing)
      {
        m_rightBracing = false;
        armStatusChanged = true;
      }
    }

    // if collision direction is left-ish brace with left arm. 
    // (left arm coverage is generous to allow us to minimize 
    // disruption to the right (normally pointing) arm).
    const float minLeftAngle = -0.4f;
    if( (directionDotSpine3Y > minLeftAngle) ||
        (directionDotSpine3Z < 0.f) ||
        (m_rightBracing && m_character->getWeaponMode() == kRifle))
    {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
      getContext()->m_mask |= bvmask_ArmLeft;
#endif

      // has hand made contact?
      m_leftContact = m_character->hasCollidedWithEnvironment(bvmask_ArmLeft);

      // disable timewarp scaling if we have hit.
      if(m_leftContact)
        m_character->setBodyStiffness(parameters->aimingArmStiffness, parameters->aimingArmDamping, "ul");

      // disable left arm pointing.
      pointGunTask->m_parameters.enableLeft = false;

      // ik.
      armIK(m_character->getLeftArmSetup());

      if(!m_leftBracing)
      {
        m_leftBracing = true;
        armStatusChanged = true;
      }
    }
    else
    {
      if(m_leftBracing)
      {
        m_leftBracing = false;
        armStatusChanged = true;
      }
    }

    // if we are crashing into a wall, try to tuck legs up under body.
    // this will help us not get stuck in corners.
    // jrp todo: state logic will not allow this to happen ever...
    if(getContext()->m_parameters.state == kCollideWall)
    {
      if(m_rightContact || m_leftContact)
        m_character->setBodyStiffness(getContext()->m_parameters.legStiffness, getContext()->m_parameters.legDamping, "lb");
      
      const float distance = 0.7f;
      const float offset = 0.2f;
      const float collisionPrecendece = 0.5f;

      rage::Matrix34 pelvisTm;
      m_character->getSpineSetup()->getPelvisPart()->getMatrix(pelvisTm);
      rage::Vector3 spine3Position(m_character->getSpineSetup()->getSpine3Part()->getPosition());
      rage::Vector3 collisionDirection(getContext()->m_parameters.collisionPoint - spine3Position);
      collisionDirection.Scale(collisionPrecendece);
      collisionDirection.AddScaled(-m_character->m_gUp, 1.f-collisionPrecendece);
      collisionDirection.Normalize();
      rage::Vector3 target(m_character->getLeftLegSetup()->getHip()->getJointPosition());
      target.AddScaled(collisionDirection, distance);
      target.AddScaled(pelvisTm.c, offset);
      m_character->leftLegIK(target, 0.f, 0.f);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "brace.leftleg", target);
#endif
      m_character->getLeftLegSetup()->getAnkle()->setDesiredLean1(0.5f);

      target.Set(m_character->getRightLegSetup()->getHip()->getJointPosition());
      target.AddScaled(collisionDirection, distance);
      target.AddScaled(pelvisTm.b, -offset);
      m_character->rightLegIK(target, 0.f, 0.f);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
      bspyScratchpad(m_character->getBSpyID(), "brace.rightleg", target);
#endif
      m_character->getRightLegSetup()->getAnkle()->setDesiredLean1(0.5f);

    }

#if 0
    // turn away from collision
    rage::Matrix34 pelvisTM;
    m_character->getSpineSetup()->getSpine3Part()->getMatrix(pelvisTM);
    const float startTurningTime = 0.5f;
    float turnScale = 1.f - rage::Clamp(parameters->collisionTime, 0.f, startTurningTime) / startTurningTime;
#if ART_ENABLE_BSPY
    bspyScratchpad_Float(m_character->getBSpyID(), "brace", turnScale);
#endif
    float backTwist = rage::Acosf(parameters->collisionNormal.Dot(-pelvisTM.c)) * turnScale;
    if(parameters->collisionNormal.Dot(pelvisTM.b) < 0.f)
      backTwist *= -1;
    const SpineSetup* spine = m_character->getSpineSetup();
    nmrsSetTwist(spine->getSpine0(), backTwist / 4.f);
    nmrsSetTwist(spine->getSpine1(), backTwist / 4.f);
    nmrsSetTwist(spine->getSpine2(), backTwist / 4.f);
    nmrsSetTwist(spine->getSpine3(), backTwist / 4.f);
    nmrsSetTwist(spine->getLowerNeck(), backTwist / 4.f);
    nmrsSetTwist(spine->getUpperNeck(), backTwist / 4.f);
#else
    // turn away from collision
    const SpineSetup* spine = m_character->getSpineSetup();
    rage::Matrix34 pelvisTM;
    spine->getSpine3Part()->getMatrix(pelvisTM);

    // ramp up turn away as we approach collision
    const float startTurningTime = 0.5f;
    float turnScale = 1.f - rage::Clamp(parameters->collisionTime, 0.f, startTurningTime) / startTurningTime;
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "brace", turnScale);
#endif

    // bend spine away from collision
    rage::Vector3 axisWorld;
    // maximum desired bend is less than 90 degrees
    // todo something wrong with angle calc.  coming through too small...
    float angle = rage::Acosf(getContext()->m_collisionDirection.Dot(-pelvisTM.a)) - PI / 2.f;
    angle = rage::Clamp(angle, 0.f, PI / 2.f);
    angle *= turnScale; 
    angle /= 4.f; // spread across spine joints
    axisWorld.Cross(getContext()->m_collisionDirection, pelvisTM.a);
    axisWorld.Normalize();
#if ART_ENABLE_BSPY
    bspyScratchpad(m_character->getBSpyID(), "brace", axisWorld);
    bspyScratchpad(m_character->getBSpyID(), "brace", angle);
#endif

    // put axis in joint frame. grouping the four spine effectors
    // is not strictly accurate, but they are orientated similarly
    // so we can save some time.
    rage::Vector3 axisLocal;
    rage::Quaternion driveQ;
    rage::Matrix34 jointFrame;
    spine->getSpine0()->getMatrix1(jointFrame);
    jointFrame.UnTransform3x3(axisWorld, axisLocal);
    driveQ.FromRotation(axisLocal, angle);

    // drive joints.
    rage::Vector3 ts = rsQuatToRageDriveTwistSwing(driveQ);
    spine->getSpine0()->getTwistAndSwingFromRawTwistAndSwing(ts, ts);
    spine->getSpine0()->setDesiredTwist(ts.x); spine->getSpine0()->setDesiredLean1(ts.y); spine->getSpine0()->setDesiredLean2(ts.z);

    ts = rsQuatToRageDriveTwistSwing(driveQ);
    spine->getSpine1()->getTwistAndSwingFromRawTwistAndSwing(ts, ts);
    spine->getSpine1()->setDesiredTwist(ts.x); spine->getSpine1()->setDesiredLean1(ts.y); spine->getSpine1()->setDesiredLean2(ts.z);

    ts = rsQuatToRageDriveTwistSwing(driveQ);
    spine->getSpine2()->getTwistAndSwingFromRawTwistAndSwing(ts, ts);
    spine->getSpine2()->setDesiredTwist(ts.x); spine->getSpine2()->setDesiredLean1(ts.y); spine->getSpine2()->setDesiredLean2(ts.z);

    ts = rsQuatToRageDriveTwistSwing(driveQ);
    spine->getSpine3()->getTwistAndSwingFromRawTwistAndSwing(ts, ts);
    spine->getSpine3()->setDesiredTwist(ts.x); spine->getSpine3()->setDesiredLean1(ts.y); spine->getSpine3()->setDesiredLean2(ts.z);

    ts = rsQuatToRageDriveTwistSwing(driveQ);
    spine->getLowerNeck()->getTwistAndSwingFromRawTwistAndSwing(ts, ts);
    spine->getLowerNeck()->setDesiredTwist(ts.x); spine->getLowerNeck()->setDesiredLean1(ts.y); spine->getLowerNeck()->setDesiredLean2(ts.z);


    // now get the head out of the way.  put axis in the upper neck
    // frame.
    spine->getUpperNeck()->getMatrix1(jointFrame);
    jointFrame.UnTransform3x3(axisWorld, axisLocal);
    driveQ.FromRotation(axisLocal, angle);

    // drive joint.
    ts = rsQuatToRageDriveTwistSwing(driveQ);
    spine->getUpperNeck()->getTwistAndSwingFromRawTwistAndSwing(ts, ts);
    spine->getUpperNeck()->setDesiredTwist(ts.x); spine->getUpperNeck()->setDesiredLean1(ts.y); spine->getUpperNeck()->setDesiredLean2(ts.z);

#if 0
    // do neck twist.
    float backTwist = rage::Acosf(parameters->collisionNormal.Dot(-pelvisTM.c)) * turnScale;
    if(parameters->collisionNormal.Dot(pelvisTM.b) < 0.f)
      backTwist *= -1;
    nmrsSetTwist(spine->getLowerNeck(), backTwist / 4.f);
    nmrsSetTwist(spine->getUpperNeck(), backTwist / 4.f);
#endif
#endif

    // indicate to the game that one or more arms are bracing.
    if(armStatusChanged)
    {
      getContext()->armStatusFeedback(m_leftBracing, m_rightBracing);
      pointGunTask ->m_parameters.enableLeft = !m_leftBracing;
      pointGunTask ->m_parameters.enableRight = !m_rightBracing;

      if(!m_leftBracing)
        m_leftContact = false;
      if(!m_rightBracing)
        m_rightContact = false;
    } 
  }

  void NmRsCBUShootDodge::BracingState::Exit()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::BracingState::Exit()");
#endif

    // if we have stopped bracing this tick, send feedback.
    if(m_leftBracing || m_rightBracing)
      getContext()->armStatusFeedback(false, false);
  }

  void NmRsCBUShootDodge::BracingState::armIK(const ArmSetup* arm)
  {
    // some common values.
    float armTwist = -0.5f; // todo: adjust based on direction and collision type.
    bool useActualAngles = false;
    float twistLimit = 2.4f;
    rage::Matrix34 spine3tm;
    m_character->getSpineSetup()->getSpine3Part()->getMatrix(spine3tm);

    // get collision direction
    // todo: move to common update state to avoid duplication.
    rage::Vector3 claviclePosition(arm->getClaviclePart()->getPosition());
    rage::Vector3 spine3Position(m_character->getSpineSetup()->getSpine3Part()->getPosition());
    rage::Vector3 collisionDirection(getContext()->m_parameters.collisionPoint - spine3Position);
    const float maxBraceDistance = 0.5f; // 0.6f; // may want to parameterize this.

    // set brace distance based on collision distance and our max reach.
    float braceDistance = rage::Clamp(collisionDirection.Mag(), 0.2f, maxBraceDistance);
    collisionDirection.Normalize();

    // offset bracing direction a bit if both arms are bracing.
    if(m_leftBracing && m_rightBracing)
    {
      float angle = 0.15f;
      if(arm == m_character->getRightArmSetup())
        angle *= -1.f;
      rage::Vector3 axis;
      axis.Cross(collisionDirection, spine3tm.b);
      axis.Normalize();
      rage::Quaternion q;
      q.FromRotation(axis, angle);
      q.Transform(collisionDirection);
    }

    // catch fall arms.

    // pick a direction in chest space to represent the center of
    // the arm's working area.
    rage::Vector3 workingCenterLocal(0.f, arm == m_character->getLeftArmSetup() ? 1.f : -1.f, -1.f);
    workingCenterLocal.Normalize(); // todo: hard code to avoid normalization.

    // put collision direction in chest space.
    rage::Vector3 collisionDirectionLocal;
    spine3tm.UnTransform3x3(collisionDirection, collisionDirectionLocal);

    // is collision direction outside working area as defined by center
    // and angle?
    const float includeAngle = -0.2f;
    float centerDotDirection = workingCenterLocal.Dot(collisionDirectionLocal);
    if(centerDotDirection < includeAngle)
    {
      // project onto the surface of the working cone.
      // may want to bias this in one direction or another...
      rage::Vector3 axis;
      axis.Cross(collisionDirectionLocal, workingCenterLocal);
      //float angle = axis.Mag();
      float angle = rage::Acosf(centerDotDirection) - rage::Acosf(includeAngle);
      axis.Normalize(); // is there a single func for this?
      rage::Quaternion rot;
      rot.FromRotation(axis, angle);
      rot.Transform(collisionDirectionLocal);

      spine3tm.Transform3x3(collisionDirectionLocal, collisionDirection);

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 0
      // check your work.
      float totalAngle = rage::Acosf(centerDotDirection);
      float limitAngle = rage::Acosf(includeAngle);
      bspyScratchpad(m_character->getBSpyID(), "arm.ik.check", totalAngle);
      bspyScratchpad(m_character->getBSpyID(), "arm.ik.check", limitAngle);
      bspyScratchpad(m_character->getBSpyID(), "arm.ik.check", angle);

      rage::Vector3 workingCenterWorld, axisWorld;
      spine3tm.Transform3x3(workingCenterLocal, workingCenterWorld);
      spine3tm.Transform3x3(axis, axisWorld);
      m_character->bspyDrawLine(claviclePosition, claviclePosition+workingCenterWorld, rage::Vector3(1,0,0));
      m_character->bspyDrawLine(claviclePosition, claviclePosition+collisionDirection, rage::Vector3(0,1,0));
      m_character->bspyDrawLine(claviclePosition, claviclePosition+axisWorld, rage::Vector3(1,1,0));
#endif
    }

    // probe down collision direction to set brace distance.
    //mmmmtodo check that the masks are correct
    //if(m_character->probeRay(claviclePosition, claviclePosition+collisionDirection*maxBraceDistance, &hitPos, &hitNorm, &component, false))
    if(m_character->probeRay(m_character->pi_UseNonAsync, claviclePosition, claviclePosition+collisionDirection*maxBraceDistance, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false))
      braceDistance = claviclePosition.Dist(m_character->m_probeHitPos[m_character->pi_UseNonAsync]);

    // set ik target at bracing distance.
    rage::Vector3 target(claviclePosition + collisionDirection * braceDistance);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    if(arm == m_character->getLeftArmSetup()) {
      bspyScratchpad(m_character->getBSpyID(), "shootDodge_brace_left", target);
    } else {
      bspyScratchpad(m_character->getBSpyID(), "shootDodge_brace_right", target);
    }
#endif

    // do the ik.
    //rage::Vector3 vel = arm->getClaviclePart()->getLinearVelocity();
    if(arm == m_character->getLeftArmSetup())
      m_character->leftArmIK(target, armTwist, /* 1.f */ 0.f, &m_character->m_COMvel);
    else
      m_character->rightArmIK(target, armTwist, /* 1.f */ 0.f, &m_character->m_COMvel);
    m_character->matchClavicleToShoulder(arm->getClavicle(), arm->getShoulder());

    // orient palm to collision normal.
    collisionDirection.Set(-getContext()->m_parameters.collisionNormal);
    if(arm == m_character->getLeftArmSetup())
    {
      m_character->leftWristIK(target, -collisionDirection, &useActualAngles, &twistLimit);
    }
    else
    {
      m_character->rightWristIK(target, -collisionDirection, &useActualAngles, &twistLimit);
    }
  }

  void NmRsCBUShootDodge::DefendingState::Enter()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::DefendingState::Enter()");
#endif
    // disable point gun arms.
    NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)getContext()->getCBUParent()->m_tasks[bvid_pointGun];
    Assert(pointGunTask);
    pointGunTask->m_parameters.enableLeft = false;

    // disable head look.
    NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)getContext()->getCBUParent()->m_tasks[bvid_headLook];
    Assert(headLookTask);
    if(headLookTask->isActive())
      headLookTask->deactivate();
  }

#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
  void NmRsCBUShootDodge::DefendingState::Update(float timeStep)
  {
    bspyLogf(info, L"NmRsCBUShootDodge::DefendingState::Tick(%d)", timeStep);
#else
  void NmRsCBUShootDodge::DefendingState::Update(float /*timeStep*/)
  {
#endif

    NmRsCBUShootDodge::Parameters* parameters = &getContext()->m_parameters;

#if 0 // done in onTick() now.
    // add some strength to the neck to
    // override head look settings.
    m_character->setBodyStiffness(parameters->trunkStiffness*getContext()->m_timeWarpScale,
                                  parameters->trunkDamping,
                                  bvmask_ArmLeft | bvmask_ArmRight | bvmask_Spine);
#endif

    // pose back and arms a la body foetal.
    // twist back to face away from collision normal.
    rage::Matrix34 pelvisTM;
    m_character->getSpineSetup()->getSpine3Part()->getMatrix(pelvisTM);
    float backTwist = rage::Acosf(parameters->collisionNormal.Dot(-pelvisTM.c));
    if(parameters->collisionNormal.Dot(pelvisTM.b) < 0.f)
      backTwist *= -1;
    const SpineSetup* spine = m_character->getSpineSetup();
    const LeftArmSetup* leftArm = m_character->getLeftArmSetup();
    const RightArmSetup* rightArm = m_character->getRightArmSetup();
    m_character->setBackAngles(1.f, 0.f, backTwist);
    nmrsSetLean1(spine->getLowerNeck(), 0.5f);
    nmrsSetLean1(spine->getUpperNeck(), 0.5f);

    // todo. drive clavicle. check that point gun will
    // not overwrite with ITMs.

    // pose the arms to defend the head.
    // todo. merge with brace state such that he braces when
    // shoulder is pointed roughly at the impact and covers
    // otherwise.

    // don't override the right arm pointing unless we are crashing
    // head-, front- or right side- first.
    NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)getContext()->getCBUParent()->m_tasks[bvid_pointGun];
    Assert(pointGunTask);
    if(getContext()->m_collisionOrientation == kHead || getContext()->m_collisionOrientation == kFront || getContext()->m_collisionOrientation == kRightSide) 
    {
      nmrsSetTwist(rightArm->getShoulder(), 0.f);
      nmrsSetLean1(rightArm->getShoulder(), 1.57f);
      nmrsSetAngle(rightArm->getElbow(), 2.f);
      pointGunTask->m_parameters.enableRight = false;
    }
    else
    {
      pointGunTask->m_parameters.enableRight = true;
    }
    nmrsSetTwist(leftArm->getShoulder(), 0.f);
    nmrsSetLean1(leftArm->getShoulder(), 1.57f);
    nmrsSetAngle(rightArm->getElbow(), 2.f);
    nmrsSetAngle(leftArm->getElbow(), 2.f);

    // try to place legs below impact point. this will encourage
    // character to fall away from the obstruction. may only be
    // useful for wall collisions.
    if(getContext()->m_orientation == kBack) // if we colliding with the back, we should just sit down.
    {
      m_character->setBodyStiffness(parameters->legStiffness, parameters->legDamping, "lb");
      nmrsSetTwist(m_character->getLeftLegSetup()->getHip(), 0.1f);
      nmrsSetLean1(m_character->getLeftLegSetup()->getHip(), 1.0f);
      nmrsSetLean2(m_character->getLeftLegSetup()->getHip(), -0.3f);
      nmrsSetTwist(m_character->getRightLegSetup()->getHip(), 0.1f);
      nmrsSetLean1(m_character->getRightLegSetup()->getHip(), 1.0f);
      nmrsSetLean2(m_character->getRightLegSetup()->getHip(), -0.3f);
    }
    else
    {
#if 0 // done in onTick() now.
      m_character->setBodyStiffness(parameters->legStiffness*getContext()->m_timeWarpScale, parameters->legDamping, "lb");
#endif

      // base target is below contact point.
      rage::Vector3 baseTarget(getContext()->m_parameters.collisionPoint);
      baseTarget.SubtractScaled(m_character->m_gUp, getContext()->m_pelvisHeight);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 1
      m_character->bspyDrawPoint(baseTarget, 0.05f, rage::Vector3(1,0,1));
#endif

      // offset individual targets along vector perpendicular to collision
      // normal and up vector. for walls, this should end up being the base
      // of the wall.
      const float offset = 0.2f;
      rage::Vector3 offsetDirection;
      offsetDirection.Cross(parameters->collisionNormal, m_character->m_gUp);
      if(offsetDirection.Dot(pelvisTM.b) < 0.f)
        offsetDirection.Scale(-1.f);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 1
      m_character->bspyDrawLine(baseTarget, baseTarget + offsetDirection, rage::Vector3(1,0,1));
#endif

      // do left.
      rage::Vector3 target(baseTarget);
      target.AddScaled(offsetDirection, offset);
      m_character->leftLegIK(target, 0.f, 0.f);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 1
      bspyScratchpad(m_character->getBSpyID(), "brace.leftleg", target);
#endif
      m_character->getLeftLegSetup()->getAnkle()->setDesiredLean1(0.5f);

      // do right.
      target.Set(baseTarget);
      target.AddScaled(offsetDirection, -offset);
      m_character->rightLegIK(target, 0.f, 0.f);
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw & 1
      bspyScratchpad(m_character->getBSpyID(), "brace.rightleg", target);
#endif
      m_character->getRightLegSetup()->getAnkle()->setDesiredLean1(0.5f);
    }
    
  }

  void NmRsCBUShootDodge::DefendingState::Exit()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::DefendingState::Exit()");
#endif
  }

  void NmRsCBUShootDodge::IdlingState::Enter()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::IdlingState::Enter()");
#endif

    // re-enable point gun arms.
    NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)getContext()->getCBUParent()->m_tasks[bvid_pointGun];
    Assert(pointGunTask);
    pointGunTask->m_parameters.enableRight = true;
    pointGunTask->m_parameters.enableLeft = true;

    if(getContext()->m_parameters.useHeadLook)
    {
    // re-enable head look if necessary.
    NmRsCBUHeadLook *headLookTask = (NmRsCBUHeadLook *)getContext()->getCBUParent()->m_tasks[bvid_headLook];
    Assert(headLookTask);
    if(!headLookTask->isActive())
    {
#if NM_SHOOTDODGE_NO_SPINE_CONTROL
      headLookTask->setBvMask(bvmask_Neck | bvmask_Head);
      headLookTask->activate();
#else
      headLookTask->activate(m_taskParent);
#endif
    }
  }
  }

  void NmRsCBUShootDodge::IdlingState::Update(float /*timeStep*/)
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::IdlingState::Update()");
#endif

    // moved from global state to disallow transition between
    // defend and brace states, effectively locking choice of
    // reaction for the duration.
    const float braceTime = 0.4f; // parameterize!!!
    if( (getContext()->m_collisionOrientation == kHead ||
        getContext()->m_collisionOrientation == kFront) &&
        (getContext()->m_orientation == kHead || 
        getContext()->m_orientation == kFront) &&
        getContext()->m_parameters.collisionTime < braceTime)
      m_fsm->ChangeState(&getContext()->m_bracingState);
    else if(getContext()->m_inAirState.m_aboutToCrash)
      m_fsm->ChangeState(&getContext()->m_defendingState);
  }
  
  void NmRsCBUShootDodge::IdlingState::Exit()
  {
#if ART_ENABLE_BSPY & ShootDodgeBSpyDraw
    bspyLogf(info, L"NmRsCBUShootDodge::IdlingState::Exit()");
#endif
  }

#if 0
  // print itms to bspy debug console in a format we
  // can use to generate the loadITMS() code.
  // remove when we no longer need to hardcode itms.
  void NmRsCBUShootDodge::cacheITMS()
  {
    int incomingComponentCount = 0;
    rage::Matrix34 *itPtr = 0;
    ART::IncomingTransformStatus itmStatusFlags = ART::kITSNone;
    m_character->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, kITSourceCurrent);
    if (incomingComponentCount > 0 && itPtr)
    {
      rage::Displayf("// spine3");
      writeITM(itPtr, m_character->getSpineSetup()->getSpine3Part()->getPartIndex());
      rage::Displayf("// right arm");
      const RightArmSetup* rightArm = m_character->getRightArmSetup();
      writeITM(itPtr, rightArm->getClaviclePart()->getPartIndex());
      writeITM(itPtr, rightArm->getUpperArm()->getPartIndex());
      writeITM(itPtr, rightArm->getLowerArm()->getPartIndex());
      writeITM(itPtr, rightArm->getHand()->getPartIndex());
      rage::Displayf("// left arm");
      const LeftArmSetup* leftArm = m_character->getLeftArmSetup();
      writeITM(itPtr, leftArm->getClaviclePart()->getPartIndex());
      writeITM(itPtr, leftArm->getUpperArm()->getPartIndex());
      writeITM(itPtr, leftArm->getLowerArm()->getPartIndex());
      writeITM(itPtr, leftArm->getHand()->getPartIndex());
    }
  }

  void NmRsCBUShootDodge::writeITM(rage::Matrix34 *itPtr, int index)
  {
    rage::Matrix34 &itm = itPtr[index];
    rage::Displayf("{rage::Matrix34 &itm = itPtr[%d];", index);
    rage::Displayf("itm.a.x=%ff; itm.a.y=%ff; itm.a.z=%ff;", itm.a.x, itm.a.y, itm.a.z);
    rage::Displayf("itm.b.x=%ff; itm.b.y=%ff; itm.b.z=%ff;", itm.b.x, itm.b.y, itm.b.z);
    rage::Displayf("itm.c.x=%ff; itm.c.y=%ff; itm.c.z=%ff;", itm.c.x, itm.c.y, itm.c.z);
    rage::Displayf("itm.d.x=%ff; itm.d.y=%ff; itm.d.z=%ff;}", itm.d.x, itm.d.y, itm.d.z);
  }
#endif


} // ART
