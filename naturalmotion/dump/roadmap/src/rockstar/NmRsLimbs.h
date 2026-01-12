/*
 * Copyright (c) 2005-2012 NaturalMotion Ltd. All rights reserved. 
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

#ifndef NM_RS_LIMBS_H
#define NM_RS_LIMBS_H

#include <vector>

#include "NmRsLimbInputData.h"
#include "NmRsLimbInputBlendable.h"
#include "NmRsLimbManager.h"

#include "NmRsIk.h"

#if ART_ENABLE_BSPY_LIMBS
#include "NmRsSpy.h"
#include "NmRsCBU_Shared.h"
#include "bSpy\bSpyCommonPackets.h"
#endif // ART_ENABLE_BSPY_LIMBS

#undef NM_RS_PARAMETER

#undef NM_RS_RO_PARAMETER
#define NM_RS_RO_PARAMETER(classname, type, name)

#undef NM_RS_RO_PARAMETER_ACTUAL
#define NM_RS_RO_PARAMETER_ACTUAL(_prefix, _type, _name)

#undef NM_RS_PARAMETER_DIRECT
#define NM_RS_PARAMETER_DIRECT(_prefix, _type, _name)

namespace ART
{
  class NmRsCharacter;
  class NmRsEffectorBase;
  class NmRs3DofEffector;
  class NmRs1DofEffector;
  class NmRsGenericPart;

  enum NmRsHumanLimbTypes
  {
    kLeftArm,
    kRightArm,
    kLeftLeg,
    kRightLeg,
    kSpine,
    kNumNmRsHumanLimbs
  };

#if ART_ENABLE_BSPY_LIMBS
  // string map of human limbs
  static const char* humanLimbTypeStrings[] = 
  {
    "Left Arm",
    "Right Arm",
    "Left Leg",
    "Right Leg",
    "Spine",
  };
#endif

  //
  // String map of input types
#if ART_ENABLE_BSPY_LIMBS
  static const char* limbInputTypeStrings[] = 
  {
    "IK",
    "PoseArm",
    "PoseLeg",
    "PoseSpine",
    "SetStiffness"
  };
#endif

  // function typedef for calling arbitrary effector functions either with or
  // without an argument
  //
  typedef void (NmRs3DofEffectorInputWrapper::*Effector3DofDataFuncFloatArg)(const float& arg);
  typedef void (NmRs1DofEffectorInputWrapper::*Effector1DofDataFuncFloatArg)(const float& arg);


  //
  // NmRsLimb
  //---------------------------------------------------------------------------
  class NmRsLimb
  {

  public:

    NmRsLimb();

    virtual ~NmRsLimb();

    virtual void tick(float timeStep);
    virtual void init() {}

    void postInput(NmRsLimbInput input);

    NmRsHumanLimbTypes getType() const { return m_type; };
    BehaviourMask getMask() const { return m_allEffectorMask; };

    virtual void setBodyStiffness(NmRsLimbInput& input, float stiffness, float damping, BehaviourMask mask = bvmask_Full, float *muscleStiffness = 0) const = 0;
    virtual void setBodyStiffnessScaling(float stiffnessScale, float dampingScale, float muscleStiffnessScale, BehaviourMask mask = bvmask_Full) const = 0;
    virtual void setRelaxation(NmRsLimbInput& input, float mult, BehaviourMask mask = bvmask_Full, float *pMultDamping = 0) const = 0;
    virtual void activePose(NmRsLimbInput& input, int transformSource, BehaviourMask mask = bvmask_Full) const = 0;
    virtual void blendToZeroPose(NmRsLimbInput& data, float t, BehaviourMask mask = bvmask_Full) const = 0;
    virtual void resetEffectors(NmRsLimbInput& input, ResetEffectorsType type, BehaviourMask mask = bvmask_Full) const = 0;
    virtual void setOpposeGravity(NmRsLimbInput& input, float oppose, BehaviourMask mask = bvmask_Full) const = 0;
    virtual void holdPose(NmRsLimbInput& input, BehaviourMask mask = bvmask_Full) const = 0;
    virtual void callMaskedEffectorFunctionFloatArg(
      NmRsLimbInput& input,
      BehaviourMask mask,
      float floatValue,
      Effector1DofDataFuncFloatArg oneDofFn,
      Effector3DofDataFuncFloatArg threeDofFn) const = 0;

  protected:

#if ART_ENABLE_BSPY_LIMBS
    void sendMessageDebug(NmRsLimbInput* input);
    void sendOutputDebug(NmRsInputWrapperBase* data);
    int m_maxInputsUsed;
#endif

    NmRsHumanLimbTypes m_type;
    NmRsLimbInputQueue m_input;
    NmRsCharacter * m_character;

    BehaviourMask m_allEffectorMask;

    float m_stiffnessScale;
    float m_dampingScale;
    float m_muscleStiffnessScale;
    BehaviourMask m_scalingMask;
  };

  //
  // NmRsHumanArm
  //---------------------------------------------------------------------------
  class NmRsHumanArm : public NmRsLimb
  {

  public:

    NmRsHumanArm();
    ~NmRsHumanArm();

    void init();

    void setup(NmRsCharacter* character,
              NmRsHumanLimbTypes type,
              NmRsEffectorBase * clavicle,
              NmRsEffectorBase * shoulder,
              NmRsEffectorBase * elbow,
              NmRsEffectorBase * wrist,
              NmRsGenericPart * root,
              NmRsGenericPart * claviclePart,
              NmRsGenericPart * upperArm,
              NmRsGenericPart * lowerArm,
              NmRsGenericPart * hand,
              float direction,
              float hingeDirection,
              bool twistIsFixed);

    inline const NmRs3DofEffector *getClavicle()    const { return m_Clavicle; }
    inline const NmRs3DofEffector *getShoulder()    const { return m_Shoulder; }
    inline const NmRs1DofEffector *getElbow()       const { return m_Elbow; }
    inline const NmRs3DofEffector *getWrist()       const { return m_Wrist; }

    inline NmRsGenericPart *getRoot()         const { return m_root; }
    inline NmRsGenericPart *getClaviclePart() const { return m_claviclePart; }
    inline NmRsGenericPart *getUpperArm()     const { return m_upperArm; }
    inline NmRsGenericPart *getLowerArm()     const { return m_lowerArm; }
    inline NmRsGenericPart *getHand()         const { return m_hand; }

    void tick(float timeStep);

#if ART_ENABLE_BSPY_LIMBS
    bool blend(NmRsArmInputWrapper* input, float weight, BehaviourMask mask, BehaviourID task, const char* subTask);
#else
    bool blend(NmRsArmInputWrapper* input, float weight, BehaviourMask mask);
#endif

    void setBodyStiffness(NmRsLimbInput& input, float stiffness, float damping, BehaviourMask mask = bvmask_Full, float *muscleStiffness = 0) const;
    void setBodyStiffnessScaling(float stiffnessScale, float dampingScale, float muscleStiffnessScale, BehaviourMask mask = bvmask_Full) const;
    void setRelaxation(NmRsLimbInput& input, float mult, BehaviourMask mask = bvmask_Full, float *pMultDamping = 0) const;
    void activePose(NmRsLimbInput& input, int transformSource, BehaviourMask mask = bvmask_Full) const;
    void blendToZeroPose(NmRsLimbInput& data, float t, BehaviourMask mask = bvmask_Full) const;
    void resetEffectors(NmRsLimbInput& input, ResetEffectorsType type, BehaviourMask mask = bvmask_Full) const;
    void setOpposeGravity(NmRsLimbInput& input, float oppose, BehaviourMask mask = bvmask_Full) const;
    void holdPose(NmRsLimbInput& input, BehaviourMask mask = bvmask_Full) const;
    void callMaskedEffectorFunctionFloatArg(
      NmRsLimbInput& input,
      BehaviourMask mask,
      float floatValue,
      Effector1DofDataFuncFloatArg oneDofFn,
      Effector3DofDataFuncFloatArg threeDofFn) const;

    void setToCurrent(NmRsArmInputWrapper* data, BehaviourMask mask = bvmask_Full) const;

  protected:

    void doIk(NmRsLimbInput& ikMsg, NmRsArmInputWrapper& poseDataOut);

    void matchClavicleToShoulder(NmRsArmInputWrapper& poseData);
    void matchClavicleToShoulderBetter(NmRsArmInputWrapper& poseData);
    void matchClavicleToShoulderUsingTwist(NmRsArmInputWrapper& poseData);

    void wristIk(NmRsArmInputWrapper& poseData, NmRsIKInputWrapper* ikDataIn);

    void setStiffness(NmRsArmInputWrapper* poseData, float stiffness, float damping, BehaviourMask mask = bvmask_Full, float *muscleStiffness = 0) const;

    NmRs3DofEffector* m_Clavicle;
    NmRs3DofEffector* m_Shoulder;
    NmRs1DofEffector* m_Elbow;
    NmRs3DofEffector* m_Wrist;

    NmRsGenericPart*  m_root;
    NmRsGenericPart*  m_claviclePart;
    NmRsGenericPart*  m_upperArm;
    NmRsGenericPart*  m_lowerArm;
    NmRsGenericPart*  m_hand;

    NmRsArmInputBlendable m_blendTarget;

    // ik inputs that are fixed per-limb
    float m_direction;
    float m_hingeDirection;
  public:
    // todo not entirely sure what behaviours are doing with this mat...
    rage::Matrix34 m_elbowMat;

  protected:
    
  };

  //
  // NmRsHumanLeg
  //---------------------------------------------------------------------------
  class NmRsHumanLeg : public NmRsLimb
  {

  public:

    NmRsHumanLeg();
    ~NmRsHumanLeg();

    void init();

    void setup(NmRsCharacter* character,
              NmRsHumanLimbTypes type,
              NmRsEffectorBase * hip,
              NmRsEffectorBase * knee,
              NmRsEffectorBase * ankle,
              NmRsGenericPart * root,
              NmRsGenericPart * thigh,
              NmRsGenericPart * shin,
              NmRsGenericPart * foot,
              float direction,
              float hingeDirection,
              bool twistIsFixed);

    inline const NmRs3DofEffector *getHip()   const { return m_Hip; }
    inline const NmRs1DofEffector *getKnee()  const { return m_Knee; }
    inline const NmRs3DofEffector *getAnkle() const { return m_Ankle; }

    inline NmRsGenericPart *getRoot()   const { return m_root; }
    inline NmRsGenericPart *getThigh()  const { return m_thigh; }
    inline NmRsGenericPart *getShin()   const { return m_shin; }
    inline NmRsGenericPart *getFoot()   const { return m_foot; }

    void tick(float timeStep);

#if ART_ENABLE_BSPY_LIMBS
    bool blend(NmRsLegInputWrapper* input, float weight, BehaviourMask mask, BehaviourID task, const char* subTask = 0);
#else
    bool blend(NmRsLegInputWrapper* input, float weight, BehaviourMask mask);
#endif

    void setBodyStiffness(NmRsLimbInput& input, float stiffness, float damping, BehaviourMask mask = bvmask_Full, float *muscleStiffness = 0) const;
    void setBodyStiffnessScaling(float stiffnessScale, float dampingScale, float muscleStiffnessScale, BehaviourMask mask = bvmask_Full) const;
    void setRelaxation(NmRsLimbInput& input, float mult, BehaviourMask mask = bvmask_Full, float *pMultDamping = 0) const;
    void activePose(NmRsLimbInput& input, int transformSource, BehaviourMask mask = bvmask_Full) const;
    void blendToZeroPose(NmRsLimbInput& data, float t, BehaviourMask mask = bvmask_Full) const;
    void resetEffectors(NmRsLimbInput& input, ResetEffectorsType type, BehaviourMask mask = bvmask_Full) const;
    void setOpposeGravity(NmRsLimbInput& input, float oppose, BehaviourMask mask = bvmask_Full) const;
    void holdPose(NmRsLimbInput& input, BehaviourMask mask = bvmask_Full) const;
    void callMaskedEffectorFunctionFloatArg(
      NmRsLimbInput& input,
      BehaviourMask mask,
      float floatValue,
      Effector1DofDataFuncFloatArg oneDofFn,
      Effector3DofDataFuncFloatArg threeDofFn) const;

    void setToCurrent(NmRsLegInputWrapper* data, BehaviourMask mask = bvmask_Full);

  protected:

    void doIk(NmRsLimbInput& ikMsg, NmRsLegInputWrapper& poseDataOut);

    void setStiffness(NmRsLegInputWrapper* poseData, float stiffness, float damping, BehaviourMask mask = bvmask_Full, float *muscleStiffness = 0) const;

    NmRs3DofEffector* m_Hip;
    NmRs1DofEffector* m_Knee;
    NmRs3DofEffector* m_Ankle;

    NmRsGenericPart* m_root;
    NmRsGenericPart* m_thigh;
    NmRsGenericPart* m_shin;
    NmRsGenericPart* m_foot;

    NmRsLegInputBlendable m_blendTarget;

    // ik inputs that are fixed per-limb
    float m_direction;
    float m_hingeDirection;
  public:
    // todo not entirely sure what behaviours are doing with this mat...
    rage::Matrix34 m_elbowMat;
  };

  //
  // NmRsHumanSpine
  //---------------------------------------------------------------------------
  class NmRsHumanSpine : public NmRsLimb
  {

  public:

    NmRsHumanSpine();
    ~NmRsHumanSpine();

    void setup(NmRsCharacter* character,
              NmRsHumanLimbTypes type,
              NmRsEffectorBase * spine0,
              NmRsEffectorBase * spine1,
              NmRsEffectorBase * spine2,
              NmRsEffectorBase * spine3,
              NmRsEffectorBase * lowerNeck,
              NmRsEffectorBase * upperNeck,
              NmRsGenericPart * pelvis,
              NmRsGenericPart * spine0Part,
              NmRsGenericPart * spine1Part,
              NmRsGenericPart * spine2Part,
              NmRsGenericPart * spine3Part,
              NmRsGenericPart * neck,
              NmRsGenericPart * head );

    inline const NmRs3DofEffector *getSpine0()    const { return m_Spine0; }
    inline const NmRs3DofEffector *getSpine1()    const { return m_Spine1; }
    inline const NmRs3DofEffector *getSpine2()    const { return m_Spine2; }
    inline const NmRs3DofEffector *getSpine3()    const { return m_Spine3; }
    inline const NmRs3DofEffector *getLowerNeck() const { return m_LowerNeck; }
    inline const NmRs3DofEffector *getUpperNeck() const { return m_UpperNeck; }

    inline NmRsGenericPart *getPelvisPart() const { return m_pelvis; }
    inline NmRsGenericPart *getSpine0Part() const { return m_Spine0Part; }
    inline NmRsGenericPart *getSpine1Part() const { return m_Spine1Part; }
    inline NmRsGenericPart *getSpine2Part() const { return m_Spine2Part; }
    inline NmRsGenericPart *getSpine3Part() const { return m_Spine3Part; }
    inline NmRsGenericPart *getNeckPart()   const { return m_neck; }
    inline NmRsGenericPart *getHeadPart()   const { return m_head; }

    void tick(float timeStep);

#if ART_ENABLE_BSPY_LIMBS
    bool blend(NmRsSpineInputWrapper* input, float weight, BehaviourMask mask, BehaviourID task, const char* subTask = 0);
#else
    bool blend(NmRsSpineInputWrapper* input, float weight, BehaviourMask mask);
#endif

    void setBodyStiffness(NmRsLimbInput& input, float stiffness, float damping, BehaviourMask mask = bvmask_Full, float *muscleStiffness = 0) const;
    void setBodyStiffnessScaling(float stiffnessScale, float dampingScale, float muscleStiffnessScale, BehaviourMask mask = bvmask_Full) const;
    void setRelaxation(NmRsLimbInput& input, float mult, BehaviourMask mask = bvmask_Full, float *pMultDamping = 0) const;
    void activePose(NmRsLimbInput& input, int transformSource, BehaviourMask mask = bvmask_Full) const;
    void blendToZeroPose(NmRsLimbInput& input, float t, BehaviourMask mask = bvmask_Full) const;
    void resetEffectors(NmRsLimbInput& input, ResetEffectorsType type, BehaviourMask mask = bvmask_Full) const;
    void setOpposeGravity(NmRsLimbInput& input, float oppose, BehaviourMask mask = bvmask_Full) const;
    void holdPose(NmRsLimbInput& input, BehaviourMask mask = bvmask_Full) const;
    void callMaskedEffectorFunctionFloatArg(
      NmRsLimbInput& input,
      BehaviourMask mask,
      float floatValue,
      Effector1DofDataFuncFloatArg oneDofFn,
      Effector3DofDataFuncFloatArg threeDofFn) const;

    void keepHeadAwayFromGround(NmRsLimbInput& input, float leanAmount, rage::Vector3 *direction = NULL);

    void setToCurrent(NmRsSpineInputWrapper* data, BehaviourMask mask = bvmask_Full);

  protected:

    void setStiffness(NmRsSpineInputWrapper* poseData, float stiffness, float damping, BehaviourMask mask = bvmask_Full, float *muscleStiffness = 0) const;

    NmRs3DofEffector* m_Spine0;
    NmRs3DofEffector* m_Spine1;
    NmRs3DofEffector* m_Spine2;
    NmRs3DofEffector* m_Spine3;
    NmRs3DofEffector* m_LowerNeck;
    NmRs3DofEffector* m_UpperNeck;

    NmRsGenericPart* m_pelvis;
    NmRsGenericPart* m_Spine0Part;
    NmRsGenericPart* m_Spine1Part;
    NmRsGenericPart* m_Spine2Part;
    NmRsGenericPart* m_Spine3Part;
    NmRsGenericPart* m_neck;
    NmRsGenericPart* m_head;

    NmRsSpineInputBlendable m_blendTarget;
  };

//
// NmRsBody
//
//-----------------------------------------------------------------------------
class NmRsBody
{
  // change this when new body types are added.
  static const int MAX_LIMBS = kNumNmRsHumanLimbs;

public:

  NmRsBody(MemoryManager* services);

  virtual ~NmRsBody();

  void init(NmRsCharacter* character);

  void initAllLimbs();

#if ART_ENABLE_BSPY_LIMBS
  virtual void setup(BehaviourID bvid, int priority, int subPriority, float blend, BehaviourMask mask = bvmask_Full, const char* subTask = 0);
#else
  virtual void setup(BehaviourID bvid, int priority, int subPriority, float blend, BehaviourMask mask = bvmask_Full);
#endif

  void tick(float timeStep);

  class LimbIterator
  {

  public:
    // todo protect some stuff.
    inline LimbIterator(NmRsBody& body) :
      m_body(body),
      m_index(0)
    {
    }

    inline void next()
    {
      ++m_index;
    }

    inline void nextWithMask(BehaviourMask mask)
    {
      do
      {
        ++m_index;
      } while ((m_index < MAX_LIMBS) && !(mask & currentLimb()->getMask()));
    }

    inline bool finished()
    {
      return (m_index >= MAX_LIMBS);
    }

    inline NmRsLimb* currentLimb() const
    {
      Assert(m_body.getLimb((NmRsHumanLimbTypes)m_index));
      return m_body.getLimb((NmRsHumanLimbTypes)m_index);
    }

    inline NmRsLimbInput& currentLimbInput(bool autoCreate = true) const
    {
      if (autoCreate)
      {
        return m_body.getInput((NmRsHumanLimbTypes)m_index);
      }
      else
      {
        return m_body.getInputNoSetup((NmRsHumanLimbTypes)m_index);
      }
    }

  private:
    NmRsBody&  m_body;
    int        m_index;
  };

  void postLimbInputs();

  NmRsLimb*       getLimb(NmRsHumanLimbTypes limbType);
  const NmRsLimb* getLimb(NmRsHumanLimbTypes limbType) const;

  virtual NmRsLimbInput& getInput(NmRsHumanLimbTypes limbType) = 0;
  NmRsLimbInput& getInputNoSetup(NmRsHumanLimbTypes limbType);

  template <typename T>
#if ART_ENABLE_BSPY_LIMBS
  inline NmRsLimbInput createNmRsLimbInput(int subPriority = 0, float weight = 1.0f, BehaviourMask mask = bvmask_Full, const char* subTask = 0)
#else
  inline NmRsLimbInput createNmRsLimbInput(int subPriority = 0, float weight = 1.0f, BehaviourMask mask = bvmask_Full)
#endif
  {
    Assert(m_character);
    Assert(m_character->getLimbManager());

    // Make sure the specified blend weight is equal to or less than the body-
    // wide weight.
    if(weight > m_blend)
      weight = m_blend;

#if ART_ENABLE_BSPY_LIMBS
    const char* _subTask = 0; 
    if(subTask)
      _subTask = subTask;
    else if(m_subTask)
      _subTask = m_subTask;

    return m_character->getLimbManager()->createNmRsLimbInput<T>(
      m_priority,
      subPriority,
      m_mask & mask,
      weight,
      m_bvid,
      _subTask);
#else
    return m_character->getLimbManager()->createNmRsLimbInput<T>(
      m_priority,
      subPriority,
      m_mask & mask,
      weight);
#endif
  }

  //
  // generic whole body functions
  //---------------------------------------------------------------------------

  void setStiffness(float stiffness, float damping, BehaviourMask mask = bvmask_Full, float *muscleStiffness = NULL, bool queued = false);
  void setStiffnessScaling(float stiffnessScale, float dampingScale, float muscleStiffnessScale, BehaviourMask mask = bvmask_Full);
  void setRelaxation(float mult, BehaviourMask mask = bvmask_Full, float *pMultDamping = 0);
  void activePose(int transformSource, BehaviourMask mask = bvmask_Full);
  void holdPose(BehaviourMask mask = bvmask_Full);
  void blendToZeroPose(float blend, BehaviourMask mask = bvmask_Full);
  void setOpposeGravity(float oppose, BehaviourMask mask = bvmask_Full);
  void resetEffectors(ResetEffectorsType resetType, BehaviourMask mask = bvmask_Full);
  void callMaskedEffectorDataFunctionFloatArg(
    BehaviourMask mask,
    float floatValue,
    Effector1DofDataFuncFloatArg oneDofFn,
    Effector3DofDataFuncFloatArg threeDofFn);

protected:
  NmRsLimb*      m_allLimbs[MAX_LIMBS];
  NmRsLimbInput  m_allLimbInputs[MAX_LIMBS];

  NmRsCharacter* m_character;
  MemoryManager* m_artMemoryManager;
  int            m_priority;
  int            m_subPriority;
  BehaviourID    m_bvid;
  BehaviourMask  m_mask;
  float          m_blend;

  // TODO wrap for bspy
  const char*    m_subTask;
};


//
// NmRsHumanBody
//
//-----------------------------------------------------------------------------

class NmRsHumanBody : public NmRsBody
{
public:

  NmRsHumanBody(MemoryManager* services);

  virtual ~NmRsHumanBody() {}

#if ART_ENABLE_BSPY_LIMBS
  void setup(BehaviourID bvid, int priority, int subPriority, float blend, BehaviourMask mask, const char* subTask = 0);
#else
  void setup(BehaviourID bvid, int priority, int subPriority, float blend, BehaviourMask mask);
#endif

  void addHumanArm(
    NmRsHumanLimbTypes type,
    NmRsEffectorBase * clavicle,
    NmRsEffectorBase * shoulder,
    NmRsEffectorBase * elbow,
    NmRsEffectorBase * wrist,
    NmRsGenericPart * root,
    NmRsGenericPart * claviclePart,
    NmRsGenericPart * upperArm,
    NmRsGenericPart * lowerArm,
    NmRsGenericPart * hand,
    float direction,
    float hingeDirection,
    bool twistIsFixed);

  void addHumanLeg(
    NmRsHumanLimbTypes type,
    NmRsEffectorBase * hip,
    NmRsEffectorBase * knee,
    NmRsEffectorBase * ankle,
    NmRsGenericPart * root,
    NmRsGenericPart * thigh,
    NmRsGenericPart * shin,
    NmRsGenericPart * foot,
    float direction,
    float hingeDirection,
    bool twistIsFixed);

  void addHumanSpine(
    NmRsHumanLimbTypes type,
    NmRsEffectorBase * spine0,
    NmRsEffectorBase * spine1,
    NmRsEffectorBase * spine2,
    NmRsEffectorBase * spine3,
    NmRsEffectorBase * lowerNeck,
    NmRsEffectorBase * upperNeck,
    NmRsGenericPart * pelvis,
    NmRsGenericPart * spine0Part,
    NmRsGenericPart * spine1Part,
    NmRsGenericPart * spine2Part,
    NmRsGenericPart * spine3Part,
    NmRsGenericPart * neck,
    NmRsGenericPart * head );

  //
  // human limb accessors, for convenience
  //---------------------------------------------------------------------------

  NmRsLimbInput& getInput(NmRsHumanLimbTypes limbType);

  inline NmRsHumanArm*   getLeftArm()  { return static_cast<NmRsHumanArm*>(getLimb(kLeftArm)); }
  inline NmRsHumanArm*   getRightArm() { return static_cast<NmRsHumanArm*>(getLimb(kRightArm)); }
  inline NmRsHumanLeg*   getLeftLeg()  { return static_cast<NmRsHumanLeg*>(getLimb(kLeftLeg)); }
  inline NmRsHumanLeg*   getRightLeg() { return static_cast<NmRsHumanLeg*>(getLimb(kRightLeg)); }
  inline NmRsHumanSpine* getSpine()    { return static_cast<NmRsHumanSpine*>(getLimb(kSpine)); }

  inline const NmRsHumanArm*   getLeftArm()  const { return static_cast<const NmRsHumanArm*>(getLimb(kLeftArm)); }
  inline const NmRsHumanArm*   getRightArm() const { return static_cast<const NmRsHumanArm*>(getLimb(kRightArm)); }
  inline const NmRsHumanLeg*   getLeftLeg()  const { return static_cast<const NmRsHumanLeg*>(getLimb(kLeftLeg)); }
  inline const NmRsHumanLeg*   getRightLeg() const { return static_cast<const NmRsHumanLeg*>(getLimb(kRightLeg)); }
  inline const NmRsHumanSpine* getSpine()    const { return static_cast<const NmRsHumanSpine*>(getLimb(kSpine)); }

  inline NmRsLimbInput& getLeftArmInput()  { return getInput(kLeftArm); }
  inline NmRsLimbInput& getRightArmInput() { return getInput(kRightArm); }
  inline NmRsLimbInput& getLeftLegInput()  { return getInput(kLeftLeg); }
  inline NmRsLimbInput& getRightLegInput() { return getInput(kRightLeg); }
  inline NmRsLimbInput& getSpineInput()    { return getInput(kSpine); }

  NmRsArmInputWrapper* getLeftArmInputData();
  NmRsArmInputWrapper* getRightArmInputData();
  NmRsLegInputWrapper* getLeftLegInputData();
  NmRsLegInputWrapper* getRightLegInputData();
  NmRsSpineInputWrapper* getSpineInputData();

};


} // namespace ART
#endif // NM_RS_LIMBS_H
