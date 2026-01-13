//
// system/guid.h
//
// Copyright (C) 2016 Rockstar Games.  All Rights Reserved.
//
//
// This file provides a general purpose 128 bit unique identifier. Generation facilities are also included.
//

#ifndef SYSTEM_GUID_H
#define SYSTEM_GUID_H

#include "data/safestruct.h"
#include "data/serialize.h"
#include <utility>

//------------------------------------------------------------------------------------------------------------------
// SYS_IMPLEMENT_GUID
//
// PURPOSE
//	Helper macro for custom guid types inheriting from sysGuid. SYS_IMPLEMENT_GUID implements the required methods
//	for automatic conversion between the custom type and sysGuid.
//------------------------------------------------------------------------------------------------------------------
#define SYS_IMPLEMENT_GUID(GuidClass) \
GuidClass() : sysGuid() {} \
GuidClass(const sysGuid& other) { Copy(other); } \
GuidClass(sysGuid&& other) { Move(std::move(other)); } \
GuidClass& operator=(sysGuid&& other) {	Move(std::move(other));	return *this; } \
GuidClass& operator=(const sysGuid& other) { Copy(other); return *this; }

namespace rage {

//------------------------------------------------------------------------------------------------------------------
// sysGuid
//
// PURPOSE
//	Generic 128-bit guid. Default constructed guids have all bits to "0" and are considered invalid.
//------------------------------------------------------------------------------------------------------------------
class sysGuid
{
public:
	union
	{
		u64 m_data64[2];
		u32 m_data32[4];
		u16 m_data16[8];
		u8 m_data8[16];
	};

	sysGuid() {	Clear(); }

	explicit sysGuid(datResource&) {}

	bool IsValid() const { return !(m_data64[0] == 0 && m_data64[1] == 0); }

	void Clear()
	{
		m_data64[0] = 0;
		m_data64[1] = 0;
	}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct& s)
	{
		SSTRUCT_BEGIN(sysGuid)
		SSTRUCT_CONTAINED_ARRAY(sysGuid, m_data64)
		SSTRUCT_END(sysGuid)
	}
#endif // __DECLARESTRUCT

protected:
	void Move(sysGuid&& other)
	{
		m_data64[0] = other.m_data64[0];
		m_data64[1] = other.m_data64[1];
		other.m_data64[0] = 0;
		other.m_data64[1] = 0;
	}

	void Copy(const sysGuid& other)
	{
		m_data64[0] = other.m_data64[0];
		m_data64[1] = other.m_data64[1];
	}
};

// datSerialize operator
inline datSerialize& operator<<(datSerialize &ser, sysGuid& guid)
{
	ser << guid.m_data32[0];
	ser << guid.m_data32[1];
	ser << guid.m_data32[2];
	ser << guid.m_data32[3];
	return ser;
}

// Comparison operators
inline bool operator==(const sysGuid& left, const sysGuid& right)
{
	return left.m_data64[0] == right.m_data64[0] && left.m_data64[1] == right.m_data64[1];
}

inline bool operator!=(const sysGuid& left, const sysGuid& right)
{
	return !operator==(left, right);
}

} // namespace rage

#endif	// SYSTEM_GUID_H
