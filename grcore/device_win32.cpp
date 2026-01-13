// 
// grcore/device_win32.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "device.h"
#include "channel.h"
#include "string/string.h"
#include "string/unicode.h"
#include "file/file_config.h"

#if __WIN32PC
#pragma warning(disable: 4668)
#include <winsock2.h>
#include <Wtsapi32.h>
#pragma comment(lib,"Wtsapi32.lib")
#pragma warning(error: 4668)

#include "system/xtl.h"
#include "system/d3d9.h"
#include "grcore/d3dwrapper.h"

#include "bank/bkmgr.h"
#include "bank/packet.h"
#include "diag/diagerrorcodes.h"
#include "system/bootmgr.h"
#include "system/wndproc.h"
#include "system/service.h"
#include "system/timemgr.h"
#include "input/input.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "resourcecache.h"
#include "texture.h"
#include "viewport.h"
#include <ddraw.h>
#include <wbemidl.h>

#if RSG_PC
#include <dbt.h>
#endif

#if __D3D11
#include "grcore/adapter.h"
#include "grcore/adapter_d3d11.h"
#endif

#include <dxgi.h>
#if __D3D9
#include <dxdiag.h>
#endif // __D3D9
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#include <strsafe.h>

#pragma comment(lib,"wbemuuid.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"d3d9.lib")

// #include <shellscalingapi.h>

// Windows 10 Shore functions 
typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE = 0,
	PROCESS_SYSTEM_DPI_AWARE = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;

typedef HRESULT(WINAPI* LPSetProcessDpiAwareness)(PROCESS_DPI_AWARENESS value);
LPSetProcessDpiAwareness fnSetProcessDpiAwareness = nullptr;

#define WM_DPICHANGED                   0x02E0
#define WM_DPICHANGED_BEFOREPARENT      0x02E2
#define WM_DPICHANGED_AFTERPARENT       0x02E3
#define WM_GETDPISCALEDSIZE             0x02E4

// Windows 10 User functons
typedef BOOL(WINAPI* LPEnableNonClientDpiScaling)(HWND hwnd);
LPEnableNonClientDpiScaling fnEnableNonClientDpiScaling = nullptr;
#endif // __WIN32PC

#if RSG_DURANGO
#include "system/xtl.h"
#endif

#include "system/param.h"

#if NV_SUPPORT
#include "../../3rdParty/NVidia/nvapi.h"
# if RSG_CPU_X64
#  pragma comment(lib,"nvapi64.lib")
# elif RSG_CPU_X86
#  pragma comment(lib,"nvapi.lib")
# endif //CPU
#endif //NV_SUPPORT

#if ATI_SUPPORT
#include "../../3rdparty/AMD/AGS_v2.1/AGS Lib/inc/amd_ags.h"
# if RSG_CPU_X64
#   pragma comment(lib,"amd_ags64.lib")
# elif RSG_CPU_X86
#   pragma comment(lib,"amd_ags.lib")
# endif //CPU
#endif //ATI_SUPPORT

extern grcPresentParameters s_d3dpp;

#if __D3D11 || __BANK
XPARAM(rag);
XPARAM(ragUseOwnWindow);
#endif

namespace rage {

#if !__FINAL
PARAM(minimizableWindow, "Have minimize button active on window");
PARAM(maximizableWindow, "Have maximize button active on window");
PARAM(closeableWindow, "Have close button active on window");

PARAM(noRefocusWindow, "Don't refocus the window when it loses focus");

PARAM(forceResolution, "Force the window to not adjust to the monitor size");

PARAM(hidewindow, "[grcore] the main window is created hidden");

PARAM(minimizeLostFocus, "minimize when fullscreen loses focus");
#endif

NOSTRIP_XPARAM(availablevidmem);

NOSTRIP_PC_PARAM(borderless,"[grcore] Set main window to be borderless");
NOSTRIP_PC_PARAM(allowResizeWindow,"[grcore] Allow the window to be rezised");
NOSTRIP_PC_PARAM(disallowResizeWindow,"[grcore] Do Not allow the window to be rezised");
NOSTRIP_PC_PARAM(GPUCount,"[grcore] Manual override GPU Count");
NOSTRIP_PC_PARAM(setWindowPosition, "[graphics] Set Window Position");
XPARAM(setHwndMain);


#if !__FINAL
PARAM(BlockOnLostFocus, "[RenderThread] Block when game loses focus");
#endif

#if __WIN32PC || RSG_DURANGO
int grcDevice::sm_ThreadId = -1;
sysIpcSema grcDevice::sm_Controller = NULL;
sysIpcCurrentThreadId grcDevice::sm_CreationOwner = sysIpcCurrentThreadIdInvalid;
bool grcDevice::sm_AllowUpdateThreadBlock = true;
#endif

#if __WIN32PC
float grcDevice::sm_AspectRatio;

float grcDevice::sm_maxAspectRatio = 1366.0f/768.0f;
float grcDevice::sm_minAspectRatio = 1280.0f/1024.0f;

bool  grcDevice::sm_IgnoreStereoMsg = false;
int   grcDevice::sm_StereoDesired = 0;
bool  grcDevice::sm_bCanUseStereo = false;
bool  grcDevice::sm_bStereoEnabled = false;
bool  grcDevice::sm_bStereoScissor = true;
void* grcDevice::sm_pStereoHandle = NULL;
grcTexture* grcDevice::sm_StereoTex = NULL;
float grcDevice::sm_fStereoConvergence = 3.5f;
float grcDevice::sm_fEyeSeparation = 0.0f;
float grcDevice::sm_fDefaultStereoConvergence = 1.0f;
float grcDevice::sm_fStereoSeparationPercentage = 20;

float grcDevice::sm_DesiredSeparation = -1.0f;
float grcDevice::sm_DesiredConvergence = -1.0f;

bool grcDevice::sm_UseNVidiaAPI = true;
bool grcDevice::sm_UseVendorAPI = true;

// True if we have lost focus (and therefore need to block, rather than poll, for messages)
bool grcDevice::sm_BlockOnLostFocus = true;
bool grcDevice::sm_AllowFullscreenToggle = true;
bool grcDevice::sm_bIssuingReset = false;
u32  grcDevice::sm_uDeviceStatus = S_OK;
bool grcDevice::sm_Paused = false;
bool grcDevice::sm_LostFocus = false;
bool grcDevice::sm_LostFocusForAudio = false;
bool grcDevice::sm_Minimized = false;
bool grcDevice::sm_Maximized = false;
bool grcDevice::sm_Occluded = false;
bool grcDevice::sm_MinimizedWhileFullscreen = false;
bool grcDevice::sm_TopMostWhileWindowed = false;
bool grcDevice::sm_IgnoreSizeChange = false;
bool grcDevice::sm_Active = true;
bool grcDevice::sm_InSizeMove = false;
bool grcDevice::sm_bStereoChangeEnable = true;
bool grcDevice::sm_ReleasingSwapChain = false;
bool grcDevice::sm_Lost = false;
u32	grcDevice::sm_WindowStyle = 0;
bool grcDevice::sm_InsideDeviceChange = false;
bool grcDevice::sm_DoNoStoreNewSize = false;
bool grcDevice::sm_ClipCursorWhenFullscreen = !__DEV;
bool grcDevice::sm_DeviceRestored = false;
bool grcDevice::sm_ForceWindowResize = false;

bool grcDevice::sm_SwapChainFullscreen = false;

bool grcDevice::sm_MatchDesiredWindow = false;
bool grcDevice::sm_RecheckDeviceChanges = false;

bool grcDevice::sm_FrameLockOverride = false;


#if __D3D11
bool grcDevice::sm_ChangeDeviceRequest = false;
bool grcDevice::sm_IsInPopup = false;
bool grcDevice::sm_ForceChangeDevice = false;
bool grcDevice::sm_ForceDeviceReset = false;
bool grcDevice::sm_RequireDeviceRestoreCallbacks = false;
bool grcDevice::sm_IgnoreMonitorWindowLimits = false;
#endif

#if USE_NV_TXAA
bool grcDevice::sm_TXAASupported = false;
#endif

#if RSG_PC && __D3D11
bool grcDevice::sm_DragResized = false;
bool grcDevice::sm_RecenterWindow = false;
bool grcDevice::sm_Borderless = false;
bool grcDevice::sm_DesireBorderless = false;
#endif

#if __D3D11
bool grcDevice::sm_BusyAltTabbing = false;
bool grcDevice::sm_IgnoreDeviceReset = false;
#endif
#if RSG_PC && __DEV
fiStream* grcDevice::sm_DeviceResetLoggingStream = NULL;
bool grcDevice::sm_DeviceResetTestActive = false;
#endif

unsigned long grcDevice::sm_WindowFlags = 0;

bool grcDevice::sm_RequestPauseMenu = false;

bool grcDevice::sm_AllowPauseOnFocusLoss = true;

bool grcDevice::sm_DisablePauseOnFocusLossSystemOverride = true;
bool grcDevice::sm_VideoEncodingOverride = false;
bool grcDevice::sm_HeadBlendingOverride = false;

bool sm_BlockOnLostFocus;
bool sm_AllowFullscreenToggle;
u32  sm_uDeviceStatus;	

atFixedArray<Functor0, grcDevice::MAX_DEVICE_CALLBACKS> grcDevice::sm_DeviceLostCb;
atFixedArray<Functor0, grcDevice::MAX_DEVICE_CALLBACKS> grcDevice::sm_DeviceResetCb;

#if NV_SUPPORT
static bool s_gAltTabbingState = false;
#endif

#if NV_SUPPORT
Functor1<float> grcDevice::sm_StereoSepChangeCb;
Functor1<float> grcDevice::sm_StereoConvChangeCb;
#endif

WINDOWPLACEMENT s_WindowedPlacement;
extern unsigned int g_grcDepthFormat;
#endif

#if RSG_DURANGO
u32  grcDevice::sm_uDeviceStatus = S_OK;
volatile bool grcDevice::sm_HasFocus = true;
volatile u32 grcDevice::sm_KillSwitch = 0;
#endif

#if __WIN32
// Application window title.
static char s_WindowTitle[64];

void grcDevice::SetWindowTitle(const char *appName) {
	safecpy(s_WindowTitle,appName,sizeof(s_WindowTitle));
#if __WIN32PC
	if (g_hwndMain)	// Set title immediately if window is already up
		::PostMessageA(g_hwndMain, WM_SETTEXT, 0, (LPARAM)appName);
#endif
}
#endif

/* True if user has attempted to close the application window. */
static bool s_Closed;

bool grcDevice::QueryCloseWindow() {
	return s_Closed;
}

void grcDevice::ClearCloseWindow() {
	s_Closed = false;
}

#if RSG_PC
void grcDevice::SetCloseWindow() {
	s_Closed = true;
}
#endif	//RSG_PC



#if __WIN32PC

#define MINIMUM_WIDTH 800
#define MINIMUM_HEIGHT 600

//-----------------------------------------------------------------------------
struct DDRAW_MATCH
{
    GUID guid;
    HMONITOR hMonitor;
    CHAR strDriverName[512];
    bool bFound;
};

//-----------------------------------------------------------------------------
typedef HRESULT ( WINAPI* LPCREATEDXGIFACTORY )( REFIID, void** );

#if __D3D9
typedef HRESULT ( WINAPI* LPDIRECTDRAWCREATE )( GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter );
#endif // (__SHADERMODEL < 40)

typedef BOOL    ( WINAPI* PfnCoSetProxyBlanket )( IUnknown* pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc,
                                                  OLECHAR* pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel,
                                                  RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities );

//-----------------------------------------------------------------------------
BOOL CALLBACK MonitorEnumProc( HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData );
HRESULT GetDeviceIDFromHMonitor( HMONITOR hm, WCHAR* strDeviceID, int cchDeviceID );
HRESULT GetVideoMemoryViaWMI( HMONITOR hMonitor, SIZE_T* pdwAdapterRam );
HRESULT GetVideoMemoryViaD3D9( HMONITOR hMonitor, SIZE_T* pdwAvailableTextureMem );
HRESULT GetVideoMemoryViaDXGI( HMONITOR hMonitor,
                               SIZE_T* pDedicatedVideoMemory,
                               SIZE_T* pDedicatedSystemMemory,
                               SIZE_T* pSharedSystemMemory );
HRESULT GetVideoMemoryViaDxDiag( HMONITOR hMonitor, SIZE_T* pdwDisplayMemory );
HRESULT GetVideoMemoryViaDirectDraw( HMONITOR hMonitor, SIZE_T* pdwAvailableVidMem, SIZE_T* pdwFreeVidMem, bool bAnycard );

//-----------------------------------------------------------------------------
BOOL WINAPI DDEnumCallbackEx( GUID FAR* lpGUID, LPSTR /*lpDriverDescription*/, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm )
{
    DDRAW_MATCH* pDDMatch = ( DDRAW_MATCH* ) lpContext;
    if( pDDMatch->hMonitor == hm )
	{
        pDDMatch->bFound = true;
        strncpy( pDDMatch->strDriverName, lpDriverName, 512 );
        memcpy( &pDDMatch->guid, lpGUID, sizeof( GUID ) );
		return FALSE;
    }
    return TRUE;
}

#if __D3D9
//-----------------------------------------------------------------------------
HRESULT GetDeviceIDFromHMonitor( HMONITOR hm, WCHAR* strDeviceID, int cchDeviceID )
{
    HINSTANCE hInstDDraw;

    hInstDDraw = LoadLibrary( "ddraw.dll" );
    if( hInstDDraw )
    {
        DDRAW_MATCH match;
        ZeroMemory( &match, sizeof( DDRAW_MATCH ) );
        match.hMonitor = hm;

        LPDIRECTDRAWENUMERATEEXA pDirectDrawEnumerateEx = NULL;
        pDirectDrawEnumerateEx = ( LPDIRECTDRAWENUMERATEEXA )GetProcAddress( hInstDDraw, "DirectDrawEnumerateExA" );

        if( pDirectDrawEnumerateEx )
            pDirectDrawEnumerateEx( DDEnumCallbackEx, ( VOID* )&match, DDENUM_ATTACHEDSECONDARYDEVICES );

        if( match.bFound )
        {
            LONG iDevice = 0;
            DISPLAY_DEVICEA dispdev;

            ZeroMemory( &dispdev, sizeof( dispdev ) );
            dispdev.cb = sizeof( dispdev );

            while( EnumDisplayDevicesA( NULL, iDevice, ( DISPLAY_DEVICEA* )&dispdev, 0 ) )
            {
                // Skip devices that are monitors that echo another display
                if( dispdev.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER )
                {
                    iDevice++;
                    continue;
                }

                // Skip devices that aren't attached since they cause problems
                if( ( dispdev.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP ) == 0 )
                {
                    iDevice++;
                    continue;
                }

                if( _stricmp( match.strDriverName, dispdev.DeviceName ) == 0 )
                {
                    MultiByteToWideChar( CP_ACP, 0, dispdev.DeviceID, -1, strDeviceID, cchDeviceID );
                    return S_OK;
                }

                iDevice++;
            }
        }

        FreeLibrary( hInstDDraw );
    }

    return E_FAIL;
}

HRESULT GetVideoMemory( HMONITOR hMonitor, SIZE_T * pdwAvailableVidMem, SIZE_T * pdwFreeVidMem )
{
	SIZE_T uSharedMemory = 0;

	if (GetVideoMemoryViaDirectDraw(hMonitor, pdwAvailableVidMem, pdwFreeVidMem, false) == S_OK)
	{
        return S_OK;
	}
	else if (GetVideoMemoryViaWMI(hMonitor, pdwFreeVidMem) == S_OK)
	{
		*pdwAvailableVidMem = *pdwFreeVidMem;
		return S_OK;
	}
	else if (GetVideoMemoryViaDXGI(hMonitor, pdwFreeVidMem, pdwAvailableVidMem, &uSharedMemory) == S_OK)
	{
		if (*pdwFreeVidMem == 0)
		{
			*pdwFreeVidMem = uSharedMemory;
		}
		*pdwAvailableVidMem = *pdwFreeVidMem;
		return S_OK;
	}
	else if (GetVideoMemoryViaDxDiag(hMonitor, pdwFreeVidMem) == S_OK)
	{
		*pdwAvailableVidMem = *pdwFreeVidMem;
		return S_OK;
	}
	else if (GetVideoMemoryViaDirectDraw(hMonitor, pdwAvailableVidMem, pdwFreeVidMem, true) == S_OK)
	{
        return S_OK;
	}
#if __D3D9
	else if (GetVideoMemoryViaD3D9(hMonitor, pdwFreeVidMem) == S_OK)
	{
		*pdwAvailableVidMem = *pdwFreeVidMem;
		return S_OK;
	}
#endif
	else
	{				
		// If the card fails both test default to 512MB of video memory
		*pdwAvailableVidMem = 512 * 1024 * 1024;
		*pdwFreeVidMem = *pdwAvailableVidMem;
        //return E_FAIL;
		Assert(0 && "Failed to Query for Video Memory - Assuming 512MB of video memory available - Ignore but tell Oscar");
		return S_OK;
	}
}

//-----------------------------------------------------------------------------
HRESULT GetVideoMemoryViaDirectDraw( HMONITOR hMonitor, SIZE_T* pdwAvailableVidMem, SIZE_T* pdwFreeVidMem, bool bAnycard )
{
    LPDIRECTDRAW pDDraw = NULL;
    LPDIRECTDRAWENUMERATEEXA pDirectDrawEnumerateEx = NULL;
    HRESULT hr;
    bool bGotMemory = false;
	*pdwAvailableVidMem = 0;

    HINSTANCE hInstDDraw;
    LPDIRECTDRAWCREATE pDDCreate = NULL;

    hInstDDraw = LoadLibrary( "ddraw.dll" );
    if( hInstDDraw )
    {
        DDRAW_MATCH match;
        ZeroMemory( &match, sizeof( DDRAW_MATCH ) );
        match.hMonitor = hMonitor;

        pDirectDrawEnumerateEx = ( LPDIRECTDRAWENUMERATEEXA )GetProcAddress( hInstDDraw, "DirectDrawEnumerateExA" );

        if( pDirectDrawEnumerateEx )
        {
            hr = pDirectDrawEnumerateEx( DDEnumCallbackEx, ( VOID* )&match, DDENUM_ATTACHEDSECONDARYDEVICES );
        }

        pDDCreate = ( LPDIRECTDRAWCREATE )GetProcAddress( hInstDDraw, "DirectDrawCreate" );
        if( pDDCreate )
        {
            hr = pDDCreate( &match.guid, &pDDraw, NULL );
			if( SUCCEEDED( hr ) )
			{
				if (match.bFound || bAnycard)
				{
					LPDIRECTDRAW7 pDDraw7;
					if( SUCCEEDED( pDDraw->QueryInterface( IID_IDirectDraw7, ( VOID** )&pDDraw7 ) ) )
					{
						DDSCAPS2 ddscaps;
						ZeroMemory( &ddscaps, sizeof( DDSCAPS2 ) );
						ddscaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
						DWORD dwVidMem, dwFreeVidMem;
						hr = pDDraw7->GetAvailableVidMem( &ddscaps, &dwVidMem, &dwFreeVidMem );
						*pdwAvailableVidMem = dwVidMem;
						*pdwFreeVidMem = dwFreeVidMem;
						if( SUCCEEDED( hr ) )
							bGotMemory = true;
						pDDraw7->Release();
					}
				}
				else
				{
					grcWarningf( "Probably wrong video card was queried for video memory if you get this warning" );
				}
				pDDraw->Release();
			}
        }
        FreeLibrary( hInstDDraw );
    }

    if( bGotMemory )
	{
		if (bAnycard)
		{
			grcWarningf("Cheated to get video memory");
		}
        return S_OK;
	} 
	else
	{
        return E_FAIL;
	}
}

HRESULT GetVideoMemoryViaDXGI( HMONITOR hMonitor,
                               SIZE_T* pDedicatedVideoMemory,
                               SIZE_T* pDedicatedSystemMemory,
                               SIZE_T* pSharedSystemMemory )
{
    HRESULT hr;
    bool bGotMemory = false;
    *pDedicatedVideoMemory = 0;
    *pDedicatedSystemMemory = 0;
    *pSharedSystemMemory = 0;

    HINSTANCE hDXGI = LoadLibrary( "dxgi.dll" );
    if( hDXGI )
    {
        LPCREATEDXGIFACTORY pCreateDXGIFactory = NULL;
        IDXGIFactory* pDXGIFactory = NULL;

        pCreateDXGIFactory = ( LPCREATEDXGIFACTORY )GetProcAddress( hDXGI, "CreateDXGIFactory" );
        pCreateDXGIFactory( __uuidof( IDXGIFactory ), ( LPVOID* )&pDXGIFactory );

        for( int index = 0; ; ++index )
        {
            bool bFoundMatchingAdapter = false;
            IDXGIAdapter* pAdapter = NULL;
            hr = pDXGIFactory->EnumAdapters( index, &pAdapter );
            if( FAILED( hr ) ) // DXGIERR_NOT_FOUND is expected when the end of the list is hit
                break;

            for( int iOutput = 0; ; ++iOutput )
            {
                IDXGIOutput* pOutput = NULL;
                hr = pAdapter->EnumOutputs( iOutput, &pOutput );
                if( FAILED(hr) || !pOutput ) // DXGIERR_NOT_FOUND is expected when the end of the list is hit
                    break;

                DXGI_OUTPUT_DESC outputDesc;
                ZeroMemory( &outputDesc, sizeof( DXGI_OUTPUT_DESC ) );
                if( SUCCEEDED( pOutput->GetDesc( &outputDesc ) ) )
                {
                    if( hMonitor == outputDesc.Monitor )
                        bFoundMatchingAdapter = true;

                }

				pOutput->Release();
				pOutput = NULL;
            }

            if( bFoundMatchingAdapter )
            {
                DXGI_ADAPTER_DESC desc;
                ZeroMemory( &desc, sizeof( DXGI_ADAPTER_DESC ) );
                if( SUCCEEDED( pAdapter->GetDesc( &desc ) ) )
                {
                    bGotMemory = true;
                    *pDedicatedVideoMemory = desc.DedicatedVideoMemory;
                    *pDedicatedSystemMemory = desc.DedicatedSystemMemory;
                    *pSharedSystemMemory = desc.SharedSystemMemory;
                }
                break;
            }
        }

        FreeLibrary( hDXGI );
    }

    if( bGotMemory )
        return S_OK;
    else
        return E_FAIL;
}

HRESULT GetVideoMemoryViaDxDiag( HMONITOR hMonitor, SIZE_T* pdwDisplayMemory )
{
    WCHAR strInputDeviceID[512];
    if (GetDeviceIDFromHMonitor( hMonitor, strInputDeviceID, 512 ) != S_OK)
		return E_FAIL;

    HRESULT hr;
    HRESULT hrCoInitialize = S_OK;
    bool bGotMemory = false;
    IDxDiagProvider* pDxDiagProvider = NULL;
    IDxDiagContainer* pDxDiagRoot = NULL;
    IDxDiagContainer* pDevices = NULL;
    IDxDiagContainer* pDevice = NULL;
    WCHAR wszChildName[256];
    WCHAR wszPropValue[256];
    DWORD dwDeviceCount;
    VARIANT var;

    *pdwDisplayMemory = 0;
    hrCoInitialize = CoInitialize( 0 );
    VariantInit( &var );

    // CoCreate a IDxDiagProvider*
    hr = CoCreateInstance( CLSID_DxDiagProvider,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IDxDiagProvider,
                           ( LPVOID* )&pDxDiagProvider );
    if( SUCCEEDED( hr ) ) // if FAILED(hr) then it is likely DirectX 9 is not installed
    {
        DXDIAG_INIT_PARAMS dxDiagInitParam;
        ZeroMemory( &dxDiagInitParam, sizeof( DXDIAG_INIT_PARAMS ) );
        dxDiagInitParam.dwSize = sizeof( DXDIAG_INIT_PARAMS );
        dxDiagInitParam.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
        dxDiagInitParam.bAllowWHQLChecks = FALSE;
        dxDiagInitParam.pReserved = NULL;
        pDxDiagProvider->Initialize( &dxDiagInitParam );

        hr = pDxDiagProvider->GetRootContainer( &pDxDiagRoot );
        if( SUCCEEDED( hr ) )
        {
            hr = pDxDiagRoot->GetChildContainer( L"DxDiag_DisplayDevices", &pDevices );
            if( SUCCEEDED( hr ) )
            {
                hr = pDevices->GetNumberOfChildContainers( &dwDeviceCount );
                if( SUCCEEDED( hr ) )
                {
                    for( DWORD iDevice = 0; iDevice < dwDeviceCount; iDevice++ )
                    {
                        bool bFound = false;
                        hr = pDevices->EnumChildContainerNames( iDevice, wszChildName, 256 );
                        if( SUCCEEDED( hr ) )
                        {
                            hr = pDevices->GetChildContainer( wszChildName, &pDevice );
                            if( SUCCEEDED(hr) && pDevice )
                            {
                                hr = pDevice->GetProp( L"szKeyDeviceID", &var );
                                if( SUCCEEDED( hr ) )
                                {
                                    if( var.vt == VT_BSTR )
                                    {
                                        if( wcsstr( var.bstrVal, strInputDeviceID ) != 0 )
                                            bFound = true;
                                    }
                                    VariantClear( &var );
                                }

                                if( bFound )
                                {
                                    hr = pDevice->GetProp( L"szDisplayMemoryEnglish", &var );
                                    if( SUCCEEDED( hr ) )
                                    {
                                        if( var.vt == VT_BSTR )
                                        {
                                            bGotMemory = true;
                                            wcscpy_s( wszPropValue, 256, var.bstrVal );

                                            // wszPropValue should be something like "256.0 MB" so _wtoi will convert it correctly
                                            *pdwDisplayMemory = _wtoi( wszPropValue );

                                            // Convert from MB to bytes
                                            *pdwDisplayMemory *= 1024 * 1024;
                                        }
                                        VariantClear( &var );
                                    }
                                }
                               
								pDevice->Release();
								pDevice = NULL;
                            }
                        }
                    }
                }
				if (pDevices != NULL)
				{
					pDevices->Release();
					pDevices = NULL;
				}
            }
			if (pDxDiagRoot != NULL)
			{
				pDxDiagRoot->Release();
				pDxDiagRoot = NULL;
			}
        }
		if (pDxDiagProvider != NULL)
		{
			pDxDiagProvider->Release();
			pDxDiagProvider = NULL;
		}
    }

    if( SUCCEEDED( hrCoInitialize ) )
        CoUninitialize();

    if( bGotMemory )
        return S_OK;
    else
        return E_FAIL;
}

#if __D3D9
HRESULT GetVideoMemoryViaD3D9( HMONITOR hMonitor, SIZE_T* pdwAvailableTextureMem)
{
    HRESULT hr;
    bool bGotMemory = false;
    *pdwAvailableTextureMem = 0;

    IDirect3D9* pD3D9 = NULL;
	if (g_Direct3DCreate9 == NULL)
		return 0;

    pD3D9 = g_Direct3DCreate9( D3D_SDK_VERSION );
    if( pD3D9 )
    {
        UINT dwAdapterCount = pD3D9->GetAdapterCount();
        for( UINT iAdapter = 0; iAdapter < dwAdapterCount; iAdapter++ )
        {
            IDirect3DDevice9* pd3dDevice = NULL;

            HMONITOR hAdapterMonitor = pD3D9->GetAdapterMonitor( iAdapter );
            if( hMonitor != hAdapterMonitor )
                continue;

            HWND hWnd = GetDesktopWindow();

            grcPresentParameters pp;
            ZeroMemory( &pp, sizeof( grcPresentParameters ) );
            pp.BackBufferWidth = 800;
            pp.BackBufferHeight = 600;
            pp.BackBufferFormat = D3DFMT_R5G6B5;
            pp.BackBufferCount = 1;
            pp.MultiSampleType = D3DMULTISAMPLE_NONE;
            pp.MultiSampleQuality = 0;
            pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
            pp.hDeviceWindow = hWnd;
            pp.Windowed = TRUE;

            pp.EnableAutoDepthStencil = FALSE;
            pp.Flags = 0;
            pp.FullScreen_RefreshRateInHz = 0;
            pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

            hr = pD3D9->CreateDevice( iAdapter, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &pd3dDevice );
            if( SUCCEEDED( hr ) )
            {
                *pdwAvailableTextureMem = pd3dDevice->GetAvailableTextureMem();
                bGotMemory = true;
				if ( pd3dDevice != NULL)
				{
					pd3dDevice->Release();
					pd3dDevice = NULL;
				}
            }
        }

        if ( pD3D9 != NULL )
		{
			pD3D9->Release();
			pD3D9 = NULL;
		}
    }

    if( bGotMemory )
        return S_OK;
    else
        return E_FAIL;
}
#endif

HRESULT GetVideoMemoryViaWMI( HMONITOR hMonitor, SIZE_T* pdwAdapterRam )
{
    WCHAR strInputDeviceID[512];
    GetDeviceIDFromHMonitor( hMonitor, strInputDeviceID, 512 );

    HRESULT hr;
    bool bGotMemory = false;
    HRESULT hrCoInitialize = S_OK;
    IWbemLocator* pIWbemLocator = NULL;
    IWbemServices* pIWbemServices = NULL;
    BSTR pNamespace = NULL;

    *pdwAdapterRam = 0;
    hrCoInitialize = CoInitialize( 0 );

    hr = CoCreateInstance( CLSID_WbemLocator,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IWbemLocator,
                           ( LPVOID* )&pIWbemLocator );

    if( SUCCEEDED( hr ) && pIWbemLocator )
    {
        // Using the locator, connect to WMI in the given namespace.
        pNamespace = SysAllocString( L"\\\\.\\root\\cimv2" );

        hr = pIWbemLocator->ConnectServer( pNamespace, NULL, NULL, 0L,
                                           0L, NULL, NULL, &pIWbemServices );
        if( SUCCEEDED( hr ) && pIWbemServices != NULL )
        {
            HINSTANCE hinstOle32 = NULL;

            hinstOle32 = LoadLibraryW( L"ole32.dll" );
            if( hinstOle32 )
            {
                PfnCoSetProxyBlanket pfnCoSetProxyBlanket = NULL;

                pfnCoSetProxyBlanket = ( PfnCoSetProxyBlanket )GetProcAddress( hinstOle32, "CoSetProxyBlanket" );
                if( pfnCoSetProxyBlanket != NULL )
                {
                    // Switch security level to IMPERSONATE. 
                    pfnCoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                                          RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0 );
                }

                FreeLibrary( hinstOle32 );
            }

            IEnumWbemClassObject* pEnumVideoControllers = NULL;
            BSTR pClassName = NULL;

            pClassName = SysAllocString( L"Win32_VideoController" );

            hr = pIWbemServices->CreateInstanceEnum( pClassName, 0,
                                                     NULL, &pEnumVideoControllers );

            if( SUCCEEDED( hr ) && pEnumVideoControllers )
            {
                IWbemClassObject* pVideoControllers[10] =
                {
                    0
                };
                DWORD uReturned = 0;
                BSTR pPropName = NULL;

                // Get the first one in the list
                pEnumVideoControllers->Reset();
                hr = pEnumVideoControllers->Next( 5000,             // timeout in 5 seconds
                                                  10,                  // return the first 10
                                                  pVideoControllers,
                                                  &uReturned );

                VARIANT var;
                if( SUCCEEDED( hr ) )
                {
                    bool bFound = false;
                    for( UINT iController = 0; iController < uReturned; iController++ )
                    {
                        pPropName = SysAllocString( L"PNPDeviceID" );
                        hr = pVideoControllers[iController]->Get( pPropName, 0L, &var, NULL, NULL );
                        if( SUCCEEDED( hr ) )
                        {
                            if( wcsstr( var.bstrVal, strInputDeviceID ) != 0 )
                                bFound = true;
                        }
                        VariantClear( &var );
                        if( pPropName ) SysFreeString( pPropName );

                        if( bFound )
                        {
                            pPropName = SysAllocString( L"AdapterRAM" );
                            hr = pVideoControllers[iController]->Get( pPropName, 0L, &var, NULL, NULL );
                            if( SUCCEEDED( hr ) )
                            {
                                bGotMemory = true;
                                *pdwAdapterRam = var.ulVal;
                            }
                            VariantClear( &var );
                            if( pPropName ) SysFreeString( pPropName );
                            break;
                        }
                        
						pVideoControllers[iController]->Release();
						pVideoControllers[iController] = NULL;
                    }
                }
            }

            if( pClassName )
                SysFreeString( pClassName );

            if ( pEnumVideoControllers != NULL )
			{
				pEnumVideoControllers->Release();
				pEnumVideoControllers = NULL;
			}
        }

        if( pNamespace )
            SysFreeString( pNamespace );

		if ( pIWbemServices != NULL )
		{
			pIWbemServices->Release();
			pIWbemServices = NULL;
		}
    }

	if ( pIWbemLocator != NULL )
	{
		pIWbemLocator->Release();
		pIWbemLocator = NULL;
	}

    if( SUCCEEDED( hrCoInitialize ) )
        CoUninitialize();

    if( bGotMemory )
        return S_OK;
    else
        return E_FAIL;
}
#endif // __D3D9

#if __D3D && RSG_PC && __D3D9 && !__RESOURCECOMPILER
s64 grcDevice::GetAvailableVideoMemory(int adapter)
{
	(void) adapter;
	// Warning - Very slow operation the way this is implemented - Can be improved
	// but documentation does not recommend this for precision memory allocation but a general overview
	// of the available video memory
	// PC TODO - Check to see how on board memory solutions work with this
	// May need to do addition caps query for better numbers on shared memory architectures
    IDirect3D9* pD3D9 = NULL;
	if (g_Direct3DCreate9 == NULL)
		return 0;

    pD3D9 = g_Direct3DCreate9( D3D_SDK_VERSION );
	if (pD3D9 == NULL)
	{
		Printf("Failed to Query Available Video Memory");
		Quitf("D3D Error - Failed to query memory. Please re-start the game.");
		// return 0;
	}	
    SIZE_T dwAvailableVidMem;
	SIZE_T dwFreeVidMem;
	HMONITOR hMonitor = pD3D9->GetAdapterMonitor(GRCDEVICE.GetAdapterOrdinal());
    if( FAILED( GetVideoMemory( hMonitor, &dwAvailableVidMem, &dwFreeVidMem ) ) )
	{
		Printf("Failed to Query Available Video Memory");
		Quitf("D3D Error - Failed to query memory. Please re-start the game.");
	}
	
	pD3D9->Release();
	pD3D9=NULL;

	float fScale = 1.0f;
	float fUserInput;

	if(PARAM_availablevidmem.GetArray(&fUserInput, 1))
	{
		fScale = fUserInput;
		grcDisplayf("Overriding default available video memory to %f\n", fScale);
	}

	return (s64)((float)dwFreeVidMem * fScale);
}

DeviceManufacturer grcDevice::GetManufacturer(int adapter)
{
	(void) adapter;
	static DeviceManufacturer m_eManufacturer = DM_LAST;
	if (m_eManufacturer >= DM_LAST)
	{
		// Find manufacturer
		u16 szData[2048];
		for (u32 uIndex = 0; uIndex < 4; uIndex++)
		{
			wchar_t szSearch[512];
			formatf(&szSearch[0], (sizeof(szSearch)/sizeof(wchar_t)) - 1, L"DxDiag_DisplayDevices.%d.iAdapter", uIndex);
			if (grcDevice::Query(szData, (const u16*)&szSearch[0]))
			{
				if (!wcscmp(L"0", (const wchar_t*)&szData[0]))
				{
					wchar_t szManufacturer[512];
					formatf(szManufacturer, (sizeof(szManufacturer)/sizeof(wchar_t)) - 1, L"DxDiag_DisplayDevices.%d.szManufacturer", uIndex);
					if (grcDevice::Query(szData, (const u16*)&szManufacturer[0]))
					{
						if (!wcscmp(L"NVIDIA", (const wchar_t*)&szData[0]))
						{
							m_eManufacturer = NVIDIA;
							break;
						}
						else
						{
							m_eManufacturer = ATI;
							break;
						}
					}
				}
			}
		}
	}
	Assert(m_eManufacturer < DM_LAST);
	if (m_eManufacturer >= DM_LAST)
	{
		m_eManufacturer = NVIDIA;
	}
	return m_eManufacturer;
}
#endif // __D3D9

u32 grcDevice::GetDepthFormat()
{
	return g_grcDepthFormat;
}	

void grcDevice::SetDepthFormat(u32 eFormat)
{
	g_grcDepthFormat = (grcTextureFormat)eFormat;
}

void grcDevice::IgnoreStereoMsg(bool bIgnore)
{
	sm_IgnoreStereoMsg = bIgnore;
}

bool grcDevice::UsingMultipleGPUs()
{
	return (GetGPUCount(true) > 1);
}

static u32 s_gpuIndexRT = 0;	// gpu index for render thread
static u32 s_gpuIndexMT = 0;	// gpu index for main thread

void grcDevice::IncrementGPUIndex()
{
	s_gpuIndexRT = (s_gpuIndexRT + 1) % GRCDEVICE.GetGPUCount(true);
}

u32 grcDevice::GPUIndex()
{
	Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());
	return s_gpuIndexRT;
}

u32 grcDevice::GPUIndexMT()
{
	Assert(!IsMessagePumpThreadThatCreatedTheD3DDevice());
	return s_gpuIndexMT;
}

void grcDevice::CopyGPUIndex()
{
	// copy render thread gpu index to main thread gpu index
	s_gpuIndexMT = s_gpuIndexRT;
}

u32 grcDevice::GetGPUCount(bool bActiveCount, bool bResetActiveCount)
{
#if __WIN32PC
	static u32 uCount = 0;
	static u32 uActiveCount = 0;

	if (bResetActiveCount)
		uActiveCount = 0;

	if (bActiveCount && uActiveCount != 0)
		return uActiveCount;

	if (!bActiveCount && uCount != 0)
		return uCount;

	uActiveCount = 1;

	if (GetManufacturer() == ATI && bActiveCount)
	{
		// if amd, only fullscreen enables crossfire mode
		if (!GRCDEVICE.IsSwapChainFullscreen())
		{
			uActiveCount = 1;
			return uActiveCount;
		}
	}

	uCount = 1;

#if ATI_SUPPORT
	if (GetManufacturer() == ATI)
	{
		AGSReturnCode ret = AGSInit();
		if (AssertVerify(ret == AGS_SUCCESS))
		{
			int displayIndex = 0;
			ret = AGSGetDefaultDisplayIndex(&displayIndex);
			if (AssertVerify(ret == AGS_SUCCESS))
			{
				int gpuCount = 0;
				ret = AGSCrossfireGetGPUCount( displayIndex, &gpuCount );
				if (AssertVerify(ret == AGS_SUCCESS))
				{
					uCount = gpuCount;
					// force this for AMD since it can't use crossfire in windowed
					//if (uCount > 1 && !GRCDEVICE.IsSwapChainFullscreen())
					//	uCount = 1;
				}
			}
			ret = AGSDeInit();
			Assert(ret == AGS_SUCCESS);
		}
	}
#endif
#if NV_SUPPORT
	if (GetManufacturer() == NVIDIA)
	{
		NvLogicalGpuHandle  logicalGPUs[NVAPI_MAX_LOGICAL_GPUS];
		NvU32               logicalGPUCount;
		NvPhysicalGpuHandle physicalGPUs[NVAPI_MAX_PHYSICAL_GPUS];
		NvU32               physicalGPUCount;

		NvAPI_Status status;

		NV_GET_CURRENT_SLI_STATE sliState;
		sliState.version = NV_GET_CURRENT_SLI_STATE_VER;
		sliState.numAFRGroups = 1;
		status = NvAPI_D3D_GetCurrentSLIState( GRCDEVICE.GetCurrent(), &sliState);

		if ( status == NVAPI_OK) 
		{
			// enumerate logical gpus
			status = NvAPI_EnumLogicalGPUs(logicalGPUs, &logicalGPUCount);
			if (status != NVAPI_OK)
			{
				grcWarningf("Failed to query for logical GPUs");
				return uCount;
			}

			// enumerate physical gpus
			status = NvAPI_EnumPhysicalGPUs(physicalGPUs, &physicalGPUCount);
			if (status != NVAPI_OK)
			{
				grcWarningf("Failed to query for physical GPUs");
				return uCount;
			}

			if(logicalGPUCount < physicalGPUCount)
			{
				uCount = physicalGPUCount;
			}
		}
		else 
		{
			uCount = sliState.numAFRGroups;
		}
	}
#endif

	PARAM_GPUCount.Get(uCount);

	uActiveCount = uCount; 

	return uCount;
#endif // __WIN32PC
}	


bool grcDevice::CleanupDevice()
{
	if (IsCreated())
	{
		SetInsideDeviceChange( true );
		DeviceLost();
		grcViewport::ShutdownClass();
		if (!FreeSwapChain())
		{
			return false;
		}
		SetInsideDeviceChange( false );
	}
	return true;
}


#if __D3D9
bool grcDevice::CheckForDeviceChanges()
{
	if (!IsCreated() || CanIgnoreSizeChange() || !IsWindowed())
		return false;

	RECT rcCurrentClient;
	GetClientRect( g_hwndMain, &rcCurrentClient );

	if(	(( s32 )rcCurrentClient.bottom != 0 &&
		( s32 )rcCurrentClient.right != 0) &&		
		(( s32 )rcCurrentClient.right != GetWidth() ||
		( s32 )rcCurrentClient.bottom != GetHeight()) )
	{
		grcDisplayWindow oNewSettings = GetCurrentWindow();
		oNewSettings.uWidth  = rcCurrentClient.right;
		oNewSettings.uHeight = rcCurrentClient.bottom;
		return ChangeDevice(oNewSettings);
	}
	return false;
}

bool grcDevice::ChangeDevice(grcDisplayWindow oNewSettings)
{
	if (!IsCreated())
		return false;

	sysIpcWaitSema(sm_Controller);

	SetPaused(true);
#if __D3D9
	SetIgnoreSizeChange(true);
#endif

	if (!oNewSettings.bFullscreen)
	{
		if (!IsWindowed())
		{
			Displayf("Transitioning from Fullscreen to Windowed");
			g_inWindow = !g_inWindow;

			SetWindowLong( g_hwndMain, GWL_STYLE, sm_WindowStyle );
	        SetWindowPlacement( g_hwndMain, &s_WindowedPlacement );
	        // Also restore the z-order of window to previous state
		    HWND hWndInsertAfter = g_isTopMost ? HWND_TOPMOST : HWND_NOTOPMOST;
			pcdDisplayf("ChangeDevice is doing SetWindowPos");
			SetWindowPos( g_hwndMain, hWndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE );

			if( ( s_WindowedPlacement.flags & WPF_RESTORETOMAXIMIZED ) != 0 && s_WindowedPlacement.showCmd == SW_SHOWMINIMIZED )
			{
				// WPF_RESTORETOMAXIMIZED means that when the window is restored it will
				// be maximized.  So maximize the window temporarily to get the client rect 
				// when the window is maximized.  GetSystemMetrics( SM_CXMAXIMIZED ) will give this 
				// information if the window is on the primary but this will work on multimon.
				ShowWindow( g_hwndMain, SW_RESTORE );
				RECT rcClient;
				GetClientRect( g_hwndMain, &rcClient );
				oNewSettings.uWidth = ( UINT )( rcClient.right - rcClient.left );
				oNewSettings.uHeight = ( UINT )( rcClient.bottom - rcClient.top );
				ShowWindow( g_hwndMain, SW_MINIMIZE );
			}
		}

		// If Fullscreen and Windowed using separate HWND then you should hide the windowed window
		// Otherwise reattach to windowed handle for menus
	}
	else
	{
		// Going to full screen mode
		if (IsWindowed())
		{
			Displayf("Transitioning from Windowed to Fullscreen");
			g_inWindow = !g_inWindow;
			ZeroMemory( &s_WindowedPlacement, sizeof( WINDOWPLACEMENT ) );
			s_WindowedPlacement.length = sizeof( WINDOWPLACEMENT );
			GetWindowPlacement( g_hwndMain, &s_WindowedPlacement );

			bool bIsTopmost = ( ( GetWindowLong( g_hwndMain,
												 GWL_EXSTYLE ) & WS_EX_TOPMOST ) != 0 );
			SetTopMostWhileWindowed( bIsTopmost );

			// Back up current window style
			sm_WindowStyle = GetWindowLong( g_hwndMain, GWL_STYLE );
			sm_WindowStyle &= ~WS_MAXIMIZE & ~WS_MINIMIZE; // remove minimize/maximize style

			// Back up old windowed mode res? - Optional really I think
		}

        // Hide the window to avoid displaying of blank windows
        //ShowWindow( g_hwndMain, SW_HIDE );

        // Set FS window style
        SetWindowLong( g_hwndMain, GWL_STYLE, WS_POPUP | WS_SYSMENU );

		// If Windowed and Fullscreen have unique HWND you should back up HWND of fullscreen device
			
        WINDOWPLACEMENT wpFullscreen;
        ZeroMemory( &wpFullscreen, sizeof( WINDOWPLACEMENT ) );
        wpFullscreen.length = sizeof( WINDOWPLACEMENT );
        GetWindowPlacement( g_hwndMain, &wpFullscreen );
        if( ( wpFullscreen.flags & WPF_RESTORETOMAXIMIZED ) != 0 )
        {
            // Restore the window to normal if the window was maximized then minimized.  This causes the 
            // WPF_RESTORETOMAXIMIZED flag to be set which will cause SW_RESTORE to restore the 
            // window from minimized to maxmized which isn't what we want
            wpFullscreen.flags &= ~WPF_RESTORETOMAXIMIZED;
            wpFullscreen.showCmd = SW_RESTORE;
            SetWindowPlacement( g_hwndMain, &wpFullscreen );
        }
	}

	// Clean up device
	if(IsCreated())
	{
		if (!CleanupDevice())
		{
			sm_GlobalWindow = sm_CurrentWindows[g_RenderThreadIndex] = oNewSettings;
			SetIgnoreSizeChange( false );
			SetPaused( false );		
			sysIpcSignalSema(sm_Controller);
			return false;
		}
	}

	grcDisplayWindow oBackUp = sm_CurrentWindows[g_RenderThreadIndex];
	sm_GlobalWindow = sm_CurrentWindows[g_RenderThreadIndex] = oNewSettings;
	Displayf("Changing Display Mode %d %d %s", sm_CurrentWindows[g_RenderThreadIndex].uWidth, sm_CurrentWindows[g_RenderThreadIndex].uHeight, sm_CurrentWindows[g_RenderThreadIndex].bFullscreen ? "Fullscreen" : "Windowed" );

	if (!CreateDevice())
	{
		CleanupDevice();

		sm_GlobalWindow = sm_CurrentWindows[g_RenderThreadIndex] = oBackUp;

		if (!CreateDevice())
		{
			// If fail restore old environment
			CleanupDevice();
			SetPaused(false);
			SetIgnoreSizeChange( false );
			Errorf("Failed to Create D3D Device");
			sysIpcSignalSema(sm_Controller);
			return false;
		}
	}

    // Make the window visible
    if( !IsWindowVisible( g_hwndMain ) )
        ShowWindow( g_hwndMain, SW_SHOW );

    // Ensure that the display doesn't power down when fullscreen but does when windowed
    if( !IsWindowed() )
        SetThreadExecutionState( ES_DISPLAY_REQUIRED | ES_CONTINUOUS );
    else
        SetThreadExecutionState( ES_CONTINUOUS );

	SetIgnoreSizeChange( false );
	SetPaused( false );
	sysIpcSignalSema(sm_Controller);
	return true;
}


bool grcDevice::CreateDevice()
{
	Assert(IsCreated());    

    // Setup cursor based on current settings (window/fullscreen mode, show cursor state, clip cursor state)
    SetupCursor(false);

    // Call the app's device reset callback if non-NULL
    SetInsideDeviceChange( true );
	if (!Reset())
	{
		SetInsideDeviceChange( false );
		return false;
	}
	SetInsideDeviceChange( false );
    SetInReset( false );

    // Call the app's device created callback if non-NULL
    SetInsideDeviceChange( true );
	if (!AllocateSwapChain())
	{
		SetInsideDeviceChange( false );
		return false;
	}

	DeviceRestored();
    SetInsideDeviceChange( false );

	grcViewport::InitClass();

    return true;
}
#endif

bool grcDevice::ToggleFullscreen()
{
	grcDisplayWindow oNewSettings;

	if (IsWindowed())
	{
		return GoFullscreen();
	}
	else
	{
		return GoWindowed();
	}
}

static bool sbToggledFromBorderlessWindow = false;
bool grcDevice::GoWindowed()
{
	grcDisplayf("Go Windowed");
#if __D3D11
	if (sbToggledFromBorderlessWindow)
	{
		GRCDEVICE.SetDesireBorderless(true);
		GRCDEVICE.SetRecenterWindow(true);
	}
#endif // __D3D11
	grcDisplayWindow ONewSettings = sm_GlobalWindow;
	ONewSettings.bFullscreen = false;
	return ChangeDevice(ONewSettings);
}

bool grcDevice::GoFullscreen()
{
#if __D3D11
	sbToggledFromBorderlessWindow = GRCDEVICE.IsDesireBorderless();
	GRCDEVICE.SetDesireBorderless(false);
#endif // __D3D11
	grcDisplayf("Go Fullscreen");
	return ChangeDevice(sm_FullscreenWindow);
}

#if __D3D9
void grcDevice::SetupCursor(WIN32PC_ONLY(bool bEnable))
{
	// NOTE: In device_d3d11.cpp in grcDevice::SetupCursor() we only call this function!
	// WIN32PC_ONLY(ioMouse::SetCursorVisible(bEnable));

//	if (PARAM_rag.Get()) bEnable = true;

    // Show the cursor again if returning to fullscreen 
    if( WIN32PC_ONLY(bEnable && IsActive() &&) !IsWindowed() && IsCreated() )
    {
        if( __DEV ) 
        {
            // SetCursor( NULL ); // Turn off Windows cursor in full screen mode
            //HCURSOR hCursor = ( HCURSOR )( ULONG_PTR )GetClassLongPtr( g_hwndMain, GCLP_HCURSOR );
			// In Fullscreen you need to set up your own cursor
			//DXUTSetD3D9DeviceCursor( pd3dDevice, hCursor, false );
            //DXUTGetD3D9Device()->ShowCursor( true );
        }
        else
        {
            // SetCursor( NULL ); // Turn off Windows cursor in full screen mode
//            GetCurrentInner()->ShowCursor( false );
        }
    }

    // Clip cursor if requested
    if( WIN32PC_ONLY(bEnable && IsActive() &&) !IsWindowed() && ShouldClipCursorWhenFullscreen() )
    {
        // Confine cursor to full screen window
        RECT rcWindow;
        GetWindowRect( g_hwndMain, &rcWindow );
        ClipCursor( &rcWindow );
    }
    else
    {
        ClipCursor( NULL );
    }

	static int iCursorRefCount = 0;
	static int iExceed = 3;
	if (bEnable)
	{
		if (iCursorRefCount < iExceed)
		{
			int iLast = iCursorRefCount;
			grcDisplayf("Setting visible %d", iCursorRefCount);
			while ((iCursorRefCount = ::ShowCursor( true )) < 0)
			{
				Assert(iLast != iCursorRefCount);
				iLast = iCursorRefCount;
			}
			
		}
		else
		{
			//Displayf("Show Already visible %d", iCursorRefCount);
		}
	}
	else
	{
		if (iCursorRefCount >= -iExceed)
		{
			int iLast = iCursorRefCount;
			grcDisplayf("Setting Invisible %d", iCursorRefCount);
			while ((iCursorRefCount = ::ShowCursor( false )) >= 0)
			{
				Assert(iLast != iCursorRefCount);
				iLast = iCursorRefCount;
			}
		}
		else
		{
			//Displayf("Show Already invisible %d", iCursorRefCount);
		}
	}
}


bool grcDevice::Reset()
{
	Assert(IsCreated());

    // Call the app's device lost callback
    SetInsideDeviceChange( true );
	DeviceLost();
    SetInsideDeviceChange( false );

	UpdatePresentParameters();
#if __D3D9
	HRESULT hr = sm_Current->Reset(&s_d3dpp);
    if( hr != D3D_OK )
    {
        if( hr == D3DERR_DEVICELOST )
		{
			Warningf("Reset Failed - Device Lost");
            return false; // Reset could legitimately fail if the device is lost
		}
        else
		{
			Errorf("Failed to Reset Device %x", hr);
			return false;
		}
    }
#endif

    // Setup cursor based on current settings (window/fullscreen mode, show cursor state, clip cursor state)
    SetupCursor(false);

    // Call the app's OnDeviceReset callback
    SetInsideDeviceChange( true );

	// PC TODO - Issue a callback to report device reset properly
    SetInsideDeviceChange( false );
    // Success
    SetInReset( false );
	return true;
}
#endif // __D3D9

#if __WIN32PC && __D3D11
enum ALT_TAB_STATE
{
	ATS_NotActive = 0,
	ATS_FullscreenToMinimize,
	ATS_WaitForMinimized,
	ATS_FullscreenMinimized,
	ATS_MinimizedToWindow,
	ATS_MinimizedToWindowDone,
	ATS_MinimizedToFullscreen,
};

static ALT_TAB_STATE altTabbingState = ATS_NotActive;

#endif

static bool s_bHasStoppedHangDetection = false;

#if RSG_PC
static void dont_handle() {}

void (*grcDevice::DeviceChangeHandler)() = dont_handle;
#endif

static LRESULT CALLBACK grcWindowProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam) 
{
	// NOTE: Depending upon the type of suspend we might not receive a suspend event. If this happens
	//		 Do not send a resume event as some parts of code require the suspend to be sent first.
	static bool bHasSentSuspendMessage = false;

	switch (msg) 
	{
	case WM_DPICHANGED:
		grcDisplayf("WM_DPICHANGED message not handled");
		break;

	case WM_DPICHANGED_AFTERPARENT:
		grcDisplayf("WM_DPICHANGED_AFTERPARENT message not handled");
		break;

	case WM_DPICHANGED_BEFOREPARENT:
		grcDisplayf("WM_DPICHANGED_BEFOREPARENT message not handled");
		break;

	case WM_GETDPISCALEDSIZE:
		grcDisplayf("WM_GETDPISCALEDSIZE message not handled");
		break;

	case WM_NCCREATE:
		{
			OUTPUT_ONLY(CREATESTRUCT* poCreateWin = (CREATESTRUCT*)lParam);
			grcDisplayf("Width %d, Height %d, X %d Y %d", poCreateWin->cx, poCreateWin->cy, poCreateWin->x, poCreateWin->y);
			if (fnEnableNonClientDpiScaling)
				fnEnableNonClientDpiScaling(hwnd);
		}
		break;

	case WM_PAINT:
		if (GRCDEVICE.IsPaused() && GRCDEVICE.IsCreated() && !GRCDEVICE.IsInReset())
		{
			// If you want to support the game paused with the window moving around hook up this call back
			if (!GRCDEVICE.IsOccluded())
			{
				if (GRCDEVICE.GetRenderCallback() != NULL)
				{
					GRCDEVICE.GetRenderCallback()();
				}
				else
				{
					// Displayf("Render Callback is NULL - WM_PAINT will not display properly if you care");
				}
			}
			// sm_uDeviceStatus should contain the results of the last present
			if (GRCDEVICE.DeviceStatus() == D3DERR_DEVICELOST)
			{
				GRCDEVICE.SetLost(true);
			}
			else if (GRCDEVICE.DeviceStatus() == D3DERR_DRIVERINTERNALERROR)
			{
				Warningf("Internal Driver Error - Attempting to recreate device");
				GRCDEVICE.SetLost(true);
			}
		}
		break;

	case WM_SIZE:
		grcDisplayf("WM_SIZE %d", wParam);
		// Ignore initial window creation!
#if __D3D
		if (GRCDEVICE.IsCreated()) 
#endif
		{
			if(SIZE_MINIMIZED == wParam)
			{
				pcdDisplayf("WM_SIZE Window was minimized");
#if __WIN32PC && __D3D11
				if (altTabbingState == ATS_FullscreenToMinimize || altTabbingState == ATS_WaitForMinimized)
				{
					grcDisplayf("Alt-Tabbing mode is now ATS_FullscreenMinimized");
					altTabbingState = ATS_FullscreenMinimized;
				}
#endif
				grcDisplayf("WM_SIZE Game window was minimized");
				if (!GRCDEVICE.GetVideoEncodingOverride())
				{
					GRCDEVICE.SetPaused(true);
					GRCDEVICE.SetRequestPauseMenu(true);
				}
				GRCDEVICE.SetMinimized(true);
				GRCDEVICE.SetMaximized(false);
			}
			else
			{
                RECT rcCurrentClient;
                GetClientRect( g_hwndMain, &rcCurrentClient );
				pcdDisplayf("WM_SIZE current client rectangle is left: %d, right: %d, top: %d, bottom: %d", rcCurrentClient.left, rcCurrentClient.right, rcCurrentClient.top, rcCurrentClient.bottom);
                if( rcCurrentClient.top == 0 && rcCurrentClient.bottom == 0 )
                {
                    // Rapidly clicking the task bar to minimize and restore a window
                    // can cause a WM_SIZE message with SIZE_RESTORED when 
                    // the window has actually become minimized due to rapid change
                    // so just ignore this message
                }
				else if(SIZE_MAXIMIZED == wParam)
				{
					pcdDisplayf("WM_SIZE Window was maximized");
#if __D3D11
					RECT rcCurrentWindow;
					GetWindowRect( g_hwndMain, &rcCurrentWindow );

					GRCDEVICE.ComputeMaximizedWindowDimensions(rcCurrentWindow);
					pcdDisplayf("Set Window Position: %dx%d Dimensions %dx%d in WM_SIZE", rcCurrentWindow.left, rcCurrentWindow.top, rcCurrentWindow.right - rcCurrentWindow.left, rcCurrentWindow.bottom - rcCurrentWindow.top);
					SetWindowPos( g_hwndMain, HWND_TOP, rcCurrentWindow.left, rcCurrentWindow.top, rcCurrentWindow.right - rcCurrentWindow.left, rcCurrentWindow.bottom - rcCurrentWindow.top, SWP_NOZORDER | SWP_FRAMECHANGED);
					GRCDEVICE.SetWindowDragResized(true);
#endif // __D3D11

					grcDisplayf("Game window was maximized");
					GRCDEVICE.SetPaused(false);
					GRCDEVICE.SetMaximized(false);
					GRCDEVICE.SetRecheckDeviceChanges(true);//CheckForDeviceChanges();
				}
				else if(SIZE_RESTORED == wParam)
				{
					pcdDisplayf("WM_SIZE Window was restored");
					if (GRCDEVICE.IsMaximized())
					{
						GRCDEVICE.SetMaximized(false);
						GRCDEVICE.SetRecheckDeviceChanges(true);//CheckForDeviceChanges();
					}
					else if(GRCDEVICE.IsMinimized())
					{
#if __WIN32PC && __D3D11
						if (altTabbingState == ATS_FullscreenMinimized)
						{
							grcDisplayf("Alt-Tabbing mode is now ATS_MinimizedToWindow");
							altTabbingState = ATS_MinimizedToWindow;
						}
#endif
						GRCDEVICE.SetPaused(false);
						GRCDEVICE.SetMinimized(false);
						GRCDEVICE.SetRecheckDeviceChanges(true);//CheckForDeviceChanges();
					}
					else if (GRCDEVICE.IsInSizeMove())
					{
						// Wait until window changes are done
					}
					else if (!GRCDEVICE.GetLostFocus())
					{
						GRCDEVICE.SetRecheckDeviceChanges(true);//CheckForDeviceChanges();
					}
				}
			}
		}
		break;

	case WM_GETMINMAXINFO:
		// Prevent the window from going below a certain size
		if ((((( MINMAXINFO* )lParam )->ptMinTrackSize.x) < MINIMUM_WIDTH) ||
			(((( MINMAXINFO* )lParam )->ptMinTrackSize.y) < MINIMUM_HEIGHT))
		{
			( ( MINMAXINFO* )lParam )->ptMinTrackSize.x = MINIMUM_WIDTH;
			( ( MINMAXINFO* )lParam )->ptMinTrackSize.y = MINIMUM_HEIGHT;
		}
        break;

    case WM_ENTERSIZEMOVE:
		grcDisplayf("WM_ENTERSIZEMOVE: Enter size move");

        // Halt frame movement while the app is sizing or moving
        //GRCDEVICE.SetPaused(true);
		if (!GRCDEVICE.GetVideoEncodingOverride())
		{
			RECT rcCurrentWindow;
			GetWindowRect( g_hwndMain, &rcCurrentWindow );

			LONG width = rcCurrentWindow.right - rcCurrentWindow.left;
			LONG height = rcCurrentWindow.bottom - rcCurrentWindow.top;

			grcDevice::sm_AspectRatio = (float)width / (float)height;

			GRCDEVICE.SetInSizeMove(true);
			SetTimer(g_hwndMain, 1, USER_TIMER_MINIMUM, NULL);
		}
        break;

    case WM_EXITSIZEMOVE:
		grcDisplayf("WM_EXITSIZEMOVE: Exit size move");
        //GRCDEVICE.SetPaused(false);
		{
			int resizable = WIN_RESIZABLE;
			if (PARAM_allowResizeWindow.Get())
			{
				resizable = 1;
			}
			else if (PARAM_disallowResizeWindow.Get())
			{
				resizable = 0;
			}
		}



		KillTimer(g_hwndMain, 1);
        GRCDEVICE.SetRecheckDeviceChanges(true);//CheckForDeviceChanges();
		GRCDEVICE.SetInSizeMove(false);
        break;

	case WM_SIZING:
		{
			pcdDisplayf("WM_SIZING");
#if __D3D11
			GRCDEVICE.SetWindowDragResized(true);
#endif // __D3D11
#if !__FINAL
			if (!PARAM_forceResolution.Get())
#endif
			{
				RECT* windowRect = (RECT*)lParam;

				LONG width = windowRect->right - windowRect->left;
				LONG height = windowRect->bottom - windowRect->top;
				LONG newWidth, newHeight;

				if (wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT)
				{
					newHeight = (LONG)(width / grcDevice::sm_AspectRatio);
					if (newHeight < MINIMUM_HEIGHT)
					{
						newHeight = MINIMUM_HEIGHT;
						newWidth = (LONG)(height * grcDevice::sm_AspectRatio);
						if (wParam == WMSZ_LEFT)
						{
							windowRect->left = windowRect->right - newWidth;
						}
						else
						{
							windowRect->right = windowRect->left + newWidth;
						}
					}
					LONG heightChange = (newHeight - height) / 2;
					windowRect->bottom += heightChange;
					windowRect->top -= heightChange;
				}
				else if (wParam == WMSZ_TOP || wParam == WMSZ_BOTTOM)
				{
					newWidth = (LONG)(height * grcDevice::sm_AspectRatio);
					if (newWidth < MINIMUM_WIDTH)
					{
						newWidth = MINIMUM_WIDTH;
						newHeight = (LONG)(width / grcDevice::sm_AspectRatio);
						if (wParam == WMSZ_TOP)
						{
							windowRect->top = windowRect->bottom - newHeight;
						}
						else
						{
							windowRect->bottom = windowRect->top + newHeight;
						}
					}
					LONG widthChange = (newWidth - width) / 2;
					windowRect->right += widthChange;
					windowRect->left -= widthChange;
				}
				else
				{
					newHeight = (LONG)(width / grcDevice::sm_AspectRatio);
					newWidth = (LONG)(height * grcDevice::sm_AspectRatio);

					RECT heightRect = *windowRect;
					switch (wParam)
					{
					case WMSZ_BOTTOM:
					case WMSZ_BOTTOMLEFT:
					case WMSZ_BOTTOMRIGHT:
						heightRect.bottom = heightRect.top + newHeight;
						break;
					case WMSZ_TOP:
					case WMSZ_TOPLEFT:
					case WMSZ_TOPRIGHT:
						heightRect.top = heightRect.bottom - newHeight;
						break;
					case WMSZ_LEFT:
					case WMSZ_RIGHT:
						heightRect.top = heightRect.bottom - (newHeight);
						break;
					}


					RECT widthRect = *windowRect;
					switch (wParam)
					{
					case WMSZ_LEFT:
					case WMSZ_BOTTOMLEFT:
					case WMSZ_TOPLEFT:
						widthRect.left = widthRect.right - newWidth;
						break;
					case WMSZ_RIGHT:
					case WMSZ_BOTTOMRIGHT:
					case WMSZ_TOPRIGHT:
						widthRect.right = widthRect.left + newWidth;
						break;
					case WMSZ_TOP:
					case WMSZ_BOTTOM:
						widthRect.left = widthRect.right - newWidth;
						break;
					}


					CURSORINFO Info;
					memset(&Info, 0, sizeof(CURSORINFO));
					Info.cbSize = sizeof(CURSORINFO);
					::GetCursorInfo(&Info);

					if ((widthRect.left < Info.ptScreenPos.x) &&  (widthRect.right > Info.ptScreenPos.x))
					{
						*windowRect = widthRect;
					}
					else
					{
						*windowRect = heightRect;
					}
				}
			}
		}
		break;

	case WM_TIMER:
		grcDisplayf("WM_TIMER: this should mean we are in a WM_SIZEMOVE state");
		if (GRCDEVICE.IsInSizeMove())
		{
			// Calling the render function here can cause a deadlock as the control thread will be waiting for this message pump to finish. To stop this
			// inform the control thread to continue as the game is not using the input.
			ioInput::SignalMessagePumpUpdate();
			GRCDEVICE.RenderFunctionCallback();
		}
		break;

	case WM_DISPLAYCHANGE:
		grcDisplayf("WM_DISPLAYCHANGE: Display Mode Change %d * %d (%d)", (lParam & 0xFFFF), (lParam >> 16), wParam);
		//GRCDEVICE.CheckForDeviceChanges();
		break;

#if RSG_PC
	case WM_DEVICECHANGE:
		{
 			if (wParam == DBT_DEVNODES_CHANGED)
 			{
				GRCDEVICE.DeviceChangeHandler();
 				grcDisplayf("Device added or removed from the system");
 			}
			grcDisplayf("WM_DEVICECHANGE %d", wParam);
		}
		break;
		// NOTE: Do not return here, this event needs to be passed onto g_WindowProc().
#endif
#if NV_SUPPORT
	case WM_STEREO_CHANGE:
		if (GRCDEVICE.IsStereoChangeEnabled())
		{
			bool bIgnoreStereoMsgConv = GRCDEVICE.GetIgnoreStereoMsg();

			// Currently just want to spam when stereo mode is activated/deactivated.
			// At some point we might have to take action when it gets activated/deactivated.
			WORD l = LOWORD(wParam);
			WORD h = HIWORD(wParam);

			float fSep = Clamp(float(h*100.f/0xFFFF),1.0f,100.0f);
			float fConv = *(float*)&lParam;

			if (s_gAltTabbingState)
			{
				// we want to retain the current sep/conv setting when alt-tabbed
				// device reset is causing nvidia driver to reset sep/conv values to profile values
				fConv = GRCDEVICE.GetDefaultConvergenceDistance();
				fSep = GRCDEVICE.GetDefaultSeparationPercentage();
				s_gAltTabbingState = false;
			}

			if (l == 0)
			{
				Displayf("Stereo mode de-activated!!");
				GRCDEVICE.ActivateStereo(false);
				GRCDEVICE.SetStereoTexture();
				GRCDEVICE.ChangeStereoSep(-1.0f);
			}
			else
			{
				Displayf("Stereo mode activated!!");
				GRCDEVICE.ActivateStereo(true);

				if (!bIgnoreStereoMsgConv)
				{
					GRCDEVICE.SetDefaultConvergenceDistance(fConv);
					GRCDEVICE.ChangeStereoConv(fConv);
					GRCDEVICE.SetDesiredConvergence(fConv);
				}

				GRCDEVICE.SetSeparationPercentage(fSep);
				GRCDEVICE.ChangeStereoSep(fSep);
				GRCDEVICE.SetDesiredSeparation(fSep);

				GRCDEVICE.UpdateStereoStatus();
			}
		}
		break;
#endif

	case WM_DWMCOMPOSITIONCHANGED:
		grcDisplayf("WM_DWMCOMPOSITIONCHANGED: Desktop Window Manager (DWM) composition changed.");
		break;

    case WM_MOUSEMOVE:
        if( GRCDEVICE.IsCreated() && GRCDEVICE.IsActive() && !GRCDEVICE.IsWindowed() )
        {
#if __D3D9
                if( GRCDEVICE.GetCurrent() )
                {
                    int x, y;
					ioMouse::GetPlatformCursorPosition(&x, &y);
                    GRCDEVICE.GetCurrent()->SetCursorPosition( x, y, 0 );
                }
#endif
                // For D3D10, no processing is necessary.  D3D10 cursor
                // is handled in the traditional Windows manner.
        }
        break;

    case WM_SETCURSOR:
			//grcDisplayf("WM_SETCURSOR %d", wParam);
			if( GRCDEVICE.IsCreated())
			{
				if (GRCDEVICE.IsActive())
				{
					if (!GRCDEVICE.IsWindowed())
					{
	#if __DEV
		#if !__D3D11
						if (1) // GRCDEVICE.GetDxVersion() < 1000)
						{
							GRCDEVICE.GetCurrent()->ShowCursor( true );
						}
						else
		#endif // !__D3D11
						{
							//SetCursor( NULL );
						}
	#else
						// Fullscreen... no cursor for you!
						// SetCursor( NULL );
	#endif // __DEV
						GRCDEVICE.SetupCursor( false );
					}
					else
					{
						if (MOUSE.IsCurrentlyExclusive())
						{
							// Mouse exclusive... no cursor for you!
							// SetCursor( NULL );
							GRCDEVICE.SetupCursor( false );
						}
					}
				}
				else
				{
					GRCDEVICE.SetupCursor( false );
				}
				//return true; // prevent Windows from setting cursor to window class cursor
			}
			if (GRCDEVICE.GetLostFocus())
			{
				GRCDEVICE.SetupCursor( true );
			}
        break;

	case WM_SYSCOMMAND:
		if( wParam == SC_SCREENSAVE || wParam  == SC_MONITORPOWER )  // Prevents Screen saver and Monitorpower to activate CO.
		{
			return 0;
		}

#if __STEAM_BUILD
		if (g_SysService.IsUiOverlaid() && (wParam & 0xFFF0) != SC_RESTORE) /* do not ignore restore requests */
		{
			Displayf("Windows event [wparam: %d] [sc: %d] skipped due to Ui Overlaid", wParam, wParam & 0xFFF0);
			return 0; 
		}
#endif

		// don't post the system menu when hitting ALT.
		if ((wParam & 0xFFF0) == SC_KEYMENU && !GRCDEVICE.GetLostFocus())
			return 0;

        // Prevent moving/sizing in full screen mode - If you want to allow system menu
        switch( ( wParam & 0xFFF0 ) )
        {
            case SC_MOVE:
            case SC_SIZE:
            case SC_MAXIMIZE:
            case SC_KEYMENU:
				{
					if( !GRCDEVICE.IsWindowed() )
						return 0;
				}
				break;
        }
		break;

	case WM_CLOSE:
		//GRCDEVICE.SetLostFocus(false);		// so game doesn't need to be activated first before it will quit
		//GRCDEVICE.SetPaused(true);
		s_Closed = true;
		return 0;

#if __WIN32PC
	case WM_WINDOWPOSCHANGED:
		{
			OUTPUT_ONLY(const WINDOWPOS* pWinPos = (const WINDOWPOS*)lParam);
			grcDisplayf("WM_WINDOWPOSCHANGED - Width %d, Height %d, X %d Y %d", pWinPos->cx, pWinPos->cy, pWinPos->x, pWinPos->y);
			if (GRCDEVICE.IsCreated() && GRCDEVICE.IsActive() && !GRCDEVICE.IsInSizeMove())
				GRCDEVICE.SetRecheckDeviceChanges(true);//CheckForDeviceChanges();
		}
		break;

	case WM_WTSSESSION_CHANGE:
		switch( wParam )
		{
		case WTS_CONSOLE_CONNECT:
			grcDisplayf("Console connected");
			ShowWindow(g_hwndMain, SW_MINIMIZE);
			break;
		case WTS_CONSOLE_DISCONNECT:
			grcDisplayf("Console disconnected");
			break;
		default:
			break;
		}
		break;

#endif
	case WM_ACTIVATEAPP:
		grcDisplayf ("WM_ACTIVATEAPP %d", wParam);
		// Use ACTIVATEAPP, not ACTIVATE, so that we don't grind to a halt whenever a bank gets focus.
		if ((wParam == TRUE) && !GRCDEVICE.IsActive()) 
		{	// Regaining focus
			pcdDisplayf("Gaining Focus");
#if __BANK
			if (GRCDEVICE.IsWindowed())
			{
				if ( bkManager::IsEnabled() && !bkRemotePacket::IsConnectedToRag() )
				{
					BANKMGR.RaiseAllBanks(false);
				}
				::SetFocus(hwnd);
			}
#endif
			GRCDEVICE.SetActive(true);
			GRCDEVICE.SetLostFocus(false);
			SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);

			// Restart the hang detection.
			if(s_bHasStoppedHangDetection)
			{
				HANG_DETECT_SAVEZONE_EXIT(NULL);
				s_bHasStoppedHangDetection = false;
			}

			TIME.ResetInternalTimer();
/*			if ( GRCDEVICE.IsCreated() &&
				(!GRCDEVICE.IsWindowed() || MOUSE.IsCurrentlyExclusive()) )
			{
				GRCDEVICE.SetupCursor( false );
			}*/
			// PC TODO - Activate control rumble

/*			if (GRCDEVICE.IsMinimizedWhileFullscreen())
			{
				if (__D3D9)
				{
					GRCDEVICE.SetPaused(false);
				}
				GRCDEVICE.SetMinimizedWhileFullscreen(false);

				if (__D3D11)
				{
					if (GRCDEVICE.IsWindowed())
					{
						GRCDEVICE.GoFullscreen();
					}
				}
			}*/				
			#if __WIN32PC
				if (GRCDEVICE.IsWindowed() && MOUSE.IsExclusiveSet())
				{
					MOUSE.DeviceResetCB();
					GRCDEVICE.SetupCursor( false );
				}
				else// if (!GRCDEVICE.IsWindowed())
				{
					GRCDEVICE.SetupCursor( false );
				}
			#endif // __WIN32PC
		}
		else if ((wParam == FALSE) && GRCDEVICE.IsActive())
		{	// Losing focus
			pcdDisplayf("Losing Focus");
			GRCDEVICE.SetActive(false);
			GRCDEVICE.SetLostFocus(true);
			SetThreadExecutionState(ES_CONTINUOUS);

			ioMouse::FlushMouse();
			ioKeyboard::FlushKeyboard();

			// As the PC version does not update the screen when we do not have focus, the hang detection thread thinks we have hung.
			if(!s_bHasStoppedHangDetection)
			{
				HANG_DETECT_SAVEZONE_ENTER();
				s_bHasStoppedHangDetection = true; // only update the counter once.
			}
#if __WIN32PC && __D3D11
#if __FINAL
			if(GRCDEVICE.IsAllowPauseOnFocusLoss())
#else
			if (PARAM_BlockOnLostFocus.Get() && GRCDEVICE.IsAllowPauseOnFocusLoss())
#endif
			{
				GRCDEVICE.SetRequestPauseMenu(true);
			}
			// PC TODO - Disable Controller Rumble

			BOOL bFullScreen = false;
			((IDXGISwapChain*)GRCDEVICE.GetSwapChain())->GetFullscreenState(&bFullScreen, NULL);

			if (altTabbingState == ATS_NotActive && !g_inWindow /*(bFullScreen != 0)*/ && __D3D11)
			{
				GRCDEVICE.SetBusyAltTabbing(true);
				GRCDEVICE.SuppressAltEnter();
				grcDisplayf("Alt-Tabbing mode is now ATS_FullscreenToMinimize");
				altTabbingState = ATS_FullscreenToMinimize;
			}
#endif

			if (!GRCDEVICE.IsWindowed())
			{
                // Going from full screen to a minimized state 
                ClipCursor( NULL );      // don't limit the cursor anymore
				if (__D3D9)
				{
					GRCDEVICE.SetPaused(true);
				}
				GRCDEVICE.SetMinimizedWhileFullscreen(true);
			}
			#if __WIN32PC
				GRCDEVICE.SetupCursor( true );
				if (GRCDEVICE.IsWindowed() && MOUSE.IsExclusiveSet())
				{
					MOUSE.DeviceLostCB();
				}
			#endif // __WIN32PC
		}
#if __WIN32PC
		if (GRCDEVICE.IsCreated() && GRCDEVICE.IsActive() && !GRCDEVICE.IsInSizeMove())
		{
			GRCDEVICE.SetRecheckDeviceChanges(true);//CheckForDeviceChanges();
		}
#endif
		break;

#if __WIN32PC && __D3D11
	case WM_ACTIVATE:
		grcDisplayf ("WM_ACTIVATE %d", wParam);
		if (altTabbingState == ATS_MinimizedToWindow)
		{
			grcDisplayf("Alt-Tabbing mode is now ATS_MinimizedToWindowDone");
			altTabbingState = ATS_MinimizedToWindowDone;
		}
		break;
#endif


    case WM_ENTERMENULOOP:
        // Pause the app when menus are displayed
        GRCDEVICE.SetPaused(true);
        break;

    case WM_EXITMENULOOP:
        GRCDEVICE.SetPaused(false);
		break;

    case WM_NCHITTEST:
        // Prevent the user from selecting the menu in full screen mode
        if( !GRCDEVICE.IsWindowed() )
            return HTCLIENT;
        break;

    case WM_POWERBROADCAST:
        switch( wParam )
        {
#ifndef PBT_APMQUERYSUSPEND
#define PBT_APMQUERYSUSPEND 0x0000
#endif
			case PBT_APMQUERYSUSPEND:
				Displayf("Entering Suspend mode - Callback for this?");
				// At this point, the app should save any data for open
				// network connections, files, etc., and prepare to go into
				// a suspended mode.  The app can use the MsgProc callback
				// to handle this if desired.

				return true;

#ifndef PBT_APMSUSPEND
#define PBT_APMSUSPEND 0x0004
#endif
			case PBT_APMSUSPEND:
				if(bHasSentSuspendMessage == false)
				{
					sysServiceEvent e1(sysServiceEvent::SUSPEND_IMMEDIATE);
					g_SysService.TriggerEvent(&e1);

					// We need to send both events.
					sysServiceEvent e2(sysServiceEvent::SUSPENDED);
					g_SysService.TriggerEvent(&e2);

					bHasSentSuspendMessage = true;
				}

				return true;

#ifndef PBT_APMRESUMESUSPEND
#define PBT_APMRESUMESUSPEND 0x0007
#endif
            case PBT_APMRESUMESUSPEND:
				if(bHasSentSuspendMessage)
				{
					sysServiceEvent e1(sysServiceEvent::RESUME_IMMEDIATE);
					g_SysService.TriggerEvent(&e1);

					// We need to send both events.
					sysServiceEvent e2(sysServiceEvent::RESUMING);
					g_SysService.TriggerEvent(&e2);

					bHasSentSuspendMessage = false;
				}

				Displayf("Resuming from Suspend Mode - Callback for this?");
				// At this point, the app should recover any data, network
				// connections, files, etc., and resume running from when
				// the app was suspended. The app can use the MsgProc callback
				// to handle this if desired.

				// QPC may lose consistency when suspending, so reset the timer
				// upon resume.
				TIME.ResetInternalTimer();
				return true;
		}
        break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN && GRCDEVICE.IsFullscreenToggleEnable()) 
		{
			if (__D3D9)
			{
				DWORD dwMask = (1 << 29);
                if( ( lParam & dwMask ) != 0 ) // Alt is down also
                {
					GRCDEVICE.SetPaused(true);
					GRCDEVICE.ToggleFullscreen();
					GRCDEVICE.SetPaused(false);
					return 0;
				}
			}
		}
		break;
//     case WM_KEYDOWN:
// 		{
// 			switch( wParam )
// 			{
// 				case VK_PAUSE:
// 				{
// 					// Comment out if you don't want this to pause rendering                
// 					{
// 						GRCDEVICE.SetPaused(!GRCDEVICE.IsPaused());
// 					}
// 					break;
// 				}
// 			}
// 			break;
// 		}
	}
	return g_WindowProc(hwnd,msg,wParam,lParam);
}

#if __D3D11
void grcDevice::ComputeMaximizedWindowDimensions(RECT &rect)
{
	const grcAdapterD3D11* adapter = (grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(0);
	const grcAdapterD3D11Output* monitorOutput = adapter->GetOutput(0);
	DXGI_OUTPUT_DESC bestDesc;
	unsigned int dpiX, dpiY;
	grcAdapterD3D11Output::GetDesc(adapter->GetHighPart(), adapter->GetLowPart(), monitorOutput, bestDesc, dpiX, dpiY);

	for (int adapterIndex = 0; adapterIndex < grcAdapterManager::GetInstance()->GetAdapterCount(); adapterIndex++)
	{
		const grcAdapterD3D11* adapter = (grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(adapterIndex);
		for (int monitorIndex = 0; monitorIndex < adapter->GetOutputCount(); monitorIndex++)
		{
			const grcAdapterD3D11Output* monitorOutput = adapter->GetOutput(monitorIndex);
			DXGI_OUTPUT_DESC desc;
			unsigned int dpiX, dpiY;
			grcAdapterD3D11Output::GetDesc(adapter->GetHighPart(), adapter->GetLowPart(), monitorOutput, desc, dpiX, dpiY);
			int bestDistFromCenter = abs((bestDesc.DesktopCoordinates.left - rect.left) + (bestDesc.DesktopCoordinates.right - rect.right)) + abs((bestDesc.DesktopCoordinates.top - rect.top) + (bestDesc.DesktopCoordinates.bottom - rect.bottom));
			int descDistFromCenter = abs((desc.DesktopCoordinates.left - rect.left) + (desc.DesktopCoordinates.right - rect.right)) + abs((desc.DesktopCoordinates.top - rect.top) + (desc.DesktopCoordinates.bottom - rect.bottom));

			if (descDistFromCenter < bestDistFromCenter)
			{
				bestDesc = desc;
			}
		}
	}

	rect.right = bestDesc.DesktopCoordinates.right;
	rect.left = bestDesc.DesktopCoordinates.left;
	rect.top = bestDesc.DesktopCoordinates.top;
	rect.bottom = bestDesc.DesktopCoordinates.bottom;

	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;
	if (height > width)
	{
		height = (u32)((float)width / grcDevice::sm_minAspectRatio); 
		rect.top = max(bestDesc.DesktopCoordinates.top, (bestDesc.DesktopCoordinates.top + bestDesc.DesktopCoordinates.bottom)/2 - height/2);
		rect.bottom = rect.top + height;
	}
}

bool grcDevice::ComputeWindowDimensions(RECT &rect, unsigned int windowFlags)
{
	pcdDisplayf("Pre-ComputeWindowDimensions LT: %dx%d RB: %dx%d in ChangeDevice", rect.left, rect.top, rect.right, rect.bottom);

	bool modifiedWindowSettings = false;

	const grcAdapterD3D11* adapter = (grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(sm_AdapterOrdinal);
	const grcAdapterD3D11Output* monitorOutput = adapter->GetOutput(sm_OutputMonitor);

	DXGI_OUTPUT_DESC desc;
	unsigned int dpiX, dpiY;
	grcAdapterD3D11Output::GetDesc(adapter->GetHighPart(), adapter->GetLowPart(), monitorOutput, desc, dpiX, dpiY);
	DWORD exFlags = g_inWindow&&g_isTopMost ? WS_EX_TOPMOST : 0;

	::AdjustWindowRectEx(&rect, windowFlags, false, exFlags);

	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;

	if (!GetIgnoreWindowLimits()
#if !__FINAL
	&& (!PARAM_forceResolution.Get())
#endif
	)
	{
		if (height > width)
		{
			sm_CurrentWindows[g_RenderThreadIndex].bFullscreen = false;
			g_inWindow = true;
			rect.right = desc.DesktopCoordinates.right;
			rect.left = desc.DesktopCoordinates.left;
			width = rect.right - rect.left;
			height = (u32)((float)width / grcDevice::sm_minAspectRatio); 

			modifiedWindowSettings = true;
		}

		LONG desktopWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
		LONG desktopHeight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;

		if (width > desktopWidth)
		{
			rect.left += (width-desktopWidth) / 2;
			rect.right -= (width-desktopWidth) / 2;
			width = rect.right - rect.left;
		}
		if (height > desktopHeight)
		{
			rect.top += (height-desktopHeight) / 2;
			rect.bottom -= (height-desktopHeight) / 2;
			height = rect.bottom - rect.top;
		}
	}

	int coords[2];
	if (PARAM_setWindowPosition.GetArray(coords, 2))
	{
		rect.left = coords[0];
		rect.top = coords[1];
		rect.right = rect.left + width;
		rect.bottom = rect.top + height;
	}
	else
	{
		rect.left = (desc.DesktopCoordinates.left + desc.DesktopCoordinates.right)/2 - width/2;
		rect.right = rect.left + width;
		rect.top = max(desc.DesktopCoordinates.top, (desc.DesktopCoordinates.top + desc.DesktopCoordinates.bottom)/2 - height/2);
		rect.bottom = rect.top + height;
	}
	pcdDisplayf("Post-ComputeWindowDimensions LT: %dx%d RB: %dx%d in ChangeDevice", rect.left, rect.top, rect.right, rect.bottom);
	return modifiedWindowSettings;
}
#endif

HWND grcDevice::CreateDeviceWindow(HWND parent) 
{
	static HINSTANCE hShCore = LoadLibrary("Shcore.dll");
	if (hShCore)
	{
		fnSetProcessDpiAwareness = (LPSetProcessDpiAwareness)GetProcAddress(hShCore, "SetProcessDpiAwareness");
		if (fnSetProcessDpiAwareness) {
			/*HRESULT hr =*/ fnSetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
			/* E_ACCESSDENID - The DPI awareness is already set, either by calling this API previously or through the application (.exe) manifest. 
			if (hr != S_OK) {
				char buff[256];
				diagErrorCodes::Win32ErrorToString(GetLastError(), buff, NELEM(buff));
				pcdAssertf(hr == S_OK, "Set Process DPI Awareness failed %ld - %s", GetLastError(), buff);
			}
			*/
		}
	}

	// Look up from user.dll
	static HINSTANCE hUserDLL = LoadLibrary("user32.dll");
	if (hUserDLL)
	{
		fnEnableNonClientDpiScaling = (LPEnableNonClientDpiScaling)GetProcAddress(hUserDLL, "EnableNonClientDpiScaling");
	}

	int minimizable = WIN_MINIMIZABLE;
	int maximizable = WIN_MAXIMIZABLE;
	int resizable = WIN_RESIZABLE;
	int closeable = WIN_CLOSEABLE;

#if !__FINAL
	PARAM_minimizableWindow.Get(minimizable);
	PARAM_maximizableWindow.Get(maximizable);
	PARAM_closeableWindow.Get(closeable);
#endif
	if (PARAM_allowResizeWindow.Get())
	{
		resizable = 1;
	}
	else if (PARAM_disallowResizeWindow.Get())
	{
		resizable = 0;
	}

	DWORD flags = WS_CAPTION; 

	if (closeable) flags |= WS_SYSMENU;
	if (minimizable) flags |= WS_MINIMIZEBOX | WS_SYSMENU;
	if (maximizable) flags |= WS_MAXIMIZEBOX | WS_SYSMENU;
	if (resizable) flags |= WS_THICKFRAME;

	SetWindowFlags(flags);

	// Since UNICODE is undefined, we will explicitly call the Unicode functions 
	// RegisterClassW and CreateWindowExW to create the window as a Unicode window; this
	// will allow us to generate the proper WM_CHAR messages to support the TCR.
	static WCHAR grcWindow[] = L"grcWindow";

	if (!g_winClass) 
	{
        g_hInstance = ( HINSTANCE )GetModuleHandle( NULL );

		WNDCLASSW wc;
		memset(&wc,0,sizeof(wc));
        wc.style = CS_DBLCLKS; // CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = grcWindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = g_hInstance;
        wc.hIcon = /* gfxIcon? LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(gfxIcon)) : */ LoadIcon(::GetModuleHandle(0), IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL /*::GetModuleHandle(0)*/, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); //CreateSolidBrush(0);		// always black now
        wc.lpszMenuName = NULL;		// MAKEINTRESOURCE(gfxMenuBar);
        wc.lpszClassName = grcWindow;
		g_winClass = ::RegisterClassW(&wc);
		if (!g_winClass)
		{
			rage::diagErrorCodes::SetExtraReturnCodeNumber(GetLastError());
			Quitf(ERR_GFX_WIN,"Failed to RegisterClass - Error Code %d", GetLastError());
			return NULL;
		}
	}

	HWND effParent = g_inWindow? parent : 0;


#if __D3D11
	if (PARAM_borderless.Get())
	{
		SetDesireBorderless(true);
	}
	SetBorderless(IsDesireBorderless());

//	if (!PARAM_disallowResizeWindow.Get())
	{
		if (effParent)
		{
			flags = WS_CHILD;
			sm_WindowStyle = WS_CHILD;
		}
		else if (IsDesireBorderless())
		{
			flags = WS_POPUP;
			sm_WindowStyle = WS_CAPTION;
		}
		else
		{
			sm_WindowStyle = WS_CAPTION;
		}
	}
// 	else
// 	{
// 		flags = (/*g_inWindow && */!IsDesireBorderless())? (effParent? WS_CHILD : (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)) : WS_POPUP | WS_SYSMENU;
// 		sm_WindowStyle = (effParent || IsDesireBorderless()) ? WS_CHILD : (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
// 	}
#else
	flags = g_inWindow? (effParent? WS_CHILD : WS_OVERLAPPEDWINDOW) : WS_POPUP | WS_SYSMENU;
	sm_WindowStyle = (effParent? WS_CHILD : WS_OVERLAPPEDWINDOW);
#endif
	DWORD exFlags = g_inWindow&&g_isTopMost ? WS_EX_TOPMOST : 0;

	RECT rect = {0,0,sm_CurrentWindows[g_RenderThreadIndex].uWidth,sm_CurrentWindows[g_RenderThreadIndex].uHeight};

	bool forcedToWindowSettings = false;
	int coords[2];

#if __D3D11
	if ((!PARAM_rag.Get() && !PARAM_setHwndMain.Get()) || PARAM_ragUseOwnWindow.Get())
	{
		forcedToWindowSettings = ComputeWindowDimensions(rect, flags);
	}
	else
#endif
	{
		HDC hdc = ::GetDC(0);
		int sw = ::GetDeviceCaps(hdc,HORZRES);
		int sh = ::GetDeviceCaps(hdc,VERTRES);
		::ReleaseDC(0,hdc);

		RECT rc;
		::SetRect(&rc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);
		::AdjustWindowRectEx(&rc, flags, false, exFlags);

		int x = effParent? 0 : (sw - (rect.right - rect.left)) >> 1;
		int y = effParent? 0 : (sh - (rect.bottom - rect.top)) >> 1;

		// ensure the title bar is visible.
		if(x < 0)
		{
			x = 0;
		}
		if(y < 0)
		{
			y = 0;
		}

		if (PARAM_setWindowPosition.GetArray(coords, 2))
		{
			rect.left += coords[0];
			rect.right += coords[0];
			rect.top += coords[1];
			rect.bottom += coords[1];
		}
		else
		{
			rect.right += rect.left - x;
			rect.left = x;
			rect.bottom += rect.top - y;
			rect.top = y;
		}
	}
	Displayf("Adjusted Window Size %d %d Offset %d %d", rect.right - rect.left, rect.bottom - rect.top, rect.left, rect.top);

	USES_CONVERSION;
	const wchar_t* title = reinterpret_cast<const wchar_t*>(UTF8_TO_WIDE(s_WindowTitle));
	HWND result = ::CreateWindowExW(g_inWindow&&g_isTopMost?WS_EX_TOPMOST:0,grcWindow,title,flags,
		rect.left,rect.top,rect.right - rect.left,rect.bottom - rect.top,effParent,(HMENU)NULL /*gfxMenuBarHandle*/,NULL,NULL);
#if !__FINAL
	if (!result) {
		DWORD code = GetLastError();
		grcErrorf("Could not create main window. Error code %d\n", code);
	}
	// force graphics mode even if in console app
	if( !PARAM_hidewindow.Get() )
#endif
	{
		::ShowWindow(result,SW_NORMAL);
		::UpdateWindow(result);
		::SetFocus(result);
	}

	if (!sm_CurrentWindows[g_RenderThreadIndex].bFullscreen)
	{
	RECT rcCurrentClient;
	GetClientRect( result, &rcCurrentClient );

	sm_CurrentWindows[g_RenderThreadIndex].uWidth = rcCurrentClient.right - rcCurrentClient.left;
	sm_CurrentWindows[g_RenderThreadIndex].uHeight = rcCurrentClient.bottom - rcCurrentClient.top;
	}
	sm_GlobalWindow = sm_CurrentWindows[g_RenderThreadIndex];
	if (forcedToWindowSettings)
		sm_DesiredWindow = sm_CurrentWindows[g_RenderThreadIndex];

	SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);

	return result;
}

bool grcDevice::IsWindowed()
{
	return g_inWindow;
}

#if __D3D9
void grcDevice::SetDepthBoundsTestEnable(u32 D3D9_ONLY(enable))
{
	if (!SupportsFeature(DEPTH_BOUNDS))
		return;

	if (enable)
	{
		GRCDEVICE.GetCurrent()->SetRenderState(D3DRS_ADAPTIVETESS_X,MAKEFOURCC('N','V','D','B')); 		
	}
	else
	{
		GRCDEVICE.GetCurrent()->SetRenderState(D3DRS_ADAPTIVETESS_X,0);
	}
}

void grcDevice::SetDepthBounds(float D3D9_ONLY(zmin), float D3D9_ONLY(zmax))
{
	if (!SupportsFeature(DEPTH_BOUNDS))
		return;

	GRCDEVICE.GetCurrent()->SetRenderState(D3DRS_ADAPTIVETESS_Z,*(DWORD*)&zmin); 
	GRCDEVICE.GetCurrent()->SetRenderState(D3DRS_ADAPTIVETESS_W,*(DWORD*)&zmax); 
}
#endif

bool grcDevice::InitializeFeatureSet()
{
	sm_uFeatures = 0;
#if __D3D9
#if NV_SUPPORT
	DeviceManufacturer eManufacturer = GetManufacturer();
#endif

	IDirect3D9* pDevice;
	if (FAILED(GRCDEVICE.GetCurrent()->GetDirect3D(&pDevice)))
	{
		Printf("Unable to retrieve D3D Device");
		Quitf("D3D Error - Please re-boot your system");
		// return false;
	}

	/*
	VERTEX_TEXTURE_SAMPLING					= 0x1,
	ZSAMPLE_STENCILOPERATIONS_SUPPORT		= 0x2,
	STRAIGHT_RAW_SAMPLE						= 0x4,
	ZSAMPLE_FROM_VERTEX_SHADER				= 0x8,
	Z_MSAA_SAMPLE							= 0x10,
	TESSELATION								= 0x20,
	AUTOSTEREO								= 0x40,
	DEPTH_BOUNDS							= 0x80
	MULTIGPU								= 0x100
	*/

	if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
					D3DDEVTYPE_HAL,
					D3DFMT_X8R8G8B8,
					D3DUSAGE_RENDERTARGET | D3DUSAGE_QUERY_VERTEXTEXTURE,
					D3DRTYPE_TEXTURE,
					D3DFMT_A32B32G32R32F)))
	{
		// this card doesn't support sampling textures in vertex shaders
		sm_uFeatures |= VERTEX_TEXTURE_SAMPLING;
	}
	else if ( FAILED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
					D3DDEVTYPE_HAL,
					D3DFMT_X8R8G8B8,
					D3DUSAGE_DEPTHSTENCIL,
					D3DRTYPE_TEXTURE,
		 			(D3DFORMAT)(MAKEFOURCC('R','A','W','Z')))))
	{
		sm_uFeatures |= PROPER_ZSAMPLE;		
		sm_uFeatures |= ZSAMPLE_FROM_VERTEX_SHADER;
	}

	if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
					D3DDEVTYPE_HAL,
					D3DFMT_X8R8G8B8,
					D3DUSAGE_DEPTHSTENCIL,
					D3DRTYPE_TEXTURE,
		 			(D3DFORMAT)(MAKEFOURCC('I','N','T','Z')))))
	{
		sm_uFeatures |= ZSAMPLE_STENCILOPERATIONS_SUPPORT;
	}

	if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
					D3DDEVTYPE_HAL,
					D3DFMT_X8R8G8B8,
					D3DUSAGE_DEPTHSTENCIL,
					D3DRTYPE_TEXTURE,
		 			(D3DFORMAT)(MAKEFOURCC('D','F','2','4')))))
	{
		sm_uFeatures |= PROPER_ZSAMPLE;
	}

#if NV_SUPPORT
	if (eManufacturer == NVIDIA)
	{
		NvAPI_Status status = NVAPI_ERROR;

		NvU8 isStereoDriverEnabled;
		status = NvAPI_Stereo_IsEnabled( &isStereoDriverEnabled );
		// GeForce Stereoscopic 3D driver is not installed on the system		
		status = NvAPI_Stereo_Enable();
		if ( status == NVAPI_OK)
		{
			sm_uFeatures |= AUTOSTEREO;
		}
	}

	if(pDevice->CheckDeviceFormat(GRCDEVICE.GetAdapterOrdinal(), D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0, D3DRTYPE_SURFACE, (D3DFORMAT)MAKEFOURCC('N','V','D','B') ) == S_OK) 
	{ 
		sm_uFeatures |= DEPTH_BOUNDS;
	}

	// Multi-GPU
	sm_uFeatures |= (GetGPUCount() > 1) ? MULTIGPU : 0;
	
	if (SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
					D3DDEVTYPE_HAL,
					D3DFMT_X8R8G8B8,
					0,
					D3DRTYPE_SURFACE,
					(D3DFORMAT)MAKEFOURCC('R','2','V','B'))))
	{
		sm_uFeatures |= RENDER_TO_VERTEX_BUFFER;
	}
#endif

	// Clean up
	pDevice->Release();
#endif // __D3D9
	return true;
}
#endif	// __WIN32PC

#if __WIN32PC

#if NV_SUPPORT
void grcDevice::ChangeStereoSep(float fSep)
{
	if (sm_StereoSepChangeCb)
		sm_StereoSepChangeCb(fSep);
}

void grcDevice::ChangeStereoConv(float fConv)
{
	if (sm_StereoConvChangeCb)
		sm_StereoConvChangeCb(fConv);
}
#endif

bool grcDevice::StereoIsPossible()
{
#if NV_SUPPORT
	LockContextNVStereo();

	NvAPI_Status status = NVAPI_ERROR;

	NvU8 originalStereoEnabled = false;
	status = NvAPI_Stereo_IsEnabled(&originalStereoEnabled);

	NvAPI_Stereo_Enable();

	NvU8 isStereoEnabled = false;
	status = NvAPI_Stereo_IsEnabled(&isStereoEnabled);

	if (!originalStereoEnabled)
	{
		NvAPI_Stereo_Disable();
	}

	UnlockContextNVStereo();

	if (status == NVAPI_OK)
	{
		return isStereoEnabled ? true : false;
	}
	else
	{
		return false;
	}
#else // NV_SUPPORT
	return false;
#endif
}

void grcDevice::UpdateStereoStatus()
{
#if NV_SUPPORT
	Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

	if (GetManufacturer() == NVIDIA)
	{
		NvAPI_Status status = NVAPI_ERROR;
		NvU8 isStereoEnabled = false;
		NvU8 isStereoActivated = false;

		status = NvAPI_Stereo_IsEnabled(&isStereoEnabled);

		if (isStereoEnabled && sm_pStereoHandle)
		{
			status = NvAPI_Stereo_IsActivated(sm_pStereoHandle, &isStereoActivated);
			if ((status == NVAPI_OK) && isStereoActivated)
			{
				GRCDEVICE.GetConvergenceDistance();
				GRCDEVICE.GetSeparationPercentage();
			}
		}

		sm_bStereoEnabled = isStereoActivated ? true : false;
	}
#endif	
}

void grcDevice::InitializeStereoSystem(bool 
  #if NV_SUPPORT
	useStereo
  #endif
	)
{
#if NV_SUPPORT
	if (GetManufacturer() == NVIDIA)
	{
		NvAPI_Status status = NVAPI_ERROR;
		NvU8 isStereoEnabled = false;
		status = NvAPI_Stereo_IsEnabled(&isStereoEnabled);

		if (status != NVAPI_OK)
		{
			isStereoEnabled = false;
		}

		if (isStereoEnabled)
		{
			if (useStereo)
			{
				sm_bCanUseStereo = true;
				sm_bStereoEnabled = true;
				ASSERT_ONLY(NvAPI_Status status = )NvAPI_Stereo_Enable();
				Assert(status == NVAPI_OK);
			}
			else
			{
				sm_bCanUseStereo = false;
				sm_bStereoEnabled = false;
				ASSERT_ONLY(NvAPI_Status status = )NvAPI_Stereo_Disable();
				Assert(status == NVAPI_OK);
			}
		}
		else
		{
			sm_bCanUseStereo = false;
			sm_bStereoEnabled = false;
			Warningf("NvAPI_Stereo_IsEnabled failed.\n");
		}
	}
#endif
}
#endif

void grcDevice::Manage() 
{
	WIN32PC_ONLY(TELEMETRY_START_ZONE(PZONE_NORMAL, __FILE__,__LINE__,"grcDevice::Manage()"));
#if __WIN32PC
	// Do not enter sleep mode or dim the display.
	SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);

	MSG msg;

	while (true)
	{
		BOOL bMessage = PeekMessageW(&msg,0,0,0,PM_REMOVE);

		if (bMessage)
		{
		  #if __DEV
			// TranslateAccelerator(g_hwndMain, hAccel, &msg);
		  #endif

		  #if __GFWL
			if (XLivePreTranslateMessage(&msg) == FALSE)
		  #endif
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}
		else if (grcDevice::GetLostFocus() && sm_BlockOnLostFocus)
		{
#if __D3D11
			if (!altTabbingState || altTabbingState == ATS_FullscreenMinimized)
			{
				if (altTabbingState == ATS_FullscreenMinimized)
					sysIpcSleep(100);

				sysIpcSleep(1);
				break;
			}
			else
			{
				break;
			}
#else
			{
				sysIpcSleep(100);
			}
#endif
		}
		else
		{
			break;
		}
// TODO: This needs to be re-evaluated
		// If the application wants to show/hide the mouse cursor, this apparently needs to be done from the same thread that
		// handles the windows messages.
		CURSORINFO Info;
		memset(&Info, 0, sizeof(CURSORINFO));
		Info.cbSize = sizeof(CURSORINFO);
		::GetCursorInfo(&Info);
		if( ioMouse::GetAbsoluteOnly() != (Info.flags == CURSOR_SHOWING) )
		{
			SetupCursor(ioMouse::GetAbsoluteOnly() && IsWindowed());
		}

		// If the mouse capture state has changed then update it.
		if(!ioMouse::IsIgnoringInput())
		{
			HWND mouseOwner = ::GetCapture();
			s32 wantCapture = ioMouse::GetCaptureCount();
			if(mouseOwner != g_hwndMain && wantCapture > 0)
				::SetCapture(g_hwndMain);
			else if(mouseOwner == g_hwndMain && wantCapture <= 0)
				::ReleaseCapture();
		}
	}

#if RSG_PC
	if(GRCDEVICE.IsMessagePumpThreadThatCreatedTheD3DDevice())
	{
		ioInput::SignalMessagePumpUpdate();
	}
	TELEMETRY_END_ZONE(__FILE__,__LINE__);
#endif // RSG_PC

#if __WIN32PC && __D3D11
	if (GRCDEVICE.IsMessagePumpThreadThatCreatedTheD3DDevice())
	{
		BOOL bFullScreen = false;
		((IDXGISwapChain*)GRCDEVICE.GetSwapChain())->GetFullscreenState(&bFullScreen, NULL);

		GRCDEVICE.SetSwapChainFullscreen(bFullScreen == TRUE);

		if (sm_DesiredSeparation != -1.0f && GRCDEVICE.CanUseStereo())
		{
			GRCDEVICE.SetSeparationPercentage(sm_DesiredSeparation);
		}
		sm_DesiredSeparation = -1.0f;
		if (sm_DesiredConvergence != -1.0f && GRCDEVICE.CanUseStereo())
		{
			GRCDEVICE.SetDefaultConvergenceDistance(sm_DesiredConvergence);
		}
		sm_DesiredConvergence = -1.0f;

		if (IsBorderless() != IsDesireBorderless())
		{
			LONG_PTR result = GetWindowLongPtr( g_hwndMain, GWL_STYLE );
			if (!IsDesireBorderless())
			{
				result &= ~WS_POPUP;
				result |= GetWindowFlags();
			}
			else if (IsDesireBorderless())
			{
				result &= ~GetWindowFlags();
				result |= WS_POPUP;
			}
			result = SetWindowLongPtr( g_hwndMain, GWL_STYLE, result);
			if (result != 0)
			{
				SetBorderless(IsDesireBorderless());
				pcdDisplayf("Set Window Position: Switching borderless mode");
				SetWindowPos( g_hwndMain, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		}

		if (GRCDEVICE.IsChangeDeviceRequest())
		{
			GRCDEVICE.ChangeDevice(sm_DesiredWindow);
		}

		if (GRCDEVICE.IsForceDeviceReset())
		{
			GRCDEVICE.ForceDeviceReset();
		}

		if (IsRecheckDeviceChanges())
			CheckForDeviceChanges();

		if (altTabbingState)
		{
			if (altTabbingState == ATS_FullscreenToMinimize && bFullScreen)
			{
				if (!GRCDEVICE.GetHasFocus())
				{
//					grcDisplayf("Alt-Tabbed out but still fullscreen");
#if !__FINAL
					if (PARAM_minimizeLostFocus.Get() || GRCDEVICE.IsOccluded())
#endif
					{
						GRCDEVICE.GoWindowed();
						grcDisplayf("Go Windowed because out of focus");
					}
				}
				else
				{
					grcDisplayf("Alt-Tabbing mode is now ATS_NotActive");
					altTabbingState = ATS_NotActive;
					GRCDEVICE.SetBusyAltTabbing(false);
					GRCDEVICE.AllowAltEnter();
				}
			}
			if (altTabbingState == ATS_FullscreenToMinimize && !bFullScreen)
			{
				grcDisplayf("Alt-Tabbing mode is now ATS_WaitForMinimized");
				altTabbingState = ATS_WaitForMinimized;
				g_inWindow = true;
				ShowWindow(g_hwndMain, SW_MINIMIZE);
			}

			if (altTabbingState > ATS_FullscreenMinimized && bFullScreen)
			{
				grcDisplayf("Alt-Tabbing mode was %d is now ATS_NotActive", altTabbingState);
				altTabbingState = ATS_NotActive;
				GRCDEVICE.SetBusyAltTabbing(false);
				GRCDEVICE.AllowAltEnter();
			}

			if (altTabbingState == ATS_MinimizedToWindowDone && !bFullScreen && GRCDEVICE.IsReady() && !GRCDEVICE.GetLostFocus())
			{
				grcDisplayf("Alt-Tabbing mode is now ATS_MinimizedToFullscreen");
				altTabbingState = ATS_MinimizedToFullscreen;
				GoFullscreen();
				GRCDEVICE.SetBusyAltTabbing(false);
				GRCDEVICE.AllowAltEnter();
#if NV_SUPPORT
				s_gAltTabbingState = true;
#endif
			}
		}
	}
#endif

#	if !__FINAL
	// Rag closes the window when we exit and we do not get informed of this. So every frame we will check that our
	// handle to the window is valid, if it is not then close the game as the window we are displaying to no longer
	// exists.
	if(!IsWindow(g_hwndMain))
	{
		GRCDEVICE.SetLostFocus(false);		// so game doesn't need to be activated first before it will quit
		GRCDEVICE.SetPaused(true);
		s_Closed = true;
	}
#	endif // !__FINAL
#endif
}

#if __WIN32PC
#if NV_SUPPORT
void grcDevice::RegisterStereoChangeSepCallbacks(Functor1<float> changeCallback)
{
	sm_StereoSepChangeCb = changeCallback;
}
void grcDevice::RegisterStereoChangeConvCallbacks(Functor1<float> changeCallback)
{
	sm_StereoConvChangeCb = changeCallback;
}
#endif

//void grcDevice::RemoveStereoChangeCallbacks()
//{
//
//}

void grcDevice::RegisterDeviceLostCallbacks(Functor0 lostCallback, Functor0 resetCallback) {
	grcDebugf1("RegisterDeviceLostCallbacks, length of s_DeviceLostCb list: %d", sm_DeviceLostCb.GetCount());
	grcDebugf1("RegisterDeviceLostCallbacks, length of s_DeviceResetCb list: %d", sm_DeviceResetCb.GetCount());

	int i=0;
	while (i<sm_DeviceLostCb.GetCount())
	{
		if (sm_DeviceLostCb[i].GetFunc()==lostCallback.GetFunc())
		{
			grcDisplayf("Already have this lostCallback: %d", lostCallback.GetFunc());
			sm_DeviceLostCb.Delete(i);
		}
		else
		{
			i++;
		}
	}

	i=0;
	while (i<sm_DeviceResetCb.GetCount())
	{
		if (sm_DeviceResetCb[i].GetFunc()==resetCallback.GetFunc())
		{
			grcDisplayf("Already have this resetCallback: %d", resetCallback.GetFunc());
			sm_DeviceResetCb.Delete(i);
		}
		else
		{
			i++;
		}
	}

	sm_DeviceLostCb.Append() = lostCallback;
	sm_DeviceResetCb.Append() = resetCallback;
}

void grcDevice::RemoveDeviceLostCallbacksThatHaveCallee(void* callee)
{
	int i=0;
	while (i<sm_DeviceLostCb.GetCount())
	{
		if (sm_DeviceLostCb[i].GetCallee()==callee)
			sm_DeviceLostCb.Delete(i);
		else
			i++;
	}
	grcDisplayf("Issuing Reset Device Lost Callbacks");
	i=0;
	while (i<sm_DeviceResetCb.GetCount())
	{
		if (sm_DeviceResetCb[i].GetCallee()==callee)
			sm_DeviceResetCb.Delete(i);
		else
			i++;
	}
}

void grcDevice::UnregisterAllDeviceLostCallbacks()
{
	sm_DeviceLostCb.Reset();
	sm_DeviceResetCb.Reset();
}

#endif		// __WIN32PC

#if __WIN32PC || RSG_DURANGO

bool grcDevice::GetHasFocus()
{
#if __WIN32PC
#if __BANK

	static bool usingRag = PARAM_rag.Get();

	if (!usingRag)
	{
		return !sm_LostFocus;
	}
	// might be in rag so see if the parent window has focus.
	HWND foregroundWindow = ::GetForegroundWindow();
	HWND window = g_hwndMain;
	while (window != NULL)
	{
		if (foregroundWindow == window)
			return true;
		window = ::GetParent(window);
	}
	return false;

#else
	return !sm_LostFocus;
//	return ::GetForegroundWindow() == g_hwndMain;

#endif // __BANK

#elif RSG_DURANGO
	return sm_HasFocus;
#endif // RSG_DURANGO
}

#endif //__WIN32PC || RSG_DURANGO

#if RSG_PC
sysIpcThreadId grcDevice::sm_FocusQueryThreadId = sysIpcThreadIdInvalid;
static bool sm_bRunFocusQueryThread = false;

void grcDevice::KillQueryFocusThread()
{
	sm_bRunFocusQueryThread = false;
}

void grcDevice::QueryFocusThread(void*)
{
	while (sm_bRunFocusQueryThread)
	{
		HWND foregroundWindow = ::GetForegroundWindow();
		HWND window = g_hwndMain;
		bool currentlyHasFocus = false;
		while (window != NULL)
		{
			if (foregroundWindow == window)
				currentlyHasFocus = true;
			window = ::GetParent(window);
		}
		GRCDEVICE.SetLostFocusForAudio(!currentlyHasFocus);

		Sleep(60);
	}
}

void grcDevice::InitFocusQueryThread()
{
	sm_bRunFocusQueryThread = true;
	sm_FocusQueryThreadId = sysIpcCreateThread(&QueryFocusThread, (void*)0, sysIpcMinThreadStackSize, PRIO_ABOVE_NORMAL, "Query Window Focus");
}
#endif

#if RSG_PC
static void doNothingRenderFunction()
{
}
void (*grcDevice::RenderFunctionCallback)() = doNothingRenderFunction;
#endif

}	// namespace rage

