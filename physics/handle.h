// 
// physics/handle.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_HANDLE_H
#define PHYSICS_HANDLE_H

#include "math/amath.h"

namespace rage
{

#if __WIN32
#pragma warning(push)
#pragma warning(disable:4201)	// nonstandard extension used : nameless struct/union
#endif // __WIN32

class phHandle
{
public:
	phHandle();
	phHandle(u16 levelIndex, u16 generationId);
	explicit phHandle(u32 levelIndexAndGenerationId);

	u16 GetLevelIndex() const;
	u16 GetGenerationId() const;
	u32 GetLevelIndexAndGenerationId() const;

	void SetLevelIndex(u16 levelIndex);
	void SetGenerationId(u16 generationId);
	void SetLevelIndexAndGenerationId(u32 levelIndexAndGenerationId);

	bool operator==(phHandle rhs) const;
	bool operator!=(phHandle rhs) const;

private:
	union
	{
		struct
		{
			u16 m_LevelIndex;
			u16 m_GenerationId;
		};
		u32 m_LevelIndexAndGenerationId;
	};
};

#if __WIN32
#pragma warning(pop)
#endif // __WIN32

__forceinline phHandle::phHandle()
{
}
__forceinline  phHandle::phHandle(u16 levelIndex, u16 generationId)
:	m_LevelIndex(levelIndex),
m_GenerationId(generationId)
{
}
__forceinline  phHandle::phHandle(u32 levelIndexAndGenerationId)
: m_LevelIndexAndGenerationId(levelIndexAndGenerationId)
{
}

__forceinline u16 phHandle::GetLevelIndex() const
{
	return m_LevelIndex;
}
__forceinline u16 phHandle::GetGenerationId() const
{
	return m_GenerationId;
}
__forceinline u32 phHandle::GetLevelIndexAndGenerationId() const
{
	return m_LevelIndexAndGenerationId;
}

__forceinline void phHandle::SetLevelIndex(u16 levelIndex)
{
	m_LevelIndex = levelIndex;
}
__forceinline void phHandle::SetGenerationId(u16 generationId)
{
	m_GenerationId = generationId;
}
__forceinline void phHandle::SetLevelIndexAndGenerationId(u32 levelIndexAndGenerationId)
{
	m_LevelIndexAndGenerationId = levelIndexAndGenerationId;
}

__forceinline bool phHandle::operator==(phHandle rhs) const
{
	return GetLevelIndexAndGenerationId() == rhs.GetLevelIndexAndGenerationId();
}
__forceinline bool phHandle::operator!=(phHandle rhs) const
{
	return GetLevelIndexAndGenerationId() != rhs.GetLevelIndexAndGenerationId();
}

__forceinline size_t GenerateMaskEq(phHandle handle1, phHandle handle2)
{
	return GenerateMaskEq(handle1.GetLevelIndexAndGenerationId(),handle2.GetLevelIndexAndGenerationId());
}
__forceinline size_t GenerateMaskNE(phHandle handle1, phHandle handle2)
{
	return GenerateMaskNE(handle1.GetLevelIndexAndGenerationId(),handle2.GetLevelIndexAndGenerationId());
}

} // namespace rage

#endif // PHYSICS_HANDLE_H