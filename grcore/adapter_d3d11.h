//
// grcore/adapter_d3d11.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_ADAPTER_D3D11_H
#define GRCORE_ADAPTER_D3D11_H

#if RSG_PC && __D3D11

#include "grcore/adapter.h"
#include <vector>

namespace rage {

enum OutputRotation { // Shifted values conform to Vulkan VkSurfaceTransformFlagBitsKHR
	roUnspecified, 
	roIdentity, 
	roRotate90, 
	roRotate180, 
	roRotate270,
	roHorizontal_Mirror,
	roHorizontal_Mirror_Rotate90, 
	roHorizontal_Mirror_Rotate180,
	roHorizontal_Mirror_Rotate270,
	roInheritBit
};

// This is like the Win32 RECT class, not our fwRect class.
template <class T> struct Rect {
public:
	Rect() { }
	Rect(T l, T t, T r, T b) : left(l), top(t), right(r), bottom(b) {
		Assert(left <= right);
		// If this fires, it's probably because somebody was expecting fwRect's reversed notion of top and bottom.
		Assert(top <= bottom);
	}

	void GetCenter(T &x, T &y) const {
		x = (left + right) / 2;
		y = (top + bottom) / 2;
	}

	T GetWidth() const {
		return right - left;
	}
	T GetHeight() const {
		return bottom - top;
	}
	T GetLeft() const {
		return left;
	}
	T GetRight() const {
		return right;
	}
	T GetTop() const {
		return top;
	}
	T GetBottom() const {
		return bottom;
	}

	T left, top, right, bottom;
};

enum DisplayColorSpace {
	csRGB_Full_G10_BT709,
	csRGB_Limited_G22_BT709,
	csRGB_Full_G22_BT709, // Default SDR color space
	csRGB_Limited_G24_BT709,
	csRGB_Limited_G2084_BT2020,
	csRGB_Full_G2084_BT2020 // Default HDR color space
};

class grcDisplayWindow;

class grcAdapterD3D11Output {
	friend class grcAdapterD3D11;
public:
	grcAdapterD3D11Output(s32 adapterHighPart, u32 adapterLowPart, IDXGIOutput* pDeviceOutput, u32 uFormat);
	~grcAdapterD3D11Output();

	static bool GetDesc(s32 adapterHighPart, u32 adapterLowPart, const grcAdapterD3D11Output *pOutput, DXGI_OUTPUT_DESC& outputDesc, unsigned int &dpiX, unsigned int &dpiY);

	u32  GetModeCount() const										{ return m_uModeCount; }
	bool GetMode(grcDisplayWindow* pDisplayModeOut, u32 uMode) const;
	const DXGI_MODE_DESC& GetClosestMode(const grcDisplayWindow& rDisplayWindow) const;
	const DXGI_MODE_DESC& GetWidestMode() const;
	const DXGI_MODE_DESC& GetClosestModeMatchingAspect(const grcDisplayWindow& rDisplayWindow, float aspect) const;
	RefreshRate GetClosestRefreshRate(float refreshRate, const grcDisplayWindow& rDisplayWindow) const;

	u32  GetLargestModeIndex() const								{ return m_uLargestMode; }
	bool SupportsStereo() const										{ return m_bSupportsStereo; }
	bool SupportsHeadTracking() const								{ return m_bSupportsHeadTracking; }

	IDXGIOutput* GetDeviceOutput() const							{ return m_pDeviceOutput; }

	bool MatchesWith(const grcAdapterD3D11Output *) const;
	static bool IsOutputPortrait(DXGI_OUTPUT_DESC& oDesc);
	Rect<int> GetDesktopCoordinates() const { return DesktopCoordinates; }

private:
	void FilterModes(u32 uModeCount, DXGI_MODE_DESC* aModes, bool* aModeValid);
	void EnumerateModes(u32 uFormat);

	IDXGIOutput*	m_pDeviceOutput;
	DXGI_MODE_DESC*	m_aModes;
	u32				m_uModeCount;
	u32				m_uLargestMode;

	bool			m_bSupportsHeadTracking;
	bool			m_bSupportsStereo;

	unsigned int dpiX, dpiY;
	u32 DisplayConfig_TargetId;
	OutputRotation Rotation;
	Rect<int> DesktopCoordinates;
	void *Monitor;
};

#define DRIVER_STRING_SIZE 64
class grcAdapterD3D11 : public grcAdapter {
	friend class grcAdapterManagerD3D11;
public:
	grcAdapterD3D11(IDXGIAdapter* pDeviceAdapter, u32 uFormat);
	~grcAdapterD3D11();

	IDXGIAdapter* GetDeviceAdapter() const							{ return m_pDeviceAdapter; }

	int GetOutputCount() const										{ return static_cast<int>(m_aOutputs.size()); }
	const grcAdapterD3D11Output* GetOutput(u32 uOutput) const;
	const grcAdapterD3D11Output* GetOculusOutput() const;

	u32  GetModeCount(u32 uOutput = 0) const;
	bool GetMode(grcDisplayWindow* pDisplayModeOut, u32 uMode, u32 uOutput = 0) const;
	bool GetClosestMode(grcDisplayWindow* pDisplayModeInOut, u32 uOutput = 0) const;
	bool GetClosestMode(grcDisplayWindow* pDisplayModeInOut, IDXGIOutput *pOutputMonitor) const;
	u32  GetLargestModeIndex(u32 uOutput = 0) const;
	bool GetWidestMode (grcDisplayWindow* pDisplayModeInOut, u32 uOutput = 0) const;

	bool GetClosestModeMatchingAspect (grcDisplayWindow* pDisplayModeInOut, float aspect, u32 uOutput = 0) const;

	DeviceManufacturer GetManufacturer() const { return m_eManufacturer; }

	void DetermineAvailableMemory();
	s64 GetAvailableVideoMemory() {
		return m_AvailableVideoMemory;
	}

	void SetAdapterIndex(int index) {m_adapterIndex = index;}
	int GetAdapterIndex() {return m_adapterIndex;}

	s32 GetHighPart() const { return m_AdapterHighPart; }
	u32	GetLowPart() const { return m_AdapterLowPart; }

#if NV_SUPPORT
	static void GetNvidiaDisplayInfo(grcDisplayInfo&);
#endif
#if ATI_SUPPORT
	static void GetATIDisplayInfo(grcDisplayInfo&);
#endif

	u32 GetGPUCount();

	const char* GetDriverString() const;

	bool MatchesWith(grcAdapter *) const;

private:
	bool SetupAdapterOutputWindows();
	void EnumerateOutputs(u32 uFormat);
	void DetermineManufacturer();

	IDXGIAdapter*						m_pDeviceAdapter;
	atArray<grcAdapterD3D11Output*>		m_aOutputs;
	DeviceManufacturer					m_eManufacturer;
	int									m_adapterIndex;
	s64									m_AvailableVideoMemory;
	u32									m_GPUCount;

	char								m_DriverString[DRIVER_STRING_SIZE];
	s32									m_AdapterHighPart;
	u32									m_AdapterLowPart;
};

class grcAdapterManagerD3D11 : public grcAdapterManager {

public:
	grcAdapterManagerD3D11();
	virtual ~grcAdapterManagerD3D11();

	virtual bool VerifyAdapters(u32 uFormat);
protected:
	virtual void Enumerate(u32 uFormat);

private:

};

} // namespace rage

#endif // RSG_PC && HIGHEST_D3D_SUPPORTED >= 1000

#endif // GRCORE_ADAPTER_D3D11_H
