//
// profile/ekg.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "grprofile/ekg.h"

#if __STATS

#include "bank/bank.h"
#include "bank/combo.h"
#include "grcore/device.h"
#include "grcore/im.h"
#include "profile/profiler.h"
#include "string/string.h"
#include "system/timemgr.h"
#include "vector/colors.h"

using namespace rage;

namespace rage {
	extern bool g_TallyGrcCalls;
	Color32 kEKGTextFontColor(255,255,255,255);

	#define		DEFAULT_RESET_TIME		10.0f				// in seconds
}

/////////////////////////////////////////////////////////////////////
// pfEKGLog

void pfEKGLog::AddSample (float f)
{
	m_SampleStart = (m_SampleStart+MAX_SAMPLES-1) % MAX_SAMPLES;
	m_Samples[m_SampleStart] = f;
	m_NumSamples = Min(m_NumSamples+1,(int)MAX_SAMPLES);
}


void pfEKGLog::SetColor ()
{
	static float red = 0.25f ;
	static float green = 0.75f ;
	static float blue = 0.9f ;
	static int cycle = 0 ;

	float prevRed, prevGreen, prevBlue;

	prevRed = red;
	prevGreen = green;
	prevBlue = blue;

	float add = (float)(cycle % 1000) / 1000.0f ;

	switch (cycle % 3)
	{
		case 0 :
			red += add * 1.1f ;
			if (red > 1.0f)
			{
				red -= 1.0f ;
			}
			break ;

		case 1 :
			green += add * 0.9f ;
			if (green > 1.0f)
			{
				green -= 1.0f ;
			}
			break ;
		
		case 2 :
			blue += add * 0.5f ;
			if (blue > 1.0f)
			{
				blue -= 1.0f ;
			}
			break ;
	}

	cycle += 343 ;

	// make sure it is bright enough
	while (red*red+green*green+blue*blue < 1.0f)
	{
		red += 0.1f;
		blue += 0.1f;
		green += 0.1f;
	}

	// make sure it is different enough
	if ((prevRed-red)*(prevRed-red) + (prevBlue-blue)*(prevBlue-blue) + (prevGreen-green)*(prevGreen-green) < 0.5f)
	{
		float temp;
		temp = red;
		red = green;
		green = blue;
		blue = temp;
	}

	m_Color.Set(red,blue,green);
}


/////////////////////////////////////////////////////////////////////
// pfEKGMgr

const float pfEKGMgr::SCALING_FACTOR = 1.2f;

const int kSamplesPerSecond = 60;

pfEKGMgr::pfEKGMgr()
{
	m_Enabled = true;
	m_ShowType = pfEKGMgr::kShowTimeTypeFrame;
	m_ShowValueType = pfValue::MAX_GET_AS_FLOAT_RETURNS;
	m_CurrentPage = NULL;
	strcpy(m_CurrentPageName, "*no page*");
#if __BANK
	m_ComboPageSelection = 0;
	m_SelectionPageCombo = NULL;
#endif	// __BANK
	m_NumUsedLogs = 0;
	m_AutoScale = true;
	m_AutoDownScale = true;
	m_AutoSort = true;

	m_Samples = 0;
	m_TimeMod = 0.0f;
	m_TimeEarliest = -(float)pfEKGLog::MAX_SAMPLES / (float)kSamplesPerSecond;
}

void pfEKGMgr::Enable(bool enable)
{
	m_Enabled = enable;
}

bool pfEKGMgr::NextPage ()
{
	SetPage(GetRageProfiler().GetNextPage(m_CurrentPage));
	return (m_CurrentPage != NULL);
}


bool pfEKGMgr::PrevPage ()
{
	SetPage(GetRageProfiler().GetPrevPage(m_CurrentPage));
	return (m_CurrentPage != NULL);
}


void pfEKGMgr::SetPage(pfPage *page)
{
	ResetLogs();

	m_CurrentPage = page;

	if (m_CurrentPage)
	{
		Displayf("pfEKGMgr - EKG page '%s'",m_CurrentPage->GetName());
		strcpy(m_CurrentPageName,m_CurrentPage->GetName());
	}
	else
	{
		Displayf("pfEKGMgr - no EKG page");
		strcpy(m_CurrentPageName,"*no page*");
	}

	// Sometimes redundant, it sets the current selection whether from combo box or hot keys.
#if __BANK
	if ( m_SelectionPageCombo != NULL )
	{
		m_ComboPageSelection = m_SelectionPageCombo->GetStringIndex(m_CurrentPageName);
	}
#endif	// __BANK

	m_TimeMax = 0.0f;
	m_ValueMax = 0.0f;
	m_ValueMin = 0.0f;

	m_DisplayMax = 1.0f / 1000.0f;
	m_DisplayMin = 0.0f;

	m_ResetTime = DEFAULT_RESET_TIME;

	m_AutoScale = true;
	m_AutoDownScale = true;
}

static const char *sShowTypes[] =
{
	"Frame Time",
	"Average Time",
	"Peak Time",
	"Frame Calls",
	"Average Calls",
	"Peak Calls"
};


#if __BANK
void pfEKGMgr::AddWidgets (bkBank & bank)
{
	static const char *sShowValueTypes[pfValue::MAX_GET_AS_FLOAT_RETURNS + 1] =
	{
		"Current",
		"Average",
		"Peak", 
		"Value Default"
	};

	char temp[128] = "EKG - enabled";

	bank.AddToggle(temp,&m_Enabled);
	bank.AddCombo("ShowTimeType",&m_ShowType,kShowTypeNum,sShowTypes);
	bank.AddCombo("ShowValueType",&m_ShowValueType,pfValue::MAX_GET_AS_FLOAT_RETURNS + 1,sShowValueTypes);
	bank.AddToggle("AutoScale",&m_AutoScale);
	bank.AddToggle("AutoDownScale",&m_AutoDownScale);
	bank.AddToggle("AutoSort",&m_AutoSort);

	// Add a combo box with the active page names
	int totalPageCount = GetRageProfiler().GetNumGroups() + 1;
	const char** comboNames = rage_new const char*[totalPageCount];
	comboNames[0] = "*no page*";
	int activePageCount = 1;
	pfPage *currentPage = NULL;
	for (int i = 0; i < totalPageCount; ++i)
	{
		currentPage = GetRageProfiler().GetNextPage(currentPage);
		if (currentPage)
		{
			comboNames[activePageCount] = currentPage->GetName();
		}
		else
		{ // No more active pages
			break;
		}

		++activePageCount;
	}

	m_SelectionPageCombo = bank.AddCombo("EKG - page", &m_ComboPageSelection, activePageCount, comboNames, datCallback(MFA(pfEKGMgr::SelectPageComboCb), this), "Select the EKG page to view");

	delete [] comboNames;
}

void pfEKGMgr::SelectPageComboCb ()
{
	if (m_SelectionPageCombo == NULL)
	{
		return;
	}

	const char* pageName = m_SelectionPageCombo->GetString(m_ComboPageSelection);
	if (pageName == NULL)
	{
		return;
	}

	if (strcmp(pageName, "*no page*") == 0)
	{
		SetPage(NULL);
		return;
	}

	pfPage *page = GetRageProfiler().GetPageByName(pageName);
	if (page == NULL)
	{
		return;
	}

	SetPage(page);
}

#endif	// __BANK

int pfEKGMgr::FindTimer (const pfTimer * timer)
{
	for (int i=0; i<kMaxLogs; i++)
	{
		if (m_ActiveTimers[i]==timer)
		{
			return i;
		}
	}
	return -1;
}


int pfEKGMgr::FindValue (const pfValue * value)
{
	for (int i=0; i<kMaxLogs; i++)
	{
		if (m_ActiveValues[i]==value)
		{
			return i;
		}
	}
	return -1;
}


int pfEKGMgr::InsertTimer (const pfTimer * timer)
{
	Assert(FindTimer(timer)<0);

	Assert(m_NumUsedLogs<kMaxLogs);

	int logIndex = m_UnusedLogs[0];
	Assert(logIndex>=0 && logIndex<kMaxLogs);
	m_UnusedLogs[0] = m_UnusedLogs[kMaxLogs-m_NumUsedLogs-1];
	m_UnusedLogs[kMaxLogs-m_NumUsedLogs-1] = -1;
	m_NumUsedLogs++;

	m_ActiveTimers[logIndex] = timer;
	m_ActiveValues[logIndex] = NULL;

	return logIndex;
}


void pfEKGMgr::RemoveTimer (const pfTimer * timer)
{
	int logIndex = FindTimer(timer);
	Assert(logIndex>=0 && logIndex<kMaxLogs);

	Assert(m_ActiveTimers[logIndex]==timer);
	Assert(m_ActiveValues[logIndex]==NULL);
	m_ActiveTimers[logIndex] = NULL;

	m_Logs[logIndex].Reset();
	m_UnusedLogs[kMaxLogs-m_NumUsedLogs] = logIndex;

	m_NumUsedLogs--;
}


int pfEKGMgr::InsertValue (const pfValue * value)
{
	Assert(FindValue(value)<0);

	Assert(m_NumUsedLogs<kMaxLogs);

	int logIndex = m_UnusedLogs[0];
	Assert(logIndex>=0 && logIndex<kMaxLogs);
	m_UnusedLogs[0] = m_UnusedLogs[kMaxLogs-m_NumUsedLogs-1];
	m_UnusedLogs[kMaxLogs-m_NumUsedLogs-1] = -1;
	m_NumUsedLogs++;

	m_ActiveTimers[logIndex] = NULL;
	m_ActiveValues[logIndex] = value;

	return logIndex;
}


void pfEKGMgr::RemoveValue (const pfValue * value)
{
	int logIndex = FindValue(value);
	Assert(logIndex>=0 && logIndex<kMaxLogs);

	Assert(m_ActiveTimers[logIndex]==NULL);
	Assert(m_ActiveValues[logIndex]==value);
	m_ActiveValues[logIndex] = NULL;

	m_Logs[logIndex].Reset();
	m_UnusedLogs[kMaxLogs-m_NumUsedLogs] = logIndex;

	m_NumUsedLogs--;
}


int sort_compare_timer(const void * t1, const void * t2)
{
	const pfTimer & timer1 = **(const pfTimer**)t1;
	const pfTimer & timer2 = **(const pfTimer**)t2;
	return (timer1.GetAverageTime()<=timer2.GetAverageTime()) ? 1 : -1;
}


int sort_compare_timer_peak(const void * t1, const void * t2)
{
	const pfTimer & timer1 = **(const pfTimer**)t1;
	const pfTimer & timer2 = **(const pfTimer**)t2;
	return (timer1.GetPeakTime()<=timer2.GetPeakTime()) ? 1 : -1;
}


int sort_compare_calls(const void * t1, const void * t2)
{
	const pfTimer & timer1 = **(const pfTimer**)t1;
	const pfTimer & timer2 = **(const pfTimer**)t2;
	return (timer1.GetAverageCalls()<=timer2.GetAverageCalls()) ? 1 : -1;
}


int sort_compare_calls_peak(const void * t1, const void * t2)
{
	const pfTimer & timer1 = **(const pfTimer**)t1;
	const pfTimer & timer2 = **(const pfTimer**)t2;
	return (timer1.GetPeakCalls()<=timer2.GetPeakCalls()) ? 1 : -1;
}


int sort_compare_value(const void * v1, const void * v2)
{
	const pfValue & value1 = **(const pfValue**)v1;
	const pfValue & value2 = **(const pfValue**)v2;
	//return (value1.GetAsFloat()<=value2.GetAsFloat()) ? 1 : -1;
	return (value1.GetAverage()<=value2.GetAverage()) ? 1 : -1;
}


void pfEKGMgr::SortPageElements (int & foundTimers, int & foundValues, int bufferSize, const pfTimer ** timerBuffer, const pfValue ** valueBuffer)
{
	foundTimers = 0;
	foundValues = 0;

	pfGroup * group = NULL;

	while ((group = m_CurrentPage->GetNextGroup(group))!=NULL)
	{
		if (group->GetActive())
		{
			pfTimer * timer = NULL;
			while ((timer = group->GetNextTimer(timer))!=NULL && foundTimers < bufferSize)
			{
				if (timer->GetInit() && timer->GetActive())
				{
					timerBuffer[foundTimers++] = timer;
				}
			}

			pfValue * value = NULL;
			while ((value = group->GetNextValue(value))!=NULL && foundValues < bufferSize)
			{
				if (value->GetInit() && value->GetActive())
				{
					valueBuffer[foundValues++] = value;
				}
			}
		}
	}

	switch(m_ShowType)
	{
	case kShowTimeTypeFrame:
	case kShowTimeTypeAverage:
		qsort(timerBuffer,foundTimers,sizeof(const pfTimer *),sort_compare_timer);
		break;
	case kShowTimeTypePeak:
		qsort(timerBuffer,foundTimers,sizeof(const pfTimer *),sort_compare_timer_peak);
		break;
	case kShowCallsTypeFrame:
	case kShowCallsTypeAverage:
		qsort(timerBuffer,foundTimers,sizeof(const pfTimer *),sort_compare_calls);
		break;
	default: // case kShowCallsTypePeak:
		qsort(timerBuffer,foundTimers,sizeof(const pfTimer *),sort_compare_calls_peak);
		break;
	}
	if(m_AutoSort)
		qsort(valueBuffer,foundValues,sizeof(const pfValue *),sort_compare_value);
}


void pfEKGMgr::UpdateTimersAndValues ()
{
	const int kMaxConsideredElements = 256;
	const pfTimer * timerBuffer[kMaxConsideredElements];
	const pfValue * valueBuffer[kMaxConsideredElements];
	int foundTimers, foundValues;

	SortPageElements(foundTimers,foundValues,kMaxConsideredElements,timerBuffer,valueBuffer);

	// timerBuffer[0..foundTimers] contains top requested timers
	// valueBuffer[0..foundValues] contains top requested values

	// Interleave the timers and values, so that the top N of both are displayed.

	bool keepValue[kMaxLogs];
	bool keepTimer[kMaxLogs];
	const pfTimer * addTimer[kMaxLogs];
	const pfValue * addValue[kMaxLogs];
	int addedOrder[kMaxLogs];

	for (int i=0; i<kMaxLogs; i++)
	{
		keepValue[i] = false;
		keepTimer[i] = false;
		addTimer[i] = NULL;
		addValue[i] = NULL;
		addedOrder[i] = -1;
		m_LogOrder[i] = -1;
	}

	int numElements = 0;
	int addedItems = 0;
	// Loop over the timerBuffer, valueBuffer arrays. For each item in timerBuffer, valueBuffer, see
	// if it already exists in the list of active timers. If so, leave it there. If not, add it. 
	// Continue until we've got at most kMaxLogs elements.
	for (int i=0; i<kMaxLogs && numElements<kMaxLogs; i++)
	{
		if (i<foundTimers && numElements<kMaxLogs)
		{
			int logIndex = FindTimer(timerBuffer[i]);
			if (logIndex>=0)
			{
				// This timer already exists, keep it at the same index
				keepTimer[logIndex] = true;
				m_LogOrder[numElements] = logIndex;
			}
			else
			{
				// This is a new timer, add it to an empty spot in the active array
				addTimer[i] = timerBuffer[i];
				addedOrder[addedItems++] = numElements; // 
				m_LogOrder[numElements] = -1;
			}
			numElements++;
		}

		if (i<foundValues && numElements<kMaxLogs)
		{
			int logIndex = FindValue(valueBuffer[i]);
			if (logIndex>=0)
			{
				// This value already exists, keep it at the same index.
				keepValue[logIndex] = true;
				m_LogOrder[numElements] = logIndex;
			}
			else
			{
				// This is a new value, add it to an empty spot in the active array
				addValue[i] = valueBuffer[i];
				addedOrder[addedItems++] = numElements;
				m_LogOrder[numElements] = -1;
			}
			numElements++;
		}
	}

	// Remove timers/values that are no longer wanted.
	for (int i=0; i<kMaxLogs; i++)
	{
		if (!keepTimer[i] && m_ActiveTimers[i])
		{
			RemoveTimer(m_ActiveTimers[i]);
		}
		if (!keepValue[i] && m_ActiveValues[i])
		{
			RemoveValue(m_ActiveValues[i]);
		}
	}

	// Add new timers/values.
	addedItems = 0;
	for (int i=0; i<kMaxLogs; i++)
	{
		int logIndex = -1;
		if (addTimer[i])
		{
			logIndex = InsertTimer(addTimer[i]);
		}
		if (logIndex>=0)
		{
			Assert(addedOrder[addedItems]>=0);
			m_LogOrder[addedOrder[addedItems]] = logIndex;
			addedItems++;
		}

		logIndex = -1;
		if (addValue[i])
		{
			logIndex = InsertValue(addValue[i]);
		}
		if (logIndex>=0)
		{
			Assert(addedOrder[addedItems]>=0);
			m_LogOrder[addedOrder[addedItems]] = logIndex;
			addedItems++;
		}
	}

	// Get rid of all the -1s in the m_LogOrder array (push everything else down)
	for (int i=0, j=0; j<kMaxLogs; j++)
	{
		if (m_LogOrder[i]<0)
		{
			j++;
		}
		if (i!=j && j<kMaxLogs)
		{
			m_LogOrder[i]=m_LogOrder[j];
		}
		if (m_LogOrder[i]>=0)
		{
			i++;
		}
	}

#if __ASSERT
	for (int i=0; i<kMaxLogs; i++)
	{
		if (i<m_NumUsedLogs)
		{
			Assert(m_LogOrder[i]>=0 && m_LogOrder[i]<kMaxLogs);
		}
		else
		{
			Assert(m_LogOrder[i]==-1);
		}
	}
#endif
}


void pfEKGMgr::CollectSamples ()
{
	float sample=0;

	for (int slot=0; slot<kMaxLogs; slot++)
	{
		Assert((m_ActiveTimers[slot]==NULL || m_ActiveValues[slot]==NULL));
		if (m_ActiveTimers[slot]!=NULL)
		{
			switch(m_ShowType)
			{
				case kShowTimeTypeFrame:
					sample = m_ActiveTimers[slot]->GetFrameTime();
					break;
				case kShowTimeTypeAverage:
					sample = m_ActiveTimers[slot]->GetAverageTime();
					break;
				case kShowTimeTypePeak:
					sample = m_ActiveTimers[slot]->GetPeakTime();
					break;
				case kShowCallsTypeFrame:
					sample = float(m_ActiveTimers[slot]->GetCalls());
					break;
				case kShowCallsTypeAverage:
					sample = m_ActiveTimers[slot]->GetAverageCalls();
					break;
				case kShowCallsTypePeak:
					sample = float(m_ActiveTimers[slot]->GetPeakCalls());
					break;
				default:
					Assert(0);
					break;
			};
			m_Logs[slot].SetType(pfEKGLog::TimerLog);
			m_Logs[slot].AddSample(sample);

			switch(m_ShowType)
			{
			case kShowCallsTypeFrame:
			case kShowCallsTypeAverage:
			case kShowCallsTypePeak:
				if (sample>m_ValueMax)
				{
					m_ValueMax = sample;
					m_ResetTime = DEFAULT_RESET_TIME;
				}

				if(sample < m_ValueMin)
				{
					m_ValueMin = sample;
				}
				break;
			default:
				if (sample>m_TimeMax)
				{
					m_TimeMax = sample;
					m_ResetTime = DEFAULT_RESET_TIME;
				}
				break;
			}
		}
		if (m_ActiveValues[slot]!=NULL)
		{
			sample = m_ActiveValues[slot]->GetAsFloat((pfValue::eGetFloatReturns)m_ShowValueType);
			m_Logs[slot].SetType(pfEKGLog::ValueLog);
			m_Logs[slot].AddSample(sample);

			if (sample>m_ValueMax)
			{
				m_ValueMax = sample;
				m_ResetTime = DEFAULT_RESET_TIME;
			}

			if(sample < m_ValueMin)
			{
				m_ValueMin = sample;
			}
		}
	}
}


void pfEKGMgr::Update (bool paused)
{
	if (m_Enabled && m_CurrentPage && !paused)
	{
		if( m_AutoDownScale )
		{
			m_ResetTime -= TIME.GetSeconds();
			if( m_ResetTime < 0.0f )
			{
				m_TimeMax = 0.0f;
				m_ValueMax = 0.0f;

				for (int slot=0; slot<kMaxLogs; ++slot )
				{			
					if( m_ActiveTimers[slot] != NULL )
					{
						for( int j = 0; j < m_Logs[slot].m_NumSamples; ++j )
						{				
							float logSample = m_Logs[slot].m_Samples[j];

							switch(m_ShowType)
							{
								case kShowCallsTypeFrame:
								case kShowCallsTypeAverage:
								case kShowCallsTypePeak:
									if (logSample > m_ValueMax )
									{
										m_ValueMax = logSample;
									}

									if( logSample < m_ValueMin )
									{
										m_ValueMin = logSample;
									}
									break;
								default:
									if (logSample > m_TimeMax )
									{
										m_TimeMax = logSample;
									}
									break;
							}

						} // end for
					} // end if
				} // end for
							
				m_DisplayMax = 1.0f / 1000.0f;
				m_ResetTime = 0.0f;
			}
		} // autodownscale

		UpdateTimersAndValues();

		// collect samples
		CollectSamples();

		if(m_AutoScale)
		{
			if(m_ValueMax)
			{
				m_DisplayMax = m_ValueMax;
			}
			if(m_ValueMin)
			{
				m_DisplayMin = m_ValueMin;
			}
			else if(m_TimeMax)
			{
				m_DisplayMax = m_TimeMax;
			}
		}

		// update log times
		m_Samples++;
		m_TimeMod = (float)(m_Samples % kSamplesPerSecond) / (float)kSamplesPerSecond;
	}
}

const float EKG_CHART_RIGHT = 0.6725f;
const float EKG_CHART_LEFT = 0.0325f;

void pfEKGMgr::UpdateMouse (float x, float y)
{
	if (y < 0.0f || y >= 1.0 || x < EKG_CHART_LEFT || x >= EKG_CHART_RIGHT)
	{
		m_HoverSlot = 0;
	}
	else
	{
		m_HoverSlot = pfEKGLog::MAX_SAMPLES - (int)(pfEKGLog::MAX_SAMPLES * ((x - EKG_CHART_LEFT) / (EKG_CHART_RIGHT - EKG_CHART_LEFT)));
	}
}


void pfEKGMgr::ResetLogs ()
{
	int i;

	for (i=0; i<kMaxLogs; i++)
	{
		m_Logs[i].Reset();
	}

	for (i=0; i<kMaxLogs; i++)
	{
		m_ActiveTimers[i] = NULL;
		m_ActiveValues[i] = NULL;
		m_UnusedLogs[i] = i;
		m_LogOrder[i] = -1;
	}

	m_NumUsedLogs = 0;
}


void pfEKGMgr::Draw (float top, float bottom)
{
    Draw(GRCDEVICE.GetWidth(), GRCDEVICE.GetHeight(), top, bottom);
}


void pfEKGMgr::Draw (int screenWidth, int screenHeight, float top, float bottom)
{
	if (!GetRageProfiler().m_Enabled || !m_Enabled || !m_CurrentPage)
	{
		return;
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// drawing variables

    float width = float(screenWidth);
    float height = float(screenHeight);

	const float boxTop = height * top;
	const float boxBottom = height * bottom;

	float boxRight = width * EKG_CHART_RIGHT;
	float boxLeft = width * EKG_CHART_LEFT;

	const float boxHeight = boxBottom - boxTop;
	const float boxWidth = boxRight - boxLeft;

	float border = Max((height + width) * 0.003f,1.0f);

	float graphTop = boxTop + border;
	float graphLeft = boxLeft + border;
	float graphBottom = boxBottom - border;
	float graphRight = boxRight - 1.05f * border - 1;

	float graphHeight = graphBottom - graphTop;

	float tickBase = 10.0f;
	float vTickWidth = 1.0e-6f;

	// now allowing the graphing to go negative. vTickWidth, tickWidth, and displayScale are now based on this instead of sm_DisplayMax
	float displayRange = m_DisplayMax - m_DisplayMin;

	while (vTickWidth * tickBase < displayRange)
	{
		vTickWidth *= tickBase;
	}
	
	float displayScale = 1.0f / Max(1.0e-6f,displayRange);

	/////////////////////////////////////////////////////////////////////////////////////
	// setup the graphics

	if(!m_Chart.DrawOrthoBegin())
	{
		return;
	}

	bool oldTally = g_TallyGrcCalls;
	g_TallyGrcCalls = false;

	/////////////////////////////////////////////////////////////////////////////////////
	// draw the background

	m_Chart.SetDrawPosition(graphTop,graphBottom,graphLeft,graphRight);
	m_Chart.SetBorderWidth(border,border*5.0f/8.0f);
	m_Chart.DrawBackground();

	float xMin = m_TimeEarliest+m_TimeMod;
	float xMax = m_TimeMod;
	float yMin = Min(0.0f, m_DisplayMin);
	float yMax = Max(2.0e-6f, m_DisplayMax);

	m_Chart.SetXRange(xMin,xMax);
	m_Chart.SetYRange(yMin,yMax);
	m_Chart.DrawGrid(1.0f,vTickWidth);

	if(m_TimeMax>0 && m_ValueMax==0 && m_ValueMin==0)
	{
		grcColor4f(0.8f,0.5f,0.5f,0.6f);
		// draw a 1/60th second mark
		if (m_DisplayMax >= 1.0f/60.0f)
		{
			m_Chart.DrawSegment(Vector2(xMin,1.0f/60.0f),Vector2(xMax,1.0f/60.0f));
		}
		// draw a 1/30th second mark
		if (m_DisplayMax >= 1.0f/30.0f)
		{
			m_Chart.DrawSegment(Vector2(xMin,1.0f/30.0f),Vector2(xMax,1.0f/30.0f));
		}
		// draw a 1/20th second mark
		if (m_DisplayMax >= 1.0f/20.0f)
		{
			m_Chart.DrawSegment(Vector2(xMin,1.0f/20.0f),Vector2(xMax,1.0f/20.0f));
		}
	}

	if (m_HoverSlot >= 0)
	{
		float sampleIX  = xMax - (m_HoverSlot / (float)(pfEKGLog::MAX_SAMPLES-1)) * (xMax - xMin);

		grcColor4f(0.5f, 0.5f, 0.8f, 0.6f);
		grcBegin(drawLines,2);
		grcVertex2f(sampleIX,yMin);
		grcVertex2f(sampleIX,yMax);
		grcEnd();
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// draw data
	int i, j;
	const int lineBatchSize = 100;
	int segmentsDrawn = 0;
	for (j=0; j<m_NumUsedLogs; j++)
	{
		int logIndex = m_LogOrder[j];
		grcColor3f(m_Logs[logIndex].GetColor());

		float y0, y1;
		for (i=0; i<m_Logs[logIndex].m_NumSamples-1; i++)
		{
			if (segmentsDrawn % lineBatchSize == 0)
			{
				grcBegin(drawLines,lineBatchSize*2);
			}

			y0 = m_Logs[logIndex].m_Samples[(m_Logs[logIndex].m_SampleStart+i)%pfEKGLog::MAX_SAMPLES];
			y1 = m_Logs[logIndex].m_Samples[(m_Logs[logIndex].m_SampleStart+i+1)%pfEKGLog::MAX_SAMPLES];

			float sampleI0X  = xMax - (i / (float)(pfEKGLog::MAX_SAMPLES-1)) * (xMax - xMin);
			float sampleI1X = xMax - ((i+1) / (float)(pfEKGLog::MAX_SAMPLES-1)) * (xMax - xMin);
			grcVertex2f(sampleI0X,y0);
			grcVertex2f(sampleI1X,y1);

			segmentsDrawn++;

			if (segmentsDrawn % lineBatchSize == 0)
			{
				grcEnd();
			}
		}
	}

	// clean up final grc batch
	while (segmentsDrawn % lineBatchSize != 0)
	{
		grcColor(Color32(0,0,0,0));
		grcVertex2f(Vector2(-1000,-1000));
		grcVertex2f(Vector2(-1000,-1000));
		segmentsDrawn++;
		if (segmentsDrawn % lineBatchSize == 0)
		{
			grcEnd();
		}
	}

	m_Chart.DrawOrthoEnd();

	/////////////////////////////////////////////////////////////////////////////////////
	// draw a legend

	grcViewport * poppedViewport = grcViewport::GetCurrent();
	grcViewport::SetCurrent(grcViewport::GetDefaultScreen());

	float xpos, ypos;
	const float kMaxTextVertSpace = 22.0f;
	const float kMinTextVertSpace = 13.0f;
	const float kMaxTextVertSpaceBoxHeight = 300.0f;
	const float kMinTextVertSpaceBoxHeight = 160.0f;
	const float textVertSpace = RampValue(boxHeight,kMinTextVertSpaceBoxHeight,kMaxTextVertSpaceBoxHeight,kMinTextVertSpace,kMaxTextVertSpace);
	const float dashWidth = 10;

	const float textHeight = 15.0f;

	const float legendHorizOffset = Clamp(boxWidth/100.0f,2.0f,8.0f);

	xpos = boxRight + legendHorizOffset;
	ypos = boxTop + 1.0f * textVertSpace + 0.5f * textHeight;

	for (j=0; j<m_NumUsedLogs; j++)
	{
		// draw legend lines
		grcBegin(drawLines,4);
		grcColor3f(m_Logs[m_LogOrder[j]].GetColor());
		grcVertex2f(xpos,ypos);
		grcVertex2f(xpos+dashWidth,ypos);
		grcVertex2f(xpos,ypos+1.0f);
		grcVertex2f(xpos+dashWidth,ypos+1.0f);
		grcEnd();

		ypos += textVertSpace;
	}

	grcViewport::SetCurrent(poppedViewport);

	xpos = boxRight + dashWidth + legendHorizOffset * 2.0f;
	ypos = boxTop;

	if (m_CurrentPage)
	{
		char	str[256];
		if(m_ShowType == kShowTimeTypeFrame || m_ShowType == kShowTimeTypePeak)
		{
			formatf(str, sizeof(str), "%s - %s (and average)", m_CurrentPage->GetName(), sShowTypes[m_ShowType]);
		}
		else
		{
			formatf(str, sizeof(str), "%s - %s", m_CurrentPage->GetName(), sShowTypes[m_ShowType]);
		}
		grcColor(Color_black);
		grcDraw2dText(xpos+1.0f, ypos+1.0f, str);
		grcColor(kEKGTextFontColor);
		grcDraw2dText(xpos,ypos,str);
	}

	ypos += textVertSpace + 4;

	for (j=0; j<m_NumUsedLogs; j++)
	{
		// draw the name
		const int slot = m_LogOrder[j];
		if (slot != -1 && m_ActiveTimers[slot]!=NULL)
		{
            char	str[256];
			switch (m_ShowType)
			{
			case kShowCallsTypeFrame:
			case kShowCallsTypePeak:
				{
					int calls = 0;
					switch (m_ShowType)
					{
					case kShowCallsTypeFrame:
						// Make sure we use the value we stored in CollectSamples(), so that there is no
						// discrepancy between the numbers displayed and the graph being drawn. /FF
						calls = (int)(m_Logs[slot].m_Samples[(m_Logs[slot].m_SampleStart+m_HoverSlot)%pfEKGLog::MAX_SAMPLES]);
						break;
					case kShowCallsTypePeak:
						calls = m_ActiveTimers[m_LogOrder[j]]->GetPeakCalls();
						break;
					}
					sprintf(str, "%s  %d", m_ActiveTimers[m_LogOrder[j]]->GetName(), calls);
					break;
				}
			default:
				{
					float time = 0.0f;
					switch (m_ShowType)
					{
					case kShowTimeTypeFrame:
						// Make sure we use the value we stored in CollectSamples(), so that there is no
						// discrepancy between the numbers displayed and the graph being drawn. /FF
						time = m_Logs[slot].m_Samples[(m_Logs[slot].m_SampleStart+m_HoverSlot)%pfEKGLog::MAX_SAMPLES] * 1000.0f;
						break;
					case kShowTimeTypeAverage:
						time = m_ActiveTimers[m_LogOrder[j]]->GetAverageTime() * 1000.0f;
						break;
					case kShowTimeTypePeak:
						time = m_ActiveTimers[m_LogOrder[j]]->GetPeakTime() * 1000.0f;
						break;
					case kShowCallsTypeAverage:
						time = m_ActiveTimers[m_LogOrder[j]]->GetAverageCalls();
						break;
					}
					if(m_ShowType == kShowTimeTypeFrame || m_ShowType == kShowTimeTypePeak)
					{
						float timeAve = m_ActiveTimers[m_LogOrder[j]]->GetAverageTime() * 1000.0f;
						sprintf(str, "%s  %0.4f (%0.3f)", m_ActiveTimers[m_LogOrder[j]]->GetName(), time, timeAve);
					}
					else
					{
						sprintf(str, "%s  %f", m_ActiveTimers[m_LogOrder[j]]->GetName(), time);
					}
					break;
				}
			}
			grcColor(Color_black);
			grcDraw2dText(xpos+1.0f, ypos+1.0f, str);
            grcColor(kEKGTextFontColor);
            grcDraw2dText(xpos,ypos, str);
		}
		else if(m_ActiveValues[m_LogOrder[j]]!=NULL)
		{
		//	Assert(m_ActiveValues[m_LogOrder[j]]!=NULL);
			const char *name;
			s32 index;
			name = m_ActiveValues[m_LogOrder[j]]->GetName(index);

			// Make sure we use the value we stored in CollectSamples(), so that there is no
			// discrepancy between the numbers displayed and the graph being drawn. /FF
			float value = m_Logs[slot].m_Samples[(m_Logs[slot].m_SampleStart+m_HoverSlot) % pfEKGLog::MAX_SAMPLES];

			char formattedValue[128];
			if ( Abs( value - round(value) ) < 0.001f )
				prettyprinter(formattedValue, sizeof(formattedValue), (s64) value);
			else
				sprintf( formattedValue, "%f", value );

			if(index >= 0)
			//if(ActiveValues[LogOrder[j]]->NumValues() > 1)
			{
				char	str[256];
				formatf(str, sizeof(str), "%s_%d %s", name, index, formattedValue);
				grcColor(Color_black);
				grcDraw2dText(xpos+1.0f, ypos+1.0f, str);
				grcColor(kEKGTextFontColor);
				grcDraw2dText(xpos,ypos,str);
			}
			else
			{
				char	str[256];
				formatf(str, sizeof(str), "%s %s", name, formattedValue);
				grcColor(Color_black);
				grcDraw2dText(xpos+1.0f, ypos+1.0f, str);
				grcColor(kEKGTextFontColor);
				grcDraw2dText(xpos,ypos,str);
			}
		}

		ypos += textVertSpace;
	}


	/////////////////////////////////////////////////////////////////////////////////////
	// draw a scale

	float base = 10.0f;
	float tickWidth = 1.0e-6f;
	

	while (tickWidth * base < displayRange)
	{
		tickWidth *= base;
	}

	if (m_ValueMin || m_ValueMax || m_TimeMax) {
		
		int tickCount = 0;
		for (int tickNum = int(m_DisplayMin/tickWidth); tickNum * tickWidth <= displayRange; tickNum++, tickCount++)
		{
			char tempString[256];
			if(m_ValueMax || m_ValueMin)
			{
				if(displayRange < SMALL_FLOAT)
					formatf(tempString,256,"%2.4f",tickNum*tickWidth + 0.001f);
				else if(displayRange < 1.0f)
					formatf(tempString,256,"%2.2f",tickNum*tickWidth + 0.001f);
				else
					formatf(tempString,256,"%4.0f",tickNum*tickWidth + 0.001f);

			}
			else /* if(TimeMax) -- redundant due to test in outer scope */
			{
				// time units are us or ms
				// 0 = us, 1 = ms
				int timeUnits;
				timeUnits = (displayRange > 0.001f) ? 0 : 1;

				if (timeUnits==0)
				{
					static const char *fmts[3] = { "%4.0fms", "%3.0fKI$","%3.0fKD$" };
					formatf(tempString,256,fmts[pfTimer::GetMode()],tickNum*tickWidth*1e3f + 0.001f);
				}
				else
				{
					static const char *fmts[3] = { "%4.0fus", "%4.0fI$","%4.0fD$" };
					formatf(tempString,256,fmts[pfTimer::GetMode()],tickNum*tickWidth*1e6f + 0.001f);
				}
			}
			grcColor(kEKGTextFontColor);
			grcDraw2dText(graphLeft-5,(graphBottom-textVertSpace/2-(tickCount*tickWidth)*displayScale*graphHeight),tempString);
		}
	}
	else
	{
		grcColor(kEKGTextFontColor);
		grcDraw2dText(graphLeft-5,(graphBottom-textVertSpace/2),"No data");
	}

	g_TallyGrcCalls = oldTally;
}

#endif	// __STATS

