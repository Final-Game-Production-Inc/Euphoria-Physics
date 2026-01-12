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

#ifndef NM_RS_SPY_PACKETS_H
#define NM_RS_SPY_PACKETS_H

#include "NMutils/NMTypes.h"

#ifdef BSPY_APP
#include "bSpyStringCache.h"    // used to resolve string tokens for debugDescribe
#endif // BSPY_APP

#if ART_ENABLE_BSPY

#include "bSpyCommonPackets.h"

using namespace bSpy;

#pragma pack(4)

namespace rage
{
  class Matrix34;
  class Vector3;
}

namespace ART
{
  enum 
  {
    // client-specific packet IDs start at bspy_pk_user
    bspy_pk_nmrs        = bspy_pk_user,

#define NMRS_BSPY_PACKET(pkn)   bspy_pk_nmrs_##pkn,
#include "NmRsSpyPackets.inl"
#undef NMRS_BSPY_PACKET

    bspy_pk_nmrs_top
  };

#define NMRS_BSPY_HEADER(pkn) \
  hdr.m_id = bspy_pk_nmrs_##pkn; \
  hdr.m_length = sizeof(pkn##Packet);

  // ditto the above, but toggles the magic byte to reflect
  // that this packet should be stored in the zero frame
#define NMRS_BSPY_HEADER_ZF(pkn) \
  hdr.m_magicB = NM_BSPY_PKT_MAGIC_ZF; \
  hdr.m_id = bspy_pk_nmrs_##pkn; \
  hdr.m_length = sizeof(pkn##Packet);

  /**
   * describes the basics of a character on a per-asset basis;
   * this is then used to generate an agent storage structure on a per-agent basis.
   */
  struct AssetDescriptorPacket : public PacketBase
  {
    inline AssetDescriptorPacket() : PacketBase(),
      m_assetID(-1),
      m_bodyIdent(-1),
      m_genericPartCount(0),
      m_1dofCount(0),
      m_3dofCount(0)
    {
      NMRS_BSPY_HEADER_ZF(AssetDescriptor)
      m_identity[0] = '\0';
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_assetID);
      bSpy::utils::endianSwap(m_bodyIdent);

      bSpy::utils::endianSwap(m_genericPartCount);
      bSpy::utils::endianSwap(m_1dofCount);
      bSpy::utils::endianSwap(m_3dofCount);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"ident=%S, assetID=%i, body=%i, parts=%i, 1dofs=%i, 3dofs=%i", m_identity, m_assetID, m_bodyIdent, m_genericPartCount, m_1dofCount, m_3dofCount);
    }
#endif // BSPY_APP

    char          m_identity[8];

    bs_int16      m_assetID,
                  m_bodyIdent;

    bs_int32      m_genericPartCount,
                  m_1dofCount,
                  m_3dofCount;
  };

  /**
  * character update packet
  * defines storage space for a character, mapped by CharacterID
  */
  struct CharacterUpdatePacket : public PacketBase
  {
    inline CharacterUpdatePacket() : PacketBase()
    {
      NMRS_BSPY_HEADER(CharacterUpdate)
    }

    inline void endianSwap()
    {
      d.m_gUp.endianSwap();
      d.m_COMvel.endianSwap();
      d.m_COMangvel.endianSwap();
      d.m_COMtm.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      sizeof(payload);
    }
#endif // BSPY_APP

    struct data
    {
      bSpyVec3    m_gUp,
                  m_COMvel,
                  m_COMangvel;
      bSpyMat34   m_COMtm;
    } d;
  };

  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct CharacterUpdateVarPacket : public PacketBase
  {
    inline CharacterUpdateVarPacket(bSpyValueUnion::Type paramType, bool /*isParameter*/) : PacketBase()
    {
      NMRS_BSPY_HEADER(CharacterUpdateVar)

      m_data.m_type = (bs_uint8)paramType;
    }

    inline void endianSwap()
    {
      m_data.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      m_data.debugDescribe(payload);
    }
#endif // BSPY_APP


    bSpyValueUnion      m_data;
  };


  /**
  * generic part descriptor
  */
  struct GPartDescriptorPacket : public PacketBase
  {
    inline GPartDescriptorPacket() : PacketBase(),
      m_partIndex(0),
      m_nameToken(0),
      m_mass(0)
    {
      NMRS_BSPY_HEADER_ZF(GPartDescriptor)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_partIndex);
      bSpy::utils::endianSwap(m_nameToken);

      m_shape.endianSwap();

      bSpy::utils::endianSwap(m_mass);

      m_initialTM.endianSwap();
      m_toBoneTM.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"part=%i, name=\"%s\" ", 
        m_partIndex, 
        bSpyStringCache::getWStringForToken(m_nameToken));

      m_shape.debugDescribeAppend(payload);
    }
#endif // BSPY_APP

    bs_uint16           m_partIndex;

    bSpyStringToken     m_nameToken;

    bSpyShapePrimitive  m_shape;

    float               m_mass;

    bSpyMat34           m_initialTM;
    bSpyMat34           m_toBoneTM;
  };


  /**
  * generic part update
  */
  struct GPartUpdatePacket : public PacketBase
  {
    inline GPartUpdatePacket() : PacketBase(),
      m_partIndex(0)
    {
      NMRS_BSPY_HEADER(GPartUpdate)
    }

    inline void endianSwap()
    {
      m_currentTM.endianSwap();
      m_linVel.endianSwap();
      m_angVel.endianSwap();

      bSpy::utils::endianSwap(m_fictionMultiplier);
      bSpy::utils::endianSwap(m_elasticityMultiplier);

      bSpy::utils::endianSwap(m_collisionState);

// HDD removed, couldn't see it being used in bSpy
//       d.m_bodyMatrix.endianSwap();
    }

#ifdef BSPY_APP
    // used to get names for each of the collision state bits
    static const wchar_t* getCollisionStateName(unsigned int index)
    {
      const wchar_t* names[csf_numBitsUsed] = 
      {
        L"Enabled",
        L"Collided",
        L"CollidedOther",
        L"CollidedSelf",
        L"CollidedEnvironment",
        L"PreviouslyCollided",
        L"PreviouslyCollidedOther",
        L"PreviouslyCollidedSelf",
        L"PreviouslyCollidedEnvironment",
      };

      if (index >= csf_numBitsUsed)
        return L"Unknown";
      return names[index];
    }

    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"part=%02i, CSF: %s%s%s", 
        m_partIndex, 
        ((m_collisionState & csf_enabled)==csf_enabled)?L"enabled":L"",
        ((m_collisionState & (csf_collided | csf_collidedOther | csf_collidedSelf))!=0)?L", collided":L"",
        ((m_collisionState & (csf_prevCollided | csf_prevCollidedOther | csf_prevCollidedSelf))!=0)?L", prevCollided":L""
        );
    }
#endif // BSPY_APP

    bSpyMat34         m_currentTM;

    bSpyVec3          m_linVel;
    bSpyVec3          m_angVel;

    float             m_fictionMultiplier;
    float             m_elasticityMultiplier;

    enum CollisionStateFlags
    {
      csf_enabled                 = 1 << 0,
      csf_collided                = 1 << 1,
      csf_collidedOther           = 1 << 2,
      csf_collidedSelf            = 1 << 3,
      csf_collidedEnvironment     = 1 << 4,
      csf_prevCollided            = 1 << 5,
      csf_prevCollidedOther       = 1 << 6,
      csf_prevCollidedSelf        = 1 << 7,
      csf_prevCollidedEnvironment = 1 << 8,

      csf_numBitsUsed = 9
    };
    bs_uint16         m_collisionState;

    bs_uint8          m_partIndex;
  };


  /**
  * 
  */
  struct Effector1DofDescriptorPacket : public PacketBase
  {
    inline Effector1DofDescriptorPacket(bs_uint8 effectorJointIndex, bs_uint8 effectorTypeIndex) : PacketBase()
    {
      NMRS_BSPY_HEADER_ZF(Effector1DofDescriptor)

        d.m_effectorJointIndex = effectorJointIndex;
      d.m_effectorTypeIndex = effectorTypeIndex;
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(d.m_nameToken);

      bSpy::utils::endianSwap(d.m_minAngle);
      bSpy::utils::endianSwap(d.m_maxAngle);

      d.m_positionParent.endianSwap();
      d.m_positionChild.endianSwap();

      d.m_orientParent.endianSwap();
      d.m_orientChild.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"jointIndex=%i", d.m_effectorJointIndex);
    }
#endif // BSPY_APP

    struct data
    {
      bSpyStringToken   m_nameToken;

      bs_uint8          m_effectorJointIndex;
      bs_uint8          m_effectorTypeIndex;

      bs_uint8          m_parentIndex;
      bs_uint8          m_childIndex;

      float             m_minAngle,
                        m_maxAngle;

      bSpyVec3          m_positionParent;
      bSpyVec3          m_positionChild;

      bSpyMat34         m_orientParent;
      bSpyMat34         m_orientChild;

    } d;
  };    


  /**
  * 1-dof effector update packet
  */
  struct Effector1DofUpdatePacket : public PacketBase
  {
    inline Effector1DofUpdatePacket(bs_uint8 effectorJointIndex, bs_uint8 effectorTypeIndex) : PacketBase(),
      m_effectorJointIndex(effectorJointIndex),
      m_effectorTypeIndex(effectorTypeIndex)
    {
      NMRS_BSPY_HEADER(Effector1DofUpdate)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(d.m_desired);
      bSpy::utils::endianSwap(d.m_actual);
      bSpy::utils::endianSwap(d.m_actualVel);
      bSpy::utils::endianSwap(d.m_zeroPose);

      bSpy::utils::endianSwap(d.m_stiffness);
      bSpy::utils::endianSwap(d.m_strength);
      bSpy::utils::endianSwap(d.m_damping);

      bSpy::utils::endianSwap(d.m_stiffnessScale);
      bSpy::utils::endianSwap(d.m_strengthScale);
      bSpy::utils::endianSwap(d.m_dampingScale);
      bSpy::utils::endianSwap(d.m_injury);

      bSpy::utils::endianSwap(d.m_desiredAngleSetBy);
      bSpy::utils::endianSwap(d.m_muscleStiffnessSetBy);
      bSpy::utils::endianSwap(d.m_muscleStrengthSetBy);
      bSpy::utils::endianSwap(d.m_muscleDampingSetBy);

      bSpy::utils::endianSwap(d.m_desiredAngleSetByFrame);
      bSpy::utils::endianSwap(d.m_muscleStiffnessSetByFrame);
      bSpy::utils::endianSwap(d.m_muscleStrengthSetByFrame);
      bSpy::utils::endianSwap(d.m_muscleDampingSetByFrame);

      bSpy::utils::endianSwap(d.m_opposeGravity);

      for (int i=0; i<4; i++)
        bSpy::utils::endianSwap(d.m_itmDriveQuat[i]);

      d.m_matrix1.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"jointIndex=%i", m_effectorJointIndex);
    }
#endif // BSPY_APP

    bs_uint8        m_effectorJointIndex;
    bs_uint8        m_effectorTypeIndex;

    struct data
    {
      float           m_desired;
      float           m_actual,
                      m_actualVel,
                      m_zeroPose;

      float           m_stiffness,
                      m_strength,
                      m_damping;
      float           m_stiffnessScale,
                      m_strengthScale,
                      m_dampingScale,
                      m_injury;

      bSpyStringToken m_desiredAngleSetBy;
      bSpyStringToken m_muscleStiffnessSetBy;
      bSpyStringToken m_muscleStrengthSetBy;
      bSpyStringToken m_muscleDampingSetBy;

      int             m_desiredAngleSetByFrame;
      int             m_muscleStiffnessSetByFrame;
      int             m_muscleStrengthSetByFrame;
      int             m_muscleDampingSetByFrame;

      float           m_opposeGravity;

      float           m_itmDriveQuat[4];
      bSpyMat34       m_matrix1;

    } d;
  };


  /**
  * 
  */
  struct Effector3DofDescriptorPacket : public PacketBase
  {
    inline Effector3DofDescriptorPacket(bs_uint8 effectorJointIndex, bs_uint8 effectorTypeIndex) : PacketBase()
    {
      NMRS_BSPY_HEADER_ZF(Effector3DofDescriptor)

        d.m_effectorJointIndex = effectorJointIndex;
      d.m_effectorTypeIndex = effectorTypeIndex;
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(d.m_nameToken);

      d.m_minAngles.endianSwap();
      d.m_maxAngles.endianSwap();

      d.m_positionParent.endianSwap();
      d.m_positionChild.endianSwap();

      d.m_orientParent.endianSwap();
      d.m_orientChild.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"jointIndex=%i", d.m_effectorJointIndex);
    }
#endif // BSPY_APP

    struct data
    {
      bSpyStringToken   m_nameToken;

      bs_uint8          m_effectorJointIndex;
      bs_uint8          m_effectorTypeIndex;
      bs_uint8          m_parentIndex;
      bs_uint8          m_childIndex;

      bool              m_reverseMotor[3];  // twist  |  lean-1  |  lean-2

      bSpyVec3          m_minAngles;
      bSpyVec3          m_maxAngles;

      bSpyVec3          m_positionParent;
      bSpyVec3          m_positionChild;

      bSpyMat34         m_orientParent;
      bSpyMat34         m_orientChild;

    } d;
  };    


  /**
  * 3-dof effector update packet
  */
  struct Effector3DofUpdatePacket : public PacketBase
  {
    inline Effector3DofUpdatePacket(bs_uint8 effectorJointIndex, bs_uint8 effectorTypeIndex) : PacketBase(),
      m_effectorJointIndex(effectorJointIndex),
      m_effectorTypeIndex(effectorTypeIndex)
    {
      NMRS_BSPY_HEADER(Effector3DofUpdate)
    }

    inline void endianSwap()
    {
      d.m_desired.endianSwap();
      d.m_actual.endianSwap();
      d.m_actualVel.endianSwap();
      d.m_zeroPose.endianSwap();

      bSpy::utils::endianSwap(d.m_stiffness);
      bSpy::utils::endianSwap(d.m_strength);
      bSpy::utils::endianSwap(d.m_damping);

      bSpy::utils::endianSwap(d.m_stiffnessScale);
      bSpy::utils::endianSwap(d.m_strengthScale);
      bSpy::utils::endianSwap(d.m_dampingScale);
      bSpy::utils::endianSwap(d.m_injury);

      bSpy::utils::endianSwap(d.m_desiredTwistSetBy);
      bSpy::utils::endianSwap(d.m_desiredLean1SetBy);
      bSpy::utils::endianSwap(d.m_desiredLean2SetBy);
      bSpy::utils::endianSwap(d.m_muscleStiffnessSetBy);
      bSpy::utils::endianSwap(d.m_muscleStrengthSetBy);
      bSpy::utils::endianSwap(d.m_muscleDampingSetBy);

      bSpy::utils::endianSwap(d.m_desiredTwistSetByFrame);
      bSpy::utils::endianSwap(d.m_desiredLean1SetByFrame);
      bSpy::utils::endianSwap(d.m_desiredLean2SetByFrame);
      bSpy::utils::endianSwap(d.m_muscleStiffnessSetByFrame);
      bSpy::utils::endianSwap(d.m_muscleStrengthSetByFrame);
      bSpy::utils::endianSwap(d.m_muscleDampingSetByFrame);

      bSpy::utils::endianSwap(d.m_opposeGravity);

      for (int i=0; i<4; i++)
        bSpy::utils::endianSwap(d.m_itmDriveQuat[i]);

      d.m_matrix1.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"jointIndex=%i", m_effectorJointIndex);
    }
#endif // BSPY_APP

    bs_uint8        m_effectorJointIndex;
    bs_uint8        m_effectorTypeIndex;

    struct data
    {
      bSpyVec3        m_desired;
      bSpyVec3        m_actual,
                      m_actualVel,
                      m_zeroPose;

      float           m_stiffness,
                      m_strength,
                      m_damping;
      float           m_stiffnessScale,
                      m_strengthScale,
                      m_dampingScale,
                      m_injury;

      bSpyStringToken m_desiredTwistSetBy;
      bSpyStringToken m_desiredLean1SetBy;
      bSpyStringToken m_desiredLean2SetBy;
      bSpyStringToken m_muscleStiffnessSetBy;
      bSpyStringToken m_muscleStrengthSetBy;
      bSpyStringToken m_muscleDampingSetBy;

      int           m_desiredTwistSetByFrame;
      int           m_desiredLean1SetByFrame;
      int           m_desiredLean2SetByFrame;
      int           m_muscleStiffnessSetByFrame;
      int           m_muscleStrengthSetByFrame;
      int           m_muscleDampingSetByFrame;

      float           m_opposeGravity;

      float           m_itmDriveQuat[4];
      bSpyMat34       m_matrix1;

    } d;
  };

  /**
  * 
  */
  struct EffectorModifyLimitsPacket : public PacketBase
  {
    inline EffectorModifyLimitsPacket(bs_int16 agid, bs_uint8 effectorJointIndex) : PacketBase(),
      m_effectorJointIndex(effectorJointIndex),
      m_agentID(agid),
      m_cacheUpdateID(-1)
    {
      NMRS_BSPY_HEADER(EffectorModifyLimits)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_agentID);
      bSpy::utils::endianSwap(m_cacheUpdateID);
      m_minAngles.endianSwap();
      m_maxAngles.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"jointIndex=%i, agent=%i, updateID=%i", m_effectorJointIndex, m_agentID, m_cacheUpdateID);
    }
#endif // BSPY_APP

    bs_uint8          m_effectorJointIndex;
    bs_int16          m_agentID;
    bs_int32          m_cacheUpdateID;

    bSpyVec3          m_minAngles;
    bSpyVec3          m_maxAngles;
  };    
}

#pragma pack()


#endif // ART_ENABLE_BSPY

#endif // NM_RS_SPY_PACKETS_H

