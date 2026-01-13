// 
// grcore/d3dwrapper.cpp 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/d3dwrapper.h"

namespace rage {

#if __WIN32PC

HMODULE g_hD3D9;
IDirect3D9 * (WINAPI *g_Direct3DCreate9)(UINT SDKVersion);
int (WINAPI *g_D3DPERF_BeginEvent)( D3DCOLOR col, LPCWSTR wszName );
int (WINAPI *g_D3DPERF_EndEvent)( void );
int (WINAPI *g_D3DPERF_GetStatus)( void );

#if __D3D11
HMODULE g_hD3D10;
HRESULT (WINAPI *g_D3D10CreateDevice1)(
	IDXGIAdapter *pAdapter,
	D3D10_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	D3D10_FEATURE_LEVEL1 HardwareLevel,
	UINT SDKVersion,
	ID3D10Device1 **ppDevice);
HRESULT (WINAPI *g_D3D10CreateDeviceAndSwapChain1)(
	IDXGIAdapter *pAdapter,
	D3D10_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	D3D10_FEATURE_LEVEL1 HardwareLevel,
	UINT SDKVersion,
	DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
	IDXGISwapChain **ppSwapChain,
	ID3D10Device1 **ppDevice);
#endif

#if __D3D11
HMODULE g_hD3D11;
HRESULT (WINAPI *g_D3D11CreateDevice)(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext );

HRESULT (WINAPI *g_D3D11CreateDeviceAndSwapChain)(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    IDXGISwapChain** ppSwapChain,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext );
#endif

#endif

} // namespace rage
