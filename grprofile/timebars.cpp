//
// profile/timebars.cpp
//
// Copyright (C) 1999-2015 Rockstar Games.  All Rights Reserved.
//

#include "timebars.h"
#include "profile/cputrace.h"
#include "system/stack.h"

#if RAGE_TIMEBARS

// C headers
#include <stdio.h> 
#include <string.h> 

#include "file/asset.h"
#include "file/remote.h"
#include "file/stream.h"
#include "grcore/font.h"
#include "grcore/im.h"
#include "grcore/light.h"
#include "grcore/quads.h"
#include "grcore/setup.h" 
#include "grcore/stateblock.h"
#include "grcore/viewport.h"
#include "grprofile/pix.h"
#include "input/keyboard.h"
#include "input/keys.h"
#include "input/mapper.h"
#include "parsercore/streamxml.h"
#include "profile/timebars.h"
#include "string/string.h"
#include "string/stringutil.h"
#include "system/bootmgr.h"
#include "system/criticalsection.h"
#include "system/exception.h"
#include "system/param.h"
#include "system/threadtype.h"
#include "system/timemgr.h"
#include "system/tls.h"
#include "system/xtl.h"
#include "vector/colors.h"

static void DefaultStartupCallback(const char*) { }

namespace rage { void (*g_pfStartupCallback)(const char*) = DefaultStartupCallback; }

#if __XENON
#include "tracerecording.h"
#pragma comment (lib, "tracerecording.lib")
#elif __PPU
// #include <cell/gcm.h>
#include <sn/libsntuner.h>
#pragma comment(lib,"sntuner")
#elif RSG_ORBIS
#include <perf.h>		// device_gnm already pragma's the lib in.
#endif


//OPTIMISATIONS_OFF()


#define HASH_VALUE_DEBUG	(0)

#if HASH_VALUE_DEBUG
#define HASH_VALUE_DEBUG_ONLY(_x) _x
#else // HASH_VALUE_DEBUG
#define HASH_VALUE_DEBUG_ONLY(_x)
#endif // HASH_VALUE_DEBUG

namespace rage {

#if HASH_VALUE_DEBUG
static size_t hashValueCompare = 0;
#endif // HASH_VALUE_DEBUG

// --- defines ---
#define TB_X		(0.3f)
#define TB_Y		(0.88f)
#define TB_HEIGHT	(0.01f)
#define TB_TEXT		(0.021f)
#define TB_SCALE	(6.0f)
#define TB_SCROLL_SPEED (5.0f)

#define OVERBUDGET_LENGTH	(30*10)		// Number of frames an over-budget message will be shown

#define MISSING_START_TIME (0x40000000)	// If this in a timebar frame number, it means that we don't
										// have the proper start time yet and are still waiting for a GPU
										// callback.

#define AUTO_TIMEBAR_DUMP_MIN_DELAY (300)	// Automatic timebar dumps due to long frames must be at least this
											// many frames apart.

const char *timebarBucketNames[TIMEBAR_BUCKET_COUNT] =
{
	"DEFAULT",
	"STREAMING",
	"WORLD_POP",
	"ANIMATION",
	"PEDS_OBJECT_UPDATE",
	"VEHICLE_UPDATE",
	"PHYSICS",
	"SCRIPT",
	"RENDER_UPDATE",
	"VISIBILITY",
	"SAFE_EXECUTION",
	"BUILD_DRAW_LIST",
	"GBUFFER",
	"SHADOWS",
	"REFLECTION",
	"SSAO",
	"DIRECTION_AMBIENT_LIGHT",
	"ALPHA_OBJECTS",
	"VFX",
	"SSA",
	"POSTFX"
};

#define TIMEBAR_CSV_FILE "common:/data/timebar"

// --- Global variables ---
pfTimeBarMgr g_pfTB;

pfTimeBarBucketSetHelper	g_pfTB_BucketSetHelper;

// thread local variable
__THREAD s32 gTimebarSetId = -1;

// --- Static Global variables ---
static const char *g_traceTimebarName;
static bool g_traceCurrentTimebar = false;
static bool s_ShowStallsInGhostsOnly = true;
#if __XENON
static u32 g_traceTimebarIndex = 0;
static s32 g_traceFilenumber = 0;
#endif
static float s_TimeBarX = TB_X;
static float s_TimeBarY = TB_Y;
static float s_TimeBarHeight = TB_HEIGHT;
static float s_TimeTextHeight = TB_TEXT;
static float s_TimebarScale = TB_SCALE;
static float s_TimebarOffset = 0.0f;
static float s_TimebarOffsetInertia = 0.0f;

#if !__FINAL
static bool GetKeyJustDown_RAGE(int key,const char *);
static bool GetKeyDown_RAGE(int key,const char *);
static bool (*s_GetKeyJustDown)(int,const char*) = GetKeyJustDown_RAGE;
static bool (*s_GetKeyDown)(int,const char*) = GetKeyDown_RAGE;
#endif

static void DrawText_Rage(float px,float py,const char *buf);
static void DrawText_RageColor(float px,float py,const char *buf,Color32 color);
static int GetStringWidth_Rage(const char *buf);

static void (*s_DrawTextColor)(float,float,const char*,Color32) = DrawText_RageColor;
static void (*s_DrawText)(float,float,const char*) = DrawText_Rage;
static int (*s_GetStringWidth)(const char*) = GetStringWidth_Rage;

static Color32 AColor(128,128,128);
static Color32 BColor(0,0,220);
static Color32 SelectedColor(255,255,255);
static Color32 OverBudgetColor(255,0,0);
static Color32 AIdleColor(0xd6,0x8b,0x1a);
static Color32 BIdleColor(0xad,0x71,0x15);
static Color32 SelectedOverBudgetColor(255,128,128);
static Color32 TimebarOverBudgetColor(255,64,64);
static Color32 SameNameColor(255,240,0);
static sysCriticalSectionToken	s_CS;
static s32 s_spikeFrameCount = 3;
static bool s_ShowHierarchicalOverbudget = true;

static int maxRenderedLayer = 0;

PARAM(checkoverbudgetonly, "[profile] Only care about these systems when checking for execution time excesses");
PARAM(displaySpikes, "[profile] When set, over budget spikes will be displayed.");
PARAM(spikeFrameCount, "[profile] Under which number of frame is a timebar change considered a spike.");
PARAM(ratedframerate, "[profile] The framerate we're currently supporting");
PARAM(autopausetimebars, "[profile] When game is paused also pause the timebars");
PARAM(absolutemodetimebars, "[profile] Start timebars off in absolute mode");
PARAM(showdetailtimebars, "[profile] Include the minor timebars");

size_t pfTimeBars::sTimebarFrame::ComputeCallstackHash(const sFuncTime &time) const
{
	size_t result = (size_t) time.nameptr;
	s16 parentId = time.parentId;

	while (parentId != -1)
	{
		// Roll the hash around so we don't get zero when getting the same string twice.
		size_t rollOver = result >> ((sizeof(size_t) * 8) - 1);
		result <<= 1;
		result |= rollOver;
		sFuncTime &parent = m_pTimes[parentId];
		result ^= (size_t) time.nameptr;
		parentId = parent.parentId;
	}

	return result;	
}

size_t pfTimeBars::sTimebarFrame::ComputeCallstackHash(s32 id) const
{
	return ComputeCallstackHash(m_pTimes[id]);
}

float pfTimeBars::sTimebarFrame::ComputeFrameTime(bool includeIdle) const
{
	float result = 0.0f;

	if (m_number != 0)
	{
		// Get the first element
		s32 id = 0;

		while (id != -1)
		{
			const sFuncTime &time = m_pTimes[id];

			// Tally up the time this took, unless we're meant to exclude idle elements.
			if (includeIdle || !time.isIdle)
			{
				result += time.endTime - time.startTime;
			}

			// Next one.
			id = m_pTimes[id].nextId;
		}
	}

	return result;
}

//
// name:		pfTimeBars::pfTimeBars
// description:	Constructor
pfTimeBars::pfTimeBars() :
m_selected(0),
m_ExcludeFromAbsoluteMode(false),
m_LastAutodumpTimestamp(0),
m_frame(TIMEBAR_BUFFERS),
m_functionStack(MAX_FUNCTION_STACK_DEPTH),
m_selectedNameStack(MAX_FUNCTION_STACK_DEPTH)
{
}

//
// name:		pfTimeBars::~pfTimeBars
// description:	
pfTimeBars::~pfTimeBars()
{
	sysMemAutoUseDebugMemory mem;

	for (int x=0; x<TIMEBAR_BUFFERS; x++)
	{
		delete[] m_frame[x].m_pTimes;
	}
}

//
// name:		pfTimeBars::Init
// description:	Initialise timebar set
void pfTimeBars::Init(const char* name, s32 num, float maxAllowedTime)
{
	sysMemAutoUseDebugMemory mem;

	// We store IDs as s16s, so we can't allow for more than 32767.
	Assert(num < 0x8000);

	m_name = name;
	for (int x=0; x<TIMEBAR_BUFFERS; x++)
	{
		m_frame[x].m_pTimes = rage_new sFuncTime[num];
	}
	// don't want to double buffer max times as they have to be the same across both buffers, plus because the render
	// only reads from it while the process reads and writes we don't have to worry about about the max times getting 
	// screwed up
	m_MaxTimes.Init((u16) num);

	m_size = num;
#if __DEV
	m_requiredSize = num;
	m_maxRequiredSize = num; 
#endif // __DEV	
	m_processFrame = 1;
	m_renderFrame = 0;
	m_previous = INVALID_TIMEBAR;
	m_previousTiming = INVALID_TIMEBAR;
	m_MaxAllowedTime = maxAllowedTime;

	PARAM_spikeFrameCount.Get(s_spikeFrameCount);

	FrameInit(0);
}

//
// name:		pfTimeBars::FrameInit
// description:	Initialise timebars for this frame. End previous timebars, clear timers, swap buffers.
void pfTimeBars::FrameInit(u32 frameNumber)
{
	// finish last timebar of previous frame
	if(m_previousTiming != INVALID_TIMEBAR)
		EndTimeBar();

#if __ASSERT
	s32 layer = m_layer;
	Assertf((m_layer < MAX_FUNCTION_STACK_DEPTH), "Function stack overflow %d of %d", m_layer, MAX_FUNCTION_STACK_DEPTH);
	while (m_layer)
	{
		s32 previous = m_functionStack[m_layer];
		sFuncTime &timeBar = m_frame[m_processFrame].m_pTimes[previous];
		Errorf("LEFT-OVER TIMEBAR: %s", timeBar.nameptr);
		Pop(-1);
	}
	Assertf(layer == 0, "Mismatched timebar - there's a PF_PUSH_TIMEBAR without a matching PF_POP_TIMEBAR. See TTY for a list of left-over timebars to get an idea where the mismatch might be.");
#endif // __ASSERT

	// wait on render to finish. and block access until process and render frame are swapped
	{
		SYS_CS_SYNC(m_renderCsToken);
		if (!g_pfTB.IsFrozen())
		{
			m_processFrame = (m_processFrame + 1) % TIMEBAR_BUFFERS;
			m_renderFrame = (m_renderFrame + 1) % TIMEBAR_BUFFERS;
			m_frame[m_processFrame].m_frameNumber = frameNumber;

			m_frame[m_processFrame].m_Buckets.Reset();

			// How long did the previous frame take?
			if (m_MaxAllowedTime != 0.0f && fiGetFramesSinceLastMessageBox() > 1 && m_LastAutodumpTimestamp - TIME.GetFrameCount() > AUTO_TIMEBAR_DUMP_MIN_DELAY)
			{
				float frameTime = m_frame[m_renderFrame].ComputeFrameTime(true);

				if (frameTime > m_MaxAllowedTime)
				{
					SYS_CS_SYNC(m_renderCsToken);
					Warningf("%s took %.fms. Here is a full timebar dump:", m_name, frameTime);
					TabbedDump(0);

					m_LastAutodumpTimestamp = TIME.GetFrameCount();
				}
			}
		}
	}

	// reset variables for the next frame
	m_timer.Reset();	
	m_functionStack[0] = INVALID_TIMEBAR;
	m_frame[m_processFrame].m_number = 0;
	m_layer = 0;
	m_previous = INVALID_TIMEBAR;
	m_previousTiming = INVALID_TIMEBAR;

	m_traceIndex = -1;
#if __DEV
	if( m_requiredSize > m_maxRequiredSize )
		Warningf("Out of timebars: %d/%d",m_requiredSize,m_size);
	m_maxRequiredSize = Max(m_maxRequiredSize,m_requiredSize);
	m_requiredSize = m_size;
#endif // __DEV

	m_frame[m_processFrame].m_startTime = sysTimer::GetTicks();

	// start the frame	
	Start("Start", __FILE__, __LINE__, false);
}

void pfTimeBars::FrameEnd()
{
	m_frame[m_processFrame].m_endTime = sysTimer::GetTicks();
}

void pfTimeBars::GpuFrameInit(u32 frameNumber)
{
	// wait on render to finish. and block access until process and render frame are swapped
	{
		if (!g_pfTB.IsFrozen())
		{
			m_processFrame = (m_processFrame + 1) % TIMEBAR_BUFFERS;
			m_renderFrame = (m_renderFrame + 1) % TIMEBAR_BUFFERS;
			m_frame[m_processFrame].m_frameNumber = frameNumber | MISSING_START_TIME;

			m_frame[m_processFrame].m_Buckets.Reset();
		}
	}

	// reset variables for the next frame
	m_timer.Reset();	
	m_functionStack[0] = INVALID_TIMEBAR;
	m_frame[m_processFrame].m_number = 0;
	m_previous = INVALID_TIMEBAR;
	m_previousTiming = INVALID_TIMEBAR;

	m_traceIndex = -1;
#if __DEV
	if( m_requiredSize > m_maxRequiredSize )
		Warningf("Out of timebars: %d/%d",m_requiredSize,m_size);
	m_maxRequiredSize = Max(m_maxRequiredSize,m_requiredSize);
	m_requiredSize = m_size;
#endif // __DEV
	m_frame[m_processFrame].m_startTime = sysTimer::GetTicks();
}

void pfTimeBars::MarkGpuBegin(u32 frameNumber)
{
	// If we're frozen, we never created a new timebar, so there's nothing to mark here.
	if (g_pfTB.IsFrozen())
	{
		return;
	}

	// Only look at the lowest 16 bits - that's good enough. We're losing some of the upper
	// bits as the value is passed down the GPU callback.
	u32 maskedFrameNumber = frameNumber & 0xffff;

	// Find the frame number.
	for (int x=0; x<TIMEBAR_BUFFERS; x++)
	{
		if ((m_frame[x].m_frameNumber & 0xffff) == maskedFrameNumber)
		{
			// Found it.
#if !__WIN32
			if (!(m_frame[x].m_frameNumber & MISSING_START_TIME))
			{
				g_pfTB.DumpTimebarFrameCounts();
				Displayf("Frame number: %08x", frameNumber);
			}

			// We can't assert on WIN32 in GPU callbacks.
			Assertf(m_frame[x].m_frameNumber & MISSING_START_TIME, "Received GPU timing information about frame %d twice (Timebar %s, %d). See TTY for more information.",
				frameNumber, m_name, x);
#endif // !__WIN32

			m_frame[x].m_startTime = sysTimer::GetTicks();
			m_frame[x].m_frameNumber &= ~MISSING_START_TIME;

			return;
		}
	}

	// If we didn't find it, it may have taken the GPU an absurb amount of time to get crackin'.
	// OR we just returned from a loading screen, and update thread and render thread are getting
	// back in sync.
}

void pfTimeBars::GpuFrameStartTime()
{
//	m_frame[m_processFrame].m_startTime = sysTimer::GetTicks();
}

//
// name:		pfTimeBars::Start
// description:	Start a new timebar
void pfTimeBars::Start(const char* pName, const char * file, unsigned int line, bool detail, float budget, ProfileZoneType zoneType)
{
	StartTimeBar(pName, file, line, detail, budget, zoneType);

	// set this timer to be the previous timer
	m_previousTiming = m_previous = m_frame[m_processFrame].m_number - 1;

	RAGETRACE_STARTNAME(pName);
}

void pfTimeBars::InsertBlock(const char *pName, float time, bool detail, float budget/* = 0.0f*/, bool isIdle)
{
	SYS_CS_SYNC(m_renderCsToken);
	{
		Assertf(pName != NULL && pName[0] != '\0', "Can't create a timebar without a name");
		Assertf(!sysIpcIsStackAddress(pName),"Timebar name string '%s' is on the stack.", pName);

		// If we had a previous timebar then setup the nextId and endTime on it
		if (m_previous != INVALID_TIMEBAR)
		{
			// EndTimeBar(); //call end as soon as possible so we dont end up including much time spent in this function
			m_frame[m_processFrame].m_pTimes[m_previous].nextId = m_frame[m_processFrame].m_number;
		}

		// Check oversized, and keep assigning to the last timebar if it is.
		if (m_frame[m_processFrame].m_number >= m_size-1)
		{
			sFuncTime& timeBar = m_frame[m_processFrame].m_pTimes[m_size-1];

			// want to stop the renderer from dying when processing this (don't know if these values are meaningful)...
			timeBar.nextId = INVALID_TIMEBAR;
			timeBar.parentId = INVALID_TIMEBAR;		
			timeBar.layer = 0;

	#if __ASSERT
			static bool alreadyWarned = false;

			if (!alreadyWarned)
			{
				alreadyWarned = true;
				Errorf("== List of all timebars: ==");
				for (int x=0; x<m_size-1; x++)
				{
					sFuncTime &bar = m_frame[m_processFrame].m_pTimes[x];

					Errorf("%s", bar.nameptr);
				}

				Errorf("== End list of timebars ==");
			}
	#endif // __ASSERT

			Assertf(false,"Ran out of timebars, please change [%s] thread's PF_INIT_TIMEBARS call to a number larger than %d or use fewer timebars. See TTY for a list.", g_CurrentThreadName, m_size );

			return; //avoid crash after assert
		}

		// Populate new bar.
		sFuncTime& timeBar = m_frame[m_processFrame].m_pTimes[m_frame[m_processFrame].m_number];
		timeBar.nameptr = pName;


		timeBar.layer = (s8) m_layer;
		timeBar.prevId = m_previous;
		timeBar.childId = INVALID_TIMEBAR;
		timeBar.nextId = INVALID_TIMEBAR;
		timeBar.parentId = m_functionStack[m_layer];
		timeBar.budget = budget;
		timeBar.isIdle = isIdle;
		timeBar.overBudget = false;
		timeBar.isSlower = false;
		timeBar.detail = detail;
		timeBar.bucketId = (u8)g_pfTB_BucketSetHelper.GetNextTimeBarBucket();

		// If time bar has a parent and its child hasn't been set then this must be the first child
		// of the parent timebar
		s32 parentId = timeBar.parentId;
		if (parentId != INVALID_TIMEBAR && m_frame[m_processFrame].m_pTimes[parentId].childId == INVALID_TIMEBAR)
			m_frame[m_processFrame].m_pTimes[parentId].childId = m_frame[m_processFrame].m_number;


		if (parentId == INVALID_TIMEBAR)
		{
			TELEMETRY_PLOT_MS(time, pName);
		}

		// If we are tracing this time bar
		if (g_traceCurrentTimebar && g_traceTimebarName == timeBar.nameptr)
		{
			m_traceIndex = m_frame[m_processFrame].m_number;
		}

		m_frame[m_processFrame].m_number++;
		if (timeBar.prevId == INVALID_TIMEBAR)
			timeBar.startTime = 0.0f;
		else
			timeBar.startTime = m_frame[m_processFrame].m_pTimes[timeBar.prevId].endTime;

		timeBar.endTime = timeBar.startTime + time;

		// Update the bucket
		m_frame[m_processFrame].m_Buckets.AddTime(timeBar.bucketId, (timeBar.endTime - timeBar.startTime));


		m_previousTiming = m_previous = m_frame[m_processFrame].m_number - 1;


		// End Frame
		if (m_previous == m_traceIndex)
		{
			m_traceIndex = -1;
		}

		// Let the system know if we're over budget.
		if (timeBar.budget != 0.0f && ((timeBar.endTime - timeBar.startTime) > (timeBar.budget * g_pfTB.GetRatedFramerateScale())))
		{
			g_pfTB.MarkOverBudget(timeBar, m_frame[m_processFrame], *this, m_previous);
			timeBar.overBudget = true;
		}

		Assert(m_previous >= 0);
		UpdateMaxAndAverage(m_frame[m_processFrame], m_previous);
	}
}

//
// name:		pfTimeBars::Push
// description:	Start a new timebar and add it to the stack. Timebars created after this will be children of this timebar
void pfTimeBars::Push(const char* pName, const char * file, unsigned int line, bool detail, float budget, ProfileZoneType zoneType)
{
	StartTimeBar(pName, file, line, detail, budget, zoneType);
	// add timebar to stack
	m_layer++;
	Assert(m_layer > 0);

	if (m_layer >= MAX_FUNCTION_STACK_DEPTH)
	{
		static bool alreadyDumped = false;

		if (!alreadyDumped)
		{
			alreadyDumped = true;
			DumpCurrentStack();
		}
		Assertf(false, "Pushing too many timebars - see TTY for hierarchy.");
	}

	Assertf((m_layer < MAX_FUNCTION_STACK_DEPTH), "Function stack overflow %d of %d", m_layer, MAX_FUNCTION_STACK_DEPTH);
	m_functionStack[m_layer] = m_frame[m_processFrame].m_number - 1;

	m_previousTiming = m_previous = INVALID_TIMEBAR;
	
	RAGETRACE_PUSHNAME(pName);
}

pfTimeBars::sFuncTime *pfTimeBars::GetCurrentFunc() const
{
	if (m_previous < 0)
	{
		return NULL;
	}

	//sFuncTime& timeBar = m_frame[m_processFrame].m_pTimes[m_previous];
	//return NULL;
	return &m_frame[m_processFrame].m_pTimes[m_previous];
}

pfTimeBars::sFuncTime *pfTimeBars::GetSelectedFunc() const
{
	if (m_selected >= 0)
	{
		return &m_frame[m_renderFrame].m_pTimes[m_selected];
	}

	return NULL;
}

//
// name:		pfTimeBars::Pop
// description:	Remove time bar from stack
void pfTimeBars::Pop(int ASSERT_ONLY(detail))
{
	RAGETRACE_POP();

	Assert(m_layer > 0);

	// if no child timer then don't end it
	if(m_previousTiming != -1)
	{
		EndTimeBar();
		m_previousTiming = m_functionStack[m_layer];
	}
	else
	{
		m_previousTiming = m_functionStack[m_layer];
		EndTimeBar();
		m_previousTiming = INVALID_TIMEBAR;
	}

	// set previous time bar to be the one on the stack
	m_previous = m_functionStack[m_layer];
	m_layer--;

#if __ASSERT
	if (detail != -1)
	{
		// Make sure we started with the same detail flag as the one used here.
		sFuncTime &timeBar = m_frame[m_processFrame].m_pTimes[m_previous];
		Assertf((int) timeBar.detail == detail, "The timebar %s was pushed as a %s event, but not popped as such",
			timeBar.nameptr, (detail) ? "detail" : "important");
	}
#endif // __ASSERT
}

//
// name:		pfTimeBars::StartTimeBar
// description:	Function used by Start() and Push() to create a timebar
void pfTimeBars::StartTimeBar(const char* pName, const char * TELEMETRY_ONLY(file), unsigned int TELEMETRY_ONLY(line), bool detail, float budget, ProfileZoneType zoneType)
{
	Assertf(pName != NULL && pName[0] != '\0', "Can't create a timebar without a name");
	Assertf(!sysIpcIsStackAddress(pName),"Timebar name string '%s' is on the stack.",pName);

	if (m_previousTiming  != INVALID_TIMEBAR)
	{
		EndTimeBar(); //call end as soon as possible so we dont end up including much time spent in this function
	}

	// If we had a previous timebar then setup the nextId and endTime on it
	if(m_previous != INVALID_TIMEBAR)
	{
		m_frame[m_processFrame].m_pTimes[m_previous].nextId = m_frame[m_processFrame].m_number;
	}

#if USE_TELEMETRY
	TELEMETRY_START_ZONE(zoneType, file, line, pName);
#endif

	if(m_frame[m_processFrame].m_number >= m_size-1){
		sFuncTime& timeBar = m_frame[m_processFrame].m_pTimes[m_size-1];

		// want to stop the renderer from dying when processing this (don't know if these values are meaningful)...
		timeBar.nextId = INVALID_TIMEBAR;
		timeBar.parentId = INVALID_TIMEBAR;		
		timeBar.layer = 0;

#if __ASSERT
		static bool alreadyWarned = false;

		if (!alreadyWarned)
		{
			alreadyWarned = true;
			Errorf("== List of all timebars: ==");
			for (int x=0; x<m_size-1; x++)
			{
				sFuncTime &bar = m_frame[m_processFrame].m_pTimes[x];

				Errorf("%s", bar.nameptr);
			}

			Errorf("== End list of timebars ==");
		}
#endif // __ASSERT

		Assertf(false,"Ran out of timebars, please change [%s] thread's PF_INIT_TIMEBARS call to a number larger than %d or use fewer timebars. See TTY for a list.", g_CurrentThreadName, m_size );

		return; //avoid crash after assert
	}
	
#if __PPU
	snStartMarker(m_frame[m_processFrame].m_number&31, pName);  
#elif RSG_ORBIS
	if(sysBootManager::IsDevkit())
		sceRazorCpuPushMarker(pName, SCE_RAZOR_COLOR_GREEN, SCE_RAZOR_MARKER_DISABLE_HUD);
#endif

	int prevFrame = m_processFrame - 1;

	if (prevFrame < 0)
	{
		prevFrame = TIMEBAR_BUFFERS - 1;
	}

	sFuncTime& timeBar = m_frame[m_processFrame].m_pTimes[m_frame[m_processFrame].m_number];
	timeBar.nameptr = pName;


	//timeBar.startTime = m_timer.GetMsTime(); 
	timeBar.layer = (s8) m_layer;
	timeBar.prevId = m_previous;
	timeBar.childId = INVALID_TIMEBAR;
	timeBar.nextId = INVALID_TIMEBAR;
	timeBar.parentId = m_functionStack[m_layer];
	timeBar.budget = budget;
	timeBar.isIdle = (zoneType == PZONE_IDLE);
	timeBar.overBudget = false;
	timeBar.detail = detail;
	timeBar.bucketId = (u8)g_pfTB_BucketSetHelper.GetNextTimeBarBucket();
	
	// If time bar has a parent and its child hasn't been set then this must be the first child
	// of the parent timebar
	s32 parentId = timeBar.parentId;
	if(parentId != INVALID_TIMEBAR && m_frame[m_processFrame].m_pTimes[parentId].childId == INVALID_TIMEBAR)
		m_frame[m_processFrame].m_pTimes[parentId].childId = m_frame[m_processFrame].m_number;

	// If we are tracing this time bar
	if(g_traceCurrentTimebar && g_traceTimebarName == timeBar.nameptr)
	{
		m_traceIndex = m_frame[m_processFrame].m_number;
#if __XENON
		char filename[128];
		formatf(filename, "e:\\%s%d.pix2", pName, g_traceFilenumber++);
		XTraceStartRecording(filename);
#endif
	}

	m_frame[m_processFrame].m_number++;

	PIXBeginC(1, 0x00FFFF,pName);
	timeBar.startTime = m_timer.GetMsTime();

}

//
// name:		pfTimeBars::EndTimeBar
// description:	Called whenever a timebar needs finishing
void pfTimeBars::EndTimeBar()
{
	TELEMETRY_END_ZONE(__FILE__, __LINE__);

	Assert(m_previousTiming >= 0);
	
	sFuncTime& timeBar = m_frame[m_processFrame].m_pTimes[m_previousTiming];
	timeBar.endTime = m_timer.GetMsTime(); //do this ASAP to make sure we don't time any internals of TimerBar

	// Update the bucket
	m_frame[m_processFrame].m_Buckets.AddTime(timeBar.bucketId, (timeBar.endTime - timeBar.startTime));

#if __PPU
	snStopMarker(m_previousTiming&31);
#elif RSG_ORBIS
	if(sysBootManager::IsDevkit())
		sceRazorCpuPopMarker();
#endif

	if(m_previousTiming == m_traceIndex)
	{
#if __XENON
		XTraceStopRecording();
#endif
		m_traceIndex = -1;
	}
	PIXEnd();

	// Let the system know if we're over budget.
	if (timeBar.budget != 0.0f && ((timeBar.endTime - timeBar.startTime) > (timeBar.budget * g_pfTB.GetRatedFramerateScale())))
	{
		g_pfTB.MarkOverBudget(timeBar, m_frame[m_processFrame], *this, m_previousTiming);
		timeBar.overBudget = true;
	}

#if (!__FINAL || __FINAL_LOGGING) && !__NO_OUTPUT
	if (1 EXCEPTION_HANDLING_ONLY(&& !sysException::HasBeenThrown()))
	{
		if ((timeBar.endTime - timeBar.startTime) >= sysTimer::GetStallThreshold())
		{
			Displayf("** POSSIBLE STALL: time bar %s took %4.2f ms, stack follows", timeBar.nameptr, (timeBar.endTime - timeBar.startTime) );

			s32 iParent = timeBar.parentId;
			while(iParent != INVALID_TIMEBAR)
			{
				sFuncTime& parentBar = m_frame[m_processFrame].m_pTimes[iParent];
				Displayf("\t\t>\t%s",parentBar.nameptr);
				iParent = parentBar.parentId;
			}
		}
	}
#endif	// (!__FINAL || __FINAL_LOGGING) && !__NO_OUTPUT

	UpdateMaxAndAverage(m_frame[m_processFrame], m_previousTiming);
}

void pfTimeBars::UpdateMaxAndAverage(sTimebarFrame &frame, s32 id)
{
	sMaxFuncTime& maxTime = m_MaxTimes.m_Map[frame.ComputeCallstackHash(id)];
	sFuncTime &timeBar = frame.m_pTimes[id];

#if ENABLE_TIMEBAR_AVERAGE
	if (!g_pfTB.IsFrozen())
	{
#if HASH_VALUE_DEBUG
		size_t hashValue = frame.ComputeCallstackHash(timeBar);

		if (hashValue == hashValueCompare)
		{
			sFuncTime *time = &timeBar;
			while (true)
			{
				Displayf("Name: %s (%p), parent=%d", time->nameptr, time->nameptr, time->parentId);

				if (time->parentId == -1)
					break;

				time = &m_frame[m_processFrame].m_pTimes[time->parentId];
			}

			Displayf("Hash: %x", hashValue);
		}
#endif // HASH_VALUE_DEBUG

		if (maxTime.averageTime == 0.0f)
		{
			// This was just reset, so set it straight to the current time so it doesn't take
			// a second for it to "warm up".
			maxTime.averageTime = timeBar.endTime - timeBar.startTime;
		}
		else
		{
			maxTime.averageTime = (maxTime.averageTime * AVERAGE_TIMEBAR_FACTOR) + (timeBar.endTime - timeBar.startTime) * (1.0f - AVERAGE_TIMEBAR_FACTOR);
		}
	}
#endif // ENABLE_TIMEBAR_AVERAGE

	// set maxtime
	if ( maxTime.maxTime < timeBar.endTime -  timeBar.startTime)
	{
		maxTime.maxTime = timeBar.endTime -  timeBar.startTime;
		maxTime.maxTimeCount = 0;
	}
	else
	{
		maxTime.maxTimeCount++;
		if ( maxTime.maxTimeCount >= 100 && !g_pfTB.IsFrozen())
		{
			maxTime.maxTime	= timeBar.endTime -  timeBar.startTime;
			maxTime.maxTimeCount = 0;
		}
	}
}

#if !__FINAL

//
// name:		GetKeyJustDown_RAGE
// description:	Default version of key just down test
static bool GetKeyJustDown_RAGE(int key,const char *)
{
	// ioMapper::GetEnableKeyboard() is true in game mode and false in other (including debug) modes.
	return ioMapper::DebugKeyPressed(key) != 0;
}

//
// name:		GetKeyDown_RAGE
// description:	Default version of key down test
static bool GetKeyDown_RAGE(int key,const char *)
{
	// ioMapper::GetEnableKeyboard() is true in game mode and false in other (including debug) modes.
	return ioMapper::DebugKeyDown(key) != 0;
}

#endif

//
// name:		pfTimeBars::ProcessInput
// description:	Process the keyboard input. Scale timebars, choose a timebar etc
void pfTimeBars::ProcessInput()
{
#if !__FINAL
	// move between time bars and scale them
	if (s_GetKeyJustDown(KEY_NUMPAD4, "previous timebar"))
	{
		if(m_frame[m_renderFrame].m_pTimes[m_selected].prevId != INVALID_TIMEBAR)
			m_selected = m_frame[m_renderFrame].m_pTimes[m_selected].prevId;
	}
	else if (s_GetKeyJustDown(KEY_NUMPAD6, "next timebar"))
	{
		if(m_frame[m_renderFrame].m_pTimes[m_selected].nextId != INVALID_TIMEBAR)
			m_selected = m_frame[m_renderFrame].m_pTimes[m_selected].nextId;
	}	
	else if (s_GetKeyJustDown(KEY_NUMPAD9, "biggest timebar"))
	{
		if (!(s_GetKeyDown(KEY_LCONTROL, "toggle") || s_GetKeyDown(KEY_RCONTROL, "toggle")
			|| s_GetKeyDown(KEY_LMENU, "average") || s_GetKeyDown(KEY_RMENU, "average")))
		{
			s32 scanID = m_frame[m_renderFrame].m_pTimes[m_selected].parentId;
			if(scanID == INVALID_TIMEBAR) { scanID = 0; }
			else { scanID = m_frame[m_renderFrame].m_pTimes[scanID].childId; }
			float largest = 0.0f;
			s32 largestID = scanID;
			while(scanID != INVALID_TIMEBAR)
			{
				sFuncTime& entry = m_frame[m_renderFrame].m_pTimes[scanID];
				float duration = entry.endTime - entry.startTime;
				if(duration > largest) 
				{ 
					largestID = scanID;
					largest = duration;
				}
				scanID = entry.nextId;
			}
			m_selected = largestID;
		}
	}

	if (s_GetKeyJustDown(KEY_NUMPAD8, "previous timebar Layer"))
	{
		if (m_frame[m_renderFrame].m_pTimes[m_selected].childId != INVALID_TIMEBAR)
			m_selected = m_frame[m_renderFrame].m_pTimes[m_selected].childId;
	}
	else if (s_GetKeyJustDown(KEY_NUMPAD2, "next timebar Layer"))
	{
		if (m_frame[m_renderFrame].m_pTimes[m_selected].parentId != INVALID_TIMEBAR)
			m_selected = m_frame[m_renderFrame].m_pTimes[m_selected].parentId;
	}	

	if (s_GetKeyDown(KEY_LCONTROL, "toggle") || s_GetKeyDown(KEY_RCONTROL, "toggle"))
	{
		if (s_GetKeyDown(KEY_NUMPAD1, "move timebar left"))
		{
			s_TimebarOffsetInertia = -TB_SCROLL_SPEED;
		}
		else if (s_GetKeyDown(KEY_NUMPAD3, "move timebar right"))	
		{
			s_TimebarOffsetInertia = TB_SCROLL_SPEED;
		}

		if (s_GetKeyJustDown(KEY_NUMPAD7, "record baseline"))
		{
			g_pfTB.SetRecordBaseline();
		}
	}
	else
	{
		if (s_GetKeyDown(KEY_NUMPAD1, "scale timebar down"))
		{
			if(s_TimebarScale > 0.25f)
				s_TimebarScale-=0.25f;
		}
		else if (s_GetKeyDown(KEY_NUMPAD3, "scale timebar up"))	
		{
			s_TimebarScale += 0.25f;
		}	

		if (s_GetKeyJustDown(KEY_NUMPAD7, "show baseline"))
		{
			g_pfTB.ToggleShowBaselineDifference();
		}
	}
	if (s_GetKeyDown(KEY_T, "trace selected timebar"))
		g_traceCurrentTimebar = true;
	else
		g_traceCurrentTimebar = false;
#endif
}


static void DrawRect(float x1,float y1,float x2,float y2,const Color32 color)
{
	grcBegin(drawTriStrip,4);
		grcColor(color);
		grcVertex2f(x1,y1);
		grcVertex2f(x1,y2);
		grcVertex2f(x2,y1);
		grcVertex2f(x2,y2);
	grcEnd();
}

//
// name:		DrawTimeBar
// description:	Draw one single timebar
void DrawTimeBar(float startTime, float endTime, float scale, s32 layer, const Color32& color)
{
	float start = (startTime-s_TimebarOffset)*scale;
	float end = (endTime-s_TimebarOffset)*scale;

	if (start < 0.0f && end < 0.0f)
	{
		// Off screen. Ignore.
		return;
	}

	// Clamp to the left edge.
	start = Max(start, 0.0f);

	float yStart = (s_TimeBarY * GRCDEVICE.GetHeight()) - layer * (s_TimeBarHeight*GRCDEVICE.GetHeight());
	DrawRect( (s_TimeBarX*GRCDEVICE.GetWidth()) + start, yStart, (s_TimeBarX*GRCDEVICE.GetWidth()) + end, yStart+(s_TimeBarHeight*GRCDEVICE.GetHeight()), color);
}

void DrawSubTimeBar(float startTime, float endTime, float scale, s32 layer, const Color32& color)
{
	float start = (startTime-s_TimebarOffset)*scale;
	float end = (endTime-s_TimebarOffset)*scale;

	if (start < 0.0f && end < 0.0f)
	{
		// Off screen. Ignore.
		return;
	}

	// Clamp to the left edge.
	start = Max(start, 0.0f);

	float yStart = (s_TimeBarY * GRCDEVICE.GetHeight()) - layer * (s_TimeBarHeight*GRCDEVICE.GetHeight());
	yStart += 0.75f * s_TimeBarHeight*GRCDEVICE.GetHeight();
	DrawRect( (s_TimeBarX*GRCDEVICE.GetWidth()) + start, yStart, (s_TimeBarX*GRCDEVICE.GetWidth()) + end, yStart+(s_TimeBarHeight*GRCDEVICE.GetHeight()*0.25f), color);
}


//
// name:		DrawText_Rage
// description:	Default draw text function
void DrawText_Rage(float px,float py,const char *buf)
{
	grcFont::GetCurrent().DrawScaled(px,py,0.0f,Color32(0xffffffff),1.0f,1.0f,buf);
}

//
// name:		DrawText_RageColor
// description:	Default draw text function that takes a color argument
void DrawText_RageColor(float px,float py,const char *buf, Color32 color)
{
	grcFont::GetCurrent().DrawScaled(px,py,0.0f,color,1.0f,1.0f,buf);
}

int GetStringWidth_Rage(const char *buf)
{
	return grcFont::GetCurrent().GetStringWidth(buf,istrlen(buf));
}

//
// name:		PrintTimebarInfo
// description:	Print information about a timebar "name, time (maxTime OR avgTime)"
void PrintTimebarInfo(const char* name, float time, float averageTime, float maxTime, const pfTimeBars::sFuncTime& funcTime, const pfTimeBars::sMaxFuncTime &maxFuncTime,int call_count, const pfTimeBars::sTimebarFrame &HASH_VALUE_DEBUG_ONLY(frame))
{
	char buf[255];
	
	float px = s_TimeBarX * GRCDEVICE.GetWidth();
	float py = (s_TimeBarY + s_TimeBarHeight) * GRCDEVICE.GetHeight();

	char budget[80] = "";
	if (funcTime.budget)
	{
		if (g_pfTB.GetRatedFramerate() == 30.0f)
		{
			formatf(budget, "- Budget %2.3f", funcTime.budget);
		}
		else
		{
			formatf(budget, "- Budget %2.3f (%2.3f)", funcTime.budget * g_pfTB.GetRatedFramerateScale(), funcTime.budget);
		}
	}

	if (g_pfTB.GetShowBaselineDifference())
	{
		float diff = (funcTime.endTime - funcTime.startTime);
		formatf(buf,"(%d) %s: Diff %f", call_count,funcTime.nameptr, diff);
	}
	else
#if ENABLE_TIMEBAR_AVERAGE

#if HASH_VALUE_DEBUG
	char hashValue[32];
	formatf(hashValue, " Hash: %08x", frame.ComputeCallstackHash(funcTime));
	safecat(budget, hashValue);
#endif // HASH_VALUE_DEBUG

	if (g_pfTB.GetShowAverages())
	{
		formatf(buf,"(%d) %s: Average %f (Peak %f)%s", call_count,funcTime.nameptr, maxFuncTime.averageTime, maxFuncTime.maxTime, budget);
	}
	else
	{
		formatf(buf,"(%d) %s: %f(Avg %f, Peak %f)%s", call_count,funcTime.nameptr, funcTime.endTime - funcTime.startTime, maxFuncTime.averageTime, maxFuncTime.maxTime, budget);
	}
#else // ENABLE_TIMEBAR_AVERAGE
	formatf(buf,"(%d) %s: %f(%f)%", call_count,funcTime.name, funcTime.endTime - funcTime.startTime, maxFuncTime.maxTime, budget);
#endif // ENABLE_TIMEBAR_AVERAGE

	Color32 color = (funcTime.budget != 0.0f && funcTime.endTime - funcTime.startTime > funcTime.budget) ? TimebarOverBudgetColor : SelectedColor;
	s_DrawTextColor(px,py,buf,color);

	py = (s_TimeBarY + s_TimeBarHeight + s_TimeTextHeight) * GRCDEVICE.GetHeight();

#if ENABLE_TIMEBAR_AVERAGE
	formatf(buf,"%s : %f(Avg %f, Peak %f)", name, time, averageTime, maxTime);
#else // ENABLE_TIMEBAR_AVERAGE
	formatf(buf,"%s : %f(%f)", name, time, maxTime);
#endif // ENABLE_TIMEBAR_AVERAGE

	s_DrawText(px,py, buf);
} 

bool pfTimeBars::GetFunctionTotals(const char* pName, int &call_count, float &time)
{
	//work out how long the bar will be
	int localCallCount = 0;
	float localTime = 0.0f;

	const sTimebarFrame &frame = m_frame[m_renderFrame];

	if (frame.m_pTimes)
	{
		int totalCount = frame.m_number;

		for (s32 tb=0; tb<totalCount; tb++)
		{
			// Could compare pointers here, but that would depend on string pooling being 100% reliable.
			sFuncTime &func = frame.m_pTimes[tb];
			Assert(func.nameptr != NULL);
			if( func.nameptr != NULL && strcmp(func.nameptr, pName)==0 )
			{
				float time_width=func.endTime-func.startTime;
				localTime+=time_width;
				localCallCount++;
			}
		}
	}

	call_count = localCallCount;
	time = localTime;

	return (localCallCount != 0);
}

#if ENABLE_TIMEBAR_AVERAGE
void pfTimeBars::AdjustAverages(s32 renderFrame, s32 tb, float time)
{
	while(tb != INVALID_TIMEBAR)
	{
		sMaxFuncTime &maxFunc = m_MaxTimes.m_Map[m_frame[renderFrame].ComputeCallstackHash(tb)];
		float origStart = m_frame[renderFrame].m_pTimes[tb].startTime;
		m_frame[renderFrame].m_pTimes[tb].startTime = time;
		m_frame[renderFrame].m_pTimes[tb].endTime = time + maxFunc.averageTime;

		s32 child = m_frame[renderFrame].m_pTimes[tb].childId;

		if (child != -1)
		{
			AdjustAverages(renderFrame, child, m_frame[renderFrame].m_pTimes[child].startTime - origStart + time);
		}

		time += maxFunc.averageTime;
		tb = m_frame[renderFrame].m_pTimes[tb].nextId;
	}
}
#endif // ENABLE_TIMEBAR_AVERAGE

void pfTimeBars::AdjustBaselines(s32 renderFrame, s32 tb, s32 baseline, float time)
{	
	bool slowerOnly = g_pfTB.GetShowBaselineSlowerOnly();

	sTimebarFrame& frame = m_frame[renderFrame];

	u32 currentTimebar = 0;
	while(tb != INVALID_TIMEBAR)
	{
		float origStart = frame.m_pTimes[tb].startTime;
		float delta = frame.m_pTimes[tb].endTime - frame.m_pTimes[tb].startTime;
		float diff = delta;

		sMaxFuncTime &maxFunc = m_MaxTimes.m_Map[m_frame[renderFrame].ComputeCallstackHash(baseline)];
		Assert((frame.m_pTimes[tb].nameptr == m_MaxTimes.m_Map[baseline].nameptr));
		diff -= maxFunc.baseline;

		float absDiff = slowerOnly?Max(0.0f, diff):Abs(diff);

		frame.m_pTimes[tb].startTime = time;
		frame.m_pTimes[tb].endTime = time + absDiff;
		frame.m_pTimes[tb].isSlower = diff >= 0.0f;
	
		s32 child = frame.m_pTimes[tb].childId;

		if (child != -1)
		{
			AdjustBaselines(renderFrame, child, baseline, frame.m_pTimes[child].startTime - origStart + time);
		}

		time += absDiff;
		tb = frame.m_pTimes[tb].nextId;
		++currentTimebar;
	}
}

void pfTimeBars::ResetBaseline()
{
	atSimplePooledMapType<size_t, sMaxFuncTime>::Iterator it = m_MaxTimes.CreateIterator();

	for (it.Start(); !it.AtEnd(); it.Next())
	{
		it.GetData().baseline = 0.0f;
	}
}

void pfTimeBars::RecordBaseline(s32 tb)
{
	while(tb != INVALID_TIMEBAR)
	{
		sMaxFuncTime &maxFunc = m_MaxTimes.m_Map[m_frame[m_renderFrame].ComputeCallstackHash(tb)];
		maxFunc.baseline = maxFunc.averageTime;
		s32 child = m_frame[m_renderFrame].m_pTimes[tb].childId;
		if (child != - 1)
		{
			RecordBaseline(child);
		}

		tb = m_frame[m_renderFrame].m_pTimes[tb].nextId;
	}
}

//
// name:		pfTimeBars::Display
// description:	Render timebars
void pfTimeBars::Display(bool isPrimary, bool printName, s32 &yOffset, float timeOffset, int /*barIndex*/, s32 renderFrame, const char *highlightName, bool isGhost)
{
	if (renderFrame == -1)
	{
		renderFrame = m_renderFrame;
	}

	if (m_frame[renderFrame].m_number == 0)
		return;

	// block while rendering
	SYS_CS_SYNC(m_renderCsToken);

	// Adjust the times to match the averages.
#if ENABLE_TIMEBAR_AVERAGE
	if (g_pfTB.GetShowAverages())
	{
		s32 tb = 0;
		float time = m_frame[renderFrame].m_pTimes[tb].startTime;

		AdjustAverages(renderFrame, tb, time);
	}
#endif // ENABLE_TIMEBAR_AVERAGE

	if (g_pfTB.GetShowBaselineDifference())
	{
		float time = m_frame[renderFrame].m_pTimes[0].startTime;
		AdjustBaselines(renderFrame, 0, 0, time);
	}

	FindSelectedTimebar();

	// process input after find selected. Otherwise find selected will go back to the original selected timebar
	if (isPrimary)
	{
		ProcessInput();
	}

	PUSH_DEFAULT_SCREEN();
	
	grcStateBlock::SetBlendState(grcFont::GetCurrent().HasOutline()? grcStateBlock::BS_Normal : grcStateBlock::BS_Default);
	grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_IgnoreDepth);
	grcStateBlock::SetRasterizerState(grcStateBlock::RS_NoBackfaceCull);
	grcLightState::SetEnabled(false);
	grcBindTexture(NULL);

	// Compute the total number of time it took for this thread, as well as average and
	// peak time.
	float totalTime = 0.0f;
	float averageTime = 0.0f;
	float peakTime = 0.0f;
	{
		s32 tb = 0;
		while (m_frame[renderFrame].m_pTimes[tb].nextId != INVALID_TIMEBAR)
		{
			tb = m_frame[renderFrame].m_pTimes[tb].nextId;
			sMaxFuncTime &maxFunc = m_MaxTimes.m_Map[m_frame[renderFrame].ComputeCallstackHash(tb)];
#if ENABLE_TIMEBAR_AVERAGE
			averageTime += maxFunc.averageTime;
#endif
			peakTime += maxFunc.maxTime;
		}
		totalTime = m_frame[renderFrame].m_pTimes[tb].endTime - m_frame[renderFrame].m_pTimes[0].startTime;
	}

	// Draw frame markers for reference
	float scale = s_TimebarScale;
	if (totalTime > 1000.0f)
	{
		s32 total = (s32(totalTime+1000)/1000)*1000;
		// draw 1 second markers
		scale = (1.0f - 2.0f*s_TimeBarX) * GRCDEVICE.GetWidth() / total;
		for (s32 i = 0; i <= total; i+=1000)
		{
			float x = (s_TimeBarX*GRCDEVICE.GetWidth()) + scale * i;
			DrawRect( x, ((s_TimeBarY - s_TimeBarHeight)*GRCDEVICE.GetHeight()),(x+2), (s_TimeBarY + s_TimeBarHeight)*GRCDEVICE.GetHeight(),Color32(255,255,255));
		}
	}
	else
	{
		// draw 60 fps markers
		for(s32 i=0; i<5; i++)
		{
			float x = (s_TimeBarX*GRCDEVICE.GetWidth()) + (1000.0f/60.0f) * s_TimebarScale * i;
			DrawRect( x, ((s_TimeBarY - s_TimeBarHeight)*GRCDEVICE.GetHeight()),(x+2), (s_TimeBarY + s_TimeBarHeight)*GRCDEVICE.GetHeight(),Color32(0,0,0));
		}
	}

	// draw timebars
	s32 tb=0;
	bool redColour[MAX_FUNCTION_STACK_DEPTH];
	Color32 colour;
	redColour[0] = true;
	maxRenderedLayer = 0;

	s32 maxLayer = 0;
	
	while(tb != INVALID_TIMEBAR)
	{
		sFuncTime &funcTime = m_frame[renderFrame].m_pTimes[tb];

		s32 layer = funcTime.layer; 
		maxLayer = Max(maxLayer, layer);
		maxRenderedLayer = Max(maxRenderedLayer,layer);
		bool isIdle = funcTime.isIdle;
		
		//decide on color
		if (!isPrimary && funcTime.nameptr == highlightName)
			colour = SameNameColor;
		else if( funcTime.overBudget )
			if(isPrimary && tb == m_selected)
				colour = OverBudgetColor;
			else
				colour = SelectedOverBudgetColor;
		else if(isPrimary && tb == m_selected)
			colour = SelectedColor;
		else if(redColour[layer])
			colour = (isIdle) ? AIdleColor : AColor;
		else
			colour = (isIdle) ? BIdleColor : BColor;
		redColour[layer] = !redColour[layer];

		if (isGhost)
		{
			colour = Lerp(0.6f, colour, Color32(0));
		}

		// Don't draw if this is a ghost bar and we're not supposed to show anything that's not a stall (or selected).
		if (!isGhost || !s_ShowStallsInGhostsOnly || isIdle || funcTime.nameptr == highlightName)
		{
			DrawTimeBar(funcTime.startTime + timeOffset, funcTime.endTime + timeOffset, scale, layer + yOffset, colour);

			if (g_pfTB.GetShowBaselineDifference() && !g_pfTB.GetShowBaselineSlowerOnly())
			{
				DrawSubTimeBar(funcTime.startTime + timeOffset, funcTime.endTime + timeOffset, scale, layer + yOffset, funcTime.isSlower?Color_red:Color_green);
			}
		}

		// next timer
		s32 prevTb = tb;

		// If reach the end of a set of timebars then chose the parent timebar. If we reach the root then break out of loop
		while(m_frame[renderFrame].m_pTimes[tb].nextId == INVALID_TIMEBAR)
		{
			tb = m_frame[renderFrame].m_pTimes[tb].parentId;
			if(tb == INVALID_TIMEBAR)
				break;
		}
		// If timebar isn't invalid then chose the next timebar
		if(tb != INVALID_TIMEBAR)
			tb = m_frame[renderFrame].m_pTimes[tb].nextId;

		// if selected is in between the current and next time bar then look at children of this timebar or selected is after the current 
		// timebar and there aren't any valid timebars after that
		if(isPrimary && m_selected > prevTb && (tb == INVALID_TIMEBAR || m_selected < tb))
		{
			tb = m_frame[renderFrame].m_pTimes[prevTb].childId;
			if(tb == INVALID_TIMEBAR)
				break;
			// first child is always red
			redColour[m_frame[renderFrame].m_pTimes[tb].layer] = true;
		}
	}

	if (isPrimary)
	{
		sMaxFuncTime &maxFunc = m_MaxTimes.m_Map[m_frame[renderFrame].ComputeCallstackHash(m_selected)];
		PrintTimebarInfo(m_name, totalTime, averageTime, peakTime, m_frame[renderFrame].m_pTimes[m_selected], maxFunc, 1, m_frame[renderFrame]);
	}

	if (printName)
	{
		float px = s_TimeBarX * GRCDEVICE.GetWidth();
		float py = (s_TimeBarY - (yOffset * s_TimeBarHeight)) * GRCDEVICE.GetHeight();

		char finalName[128];
		formatf(finalName, "%s%s ", (isPrimary) ? "*" : "", m_name);
		s_DrawText(px - s_GetStringWidth(finalName),py,finalName);
	}

	POP_DEFAULT_SCREEN();

	StoreSelectedTimebarStack();

	// Move the offset up in case we're rendering more timebars after this.
	yOffset += maxLayer + 1;
}


void pfTimeBars::DumpCurrentStack()
{
	const int kCurrent = m_frame[m_processFrame].m_number;
	if (kCurrent > 0)
	{
		sFuncTime& timeBar = m_frame[m_processFrame].m_pTimes[kCurrent-1];
		Displayf("\t\t%s",timeBar.nameptr);
		s32 iParent = timeBar.parentId;
		while(iParent != INVALID_TIMEBAR)
		{
			sFuncTime& parentBar = m_frame[m_processFrame].m_pTimes[iParent];
			Displayf("\t\t>\t%s",parentBar.nameptr);
			iParent = parentBar.parentId;
		}
	}
}


//
// name:		pfTimeBars::Dump
// description:	Dump all stats in CSV format to TTY
void pfTimeBars::Dump(float threshold)
{
	if (m_frame[m_renderFrame].m_number == 0)
		return;

	// block while printing
	SYS_CS_SYNC(m_renderCsToken);

	// draw timebars
	Dump(0, 0, threshold);
}


//
// name:		pfTimeBars::TabbedDump
// description:	Dump all stats in Tabbed format to TTY
void pfTimeBars::TabbedDump()
{
	if (m_frame[m_renderFrame].m_number == 0)
		return;

	// block while printing
	SYS_CS_SYNC(m_renderCsToken);

	// draw timebars
	TabbedDump(0);
}


//
// name:		pfTimeBars::DumpCSV
// description:	Dump all stats in CSV format to file
void pfTimeBars::DumpToFile(float threshold, fiStream * csv_stream)
{
	if (m_frame[m_renderFrame].m_number == 0)
		return;
	// block while printing
	SYS_CS_SYNC(m_renderCsToken);

	if (!csv_stream)
	{
		Errorf("Could not create '%s'", TIMEBAR_CSV_FILE".csv");
		return;
	}

	// draw timebars	
	Dump(0, 0, threshold, csv_stream);	
}


enum { MAX_INDENTS = 16 };	// This should be the number of commas in indentString
OUTPUT_ONLY(const char *indentString = ",,,,,,,,,,,,,,,,");
//
// name:		pfTimeBars::Dump
// description:	Dump a timebar in CSV format (to a file if stream is not null)
void pfTimeBars::Dump(int tb, int indent, float threshold, fiStream* stream)
{
	Assert(indent < MAX_INDENTS);

	while(tb != INVALID_TIMEBAR)
	{
		const sFuncTime& funcTime = m_frame[m_renderFrame].m_pTimes[tb];
		const sMaxFuncTime &maxTime = m_MaxTimes.m_Map[m_frame[m_renderFrame].ComputeCallstackHash(tb)];

#if ENABLE_TIMEBAR_AVERAGE
		if (funcTime.endTime - funcTime.startTime >= threshold || maxTime.averageTime >= threshold || maxTime.maxTime >= threshold)
		{
			if (stream != NULL)
			{
				fprintf(stream, "%s%s%s%s%.5f\n", &indentString[MAX_INDENTS-indent], funcTime.nameptr, &indentString[indent], &indentString[MAX_INDENTS-indent], funcTime.endTime - funcTime.startTime);
				fflush(stream);
			}
			else
			{
				Displayf("%s%s%s%.5f,%.5f,%.5f", &indentString[MAX_INDENTS-indent], funcTime.nameptr, &indentString[indent], funcTime.endTime - funcTime.startTime, maxTime.averageTime, maxTime.maxTime);
			}			
		}
#else // ENABLE_TIMEBAR_AVERAGE
		if (funcTime.endTime - funcTime.startTime >= threshold || maxTime.maxTime >= threshold)
		{
			if (stream != NULL)
			{
				fprintf(stream, "%s%s%s%s%.5f\n", &indentString[MAX_INDENTS-indent], funcTime.nameptr, &indentString[indent], &indentString[MAX_INDENTS-indent], funcTime.endTime - funcTime.startTime);
				fflush(stream);
			}
			else
			{
				Displayf("%s%s%s%.5f,%.5f", &indentString[MAX_INDENTS-indent], funcTime.name, &indentString[indent], funcTime.endTime - funcTime.startTime, maxTime.maxTime);
			}
		}
#endif // ENABLE_TIMEBAR_AVERAGE

		if (funcTime.childId)
		{
			Dump(funcTime.childId, indent+1, threshold, stream);
		}

		tb = funcTime.nextId;
	}
}


//
// name:		pfTimeBars::Dump
// description:	Dump a timebar in XML format
void pfTimeBars::DumpXML(int tb, int indent, float threshold, fiStream* stream)
{
	enum { MAX_INDENTS = 16 };	// This should be the number of commas in indentString
	OUTPUT_ONLY(static const char *indentString = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
	Assert(indent < MAX_INDENTS);

	while(tb != INVALID_TIMEBAR)
	{
		const sFuncTime& funcTime = m_frame[m_renderFrame].m_pTimes[tb];
		const sMaxFuncTime &maxTime = m_MaxTimes.m_Map[m_frame[m_renderFrame].ComputeCallstackHash(tb)];

#if ENABLE_TIMEBAR_AVERAGE
		if (funcTime.endTime - funcTime.startTime >= threshold || maxTime.averageTime >= threshold || maxTime.maxTime >= threshold)
		{
			fprintf(stream, "%s<Timebar name=\"", &indentString[MAX_INDENTS-indent]);
			parStreamOutXml::WriteEscapedString(stream, funcTime.nameptr, -1, true);
			fprintf(stream, "\" time=\"%.5f\" avg=\"%.5f\" max=\"%.5f\">", funcTime.endTime - funcTime.startTime, maxTime.averageTime, maxTime.maxTime);
#else // ENABLE_TIMEBAR_AVERAGE
		if (funcTime.endTime - funcTime.startTime >= threshold || maxTime.maxTime >= threshold)
		{
			fprintf(stream, "%s<Timebar name=\"", &indentString[MAX_INDENTS-indent]);
			parStreamOutXml::WriteEscapedString(stream, funcTime.nameptr, -1, true);
			fprintf(stream, "\" time=\"%.5f\" max=\"%.5f\">"funcTime.endTime - funcTime.startTime, maxTime.maxTime);
#endif // ENABLE_TIMEBAR_AVERAGE

			if (funcTime.childId && funcTime.childId != INVALID_TIMEBAR)
			{
				fprintf(stream, "\n");
				DumpXML(funcTime.childId, indent+1, threshold, stream);
				fprintf(stream, "%s", &indentString[MAX_INDENTS-indent]);
			}

			fprintf(stream, "</Timebar>\n");
		}

		tb = funcTime.nextId;
	}
}



void pfTimeBars::DumpTimebarFrameCounts()
{
	char string[256];
	formatf(string, "%-25s: ", m_name);

	for (int x=0; x<TIMEBAR_BUFFERS; x++)
	{
		char subString[32];
		sTimebarFrame &frame = m_frame[x];
		char status = ' ';

		if (x == m_renderFrame)
		{
			status = 'R';
		}

		if ( x == m_processFrame)
		{
			status = 'P';
		}

		formatf(subString, "%c%08x ", status, frame.m_frameNumber);
		safecat(string, subString);
	}

	Displayf("%s", string);
}

//
// name:		pfTimeBars::TabbedDump
// description:	Dump a timebar, using tabs instead of CSV
void pfTimeBars::TabbedDump(int tb, int indent)
{
	enum { MAX_INDENTS = 16 };	// This should be the number of tabs in indentString
	OUTPUT_ONLY(static const char *indentString = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");

	Assert(indent < MAX_INDENTS);

	while(tb != INVALID_TIMEBAR)
	{
		const sFuncTime& funcTime = m_frame[m_renderFrame].m_pTimes[tb];
		if( funcTime.nameptr[0] != '\0' )
		{
			OUTPUT_ONLY(const sMaxFuncTime &maxTime = m_MaxTimes.m_Map[m_frame[m_renderFrame].ComputeCallstackHash(tb)]);

#if ENABLE_TIMEBAR_AVERAGE
			Displayf("\t%s%s%s%.5f\t%.5f\t%.5f\t%.5f", &indentString[MAX_INDENTS-indent], funcTime.nameptr, &indentString[indent], funcTime.endTime - funcTime.startTime, maxTime.averageTime, maxTime.maxTime, funcTime.budget);
#else // ENABLE_TIMEBAR_AVERAGE
			Displayf("\t%s%s%s%.5f\t%.5f", &indentString[MAX_INDENTS-indent], funcTime.name, &indentString[indent], funcTime.endTime - funcTime.startTime, maxTime.maxTime);
#endif // ENABLE_TIMEBAR_AVERAGE
		
			if (funcTime.childId)
			{
				TabbedDump(funcTime.childId, indent+1);
			}
		}
		
		tb = funcTime.nextId;
	}
}
//
// name:		pfTimeBars::IsSelectedTimebar
// description:	Return if this timebar is the selected timebar, or likely to be.
//              Possible return values include:
//              0 - no match: Either wrong layer, or wrong stack frame.
//              1 - correct stack frame, but neither neighbor matches.
//              2 - one neighbor matches
//              3 - both neighbors match
int pfTimeBars::IsSelectedTimebar(s32 tb)
{
	// check layer
	sFuncTime &time = m_frame[m_renderFrame].m_pTimes[tb];
	if(m_selectedLayer != time.layer)
		return 0;

	// check stack
	s32 index=0;
	while(tb != INVALID_TIMEBAR)
	{
		if(m_selectedNameStack[index++] != m_frame[m_renderFrame].m_pTimes[tb].nameptr)
			return 0;

		tb = m_frame[m_renderFrame].m_pTimes[tb].parentId;
	}

	// Check neighbors
	s32 score = 1;

	score += (int) (m_frame[m_renderFrame].GetNameSafe(time.prevId) == m_selectedPrev);
	score += (int) (m_frame[m_renderFrame].GetNameSafe(time.nextId) == m_selectedNext);

	return score;
}

//
// name:		pfTimeBars::FindSelectedTimebar
// description:	Find the selected timebar
void pfTimeBars::FindSelectedTimebar()
{
	// check currently selected
	if(IsSelectedTimebar(m_selected) != 0)
		return;

	s32 bestMatch[2] = { -1, -1 };
	s32 bestLayer[2] = { -1, -1 };

	// if currently selected is wrong then go through all the timebars
	int tbCount = m_frame[m_renderFrame].m_number;

	// We're hopefully just one or two timebars off, so let's try to start from where we began and move
	// away from it.
	s32 startingLocation = m_selected;

	for(s32 x=0; x<tbCount*2; x++)
	{
		// Find the next timebar to use.
		int offset = (x+1) >> 1;

		if (x & 1)
		{
			offset = -offset;
		}
		
		int i = startingLocation + offset;

		// Are we done? Or on an invalid timebar?
		if (i < 0)
		{
			continue;
		}

		if (i >= tbCount)
		{
			if (offset > startingLocation)
			{
				// We're done in both directions - we can abort now.
				break;
			}

			continue;
		}

		int score = IsSelectedTimebar(i);

		if (score == 3)
		{
			m_selected = i;
			m_selectedLayer = m_frame[m_renderFrame].m_pTimes[i].layer;
			return;
		}

		if (score > 0)
		{
			bestMatch[score-1] = i;
			bestLayer[score-1] = m_frame[m_renderFrame].m_pTimes[i].layer;
		}
	}

	for (int x=1; x>=0; x--)
	{
		if (bestMatch[x] != -1)
		{
			m_selected = bestMatch[x];
			m_selectedLayer = bestLayer[x];
			m_selectedPrev = m_frame[m_renderFrame].GetNameSafe(m_frame[m_renderFrame].m_pTimes[bestMatch[x]].prevId);
			m_selectedNext = m_frame[m_renderFrame].GetNameSafe(m_frame[m_renderFrame].m_pTimes[bestMatch[x]].nextId);
			return;
		}
	}

	int newSelected = Min(m_selected, m_frame[m_renderFrame].m_number-1);
	int rightMatch = -1, leftMatch = -1;
	int cursor = newSelected;

	// Try to stay in the same layer - it's utterly confusing when we all of a sudden switch layers.
	while (cursor < tbCount)
	{
		if (m_frame[m_renderFrame].m_pTimes[cursor].layer == m_selectedLayer)
		{
			rightMatch = cursor;
			break;
		}

		cursor++;
	}

	cursor = newSelected;

	while (cursor >= 0)
	{
		if (m_frame[m_renderFrame].m_pTimes[cursor].layer == m_selectedLayer)
		{
			leftMatch = cursor;
			break;
		}

		cursor--;
	}

	if (leftMatch != -1)
	{
		if (rightMatch != -1)
		{
			// We have valid candidates in both directions. Let's try to find the one that's closest to where
			// we were before.
			if (newSelected - leftMatch > rightMatch - newSelected)
			{
				m_selected = rightMatch;
			}
			else
			{
				m_selected = leftMatch;
			}
		}
		else
		{
			m_selected = leftMatch;
		}
	}
	else
	{
		if (rightMatch != -1)
		{
			m_selected = rightMatch;
		}
		else
		{
			// There is no match on the same layer. Let's just pick whatever we have.
			m_selected = newSelected;
		}
	}

	// Update our selection once more so we don't try again next frame.
	StoreSelectedTimebarStack();
}

//
// name:		pfTimeBars::StoreSelectedTimebarStack
// description:	Store the current timebar stack, so we can use it to find the selected timebar next frame
void pfTimeBars::StoreSelectedTimebarStack()
{
	// store selected name stack. This should be enough to identify the selected timebar if it moves
	s32 tb = m_selected;
	s32 stackPosn = 0;
	while(tb != INVALID_TIMEBAR)
	{
		m_selectedNameStack[stackPosn++] = m_frame[m_renderFrame].m_pTimes[tb].nameptr;
		tb = m_frame[m_renderFrame].m_pTimes[tb].parentId;
	}
	m_selectedNameStack[stackPosn] = NULL;
	m_selectedLayer = m_frame[m_renderFrame].m_pTimes[m_selected].layer;
	m_selectedPrev = m_frame[m_renderFrame].GetNameSafe(m_frame[m_renderFrame].m_pTimes[m_selected].prevId);
	m_selectedNext = m_frame[m_renderFrame].GetNameSafe(m_frame[m_renderFrame].m_pTimes[m_selected].nextId);
	g_traceTimebarName = m_selectedNameStack[0];
}

void pfTimeBars::DisplayBudgets()
{
	if (m_frame[m_renderFrame].m_number == 0)
		return;

	SYS_CS_SYNC(m_renderCsToken);

	float childTotal = DisplayBudgets(0, 0);

	Displayf("TOTAL,,,,,,,,,%2.1f", childTotal);
}

float pfTimeBars::DisplayBudgets(int tb, int indent)
{
	enum { MAX_INDENTS = 10 };	// This should be the number of commas in indentString
	OUTPUT_ONLY(static const char *indentString = ",,,,,,,,,,");

	Assert(indent < MAX_INDENTS);

	float total = 0.0f;

	while (tb != INVALID_TIMEBAR)
	{
		const sFuncTime& funcTime = m_frame[m_renderFrame].m_pTimes[tb];

		if( funcTime.nameptr[0] != '\0' )
		{
			if (funcTime.budget != 0.0f)
			{
				total += funcTime.budget;

				Displayf("%s%s%s%2.3f", &indentString[MAX_INDENTS-indent], funcTime.nameptr, &indentString[indent], funcTime.budget);
			}

			if (funcTime.childId)
			{
				total += DisplayBudgets(funcTime.childId, indent+1);
			}
		}

		tb = funcTime.nextId;
	}

	return total;
}


bank_s32	s_required = -1;
bank_float	s_cuttoff = 1.0f;
void pfTimeBars::VerifyBudgets()
{
	if (m_frame[m_renderFrame].m_number == 0)
		return;

	SYS_CS_SYNC(m_renderCsToken);

	float childTotal = VerifyBudgets(1000.0f/30.0f, 0, 0, s_required, s_cuttoff);

	Displayf("TOTAL,,,,,,,,,%2.1f,%2.1f", 30.0f, childTotal);
}

float pfTimeBars::VerifyBudgets(float expectedBudget, int tb, int indent, int requiredLevel, float cutoff)
{
	enum { MAX_INDENTS = 10 };	// This should be the number of commas in indentString
	OUTPUT_ONLY(static const char *indentString = ",,,,,,,,,,");

	Assert(indent < MAX_INDENTS);

	float total = 0.0f;
	float childTotal = 0.0f;
	int childrenBudgets = 0;

	while (tb != INVALID_TIMEBAR)
	{
		const sFuncTime& funcTime = m_frame[m_renderFrame].m_pTimes[tb];
		
		if( funcTime.nameptr[0] != '\0' )
		{
			if (funcTime.budget != 0.0f)
			{
				total += funcTime.budget;

				Displayf("%s%s%s", &indentString[MAX_INDENTS-indent], funcTime.nameptr, &indentString[indent]);
			}

			if (funcTime.childId)
			{
				float child = VerifyBudgets(funcTime.budget, funcTime.childId, indent+1, requiredLevel, cutoff);

				OUTPUT_ONLY(const sMaxFuncTime &maxTime = m_MaxTimes.m_Map[m_frame[m_renderFrame].ComputeCallstackHash(tb)]);

				if ((indent < requiredLevel) || (funcTime.budget != 0.0f) || ((maxTime.averageTime/g_pfTB.GetRatedFramerateScale()) > cutoff))
				{
					const char* tag = "";
				
					if ((funcTime.budget == 0.0f) &&(maxTime.averageTime/g_pfTB.GetRatedFramerateScale()) > cutoff)
					{
						tag = "MISSING - SIGNIFICANT";
					}
					else if ((funcTime.budget != 0.0f) && (maxTime.averageTime/g_pfTB.GetRatedFramerateScale()) * 2.0f < cutoff)
					{
						tag = "TOO HIGH";
					}
					else if (child > funcTime.budget)
					{
						tag = "OVERFLOW";
					}
					else if (indent < requiredLevel && funcTime.budget == 0.0f)
					{
						tag = "MISSING - LEVEL";
					}

					++childrenBudgets;
					childTotal += child;
					Displayf("%s%s%s%2.1f,%2.1f,%2.1f,%s", &indentString[MAX_INDENTS-indent], funcTime.nameptr, &indentString[indent], funcTime.budget, child, maxTime.averageTime/g_pfTB.GetRatedFramerateScale(), tag);
				}
			}
		}

		tb = funcTime.nextId;
	}

	if (expectedBudget && childrenBudgets > 1)
	{
		const char* tag = (expectedBudget > childTotal)?"":"OVERFLOW";

		Displayf("%s%s%s%2.1f,%2.1f, %s", &indentString[MAX_INDENTS-indent], "SUBTOTAL", &indentString[indent], expectedBudget, childTotal, tag);
	}

	return total;
}

// --- pfTimeBarMgr --------------------------------------------------------------------------------------------------------------
const float s_baseFramerate = 30.0f;

pfTimeBarMgr::pfTimeBarMgr() : m_startup(-1), m_gpu(-1), m_timeBarSets(NUM_TIMEBAR_SETS), m_current(1), m_display(1), m_Frozen(false), m_DisplayOverbudget(__BANK && (!__DEV || (RSG_PC || RSG_ORBIS))), m_disallowMoreTimeBars(false)
{
}

void pfTimeBarMgr::SetRatedFramerate(float ratedFrameRate)
{
	m_ratedFramerate = ratedFrameRate;
	m_ratedFramerateScale = s_baseFramerate/m_ratedFramerate;
}

void pfTimeBarMgr::SetRatedFramerateScale(float ratedFrameRateScale)
{
	m_ratedFramerateScale = ratedFrameRateScale;
	m_ratedFramerate = s_baseFramerate * 1.0f/ratedFrameRateScale;
}

void pfTimeBarMgr::UpdateRatedFrameRateBasedOnSettings(float targetFrameRate)
{
	// commandline param overrides value from settings
	if (PARAM_ratedframerate.Get())
	{
		return;
	}

	SetRatedFramerate(targetFrameRate);
}

#if __BANK
static const int TIMEBAR_SELECT_BY_NAME_SIZE = 256;
static char s_SelectByName[TIMEBAR_SELECT_BY_NAME_SIZE];

void pfTimeBarMgr::SelectTimebarByName()
{
	pfTimeBars& pBars = m_timeBarSets[m_display];
	pBars.SelectTimebarByName(s_SelectByName);
}
#endif // __BANK

namespace grProfileCallbacks {
	void AutoMarkerBegin(const char* name, int PS3_ONLY(id)) PS3_ONLY(: m_Id(id))
	{
#if __PS3
		snStartMarker(id, name);  
#elif RSG_ORBIS
		if(sysBootManager::IsDevkit())
			sceRazorCpuPushMarker(name, SCE_RAZOR_COLOR_BLUE, SCE_RAZOR_MARKER_DISABLE_HUD);
#else
		PIXBegin(0, name);  
#endif
	}
	void AutoMarkerEnd(int PS3_ONLY(id)) 
	{
#if __PS3
		snStopMarker(id);  
#elif RSG_ORBIS
		if(sysBootManager::IsDevkit())
			sceRazorCpuPopMarker();
#else
		PIXEnd();  
#endif
	}

	void StartStartupBar(uptr /*dummy*/, const char *pName, const char * file, unsigned int line, bool detail, float budget, ProfileZoneType zoneType) {
		::rage::g_pfTB.GetStartupBar().Start(pName, file, line, detail, budget, zoneType);
		::rage::g_pfStartupCallback(pName);
	}

	void PushStartupBar(uptr /*dummy*/, const char *pName, const char * file, unsigned int line, bool detail, float budget, ProfileZoneType zoneType) {
		::rage::g_pfTB.GetStartupBar().Push(pName, file, line, detail, budget, zoneType);
		::rage::g_pfStartupCallback(pName);
	}

	void PopStartupBar(uptr /*dummy*/, int detail) {
		::rage::g_pfTB.GetStartupBar().Pop(detail);
	}

	void Start(uptr /*dummy*/, const char* pName, const char * file, unsigned int line, bool detail, float budget, ProfileZoneType zoneType) {
		::rage::g_pfTB.Start(pName, file, line, detail, budget, zoneType);
	}

	void Push(uptr /*dummy*/, const char* pName, const char * file, unsigned int line, bool detail, float budget, ProfileZoneType zoneType) {
		::rage::g_pfTB.Push(pName, file, line, detail, budget, zoneType);
	}

	void Pop(uptr /*dummy*/, int detail) {
		::rage::g_pfTB.Pop(detail);
	}
}

//
// name:		pfTimeBarMgr::Init
// description:	Init timebars for current thread
void pfTimeBarMgr::Init(const char* name, s32 size, float maxAllowedTime, bool supportAbsoluteMode)
{
	// Install the timebar callbacks
	pfTimebarFuncs::g_StartCb = grProfileCallbacks::Start;
	pfTimebarFuncs::g_PushCb = grProfileCallbacks::Push;
	pfTimebarFuncs::g_PopCb = grProfileCallbacks::Pop;
	pfTimebarFuncs::g_StartStartupBarCb = grProfileCallbacks::StartStartupBar;
	pfTimebarFuncs::g_PushStartupBarCb = grProfileCallbacks::PushStartupBar;
	pfTimebarFuncs::g_PopStartupBarCb= grProfileCallbacks::PopStartupBar;
	pfTimebarFuncs::g_PushMarkerCb = grProfileCallbacks::AutoMarkerBegin;
	pfTimebarFuncs::g_PopMarkerCb = grProfileCallbacks::AutoMarkerEnd;


	m_IsAbsoluteMode = PARAM_absolutemodetimebars.Get();
	m_ShowAverages = false;
	m_ShowBaselineDifference = false;
	m_ShowBaselineDifferenceSlowerOnly = true;
	m_RecordBaseline = false;
	float ratedFramerate = 30.0f;
	m_AutoPauseTimeBars = false;
#if __BANK
	if (PARAM_ratedframerate.Get())
	{
		ratedFramerate = PARAM_ratedframerate.Get();
	}
	if (PARAM_autopausetimebars.Get())
	{
		m_AutoPauseTimeBars = true;
	}
#endif // __BANK
	m_IncludeDetailBarsToggle = m_IncludeDetailBars = true || PARAM_showdetailtimebars.Get();

	if(sysThreadType::IsUpdateThread())
	{
		SetRatedFramerate(ratedFramerate);
	}

	USE_DEBUG_MEMORY();
	// If we haven't setup a time bar set than initialize one now
	if(gTimebarSetId == -1)
	{
		SYS_CS_SYNC(s_CS);

		Assert(m_current < NUM_TIMEBAR_SETS);
		gTimebarSetId = m_current;
		m_timeBarSets[gTimebarSetId].Init(name, size, maxAllowedTime);
		m_timeBarSets[gTimebarSetId].m_ExcludeFromAbsoluteMode = !supportAbsoluteMode;
		m_current++;
	}

	// Check for the over-budget inclusion list.
	char budgetedBars[1024];
	const char *budgetedBarArray[NUM_OVER_BUDGET_BARS];
	int inclusionCount = PARAM_checkoverbudgetonly.GetArray(budgetedBarArray, NUM_OVER_BUDGET_BARS, budgetedBars, sizeof(budgetedBars));

	for (int x=0; x<inclusionCount; x++)
	{
		// Convert ^ to space.
		char *convertPtr = (char *) budgetedBarArray[x];

		while (*convertPtr)
		{
			if (*convertPtr == '^')
			{
				*convertPtr = ' ';
			}

			convertPtr++;
		}

		AddToOverBudgetInclusionList(budgetedBarArray[x]);	
	}

	PS3_ONLY(pfDummySync()); // to prevent this from being stripped
}

void pfTimeBarMgr::InitStartupBar(const char* name, s32 size)
{
	USE_DEBUG_MEMORY();
	// If we haven't setup a time bar set than initialize one now
	if (m_startup == -1)
	{
		m_startup = 0;
		m_timeBarSets[m_startup].Init(name, size);
		m_timeBarSets[m_startup].m_ExcludeFromAbsoluteMode = true;
	}
}

//#if __PS3
//void RsxCallback(const uint32_t cause)
//{
//	if (cause == 0)
//		g_pfTB.GetGpuBar().GpuFrameStartTime();
//}
//#else
//#error 
//#endif

void pfTimeBarMgr::InitGpuBar(const char* name, s32 size)
{
	USE_DEBUG_MEMORY();
	// If we haven't setup a time bar set than initialize one now
	if (m_gpu == -1)
	{
		SYS_CS_SYNC(s_CS);
		m_gpu = m_current++;
		m_timeBarSets[m_gpu].Init(name, size);
		m_timeBarSets[m_gpu].m_ExcludeFromAbsoluteMode = true;

//#if __PS3
//		cellGcmSetUserHandler(RsxCallback);
//#else
//		// fuckit
//#endif
	}
}


//
// name:		pfTimeBarMgr::FrameInit
// description:	Frame init for current thread
void pfTimeBarMgr::FrameInit(u32 frameNumber)
{
	Assert(gTimebarSetId != -1);
	Assert(gTimebarSetId < NUM_TIMEBAR_SETS);
	m_timeBarSets[gTimebarSetId].FrameInit(frameNumber);
	m_IncludeDetailBars = m_IncludeDetailBarsToggle;
}

void pfTimeBarMgr::FrameEnd()
{
	Assert(gTimebarSetId != -1);
	Assert(gTimebarSetId < NUM_TIMEBAR_SETS);
	m_timeBarSets[gTimebarSetId].FrameEnd();
}

//
// name:		pfTimeBarMgr::Start
// description:	Start timebar in current thread
void pfTimeBarMgr::Start(const char* pName, const char * file, unsigned int line, bool detail, float budget, ProfileZoneType zoneType)
{
	if (!m_disallowMoreTimeBars && (!detail || m_IncludeDetailBars))
	{
		if (gTimebarSetId != -1)
		{
			Assert(gTimebarSetId < NUM_TIMEBAR_SETS);
			m_timeBarSets[gTimebarSetId].Start(pName, file, line, detail, budget, zoneType);
		}
	}
}

//
// name:		pfTimeBarMgr::Push
// description:	Push timebar in current thread
void pfTimeBarMgr::Push(const char* pName, const char * file, unsigned int line, bool detail, float budget, ProfileZoneType zoneType)
{
	if (!m_disallowMoreTimeBars && (!detail || m_IncludeDetailBars))
	{
		if (gTimebarSetId != -1)
		{
			Assert(gTimebarSetId < NUM_TIMEBAR_SETS);
			m_timeBarSets[gTimebarSetId].Push(pName, file, line, detail, budget, zoneType);
		}
	}
}

//
// name:		pfTimeBarMgr::Pop
// description:	Pop timebar in current thread
void pfTimeBarMgr::Pop(int detail)
{
	if (!m_disallowMoreTimeBars && (detail != 1 || m_IncludeDetailBars))
	{
		if (gTimebarSetId != -1)
		{
			Assert(gTimebarSetId < NUM_TIMEBAR_SETS);
			m_timeBarSets[gTimebarSetId].Pop(detail);
		}
	}
}

pfTimeBars::sFuncTime *pfTimeBarMgr::GetCurrentFunc() const
{
	if (gTimebarSetId != -1)
	{
		Assert(gTimebarSetId < NUM_TIMEBAR_SETS);
		return m_timeBarSets[gTimebarSetId].GetCurrentFunc();
	}

	return NULL;
}

//
// name:		pfTimeBarMgr::Display
// description:	Display one timebar set. Pressing 5 on the number pad goes through timebar sets
void pfTimeBarMgr::Display()
{
	// Handle the scrolling.
	s_TimebarOffset += s_TimebarOffsetInertia;
	s_TimebarOffsetInertia *= 0.5f;

	// Avoid annoying little movements.
	if (s_TimebarOffsetInertia < 0.01f)
	{
		s_TimebarOffsetInertia = 0.0f;
	}

#if !__FINAL
	// move between time bars sets
	if (s_GetKeyJustDown(KEY_NUMPAD5, "next timebar set"))
	{
		if (s_GetKeyDown(KEY_LSHIFT, "freeze") || s_GetKeyDown(KEY_RSHIFT, "freeze"))
		{
			m_Frozen = !m_Frozen;
		}
		else if (s_GetKeyDown(KEY_LCONTROL, "dump") || s_GetKeyDown(KEY_RCONTROL, "dump"))
		{
			DumpTimebars();
		}
		else if (s_GetKeyDown(KEY_LMENU, "budgets") || s_GetKeyDown(KEY_RMENU, "budgets"))
		{
			m_DisplayOverbudget = !m_DisplayOverbudget;
		}
		else
		{
			m_display++;
			if(m_display >= m_current)
				m_display = 0;
		}
	}

	if (s_GetKeyJustDown(KEY_NUMPAD9, "Toggle Absolute Mode"))
	{
		if (s_GetKeyDown(KEY_LCONTROL, "toggle") || s_GetKeyDown(KEY_RCONTROL, "toggle"))
		{
			m_IsAbsoluteMode = !m_IsAbsoluteMode;
		}
		else if (s_GetKeyDown(KEY_LMENU, "average") || s_GetKeyDown(KEY_RMENU, "average"))
		{
#if ENABLE_TIMEBAR_AVERAGE
			m_ShowAverages = !m_ShowAverages;
#endif // ENABLE_TIMEBAR_AVERAGE
		}
	}
#endif // !__FINAL

	s32 yOffset = 0;

	if (m_IsAbsoluteMode)
	{
		// Let's agree on one particular frame.
		u32 frameNumber = 0;
		s32 renderFrames[NUM_TIMEBAR_SETS];
		u64 lowestClock = 0;
		bool isValid = false;

		// Find the first bar that uses absolute mode.
		for (int bar=0; bar<m_current; bar++)
		{
			if (!m_timeBarSets[bar].m_ExcludeFromAbsoluteMode)
			{
				// We have it, now find a frame number we can all agree on.
				s32 renderFrame = m_timeBarSets[bar].GetRenderFrame();

				for (int x=0; x<TIMEBAR_BUFFERS-1; x++)
				{
					lowestClock = m_timeBarSets[bar].GetStartTime(renderFrame);
					renderFrames[bar] = renderFrame;
					frameNumber = m_timeBarSets[bar].GetFrameNumber(renderFrame);
					isValid = true;

					for (int otherBar=bar+1; otherBar<m_current; otherBar++)
					{
						// Does everyone else have that number too?
						if (!m_timeBarSets[otherBar].m_ExcludeFromAbsoluteMode)
						{
							s32 otherRenderFrame = m_timeBarSets[otherBar].GetRenderFrame();
							bool hasMatch = false;

							for (int y=0; y<TIMEBAR_BUFFERS-1; y++)
							{
								u32 otherFrameNumber = m_timeBarSets[otherBar].GetFrameNumber(otherRenderFrame);
								if (otherFrameNumber == frameNumber)
								{
									// Matches. Good.
									hasMatch = true;
									lowestClock = Min(lowestClock, m_timeBarSets[otherBar].GetStartTime(otherRenderFrame));
									renderFrames[otherBar] = otherRenderFrame;
									break;
								}

								// Next one maybe?
								if (--otherRenderFrame < 0)
									otherRenderFrame = TIMEBAR_BUFFERS - 1;
							}

							if (!hasMatch)
							{
								// Try a different frame number.
								isValid = false;
								break;
							}
						}
					}

					if (isValid)
					{
						// Found one!
						break;
					}

					// That one didn't work, let's try another one.
					if (--renderFrame < 0)
						renderFrame = TIMEBAR_BUFFERS - 1;
				}

				break;
			}
		}

		if (!isValid)
		{
			// We don't have enough data yet to display a full frame's worth on all threads.
			return;
		}

		// Get the name of the currently selected element in the active timebar.
		// That allows us to highlight othe timebars with the same name.
		const char *highlightName = NULL;
		pfTimeBars::sFuncTime *funcTime = m_timeBarSets[m_display].GetSelectedFunc();

		if (funcTime)
		{
			highlightName = funcTime->nameptr;
		}

		// In absolute mode, we render all timebars (except for the startup one),
		// with one of them being the active one that can be navigated through.
		for (int bar=0; bar<m_current; bar++)
		{
			if (!m_timeBarSets[bar].m_ExcludeFromAbsoluteMode)
			{
				s32 renderFrame = renderFrames[bar];
				float timeOffset = ((float) (m_timeBarSets[bar].GetStartTime(renderFrame) - lowestClock)) * sysTimerConsts::TicksToMilliseconds;
				s32 savedYOffset = yOffset;
				m_timeBarSets[bar].Display((bar == m_display), true, yOffset, timeOffset, bar, (s32) renderFrame, highlightName);

				// Handle the ghost bar.
				if (timeOffset != 0.0f || s_TimebarOffset != 0.0f)
				{
					// Go to the previous frame.
					if (--renderFrame < 0)
						renderFrame = TIMEBAR_BUFFERS - 1;

					float prevTimeOffset = ((float) ((s64) m_timeBarSets[bar].GetStartTime(renderFrame) - (s64) lowestClock)) * sysTimerConsts::TicksToMilliseconds;

					m_timeBarSets[bar].Display(false, false, savedYOffset, prevTimeOffset, bar, (s32) renderFrame, highlightName, true);
				}
			}
		}
	}
	else
	{
		Assert(m_display < NUM_TIMEBAR_SETS);
		m_timeBarSets[m_display].Display(true, false, yOffset, 0.0f, 0);
	}

	if (m_RecordBaseline)
	{
		for (int bar=1; bar<m_current; bar++)
		{
			m_timeBarSets[bar].ResetBaseline();
			m_timeBarSets[bar].RecordBaseline(0);
		}
		m_RecordBaseline = false;
	}

	if (m_DisplayOverbudget)
		DisplayOverBudgets();
}

// name: GetSetName
// desc: Get the name of the specified set.
//
const char* pfTimeBarMgr::GetSetName(int timebarsetid) const
{
    if(timebarsetid == -1)
    {
        timebarsetid = gTimebarSetId;
    }

    Assert(timebarsetid < NUM_TIMEBAR_SETS);
    return m_timeBarSets[timebarsetid].GetName();
}

//
// name: GetFunctionTotals
// desc: Wraps the GetFunctionTotals function for the current thread global id.
//
bool pfTimeBarMgr::GetFunctionTotals(const char* pName, int &call_count, float &time,int timebarsetid)
{
	if(timebarsetid == -1)
		timebarsetid = gTimebarSetId;

	Assert(timebarsetid < NUM_TIMEBAR_SETS);
	return this->m_timeBarSets[timebarsetid].GetFunctionTotals( pName, call_count, time );
}

float pfTimeBarMgr::GetSelectedTime() const
{
	pfTimeBars::sFuncTime *time = m_timeBarSets[m_display].GetSelectedFunc();

	if (time)
	{
		return time->endTime - time->startTime;
	}

	return 0.0f;
}

#if !__FINAL
//
// name:		pfTimeBarMgr::SetKeyboardCallbacks
// description:	Set functions for key down and key just down
void pfTimeBarMgr::SetKeyboardCallbacks(bool (*keyJustDown)(int,const char*),bool (*keyDown)(int,const char*))
{
	s_GetKeyJustDown = keyJustDown;
	s_GetKeyDown = keyDown;
}
#endif // !__FINAL

//
// name:		pfTimeBarMgr::SetDimensions
// description:	Set dimensions of timebars
void pfTimeBarMgr::SetTimebarDimensions(float x, float y, float height, float defaultFrameWidth)
{
	s_TimeBarX = x;
	s_TimeBarY = y;
	s_TimeBarHeight = height;
	s_TimebarScale = (defaultFrameWidth * 60.0f/1000.0f)*1280.0f;
}

//
// name:		pfTimeBarMgr::SetTextCallback
// description:	Set callback for drawing text
void pfTimeBarMgr::SetTextCallback(void (*textcb)(float,float,const char*))
{
	s_DrawText = textcb;
}

// name: DumpTimebars
// desc: Dump the stats of all registered time bars to the TTY.
//
void pfTimeBarMgr::DumpTimebars(float 
#if !HACK_GTA4
								threshold
#endif
								)
{
	for (int x=0; x<m_current; x++)
	{
		Displayf("\tTimebar %d", x);
#if HACK_GTA4
		m_timeBarSets[x].TabbedDump();
#else // HACK_GTA4
		m_timeBarSets[x].Dump(threshold);
#endif // HACK_GTA4
	}
}

void pfTimeBarMgr::DumpTimebarsXML(fiStream* stream, float threshold)
{
	fprintf(stream, "<TimebarSets>\n");
	for (int x=0; x<m_current; x++)
	{
		fprintf(stream, "\t<TimebarSet>\n");
		m_timeBarSets[x].DumpXML(0, 2, threshold, stream);
		fprintf(stream, "\t</TimebarSet>\n");
	}
	fprintf(stream, "</TimebarSets>\n");
}

// name: DumpTimebars
// desc: Dump the stats of all registered time bars to the TTY.
//
void pfTimeBarMgr::DumpTimebarsToFile(float threshold)
{
	fiStream* csv_stream = ASSET.Create( TIMEBAR_CSV_FILE, "csv" );

	if (!csv_stream)
	{
		Errorf("Could not create '%s'", TIMEBAR_CSV_FILE".csv");
		return;
	}

	fprintf(csv_stream, "%sTime\n", indentString);
	fflush(csv_stream);

	for (int x=0; x<m_current; x++)
	{
		m_timeBarSets[x].DumpToFile(threshold, csv_stream);
	}

	csv_stream->Close();
	csv_stream = NULL;
	Displayf("Timebar dumped to %s", TIMEBAR_CSV_FILE".csv");
}

void pfTimeBarMgr::VerifyBudgets()
{
	for (int x=1; x<m_current; x++)
	{
		Displayf("\tTimebar %s", m_timeBarSets[x].GetName());
		m_timeBarSets[x].VerifyBudgets();
	}
}

void pfTimeBarMgr::DisplayBudgets()
{
	for (int x=1; x<m_current; x++)
	{
		Displayf("\tTimebar %s", m_timeBarSets[x].GetName());
		m_timeBarSets[x].DisplayBudgets();
	}
}

void pfTimeBarMgr::DumpCurrentStack()
{
	Displayf("Timebar Stacks");
	for (int x=0; x<m_current; x++)
	{
		Displayf("\tTimebar %d", x);
		m_timeBarSets[x].DumpCurrentStack();
	}
}
void pfTimeBarMgr::AddToOverBudgetInclusionList(const char *taskName)
{
	m_OverBudgetInclusionList.Append() = taskName;
}

bool pfTimeBarMgr::IsFrozen() const 
{
	return (m_Frozen || (m_AutoPauseTimeBars && grcSetup::IsPaused()));
}

void pfTimeBarMgr::DumpTimebarFrameCounts()
{
	for (int x=0; x<m_current; x++)
	{
		m_timeBarSets[x].DumpTimebarFrameCounts();
	}
}

void pfTimeBarMgr::MarkOverBudget(const pfTimeBars::sFuncTime &func, const pfTimeBars::sTimebarFrame &frame, const pfTimeBars &timebars, s32 id)
{
	if (IsFrozen())
	{
		return;
	}

	// If we have an inclusion list, disregard anything that's not in the list.
	int inclusionCount = m_OverBudgetInclusionList.GetCount();

	char threadIdentifier = timebars.GetName()[0];

	if (inclusionCount)
	{
		bool found = false;
		const char *nameptr = func.nameptr;

		for (int x=0; x<inclusionCount; x++)
		{
			if (m_OverBudgetInclusionList[x] == nameptr)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			return;
		}
	}

	SYS_CS_SYNC(s_CS);

	// Is this one already registered?
	int count = m_OverBudgetBars.GetCount();
	pfOverBudgetBar *bar = NULL;

	for (int x=0; x<count; x++)
	{
		if (m_OverBudgetBars[x].m_TaskName == func.nameptr && m_OverBudgetBars[x].m_ThreadIdentifier == threadIdentifier)
		{
			// Got it.
			bar = &m_OverBudgetBars[x];
			break;
		}
	}

	if (!bar)
	{
		// Allocate a new one.
		if (m_OverBudgetBars.IsFull())
		{
			return;
		}

		bar = &m_OverBudgetBars.Append();
		bar->m_TaskName = func.nameptr;
		bar->m_Peak = 0.0f;
		bar->m_ConsecutiveFrames = 0;
		bar->m_Budget = func.budget;
		bar->m_Age = 0;
		bar->m_BeenVisible = false;
		bar->m_ThreadIdentifier = timebars.GetName()[0];
		bar->m_Callstackhash = frame.ComputeCallstackHash(id);
	}

	// If we had been in budget before, reset the budget counter to re-determine
	// if this is a spike or not.
	if (bar->m_GoodFrames > s_spikeFrameCount)
	{
		bar->m_ConsecutiveFrames = 0;
	}

	float funcTime = func.endTime - func.startTime;

	// Reset the age if we hit a new peak.
	if (funcTime >= bar->m_Peak)
	{
		bar->m_Peak = funcTime;
		bar->m_Age = 0;

		// Find the slowest child.
		s32 childId = func.childId;

		if (childId != pfTimeBars::INVALID_TIMEBAR)
		{
			// Find the slowest child.
			float slowest = 0.0f;

			while (childId != pfTimeBars::INVALID_TIMEBAR)
			{
				const pfTimeBars::sFuncTime &childFunc = frame.m_pTimes[childId];
				float time = childFunc.endTime - childFunc.startTime;

				if (time > slowest)
				{
					slowest = time;
					bar->m_SlowestChildName = childFunc.nameptr;
					bar->m_SlowestChildPeak = time;
				}

				childId = childFunc.nextId;
			}
		}
		else
		{
			bar->m_SlowestChildName = "";
			bar->m_SlowestChildPeak = 0.0f;			
		}
	}

	bar->m_ConsecutiveFrames++;
	bar->m_GoodFrames = 0;
}

void pfTimeBarMgr::UpdateOverBudgets()
{
	// Don't update anyhing if we're frozen.
	if (IsFrozen())
	{
		return;
	}

	SYS_CS_SYNC(s_CS);

	for (int x=m_OverBudgetBars.GetCount()-1; x>=0; x--)
	{
		pfOverBudgetBar &bar = m_OverBudgetBars[x];

		bar.m_GoodFrames++;
		bar.m_Age++;

		if (bar.m_Age >= OVERBUDGET_LENGTH)
		{
			m_OverBudgetBars.DeleteFast(x);
		}
	}
}

void pfTimeBarMgr::DisplayOverBudgets()
{
	PUSH_DEFAULT_SCREEN();

	grcStateBlock::SetBlendState(grcStateBlock::BS_Normal);
	grcStateBlock::SetRasterizerState(grcStateBlock::RS_NoBackfaceCull);
	grcLightState::SetEnabled(false);
	grcBindTexture(NULL);

	float height = (float) grcFont::GetCurrent().GetHeight();

	float yPos = (float) GRCDEVICE.GetHeight() * 0.44f; 
	float xPos = GRCDEVICE.GetWidth() * 0.60f;

	float x1 = (float) GRCDEVICE.GetWidth() * 0.58f;
	float x2 = (float) GRCDEVICE.GetWidth() * 0.95f;
	float y2 = (float) GRCDEVICE.GetHeight() * 0.82f;

	// Draw the background.
	grcDrawSingleQuadf(x1 - 5.0f, yPos - 5.0f, x2, y2, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, Color32(0x80000000));

	y2 -= height;

	grcStateBlock::SetBlendState(grcFont::GetCurrent().HasOutline()? grcStateBlock::BS_Normal : grcStateBlock::BS_Default);

	s_DrawText(xPos, yPos, "OVER BUDGET (Pink=Short spikes) (Alt+Num5 closes)");
	yPos += height * 1.5f;

	if (s_ShowHierarchicalOverbudget)
	{
		// If in absolute mode, show the overbudget items for ALL threads.
		if (m_IsAbsoluteMode)
		{
			for (int bar=0; bar<m_current; bar++)
			{
				if (!m_timeBarSets[bar].m_ExcludeFromAbsoluteMode)
				{
					DisplayOverBudgets(m_timeBarSets[bar], yPos, y2);
				}
			}
		}
		else
		{
			// Show overbudget items only for the current thread.
			Assert(m_display < NUM_TIMEBAR_SETS);
			DisplayOverBudgets(m_timeBarSets[m_display], yPos, y2);
		}
	}
	else
	{
		// Show only allocations of the current thread, unless we're in absolute mode.
		char showOnlyThread = (m_IsAbsoluteMode) ? 0 : m_timeBarSets[m_display].m_name[0];

		for (int x=m_OverBudgetBars.GetCount()-1; x>=0; x--)
		{
			pfOverBudgetBar &bar = m_OverBudgetBars[x];

			// Skip those of other threads.
			if (showOnlyThread && bar.m_ThreadIdentifier != showOnlyThread)
			{
				continue;
			}

			Color32 color;
			// Set the alpha value based on how old this message is.
			//float alpha = (float) bar.m_Age / (float) OVERBUDGET_LENGTH;
			float alpha = 1.0f;

			// Make it fully visible at first and then fade it quickly.
			//alpha *= -4.0f;
			//alpha = Min(alpha + 3.0f, 1.0f);

			// Spike or just over-budget?
			if (bar.m_ConsecutiveFrames < s_spikeFrameCount)
			{
				// Spike.
				if( PARAM_displaySpikes.Get() || bar.m_BeenVisible)
				{
					color = Color32(1.0f, 0.0f, 1.0f, alpha);
				}
				else
				{
					continue;
				}
			}
			else
			{
				// Consistently over-budget.
				color = Color32(1.0f, 0.9f, 0.9f, alpha);
			}

			char finalStr[128];

			formatf(finalStr, "%c %-25s %5.2fms", bar.m_ThreadIdentifier, bar.m_TaskName, /*bar.m_Budget, */bar.m_Peak);

			if (yPos < y2)
			{
				s_DrawTextColor(xPos, yPos, finalStr, color);
				yPos += height;
			}

			if (bar.m_SlowestChildName[0] && yPos < y2)
			{
				formatf(finalStr, "%c  %-24s %5.2fms", bar.m_ThreadIdentifier, bar.m_SlowestChildName, bar.m_SlowestChildPeak);
				s_DrawTextColor(xPos, yPos, finalStr, color);
				yPos += height;
			}

			bar.m_BeenVisible = true;
		}
	}

	grcStateBlock::SetRasterizerState(grcStateBlock::RS_Default);

	POP_DEFAULT_SCREEN();
}

void pfTimeBarMgr::DisplayOverBudgets(pfTimeBars &bars, float &yPos, float y2)
{
	// An array of spaces to print indentation.
	const char *spaces = "                ";
	const int SPACE_COUNT = 16;	// Must be the number of spaces in 'spaces'.

	float xPos = GRCDEVICE.GetWidth() * 0.58f;
	float height = (float) grcFont::GetCurrent().GetHeight();
	bool firstLine = true;

	s32 tb=0;
	maxRenderedLayer = 0;
	int renderFrame = bars.m_renderFrame;
	pfTimeBars::sTimebarFrame &frame = bars.m_frame[renderFrame];

	// The current hierarchy that we're in.
	atFixedArray<pfTimeBars::sFuncTime *, 16> stack;

	// The current hierarchy as we displayed it on the screen.
	atFixedArray<pfTimeBars::sFuncTime *, 16> displayedStack;

	if  (tb == pfTimeBars::INVALID_TIMEBAR)
	{
		return;
	}

	char threadIdentifier = bars.GetName()[0];
	bool oddLine = false;

	while(tb != pfTimeBars::INVALID_TIMEBAR)
	{
		pfTimeBars::sFuncTime &funcTime = frame.m_pTimes[tb];
		Assert(!stack.IsFull());
		if (stack.IsFull())
			break;

		stack.Append() = &funcTime;

		if (funcTime.budget != 0.0f)
		{
			const char *nameptr = funcTime.nameptr;
			pfOverBudgetBar *bar = NULL;

			// Find the matching overbudget item.
			for (int x=m_OverBudgetBars.GetCount()-1; x>=0; x--)
			{
				pfOverBudgetBar &testBar = m_OverBudgetBars[x];
				// Skip those of other threads.
				if (testBar.m_ThreadIdentifier == threadIdentifier)
				{
					if (testBar.m_TaskName == nameptr)
					{
						bar = &testBar;
						break;
					}
				}
			}

			// If we didn't find a matching bar, forget about it.
			if (bar)
			{
				Color32 color;

				// Spike or just over-budget?
				if (bar->m_ConsecutiveFrames < s_spikeFrameCount)
				{
					// Spike.
					if( PARAM_displaySpikes.Get() || bar->m_BeenVisible)
					{
						color = Color32(1.0f, 0.0f, 1.0f, 1.0f);
					}
					else
					{
						bar = NULL;
					}
				}
				else
				{
					// Consistently over-budget.
					color = Color32(1.0f, 0.5f, 0.3f, 1.0f);
				}

				if (bar)
				{
					// This one is over budget. Let's print it.
					// Do we need a header?
					if (firstLine && yPos < y2)
					{
						// Yes. The first time, we print a header with the thread name.
						char header[128];

						formatf(header, "%-25s        Avg     Peak   %% Budget", bars.GetName());
						firstLine = false;
						yPos += 5.0f;
						s_DrawTextColor(xPos, yPos, header, Color32(0xffffff00));
						yPos += height + 5.0f;
					}

					// Now show the hierarchy all the way up to the actual element that is overbudget.
					int matchingIndex = 0;
					int displayedCount = Min(displayedStack.GetCount(), stack.GetCount());

					// Don't print something that we already printed.
					while (matchingIndex < displayedCount)
					{
						if (displayedStack[matchingIndex] == stack[matchingIndex])
						{
							matchingIndex++;
						}
						else
						{
							break;
						}
					}

					float budgetPercentage = bar->m_Peak * 100.0f / bar->m_Budget;

					// Update the displayed stack, i.e. the elements that we displayed on the screen.
					int stackSize = stack.GetCount();
					displayedStack.Resize(stackSize);
					Assert(matchingIndex < stack.GetCount());
					// Print the part of the hierarchy we haven't printed yet.
					for (int x=matchingIndex; x<stackSize - 1; x++)
					{
						if (yPos < y2)
						{
							char finalLine[64];
							formatf(finalLine, "%s %s", &spaces[Clamp(16-x, 0, SPACE_COUNT)], stack[x]->nameptr);
							s_DrawTextColor(xPos, yPos, finalLine, Color32(0xffffffff));
							yPos += height;
							oddLine = !oddLine;
						}

						displayedStack[x] = stack[x];
					}

					// Set the final one too - we're displaying it in a separate function so
					// we can show the budget information, but we still need to keep track of
					// it in the displayedStack in case we encounter one of its children
					// that's also overbudget.
					displayedStack[stackSize-1] = stack[stackSize-1];

					// Now show information about the overbudget item.
					int indent = stack.GetCount();

					char finalStr[256];

					// Now print the actual element that's over budget.
					pfTimeBars::sMaxFuncTime& maxTime = bars.m_MaxTimes.m_Map[bar->m_Callstackhash];

					formatf(finalStr, "%s %-25s%s%5.2fms %5.2fms (%.0f%%)", &spaces[Clamp(17-indent, 0, SPACE_COUNT)], bar->m_TaskName, &spaces[Max(0, Min((int)strlen(spaces) - 1,Min(indent+9, 17)))],
						/*bar->m_Budget, */maxTime.averageTime, maxTime.maxTime, budgetPercentage);

					if (oddLine)
						color.SetBlue(255 - color.GetBlue());

					if (yPos < y2)
					{
						s_DrawTextColor(xPos, yPos, finalStr, color);
						yPos += height;
						oddLine = !oddLine;
					}

					bar->m_BeenVisible = true;

					if (bar->m_SlowestChildName[0])
					{
						// NOTE - if this child happens to have its own overbudget timebar, we have to skip it.
						bool slowestChildBudgeted = false;
						const char *slowestChildName = bar->m_SlowestChildName;

						for (int x=m_OverBudgetBars.GetCount()-1; x>=0; x--)
						{
							pfOverBudgetBar &testBar = m_OverBudgetBars[x];
							// Skip those of other threads.
							if (testBar.m_ThreadIdentifier == threadIdentifier)
							{
								if (testBar.m_TaskName == slowestChildName)
								{
									// It'll be displayed when we get to it - skip now.
									slowestChildBudgeted = true;
									break;
								}
							}
						}

						if (!slowestChildBudgeted)
						{
							color.SetBlue(255 - color.GetBlue());
							const char *childName = bar->m_SlowestChildName;

							if (yPos < y2)
							{
								formatf(finalStr, "%s %-24s%s        %5.2fms", &spaces[Clamp(16-indent, 0, SPACE_COUNT)], childName, &spaces[Max(0, Min((int)strlen(spaces) - 1,Min(indent+9, 17)))], bar->m_SlowestChildPeak);
								s_DrawTextColor(xPos, yPos, finalStr, color);
								yPos += height;
								oddLine = !oddLine;
							}

							// Now find the matching child here.
							pfTimeBars::sFuncTime *barEntry = stack[stackSize-1];

							int childId = barEntry->childId;

							while (childId != -1)
							{
								pfTimeBars::sFuncTime *child = &frame.m_pTimes[childId];

								if (child->nameptr == childName)
								{
									// Found it.
									displayedStack.Resize(stackSize+1);
									displayedStack[stackSize] = child;
									break;
								}

								// try the next sibling then.
								childId = child->nextId;
							}
						}
					}
				}
			}
		}

		// Traverse through the entire hierarchy.
		s32 prevTb = tb;

		stack.Pop();
		int oldStackSize = stack.GetCount();

		// If reach the end of a set of timebars then chose the parent timebar. If we reach the root then break out of loop
		while(frame.m_pTimes[tb].nextId == pfTimeBars::INVALID_TIMEBAR)
		{
			if (stack.GetCount() == 0)
			{
				// We hit the end of the timebars.
				tb = pfTimeBars::INVALID_TIMEBAR;
				break;
			}
			stack.Pop();
			tb = frame.m_pTimes[tb].parentId;
			if(tb == pfTimeBars::INVALID_TIMEBAR)
				break;
		}
		// If timebar isn't invalid then chose the next timebar
		if(tb != pfTimeBars::INVALID_TIMEBAR)
		{
			tb = frame.m_pTimes[tb].nextId;
		}

		// if selected is in between the current and next time bar then look at children of this timebar or selected is after the current 
		// timebar and there aren't any valid timebars after that
		if (frame.m_pTimes[prevTb].childId != pfTimeBars::INVALID_TIMEBAR)
		{
			stack.Resize(oldStackSize);

			tb = frame.m_pTimes[prevTb].childId;
			stack.Append() = &frame.m_pTimes[prevTb];

			if(tb == pfTimeBars::INVALID_TIMEBAR)
				break;
		}
	}
}


#if __BANK
void pfTimeBars::SelectTimebarByName(const char* pName)
{
	const sTimebarFrame& rFrame = m_frame[m_renderFrame];

	if (rFrame.m_pTimes)
	{
		int nTotalCount = rFrame.m_number;

		for (s32 nTbIndex = 0; nTbIndex< nTotalCount; ++nTbIndex)
		{
			// Could compare pointers here, but that would depend on string pooling being 100% reliable.
			sFuncTime& rFunc = rFrame.m_pTimes[nTbIndex];
			if( stristr(rFunc.nameptr, pName) )
			{
				m_selected = nTbIndex;
				m_selectedLayer = rFunc.layer;
				return;
			}
		}
	}
}

static void DumpTimebarsCB()
{
	g_pfTB.DumpTimebars();
}

static void DumpTimebarsToFileCB()
{
	g_pfTB.DumpTimebarsToFile();
}

static void VerifyBudgetsCB()
{
	g_pfTB.VerifyBudgets();
}

static void DisplayBudgetsCB()
{
	g_pfTB.DisplayBudgets();
}

static void RecordBaselineCB()
{
	g_pfTB.SetRecordBaseline();
}

static void UpdateFramerateScaleCB()
{
	g_pfTB.SetRatedFramerateScale(g_pfTB.GetRatedFramerateScale());
}

static void UpdateFramerateCB()
{
	g_pfTB.SetRatedFramerate(g_pfTB.GetRatedFramerate());
}

static void SelectTimebarByNameCB()
{
	g_pfTB.SelectTimebarByName();
}

void pfTimeBarMgr::AddWidgets(bkBank &bank)
{
	bank.PushGroup("Timebars",false);
	bank.AddText("Select timebar", s_SelectByName, TIMEBAR_SELECT_BY_NAME_SIZE, false, SelectTimebarByNameCB );
	bank.AddSlider("Rated framerate", &m_ratedFramerate, 1.0f, 240.0f, 1.0f, UpdateFramerateCB );
	bank.AddSlider("Rated framerate scale", &m_ratedFramerateScale, 0.125f, 30.0f, 0.01f, UpdateFramerateScaleCB );
	bank.AddToggle("Toggle overbudget", &m_DisplayOverbudget);
	bank.AddToggle("Toggle autopausetimebars", &m_AutoPauseTimeBars);
	bank.AddToggle("Toggle frozen", &m_Frozen);
	bank.AddToggle("Absolute Mode", &m_IsAbsoluteMode);
	bank.AddToggle("Show only stalls in ghost bars", &s_ShowStallsInGhostsOnly);
	bank.AddToggle("Show detail bars", &m_IncludeDetailBarsToggle);
	
	bank.AddButton("Dump timebars", DumpTimebarsCB);
	bank.AddButton("Dump budgets", DisplayBudgetsCB);
	bank.AddButton("Verify budgets", VerifyBudgetsCB);
	bank.AddButton("Dump timebars to file", DumpTimebarsToFileCB);

	bank.AddSeparator("Baseline");
	bank.AddButton("Record baseline", &RecordBaselineCB);
	bank.AddToggle("Show", &m_ShowBaselineDifference);
	bank.AddToggle("Show (Slower only)", &m_ShowBaselineDifferenceSlowerOnly);

	bank.PopGroup();
}
#endif // __BANK

}	// namespace rage


#endif // RAGE_TIMEBARS
