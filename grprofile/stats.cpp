//
// profile/stats.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "grprofile/stats.h"

#include "profiler.h"

#include "file/stream.h"
#include "data/callback.h"
#include "input/pad.h"
#include "grcore/device.h"
#include "grcore/font.h"

#include <stdarg.h>
#include <stdio.h>

using namespace rage;

#if __STATS

int pfStatsManager::m_Active = -1;
int pfStatsManager::m_Count;
fiStream * pfStatsManager::g_pstream = NULL;

namespace pfStatsManagerGlobals
{
	enum {maxPages = pfProfiler::kMaxStatsViews};
	atRangeArray<Functor0, maxPages> m_Pages;
}

void pfStatsManager::AddPage(Functor0 page,bool toFront) {
	using namespace pfStatsManagerGlobals;
	Assert(m_Count < maxPages);

	if (toFront)
	{
		// move the other down a bit
		for (int i=m_Count;i>0;i--)
			m_Pages[i] = m_Pages[i-1];
		m_Count++;
		m_Pages[0] = page;
	}
	else
	{
		m_Pages[m_Count++] = page;
	}
}

void pfStatsManager::RemovePage(Functor0 page) {
	using namespace pfStatsManagerGlobals;
	// Search for the page
	int i;
	for (i=0;i<m_Count;++i)
	{
		if (m_Pages[i] == page)
		{
			break;
		}
	}

	// Complain if we don't find it
	AssertMsg(i < m_Count, "pfStatsManager::RemovePage, page not found!");

	// Move the rest down on top of it
	for (; i < m_Count - 1; ++i)
	{
		m_Pages[i] = m_Pages[i+1];
	}

	m_Count--;
}


static int StatsFirst, StatsSkip;

bool pfStatsManager::Next() {
	StatsFirst = 0;
	if (++m_Active == m_Count)
		m_Active = -1;
	return m_Active >= 0;
}


bool pfStatsManager::Prev() {
	StatsFirst = 0;
	if (--m_Active == -2)
		m_Active = m_Count - 1;
	return m_Active >= 0;
}


static int StatsRow, StatsCol;

int pfStatsManager::Cull() {
	using namespace pfStatsManagerGlobals;
	StatsRow = 20;
	StatsCol = 20;
#if IS_CONSOLE && __DEBUGBUTTONS
	if ((ioPad::GetPad(0).GetPressedDebugButtons() & ioPad::LUP) && StatsFirst)
		--StatsFirst;
	else if (ioPad::GetPad(0).GetPressedDebugButtons() & ioPad::LDOWN)
		++StatsFirst;
#endif
	StatsSkip = StatsFirst;
	if (m_Active != -1) {
		m_Pages[m_Active]();
	}
	return m_Active;
}


void rage::pfStatsf(const char *fmt,...) {
	va_list args;
	va_start(args,fmt);
	char buf[256];

	if (StatsSkip) {
		--StatsSkip;
		return;
	}

	vsprintf(buf,fmt,args);

	if ( pfStatsManager::GetStream())
		fprintf( pfStatsManager::GetStream(), "%s\r\n", buf );

	if (StatsRow > GRCDEVICE.GetHeight()-grcFont::GetCurrent().GetHeight())
		return;

	// bool aa = RSTATE.GetAutoAlpha();
	// RSTATE.EnableAutoAlpha(false);
	grcFont::GetCurrent().Draw((float)StatsCol,(float)StatsRow,buf);
	StatsRow += grcFont::GetCurrent().GetHeight();
	// RSTATE.EnableAutoAlpha(aa);
	va_end(args);
}

#endif
