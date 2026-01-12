/*
 * Copyright (c) 2007-2008 NaturalMotion Ltd. All rights reserved. 
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

#ifndef NM_BSPY_COMMON_PACKETS_H
#define NM_BSPY_COMMON_PACKETS_H

#include "bSpyPacket.h"

// hack this in here, as we don't want to include amath.h inside bSpy just for Min
// they are undef'd later
#define v_max(a,b)    (((a) > (b)) ? (a) : (b))
#define v_min(a,b)    (((a) < (b)) ? (a) : (b))

#ifdef BSPY_APP
#include "bSpyStringCache.h"    // used to resolve string tokens for debugDescribe
#endif // BSPY_APP

namespace bSpy
{
  #pragma pack(4)

  /**
   * -----------------------------------------------------------------------------
   * generic 3-float vector class
   */
  struct bSpyVec3
  {
    float   v[3];

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(v[0]);
      bSpy::utils::endianSwap(v[1]);
      bSpy::utils::endianSwap(v[2]);
    }
  };


  /**
   * -----------------------------------------------------------------------------
   * full 3x4 matrix representation
   */
  struct bSpyMat34
  {
    bSpyVec3    m_row[4];

    inline void endianSwap()
    {
      m_row[0].endianSwap();
      m_row[1].endianSwap();
      m_row[2].endianSwap();
      m_row[3].endianSwap();
    }
  };


  /**
   * -----------------------------------------------------------------------------
   * variant type for float/bool/int/string token/vec3 storage
   */
  struct bSpyValueUnion
  {
    enum Type
    {
      pt_float,
      pt_bool,
      pt_int,
      pt_bitfield32,
      pt_string,
      pt_vector3,
      pt_address,
      pt_invalid
    };

    bSpyValueUnion() : m_type((bs_uint8)pt_invalid), m_name(BSPY_INVALID_MSGTOKEN) { m_value.i = 0; }

    explicit bSpyValueUnion(float f) : m_type((bs_uint8)pt_float), m_name(BSPY_INVALID_MSGTOKEN) { m_value.f = f; }
    explicit bSpyValueUnion(bs_int32 i) : m_type((bs_uint8)pt_int), m_name(BSPY_INVALID_MSGTOKEN) { m_value.i = i; }
    explicit bSpyValueUnion(bool b) : m_type((bs_uint8)pt_bool), m_name(BSPY_INVALID_MSGTOKEN) { m_value.b = b; }
    explicit bSpyValueUnion(bSpyStringToken s) : m_type((bs_uint8)pt_string), m_name(BSPY_INVALID_MSGTOKEN) { m_value.s = s; }
    explicit bSpyValueUnion(const bSpyVec3& v) : m_type((bs_uint8)pt_vector3), m_name(BSPY_INVALID_MSGTOKEN) { m_value.v = v; }
    explicit bSpyValueUnion(float x, float y, float z) : m_type((bs_uint8)pt_vector3), m_name(BSPY_INVALID_MSGTOKEN) { m_value.v.v[0] = x; m_value.v.v[1] = y; m_value.v.v[2] = z; }

    inline void set(float f) { m_type = (bs_uint8)pt_float; m_value.f = f; }
    inline void set(bs_int32 i) { m_type = (bs_uint8)pt_int; m_value.i = i; }
    inline void set(bool b) { m_type = (bs_uint8)pt_bool; m_value.b = b; }
    inline void set(bSpyStringToken s) { m_type = (bs_uint8)pt_string; m_value.s = s; }
    inline void set(const bSpyVec3& v) { m_type = (bs_uint8)pt_vector3; m_value.v = v; }
    inline void set(float x, float y, float z) { m_type = (bs_uint8)pt_vector3; m_value.v.v[0] = x; m_value.v.v[1] = y; m_value.v.v[2] = z; }
	inline void set(const void* a) { m_type = (bs_uint8)pt_address; m_value.a = (size_t)a; }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_name);

      if (m_type == static_cast<bs_uint8>(pt_float) || 
          m_type == static_cast<bs_uint8>(pt_int)   || 
          m_type == static_cast<bs_uint8>(pt_bitfield32))
        bSpy::utils::endianSwap(m_value.i); // 32 bits
      else if (m_type == static_cast<bs_uint8>(pt_string))
        bSpy::utils::endianSwap(m_value.s); // 16 bits
      else if (m_type == static_cast<bs_uint8>(pt_vector3))
        m_value.v.endianSwap();
      else if (m_type == static_cast<bs_uint8>(pt_address))
        bSpy::utils::endianSwap(m_value.a); // 64 bits
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      switch (static_cast<bs_uint8>(m_type))
      {
        case pt_float:
          payload += wxString::Format(L"\"%s\"  [float=%f]", bSpyStringCache::getWStringForToken(m_name), m_value.f); break;
        case pt_bool:
          payload += wxString::Format(L"\"%s\"  [bool=%s]", bSpyStringCache::getWStringForToken(m_name), m_value.b?L"true":L"false"); break;
        case pt_int:
          payload += wxString::Format(L"\"%s\"  [int=%i]", bSpyStringCache::getWStringForToken(m_name), m_value.i); break;
        case pt_bitfield32: // TODO properly
          payload += wxString::Format(L"\"%s\"  [bitfield32=%i]", bSpyStringCache::getWStringForToken(m_name), m_value.u); break;
        case pt_string:
          payload += wxString::Format(L"\"%s\"  [string=\"%s\"]", bSpyStringCache::getWStringForToken(m_name), bSpyStringCache::getWStringForToken(m_value.s)); break;
        case pt_vector3:
          payload += wxString::Format(L"\"%s\"  [vec3=<%f, %f, %f>]", bSpyStringCache::getWStringForToken(m_name), m_value.v.v[0], m_value.v.v[1], m_value.v.v[2]); break;
        case pt_address:
          payload += wxString::Format(L"\"%s\"  [addr=%08X]", bSpyStringCache::getWStringForToken(m_name), (bs_uint32)m_value.a); break;

        default:
        case pt_invalid:
          payload += wxString::Format(L"\"%s\"  [invalid_value]", bSpyStringCache::getWStringForToken(m_name)); break;
      }      
    }
#endif // BSPY_APP

    bs_uint8            m_type;
    bSpyStringToken     m_name;

    union
    {
      bool              b;
      bs_int32          i;
      bs_uint32         u;
      float             f;
      bSpyStringToken   s;
      bs_int64          a;
      bSpyVec3          v;
    }                   m_value;
  };


  /**
   * -----------------------------------------------------------------------------
   * container for standard primitive shapes
   */
  struct bSpyShapePrimitive
  {
    inline bSpyShapePrimitive() : 
      m_type((bs_uint8)eInvalid)
    {
    }

    inline void endianSwap()
    {
      // swap the largest in the union
      bSpy::utils::endianSwap(m_data.box.m_dims[0]);
      bSpy::utils::endianSwap(m_data.box.m_dims[1]);
      bSpy::utils::endianSwap(m_data.box.m_dims[2]);
    }

#ifdef BSPY_APP
    inline const wchar_t* getShapeTypeName() const
    {
      static wchar_t* shapeTypeNames[8] = 
      {
        L"invalid",
        L"Sphere",
        L"Box",
        L"Capsule",
        L"CapsuleVertical",
        L"Cylinder",
        L"CylinderVertical",
        L"MeshRef"
      };

      int shapeTypeIndex = m_type;
      if (shapeTypeIndex > 7)
        shapeTypeIndex = 0;

      return shapeTypeNames[m_type];
    }

    inline void debugDescribeAppend(wxString& payload) const
    {
      payload += wxString::Format(L"[%s]", getShapeTypeName());
    }
#endif // BSPY_APP

    enum
    {
      eInvalid = 0,
      eShapeSphere = 1,
      eShapeBox,
      eShapeCapsule,
      eShapeCapsuleVertical,
      eShapeCylinder,
      eShapeCylinderVertical,
      eShapeMeshRef
    };
    bs_uint8          m_type;

    union
    {
      struct sphereShapeData
      {
        float         m_radius;
      } sphere;

      struct boxShapeData
      {
        float         m_dims[3];
      } box;

      struct capsuleShapeData
      {
        float         m_length, 
                      m_radius;
      } capsule;

      struct cylinderShapeData
      {
        float         m_length,
                      m_radius;
      } cylinder;

      struct meshRefData
      {
        bs_uint32     m_polyCount;
      } mesh;
    }                 m_data;
  };


  /**
   * -----------------------------------------------------------------------------
   * simple ping packet, used for timing, socket testing, etc
   */
  struct PingPacket : public PacketBase
  {
    inline PingPacket(bs_uint32 id) : PacketBase(),
      m_id(id)
    {
      BSPY_HEADER(Ping)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_id);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"ID=%i", m_id);
    }
#endif // BSPY_APP

    bs_uint32 m_id;
  };

  /**
   * -----------------------------------------------------------------------------
   * game->bspy 'handshake' packet sent to configure the application.
   * its arrival indicates that we have a working connection and the game
   * is aware of our presence.
   */
  struct IdentityPacket : public PacketBase
  {
    inline IdentityPacket(
        const char* fourCC_gameID,
        bs_uint32 verCodeRevision,
        bs_uint32 verRageRelease,
        bs_uint32 verRageMajor,
        bs_uint32 verRageMinor,
        bSpyStringToken timestamp) : PacketBase(),
      m_protocolVerSpy(NM_BSPY_PROTOCOL_VER),
      m_codeRevision(verCodeRevision),
      m_rageRelease(verRageRelease),
      m_rageMajor(verRageMajor),
      m_rageMinor(verRageMinor),
      m_compilationTimestamp(timestamp)
    {
      BSPY_HEADER(Identity)

      m_gameID[0] = fourCC_gameID[0];
      m_gameID[1] = fourCC_gameID[1];
      m_gameID[2] = fourCC_gameID[2];
      m_gameID[3] = fourCC_gameID[3];
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_codeRevision);
      bSpy::utils::endianSwap(m_rageRelease);
      bSpy::utils::endianSwap(m_rageMajor);
      bSpy::utils::endianSwap(m_rageMinor);

      bSpy::utils::endianSwap(m_compilationTimestamp);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"game=[%c%c%c%c] ver=[%i] rage=[%i.%i.%i]", 
        m_gameID[0], m_gameID[1], m_gameID[2], m_gameID[3],
        m_codeRevision,
        m_rageRelease, m_rageMajor, m_rageMinor);
    }
#endif // BSPY_APP

    bs_uint8          m_protocolVerSpy;     // version code for making sure bSpy network code matches between app & game

    // code revision / rage version stamping
    bs_uint32         m_codeRevision,
                      m_rageRelease,
                      m_rageMajor,
                      m_rageMinor;

    bSpyStringToken   m_compilationTimestamp;

    char              m_gameID[4];          // 4-character identifier, eg. 'GTA5', 'INDY', etc.
  };


  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct BeginStepPacket : public PacketBase
  {
    inline BeginStepPacket(float timeStep) : PacketBase(),
      m_timeStep(timeStep)
    {
      BSPY_HEADER(BeginStep)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_timeStep);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"timeStep=%.4f, ", m_timeStep);
    }
#endif // BSPY_APP

    float         m_timeStep;
  };

  // -------------------------------------------------------------------------------------------------------------------
  struct AgentStatePacket : public PacketBase
  {
    inline AgentStatePacket(AgentState &agentState) : PacketBase(),
      m_agentState(agentState)
    {
      BSPY_HEADER(AgentState)
    }

    inline void endianSwap()
    {
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"State: ");

      wchar_t buf[32];
      for (int b=0; b<32; b++)
      {
        if (m_agentState.m_agentAssetLayout[b] >= 0)
        {
          swprintf(buf, L"[%i = %i] ", b, m_agentState.m_agentAssetLayout[b]);
          payload.Append(buf);
        }
      }
    }
#endif // BSPY_APP

    AgentState    m_agentState;
  };


  /**
   * -----------------------------------------------------------------------------
   * marker packet sent at end of Step 2
   */
  struct EndStepPacket : public PacketBase
  {
    inline EndStepPacket() : PacketBase()
    {
      BSPY_HEADER(EndStep)
    }

    inline void endianSwap()
    {
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString&) const
    {
    }
#endif // BSPY_APP
  };


  /**
   * -----------------------------------------------------------------------------
   * sent before other update packets to choose which agent ID we're operating on
   */
  struct SelectAgentPacket : public PacketBase
  {
    inline SelectAgentPacket(bs_int16 agid) : PacketBase(),
      m_agentID(agid)
    {
      BSPY_HEADER(SelectAgent)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_agentID);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"agentID=%i", m_agentID);
    }
#endif // BSPY_APP

    bs_int16      m_agentID;
  };


  /**
   * -----------------------------------------------------------------------------
   * sent before other descriptor packets to choose which agent ID we're operating on
   */
  struct SelectAssetPacket : public PacketBase
  {
    inline SelectAssetPacket(bs_int16 asid) : PacketBase(),
      m_assetID(asid)
    {
      BSPY_HEADER_ZF(SelectAsset)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_assetID);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"assetID=%i", m_assetID);
    }
#endif // BSPY_APP

    bs_int16      m_assetID;
  };    


  // -------------------------------------------------------------------------------------------------------------------
  // sent when a new string has been cached on the runtime host
  //
  struct AddStringToCachePacket : public PacketBase
  {
    inline AddStringToCachePacket(const char* string, bSpyStringToken token) : PacketBase()
    {
      BSPY_HEADER_ZF(AddStringToCache)

      size_t stringLen = strlen(string);
      BSPY_ASSERT(stringLen <= 2047);

      m_token = token;
      strncpy(m_string, string, v_min(stringLen + 1, 2047u));

      // modify the length of the packet to fit the string
      hdr.m_length = (bs_uint16)(sizeof(PacketBase) + sizeof(bSpyStringToken) + stringLen + 1);
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_token);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"token=%i, string=\"%S\"", m_token, m_string);
    }
#endif // BSPY_APP

    bSpyStringToken       m_token;
    char                  m_string[2048];
  };


  /**
   * -----------------------------------------------------------------------------
   * stream of transforms, bound to an asset type and optional agent ID
   */
  struct TransformStreamPacket : public PacketBase
  {
    enum { MaxTransformsToStore = 32 };

    inline TransformStreamPacket(bs_uint8 numTms, bs_int8 assetID, bSpyStringToken label) : PacketBase(),
      m_numTms(numTms),
      m_assetID(assetID),
      m_agentID(-1),
      m_labelToken(label)
    {
      BSPY_HEADER(TransformStream)
      BSPY_ASSERT(m_numTms < MaxTransformsToStore);

      hdr.m_length = (bs_uint16)(sizeof(TransformStreamPacket) - ((MaxTransformsToStore - m_numTms) * sizeof(bSpyMat34)));
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_labelToken);
      for (bs_uint8 i=0; i<m_numTms; i++)
        m_tms[i].endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"%s : numTms=%i, assetID=%i, agentID=%i", bSpyStringCache::getWStringForToken(m_labelToken), m_numTms, m_assetID, m_agentID);
    }
#endif // BSPY_APP

    bs_uint8              m_numTms;
    bs_int8               m_assetID;
    bs_int8               m_agentID;  // <0 for none
    bSpyStringToken       m_labelToken;
    bSpyMat34             m_tms[MaxTransformsToStore];
  };


  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct DirectInvokePacket : public PacketBase
  {
    static const unsigned int sMaxParams = 96;

    inline DirectInvokePacket(bs_uint16 agentID, bs_uint8 numParams) : PacketBase(),
      m_msgNameToken(BSPY_INVALID_MSGTOKEN),
      m_msgOrigin(BSPY_INVALID_MSGTOKEN),
      m_numParams(numParams)
    {
      BSPY_HEADER(DirectInvoke)

      BSPY_ASSERT(m_numParams < sMaxParams);

      m_agentID = agentID;

      hdr.m_length = (bs_uint16)(sizeof(DirectInvokePacket) - ((sMaxParams - m_numParams) * sizeof(bSpyValueUnion)));
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_msgNameToken);
      bSpy::utils::endianSwap(m_msgOrigin);
      bSpy::utils::endianSwap(m_agentID);

      for (bs_uint16 i=0; i<m_numParams; i++)
        m_params[i].endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"msgName=\"%s\", agentID=%i, params=%i", bSpyStringCache::getWStringForToken(m_msgNameToken), m_agentID, m_numParams);
    }
#endif // BSPY_APP

    bSpyStringToken     m_msgNameToken;
    bSpyStringToken     m_msgOrigin;
    bs_uint16           m_agentID;
    bs_uint8            m_numParams;
    bSpyValueUnion      m_params[sMaxParams];
  };


  /**
   * -----------------------------------------------------------------------------
   * packet that can be sent to the bSpy server in-game to control
   * what data will be sent - used to filter down the bandwidth used
   */
  struct TransmissionControlPacket : public PacketBase
  {
    // add a flag here, ensure you add a readable name below in getTxOpList
    // this ensures bSpy can build the UI to toggle them on and off
    enum TransmissionFlags
    {
      bSpyTF_None                     = 0,
      bSpyTF_TransformStreams         = (1 << 0),
      bSpyTF_CollisionContactData     = (1 << 1),
      bSpyTF_StaticMeshData           = (1 << 2),
      bSpyTF_LogData                  = (1 << 3),
      bSpyTF_DebugDraw                = (1 << 4),
      bSpyTF_DynamicCollisionShapes   = (1 << 5),
      bSpyTF_Scratchpad               = (1 << 6),
      bSpyTF_Tasks                    = (1 << 7),
      bSpyTF_Limbs                    = (1 << 8),

      bSpyTF_All                      = 0xFFFFFFFF
    };

    inline TransmissionControlPacket(bs_uint32 flags = bSpyTF_All) : PacketBase(),
      m_txFlags(flags)
    {
      BSPY_HEADER(TransmissionControl)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_txFlags);
    }

#ifdef BSPY_APP
    inline static void getTxOpList(wxStringList& nameList)
    {
      // ensure same order as flag list
      nameList.Add(wxString(L"Transform Streams"));
      nameList.Add(wxString(L"Collision Contact Data"));
      nameList.Add(wxString(L"Static Mesh Data"));
      nameList.Add(wxString(L"Log Data"));
      nameList.Add(wxString(L"Debug Draw"));
      nameList.Add(wxString(L"Dynamic Collision Shapes"));
      nameList.Add(wxString(L"Scratchpad"));
      nameList.Add(wxString(L"Tasks"));
      nameList.Add(wxString(L"Limbs"));
    }

    inline void debugDescribe(wxString& payload) const
    {
      // unroll the flags variable
      char bitBuf[33];
      unsigned int tx = m_txFlags;
      for (int b=0; b<32; b++)
      {
        if ((tx & 1) == 1)
        {
          bitBuf[b] = 'O';
        }
        else
          bitBuf[b] = '_';

        tx >>= 1;
      }
      bitBuf[32] = 0;

      payload.Printf(L"txFlags=[%S]", bitBuf);    
    }
#endif // BSPY_APP

    bs_uint32           m_txFlags;
  };


  /**
   * -----------------------------------------------------------------------------
   * packet containing a 3 or 4 vertex polygon
   */
  struct PolyPacket : public PacketBase
  {
    enum PolyFlags
    {
      bSpyPF_None             = 0,
      bSpyPF_IsQuad           = (1 << 0),
      bSpyPF_IsStaticGeom     = (1 << 1),
    };

    inline PolyPacket(unsigned int flags = bSpyPF_None) : PacketBase()
    {
      BSPY_HEADER_ZF(Poly)

      m_flags = flags;

      if ((m_flags & bSpyPF_IsQuad) == bSpyPF_IsQuad)
        hdr.m_length -= sizeof(bSpyVec3);
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_flags);      

      m_vert[0].endianSwap();
      m_vert[1].endianSwap();
      m_vert[2].endianSwap();

      if ((m_flags & bSpyPF_IsQuad) == bSpyPF_IsQuad)
        m_vert[3].endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Clear();

      if ((m_flags & bSpyPF_IsQuad) == bSpyPF_IsQuad)
        payload += L"isQuad ";

      if ((m_flags & bSpyPF_IsStaticGeom) == bSpyPF_IsStaticGeom)
        payload += L"IsStaticGeom ";
    }
#endif // BSPY_APP

    inline unsigned int getFlags() const { return m_flags; }

  protected:
    // protected - shouldn't change these without adjusting header flags, for example
    unsigned int        m_flags;

  public:
    bSpyVec3            m_vert[4];
  };


  /**
   * -----------------------------------------------------------------------------
   * a per-frame collision with a non-NM dynamic physics object
   */
  struct DynamicCollisionShapePacket : public PacketBase
  {
    inline DynamicCollisionShapePacket(bs_uint32 levelIndex, bs_uint32 shapeUID, bs_int16 agentID, bs_int32 instClassType) : PacketBase(),
      m_shapeUID(shapeUID),
      m_levelIndex(levelIndex),
      m_agentID(agentID),
      m_instanceClassType(instClassType),
      m_flags(0)
    {
      BSPY_HEADER(DynamicCollisionShape)
    }

    enum DOFlags
    {
      bSpyDO_None             = 0,
      bSpyDO_Fixed            = (1 << 0),
      bSpyDO_Inactive         = (1 << 1),
      bSpyDO_EPatch           = (1 << 2),
    };

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"agent=%i, levelIndex=%0X8, shape=%08X, class=%i, tr:[%g, %g, %g]", 
        m_agentID, 
        m_levelIndex,
        m_shapeUID, 
        m_instanceClassType, 
        m_tm.m_row[3].v[0],
        m_tm.m_row[3].v[1],
        m_tm.m_row[3].v[2]);
      m_shape.debugDescribeAppend(payload);
    }
#endif // BSPY_APP

    inline void endianSwap()
    {
      m_shape.endianSwap();
      m_tm.endianSwap();

      bSpy::utils::endianSwap(m_shapeUID);
      bSpy::utils::endianSwap(m_levelIndex);
      bSpy::utils::endianSwap(m_instanceClassType);
      bSpy::utils::endianSwap(m_agentID);
      bSpy::utils::endianSwap(m_flags);
    }

    bSpyShapePrimitive    m_shape;
    bSpyMat34             m_tm;
    bs_uint32             m_shapeUID,           // uid for the shape (may be shared, eg meshes)
                          m_levelIndex,         // rage level index
                          m_instanceClassType;
    bs_int16              m_agentID;            // NM agent collided with (or -1)
    bs_uint16             m_flags;              // from DOFlags
  };


  /**
   * -----------------------------------------------------------------------------
   * send a log string using the string token mechanism
   */
  struct LogStringPacket : public PacketBase
  {
    inline LogStringPacket(bool ZF, bs_uint8 log_type, bSpyStringToken token) : PacketBase(),
      m_type(log_type),
      m_hasFileInfo(0),
      m_fileLine(0),
      m_token(token),
      m_filenameToken(BSPY_INVALID_MSGTOKEN)
    {
      if (ZF)
      {
        BSPY_HEADER_ZF(LogString)
      }
      else
      {
        BSPY_HEADER(LogString)
      }
    }

    inline void setFileInfo(bSpyStringToken file, bs_uint16 line)
    {
      m_hasFileInfo = 1;
      m_filenameToken = file;
      m_fileLine = line;
    }

    inline bool hasFileInfo() const 
    {
      return (m_hasFileInfo == 1);
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_fileLine);
      bSpy::utils::endianSwap(m_token);
      bSpy::utils::endianSwap(m_filenameToken);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"\"%s\"", bSpyStringCache::getWStringForToken(m_token));
    }
#endif // BSPY_APP

    enum LogType
    {
      lt_info,
      lt_warning,
      lt_error,
      lt_invalid
    };

    bs_uint8              m_type;
  protected:
    bs_uint8              m_hasFileInfo;
  public:
    bs_uint16             m_fileLine;
    bSpyStringToken       m_token;
    bSpyStringToken       m_filenameToken;
  };


  /**
   * -----------------------------------------------------------------------------
   *
   */
  struct FeedbackPacket : public PacketBase
  {
    inline FeedbackPacket(bs_uint8 feedback_type, bs_uint8 agent_id, bs_uint8 argNum) : PacketBase()
    {
      BSPY_HEADER(Feedback)

      m_type  = feedback_type;
      m_agentId = agent_id;
      m_argsNum = argNum;
      m_retVal = 0;

      hdr.m_length = (bs_uint16)(sizeof(FeedbackPacket) - ((feedbackMaxUserData - m_argsNum) * sizeof(FeedbackUserdata)));
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_behaviourName);
      bSpy::utils::endianSwap(m_calledByBehaviourName);
      bSpy::utils::endianSwap(m_retVal);

      for (int i=0; i<m_argsNum; ++i)
        m_args[i].endianSwap();
    }

    enum FeedbackType
    {
      ft_start,     // automatically fired when a behaviour is started
      ft_failure,
      ft_success,
      ft_finish,    // automatically fired when a behaviour deactivates
      ft_event,     // allows arbitrary event feedback from behaviours
      ft_request    // used to communicate between behaviour -> game -> behaviour
    };

#ifdef BSPY_APP
    inline static wchar_t* getNameForType(bs_uint8 fbt)
    {
      static wchar_t* typeNames[6] = 
      {
        L"start",
        L"failure",
        L"success",
        L"finish",
        L"event",
        L"request"
      };
      return typeNames[fbt];
    }
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"\"%s\", agentID=%i, bv=\"%s\", bvBy=\"%s\", params=%i", getNameForType(m_type), m_agentId, bSpyStringCache::getWStringForToken(m_behaviourName), bSpyStringCache::getWStringForToken(m_calledByBehaviourName), m_argsNum);
    }
#endif // BSPY_APP

    struct FeedbackUserdata
    {
      enum FeedbackArgType
      {
        at_int,
        at_float,
        at_bool,
        at_void,    // we don't want to send references, but we need to know if a behaviour sends one.
        at_stringToken,    
      };

      union
      {
        bs_int32            m_int;
        float               m_float;
        bool                m_bool;
		// This should really be defined as a bs_int64 on *all* platforms to accomodate either 32-bit or 64-bit systems
#if defined(_WIN64) || defined(NM_IA64)
        bs_int32            m_ptr;
#else
        void*               m_ptr;
#endif // 64b bspy
        bSpyStringToken     m_stringToken;
      };

      bs_uint8              m_argType;

      inline void setInt(bs_int32 value) { m_argType = at_int; m_int = value; }
      inline void setFloat(float value)  { m_argType = at_float; m_float = value; }
      inline void setBool(bool value)    { m_argType = at_bool; m_bool = value; }
      inline void setVoid()              { m_argType = at_void; m_ptr = 0; } // no value for references
      inline void setStringToken(bSpyStringToken value)       { m_argType = at_stringToken; m_stringToken = value; } 

      inline void endianSwap()
      {
        bSpy::utils::endianSwap(m_int); // swap largest
      }
    };


    static const int feedbackMaxUserData = 32;

    bs_uint8              m_type;
    bs_uint8              m_agentId;
    bSpyStringToken       m_behaviourName;
    bSpyStringToken       m_calledByBehaviourName;
    
    bs_uint32             m_retVal;
    bs_uint8              m_argsNum;
    FeedbackUserdata      m_args[feedbackMaxUserData];
  };


  /**
   * -----------------------------------------------------------------------------
   *
   */
  struct DebugLinePacket : public PacketBase
  {
    enum LineType
    {
      LT_Default = 0,

      LT_Force, 
      LT_Torque,
      LT_Acceleration,
      LT_AngularAcceleration,
      LT_Impulse,
      LT_TorqueImpulse,
      LT_Velocity, 
      LT_AngularVelocity,
      LT_Normal,

      LT_Invalid
    };

    inline DebugLinePacket(bs_int8 agent_id, LineType lt = LT_Default) : PacketBase(),
      m_agentId(agent_id),
      m_taskId(-1),
      m_lineType((bs_uint8)lt)
    {
      BSPY_HEADER(DebugLine)
    }

    inline void endianSwap()
    {
      m_start.endianSwap();
      m_end.endianSwap();
      m_colour.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      static const wchar_t* lineTypes[] = 
      {
        L"Default",
        L"Force", 
        L"Torque",
        L"Acceleration",
        L"AngularAcceleration",
        L"Impulse",
        L"TorqueImpulse",
        L"Velocity", 
        L"AngularVelocity",
        L"Normal",
        L"Invalid"
      };

      bs_uint8 lt = m_lineType;
      if (lt > LT_Normal)
        lt = LT_Invalid;

      payload.Printf(L"agent=%i, task=%i, type='%s'", m_agentId, m_taskId, lineTypes[lt]);
    }
#endif // BSPY_APP

    bSpyVec3   m_start;
    bSpyVec3   m_end;
    bSpyVec3   m_colour;
    bs_int8    m_agentId;
    bs_int8    m_taskId;
    bs_uint8   m_lineType;
  };



  /**
   * -----------------------------------------------------------------------------
   * 
   */
  const bs_uint8 TaskDescriptorMaxTasks = 128;
  struct TaskDescriptorPacket : public PacketBase
  {
    inline TaskDescriptorPacket(bs_uint8 numTasks) : PacketBase(),
      m_numTasks(numTasks)
    {
      BSPY_HEADER_ZF(TaskDescriptor)

      BSPY_ASSERT(m_numTasks < TaskDescriptorMaxTasks);

      hdr.m_length = (bs_uint16)(sizeof(PacketBase) + 
                      sizeof(bs_uint8) + 
                      (m_numTasks * sizeof(bSpyStringToken)) +
                      (m_numTasks * sizeof(bs_uint8) * 3)
                      );
    }

    inline void endianSwap()
    {
      for (bs_uint8 i=0; i<m_numTasks; i++)
        bSpy::utils::endianSwap(m_taskName[i]);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"numTasks=%i (of %i)", m_numTasks, TaskDescriptorMaxTasks);
    }
#endif // BSPY_APP

    bs_uint8          m_numTasks;
    bSpyStringToken   m_taskName[TaskDescriptorMaxTasks];
    struct Col8
    {
      bs_uint8 r,g,b;
    }                 m_taskColour[TaskDescriptorMaxTasks];
  };


  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct TaskBeginTaskPacket : public PacketBase
  {
    inline TaskBeginTaskPacket(bs_uint8 bvid) : PacketBase(),
      m_bvid(bvid),
      m_dynamicTask(false)
    {
      BSPY_HEADER(TaskBeginTask)
    }

    inline void endianSwap()
    {
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"behaviourID=%i, dynamic=%s", m_bvid, m_dynamicTask?L"Y":L"N");
    }
#endif // BSPY_APP

    bs_uint8          m_bvid;
    bool              m_dynamicTask;
  };


  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct TaskVarPacket : public PacketBase
  {
    inline TaskVarPacket(bSpyValueUnion::Type paramType, bool parameterBlock) : PacketBase()
    {
      BSPY_HEADER(TaskVar)

      m_data.m_type = (bs_uint8)paramType;
      m_paramBlock = parameterBlock;
    }

    inline void endianSwap()
    {
      m_data.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"%s", m_paramBlock?L"[pb] ":L"");
      m_data.debugDescribe(payload);
    }
#endif // BSPY_APP


    bSpyValueUnion      m_data;
    bool                m_paramBlock;
  };

  /**
   * -----------------------------------------------------------------------------
   * specialized version handling matrix values, to save bulking out the other packet
   */
  struct TaskMatrixVarPacket : public PacketBase
  {
    inline TaskMatrixVarPacket(bool parameterBlock) : PacketBase(),
      m_paramBlock(parameterBlock)
    {
      BSPY_HEADER(TaskMatrixVar)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_name);
      m_matrix.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"%s", m_paramBlock?L"[pb] ":L"");
    }
#endif // BSPY_APP


    bSpyStringToken     m_name;
    bSpyMat34           m_matrix;
    bool                m_paramBlock;
  };



  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct TaskEndTaskPacket : public PacketBase
  {
    inline TaskEndTaskPacket(bs_uint8 bvid) : PacketBase(),
      m_bvid(bvid)
    {
      BSPY_HEADER(TaskEndTask)
    }

    inline void endianSwap()
    {
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"behaviourID=%i, time=%f", m_bvid);
    }
#endif // BSPY_APP

    bs_uint8          m_bvid;
  };


  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct ContactPointPacket : public PacketBase
  {
    inline ContactPointPacket(bs_int32 agentID, bs_int32 agentID2) : PacketBase()
    {
      BSPY_HEADER(ContactPoint)

      d.m_agentID = agentID;
      d.m_agentID2 = agentID2;

      d.m_agentID2IsLevelIndex = false;
    }

    inline void endianSwap()
    {
      d.m_pos.endianSwap();
      d.m_posOther.endianSwap();
      d.m_norm.endianSwap();
      d.m_force.endianSwap();

      bSpy::utils::endianSwap(d.m_depth);
      bSpy::utils::endianSwap(d.m_friction);
      bSpy::utils::endianSwap(d.m_elasticity);

      bSpy::utils::endianSwap(d.m_agentID);
      bSpy::utils::endianSwap(d.m_agentID2);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"agentID=%i, agentID2=%i, partA=%i, partB=%i, depth=%f  %s", d.m_agentID, d.m_agentID2, d.m_part1, d.m_part2, d.m_depth, d.m_isConstraint?L"(isConstraint)":L"");
    }
#endif // BSPY_APP

    struct data
    {
      bSpyVec3      m_pos,
                    m_posOther,
                    m_norm,
                    m_force;
      float         m_depth,
                    m_friction,
                    m_elasticity;
      bs_int32      m_agentID,
                    m_agentID2;   // -1 if not NM-controlled character
      bs_int8       m_part1,      // -1 if NM-controlled part(s)
                    m_part2;
      bool          m_isForce, 
                    m_isConstraint,
                    m_isDisabled,
                    m_agentID2IsLevelIndex;
    } d;
  };


  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct ScratchpadPacket : public PacketBase
  {
    inline ScratchpadPacket(bs_int8 id) : PacketBase(),
      m_lineNumber(0),
      m_callStamp(0),
      m_agentID(id),
      m_taskID(-1)
    {
      BSPY_HEADER(Scratchpad)
    }

    inline void endianSwap()
    {
      m_data.endianSwap();
      bSpy::utils::endianSwap(m_tag);
      bSpy::utils::endianSwap(m_lineNumber);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"name=\"%s\", tag=\"%s\", line: %i, stamp: %i {agent %i, task %i}", 
        bSpyStringCache::getWStringForToken(m_data.m_name), 
        bSpyStringCache::getWStringForToken(m_tag), 
        m_lineNumber, 
        m_callStamp,
        m_agentID,
        m_taskID);
    }
#endif // BSPY_APP


    bSpyValueUnion      m_data;
    bSpyStringToken     m_tag;
    bs_uint16           m_lineNumber;
    bs_uint8            m_callStamp;
    bs_int8             m_agentID;
    bs_int8             m_taskID;
  };


  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct MemoryUsageRecordPacket : public PacketBase
  {
    inline MemoryUsageRecordPacket(bSpyStringToken name, bs_uint32 amountInBytes) : PacketBase(),
      m_name(name),
      m_amount(amountInBytes)
    {
      BSPY_HEADER_ZF(MemoryUsageRecord)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_name);
      bSpy::utils::endianSwap(m_amount);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"\"%s\" = %u bytes", bSpyStringCache::getWStringForToken(m_name), m_amount);
    }
#endif // BSPY_APP


    bSpyStringToken     m_name;
    bs_uint32           m_amount;
  };

  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct ProfilerPacket : public PacketBase
  {
    inline ProfilerPacket(bSpyStringToken name, bs_int8 agentID, float msTimestamp, bool isStart) : PacketBase(),
      m_name(name),
      m_time(msTimestamp),
      m_agentID(agentID),
      m_isStart(isStart)
    {
      BSPY_HEADER(Profiler)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_name);
      bSpy::utils::endianSwap(m_time);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"\"%s\" [%i] %s - %g", bSpyStringCache::getWStringForToken(m_name), m_agentID, m_isStart ? L"start" : L"end", m_time);
    }
#endif // BSPY_APP


    bSpyStringToken     m_name;
    float               m_time;
    bs_int8             m_agentID;
    bool                m_isStart;
  };

  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct LimbsBeginPacket : public PacketBase
  {
    inline LimbsBeginPacket(
      bSpyStringToken limbName,
      bSpyStringToken messageName,
      bSpyStringToken task,
      int priority,
      int subPriority,
      int mask,
      float weight,
      bs_int8 agid) : PacketBase(),
      m_limbName(limbName),
      m_messageName(messageName),
      m_task(task),
      m_priority(priority),
      m_subPriority(subPriority),
      m_mask(mask),
      m_weight(weight),
      m_agentID(agid)
    {
      BSPY_HEADER(LimbsBegin)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_limbName);
      bSpy::utils::endianSwap(m_messageName);
      bSpy::utils::endianSwap(m_task);
      bSpy::utils::endianSwap(m_priority);
      bSpy::utils::endianSwap(m_subPriority);
      bSpy::utils::endianSwap(m_mask);
      bSpy::utils::endianSwap(m_weight);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
      payload.Printf(L"%s [%d|%s]", bSpyStringCache::getWStringForToken(m_messageName), m_agentID, bSpyStringCache::getWStringForToken(m_task));
    }
#endif // BSPY_APP

    bSpyStringToken   m_limbName;
    bSpyStringToken   m_messageName;
    bSpyStringToken   m_task;
    int               m_priority;
    int               m_subPriority;
    int               m_mask;
    float             m_weight;
    bs_int8           m_agentID;
  };

  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct LimbsEndPacket : public PacketBase
  {
    inline LimbsEndPacket() : PacketBase()
    {
      BSPY_HEADER(LimbsEnd)
    }

    inline void endianSwap()
    {
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
    }
#endif // BSPY_APP
  };

#undef NM_RS_RO_PARAMETER
#define NM_RS_RO_PARAMETER(classname, type, name)

#undef NM_RS_RO_PARAMETER_ACTUAL
#define NM_RS_RO_PARAMETER_ACTUAL(classname, type, name)

#undef NM_RS_PARAMETER_DIRECT
#define NM_RS_PARAMETER_DIRECT(_prefix, _type, _name, _min, _max, _default)

#define DECLARE_LIMB_COMPONENT_VALUE(type, name)\
  type m_##name;

  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct LimbComponentIKPacket : public PacketBase
  {
    static const int FlagOffset = __COUNTER__;
    enum Flags
    {
      #undef NM_RS_PARAMETER
      #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) apply##_name = 1 << (__COUNTER__ - FlagOffset - 1),
      #ifdef BSPY_APP
      #include "naturalmotion/include/common/NmRsIK.inl"
      #else
      #include "common/NmRsIK.inl"
      #endif
      #undef NM_RS_PARAMETER

      flagCount
    };

    inline LimbComponentIKPacket(bSpyStringToken name) : PacketBase(),
      m_name(name),
      m_parameterSetFlags(0)
    {
      BSPY_HEADER(LimbComponentIK)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_name);
      m_Target.endianSwap();
      m_Velocity.endianSwap();
      bSpy::utils::endianSwap(m_Twist);
      bSpy::utils::endianSwap(m_DragReduction);
      bSpy::utils::endianSwap(m_MaxReachDistance);
      bSpy::utils::endianSwap(m_Blend);
      bSpy::utils::endianSwap(m_AdvancedStaightness);
      bSpy::utils::endianSwap(m_AdvancedMaxSpeed);
      m_PoleVector.endianSwap();
      m_EffectorOffset.endianSwap();
      bSpy::utils::endianSwap(m_MatchClavicle);
      bSpy::utils::endianSwap(m_MinimumElbowAngle);
      bSpy::utils::endianSwap(m_MaximumElbowAngle);
      m_WristTarget.endianSwap();
      m_WristNormal.endianSwap();
      bSpy::utils::endianSwap(m_WristTwistLimit);
      bSpy::utils::endianSwap(m_parameterSetFlags);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
    }
#endif // BSPY_APP

    bSpyStringToken m_name;

#if 0
    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DECLARE_LIMB_COMPONENT_VALUE(_type, _name)
    #ifdef BSPY_APP
    #include "naturalmotion/include/common/NmRsIK.inl"
    #else
    #include "common/NmRsIK.inl"
    #endif
    #undef NM_RS_PARAMETER
#else
    // Needs to be done this way due to difference in bSpy vector types...
    // Perhaps some macro magician has an answer to this...
    DECLARE_LIMB_COMPONENT_VALUE(bSpyVec3, Target)
    DECLARE_LIMB_COMPONENT_VALUE(bool, UseTargetVelocity)
    DECLARE_LIMB_COMPONENT_VALUE(bool, TwistIsFixed)
    DECLARE_LIMB_COMPONENT_VALUE(bool, CanDoIKGreedy)
    DECLARE_LIMB_COMPONENT_VALUE(bSpyVec3, Velocity)
    DECLARE_LIMB_COMPONENT_VALUE(float, Twist)
    DECLARE_LIMB_COMPONENT_VALUE(float, DragReduction)
    DECLARE_LIMB_COMPONENT_VALUE(float, MaxReachDistance)
    DECLARE_LIMB_COMPONENT_VALUE(float, Blend)
    DECLARE_LIMB_COMPONENT_VALUE(bool, UseAdvancedIk)
    DECLARE_LIMB_COMPONENT_VALUE(float, AdvancedStaightness)
    DECLARE_LIMB_COMPONENT_VALUE(float, AdvancedMaxSpeed)
    DECLARE_LIMB_COMPONENT_VALUE(bool, UsePoleVector)
    DECLARE_LIMB_COMPONENT_VALUE(bSpyVec3, PoleVector)
    DECLARE_LIMB_COMPONENT_VALUE(bool, UseEffectorOffset)
    DECLARE_LIMB_COMPONENT_VALUE(bSpyVec3, EffectorOffset)
    DECLARE_LIMB_COMPONENT_VALUE(int, MatchClavicle)
    DECLARE_LIMB_COMPONENT_VALUE(float, MinimumElbowAngle)
    DECLARE_LIMB_COMPONENT_VALUE(float, MaximumElbowAngle)
    DECLARE_LIMB_COMPONENT_VALUE(bSpyVec3, WristTarget)
    DECLARE_LIMB_COMPONENT_VALUE(bSpyVec3, WristNormal)
    DECLARE_LIMB_COMPONENT_VALUE(float, WristTwistLimit)
    DECLARE_LIMB_COMPONENT_VALUE(bool, WristUseActualAngles)
#endif

    bs_uint32 m_parameterSetFlags;
  };

  /**
   * -----------------------------------------------------------------------------
   * 
   */

#undef DECLARE_LIMB_COMPONENT_VALUE
#define DECLARE_LIMB_COMPONENT_VALUE(type, name)\
  type m_##name;\
  bSpyStringToken m_##name##SetBy;

  struct LimbComponent1DofPacket : public PacketBase
  {
    static const int FlagOffset = __COUNTER__;
    enum Flags
    {
      #undef NM_RS_PARAMETER
      #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) \
      apply##_name = 1 << (__COUNTER__ - FlagOffset - 1),
      #ifdef BSPY_APP
      #include "naturalmotion/include/common/NmRs1DofEffector.inl"
      #else
      #include "common/NmRs1DofEffector.inl"
      #endif
      #undef NM_RS_PARAMETER

      flagCount
    };

    inline LimbComponent1DofPacket(bSpyStringToken name) : PacketBase(),
      m_name(name),
      m_parameterSetFlags(0)
    {
      BSPY_HEADER(LimbComponent1Dof)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_name);
      bSpy::utils::endianSwap(m_MuscleStrength);
      bSpy::utils::endianSwap(m_MuscleStrengthSetBy);
      bSpy::utils::endianSwap(m_MuscleDamping);
      bSpy::utils::endianSwap(m_MuscleDampingSetBy);
      bSpy::utils::endianSwap(m_MuscleStiffness);
      bSpy::utils::endianSwap(m_MuscleStiffnessSetBy);
      bSpy::utils::endianSwap(m_OpposeGravity);
      bSpy::utils::endianSwap(m_OpposeGravitySetBy);
      bSpy::utils::endianSwap(m_DesiredAngle);
      bSpy::utils::endianSwap(m_DesiredAngleSetBy);
      bSpy::utils::endianSwap(m_parameterSetFlags);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
    }
#endif // BSPY_APP

    bSpyStringToken m_name;

    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DECLARE_LIMB_COMPONENT_VALUE(_type, _name)
    #ifdef BSPY_APP
    #include "naturalmotion/include/common/NmRs1DofEffector.inl"
    #else
    #include "common/NmRs1DofEffector.inl"
    #endif
    #undef NM_RS_PARAMETER

    bs_uint16 m_parameterSetFlags;
  };

  /**
   * -----------------------------------------------------------------------------
   * 
   */
  struct LimbComponent3DofPacket : public PacketBase
  {
    static const int FlagOffset = __COUNTER__;
    enum Flags
    {
      #undef NM_RS_PARAMETER
      #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) \
      apply##_name = 1 << (__COUNTER__ - FlagOffset - 1),
      #ifdef BSPY_APP
      #include "naturalmotion/include/common/NmRs3DofEffector.inl"
      #else
      #include "common/NmRs3DofEffector.inl"
      #endif
      #undef NM_RS_PARAMETER

      flagCount
    };

    inline LimbComponent3DofPacket(bSpyStringToken name) : PacketBase(),
      m_name(name),
      m_parameterSetFlags(0)
    {
      BSPY_HEADER(LimbComponent3Dof)
    }

    inline void endianSwap()
    {
      bSpy::utils::endianSwap(m_name);
      bSpy::utils::endianSwap(m_MuscleStrength);
      bSpy::utils::endianSwap(m_MuscleStrengthSetBy);
      bSpy::utils::endianSwap(m_MuscleDamping);
      bSpy::utils::endianSwap(m_MuscleDampingSetBy);
      bSpy::utils::endianSwap(m_MuscleStiffness);
      bSpy::utils::endianSwap(m_MuscleStiffnessSetBy);
      bSpy::utils::endianSwap(m_OpposeGravity);
      bSpy::utils::endianSwap(m_OpposeGravitySetBy);
      bSpy::utils::endianSwap(m_DesiredTwist);
      bSpy::utils::endianSwap(m_DesiredTwistSetBy);
      bSpy::utils::endianSwap(m_DesiredLean1);
      bSpy::utils::endianSwap(m_DesiredLean1SetBy);
      bSpy::utils::endianSwap(m_DesiredLean2);
      bSpy::utils::endianSwap(m_DesiredLean2SetBy);
      bSpy::utils::endianSwap(m_parameterSetFlags);
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const
    {
    }
#endif // BSPY_APP

    bSpyStringToken m_name;

    #define NM_RS_PARAMETER(_prefix, _type, _name, _min, _max, _default) DECLARE_LIMB_COMPONENT_VALUE(_type, _name)
    #ifdef BSPY_APP
    #include "naturalmotion/include/common/NmRs3DofEffector.inl"
    #else
    #include "common/NmRs3DofEffector.inl"
    #endif
    #undef NM_RS_PARAMETER

    bs_uint16 m_parameterSetFlags;
  };

  /*
   *  Generic data packet used for building arbitrary limb requests
   */
  struct LimbComponentDataPacket : public PacketBase
  {
    inline LimbComponentDataPacket(bSpyValueUnion::Type paramType) : PacketBase()
    {
      BSPY_HEADER(LimbComponentData)
      m_data.m_type = (bs_uint8)paramType;
    }

    inline void endianSwap()
    {
      m_data.endianSwap();
    }

#ifdef BSPY_APP
    inline void debugDescribe(wxString& payload) const {}
#endif // BSPY_APP

    bSpyValueUnion m_data;
  };

  #pragma pack()
}

#undef v_max
#undef v_min

#endif // NM_BSPY_COMMON_PACKETS_H
