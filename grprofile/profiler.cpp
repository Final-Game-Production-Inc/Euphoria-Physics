//
// profile/profiler.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

//==============================================================
// external defines

#include "diag/stats.h"

#include "profiler.h"

#if __STATS
#include "ekg.h"
#include "stats.h"

#include "bank/bank.h"
#include "bank/bkmgr.h"
#include "bank/button.h"
#include "data/callback.h"
#include "file/tcpip.h"
#include "grcore/setup.h"
#include "input/pad.h"
#include "input/keyboard.h"
#include "input/keys.h"
#include "input/mapper.h"
#include "input/mouse.h"
#include "parser/manager.h"
#include "profile/element.h"
#include "profile/group.h"
#include "profile/log.h"
#include "profile/pfpacket.h"
#include "string/stringutil.h"
#include "system/param.h"
#include "vector/vector3.h"


using namespace rage;

//==============================================================
// pfProfileRenderer


PARAM(autopauseprofiler,"[profile] Pause the profiler automatically when the game is paused");
PARAM(ekgpage, "EKG to enable at boot time (primary)");
PARAM(ekgpage2, "EKG to enable at boot time (secondary)");


void pfTimer_OnStart(pfTimer* me)
{
	// We only have a very limited number of time stamps on PS3, so we can't be so
	// liberal with sprinkling PIXBegin/PIXEnd calls everywhere
#if !__PPU
	PIXBeginC(0, 0xFF00FF,me->GetName());
#endif // !__PPU

#if __PPU
	if (g_EnableTunerAnnotation)
	{
		snPushMarker(me->GetName());
	}
#endif

#if __XENON
	if (sm_TraceTimer == me)
	{
		char nameBuff[RAGE_MAX_PATH];
		formatf(nameBuff, RAGE_MAX_PATH, "devkit:\\%s.pix2", me->GetName());
		// convenience, replace ' ' with '_'
		for(int i = 0; i < RAGE_MAX_PATH && nameBuff[i] != '\0'; i++)
		{
			if (nameBuff[i] == ' ') {
				nameBuff[i] = '_';
			}
		}
		XTraceStartRecording(nameBuff);
	}
#endif

}

void pfTimer_OnStop(pfTimer* XENON_ONLY(me))
{
#if __XENON
	if (sm_TraceTimer == me)
	{
		XTraceStopRecording();
		sm_TraceTimer = NULL;
	}
#endif

#if __PPU
	if (g_EnableTunerAnnotation)
	{
		snPopMarker();
	}
#else
	// We only have a very limited number of time stamps on PS3, so we can't be so
	// liberal with sprinkling PIXBegin/PIXEnd calls everywhere
	PIXEnd();
#endif
}

void pfProfileRenderer::Init (bool /*createWidgets*/)
{
	if (!m_Initialized)
	{
		GetRageProfiler().m_OnCreateWidgets = atDelegate<void(bkBank&)>(this, &pfProfileRenderer::AddWidgets);

		pfTimer::sm_OnStart = pfTimer_OnStart;
		pfTimer::sm_OnStop = pfTimer_OnStop;

		CreateStatsViews();

		m_Ekgs[0] = rage_new pfEKGMgr;
		m_Ekgs[1] = rage_new pfEKGMgr;

		if (PARAM_autopauseprofiler.Get())
		{
			m_AutoPause = true;
		}

		const char* ekgPage;
		if (PARAM_ekgpage.Get(ekgPage))
		{
			while (true)
			{
				if (m_Ekgs[0]->NextPage() == false)
				{
					break;
				}

				if (stristr(m_Ekgs[0]->GetPage()->GetName(), ekgPage))
				{
					break;
				}
			}
		}

		const char* ekgPage2;
		if (PARAM_ekgpage2.Get(ekgPage2))
		{
			while (true)
			{
				if (m_Ekgs[1]->NextPage() == false)
				{
					break;
				}

				if (stristr(m_Ekgs[1]->GetPage()->GetName(), ekgPage2))
				{
					break;
				}
			}
		}

		m_Initialized = true;
	}
	else
	{
		Warningf("pfProfileRenderer:Init while already initialized");
	}
}


void pfProfileRenderer::Shutdown ()
{
	if (m_Initialized)
	{

		DeleteStatsViews();
		delete m_Ekgs[0];
		m_Ekgs[0] = NULL;
		delete m_Ekgs[1];
		m_Ekgs[1] = NULL;

		delete m_Ekgs[0];
		delete m_Ekgs[1];

	}
	else
	{
		Warningf("pfProfileRenderer::Shutdown - but not initialized");
	}

	m_Initialized = false;
}


#if __BANK

void ResetEkgCb()
{
	pfEKGMgr* ekg;
	
	ekg = GetRageProfileRenderer().GetEkgMgr(0);
	if (ekg)
	{
		ekg->SetPage(ekg->GetPage());
	}

	ekg = GetRageProfileRenderer().GetEkgMgr(1);
	if (ekg)
	{
		ekg->SetPage(ekg->GetPage());
	}
}


void pfProfileRenderer::AddCreateWidgetButton(bkBank& bank)
{
	bank.AddButton("Create Profile Stats Widgets", datCallback( MFA1(pfProfiler::AddWidgets), &GetRageProfileRenderer(), &bank));
}

void pfProfileRenderer::AddWidgets (bkBank& bank)
{
	// add profile managers
	bank.PushGroup("Primary EKG");
	m_Ekgs[0]->AddWidgets(bank);
	bank.PopGroup();
	bank.PushGroup("Secondary EKG");
	m_Ekgs[1]->AddWidgets(bank);
	bank.PopGroup();

	bank.AddButton("Reset EKGs", datCallback(CFA(ResetEkgCb)));
	bank.AddToggle("Toggle autopauseekgs", &m_AutoPause);
}
#endif // __BANK

void pfProfileRenderer::Update (bool paused)
{
	if (m_AutoPause && grcSetup::IsPaused())
	{
		return;
	}

	if ( GetRageProfiler().m_Enabled )
	{
#if __BANK
		if ( pfPacket::IsConnected() )
		{
			pfPacket::ReceivePackets();
		}
#endif

		pfLogMgr::Update( paused );
		m_Ekgs[0]->Update( paused );
		m_Ekgs[1]->Update( paused );

#if __BANK
		if ( pfPacket::IsConnected() )
		{
			pfPacket::SendPackets();
		}
#endif
	}
}

static const float PRIMARY_EKG_TOP = 0.05f;
static const float PRIMARY_EKG_BOTTOM = 0.35f;
static const float SECONDARY_EKG_TOP = 0.65f;
static const float SECONDARY_EKG_BOTTOM = 0.95f;

void pfProfileRenderer::UpdateInput ()
{
	// check pads
	if (ioPad::GetPad(0).GetPressedDebugButtons() & ioPad::LUP)
	{
		m_Ekgs[0]->ScaleDisplayUp();
		m_Ekgs[1]->ScaleDisplayUp();
	}

	if (ioPad::GetPad(0).GetPressedDebugButtons() & ioPad::LDOWN)
	{
		m_Ekgs[0]->ScaleDisplayDown();
		m_Ekgs[1]->ScaleDisplayDown();
	}

	if (ioPad::GetPad(0).GetPressedDebugButtons() & ioPad::L3)
	{
		pfTimer::NextMode();
	}

	// Stats pages - here is the logic:
	// Pressing START + L2 will turn it on.
	// Pressing dpad left/right while holding START + L2 will switch pages.
	// Pressing and releasing START + L2 with the display on without having
	// pressed DPAD keys will turn it off.

	static bool statsWereOn;

	// Enable/Advance the stats page with F2 (keyboard) or start-L2 (controller)
	if (ioMapper::DebugKeyPressed(KEY_F2) || ioPad::GetPad(0).GetPressedDebugButtons() & ioPad::L2)
	{
		statsWereOn = PFSTATS.IsActive();

		if (!statsWereOn)
		{
			PFSTATS.Next();

			m_Ekgs[0]->SetPage(NULL);
			m_Ekgs[1]->SetPage(NULL);
		}
	}

	if (ioMapper::DebugKeyDown(KEY_F2) || ioPad::GetPad(0).GetDebugButtons() & ioPad::L2)
	{
		if (ioPad::GetPad(0).GetPressedDebugButtons() & ioPad::LLEFT)
		{
			statsWereOn = false;
			PFSTATS.Prev();
		}

		if (ioPad::GetPad(0).GetPressedDebugButtons() & ioPad::LRIGHT)
		{
			statsWereOn = false;
			PFSTATS.Next();
		}
	}
	else
	{
		if (ioPad::GetPad(0).GetPressedDebugButtons() & ioPad::LLEFT)
		{
			m_Ekgs[0]->PrevPage();
		}

		if (ioPad::GetPad(0).GetPressedDebugButtons() & ioPad::LRIGHT)
		{
			m_Ekgs[0]->NextPage();
		}
	}

	if (ioMapper::DebugKeyReleased(KEY_F2) || ioPad::GetPad(0).GetReleasedDebugButtons() & ioPad::L2)
	{
		if (statsWereOn)
		{
			PFSTATS.MakeInactive();
		}
	}

	if (ioMapper::DebugKeyPressed(KEY_F7) && ioMapper::DebugKeyDown(KEY_SHIFT))
	{
		pfLogMgr::NextPage();
	}

	if (ioMapper::DebugKeyPressed(KEY_F8) && ioMapper::DebugKeyDown(KEY_SHIFT))
	{
		PFSTATS.SetPage(-1);

		int whichEkg = ioMapper::DebugKeyDown(KEY_ALT) ? 1 : 0;

		if (ioMapper::DebugKeyDown(KEY_CONTROL))
		{
			m_Ekgs[whichEkg]->PrevPage();
		}
		else
		{
			m_Ekgs[whichEkg]->NextPage();
		}
	}

	if (ioMapper::DebugKeyPressed(KEY_F9) && ioMapper::DebugKeyDown(KEY_SHIFT))
	{
		m_Ekgs[0]->ScaleDisplayUp();
		m_Ekgs[1]->ScaleDisplayUp();
	}

	if (ioMapper::DebugKeyPressed(KEY_F10) && ioMapper::DebugKeyDown(KEY_SHIFT))
	{
		m_Ekgs[0]->ScaleDisplayDown();
		m_Ekgs[1]->ScaleDisplayDown();
	}

	// Get the raw mouse position.
	const float  mouseScreenX = static_cast<float>(ioMouse::GetX());
	const float  mouseScreenY = static_cast<float>(ioMouse::GetY());

	// Convert the mouse position into screen 0 to 1 coordinates.
	const float mouseScreen0to1X = mouseScreenX / grcViewport::GetDefaultScreen()->GetWidth();
	const float mouseScreen0to1Y = mouseScreenY / grcViewport::GetDefaultScreen()->GetHeight();

	// Convert the mouse position into ekg 0 to 1 coordinates.
	const float primaryMouseEKG0to1Y = (mouseScreen0to1Y - PRIMARY_EKG_TOP) / (PRIMARY_EKG_BOTTOM - PRIMARY_EKG_TOP);
	const float secondaryMouseEKG0to1Y = (mouseScreen0to1Y - SECONDARY_EKG_TOP) / (SECONDARY_EKG_BOTTOM - SECONDARY_EKG_TOP);

	m_Ekgs[0]->UpdateMouse(mouseScreen0to1X, primaryMouseEKG0to1Y);
	m_Ekgs[1]->UpdateMouse(mouseScreen0to1X, secondaryMouseEKG0to1Y);
}


void pfProfileRenderer::Draw (int screenWidth, int screenHeight)
{
	m_Ekgs[0]->Draw(screenWidth, screenHeight, PRIMARY_EKG_TOP, PRIMARY_EKG_BOTTOM);
	m_Ekgs[1]->Draw(screenWidth, screenHeight, SECONDARY_EKG_TOP, SECONDARY_EKG_BOTTOM);
}


void pfProfileRenderer::Draw ()
{
    m_Ekgs[0]->Draw(0.05f, 0.35f);
	m_Ekgs[1]->Draw(0.65f, 0.95f);
}


void pfProfileRenderer::BeginFrame ()
{
	if (m_AutoPause && grcSetup::IsPaused())
	{
		return;
	}

	for(int i=0; i<GetRageProfiler().GetNumGroups(); i++)
	{
		GetRageProfiler().GetGroup(i)->BeginFrame();
	}
}


void pfProfileRenderer::EndFrame ()
{
	if (m_AutoPause && grcSetup::IsPaused())
	{
		return;
	}

	GetRageProfiler().FlipElapsedTimes();

	for(int i=0; i<GetRageProfiler().GetNumGroups(); i++)
	{
		GetRageProfiler().GetGroup(i)->EndFrame();
	}
}


void pfProfileRenderer::DebugView ()
{
	for(int i=0; i<GetRageProfiler().GetNumGroups(); i++)
	{
		GetRageProfiler().GetGroup(i)->DebugView();
	}
}

bool pfProfileRenderer::AnyEkgsActive ()
{
	return m_Ekgs[0]->GetPage() || m_Ekgs[1]->GetPage();
}


void pfProfileRenderer::DisplayGroupStatsf(pfGroup* group)
{
	pfStatsf("Group: %s",group->GetName());	

	////////////////////////////////////////////
	// display the timer elements

	pfTimer * timer = group->GetNextTimer(NULL);

	if (timer!=NULL)
	{
		switch (pfTimer::GetMode()) {
		case pfTimer::ELAPSED_TIME:
			pfStatsf("  --- Timers ---           Frame/    peak  -  Calls/ peak  - PerCall");
			break;
		case pfTimer::COUNTER0:
			pfStatsf("  -- Insn Cache Misses --  Frame/    peak  -  Calls/ peak  - PerCall");
			break;
		case pfTimer::COUNTER1:
			pfStatsf("  -- Data Cache Misses --  Frame/    peak  -  Calls/ peak  - PerCall");
			break;
		}
	}

	while (timer!=NULL)
	{
		if (timer->GetActive())
		{
			DisplayTimerStatsf(timer);
		}

		timer = timer->GetNext();
	}

	////////////////////////////////////////////
	// display the value elements

	pfValue * value = group->GetNextValue(NULL);

	if (value!=NULL)
	{
		pfStatsf("  --- Values ---");
	}

	while (value!=NULL)
	{
		char displayString[64];
		value->WriteToString(displayString,64);

		const char *name;
		s32 index;
		name = value->GetName(index);
		if(index >= 0)
		//if(value->NumValues() > 1)
		{
			pfStatsf("  %s_%d: %s",name,index,displayString);
		}
		else
		{
			pfStatsf("  %s: %s",name,displayString);
		}

		value = value->GetNext();
	}
}

void pfProfileRenderer::DisplayTimerStatsf(pfTimer* timer)
{
	// formatted printing to Statsf
	const int labelMaxLength = 25;
	static char outputString[512];
	sprintf(outputString,"  %s:", timer->GetName());

	if (StringLength(timer->GetName())<labelMaxLength-3)
	{
		memset(outputString+StringLength(timer->GetName())+3,' ',labelMaxLength-StringLength(timer->GetName())-3);
	}

	if (timer->GetAverageCalls() > 0.1f)
	{
		sprintf(outputString+labelMaxLength,"%7.1f/%8.1f   %7d/%5d    %7.1f",
			1000000.0f * timer->GetAverageTime(), 1000000.0f * timer->GetPeakTime(),
		    round(timer->GetAverageCalls()), timer->GetPeakCalls(), 1000000.0f * timer->GetAverageTime()/(Max(timer->GetAverageCalls(),0.01f)));
	}
	else
	{
		sprintf(outputString+labelMaxLength,"%7.1f/%8.1f   %7d/%5d        ---",
			1000000.0f * timer->GetAverageTime(), 1000000.0f * timer->GetPeakTime(), round(timer->GetAverageCalls()), timer->GetPeakCalls());
	}

	pfStatsf(outputString);
}

void pfProfileRenderer::CreateStatsViews ()
{
	Assert(m_NumStatsViews==0);

	pfPage* page = GetRageProfiler().GetNextPage(NULL, false);
	while (page)
	{
		Assert(m_NumStatsViews<kMaxStatsViews);
		m_StatsViews[m_NumStatsViews++] = rage_new pfStatsView(page);
		page = GetRageProfiler().GetNextPage(page, false);
	}
}


void pfProfileRenderer::DeleteStatsViews ()
{
	Assert(m_Initialized);

	for (int i=0; i<m_NumStatsViews; i++)
	{
		delete m_StatsViews[i];
	}

	m_NumStatsViews = 0;
}

//==============================================================
// pfStatsView

pfStatsView::pfStatsView (pfPage * page)
{
	m_Page = page;

	PFSTATS.AddPage(MakeFunctor(*this, &pfStatsView::Display));
}

pfStatsView::~pfStatsView ()
{
	PFSTATS.RemovePage(MakeFunctor(*this, &pfStatsView::Display));
}


void pfStatsView::Display () const
{
	grcViewport::SetCurrent(grcViewport::GetDefaultScreen());

	pfStatsf("Profiler Data: %s",m_Page->GetName());
	pfStatsf("");

	int i, numGroups;
	pfGroup * group;

	numGroups = m_Page->GetNumGroups();
	for (i=0; i<numGroups; i++)
	{
		group = m_Page->GetGroup(i);

		if (group->GetActive())
		{
			GetRageProfileRenderer().DisplayGroupStatsf(group);

			if (i!=numGroups-1)
			{
				// add a blank space before the next group
				pfStatsf("");
			}
		}
	}
}


#endif // __STATS

rage::pfProfileRenderer & rage::GetRageProfileRenderer()
{
	static rage::pfProfileRenderer sProfileRenderer(GetRageProfiler());
	return sProfileRenderer;
}

