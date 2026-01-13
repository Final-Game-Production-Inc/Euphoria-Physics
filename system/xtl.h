//
// system/xtl.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_XTL_H
#define SYSTEM_XTL_H

#ifdef ERROR_ON_XTL_H
#error "User-requested abort"
#endif

#define USE_D3D_DEBUG_LIBS	(__DEV && 0)		// If 1, we'll use d3d9d.lib
												// Note that this will automatically link with xapilibd.lib. See main.cpp.

#if __XENON
# pragma warning(push)
# pragma warning(disable: 4668)
# pragma warning(disable: 4062)
# if __DEV || __PROFILE  // let pix.h know we're a profile build
#  define PROFILE_BUILD
# endif

#ifndef _DEBUG
#define GAME_IS_NOT_USING_DEBUG
#endif // _DEBUG

#if USE_D3D_DEBUG_LIBS || !__OPTIMIZED
	#define D3D_DEBUG_INFO
	#ifndef _DEBUG
		#define _DEBUG 1
		#endif // _DEBUG
#endif // USE_D3D_DEBUG_LIBS

# include <xtl.h>

#ifdef GAME_IS_NOT_USING_DEBUG
	#undef _DEBUG
	#undef GAME_IS_NOT_USING_DEBUG
#endif // GAME_IS_NOT_USING_DEBUG

# if _XDK_VER < 1838
#  error "Rage requires XeDK 1838 or higher."
# endif
# pragma warning(pop)
#elif __WIN32PC || RSG_DURANGO
#ifndef STRICT
# define STRICT
#endif
# pragma warning(push)
# pragma warning(disable: 4668)
# include <WinSock2.h>  //Prevent windows.h from including winsock.h
# include <windows.h>
# pragma warning(pop)
#endif

#if __WIN32
#undef DeleteFile
#undef EnumProps
#undef GetCharWidth
#undef GetClassName
#undef GetCurrentTime
#undef GetObject
#undef PlaySound
#undef SetPort
#undef RegisterClass
// #undef SendMessage - This is used all over the place and rarely conflicts.
#undef PostMessage
#endif

#endif
