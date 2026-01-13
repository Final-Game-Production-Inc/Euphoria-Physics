//
// system/guid.h
//
// Copyright (C) 2016 Rockstar Games.  All Rights Reserved.
//
//
// This file provides a utility methods for sysGuid including generation.
//

#ifndef SYSTEM_GUID_UTIL_H
#define SYSTEM_GUID_UTIL_H

#include "math/random.h"
#include "system/interlocked.h"
#include "system/guid.h"
#include "system/noncopyable.h"
#include "system/timer.h"
#include "atl/atfixedstring.h"

namespace rage {

typedef atFixedString<48> sysGuidFixedString;

//------------------------------------------------------------------------------------------------------------------
// sysGuidGenerator
//
// PURPOSE
//	Generic 128-bit guid generator.
//------------------------------------------------------------------------------------------------------------------
class sysGuidGenerator
{
public:
	static sysGuidGenerator& Instance() { return sm_instance; }

	// PURPOSE
	// Generates a generic 128-bit guid in a fast manner. Generated guids are composed of a time-based part 
	// dependent on the machine's system time and an atomic counter (thread-safe but consistent every run).
	//
	//	* +--------------------------------------------------------------+
	//	* |                       64 bits								 |  0-7		.atomic counter
	//	* +-------------------------------+-------------------------------
	//	* |			key (32-bits)         |  8-11									.random seeded value
	//	* +-------+-----------------------+
	//	* |				seed			  |  12-15									.initial system time stamp
	//	* +-------+-------+---------------+
	//
	// NOTES
	// Generated guids are suitable for both runtime and persistent guids although persistent guids generated
	// through GenerateGuid should be more resilient to collision.
	sysGuid GenerateFastGuid()
	{
		sysGuid guid;
		guid.m_data64[0] = sysInterlockedIncrement(&m_counter);
		guid.m_data32[2] = static_cast<u32>(m_rng.GetInt());
		guid.m_data32[3] = m_seed;
		return guid;
	}

	// PURPOSE
	// Generates a version #1 standard guid. This method can be safely used across multiple threads.
	//	
	//	* +--------------------------------------------------------------+
	//	* |                     low 32 bits of time                      |  0-3		.time_low (a)
	//	* +-------------------------------+-------------------------------
	//	* |     mid 16 bits of time       |  4-5									.time_mid (b)
	//	* +-------+-----------------------+
	//	* | vers. |   hi 12 bits of time  |  6-7									.time_hi (c) and version (v)
	//	* +-------+-------+---------------+
	//	* |Res|  clkSeqHi |  8														.clock_seq_hi_and_reserved (d)
	//	* +---------------+
	//	* |   clkSeqLow   |  9														.clock_seq_low (e)
	//	* +---------------+--------------------------------- ... --------+
	//	* |                         node address						 | 10-15	.MAC address (f)
	//	* +------------------------------------------------- ... --------+
	//
	// NOTES
	// Generated guids are suitable for both runtime and persistent guids although GenerateFastGuid should be
	// preferred for runtime guids generated (it requires much less CPU cycles).
	sysGuid GenerateGuid();

private:
	static sysGuidGenerator sm_instance;

	unsigned m_counter;
	u32 m_seed;
	mthRandom m_rng;

	sysGuidGenerator()
	{
		// Ensure non-zero m_seed
		do
		{
			m_seed = sysTimer::GetSystemMsTime();
			m_rng.Reset(m_seed);
		} while (!m_seed);
	}

	NON_COPYABLE(sysGuidGenerator);
};

//------------------------------------------------------------------------------------------------------------------
// sysGuidUtil
//
// PURPOSE
//	Utility namespace for sysGuid including generation and conversion functions.
//------------------------------------------------------------------------------------------------------------------
namespace sysGuidUtil
{
	enum GuidStringFormat
	{
		kGuidStringFormatNonDecorated,		// "aaaaaaaa-bbbb-vccc-ddee-ffffffffffff"
		kGuidStringFormatDecorated,			// "{aaaaaaaa-bbbb-vccc-ddee-ffffffffffff}"
		kGuidStringFormatNonDecoratedFlat	// "aaaaaaaabbbbvcccddeeffffffffffff"
	};

	// PURPOSE: reads the atGuid data from parameter string. The parameter string is validated to comply with the
	// supported atGuid string formats.
	size_t GuidFromString(sysGuid& targetGuid, const char* str);

	// PURPOSE
	// Converts a sysGuid instance to its string representation.
	void GuidToString(const sysGuid& guid, char* targetBuffer, size_t size, GuidStringFormat format);

	// PURPOSE
	// Converts a sysGuid instance to its string representation and returns it as fixed string
	sysGuidFixedString GuidToFixedString(const sysGuid& guid, GuidStringFormat format = kGuidStringFormatNonDecorated);

	// PURPOSE
	// Generates a unique guid in a fast manner. This method can be safely used across multiple threads.
	inline sysGuid GenerateFastGuid() { return sysGuidGenerator::Instance().GenerateFastGuid(); }
	
	// PURPOSE
	// Generates a version #1 standard guid. This method can be safely used across multiple threads.
	inline sysGuid GenerateGuid() { return sysGuidGenerator::Instance().GenerateGuid(); }

	// PURPOSE
	// Generates a unique id valid during a single runtime (not suitable for persistent uniqueness across runs).
	// This is achieved using the atomic counter part of a generated sysGuid. 
	// This method can be safely used across multiple threads.
	inline u64 GenerateRuntimeId() { return GenerateGuid().m_data64[0]; }

	// PURPOSE
	// Creates a unique 64-bit hash value from a generated sysGuid instance stripping the atomic counter value's
	// lower 32-bit part and combining it with the random part.
	// NOTES
	// This hash may be suitable for persistent id uniqueness between runs. However, it has a higher collision 
	// probability than the full 128 bit guid.
	// HashFastGuid should only be used with GenerateFastGuid generated guids.
	inline u64 HashFastGuid(const sysGuid& guid) { return guid.m_data32[0] + (guid.m_data64[1] << 32); }
};

} // namespace rage

#endif	// SYSTEM_GUID_UTIL_H
