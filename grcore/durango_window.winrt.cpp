//
// system/durango_window.winrt.cpp
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#if RSG_DURANGO

#include "durango_window.h"

namespace rage
{

IUnknown* GetWindowDeviceForThread()
{
	IUnknown* window = nullptr;
	try
	{
		window = reinterpret_cast< IUnknown* >(Windows::UI::Core::CoreWindow::GetForCurrentThread());
	}
	catch(Platform::Exception^)
	{
		Quitf("Failed to get window device!");
	}
	
	return window;
}

}
#endif // RSG_DURANGO

