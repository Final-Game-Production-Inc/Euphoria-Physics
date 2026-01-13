// 
// system/epicevents.cpp
// 
// Copyright (C) 2019 Rockstar Games.  All Rights Reserved. 
// 

#include "file/file_config.h"

#if EPIC_API_SUPPORTED

#include "epicevents.h"

namespace rage
{

AUTOID_IMPL(sysEpicEvent);
AUTOID_IMPL(sysEpicEventSignInStateChanged);
AUTOID_IMPL(sysEpicEventSignInError);
AUTOID_IMPL(sysEpicEventAccessTokenChanged);

bool sysEpicEventSignInError::IsNoConnection() const
{
	return (m_ErrorCode == (int)EOS_EResult::EOS_NoConnection);
}

} // namespace rage

#endif // EPIC_API_SUPPORTED
