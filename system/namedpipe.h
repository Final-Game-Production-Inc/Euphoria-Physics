//
// system/namedpipe.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_NAMEDPIPE_H
#define SYSTEM_NAMEDPIPE_H

#include <stddef.h>		// for size_t

#include "file/handle.h"

namespace rage {


//
// PURPOSE:
//	The sysNamedPipe is designed for PS2, GC, and Xbox to read/write data through the
//	connection.  It is actually a normal TCP/IP connection these days.
// <FLAG Component>
//
class sysNamedPipe {
public:
	//PURPOSE: Create a named pipe
	sysNamedPipe();

	//PURPOSE: Open a named pipe.
	//PARAMS
	//  socketPort - socket port
	//RETURNS:	true is open success.
	bool Open(int socketPort);

	//PURPOSE: Open a named pipe.
	//PARAMS
	//  addr - the ip address
	//  socketPort - socket port
	//RETURNS:	true is open success. 
	bool Open( const char* addr, int sockertPort );

	//PURPOSE: Open a named pipe.
	//PARAMS
	//  socketPort - socket port
	//RETURNS:	true is create success.
	bool Create(int socketPort);

	/*
	PURPOSE: Read data from the pipe into the buffer.
	PARAMS
		buffer - where to store the data.
		size - the maximum amount of data to read
	RETURNS: total bytes read.
	*/
	int Read(void *buffer,int size);
	
	/*
	PURPOSE: Read data from the pipe into the buffer.
	PARAMS
		buffer - where to store the data.
		size - the amount of data to read
	*/
	void SafeRead(void *buffer,int size);

	/*
	PURPOSE: Write the data in the buffer to the named pipe.
	PARAMS
		buffer - the data to be written.
		size - the amount of data to write
	RETURNS: the amount actually written.
	*/
	int Write(const void *buffer,size_t size);

	/*
	PURPOSE: Writes the data in the buffer to the named pipe. Blocks until all data has been written
	PARAMS:
		buffer - The data to be written
		size - The amount of data to write
	*/
	void SafeWrite(const void *buffer,size_t size);

	size_t WriteWithTimeout(const void* buffer, size_t size, int milliseconds);

	// PURPOSE: Closes the pipe and releases its resources
	void Close();

	// PURPOSE: Determine if a pipe is valid (i.e. if it can be read or written to)
	// RETURNS: True if a pipe is valid
	bool IsValid() const { return fiIsValidHandle(m_Handle); }

	// PURPOSE: Turns on/off the Nagle Coalescence algorithm on packet write.
	// PARAMS:
	//     noDelay - true to disable Nagle.  All sockets are false by default.
	// NOTES:
	//     The Nagle algorithm involves buffering multiple write operations into a single packet
	//     of optimum size before actually sending the data across the network.  If you need to
	//     send data immediately and/or have small packet sizes, it may be a good idea to turn
	//     on No Delay.  Otherwise, it is recommended that you use the default.
	//     The Xenon platform does not implement Nagle Coalescence, so setting this should have
	//     no discernible effect.
	void SetNoDelay( bool noDelay );

	// PURPOSE: Get whether the Nagle algorithm is enabled or disabled
	// RETURNS: true if Nagle is off, false if it is on
	bool GetNoDelay() const;

	// PURPOSE: Turns on or off blocking on this socket. Default is ON
	void SetBlocking(bool blocking);

	// PURPOSE:	Returns amount of data bytes waiting on the socket
	// RETURNS:	Number of bytes waiting to read, or zero if none available
	// NOTES:	Call this first to avoid blocking on a read.
	int GetReadCount() const;

private:
	fiHandle m_Handle;
};

}	// namespace rage

#endif
