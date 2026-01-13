//
// grprofile/timebars.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//
#ifndef GRPROFILE_TIMEBARS_H
#define GRPROFILE_TIMEBARS_H

// Rage headers
#include "atl/array.h"
#include "atl/map.h"
#include "profile/telemetry.h"
#include "profile/timebars.h"
#include "system/criticalsection.h"
#include "system/timer.h"

#define RAGE_TIMEBARS	((!__FINAL || __FINAL_LOGGING) && !__PROFILE && !__TOOL)

#if RAGE_TIMEBARS
#define RAGE_TIMEBARS_ONLY(x)	x
#else
#define RAGE_TIMEBARS_ONLY(x)
#endif 

#if RAGE_TIMEBARS

namespace rage {

#if RSG_ORBIS || RSG_DURANGO || RSG_PC
#define NUM_TIMEBAR_SETS	(10)
#else
#define NUM_TIMEBAR_SETS	(5)
#endif
#define NUM_OVER_BUDGET_BARS		(50)			// Max # of bars that can be called out for being over budget consecutively
#define MAX_FUNCTION_STACK_DEPTH	(15)
#define ENABLE_TIMEBAR_AVERAGE		(1)				// If enabled, keep an smoothed average for every function
#define AVERAGE_TIMEBAR_FACTOR		(0.95f)			// Smoothing factor for the average, close to 1.0f = more smoothing

// 2 to double-buffer timebars, 3 for triple-buffering, 4 for quad-buffering
#define TIMEBAR_BUFFERS 5

#define INCLUDE_DETAIL_TIMEBARS		(__DEV || __BANK)			// Omit minor timebars outside of Debug and Beta
#if INCLUDE_DETAIL_TIMEBARS
#define DETAIL_TIMEBARS_ONLY(x)	x
#else
#define DETAIL_TIMEBARS_ONLY(x)
#endif 

class bkBank;
class fiStream;

enum	eTIMEBAR_BUCKET
{
	TIMEBAR_BUCKET_DEFAULT = 0,
	TIMEBAR_BUCKET_STREAMING,
	TIMEBAR_BUCKET_WORLD_POP,
	TIMEBAR_BUCKET_ANIMATION,
	TIMEBAR_BUCKET_PEDS_OBJECT_UPDATE,
	TIMEBAR_BUCKET_VEHICLE_UPDATE,
	TIMEBAR_BUCKET_PHYSICS,
	TIMEBAR_BUCKET_SCRIPT,
	TIMEBAR_BUCKET_RENDER_UPDATE,
	TIMEBAR_BUCKET_VISIBILITY,
	TIMEBAR_BUCKET_SAFE_EXECUTION,
	TIMEBAR_BUCKET_BUILD_DRAW_LIST,
	TIMEBAR_BUCKET_GBUFFER,
	TIMEBAR_BUCKET_SHADOWS,
	TIMEBAR_BUCKET_REFLECTION,
	TIMEBAR_BUCKET_SSAO,
	TIMEBAR_BUCKET_DIRECTION_AMBIENT_LIGHT,
	TIMEBAR_BUCKET_ALPHA_OBJECTS,
	TIMEBAR_BUCKET_VFX,
	TIMEBAR_BUCKET_SSA,
	TIMEBAR_BUCKET_POSTFX,

	TIMEBAR_BUCKET_COUNT
};


//
// name:		pfTimeBars
// description:	Used to create hierarchical timing information. Stores two versions of this data one that is currently 
//				begin generated and one to render
class pfTimeBars
{
	friend class pfTimeBarMgr;
public:
	enum {
		INVALID_TIMEBAR = -1
	};
	// Defines structure to hold one functions data
	struct sFuncTime
	{
		const char *nameptr;
		float		startTime;
		float		endTime;
		float		budget;			// Budget for this task, 0.0f = no budget
		s16			childId;
		s16			parentId;
		s16			prevId;
		s16			nextId;
		bool		overBudget:1;
		bool		isIdle:1;
		bool		isSlower:1;
		bool		detail:1;
		s8			layer;
		u8			bucketId;
	};
	struct sMaxFuncTime
	{
		sMaxFuncTime()
			: maxTime(0.0f)
			, maxTimeCount(0)
			, baseline(0.0f)
#if ENABLE_TIMEBAR_AVERAGE
			, averageTime(0.0f)
#endif // ENABLE_TIMEBAR_AVERAGE
		{}

		const char *nameptr;
		float		maxTime;
		u32			maxTimeCount;
#if ENABLE_TIMEBAR_AVERAGE
		float		averageTime;
#endif // ENABLE_TIMEBAR_AVERAGE
		float		baseline;
	};

	struct sTimeBarBuckets
	{
	public:
		void Reset()
		{
			for(int i=0;i<TIMEBAR_BUCKET_COUNT;i++)
			{
				m_Buckets[i] = 0.0f;
			}
		}

		void AddTime(u32 bucketID, float time)
		{
			Assert(bucketID < TIMEBAR_BUCKET_COUNT);
			m_Buckets[bucketID] += time;
		}

		float m_Buckets[TIMEBAR_BUCKET_COUNT];
	};


	// Defines one frames worth of time bars
	struct sTimebarFrame
	{
		sTimebarFrame() : m_pTimes(NULL) ,m_number(0), m_startTime(0) 
		{
			m_Buckets.Reset();
		}

		const char *GetNameSafe(s32 id) const			{ return (id == -1) ? NULL : m_pTimes[id].nameptr; }

		size_t		ComputeCallstackHash(const sFuncTime &time) const;
		size_t		ComputeCallstackHash(s32 id) const;

		float		ComputeFrameTime(bool includeIdle = false) const;

		sFuncTime*	m_pTimes;

		// This the number of valid sFuncTime elements in m_pTimes (sequentially starting at 0).
		s16 		m_number;
		u64			m_startTime;
		u64			m_endTime;
		
		// This is the frame number provided by the thread, used to match up pfTimeBars.
		u32			m_frameNumber;

		sTimeBarBuckets	m_Buckets;
	};

	pfTimeBars();
	~pfTimeBars();

	// description:	Initial timebar class. Allocate space for timebars. Create semaphore to lock renderer
	// params: size = number of timebars
	void Init(const char* name, s32 size, float maxAllowedTime = 0.0f);
	// description: Set the start of a timebar frame. This swaps round the render and process timebars and resets
	//				all the timers.
	void FrameInit(u32 frameNumber);
	void FrameEnd();
	void GpuFrameInit(u32 frameNumber);
	void GpuFrameStartTime();
	void MarkGpuBegin(u32 frameNumber);

	// description: Set the beginning of a new timebar and finish the previous timebar
	void Start(const char *pName, const char * file, unsigned int line, bool detail, float budget = 0.0f, ProfileZoneType zoneType = PZONE_NORMAL);
	// description: Set the beginning of a new timebar and add it to the timebar stack so that any timebars
	//				created after are its children.
	void Push(const char *pName, const char * file, unsigned int line, bool detail, float budget = 0.0f, ProfileZoneType zoneType = PZONE_NORMAL);
	
	// PURPOSE: Insert a block with a known time
	void InsertBlock(const char *pName, float time, bool detail, float budget = 0.0f, bool isIdle = false);

	// description: Remove timebar from stack. Current timebar is finished
	void Pop(int detail);
	/**  Render timebars. This will render all the top-level bars and recurse down to the
	 *   selected ones.
	 *
	 *  PARAMS:
	 *   isPrimary - If true, this is the main timebar. This will result in additional information being displayed,
	 *               like the name of the currently selected event.
	 *   printName - If true, the name of this timebar will be printed as well.
	 *   offset - Vertical offset. A higher number will move this timebar further up. This value will be
	 *            automatically incremented, so the next call to Display() will display the next timebar
	 *            flush on top of the previously drawn timebars.
	 *   timeOffset - Horizontal offset, specified in milliseconds.
	 *   barIndex - Index of this timebar in the global array of bars. This is only relevant when using printName
	 *              as it will offset the name based on barIndex.
	 *   renderFrame - The index in the triple buffer of timebars to render. -1, the default, will use whatever
	 *                 is the current render index (m_renderFrame), but this value can be overriden, which is
	 *                 useful when trying to render a previous frame.
	 *   highlightName - Will draw events that have this very name in a different color if isPrimary is false.
	 *                   Note that this is done with a pointer comparison, not strcmp().
	 */	 
	void Display(bool isPrimary, bool printName, s32 &offset, float timeOffset, int barIndex, s32 renderFrame = -1, const char *highlightName = NULL, bool isGhost = false);
	// description: Dump stats to TTY
	void Dump(float threshold);  
	void DumpToFile(float threshold, fiStream* csv_stream);
	void Dump(int tb, int indent = 0, float threshold = 0.0f, fiStream* stream = NULL);
	void DumpXML(int tb, int indent, float threshold, fiStream* stream);
	void DumpTimebarFrameCounts();

	// description: Dump current profiling stack to TTY (used for hung thread tracking)
	void DumpCurrentStack();
	void TabbedDump();
	void TabbedDump(int tb, int indent = 0);

	s32 GetRenderFrame() const			{ return m_renderFrame; }
	u32 GetFrameNumber(int index) const	{ return m_frame[index].m_frameNumber; }
	u64 GetStartTime(int index) const	{ return m_frame[index].m_startTime; }

	void DisplayBudgets();
	float DisplayBudgets(int tb, int indent);
	void VerifyBudgets();
	float VerifyBudgets(float expectedBudget, int tb, int indent, int requiredLevel, float cutoff);
	sFuncTime *GetCurrentFunc() const;
	sFuncTime *GetSelectedFunc() const;

	// Specify how long this thread may take per frame before the system dumps the timebars to the TTY
	void SetMaxAllowedTime(float maxAllowedTime)			{ m_MaxAllowedTime = maxAllowedTime; }

	const sTimebarFrame &GetRenderTimebarFrame() const		{ return m_frame[m_renderFrame]; }

    const char* GetName() const { return m_name; }
	bool GetFunctionTotals(const char* pName, int &call_count, float &time);

#if __BANK
	void SelectTimebarByName(const char* pName);
#endif // BANK

private:
	void StartTimeBar(const char* pName, const char * file, unsigned int line, bool detail, float budget = 0.0f, ProfileZoneType zoneType = PZONE_NORMAL);
	void UpdateMaxAndAverage(sTimebarFrame &frame, s32 id);
	void EndTimeBar();
	void ProcessInput();
	int IsSelectedTimebar(s32 index);
	void FindSelectedTimebar();
	void StoreSelectedTimebarStack();
#if ENABLE_TIMEBAR_AVERAGE
	void AdjustAverages(s32 renderFrame, s32 tb, float time);
#endif // ENABLE_TIMEBAR_AVERAGE
	void AdjustBaselines(s32 renderFrame, s32 tb, s32 baseline, float time);
	void ResetBaseline();
	void RecordBaseline(s32 tb);

	// Double-buffered set of timebar elements
	atFixedArray<sTimebarFrame, TIMEBAR_BUFFERS> m_frame;
	atSimplePooledMapType<size_t, sMaxFuncTime> m_MaxTimes;
	const char* m_name;

	// Index in m_frame of the timebar currently being populated
	s32			m_processFrame;

	// Index in m_frame of the timebar currently being rendered
	s32			m_renderFrame;

	atFixedArray<s16, MAX_FUNCTION_STACK_DEPTH> m_functionStack;
	s32			m_size;
#if __DEV
	s32			m_requiredSize;
	s32			m_maxRequiredSize;
#endif // __DEV	
	s16			m_previous;
	s16			m_previousTiming;
	s32			m_layer;

	// Timer for the timebar. Reset to 0 on each call to FrameInit().
	sysTimer	m_timer;
	sysCriticalSectionToken m_renderCsToken;

	atFixedArray<const char *, MAX_FUNCTION_STACK_DEPTH> m_selectedNameStack;
	const char *m_selectedPrev;
	const char *m_selectedNext;
	s32			m_selectedLayer;
	s32 		m_selected;
	s32 		m_traceIndex;

	// Last timestamp at which we did an auto-dump
	u32			m_LastAutodumpTimestamp;

	// If we exceed this time, we'll dump the timebars to the TTY.
	float		m_MaxAllowedTime;

	// If true, we're not including this timebar in the absolute mode display
	bool		m_ExcludeFromAbsoluteMode;
};

struct pfOverBudgetBar
{
	const char	*m_TaskName;
	const char	*m_SlowestChildName;
	size_t		m_Callstackhash;
	int			m_ConsecutiveFrames;			// Number of consecutive frames over budget
	int			m_GoodFrames;					// Number of consecutive frames in budget
	int			m_Age;							// Number of consecutive frames without hitting a new peak
	float		m_Peak;							// Peak time value
	float		m_Budget;						// Budget value
	float		m_SlowestChildPeak;				// Peak time value of the slowest child
	char		m_ThreadIdentifier;				// Single ASCII character identifying the thread this occured on
	bool		m_BeenVisible;					// Has been shown in the display already
};

//
// name:		pfTimeBarMgr
// description:	Interface class for timebars. All the operations except Display() work on the timebars for this thread
class pfTimeBarMgr
{
public:
	pfTimeBarMgr();

	void Init(const char* name, s32 size, float maxAllowedTime = 0.0f, bool supportAbsoluteMode = true);
	void InitStartupBar(const char* name, s32 size);
	void InitGpuBar(const char* name, s32 size);

#if __BANK
	void AddWidgets(bkBank &bank);
#endif // __BANK

	pfTimeBars& GetStartupBar() { FastAssert(m_startup!=-1); return m_timeBarSets[m_startup]; }
	pfTimeBars& GetGpuBar() { FastAssert(m_gpu!=-1); return m_timeBarSets[m_gpu]; }
	const pfTimeBars& GetTimeBar(int bar) const			{ FastAssert((u32) bar < (u32) m_current); return m_timeBarSets[bar]; }
	s32 GetTimebarCount() const							{ return m_current; }

	void Kill(); //Kill The Thread off, but leave the data there
	void FrameInit(u32 frameNumber);
	void FrameEnd();
	void Start(const char* pName, const char * file, unsigned int line, bool detail, float budget = 0.0f, ProfileZoneType zoneType = PZONE_NORMAL);
	void Push(const char* pName, const char * file, unsigned int line, bool detail, float budget = 0.0f, ProfileZoneType zoneType = PZONE_NORMAL);
	void Pop(int detail=-1);
	void Display();
	// description: Update information about over-budget tasks
	void UpdateOverBudgets();
	// description: Display all over-budget tasks on the screen
	void DisplayOverBudgets();
	// add pix timers without adding timebars
	void StartPixTimer(const char* pName);
	void EndPixTimer();
	void MarkOverBudget(const pfTimeBars::sFuncTime &func, const pfTimeBars::sTimebarFrame &frame, const pfTimeBars &timebars, s32 id);
	void DumpTimebars(float threshold = 0.0f);			// Dump all stats to the TTY
	void DumpTimebarsXML(fiStream* stream, float threshold=0.0f);
	void DumpTimebarsToFile(float threshold=0.0f);

	void DisplayBudgets();
	void VerifyBudgets();
	void DumpCurrentStack();		//Dump all stacks tot he TTYE
	bool IsFrozen() const;
	pfTimeBars::sFuncTime *GetCurrentFunc() const;

	void AddToOverBudgetInclusionList(const char *taskName);

    const char* GetSetName(int timebarsetid = -1) const;
	bool GetFunctionTotals(const char* pName, int &call_count, float &time,int timebarsetid = -1);

	// Remap internal callbacks to game-specific debug keyboard.
#if !__FINAL
	static void SetKeyboardCallbacks(bool (*keyJustDown)(int,const char*),bool (*keyDown)(int,const char*));
#endif

	static void SetTextCallback(void (*DrawText)(float,float,const char*));
	static void SetTimebarDimensions(float x, float y, float height, float defaultFrameWidth);

	float GetRatedFramerateScale() { return m_ratedFramerateScale; }
	float GetRatedFramerate() { return m_ratedFramerate; }
	void SetRatedFramerate(float ratedFrameRate);
	void SetRatedFramerateScale(float ratedFrameRateScale);

	void UpdateRatedFrameRateBasedOnSettings(float targetFrameRate);

#if __BANK
	void SelectTimebarByName();
#endif // __BANK

	bool GetDisplayOverbudget() { return m_DisplayOverbudget; }
	void SetDisplayOverbudget(bool enable) { m_DisplayOverbudget = enable; }

	void DumpTimebarFrameCounts();

	float GetSelectedTime() const;

	bool GetShowAverages() const			{ return m_ShowAverages; }
	void ToggleShowBaselineDifference()		{ m_ShowBaselineDifference = !m_ShowBaselineDifference; }
	bool GetShowBaselineDifference() const	{ return m_ShowBaselineDifference; }
	bool GetShowBaselineSlowerOnly() const	{ return m_ShowBaselineDifferenceSlowerOnly; }
	void SetRecordBaseline()				{ m_RecordBaseline = true; }
	bool GetRecordBaseline() const			{ return m_RecordBaseline; }

	void SetDisableMoreTimeBars(bool disable) { m_disallowMoreTimeBars = disable; }

private:
	void DisplayOverBudgets(pfTimeBars &bars, float &yPos, float y2);

	// The total number of timebars we have in m_timeBarSets.
	s32 m_current;

	// Index of the timebar in m_timeBarSets to display.
	s32 m_display;

	// Initially -1, set to 0 after the first initialization, which indicates the index
	// of the startup timebar in m_timeBarSets.
	s32 m_startup;

	// Initially -1, then set to the index within m_timeBarSets of the GPU timebar. 
	s32 m_gpu;
	atFixedArray<pfOverBudgetBar, NUM_OVER_BUDGET_BARS> m_OverBudgetBars;
	atFixedArray<const char *, NUM_OVER_BUDGET_BARS> m_OverBudgetInclusionList;

	// All the timebars we're managing, typically one for each thread we care about, one for the
	// GPU, and one for startup.
	atFixedArray<pfTimeBars, NUM_TIMEBAR_SETS> m_timeBarSets;
	sysCriticalSectionToken	m_TimerCsToken;

	// If true, timebars are currently frozen.
	bool m_Frozen;
	bool m_DisplayOverbudget;
	bool m_AutoPauseTimeBars;

	// If true, the timebar rendering is in absolute mode.
	bool m_IsAbsoluteMode;

	// If true, we're displaying the averages of timebars, not the current value.
	bool m_ShowAverages;

	// If true, we're displaying the difference to a baseline.
	bool m_ShowBaselineDifference;
	bool m_ShowBaselineDifferenceSlowerOnly;
	bool m_RecordBaseline;

	bool m_IncludeDetailBars;
	bool m_IncludeDetailBarsToggle;		// Desired value for m_IncludeDetailBars, will it set at the beginning of a frame

	bool m_disallowMoreTimeBars;

	float m_ratedFramerate;
	float m_ratedFramerateScale;
};

// Stuff for setting the next timebar bucket ID
class pfTimeBarBucketSetHelper
{
public:

	pfTimeBarBucketSetHelper() { m_NextBucketId = TIMEBAR_BUCKET_DEFAULT; }

	void SetNextTimeBarBucket(u32 bucket) { m_NextBucketId = bucket; }
	u32 GetNextTimeBarBucket() 
	{ 
		u32	thisBucket = m_NextBucketId;
		m_NextBucketId = TIMEBAR_BUCKET_DEFAULT;
		return thisBucket; 
	}
private:
	u32	 m_NextBucketId;
};



extern pfTimeBarMgr g_pfTB;
extern pfTimeBarBucketSetHelper	g_pfTB_BucketSetHelper;
extern const char *timebarBucketNames[TIMEBAR_BUCKET_COUNT];


extern void (*g_pfStartupCallback)(const char*);
}	// namespace rage

// Recommended for new code:

#define PF_SET_NEXT_TIMEBAR_BUCKET(bucket)			::rage::g_pfTB_BucketSetHelper.SetNextTimeBarBucket(bucket)

#define PF_INIT_TIMEBARS(name, size, maxAllowedTime)	::rage::g_pfTB.Init(name,size,maxAllowedTime)
#define PF_INIT_TIMEBARS_NO_ABSOLUTE(name, size)	::rage::g_pfTB.Init(name,size,0.0f,false)
#define PF_FRAMEINIT_TIMEBARS(frameNumber)				::rage::g_pfTB.FrameInit(frameNumber)
#define PF_FRAMEEND_TIMEBARS()						::rage::g_pfTB.FrameEnd()
#define PF_DISPLAY_TIMEBARS()						::rage::g_pfTB.Display()
#define PF_DISPLAY_OVERBUDGET_TIMEBARS()			::rage::g_pfTB.DisplayOverBudgets()
#define PF_UPDATE_OVERBUDGET_TIMEBARS()				::rage::g_pfTB.UpdateOverBudgets()
#define PF_DUMP_STACK()								::rage::g_pfTB.DumpCurrentStack();
#define PF_DUMP_TIMEBARS()							::rage::g_pfTB.DumpTimebars()

#define PF_INIT_STARTUPBAR(name, size)				::rage::g_pfTB.InitStartupBar(name,size)
#define PF_BEGIN_STARTUPBAR()						::rage::g_pfTB.GetStartupBar().FrameInit(0)
#define PF_FINISH_STARTUPBAR()						::rage::g_pfTB.GetStartupBar().FrameInit(0)

#define PF_INIT_GPUBAR(name, size)					::rage::g_pfTB.InitGpuBar(name,size)
#define PF_BEGIN_GPUBAR(frameNumber)					::rage::g_pfTB.GetGpuBar().GpuFrameInit(frameNumber)
#define PF_INSERT_GPUBAR_BUDGETED(name,time, budget)::rage::g_pfTB.GetGpuBar().InsertBlock(name, time, false, budget)
#define PF_INSERT_GPUBAR(name, time)				::rage::g_pfTB.GetGpuBar().InsertBlock(name, time, false)
#define PF_MARK_GPU_BEGIN(frameNumber)				::rage::g_pfTB.GetGpuBar().MarkGpuBegin(frameNumber)

#else // !RAGE_TIMEBARS

#define PF_SET_NEXT_TIMEBAR_BUCKET(bucket)

#define PF_INIT_TIMEBARS(name,size,maxAllowedTime)	
#define PF_INIT_TIMEBARS_NO_ABSOLUTE(name, size)
#define PF_FRAMEINIT_TIMEBARS(frameNumber)
#define PF_FRAMEEND_TIMEBARS()
#define PF_DISPLAY_TIMEBARS()
#define PF_DISPLAY_OVERBUDGET_TIMEBARS()
#define PF_UPDATE_OVERBUDGET_TIMEBARS()
#define PF_DUMP_TIMEBARS()
#define PF_DUMP_TIMEBARS_THRESHOLD()
#define PF_DUMP_STACK()

#define PF_INIT_STARTUPBAR(name, size)				
#define PF_BEGIN_STARTUPBAR()						
#define PF_FINISH_STARTUPBAR()						

#define PF_INIT_GPUBAR(name,size)	
#define PF_BEGIN_GPUBAR(frameNumber)							
#define PF_INSERT_GPUBAR_BUDGETED(name,time, budget)
#define PF_INSERT_GPUBAR(name, time)				

#define PF_MARK_GPU_BEGIN(frameNumber)

#endif	// RAGE_TIMEBARS

#if __PS3
namespace rage {
#if !__FINAL
	PS3_ONLY(void pfDummySync() __attribute__((noinline));)
#else
	inline void pfDummySync() {}
#endif
}
#endif // __PS3

#endif //! GRPROFILE_TIMEBARS_H
