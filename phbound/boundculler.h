//
// phbound/boundculler.h
//
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDCULLER_H
#define PHBOUND_BOUNDCULLER_H

#include "phbound/primitives.h"		// This is just so that we can see the typedef for phPolygon::Index.
#include "phbound/bound.h"			// Just for phBound::MessagesEnabled

#define DEFAULT_MAX_CULLED_POLYS	8192

namespace rage {

// phBoundCuller functions mainly as a repository for storing which polygons were found as the result of a culling operation.  It is
//   also, somewhat peculiarly, used by shapetests to keep track of BVH bounds that have been culled from the physics world to avoid
//   the work of culling polygons from them again.  And finally it tracks some almost-unused information about cull shapes.
// The big advantage to having this as its own object (rather than having it be static or something) is that it allows more than one
//   client to be executing a cull operation on the same bound at the same time, such as might occur when multi-threading.
class phBoundCuller
{
public:
	// PURPOSE: default constructor
	phBoundCuller ();

	// PURPOSE: allocate arrays of culled polygon indices
	// PARAMS:
	//	maxCulledPolygons - optional maximum number of culled polygons
	void AllocateArrays (u32 maxCulledPolys=DEFAULT_MAX_CULLED_POLYS);

	// PURPOSE: take external lists for culled polygon indices
	// PARAMS:
	//	culledPolyIndexList - pointer to an array for storing culled polygon index numbers
	//	maxCulledPolys - the available size of culledPolyIndexList
	void SetArrays (phPolygon::Index* culledPolyIndexList, u32 maxCulledPolys);

	// PURPOSE: destructor
	~phBoundCuller ();

	// PURPOSE: Reset the bound culler.
	void Reset ();

	bool HasArrays () const;
	void AddCulledPolygonIndex (u16 polygonIndex);
	int AddCulledPolygonIndices(int startPolygonIndex, int polygonCount);

	u16 GetNumCulledPolygons () const;
	int GetMaxNumCulledPolygons () const;
	phPolygon::Index GetCulledPolygonIndex (int culledIndex) const;
	phPolygon::Index* GetCulledPolygonIndexList ();

protected:
	// The culler isn't really used for storing culling shape information any more, so these should get removed as soon as possible.  I went to remove them
	//   myself but there still used by octrees, quadtrees and BVHs.  As soon as the former two go away we should fix up the BVH to not use it any more and
	//   get rid of these here.

	// PURPOSE: the list of index numbers of culled polygons
	phPolygon::Index* m_CulledPolyIndexList;

	// PURPOSE: the maximum number of culled polygons
	u16 m_MaxCulledPolys;

	// PURPOSE: the number of culled polygons
	u16 m_NumCulledPolys;

	// PURPOSE: tell whether this culler created the culled polygon lists, and is therefore responsible for destroying them
	bool m_MyCulledLists;
};


inline phBoundCuller::phBoundCuller ()
{
	m_CulledPolyIndexList = NULL;
	m_MaxCulledPolys = 0;
	m_NumCulledPolys = 0;
	m_MyCulledLists = false;
}

inline void phBoundCuller::AllocateArrays (u32 maxCulledPolys)
{
	FastAssert(maxCulledPolys <= 0xffff);
	m_CulledPolyIndexList = rage_new phPolygon::Index[maxCulledPolys];
	m_MaxCulledPolys = (u16)maxCulledPolys;
	m_MyCulledLists = true;
}

inline void phBoundCuller::SetArrays (phPolygon::Index* culledPolyIndexList, u32 maxCulledPolys)
{
	FastAssert(maxCulledPolys <= 0xffff);
	m_CulledPolyIndexList = culledPolyIndexList;
	m_MaxCulledPolys = (u16)maxCulledPolys;
	m_MyCulledLists = false;
}

inline phBoundCuller::~phBoundCuller ()
{
	if (m_MyCulledLists)
	{
		delete[] m_CulledPolyIndexList;
	}
}

inline void phBoundCuller::Reset ()
{
	m_NumCulledPolys = 0;
}

inline bool phBoundCuller::HasArrays () const
{
	return (m_CulledPolyIndexList!=NULL);
}

inline int phBoundCuller::GetMaxNumCulledPolygons () const
{
	return m_MaxCulledPolys;
}


inline void phBoundCuller::AddCulledPolygonIndex (u16 polygonIndex)
{
	int totalNumCulled = m_NumCulledPolys;
	if (totalNumCulled<m_MaxCulledPolys)
	{
		m_CulledPolyIndexList[totalNumCulled] = polygonIndex;
		m_NumCulledPolys++;
	}
#if !__SPU && __ASSERT
	else if (phBound::MessagesEnabled())
	{
		static int s_PolyWarnCount = 0;
		if (s_PolyWarnCount < 10)
		{
			s_PolyWarnCount++;
			Warningf("phBoundCuller::AddCulledPolygonIndex skipped polygons after the array filled up."
						"Max size is %i and %i were culled.",m_MaxCulledPolys,totalNumCulled);
		}
	}
#endif // !__SPU
}


inline int phBoundCuller::AddCulledPolygonIndices(int startPolygonIndex, int polygonCount)
{
	int polyIndexListIndex = m_NumCulledPolys;
	FastAssert(polyIndexListIndex < m_MaxCulledPolys);
	int numToAdd = Min(m_MaxCulledPolys - polyIndexListIndex, polygonCount);

	m_NumCulledPolys = static_cast<u16>(m_NumCulledPolys + numToAdd);
	while(numToAdd > 0)
	{
		m_CulledPolyIndexList[polyIndexListIndex] = static_cast<phPolygon::Index>(startPolygonIndex);
		++polyIndexListIndex;
		++startPolygonIndex;
		--numToAdd;
	}

	return numToAdd;
}

inline u16 phBoundCuller::GetNumCulledPolygons () const
{
	return m_NumCulledPolys;
}

inline phPolygon::Index phBoundCuller::GetCulledPolygonIndex (int culledIndex) const
{
	TrapGE(culledIndex, m_MaxCulledPolys);
	return m_CulledPolyIndexList[culledIndex];
}

inline phPolygon::Index* phBoundCuller::GetCulledPolygonIndexList ()
{
	return m_CulledPolyIndexList;
}

} // namespace rage

#endif
