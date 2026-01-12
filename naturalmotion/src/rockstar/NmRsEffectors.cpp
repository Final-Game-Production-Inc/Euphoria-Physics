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
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

namespace ART
{
  void NmRsEffectorBase::initialiseData()
  {
    state.m_jointMatrixCacheValid = false;
    state.m_jointQuatFromITMValid = false;
  }

  rage::phArticulatedBodyPart *NmRsEffectorBase::getParentPart() const
  { 
    return &m_character->getArticulatedBody()->GetLink(getParentIndex()); 
  }

  rage::phArticulatedBodyPart *NmRsEffectorBase::getChildPart() const
  { 
    return &m_character->getArticulatedBody()->GetLink(getChildIndex()); 
  }

  void NmRsEffectorBase::calculateJointMatrixCache() const
  {
    rage::Matrix34 orientParent = getJoint()->GetOrientationParent();
    rage::Matrix34 orientChild;
    orientChild = getJoint()->GetOrientationChild();
    rage::Matrix34 instMatrix = RCC_MATRIX34(m_character->getArticulatedWrapper()->getArticulatedCollider()->GetMatrix());

    const rage::Matrix34 parentMatrix = MAT34V_TO_MATRIX34(getParentPart()->GetMatrix());
    const rage::Matrix34 childMatrix = MAT34V_TO_MATRIX34(getChildPart()->GetMatrix());

    // somewhat horrible but getMatrixX functions, which call calculateJointMatrixCache
    // are effectively const functions from a usage perspective
    rage::Matrix34 *matrix1Cache = const_cast<rage::Matrix34*>(&m_matrix1Cache);
    rage::Matrix34 *matrix2Cache = const_cast<rage::Matrix34*>(&m_matrix2Cache);
    EffectorBitField *_state = const_cast<EffectorBitField*>(&state);

    matrix1Cache->Set(parentMatrix);
    matrix2Cache->Set(childMatrix);
    matrix1Cache->Dot3x3(orientParent);
    matrix2Cache->Dot3x3(orientChild);
    matrix1Cache->Transpose();
    matrix2Cache->Transpose();
    parentMatrix.UnTransform3x3( getJoint()->GetPositionParent(), matrix1Cache->d );
    childMatrix.UnTransform3x3( getJoint()->GetPositionChild(), matrix2Cache->d );
    matrix1Cache->d.Add(instMatrix.d);
    matrix2Cache->d.Add(instMatrix.d);
    matrix1Cache->d.Add(parentMatrix.d);
    matrix2Cache->d.Add(childMatrix.d);
    _state->m_jointMatrixCacheValid = true;
  }

  void NmRsEffectorBase::getMatrix1(rage::Matrix34 &mat) const
  {
    if (!state.m_jointMatrixCacheValid)
      calculateJointMatrixCache();

    mat.Set(m_matrix1Cache);
  }

  void NmRsEffectorBase::getMatrix2(rage::Matrix34 &mat) const
  {
    if (!state.m_jointMatrixCacheValid)
      calculateJointMatrixCache();

    mat.Set(m_matrix2Cache);
  }

  rage::Vector3 NmRsEffectorBase::getJointPosition() const
  {
    rage::Vector3 pos(getJoint()->GetJointPosition(m_character->getArticulatedBody()));
    rage::Matrix34 instMatrix = RCC_MATRIX34(m_character->getArticulatedWrapper()->getArticulatedCollider()->GetMatrix());
    return pos + instMatrix.d;
  }

  void NmRsEffectorBase::getMatrixIncomingTransform1(rage::Matrix34 &jointMatrix1, rage::Matrix34 &parentPartTM, rage::Vector3 *rotVel) const
  {
    jointMatrix1 = parentPartTM;
    rage::Matrix34 orientParent = getJoint()->GetOrientationParent();
    jointMatrix1.Dot(orientParent);
    jointMatrix1.Transpose();
    if (rotVel)
      rotVel->Dot3x3Transpose(jointMatrix1);
    jointMatrix1.d.Set(getJoint()->GetJointPosition(m_character->getArticulatedBody()));
  }

  void NmRsEffectorBase::getMatrixIncomingTransform2(rage::Matrix34 &mat, rage::Matrix34 &mat2) const
  {
    mat = mat2;
    rage::Matrix34 orientChild;
    orientChild = getJoint()->GetOrientationChild();
    mat.Dot3x3(orientChild);
    mat.Transpose();
    mat.d.Set(getJoint()->GetJointPosition(m_character->getArticulatedBody()));
  }

  /**
  * Calculates twist, swing1, and swing2 parameters from the current incoming transforms
  */
  void NmRsEffectorBase::calculateJointQuatFromITMCache(IncomingTransformSource transformSource) const
  {
    Assert(m_character);

    // somewhat horrible but getMatrixX functions, which call calculateJointMatrixCache
    // are effectively const functions from a usage perspective
    rage::Quaternion *jointQuatCache = const_cast<rage::Quaternion*>(&m_jointQuatCache);

    EffectorBitField *_state = const_cast<EffectorBitField*>(&state);

    _state->m_jointQuatFromITMSuccess = false;

    _state->m_jointQuatFromITMValid = true;

    (*jointQuatCache).Identity();
    (*jointQuatCache).w = 0.0f;

    // Get the transforms from the agent.
    int incomingComponentCount = 0;

    rage::Matrix34 *itPtr = 0;
    IncomingTransformStatus itmStatusFlags = kITSNone;
    m_character->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, transformSource);

    // nothing to work with?
    if (incomingComponentCount == 0 || itPtr == 0)
    {
      _state->m_jointQuatFromITMSuccess = false;
      return;
    }

    // Check the ranges of the parent and child part indices.
    int indexParent = getParentIndex();
    int indexChild = getChildIndex();
    if ((indexParent > - 1 && indexParent < incomingComponentCount) && (indexChild > -1 && indexChild < incomingComponentCount))
    {
      rage::Matrix34 jP;
      rage::Matrix34 mat1 = itPtr[indexParent];
      mat1.Transpose();
      getMatrixIncomingTransform1(jP, mat1);

      // Joint coordinate frame relative to child.
      rage::Matrix34 jC;
      rage::Matrix34 mat2 = itPtr[indexChild];
      mat2.Transpose();
      getMatrixIncomingTransform2(jC, mat2);

      rage::Matrix34 mat;
      mat.Dot3x3Transpose(jC, jP);
      mat.ToQuaternion(*jointQuatCache);

      _state->m_jointQuatFromITMSuccess = true;
    }
  }

  bool NmRsEffectorBase::getJointQuaternionFromIncomingTransform(rage::Quaternion* q, IncomingTransformSource transformSource) const
  {
    if (!state.m_jointQuatFromITMValid)
      calculateJointQuatFromITMCache(transformSource);

    *q = m_jointQuatCache;

    return state.m_jointQuatFromITMSuccess;
  }

  bool NmRsEffectorBase::getJointQuaternionFromIncomingTransform_uncached(rage::Quaternion* q, IncomingTransformSource transformSource) const
  {
    calculateJointQuatFromITMCache(transformSource);
    *q = m_jointQuatCache;
    return state.m_jointQuatFromITMSuccess;
  }

  bool NmRsEffectorBase::getJointQuatPlusVelFromIncomingTransform(rage::Quaternion &quat, rage::Vector3 &rotVel) const
  {
    // Get the transforms from the character.
    int incomingComponentCount = 0;
    rage::Matrix34 *prevPtr = 0;
    rage::Matrix34 *itPtr = 0;

    IncomingTransformStatus itmStatusFlags = kITSNone;
    m_character->getIncomingTransforms(&prevPtr, itmStatusFlags, incomingComponentCount, kITSourcePrevious);
    m_character->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, kITSourceCurrent);

    // nothing to work with?
    if (incomingComponentCount == 0 || itPtr == 0)
      return false;

    // Check the ranges of the parent and child part indices.
    int indexParent = getParentIndex();
    int indexChild = getChildIndex();
    if ((indexParent > - 1 && indexParent < incomingComponentCount) && (indexChild > -1 && indexChild < incomingComponentCount))
    {
      rage::Vector3 axisP, axisC;
      float angle;
      rage::Quaternion qRel;
      rage::Matrix34 relative;
      rage::Matrix34 parent, child;
      parent = prevPtr[indexParent];
      rage::Matrix34 parent2 = itPtr[indexParent];
      rage::Matrix34 invParent;
      invParent.Inverse3x3(parent);
      relative.Dot3x3(invParent, parent2);
      qRel.FromMatrix34(relative);
      qRel.ToRotation(axisP, angle);
      axisP *= angle;
      child  = prevPtr[indexChild];
      rage::Matrix34 child2 = itPtr[indexChild];
      rage::Matrix34 invChild;
      invChild.Inverse3x3(child);
      relative.Dot3x3(invChild, child2);
      qRel.FromMatrix34(relative);
      qRel.ToRotation(axisC, angle);
      axisC *= angle;

      rotVel = axisC - axisP;

      // Joint coordinate frame relative to parent.
      rage::Matrix34 jP;
      parent.Transpose();
      getMatrixIncomingTransform1(jP, parent, &rotVel);

      rage::Matrix34 jC;
      child.Transpose();
      getMatrixIncomingTransform2(jC, child);

      jC.Dot3x3Transpose(jP);
      quat.FromMatrix34(jC);
    }
    return true;
  }

  void callMaskedEffectorFunctionFloatArg(
    NmRsCharacter* character, 
    BehaviourMask mask,
    float floatValue,
    Effector1DFunctionFloatArg oneDofFn,
    Effector3DFunctionFloatArg threeDofFn)
  {
    for(int i = 0; i < character->getNumberOfEffectors(); ++i)
    {
      NmRsEffectorBase* effector = character->getEffectorDirect(i);
      Assert(effector);
      if(character->isEffectorInMask(mask, i))
      {
        if(effector->is3DofEffector())
        {
          NmRs3DofEffector* threeDof = (NmRs3DofEffector*) effector;
          Assert(threeDof);
#if ART_ENABLE_BSPY
          (threeDof->*threeDofFn)(floatValue, 0);
#else
          (threeDof->*threeDofFn)(floatValue);
#endif
        }
        else
        {
          NmRs1DofEffector* oneDof = (NmRs1DofEffector*) effector;
          Assert(oneDof);
#if ART_ENABLE_BSPY
          (oneDof->*oneDofFn)(floatValue, 0);
#else
          (oneDof->*oneDofFn)(floatValue);
#endif
        }
      }
    }
  }

#if 0
  void callMaskedEffectorFunctionIntArg(
    NmRsCharacter* character, 
    BehaviourMask mask,
    int intValue,
    Effector1DFunctionIntArg oneDofFn,
    Effector3DFunctionIntArg threeDofFn)
  {
    for(int i = 0; i < character->getNumberOfEffectors(); ++i)
    {
      NmRsEffectorBase* effector = character->getEffectorDirect(i);
      Assert(effector);
      if(character->isEffectorInMask(mask, i))
      {
        if(effector->is3DofEffector())
        {
          NmRs3DofEffector* threeDof = (NmRs3DofEffector*) effector;
          Assert(threeDof);
          (threeDof->*threeDofFn)(intValue);
        }
        else
        {
          NmRs1DofEffector* oneDof = (NmRs1DofEffector*) effector;
          Assert(oneDof);
          (oneDof->*oneDofFn)(intValue);
        }
      }
    }
  }
#endif

  void callMaskedEffectorFunctionNoArgs(
    NmRsCharacter* character, 
    BehaviourMask mask,
    Effector1DFunctionNoArgs oneDofFn,
    Effector3DFunctionNoArgs threeDofFn)
  {
    for(int i = 0; i < character->getNumberOfEffectors(); ++i)
    {
      NmRsEffectorBase* effector = character->getEffectorDirect(i);
      Assert(effector);
      if(character->isEffectorInMask(mask, i))
      {
        if(effector->is3DofEffector())
        {
          NmRs3DofEffector* threeDof = (NmRs3DofEffector*) effector;
          Assert(threeDof);
          (threeDof->*threeDofFn)();
        }
        else
        {
          NmRs1DofEffector* oneDof = (NmRs1DofEffector*) effector;
          Assert(oneDof);
          (oneDof->*oneDofFn)();
        }
      }
    }
  }
}
