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

#ifndef NM_ART_BSPY_SERVER
#define NM_ART_BSPY_SERVER

#include "NMutils/NMTypes.h"

// So that the gameLibs can be compiled with ART_ENABLE_BSPY 1 
// and still link with NM code if ART_ENABLE_BSPY 0 (e.g. for profiling)
// Bspy functions called by the gamelibs are defined here but do nothing
#if (!ART_ENABLE_BSPY) && NM_EMPTY_BSPYSERVER

namespace rage
{
  class Vector3;
  class Matrix34;
}

#include "NMutils/TypeUtils.h"
#include "art/ARTInternal.h"

namespace bSpy
{
  typedef rage::u8         bs_uint8;
  typedef rage::u16        bs_uint16;
  typedef rage::u32        bs_uint32;
  typedef rage::u64        bs_uint64;
  typedef rage::s8         bs_int8;
  typedef rage::s16        bs_int16;
  typedef rage::s32        bs_int32;
  typedef rage::s64        bs_int64;
  typedef bs_uint16        bSpyStringToken;

  struct bSpyVec3
  {
    float   v[3];
  };

  struct PacketHeader
  {
    bs_uint8     m_magicA;
    bs_uint8     m_magicB;
    bs_uint16    m_id;
    bs_uint16    m_length;
  };

  struct PacketBase
  {
    PacketHeader    hdr;
  };
}
 namespace ART
 {
  class bSpyServer
  {
    friend class NmRsCharacter;
  public:

    bSpyServer(ART::MemoryManager* services);
    virtual ~bSpyServer();

    static bSpyServer* inst();

    bool sendPacket(bSpy::PacketBase& pkt);
    inline float getMSTimeSinceBeginStep() const { return 0.0f; }

    bSpy::bSpyStringToken getTokenForString(const char* str);
    bSpy::bSpyVec3      bSpyVec3fromVector3(const rage::Vector3& vec);
  };
}

#endif//#if (!ART_ENABLE_BSPY) && NM_EMPTY_BSPYSERVER

#if ART_ENABLE_BSPY

#include "NMutils/TypeUtils.h"
#include "art\ARTInternal.h"

#include "bspy/bSpyCommonPackets.h"

#include <hash_map>

#if defined(NM_PLATFORM_CELL_PPU)
# include <sys/ppu_thread.h>
# include <sys/synchronization.h>
#endif // NM_PLATFORM_CELL_PPU

// convenient macros for switching out large blocks of bspy-only code when
// client is not connected.
#define BSPY_ISCLIENTCONNECTED_BEGIN() if (ART::bSpyServer::inst() && ART::bSpyServer::inst()->isClientConnected()) {
#define BSPY_ISCLIENTCONNECTED_END() }

// trying to avoid having to include winsock/xtl/windows here
typedef unsigned int      SOCKET;
typedef void*             HANDLE;
#ifndef INVALID_SOCKET
# define INVALID_SOCKET   (SOCKET)(~0)
#endif // INVALID_SOCKET
#ifndef SOCKET_ERROR
# define SOCKET_ERROR     (-1)
#endif // SOCKET_ERROR

#if defined(NM_COMPILER_MSVC)
# define nmsock_close           closesocket
# define nmsock_select          select
#elif defined(NM_PLATFORM_CELL_PPU)
# define nmsock_close           socketclose
# define nmsock_select          socketselect
#endif // NM_PLATFORM_CELL_PPU
#define  nmsock_socket          socket
#define  nmsock_bind            bind
#define  nmsock_listen          listen
#define  nmsock_shutdown        shutdown
#define  nmsock_accept          accept
#define  nmsock_setsockopt      setsockopt
#define  nmsock_recv            recv
#define  nmsock_send            send

#if defined(NM_COMPILER_MSVC)
typedef  HANDLE                 ThreadHandle;
typedef  HANDLE                 EventHandle;
#elif defined(NM_PLATFORM_CELL_PPU)
typedef  sys_ppu_thread_t       ThreadHandle;
typedef  sys_semaphore_t        EventHandle;
#endif // NM_PLATFORM_CELL_PPU

#define   BS_LOG_BUFFER_SIZE    512


namespace bSpy
{
  struct PacketBase;
}

namespace rage
{
  class Vector3;
  class Matrix34;
  class sysTimer;
}

namespace ART
{
  class NmRsCharacter;
  class bSpyChunkMemory;
  class bSpyStringMap;

  /**
   * Structures and types used by the string caching mechanism.
   * It uses the hash_map template; ideally we just use our own hash table class, but we don't have one :[
   */
  struct bSpyStringHashed
  {
    bSpy::bSpyStringToken   m_token;          // sequentially incrementing ID that uniquely identifies the string in question
    size_t                  m_stringLength;   // we calculate this when we create this structure, so cache it for posterity
    const char             *m_string;         // pointer to the string buffer containing a copy of the original string
  };


  /**
   * bSpyServer is designed to be overridden by euphoria plug-ins seeking to incorporate
   * bSpy functionality. This class encapsulates all the lower-level networking and general management code
   * and contains hooks that the derived class will implement to supply physics-engine-specific functionality.
   */
  class bSpyServer
  {
    friend class NmRsCharacter;

  public:

    bSpyServer(ART::MemoryManager* services);
    virtual ~bSpyServer();

    static bSpyServer* inst();

    /**
     * Initialize the server; if initNetworking is true, the server will bring Winsock, XNet, etc online first;
     * if the server is integrated into a game title, this is probably unnecessary, as the game will have done it already.
     */
    bool init(bool initNetworking);
    
    bool startServer();
    void stopServer();

    virtual void beginStep(float deltaTime);
    virtual void endStep(float deltaTime);

    void term();

    /**
     * adds a packet to be sent. buffers internally so that packets are grouped automatically, reducing number of ::send calls
     */
    bool sendPacket(bSpy::PacketBase& pkt);

    /**
     * manually flush the packet buffer (eg. at the end of a step) and reset the counters
     */
    void sendBuffer();


    /**
     * specialized version of sendPacket that can be used before a connection is made; if nothing
     * is connected yet, it buffers the data to be sent during onNewConnection(). Use with care!
     */
    bool sendOrPrecachePacket(bSpy::PacketBase& pkt);


    inline bool isBufferEmpty() const { return m_bufferUsed == 0; }
    inline unsigned int getSessionUID() const { return m_sessionUID; }

    /**
     * access the string cache; either return a fresh ID for the given string or 
     * return the ID assigned during a previous call. All new strings are automatically
     * transmitted to the host; the bSpyStringToken can then be used to reference the strings
     * in other packet structures.
     */
    bSpy::bSpyStringToken getTokenForString(const char* str);

    /**
     * helper function for sending logging packets, acts like a logf() implementation
     */
    void bSpyLogFormat(bool ZF, bool tryCache, bSpy::LogStringPacket::LogType type, const char* file, int line, const wchar_t* format, ...);

    int bSpyAssertHandler(const char* file, int line, const char* format, ...);

    // system for logging the last 'seen' agent, set at the start of pre/post steps
    // and used so that scratchpad sending can try and use agent IDs from the middle of code
    // that may not have access to a NmRsCharacter ptr
    inline void setLastSeenAgent(ART::AgentID id) { m_lastSeenAgent = id; }
    inline void clearLastSeenAgent() { m_lastSeenAgent = ART::INVALID_AGENTID; }
    inline ART::AgentID getLastSeenAgent() const { return m_lastSeenAgent; }


    inline unsigned int getFrameTicker() const { return m_frameTicker; }
    float getMSTimeSinceBeginStep() const;

    // todo; make threadsafe
    inline void setTransmissionFlags(unsigned int flags) { m_transmissionFlags = flags; }
    inline bool shouldTransmit(unsigned int flag) { return ((m_transmissionFlags & flag) == flag); }

    inline bool isInited() const { return m_inited; }
    inline bool isServerRunning() const { return (isInited() && m_serverSocket != INVALID_SOCKET); }
    inline bool isNetworkClientConnected() const { return (m_clientSocket != INVALID_SOCKET); }

#if __WIN32
    inline bool isClientConnected() const { return (isInited() && (m_fileToSerializeTo || isNetworkClientConnected())); }
#else
    inline bool isClientConnected() const { return (isInited() && isNetworkClientConnected()); }
#endif // __WIN32

    inline SOCKET getClientSocket() const { return m_clientSocket; }
    inline EventHandle getTerminatorEvent() const { return m_readThreadTerminator; }

#if __WIN32
    // we can capture network traffic at the lowest level and store it if the runtime is running
    // headless (eg. RASH) .. bSpy3 can then import this raw stream as a simulated network connection
    void setFileSerializeHandle(FILE* fs);
#endif // __WIN32

  protected:

    /**
     * The derived class implements this function to send all 'frame 0' data across - descriptors and
     * configuration packets.
     */
    virtual void onNewConnection()=0;

    /**
     * use this to send the identity packet and all cache buffers; should be done
     * first thing during onNewConnection()
     */
    void sendInitialPayloads(bSpy::IdentityPacket& ip);

    void sendIncomingTransforms(ART::NmRsCharacter* character);
    ART::MemoryManager  *m_artMemoryManager;
    static bSpyServer    *s_instance;

  private:

    // cached data, sent on new connection
    void sendStringMap();
    void sendPCBuffer();

    void shutdownClient();

    bool                  m_inited,
                          m_networkInited;

    SOCKET                m_serverSocket;
    SOCKET                m_clientSocket;

    ThreadHandle          m_readThread;
    EventHandle           m_readThreadTerminator;

    unsigned char        *m_buffer;
    unsigned int          m_bufferSize,
                          m_bufferUsed;

    unsigned char        *m_pcBuffer;
    unsigned int          m_pcBufferSize,
                          m_pcBufferUsed;

    int                   m_socketListenInterval;

    // see bSpy::TransmissionControlPacket enum
    unsigned int          m_transmissionFlags;

    ART::AgentID          m_lastSeenAgent;

    unsigned int          m_stringTokenIndex;
    bSpyStringMap        *m_stringMap;
    bSpyChunkMemory      *m_stringCacheMMgr;

    bSpy::bSpyStringToken m_defaultITMStreamLabels[ART::KITSourceCount];

    unsigned int          m_frameTicker;

    unsigned int          m_sessionUID;
    wchar_t               m_logBuffer[BS_LOG_BUFFER_SIZE];

    rage::sysTimer       *m_perfTimer;

#if __WIN32
    FILE                 *m_fileToSerializeToPending,   // set in accessor; we wait until beginStep to actually set the 
                         *m_fileToSerializeTo;          // real handle, so we keep things in step with the network
#endif // __WIN32

  };

  /**
   * a memory manager for the string cache buffer.
   * it can only allocate, deallocates do nothing. once finished with, all heaps are simply discarded.
   * it creates a heap of 'heapSize' bytes and allocates directly from that
   * pool without adding headers/footers to the returned pointers - once full, it allocates
   * new pools of at least 'subHeapSize' bytes. 
   *
   */
  class bSpyChunkMemory
  {
  public:

    bSpyChunkMemory(ART::MemoryManager* services, size_t heapSize, size_t subHeapSize);
    ~bSpyChunkMemory();

    // malloc & free substitutes
    void* alloc   (size_t size);
    void  dealloc (void* ptr);

    // reset the manager but don't discard heaps;
    // allows for easy reuse of pool memory
    void  resetForReuse();

  protected:

    void logStats();

    // create a new heap and assign to m_currentHeap
    void newSubHeap(size_t sz);

    // heap structure used to store everything we need to know about
    // a single storage block
    struct Heap
    {
      unsigned char*  m_heap;
      size_t          m_heapUse,
                      m_heapSize;
      unsigned int    m_pad;
    };

    ART::MemoryManager  *m_artMemoryManager;

    Heap              m_heapList[64];     ///< list of used heaps
    unsigned int      m_heapIndex;        ///< number of used heap entries
    Heap*             m_currentHeap;      ///< heap being used presently
    unsigned int      m_heapWalkIdx;      ///< 

    unsigned int      m_allocCount,       ///< number of alloc calls
                      m_deallocCount;     ///< number of dealloc calls
    size_t            m_subHeapSize,      ///< size of a new sub-heap, set in ctor
                      m_totalAllocSize;   ///< total memory usage footprint
  };

  // used when finding highest-bit-set in find/erase functions below
  static const unsigned int MultiplyDeBruijnBitPosition[32] = 
  {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
  };

  /**
   * A simplified/specialized NMTL::ChunkyHash, now used to operate the bSpy String Map
   */
  class bSpyStringMap
  {
  public:

    inline bSpyStringMap(ART::MemoryManager* services);
    inline ~bSpyStringMap();

    inline void insert(int key, const bSpyStringHashed& value);
    inline bool find(int key, bSpyStringHashed& value);

    typedef void (*iterateFn)(int key, bSpyStringHashed& value);
    inline void iterate(iterateFn fn);

    // call to purge the whole table, effectively resetting it;
    // note that chunks will still be pushed to the dismissal list, so 
    // if you really want all the memory nuked, call dismissUnusedChunks() afterwards.
    // see the dtor for example.
    inline void deleteAll();

  protected:

    inline unsigned int hashFunction(int key)
    {
      const unsigned int c2 = 0x27d4eb2d;
      key = (key ^ 61) ^ (key >> 16);
      key = key + (key << 3);
      key = key ^ (key >> 4);
      key = key * c2;
      key = key ^ (key >> 15);
      return key;
    }

    // hash table size, must by pow-2
    enum { TABLESIZE = 64 };

    struct HChunk
    {
      unsigned int    m_usage;    // bit field tracking usage

      // these are unpacked as we usually look at m_keys for the most part
      // so it's more cache efficient to have them sequential
      int                 m_keys[32];
      bSpyStringHashed    m_values[32];

      // linkage
      HChunk         *m_prev;
      HChunk         *m_next;

      // bitswizzling hack to find the index of the highest bit set; used
      // to (hopefully) shorten the iteration over a chunk's 'usage' bit field.
      inline unsigned int findHighestSetBit()
      {
        unsigned int v = m_usage;

        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v = (v >> 1) + 1;

        return MultiplyDeBruijnBitPosition[(v * 0x077CB531UL) >> 27];
      }

      inline void insert(int key, const bSpyStringHashed& value)
      {
        for (unsigned int i=0; i<32; i++)
        {
          if ((m_usage & (1 << i)) == 0)
          {
            m_keys[i] = key;
            m_values[i] = value;
            m_usage |= (1 << i);
            return;
          }
        }
        FastAssert(0);
      }

    };

    inline void deleteChunk(HChunk* chunk)
    {
#ifdef NMTL_CHUNKYHASH_STATS
        ++ m_dbgFrees;
#endif // NMTL_CHUNKYHASH_STATS

      m_artMemoryManager->deallocate(chunk, NM_MEMORY_TRACKING_ARGS);
    }

    inline HChunk* createChunk()
    {
#ifdef NMTL_CHUNKYHASH_STATS
        ++ m_dbgAllocations;
#endif // NMTL_CHUNKYHASH_STATS

      HChunk *newChunk = (HChunk*)m_artMemoryManager->allocate(sizeof(HChunk), NM_MEMORY_TRACKING_ARGS);
      newChunk->m_usage = 0;
      newChunk->m_prev = 0;
      newChunk->m_next = 0;

      return newChunk;
    }


    ART::MemoryManager    *m_artMemoryManager;

    // hash table storage
    HChunk                 *m_buckets[TABLESIZE];

#ifdef NMTL_CHUNKYHASH_STATS
    unsigned int            m_dbgInserts,               // number of calls to insert()
                            m_dbgErases,                //    "         "     erase()
                            m_dbgAllocations,           // number of new Chunk objects malloc'd
                            m_dbgFrees,                 // number of Chunks free'd
                            m_dbgCollisions,            // number of times a Chunk is chained to another (eg. collided with a filled Chunk)
                            m_dbgAddedRoot,             // number of times a new root Chunk is added
                            m_dbgRemovedRoot,           // number of times an empty root chunk was dismissed
                            m_dbgReusedDismissedChunk;  // number of times the dismissal stack was used to avoid allocation
#endif // NMTL_CHUNKYHASH_STATS
  };

  bSpyStringMap::bSpyStringMap(ART::MemoryManager* services) :
    m_artMemoryManager(services)
#ifdef NMTL_CHUNKYHASH_STATS
    ,m_dbgInserts(0),
    m_dbgErases(0),
    m_dbgAllocations(0),
    m_dbgFrees(0),
    m_dbgCollisions(0),
    m_dbgAddedRoot(0),
    m_dbgRemovedRoot(0),
    m_dbgReusedDismissedChunk(0)
#endif // NMTL_CHUNKYHASH_STATS
  {
    memset(m_buckets, 0, sizeof(HChunk*) * TABLESIZE);
  }

  bSpyStringMap::~bSpyStringMap()
  {
    deleteAll();
  }

  void bSpyStringMap::deleteAll()
  {
    for (unsigned int i=0; i<TABLESIZE; i++)
    {
      HChunk* chunk = m_buckets[i];
      while (chunk)
      {
        HChunk* nextChunk = chunk->m_next;
        deleteChunk(chunk);
        chunk = nextChunk;
      }
    }
  }

  void bSpyStringMap::insert(int key, const bSpyStringHashed& value)
  {
    unsigned int bkIndex = hashFunction(key);
    bkIndex &= (TABLESIZE - 1);

    HChunk* chunk = m_buckets[bkIndex];

#ifdef NMTL_CHUNKYHASH_STATS
    ++ m_dbgInserts;
#endif // NMTL_CHUNKYHASH_STATS

    if (chunk == 0)
    {
#ifdef NMTL_CHUNKYHASH_STATS
      ++ m_dbgAddedRoot;
#endif // NMTL_CHUNKYHASH_STATS
      chunk = (m_buckets[bkIndex] = createChunk());
    }
    else
    {
      while (chunk->m_usage == 0xFFFFFFFF)
      {
        if (chunk->m_next)
        {
          chunk = chunk->m_next;
        }
        else
        {
#ifdef NMTL_CHUNKYHASH_STATS
          ++ m_dbgCollisions;
#endif // NMTL_CHUNKYHASH_STATS

          HChunk* newChunk = createChunk();

          // push new chunk to the start of the chain where
          // it can be used immediately
          newChunk->m_next = m_buckets[bkIndex];
          m_buckets[bkIndex]->m_prev = newChunk;

          m_buckets[bkIndex] = newChunk;

          chunk = newChunk;
          break;
        }
      }
    }

    chunk->insert(key, value);
  }

  bool bSpyStringMap::find(int key, bSpyStringHashed& value)
  {
    unsigned int bkIndex = hashFunction(key);
    bkIndex &= (TABLESIZE - 1);

    HChunk* chunk = m_buckets[bkIndex];

    while (chunk && chunk->m_usage != 0)
    {
      unsigned int i = 0, highBit = 32; 
      if (chunk->m_usage != 0xFFFFFFFF)
        highBit = chunk->findHighestSetBit();

      for (; i<=highBit; ++i)
      {
        if ((chunk->m_usage & (1 << i)))
        {
          if (chunk->m_keys[i] == key)
          {
            value = chunk->m_values[i];
            return true;
          }
        }
      }

      chunk = chunk->m_next;
    }

    return false;
  }

  void bSpyStringMap::iterate(iterateFn fn)
  {
    for (int bkIndex=0; bkIndex<TABLESIZE; bkIndex++)
    {
      HChunk* chunk = m_buckets[bkIndex];

      while (chunk && chunk->m_usage != 0)
      {
        unsigned int i = 0, highBit = 32; 
        if (chunk->m_usage != 0xFFFFFFFF)
          highBit = chunk->findHighestSetBit();

        for (; i<=highBit; ++i)
        {
          if ((chunk->m_usage & (1 << i)))
          {
            fn(chunk->m_keys[i], chunk->m_values[i]);
          }
        }

        chunk = chunk->m_next;
      }
    }
  }

  bSpy::bSpyVec3      bSpyVec3fromVector3(const rage::Vector3& vec);
  bSpy::bSpyMat34     bSpyMat34fromMatrix34(const rage::Matrix34& tm);


  // -------------------------------------------------------------------------------------------------------------------
  // macros that define functions for sending TaskVar and CharacterUpdateVar packets
  // -------------------------------------------------------------------------------------------------------------------

  #ifdef _MSC_VER
  #  define BSPY_FORCEINLINE __forceinline
  #elif defined(__GCC__)
  # ifdef _DEBUG
  #  define BSPY_FORCEINLINE inline
  # else
  #  define BSPY_FORCEINLINE inline __attribute((always_inline))
  # endif
  #else
  # define BSPY_FORCEINLINE inline
  #endif

#define BSPY_DEF_VARFN_BASIC(_varN, _pktN, _type, _typeMemb) \
  BSPY_FORCEINLINE void do_##_varN(_type mbv, const char* mbvName, bool isParameter) \
  { \
    _pktN pp(bSpy::bSpyValueUnion::pt_##_type, isParameter);\
    pp.m_data.m_value._typeMemb = mbv;\
    pp.m_data.m_name = ART::bSpyServer::inst()->getTokenForString(mbvName);\
    bspySendPacket(pp); \
  }

#define BSPY_DEF_VARFN_STRING(_varN, _pktN) \
  BSPY_FORCEINLINE void do_##_varN(const char* mbv, const char* mbvName, bool isParameter) \
  { \
    _pktN pp(bSpy::bSpyValueUnion::pt_string, isParameter);\
    pp.m_data.m_value.s = ART::bSpyServer::inst()->getTokenForString(mbv);\
    pp.m_data.m_name = ART::bSpyServer::inst()->getTokenForString(mbvName);\
    bspySendPacket(pp); \
  }

#define BSPY_DEF_VARFN_VECTOR3(_varN, _pktN) \
  BSPY_FORCEINLINE void do_##_varN(const rage::Vector3& mbv, const char* mbvName, bool isParameter) \
  { \
    _pktN pp(bSpy::bSpyValueUnion::pt_vector3, isParameter);\
    pp.m_data.m_value.v = ART::bSpyVec3fromVector3(mbv);\
    pp.m_data.m_name = ART::bSpyServer::inst()->getTokenForString(mbvName);\
    bspySendPacket(pp); \
  }

  BSPY_DEF_VARFN_BASIC(bspyTaskVar, bSpy::TaskVarPacket, float, f);
  BSPY_DEF_VARFN_BASIC(bspyTaskVar, bSpy::TaskVarPacket, int, i);
  BSPY_DEF_VARFN_BASIC(bspyTaskVar, bSpy::TaskVarPacket, bool, b);
  BSPY_DEF_VARFN_STRING(bspyTaskVar, bSpy::TaskVarPacket);
  BSPY_DEF_VARFN_VECTOR3(bspyTaskVar, bSpy::TaskVarPacket);

#define bspyTaskVar(mbv, isParameter) do_bspyTaskVar(mbv, #mbv, isParameter);

#define bspyTaskVar_Bitfield32(mbv, isParameter) \
  {\
    bSpy::TaskVarPacket pp(bSpy::bSpyValueUnion::pt_bitfield32, isParameter);\
    pp.m_data.m_value.i = mbv;\
    pp.m_data.m_name = ART::bSpyServer::inst()->getTokenForString(#mbv);\
    bspySendPacket(pp); \
  }

#define bspyTaskVar_StringEnum(mbv, mbvStrings, isParameter) \
  {\
    if ((mbv >=0) && (mbv <= sizeof(mbvStrings) / sizeof(const char*)))\
    {\
    bSpy::TaskVarPacket pp(bSpy::bSpyValueUnion::pt_string, isParameter);\
    pp.m_data.m_value.s = ART::bSpyServer::inst()->getTokenForString(mbvStrings[mbv]);\
    pp.m_data.m_name = ART::bSpyServer::inst()->getTokenForString(#mbv);\
    bspySendPacket(pp); \
  }\
  else\
    bspyTaskVar(mbv, isParameter);\
  }

#define bspyTaskVar_Mat34(mbv, isParameter) \
  {\
    bSpy::TaskMatrixVarPacket mtv(isParameter);\
    mtv.m_matrix = bSpyMat34fromMatrix34(mbv);\
    mtv.m_name = ART::bSpyServer::inst()->getTokenForString(#mbv);\
    bspySendPacket(mtv); \
  }

  // ------------------------------------------------------------------
  // 
  // ------------------------------------------------------------------

  #define bspyNoAgent         (-3)
  #define bspyLastSeenAgent   (-2)

#define BSPY_DEF_SP_VARFN_BASIC(_type, _typeMemb) \
  BSPY_FORCEINLINE void _bspyScratchpadSetValue(bSpy::ScratchpadPacket &spp, _type mbv) \
  { \
    spp.m_data.m_type = bSpy::bSpyValueUnion::pt_##_type;\
    spp.m_data.m_value._typeMemb = mbv;\
  }

  BSPY_FORCEINLINE void _bspyScratchpadSetValue(bSpy::ScratchpadPacket &spp, const char* mbv) 
  {
    spp.m_data.m_type = bSpy::bSpyValueUnion::pt_string;
    spp.m_data.m_value.s = ART::bSpyServer::inst()->getTokenForString(mbv);
  }

  BSPY_FORCEINLINE void _bspyScratchpadSetValue(bSpy::ScratchpadPacket &spp, const rage::Vector3& mbv) 
  {
    spp.m_data.m_type = bSpy::bSpyValueUnion::pt_vector3;
    spp.m_data.m_value.v = ART::bSpyVec3fromVector3(mbv);
  }

  BSPY_DEF_SP_VARFN_BASIC(float, f);
  BSPY_DEF_SP_VARFN_BASIC(int, i);
  BSPY_DEF_SP_VARFN_BASIC(bool, b);

  #define bspyTaskScratchpad(_taskID, _tag, _value) \
  if (ART::bSpyServer::inst() && ART::bSpyServer::inst()->isClientConnected() && ART::bSpyServer::inst()->shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_Scratchpad)) \
  {\
    static bSpy::bs_uint8 __callstamp = 0;\
    static bSpy::bs_uint32 __framestamp = 0;\
    if (__framestamp != ART::bSpyServer::inst()->getFrameTicker())\
    { __callstamp = 0; __framestamp = ART::bSpyServer::inst()->getFrameTicker(); }\
    bSpy::ScratchpadPacket spp((bSpy::bs_int8)ART::bSpyServer::inst()->getLastSeenAgent());\
    _bspyScratchpadSetValue(spp, _value);\
    spp.m_data.m_name = ART::bSpyServer::inst()->getTokenForString(#_value);\
    spp.m_tag = ART::bSpyServer::inst()->getTokenForString(_tag);\
    spp.m_lineNumber = (bSpy::bs_uint16)__LINE__;\
    spp.m_callStamp = __callstamp++;\
    spp.m_taskID = (bs_int8)_taskID;\
    bspySendPacket(spp); \
  }

  #define bspyTaskScratchpadAuto(_tag, _value)  bspyTaskScratchpad(getBvID(), _tag, _value)


  #define bspyScratchpad(_agentID, _tag, _value) \
  if (ART::bSpyServer::inst() && ART::bSpyServer::inst()->isClientConnected() && ART::bSpyServer::inst()->shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_Scratchpad) && (bSpy::bs_int8)_agentID != (bSpy::bs_int8)INVALID_AGENTID)\
  {\
    static bSpy::bs_uint8 __callstamp = 0;\
    static bSpy::bs_uint32 __framestamp = 0;\
    if (__framestamp != ART::bSpyServer::inst()->getFrameTicker())\
    { __callstamp = 0; __framestamp = ART::bSpyServer::inst()->getFrameTicker(); }\
    bSpy::ScratchpadPacket spp((bSpy::bs_int8)(((bSpy::bs_int8)_agentID == bspyLastSeenAgent)?(bSpy::bs_int8)ART::bSpyServer::inst()->getLastSeenAgent():(bSpy::bs_int8)_agentID));\
    _bspyScratchpadSetValue(spp, _value);\
    spp.m_data.m_name = ART::bSpyServer::inst()->getTokenForString(#_value);\
    spp.m_tag = ART::bSpyServer::inst()->getTokenForString(_tag);\
    spp.m_lineNumber = (bSpy::bs_uint16)__LINE__;\
    spp.m_callStamp = __callstamp++;\
    bspySendPacket(spp); \
  }

  #define bspyScratchpad_Bitfield32(_agentID, _tag, _value) \
  if (ART::bSpyServer::inst() && ART::bSpyServer::inst()->isClientConnected() && ART::bSpyServer::inst()->shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_Scratchpad) && (bSpy::bs_int8)_agentID != (bSpy::bs_int8)INVALID_AGENTID)\
  {\
    static bSpy::bs_uint8 __callstamp = 0;\
    static bSpy::bs_uint32 __framestamp = 0;\
    if (__framestamp != ART::bSpyServer::inst()->getFrameTicker())\
    { __callstamp = 0; __framestamp = ART::bSpyServer::inst()->getFrameTicker(); }\
    bSpy::ScratchpadPacket spp((bSpy::bs_int8)(((bSpy::bs_int8)_agentID == bspyLastSeenAgent)?(bSpy::bs_int8)ART::bSpyServer::inst()->getLastSeenAgent():(bSpy::bs_int8)_agentID));\
    spp.m_data.m_type = bSpy::bSpyValueUnion::pt_bitfield32;\
    spp.m_data.m_value.u = _value;\
    spp.m_data.m_name = ART::bSpyServer::inst()->getTokenForString(#_value);\
    spp.m_tag = ART::bSpyServer::inst()->getTokenForString(_tag);\
    spp.m_lineNumber = (bSpy::bs_uint16)__LINE__;\
    spp.m_callStamp = __callstamp++;\
    bspySendPacket(spp); \
  }

  // ------------------------------------------------------------------
  // simplified way to log messages via bSpyLogStub()
  // eg, bspyLogf(info, L"stuff %i", foo);
  #define bspyLogf(_type, ...) ART::bSpyServer::inst()->bSpyLogFormat(false, false, bSpy::LogStringPacket::lt_##_type, __FILE__, __LINE__, __VA_ARGS__);
  #define bspyLogf_ZF(_type, ...) ART::bSpyServer::inst()->bSpyLogFormat(true, false, bSpy::LogStringPacket::lt_##_type, __FILE__, __LINE__, __VA_ARGS__);

  // variant that can be used outside of an active connection, if required; bear in mind any temporal
  // context will be lost, all entries will just be grouped together in the PC buffer
  #define bspyCachedLogf(_type, ...) ART::bSpyServer::inst()->bSpyLogFormat(false, true, bSpy::LogStringPacket::lt_##_type, __FILE__, __LINE__, __VA_ARGS__);
  #define bspyCachedLogf_ZF(_type, ...) ART::bSpyServer::inst()->bSpyLogFormat(true, true, bSpy::LogStringPacket::lt_##_type, __FILE__, __LINE__, __VA_ARGS__);


  // ------------------------------------------------------------------
  // helper for sending memory usage records to bSpy
  #define bspyMemoryUsageRecord(_name, _amount) \
  {\
    bSpy::MemoryUsageRecordPacket mup(ART::bSpyServer::inst()->getTokenForString(_name), (bSpy::bs_uint32)_amount);\
    bspySendOrPrecachePacket(mup); \
  }

  // ------------------------------------------------------------------
  // 
  #define bspyProfileStart(_name) \
  {\
    bSpy::ProfilerPacket pp(ART::bSpyServer::inst()->getTokenForString(_name), (bSpy::bs_int8)ART::bSpyServer::inst()->getLastSeenAgent(), ART::bSpyServer::inst()->getMSTimeSinceBeginStep(), true);\
    bspySendOrPrecachePacket(pp); \
  }
  #define bspyProfileEnd(_name) \
  {\
    bSpy::ProfilerPacket pp(ART::bSpyServer::inst()->getTokenForString(_name), (bSpy::bs_int8)ART::bSpyServer::inst()->getLastSeenAgent(), ART::bSpyServer::inst()->getMSTimeSinceBeginStep(), false);\
    bspySendOrPrecachePacket(pp); \
  }

  class bspyScopedPP
  {
  public:
    bspyScopedPP(const char* name);
    ~bspyScopedPP();
  protected:
    bSpy::bSpyStringToken   m_name;
    bSpy::bs_int8           m_agent;
  };
  #define bspyScopedProfilePoint(_name) ART::bspyScopedPP _pp_##__LINE__(_name);

} // namespace ART




#endif // ART_ENABLE_BSPY

#endif // NM_ART_BSPY_SERVER
