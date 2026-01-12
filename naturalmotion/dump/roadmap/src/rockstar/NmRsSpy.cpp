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

#if ART_ENABLE_BSPY

#include "NmRsCharacter.h"
#include "NmRsEngine.h"
#include "NmRsCBU_TaskManager.h"

#include "NmRsSpy.h"
#include "bspy/bSpyCommonPackets.h"

#include "fragmentnm/manager.h"

using namespace bSpy;

namespace ART
{
  NmRsSpy::NmRsSpy(ART::MemoryManager* services, NmRsEngine* engine) : bSpyServer(services),
    m_staticBoundTxTable(0),
    m_engine(engine)
  {
  }

  NmRsSpy::~NmRsSpy()
  {
    if (m_staticBoundTxTable)
      delete m_staticBoundTxTable;
  }

  void NmRsSpy::beginStep(float deltaTime)
  {
    bSpyServer::beginStep(deltaTime);

    bSpy::BeginStepPacket bsp(deltaTime);
    bspySendPacket(bsp);
    sendAgentActiveState();
  }

  //send what characters are inserted
  void NmRsSpy::sendAgentActiveState()
  {
    if (isClientConnected())
    {
#if NM_ANIM_MATRICES
//       //Activate an agent in bSpy as the animation has been sent in when no NM character exists
//       for (unsigned int characterTypeInBSpy = 0; characterTypeInBSpy<NUM_ASSETS; characterTypeInBSpy++)
//       {
//   [HDD]      unsigned int bSpyAnimationCharacterID = MAX_BSPY_AGENTS - characterTypeInBSpy - 1;
//         if (m_engine->leadInAnimationSentIn(characterTypeInBSpy))
//           agentActiveState |= (1 << bSpyAnimationCharacterID);
//       }
#endif// NM_ANIM_MATRICES

      AgentState asblock;
      m_engine->getAgentStateBlock(asblock);
      bSpy::AgentStatePacket asp(asblock);
      sendPacket(asp);
    }
  }

  void NmRsSpy::sendInfo()
  {
    if (isClientConnected())
    {
      unsigned int i, maxCharacters = MAX_AGENTS;
      FastAssert(maxCharacters < 32);
      // tell bSpy about the current ITMs set
      for (i=0; i<maxCharacters; i++)
      {
        NmRsCharacter* character = m_engine->getCharacterFromAgentID((ART::AgentID)i);
        if (character && character->isInsertedInScene() && character->getBSpyID() != INVALID_AGENTID)
        {
          bSpy::SelectAgentPacket sap((bs_int16)character->getBSpyID());
          bspySendPacket(sap);

          sendIncomingTransforms(character);
#if NM_ANIM_MATRICES && NM_TESTING_ANIM_MATRICES && 0
          //send character 0's current itms to engine's animation matrices - FOR TESTING ONLY
          if (i==0)
          {
            //sendIncomingTransformsToAnimation
            int incomingComponentCount = 0;
            NMutils::NMMatrix4 *itPtr = 0;
            ART::IncomingTransformStatus itmStatusFlags = ART::kITSNone;
            character->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, ART::kITSourceCurrent);
            if (itPtr != 0)
            {
              m_engine->setLeadInAnimationTMs(itPtr);
              m_engine->setLeadInAnimationSentIn(character->getAssetID(), true);
            }
          }
#endif
        }
      }

      // send character updates
      for (i=0; i<maxCharacters; i++)
      {
        NmRsCharacter* character = m_engine->getCharacterFromAgentID((ART::AgentID)i);
        if (character && character->isInsertedInScene() && character->getBSpyID() != INVALID_AGENTID)
        {
          sendUpdate(character);
        }
      }      
    }
  }      

  void NmRsSpy::sendUpdate(ART::NmRsCharacter* character)
  {
    if (!m_assetDescriptorSent[character->getAssetID()])
    {
      character->sendDescriptor();

#if NM_ANIM_MATRICES
      // [HDD]       bSpy::MapAgentToAssetPacket mppAnim((bs_int16)(MAX_BSPY_AGENTS - characterTypeInBSpy - 1), (bs_int16)characterTypeInBSpy, true);
      //       bspySendPacket(mppAnim);
#endif

      m_assetDescriptorSent[character->getAssetID()] = true;
    }

    character->sendUpdate();
  }

  void NmRsSpy::endStep(float deltaTime)
  {
    if (isClientConnected())
    {
      // mark as the end of transmission for this frame
      EndStepPacket esp;
      sendPacket(esp);
    }

    bSpyServer::endStep(deltaTime);
  }

  /**
  * when the bSpy application connects we must inform it about the various
  * character types present in the system
  */
  void NmRsSpy::onNewConnection()
  {
    if (m_staticBoundTxTable == 0)
    {
      // build the transmission lookup tables on-demand
      m_dynBoundTxTable = rage_new DynamicBoundTxTable(2048, m_artMemoryManager);
      m_staticBoundTxTable = rage_new StaticBoundTxTable(64, m_artMemoryManager);
    }

    bool gameIsYUp = false;
    if (m_engine->getUpVector().GetY() > FLT_EPSILON)
      gameIsYUp = true;

    // setup our identity packet
    //"Rs_z" means don't apply the 90deg rotation to the root(e.g. for R*N)
    //"Rs_r" means do apply the rotation(e.g. for MP3)
    //"Rs_Z" means do apply the rotation for character assetID's >=2 (old way for both MP3 and R*N)
    bSpy::IdentityPacket cp(
      gameIsYUp?"Rs_Y":"Rs_z",
      CODE_REVISION,
      RAGE_RELEASE, RAGE_MAJOR_VERSION, RAGE_MINOR_VERSION,
      getTokenForString(__DATE__));

    // begin transmission
    sendInitialPayloads(cp);

    //send Code versions to the zero frame
    bspyLogf_ZF(info, L"NMCode Rev: %i", CODE_REVISION);
    bspyLogf_ZF(info, L"RAGE %i %i-%i", RAGE_RELEASE, RAGE_MAJOR_VERSION, RAGE_MINOR_VERSION);

    m_dynBoundTxTable->clear();
    m_staticBoundTxTable->clear();

    unsigned int i;
    for (i=0; i<NUM_ASSETS; i++)
    {
      m_assetDescriptorSent[i] = false;
    }

    m_engine->getTaskManager()->sendDescriptor(*this);
  }

  bSpyVec3 bSpyVec3fromVector3(const rage::Vector3& vec)
  {
    bSpyVec3 out;

    out.v[0] = vec.GetX();
    out.v[1] = vec.GetY();
    out.v[2] = vec.GetZ();

    return out;
  }

  bSpyMat34 bSpyMat34fromMatrix34(const rage::Matrix34& tm)
  {
    bSpyMat34 out;
    memset(&out, 0, sizeof(bSpyMat34));

    for (int i=0; i<3; i++)
      out.m_row[0].v[i] = tm.a[i];
    for (int i=0; i<3; i++)
      out.m_row[1].v[i] = tm.b[i];
    for (int i=0; i<3; i++)
      out.m_row[2].v[i] = tm.c[i];
    for (int i=0; i<3; i++)
      out.m_row[3].v[i] = tm.d[i];

    return out;
  }

  void phBoundToShapePrimitive(const rage::phBound* bound, bSpy::bSpyShapePrimitive& shape)
  {
    switch (bound->GetType())
    {
    case rage::phBound::SPHERE:
      {
        const rage::phBoundSphere *sph = reinterpret_cast<const rage::phBoundSphere*>(bound);
        shape.m_type = bSpyShapePrimitive::eShapeSphere;
        shape.m_data.sphere.m_radius = sph->GetRadius();
      }
      break;

    case rage::phBound::CAPSULE:
      {
        const rage::phBoundCapsule *cap = reinterpret_cast<const rage::phBoundCapsule*>(bound);
        shape.m_type = bSpyShapePrimitive::eShapeCapsuleVertical;
        shape.m_data.capsule.m_length = cap->GetLength();
        shape.m_data.capsule.m_radius = cap->GetRadius();
      }
      break;

    case rage::phBound::CYLINDER:
      {
        const rage::phBoundCylinder *cyl = reinterpret_cast<const rage::phBoundCylinder*>(bound);
        shape.m_type = bSpyShapePrimitive::eShapeCylinderVertical;
        shape.m_data.cylinder.m_length = cyl->GetHalfHeight() * 2.0f;
        shape.m_data.cylinder.m_radius = cyl->GetRadius();
      }
      break;

    case rage::phBound::BOX:
      {
        const rage::phBoundBox *box = reinterpret_cast<const rage::phBoundBox*>(bound);
        shape.m_type = bSpyShapePrimitive::eShapeBox;
        shape.m_data.box.m_dims[0] = box->GetBoxSize().GetXf();
        shape.m_data.box.m_dims[1] = box->GetBoxSize().GetYf();
        shape.m_data.box.m_dims[2] = box->GetBoxSize().GetZf();
      }
      break;

    case rage::phBound::GEOMETRY:
    case rage::phBound::BVH:
      {
        // fallback
        shape.m_type = bSpyShapePrimitive::eShapeBox;
        shape.m_data.box.m_dims[0] = 0.1f;
        shape.m_data.box.m_dims[1] = 0.1f;
        shape.m_data.box.m_dims[2] = 0.1f;
      }
      break;

    default:
      Assert(0);
      break;
    }

  }
}


#endif // ART_ENABLE_BSPY

