// 
// system/tls.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_TLS_H
#define SYSTEM_TLS_H

#if __WIN32PC
// #ifndef DBG
// #if __DEV
// #define DBG 1
// #else
// #define DBG 0
// #endif
// #endif
#include "file/file_config.h"
#endif

#if __PPU || defined(__SNC__) || (defined(__ORBIS__) && __clang_minor__ < 2)
#define __THREAD	__thread
#elif defined(__ORBIS__)
#define __THREAD __thread __attribute__((__tls_model__("initial-exec")))
#elif __SPU
#define __THREAD
#elif __TOOL
#define __THREAD
#elif defined(__RGSC_DLL) && (__RGSC_DLL == 1)
// Windows Server 2003 and Windows XP: The Visual C++ compiler supports a syntax that enables you to
// declare thread-local variables: _declspec(thread). If you use this syntax in a DLL, you will not
// be able to load the DLL explicitly using LoadLibrary on versions of Windows prior to Windows Vista.
#if __64BIT 
// we don't support anything prior to Windows Vista on x64 builds of the Social Club DLL so enable TLS
#define __THREAD	
#else
#define __THREAD	
#endif
#elif __WIN32
#define __THREAD	__declspec(thread)
#endif

#define DECLARE_THREAD_VAR(type,name)			__THREAD type name
#define DEFINE_THREAD_VAR(type,name)			__THREAD type name
#define INITIALIZE_THREAD_VAR(type,name,val)	__THREAD type name = val
#define STATIC_CAST_THREAD_PTR(type,name)		static_cast<type*>(name)

#define DECLARE_THREAD_PTR(type,name)			DECLARE_THREAD_VAR(type*,name)
#define DEFINE_THREAD_PTR(type,name)			DEFINE_THREAD_VAR(type*,name)
#define INITIALIZE_THREAD_PTR(type,name,val)	INITIALIZE_THREAD_VAR(type*,name,val)

#endif	// SYSTEM_TLS_H
