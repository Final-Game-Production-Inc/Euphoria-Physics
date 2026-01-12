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

#if __WIN32
#ifdef _DURANGO
#define _WIN32_WINNT 0x502
#else
#define _WIN32_WINNT 0x500
#endif
#include "system/xtl.h"
#endif // __WIN32

#include "ART\bSpyServer.h"

//So that the gameLibs can be compiled with ART_ENABLE_BSPY 1 
// and still link with NM code if ART_ENABLE_BSPY 0 (e.g. for profiling)
// Bspy functions called by the gamelibs are defined here but do nothing
#if (!ART_ENABLE_BSPY) && NM_EMPTY_BSPYSERVER
namespace ART
{
  bSpyServer* bSpyServer::inst()
  {
    return NULL;
  }
  bSpyServer::~bSpyServer()
  {
  }
  bool bSpyServer::sendPacket(bSpy::PacketBase& /*pkt*/)
  {
    return false;
  }
  bSpy::bSpyStringToken bSpyServer::getTokenForString(const char* /*str*/)
  {
    return (bSpy::bSpyStringToken) 0;
  }
  const char* bSpyServer::getTokenForString(bSpy::bSpyStringToken)
  {
    return 0;
  }
  bSpy::bSpyVec3 bSpyVec3fromVector3(const rage::Vector3& /*vec*/)
  {
    bSpy::bSpyVec3 out;

    out.v[0] = 0.0f;
    out.v[1] = 0.0f;
    out.v[2] = 0.0f;

    return out;
  }

}
#endif//#if (!ART_ENABLE_BSPY) && NM_EMPTY_BSPYSERVER

#if ART_ENABLE_BSPY

#include "rockstar/NmRsEngine.h"
#include "rockstar/NmRsCharacter.h"

#include "bspy/bSpyCommonPackets.h"

#if defined(NM_PLATFORM_X360)
  #include "system/xtl.h"
  #include <winsockx.h>
  typedef int nmsock_socklen_t;
#elif defined(NM_PLATFORM_WIN32)
  #include <winsock.h>
  typedef int nmsock_socklen_t;
#elif defined(NM_PLATFORM_DURANGO)
  #include <ws2tcpip.h>
  typedef int nmsock_socklen_t;
#elif defined(NM_PLATFORM_CELL_PPU)
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <sys/select.h>
  #include <sys/ansi.h> // socklen_t definition
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netex/net.h>
  #include <netex/errno.h>
  typedef socklen_t nmsock_socklen_t;
#elif defined(NM_PLATFORM_ORBIS)
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <libnet/errno.h>
  #include <net.h>
  typedef socklen_t nmsock_socklen_t;
#else
  #error unknown platform
#endif
#include "system/timer.h"

#define ART_ENABLE_BSPY_LOGGING       1

// if ART_EXPORTS is defined, this will be a PC DLL build, most likely destined for
// e:studio and therefore we need bSpy to poll its connection socket every frame, otherwise
// we risk missing frames during simulation (unlike a game, where a second of latency is acceptable
// given the performance hit of listening every frame)
#ifdef ART_EXPORTS
# define BSPY_LISTEN_FRAME_INTERVAL    1
#else
# define BSPY_LISTEN_FRAME_INTERVAL    60
#endif // ART_EXPORTS

#define NM_BSPY_STRINGCACHE_CHUNK     (1024 * 32)

// define this to make the string cache heap manager spew out the number of allocs, footprint, heap stats, etc on dtor
#if 0
# define BSPY_CHUNKMEM_LOG_DTOR
#endif

namespace ART
{
  // -------------------------------------------------------------------------------------------------------------------
  bspyScopedPP::bspyScopedPP(const char* name)
  {
    BSPY_ISCLIENTCONNECTED_BEGIN()

    m_name = bSpyServer::inst()->getTokenForString(name);
    m_agent = (bs_int8)bSpyServer::inst()->getLastSeenAgent();

    bSpy::ProfilerPacket pp(
      m_name, 
      m_agent, 
      bSpyServer::inst()->getMSTimeSinceBeginStep(), 
      true);
    bspySendOrPrecachePacket(pp);

    BSPY_ISCLIENTCONNECTED_END()
  }

  // -------------------------------------------------------------------------------------------------------------------
  bspyScopedPP::~bspyScopedPP()
  {
    BSPY_ISCLIENTCONNECTED_BEGIN()

    bSpy::ProfilerPacket pp(
      m_name, 
      m_agent, 
      bSpyServer::inst()->getMSTimeSinceBeginStep(), 
      false);
    bspySendOrPrecachePacket(pp);

    BSPY_ISCLIENTCONNECTED_END()
  }


  bSpyServer *bSpyServer::s_instance = 0;

  // -------------------------------------------------------------------------------------------------------------------
  bSpyServer::bSpyServer(ART::MemoryManager* services) :
    m_artMemoryManager(services),
    m_inited(false),
    m_networkInited(false),
    m_serverSocket(INVALID_SOCKET),
    m_clientSocket(INVALID_SOCKET),
    m_readThread(sysIpcThreadIdInvalid),
    m_buffer(0),
    m_bufferSize(NM_BSPY_COMMBUF_SZ),
    m_bufferUsed(0),
    m_pcBuffer(0),
    m_pcBufferSize(NM_BSPY_PC_COMMBUF_SZ),
    m_pcBufferUsed(0),
    m_socketListenInterval(0),
    m_transmissionFlags((unsigned int)bSpy::TransmissionControlPacket::bSpyTF_All),
    m_lastSeenAgent(ART::INVALID_AGENTID),
    m_stringTokenIndex(0)
  {
    FastAssert(s_instance == 0);
    s_instance = this;

    ARTCustomPlacementNew(m_stringMap, bSpyStringMap);
    ARTCustomPlacementNew2Arg(m_stringCacheMMgr, bSpyChunkMemory, 0, NM_BSPY_STRINGCACHE_CHUNK);

    m_buffer = (unsigned char*)m_artMemoryManager->allocate(m_bufferSize, NM_MEMORY_TRACKING_ARGS);
    m_pcBuffer = (unsigned char*)m_artMemoryManager->allocate(m_pcBufferSize, NM_MEMORY_TRACKING_ARGS);

    m_frameTicker = 0;
    m_sessionUID = (size_t)this;

    ARTCustomPlacementNewNoService(m_perfTimer, rage::sysTimer);

#if __WIN32
    m_fileToSerializeToPending = 0;
    m_fileToSerializeTo = 0;
#endif // __WIN32
  }

  bSpyServer::~bSpyServer()
  {
    FastAssert(s_instance == this);
    s_instance = 0;

    ARTCustomPlacementDelete(m_perfTimer, sysTimer);

#if __WIN32
    m_fileToSerializeToPending = 0;
    m_fileToSerializeTo = 0;
#endif // __WIN32

    m_artMemoryManager->deallocate(m_pcBuffer, NM_MEMORY_TRACKING_ARGS);
    m_pcBuffer = 0;

    m_artMemoryManager->deallocate(m_buffer, NM_MEMORY_TRACKING_ARGS);
    m_buffer = 0;

    m_artMemoryManager->deallocate(m_stringCacheMMgr, NM_MEMORY_TRACKING_ARGS);
    m_stringCacheMMgr = 0;

    ARTCustomPlacementDelete(m_stringMap, bSpyStringMap);
    m_stringMap = 0;

    m_buffer = 0;
    m_bufferSize = 0;
  }

  bSpyServer* bSpyServer::inst() 
  { 
    return s_instance; 
  }

  bool bSpyServer::init(bool initNetworking)
  {
    FastAssert(!m_inited);

    if (initNetworking)
    {
#if defined(NM_PLATFORM_CELL_PPU) || defined(NM_PLATFORM_ORBIS)

      // HDD not done yet

#else

# ifdef NM_PLATFORM_X360

      XNetStartupParams xnsp;
      memset(&xnsp, 0, sizeof(xnsp));

      xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
      xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;

      if (XNetStartup(&xnsp))
      {
#if ART_ENABLE_BSPY_LOGGING
        Displayf("NMLOG | bSpyServer: XNetStartup failed");
#endif // ART_ENABLE_BSPY_LOGGING
        return false;
      }

# endif // NM_PLATFORM_X360

      WORD versionRequested = MAKEWORD (2, 2);
      WSADATA wsaData;

      if (WSAStartup(versionRequested, & wsaData))
      {
#if ART_ENABLE_BSPY_LOGGING
        Displayf("NMLOG | bSpyServer: Winsock WSAStartup failed");
#endif // ART_ENABLE_BSPY_LOGGING
        WSACleanup();

        return false;
      }

      if (LOBYTE(wsaData.wVersion) != 2 ||
          HIBYTE(wsaData.wVersion) != 2)
      {
#if ART_ENABLE_BSPY_LOGGING
        Displayf("NMLOG | bSpyServer: Winsock 2.2 could not load");
#endif // ART_ENABLE_BSPY_LOGGING
        WSACleanup();

        return false;
      }

#endif // NM_PLATFORM_X360

      m_networkInited = true;
    }

    m_inited = true;
    return true;
  }

  void SpyReadThread(void* parameter)
  {
    bSpyServer *spy = (bSpyServer*)parameter;
    SOCKET clientSocket = spy->getClientSocket();
    EventHandle terminationEvent = spy->getTerminatorEvent();

    const int recvBufferSz = NM_BSPY_COMMBUF_SZ;
    char recvBuffer[NM_BSPY_COMMBUF_SZ];
    int recvOffset = 0, junkCounter = 0;

    while (!rage::sysIpcWaitSemaTimed(terminationEvent, 200))
    {
      if (junkCounter)
      {
        //bSpyNetControl::getInstance()->logf(EAT::Logger::kWarning, L"Junk in receive buffer (ran off end) - %i bytes", junkCounter);
        junkCounter = 0;
      }

      // read in from the socket;
      // if recvOffset != 0, we had a partial transmission in the last loop, meaning there will be
      // some extra data left at the start of the receive buffer. we buffer in behind that to (hopefully)
      // complete the data
      int amountRead = ::recv(clientSocket, (char*)&recvBuffer[recvOffset], recvBufferSz - recvOffset, 0);

      // add on recvOffset, if there was already data in the buffer
      amountRead += recvOffset;
      recvOffset = 0;


      // socket error, scrap the connection
      // read amount of 0 means a graceful close on target
      if (amountRead == SOCKET_ERROR || 
          amountRead == 0)
      {
        break;
      }
      else if (amountRead > 0)
      {
        // go slice out packets
        char* readPointer = recvBuffer;
        int dataRemaining = amountRead;

        while (dataRemaining > 0)
        {
          bSpy::PacketHeader pktHeader = *reinterpret_cast<bSpy::PacketHeader*>(readPointer);
#if BSPY_ENDIAN_SWAP
          pktHeader.endianSwap();
#endif // BSPY_ENDIAN_SWAP

          // handle the case where we have a tiny amount of data remaining; may need 
          // to do same as when we handle partial validated packet transmissions
          if (dataRemaining <= (int)sizeof(bSpy::PacketHeader))
          {
            // do these last remaining bytes hold the promise of a valid header?
            if ((pktHeader.m_magicA == NM_BSPY_PKT_MAGIC_A) && 
                 (dataRemaining == 1 || 
                 (pktHeader.m_magicB == NM_BSPY_PKT_MAGIC_STD || pktHeader.m_magicB == NM_BSPY_PKT_MAGIC_ZF)
                 )
                )
            {
              // copy the unfinished data back to the start of the receive buffer
              memcpy(recvBuffer, readPointer, dataRemaining);

              // adjust the reception point so that the next time recv is called, new data
              // will concatenate onto the partial
              recvOffset = dataRemaining;

              // abort this read block
              dataRemaining = 0;
              continue;           
            }
          }

          if (pktHeader.validMagic())
          {
            // did we spend time rifling through junk bytes before finding a packet?
            if (junkCounter)
            {
              //bSpyNetControl::getInstance()->logf(EAT::Logger::kWarning, L"Junk in receive buffer - %i bytes, %i bytes in [%i read]", junkCounter, amountRead - dataRemaining, amountRead);
              junkCounter = 0;
            }

            // ----------------------------------------------------------------------------------
            // packet length invalid? 
            if (pktHeader.m_length < sizeof(bSpy::PacketHeader) ||
                pktHeader.m_length > NM_BSPY_PKT_MAX_SIZE)
            {
              //bSpyNetControl::getInstance()->logf(EAT::Logger::kError, L"Packet received with invalid length specified (or damaged data) (len=%i) (id=%i)", pktHeader->m_length, pktHeader->m_id);
            }
            // ----------------------------------------------------------------------------------
            // we have a partial packet - need to stop processing, save the partial data and loop around again to 
            // hopefully receive the remainder
            else if (pktHeader.m_length > dataRemaining)
            {
              // copy the unfinished data back to the start of the receive buffer
              memcpy(recvBuffer, readPointer, dataRemaining);

              // adjust the reception point so that the next time recv is called, new data
              // will concatenate onto the partial
              recvOffset = dataRemaining;

              // abort this read block
              dataRemaining = 0;
              continue;
            }
            // ----------------------------------------------------------------------------------
            // we have a working, complete packet to look at
            else
            {
              bSpy::PacketBase* packet = reinterpret_cast<bSpy::PacketBase*>(readPointer);

              // internal packet ID
              if (pktHeader.m_id > bSpy::bspy_pk_internal && 
                  pktHeader.m_id < bSpy::bspy_pk_user)
              {
                switch (pktHeader.m_id)
                {
                  case bSpy::bspy_pk_TransmissionControl:
                    {                      
                      bSpy::TransmissionControlPacket tc = *reinterpret_cast<const bSpy::TransmissionControlPacket*>(packet);
#if BSPY_ENDIAN_SWAP
                      tc.endianSwap();
#endif // BSPY_ENDIAN_SWAP

//For some reason this crashes
//#if ART_ENABLE_BSPY_LOGGING
////                      Displayf("NMLOG | bSpyServer: TransmissionControl" 0x%08X", tc.m_txFlags);
//#endif // ART_ENABLE_BSPY_LOGGING

                      spy->setTransmissionFlags(tc.m_txFlags);
                    }
                    break;

                  case bSpy::bspy_pk_Ping:
                    {
                    }
                    break;
                }
              }
              // invalid ID
              else
              {
                // bSpyNetControl::getInstance()->logf(EAT::Logger::kError, L"Packet received with invalid ID (%i)", pktHeader->m_id);
              }

              // skip past packet
              readPointer += pktHeader.m_length;
              dataRemaining -= pktHeader.m_length;
              continue;
            }
          }
          else
          {
            junkCounter ++;
          }

          // fall-through to stepping byte-by-byte trying to find valid packets
          ++ readPointer;
          -- dataRemaining;
        }
      }

    }

  }

  bool bSpyServer::startServer()
  {
    FastAssert(m_inited);

    int ret;
    // create socket
#if defined(NM_PLATFORM_DURANGO)
	struct sockaddr_in6 ourAddr;
	memset(&ourAddr, 0, sizeof(ourAddr));
    ourAddr.sin6_family = AF_INET6;
    ourAddr.sin6_port   = htons(NM_BSPY_PORT);
    ourAddr.sin6_addr   = in6addr_any;

    m_serverSocket = nmsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
#else
    struct sockaddr_in ourAddr;
    memset(&ourAddr, 0, sizeof(ourAddr));
    ourAddr.sin_family = AF_INET;
    ourAddr.sin_port   = htons(NM_BSPY_PORT);
    ourAddr.sin_addr.s_addr = INADDR_ANY;

    m_serverSocket = nmsock_socket(AF_INET, SOCK_STREAM, 0);
#endif // NM_PLATFORM_DURANGO

    if ((int)m_serverSocket < 0)
    {
#if ART_ENABLE_BSPY_LOGGING
      Displayf("NMLOG | bSpyServer: unable to create socket (error: %d)", nmsock_error);
#endif // ART_ENABLE_BSPY_LOGGING
      return false;
    }

#if defined(__PPU__) || defined(NM_PLATFORM_ORBIS)
    int optOn = 1;
    ret = nmsock_setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &optOn, sizeof(int));
#else
    const char on = 1;
    ret = nmsock_setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#endif

    if (ret != 0)
    {
#if ART_ENABLE_BSPY_LOGGING
      Displayf("NMLOG | bSpyServer: setsockopt failed (error: %d)", nmsock_error);
#endif // ART_ENABLE_BSPY_LOGGING

	  nmsock_close(m_serverSocket);

      return false;
    }

#ifdef NM_PLATFORM_DURANGO
    // disable IPv6-only mode, we support IPv4-mapped IPv6 addresses
    int v6only = 0;
    ret = nmsock_setsockopt(m_serverSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&v6only, sizeof(v6only));

	if (ret < 0)
    {
      nmsock_close(m_serverSocket);

#if ART_ENABLE_BSPY_LOGGING
      Displayf("NMLOG | bSpyServer: could not disable IPv6-only mode (error: %d)", nmsock_error);
#endif // ART_ENABLE_BSPY_LOGGING

      return false;
    }
#endif

    int rcvBufSize = NM_BSPY_COMMBUF_SZ;
    nmsock_setsockopt(m_serverSocket, SOL_SOCKET, SO_RCVBUF, ( char * )&rcvBufSize, sizeof(rcvBufSize) );

    int sndBufSize = NM_BSPY_COMMBUF_SZ;
    nmsock_setsockopt(m_serverSocket, SOL_SOCKET, SO_SNDBUF, ( char * ) &sndBufSize, sizeof(sndBufSize) );

	ret = nmsock_bind(m_serverSocket, (sockaddr* )&ourAddr, sizeof(ourAddr));
    if (ret < 0)
    {
      nmsock_close(m_serverSocket);

#if ART_ENABLE_BSPY_LOGGING
      Displayf("NMLOG | bSpyServer: could not bind socket");
#endif // ART_ENABLE_BSPY_LOGGING

      return false;
    }

    ret = nmsock_listen(m_serverSocket, 1);
    if (ret < 0)
    {
      nmsock_close(m_serverSocket);

#if ART_ENABLE_BSPY_LOGGING
      Displayf("NMLOG | bSpyServer: could not listen on server socket");
#endif // ART_ENABLE_BSPY_LOGGING
      return false;
    }     

#ifdef NM_PLATFORM_X360
    XNADDR xnAddr;
    while (XNetGetTitleXnAddr(&xnAddr) == XNET_GET_XNADDR_PENDING)
    {
      Sleep(100);
    }

# if ART_ENABLE_BSPY_LOGGING
    unsigned char *ucp = (unsigned char *)&xnAddr.ina;
    Displayf("NMLOG | bSpyServer: X360 server IP: %d.%d.%d.%d", ucp[0] & 0xff, ucp[1] & 0xff, ucp[2] & 0xff, ucp[3] & 0xff);
# endif // ART_ENABLE_BSPY_LOGGING
#endif // NM_PLATFORM_X360

#if ART_ENABLE_BSPY_LOGGING
    Displayf("NMLOG | bSpyServer: server started on port %i", NM_BSPY_PORT);
#endif // ART_ENABLE_BSPY_LOGGING

    return true;
  }

  void bSpyServer::stopServer()
  {
    shutdownClient();

    if (m_serverSocket != INVALID_SOCKET)
    {
      nmsock_shutdown(m_serverSocket, 0x02);
      nmsock_close(m_serverSocket);
    }

    m_serverSocket = INVALID_SOCKET;
  }

  void bSpyServer::shutdownClient()
  {
    if (m_clientSocket == INVALID_SOCKET)
    {
      FastAssert(m_readThread == sysIpcThreadIdInvalid);
      return;
    }

    if (m_readThread != sysIpcThreadIdInvalid)
    {
      rage::sysIpcSignalSema(m_readThreadTerminator);
	  rage::sysIpcWaitThreadExit(m_readThread);
	  rage::sysIpcDeleteSema(m_readThreadTerminator);

      m_readThread = sysIpcThreadIdInvalid;
	  m_readThreadTerminator = NULL;
    }

    if (m_clientSocket != INVALID_SOCKET)
    {
      nmsock_shutdown(m_clientSocket, 0x02);
      nmsock_close(m_clientSocket);
    }
    m_clientSocket = INVALID_SOCKET;
  }

  void bSpyServer::beginStep(float)
  {
    m_frameTicker++;
    m_perfTimer->Reset();

    bool newSession = false;

    // listen for connection if we don't already have a client
    if (!isNetworkClientConnected() 
#if __WIN32
      && m_fileToSerializeTo == 0
#endif // __WIN32
      )
    {
      if (m_socketListenInterval <= 0)
      {
        m_socketListenInterval = BSPY_LISTEN_FRAME_INTERVAL;

        fd_set readSockets;
        struct timeval timeout;

        FD_ZERO(&readSockets);
        FD_SET(m_serverSocket, &readSockets);

        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

		// Even though sizeof(SOCKET) is 8 on _WIN64 platforms, it's safe to cast it to int, 
		// because the value constitutes an index in per-process table of limited size
		// and not a real pointer.
		// Kernel object handles are process specific.
		// That is, a process must either create the object or open an existing object to obtain a kernel object handle. 
		// The per-process limit on kernel handles is 2^24.
        int result = nmsock_select((int)(m_serverSocket + 1), &readSockets, 0, 0, &timeout);

		if (result == -1)
		{
#if ART_ENABLE_BSPY_LOGGING
          Warningf("NMLOG Warning| bSpyServer: Wait for socket - (error: %d)", nmsock_error);
#endif // ART_ENABLE_BSPY_LOGGING
		}
		else if (result != 0)
		{
#if ART_ENABLE_BSPY_LOGGING
          Warningf("NMLOG Warning| bSpyServer: Wait for socket succeeded");
#endif // ART_ENABLE_BSPY_LOGGING
		}

        if (FD_ISSET(m_serverSocket, &readSockets))
        {
#if defined(NM_PLATFORM_DURANGO)
          struct sockaddr_in6 clientAddr;
#else
          struct sockaddr_in clientAddr;
#endif
          int clientAddrLen = sizeof(clientAddr);

#if ART_ENABLE_BSPY_LOGGING
          Displayf("NMLOG | bSpyServer: server has new connection...");
#endif // ART_ENABLE_BSPY_LOGGING

          m_clientSocket = nmsock_accept(m_serverSocket, (sockaddr *)&clientAddr, (nmsock_socklen_t *)&clientAddrLen);

		  // manual reset flag + initially non-signaled
          FastAssert(m_readThread == sysIpcThreadIdInvalid);

		  m_readThreadTerminator = rage::sysIpcCreateSema(0);
		  if (m_readThreadTerminator)
		  {
            m_readThread = rage::sysIpcCreateThread(SpyReadThread, this, NM_BSPY_COMMBUF_SZ + (2 * 1024), rage::PRIO_NORMAL, "SpyReadThread");

			FastAssert(m_readThread != sysIpcThreadIdInvalid);
		  }

          newSession = true;
        }
      }
      else
      {
        m_socketListenInterval --;
      }
    }

#if __WIN32
    // check for new serializer 'client'
    if (m_fileToSerializeToPending)
    {
      m_fileToSerializeTo = m_fileToSerializeToPending;
      m_fileToSerializeToPending = 0;

#if ART_ENABLE_BSPY_LOGGING
      Displayf("NMLOG | bSpyServer: traffic serializer activated [%p]", m_fileToSerializeTo);
#endif // ART_ENABLE_BSPY_LOGGING

      // flag up a new session if network hasn't already
      if (!isNetworkClientConnected())
      {
#if ART_ENABLE_BSPY_LOGGING
        Displayf("NMLOG | bSpyServer: no network client connected - file serializer inhibits futher network connections");
#endif // ART_ENABLE_BSPY_LOGGING

        stopServer();
        newSession = true;
      }
    }
#endif // __WIN32

    if (newSession)
    {
      // nuke the buffer, ready for use
      m_bufferUsed = 0;

      // create a new unique ID per session
      m_sessionUID ++;

#if ART_ENABLE_BSPY_LOGGING
      Displayf("NMLOG | bSpyServer: session UID [%" SIZETFMT "u]", m_sessionUID);
#endif // ART_ENABLE_BSPY_LOGGING

      // precache these for use when sending back our standard set of animation streams (the ITMs)
      m_defaultITMStreamLabels[ART::kITSourceCurrent] = getTokenForString("ITM-Current");
      m_defaultITMStreamLabels[ART::kITSourcePrevious] = getTokenForString("ITM-Previous");
#if NM_ANIM_MATRICES
      m_defaultITMStreamLabels[ART::kITSourceAnimation] = getTokenForString("ITM-Animation");
#endif // NM_ANIM_MATRICES

      // send character descriptors, etc
      // this is a pure virtual call, derived class provides the functionality
      onNewConnection();
    }
    else
    {
      // purge the send buffer
      sendBuffer();
    }
  }

  void bSpyServer::endStep(float)
  {
    sendBuffer();
  }

  void bSpyServer::term()
  {
    if (m_inited)
    {
      FastAssert(!isServerRunning());

      if (m_networkInited)
      {
#if defined(NM_PLATFORM_CELL_PPU) || defined(NM_PLATFORM_ORBIS)

        // HDD not done yet

#else // NM_PLATFORM_CELL_PPU

        WSACleanup();

#ifdef NM_PLATFORM_X360
        XNetCleanup();
#endif // NM_PLATFORM_X360

#endif

        m_networkInited = false;
      }

      m_inited = false;
    }
  }

  bool bSpyServer::sendPacket(bSpy::PacketBase& pkt)
  {
    FastAssert(m_buffer);
    FastAssert(m_bufferSize);

    // check we have space in the buffer for this packet?
    if (m_bufferUsed + pkt.hdr.m_length >= m_bufferSize)
    {
      // send the current buffer and reset!
      sendBuffer();
      m_bufferUsed = 0;
    }
    
    // validate packet contents
    unsigned int pktLen = (unsigned int)pkt.hdr.m_length;
    const char *dPt = (const char*)&pkt;

	Assert(pktLen <= NM_BSPY_PKT_MAX_SIZE);
	if (pktLen > NM_BSPY_PKT_MAX_SIZE)
	{
		return false;
	}

    FastAssert(pktLen >= sizeof(bSpy::PacketHeader) && pktLen < NM_BSPY_PKT_MAX_SIZE);
    FastAssert(pkt.hdr.m_id > bSpy::bspy_pk_internal && pkt.hdr.m_id < ART::bspy_pk_nmrs_top);

    // swap the header if required (rest of packet is swapped by owner)
#if BSPY_ENDIAN_SWAP
    pkt.hdr.endianSwap();
#endif // BSPY_ENDIAN_SWAP

    unsigned char* bufPtr = &m_buffer[m_bufferUsed];
	memcpy(bufPtr, dPt, pktLen);
    m_bufferUsed += pktLen;
    
    return true;
  }

  bool bSpyServer::sendOrPrecachePacket(bSpy::PacketBase& pkt)
  {
    // if no connection yet, we cache packets for later transmission (up to a fixed space limit)
    if (!isClientConnected())
    {
      FastAssert(m_pcBuffer);
      FastAssert(m_pcBufferSize);

      // no more space left in the PC buffer.. can't cache this packet!
      if (m_pcBufferUsed + pkt.hdr.m_length > m_pcBufferSize)
      {
        return false;
      }

      // validate packet contents
      unsigned int pktLen = (unsigned int)pkt.hdr.m_length;
      const char *dPt = (const char*)&pkt;
      FastAssert(pktLen >= sizeof(bSpy::PacketHeader));
      FastAssert(pkt.hdr.m_id > bSpy::bspy_pk_internal && pkt.hdr.m_id < bSpy::bspy_pk_top);

      // swap the header if required (rest of packet is swapped by owner)
#if BSPY_ENDIAN_SWAP
      pkt.hdr.endianSwap();
#endif // BSPY_ENDIAN_SWAP

      unsigned char* bufPtr = &m_pcBuffer[m_pcBufferUsed];
      memcpy(bufPtr, dPt, pktLen);
      m_pcBufferUsed += pktLen;
    }
    else 
    {
      // if connected, revent to standard approach
      return sendPacket(pkt);
    }

    return true;
  }


  void bSpyServer::sendBuffer()
  {
#if __WIN32
    if (m_fileToSerializeTo)
    {
      if (m_bufferUsed > 0)
        fwrite(m_buffer, m_bufferUsed, 1, m_fileToSerializeTo);

      // no client? we consume the buffer regardless so that we keep accepting data from the runtime
      if (!isNetworkClientConnected())
        m_bufferUsed = 0;
    }
#endif // __WIN32

    if (!isNetworkClientConnected() || m_bufferUsed == 0)
      return;

    FastAssert(m_buffer);

    // send the buffer, one MTU at a time
    unsigned int toSend = m_bufferUsed;
    unsigned char* bufPtr = m_buffer;
    do 
    {
      int sent = nmsock_send(m_clientSocket, (const char*)bufPtr, toSend, 0);

      if (sent < 0)
      {
#if ART_ENABLE_BSPY_LOGGING
        Warningf("NMLOG Warning| bSpyServer: sendBuffer (%i bytes) - failed (error: %d), closing connection", m_bufferUsed, nmsock_error);
#endif // ART_ENABLE_BSPY_LOGGING

        shutdownClient();

		return;
      }

      toSend -= sent;
      bufPtr += sent;

    } while (toSend > 0);

    m_bufferUsed = 0;
  }

  void bSpyServer::sendPCBuffer()
  {
#if __WIN32
    if (m_fileToSerializeTo && m_pcBufferUsed > 0)
    {
      sendBuffer();

      fwrite(m_pcBuffer, m_pcBufferUsed, 1, m_fileToSerializeTo);
      return;
    }
#endif // __WIN32

    if (m_pcBufferUsed == 0)
      return;

    // to try and keep packet ordering consistent, we first flush the main buffer
    sendBuffer();

    FastAssert(isNetworkClientConnected());
    FastAssert(m_buffer);

    // send the buffer, one MTU at a time
    unsigned int toSend = m_pcBufferUsed;
    unsigned char* bufPtr = m_pcBuffer;
    do 
    {
      int sent = nmsock_send(m_clientSocket, (const char*)bufPtr, toSend, 0);

      if (sent < 0)
      {
#if ART_ENABLE_BSPY_LOGGING
        Warningf("NMLOG Warning| bSpyServer: sendPCBuffer (%i bytes) - failed (error: %d), closing connection", m_bufferUsed, nmsock_error);
#endif // ART_ENABLE_BSPY_LOGGING

        shutdownClient();

		return;
      }

      toSend -= sent;
      bufPtr += sent;

    } while (toSend > 0);
  }

  void bSpyServer::sendInitialPayloads(bSpy::IdentityPacket& ip)
  {
    bspySendPacket(ip);

    // make sure string map, if already used, is synched to the app immediately
    sendStringMap();

    // send any packets cached pre-connection
    sendPCBuffer();
  }

  bSpy::bSpyStringToken bSpyServer::getTokenForString(const char* str)
  {
    unsigned int stringLen = 0;
    int stringHash = bSpy::utils::hashString(str, stringLen);

    bSpyStringHashed nHashed;

    // go see if we already have this key
    if (!m_stringMap->find(stringHash, nHashed))
    {
      // the string tokens are u16 values
      FastAssert(m_stringTokenIndex < 0xFFFF);
      
      nHashed.m_stringLength = stringLen;
      nHashed.m_token   = (bSpy::bSpyStringToken)m_stringTokenIndex;

      // copy the string locally so we have a permanent reference to it
      char* copiedString = (char*)m_stringCacheMMgr->alloc(nHashed.m_stringLength + 1);
      strcpy(copiedString, str);

      nHashed.m_string  = copiedString;

      // log the new string in the hashmap
      m_stringMap->insert(stringHash, nHashed);

      // increment our 'unique ID' counter
      m_stringTokenIndex++;

      // if we are connected, send the string straight away;
      // on a new connection, any strings cached before the connection was made
      // are sent automatically (eg. strings encountered during deserialization)
      if (isClientConnected())
      {
        // send the string data to the app the first time it's encountered
        bSpy::AddStringToCachePacket astc(nHashed.m_string, nHashed.m_token);
        bspySendPacket(astc);
      }
    }

    return nHashed.m_token;
  }

  const char* bSpyServer::getStringForToken(bSpy::bSpyStringToken token)
  {
    return m_stringMap->findString(token);
  }

  float bSpyServer::getMSTimeSinceBeginStep() const
  {
    return m_perfTimer->GetMsTime();
  }

  void StringMapIterator(int /*key*/, bSpyStringHashed& value)
  {
    bSpy::AddStringToCachePacket astc(value.m_string, value.m_token);
    bspySendPacket(astc);
  }

  void bSpyServer::sendStringMap()
  {
    m_stringMap->iterate(StringMapIterator);
  }

  void bSpyServer::sendIncomingTransforms(ART::NmRsCharacter* character)
  {
    if (!shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_TransformStreams))
      return;

    // go get the transforms from the agent
    int incomingComponentCount = 0;

    rage::Matrix34 *itPtr = 0;

    IncomingTransformStatus itmStatusFlags = kITSNone;

#if NM_ANIM_MATRICES
    enum { numStreamsToSend = 3 };
    const ART::IncomingTransformSource streams[numStreamsToSend] = {kITSourceCurrent, kITSourcePrevious, kITSourceAnimation};
#else // NM_ANIM_MATRICES
    enum { numStreamsToSend = 2 };
    const ART::IncomingTransformSource streams[numStreamsToSend] = {kITSourceCurrent, kITSourcePrevious};
#endif // NM_ANIM_MATRICES

    for (int iS=0; iS<numStreamsToSend; iS++)
    {
      character->getIncomingTransforms(&itPtr, itmStatusFlags, incomingComponentCount, streams[iS]);
      if (incomingComponentCount == 0 || itPtr == 0)
        continue;

      bSpy::TransformStreamPacket tsp((bSpy::bs_uint8)incomingComponentCount, (bs_int8)character->getAssetID(), m_defaultITMStreamLabels[(int)streams[iS]]);

      for (int i=0; i<incomingComponentCount; i++)
      {
        rage::Matrix34& _tm = itPtr[i];
        tsp.m_tms[i].m_row[0].v[0] = _tm.a.x;
        tsp.m_tms[i].m_row[0].v[1] = _tm.a.y;
        tsp.m_tms[i].m_row[0].v[2] = _tm.a.z;

        tsp.m_tms[i].m_row[1].v[0] = _tm.b.x;
        tsp.m_tms[i].m_row[1].v[1] = _tm.b.y;
        tsp.m_tms[i].m_row[1].v[2] = _tm.b.z;

        tsp.m_tms[i].m_row[2].v[0] = _tm.c.x;
        tsp.m_tms[i].m_row[2].v[1] = _tm.c.y;
        tsp.m_tms[i].m_row[2].v[2] = _tm.c.z;

        tsp.m_tms[i].m_row[3].v[0] = _tm.d.x;
        tsp.m_tms[i].m_row[3].v[1] = _tm.d.y;
        tsp.m_tms[i].m_row[3].v[2] = _tm.d.z;
      }

      tsp.m_agentID = (bs_int8)character->getID();
      bspySendPacket(tsp);
    }
  } 

  void bSpyServer::bSpyLogFormat(bool ZF, bool tryCache, bSpy::LogStringPacket::LogType type, const char* file, int line, const wchar_t* format, ...)
  {
    va_list args;
    va_start(args, format);
    int bufWritten = vformatf_n(m_logBuffer, BS_LOG_BUFFER_SIZE -1, format, args);
    va_end(args);

    if ((isClientConnected() || tryCache) &&
        shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_LogData))
    {
      if (bufWritten > 0)
      {
        char outBuf[BS_LOG_BUFFER_SIZE];

        int i=0;
        for (; i<bufWritten; i++)
        {
          wchar_t ch = m_logBuffer[i];
          if(ch < 128)
            outBuf[i] = static_cast<char>(ch);
        }
        outBuf[i] = 0;

			  bSpy::LogStringPacket lsp(ZF, (bs_uint8)type, getTokenForString(outBuf));
        lsp.setFileInfo(getTokenForString(file), bSpy::bs_uint16(line));
        if (tryCache)
        {
          bspySendOrPrecachePacket(lsp);
        }
        else
        {
          bspySendPacket(lsp);
        }
      }
    }
  }

  int bSpyServer::bSpyAssertHandler(const char* file, int line, const char* format, ...)
  {
    va_list args;
    char buffer[1024];

    va_start(args,format);
    vformatf(buffer,sizeof(buffer),format,args);
    va_end(args); 
    
    bSpy::LogStringPacket lsp(false, (bs_uint8)bSpy::LogStringPacket::lt_error, getTokenForString(buffer));
    lsp.setFileInfo(getTokenForString(file), bSpy::bs_uint16(line));
    bspySendOrPrecachePacket(lsp);

    return 0;
  }

#if __WIN32
  void bSpyServer::setFileSerializeHandle(FILE* fs)
  {
    // this is a once-per-session call!
    Assert(m_fileToSerializeToPending == 0 && m_fileToSerializeTo == 0);
    m_fileToSerializeToPending = fs;
  }
#endif // __WIN32


  bSpyChunkMemory::bSpyChunkMemory(ART::MemoryManager* services, size_t initialSize, size_t reallocSize) :
    m_artMemoryManager(services),
    m_heapIndex(0),
    m_currentHeap(0),
    m_heapWalkIdx(0),
    m_allocCount(0),
    m_deallocCount(0),
    m_subHeapSize(reallocSize),
    m_totalAllocSize(0)
  {
    if (initialSize != 0)
      newSubHeap(initialSize);
  }

  bSpyChunkMemory::~bSpyChunkMemory()
  {
    logStats();

    for (unsigned int i=0; i<m_heapIndex; i++)
    {
      m_artMemoryManager->deallocate(m_heapList[i].m_heap, NM_MEMORY_TRACKING_ARGS);
    }
  }

  void bSpyChunkMemory::logStats()
  {
#ifdef BSPY_CHUNKMEM_LOG_DTOR

    const int dbgBufSize = 256;
    char dbg[dbgBufSize];

    _snprintf(dbg, dbgBufSize, "bSpyChunkMemory: reset/dtor [  alloc: %u = %u KB  |  dealloc: %u  ]\n", m_allocCount, m_totalAllocSize / 1024, m_deallocCount);
    OutputDebugStringA(dbg);

    _snprintf(dbg, dbgBufSize, "  /-- %i heaps :\n", m_heapIndex);
    OutputDebugStringA(dbg);

    for (unsigned int i=0; i<m_heapIndex; i++)
    {
      _snprintf(dbg, dbgBufSize, "  + heap sz: %u  max: %u \n", m_heapList[i].m_heapUse, m_heapList[i].m_heapSize);
      OutputDebugStringA(dbg);
    }

#endif // BSPY_CHUNKMEM_LOG_DTOR
  }

  void bSpyChunkMemory::resetForReuse()
  {
    logStats();

    if (m_heapIndex > 0)
    {
      for (unsigned int i=0; i<m_heapIndex; i++)
      {
        m_heapList[i].m_heapUse = 0;
      }
      m_currentHeap = &m_heapList[0];
    }
    else
      m_currentHeap = 0;

    m_allocCount = 0;
    m_deallocCount = 0;
    m_totalAllocSize = 0;
    m_heapWalkIdx = 0;
  }

  void bSpyChunkMemory::newSubHeap(size_t sz)
  {
    Heap *tmpHeap = 0;
    for (unsigned int i=m_heapWalkIdx; i<m_heapIndex; i++)
    {
      tmpHeap = &m_heapList[i];

      if (tmpHeap->m_heapUse + sz <= tmpHeap->m_heapSize)
      {
        m_currentHeap = tmpHeap;
        m_heapWalkIdx = i;
        return;
      }
    }

    Heap *h = &m_heapList[m_heapIndex];
    m_heapIndex ++;

    // we run outta heaps
    FastAssert(m_heapIndex < 64);

    h->m_heap = (unsigned char*)m_artMemoryManager->allocate(sz, NM_MEMORY_TRACKING_ARGS);
    h->m_heapSize = sz;
    h->m_heapUse = 0;

    m_currentHeap = h;
    m_heapWalkIdx ++;
  }

  void* bSpyChunkMemory::alloc(size_t size)
  {
    // optionally we can store a heap to pop back to using
    // after the function has returned a pointer; this is used
    // if the size requested is > the sub-heap size (see comments below)
    Heap *pushedHeap = 0;
    unsigned int pushedWalkIdx = 0;

    // new heap required?
    if ( m_currentHeap == 0 || 
        (m_currentHeap->m_heapUse + size) > m_currentHeap->m_heapSize)
    {
      // ensure the next sub-heap can contain the allocation
      if (size <= m_subHeapSize)
        newSubHeap(m_subHeapSize);
      else
      {
        // otherwise build a heap to contain this size of allocation,
        // do the work of returning the pointer, then rewind back to the 
        // other heap to use it up
        pushedHeap = m_currentHeap;
        pushedWalkIdx = m_heapWalkIdx;
        newSubHeap(size);

        // we could up m_subHeapSize to 'size' here...
      }
    }

    // return memory block to use
    unsigned char* ptr = m_currentHeap->m_heap + m_currentHeap->m_heapUse;
    m_currentHeap->m_heapUse += size;

    // update class-local stats
    m_allocCount ++;
    m_totalAllocSize += size;

    // pop back to a heap recorded above
    if (pushedHeap)
    {
      m_currentHeap = pushedHeap;
      m_heapWalkIdx = pushedWalkIdx;
    }

    return (void*)ptr;
  }

  void bSpyChunkMemory::dealloc(void* /*ptr*/)
  {
    m_deallocCount ++;
  }

}


#endif // ART_ENABLE_BSPY

