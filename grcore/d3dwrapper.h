// 
// grcore/d3dwrapper.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_D3DWRAPPER_H 
#define GRCORE_D3DWRAPPER_H 

#if (RSG_PC || RSG_DURANGO)
#include "grcore/config.h"

#include "system/xtl.h"
#include "system/d3d9.h"
#include "system/d3d11.h"

namespace rage {

#if __WIN32PC
extern HMODULE g_hD3D9;
extern IDirect3D9 * (WINAPI *g_Direct3DCreate9)(UINT SDKVersion);
extern int (WINAPI *g_D3DPERF_BeginEvent)( D3DCOLOR col, LPCWSTR wszName );
extern int (WINAPI *g_D3DPERF_EndEvent)( void );
extern int (WINAPI *g_D3DPERF_GetStatus)( void );
#endif

#if __D3D11

extern HMODULE g_hD3D11;

extern HRESULT (WINAPI *g_D3D11CreateDevice)(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    __out_opt ID3D11DeviceContext** ppImmediateContext );

extern HRESULT (WINAPI *g_D3D11CreateDeviceAndSwapChain)(
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

} // namespace rage

#endif

#endif // GRCORE_D3DWRAPPER_H 
