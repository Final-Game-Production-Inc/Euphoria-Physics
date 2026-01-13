// 
// system/epicevents.h
// 
// Copyright (C) 2019 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_EPIC_EVENTS_H
#define SYSTEM_EPIC_EVENTS_H

#include "file/file_config.h"

#if EPIC_API_SUPPORTED

// rage includes
#include "atl/delegate.h"
#include "data/autoid.h"

#include "epiccommon.h"

namespace rage
{

enum sysEpicEventType
{
	EPIC_EVENT_SIGN_IN_STATE_CHANGED,
	EPIC_EVENT_SIGN_IN_ERROR,
	EPIC_EVENT_ACCESS_TOKEN_CHANGED,
};

enum sysEpicSignInError
{
	SIGN_IN_FAILED = 1,
	SIGN_IN_AUTH_TOKEN_FAILED = 2,
};

//! Flags to indicate the type of signin event that has occurred.
enum sysEpicSignInStateFlags
{
	STATE_SIGNED_IN_USING_LOCAL_PROFILE = 0x0001,	//Gamer signed in using a local profile
	STATE_SIGNED_ONLINE = 0x0002,					//Gamer signed in online
	STATE_SIGNED_OUT = 0x0004,						//Gamer signed out
};

#define EPIC_EVENT_COMMON_DECL( name )\
	static unsigned EVENT_ID() { return name::GetAutoId(); }\
	virtual unsigned GetId() const { return name::GetAutoId(); }

#define EPIC_EVENT_DECL( name, id )\
	AUTOID_DECL_ID( name, rage::sysEpicEvent, id )\
	EPIC_EVENT_COMMON_DECL( name )

class sysEpicEvent
{
public:
	AUTOID_DECL_ROOT(sysEpicEvent);

	EPIC_EVENT_COMMON_DECL(sysEpicEvent);

	sysEpicEvent() {}
	virtual ~sysEpicEvent() {}
};

class sysEpicEventSignInStateChanged : public sysEpicEvent
{
public:
	EPIC_EVENT_DECL(sysEpicEventSignInStateChanged, EPIC_EVENT_SIGN_IN_STATE_CHANGED);

	explicit sysEpicEventSignInStateChanged(const sysEpicSignInStateFlags state)
		: m_State(state)
	{
	}

	sysEpicSignInStateFlags m_State;
};

class sysEpicEventSignInError : public sysEpicEvent
{
public:
	EPIC_EVENT_DECL(sysEpicEventSignInError, EPIC_EVENT_SIGN_IN_ERROR);

	explicit sysEpicEventSignInError(const sysEpicSignInError err, const int errorCode)
		: m_Error(err)
		, m_ErrorCode(errorCode)
	{
	}

	bool IsNoConnection() const;

	sysEpicSignInError m_Error;
	int m_ErrorCode;
};

class sysEpicEventAccessTokenChanged : public sysEpicEvent
{
public:
	EPIC_EVENT_DECL(sysEpicEventAccessTokenChanged, EPIC_EVENT_ACCESS_TOKEN_CHANGED);

	explicit sysEpicEventAccessTokenChanged(bool success, int errorCode)
		: m_Success(success)
		, m_ErrorCode(errorCode)
	{
	}

	bool m_Success;
	int m_ErrorCode;
};

typedef atDelegator<void(class sysEpic*, const class sysEpicEvent*)> sysEpicEventDelegator;
typedef sysEpicEventDelegator::Delegate sysEpicEventDelegate;

} // namespace rage

#endif // EPIC_API_SUPPORTED
#endif // SYSTEM_EPIC_EVENTS_H
