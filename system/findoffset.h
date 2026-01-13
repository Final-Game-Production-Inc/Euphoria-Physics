// 
// system/findoffset.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_FINDOFFSET_H
#define SYSTEM_FINDOFFSET_H

// This is a #include you can use to find the offset of a member in an object. 
// Use as follows:
//  #include "system/findoffset.h"
//  FindOffset(MyObject, m_Data);
//
// Compile that .cpp and it will give an error message containing the offset of the data member

// In both of these examples, FindOffset(MyObject, m_Data) was on line 44 of ptxmanager.cpp, and the offset was 16 bytes.
//
// Xenon error message:
//
// 1>e:\ragesoft\rage\base\src\system/findoffset.h(38) : error C2118: negative subscript
// 1>        .\ptxmanager.cpp(44) : see reference to class template instantiation 'FindAlignHelper<_Type,_Offset>' being compiled
// 1>        with
// 1>        [
// 1>            _Type=rage::ptxRuleBehavior,
// 1>            _Offset=16
// 1>        ]
// 
// PS3 error message:
//
// 1>../../../../rage/base/src/system/findoffset.h: At global scope:
// 1>../../../../rage/base/src/system/findoffset.h: In instantiation of 'FindOffsetHelper<rage::ptxRuleBehavior, 16u>':
// 1>e:/ragesoft/rage/suite/src/rmptfx/ptxmanager.cpp(44):   instantiated from here
// 1>../../../../rage/base/src/system/findoffset.h(38): error: creating array with negative size ('-0x00000000000000001')

template <typename _Type, size_t _Offset>
struct FindOffsetHelper
{
	CompileTimeAssert(_Offset == 99999);
};
#if __SPU
#define FindOffset(type, member) const FindOffsetHelper<type, __builtin_offsetof(type, member)> MacroJoin(MacroJoin(__FIND_OFFSET, __LINE__), __)
#else
#define FindOffset(type, member) const FindOffsetHelper<type, OffsetOf(type, member)> MacroJoin(MacroJoin(__FIND_OFFSET_, __LINE__), __)
#endif

#endif // SYSTEM_FINDALIGN_H

