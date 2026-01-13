//
// system/durango_window.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef DURANGO_WINDOW_H
#define DURANGO_WINDOW_H

#if RSG_DURANGO

struct IUnknown;

namespace rage
{
	// PURPOSE:	Retrieves the device for the window on the current thread.
	IUnknown* GetWindowDeviceForThread();
}

#endif // RSG_DURANGO

#endif // DURANGO_WINDOW_H
