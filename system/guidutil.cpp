//
// system/guid.cpp
//
// Copyright (C) 2016 Rockstar Games.  All Rights Reserved.
//
//

#include "system/guidutil.h"
#include "system/criticalsection.h"
#include "atl/uuid.h"
#include "string/string.h"

//----------------------------------------------------------------------------------------------------------------------
// Macros
//----------------------------------------------------------------------------------------------------------------------
#define SYS_GUID_STRING_FORMAT "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define SYS_GUID_STRING_FORMAT_FLAT "%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x"

namespace rage {

//----------------------------------------------------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------------------------------------------------
sysGuidGenerator sysGuidGenerator::sm_instance;
sysCriticalSectionToken g_uuidGenerationToken;

static const char* k_guidStringFormat[] =
{
	SYS_GUID_STRING_FORMAT,
	"{" SYS_GUID_STRING_FORMAT "}", // C++11 requires a space between literal and identifier
	SYS_GUID_STRING_FORMAT_FLAT
};

static const size_t k_guidStringLength[] =
{
	36,	// kGuidStringFormatNonDecorated
	38,	// kGuidStringFormatDecorated
	32, // kGuidStringFormatNonDecoratedFlat
};

//----------------------------------------------------------------------------------------------------------------------
// sysGuidGenerator methods
//----------------------------------------------------------------------------------------------------------------------
sysGuid sysGuidGenerator::GenerateGuid()
{
	// _CFGenerateUUID is not thread safe as it does not take into account the thread id at the time this comment was
	// written, therefore we need a critical section to ensure thread-safety.
	UuidUtils::Uuid_t uuid;
	{
		SYS_CS_SYNC(g_uuidGenerationToken);
		UuidUtils::_CFGenerateUUID(&uuid);
	}

	sysGuid guid;
	guid.m_data32[0] = u32(uuid.time_low);
	guid.m_data16[2] = uuid.time_mid;
	guid.m_data16[3] = uuid.time_hi_and_version;
	guid.m_data8[8] = uuid.clock_seq_hi_and_reserved;
	guid.m_data8[9] = uuid.clock_seq_low;
	guid.m_data8[10] = uuid.node[0];
	guid.m_data8[11] = uuid.node[1];
	guid.m_data8[12] = uuid.node[2];
	guid.m_data8[13] = uuid.node[3];
	guid.m_data8[14] = uuid.node[4];
	guid.m_data8[15] = uuid.node[5];

	return guid;
}

//----------------------------------------------------------------------------------------------------------------------
// sysGuidUtil methods
//----------------------------------------------------------------------------------------------------------------------
size_t sysGuidUtil::GuidFromString(sysGuid& targetGuid, const char* str)
{
	targetGuid.Clear();
	if (Verifyf(str != nullptr, "Guid string is null"))
	{
		size_t strLength = strlen(str);
		if (Verifyf(strLength >= k_guidStringLength[kGuidStringFormatNonDecoratedFlat], "Invalid guid string length"))
		{
			GuidStringFormat format = kGuidStringFormatNonDecorated;
			if (strLength == k_guidStringLength[kGuidStringFormatNonDecoratedFlat])
			{
				format = kGuidStringFormatNonDecoratedFlat;
			}
			else if (strLength >= k_guidStringLength[kGuidStringFormatNonDecorated] && 
				str[0] == '{' && 
				str[k_guidStringLength[kGuidStringFormatDecorated] - 1] == '}')
			{
				format = kGuidStringFormatDecorated;
			}

			u32 p0 = 0, p1 = 0, p2 = 0, p3 = 0, p4 = 0, p5 = 0, p6 = 0, p7 = 0, p8 = 0, p9 = 0, p10 = 0;
			sscanf(str, k_guidStringFormat[format], &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);

			targetGuid.m_data32[0] = p0;
			targetGuid.m_data16[2] = (u16) p1;
			targetGuid.m_data16[3] = (u16) p2;
			targetGuid.m_data8[8] = (u8) p3;
			targetGuid.m_data8[9] = (u8) p4;
			targetGuid.m_data8[10] = (u8) p5;
			targetGuid.m_data8[11] = (u8) p6;
			targetGuid.m_data8[12] = (u8) p7;
			targetGuid.m_data8[13] = (u8) p8;
			targetGuid.m_data8[14] = (u8) p9;
			targetGuid.m_data8[15] = (u8) p10;

			return k_guidStringLength[format];
		}
	}
	return 0;
}

void sysGuidUtil::GuidToString(const sysGuid& guid, char* targetBuffer, size_t size, GuidStringFormat format)
{
	if (Verifyf(size > k_guidStringLength[format], 
		"Target buffer size [%" SIZETFMT "d] is too small for guid formatting", size))
	{
		formatf(targetBuffer, size, k_guidStringFormat[format], guid.m_data32[0], guid.m_data16[2], guid.m_data16[3],
			guid.m_data8[8], guid.m_data8[9], guid.m_data8[10], guid.m_data8[11], guid.m_data8[12], guid.m_data8[13],
			guid.m_data8[14], guid.m_data8[15]);
	}
}

sysGuidFixedString sysGuidUtil::GuidToFixedString(const sysGuid& guid, GuidStringFormat format)
{
	sysGuidFixedString str;
	GuidToString(guid, str.GetInternalBuffer(), str.GetInternalBufferSize(), format);
	str.SetLengthUnsafe((short)strlen(str.GetInternalBuffer()));
	return str;
}

} // namespace rage
