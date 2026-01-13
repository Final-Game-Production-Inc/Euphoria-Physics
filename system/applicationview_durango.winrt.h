//
// system/applicationview_durango.winrt.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_APPLICATIONVIEW_DURANGO_H
#define SYSTEM_APPLICATIONVIEW_DURANGO_H

#if RSG_DURANGO

// XDK headers.
#include <xdk.h>

#define ENABLE_DURANGO_SCREEN_DIMMING (1 && (_XDK_VER >= 11511))

namespace rage
{

// Application - implements the required functionality for a application
ref class sysApplicationView sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
    sysApplicationView();
    
    // IFrameworkView Methods
    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
    virtual void Load(Platform::String^ entryPoint);
    virtual void Run();
    virtual void Uninitialize();

protected:

	// Event Handlers
	void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
	void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);

	// TR: TODO: Currently, the args are not finalized!
	void OnResourcesAvailableChanged(Platform::Object^ sender, Platform::Object^ args);
	void OnResuming(Platform::Object^ sender, Platform::Object^ args);
	void OnExiting(Platform::Object^ sender, Platform::Object^ args);

	// NOTE: These events are on the CoreWindow not the ApplicationView. I originally put these inside sysService and set them up in sysService::InitClass()
	// but they need to be setup before we call CoreWindow::Activate() (or else we will miss some events) which we do inside OnActivate() here. For this 
	// reason they have been added here and push the events down to sysService.
	void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::VisibilityChangedEventArgs^ args);
	void OnSizeChanged(Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
	void OnInputFocusChanged(Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::InputEnabledEventArgs^ args);
	void OnWindowActivated(Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::WindowActivatedEventArgs^ args);

private:
#if ENABLE_DURANGO_SCREEN_DIMMING
	// PURPOSE: Indicates that screen dimming has be disabled.
	bool m_IsScreenDimmingDisabled;
#endif // ENABLE_DURANGO_SCREEN_DIMMING
};

// ApplicationSource - responsible for creating the Application instance 
// and passing it back to the system
ref class sysApplicationViewSource : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};

}

#endif // RSG_DURANGO

#endif // SYSTEM_APPLICATIONVIEW_DURANGO_H
