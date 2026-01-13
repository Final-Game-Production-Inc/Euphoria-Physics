//
// system/namedpipe.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//
#if !__FINAL || __FINAL_LOGGING

#include "namedpipe.h"
#include "file/file_config.h"
#include "file/device.h"
#include "file/tcpip.h"
#include "string/string.h"
#include "system/ipc.h"
#include "system/timer.h"

#if __PPU || RSG_ORBIS
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#elif __PSP2
#include <net.h>

#elif __LIVE

#include "grprofile/pix.h" // for PROFILE_BUILD  define before xtl.h include. makes unity builds happy.
#include "system/xtl.h"

#elif __WIN32PC

#define STRICT
#pragma warning(disable: 4668)
#include <winsock2.h>
#pragma warning(error: 4668)

#elif RSG_DURANGO

#define STRICT
#pragma warning(disable: 4668)
#include <Windows.h>
#include <winsock2.h>
#pragma warning(error: 4668)

#endif

using namespace rage;


sysNamedPipe::sysNamedPipe()
: m_Handle(fiHandleInvalid)
{
}


bool sysNamedPipe::Open(int socketPort) {
	return Open( fiDeviceTcpIp::GetLocalHost(), socketPort );
}

bool sysNamedPipe::Open( const char* addr, int socketPort ) {
	m_Handle = fiDeviceTcpIp::Connect(addr,socketPort);
#if __WIN32PC
	if (!fiIsValidHandle(m_Handle))
		Errorf("sysNamedPipe - cannot connect to %s:%d",addr,socketPort);
#endif
	return fiIsValidHandle(m_Handle);
}

bool sysNamedPipe::Create(int socketPort) {
	fiHandle listener = fiDeviceTcpIp::Listen(socketPort,1);
	if (!fiIsValidHandle(listener)) {
		Errorf("sysNamedPipe - cannot create listen socket on port %d",socketPort);
		return false;
	}
	m_Handle = fiDeviceTcpIp::Pickup(listener);
	fiDeviceTcpIp::GetInstance().Close(listener);
	if (!fiIsValidHandle(m_Handle))
		Errorf("sysNamedPipe - cannot pickup on port %s:%d",fiDeviceTcpIp::GetLocalHost(),socketPort);
	return fiIsValidHandle(m_Handle);
}

int sysNamedPipe::Read(void *buffer,int size) {
	if (!fiIsValidHandle(m_Handle))
		return -1;

	if (fiDeviceTcpIp::GetReadCount(m_Handle))
		return fiDeviceTcpIp::GetInstance().Read(m_Handle,buffer,size);
	else
		return 0;
}


void sysNamedPipe::SafeRead(void *buffer,int size) {
	fiDeviceTcpIp::GetInstance().SafeRead(m_Handle,buffer,size);
}


int sysNamedPipe::Write(const void *buffer,size_t size) {
	if (!fiIsValidHandle(m_Handle))
		return -1;

	return fiDeviceTcpIp::StaticWrite(m_Handle,buffer,ptrdiff_t_to_int(size));
}

size_t sysNamedPipe::WriteWithTimeout(const void* buffer, size_t size, int milliseconds)
{
	sysTimer timer;

	size_t written = 0;
	while(written < size && timer.GetMsTime() < milliseconds)
	{
		int thisWrite = fiDeviceTcpIp::StaticWrite(m_Handle, (const char*)buffer + written, int(size - written));
		if (thisWrite > 0)
		{
			written += thisWrite;
		}
		else if (thisWrite == 0)
		{
			// I should really do a select or something here
			sysIpcSleep(1);
		}
		else
		{	  
			// thisWrite < 0 means error code was returned, bail out
			break;
		}
	}

	return written;
}


void sysNamedPipe::SafeWrite(const void *buffer,size_t size) {
	if (fiIsValidHandle(m_Handle))
		fiDeviceTcpIp::GetInstance().SafeWrite(m_Handle,buffer,ptrdiff_t_to_int(size));
}


void sysNamedPipe::Close() {
	if (fiIsValidHandle(m_Handle)) {
		fiDeviceTcpIp::GetInstance().Close(m_Handle);
		m_Handle = fiHandleInvalid;
	}
}

#if __PSP2
#define setsockopt sceNetSetsockopt
#define getsockopt sceNetGetsockopt
#define socklen_t SceNetSocklen_t
#define IPPROTO_TCP SCE_NET_IPPROTO_TCP
#define TCP_NODELAY SCE_NET_TCP_NODELAY
#endif

#if RSG_ORBIS
#define SOCKET int
#define SOCKET_ERROR -1
#endif

void sysNamedPipe::SetNoDelay( bool noDelay )
{
	if ( m_Handle != fiHandleInvalid )
	{
#if __PPU || __PSP2 || RSG_ORBIS
		int noNagle = noDelay;
		socklen_t len = sizeof(int);
		if ( setsockopt( (int)m_Handle, IPPROTO_TCP, TCP_NODELAY, &noNagle, len ) < 0 )
		{
			Displayf( "Unable to turn off the Nagle algorithm." );
		}
#else
		bool noNagle = noDelay;
		int len = sizeof(bool);
		if ( setsockopt( (SOCKET)m_Handle, IPPROTO_TCP, TCP_NODELAY, (char *)(&noNagle), len ) == SOCKET_ERROR )
		{
			Displayf( "Unable to turn off the Nagle algorithm." );
		}
#endif
	}
}

bool sysNamedPipe::GetNoDelay() const
{
	if ( m_Handle == fiHandleInvalid )
	{
		return false;
	}

#if __PPU || __PSP2 || RSG_ORBIS
	int noNagle;
	socklen_t len = sizeof(int);
	if ( getsockopt( (int)m_Handle, IPPROTO_TCP, TCP_NODELAY, &noNagle, &len ) < 0 )
	{
		return false;
	}

	return noNagle == 0;
#else
	int noNagle;
	int len = sizeof(int);
	if ( getsockopt( (SOCKET)m_Handle, IPPROTO_TCP, TCP_NODELAY, (char *)(&noNagle), &len ) == SOCKET_ERROR )
	{
		return false;
	}

	return noNagle == 0;
#endif	
}

void sysNamedPipe::SetBlocking(bool blocking)
{
	fiDeviceTcpIp::SetBlocking(m_Handle, blocking);
}

int sysNamedPipe::GetReadCount() const 
{
	if(fiIsValidHandle(m_Handle))
	{
		return fiDeviceTcpIp::GetInstance().GetReadCount(m_Handle);
	}
	return 0;
}
#endif
