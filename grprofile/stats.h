//
// grprofile/stats.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRPROFILE_STATS_H
#define GRPROFILE_STATS_H

#include "atl/functor.h"
#include "diag/stats.h"

namespace rage {

class fiStream;

void pfStatsf(const char*,...);

#if __STATS


//=============================================================================
// pfStatsManager
// PURPOSE
//   Deprecated.
class pfStatsManager {
public:
	static void Reset() { m_Active = -1; m_Count = 0; }
	static bool Next();
	static bool Prev();
	static void MakeInactive()	{ m_Active = -1; }
	static void SetPage(int page) { FastAssert(page < m_Count); m_Active = page; }
	static bool IsActive() { return m_Active != -1; }
	static bool HasPages() { return m_Count != 0; }
	static void AddPage(Functor0, bool toFront=false);
	static void RemovePage(Functor0);
	static void SetStream ( fiStream * pstream ) { g_pstream = pstream; }
	static fiStream * GetStream () { return g_pstream; }

	static int Cull();
private:
	static int m_Active, m_Count;
	static fiStream * g_pstream;
};

extern pfStatsManager PFSTATS;

#endif

}	 // namespace rage

#endif
