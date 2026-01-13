// 
// grcore/adapter_d3d11.cpp
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
// 

#include "grcore/channel.h"
#include "grcore/d3dwrapper.h"

#if RSG_PC && __D3D11

#include "grcore/device.h"
#include "grcore/adapter_d3d11.h"
#include "input/headtracking.h"

#include "../../3rdparty/AMD/AGS_v2.1/AGS Lib/inc/amd_ags.h"
# if RSG_CPU_X64
#   pragma comment(lib,"amd_ags64.lib")
# elif RSG_CPU_X86
#   pragma comment(lib,"amd_ags.lib")
# endif //CPU

#if NV_SUPPORT
#include "../../3rdParty/NVidia/nvapi.h"
#endif //NV_SUPPORT

#if ATI_SUPPORT
#include "../../3rdParty/AMD/ADL_SDK_7.0/include/adl_sdk.h"
#endif

typedef HRESULT (WINAPI* LPCREATEDXGIFACTORY)(REFIID, void**);

namespace rage {

PARAM(availablevidmem, "[MEMORY] Percentage of available video memory");
NOSTRIP_XPARAM(GPUCount);
NOSTRIP_PARAM(NoDPIAdjust, "[graphics] Ignore the windows DPI values and compute from raw pixel dimensions");

const float cfDefaultDPI = 96.0f;

grcAdapterD3D11Output::grcAdapterD3D11Output(s32 AdapterHighPart, u32 AdapterLowPart, IDXGIOutput* pDeviceOutput, u32 uFormat) :
	m_pDeviceOutput(pDeviceOutput),
	m_aModes(NULL),
	m_uModeCount(0),
	m_uLargestMode(0),
	m_bSupportsHeadTracking(false),
	m_bSupportsStereo(false),
	dpiX((unsigned int)cfDefaultDPI),
	dpiY((unsigned int)cfDefaultDPI),
	DisplayConfig_TargetId(0),
	Rotation(roUnspecified),
	DesktopCoordinates(0,0,1920,1080),
	Monitor(nullptr)
{
	EnumerateModes(uFormat);

	ioHeadTracking::Initialize();

	DXGI_OUTPUT_DESC outputMonitorDescription;
	GetDesc(AdapterHighPart, AdapterLowPart, this, outputMonitorDescription, dpiX, dpiY);
	char outputName[MAX_ADAPTER_NAME_LEN];
	int len = (int)wcstombs(outputName, outputMonitorDescription.DeviceName, MAX_ADAPTER_NAME_LEN);
	outputName[len - 1] = 0;
	if (strstr(grcAdapterManager::GetOculusOutputMonitorName(), outputName))
	{
		m_bSupportsHeadTracking = m_bSupportsStereo = true;
	}
}

grcAdapterD3D11Output::~grcAdapterD3D11Output()
{
	if (m_pDeviceOutput)
	{
		m_pDeviceOutput->Release();
		m_pDeviceOutput = NULL;
	}

	if (m_aModes)
	{
		delete[] m_aModes;
		m_aModes = NULL;

		m_uModeCount = 0;
	}
}

bool grcAdapterD3D11Output::IsOutputPortrait(DXGI_OUTPUT_DESC& oDesc)
{
	DEVMODEW CurrentMode = {};
	if (!EnumDisplaySettingsW(oDesc.DeviceName,
		ENUM_CURRENT_SETTINGS,
		&CurrentMode))
		return false;

	if (CurrentMode.dmPelsWidth < CurrentMode.dmPelsHeight)
	{
		return true;
	}

	return false;
}

typedef enum MONITOR_DPI_TYPE {
	MDT_EFFECTIVE_DPI = 0,
	MDT_ANGULAR_DPI = 1,
	MDT_RAW_DPI = 2,
	MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;

typedef HRESULT(WINAPI* LPGetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);

u32 stripHexID(char *pstrSource, const char* pstrID) {
	char*pstrIDStart = strstr(pstrSource, pstrID);
	if (!pstrIDStart) return 0;
	char*pstrIDEnd = strstr(pstrIDStart, "&");
	if (!pstrIDEnd) return 0;
	int strLenIDValue = (int)(pstrIDEnd - pstrIDStart) - (int)strlen(pstrID);
	char strIDValue[20];
	memcpy((void*)strIDValue, (void*)(pstrIDStart + (int)strlen(pstrID)), strLenIDValue);
	strIDValue[strLenIDValue] = '\0';
	return (u32)strtol(strIDValue, NULL, 16);
}

void StripVendorAndDeviceID(char *pstrSource, u32 &VendorID, u32 &DeviceID) {
	VendorID = stripHexID(pstrSource, "VEN_");
	DeviceID = stripHexID(pstrSource, "DEV_");
}

bool grcAdapterD3D11::SetupAdapterOutputWindows()
{
	DXGI_ADAPTER_DESC desc;
	GetDeviceAdapter()->GetDesc(&desc);

	UINT32 numPathArrayElements = 256;
	DISPLAYCONFIG_PATH_INFO pathArray[256];
	UINT32 numModeInfoArrayElements = 256;
	DISPLAYCONFIG_MODE_INFO modeInfoArray[256];
	LONG QResult = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &numPathArrayElements, pathArray, &numModeInfoArrayElements, modeInfoArray, NULL);

	if (QResult != ERROR_SUCCESS)
	{
		Warningf("Failed to query display config");
		return false;
	}

	LONG adapterHighPart = 0;
	DWORD adapterLowPart = 0;
	bool outputDeviceFound = false;

	for (UINT32 index = 0; index < numPathArrayElements; index++)
	{
		DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceInfo;
		sourceInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
		sourceInfo.header.id = pathArray[index].sourceInfo.id;
		sourceInfo.header.adapterId = pathArray[index].sourceInfo.adapterId;
		sourceInfo.header.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);
		QResult = DisplayConfigGetDeviceInfo(&sourceInfo.header);

		DISPLAY_DEVICEW dd;
		dd.cb = sizeof(DISPLAY_DEVICEW);
		DWORD deviceNum = 0;
		while (EnumDisplayDevicesW(NULL, deviceNum++, &dd, 0)) {
			if (wcscmp(dd.DeviceName, sourceInfo.viewGdiDeviceName) == 0)
			{
				char deviceIDStr[256];
				wcstombs(deviceIDStr, dd.DeviceID, sizeof(dd.DeviceID));
				u32 VendorID, DeviceID;
				StripVendorAndDeviceID(deviceIDStr, VendorID, DeviceID);
				if ((DeviceID == desc.DeviceId && VendorID == desc.VendorId))
				{
					adapterHighPart = sourceInfo.header.adapterId.HighPart;
					adapterLowPart = sourceInfo.header.adapterId.LowPart;
					outputDeviceFound = true;
					/*
					if (adapterHighPart == desc->AdapterHighPart && adapterLowPart == desc->AdapterLowPart)
					{
						outputDeviceFound = false;
					}
					*/
					if (outputDeviceFound) 
						break;
				}
			}
		}
		if (outputDeviceFound) break;
	}

	if (!outputDeviceFound) {
		Warningf("Failed to find device");
		return false;
	}

	m_AdapterHighPart = adapterHighPart;
	m_AdapterLowPart = adapterLowPart;
	
	int outputCount = 0;
	for (UINT32 index = 0; index < numPathArrayElements; index++)
	{
		if (adapterHighPart == pathArray[index].sourceInfo.adapterId.HighPart && adapterLowPart == pathArray[index].sourceInfo.adapterId.LowPart)
		{
			outputCount++;
		}
	}
	if (!outputCount) {
		Warningf("Failed to find device");
		return false;
	}
	if (outputCount != GetOutputCount())
	{
		Warningf("Mismatching output counts");
		return false;
	}

	/*
	for (int outputIdx = 0; outputIdx < GetOutputCount(); outputIdx++)
	{
		DXGI_OUTPUT_DESC desc;
		grcAdapterD3D11Output* pOutput = m_aOutputs[outputIdx];

		if (!GetDesc(pOutput, desc)) {
			Warningf("Failed to get description of output %d", outputIdx);
			continue;
		}
	}
	*/

	int outputIndex = 0;
	for (UINT32 index = 0; index < numPathArrayElements; index++)
	{
		// DisplayColorSpace outDesColorSpace = csRGB_Full_G22_BT709;

		if (adapterHighPart == pathArray[index].sourceInfo.adapterId.HighPart && adapterLowPart == pathArray[index].sourceInfo.adapterId.LowPart)
		{
			grcAdapterD3D11Output* pOutput = m_aOutputs[outputIndex];
			DXGI_OUTPUT_DESC oDesc;
			unsigned int dpiX, dpiY;
			if (!pOutput->GetDesc(m_AdapterHighPart, m_AdapterLowPart, pOutput, oDesc, dpiX, dpiY)) {
				Warningf("Failed to get description of output %d", outputIndex);
				continue;
			}
			memcpy(&pOutput->DesktopCoordinates, &oDesc.DesktopCoordinates, sizeof(pOutput->DesktopCoordinates));
			//pOutput->AttachedToDesktop = false;
			pOutput->Rotation = (OutputRotation)pathArray[index].targetInfo.rotation;
			pOutput->DisplayConfig_TargetId = pathArray[index].targetInfo.id;

			DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceInfo;
			sourceInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
			sourceInfo.header.id = pathArray[index].sourceInfo.id;
			sourceInfo.header.adapterId = pathArray[index].sourceInfo.adapterId;
			sourceInfo.header.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);
			QResult = DisplayConfigGetDeviceInfo(&sourceInfo.header);

			//wcscpy(desc.Outputs[outputIndex].DeviceName, sourceInfo.viewGdiDeviceName);

			DISPLAYCONFIG_TARGET_DEVICE_NAME targetInfo;
			targetInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
			targetInfo.header.id = pathArray[index].targetInfo.id;
			targetInfo.header.adapterId = pathArray[index].targetInfo.adapterId;
			targetInfo.header.size = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);
			QResult = DisplayConfigGetDeviceInfo(&targetInfo.header);

			//wcscpy(desc.Outputs[outputIndex].FriendlyDeviceName, targetInfo.monitorFriendlyDeviceName);

			if (pOutput->Rotation == roRotate90 || pOutput->Rotation == roRotate270)
			{
				int temp = pOutput->DesktopCoordinates.right;
				pOutput->DesktopCoordinates.right = pOutput->DesktopCoordinates.bottom;
				pOutput->DesktopCoordinates.bottom = temp;
			}

#if defined(NTDDI_WIN10_RS2) && 0 //not available till sdk 10.0.15063
			DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO colorInfo;
			colorInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
			colorInfo.header.id = pathArray[index].targetInfo.id;
			colorInfo.header.adapterId = pathArray[index].targetInfo.adapterId;
			colorInfo.header.size = sizeof(DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO);
			QResult = DisplayConfigGetDeviceInfo(&colorInfo.header);
			if (QResult == ERROR_SUCCESS) {
				// wide color gamut support needs to be enabled as well (advancedColorEnabled) so we can create a HDR swap chain
				if (colorInfo.advancedColorSupported && colorInfo.advancedColorEnabled) {
					pOutput->eBitSetFeatures |= (1 << OF_ADVANCED_COLOR);
				}
				else {
					pOutput->eBitSetFeatures &= ~(1 << OF_ADVANCED_COLOR);
				}
			}
#endif //NTDDI_WIN10_RS2
			//pOutput->eDisplayColorSpace = outDesColorSpace;
			outputIndex++;
		}
	}
	FatalAssertf(GetOutputCount() == outputIndex, "Adapter Output Search Mismatch %d found %d", GetOutputCount(), outputIndex);
	//desc.OutputCount = outputIndex;

	/*
	DISPLAY_DEVICEW dd;
	dd.cb = sizeof(DISPLAY_DEVICEW);
	DWORD deviceNum = 0;

	while (EnumDisplayDevicesW(NULL, deviceNum, &dd, 0)) {
		for (u32 outputIndex = 0; outputIndex < desc.OutputCount; outputIndex++)
		{
			if (wcscmp(dd.DeviceName, desc.Outputs[outputIndex].DeviceName) == 0)
			{
				DWORD rawModeCount = 0;
				u32 enumIndex = 0;
				DEVMODEW displayMode = { 0 };
				displayMode.dmSize = sizeof(DEVMODE);

				while (EnumDisplaySettingsW(dd.DeviceName, enumIndex, &displayMode))
				{
					//grab all the total number of modes but then only collect the unique ones with the best state.
					if (displayMode.dmBitsPerPel >= 32 && IsValidDisplayMode(displayMode.dmPelsWidth, displayMode.dmPelsHeight, (float)displayMode.dmDisplayFrequency))
					{
						rawModeCount++;
					}
					enumIndex++;
				}
				FatalAssertf(rawModeCount, "No display modes found for output device %u on adapter %s", outputIndex, desc.strDescription);

				ModeDesc *rawModes = rage_new ModeDesc[rawModeCount*uNumFormat];
				DWORD modeCount = 0;
				enumIndex = 0;

				while (EnumDisplaySettingsW(dd.DeviceName, enumIndex, &displayMode))
				{
					if (displayMode.dmBitsPerPel >= 32 && IsValidDisplayMode(displayMode.dmPelsWidth, displayMode.dmPelsHeight, (float)displayMode.dmDisplayFrequency))
					{
						//loop through all the already chosen modes to see if it already exists with different scaling and if so choose the preferable scaling. (Stretch > Center > Unknown)
						bool alreadyExists = false;
						for (u32 existingIndex = 0; existingIndex < modeCount; existingIndex++)
						{
							ModeDesc &existingMode = rawModes[existingIndex];
							if (existingMode.Height == displayMode.dmPelsHeight && existingMode.Width == displayMode.dmPelsWidth && existingMode.RefreshRate.Numerator == displayMode.dmDisplayFrequency)
							{
								if (displayMode.dmDisplayFixedOutput == DMDFO_STRETCH)
									existingMode.Scaling = (ModeScaling)DMDFO_STRETCH;
								else if (displayMode.dmDisplayFixedOutput == DMDFO_CENTER && existingMode.Scaling != DMDFO_STRETCH)
									existingMode.Scaling = (ModeScaling)DMDFO_CENTER;
								alreadyExists = true;
								break;
							}
						}

						if (!alreadyExists)
						{
							for (u32 formatIndex = 0; formatIndex < uNumFormat; formatIndex++)
							{
								ModeDesc &oMode = rawModes[modeCount];
								memset(&oMode, 0, sizeof(ModeDesc));
								oMode.Format = paeFormats[formatIndex];
								oMode.Width = displayMode.dmPelsWidth;
								oMode.Height = displayMode.dmPelsHeight;
								oMode.RefreshRate.Numerator = displayMode.dmDisplayFrequency;
								oMode.RefreshRate.Denominator = 1;
								oMode.ScanlineOrdering = soUnspecified;
								oMode.Scaling = (ModeScaling)displayMode.dmDisplayFixedOutput;
								modeCount++;
							}
						}
					}
					enumIndex++;
				}

				desc.Outputs[outputIndex].ModeCount = modeCount;
				desc.Outputs[outputIndex].Modes = rage_new ModeDesc[modeCount];
				memcpy(desc.Outputs[outputIndex].Modes, rawModes, sizeof(ModeDesc) * modeCount);
				delete[] rawModes;
			}
		}
		deviceNum++;
	}
	*/

	return true;
}

#if RSG_PC
typedef UINT(WINAPI* LPGetDpiForWindow)(HWND hwnd);
LPGetDpiForWindow fnGetDpiForWindow = nullptr;
#endif

bool grcAdapterD3D11Output::GetDesc(s32 adapterHighPart, u32 adapterLowPart, const grcAdapterD3D11Output* pOutput, DXGI_OUTPUT_DESC& outputDesc, unsigned int& dpiX, unsigned int& dpiY)
{
	IDXGIOutput* pDeviceOutput = pOutput->GetDeviceOutput();
	HRESULT hr = pDeviceOutput->GetDesc(&outputDesc);
	if (FAILED(hr))
		return false;

	static HINSTANCE dpiLib = LoadLibrary("Shcore.dll");
	static LPGetDpiForMonitor pfnDPIForMonitor = NULL;
	if (dpiLib && !pfnDPIForMonitor) {
		pfnDPIForMonitor = (LPGetDpiForMonitor)GetProcAddress(dpiLib, "GetDpiForMonitor");
	}

	// Look up from user.dll
	static HINSTANCE hUserDLL = LoadLibrary("user32.dll");
	if (hUserDLL && !fnGetDpiForWindow)
	{
		fnGetDpiForWindow = (LPGetDpiForWindow)GetProcAddress(hUserDLL, "GetDpiForWindow");
	}

	UINT32 numPathArrayElements = 256;
	DISPLAYCONFIG_PATH_INFO pathArray[256];
	UINT32 numModeInfoArrayElements = 256;
	DISPLAYCONFIG_MODE_INFO modeInfoArray[256];
	LONG QResult = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &numPathArrayElements, pathArray, &numModeInfoArrayElements, modeInfoArray, NULL);
	if (QResult != ERROR_SUCCESS) {
		return true;
	}

	u32 DisplayConfig_TargetId;

	// for (int outputIndex = 0; outputIndex < GetOutputCount(); outputIndex++) {
	for (u32 index = 0; index < numPathArrayElements; index++) {
		// only Vulkan will pass this check

		if (adapterHighPart == pathArray[index].sourceInfo.adapterId.HighPart && adapterLowPart == pathArray[index].sourceInfo.adapterId.LowPart)
		{
			outputDesc.AttachedToDesktop = false;
			outputDesc.Rotation = (DXGI_MODE_ROTATION)pathArray[index].targetInfo.rotation;
			outputDesc.Monitor = nullptr;
			DisplayConfig_TargetId = pathArray[index].targetInfo.id;

			DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceInfo;
			sourceInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
			sourceInfo.header.id = pathArray[index].sourceInfo.id;
			sourceInfo.header.adapterId = pathArray[index].sourceInfo.adapterId;
			sourceInfo.header.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);
			QResult = DisplayConfigGetDeviceInfo(&sourceInfo.header);

			wcscpy(outputDesc.DeviceName, sourceInfo.viewGdiDeviceName);
			/*
			DISPLAYCONFIG_TARGET_DEVICE_NAME targetInfo;
			targetInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
			targetInfo.header.id = pathArray[index].targetInfo.id;
			targetInfo.header.adapterId = pathArray[index].targetInfo.adapterId;
			targetInfo.header.size = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);
			QResult = DisplayConfigGetDeviceInfo(&targetInfo.header);

			wcscpy(outputDesc.FriendlyDeviceName, targetInfo.monitorFriendlyDeviceName);
			*/

			if (outputDesc.Rotation == roRotate90 || outputDesc.Rotation == roRotate270)
			{
				int temp = outputDesc.DesktopCoordinates.right;
				outputDesc.DesktopCoordinates.right = outputDesc.DesktopCoordinates.bottom;
				outputDesc.DesktopCoordinates.bottom = temp;
			}
	/*
#ifdef NTDDI_WIN10_RS2  //not available till sdk 10.0.15063
DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO colorInfo;
colorInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
colorInfo.header.id = pathArray[index].targetInfo.id;
colorInfo.header.adapterId = pathArray[index].targetInfo.adapterId;
colorInfo.header.size = sizeof(DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO);
QResult = DisplayConfigGetDeviceInfo(&colorInfo.header);
if (QResult == ERROR_SUCCESS) {
// wide color gamut support needs to be enabled as well (advancedColorEnabled) so we can create a HDR swap chain
if (colorInfo.advancedColorSupported && colorInfo.advancedColorEnabled) {
desc.Outputs[outputIndex].eBitSetFeatures |= (1 << OF_ADVANCED_COLOR);
}
else {
desc.Outputs[outputIndex].eBitSetFeatures &= ~(1 << OF_ADVANCED_COLOR);
}
}
#endif //NTDDI_WIN10_RS2
desc.Outputs[outputIndex].eDisplayColorSpace = outDesColorSpace;
	*/

			if (pathArray[index].targetInfo.id == DisplayConfig_TargetId) { // pOutput->DisplayConfig_TargetId) {
				DISPLAYCONFIG_SOURCE_MODE &sourceMode = modeInfoArray[pathArray[index].sourceInfo.modeInfoIdx].sourceMode;
				int width = sourceMode.width;
				int height = sourceMode.height;
				if (width > height && grcAdapterD3D11Output::IsOutputPortrait(outputDesc)) {
					width = sourceMode.height;
					height = sourceMode.width;
				}
				outputDesc.DesktopCoordinates.top = sourceMode.position.y;
				outputDesc.DesktopCoordinates.left = sourceMode.position.x;
				outputDesc.DesktopCoordinates.right = sourceMode.position.x + width;
				outputDesc.DesktopCoordinates.bottom = sourceMode.position.y + height;

				POINT monPoint = { sourceMode.position.x + 1, sourceMode.position.y + 1 };
				outputDesc.Monitor = MonitorFromPoint(monPoint, MONITOR_DEFAULTTONEAREST);
			}
		}
	}

	if (pfnDPIForMonitor && !PARAM_NoDPIAdjust.Get() && outputDesc.Monitor) {
		pfnDPIForMonitor((HMONITOR)outputDesc.Monitor, MDT_ANGULAR_DPI, &dpiX, &dpiY);
		unsigned int uDPI = (unsigned int)cfDefaultDPI; // (fnGetDpiForWindow && g_hwndMain) ? fnGetDpiForWindow(g_hwndMain) : (unsigned int)cfDefaultDPI;
		outputDesc.DesktopCoordinates.top = int(outputDesc.DesktopCoordinates.top * (float)(uDPI ? uDPI : dpiY) / cfDefaultDPI); 
		outputDesc.DesktopCoordinates.left = int(outputDesc.DesktopCoordinates.left * (float)(uDPI ? uDPI : dpiX) / cfDefaultDPI); 
		outputDesc.DesktopCoordinates.right = int(outputDesc.DesktopCoordinates.right * (float)(uDPI ? uDPI :dpiX) / cfDefaultDPI); 
		outputDesc.DesktopCoordinates.bottom = int(outputDesc.DesktopCoordinates.bottom * (float)(uDPI ? uDPI : dpiY) / cfDefaultDPI); 
	}
	else {
		const DXGI_MODE_DESC& oWidestMode = pOutput->GetWidestMode();

		float width = (float)(outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left);
		float height = (float)(outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top);

		dpiX = UINT(cfDefaultDPI * (width / (float)oWidestMode.Width));
		dpiY = UINT(cfDefaultDPI * (height/ (float)oWidestMode.Height));
	}
	return true;
}

bool grcAdapterD3D11Output::GetMode(grcDisplayWindow* pDisplayModeOut, u32 uMode) const
{
	if (pDisplayModeOut == NULL)
		return false;

	if (uMode >= m_uModeCount)
		return false;

	pDisplayModeOut->uWidth		  = m_aModes[uMode].Width;
	pDisplayModeOut->uHeight	  = m_aModes[uMode].Height;
	pDisplayModeOut->uRefreshRate = m_aModes[uMode].RefreshRate;
	pDisplayModeOut->uFrameLock	  = 0;
	pDisplayModeOut->bFullscreen  = true;
//	pDisplayModeOut->bCentered	  = (m_aModes[uMode].Scaling == DXGI_MODE_SCALING_CENTERED);

	return true;
}

const DXGI_MODE_DESC& grcAdapterD3D11Output::GetClosestMode(const grcDisplayWindow& rDisplayWindow) const
{
	AssertMsg(m_aModes, "Invalid modes");

	s32 iClosestMode = -1;

	u32 rDisplayRefreshRate = (u32)floorf((float)rDisplayWindow.uRefreshRate.Numerator / (float)rDisplayWindow.uRefreshRate.Denominator);

	// Find exact match
	for (s32 iModeIndex = 0; iModeIndex < (s32)m_uModeCount; ++iModeIndex)
	{
		if ((rDisplayWindow.uWidth == m_aModes[iModeIndex].Width)	 &&
			(rDisplayWindow.uHeight == m_aModes[iModeIndex].Height) &&
			rDisplayRefreshRate == ((u32)floorf((float)m_aModes[iModeIndex].RefreshRate.Numerator / (float)m_aModes[iModeIndex].RefreshRate.Denominator)))
		{
			iClosestMode = iModeIndex;
			break;
		}
	}

	if (iClosestMode < 0)
	{
		// Match dimensions.
		u32 BestRefreshRate = 80000;
		for (s32 iModeIndex = 0; iModeIndex < (s32)m_uModeCount; ++iModeIndex)
		{
			if ((m_aModes[iModeIndex].Width == rDisplayWindow.uWidth) && (m_aModes[iModeIndex].Height == rDisplayWindow.uHeight))
			{
				u32 uRefreshRate = (u32)floorf((float)m_aModes[iModeIndex].RefreshRate.Numerator / (float)m_aModes[iModeIndex].RefreshRate.Denominator);

				if (abs((s32)uRefreshRate - (s32)rDisplayRefreshRate) < abs((s32)BestRefreshRate - (s32)rDisplayRefreshRate))
				{
					BestRefreshRate = uRefreshRate;
					iClosestMode = iModeIndex;
				}
			}
		}
	}

	if (iClosestMode < 0)
	{
		u32 uMinWidthDiff	= 0xFFFFFFFF;
		u32 uMinHeightDiff	= 0xFFFFFFFF;

		for (s32 iModeIndex = 0; iModeIndex < (s32)m_uModeCount; ++iModeIndex)
		{
			u32 uWidthDiff	= abs((int)(rDisplayWindow.uWidth - m_aModes[iModeIndex].Width));
			u32 uHeightDiff	= abs((int)(rDisplayWindow.uHeight - m_aModes[iModeIndex].Height));

			if ((uWidthDiff + uHeightDiff) <= (uMinWidthDiff + uMinHeightDiff))
			{
				uMinWidthDiff	= uWidthDiff;
				uMinHeightDiff	= uHeightDiff;
				iClosestMode	= iModeIndex;
			}
		}
	}

	if (iClosestMode < 0)
	{
		static DXGI_MODE_DESC sMode = {0};
		Warningf("Output mode list is empty!");
		sMode.Width		= rDisplayWindow.uWidth;
		sMode.Height	= rDisplayWindow.uHeight;
		sMode.RefreshRate.Numerator		= rDisplayWindow.uRefreshRate.Numerator;
		sMode.RefreshRate.Denominator	= rDisplayWindow.uRefreshRate.Denominator;
		sMode.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;
		return sMode;
	}

	return m_aModes[iClosestMode];
}

const DXGI_MODE_DESC& grcAdapterD3D11Output::GetWidestMode() const
{
	//Note: this will return the mode that is the widest.  If there are multiple candidates with the same max width, then the one with the most height will be chosen

	AssertMsg(m_aModes, "Invalid modes");
	// Match dimensions.
	u32 BestWidth = 0;
	u32 BestHeight = 0;
	u32 BestRefreshRate = 0;

	s32 iBestIndex = -1;
	for (s32 iModeIndex = 0; iModeIndex < (s32)m_uModeCount; ++iModeIndex)
	{
		if (m_aModes[iModeIndex].Width > BestWidth)
		{
			iBestIndex = iModeIndex;
			BestWidth = m_aModes[iModeIndex].Width;
			BestHeight = m_aModes[iModeIndex].Height;
			BestRefreshRate = (u32)floorf((float)m_aModes[iModeIndex].RefreshRate.Numerator / (float)m_aModes[iModeIndex].RefreshRate.Denominator);
		}
		else if (m_aModes[iModeIndex].Width == BestWidth)
		{
			if (m_aModes[iModeIndex].Height > BestHeight)
			{
				iBestIndex = iModeIndex;
				BestWidth = m_aModes[iModeIndex].Width;
				BestHeight = m_aModes[iModeIndex].Height;
				BestRefreshRate = (u32)floorf((float)m_aModes[iModeIndex].RefreshRate.Numerator / (float)m_aModes[iModeIndex].RefreshRate.Denominator);
			}
			else if (m_aModes[iModeIndex].Height == BestHeight)
			{
				u32 uRefreshRate = (u32)floorf((float)m_aModes[iModeIndex].RefreshRate.Numerator / (float)m_aModes[iModeIndex].RefreshRate.Denominator);

				if (BestRefreshRate < uRefreshRate)
				{
					iBestIndex = iModeIndex;
					BestWidth = m_aModes[iModeIndex].Width;
					BestHeight = m_aModes[iModeIndex].Height;
					BestRefreshRate = (u32)floorf((float)m_aModes[iModeIndex].RefreshRate.Numerator / (float)m_aModes[iModeIndex].RefreshRate.Denominator);
				}
			}
		}
	}

	if (iBestIndex < 0)
	{
		static DXGI_MODE_DESC sMode = {0};
		Warningf("Output mode list is empty!");
		sMode.Width		= 800;
		sMode.Height	= 600;
		sMode.RefreshRate.Numerator		= 60;
		sMode.RefreshRate.Denominator	= 1;
		sMode.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;
		return sMode;
	}

	return m_aModes[iBestIndex];
}

const DXGI_MODE_DESC& grcAdapterD3D11Output::GetClosestModeMatchingAspect(const grcDisplayWindow& rDisplayWindow, float aspect) const
{
	AssertMsg(m_aModes, "Invalid modes");

	s32 iClosestMode = -1;

	u32 rDisplayRefreshRate = (u32)floorf((float)rDisplayWindow.uRefreshRate.Numerator / (float)rDisplayWindow.uRefreshRate.Denominator);

	// Find exact match
	for (s32 iModeIndex = 0; iModeIndex < (s32)m_uModeCount; ++iModeIndex)
	{
		if ((rDisplayWindow.uWidth == m_aModes[iModeIndex].Width)	 &&
			(rDisplayWindow.uHeight == m_aModes[iModeIndex].Height) &&
			rDisplayRefreshRate == ((u32)floorf((float)m_aModes[iModeIndex].RefreshRate.Numerator / (float)m_aModes[iModeIndex].RefreshRate.Denominator)))
		{
			iClosestMode = iModeIndex;
			break;
		}
	}

	if (iClosestMode < 0)
	{
		// Match dimensions.
		u32 BestRefreshRate = 0;
		for (s32 iModeIndex = 0; iModeIndex < (s32)m_uModeCount; ++iModeIndex)
		{
			if ((m_aModes[iModeIndex].Width == rDisplayWindow.uWidth) && (m_aModes[iModeIndex].Height == rDisplayWindow.uHeight))
			{
				u32 uRefreshRate = (u32)floorf((float)m_aModes[iModeIndex].RefreshRate.Numerator / (float)m_aModes[iModeIndex].RefreshRate.Denominator);

				if (abs((s32)uRefreshRate - (s32)rDisplayRefreshRate) < abs((s32)BestRefreshRate - (s32)rDisplayRefreshRate))
				{
					BestRefreshRate = uRefreshRate;
					iClosestMode = iModeIndex;
				}
			}
		}
	}

	if (iClosestMode < 0)
	{
		u32 uMinWidthDiff	= 0xFFFFFFFF;
		u32 uMinHeightDiff	= 0xFFFFFFFF;

		for (s32 iModeIndex = 0; iModeIndex < (s32)m_uModeCount; ++iModeIndex)
		{
			float aspectRatio = (float) m_aModes[iModeIndex].Width / (float) m_aModes[iModeIndex].Height;

			float aspectDifference = abs (aspectRatio - aspect);
			u32 uWidthDiff	= abs((int)(rDisplayWindow.uWidth - m_aModes[iModeIndex].Width));
			u32 uHeightDiff	= abs((int)(rDisplayWindow.uHeight - m_aModes[iModeIndex].Height));

			if ((aspectDifference < 0.1) && (uWidthDiff <= uMinWidthDiff) && (uHeightDiff <= uMinHeightDiff))
			{
				uMinWidthDiff	= uWidthDiff;
				uMinHeightDiff	= uHeightDiff;
				iClosestMode	= iModeIndex;
			}
		}
	}


	if (iClosestMode < 0)
	{
		u32 uMinWidthDiff	= 0xFFFFFFFF;
		u32 uMinHeightDiff	= 0xFFFFFFFF;

		for (s32 iModeIndex = 0; iModeIndex < (s32)m_uModeCount; ++iModeIndex)
		{
			u32 uWidthDiff	= abs((int)(rDisplayWindow.uWidth - m_aModes[iModeIndex].Width));
			u32 uHeightDiff	= abs((int)(rDisplayWindow.uHeight - m_aModes[iModeIndex].Height));

			if ((uWidthDiff <= uMinWidthDiff) && (uHeightDiff <= uMinHeightDiff))
			{
				uMinWidthDiff	= uWidthDiff;
				uMinHeightDiff	= uHeightDiff;
				iClosestMode	= iModeIndex;
			}
		}
	}


	if (iClosestMode < 0)
	{
		static DXGI_MODE_DESC sMode = {0};
		Warningf("Output mode list is empty!");
		sMode.Width		= rDisplayWindow.uWidth;
		sMode.Height	= rDisplayWindow.uHeight;
		sMode.RefreshRate.Numerator		= rDisplayWindow.uRefreshRate.Numerator;
		sMode.RefreshRate.Denominator	= rDisplayWindow.uRefreshRate.Denominator;
		sMode.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;
		return sMode;
	}

	return m_aModes[iClosestMode];
}

RefreshRate grcAdapterD3D11Output::GetClosestRefreshRate(float refreshRate, const grcDisplayWindow& rDisplayWindow) const
{
	DXGI_MODE_DESC desc = GetClosestMode(rDisplayWindow);
	u32 width = desc.Width;
	u32 height = desc.Height;

	RefreshRate bestRefreshRate;
	bestRefreshRate.Numerator = 60000; bestRefreshRate.Denominator = 1000;
	float closesRefreshRate = 10000000.0f;

	for (s32 iModeIndex = 0; iModeIndex < (s32)m_uModeCount; ++iModeIndex)
	{
		if ((width == m_aModes[iModeIndex].Width) && (height == m_aModes[iModeIndex].Height))
		{
			float modeRefreshRate = (float)m_aModes[iModeIndex].RefreshRate.Numerator / (float)m_aModes[iModeIndex].RefreshRate.Denominator;
			if (abs(modeRefreshRate - refreshRate) < abs(closesRefreshRate - refreshRate))
			{
				bestRefreshRate = m_aModes[iModeIndex].RefreshRate;
				closesRefreshRate = modeRefreshRate;
			}
		}
	}

	return bestRefreshRate;
}

void grcAdapterD3D11Output::FilterModes(u32 uModeCount, DXGI_MODE_DESC* aModes, bool* aModeValid)
{
	if (aModes && aModeValid)
	{
		for (u32 uiModeIndex = 0; uiModeIndex < uModeCount; ++uiModeIndex)
		{
			aModeValid[uiModeIndex] = (aModes[uiModeIndex].ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE)	&&
									  ((aModes[uiModeIndex].Scaling == DXGI_MODE_SCALING_UNSPECIFIED) ||
									   (aModes[uiModeIndex].Scaling == DXGI_MODE_SCALING_CENTERED)) &&
									  (aModes[uiModeIndex].Width >= 800);
		}

		// If the scaling type is Centered, find the Unspecified version and mark it as invalid.
		for (u32 uiModeIndex = 0; uiModeIndex < uModeCount; ++uiModeIndex)
		{
			if ( aModes[uiModeIndex].Scaling == DXGI_MODE_SCALING_UNSPECIFIED )
			{
				for ( u32 i = 0; i < uModeCount; ++i )
				{
					if( aModes[i].Width == aModes[uiModeIndex].Width &&
						aModes[i].Height == aModes[uiModeIndex].Height &&
						aModes[i].RefreshRate.Numerator == aModes[uiModeIndex].RefreshRate.Numerator &&
						aModes[i].RefreshRate.Denominator == aModes[uiModeIndex].RefreshRate.Denominator &&
						aModes[i].Scaling == DXGI_MODE_SCALING_CENTERED)
					{
						aModeValid[i] = false;
					}
				}
			}
		}
	}
}

void grcAdapterD3D11Output::EnumerateModes(u32 uFormat)
{
	if (m_pDeviceOutput)
	{
		AssertMsg(m_aModes == NULL, "Modes already enumerated");

		DXGI_FORMAT eFormat = (DXGI_FORMAT)uFormat;

		UINT uiDisplayModeCount = 0;
		HRESULT hr = m_pDeviceOutput->GetDisplayModeList(eFormat, 0, &uiDisplayModeCount, NULL);

		if (SUCCEEDED(hr))
		{
			bool* aModeValid = rage_new bool[uiDisplayModeCount];
			AssertMsg(aModeValid, "Memory allocation failed - EnumerateModes");
			memset(aModeValid, 0, uiDisplayModeCount * sizeof(bool));

			DXGI_MODE_DESC* aRawModes = rage_new DXGI_MODE_DESC[uiDisplayModeCount];
			AssertMsg(aRawModes, "Memory allocation failed - EnumerateModes");

			hr = m_pDeviceOutput->GetDisplayModeList(eFormat, 0, &uiDisplayModeCount, aRawModes);

			if (SUCCEEDED(hr))
			{
				FilterModes(uiDisplayModeCount, aRawModes, aModeValid);

				u32 uiValidModeCount = 0;
				for (u32 uiModeIndex = 0; uiModeIndex < uiDisplayModeCount; ++uiModeIndex)
				{
					uiValidModeCount += (u32)aModeValid[uiModeIndex];
				}

				m_aModes = rage_new DXGI_MODE_DESC[uiValidModeCount];
				AssertMsg(m_aModes, "Mode allocation failed - EnumerateModes");

				u32 uModeArea = 0;
				for (u32 uiModeIndex = 0, uiValidModeIndex = 0; uiModeIndex < uiDisplayModeCount; ++uiModeIndex)
				{
					if (aModeValid[uiModeIndex])
					{
						m_aModes[uiValidModeIndex++] = aRawModes[uiModeIndex];
						if (aRawModes[uiModeIndex].Width * aRawModes[uiModeIndex].Height > uModeArea)
						{
							uModeArea = aRawModes[uiModeIndex].Width * aRawModes[uiModeIndex].Height;
							m_uLargestMode = uiValidModeIndex-1;
						}
					}
				}

				m_uModeCount = uiValidModeCount;
			}

			delete[] aModeValid;
			delete[] aRawModes;
			return;
		}
	}

	// Fake res
	m_uModeCount = 1;
	m_uLargestMode = 0;
	m_aModes = rage_new DXGI_MODE_DESC[m_uModeCount];
	m_aModes[0].Width = 800;
	m_aModes[0].Height = 600;
	m_aModes[0].RefreshRate.Numerator = 60;
	m_aModes[0].RefreshRate.Denominator = 1;
	m_aModes[0].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_aModes[0].ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_aModes[0].Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
}

bool grcAdapterD3D11Output::MatchesWith(const grcAdapterD3D11Output *output) const
{
	if (output->GetModeCount() != m_uModeCount) return false;

	for (u32 index = 0; index < m_uModeCount; index++)
	{
		grcDisplayWindow outputWindow;
		output->GetMode(&outputWindow, index);
		if (outputWindow.uWidth != m_aModes[index].Width) return false;
		if (outputWindow.uHeight != m_aModes[index].Height) return false;
		if (outputWindow.uRefreshRate.Numerator != m_aModes[index].RefreshRate.Numerator || outputWindow.uRefreshRate.Denominator != m_aModes[index].RefreshRate.Denominator) return false;
	}

	return true;
}

grcAdapterD3D11::grcAdapterD3D11(IDXGIAdapter* pDeviceAdapter, u32 uFormat) :
	m_pDeviceAdapter(pDeviceAdapter)
{
	m_GPUCount = 0;
	m_AdapterHighPart = 0;
	m_AdapterLowPart = 0;
	DetermineManufacturer();
	EnumerateOutputs(uFormat);
}

grcAdapterD3D11::~grcAdapterD3D11()
{
	if (m_pDeviceAdapter)
	{
		m_pDeviceAdapter->Release();
		m_pDeviceAdapter = NULL;
	}

	for (int outputIndex = 0; outputIndex < (int)m_aOutputs.size(); ++outputIndex)
	{
		delete m_aOutputs[outputIndex];
	}

	m_aOutputs.clear();
}

void grcAdapterD3D11::DetermineManufacturer()
{
	DXGI_ADAPTER_DESC oAdapterDesc;
	GetDeviceAdapter()->GetDesc(&oAdapterDesc);
	m_AdapterHighPart = oAdapterDesc.AdapterLuid.HighPart;
	m_AdapterLowPart = oAdapterDesc.AdapterLuid.LowPart;

	// Vendor IDs obtained from http://www.pcidatabase.com/
	switch (oAdapterDesc.VendorId)
	{
	case 0x10DE:	//NV
		Displayf("\t... recognised as NVidia");
		m_eManufacturer = NVIDIA;
		break;
	case 0x1002:	//ATI
	case 0x1022:	//AMD
		Displayf("\t... recognised as ATI");
		m_eManufacturer = ATI;
		break;
	case 0x8086:
	case 0x8087:
		Displayf("\t... recognised as Intel");
		m_eManufacturer = INTEL;
		break;

	default:
		Displayf("\t... not recognised");
		m_eManufacturer = UNKNOWN_DEVICE;
	}

	Assert(m_eManufacturer < DM_LAST);

	if (m_eManufacturer >= DM_LAST)
		m_eManufacturer = UNKNOWN_DEVICE;
}

void grcAdapterD3D11::EnumerateOutputs(u32 uFormat)
{
	if (m_pDeviceAdapter)
	{
		AssertMsg(m_aOutputs.size() == 0, "Outputs already enumerated");

		UINT uiOutputIndex = 0;
		IDXGIOutput* pOutput = NULL;

		while (m_pDeviceAdapter->EnumOutputs(uiOutputIndex, &pOutput) != DXGI_ERROR_NOT_FOUND)
		{
			grcAdapterD3D11Output* pAdapterD3D11Output = rage_new grcAdapterD3D11Output(m_AdapterHighPart, m_AdapterLowPart, pOutput, uFormat);
			m_aOutputs.PushAndGrow(pAdapterD3D11Output);

			if (pAdapterD3D11Output->SupportsHeadTracking())
			{
				sm_OculusMonitor = uiOutputIndex;
			}

			pOutput = NULL;
			uiOutputIndex++;
		}
	}
}

const grcAdapterD3D11Output* grcAdapterD3D11::GetOutput(u32 uOutput) const
{
	AssertMsg(uOutput < (u32)m_aOutputs.size(), "Invalid output");

	if (uOutput < (u32)m_aOutputs.size())
	{
		return m_aOutputs[uOutput];
	}

	return NULL;
}

u32 grcAdapterD3D11::GetModeCount(u32 uOutput) const
{
	AssertMsg(uOutput < (u32)m_aOutputs.size(), "Invalid output");

	if (uOutput < (u32)m_aOutputs.size())
	{
		return m_aOutputs[uOutput]->GetModeCount();
	}

	return 0;
}

u32 grcAdapterD3D11::GetLargestModeIndex(u32 uOutput) const
{
	AssertMsg(uOutput < (u32)m_aOutputs.size(), "Invalid output");

	if (uOutput < (u32)m_aOutputs.size())
	{
		return m_aOutputs[uOutput]->GetLargestModeIndex();
	}

	return 0;
}

bool grcAdapterD3D11::GetMode(grcDisplayWindow* pDisplayModeOut, u32 uMode, u32 uOutput) const
{
	AssertMsg(uOutput < (u32)m_aOutputs.size(), "Invalid output");

	if (uOutput < (u32)m_aOutputs.size())
	{
		return m_aOutputs[uOutput]->GetMode(pDisplayModeOut, uMode);
	}

	return false;
}

bool grcAdapterD3D11::GetClosestMode(grcDisplayWindow* pDisplayModeInOut, u32 uOutput) const
{
	AssertMsg(uOutput < (u32)m_aOutputs.size(), "Invalid output");

	if (pDisplayModeInOut && (uOutput < (u32)m_aOutputs.size()))
	{
		const DXGI_MODE_DESC& oModeDesc = m_aOutputs[uOutput]->GetClosestMode(*pDisplayModeInOut);

		pDisplayModeInOut->uWidth		= oModeDesc.Width;
		pDisplayModeInOut->uHeight		= oModeDesc.Height;
		pDisplayModeInOut->uRefreshRate	= oModeDesc.RefreshRate;
//		pDisplayModeInOut->bCentered	= (oModeDesc.Scaling == DXGI_MODE_SCALING_CENTERED);

		return true;
	}

	return false;
}

bool grcAdapterD3D11::GetClosestMode(grcDisplayWindow* pDisplayModeInOut, IDXGIOutput *pOutputMonitor) const
{
	DXGI_OUTPUT_DESC outputMonitorDescription;
	pOutputMonitor->GetDesc(&outputMonitorDescription);

	for (u32 uOutput = 0; uOutput < (u32)m_aOutputs.size(); uOutput++)
	{
		DXGI_OUTPUT_DESC outputDescription;
		unsigned int dpiX, dpiY;
		grcAdapterD3D11Output::GetDesc(m_AdapterHighPart, m_AdapterLowPart, m_aOutputs[uOutput], outputDescription, dpiX, dpiY);

		if (outputDescription.Monitor == outputMonitorDescription.Monitor)
			return GetClosestMode(pDisplayModeInOut, uOutput);
	}
	return false;
}


bool grcAdapterD3D11::GetWidestMode(grcDisplayWindow* pDisplayModeInOut, u32 uOutput) const
{
	AssertMsg(uOutput < (u32)m_aOutputs.size(), "Invalid output");

	if (pDisplayModeInOut && (uOutput < (u32)m_aOutputs.size()))
	{
		const DXGI_MODE_DESC& oModeDesc = m_aOutputs[uOutput]->GetWidestMode();

		pDisplayModeInOut->uWidth		= oModeDesc.Width;
		pDisplayModeInOut->uHeight		= oModeDesc.Height;
		pDisplayModeInOut->uRefreshRate	= oModeDesc.RefreshRate;
		//		pDisplayModeInOut->bCentered	= (oModeDesc.Scaling == DXGI_MODE_SCALING_CENTERED);

		return true;
	}

	return false;
}

bool grcAdapterD3D11::GetClosestModeMatchingAspect (grcDisplayWindow* pDisplayModeInOut, float aspect, u32 uOutput) const
{
	AssertMsg(uOutput < (u32)m_aOutputs.size(), "Invalid output");

	if (pDisplayModeInOut && (uOutput < (u32)m_aOutputs.size()))
	{
		const DXGI_MODE_DESC& oModeDesc = m_aOutputs[uOutput]->GetClosestModeMatchingAspect(*pDisplayModeInOut, aspect);

		pDisplayModeInOut->uWidth		= oModeDesc.Width;
		pDisplayModeInOut->uHeight		= oModeDesc.Height;
		pDisplayModeInOut->uRefreshRate	= oModeDesc.RefreshRate;
		//		pDisplayModeInOut->bCentered	= (oModeDesc.Scaling == DXGI_MODE_SCALING_CENTERED);

		return true;
	}

	return false;
}

#if NV_SUPPORT
void grcAdapterD3D11::GetNvidiaDisplayInfo(grcDisplayInfo& info)
{
	// PC TODO - Figure out how to match index to actual adapter/display we are using
	NvDisplayHandle oDisplayHandles [256];
	_NvAPI_Status retValue = NVAPI_OK;
	int numDisplayHandles = 0;

	char adapterName[ 128 ];
	wcstombs(adapterName, info.oAdapterDesc->Description, 128);

	int displayHandleToUse = 0;
	NvPhysicalGpuHandle gpuHandleToUse = NULL;

	for (; retValue != NVAPI_NO_IMPLEMENTATION && retValue != NVAPI_END_ENUMERATION && retValue != NVAPI_INVALID_ARGUMENT; numDisplayHandles++)
	{
		retValue = NvAPI_EnumNvidiaDisplayHandle((NvU32)numDisplayHandles, &oDisplayHandles[numDisplayHandles]);

		if (retValue == NVAPI_OK)
		{
			NvPhysicalGpuHandle nvPhysicalGPUHandle[NVAPI_MAX_PHYSICAL_GPUS] = {NULL};
			NvU32 gpuCount;
			_NvAPI_Status gpuRetValue = NvAPI_GetPhysicalGPUsFromDisplay(oDisplayHandles[numDisplayHandles], nvPhysicalGPUHandle, &gpuCount);

			if (gpuRetValue == NVAPI_OK)
			{
				NvAPI_ShortString name;
				NvAPI_GPU_GetFullName(nvPhysicalGPUHandle[0], name);

				if (strstr(adapterName, name))
				{
					displayHandleToUse = numDisplayHandles;
					gpuHandleToUse = nvPhysicalGPUHandle[0];
				}
			}
		}
	}
	numDisplayHandles--;

	if (!gpuHandleToUse)
	{
		NvPhysicalGpuHandle nvPhysicalGPUHandle[NVAPI_MAX_PHYSICAL_GPUS];
		NvU32 gpuCount;
		retValue = NvAPI_EnumPhysicalGPUs(nvPhysicalGPUHandle, &gpuCount);
		if (retValue == NVAPI_OK)
		{
			gpuHandleToUse = nvPhysicalGPUHandle[0];
		}
	}

	if (gpuHandleToUse && numDisplayHandles)
	{
		NvU32 CoreCount;
		NvAPI_GPU_GetGpuCoreCount (gpuHandleToUse, &CoreCount);
		info.numOfCores = CoreCount;

		NV_GPU_CLOCK_FREQUENCIES_V2 oClockTable;
		ZeroMemory(&oClockTable, sizeof(oClockTable));
		oClockTable.version = NV_GPU_CLOCK_FREQUENCIES_VER_2;
		oClockTable.ClockType = NV_GPU_CLOCK_FREQUENCIES_BASE_CLOCK;
		retValue = NvAPI_GPU_GetAllClockFrequencies(gpuHandleToUse, &oClockTable);

		if (retValue == NVAPI_OK) {
			retValue = NvAPI_GPU_GetAllClockFrequencies(gpuHandleToUse, &oClockTable);

			for (u32 index = 0; index < NVAPI_MAX_GPU_PUBLIC_CLOCKS; index++) {
				if (oClockTable.domain[index].bIsPresent)
				{
					if (index == NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS)
					{
						info.gpuClockMax = oClockTable.domain[index].frequency; //khz
					}
					else if (index == NVAPI_GPU_PUBLIC_CLOCK_MEMORY)
					{
						info.memClockMax = oClockTable.domain[index].frequency; //khz
					}
					else if (index == NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR)
					{
					}
				}
			}
		}

		NvU32 uMemSize;
		retValue = NvAPI_GPU_GetPhysicalFrameBufferSize(gpuHandleToUse, &uMemSize);
		if (retValue == NVAPI_OK)
		{
			info.videoMemSize = (s64)uMemSize * (s64)1024;
		}
		retValue = NvAPI_GPU_GetVirtualFrameBufferSize(gpuHandleToUse, &uMemSize);
		if (retValue == NVAPI_OK)
		{
			info.sharedMemSize = (s64)uMemSize * (s64)1024;
		}
	}

	//		NVAPI_INTERFACE NvAPI_EnumPhysicalGPUs(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount)
	//		NVAPI_INTERFACE NvAPI_EnumLogicalGPUs(NvLogicalGpuHandle nvGPUHandle[NVAPI_MAX_LOGICAL_GPUS], NvU32 *pGpuCount)
	//		NVAPI_INTERFACE NvAPI_GetLogicalGPUFromDisplay(NvDisplayHandle hNvDisp, NvLogicalGpuHandle *pLogicalGPU);
	//		NVAPI_INTERFACE  NvAPI_GPU_GetTotalSMCount (NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pCount)
}
#endif

// A helper macros to declare a constant pointer to the ADL function with the same name
#define GENERIC_FUNCTOR(Dll,Ret,name,...)	\
	Ret(*const name)(__VA_ARGS__) = (Ret(*)(__VA_ARGS__)) GetProcAddress(Dll,#name)
#define ATIFUNCTOR(name,...)	GENERIC_FUNCTOR(hDLL,int,name,__VA_ARGS__)


#if ATI_SUPPORT

namespace adl {
	// these are defined in monitor.cpp and used in this file below
	extern void* __stdcall Main_Memory_Alloc ( int iSize );
	extern void __stdcall Main_Memory_Free ( void** lpBuffer );
}

static char							m_AMDDriverPath[ADL_MAX_PATH] = "";

UINT stripID (char *pstrSource, char* pstrID)
{
		char*pstrIDStart = strstr(pstrSource, pstrID);
		if (!pstrIDStart) return 0;
		char*pstrIDEnd = strstr(pstrIDStart, "&");
		if (!pstrIDEnd) return 0;
		int strLenIDValue = (int)(pstrIDEnd - pstrIDStart) - (int)strlen(pstrID);
		char strIDValue[20];
		memcpy((void*)strIDValue, (void*)(pstrIDStart + (int)strlen(pstrID)), strLenIDValue);
		strIDValue[strLenIDValue] = '\0';
		return (UINT) strtol(strIDValue, NULL, 16);
}

void grcAdapterD3D11::GetATIDisplayInfo (grcDisplayInfo& info)
{
	HINSTANCE hDLL = NULL;
	int  iNumberAdapters;
	LPAdapterInfo     lpAdapterInfo = NULL;
	LPADLDisplayInfo  lpAdlDisplayInfo = NULL;
	int  iPrimaryAdapterIndex;
	ADLMemoryInfo memoryInfo;
	ADLODPerformanceLevels *performanceLevels;
	ADLODParameters clockParameters;

	hDLL = LoadLibrary("atiadlxx.dll");
	if (hDLL == NULL)
	{
		// A 32 bit calling application on 64 bit OS will fail to LoadLIbrary.
		// Try to load the 32 bit library (atiadlxy.dll) instead
		hDLL = LoadLibrary("atiadlxy.dll");
	}
	if (hDLL == NULL)
	{
		Printf("ADL: Failed to load the library!\n");
		return;
	}

	ATIFUNCTOR( ADL_Main_Control_Create, ADL_MAIN_MALLOC_CALLBACK, int );
	ATIFUNCTOR( ADL_Main_Control_Destroy );
	ATIFUNCTOR( ADL_Adapter_NumberOfAdapters_Get, int* );
	ATIFUNCTOR( ADL_Adapter_AdapterInfo_Get, LPAdapterInfo, int );
	ATIFUNCTOR( ADL_Adapter_MemoryInfo_Get, int, ADLMemoryInfo* );
	//	ATIFUNCTOR( ADL_Adapter_ClockInfo_Get, int, ADLClockInfo* );
	ATIFUNCTOR( ADL_Adapter_Primary_Get, int* );
	ATIFUNCTOR( ADL_Overdrive5_ODPerformanceLevels_Get, int, int, ADLODPerformanceLevels* );
	ATIFUNCTOR( ADL_Overdrive5_ODParameters_Get, int, ADLODParameters* );
	ATIFUNCTOR( ADL_Display_DisplayInfo_Get, int, int*, ADLDisplayInfo**, int );

	if (NULL == ADL_Main_Control_Create				||
		NULL == ADL_Main_Control_Destroy			||
		NULL == ADL_Adapter_NumberOfAdapters_Get	||
		NULL == ADL_Adapter_AdapterInfo_Get			||
		NULL == ADL_Adapter_MemoryInfo_Get			||
		NULL == ADL_Adapter_Primary_Get				||
		NULL == ADL_Overdrive5_ODParameters_Get		||
		NULL == ADL_Display_DisplayInfo_Get) 
	{
		Printf(false, "ADL's API is missing!\n");
		return;
	}

	// Initialize ADL. The second parameter is 1, which means:
	// retrieve adapter information only for adapters that are physically present and enabled in the system
	if ( ADL_OK != ADL_Main_Control_Create (adl::Main_Memory_Alloc, 1) )
	{
		Printf("ADL Initialization Error!\n");
		return;
	}

	// Obtain the number of adapters for the system
	if ( ADL_OK != ADL_Adapter_NumberOfAdapters_Get ( &iNumberAdapters ) )
	{
		Printf("Cannot get the number of adapters!\n");
		return;
	}

	if ( ADL_OK != ADL_Adapter_Primary_Get (&iPrimaryAdapterIndex) )
	{
		Printf("Cannot get primary adapter");
		return;
	}

	if ( 0 < iNumberAdapters )
	{
		lpAdapterInfo = (LPAdapterInfo) malloc ( sizeof (AdapterInfo) * iNumberAdapters );
		memset ( lpAdapterInfo,'\0', sizeof (AdapterInfo) * iNumberAdapters );

		// Get the AdapterInfo structure for all adapters in the system
		ADL_Adapter_AdapterInfo_Get (lpAdapterInfo, sizeof (AdapterInfo) * iNumberAdapters);
	}

	char adapterName[ 128 ];
	wcstombs(adapterName, info.oAdapterDesc->Description, 128);
	// Repeat for all available adapters in the system
	for (int i = 0; i < iNumberAdapters; i++ )
	{
		if ((stripID(lpAdapterInfo[i].strUDID, "DEV_") != info.oAdapterDesc->DeviceId || stripID(lpAdapterInfo[i].strUDID, "VEN_") != info.oAdapterDesc->VendorId)
			&& (strcmp(adapterName, lpAdapterInfo[i].strAdapterName)))
			continue;
		// officially supported only by Linux
		int iRet = ADL_OK;
		if ((iRet = ADL_Adapter_MemoryInfo_Get (lpAdapterInfo[i].iAdapterIndex, &memoryInfo)) == ADL_OK)
		{
			info.videoMemSize = (s64)memoryInfo.iMemorySize;
			info.memBandwidth = (s64)memoryInfo.iMemoryBandwidth * 1024;
			info.sharedMemSize = (s64)info.oAdapterDesc->SharedSystemMemory;
		}else
		{	// default values to not fall under "weak" category
			info.videoMemSize = 512<<20;
			info.memBandwidth = 20<<20;
		}
		if (ADL_OK == ADL_Overdrive5_ODParameters_Get (lpAdapterInfo[i].iAdapterIndex, &clockParameters))
		{
			int performanceSize = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * (clockParameters.iNumberOfPerformanceLevels - 1);

			performanceLevels = (ADLODPerformanceLevels *) malloc(performanceSize);
			performanceLevels->iSize = performanceSize;
			int eRet = ADL_Overdrive5_ODPerformanceLevels_Get (info.iTargetAdapter /*lpAdapterInfo[i].iAdapterIndex*/, 1, performanceLevels);
			if (ADL_OK == eRet)
			{
				info.gpuClockMax = performanceLevels->aLevels[clockParameters.iNumberOfPerformanceLevels - 1].iEngineClock * 10; //khz
				info.memClockMax = performanceLevels->aLevels[clockParameters.iNumberOfPerformanceLevels - 1].iMemoryClock * 10; //khz
			}
		}

		char* startIndex = strstr(lpAdapterInfo[0].strDriverPath, "System");
		if (startIndex)
		{
			strcpy(m_AMDDriverPath, startIndex);
			strcat(m_AMDDriverPath, "\\");
		}
		break;
	}

	adl::Main_Memory_Free ((void**) &lpAdapterInfo );
	adl::Main_Memory_Free ((void**) &lpAdlDisplayInfo );
	ADL_Main_Control_Destroy ();
	FreeLibrary( hDLL );
}
#endif //ATI_SUPPORT...

void grcAdapterD3D11::DetermineAvailableMemory()
{
 	s64 DedicatedVideoMemory = 512 * 1024 * 1024;
	s64 DedicatedSystemMemory = 512 * 1024 * 1024;
	s64 SharedSystemMemory = 512 * 1024 * 1024;
	const SIZE_T uMinVideoMemory = 900 * 1024 * 1024;

	DXGI_ADAPTER_DESC oAdapterDesc;

	IDXGIAdapter * pDXGIAdapter = GetDeviceAdapter();

	strcpy(m_DriverString, "Unknown");

	if( SUCCEEDED( pDXGIAdapter->GetDesc(&oAdapterDesc) ) )
	{
		DedicatedVideoMemory = oAdapterDesc.DedicatedVideoMemory;
		DedicatedSystemMemory = oAdapterDesc.DedicatedSystemMemory;
		SharedSystemMemory = oAdapterDesc.SharedSystemMemory;

#if NV_SUPPORT
		if (GetManufacturer() == NVIDIA)
		{
			grcDisplayInfo info;
			info.oAdapterDesc = &oAdapterDesc;
			info.videoMemSize = -1;
			GetNvidiaDisplayInfo(info);
			if (info.videoMemSize != -1) DedicatedVideoMemory = info.videoMemSize;

			NvU32 NvDriverVersion;
			char NvBuildBranchString[64];
			_NvAPI_Status result = NvAPI_SYS_GetDriverAndBranchVersion(&NvDriverVersion, NvBuildBranchString);
			if (result == NVAPI_OK)
			{
				formatf(m_DriverString, DRIVER_STRING_SIZE, "%d.%02d", NvDriverVersion / 100, NvDriverVersion - (NvDriverVersion / 100) * 100);
			}
		}
#endif

#if ATI_SUPPORT
		if (GetManufacturer() == ATI)
		{
			grcDisplayInfo info;
			info.oAdapterDesc = &oAdapterDesc;
			info.iTargetAdapter = GetAdapterIndex();
			info.videoMemSize = -1;
			GetATIDisplayInfo(info);
			if (info.videoMemSize != -1) DedicatedVideoMemory = info.videoMemSize;

			if (strlen(m_AMDDriverPath) > 1)
			{
				HKEY	hKey = NULL;
				DWORD	dwType = 0;
				DWORD	dwSize = MAX_PATH;
				LRESULT lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, m_AMDDriverPath, 0, KEY_READ, &hKey);

				// Make sure the key exists.
				if (lResult == ERROR_SUCCESS)
				{
					// Make sure the "DriverVersion" key value exists, otherwise return an empty serial number.
					lResult = RegQueryValueEx(hKey, "DriverVersion", 0, &dwType, (BYTE*)m_DriverString, &dwSize);
					if (lResult == ERROR_SUCCESS && dwSize < MAX_PATH)
					{
						m_DriverString[dwSize] = '\0';
					}
					else
					{
						strcpy(m_DriverString, "Unknown");
					}
					RegCloseKey(hKey);
				}
			}
		}
#endif
		if (GetManufacturer() == INTEL) {
			if (DedicatedVideoMemory <= uMinVideoMemory)
			{
				DedicatedVideoMemory = SharedSystemMemory;
			}
		}
	}

	u32 uFreeMemory = 0;

	if(PARAM_availablevidmem.Get(uFreeMemory))
	{
		DedicatedVideoMemory = (s64)uFreeMemory * (s64)1024 * (s64)1024;
		grcDisplayf("Overriding default available video memory to %dMB\n", DedicatedVideoMemory / (1024 * 1024));
		m_AvailableVideoMemory = (s64)(DedicatedVideoMemory);
		return;
	}

	m_AvailableVideoMemory = (s64)(( DedicatedVideoMemory < uMinVideoMemory ) ? ((SharedSystemMemory < uMinVideoMemory) ? (2048ULL * 1024ULL * 1024ULL) : SharedSystemMemory) : DedicatedVideoMemory);
}

u32 grcAdapterD3D11::GetGPUCount()
{
	if (m_GPUCount) return m_GPUCount;

	m_GPUCount = 1;

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
					m_GPUCount = gpuCount;
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

		// enumerate logical gpus
		status = NvAPI_EnumLogicalGPUs(logicalGPUs, &logicalGPUCount);
		if (status != NVAPI_OK)
		{
			grcWarningf("Failed to query for logical GPUs");
			return m_GPUCount;
		}

		// enumerate physical gpus
		status = NvAPI_EnumPhysicalGPUs(physicalGPUs, &physicalGPUCount);
		if (status != NVAPI_OK)
		{
			grcWarningf("Failed to query for physical GPUs");
			return m_GPUCount;
		}

		if(logicalGPUCount < physicalGPUCount)
		{
			m_GPUCount = physicalGPUCount;
		}
	}
#endif

	PARAM_GPUCount.Get(m_GPUCount);

	return m_GPUCount;
}

const char* grcAdapterD3D11::GetDriverString() const
{
	return m_DriverString;
}

bool grcAdapterD3D11::MatchesWith(grcAdapter *baseAdapter) const
{
	const grcAdapterD3D11 *adapter = (const grcAdapterD3D11 *)baseAdapter;

	if (adapter->GetOutputCount() != m_aOutputs.size()) return false;

	bool result = true;

	for (long index = 0; index < m_aOutputs.size(); index++)
	{
		if (!m_aOutputs[index]->MatchesWith(adapter->GetOutput(index)))
		{
			result = false;
		}
	}

	return result;
}

grcAdapterManagerD3D11::grcAdapterManagerD3D11()
{
}

grcAdapterManagerD3D11::~grcAdapterManagerD3D11()
{
}

void grcAdapterManagerD3D11::Enumerate(u32 uFormat)
{
	do
	{
		HINSTANCE hDXGI = LoadLibrary("dxgi.dll");
		if (!hDXGI)
		{
			Errorf("Unable to open DXGI library");
			break;
		}

		LPCREATEDXGIFACTORY pCreateDXGIFactory = NULL;
		IDXGIFactory* pFactory = NULL;

		pCreateDXGIFactory = (LPCREATEDXGIFACTORY)GetProcAddress(hDXGI, "CreateDXGIFactory");
		HRESULT hr = pCreateDXGIFactory(__uuidof(IDXGIFactory), (LPVOID*)&pFactory);

		if (!SUCCEEDED(hr))
		{
			FreeLibrary(hDXGI);
			Errorf("Unable to create DXGI factory");
			break;
		}
		
		UINT uiAdapterIndex = 0;
		IDXGIAdapter* pAdapter = NULL;
		
		while (pFactory->EnumAdapters(uiAdapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND)
		{
			grcAdapterD3D11* pAdapterD3D11 = rage_new grcAdapterD3D11(pAdapter, uFormat);
			pAdapterD3D11->SetAdapterIndex(sm_aAdapters.size());
			pAdapterD3D11->DetermineAvailableMemory();
			sm_aAdapters.PushAndGrow(pAdapterD3D11);

			if (pAdapterD3D11->GetOculusOutputIndex() >= 0)
			{
				sm_OculusAdapter = (s32)uiAdapterIndex;
			}

			pAdapter = NULL;
			uiAdapterIndex++;
		}

		pFactory->Release();
		//FreeLibrary(hDXGI);

		if (uiAdapterIndex)
			return;
	}while(0);

	Quitf(ERR_GFX_D3D_NOD3D1X_4,"Failed to enumerate DX11 adapters");
}


bool grcAdapterManagerD3D11::VerifyAdapters(u32 uFormat)
{
	do
	{
		HINSTANCE hDXGI = LoadLibrary("dxgi.dll");
		if (!hDXGI)
		{
			Errorf("Unable to open DXGI library");
			break;
		}

		LPCREATEDXGIFACTORY pCreateDXGIFactory = NULL;
		IDXGIFactory* pFactory = NULL;

		pCreateDXGIFactory = (LPCREATEDXGIFACTORY)GetProcAddress(hDXGI, "CreateDXGIFactory");
		HRESULT hr = pCreateDXGIFactory(__uuidof(IDXGIFactory), (LPVOID*)&pFactory);

		if (!SUCCEEDED(hr))
		{
			FreeLibrary(hDXGI);
			Errorf("Unable to create DXGI factory");
			break;
		}

		UINT uiAdapterIndex = 0;
		IDXGIAdapter* pAdapter = NULL;

		atArray<grcAdapter*> aAdaptersList;

		while (pFactory->EnumAdapters(uiAdapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND)
		{
			grcAdapterD3D11* pAdapterD3D11 = rage_new grcAdapterD3D11(pAdapter, uFormat);
			pAdapterD3D11->SetAdapterIndex(aAdaptersList.size());
			aAdaptersList.PushAndGrow(pAdapterD3D11);
			pAdapter = NULL;
			uiAdapterIndex++;
		}

		bool result = true;

		if (aAdaptersList.size() != sm_aAdapters.size()) result = false;

		if (result)
		{
			for (long index = 0; index < aAdaptersList.size(); index++)
			{
				if (!aAdaptersList[index]->MatchesWith(sm_aAdapters[index]))
					result = false;
			}
		}

		for (long index = aAdaptersList.size() - 1; index >= 0; index--)
		{
			delete aAdaptersList[index];
		}

		pFactory->Release();

		return result;
		//FreeLibrary(hDXGI);
	}while(0);

	return false;
}

} // namespace rage

#endif // RSG_PC && HIGHEST_D3D_SUPPORTED >= 1000
