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

#ifndef NM_RS_BODYLAYOUT_H
#define NM_RS_BODYLAYOUT_H

#if HACK_GTA4 // Integrating NM code drop (22/7/09)
// Enable/disable the new Natural Motion rig. Remember to also enable the new skeleton in AnimBones.h
#define USE_NEW_NM_RIG 1
#endif //HACK_GTA4


namespace ART
{
  class NmRsGenericPart;
  class NmRs1DofEffector;
  class NmRs3DofEffector;
  class NmRsEffectorBase;

  /*
  * place bipeds first, then non bipeds. that way the character function isBiped can easily check based on range
  */
  enum BodyIdentifier
  {
    notSpecified,
    gtaFred,
    gtaWilma,
    gtaFredLarge,
	gtaWilmaLarge,
	gtaAlien,
    rdrCowboy,
    rdrCowgirl,
    mp3Medium,
    mp3Large,
    mp3Maxine,
    rdrHorse,
    mp3Dog,
    endOfBodyId
  };

  /*
  *  Human parts/effectors
  */

  enum KnownHumanParts
  {
    Buttocks,
    Thigh_Left,
    Shin_Left,
    Foot_Left,
    Thigh_Right,
    Shin_Right,
    Foot_Right,
    Spine0,
    Spine1,
    Spine2,
    Spine3,
    Neck,
    Head,
    Clav_Left,
    Upper_Arm_Left,
    Lower_Arm_Left,
    Hand_Left,
    Clav_Right,
    Upper_Arm_Right,
    Lower_Arm_Right,
    Hand_Right,

    TotalKnownParts
  };

  enum KnownHumanEffectors
  {
    Hip_Left,
    Knee_Left,
    Ankle_Left,
    Hip_Right,
    Knee_Right,
    Ankle_Right,
    Spine_0,
    Spine_1,
    Spine_2,
    Spine_3,
    Neck_Lower,
    Neck_Upper,
    Clav_Jnt_Left,
    Shoulder_Left,
    Elbow_Left,
    Wrist_Left,
    Clav_Jnt_Right,
    Shoulder_Right,
    Elbow_Right,
    Wrist_Right,

    TotalKnownHumanEffectors
  };

#if HACK_GTA4 // Integrating NM code drop (22/7/09)
#if !USE_NEW_NM_RIG
  enum GTAHumanPartIndex
  {
    gtaButtocks,
      gtaThigh_Left,
        gtaShin_Left,
          gtaFoot_Left,
      gtaThigh_Right,
        gtaShin_Right,
          gtaFoot_Right,
      gtaSpine0,
        gtaSpine1,
          gtaSpine2,
            gtaSpine3,
              gtaNeck,
                gtaHead,
              gtaClav_Left,
                gtaUpper_Arm_Left,
                  gtaLower_Arm_Left,
                    gtaHand_Left,
              gtaClav_Right,
                gtaUpper_Arm_Right,
                  gtaLower_Arm_Right,
                    gtaHand_Right,

    gta_TotalHumanParts
  };

  enum GTAEffectorIndex
  {
    gtaJtHip_Left,
      gtaJtKnee_Left,
        gtaJtAnkle_Left,
    gtaJtHip_Right,
      gtaJtKnee_Right,
        gtaJtAnkle_Right,
    gtaJtSpine_0,
      gtaJtSpine_1,
        gtaJtSpine_2,
          gtaJtSpine_3,
            gtaJtNeck_Lower,
              gtaJtNeck_Upper,
            gtaJtClav_Jnt_Left,
              gtaJtShoulder_Left,
                gtaJtElbow_Left,
                  gtaJtWrist_Left,
            gtaJtClav_Jnt_Right,
              gtaJtShoulder_Right,
                gtaJtElbow_Right,
                  gtaJtWrist_Right,

     gta_TotalHumanEffectors
  };
#else //!USE_NEW_NM_RIG
  enum GTAHumanPartIndex
  {
    gtaButtocks,
      gtaThigh_Left,
        gtaShin_Left,
          gtaFoot_Left,
      gtaThigh_Right,
        gtaShin_Right,
          gtaFoot_Right,
      gtaSpine0,
        gtaSpine1,
          gtaSpine2,
            gtaSpine3,
              gtaClav_Left,
                gtaUpper_Arm_Left,
                  gtaLower_Arm_Left,
                    gtaHand_Left,
              gtaClav_Right,
                gtaUpper_Arm_Right,
                  gtaLower_Arm_Right,
                    gtaHand_Right,
              gtaNeck,
                gtaHead,

    gta_TotalHumanParts
    };

  enum GTAEffectorIndex
  {
    gtaJtHip_Left,
      gtaJtKnee_Left,
        gtaJtAnkle_Left,
    gtaJtHip_Right,
      gtaJtKnee_Right,
         gtaJtAnkle_Right,
    gtaJtSpine_0,
      gtaJtSpine_1,
        gtaJtSpine_2,
          gtaJtSpine_3,
            gtaJtClav_Jnt_Left,
              gtaJtShoulder_Left,
                gtaJtElbow_Left,
                  gtaJtWrist_Left,
            gtaJtClav_Jnt_Right,
              gtaJtShoulder_Right,
                gtaJtElbow_Right,
                  gtaJtWrist_Right,
            gtaJtNeck_Lower,
              gtaJtNeck_Upper,

    gta_TotalHumanEffectors
  };
#endif //!USE_NEW_NM_RIG
#else //HACK_GTA4
  enum GTAHumanPartIndex
  {
    gtaButtocks,
      gtaThigh_Left,
        gtaShin_Left,
          gtaFoot_Left,
      gtaThigh_Right,
        gtaShin_Right,
          gtaFoot_Right,
      gtaSpine0,
        gtaSpine1,
          gtaSpine2,
            gtaSpine3,
              gtaNeck,
                gtaHead,
              gtaClav_Left,
                gtaUpper_Arm_Left,
                  gtaLower_Arm_Left,
                    gtaHand_Left,
              gtaClav_Right,
                gtaUpper_Arm_Right,
                  gtaLower_Arm_Right,
                    gtaHand_Right,

    gta_TotalHumanParts
  };

  enum GTAEffectorIndex
  {
    gtaJtHip_Left,
      gtaJtKnee_Left,
        gtaJtAnkle_Left,
    gtaJtHip_Right,
      gtaJtKnee_Right,
        gtaJtAnkle_Right,
    gtaJtSpine_0,
      gtaJtSpine_1,
        gtaJtSpine_2,
          gtaJtSpine_3,
            gtaJtNeck_Lower,
              gtaJtNeck_Upper,
            gtaJtClav_Jnt_Left,
              gtaJtShoulder_Left,
                gtaJtElbow_Left,
                  gtaJtWrist_Left,
            gtaJtClav_Jnt_Right,
              gtaJtShoulder_Right,
                gtaJtElbow_Right,
                  gtaJtWrist_Right,

     gta_TotalHumanEffectors
  };
#endif //HACK_GTA4

  enum RDRHumanPartIndex
  {
    rdrSpine0,
      rdrSpine1,
        rdrSpine2,
          rdrSpine3,
            rdrNeck,
              rdrHead,
            rdrClav_Left,
              rdrUpper_Arm_Left,
                rdrLower_Arm_Left,
                  rdrHand_Left,
            rdrClav_Right,
              rdrUpper_Arm_Right,
                rdrLower_Arm_Right,
                  rdrHand_Right,
      rdrButtocks,
        rdrThigh_Left,
          rdrShin_Left,
            rdrFoot_Left,
        rdrThigh_Right,
          rdrShin_Right,
            rdrFoot_Right,

    rdr_TotalHumanParts
  };

  enum RDRHumanEffectorIndex
  {
    rdrJtSpine_1,
      rdrJtSpine_2,
        rdrJtSpine_3,
          rdrJtNeck_Lower,
            rdrJtNeck_Upper,
          rdrJtClav_Jnt_Left,
            rdrJtShoulder_Left,
              rdrJtElbow_Left,
                rdrJtWrist_Left,
          rdrJtClav_Jnt_Right,
            rdrJtShoulder_Right,
              rdrJtElbow_Right,
                rdrJtWrist_Right,
    rdrJtSpine_0,
      rdrJtHip_Left,
        rdrJtKnee_Left,
          rdrJtAnkle_Left,
      rdrJtHip_Right,
        rdrJtKnee_Right,
          rdrJtAnkle_Right,

     rdr_TotalHumanEffectors
  };

  /*
   *  Dog parts/effectors
   */

  enum KnownDogParts
  {
    spine0_nm,
    spine1_nm,
    spine2_nm,
    spine3_nm,
    neck_nm,
    neck1_nm,
    head_nm,
    clavicle_l_nm,
    upperarm_l_nm,
    forearm_l_nm,
    hand_l_nm,
    finger_l_nm,
    clavicle_r_nm,
    upperarm_r_nm,
    forearm_r_nm,
    hand_r_nm,
    finger_r_nm,
    thigh_l_nm,
    calf_l_nm,
    foot_l_nm,
    toe_l_nm,
    thigh_r_nm,
    calf_r_nm,
    foot_r_nm,
    toe_r_nm,

    TotalKnownDogParts
  };

  enum KnownDogEffectors
  {
    spine0_to_spine1,
    spine1_to_spine2,
    spine2_to_spine3,
    spine3_to_neck,
    neck_to_neck1,
    neck1_to_head,
    spine3_to_clavicle_l,
    clavicle_l_to_upperarm_l,
    upperarm_l_to_forearm_l,
    forearm_l_to_hand_l,
    hand_l_to_finger_l,
    spine3_to_clavicle_r,
    clavicle_r_to_upperarm_r,
    upperarm_r_to_forearm_r,
    forearm_r_to_hand_r,
    hand_r_to_finger_r,
    spine0_to_thigh_l,
    thigh_l_to_calf_l,
    calf_l_to_foot_l,
    foot_l_to_toe_l,
    spine0_to_thigh_r,
    thigh_r_to_calf_r,
    calf_r_to_foot_r,
    foot_r_to_toe_r,

    TotalKnownDogEffectors
  };

  enum MP3DogPartIndex
  {
    mp3DogSpine0,
      mp3DogSpine1,
        mp3DogSpine2,
          mp3DogSpine3,
            mp3DogNeck,
              mp3DogNeck1,
                mp3DogHead,
            mp3DogClavicleLeft,
              mp3DogUpperArmLeft,
                mp3DogForeArmLeft,
                  mp3DogHandLeft,
                    mp3DogFingerLeft,
            mp3DogClavicleRight,
              mp3DogUpperArmRight,
                mp3DogForeArmRight,
                  mp3DogHandRight,
                    mp3DogFingerRight,
      mp3DogThighLeft,
        mp3DogCalfLeft,
          mp3DogFootLeft,
            mp3DogToeLeft,
      mp3DogThighRight,
        mp3DogCalfRight,
          mp3DogFootRight,
            mp3DogToeRight,

    mp3_TotalDogParts
  };

  enum MP3DogEffectorIndex
  {
    mp3DogJtSpine1,
      mp3DogJtSpine2,
        mp3DogJtSpine3,
          mp3DogJtNeck,
            mp3DogJtNeck1,
              mp3DogJtHead,
          mp3DogJtClavicleLeft,
            mp3DogJtUpperArmLeft,
              mp3DogJtForeArmLeft,
                mp3DogJtHandLeft,
                  mp3DogJtFingerLeft,
          mp3DogJtClavicleRight,
            mp3DogJtUpperArmRight,
              mp3DogJtForeArmRight,
                mp3DogJtHandRight,
                  mp3DogJtFingerRight,
    mp3DogJtThighLeft,
      mp3DogJtCalfLeft,
        mp3DogJtFootLeft,
          mp3DogJtToeLeft,
    mp3DogJtThighRight,
      mp3DogJtCalfRight,
        mp3DogJtFootRight,
          mp3DogJtToeRight,

    mp3_TotalDogEffectors
  };


  /*
   *  Horse parts/effectors
   */

#if(0)
  enum KnownHorseParts
  {
    pelvis_nm,
    pelvis01_nm,
    hip_l_nm,
    knee_l_nm,
    ankle_l_nm,
    ball_l_nm,
    toe_l_nm,
    hip_r_nm,
    knee_r_nm,
    ankle_r_nm,
    ball_r_nm,
    toe_r_nm,
    spine00_nm,
    spine01_nm,
    spine02_nm,
    neck_nm,
    neck01_nm,
    neck02_nm,
    head_nm,
    clavicle_r_nm,
    arm_r_nm,
    elbow_r_nm,
    wrist_r_nm,
    finger_r_nm,
    nail_r_nm,
    clavicle_l_nm,
    arm_l_nm,
    elbow_l_nm,
    wrist_l_nm,
    finger_l_nm,
    nail_l_nm,

    TotalKnownHorseParts
  };

  enum KnownHorseEffectors
  {
    pelvis_to_pelvis01,
    pelvis01_to_hip_l,
    hip_l_to_knee_l,
    knee_l_to_ankle_l,
    ankle_l_to_ball_l,
    ball_l_to_toe_l,
    pelvis01_to_hip_r,
    hip_r_to_knee_r,
    knee_r_to_ankle_r,
    ankle_r_to_ball_r,
    ball_r_to_toe_r,
    pelvis_to_spine00,
    spine00_to_spine01,
    spine01_to_spine02,
    spine01_to_clavicle_r,
    clavicle_r_to_arm_r,
    arm_r_to_elbow_r,
    elbow_r_to_wrist_r,
    wrist_r_to_finger_r,
    finger_r_to_nail_r,
    spine01_to_clavicle_l,
    clavicle_l_to_arm_l,
    arm_l_to_elbow_l,
    elbow_l_to_wrist_l,
    wrist_l_to_finger_l,
    finger_l_to_nail_l,

    TotalKnownHorseEffectors
  };
#endif

  enum RDRHorsePartIndex
  {
    rdrHorseSpine00,
      rdrHorseSpine02,
        rdrHorseNeck,
          rdrHorseNeck02,
            rdrHorseHead,
      rdrHorseClavicleLeft,
        rdrHorseArmLeft,
          rdrHorseElbowLeft,
            rdrHorseWristLeft,
              rdrHorseFingerLeft,
                rdrHorseNailLeft,
      rdrHorseClavicleRight,
        rdrHorseArmRight,
          rdrHorseElbowRight,
            rdrHorseWristRight,
              rdrHorseFingerRight,
                rdrHorseNailRight,
      rdrHorsePelvis,
        rdrHorsePelvis01,
          rdrHorseHipLeft,
            rdrHorseKneeLeft,
              rdrHorseAnkleLeft,
                rdrHorseBallLeft,
                  rdrHorseToeLeft,
          rdrHorseHipRight,
            rdrHorseKneeRight,
              rdrHorseAnkleRight,
                rdrHorseBallRight,
                  rdrHorseToeRight,
    rdr_TotalHorseParts 
  };

  enum RDRHorseEffectorIndex
  {
    rdrHorseJtSpine02,
      rdrHorseJtNeck,
        rdrHorseJtNeck02,
          rdrHorseJtHead,
      rdrHorseJtClavicleLeft,
        rdrHorseJtArmLeft,
          rdrHorseJtElbowLeft,
            rdrHorseJtWristLeft,
              rdrHorseJtFingerLeft,
                rdrHorseJtNailLeft,
      rdrHorseJtClavicleRight,
        rdrHorseJtArmRight,  
          rdrHorseJtElbowRight,
            rdrHorseJtWristRight,
              rdrHorseJtFingerRight,
                rdrHorseJtNailRight,
      rdrHorseJtPelvis,
        rdrHorseJtPelvis01,
          rdrHorseJtHipLeft,
            rdrHorseJtKneeLeft,
              rdrHorseJtAnkleLeft,
                rdrHorseJtBallLeft,
                  rdrHorseJtToeLeft,
          rdrHorseJtHipRight,
            rdrHorseJtKneeRight,
              rdrHorseJtAnkleRight,
                rdrHorseJtBallRight,
                  rdrHorseJtToeRight,
    rdr_TotalHorseEffectors
  };

  struct GameIDMap
  {
    int   m_gta,
      m_rdr;
  };


  /**
  *
  */
  struct CharacterConfiguration
  {
#define HS_STATES(_action) \
  _action(eHS_Free) \
  /*movable with some restrictions*/ \
    /* - can reach for grab but can't grab without dropping - mmmmtodo just sends feedback to drop at moment*/ \
    /* - can brace and flinch*/ \
    /* - can balance - but slightly muted*/ \
  _action(eHS_Pistol)\
  /*barely movable*/ \
  /* - can't reach for grab - mmmmtodo send feedback to drop*/ \
  /* - can't brace or flinch*/ \
  /* - can balance - but muted*/ \
  _action(eHS_Rifle) \
  _action(eHS_NumOfHandSates)

    enum HandState
    {
#define HS_ENUM_ACTION(_name) _name,
      HS_STATES(HS_ENUM_ACTION)
#undef HS_ENUM_ACTION
    };

    float         m_legStraightness;
    float         m_legSeparation;
    float         m_charlieChapliness;
    float         m_hipYaw;
    float         m_headYaw;
    float         m_defaultHipPitch;

    HandState     m_leftHandState;    ///< set to appropriate flag to indicate how the left hand can be used
    HandState     m_rightHandState;   ///< .. ditto right hand
  };

}

#endif // NM_RS_BODYLAYOUT_H
