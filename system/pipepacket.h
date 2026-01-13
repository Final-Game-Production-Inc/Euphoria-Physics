//
// system/pipepacket.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_PIPE_PACKET_H
#define SYSTEM_PIPE_PACKET_H

#if __BANK | __DEV
namespace rage {

//
// PURPOSE:
//	This class handles the communication between a local program
//  with a remote program.  The class itself has a packet
//  that can be written and read by the local and remote programs.
// <FLAG Component>
//
class sysPipePacket {
public:
	enum
	{
		HEADER_SIZE=12,
		DATA_SIZE=512 - HEADER_SIZE
	};

	// PURPOSE: pipe packet constructor
	// PARAMS: pipe - a reference to the associated pipe
	sysPipePacket(class sysNamedPipe& pipe);

    virtual ~sysPipePacket() { }

	// PURPOSE: accessor for the length of the packet in bytes (*not* including twelve-byte header)
	// RETURNS: returns the packet length
	u16 GetLength() const;
	// PURPOSE: accessor the command type
	// RETURNS: returns the command type
	u16 GetCommand() const;
	// PURPOSE: accessor for guid for the class type to communicate with
	// RETURNS: returns the class type GUID
	int GetGuid() const;
	// PURPOSE: accessor for id relating to the instance of the class the message is for
	// RETURNS: returns the instance id
	u32 GetId() const;
	// PURPOSE: accessor for the payload of the packet
	// RETURNS: returns a pointer to the payload
	u8* GetStorage();

	//
	// PURPOSE
	//	prepare the packet for its payload
	// PARAMS
	//	command - the type of command
	//	guid - the guid of the class to communicate with
	//	id - the object instance id
	//
	void Begin(int command,int guid,u32 id);

	//
	// PURPOSE
	//	resets the current packet
	//
	void Begin() const; 

	//
	// PURPOSE
	//	writes an unsigned 8 bit piece of data to the packet
	// PARAMS
	//	value - the unsigned 8 bit piece of data to write
	//
	void Write_u8(u8 value);

	//
	// PURPOSE
	//	writes an signed 8 bit piece of data to the packet
	// PARAMS
	//	value - the signed 8 bit piece of data to write
	//
	void Write_s8(s8 value);

	//
	// PURPOSE
	//	writes an boolean piece of data to the packet
	// PARAMS
	//	value - the boolean piece of data to write
	//
	void Write_bool(bool value);

	//
	// PURPOSE
	//	writes an unsigned 16 bit piece of data to the packet
	// PARAMS
	//	value - the unsigned 16 bit piece of data to write
	//
	void Write_u16(u16 value);

	//
	// PURPOSE
	//	writes an signed 16 bit piece of data to the packet
	// PARAMS
	//	value - the signed 16 bit piece of data to write
	//
	void Write_s16(s16 value);

	//
	// PURPOSE
	//	writes an unsigned 32 bit piece of data to the packet
	// PARAMS
	//	value - the unsigned 32 bit piece of data to write
	//
	void Write_u32(u32 value);

	//
	// PURPOSE
	//	writes an unsigned 64 bit piece of data to the packet
	// PARAMS
	//	value - the unsigned 64 bit piece of data to write
	//
	void Write_u64(u64 value);

	//
	// PURPOSE
	//	writes an signed 32 bit piece of data to the packet
	// PARAMS
	//	value - the signed 32 bit piece of data to write
	//
	void Write_s32(s32 value);

	//
	// PURPOSE
	//	writes an signed 64 bit piece of data to the packet
	// PARAMS
	//	value - the signed 64 bit piece of data to write
	//
	void Write_s64(s64 value);

	//
	// PURPOSE
	//	writes an floating point piece of data to the packet
	// PARAMS
	//	value - the floating point piece of data to write
	//
	void Write_float(float value);

	//
	// PURPOSE
	//	writes a pointer to the packet
	// PARAMS
	//	value - the pointer to write
	//
	void Write_pointer(void* value);

	//
	// PURPOSE
	//	writes a pointer to a character string to the packet
	// PARAMS
	//	value - the pointer to the character string
	//
	void Write_const_char(const char* value);

	// PURPOSE: reads an unsigned 8 bit piece of data from the packet
	// RETURNS: the unsigned 8 bit piece of data
	u8 Read_u8() const;

	// PURPOSE: reads an signed 8 bit piece of data from the packet
	// RETURNS: the signed 8 bit piece of data
	s8 Read_s8() const;

	// PURPOSE: reads a boolean piece of data from the packet
	// RETURNS: the boolean piece of data
	bool Read_bool() const;
	
	// PURPOSE: reads an unsigned 16 bit piece of data from the packet
	// RETURNS: the unsigned 16 bit piece of data
	u16 Read_u16() const;

	// PURPOSE: reads an signed 16 bit piece of data from the packet
	// RETURNS: the signed 16 bit piece of data
	s16 Read_s16() const;
	
	// PURPOSE: reads an unsigned 32 bit piece of data from the packet
	// RETURNS: the unsigned 32 bit piece of data
	u32 Read_u32() const;
	
	// PURPOSE: reads an unsigned 64 bit piece of data from the packet
	// RETURNS: the unsigned 64 bit piece of data
	u64 Read_u64() const;

	// PURPOSE: reads an signed 32 bit piece of data from the packet
	// RETURNS: the signed 32 bit piece of data
	s32 Read_s32() const;

	// PURPOSE: reads an signed 64 bit piece of data from the packet
	// RETURNS: the signed 64 bit piece of data
	s64 Read_s64() const;

	// PURPOSE: reads an floating point of data from the packet
	// RETURNS: the floating point piece of data
	float Read_float() const;
	
	// PURPOSE: reads a pointer of data from the packet
	// RETURNS: the pointer
	void* Read_pointer() const;

	// PURPOSE: reads a pointer to a character string from the packet
	// RETURNS: the pointer to a character string
	const char *Read_const_char() const;

	// PURPOSE: receives a packet from the remote program
	// PARAMS
	//	timeout - the time (in seconds) to wait for a response.  a timeout of -1 will wait forever
	//  timer - timer used to track timeout, REQUIRED IF TIMEOUT IS SET
	// RETURNS: true if anything was read
	// NOTES: this function blocks until it receives the packet if no timeout is specified
	bool Receive(int timeout = -1, const class sysTimer *timer = NULL);
	// PURPOSE: sends to packet to the remote program
	virtual void Send();
	// PURPOSE: specify the end of reading or writing from a packet
	void End() const;
	// PURPOSE: swap header bytes for big endian machines
	void SwapHeaderBytes();

protected:
	u16 m_Length;
	u16 m_Command;
	int m_Guid;
	u32 m_Id;
	u8 m_Storage[DATA_SIZE];

	// these two members are not written to or read from the pipe:
	sysNamedPipe& m_Pipe;
	mutable int m_CurrentOffset;

private:
	sysPipePacket();
	const sysPipePacket& operator=(const sysPipePacket&);
};

inline sysPipePacket::sysPipePacket(sysNamedPipe& pipe) :
	m_Pipe(pipe),
	m_CurrentOffset(0)
{
}

inline u16 sysPipePacket::GetLength() const	
{
	return m_Length;
}

inline u16 sysPipePacket::GetCommand() const	
{
	return m_Command;
}

inline int sysPipePacket::GetGuid() const		
{
	return m_Guid;
}

inline u32 sysPipePacket::GetId() const
{
	return m_Id;
}

inline u8* sysPipePacket::GetStorage()	
{
	return m_Storage;
}

inline void sysPipePacket::Begin(int command,int guid,u32 id)
{
	m_Length = 0;
	m_Command = (u16) command;
	m_Guid = guid;
	m_Id = id;
	m_CurrentOffset = -1;
}
inline void sysPipePacket::Begin() const
{ 
	m_CurrentOffset = 0;
}
inline void sysPipePacket::Write_s8(s8 value) 
{ 
	Write_u8((u8)value); 
}

inline void sysPipePacket::Write_bool(bool value) 
{ 
	Write_u8((u8)(value != 0)); 
}

inline void sysPipePacket::Write_s16(s16 value) 
{ 
	Write_u16((u16)value); 
}
inline void sysPipePacket::Write_s32(s32 value) 
{ 
	Write_u32((u32)value); 
}

inline void sysPipePacket::Write_s64(s64 value) 
{ 
	Write_u64((u64)value); 
}

inline s8 sysPipePacket::Read_s8() const 
{ 
	return (s8)Read_u8(); 
}

inline bool sysPipePacket::Read_bool() const 
{ 
	return Read_u8() != 0; 
}

inline s16 sysPipePacket::Read_s16() const 
{ 
	return (s16)Read_u16(); 
}

inline s32 sysPipePacket::Read_s32() const 
{ 
	return (s32)Read_u32(); 
}

inline s64 sysPipePacket::Read_s64() const 
{ 
	return (s64)Read_u64(); 
}

inline void sysPipePacket::End() const 
{ 
	FastAssert(m_CurrentOffset == m_Length); 
	m_CurrentOffset = -1; 
}


}	// namespace rage
#endif // #if __DEV

#endif // #if __SYSTEM_PIPE_PACKET_H
