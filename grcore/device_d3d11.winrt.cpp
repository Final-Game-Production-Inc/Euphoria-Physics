// 
// grcore/device_d3d11.winrt.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/device.h"
#include "grcore/gfxcontext.h"
#include "system/d3d11.h"
#include "system/hangdetect.h"
#include "system/service_durango.winrt.h"

#if (__D3D11 && RSG_WINRT)

namespace rage
{
#if RSG_DURANGO

sysIpcSema grcDevice::sm_PlmResume;

// PURPOSE:	The suspend deferral request.
// NOTES:	As this is a WinRT type it is not a member of grcDevice.
static Windows::ApplicationModel::SuspendingDeferral^ s_SuspendDeferral = nullptr;

void grcDevice::HandlePlmChange(sysServiceEvent *evt)
{
	if(evt != NULL)
	{
		if(evt->GetType() == sysServiceEvent::SUSPEND_IMMEDIATE && 
			evt->GetLevel() == sysServiceEvent::SYSTEM_EVENT) // ignores fake RAGE-suspend events
		{
			sysDurangoServiceEvent* durangoEvent = static_cast<sysDurangoServiceEvent*>(evt);
			Windows::ApplicationModel::SuspendingEventArgs^ args = dynamic_cast<Windows::ApplicationModel::SuspendingEventArgs^>(durangoEvent->GetEventArgs());
			s_SuspendDeferral = args->SuspendingOperation->GetDeferral();
		}
		else if(evt->GetType() == sysServiceEvent::RESUME_IMMEDIATE 
			&& evt->GetLevel() == sysServiceEvent::SYSTEM_EVENT)  // ignores fake RAGE-suspend events
		{
			sysIpcSignalSema(sm_PlmResume);
		}
		else if(evt->GetType() == sysServiceEvent::FOCUS_GAINED_IMMEDIATE)
		{
			sm_HasFocus = true;
		}
		else if(evt->GetType() == sysServiceEvent::FOCUS_LOST_IMMEDIATE)
		{
			sm_HasFocus = false;
		}
	}
}

void grcDevice::HandleSuspendResume()
{
	if(s_SuspendDeferral)
	{
		// For s_SuspendDeferral to be true, the main thread will be suspended inside void sysService::UpdateClass() just next to the safe zone/
		// Grabbing the context here is safe on the render thread.
		LockContext();

#if __D3D11_MONO_DRIVER
		g_grcCurrentContext->Suspend(0U);
#else
		// Get the performance context.
		ID3DXboxPerformanceContext* performanceContext = NULL;
		CHECK_HRESULT(g_grcCurrentContext->QueryInterface(__uuidof(ID3DXboxPerformanceContext), (void**)&performanceContext));
		performanceContext->Suspend(0U);
#endif // __D3D11_MONO_DRIVER

		DETECT_SUSPEND_TIMER_FAILURE_ONLY(sysSuspendFailureDetectStopTimer());
		s_SuspendDeferral->Complete();

		// NOTE: The thread should be blocked inside s_SuspendDeferral->Complete() until we resume, to handle any race conditions,
		//		 we wait until we are told we have resumed.
		sysIpcWaitSemaTimed(sm_PlmResume, sysIpcInfinite);

#if __D3D11_MONO_DRIVER
		g_grcCurrentContext->Resume();
#else
		performanceContext->Resume();
#endif // __D3D11_MONO_DRIVER

		s_SuspendDeferral = nullptr;
		UnlockContext();
	}
}

void grcDevice::SetNuiGPUReservation( bool bEnableReservation)
{
#if _XDK_VER >= 11064
	// Optionally reclaim the NUI GPU Reserve (4.5%)
	Windows::ApplicationModel::Core::CoreApplication::DisableKinectGpuReservation = !bEnableReservation;
#else
	UNREFERENCED_PARAMETER(bEnableReservation);
#endif
}

bool grcDevice::GetNuiGPUReservation()
{
#if _XDK_VER >= 11064
	return !Windows::ApplicationModel::Core::CoreApplication::DisableKinectGpuReservation;
#else
	// GPU is always reserved prior to June 2014 XDK
	return true;
#endif
}

#endif // RSG_DURANGO
}

#endif // (__D3D11 && RSG_WINRT)
