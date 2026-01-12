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

/**
 * -------------------------- IMPORTANT --------------------------
 * This file contains general computational utilities and body helper functions.
 * It must be compilable on SPU, so no external references outside of the RAGE
 * vector/math libraries and our own math macros.
 */

#ifndef NM_RS_UTILS_H
#define NM_RS_UTILS_H

#include "NMutils/NMTypes.h"
#include "physics/inst.h"
#include "vector/geometry.h"

namespace ART
{
 #define WM_STATES(_action) \
  _action(kNone) \
  _action(kPistol)              /*Two handed. pistolRight with left supporting*/ \
  _action(kDual)                /*pistolLeft and pistolRight*/ \
  _action(kRifle)               /*Two handed. primary Right with left supporting*/ \
  _action(kSidearm)             /*pistolRight - Left hand is holding a rifle to the side*/ \
  _action(kPistolLeft) \
  _action(kPistolRight) \
  _action(kNumWeaponModes)

  enum WeaponMode 
  {
#define WM_ENUM_ACTION(_name) _name,
    WM_STATES(WM_ENUM_ACTION)
#undef WM_ENUM_ACTION
  };

  typedef unsigned int ResetEffectorsType;
  const ResetEffectorsType kResetCalibrations = (1 << 0);
  const ResetEffectorsType kResetAngles = (1 << 1);
  const ResetEffectorsType kResetMuscleStiffness = (1 << 2);

#define NMHandAnimationFeedbackName "handAnimation"
#define HA_STATES(_action) \
  _action(haNone) \
  _action(haLoose)              /*Loose*/ \
  _action(haHoldingWeapon)      /*Holding Weapon (support hand)*/ \
  _action(haGrab)               /*Grabbing*/ \
  _action(haBrace)              /*Bracing/Flat*/ \
  _action(haFlail)              /*e.g. in highFall*/ \
  _action(haImpact)              /*should be used for a second or so when the character hits the ground (or probably slightly before)*/ \
  _action(haNumHandAnimationStates)

  enum HandAnimationType 
  {
#define HA_ENUM_ACTION(_name) _name,
    HA_STATES(HA_ENUM_ACTION)
#undef HA_ENUM_ACTION
  };

  // Functions to translate between lean/twist and quaternion (with twist on z axis)
  // Used for drive NOT LIMIT.
  rage::Vector3 rsQuatToRageDriveTwistSwing(const rage::Quaternion& quat);
  rage::Quaternion rsRageDriveTwistSwingToQuat(const rage::Vector3& ts);

  // Functions to translate between standard lean twist (separate twist)
  // and quaternion.  Used for limit clamping, but not drive (RAGE does
  // these two in different spaces).
  rage::Vector3 rsQuatToRageLimitTwistSwing(const rage::Quaternion& quat);
  rage::Quaternion rsRageLimitTwistSwingToQuat(const rage::Vector3& ts);

  // modifies the positions of the two vectors given to ensure they are 'length' apart
  void separateFeet(rage::Vector3& leftFoot, rage::Vector3& rightFoot, float length);
  void constrainFeet(rage::Vector3& leftFoot, rage::Vector3& rightFoot, float length, rage::Vector3& progCom);

  void closestPointOnEllipse(float& x, float& y, float A, float B);

  //
  float rotateFromVel(
    float dir, 
    const rage::Matrix34& pelvisMat, 
    const rage::Vector3& vel, 
    rage::Vector3::Vector3Param comRotVel, 
    bool dualFacing, 
    bool incremental);

  //
  float pointInsideSupport(
    const rage::Vector3 &point, 
    const rage::Matrix34 *leftMat, 
    const rage::Matrix34 *rightMat, 
    float footWidth, 
    float footLength, 
    const rage::Vector3 &up, 
    rage::Vector3 *nearestPoint);

  // 
  void clampTarget(
    rage::Vector3 &target,
    const rage::Vector3 &upVector,
    const rage::Matrix34& pelvisMat,
    float lean1Extent,
    float lean2Extent);

  //
  float pointInsideSupportNew(
    const rage::Vector3 &point, 
    const rage::Matrix34 *leftMat, 
    const rage::Matrix34 *rightMat, 
    const rage::Matrix34 *leftHand, bool leftHandCollided ,
    const rage::Matrix34 *rightHand, bool rightHandCollided , 
    float footWidth, 
    float footLength, 
    const rage::Vector3 &up, 
    rage::Vector3 *nearestPoint);

  //avoid stepping into car functions:
	void setCorner(int index, const rage::Vector3 &corner);
	void setNumOfCorners(int numOfCorners);
	void getIntersectionPointOnPolygon(const rage::Vector3 &position/*, const rage::Vector3 &up*/, const rage::Vector3 &fromPosition, rage::Vector3 *pointOnPolygon);

  float getDistanceToPoint(const rage::Vector3& position, const rage::Vector3& normal/*, float radius*/, rage::Vector3* closestPointInPolygon); //when a member function make const;
  void buildConvexHull2D(/*const*/ rage::Vector3* points, int numPoints, const rage::Vector3& up);
  float polarAngle(const rage::Vector3 &v, const rage::Vector3 &up, const rage::Vector3 &side);
  int zoGrahamSortAnglesFn(const void *a, const void *b);
  float isPointLeftOfLine(const rage::Vector3 &p0, const rage::Vector3 &p1, const rage::Vector3 &p2, const rage::Vector3 &normal);    
#if !__SPU & ART_ENABLE_BSPY
  void drawConvexHull(const rage::Vector3 &col);
#endif
}

namespace rage
{
  Vec3V_Out nmGetOrthogonal(Vec3V_In u);
}

#endif // NM_ROCKSTAR_UTILS_H
