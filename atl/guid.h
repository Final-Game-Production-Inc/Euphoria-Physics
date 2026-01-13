//
// atl/guid.h
//
// Copyright (C) 2015 Rockstar Games.  All Rights Reserved.
//
//
// This file provides a generic 128 bit guid: atGuid and a 64 bit has version atGuidHash to unequivocally identify 
// instances of any type. It is intended for user code to use atGuidHash by default for identification, referring to 
// atGuid only for debugging purposes. Let it be noted that atGuid main purpose is to offer guid generation and string
// parsing of guids.
//

#ifndef ATL_GUID_H
#define ATL_GUID_H

#include "system/guidutil.h"

#define ATL_GUID_DEBUG (__BANK || __RESOURCECOMPILER || __TOOL) && 0

namespace rage {

// Forward declarations
class atGuidHash;

//----------------------------------------------------------------------------------------------------------------------
// atGuid
//
// PURPOSE
//	Generic 128-bit guid data struct for map data. This guid can be hashed to a 64-bit value which can be used to
//	retrieve the original guid whenever ATL_GUID_DEBUG is enabled.
//----------------------------------------------------------------------------------------------------------------------
class atGuid : public sysGuid
{
public:
	SYS_IMPLEMENT_GUID(atGuid)

	// Explicit parameter constructors
	explicit atGuid(datResource& resource) : sysGuid(resource) {}
	explicit atGuid(const char* str);

	// PURPOSE: gets the corresponding 64 bit hash value.
	atGuidHash GetHash() const;

	// PURPOSE: writes the atGuid to the parameter string buffer according to the format parameter format.
	void GetAsString(char* targetBuffer, size_t size, 
		sysGuidUtil::GuidStringFormat format = sysGuidUtil::kGuidStringFormatDecorated) const;

	// PURPOSE: reads the atGuid data from parameter buffer
	void SetFromBuffer(const char* buffer);

	// PURPOSE: reads the atGuid data from parameter string. The parameter string is validated to comply with the
	// supported atGuid string formats. SetFromString returns the read character length on success or 0 otherwise.
	size_t SetFromString(const char* str);

#if ATL_GUID_DEBUG
	// PURPOSE: Adds this GUID to be tracked, so you can convert an atGuidHash into an atGuid.
	void Track() const;

	// PURPOSE: allows retrieving the original atGuid from its 64 bit value.
	static atGuid RetrieveFromHash(atGuidHash guid);

	// PURPOSE: Returns the number of GUIDs that are currently being tracked, used for monitoring the map usage.
	static u32 GetTrackedHashCount();

	// PURPOSE: Returns the tracked GUID map's capacity, used for monitoring the map usage.
	static u32 GetTrackedHashCapacity();
#endif // ATL_GUID_DEBUG

	// PURPOSE: generates a new guid based on current machine time and MAC address
	static atGuid Generate();

private:
	u64	InternalCreateHash() const;
};

inline datSerialize& operator<<(datSerialize &ser, atGuid& guid)
{ 
	return operator<<(ser, static_cast<sysGuid&>(guid));
}

//----------------------------------------------------------------------------------------------------------------------
// atGuidHash
//
// PURPOSE
//	atGuidHash provides a 64 bit unique id based on atGuid (128 bit). Note that in order to reduce the byte size 
//	atGuidHash ignores the lower 64bits of atGuid.
//
// NOTES
//	This class has the following contract:
//	
//	- atGuidHash assumes a version #1 Guid generation (see uuid.h)
//	- atGuidHash considers the first 64bits of the version #1 guid structure:
//	
//	* +--------------------------------------------------------------+
//	* |                     low 32 bits of time                      |  0-3		.time_low (a)
//	* +-------------------------------+-------------------------------
//	* |     mid 16 bits of time       |  4-5									.time_mid (b)
//	* +-------+-----------------------+
//	* | vers. |   hi 12 bits of time  |  6-7									.time_hi (c) and version (v)
//	* +-------+-------+---------------+
//	
//	- atGuidHash reverses the order of atGuid 64 highest bits in order to have the version in the highest byte. A valid
//	atGuidHash is structured then as: vccc-bbbb-aaaaaaaa. This means clock_seq_hi_and_reserved, clock_seq_low and MAC 
//	address atGuid fields are ignored.
//	- atGuidHash is considered valid when the highest byte, version (v), is not zero.
//	- atGuidHash relies on default copy constructor and assignment operators.
//	- atGuidHash relies on default comparison operators.
//----------------------------------------------------------------------------------------------------------------------
class atGuidHash
{
public:
	atGuidHash() 
		: m_value(0)
	{
	}

	atGuidHash(u64 value) 
		: m_value(value)
	{
	}

	// Conversion to u64 operators
	operator const u64&(void) const
	{ 
		return m_value;
	}

	operator u64&(void)
	{ 
		return m_value;
	}

	bool IsValid() const
	{
		return (m_value & k_guidMask) != 0;
	}

private:
	static const u64 k_guidMask = (15ull << 60); // 11110000...0

	u64 m_value;
};

} // namespace rage

#endif
