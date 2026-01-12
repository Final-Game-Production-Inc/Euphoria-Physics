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

#ifndef NM_BSPY_PACKET_H
#define NM_BSPY_PACKET_H

#include "nmutils\nmtypes.h"

namespace bSpy
{
  #define NM_BSPY_PORT              (27014)

  // unifying protocol version (used to be split between client/server versions, now we integrate the rockstar
  // handler code directly into bSpy we just use a single number.
  //
  // WHEN YOU CHANGE THIS, PLEASE ALSO UPDATE THE NM_VERSION_RELEASE IN apps/bspy3/NMProductConfig.h
  //
  #define NM_BSPY_PROTOCOL_VER      (62)

  // WM_USER + this used to talk to bSpy via ::SendMessage on Windows
  #define NM_BSPY_WMUSER_MSG        (0xB547)

  #define NM_BSPY_COMMBUF_SZ        (1024 * 32)
  #define NM_BSPY_PC_COMMBUF_SZ     (1024 * 3)

  #define NM_BSPY_PKT_MAGIC_A       (0xFE)
  #define NM_BSPY_PKT_MAGIC_STD     (0xB0)
  #define NM_BSPY_PKT_MAGIC_ZF      (0xB1)

  #define NM_BSPY_PKT_MAX_SIZE      (4096)


  /*
   * -------------------------------------------------------------------
   */

#if defined(_MSC_VER)
  typedef signed __int8       bs_int8;
  typedef signed __int16      bs_int16;
  typedef signed __int32      bs_int32;
  typedef signed __int64      bs_int64;
  typedef unsigned __int8     bs_uint8;
  typedef unsigned __int16    bs_uint16;
  typedef unsigned __int32    bs_uint32;
  typedef unsigned __int64    bs_uint64;
#endif // _MSC_VER

#if defined(__PPU__) || defined(__ORBIS__)
  typedef char               bs_int8;
  typedef short              bs_int16;
  typedef int                bs_int32;
  typedef long               bs_int64;
  typedef unsigned char      bs_uint8;
  typedef unsigned short     bs_uint16;
  typedef unsigned int       bs_uint32;
  typedef unsigned long      bs_uint64;
#endif // __PPU__

  typedef bs_uint16          bSpyStringToken;
  #define BSPY_INVALID_MSGTOKEN   (0xFFFF)

  #define BSPY_ASSERT(_expr)

  /*
   * -------------------------------------------------------------------
   */

  namespace utils
  {

    /**
     * swap bytes in any given type.
     */
    template <typename T> inline void endianSwap(T &t)
    {
#define BSPY_XOR_BYTESWAP(a, b) (((a) == (b)) || (((a) ^= (b)), ((b) ^= (a)), ((a) ^= b)))
      T swapReturn = t;
      bs_uint8* c = reinterpret_cast<bs_uint8*>(&swapReturn);
      for (size_t s = 0; s < sizeof(t) / 2; ++s) 
      {
        BSPY_XOR_BYTESWAP(c[s], c[ (sizeof(t) - 1) - s ]);
      }
      t = swapReturn;
#undef BSPY_XOR_BYTESWAP
    }


    /**
     * hash a string into a u32
     * also returns length of the string, seeing as it iterates over it anyway
     */
    inline bs_int32 hashString(const char* const str, bs_uint32& strLength)
    {
      const char* strP = str;
      bs_uint32 hash = 0;
      bs_int32 c = *strP;
      strLength = 0;

      while (c)
      {
        hash = c + (hash << 6) + (hash << 16) - hash;

        ++ strP;
        ++ strLength;
        c = *strP;
      }

      return hash;
    }

    /**
     * hash a wide-char string into a u32
     * also returns length of the string, seeing as it iterates over it anyway
     */
    inline bs_int32 hashWString(const wchar_t* const str, bs_uint32& strLength)
    {
      const wchar_t* strP = str;
      bs_uint32 hash = 0;
      bs_int32 c = *strP;
      strLength = 0;

      while (c)
      {
        hash = c + (hash << 6) + (hash << 16) - hash;

        ++ strP;
        ++ strLength;
        c = *strP;
      }

      return hash;
    }
  }


  /*
   * -------------------------------------------------------------------
   */


  enum 
  {
    // internal bSpy packets start here
    bspy_pk_internal        = 2,

    #define BSPY_PACKET(pkn)   bspy_pk_##pkn,
    #include "bSpyPacket.inl"
    #undef BSPY_PACKET

    // client-specific packet IDs start here
    bspy_pk_user,


    // maximum packet ID (bs_uint8)
    bspy_pk_top             = 255
  };


  /*
   * -------------------------------------------------------------------
   */


  #pragma pack(4)

  /**
   * bSpy packet header must prefix all custom data packets sent;
   *   m_id - set to the unique identifier for the packet being sent
   *   m_length - set to the total packet size, header included
   */ 
  struct PacketHeader
  {
    inline PacketHeader() :
      m_magicA(NM_BSPY_PKT_MAGIC_A),
      m_magicB(NM_BSPY_PKT_MAGIC_STD),
      m_id(0),
      m_pkver(0),
      m_length(0)
    { 
    }

    bs_uint8     m_magicA;
    bs_uint8     m_magicB;
    bs_uint8     m_id;
    bs_uint8     m_pkver;
    bs_uint16    m_length;

    inline void endianSwap()
    {
      utils::endianSwap(m_length);
    }

    inline bool validMagic() const
    {
      if (m_magicA != NM_BSPY_PKT_MAGIC_A) return false;
      if (m_magicB != NM_BSPY_PKT_MAGIC_STD &&
          m_magicB != NM_BSPY_PKT_MAGIC_ZF) return false;
      return true;
    }

    inline bool isZF() const
    {
      return (m_magicB == NM_BSPY_PKT_MAGIC_ZF);
    }
  };

  struct PacketBase
  {
    PacketHeader    hdr;
  };

  struct AgentState
  {
    bs_int8   m_agentAssetLayout[32];
  };

  // simple macro function to fill in a packet header
  #define BSPY_HEADER(pkn) \
    hdr.m_id = bspy_pk_##pkn; \
    hdr.m_length = sizeof(pkn##Packet);

  // ditto the above, but toggles the magic byte to reflect
  // that this packet should be stored in the zero frame
  #define BSPY_HEADER_ZF(pkn) \
    hdr.m_magicB = NM_BSPY_PKT_MAGIC_ZF; \
    hdr.m_id = bspy_pk_##pkn; \
    hdr.m_length = sizeof(pkn##Packet);


  /**
   * bSpy network traffic requires all packets to be big-endian on transmission.
   * This means that on 360/CELL we don't need to bother swizzling data, we leave it up to
   * the bSpy app to sort things out. On LE server targets (eg. RASH) packets will be swapped before
   * sending.
   */
#if defined(NM_PLATFORM_X360) || defined(NM_PLATFORM_CELL_PPU)

  #define BSPY_ENDIAN_SWAP  0
  #define bspySendPacket(pk)              ART::bSpyServer::inst()->sendPacket(pk);
  #define bspySendOrPrecachePacket(pk)    ART::bSpyServer::inst()->sendOrPrecachePacket(pk);

#elif defined(NM_PLATFORM_WIN32) || defined(NM_PLATFORM_ORBIS) || defined(NM_PLATFORM_DURANGO)

  #define BSPY_ENDIAN_SWAP  1
  #define bspySendPacket(pk)              pk.endianSwap(); ART::bSpyServer::inst()->sendPacket(pk);
  #define bspySendOrPrecachePacket(pk)    pk.endianSwap(); ART::bSpyServer::inst()->sendOrPrecachePacket(pk);

#else // NM_PLATFORM_WIN32

  #error unknown platform

#endif 

  #pragma pack()
}

#endif // NM_BSPY_PACKET_H


