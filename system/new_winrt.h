// 
// system/new_config.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_NEW_WINRT_H
#define SYSTEM_NEW_WINRT_H

#if RSG_DURANGO && RSG_DEV

#include "new.h"


#pragma warning(push)
#pragma warning(disable: 4211) // Non standard extension.

// PRUPOSE:	Fixes WinRT 'ref new' operator.
// NOTES:	This overrides the global new operator for the translation
//			unit. This will stop new asserting informing use to use
//			rage_new for the remainder of the translation unit.
//				For the this reason it should be avoided unless there are
//			problems with WinRT 'ref new' asserting. It will also affect
//			other files in the unity file.
static inline void* operator new(size_t size)
{
	return ::operator new(size, __FILE__, __LINE__);
}

// PRUPOSE:	Fixes WinRT 'ref new' operator.
// NOTES:	This overrides the global new operator for the translation
//			unit. This will stop new asserting informing use to use
//			rage_new for the remainder of the translation unit.
//				For the this reason it should be avoided unless there are
//			problems with WinRT 'ref new' asserting. It will also affect
//			other files in the unity file.
static inline void* operator new(size_t size, const std::nothrow_t&)
{
	return ::operator new(size, __FILE__, __LINE__);
}
#pragma warning(pop)

#endif // RSG_DURANGO && RSG_DEV
// 
#endif // SYSTEM_NEW_WINRT_H