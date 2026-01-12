// 
// physics/sleepisland.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_SLEEPISLAND_H 
#define PHYSICS_SLEEPISLAND_H 

#include "grprofile/drawcore.h"

namespace rage {

class phSleepIsland
{
public:
	struct Id
	{
		Id()
			: index(0xffff)
			, generationId(0xffff)
		{
		}

		Id(u16 _index, u16 _generationId)
			: index(_index)
			, generationId(_generationId)
		{
		}

		u16 index;
		u16 generationId;
	};

	phSleepIsland(int numObjects);
	~phSleepIsland();

	static size_t ComputeSize(int numObjects);

	void SetId(Id id)
	{
		m_Id = id;
	}

	void SetObject(int objectIndex, int levelIndex);

	void SleepObjects();
	void WakeObjects();

#if __PFDRAW
	void ProfileDraw();
#endif

private:
	struct ObjectId
	{
		u16 levelIndex;
		u16 generationId;
	};

	ObjectId* m_Objects;
	int m_NumObjects;
	Id m_Id;
};

} // namespace rage

#endif // PHYSICS_SLEEPISLAND_H 
