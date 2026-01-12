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
#include "NmRsCharacter.h"
#include "NmRsEngine.h"
#include "NmRsGenericPart.h"
#include "NmRsTypeUtils.h"
#include "NmRsUtils.h"
#include "NmRsPhInstNM.h"

#include "physics/colliderdispatch.h"

namespace ART
{
   NmRsGenericPart::NmRsGenericPart(ART::MemoryManager* services, void* ptr, int partIndex, NmRsCharacter *character) : 
    m_partIndex(partIndex), 
    m_indexInLevel(0),
    m_subtreeMass(0),
    m_character(character), 
    m_ptr(ptr),
    m_wrapper(0),
    m_currentMtx(0),
    m_lastMtx(0),
    m_artMemoryManager(services)
  {
    m_wrapper = m_character->getArticulatedWrapper();
    initialiseData();
  }

  NmRsGenericPart::~NmRsGenericPart()
  { 
  }

  void NmRsGenericPart::saveToShadow(ShadowGPart& state) const
  {
    Assert(m_wrapper);

    getArticulatedMatrix(state.m_tm);
    state.m_linVel = VEC3V_TO_VECTOR3(m_character->getArticulatedBody()->GetLinearVelocityNoProp(m_partIndex)); // NoProp
    state.m_angVel = VEC3V_TO_VECTOR3(m_character->getArticulatedBody()->GetAngularVelocityNoProp(m_partIndex)); // NoProp
  }

  rage::phInst *NmRsGenericPart::getInstance() const
  {
     return m_character->getArticulatedWrapper()->getArticulatedPhysInstance();
  }

  void NmRsGenericPart::initialiseData()
  {
    m_indexInLevel = NM_RS_INVALID_LVL_INDEX;

    state.m_collisionEnabled = true;
    state.m_collided = false;
    state.m_collidedOtherCharacters = false;
    state.m_collidedOwnCharacter = false;
    resetCollided();

    m_subtreeMass = 0.0f;
    m_subtreeCOM.Set(0,0,0);
    m_subtreeCOMvel.Set(0,0,0);

    m_fictionMultiplier = 1.f;
    m_elasticityMultiplier = 1.f;

    // need to investigate whether m_initialMtx and m_lastMtx are reset properly before initialisePart
  }

  void NmRsGenericPart::resetCollided()
  { 
    m_zmpPositionOwnCharacter[zmpCalculation].Zero();
    m_zmpPositionOwnCharacter[zmpResults].Zero();
    m_zmpNormalOwnCharacter[zmpCalculation].Zero();
    m_zmpNormalOwnCharacter[zmpResults].Zero();
    m_zmpDepthOwnCharacter[zmpCalculation] = 0.0f;
    m_zmpDepthOwnCharacter[zmpResults] = 0.0f;
    m_depthTotalOwnCharacter = 1e-10f;
    m_collidedInstOwnCharacter = NULL;

    m_zmpPositionOtherCharacters[zmpCalculation].Zero();
    m_zmpPositionOtherCharacters[zmpResults].Zero();
    m_zmpNormalOtherCharacters[zmpCalculation].Zero();
    m_zmpNormalOtherCharacters[zmpResults].Zero();
    m_zmpDepthOtherCharacters[zmpCalculation] = 0.0f;
    m_zmpDepthOtherCharacters[zmpResults] = 0.0f;
    m_depthTotalOtherCharacters = 1e-10f;
    m_collidedInstOtherCharacters = NULL;
    m_collidedInstGenIDOtherCharacters = -1;

    m_zmpPositionEnvironment[zmpCalculation].Zero();
    m_zmpPositionEnvironment[zmpResults].Zero();
    m_zmpNormalEnvironment[zmpCalculation].Zero();
    m_zmpNormalEnvironment[zmpResults].Zero();
    m_zmpDepthEnvironment[zmpCalculation] = 0.0f;
    m_zmpDepthEnvironment[zmpResults] = 0.0f;
    m_depthTotalEnvironment = 1e-10f;
    m_collidedInstEnvironment = NULL;
    m_collidedInstGenIDEnvironment = -1;

    state.m_previousCollided = state.m_collided;
    state.m_previousCollidedOtherCharacters = state.m_collidedOtherCharacters;
    state.m_previousCollidedOwnCharacter = state.m_collidedOwnCharacter;
    state.m_previousCollidedEnvironment = state.m_collidedEnvironment;
    state.m_collided = false;
    state.m_collidedOtherCharacters = false;
    state.m_collidedOwnCharacter = false;
    state.m_collidedEnvironment = false;
    state.m_zmpRecalcOwnCharacter = false;
    state.m_zmpRecalcOtherCharacters = false;
    state.m_zmpRecalcEnvironment = false;
    state.m_zmpRecalcNotOwnCharacter = false;
  }

  void NmRsGenericPart::getCollisionZMPWithNotOwnCharacter(rage::Vector3 &zmpPos, rage::Vector3 &zmpNormal, float *zmpDepth, rage::phInst **zmpInst, int* zmpInstGenID)
  {

    if (state.m_zmpRecalcNotOwnCharacter)
    {
      Assert(m_depthTotalEnvironment != 0.0f);
      Assert(m_depthTotalEnvironment == m_depthTotalEnvironment);
      Assert(m_depthTotalOtherCharacters != 0.0f);
      Assert(m_depthTotalOtherCharacters == m_depthTotalOtherCharacters);

      float invDT = 1.0f / (m_depthTotalEnvironment + m_depthTotalOtherCharacters);
      rage::Vector3 invDepthTotalVec;
      invDepthTotalVec.Set(invDT);

      m_zmpNormalNotOwnCharacter.SetScaled(m_zmpNormalEnvironment[zmpCalculation], invDepthTotalVec);
      m_zmpNormalNotOwnCharacter.AddScaled(m_zmpNormalOtherCharacters[zmpCalculation], invDepthTotalVec);

      m_zmpPositionNotOwnCharacter.SetScaled(m_zmpPositionEnvironment[zmpCalculation], invDepthTotalVec);
      m_zmpPositionNotOwnCharacter.AddScaled(m_zmpPositionOtherCharacters[zmpCalculation], invDepthTotalVec);

      m_zmpDepthNotOwnCharacter = m_zmpDepthEnvironment[zmpCalculation] * invDT;
      m_zmpDepthNotOwnCharacter += m_zmpDepthOtherCharacters[zmpCalculation] * invDT;

      m_zmpNormalNotOwnCharacter.Normalize();

      state.m_zmpRecalcNotOwnCharacter = false;
    }

    zmpPos.Set(m_zmpPositionNotOwnCharacter);
    zmpNormal.Set(m_zmpNormalNotOwnCharacter);

    if (zmpDepth)
    {
      *zmpDepth = m_zmpDepthNotOwnCharacter;
    }
    if (zmpInst)
    {
      //Give out the environment inst takes priority over the otherCharacters inst
      if (m_collidedInstOtherCharacters)
      {
        *zmpInst = m_collidedInstOtherCharacters;
        if (zmpInstGenID)
          *zmpInstGenID = m_collidedInstGenIDOtherCharacters;
      }
      if (m_collidedInstEnvironment)
      {
        *zmpInst = m_collidedInstEnvironment;
        if (zmpInstGenID)
          *zmpInstGenID = m_collidedInstGenIDEnvironment;
      }
    }

  }

  void NmRsGenericPart::getCollisionZMPWithEnvironment(rage::Vector3 &zmpPos, rage::Vector3 &zmpNormal, float *zmpDepth, rage::phInst **zmpInst, int *zmpInstGenID)
  {
    if (state.m_zmpRecalcEnvironment)
    {
      Assert(m_depthTotalEnvironment != 0.0f);
      Assert(m_depthTotalEnvironment == m_depthTotalEnvironment);

      float invDT = 1.0f / m_depthTotalEnvironment;
      rage::Vector3 invDepthTotalVec;
      invDepthTotalVec.Set(invDT);

      m_zmpNormalEnvironment[zmpResults].SetScaled(m_zmpNormalEnvironment[zmpCalculation], invDepthTotalVec);
      m_zmpPositionEnvironment[zmpResults].SetScaled(m_zmpPositionEnvironment[zmpCalculation], invDepthTotalVec);
      m_zmpDepthEnvironment[zmpResults] = m_zmpDepthEnvironment[zmpCalculation] * invDT;
      m_zmpNormalEnvironment[zmpResults].Normalize();

      state.m_zmpRecalcEnvironment = false;
    }

    zmpPos.Set(m_zmpPositionEnvironment[zmpResults]);
    zmpNormal.Set(m_zmpNormalEnvironment[zmpResults]);

    if (zmpDepth)
    {
      *zmpDepth = m_zmpDepthEnvironment[zmpResults];
    }
    if (zmpInst && m_collidedInstEnvironment)
    {
        *zmpInst = m_collidedInstEnvironment;
        if (zmpInstGenID)
          *zmpInstGenID = m_collidedInstGenIDEnvironment;
    }
  }
#if NM_UNUSED_CODE
  void NmRsGenericPart::getCollisionZMPWithOwnCharacter(rage::Vector3 &zmpPos, rage::Vector3 &zmpNormal, float *zmpDepth)
  {
    if (state.m_zmpRecalcOwnCharacter)
    {
      Assert(m_depthTotalOwnCharacter != 0.0f);
      Assert(m_depthTotalOwnCharacter == m_depthTotalOwnCharacter);

      float invDT = 1.0f / m_depthTotalOwnCharacter;
      rage::Vector3 invDepthTotalVec;
      invDepthTotalVec.Set(invDT);

      m_zmpNormalOwnCharacter[zmpResults].SetScaled(m_zmpNormalOwnCharacter[zmpCalculation], invDepthTotalVec);
      m_zmpPositionOwnCharacter[zmpResults].SetScaled(m_zmpPositionOwnCharacter[zmpCalculation], invDepthTotalVec);
      m_zmpDepthOwnCharacter[zmpResults] = m_zmpDepthOwnCharacter[zmpCalculation] * invDT;
      m_zmpNormalOwnCharacter[zmpResults].Normalize();

      state.m_zmpRecalcOwnCharacter = false;
    }

    zmpPos.Set(m_zmpPositionOwnCharacter[zmpResults]);
    zmpNormal.Set(m_zmpNormalOwnCharacter[zmpResults]);

    if (zmpDepth)
    {
      *zmpDepth = m_zmpDepthOwnCharacter[zmpResults];
    }
  }

  void NmRsGenericPart::getCollisionZMPWithOtherCharacters(rage::Vector3 &zmpPos, rage::Vector3 &zmpNormal, float *zmpDepth, rage::phInst **zmpInst, int *zmpInstGenID)
  {
    if (state.m_zmpRecalcOtherCharacters)
    {
      Assert(m_depthTotalOtherCharacters != 0.0f);
      Assert(m_depthTotalOtherCharacters == m_depthTotalOtherCharacters);

      float invDT = 1.0f / m_depthTotalOtherCharacters;
      rage::Vector3 invDepthTotalVec;
      invDepthTotalVec.Set(invDT);

      m_zmpNormalOtherCharacters[zmpResults].SetScaled(m_zmpNormalOtherCharacters[zmpCalculation], invDepthTotalVec);
      m_zmpPositionOtherCharacters[zmpResults].SetScaled(m_zmpPositionOtherCharacters[zmpCalculation], invDepthTotalVec);
      m_zmpDepthOtherCharacters[zmpResults] = m_zmpDepthOtherCharacters[zmpCalculation] * invDT;
      m_zmpNormalOtherCharacters[zmpResults].Normalize();

      state.m_zmpRecalcOtherCharacters = false;
    }

    zmpPos.Set(m_zmpPositionOtherCharacters[zmpResults]);
    zmpNormal.Set(m_zmpNormalOtherCharacters[zmpResults]);

    if (zmpDepth)
    {
      *zmpDepth = m_zmpDepthOtherCharacters[zmpResults];
    }
    if (zmpInst && m_collidedInstOtherCharacters)
    {
        *zmpInst = m_collidedInstOtherCharacters;
        if (zmpInstGenID)
          *zmpInstGenID = m_collidedInstGenIDOtherCharacters;
    }
  }
#endif
  void NmRsGenericPart::initialisePart()
  {
    // make sure all variables are reset before insertion
    initialiseData();

    updateMatrices();
    updateMatrices(); // TDL this extra call means lastMatrix is initialized to the current matrix at the start. Very important.

  }

  void NmRsGenericPart::getMatrix(rage::Matrix34 &mtm) const
  {
     getArticulatedMatrix(mtm);
  }

  void NmRsGenericPart::getArticulatedMatrix(rage::Matrix34& mtm) const
  {
    Assert(m_wrapper);
    rage::Matrix34 colliderMtx(RCC_MATRIX34(m_wrapper->getArticulatedCollider()->GetMatrix()));

    //get bound's matrix when the root is rotated
    if (isRootPart())
    {
      rage::Matrix34 instMatrix = RCC_MATRIX34(m_wrapper->getArticulatedPhysInstance()->GetMatrix());
      mtm.Dot(*m_currentMtx, instMatrix);
    }
    else
    {
      rage::phArticulatedBodyPart* bodyPart = (rage::phArticulatedBodyPart*)m_ptr;
      mtm = MAT34V_TO_MATRIX34(bodyPart->GetMatrix());
      mtm.Transpose();
      mtm.d += colliderMtx.d;
    }
  }

  void NmRsGenericPart::getBoundMatrix(rage::Matrix34 *mat) const
  {

    Assert(m_wrapper);
    rage::Matrix34 instMatrix = RCC_MATRIX34(m_wrapper->getArticulatedPhysInstance()->GetMatrix());
    mat->Dot(*m_currentMtx, instMatrix);
  }

  void NmRsGenericPart::updateMatrices()
  {
    rage::Matrix34 retMtx, artPhysMtx;
    retMtx.Identity();

    if (m_lastMtx)
      m_lastMtx->Set(*m_currentMtx);

    if (m_currentMtx)
    {
      Assert(m_wrapper);

      rage::phArticulatedBodyPart* bodyPart = (rage::phArticulatedBodyPart*)m_ptr;
      rage::Matrix34 tempMatrix = MAT34V_TO_MATRIX34(bodyPart->GetMatrix());
      tempMatrix.Transpose();

      // convert to world space
      tempMatrix.d += VEC3V_TO_VECTOR3(m_wrapper->getArticulatedCollider()->GetMatrix().GetCol3());

      // localize to the instance
      rage::Matrix34 instMatrix = RCC_MATRIX34(m_wrapper->getArticulatedPhysInstance()->GetMatrix());
      instMatrix.Inverse();
      retMtx.Dot(tempMatrix, instMatrix);

      m_currentMtx->Set(retMtx);
    }
  }

  void NmRsGenericPart::setMatrix(const rage::Matrix34 &rageMatrix, bool initial)
  {
    Assert(rageMatrix.IsOrthonormal());


    Assert(m_wrapper);

    rage::phArticulatedBodyPart* bodyPart = (rage::phArticulatedBodyPart*)m_ptr;
    rage::Matrix34 partMatrix = rageMatrix;
    rage::phArticulatedCollider *collider = m_wrapper->getArticulatedCollider();

    rage::Matrix34 colliderMtx = RCC_MATRIX34(collider->GetMatrix());
    Assert(colliderMtx.IsOrthonormal());

    partMatrix.d -= colliderMtx.d; 
    partMatrix.Transpose();
    if (initial)
    {
      bodyPart->SetMatrix(partMatrix);
    }
    else
    {
      bodyPart->SetPosition(partMatrix.d); // part matrix is local to collider (int translation) and transposed
      bodyPart->SetOrientation(partMatrix);
    }

    Assert(MAT34V_TO_MATRIX34(bodyPart->GetMatrix()).IsOrthonormal());

    rage::Matrix34 instMatrix = RCC_MATRIX34(m_wrapper->getArticulatedPhysInstance()->GetMatrix());
    instMatrix.Inverse();
    m_currentMtx->Dot(rageMatrix, instMatrix);  // bound matrix must be local to the instance

#if CRAWL_LEARNING

    if (m_character->getLearningCrawl())
    {
      //For crawl Learner
      if (m_lastMtx)
        m_lastMtx->Set(*m_currentMtx);
    }
#endif

  }

  void NmRsGenericPart::teleportMatrix(const rage::Matrix34 &matrix, bool initial)
  {
    setMatrix(matrix, initial);
    if (m_lastMtx)
      m_lastMtx->Set(*m_currentMtx);
  }

  rage::Vector3 NmRsGenericPart::getPosition() const
  {
    rage::Vector3 retVec;

    Assert(m_wrapper);

    rage::phArticulatedBodyPart* bodyPart = (rage::phArticulatedBodyPart*)m_ptr;
    rage::Matrix34 artPhysMtx = RCC_MATRIX34(m_wrapper->getArticulatedCollider()->GetMatrix());

    retVec.Set(bodyPart->GetPosition());
    retVec.Add(artPhysMtx.d);

    Assert(retVec.x == retVec.x && retVec.y == retVec.y && retVec.z == retVec.z);
    return retVec;
  }

  rage::Vector3 NmRsGenericPart::getVelocityAtLocalPoint(const rage::Vector3 &point) const
  {
    rage::Vector3 retVec;

    m_character->getArticulatedBody()->GetLocalVelocityNoProp(m_partIndex, point, &retVec); // NoProp

    Assert(retVec.x == retVec.x && retVec.y == retVec.y && retVec.z == retVec.z);
    return retVec;
  }

  rage::Vector3 NmRsGenericPart::getLinearVelocity(const rage::Vector3 *point) const
  {
    rage::Vector3 retVec;

    if (point)
    {
      Assert(m_wrapper);

      rage::Matrix34 colliderMatrix = RCC_MATRIX34(m_wrapper->getArticulatedCollider()->GetMatrix());
      rage::Vector3 localPos = *point - colliderMatrix.d;
      m_character->getArticulatedBody()->GetLocalVelocityNoProp(m_partIndex, localPos, &retVec); // NoProp
    }
    else
    {
      retVec = VEC3V_TO_VECTOR3(m_character->getArticulatedBody()->GetLinearVelocityNoProp(m_partIndex)); // NoProp
    }

    Assert(retVec.x == retVec.x && retVec.y == retVec.y && retVec.z == retVec.z);
    return retVec;
  }

  rage::Vector3 NmRsGenericPart::getAngularVelocity() const
  {
    rage::Vector3 retVec;
    retVec = VEC3V_TO_VECTOR3(m_character->getArticulatedBody()->GetAngularVelocityNoProp(m_partIndex)); // NoProp

    Assert(retVec.x == retVec.x && retVec.y == retVec.y && retVec.z == retVec.z);
    return retVec;
  }

   void NmRsGenericPart::setLinearVelocity(float x, float y, float z)
  {
    rage::Vector3 lVec(x,y,z);
    setLinearVelocity(lVec);
  }

  void NmRsGenericPart::setAngularVelocity(float x, float y, float z)
  {
    rage::Vector3 aVec(x,y,z);
    setAngularVelocity(aVec);
  }

  void NmRsGenericPart::setLinearVelocity(rage::Vector3 &linVel)
  {
    m_character->getArticulatedBody()->SetLinearVelocity(m_partIndex, RCC_VEC3V(linVel));
  }

  void NmRsGenericPart::setAngularVelocity(rage::Vector3 &angVel)
  {
    m_character->getArticulatedBody()->SetAngularVelocity(m_partIndex, RCC_VEC3V(angVel));
  }

  void NmRsGenericPart::setVelocities(rage::Vector3 &linear, rage::Vector3 &angular)
  {
    Assert(m_wrapper);
    m_character->getArticulatedBody()->SetVelocities(m_partIndex, RCC_VEC3V(linear), RCC_VEC3V(angular));

    if (m_wrapper->getArticulatedCollider()->GetSleep())
      m_wrapper->getArticulatedCollider()->GetSleep()->Reset();
  }

  void NmRsGenericPart::applyImpulse(const rage::Vector3 &impulse, const rage::Vector3 &position)
  {
#if ART_ENABLE_BSPY
    rage::Vector3 force = impulse;
    float timeStepClamped = rage::Max(m_character->getLastKnownUpdateStep(), SMALLEST_TIMESTEP);//protect against division by zero
    force /= timeStepClamped*60.f;
#endif

    Assert(m_wrapper);
    rage::Vector3 locPos(position);

    rage::Matrix34 artPhysMtx = RCC_MATRIX34(m_wrapper->getArticulatedCollider()->GetMatrix());
    locPos -= artPhysMtx.d;

#if NM_CHECK_VALID_VALUES
	if (!NmRsCharacter::CheckValidVector(impulse, 1) || !NmRsCharacter::CheckValidVector(locPos, 2))
		return;
#endif

	rage::Vec3V_In vImpulse = RCC_VEC3V(impulse);
	rage::Vec3V_In vPos = RCC_VEC3V(locPos);

    if (NmRsCharacter::sm_ApplyForcesImmediately)
      m_wrapper->getArticulatedBody()->ApplyImpulse(getPartIndex(), vImpulse, vPos);
    else
	{
		m_character->AddDeferredImpulse(m_partIndex, rage::Cross(vPos, vImpulse), vImpulse);
	}

#if ART_ENABLE_BSPY
    m_character->bspyDrawLine(position, position + force, rage::Vector3(0.f, 1.f, 1.f));
#endif

  }

  void NmRsGenericPart::applyForce(const rage::Vector3 &force, rage::Vector3 *position)
  {
    rage::Vector3 pos;
    if (position)
      pos.Set(*position);

    Assert(m_wrapper);
    rage::phArticulatedBodyPart* bodyPart = (rage::phArticulatedBodyPart*)m_ptr;

    if (!position)
    {
      pos = bodyPart->GetPosition();
    }
    else
    {
      rage::Matrix34 artPhysMtx = RCC_MATRIX34(m_wrapper->getArticulatedCollider()->GetMatrix());
      pos -= artPhysMtx.d;
    }

#if NM_CHECK_VALID_VALUES
	if (!NmRsCharacter::CheckValidVector(force, 1) || !NmRsCharacter::CheckValidVector(pos, 2))
		return;
#endif

	rage::Vec3V_In vImpulse = rage::Scale(RCC_VEC3V(force), rage::ScalarV(m_character->getLastKnownUpdateStep()));
	rage::Vec3V_In vPos = RCC_VEC3V(pos);

    if (NmRsCharacter::sm_ApplyForcesImmediately)
	{
      m_wrapper->getArticulatedBody()->ApplyForce(getPartIndex(), force, pos, rage::ScalarV(m_character->getLastKnownUpdateStep()).GetIntrin128ConstRef());
	}
    else
	{
		m_character->AddDeferredImpulse(m_partIndex, rage::Cross(vPos, vImpulse), vImpulse);
	}

#if ART_ENABLE_BSPY
    rage::Matrix34 artPhysMtx = RCC_MATRIX34(m_wrapper->getArticulatedCollider()->GetMatrix());
    m_character->bspyDrawLine(artPhysMtx.d + pos, artPhysMtx.d + pos + force, rage::Vector3(0.f, 1.f, 1.f) );
#endif

  }

  void NmRsGenericPart::applyTorque(const rage::Vector3 &torque)
  {
#if NM_CHECK_VALID_VALUES
	  if (!NmRsCharacter::CheckValidVector(torque))
		  return;
#endif

    if (NmRsCharacter::sm_ApplyForcesImmediately)
	{
      m_wrapper->getArticulatedBody()->ApplyTorque(getPartIndex(), VECTOR3_TO_INTRIN(torque), rage::ScalarV(m_character->getLastKnownUpdateStep()).GetIntrin128ConstRef());
	}
    else
	{
		rage::Vec3V angImpulse = Scale(RCC_VEC3V(torque), rage::ScalarV(m_character->getLastKnownUpdateStep()));
		m_character->AddDeferredImpulse(getPartIndex(), angImpulse, rage::Vec3V(rage::V_ZERO));
	}

#if ART_ENABLE_BSPY
    rage::Matrix34 mat;
    getMatrix(mat);
    m_character->bspyDrawLine(mat.d, mat.d + torque, rage::Vector3(1.f, 0.f, 1.f));
#endif
  }

  void NmRsGenericPart::applyTorqueImpulse(const rage::Vector3 &torqueImpulse)
  {
#if NM_CHECK_VALID_VALUES
	  if (!NmRsCharacter::CheckValidVector(torqueImpulse))
		  return;
#endif

#if ART_ENABLE_BSPY
    rage::Matrix34 mat;
    getMatrix(mat);
    rage::Vector3 torque = torqueImpulse;
    float timeStepClamped = rage::Max(m_character->getLastKnownUpdateStep(), SMALLEST_TIMESTEP);//protect against division by zero
    torque /= timeStepClamped*60.f;
#endif

    if (NmRsCharacter::sm_ApplyForcesImmediately)
	{
      m_wrapper->getArticulatedBody()->ApplyAngImpulse(getPartIndex(), VECTOR3_TO_INTRIN(torqueImpulse));
	}
    else
	{
		m_character->AddDeferredImpulse(getPartIndex(), RCC_VEC3V(torqueImpulse), rage::Vec3V(rage::V_ZERO));
	}

#if ART_ENABLE_BSPY
     m_character->bspyDrawLine(mat.d, mat.d + torque, rage::Vector3(1.f, 0.f, 1.f));
#endif

  }

  const rage::phBound* NmRsGenericPart::getBound() const
  {
    if (!m_ptr)
      return 0;

    Assert(m_wrapper);
    rage::phBoundComposite *compositeBound = (rage::phBoundComposite *)m_wrapper->getArticulatedCollider()->GetInstance()->GetArchetype()->GetBound();
    return compositeBound->GetBound(m_partIndex);
  }

  void NmRsGenericPart::applyVelocitiesToPart(
    const rage::Matrix34& fromTm,
    const rage::Matrix34& toTm,
    float invDeltaTime)
  {
    rage::Matrix34 fromTmRage(fromTm);
    rage::Matrix34 toTmRage(toTm);

    fromTmRage.Transpose();
    toTmRage.Transpose();

    rage::Matrix34 deltaPosition;
    deltaPosition.Subtract(toTmRage, fromTmRage);
    rage::Vector3 linearVelocity;
    deltaPosition.Transform(VEC3V_TO_VECTOR3(getBound()->GetCGOffset()), linearVelocity);

    rage::Matrix34 lastToCurrent;
    lastToCurrent.DotTranspose(toTmRage, fromTmRage);
    rage::Quaternion tempQuat;
    lastToCurrent.ToQuaternion(tempQuat);
    rage::Vector3 angularVelocity;
    float angle;
    tempQuat.ToRotation(angularVelocity,angle);
    angularVelocity.Scale(angle);

    angularVelocity.Scale(-invDeltaTime);
    linearVelocity.Scale(invDeltaTime);

    // apply fix-up scaling to all parts
    float velScale = m_character->getIncomingAnimationVelocityScale();

    angularVelocity.Scale(velScale);
    linearVelocity.Scale(velScale);

    rage::Vector3 averageVel = m_character->getEngine()->getAnimationAverageVel();
    linearVelocity -= averageVel;
    float maxSpeed = m_character->getEngine()->getAnimationMaxSpeed();
    //mmmmtodo output that clamp has been performed
    linearVelocity.ClampMag(0.f, maxSpeed);
    linearVelocity += averageVel;
    float maxAngSpeed = m_character->getEngine()->getAnimationMaxAngSpeed();
    angularVelocity.ClampMag(0.f, maxAngSpeed);

    Assertf(linearVelocity.x == linearVelocity.x && linearVelocity.y == linearVelocity.y && linearVelocity.z == linearVelocity.z,
      "linearVelocity is %f %f %f. velScale is %f. averageVel is %f %f %f", 
      linearVelocity.x, linearVelocity.y, linearVelocity.z,
      velScale, averageVel.x, averageVel.y, averageVel.z);

    Assertf(abs(linearVelocity.x <= 200.0f) && abs(linearVelocity.y <= 200.0f) && abs(linearVelocity.z <= 200.0f),
      "linearVelocity is %f %f %f. velScale is %f. averageVel is %f %f %f", 
      linearVelocity.x, linearVelocity.y, linearVelocity.z,
      velScale, averageVel.x, averageVel.y, averageVel.z);

    setVelocities(linearVelocity, angularVelocity);
  }

  void NmRsGenericPart::handleCollision(const rage::Vector3 &pos, const rage::Vector3 &normal, float depth, NmRsGenericPart* collidee, rage::phInst *collideeInst)
  {
    state.m_collided = true;

    // Added this clamp for gta4 to avoid an occasional divide by zero crash, that we thought was
    // caused by negative depth values coming in here
    //
    // clamp depth to a small positive number, as it may be negative
    depth = rage::Max(depth, 0.01f);

    if (collidee)//collided with an NM part 
    {
      if (collidee->getCharacter() != getCharacter()) 
      {
          state.m_collidedOtherCharacters = true;
#if NM_FAST_COLLISION_CHECKING
          getCharacter()->cacheOtherCharacterCollision(m_partIndex);
#endif
          m_collidedInstOtherCharacters = collideeInst;
          if (PHLEVEL->IsInLevel(collideeInst->GetLevelIndex()))
            m_collidedInstGenIDOtherCharacters = PHLEVEL->GetGenerationID(collideeInst->GetLevelIndex());
          state.m_zmpRecalcOtherCharacters = true;
          state.m_zmpRecalcNotOwnCharacter = true;
          // sum up the collision data
          m_zmpPositionOtherCharacters[zmpCalculation].AddScaled(pos, depth);
          m_zmpNormalOtherCharacters[zmpCalculation].AddScaled(normal, depth);
          m_zmpDepthOtherCharacters[zmpCalculation] += depth * depth;
          m_depthTotalOtherCharacters += depth;
      }
      else if (collidee->getCharacter() == getCharacter())
      {
        state.m_collidedOwnCharacter = true;
#if NM_FAST_COLLISION_CHECKING
        getCharacter()->cacheOwnCharacterCollision(m_partIndex);
#endif
        m_collidedInstOwnCharacter = collideeInst;
        state.m_zmpRecalcOwnCharacter = true;
        // sum up the collision data
        m_zmpPositionOwnCharacter[zmpCalculation].AddScaled(pos, depth);
        m_zmpNormalOwnCharacter[zmpCalculation].AddScaled(normal, depth);
        m_zmpDepthOwnCharacter[zmpCalculation] += depth * depth;
        m_depthTotalOwnCharacter += depth;
      }
    }
    else //Collided with a non NM part i.e. something in the Game Environment
    {
      state.m_collidedEnvironment = true;
#if NM_FAST_COLLISION_CHECKING
      getCharacter()->cacheEnvironmentCollision(m_partIndex);
#endif
      m_collidedInstEnvironment = collideeInst;
      if (PHLEVEL->IsInLevel(collideeInst->GetLevelIndex()))
        m_collidedInstGenIDEnvironment = PHLEVEL->GetGenerationID(collideeInst->GetLevelIndex());
      state.m_zmpRecalcEnvironment = true;
      state.m_zmpRecalcNotOwnCharacter = true;
      // sum up the collision data
      m_zmpPositionEnvironment[zmpCalculation].AddScaled(pos, depth);
      m_zmpNormalEnvironment[zmpCalculation].AddScaled(normal, depth);
      m_zmpDepthEnvironment[zmpCalculation] += depth * depth;
      m_depthTotalEnvironment += depth;

      //Make glancing blows from sides of vehicles more spinny
      //create more rolling when in contact with a vehicle
      if (collideeInst->GetClassType() == m_character->m_collision_vehicleClass)
      {
        //In order to damp out excessive cartwheeling and somersaulting
        m_character->m_spinDamping.vehicleCollisionTimer = m_character->m_spinDamping.vehicleCollisionTime;
        //Modulate friction when glancing the side of a vehicle
        //Make glancing blows from sides of vehicles more spinny
        m_character->m_collision_withVehicle = true;
        //create more rolling when in contact with a vehicle
        if (m_character->m_collision_spin > 0.0f)//vehicle
        {
          bool applyToPart = m_character->m_collision_applyToAll;
          applyToPart = applyToPart || ((m_partIndex == 0 || (m_partIndex>=7 && m_partIndex<=10)) && m_character->m_collision_applyToSpine);
          applyToPart = applyToPart || ((m_partIndex == 1 || m_partIndex == 4) && m_character->m_collision_applyToThighs);
          applyToPart = applyToPart || ((m_partIndex == 11 || m_partIndex == 15) && m_character->m_collision_applyToClavicles);
          applyToPart = applyToPart || ((m_partIndex == 12 || m_partIndex == 16) && m_character->m_collision_applyToUpperArms);
          if (applyToPart)
          {
            rage::Vector3 velocity;
            rage::Vector3 position(pos);
            m_character->getVelocityOnInstance(collideeInst->GetLevelIndex(),
              position,
              &velocity);
            velocity -= getLinearVelocity(&position);
            velocity.ClampMag(0.0f, m_character->m_collision_maxVelocity);
            rage::Vector3 oppositePos = 2.0f*getPosition() - position;
            applyForce(m_character->m_collision_spin*velocity, &position);
            applyForce(-m_character->m_collision_spin*velocity, &oppositePos);
            //Allow the feet to slip round if car collisions are being enhanced 
          }//applyToPart
        }//spin
      }//vehicle

    }
  }

#if ART_ENABLE_BSPY

  void NmRsGenericPart::sendDescriptor()
  {
    GPartDescriptorPacket pdp;

    pdp.m_partIndex = (bs_uint16)m_partIndex;
    pdp.m_nameToken = m_nameToken;

    // extract bound data
    const rage::phBound* bound = getBound();
    phBoundToShapePrimitive(bound, pdp.m_shape);

    if (m_wrapper)
      pdp.m_mass = m_wrapper->getArticulatedBody()->GetMass(m_partIndex).Getf();
    else
      pdp.m_mass = 1.0f;

    rage::Matrix34 curMtx;
    getMatrix(curMtx);

    pdp.m_initialTM = bSpyMat34fromMatrix34(curMtx);
    pdp.m_toBoneTM = bSpyMat34fromMatrix34(m_toBoneMatrix);

    bspySendPacket(pdp);
  }

  void NmRsGenericPart::sendUpdate()
  {
    if (m_character->getBSpyID() == INVALID_AGENTID)
      return;
    GPartUpdatePacket pup;

    // identity in character
    pup.m_partIndex = (bs_uint8)m_partIndex;

    // physics properties
    pup.m_linVel = bSpyVec3fromVector3( getLinearVelocity() );
    pup.m_angVel = bSpyVec3fromVector3( getAngularVelocity() );

    // current matrix in world
    rage::Matrix34 curMtx;
    getMatrix(curMtx);
    pup.m_currentTM = bSpyMat34fromMatrix34(curMtx);

    pup.m_fictionMultiplier = m_fictionMultiplier;
    pup.m_elasticityMultiplier = m_elasticityMultiplier;

    // set collision bits
    pup.m_collisionState = 0;
    if (state.m_collisionEnabled)
      pup.m_collisionState |= GPartUpdatePacket::csf_enabled;
    if (state.m_collided)
      pup.m_collisionState |= GPartUpdatePacket::csf_collided;
    if (state.m_collidedOtherCharacters)
      pup.m_collisionState |= GPartUpdatePacket::csf_collidedOther;
    if (state.m_collidedOwnCharacter)
      pup.m_collisionState |= GPartUpdatePacket::csf_collidedSelf;
    if (state.m_previousCollidedEnvironment)
      pup.m_collisionState |= GPartUpdatePacket::csf_collidedEnvironment;
    if (state.m_previousCollided)
      pup.m_collisionState |= GPartUpdatePacket::csf_prevCollided;
    if (state.m_previousCollidedOtherCharacters)
      pup.m_collisionState |= GPartUpdatePacket::csf_prevCollidedOther;
    if (state.m_previousCollidedOwnCharacter)
      pup.m_collisionState |= GPartUpdatePacket::csf_prevCollidedSelf;
    if (state.m_previousCollidedEnvironment)
      pup.m_collisionState |= GPartUpdatePacket::csf_prevCollidedEnvironment;


// HDD removed, couldn't see it being used in bSpy
//     rage::phArticulatedBodyPart* bodyPart = (rage::phArticulatedBodyPart*)m_ptr;
//     rage::Matrix34 mtm = MAT34V_TO_MATRIX34(bodyPart->GetMatrix());
//     mtm.Transpose();
//     pup.m_bodyMatrix = bSpyMat34fromMatrix34(mtm);

    bspySendPacket(pup);
  }

#endif // ART_ENABLE_BSPY
}
