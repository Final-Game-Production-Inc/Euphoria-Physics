// 
// system/new_config.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_NEW_CONFIG_H
#define SYSTEM_NEW_CONFIG_H

#if !defined(__MFC) && __DEV && !__TOOL && !__SPU && (!defined(__RGSC_DLL) || !__RGSC_DLL)
#define RAGE_ENABLE_RAGE_NEW		1
#define RAGE_NEW_EXTRA_ARGS_UNUSED	,const char*,int
#define RAGE_NEW_EXTRA_ARGS			,const char* file,int line
#else
#define RAGE_ENABLE_RAGE_NEW		0
#define RAGE_NEW_EXTRA_ARGS_UNUSED
#define RAGE_NEW_EXTRA_ARGS
#endif

#endif
