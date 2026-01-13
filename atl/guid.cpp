//
// atl/guid.cpp
//
// Copyright (C) 2015 Rockstar Games.  All Rights Reserved.
//

#include "guid.h"
#include "atl/uuid.h"
#if ATL_GUID_DEBUG
#include "atl/hashmap.h"
#endif
#include "data/serialize.h"
#include "system/criticalsection.h"
#include "system/memory.h"
#include <cstdio>

namespace rage {

//----------------------------------------------------------------------------------------------------------------------
// DebugGuidTracker
//
// PURPOSE
//	This methods map and atGuid through its atGuidHash value. The values are kept in a global map to be able to retrieve
//	the original atGuid through its hash. 
//----------------------------------------------------------------------------------------------------------------------
#if ATL_GUID_DEBUG

class DebugGuidTracker
{
public:
	DebugGuidTracker()
		: k_mapInitialSize(1 << 19) // Must be a power of 2 value
	{
		USE_DEBUG_MEMORY();
		m_map.Resize(k_mapInitialSize);
	}

	~DebugGuidTracker()
	{
		USE_DEBUG_MEMORY();
		m_map.Reset();
	}

	atGuid RetrieveTrackedGuid(atGuidHash guidHash) const
	{
		Assertf(guidHash, "Parameter guidHash must be valid in order to retrieve a tracked guid!");
		if (guidHash.IsValid())
		{
			SYS_CS_SYNC(m_token);
			const atGuid* pGuid = m_map.Access(guidHash);
			if (pGuid)
			{
				return *pGuid;
			}
		}
		return atGuid();
	}

	void Track(atGuidHash hash, atGuid guid)
	{
		if (hash.IsValid() == false)
		{
			return;
		}

		SYS_CS_SYNC(m_token);
		USE_DEBUG_MEMORY();
		m_map.Insert(hash, guid);
	}

	u32 GetTrackedCount() const
	{
		return (u32)m_map.GetCount();
	}

	u32 GetTrackerCapacity() const
	{
		return (u32)m_map.GetCapacity();
	}

private:
	atHashMap<atGuidHash, atGuid, 95> m_map;
	mutable sysCriticalSectionToken m_token;
	const u32 k_mapInitialSize;
};

DebugGuidTracker g_guidTracker;

#endif

//----------------------------------------------------------------------------------------------------------------------
// atGuid methods
//----------------------------------------------------------------------------------------------------------------------
atGuid::atGuid(const char* str)
{
	SetFromString(str);
}

atGuidHash atGuid::GetHash() const
{
	// As we're converting to a hash then track this GUID so the atGuidHash can be converted back into an atGuid.
#if ATL_GUID_DEBUG
	Track(); 
#endif // ATL_GUID_DEBUG

	return  atGuidHash(InternalCreateHash());
}

void atGuid::GetAsString(char* targetBuffer, size_t size, sysGuidUtil::GuidStringFormat format) const
{
	sysGuidUtil::GuidToString(*this, targetBuffer, size, format);
}

void atGuid::SetFromBuffer(const char* buffer)
{
	sysMemCpy(&m_data32[0], buffer, sizeof(atGuid));
}

size_t atGuid::SetFromString(const char* str)
{
	return sysGuidUtil::GuidFromString(*this, str);
}

atGuid atGuid::Generate()
{
	return sysGuidUtil::GenerateGuid();
}

u64	atGuid::InternalCreateHash() const
{
	u64 hash = (((u64)m_data16[3]) << 48) + (((u64)m_data16[2]) << 32) + m_data32[0];

	return hash;
}

#if ATL_GUID_DEBUG

void atGuid::Track() const
{
	u64 hash = InternalCreateHash();

	g_guidTracker.Track(atGuidHash(hash), *this);
}

atGuid atGuid::RetrieveFromHash(atGuidHash guid)
{
	return g_guidTracker.RetrieveTrackedGuid(guid);
}

u32 atGuid::GetTrackedHashCount()
{
	return g_guidTracker.GetTrackedCount();
}

u32 atGuid::GetTrackedHashCapacity()
{
	return g_guidTracker.GetTrackerCapacity();
}

#endif // ATL_GUID_DEBUG

} // namespace rage
