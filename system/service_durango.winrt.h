// 
// system/service_durango.winrt.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_SERVICE_DURANGO_WINRT_H
#define SYSTEM_SERVICE_DURANGO_WINRT_H

#if RSG_DURANGO
#include "service.h"

namespace rage
{
	class sysDurangoServiceEvent : public sysServiceEvent
	{
	public:
		sysDurangoServiceEvent() : sysServiceEvent(sysServiceEvent::UNKNOWN), m_Args(nullptr) {}
		sysDurangoServiceEvent(sysServiceEvent::Type eventType, Platform::Object^ args);
		virtual ~sysDurangoServiceEvent();

		const Platform::Object^ GetEventArgs() const;
		Platform::Object^ GetEventArgs();

	private:
		Platform::Object^ m_Args;
	};

	inline sysDurangoServiceEvent::sysDurangoServiceEvent(sysServiceEvent::Type eventType, Platform::Object^ args)
		: sysServiceEvent(sysServiceEvent::SYSTEM_EVENT, eventType)
		, m_Args(args)
	{}

	inline sysDurangoServiceEvent::~sysDurangoServiceEvent()
	{}

	inline const Platform::Object^ sysDurangoServiceEvent::GetEventArgs() const
	{
		return m_Args;
	}

	inline Platform::Object^ sysDurangoServiceEvent::GetEventArgs()
	{
		return m_Args;
	}
}
#endif // RSG_DURANGO
#endif // SYSTEM_SERVICE_DURANGO_WINRT_H
