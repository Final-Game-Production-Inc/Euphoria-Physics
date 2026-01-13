//
// grprofile/profiler.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

//
// The profiler class is the static interface to the
// profile system.
//

#ifndef GRPROFILE_PROFILER_H
#define GRPROFILE_PROFILER_H


//==============================================================
// external defines

#include "atl/bintree.h"
#include "diag/stats.h"
#include "data/base.h"
#include "grcore/viewport.h"
#include "profile/element.h"
#include "profile/group.h"
#include "profile/page.h"
#include "profile/profiler.h"

namespace rage {

#if !__STATS

#define PF_STATS_VIEW(x)									// empty macro for !__STATS

// empty profiler class to obviate #if __STATS checks
class pfProfileRenderer
{
public:
	pfProfileRenderer(pfProfiler&) {} // This is just here to ensure a profiler is created first

	static void Init (bool =true)							{ }
	static void Shutdown ()									{ }
	static void BeginFrame ()								{ }
	static void EndFrame ()									{ }
	static void Update (bool =false)						{ }
	static void UpdateInput ()								{ }
	static void Draw ()										{ }
    static void Draw (int, int)								{ }
};

#else // __STATS

class pfEKGMgr;
class pfStatsView;


//=============================================================================
// pfProfileRenderer
// PURPOSE
//   Renders profile groups 
//
// NOTES
//<TABLE>
//      Game Controller        Action
//      ---------------------  --------------------------------
//      Lup                    scale display up
//      Ldown                  scale display down
//      Lleft                  goto previous page
//      Lright                 goto next page
//      L3                     goto the next mode of the timer
//</TABLE>
//
//<TABLE>
//      Keyboard (Windows Only)  Action
//      -----------------------  ------------------------------------------
//      F2                       goto the next set of stats to display
//      F7 + SHIFT               goto the next page in the log manager
//      F8 + SHIFT               goto the next page in the EKG manager
//      F8 + SHIFT + CONTROL     goto the previous page in the EKG manager
//      F9 + SHIFT               scale the EKG manager display up
//      F10 + SHIFT              scale the EKG manager display down
//</TABLE>
//
// <FLAG Component>	
//
class pfProfileRenderer : public datBase
{
public:
	pfProfileRenderer(pfProfiler&) {} // This is just here to ensure a profiler is created first

	// all post static declaration initialization
	void Init (bool createWidgets=true);

	void Shutdown();

	// signal the beginning of a frame
	void BeginFrame ();

	// signal the end of a frame
	void EndFrame ();

	// update the various profiling components
	void Update (bool paused=false);

	// read inputs
	void UpdateInput ();

	// draw any graphical components
	void Draw ();

    // draw any graphical components, allows you to pass custom screen-dimensions
    void Draw (int screenWidth, int screenHeight);

	// return the next active page (NULL if end, first if NULL)
	pfPage * GetNextPage (const pfPage * curPage, bool onlyActivePages = true);

	// return the previous active page (NULL if start, last if NULL)
	pfPage * GetPrevPage (const pfPage * curPage, bool onlyActivePages = true);

	// debug dump of the current profile system
	void DebugView ();

	// are there any EKGs currently drawing?
	bool AnyEkgsActive ();

	// is the renderer enabled?
	bool m_Enabled;

#if __BANK
	// add the groups, and elements to the bank (part of Init)
	void AddCreateWidgetButton(bkBank& bank);
	void AddWidgets (bkBank& bank);
#endif

	pfEKGMgr* GetEkgMgr (int index);


	// PURPOSE: Old-system text "stats" pages of the profiler pages
	enum {kMaxStatsViews = (RSG_PC || RSG_DURANGO || RSG_ORBIS) ? 255 : 160 };
	
	static void DisplayGroupStatsf(pfGroup* group);
	static void DisplayTimerStatsf(pfTimer* timer);
	
protected:
	// PURPOSE: Has the renderer been initialized?  
	bool m_Initialized;

	// PURPOSE: Automatically pause when the game is paused
	bool m_AutoPause;

	pfEKGMgr * m_Ekgs [2];

	int m_NumStatsViews;

	atRangeArray<pfStatsView *, kMaxStatsViews> m_StatsViews;

	//======================================================
	// internal functions

	// register pages with stats view callback
	void CreateStatsViews();

	// delete those links
	void DeleteStatsViews();
};


//==============================================================
// pfStatsView: allows a pfPage to be displayed textually to the screen

// declare a pfStatsView
#define PF_STATS_VIEW(pagename)									pfStatsView PFSTATSVIEW_##pagename (&PFPAGE_##pagename)

class pfStatsView : public datBase
{
public:
	pfStatsView (pfPage * page);								// create this view and register with the STATS callback system
	~pfStatsView();

	void Display () const;

	pfPage * m_Page;											// the collection of groups to display
};



inline pfEKGMgr* pfProfileRenderer::GetEkgMgr (int index)
{
	Assert(index==0 || index==1);
	return m_Ekgs[index];
}

#endif	// __STATS

//==============================================================
// GetRageProfileRenderer
//
// PURPOSE:
//	This function simply makes available the static profile renderer, and,
//	by using a function instead of a static variable directly,
//	assures that the profiler is initialized before any use.
//	See C++ FAQ Lite: 10.13
//
pfProfileRenderer & GetRageProfileRenderer();

} // namespace rage

#endif  // GRPROFILE_PROFILER_H
