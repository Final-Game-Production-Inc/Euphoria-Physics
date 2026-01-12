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

#ifndef NM_RS_LEARNEDCRAWL 
#define NM_RS_LEARNEDCRAWL

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

#define NUM_PARTS 21 
#define NUM_JOINTS 20

namespace ART
{
  class NmRsCharacter;
  extern int leftHandStance[5];
  extern int leftHandSwing[5];
  extern int leftLegStance[5];
  extern int leftLegSwing[5];    
  extern int rightHandStance[5];
  extern int rightHandSwing[5];
  extern int rightLegStance[5];
  extern int rightLegSwing[5];    

#define NMLearnedCrawlFeedbackName      "learnedCrawl" 

  class NmRsCBULearnedCrawl : public CBUTaskBase
  {
  public:
    NmRsCBULearnedCrawl(ART::MemoryManager* services);
    ~NmRsCBULearnedCrawl();

    void init();
    virtual void init(class NmRsCharacter* character, CBURecord* cbuParent){ CBUTaskBase::init(character, cbuParent); init(); }
    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      float stiffness;
      float damping;
      bool bLearn;
      int numFrames2Learn;
      int animIndex;
      float yawOffset;
      rage::Vector3 targetPosition;
      const void *inputSequence;
      int inputSequenceSize;
      float speed;
      bool learnFromAnimPlayback;
      bool useSpine3Thing;
      bool useRollBoneCompensation;
      bool useTwister;
    } m_parameters;

    void updateBehaviourMessage(const MessageParamsBase* const params);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

  protected:
    // TDL this is all the learned part
    enum Limbs
    {
      lLeftArm,
      lRightArm,
      lLeftLeg,
      lRightLeg,
    };
    int m_numberOfFrames;
    float m_frameValue;
    float m_yawOffset;

    // vector to record the progress of the crawl
    rage::Vector3 comPos;
    float m_distTravelled;

    int m_animIndex; // an index slot for the animation, gives us a look up for ground contact events
    bool bSupporting[4], bNextSupporting[4];
    struct Sequence
    {
      struct EffectorTarget
      {
        float lean1, lean2, twist;
      } m_effector[NUM_JOINTS];
      struct IKTarget
      {
        rage::Vector3 target;
        float twist;
      } m_ik[4];
    } *m_sequence;
    void initialiseCustomVariables();
    void loadDriveSequence();
    void enablePartFriction(NmRsGenericPart *part, bool enable);
    void changeFriction(bool *supporting);
    bool setSupportingLimbs(bool *supporting, int frame);
    void setupCharacter();
    void addSpineLean(float rotateVel);
    void setPartFrictionMultiplier(NmRsGenericPart *part, float friction);
    void resetFriction();
    rage::Vector3 getIKTarget(NmRsGenericPart* claviclePart, Limbs limb);

    /*********************** TDL this is all the learning part ***********************************/
    rage::Matrix34 *m_ctmTransforms;
    rage::Vector3 *m_ctmVels;
    Sequence *m_bestSequence;
    int m_frameIndex, m_totalFrame, m_frameNumber;
    FILE *m_out; // for saving .ctm file

    struct Bucket
    {
      struct EffectorBucket // hinge angles go in the lean1
      {
        float desiredLean1, desiredLean2, desiredTwist;
        float desiredLean1Vel, desiredLean2Vel, desiredTwistVel;
        float lean1Error, lean2Error, twistError;
        float lean1VelError, lean2VelError, twistVelError;
      } m_effector[NUM_JOINTS];
      struct IKBucket
      {
        rage::Vector3 desiredTarget, desiredTargetVel;
        float error, twistError;
        float desiredTwist, desiredTwistVel;
      } m_ik[4];
    } *m_bucket;
    float m_totalError;
    float m_bestTotalError;
    float m_count;

    void loadCTM();
    void buildIKBucket(NmRsHumanArm *arm, int i, Limbs limb);
    void buildBuckets();
#if ART_ENABLE_BSPY
    void drawAnimation();
#endif//#if ART_ENABLE_BSPY
    void saveDriveSequence();
    void resetCharacterToStart();
    void learnIKDrive(NmRsHumanArm *arm, Limbs limb, float scale, Bucket *bucket, Sequence *sequence, Sequence *nextSequence);
    void learnDriveSequence();
  };
}

#endif // NM_RS_LEARNEDCRAWL



