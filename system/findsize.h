// 
// system/findsize.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_FINDSIZE_H
#define SYSTEM_FINDSIZE_H

#if __PS3
#include <stdint.h>
#endif

// This is a #include you can use to find out how big an object is. 
// Use as follows:
//  #include "system/findsize.h"
//  FindSize(MyObject);
//
// Compile that .cpp and it will give an error message containing the size of your object

// In both of these examples, FindSize(MyObject) was on line 44 of ptxmanager.cpp, and the size was 432 bytes.
//
// Xenon error message:
//
// 1>e:\ragesoft\rage\base\src\system/findsize.h(38) : error C2118: negative subscript
// 1>        .\ptxmanager.cpp(44) : see reference to class template instantiation 'FindSizeHelper<_Type,Size>' being compiled
// 1>        with
// 1>        [
// 1>            _Type=rage::ptxRuleBehavior,
// 1>            _Size=432,
// 1>            _Alignment=16
// 1>        ]
// 
// PS3 error message:
//
// 1>../../../../rage/base/src/system/findsize.h: At global scope:
// 1>../../../../rage/base/src/system/findsize.h: In instantiation of 'FindSizeHelper<rage::ptxRuleBehavior, 432u>':
// 1>e:/ragesoft/rage/suite/src/rmptfx/ptxmanager.cpp(44):   instantiated from here
// 1>../../../../rage/base/src/system/findsize.h(38): error: creating array with negative size ('-0x00000000000000001')

template <typename _Type, size_t _Size, size_t _Alignment>
struct FindSizeHelper
{
	CompileTimeAssert(_Size==99999 && _Alignment==99999);
};

#define FindSize(x) const FindSizeHelper<x, sizeof(x), __alignof(x)> MacroJoin(MacroJoin(__FIND_SIZE_, __LINE__), __)

#endif