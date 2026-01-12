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
#include "NmRsGenericPart.h"
#include "NmRsCBU_Buoyancy.h"

namespace ART
{
  NmRsCBUBuoyancy::NmRsCBUBuoyancy(ART::MemoryManager* services) : CBUTaskBase(services, bvid_buoyancy)
  {
    initialiseCustomVariables();
  }

  NmRsCBUBuoyancy::~NmRsCBUBuoyancy()
  {
  }

  void NmRsCBUBuoyancy::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;

    m_timeSubmerged = 0.f;
  }

  void NmRsCBUBuoyancy::onActivate()
  {
    Assert(m_character);
    m_timeSubmerged = 0.f;
  }

  void NmRsCBUBuoyancy::onDeactivate()
  {
    Assert(m_character);
    initialiseCustomVariables();
  }

  CBUTaskReturn NmRsCBUBuoyancy::onTick(float timeStep)
  {
    Assert(m_character);

#if ART_ENABLE_BSPY
    bspyLogf(info, L"NmRsCBUBuoyancy::tick")
#endif

      rage::Vector3 point = m_parameters.surfacePoint;
    rage::Vector3 normal = m_parameters.surfaceNormal;
    normal.Normalize(); // just in case people can't read

    // move point to be above character (for visual niceness)
    rage::Vector3 temp(point);
    temp.Subtract(m_body->getSpine()->getSpine3Part()->getPosition());
    temp.Subtract(normal * normal.Dot(temp));  // remove component of difference along normal
    point.Subtract(temp);

#if ART_ENABLE_BSPY
    // quick and dirty drawing of plane. blue, for water.
    m_character->bspyDrawCircle(point, normal, 2.f, rage::Vector3(0,0,1), 4);
    // draw normal for clarity.
    m_character->bspyDrawLine(point, point + normal, rage::Vector3(0,0,1));
#endif

    float buoyancy = m_parameters.buoyancy;
    float damping = m_parameters.damping;
    const rage::phBound* bound; 

    // flag to track whether chest is in water or not.
    bool submerged = false;

    // per part
    for(int i = 0; i < m_character->getNumberOfParts(); ++i)
    {
      NmRsGenericPart* part = m_character->getGenericPartByIndex(i);
      bound = m_character->getBoundByComponentIdx(i);
      Assert(part);
      Assert(bound);

      // simple radius estimate for displacement calculation.
      float radius = bound->GetRadiusAroundCentroid(); // not a good default...
      if(bound->GetType() == rage::phBound::CAPSULE)
      {
        rage::phBoundCapsule* boundCapsule = (rage::phBoundCapsule*)bound;
        radius = boundCapsule->GetRadius();
      }
      else if(bound->GetType() == rage::phBound::BOX)
      {
        rage::phBoundBox* boundBox = (rage::phBoundBox*)bound;
        rage::Vector3 boxSize(VEC3V_TO_VECTOR3(boundBox->GetBoxSize()));
        radius = rage::Min(boxSize.x, boxSize.y, boxSize.z);
      }

      // get depth of part center and add radius estimate.
      rage::Vector3 position = part->getPosition();
      position.SubtractScaled(normal, radius);
      rage::Vector3 error = position - point;
      float depth = -error.Dot(normal);
      error.SetScaled(normal, -depth);

      if(depth > 0.f)
      {
#if ART_ENABLE_BSPY
        m_character->bspyDrawPoint(position, 0.025f, rage::Vector3(1,1,0));
        m_character->bspyDrawLine(position, position + error, rage::Vector3(1,1,0));
#endif

        // displacement estimate. treats all bounds as if they are cubes
        // oriented to surface normal (eg linear response). may want to
        // change this but seems to work well for now.
        depth = rage::Clamp(depth, 0.f, 2.f * radius);
        float displacement = bound->GetVolume() * depth / (2.f * radius);

        // density of water. normally 1000. much lower due to inaccurate
        // character density and exaggerated displacement due to overlapping
        // bounds.
        const float density = 250.f;

        // small hack to simulate chest being less dense (due to air space in lungs, etc).
        buoyancy = m_parameters.buoyancy;
        if(m_character->isPartInMask(bvmask_Spine3 | bvmask_Spine2, i))
          buoyancy *= m_parameters.chestBuoyancy;

        // go Archimedes, go!
        rage::Vector3 force(normal);
        force.Scale(buoyancy * displacement * density * 9.81f);
        part->applyImpulse(force * timeStep, part->getPosition());

        // damp part movement when submerged.
        rage::Vector3 dError(part->getLinearVelocity());
        force.Set(-damping * dError);
        part->applyImpulse(force * timeStep, part->getPosition());

        // help the character become upright.
        if(m_parameters.righting && part->getPartIndex() == gtaSpine3)
        {
          // measure horizontalness.  character should try
          // to right himself more when he's horizontal.
          // clamp into 0-1 scaler.
          rage::Matrix34 chestTm;
          part->getMatrix(chestTm);
          float horizontalness = (1.f - rage::Abs(normal.Dot(chestTm.a)));
          // compress somewhat to create a deadzone.
          const float deadzone = 0.5f;
          horizontalness *= (1.f + deadzone);
          horizontalness -= deadzone;
          horizontalness = rage::Clamp(horizontalness, 0.f, 1.f);
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "righting", horizontalness);
          // draw chest forward.
          m_character->bspyDrawCoordinateFrame(0.1f, chestTm);
#endif

          // track how long we've been in the water.
          // clamp to 0-1 scaler. character should try
          // to right himself more the longer he's in
          // the water.
          submerged = true;
          m_timeSubmerged += timeStep;
          const float maxTimeSubmerged = 2.f;
          float timeSubmerged = rage::Clamp(m_timeSubmerged, 0.f, maxTimeSubmerged);
          timeSubmerged /= maxTimeSubmerged;
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "righting", timeSubmerged);
#endif

          rage::Vector3 normalLocal;
          chestTm.UnTransform3x3(normal, temp);
          temp.x = 0.f;
          chestTm.Transform3x3(temp);
          temp.Normalize();

          float angle = rage::AcosfSafe(temp.Dot(-chestTm.c));
          if(temp.Dot(chestTm.b) < 0.f)
            angle = -angle;
#if ART_ENABLE_BSPY
          bspyScratchpad(m_character->getBSpyID(), "buoyancy.righting.", angle);
          m_character->bspyDrawLine(chestTm.d, chestTm.d + temp, rage::Vector3(0,1,0));
#endif

          part->applyTorqueImpulse(chestTm.a * angle * m_parameters.rightingStrength * horizontalness * timeSubmerged);
        }
      }
    }

    if(!submerged)
      m_timeSubmerged = 0.f;
    return eCBUTaskComplete;
  }


#if ART_ENABLE_BSPY
  void NmRsCBUBuoyancy::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.buoyancy, true);
    bspyTaskVar(m_parameters.chestBuoyancy, true);
    bspyTaskVar(m_parameters.damping, true);
    bspyTaskVar(m_parameters.surfacePoint, true);
    bspyTaskVar(m_parameters.surfaceNormal, true);
    bspyTaskVar(m_parameters.righting, true);
    bspyTaskVar(m_parameters.rightingStrength, true);
    bspyTaskVar(m_parameters.rightingTime, true);
  }
#endif // ART_ENABLE_BSPY
}

