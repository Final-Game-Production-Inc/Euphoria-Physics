//
// system/applicationview_durango.winrt.cpp
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#if RSG_DURANGO

#include "applicationview_durango.winrt.h"

// Rage Headers
#include "system/service_durango.winrt.h"
#include "file/limits.h"
#include "input/keyboard.h"
#include "rline/rltitleid.h"
#include "rline/durango/rlxbl_interface.h"
#include "rline/durango/rlxblparty_interface.h"
#include "string/unicode.h"

// XDK headers.
#include <xdk.h>

// This macro appears to not be defined in some windows header files.
#ifndef NTDDI_WIN7SP1
#define NTDDI_WIN7SP1 (0x06010100)
#endif // NTDDI_WIN7SP1
#define    NOMINMAX

#pragma warning(push)
#pragma warning(disable: 4668) // Undefined preprocessor 'NTDDI_WIN7SP1'used in #if.
#include <Windows.h>
#pragma warning(pop)


#include <string.h>
#include <stdio.h>
#include <stdlib.h>


extern bool CommonMain_Prologue_DurangoWrapper(int argc,char **argv);
extern bool CommonMain_OneLoopIteration_DurangoWrapper();
extern void CommonMain_Epilogue_DurangoWrapper();

namespace rage
{
extern void SetThreadName(const char * name);
extern __THREAD char g_CurrentThreadName[16];
extern sysMemAllocator* s_pTheAllocator;

// PURPOSE: Wraps the various WinRT types into a separate Durango NameSpace (dns).
// NOTES:	This is because Microsoft have a habit of moving namespaces around, this way we can fix this in one place.
//			It also means we do not need to type out the excessively long names (especially when template are used such
//			as IVectorView.
namespace dns
{
	using namespace Windows::Foundation;
	using namespace Windows::ApplicationModel;
	using namespace Windows::ApplicationModel::Core;
	using namespace Windows::ApplicationModel::Activation;
	using namespace Windows::UI::Core;
	using namespace Windows::Xbox::Services;
}

sysApplicationView::sysApplicationView()
{
#if ENABLE_DURANGO_SCREEN_DIMMING
	m_IsScreenDimmingDisabled = false;
#endif // ENABLE_DURANGO_SCREEN_DIMMING
}


// Called by the system.  Perform application initialization here, 
// hooking application wide events, etc.
void sysApplicationView::Initialize(dns::CoreApplicationView^ applicationView)
{
    applicationView->Activated += ref new dns::TypedEventHandler<dns::CoreApplicationView^, dns::IActivatedEventArgs^>(this, &sysApplicationView::OnActivated);

	dns::CoreApplication::Suspending += ref new dns::EventHandler<Windows::ApplicationModel::SuspendingEventArgs^>(this, &sysApplicationView::OnSuspending);
	dns::CoreApplication::ResourceAvailabilityChanged += ref new dns::EventHandler<Platform::Object^>(this, &sysApplicationView::OnResourcesAvailableChanged);
	dns::CoreApplication::Resuming += ref new dns::EventHandler<Platform::Object^>(this, &sysApplicationView::OnResuming);
	dns::CoreApplication::Exiting += ref new dns::EventHandler<Platform::Object^>(this, &sysApplicationView::OnExiting);

#if _XDK_VER >= 11064
	// Reclaim the NUI GPU Reserve (4.5%)
	dns::CoreApplication::DisableKinectGpuReservation = true;
#endif
}



// Called when we are provided a window.
void sysApplicationView::SetWindow(dns::CoreWindow^)
{}


void sysApplicationView::Load(Platform::String^ entryPoint)
{
	/*
    m_game = ref new Game();
    m_game->Initialize(CoreWindow::GetForCurrentThread());
	*/
}


Platform::String^ g_Arguments;

static void CommonMain_ThreadEntry(void *)
{
#if !__NO_OUTPUT
	// Wipe out the current thread name from logs, it's redundant for the "main" thread
	g_CurrentThreadName[0] = 0;
#endif

#if _XDK_VER < 9386
	int argc = 2;
	char *argv[3];
	// DURANGO TODO:- Ip address etc.

	argv[0] = "Durango";
	argv[1] = "@x:\\v.txt";
	argv[2] = NULL;
#else
	// We get the command line as a single string without the executable name
	// command line parser stolen from _XENON ArchInit()
	const int maxChars = 2048;
	static char commandLine[maxChars];
	static char processName[maxChars];
	size_t convertedChars;
	wcstombs_s(&convertedChars, commandLine, maxChars, g_Arguments->Data(), _TRUNCATE);

#if __FINAL_LOGGING
	static wchar_t* wCommandLine = L"G:\\game_durango_final.exe -rline_all=debug3 -ragenet_all=debug3 -net_all=debug3 -net_all=debug3 -script_net_event_data_all=debug3 -nethttpdump -output -ttyframeprefix -logfile=xd:/console_final.log";
#else
	static wchar_t* wCommandLine = reinterpret_cast<wchar_t*>(GetCommandLine());
#endif

	wcstombs_s(&convertedChars, processName, maxChars, wCommandLine, _TRUNCATE);

	int argc = 0;
	char **argv = NULL;
	char *bp = commandLine;
	if (bp)
	{
		const int maxArgs = 64;
		static char *argv_buffer[maxArgs];
		argv = argv_buffer;

		char *start;
		char *pp = processName;
		while (*pp == '"')
		{
			pp++;
		}
		start = pp;
		while (*pp && *pp != '"')
		{
			pp++;
		}
		if (*pp)
		{
			*pp++ = 0;
		}

		if (strlen(start))
		{
			argv[argc++] = start;
		}
		else
		{
			argv[argc++] = "G:\\GTAV.exe";	// Can't get actual executable name
		}

		while (*bp && argc < maxArgs) 
		{
			while (*bp ==32 || *bp == 9)
			{
				bp++;
			}
			if (*bp == '"')
			{
				start = ++bp;
				while (*bp && *bp != '"')
				{
					bp++;
				}
			}
			else
			{
				start = bp;
				while (*bp && *bp != 32 && *bp != 9)
				{
					bp++;
				}
			}
			argv[argc++] = start;
			if (*bp)
			{
				*bp++ = 0;
			}
		}
		argv[argc] = 0;
	}
#endif

	// Initialise memory allocator. Normally done in InitGameHeap(), but since the allocator is
	// per-thread, we need to do it again here.
	sysMemAllocator::SetCurrent(*s_pTheAllocator);
	sysMemAllocator::SetMaster(*s_pTheAllocator);
	sysMemAllocator::SetContainer(*s_pTheAllocator);

#if __FINAL_LOGGING
	static const char* values[] = {
		"",
		"-rline_all=debug3",
		"-ragenet_all=debug3",
		"-net_all=debug3",
		"-snet_all=debug3",
		"-script_net_event_data_all=debug3",
		"-nethttpdump",
		"-output",
		"-ttyframeprefix",
		"-logfile=xd:/console_final.log"
	};
	char** gargv = (char**)(&values[0]);
	int gargc = 10;

	CommonMain_Prologue_DurangoWrapper(gargc, gargv);
#else
	CommonMain_Prologue_DurangoWrapper(argc, argv);
#endif

	for (;;)
	{
		CommonMain_OneLoopIteration_DurangoWrapper();
	}

	//this code is currently unreachable and will need sorting out with the above code

	//CommonMain_Epilogue_DurangoWrapper();
}

// Called by the system after initialization is complete.  This
// implements the traditional game loop
void sysApplicationView::Run()
{
#if !__FINAL
	SetThreadName("[RAGE] Main Application Thread");
#endif

	// Setup the keyboard from the main application thread, before the update thread spawns
	ioKeyboard::InitKeyboardFromMainThread();

	// Allocate something so we know the heaps are valid.
	delete rage_new char;

	// Spawn a new thread for the main game thread
	sysIpcCreateThread(CommonMain_ThreadEntry, NULL, 1024 * 1024,  PRIO_NORMAL, "[RAGE] Main Game Thread", 0);

#if ENABLE_DURANGO_SCREEN_DIMMING
	Windows::System::Display::DisplayRequest^ displayRequest = ref new Windows::System::Display::DisplayRequest();
#endif // ENABLE_DURANGO_SCREEN_DIMMING
	for (;;)
	{
		// ProcessEvents will throw if the process is exiting, allowing us to
		// break out of the loop.  This will be cleaned up when we get proper
		// process lifetime management online in a future release.
		dns::CoreWindow^ window = dns::CoreWindow::GetForCurrentThread();
		if(Verifyf(window != nullptr, "Failed to get window for thread, ensure this is called on the main thread!"))
		{
			window->Dispatcher->ProcessEvents(dns::CoreProcessEventsOption::ProcessAllIfPresent);
		}

#if ENABLE_DURANGO_SCREEN_DIMMING
		try
		{
			if(g_SysService.IsScreenDimmingDisabledThisFrame() && m_IsScreenDimmingDisabled == false)
			{
				// NOTE: We do this first before requesting the disable so that, if it fails, we don't try again.
				m_IsScreenDimmingDisabled = true;

				// We only try this once in case it throws an exception, otherwise we would keep trying every frame (throwing took over 1sec in my tests).
				if(Verifyf(displayRequest != nullptr, "Failed to create display request!"))
				{
					displayRequest->RequestActive();
				}
			}
			else if(g_SysService.IsScreenDimmingDisabledThisFrame() == false && m_IsScreenDimmingDisabled)
			{
				// NOTE: We do this first before requesting the disable so that, if it fails, we don't try again.
				m_IsScreenDimmingDisabled = false;

				// Don't release if m_WasDimRequestSuccessful is false as number of calls to RequestRelease must match number of calls to RequestActive.
				if(Verifyf(displayRequest != nullptr, "Trying to release a display dimming request when not setup!"))
				{
					displayRequest->RequestRelease();
				}
			}
		}
		catch(Platform::Exception^)
		{
			Assertf(false, "Failed handle screen dimming!");
		}
#endif // ENABLE_DURANGO_SCREEN_DIMMING

		Sleep(16);
	}
}

void sysApplicationView::Uninitialize()
{}

// Called when the application is activated.
void sysApplicationView::OnActivated(dns::CoreApplicationView^ applicationView, dns::IActivatedEventArgs^ args)
{
	// TODO: reorganize the logic here; Launch and default Protocol should do (mostly) the same thing, and invites etc may need to execute the launch logic if not already running

	try
	{
		Displayf("sysApplicationView::OnActivated with Kind %d", args->Kind);
		if (args->Kind == dns::ActivationKind::Launch)
		{
			dns::LaunchActivatedEventArgs^ launchArgs = static_cast<dns::LaunchActivatedEventArgs^>(args);
			g_Arguments = launchArgs->Arguments;

			dns::CoreWindow^ window = dns::CoreWindow::GetForCurrentThread();
			if(Verifyf(window != nullptr, "Failed to retrieve CoreWindow for this thread!"))
			{
				window->VisibilityChanged += ref new dns::TypedEventHandler<dns::CoreWindow^, dns::VisibilityChangedEventArgs^>(this, &sysApplicationView::OnVisibilityChanged);
				window->SizeChanged += ref new dns::TypedEventHandler<dns::CoreWindow^, dns::WindowSizeChangedEventArgs^>(this, &sysApplicationView::OnSizeChanged);
				window->InputEnabled += ref new dns::TypedEventHandler<dns::CoreWindow^, dns::InputEnabledEventArgs^>(this, &sysApplicationView::OnInputFocusChanged);
				window->Activated += ref new dns::TypedEventHandler<dns::CoreWindow^, dns::WindowActivatedEventArgs^>(this, &sysApplicationView::OnWindowActivated);
				window->Activate();
			}
		}
		else if(args->Kind == dns::ActivationKind::Protocol)
		{
			dns::IProtocolActivatedEventArgs^ proArgs = static_cast<dns::IProtocolActivatedEventArgs^>(args);

			unsigned titleid=0;
			if (g_rlTitleId==NULL)
			{
				// if not already cached, get the title id directly from the config
				if(swscanf_s(dns::XboxLiveConfiguration::TitleId->Data(), L"%u", &titleid) != 1)
					titleid = 0;
			}
			else
			{
				// theoretically this should be exactly the same as above, but better to fetch the cached copy if game is already running
				titleid = g_rlTitleId->m_XblTitleId.GetTitleId();
			}

			// TODO: not the best way to go about it, but will do for now
			char szInviteUri[RAGE_MAX_PATH] = {0};
			char szLaunchUri[RAGE_MAX_PATH] = {0};
			formatf(szInviteUri, "ms-xbl-%08x://partyInviteAccept/", titleid);
			formatf(szLaunchUri, "ms-xbl-%08x://default/", titleid);
			wchar_t wszInviteUri[RAGE_MAX_PATH];
			wchar_t wszLaunchUri[RAGE_MAX_PATH];
			Utf8ToWide((char16*)wszInviteUri, szInviteUri, COUNTOF(wszInviteUri));
			Utf8ToWide((char16*)wszLaunchUri, szLaunchUri, COUNTOF(wszLaunchUri));

			Displayf("sysApplicationView::OnActivated - Protocol with raw URI: %ls. Invite URI: %ls", proArgs->Uri->RawUri->Data(), wszInviteUri);

			// check the raw URI from protocol activation
			if(wcsncmp(proArgs->Uri->RawUri->Data(), wszLaunchUri, wcslen(wszLaunchUri)) == 0)
			{
				Displayf("sysApplicationView::OnActivated - Game Launched");

				// TODO: if this is even needed, we might need to improve the logic, maybe react to other states, like Suspended
				if(proArgs->PreviousExecutionState!=dns::ApplicationExecutionState::Running)
				{
					dns::CoreWindow^ window = dns::CoreWindow::GetForCurrentThread();
					if(Verifyf(window != nullptr, "Failed to retrieve CoreWindow for this thread!"))
					{
						// duplicate of standard Launch setup
						window->VisibilityChanged += ref new dns::TypedEventHandler<dns::CoreWindow^, dns::VisibilityChangedEventArgs^>(this, &sysApplicationView::OnVisibilityChanged);
						window->SizeChanged += ref new dns::TypedEventHandler<dns::CoreWindow^, dns::WindowSizeChangedEventArgs^>(this, &sysApplicationView::OnSizeChanged);
						window->InputEnabled += ref new dns::TypedEventHandler<dns::CoreWindow^, dns::InputEnabledEventArgs^>(this, &sysApplicationView::OnInputFocusChanged);
						window->Activated += ref new dns::TypedEventHandler<dns::CoreWindow^, dns::WindowActivatedEventArgs^>(this, &sysApplicationView::OnWindowActivated);
						window->Activate();
					}
				}
			}
			else if(wcsncmp(proArgs->Uri->RawUri->Data(), wszInviteUri, wcslen(wszInviteUri)) == 0)
			{
				// check if an XUID was specified
				u64 xuid = 0;

				char szInviteUriWithXUID[RAGE_MAX_PATH] = {0};
				formatf(szInviteUriWithXUID, "ms-xbl-%08x://partyInviteAccept/?accepterXuid=", titleid);
				wchar_t wszInviteUriWithXUID[RAGE_MAX_PATH];
				Utf8ToWide((char16*)wszInviteUriWithXUID, szInviteUriWithXUID, COUNTOF(wszInviteUriWithXUID));

				if(wcsncmp(proArgs->Uri->RawUri->Data(), wszInviteUriWithXUID, wcslen(wszInviteUriWithXUID)) == 0)
				{
					if(swscanf_s(proArgs->Uri->RawUri->Data() + wcslen(wszInviteUriWithXUID), L"%" LI64FMT L"u", &xuid) != 1)
					{
						Errorf("sysApplicationView::OnActivated - Invalid XUID");
					}
					else
					{
						Displayf("sysApplicationView::OnActivated - Invite Accepted with XUID: %" I64FMT "u", xuid);
					}
				}
				else
				{
					Displayf("sysApplicationView::OnActivated - No XUID");
				}

				// this
				rlXbl::FlagInviteAccepted(xuid);
			}
		}
	}
	catch (Platform::Exception^ ex)
	{
		Assertf(false, "Exception 0x%08x, Msg: %ls", ex->HResult, ex->Message != nullptr  ? ex->Message->Data() : L"NULL");						
	}
}

void sysApplicationView::OnSuspending( Platform::Object^, Windows::ApplicationModel::SuspendingEventArgs^ args)
{
	Displayf("sysApplicationView::OnSuspending");
	
	// Add a deferred SUSPENDED event...
	sysDurangoServiceEvent evt(sysServiceEvent::SUSPENDED, args);
	g_SysService.DeferEvent(&evt);

	//... and immediately trigger a SUSPEND_IMMEDIATE event
	sysDurangoServiceEvent evtImm(sysServiceEvent::SUSPEND_IMMEDIATE, args);
	g_SysService.TriggerEvent(&evtImm);
}

void sysApplicationView::OnResourcesAvailableChanged( Platform::Object^, Platform::Object^ args )
{
	try
	{
		// Use a switch so that we get a compile error if/when Microsoft adds a new enum value. For this reason there is
		// no default:
		switch (dns::CoreApplication::ResourceAvailability)
		{
		case Windows::ApplicationModel::Core::ResourceAvailability::Full:
#if _XDK_VER >= 11059
		case Windows::ApplicationModel::Core::ResourceAvailability::FullWithExtendedSystemReserve:	// TODO: Probably need a separate event for this.
#endif
			{
				sysDurangoServiceEvent evt(sysServiceEvent::UNCONSTRAINED, args);
				g_SysService.DeferEvent(&evt);
			}
			break;
		case Windows::ApplicationModel::Core::ResourceAvailability::Constrained:
			{
				sysDurangoServiceEvent evt(sysServiceEvent::CONSTRAINED, args);
				g_SysService.DeferEvent(&evt);
			}
			break;
		}
	}
	catch(Platform::Exception^)
	{}
}

void sysApplicationView::OnResuming( Platform::Object^, Platform::Object^ args )
{
	Displayf("sysApplicationView::OnResuming");
	
	// Add a deferred RESUMING event...
	sysDurangoServiceEvent evt(sysServiceEvent::RESUMING, args);
	g_SysService.DeferEvent(&evt);

	//... and immediately trigger a RESUME_IMMEDIATE event
	sysDurangoServiceEvent evtImm(sysServiceEvent::RESUME_IMMEDIATE, args);
	g_SysService.TriggerEvent(&evtImm);
}

void sysApplicationView::OnExiting( Platform::Object^, Platform::Object^ args )
{
	Displayf("sysApplicationView::OnExiting");
	
	sysDurangoServiceEvent evt(sysServiceEvent::EXITING, args);
	g_SysService.DeferEvent(&evt);
}

void sysApplicationView::OnVisibilityChanged( Windows::UI::Core::CoreWindow^, Windows::UI::Core::VisibilityChangedEventArgs^ args )
{
	if(args->Visible)
	{
		sysDurangoServiceEvent evt(sysServiceEvent::VISIBILITY_GAINED, args);
		g_SysService.DeferEvent(&evt);
	}
	else
	{
		sysDurangoServiceEvent evt(sysServiceEvent::VISIBILITY_LOST, args);
		g_SysService.DeferEvent(&evt);
	}
}

void sysApplicationView::OnSizeChanged( Windows::UI::Core::CoreWindow^, Windows::UI::Core::WindowSizeChangedEventArgs^ args )
{
	sysDurangoServiceEvent evt(sysServiceEvent::VISIBLE_SIZE_CHANGED, args);
	g_SysService.DeferEvent(&evt);
}

void sysApplicationView::OnInputFocusChanged( Windows::UI::Core::CoreWindow^, Windows::UI::Core::InputEnabledEventArgs^ args )
{
	if(args->InputEnabled)
	{
		sysDurangoServiceEvent evt(sysServiceEvent::INPUT_FOCUS_GAINED, args);
		g_SysService.DeferEvent(&evt);
	}
	else
	{
		sysDurangoServiceEvent evt(sysServiceEvent::INPUT_FOCUS_LOST, args);
		g_SysService.DeferEvent(&evt);
	}
}

void sysApplicationView::OnWindowActivated( Windows::UI::Core::CoreWindow^, Windows::UI::Core::WindowActivatedEventArgs^ args )
{
	switch (args->WindowActivationState)
	{
	case dns::CoreWindowActivationState::CodeActivated:
	case dns::CoreWindowActivationState::PointerActivated:
		{
			sysDurangoServiceEvent evt(sysServiceEvent::FOCUS_GAINED, args);
			g_SysService.DeferEvent(&evt);


			sysDurangoServiceEvent evtImm(sysServiceEvent::FOCUS_GAINED_IMMEDIATE, args);
			g_SysService.TriggerEvent(&evtImm);
		}
		break;
	case dns::CoreWindowActivationState::Deactivated:
		{
			sysDurangoServiceEvent evt(sysServiceEvent::FOCUS_LOST, args);
			g_SysService.DeferEvent(&evt);


			sysDurangoServiceEvent evtImm(sysServiceEvent::FOCUS_LOST_IMMEDIATE, args);
			g_SysService.TriggerEvent(&evtImm);
		}
		break;
	default:
		Assertf(false, "Unknown window activation state!");
		break;
	}
}


// Implements a IFrameworkView factory.
dns::IFrameworkView^ sysApplicationViewSource::CreateView()
{
    return ref new sysApplicationView();
}

}

#endif // RSG_DURANGO
