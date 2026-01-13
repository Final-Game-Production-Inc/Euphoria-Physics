//
// grcore/monitor.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//


#include "channel.h"
#include "monitor.h"
#include "device.h"
#include "adapter.h"

#ifndef ATI_SUPPORT
#define ATI_SUPPORT (RSG_PC && !__RESOURCECOMPILER && !__TOOL && !__GAMETOOL)
#endif
#ifndef NV_SUPPORT
#define NV_SUPPORT (RSG_PC && !__RESOURCECOMPILER && !__TOOL && !__GAMETOOL)
#endif

#if ATI_SUPPORT
#include "../../3rdParty/AMD/ADL_SDK_7.0/include/adl_sdk.h"
#include "grcore/d3dwrapper.h"
#endif

#if NV_SUPPORT
#include "../../3rdParty/NVidia/nvapi.h"
#if RSG_CPU_X64
#pragma comment(lib,"nvapi64.lib")
#elif RSG_CPU_X86
#pragma comment(lib,"nvapi.lib")
#endif
#endif

#if __BANK
#include "bank/bkmgr.h"
#endif


namespace rage	{

static bool sb_RequestedUpdate = false;
PARAM(aspectratio,"Force a specific aspect ratio in format A:B");
PARAM(GridX, "Number of monitors wide you want to emulate");
PARAM(GridY, "Number of monitors high you want to emulate");
PARAM(HUD_LAYOUT, "HUD Layout X,Y,Width,Height");

#if SUPPORT_MULTI_MONITOR
	static bool sb_EnableMultihead = true;
#	if ATI_SUPPORT
		static bool sb_UseADLQueries = true;
#	endif
	static bool sb_ForcePortrait = false;
	static u32 si_RequestedGridSizeX = 1, si_RequestedGridSizeY = 1;
	static int s_aiHudLayout[2] = { 0,0};
#endif	//SUPPORT_MULTI_MONITOR

//	Small GridMonitor implementation	//

fwRect GridMonitor::getArea() const	{
	const float kW = 1.f / GRCDEVICE.GetGlobalWindow().uWidth;
	const float kH = 1.f / GRCDEVICE.GetGlobalWindow().uHeight;
	return fwRect( kW*uLeft, kH*uTop, kW*uRight, kH*uBottom );
}


//------------------------------------------------------------------------------------------------//
//	Monitor configuration
//------------------------------------------------------------------------------------------------//

#if SUPPORT_MULTI_MONITOR
int MonitorConfiguration::findModeIdHelper(int startId, int total, const Mode &mode) const	{
	for(int iMode=startId; m_Modes && iMode<startId+total; ++iMode)
	{
		 if (m_Modes[iMode] == mode)
			 return iMode;
	}
	return -1;
}

const GridMonitor* MonitorConfiguration::findPreferredMonitor() const	{
	if (isMultihead() && (m_bEmulated || isNativeResolution()))	{
		const unsigned targetX = m_uGridSizeX/2, targetY = m_uGridSizeY/2;

		for (unsigned i=0; i!=getMonitorCount(); ++i)	{
			if (m_Monitors[i].uGridPosX == targetX &&
				m_Monitors[i].uGridPosY == targetY)
				return m_Monitors+i;
		}

		grcErrorf("[monitor] Unable to find the target monitor at (%d, %d)",
			targetX, targetY);
	}

	return NULL;
}
#endif	//SUPPORT_MULTI_MONITOR

MonitorConfiguration::MonitorConfiguration()
	: m_fForcedGlobalAspect(0.f)
	, m_bDirty(true)
#	if SUPPORT_MULTI_MONITOR
	, m_bEmulated(false), m_bBezelCompensated(false)
	, m_Composition(MC_FLAT), m_Modes(NULL)
	, m_TotalNativeModes(-1), m_TotalBezelModes(-1)
	, m_bMerged(false), m_uGridSizeX(0), m_uGridSizeY()
	, m_Monitors(NULL), m_Preferred(NULL)
	, m_ChangeTimestamp(0)
#	endif	//SUPPORT_MULTI_MONITOR
{
}


MonitorConfiguration::~MonitorConfiguration()	{
	MULTI_MONITOR_ONLY( releaseMonitors() );
}

#if SUPPORT_MULTI_MONITOR
void MonitorConfiguration::allocateMonitors()	{
	const int count = getMonitorCount();
	Assert( !m_Monitors );
	m_Monitors = rage_new GridMonitor[count];
	grcDisplayf("[monitor] Allocating %d monitors", count);
}

void MonitorConfiguration::releaseMonitors()	{
	if (m_Preferred)
	{
		grcDisplayf("[monitor] Old preferred display [%d, %d] - [%d, %d]",
			m_Preferred->uLeft, m_Preferred->uTop, m_Preferred->uBottom, m_Preferred->uRight);
	}
	m_Preferred = NULL;
	// clean current monitors
	if (m_Monitors)
	{
		grcDisplayf("[monitor] Releasing monitors for some reason");
		delete[] m_Monitors;
		m_Monitors = NULL;
	}
	// clean remembered modes
	if (m_Modes)
		delete[] m_Modes;
	m_Modes = NULL; 
	m_TotalNativeModes = m_TotalBezelModes = 0;
	// reset settings
	m_bEmulated = m_bMerged = false;
	m_bDirty = true; 
	m_uGridSizeX = m_uGridSizeY = 0;
}
#endif	//SUPPORT_MULTI_MONITOR


float MonitorConfiguration::readAspectParameter()	{
	const char* strAspect = NULL; 
	if (!PARAM_aspectratio.Get(strAspect))
		return 0.f;
	
	int numerator = 0, denumerator = 0;
	sscanf(strAspect, "%d:%d", &numerator, &denumerator);
	if (numerator>0 && denumerator>0)
	{
		return numerator / (float)denumerator;
	}
	Errorf("[monitor] Incorrect format of -aspectratio");
	return 0.f;
}

void MonitorConfiguration::setGlobalAspect(float fAspect)	{
	Assert( fAspect >= 0.f );
	m_fForcedGlobalAspect = fAspect;
	markDirty();
}

#if SUPPORT_MULTI_MONITOR
void MonitorConfiguration::emulateGrid(unsigned gridX, unsigned gridY)	{
	grcDisplayf("[monitor] Emulating %d x %d grid", gridX, gridY);
	m_bMerged = m_bEmulated = true;
	m_uGridSizeX = gridX; m_uGridSizeY = gridY;
	Assert(gridX && gridY);
	const int sx = m_WholeScreen.uRight	 / gridX;
	const int sy = m_WholeScreen.uBottom / gridY;
	const float aspect = gridY * m_WholeScreen.fPhysicalAspect / gridX;
	allocateMonitors();
	for(unsigned i=0; i<getMonitorCount(); ++i)
	{
		GridMonitor &mon = m_Monitors[i];
		mon.bLandscape = !sb_ForcePortrait;
		mon.uGridPosX = i % m_uGridSizeX;
		mon.uGridPosY = i / m_uGridSizeX;
		mon.uLeft	= sx * mon.uGridPosX;
		mon.uRight	= sx + mon.uLeft;
		mon.uTop	= sy * mon.uGridPosY;
		mon.uBottom	= sy + mon.uTop;
		mon.fPhysicalAspect = aspect;
	}
}

void MonitorConfiguration::setHUDLayout(unsigned int uX, unsigned int uY)
{
	m_bMerged = m_bEmulated = true;
	const float aspect = m_WholeScreen.fPhysicalAspect;
	unsigned int uWidth = m_WholeScreen.uRight - 2 * uX;
	unsigned int uHeight = m_WholeScreen.uBottom - 2 * uY;
	m_uGridSizeX = m_uGridSizeY = 1;
	allocateMonitors();

	for(unsigned i=0; i<getMonitorCount(); ++i)
	{
		GridMonitor &mon = m_Monitors[i];
		mon.bLandscape = !sb_ForcePortrait;
		mon.uGridPosX = 0;
		mon.uGridPosY = 0;
		mon.uLeft	= uX;
		mon.uRight	= uX + uWidth;
		mon.uTop	= uY;
		mon.uBottom	= uY + uHeight;
		mon.fPhysicalAspect = aspect;
	}
}

#endif	//SUPPORT_MULTI_MONITOR

const MonitorConfiguration& MonitorConfiguration::update()	{
	if (!sb_RequestedUpdate && !m_bDirty)
		return *this;
	MULTI_MONITOR_ONLY( ++m_ChangeTimestamp );

#if SUPPORT_MULTI_MONITOR
	static bool bOnce = true;
	if (bOnce)
	{
		PARAM_GridX.Get(si_RequestedGridSizeX);
		PARAM_GridY.Get(si_RequestedGridSizeY);
		PARAM_HUD_LAYOUT.GetArray(s_aiHudLayout, 2);
		bOnce = false;
	}
#endif // SUPPORT_MULTI_MONITOR

	// update whole screen:
	// doing this only now because the current window
	// may be inaccessible during queryConfiguration() call
	const grcDisplayWindow &win = GRCDEVICE.GetGlobalWindow();
	m_WholeScreen.uLeft = m_WholeScreen.uTop = 0;
	m_WholeScreen.uRight	= win.uWidth;
	m_WholeScreen.uBottom	= win.uHeight;
	// get real aspect
#	if __WIN32PC
	float fAspect = m_WholeScreen.getLogicalAspect();
//	 = fAspect;

#if SUPPORT_MULTI_MONITOR && !__RESOURCECOMPILER
	const grcAdapter* pAdapter = grcAdapterManager::GetInstance()->GetAdapter(GRCDEVICE.GetAdapterOrdinal());
	grcDisplayWindow oMaxResolution;
	const u32 uLargestModeIndex = pAdapter->GetLargestModeIndex(GRCDEVICE.GetOutputMonitor());
	AssertVerify( pAdapter->GetMode(&oMaxResolution, uLargestModeIndex, GRCDEVICE.GetOutputMonitor()) );
	const float fMaxAspect = (float)oMaxResolution.uWidth / oMaxResolution.uHeight;
	grcDisplayf("[monitor] Largest mode aspect: %f", fMaxAspect);
	m_WholeScreen.fActualPhysicalAspect = fMaxAspect;
#endif // SUPPORT_MULTI_MONITOR


	if (m_fForcedGlobalAspect > 0.f)	
	{
		fAspect = m_fForcedGlobalAspect;
	} 
	else if (!GRCDEVICE.IsWindowed())
	{
#if SUPPORT_MULTI_MONITOR && !__RESOURCECOMPILER
		// multi-head mode may be in effect while we are running in a regular full-screen resolution
		if (m_WholeScreen.fActualPhysicalAspect < 2.f*fAspect)
			fAspect = m_WholeScreen.fActualPhysicalAspect;
#endif // SUPPORT_MULTI_MONITOR
	}
#	else
	float fAspect = grcDevice::GetWideScreen() ? 16.f/9.f : 4.f/3.f;
#	endif
	m_WholeScreen.fPhysicalAspect = fAspect;
#	if SUPPORT_MULTI_MONITOR
	// clean up existing emulation
	if (m_bEmulated)
		releaseMonitors();
	// check user-forced emulation
	if (si_RequestedGridSizeX>1 || si_RequestedGridSizeY>1)	{
		releaseMonitors();
		emulateGrid( si_RequestedGridSizeX, si_RequestedGridSizeY );
	} 
	else if (s_aiHudLayout[0] != 0)
	{
		setHUDLayout(s_aiHudLayout[0], s_aiHudLayout[1]);
	}
	else if (fAspect > 3.5f && (!isMultihead() || !isNativeResolution()) && !(m_bMerged && m_uGridSizeX == 2))
	{
		// ideally, we should preserve the original monitor configuration we got from AMD/Nvidia
		// there is a lot of other stuff we'd need to do, ideally
		releaseMonitors();
		emulateGrid( 3, 1 );
	}

	// search for the preferred monitor
	m_Preferred = findPreferredMonitor();
	if (m_Preferred)
	{
		grcDisplayf("[monitor] New preferred display [%d, %d] - [%d, %d]",
			m_Preferred->uLeft, m_Preferred->uTop, m_Preferred->uBottom, m_Preferred->uRight);
		// adjust the prefered monitor aspect, based on the global one
		m_Monitors[m_Preferred-m_Monitors].fPhysicalAspect = m_uGridSizeY * fAspect / m_uGridSizeX;
	}
#	endif	//SUPPORT_MULTI_MONITOR
	m_bDirty = sb_RequestedUpdate = false;
	return *this;
}

#if SUPPORT_MULTI_MONITOR
bool MonitorConfiguration::isMultihead() const	{
	return sb_EnableMultihead && 
		m_bMerged &&
		m_uGridSizeX > 2 &&
		m_Monitors
		WIN32PC_ONLY(&& (m_bEmulated || !GRCDEVICE.IsWindowed()));
}

bool MonitorConfiguration::isNativeResolution() const {
	unsigned width = 0, height = 0;
	for (unsigned i=0; i<getMonitorCount(); ++i)
	{
		width = Max(width, m_Monitors[i].uRight);
		height = Max(height, m_Monitors[i].uBottom);
	}
	grcDisplayf("[monitor] Detected monitor coverage of %d x %d", width, height);
	return width <= m_WholeScreen.uRight && height <= m_WholeScreen.uBottom;
}

const GridMonitor* MonitorConfiguration::getMonitorPtr(int id) const	{
	if (id<0)
		return &m_WholeScreen;
	return m_Monitors && (unsigned)id < getMonitorCount() ?
		m_Monitors+id : NULL;
}

const GridMonitor& MonitorConfiguration::getCentralMonitor() const	{
	return m_Preferred ? *m_Preferred : m_WholeScreen;
}

const GridMonitor& MonitorConfiguration::getLandscapeMonitor() const	{
	return m_Preferred && m_Preferred->bLandscape ? *m_Preferred : m_WholeScreen;
}

bool MonitorConfiguration::queryConfiguration()	{
	releaseMonitors();

#if RSG_PC && !__TOOL && !__RESOURCECOMPILER
	const int iTargetAdapter = GRCDEVICE.GetAdapterOrdinal();
	
	switch (GRCDEVICE.GetManufacturer())
	{
	case ATI:
		queryConfigAMD(iTargetAdapter);
		break;
	case NVIDIA:
		queryConfigNvidia(iTargetAdapter);
		break;
	default:;
	}

	for (unsigned i=0; m_Monitors && i < getMonitorCount(); ++i)
	{
		grcDisplayf("[monitor]: detected m[%d] at (%d, %d) be [%d, %d] - [%d, %d], %s", i,
			m_Monitors[i].uGridPosX, m_Monitors[i].uGridPosY,
			m_Monitors[i].uLeft, m_Monitors[i].uTop,
			m_Monitors[i].uRight, m_Monitors[i].uBottom,
			m_Monitors[i].bLandscape ? "landscape" : "portrait");
	}
#endif	//RSG_PC
	return m_bMerged;
}
#endif	//SUPPORT_MULTI_MONITOR


//------------------------------------------------------------------------------------------------//
//	Hardware queries
//------------------------------------------------------------------------------------------------//

#if ATI_SUPPORT
MonitorConfiguration::Mode::Mode(const ADLMode& am)	{
	const bool landscape = !(am.iOrientation % 180);
	width	= landscape ? am.iXRes : am.iYRes;
	height	= landscape ? am.iYRes : am.iXRes;
}
#endif


#if ATI_SUPPORT
// A helper macros to declare a constant pointer to the ADL function with the same name
#define GENERIC_FUNCTOR(Dll,Ret,name,...)	\
	Ret(*const name)(__VA_ARGS__) = (Ret(*)(__VA_ARGS__)) GetProcAddress(Dll,#name)
#define FUNCTOR(name,...)	GENERIC_FUNCTOR(dh.get(),int,name,__VA_ARGS__)

namespace adl {
	class DllHandler	{
		DllHandler(const DllHandler&);
		void operator=(const DllHandler&);
		HINSTANCE handle;
		void free() const	{
			if (valid())
				FreeLibrary(handle);
		}
	public:
		DllHandler(const char name[]): handle(LoadLibrary(name))	{}
		~DllHandler()	{ free(); }
		void load(const char name[])	{
			free();
			handle = LoadLibrary(name);
		}
		HINSTANCE get() const	{ return handle; }
		bool valid() const	{ return handle != NULL; }
	};


	void* __stdcall Main_Memory_Alloc ( int iSize )	{
		void* lpBuffer = malloc ( iSize );
		return lpBuffer;
	}

	void __stdcall Main_Memory_Free ( void** lpBuffer )	{
		if ( NULL != *lpBuffer )	{
			free ( *lpBuffer );
			*lpBuffer = NULL;
		}
	}
}
#endif	//ATI_SUPPORT

#if SUPPORT_MULTI_MONITOR
bool MonitorConfiguration::queryConfigAMD(int iTargetAdapter)	{
#	if ATI_SUPPORT
	if (!sb_UseADLQueries)
		return false;
	
	int  iNumberAdapters;
	LPAdapterInfo     lpAdapterInfo = NULL;
	LPADLDisplayInfo  lpAdlDisplayInfo = NULL;
	int  iPrimaryAdapterIndex;
	
	adl::DllHandler dh("atiadlxx.dll");
	if (!dh.valid())
	{
		// A 32 bit calling application on 64 bit OS will fail to LoadLibrary.
		// Try to load the 32 bit library (atiadlxy.dll) instead
		dh.load("atiadlxy.dll");
	}
	if (!dh.valid())
	{
		Warningf("ADL: Failed to load the library!");
		return false;
	}

	FUNCTOR( ADL_Main_Control_Create, ADL_MAIN_MALLOC_CALLBACK, int );
	FUNCTOR( ADL_Main_Control_Destroy );
	FUNCTOR( ADL_Adapter_NumberOfAdapters_Get, int* );
	FUNCTOR( ADL_Adapter_AdapterInfo_Get, LPAdapterInfo, int );
	FUNCTOR( ADL_Adapter_Primary_Get, int* );
	FUNCTOR( ADL_Display_DisplayInfo_Get, int, int*, ADLDisplayInfo**, int );
	FUNCTOR( ADL_Display_SLSMapIndex_Get, int, int, ADLDisplayTarget*, int* );
	FUNCTOR( ADL_Display_SLSMapConfig_Get, int, int, ADLSLSMap*, int*, ADLSLSTarget**, int*, ADLSLSMode**,
		int*, ADLBezelTransientMode**, int*, ADLBezelTransientMode**, int*, ADLSLSOffset**, int );
	FUNCTOR( ADL_Display_Modes_Get, int, int, int*, ADLMode** );
	FUNCTOR( ADL_Display_DisplayMapConfig_Get, int, int*, ADLDisplayMap**, int*, ADLDisplayTarget**, int );

	if (NULL == ADL_Main_Control_Create				||
		NULL == ADL_Main_Control_Destroy			||
		NULL == ADL_Adapter_NumberOfAdapters_Get	||
		NULL == ADL_Adapter_AdapterInfo_Get			||
		NULL == ADL_Adapter_Primary_Get				||
		NULL == ADL_Display_DisplayInfo_Get			||
		NULL == ADL_Display_SLSMapIndex_Get			||
		NULL == ADL_Display_SLSMapConfig_Get		||
		NULL == ADL_Display_Modes_Get				||
		NULL == ADL_Display_DisplayMapConfig_Get)
	{
		Errorf("ADL: API is missing!");
		return false;
	}

	int aCode = 0;

	// Initialize ADL. The second parameter is 1, which means:
	// retrieve adapter information only for adapters that are physically present and enabled in the system
	aCode = ADL_Main_Control_Create (adl::Main_Memory_Alloc, 1);
	if ( aCode != ADL_OK )
	{
		Errorf("ADL: Initialization Error! (code %d)", aCode);
		return false;
	}

	// Obtain the number of adapters for the system
	aCode = ADL_Adapter_NumberOfAdapters_Get ( &iNumberAdapters );
	if ( aCode != ADL_OK )
	{
		ADL_Main_Control_Destroy();
		Errorf("ADL: Cannot get the number of adapters! (code %d)", aCode);
		return false;
	}

	aCode = ADL_Adapter_Primary_Get (&iPrimaryAdapterIndex);
	if ( aCode != ADL_OK )
	{
		ADL_Main_Control_Destroy();
		Errorf("ADL: Cannot get primary adapter! (code %d)", aCode);
		return false;
	}

    if ( 0 < iNumberAdapters )
    {
		lpAdapterInfo = (LPAdapterInfo) malloc ( sizeof (AdapterInfo) * iNumberAdapters );
        memset ( lpAdapterInfo,'\0', sizeof (AdapterInfo) * iNumberAdapters );

        // Get the AdapterInfo structure for all adapters in the system
        ADL_Adapter_AdapterInfo_Get (lpAdapterInfo, sizeof (AdapterInfo) * iNumberAdapters);
    }

	// Repeat for all available adapters in the system
	for (int i = 0; i < iNumberAdapters; i++ )
	{
		const int iLocalAdapterIndex = lpAdapterInfo[ i ].iAdapterIndex;
		if (!lpAdapterInfo[i].iPresent || lpAdapterInfo[i].iOSDisplayIndex != iTargetAdapter)
			continue;

		// here goes Eyefinity section
		ADLSLSMap SLSMap = {0};
		int iNumDisplayTarget = 0;
		ADLDisplayTarget *lpDisplayTarget = NULL;
		int iNumDisplayMap = 0;
		ADLDisplayMap *lpDisplayMap = NULL;
		int iSLSMapIndex = 0;
		int iNumSLSTarget = 0;
		ADLSLSTarget *lpSLSTarget = NULL;
		ADLSLSMode *lpNativeMode = NULL;
		ADLBezelTransientMode *lpBezelMode = NULL;
		int iNumTransientMode = 0;
		ADLBezelTransientMode *lpTransientMode = NULL;
		int iNumSLSOffset = 0;
		ADLSLSOffset *lpSLSOffset = NULL;
		ADLMode *lpModes = NULL, savedMode = {0};
		int iCurDisplay = 0;
		ADLMode *lpPossibleModes = NULL;

		// Get the list of display targets associated with this adapater
		aCode = ADL_Display_DisplayMapConfig_Get( iLocalAdapterIndex,
			&iNumDisplayMap, &lpDisplayMap, &iNumDisplayTarget, &lpDisplayTarget, 
			ADL_DISPLAY_DISPLAYMAP_OPTION_GPUINFO );
		if (aCode != ADL_OK)
		{
			if (aCode == ADL_ERR)
				Displayf("ADL: AMD Eyefinity not active");
			else
				Errorf("ADL: unable to get display map configuration! (code %d)", aCode);
			continue;
		}

		// Get the Eyefinity/SLS display map index
		aCode = ADL_Display_SLSMapIndex_Get( iLocalAdapterIndex,
			iNumDisplayTarget, lpDisplayTarget, &iSLSMapIndex);
		if (aCode != ADL_OK)
		{
			if (aCode == ADL_ERR)
				Displayf("ADL: AMD Eyefinity not active");
			else
				Errorf("ADL: unable to get SLS map index! (code %d)", aCode);
			continue;
		}

		if ( iNumDisplayTarget < 2 || iSLSMapIndex < 0 )	{
			continue;
		}

		// Get the list of modes supported by the current Eyefinity/SLS index:
		// for now, we only care about native and bezel-compensated modes
		aCode = ADL_Display_SLSMapConfig_Get ( iLocalAdapterIndex, iSLSMapIndex, &SLSMap,
			&iNumSLSTarget, &lpSLSTarget, &m_TotalNativeModes, &lpNativeMode,
			&m_TotalBezelModes, &lpBezelMode, &iNumTransientMode, &lpTransientMode,
			&iNumSLSOffset, &lpSLSOffset, ADL_DISPLAY_SLSGRID_CAP_OPTION_RELATIVETO_LANDSCAPE );
		if ( aCode != ADL_OK )
		{
			Errorf("ADL: unable to get SLS map configuration! (code %d)", aCode);
			continue;
		}

		if (m_TotalNativeModes + m_TotalBezelModes > 0)
		{
			m_Modes = rage_new Mode[m_TotalNativeModes + m_TotalBezelModes];
			int iMode;
			// remember native eyefinity modes
			for ( iMode=0; iMode<m_TotalNativeModes; ++iMode)
			{
				m_Modes[iMode] = Mode( lpNativeMode[iMode].displayMode );
			}
			// remember bezel-compensated modes
			for ( iMode=0; iMode<m_TotalBezelModes; ++iMode)
			{
				m_Modes[m_TotalNativeModes+iMode] = Mode( lpBezelMode[iMode].displayMode );
			}
		}

		if ( iNumDisplayTarget != (SLSMap.grid.iSLSGridColumn * SLSMap.grid.iSLSGridRow) )
		{
			Errorf("ADL: Number of display targets returned is not equal to the SLS grid size! (code %d)", aCode);
			continue;
		}

		bool landscape = !(lpNativeMode->displayMode.iOrientation % 180);

		if (landscape)
		{
			m_uGridSizeX = SLSMap.grid.iSLSGridColumn;
			m_uGridSizeY = SLSMap.grid.iSLSGridRow;
		}
		else
		{
			m_uGridSizeX = SLSMap.grid.iSLSGridRow;
			m_uGridSizeY = SLSMap.grid.iSLSGridColumn;
		}
		allocateMonitors();
		if (!m_Monitors)
			continue;

		for (; iCurDisplay<iNumDisplayTarget; ++iCurDisplay)
		{
			const int iDisplayLogicalIndex = lpDisplayTarget[iCurDisplay].displayID.iDisplayLogicalIndex;
			int iNumModes = 0;

			adl::Main_Memory_Free( (void**) &lpModes );
			aCode = ADL_Display_Modes_Get( iLocalAdapterIndex, iDisplayLogicalIndex, &iNumModes, &lpModes );
			if ( aCode != ADL_OK )
			{
				Errorf("ADL: unable to query monitor (%d) modes! (code %d)", iCurDisplay, aCode);
				continue;
			}

			if (lpModes->displayID.iDisplayLogicalIndex != iDisplayLogicalIndex)
			{
				if (iCurDisplay)
				{
					Warningf("ADL: went crazy, but we have a saved mode, fortunately.");
					*lpModes = savedMode;	// Note: we have to copy
				}else
				{
					Errorf("ADL: went crazy, and there is no saved mode. Aborting...");
					break;
				}
			}else
			{
				savedMode = *lpModes;
			}
			
			// target monitor information
			GridMonitor& mon = m_Monitors[iCurDisplay];
			mon.bLandscape = !(lpModes->iOrientation % 180);

			// compensating current mode rotation
			const Mode curMode(*lpModes);
			const int iCurNativeMode = findNativeModeId	(curMode);
			const int iCurBezelMode = findBezelModeId	(curMode) - m_TotalNativeModes;

			const bool bEyefinity = (iCurNativeMode>=0) || (iCurBezelMode>=0);
			Assert(!iCurDisplay || m_bMerged == bEyefinity);
			m_bMerged = bEyefinity;

			if (!bEyefinity)
			{
				releaseMonitors();
				break;
			}

			int iCurTarget = 0;
			for (; iCurTarget<iNumSLSTarget; ++iCurTarget)
			{
				if (lpSLSTarget[iCurTarget].displayTarget.displayID.iDisplayLogicalIndex == iDisplayLogicalIndex)
					break;
			}

			if (iCurTarget==iNumSLSTarget)
			{
				Errorf("ADL: SL target not found for display (%d)!", iCurDisplay);
				continue;
			}

			const ADLSLSTarget& target = lpSLSTarget[iCurTarget];
			if (landscape)
			{
				mon.uGridPosX = target.iSLSGridPositionX;
				mon.uGridPosY = target.iSLSGridPositionY;
			}else
			{
				mon.uGridPosX = target.iSLSGridPositionY;
				mon.uGridPosY = target.iSLSGridPositionX;
			}

			const bool bCompensated = (iCurBezelMode>=0);
			Assert(!iCurDisplay || m_bBezelCompensated == bCompensated);
			m_bBezelCompensated = bCompensated;

			if (!bCompensated)
			{
				const int width		= curMode.width / m_uGridSizeX;
				const int height	= curMode.height/ m_uGridSizeY;
				mon.uLeft	= mon.uGridPosX * width;
				mon.uTop	= mon.uGridPosY * height;
				mon.uRight	= mon.uLeft + width;
				mon.uBottom	= mon.uTop + height;
			}else
			{	// extract visible area of bezel compensation
				int iCurOffset = 0;
				for (; iCurOffset<iNumSLSOffset; ++iCurOffset)
				{
					if (lpBezelMode[iCurBezelMode].iSLSModeIndex == lpSLSOffset[iCurOffset].iBezelModeIndex &&
						lpSLSOffset[iCurOffset].displayID.iDisplayLogicalIndex == iDisplayLogicalIndex)
						break;
				}

				if (iCurOffset==iNumSLSOffset)
				{
					Errorf("ADL: SL offset not found for display (%d)!", iCurDisplay);
					continue;
				}

				const ADLSLSOffset& offset = lpSLSOffset[iCurOffset];
				if (landscape)
				{
					mon.uLeft	= offset.iBezelOffsetX;
					mon.uTop	= offset.iBezelOffsetY;
					mon.uRight	= mon.uLeft	+ offset.iDisplayWidth;
					mon.uBottom	= mon.uTop	+ offset.iDisplayHeight;
				}else
				{
					mon.uLeft	= offset.iBezelOffsetY;
					mon.uTop	= offset.iBezelOffsetX;
					mon.uRight	= mon.uLeft	+ offset.iDisplayHeight;
					mon.uBottom	= mon.uTop	+ offset.iDisplayWidth;
				}
			}

			mon.fPhysicalAspect = mon.getLogicalAspect();
		}
		adl::Main_Memory_Free ( (void**) &lpModes );
		adl::Main_Memory_Free ( (void**) &lpPossibleModes );
		adl::Main_Memory_Free ( (void**) &lpSLSTarget );
		adl::Main_Memory_Free ( (void**) &lpNativeMode );
		adl::Main_Memory_Free ( (void**) &lpBezelMode );
		adl::Main_Memory_Free ( (void**) &lpTransientMode );
		adl::Main_Memory_Free ( (void**) &lpSLSOffset );
		adl::Main_Memory_Free ( (void**) &lpDisplayMap );
		adl::Main_Memory_Free ( (void**) &lpDisplayTarget );
	}

	adl::Main_Memory_Free ((void**) &lpAdapterInfo );
	adl::Main_Memory_Free ((void**) &lpAdlDisplayInfo );
	ADL_Main_Control_Destroy ();
	return true;
#	else
	(void)iTargetAdapter;
	return false;
#	endif	//	ATI_SUPPORT
}


bool MonitorConfiguration::queryConfigNvidia(int)	{
#if NV_SUPPORT
	NvAPI_Status ret;
	NvU32 uDisplayId = 0;
	ret = NvAPI_DISP_GetGDIPrimaryDisplayId( &uDisplayId );
	if (ret != NVAPI_OK)	{
		if (ret == NVAPI_NVIDIA_DEVICE_NOT_FOUND || ret == NVAPI_LIBRARY_NOT_FOUND)
			Displayf("NvAPI: no NV video detected");
		else
			Warningf("NvAPI: primary display query failed, code %d", ret);
		return false;
	}

	// old (deprecated) way
	if (0)	{
		NV_MOSAIC_GRID_TOPO topology = { NV_MOSAIC_GRID_TOPO_VER };
		NvU32 count = 1;
		ret = NvAPI_Mosaic_EnumDisplayGrids(&topology, &count);
		if (ret == NVAPI_OK && count == 1)	{
			m_uGridSizeX = topology.columns;
			m_uGridSizeY = topology.rows;
			m_bBezelCompensated = topology.applyWithBezelCorrect;
			m_bMerged = m_uGridSizeX > 1 || m_uGridSizeY > 1;

			if (!m_bMerged)
				return false;
			allocateMonitors();
			Displayf("NvAPI: surround mode detected in configuration %dx%d", m_uGridSizeX, m_uGridSizeY);
			const float aspect = (float)topology.displaySettings.width / (float)topology.displaySettings.height;
			
			for(unsigned y=0; y<m_uGridSizeY; ++y)	{
				for (unsigned x=0; x<m_uGridSizeX; ++x)	{
					NV_MOSAIC_GRID_TOPO_DISPLAY *const disp = topology.displays + y*m_uGridSizeY + x;
					GridMonitor *const mon = m_Monitors + y*m_uGridSizeY + x;
					mon->uGridPosX = x;
					mon->uGridPosY = y;
					mon->fPhysicalAspect = aspect;
					mon->bLandscape = disp->rotation == NV_ROTATE_0 || disp->rotation == NV_ROTATE_180;
					mon->uLeft = (x ? mon[-1].uRight : 0);
					mon->uTop = (y ? mon[-(int)m_uGridSizeY].uBottom : 0);
					if (mon->bLandscape)	{
						mon->uLeft -= disp->overlapX;
						mon->uTop -= disp->overlapY;
						mon->uRight = mon->uLeft + topology.displaySettings.width;
						mon->uBottom = mon->uTop + topology.displaySettings.height;
					}else {
						mon->uLeft -= disp->overlapY;
						mon->uTop -= disp->overlapX;
						mon->uRight = mon->uLeft + topology.displaySettings.height;
						mon->uBottom = mon->uTop + topology.displaySettings.width;
					}
				}
			}
			return true;
		}else {
			Warningf("NvAPI: failed to enumerate displays, code %d, count %d", ret, count);
		}
	}

	// new (swag) way of querying Mosaic configuration
	if (1) {
		NV_RECT viewports[NV_MOSAIC_MAX_DISPLAYS];
		NvU8 bBezelCorrected = 0;
		ret = NvAPI_Mosaic_GetDisplayViewportsByResolution( uDisplayId, 0, 0, viewports, &bBezelCorrected );
		if (ret != NVAPI_OK)	{
			if (ret == NVAPI_NO_ACTIVE_SLI_TOPOLOGY || ret == NVAPI_MOSAIC_NOT_ACTIVE)
				Displayf("NvAPI: surround mode is not enabled");
			else
				Warningf("NvAPI: display viewports query failed, code %d", ret);
			return false;
		}
	
		m_uGridSizeX = 0;
		m_uGridSizeY = 1;
		const NV_RECT *pRect = NULL;

		for (pRect=viewports; m_uGridSizeX<NV_MOSAIC_MAX_DISPLAYS && pRect->top != pRect->bottom; ++m_uGridSizeX,++pRect)	{
			if (pRect->top != viewports->top || pRect->bottom != viewports->bottom)
				m_uGridSizeY = 2;
		}

		m_uGridSizeX /= m_uGridSizeY;
		Assert( pRect == viewports + getMonitorCount() );
		m_bBezelCompensated = bBezelCorrected!=0;
		m_bMerged = m_uGridSizeX>1 || m_uGridSizeY>1;
		if (!m_bMerged)
			return false;

		allocateMonitors();
		Displayf("NvAPI: surround mode detected in configuration %dx%d", m_uGridSizeX, m_uGridSizeY);

		for (unsigned i=0; i<getMonitorCount(); ++i)	{
			GridMonitor &mon = m_Monitors[i];
			mon.uGridPosX = i % m_uGridSizeX;
			mon.uGridPosY = i / m_uGridSizeX;
			mon.uLeft	= viewports[i].left;
			mon.uRight	= viewports[i].right +1;
			mon.uTop	= viewports[i].top;
			mon.uBottom	= viewports[i].bottom +1;
			mon.fPhysicalAspect = mon.getLogicalAspect();
			mon.bLandscape = mon.fPhysicalAspect > 1.f;
		}
		return true;
	}
#else
	return false;
#endif	//NV_SUPPORT
}


//------------------------------------------------------------------------------------------------//
//	FOV helpers
//------------------------------------------------------------------------------------------------//

static const float s_fDegreesToRadians = 3.14159265258f / 180.f;

float MonitorConfiguration::computeVerticalFOV(float fovX) const	{
	const GridMonitor& mon = getCentralMonitor();
	const float oldTan = tanf( 0.5f * fovX * s_fDegreesToRadians );
	const float newTan = oldTan / mon.fPhysicalAspect;
	return 2.f * atanf(newTan) / s_fDegreesToRadians;
}

float MonitorConfiguration::transformFOV(float fovY) const	{
	if(!isMultihead())
		return fovY;
	const fwRect rect = getCentralMonitor().getArea();
	float cx=0.f, cy=0.f;
	rect.GetCentre(cx,cy);
	const float kY = 1.f / rect.GetHeight();

	if(m_Composition == MC_FLAT)	{
		// extending the tangent space
		const float oldTan = tanf( 0.5f * fovY * s_fDegreesToRadians );
		const float k1 = oldTan * 2.f * (1.f-cy) * kY;
		const float k2 = oldTan * 2.f * (0.f+cy) * kY;
		return (atanf(k1) + atanf(k2)) / s_fDegreesToRadians;
	}else
	if (m_Composition == MC_ROUND)	{
		// extending the angle space
		return fovY * kY;
	}else	{
		return fovY;
	}
}
#endif	//SUPPORT_MULTI_MONITOR

//------------------------------------------------------------------------------------------------//
//	Bank stuff
//------------------------------------------------------------------------------------------------//
#if __BANK
#if SUPPORT_MULTI_MONITOR
static void requestUpdate()	{
	sb_RequestedUpdate = true;
}

void MonitorConfiguration::addWidgets(bkBank &bank)	{
	const datCallback updateCB(requestUpdate);
	bank.PushGroup( "Emulation", false );
	bank.AddSlider( "Horisontal",	&si_RequestedGridSizeX, 1,4,1,	updateCB );
	bank.AddSlider( "Vertical",		&si_RequestedGridSizeY, 1,4,1,	updateCB );
	bank.AddToggle( "Portrait",		&sb_ForcePortrait,				updateCB );

	bank.AddSlider( "HUD X Offset",	&s_aiHudLayout[0], 0,8192,1,	updateCB );
	bank.AddSlider( "HUD Y Offset",	&s_aiHudLayout[1], 0,8192,1,	updateCB );
	
	bank.PopGroup();
}
#else	//SUPPORT_MULTI_MONITOR
void MonitorConfiguration::addWidgets(bkBank&)	{}
#endif	//SUPPORT_MULTI_MONITOR
#endif	//__BANK


}	//rage
