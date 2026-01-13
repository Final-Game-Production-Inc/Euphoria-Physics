//
// grprofile/ekg.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRPROFILE_EKG_H
#define GRPROFILE_EKG_H


//=============================================================================
// external defines

#include "diag/stats.h"
#include "grprofile/chart.h"
#include "vector/vector3.h"
#include "data/callback.h"

#if __STATS

namespace rage {

class bkBank;
class pfPage;
class bkCombo;
class pfTimer;
class pfValue;


//=============================================================================
// pfEKGLog
//
class pfEKGLog
{
public:
	void Reset ()												{m_NumSamples = 0; m_SampleStart = 0; SetColor();}
	void AddSample (float f);									// add a datapoint to the log

	enum {MAX_SAMPLES=300};										// maximum samples per log

	const Vector3 & GetColor()									{return m_Color;}

	enum {TimerLog, ValueLog};									// types of data

	void SetType (int type)										{m_Type = type;}

	int m_NumSamples;											// num samples in the log

	int m_SampleStart;											// starting index of the samples (circular buffer)

	float m_Samples[MAX_SAMPLES];								// stored samples

protected:
	void SetColor ();											// randomize the color

	int m_Type;													// type of data in this log
	Vector3 m_Color;											// color of this log
};


/////////////////////////////////////////////////////////////////////
// pfEKGMgr
// PURPOSE
//   Manager for the EKG views.  Controls selecting the page to view
//   and passing it to the viewer when needed.
//
// <FLAG Component>
//
class pfEKGMgr : public datBase
{
public:
	pfEKGMgr();

	void Update (bool paused=false);					// update the data structures
	void UpdateMouse (float x, float y);
    void Draw (int screenWidth,
		int screenHeight,
		float top,
		float bottom);									// display the current page
	void Draw (float top, float bottom);				// display the current page
	pfPage * GetPage ()									{return m_CurrentPage;}
	bool NextPage ();									// get the next page from the pfProfiler
	bool PrevPage ();									// get the previous page
	void SetPage (pfPage *page);						// set the current page

	// log control
	void Enable (bool);
	void ResetLogs ();									// erase all stored data
	void CopyLog (int dest, int src);					// copy data from log src to log dest, reset src

	// scaling
	void SetAutoScale (bool enable)						{m_AutoScale = enable;}
	void ScaleDisplayUp ()								{m_DisplayMax *= SCALING_FACTOR; m_DisplayMin *= SCALING_FACTOR; m_AutoScale=false;}
	void ScaleDisplayDown ()								{m_DisplayMax /= SCALING_FACTOR; m_DisplayMin /= SCALING_FACTOR; m_AutoScale=false;}
	void SetDisplayMax (float time)						{m_DisplayMax = time;}
	float GetDisplayMin()								{return m_DisplayMin;}
	float GetDisplayMax()								{return m_DisplayMax;}

	// sorting
	void SetAutoSort (bool enable)						{m_AutoSort = enable;}
	void SetShowValueType(int type)						{m_ShowValueType=type;}

#if __BANK
	void AddWidgets (bkBank & bank);						// add control widgets
#endif

protected:
	// PURPOSE: Find an empty log for a timer or value and assign it to that log.
	int InsertTimer (const pfTimer * timer);
	int InsertValue (const pfValue * value);

	// PURPOSE: Find a specific timer or value and remove it from the logs.
	void RemoveTimer (const pfTimer * timer);
	void RemoveValue (const pfValue * value);

	// PURPOSE: Find the log (index) for a timer or value.
	int FindTimer (const pfTimer * timer);
	int FindValue (const pfValue * value);

	// PURPOSE: Synch up timers and values to the active ones on the page
	void UpdateTimersAndValues ();

	// PURPOSE: Fill in sample data for this pass.
	void CollectSamples ();

	// PURPOSE: Sort all elements on this page (into sort buffers).
	void SortPageElements
		(int & foundTimers, int & foundValues, int bufferSize, const pfTimer ** timerBuffer, const pfValue ** valueBuffer);

	// data
	enum {kMaxLogs=24};

	// Is the EKG enabled?
	bool m_Enabled;

	// Note: This enum has corresponding strings in AddWidgets().
	enum
	{	kShowTimeTypeFrame,
		kShowTimeTypeAverage,
		kShowTimeTypePeak,
		kShowCallsTypeFrame,
		kShowCallsTypeAverage,
		kShowCallsTypePeak,
		kShowTypeNum
	};

	// Selects current, average, or peak values to display in timers.
	int m_ShowType;

	// Selects current, average, or peak values to display in values.
	int m_ShowValueType;

	// The page currently being displayed.
	pfPage * m_CurrentPage;

	// Which sample your maouse if hovering over.
	int m_HoverSlot;

	// The name of the current page.
	char m_CurrentPageName[128];

	// The int to timer mapping for currently active groups.
	const pfTimer * m_ActiveTimers[kMaxLogs];

	// The int to value mapping for currently active groups.
	const pfValue * m_ActiveValues[kMaxLogs];

	// The number of logs in use.
	int m_NumUsedLogs;

	// The actual log data.
	pfEKGLog m_Logs[kMaxLogs];

	// The logs not being used.
	int m_UnusedLogs[kMaxLogs];

	//=========================================
	// Graph range data.

	// Should the graph sort itself?
	bool m_AutoSort;

	// Should the graph scale itself?
	bool m_AutoScale;

	// Downscale the grahp
	bool m_AutoDownScale;

	// factor to scale up/down
	static const float SCALING_FACTOR;							

	// The maximum observed time
	float m_TimeMax;

	// The maximum observed value.
	float m_ValueMin;
	float m_ValueMax;

	static float sm_ValueMin;

	// The display axis maximum.
	float m_DisplayMax;
	float m_DisplayMin;

	// Time till reset of ValueMax, TimeMax and DisplayMax
	float m_ResetTime;

	int m_Samples;
	float m_TimeMod;

	float m_TimeEarliest;

	int m_LogOrder[kMaxLogs];

	// The chart object used to display graph data.
	pfChart m_Chart;

	// Page Selection Combo Box
#if __BANK
	void SelectPageComboCb ();							// select the page from the combo box
	int m_ComboPageSelection;							// current page selection in combo box
	bkCombo * m_SelectionPageCombo;						// combo box for page selection
#endif
};

}	// namespace rage

#endif // __STATS

#endif // !GRPROFILE_EKG_H

// EOF grprofile/ekg.h
