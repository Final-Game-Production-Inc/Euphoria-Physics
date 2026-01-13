// 
// system/service.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_SERVICE_ORBIS_H
#define SYSTEM_SERVICE_ORBIS_H

#if RSG_ORBIS
#include "service.h"
#include <system_service.h>

// PlayTogether additions

enum
{
	SCE_SYSTEM_SERVICE_EVENT_PLAY_TOGETHER_HOST = 0x1000000d,
};

namespace rage
{
class sysOrbisServiceEvent : public sysServiceEvent
{
public:
	sysOrbisServiceEvent(sysServiceEvent::Type eventType, SceSystemServiceEvent systemEvent);
	virtual ~sysOrbisServiceEvent();

	const SceSystemServiceEvent& GetSytemServiceEvent() const;
	SceSystemServiceEvent& GetSytemServiceEvent();

private:
	SceSystemServiceEvent m_Event;
};

inline sysOrbisServiceEvent::sysOrbisServiceEvent(sysServiceEvent::Type eventType, SceSystemServiceEvent systemEvent)
	: sysServiceEvent(sysServiceEvent::SYSTEM_EVENT, eventType)
	, m_Event(systemEvent)
{}

inline sysOrbisServiceEvent::~sysOrbisServiceEvent()
{}

inline const SceSystemServiceEvent& sysOrbisServiceEvent::GetSytemServiceEvent() const
{
	return m_Event;
}

inline SceSystemServiceEvent& sysOrbisServiceEvent::GetSytemServiceEvent()
{
	return m_Event;
}
}
#endif // RSG_ORBIS

#endif // SYSTEM_SERVICE_ORBIS_H