//
// atl/align.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_ALIGN_H
#define ATL_ALIGN_H

#include "stddef.h"

namespace rage
{
/*
PURPOSE:
This template is for creating member variables holding a data buffer, with a specified alignment. Normally we'd be able to use
std::aligned_storage here, but MSVC 2008 doesn't support aligned_storage for larger than 8 bytes. 
*/
template<size_t SizeInBytes, size_t Align> struct atAlignedStorage;
template<size_t SizeInBytes> struct atAlignedStorage<SizeInBytes, 1> { u8 m_Data[SizeInBytes]; };
template<size_t SizeInBytes> struct atAlignedStorage<SizeInBytes, 2> { ALIGNAS(2) u8 m_Data[SizeInBytes]; };
template<size_t SizeInBytes> struct atAlignedStorage<SizeInBytes, 4> { ALIGNAS(4) u8 m_Data[SizeInBytes]; };
template<size_t SizeInBytes> struct atAlignedStorage<SizeInBytes, 8> { ALIGNAS(8) u8 m_Data[SizeInBytes]; };
template<size_t SizeInBytes> struct atAlignedStorage<SizeInBytes, 16> { ALIGNAS(16) u8 m_Data[SizeInBytes]; };
template<size_t SizeInBytes> struct atAlignedStorage<SizeInBytes, 32> { ALIGNAS(32) u8 m_Data[SizeInBytes]; };
template<size_t SizeInBytes> struct atAlignedStorage<SizeInBytes, 64> { ALIGNAS(64) u8 m_Data[SizeInBytes]; };
template<size_t SizeInBytes> struct atAlignedStorage<SizeInBytes, 128> { ALIGNAS(128) u8 m_Data[SizeInBytes]; };

}

#endif