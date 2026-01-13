//
// system/pipepacket.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#if __DEV | __BANK
#include "pipepacket.h"

#include "system/namedpipe.h"
#include "system/timer.h"
#include "string/string.h"
#include "system/memops.h"

using namespace rage;


void sysPipePacket::Write_u8(u8 data) {
	Assert(m_CurrentOffset == -1);
	m_Storage[m_Length++] = data;
	Assert(m_Length <= sizeof(m_Storage));
}


void sysPipePacket::Write_u16(u16 data) {
	Assert(m_CurrentOffset == -1);
	m_Storage[m_Length++] = (u8) (data);
	m_Storage[m_Length++] = (u8) (data >> 8);
	Assert(m_Length <= sizeof(m_Storage));
}


void sysPipePacket::Write_u32(u32 data) {
	Assert(m_CurrentOffset == -1);
	m_Storage[m_Length++] = (u8) (data);
	m_Storage[m_Length++] = (u8) (data >> 8);
	m_Storage[m_Length++] = (u8) (data >> 16);
	m_Storage[m_Length++] = (u8) (data >> 24);
	Assert(m_Length <= sizeof(m_Storage));
}


void sysPipePacket::Write_u64(u64 data) {
	Assert(m_CurrentOffset == -1);
	m_Storage[m_Length++] = (u8) (data);
	m_Storage[m_Length++] = (u8) (data >> 8);
	m_Storage[m_Length++] = (u8) (data >> 16);
	m_Storage[m_Length++] = (u8) (data >> 24);
	m_Storage[m_Length++] = (u8) (data >> 32);
	m_Storage[m_Length++] = (u8) (data >> 40);
	m_Storage[m_Length++] = (u8) (data >> 48);
	m_Storage[m_Length++] = (u8) (data >> 56);
	Assert(m_Length <= sizeof(m_Storage));
}


void sysPipePacket::Write_float(float data) {
	union { float f; unsigned u; } x;
	x.f = data;
	Write_u32(x.u);
}


void sysPipePacket::Write_pointer(void* data) {
#if __64BIT
	Write_u64((u64)data);
#else
	Write_u32((u32)data);
#endif
}


void sysPipePacket::Write_const_char(const char *data) {
	Assert(m_CurrentOffset == -1);
	if (!data)
		m_Storage[m_Length++] = 0;
	else {
		int sl = StringLength(data) + 1;
		Assert(sl < 256);
		if (sl >= 256)
		{
			sl = 255;
		}
		m_Storage[m_Length++] = (u8) sl;
		sysMemCpy(m_Storage + m_Length,data,sl);
		m_Length = (u16) (m_Length + sl);
	}
	Assert(m_Length <= sizeof(m_Storage));
}


u8 sysPipePacket::Read_u8() const {
	Assert(m_CurrentOffset >= 0);
	return m_Storage[m_CurrentOffset++];
}


u16 sysPipePacket::Read_u16() const {
	Assert(m_CurrentOffset >= 0);
	u16 result = m_Storage[m_CurrentOffset++];
	return (u16) (result | (m_Storage[m_CurrentOffset++] << 8));
}


u32 sysPipePacket::Read_u32() const {
	Assert(m_CurrentOffset >= 0);
	u32 result = m_Storage[m_CurrentOffset++];
	result |= m_Storage[m_CurrentOffset++] << 8;
	result |= m_Storage[m_CurrentOffset++] << 16;
	return result | (m_Storage[m_CurrentOffset++] << 24);
}


u64 sysPipePacket::Read_u64() const {
	Assert(m_CurrentOffset >= 0);
	u64 result = m_Storage[m_CurrentOffset++];
	result |= u64(m_Storage[m_CurrentOffset++]) << 8;
	result |= u64(m_Storage[m_CurrentOffset++]) << 16;
	result |= u64(m_Storage[m_CurrentOffset++]) << 24;
	result |= u64(m_Storage[m_CurrentOffset++]) << 32;
	result |= u64(m_Storage[m_CurrentOffset++]) << 40;
	result |= u64(m_Storage[m_CurrentOffset++]) << 48;
	result |= u64(m_Storage[m_CurrentOffset++]) << 56;
	return result;
}


float sysPipePacket::Read_float() const {
	union { float f; unsigned u; } x;
	x.u = Read_u32();
	return x.f;
}


void* sysPipePacket::Read_pointer() const {
#if __64BIT
	return (void*)Read_u64();
#else
	return (void*)Read_u32();
#endif
}


const char *sysPipePacket::Read_const_char() const {
	Assert(m_CurrentOffset >= 0);
	if (m_Storage[m_CurrentOffset]) {
		const char *result = (const char*)(m_Storage + m_CurrentOffset + 1);
		m_CurrentOffset += m_Storage[m_CurrentOffset] + 1;
		return result;
	}
	else {
		++m_CurrentOffset;
		return 0;
	}
}

#if __BE
inline unsigned short SWAPS(unsigned short s) { return (s << 8) | (s >> 8); }
inline unsigned SWAPL(unsigned s) {
	return (s >> 24) | ((s >> 8) & 0xFF00) | ((s << 8) & 0xFF0000) | (s << 24);
}
#endif


void sysPipePacket::SwapHeaderBytes()
{
#if __BE
	m_Length = SWAPS(m_Length);
	m_Command = SWAPS(m_Command);
	m_Guid = SWAPL(m_Guid);
	m_Id = SWAPL(m_Id);
#endif
}

// this function could have locally allocated a sysTimer, but its constructor looks non-trivial and a timeout is not the default for this function
bool sysPipePacket::Receive(int timeout, const sysTimer *timer) 
{
	Assert(timeout == -1 || timer != NULL);
	do 
	{
		if ( m_Pipe.Read(&m_Length,1) == 1 ) 
		{
			m_Pipe.SafeRead( 1 + (char*)(&m_Length), HEADER_SIZE - 1 );
			SwapHeaderBytes();
			if (m_Length)
            {
				m_Pipe.SafeRead(m_Storage,m_Length);
            }
			return true;
		}
	} while (timeout == -1 || timer->GetTime() < timeout );
	return false;
}

void sysPipePacket::Send() 
{
	if ( m_Pipe.IsValid() )
    {
		int length = m_Length + HEADER_SIZE;

		SwapHeaderBytes();

        // HexDump( &m_Length, length );
		m_Pipe.SafeWrite( &m_Length, length );
	}
}

#endif
