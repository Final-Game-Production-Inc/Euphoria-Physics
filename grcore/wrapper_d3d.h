// 
// grcore/wrapper_d3d.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#ifndef GRCORE_WRAPPER_D3D_H
#define GRCORE_WRAPPER_D3D_H

#if __FINAL || (!RSG_PC && !RSG_DURANGO)
#define CHECK_HRESULT(hr) (hr)
#define CHECKING_DX_HRESULTS 0
#endif

#if RSG_PC || RSG_DURANGO

#include "system/xtl.h"
#include "grcore/d3dwrapper.h"
#include "grcore/device.h"
#include "grcore/resourcecache.h"
#include "math/amath.h"

#ifndef CHECKING_DX_HRESULTS
#if RSG_PC
#if __D3D11_1
#include "grcore/DxErr.h"
#else
#include <DxErr.h>
#pragma comment(lib,"DxErr.lib")
#endif
#else // RSG_PC
#include <xdk.h>
#endif // RSG_PC

HRESULT CheckDxHresult(HRESULT hr);
#define CHECK_HRESULT(hr) CheckDxHresult(hr)
#define CHECKING_DX_HRESULTS 1
#endif

void CheckDxHresultFatal(HRESULT hr);

// DX11 TODO:- Check render targets against bound textures (we only check textures against bound render targets currently).
#define __D3D11_REPORT_RESOURCE_HAZARDS ( 0 && DEVICE_USE_D3D_WRAPPER )

#define NR() AssertMsg(!m_Next,"Cannot call this function in a recording.");

#define SIZED(count,cmd)					(((count)<<8)|(cmd))
#define RECORD1(func,a)						if (m_Remain>2) { m_Next[0] = SIZED(2,REC_##func); m_Next[1] = (DWORD)(a); m_Remain-=2; m_Next+=2; } else m_Remain = 0;
#define RECORD2(func,a,b)					if (m_Remain>3) { m_Next[0] = SIZED(3,REC_##func); m_Next[1] = (DWORD)(a); m_Next[2] = (DWORD)(b); m_Remain-=3; m_Next+=3; return S_OK; } else m_Remain = 0;
#define RECORD3(func,a,b,c)					if (m_Remain>4) { m_Next[0] = SIZED(4,REC_##func); m_Next[1] = (DWORD)(a); m_Next[2] = (DWORD)(b); m_Next[3] = (DWORD)(c); m_Remain-=4; m_Next+=4; return S_OK; } else m_Remain = 0;
#define RECORD4(func,a,b,c,d)				if (m_Remain>5) { m_Next[0] = SIZED(5,REC_##func); m_Next[1] = (DWORD)(a); m_Next[2] = (DWORD)(b); m_Next[3] = (DWORD)(c); m_Next[4] = (DWORD)(d); m_Remain-=5; m_Next+=5; return S_OK; } else m_Remain = 0;
#define RECORD5(func,a,b,c,d,e)				if (m_Remain>6) { m_Next[0] = SIZED(6,REC_##func); m_Next[1] = (DWORD)(a); m_Next[2] = (DWORD)(b); m_Next[3] = (DWORD)(c); m_Next[4] = (DWORD)(d); m_Next[5] = (DWORD)(e); m_Remain-=6; m_Next+=6; return S_OK; } else m_Remain = 0;
#define RECORD6(func,a,b,c,d,e,f)			if (m_Remain>7) { m_Next[0] = SIZED(7,REC_##func); m_Next[1] = (DWORD)(a); m_Next[2] = (DWORD)(b); m_Next[3] = (DWORD)(c); m_Next[4] = (DWORD)(d); m_Next[5] = (DWORD)(e); m_Next[6] = (DWORD)(f); m_Remain-=7; m_Next+=7; return S_OK; } else m_Remain = 0;
#define COPY(func,start,pData,count,size)	if (m_Remain>3+(size)) { m_Next[0] = SIZED(3+(size),REC_##func); m_Next[1] = (DWORD)(start); m_Next[2] = (DWORD)(count); for (DWORD i=0; i<(size); i++) m_Next[3+i] = ((DWORD*)pData)[i]; m_Remain-=(3+(size)); m_Next+=(3+(size)); return S_OK; } else m_Remain = 0;

enum {
	REC_SetVertexDeclaration,
	REC_SetVertexShader,
	REC_SetIndices,
	REC_SetPixelShader,
	REC_SetRenderState,
	REC_SetTexture,
	REC_SetSamplerState,
	REC_DrawPrimitive,
	REC_DrawIndexedPrimitive,
	REC_SetStreamSource,
	REC_SetStreamSourceFreq,
	REC_SetVertexShaderConstantF,
	REC_SetVertexShaderConstantI,
	REC_SetVertexShaderConstantB,
	REC_SetPixelShaderConstantF,
	REC_SetPixelShaderConstantI,
	REC_SetPixelShaderConstantB,
	REC_SetCommandBufferPredication
};

#define DEVICE_EKG	( 1 && __BANK /*__STATS*/ && __D3D11 )

#if DEVICE_EKG

// Note that timing the calls is good for relative comparisons but slows the game down to a crawl
#define DEVICE_EKG_TIMECALLS ( 0 )

#include "profile/element.h"
#include "profile/page.h"

#define DEVICE_EKG_ONLY(...)			__VA_ARGS__
#define DEVICE_EKG_INCREMENT(x)			PF_INCREMENT(x)

#if DEVICE_EKG_TIMECALLS
#define DEVICE_EKG_TIME(name)			PF_FUNC(name)
#define DEVICE_EKG_STARTTIMER(name)		PF_START(name)
#define DEVICE_EKG_STOPTIMER(name)		PF_STOP(name)
#else // DEVICE_EKG_TIMECALLS
#define DEVICE_EKG_TIME(name)
#define DEVICE_EKG_STARTTIMER(name)
#define DEVICE_EKG_STOPTIMER(name)
#endif // DEVICE_EKG_TIMECALLS

#define DEVICE_EKG_APICALL				DEVICE_EKG_INCREMENT(APICount_AllCalls);	DEVICE_EKG_TIME(APITimer_AllCalls);		DEVICE_EKG_INCREMENT(APICount_Other);	DEVICE_EKG_TIME(APITimer_Other);
#define DEVICE_EKG_COUNT(name)			DEVICE_EKG_INCREMENT(APICount_AllCalls);	DEVICE_EKG_INCREMENT(APICount_##name);
#define DEVICE_EKG_TIMER(name)			DEVICE_EKG_INCREMENT(APICount_AllCalls);	DEVICE_EKG_TIME(APITimer_##name);		DEVICE_EKG_TIME(APITimer_AllCalls);
#define DEVICE_EKG_COUNTANDTIME(name)	DEVICE_EKG_INCREMENT(APICount_AllCalls);	DEVICE_EKG_INCREMENT(APICount_##name);	DEVICE_EKG_TIME(APITimer_##name);		DEVICE_EKG_TIME(APITimer_AllCalls);
#define DEVICE_EKG_START(name)			DEVICE_EKG_INCREMENT(APICount_AllCalls);	DEVICE_EKG_INCREMENT(APICount_##name);	DEVICE_EKG_STARTTIMER(APITimer_##name);	DEVICE_EKG_STARTTIMER(APITimer_AllCalls);
#define DEVICE_EKG_STOP(name)			DEVICE_EKG_STOPTIMER(APITimer_##name);		DEVICE_EKG_STOPTIMER(APITimer_AllCalls);

#define EXT_DEVICE_EKG_MONITOR_MAJOR(name)	EXT_PF_TIMER(APITimer_##name);				EXT_PF_COUNTER(APICount_##name);
#define EXT_DEVICE_EKG_MONITOR_MINOR(name)	EXT_PF_TIMER(APITimer_##name);				EXT_PF_COUNTER(APICount_##name);

EXT_DEVICE_EKG_MONITOR_MAJOR(AllCalls);
EXT_DEVICE_EKG_MONITOR_MINOR(Other);					// API calls we really don't expect to need profiling
EXT_DEVICE_EKG_MONITOR_MAJOR(Draw);
EXT_DEVICE_EKG_MONITOR_MAJOR(Map);
EXT_DEVICE_EKG_MONITOR_MAJOR(UnMap);
EXT_DEVICE_EKG_MONITOR_MAJOR(MapCBuffer);
EXT_DEVICE_EKG_MONITOR_MAJOR(UnMapCBuffer);
EXT_DEVICE_EKG_MONITOR_MINOR(IASetVertexBuffers);
EXT_DEVICE_EKG_MONITOR_MINOR(IASetVertexBuffersTotal);
EXT_DEVICE_EKG_MONITOR_MINOR(IASetVertexBuffersSingleCount);
EXT_DEVICE_EKG_MONITOR_MINOR(CreateBuffer);
EXT_DEVICE_EKG_MONITOR_MINOR(CreateTexture);
EXT_DEVICE_EKG_MONITOR_MINOR(PrivateData);

EXT_DEVICE_EKG_MONITOR_MINOR(CPUFlush);
EXT_DEVICE_EKG_MONITOR_MINOR(GPUFlush);

// DeviceContext funcs
EXT_DEVICE_EKG_MONITOR_MINOR(CopyResource);
EXT_DEVICE_EKG_MONITOR_MINOR(CopySubResourceRegion);
EXT_DEVICE_EKG_MONITOR_MINOR(CopyStructureCount);
EXT_DEVICE_EKG_MONITOR_MINOR(IASetIndexBuffer);
EXT_DEVICE_EKG_MONITOR_MINOR(IASetPrimitiveTopology);
EXT_DEVICE_EKG_MONITOR_MINOR(IASetInputLayout);
EXT_DEVICE_EKG_MONITOR_MINOR(PSSetConstantBuffers);
EXT_DEVICE_EKG_MONITOR_MINOR(PSSetConstantBuffers1);
EXT_DEVICE_EKG_MONITOR_MINOR(PSSetPlacementConstantBuffer);
EXT_DEVICE_EKG_MONITOR_MINOR(PSSetSamplers);
EXT_DEVICE_EKG_MONITOR_MINOR(PSSetShader);
EXT_DEVICE_EKG_MONITOR_MINOR(PSSetShaderResources);
EXT_DEVICE_EKG_MONITOR_MINOR(PSSetFastShaderResource);
EXT_DEVICE_EKG_MONITOR_MINOR(RSSets);
EXT_DEVICE_EKG_MONITOR_MINOR(VSSetConstantBuffers);
EXT_DEVICE_EKG_MONITOR_MINOR(VSSetConstantBuffers1);
EXT_DEVICE_EKG_MONITOR_MINOR(VSSetPlacementConstantBuffer);
EXT_DEVICE_EKG_MONITOR_MINOR(VSSetSamplers);
EXT_DEVICE_EKG_MONITOR_MINOR(VSSetSamplers);
EXT_DEVICE_EKG_MONITOR_MINOR(VSSetShader);
EXT_DEVICE_EKG_MONITOR_MINOR(VSSetShaderResources);
EXT_DEVICE_EKG_MONITOR_MINOR(OMSetBlendState);
EXT_DEVICE_EKG_MONITOR_MINOR(OMSetDepthStencilState);
EXT_DEVICE_EKG_MONITOR_MINOR(OMSetRenderTargets);
EXT_DEVICE_EKG_MONITOR_MINOR(OMSetRenderTargetsAndUnorderedAccessViews);

EXT_DEVICE_EKG_MONITOR_MINOR(ResolveSubresource);
EXT_DEVICE_EKG_MONITOR_MINOR(UpdateSubresource);

EXT_DEVICE_EKG_MONITOR_MINOR(ResourceGets);

EXT_DEVICE_EKG_MONITOR_MINOR(CSResourceSets);
EXT_DEVICE_EKG_MONITOR_MINOR(CSSetConstantBuffers);
EXT_DEVICE_EKG_MONITOR_MINOR(Dispatch);
EXT_DEVICE_EKG_MONITOR_MINOR(DSResourceSets);
EXT_DEVICE_EKG_MONITOR_MINOR(GSResourceSets);
EXT_DEVICE_EKG_MONITOR_MINOR(HSResourceSets);
EXT_DEVICE_EKG_MONITOR_MINOR(CommandList);
EXT_DEVICE_EKG_MONITOR_MINOR(FinishCommandList);
EXT_DEVICE_EKG_MONITOR_MINOR(Flush);
EXT_DEVICE_EKG_MONITOR_MINOR(GetData);
EXT_DEVICE_EKG_MONITOR_MINOR(MiscCreate);
EXT_DEVICE_EKG_MONITOR_MINOR(Predication);

EXT_DEVICE_EKG_MONITOR_MINOR(Flush);
EXT_DEVICE_EKG_MONITOR_MINOR(Present);	// Actually manually refrerenced inside grcDevice::EndFrame when called on the swap chain

#else // DEVICE_EKG

#define DEVICE_EKG_APICALL
#define DEVICE_EKG_ONLY(...)
#define DEVICE_EKG_INCREMENT(...)
#define DEVICE_EKG_TIME(...)
#define DEVICE_EKG_COUNTANDTIME(...)
#define DEVICE_EKG_START(...)
#define DEVICE_EKG_STOP(...)

#endif	// DEVICE_EKG

#if __D3D9
struct RageDirect3DDevice9: public IDirect3DDevice9
{
#if USE_RESOURCE_CACHE
	friend class rage::grcResourceCache;
#endif // USE_RESOURCE_CACHE

	RageDirect3DDevice9();

	void ClearCachedState();

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) 
		{ return m_Inner->QueryInterface(riid,ppvObj); }

	STDMETHOD_(ULONG,AddRef)(THIS) 
		{ return m_Inner->AddRef(); }

	STDMETHOD_(ULONG,Release)(THIS) 
		{ return m_Inner->Release(); }

	/*** IDirect3DDevice9 methods ***/
	STDMETHOD(TestCooperativeLevel)(THIS) 
		{ NR(); return m_Inner->TestCooperativeLevel(); }

	STDMETHOD_(UINT, GetAvailableTextureMem)(THIS) 
		{ NR(); return m_Inner->GetAvailableTextureMem(); }

	STDMETHOD(EvictManagedResources)(THIS) 
		{ NR(); return m_Inner->EvictManagedResources(); }

	STDMETHOD(GetDirect3D)(THIS_ IDirect3D9** ppD3D9) 
		{ NR(); return m_Inner->GetDirect3D(ppD3D9); }

	STDMETHOD(GetDeviceCaps)(THIS_ D3DCAPS9* pCaps) 
		{ NR(); return m_Inner->GetDeviceCaps(pCaps); }

	STDMETHOD(GetDisplayMode)(THIS_ UINT iSwapChain,D3DDISPLAYMODE* pMode) 
		{ NR(); return m_Inner->GetDisplayMode(iSwapChain,pMode); }

	STDMETHOD(GetCreationParameters)(THIS_ D3DDEVICE_CREATION_PARAMETERS *pParameters) 
		{ NR(); return m_Inner->GetCreationParameters(pParameters); }

	STDMETHOD(SetCursorProperties)(THIS_ UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap) 
		{ NR(); return m_Inner->SetCursorProperties(XHotSpot,YHotSpot,pCursorBitmap); }

	STDMETHOD_(void, SetCursorPosition)(THIS_ int X,int Y,DWORD Flags) 
		{ NR(); m_Inner->SetCursorPosition(X,Y,Flags); }

	STDMETHOD_(BOOL, ShowCursor)(THIS_ BOOL bShow) 
		{ NR(); return m_Inner->ShowCursor(bShow); }

	STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain) 
		{ NR(); return m_Inner->CreateAdditionalSwapChain(pPresentationParameters,pSwapChain); }

	STDMETHOD(GetSwapChain)(THIS_ UINT iSwapChain,IDirect3DSwapChain9** pSwapChain) 
		{ NR(); return m_Inner->GetSwapChain(iSwapChain,pSwapChain); }

	STDMETHOD_(UINT, GetNumberOfSwapChains)(THIS) 
		{ NR(); return m_Inner->GetNumberOfSwapChains(); }

	STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters); 
		//{ NR();return m_Inner->Reset(pPresentationParameters); }

	STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) 
		{ NR(); return m_Inner->Present(pSourceRect,pDestRect,hDestWindowOverride,pDirtyRegion); }

	STDMETHOD(GetBackBuffer)(THIS_ UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer) 
		{ NR(); return m_Inner->GetBackBuffer(iSwapChain,iBackBuffer,Type,ppBackBuffer); }

	STDMETHOD(GetRasterStatus)(THIS_ UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus) 
		{ NR(); return m_Inner->GetRasterStatus(iSwapChain,pRasterStatus); }

	STDMETHOD(SetDialogBoxMode)(THIS_ BOOL bEnableDialogs) 
		{ NR(); return m_Inner->SetDialogBoxMode(bEnableDialogs); }

	STDMETHOD_(void, SetGammaRamp)(THIS_ UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp) 
		{ NR(); m_Inner->SetGammaRamp(iSwapChain,Flags,pRamp); }

	STDMETHOD_(void, GetGammaRamp)(THIS_ UINT iSwapChain,D3DGAMMARAMP* pRamp) 
		{ NR(); m_Inner->GetGammaRamp(iSwapChain,pRamp); }

#if USE_RESOURCE_CACHE
	STDMETHOD(CreateTexture)(THIS_ UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle) 
		{ NR(); return rage::grcResourceCache::GetInstance().CreateTexture(Width,Height,Levels,Usage,Format,Pool,ppTexture,pSharedHandle); }

	STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle) 
		{ NR(); return rage::grcResourceCache::GetInstance().CreateVolumeTexture(Width,Height,Depth,Levels,Usage,Format,Pool,ppVolumeTexture,pSharedHandle); }

	STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle) 
		{ NR(); return rage::grcResourceCache::GetInstance().CreateCubeTexture(EdgeLength,Levels,Usage,Format,Pool,ppCubeTexture,pSharedHandle); }

	STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle) 
		{ NR(); return rage::grcResourceCache::GetInstance().CreateVertexBuffer(Length,Usage,FVF,Pool,ppVertexBuffer,pSharedHandle); }

	STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle) 
		{ NR(); return rage::grcResourceCache::GetInstance().CreateIndexBuffer(Length,Usage,Format,Pool,ppIndexBuffer,pSharedHandle); }

	STDMETHOD(CreateRenderTarget)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) 
		{ NR(); return rage::grcResourceCache::GetInstance().CreateRenderTarget(Width,Height,Format,MultiSample,MultisampleQuality,Lockable,ppSurface,pSharedHandle); }

	STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) 
		{ NR(); return rage::grcResourceCache::GetInstance().CreateDepthStencilSurface(Width,Height,Format,MultiSample,MultisampleQuality,Discard,ppSurface,pSharedHandle); }
#else
	STDMETHOD(CreateTexture)(THIS_ UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle) 
		{ NR(); return m_Inner->CreateTexture(Width,Height,Levels,Usage,Format,Pool,ppTexture,pSharedHandle); }

	STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle) 
		{ NR(); return m_Inner->CreateVolumeTexture(Width,Height,Depth,Levels,Usage,Format,Pool,ppVolumeTexture,pSharedHandle); }

	STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle) 
		{ NR(); return m_Inner->CreateCubeTexture(EdgeLength,Levels,Usage,Format,Pool,ppCubeTexture,pSharedHandle); }

	STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle) 
		{ NR(); return m_Inner->CreateVertexBuffer(Length,Usage,FVF,Pool,ppVertexBuffer,pSharedHandle); }

	STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle) 
		{ NR(); return m_Inner->CreateIndexBuffer(Length,Usage,Format,Pool,ppIndexBuffer,pSharedHandle); }

	STDMETHOD(CreateRenderTarget)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) 
		{ NR(); return m_Inner->CreateRenderTarget(Width,Height,Format,MultiSample,MultisampleQuality,Lockable,ppSurface,pSharedHandle); }

	STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) 
		{ NR(); return m_Inner->CreateDepthStencilSurface(Width,Height,Format,MultiSample,MultisampleQuality,Discard,ppSurface,pSharedHandle); }
#endif // USE_RESOURCE_CACHE

	STDMETHOD(UpdateSurface)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint) 
		{ NR(); return m_Inner->UpdateSurface(pSourceSurface,pSourceRect,pDestinationSurface,pDestPoint); }

	STDMETHOD(UpdateTexture)(THIS_ IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture) 
		{ NR(); return m_Inner->UpdateTexture(pSourceTexture,pDestinationTexture); }

	STDMETHOD(GetRenderTargetData)(THIS_ IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface) 
		{ NR(); return m_Inner->GetRenderTargetData(pRenderTarget,pDestSurface); }

	STDMETHOD(GetFrontBufferData)(THIS_ UINT iSwapChain,IDirect3DSurface9* pDestSurface) 
		{ NR(); return m_Inner->GetFrontBufferData(iSwapChain,pDestSurface); }

	STDMETHOD(StretchRect)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter) 
		{ NR(); return m_Inner->StretchRect(pSourceSurface,pSourceRect,pDestSurface,pDestRect,Filter); }

	STDMETHOD(ColorFill)(THIS_ IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color) 
		{ NR(); return m_Inner->ColorFill(pSurface,pRect,color); }

	STDMETHOD(CreateOffscreenPlainSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) 
		{ NR(); return m_Inner->CreateOffscreenPlainSurface(Width,Height,Format,Pool,ppSurface,pSharedHandle); }

	STDMETHOD(SetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget) 
		{ NR(); return m_Inner->SetRenderTarget(RenderTargetIndex,pRenderTarget); }

	STDMETHOD(GetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget) 
		{ NR(); return m_Inner->GetRenderTarget(RenderTargetIndex,ppRenderTarget); }

	STDMETHOD(SetDepthStencilSurface)(THIS_ IDirect3DSurface9* pNewZStencil) 
		{ NR(); return m_Inner->SetDepthStencilSurface(pNewZStencil); }

	STDMETHOD(GetDepthStencilSurface)(THIS_ IDirect3DSurface9** ppZStencilSurface) 
		{ NR(); return m_Inner->GetDepthStencilSurface(ppZStencilSurface); }

	STDMETHOD(BeginScene)(THIS) 
		{ NR(); return m_Inner->BeginScene(); }

	STDMETHOD(EndScene)(THIS) 
		{ NR(); return m_Inner->EndScene(); }

	STDMETHOD(Clear)(THIS_ DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) 
		{ NR(); return m_Inner->Clear(Count,pRects,Flags,Color,Z,Stencil); }

	STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix) 
		{ NR(); return m_Inner->SetTransform(State,pMatrix); }

	STDMETHOD(GetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) 
		{ NR(); return m_Inner->GetTransform(State,pMatrix); }

	STDMETHOD(MultiplyTransform)(THIS_ D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix) 
		{ NR(); return m_Inner->MultiplyTransform(State,pMatrix); }

	STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT9* pViewport) 
		{ NR(); return m_Inner->SetViewport(pViewport); }

	STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT9* pViewport) 
		{ NR(); return m_Inner->GetViewport(pViewport); }

	STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9* pMaterial) 
		{ NR(); return m_Inner->SetMaterial(pMaterial); }

	STDMETHOD(GetMaterial)(THIS_ D3DMATERIAL9* pMaterial) 
		{ NR(); return m_Inner->GetMaterial(pMaterial); }

	STDMETHOD(SetLight)(THIS_ DWORD Index,CONST D3DLIGHT9* pLight) 
		{ NR(); return m_Inner->SetLight(Index,pLight); }

	STDMETHOD(GetLight)(THIS_ DWORD Index,D3DLIGHT9* pLight) 
		{ NR(); return m_Inner->GetLight(Index,pLight); }

	STDMETHOD(LightEnable)(THIS_ DWORD Index,BOOL Enable) 
		{ NR(); return m_Inner->LightEnable(Index,Enable); }

	STDMETHOD(GetLightEnable)(THIS_ DWORD Index,BOOL* pEnable) 
		{ NR(); return m_Inner->GetLightEnable(Index,pEnable); }

	STDMETHOD(SetClipPlane)(THIS_ DWORD Index,CONST float* pPlane) 
		{ NR(); return m_Inner->SetClipPlane(Index,pPlane); }

	STDMETHOD(GetClipPlane)(THIS_ DWORD Index,float* pPlane) 
		{ NR(); return m_Inner->GetClipPlane(Index,pPlane); }

	STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD Value) 
		{ RECORD2(SetRenderState,State,Value); return ExternSetRenderState(State, Value); } //return m_Inner->SetRenderState(State,Value); }

	STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD* pValue) 
		{ NR(); return m_Inner->GetRenderState(State,pValue); }

	STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB) 
		{ NR(); return m_Inner->CreateStateBlock(Type,ppSB); }

	STDMETHOD(BeginStateBlock)(THIS) 
		{ NR(); return m_Inner->BeginStateBlock(); }

	STDMETHOD(EndStateBlock)(THIS_ IDirect3DStateBlock9** ppSB) 
		{ NR(); return m_Inner->EndStateBlock(ppSB); }

	STDMETHOD(SetClipStatus)(THIS_ CONST D3DCLIPSTATUS9* pClipStatus) 
		{ NR(); return m_Inner->SetClipStatus(pClipStatus); }

	STDMETHOD(GetClipStatus)(THIS_ D3DCLIPSTATUS9* pClipStatus) 
		{ NR(); return m_Inner->GetClipStatus(pClipStatus); }

	STDMETHOD(GetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9** ppTexture) 
		{ NR(); return m_Inner->GetTexture(Stage,ppTexture); }

	STDMETHOD(SetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9* pTexture)
		{ RECORD2(SetTexture,Stage,pTexture); return ExternSetTexture(Stage, pTexture); } // return m_Inner->SetTexture(Stage,pTexture); }

	STDMETHOD(GetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) 
		{ NR(); return m_Inner->GetTextureStageState(Stage,Type,pValue); }

	STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value) 
		{ NR(); return m_Inner->SetTextureStageState(Stage,Type,Value); }

	STDMETHOD(GetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue) 
		{ NR(); return m_Inner->GetSamplerState(Sampler,Type,pValue); }

	STDMETHOD(SetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value)
		{ RECORD3(SetSamplerState,Sampler,Type,Value); return ExternSetSamplerState(Sampler, Type, Value); } // return m_Inner->SetSamplerState(Sampler,Type,Value); }

	STDMETHOD(ValidateDevice)(THIS_ DWORD* pNumPasses) 
		{ NR(); return m_Inner->ValidateDevice(pNumPasses); }

	STDMETHOD(SetPaletteEntries)(THIS_ UINT PaletteNumber,CONST PALETTEENTRY* pEntries) 
		{ NR(); return m_Inner->SetPaletteEntries(PaletteNumber,pEntries); }

	STDMETHOD(GetPaletteEntries)(THIS_ UINT PaletteNumber,PALETTEENTRY* pEntries) 
		{ NR(); return m_Inner->GetPaletteEntries(PaletteNumber,pEntries); }

	STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT PaletteNumber) 
		{ NR(); return m_Inner->SetCurrentTexturePalette(PaletteNumber); }

	STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT *PaletteNumber) 
		{ NR(); return m_Inner->GetCurrentTexturePalette(PaletteNumber); }

	STDMETHOD(SetScissorRect)(THIS_ CONST RECT* pRect) 
		{ NR(); return m_Inner->SetScissorRect(pRect); }

	STDMETHOD(GetScissorRect)(THIS_ RECT* pRect) 
		{ NR(); return m_Inner->GetScissorRect(pRect); }

	STDMETHOD(SetSoftwareVertexProcessing)(THIS_ BOOL bSoftware) 
		{ NR(); return m_Inner->SetSoftwareVertexProcessing(bSoftware); }

	STDMETHOD_(BOOL, GetSoftwareVertexProcessing)(THIS) 
		{ NR(); return m_Inner->GetSoftwareVertexProcessing(); }

	STDMETHOD(SetNPatchMode)(THIS_ float nSegments) 
		{ NR(); return m_Inner->SetNPatchMode(nSegments); }

	STDMETHOD_(float, GetNPatchMode)(THIS) 
		{ NR(); return m_Inner->GetNPatchMode(); }

	STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) 
		{ RECORD3(DrawPrimitive,PrimitiveType,StartVertex,PrimitiveCount); LazyFlush(); return m_Inner->DrawPrimitive(PrimitiveType,StartVertex,PrimitiveCount); }

	STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount) 
		{ RECORD6(DrawIndexedPrimitive,PrimitiveType,BaseVertexIndex,MinVertexIndex,NumVertices,startIndex,primCount); LazyFlush(); return m_Inner->DrawIndexedPrimitive(PrimitiveType,BaseVertexIndex,MinVertexIndex,NumVertices,startIndex,primCount); }

	STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) 
		{ NR(); LazyFlush(); return m_Inner->DrawPrimitiveUP(PrimitiveType,PrimitiveCount,pVertexStreamZeroData,VertexStreamZeroStride); }

	STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) 
		{ NR(); LazyFlush(); return m_Inner->DrawIndexedPrimitiveUP(PrimitiveType,MinVertexIndex,NumVertices,PrimitiveCount,pIndexData,IndexDataFormat,pVertexStreamZeroData,VertexStreamZeroStride); }

	STDMETHOD(ProcessVertices)(THIS_ UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags) 
		{ NR(); return m_Inner->ProcessVertices(SrcStartIndex,DestIndex,VertexCount,pDestBuffer,pVertexDecl,Flags); }

	STDMETHOD(CreateVertexDeclaration)(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl) 
		{ NR(); return m_Inner->CreateVertexDeclaration(pVertexElements,ppDecl); }

	STDMETHOD(SetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9* pDecl) 
		{ RECORD1(SetVertexDeclaration,pDecl); return m_Inner->SetVertexDeclaration(pDecl); }

	STDMETHOD(GetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9** ppDecl) 
		{ NR(); return m_Inner->GetVertexDeclaration(ppDecl); }

	STDMETHOD(SetFVF)(THIS_ DWORD FVF) 
		{ NR(); return m_Inner->SetFVF(FVF); }

	STDMETHOD(GetFVF)(THIS_ DWORD* pFVF) 
		{ NR(); return m_Inner->GetFVF(pFVF); }	

	STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader) 
		{ NR(); return m_Inner->CreateVertexShader(pFunction,ppShader); }

	STDMETHOD(SetVertexShader)(THIS_ IDirect3DVertexShader9* pShader) 
		{ RECORD1(SetVertexShader,pShader); return m_Inner->SetVertexShader(pShader); }

	STDMETHOD(GetVertexShader)(THIS_ IDirect3DVertexShader9** ppShader) 
		{ NR(); return m_Inner->GetVertexShader(ppShader); }

	STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) 
		{ COPY(SetVertexShaderConstantF,StartRegister,pConstantData,Vector4fCount,Vector4fCount<<2); return ExternSetVertexShaderConstantF(StartRegister,pConstantData,Vector4fCount); }

	STDMETHOD(GetVertexShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount) 
		{ NR(); return m_Inner->GetVertexShaderConstantF(StartRegister,pConstantData,Vector4fCount); }

	STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) 
		{ COPY(SetVertexShaderConstantI,StartRegister,pConstantData,Vector4iCount,Vector4iCount<<2); return ExternSetVertexShaderConstantI(StartRegister,pConstantData,Vector4iCount); }

	STDMETHOD(GetVertexShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount) 
		{ NR(); return m_Inner->GetVertexShaderConstantI(StartRegister,pConstantData,Vector4iCount); }

	STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) 
		{ COPY(SetVertexShaderConstantB,StartRegister,pConstantData,BoolCount,BoolCount); return ExternSetVertexShaderConstantB(StartRegister,pConstantData,BoolCount); }

	STDMETHOD(GetVertexShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount) 
		{ NR(); return m_Inner->GetVertexShaderConstantB(StartRegister,pConstantData,BoolCount); }

	STDMETHOD(SetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride) 
		{ RECORD4(SetStreamSource,StreamNumber,pStreamData,OffsetInBytes,Stride); return m_Inner->SetStreamSource(StreamNumber,pStreamData,OffsetInBytes,Stride); }

	STDMETHOD(GetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* pOffsetInBytes,UINT* pStride) 
		{ NR(); return m_Inner->GetStreamSource(StreamNumber,ppStreamData,pOffsetInBytes,pStride); }

	STDMETHOD(SetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT Setting) 
		{ RECORD2(SetStreamSourceFreq,StreamNumber,Setting); return m_Inner->SetStreamSourceFreq(StreamNumber,Setting); }

	STDMETHOD(GetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT* pSetting) 
		{ NR(); return m_Inner->GetStreamSourceFreq(StreamNumber,pSetting); }

	STDMETHOD(SetIndices)(THIS_ IDirect3DIndexBuffer9* pIndexData) 
		{ RECORD1(SetIndices,pIndexData); return m_Inner->SetIndices(pIndexData); }

	STDMETHOD(GetIndices)(THIS_ IDirect3DIndexBuffer9** ppIndexData) 
		{ NR(); return m_Inner->GetIndices(ppIndexData); }

	STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader) 
		{ NR(); return m_Inner->CreatePixelShader(pFunction,ppShader); }

	STDMETHOD(SetPixelShader)(THIS_ IDirect3DPixelShader9* pShader) 
		{ RECORD1(SetPixelShader,pShader); return m_Inner->SetPixelShader(pShader); }

	STDMETHOD(GetPixelShader)(THIS_ IDirect3DPixelShader9** ppShader) 
		{ NR(); return m_Inner->GetPixelShader(ppShader); }

	STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) 
		{ COPY(SetPixelShaderConstantF,StartRegister,pConstantData,Vector4fCount,Vector4fCount<<2); return ExternSetPixelShaderConstantF(StartRegister,pConstantData,Vector4fCount); }

	STDMETHOD(GetPixelShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount) 
		{ NR(); return m_Inner->GetPixelShaderConstantF(StartRegister,pConstantData,Vector4fCount); }

	STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) 
		{ COPY(SetPixelShaderConstantI,StartRegister,pConstantData,Vector4iCount,Vector4iCount<<2); return ExternSetPixelShaderConstantI(StartRegister,pConstantData,Vector4iCount); }

	STDMETHOD(GetPixelShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount) 
		{ NR(); return m_Inner->GetPixelShaderConstantI(StartRegister,pConstantData,Vector4iCount); }

	STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) 
		{ COPY(SetPixelShaderConstantB,StartRegister,pConstantData,BoolCount,BoolCount); return ExternSetPixelShaderConstantB(StartRegister,pConstantData,BoolCount); }

	STDMETHOD(GetPixelShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount) 
		{ NR(); return m_Inner->GetPixelShaderConstantB(StartRegister,pConstantData,BoolCount); }

	STDMETHOD(DrawRectPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) 
		{ NR(); LazyFlush(); return m_Inner->DrawRectPatch(Handle,pNumSegs,pRectPatchInfo); }

	STDMETHOD(DrawTriPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) 
		{ NR(); LazyFlush(); return m_Inner->DrawTriPatch(Handle,pNumSegs,pTriPatchInfo); }

	STDMETHOD(DeletePatch)(THIS_ UINT Handle) 
		{ NR(); return m_Inner->DeletePatch(Handle); }

	STDMETHOD(CreateQuery)(THIS_ D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery) 
		{ NR(); return m_Inner->CreateQuery(Type,ppQuery); }

	void SetCommandBufferPredication(DWORD /*tilePredication*/,DWORD runPredication)
		{ RECORD1(SetCommandBufferPredication, runPredication); }

	IDirect3DDevice9 *m_Inner;
	DWORD *m_Next;
	size_t m_Remain;

	bool m_LazyTouched;

	unsigned int m_aiRenderStates[256];
	unsigned int m_aiFlushedRenderStates[256];
	unsigned m_aiRenderStateTouched;	// divided into eight-state blocks
	unsigned int m_aaiSamplerStates[256 /*D3DDMAPSAMPLER */ + 16][16];
	struct IDirect3DBaseTexture9* m_aiTextures[256 /*D3DDMAPSAMPLER */ + 16];
	unsigned __int64 m_aiPixelFloatTouched, m_aiVertexFloatTouched;

	unsigned m_aVertexIntegerConstants[256];
	unsigned m_aPixelIntegerConstants[256];

	unsigned m_aVertexBooleanConstants[256];
	unsigned m_aPixelBooleanConstants[256];

	// TODO: Align these and use vectorized compares?
	float m_aVertexFloatConstants[256][4];
	float m_aPixelFloatConstants[256][4];
	float m_aFlushedVertexFloatConstants[256][4];
	float m_aFlushedPixelFloatConstants[256][4];

	void LazyFlush()
	{
		if (m_LazyTouched)
			ExternLazyFlush();
	}
	void ExternLazyFlush();

	HRESULT ExternSetTexture(DWORD Stage,IDirect3DBaseTexture9* pTexture);
	HRESULT ExternSetRenderState(D3DRENDERSTATETYPE State,DWORD Value);
	HRESULT ExternSetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value);
	HRESULT ExternSetPixelShaderConstantF( UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount );
	HRESULT ExternSetPixelShaderConstantI( UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount );
	HRESULT ExternSetPixelShaderConstantB( UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount );
	HRESULT ExternSetVertexShaderConstantF( UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount );
	HRESULT ExternSetVertexShaderConstantI( UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount );
	HRESULT ExternSetVertexShaderConstantB( UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount );
};
#endif // __D3D9

#if DEVICE_USE_D3D_WRAPPER

#if __D3D11_MONO_DRIVER
struct RageDirect3DDevice11
#elif __D3D11_1
struct RageDirect3DDevice11: public ID3D11Device1
#else
struct RageDirect3DDevice11: public ID3D11Device
#endif // _D3D11_1
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj)
		{ DEVICE_EKG_APICALL; return m_Inner->QueryInterface(riid,ppvObj); }

	STDMETHOD_(ULONG,AddRef)(THIS)
		{ DEVICE_EKG_APICALL; return m_Inner->AddRef(); }

	STDMETHOD_(ULONG,Release)(THIS)
		{ DEVICE_EKG_APICALL; return m_Inner->Release(); }

	/*** ID3D11Device methods ***/
	STDMETHOD(CheckCounter)(THIS_ const D3D11_COUNTER_DESC *pDesc, D3D11_COUNTER_TYPE *pType, UINT *pActiveCounters, LPSTR szName, UINT *pNameLength, LPSTR szUnits, UINT *pUnitsLength, LPSTR szDescription, UINT *pDescriptionLength)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->CheckCounter(pDesc, pType, pActiveCounters, szName, pNameLength, szUnits, pUnitsLength, szDescription, pDescriptionLength); }

	STDMETHOD_(void, CheckCounterInfo)(THIS_ D3D11_COUNTER_INFO *pCounterInfo)
		{ NR(); DEVICE_EKG_APICALL; m_Inner->CheckCounterInfo(pCounterInfo); }

	STDMETHOD(CheckFeatureSupport)(THIS_ D3D11_FEATURE Feature, void *pFeatureSupportData, UINT FeatureSupportDataSize)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize); }

	STDMETHOD(CheckFormatSupport)(THIS_ DXGI_FORMAT Format, UINT *pFormatSupport)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->CheckFormatSupport(Format, pFormatSupport); }

	STDMETHOD(CheckMultisampleQualityLevels)(THIS_ DXGI_FORMAT Format, UINT SampleCount, UINT *pNumQualityLevels)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->CheckMultisampleQualityLevels(Format, SampleCount, pNumQualityLevels); }

	STDMETHOD(CreateBlendState)(THIS_ const D3D11_BLEND_DESC *pBlendStateDesc, ID3D11BlendState **ppBlendState)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->CreateBlendState(pBlendStateDesc, ppBlendState); }

#if USE_RESOURCE_CACHE
	STDMETHOD(CreateBuffer)(THIS_ const D3D11_BUFFER_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Buffer **ppBuffer)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CreateBuffer); return rage::grcResourceCache::GetInstance().CreateBuffer(pDesc, pInitialData, ppBuffer); }
#else
	STDMETHOD(CreateBuffer)(THIS_ const D3D11_BUFFER_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Buffer **ppBuffer)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CreateBuffer); return m_Inner->CreateBuffer(pDesc, pInitialData, ppBuffer); }
#endif // USE_RESOURCE_CACHE

	STDMETHOD(CreateClassLinkage)(THIS_ ID3D11ClassLinkage **ppLinkage)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateClassLinkage(ppLinkage); }

	STDMETHOD(CreateComputeShader)(THIS_ const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11ComputeShader **ppDomainShader)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateComputeShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppDomainShader); }

	STDMETHOD(CreateCounter)(THIS_ const D3D11_COUNTER_DESC *pCounterDesc, ID3D11Counter **ppCounter)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateCounter(pCounterDesc, ppCounter); }

#if __D3D11_1
	STDMETHOD(CreateDeferredContext1)(THIS_ UINT ContextFlags, ID3D11DeviceContext1 **ppDeferredContext)
		{ NR(); return m_Inner->CreateDeferredContext1(ContextFlags, ppDeferredContext); };
#endif // __D3D11_1

	STDMETHOD(CreateDeferredContext)(THIS_ UINT ContextFlags, ID3D11DeviceContext **ppDeferredContext)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateDeferredContext(ContextFlags, ppDeferredContext); }

	STDMETHOD(CreateDepthStencilState)(THIS_ const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc, ID3D11DepthStencilState **ppDepthStencilState)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateDepthStencilState(pDepthStencilDesc, ppDepthStencilState); }

#if USE_RESOURCE_CACHE
	STDMETHOD(CreateDepthStencilView)(THIS_ ID3D11Resource *pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, ID3D11DepthStencilView **ppDepthStencilView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return rage::grcResourceCache::GetInstance().CreateDepthStencilView(pResource, pDesc, ppDepthStencilView); }
#else
	STDMETHOD(CreateDepthStencilView)(THIS_ ID3D11Resource *pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, ID3D11DepthStencilView **ppDepthStencilView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateDepthStencilView(pResource, pDesc, ppDepthStencilView); }
#endif // USE_RESOURCE_CACHE
	STDMETHOD(CreateDomainShader)(THIS_ const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11DomainShader **ppDomainShader)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateDomainShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppDomainShader); }
        
	STDMETHOD(CreateGeometryShader)(THIS_ const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11GeometryShader **ppGeometryShader)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateGeometryShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppGeometryShader); }

	STDMETHOD(CreateGeometryShaderWithStreamOutput)(THIS_ const void *pShaderBytecode, SIZE_T BytecodeLength, const D3D11_SO_DECLARATION_ENTRY *pSODeclaration, UINT NumEntries,
															const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ID3D11ClassLinkage *pClassLinkage, ID3D11GeometryShader **ppGeometryShader)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateGeometryShaderWithStreamOutput(pShaderBytecode, BytecodeLength, pSODeclaration, NumEntries, pBufferStrides, NumStrides, RasterizedStream, pClassLinkage, ppGeometryShader); }

	STDMETHOD(CreateHullShader)(THIS_ const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11HullShader **ppHullShader)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateHullShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppHullShader); }
        
	STDMETHOD(CreateInputLayout)(THIS_ const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements, const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, ID3D11InputLayout **ppInputLayout)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateInputLayout(pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, ppInputLayout); }

	STDMETHOD(CreatePixelShader)(THIS_ const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11PixelShader **ppPixelShader)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreatePixelShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader); }

	STDMETHOD(CreatePredicate)(THIS_ const D3D11_QUERY_DESC *pPredicateDesc, ID3D11Predicate **ppPredicate)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreatePredicate(pPredicateDesc, ppPredicate); }

	STDMETHOD(CreateQuery)(THIS_ const D3D11_QUERY_DESC *pQueryDesc, ID3D11Query **ppQuery)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateQuery(pQueryDesc, ppQuery); }

	STDMETHOD(CreateRasterizerState)(THIS_ const D3D11_RASTERIZER_DESC *pRasterizerDesc, ID3D11RasterizerState **ppRasterizerState)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateRasterizerState(pRasterizerDesc, ppRasterizerState); }

#if USE_RESOURCE_CACHE
	STDMETHOD(CreateRenderTargetView)(THIS_ ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC *pDesc, ID3D11RenderTargetView **ppRTView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return rage::grcResourceCache::GetInstance().CreateRenderTargetView(pResource, pDesc, ppRTView); }
#else
	STDMETHOD(CreateRenderTargetView)(THIS_ ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC *pDesc, ID3D11RenderTargetView **ppRTView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateRenderTargetView(pResource, pDesc, ppRTView); }
#endif // USE_RESOURCE_CACHE
	STDMETHOD(CreateSamplerState)(THIS_ const D3D11_SAMPLER_DESC *pSamplerDesc, ID3D11SamplerState **ppSamplerState)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateSamplerState(pSamplerDesc, ppSamplerState); }

#if RSG_DURANGO
	STDMETHOD(CreateSamplerStateX)(THIS_ const D3D11X_SAMPLER_DESC *pSamplerDesc, ID3D11SamplerState **ppSamplerState)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateSamplerStateX(pSamplerDesc, ppSamplerState); }
#endif

#if USE_RESOURCE_CACHE
	STDMETHOD(CreateShaderResourceView)(THIS_ ID3D11Resource *pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, ID3D11ShaderResourceView **ppSRView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return rage::grcResourceCache::GetInstance().CreateShaderResourceView(pResource, pDesc, ppSRView); }

	STDMETHOD(CreateTexture1D)(THIS_ const D3D11_TEXTURE1D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture1D **ppTexture1D)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CreateTexture); return rage::grcResourceCache::GetInstance().CreateTexture1D(pDesc, pInitialData, ppTexture1D); }

	STDMETHOD(CreateTexture2D)(THIS_ const D3D11_TEXTURE2D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture2D **ppTexture2D)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CreateTexture); return rage::grcResourceCache::GetInstance().CreateTexture2D(pDesc, pInitialData, ppTexture2D); }

	STDMETHOD(CreateTexture3D)(THIS_ const D3D11_TEXTURE3D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture3D **ppTexture3D)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CreateTexture); return rage::grcResourceCache::GetInstance().CreateTexture3D(pDesc, pInitialData, ppTexture3D); }

	STDMETHOD(CreateUnorderedAccessView)(THIS_ ID3D11Resource *pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, ID3D11UnorderedAccessView **ppUAView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return rage::grcResourceCache::GetInstance().CreateUnorderedAccessView(pResource, pDesc, ppUAView); }
#else
	STDMETHOD(CreateShaderResourceView)(THIS_ ID3D11Resource *pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, ID3D11ShaderResourceView **ppSRView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateShaderResourceView(pResource, pDesc, ppSRView); }

	STDMETHOD(CreateTexture1D)(THIS_ const D3D11_TEXTURE1D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture1D **ppTexture1D)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CreateTexture); return m_Inner->CreateTexture1D(pDesc, pInitialData, ppTexture1D); }

	STDMETHOD(CreateTexture2D)(THIS_ const D3D11_TEXTURE2D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture2D **ppTexture2D)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CreateTexture); return m_Inner->CreateTexture2D(pDesc, pInitialData, ppTexture2D); }

	STDMETHOD(CreateTexture3D)(THIS_ const D3D11_TEXTURE3D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture3D **ppTexture3D)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CreateTexture); return m_Inner->CreateTexture3D(pDesc, pInitialData, ppTexture3D); }

	STDMETHOD(CreateUnorderedAccessView)(THIS_ ID3D11Resource *pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, ID3D11UnorderedAccessView **ppUAView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateUnorderedAccessView(pResource, pDesc, ppUAView); }
#endif // USE_RESOURCE_CACHE

	STDMETHOD(CreateVertexShader)(THIS_ const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11VertexShader **ppVertexShader)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateVertexShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader); }


	STDMETHOD_(UINT, GetCreationFlags)(THIS)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); return m_Inner->GetCreationFlags(); }

	STDMETHOD(GetDeviceRemovedReason)(THIS)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); return m_Inner->GetDeviceRemovedReason(); }

	STDMETHOD_(D3D_FEATURE_LEVEL, GetFeatureLevel)(THIS)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); return m_Inner->GetFeatureLevel(); }

	STDMETHOD_(void, GetImmediateContext)(THIS_ ID3D11DeviceContext **ppImmediateContext)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->GetImmediateContext(ppImmediateContext); }

	STDMETHOD_(UINT, GetExceptionMode)(THIS)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); return m_Inner->GetExceptionMode(); }

	STDMETHOD(SetExceptionMode)(UINT RaiseFlags)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->SetExceptionMode(RaiseFlags); }

	STDMETHOD(GetPrivateData)(THIS_ REFGUID guid, UINT *pDataSize, void *pData)
		{ NR(); DEVICE_EKG_COUNTANDTIME(PrivateData); return m_Inner->GetPrivateData(guid, pDataSize, pData); }

	STDMETHOD(SetPrivateData)(THIS_ REFGUID guid, UINT DataSize, const void *pData)
		{ NR(); DEVICE_EKG_COUNTANDTIME(PrivateData); return m_Inner->SetPrivateData(guid, DataSize, pData); }

	STDMETHOD(SetPrivateDataInterface)(THIS_ REFGUID guid, const IUnknown *pData)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->SetPrivateDataInterface(guid, pData); }


	STDMETHOD(OpenSharedResource)(THIS_ HANDLE hResource, REFIID ReturnedInterface, void **ppResource)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->OpenSharedResource(hResource, ReturnedInterface, ppResource); }

#if __D3D11_1
	STDMETHOD_(void, GetImmediateContext1)(THIS_ ID3D11DeviceContext1 **ppImmediateContext)
		{ NR(); DEVICE_EKG_APICALL; m_Inner->GetImmediateContext1(ppImmediateContext); };
        
	STDMETHOD(CreateBlendState1)(THIS_ const D3D11_BLEND_DESC1 *pBlendStateDesc, ID3D11BlendState1 **ppBlendState)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateBlendState1(pBlendStateDesc,ppBlendState); };

	STDMETHOD(CreateRasterizerState1)(THIS_ const D3D11_RASTERIZER_DESC1 *pRasterizerDesc, ID3D11RasterizerState1 **ppRasterizerState)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateRasterizerState1(pRasterizerDesc,ppRasterizerState); };

	STDMETHOD(CreateDeviceContextState)(THIS_ UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, REFIID EmulatedInterface, D3D_FEATURE_LEVEL *pChosenFeatureLevel, ID3DDeviceContextState **ppContextState)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); return m_Inner->CreateDeviceContextState(Flags, pFeatureLevels, FeatureLevels, SDKVersion, EmulatedInterface, pChosenFeatureLevel, ppContextState); };

	STDMETHOD(OpenSharedResource1)(HANDLE hResource,REFIID returnedInterface,void **ppResource)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->OpenSharedResource1(hResource, returnedInterface, ppResource); };

	STDMETHOD(OpenSharedResourceByName)(THIS_ LPCWSTR lpName, DWORD dwDesiredAccess, REFIID returnedInterface, void **ppResource)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->OpenSharedResourceByName(lpName, dwDesiredAccess, returnedInterface, ppResource); };

	ID3D11Device1			*m_Inner;
#else
	ID3D11Device			*m_Inner;
#endif // __D3D11

	DWORD					*m_Next;
	size_t					 m_Remain;
};

#if __BANK && __D3D11_REPORT_RESOURCE_HAZARDS
#define D3D11_REPORT_RESOURCE_HAZARDS_ONLY( x ) x
#else
#define D3D11_REPORT_RESOURCE_HAZARDS_ONLY( x )
#endif

// Turn this on to use shadowing of render-states, depth-stencil states etc. to save on DX calls. Mainly used to bring down
// the memory footprint of PIX grabs.
#define __D3D11_USE_STATE_CACHING 1

#if __D3D11_MONO_DRIVER
struct RageDirect3DDeviceContext11
#elif __D3D11_1
struct RageDirect3DDeviceContext11: public ID3D11DeviceContext1
#else
struct RageDirect3DDeviceContext11: public ID3D11DeviceContext
#endif // _D3D11_1
{
	enum D3D11PhysicalUnitType
	{
		D3D11PhysicalUnitCS = 0,	
		D3D11PhysicalUnitDS,	
		D3D11PhysicalUnitGS,	
		D3D11PhysicalUnitHS,	
		D3D11PhysicalUnitPS,	
		D3D11PhysicalUnitVS,	
		D3D11PhysicalUnitMax,	
	};

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj)
		{ return m_Inner->QueryInterface(riid,ppvObj); }

	STDMETHOD_(ULONG,AddRef)(THIS)
		{ return m_Inner->AddRef(); }

	STDMETHOD_(ULONG,Release)(THIS)
		{ return m_Inner->Release(); }

	/*** ID3D11DeviceChild methods ***/
	STDMETHOD_(void, GetDevice)(THIS_ ID3D11Device **ppDevice)
		{ NR(); m_Inner->GetDevice(ppDevice); }

	STDMETHOD(GetPrivateData)(THIS_ REFGUID guid, UINT *pDataSize, void *pData)
		{ NR(); DEVICE_EKG_COUNTANDTIME(PrivateData); return m_Inner->GetPrivateData(guid, pDataSize, pData); }

	STDMETHOD(SetPrivateData)(THIS_ REFGUID guid, UINT DataSize, const void *pData)
		{ NR(); DEVICE_EKG_COUNTANDTIME(PrivateData); return m_Inner->SetPrivateData(guid, DataSize, pData); }

	STDMETHOD(SetPrivateDataInterface)(THIS_ REFGUID guid, const IUnknown *pData)
		{ NR(); DEVICE_EKG_COUNTANDTIME(PrivateData); return m_Inner->SetPrivateDataInterface(guid, pData); }

	/*** ID3D11DeviceContext methods ***/
	STDMETHOD_(void, Begin)(THIS_ ID3D11Asynchronous *pAsync)
		{ NR(); DEVICE_EKG_APICALL; m_Inner->Begin(pAsync); }

	STDMETHOD_(void, End)(THIS_ ID3D11Asynchronous *pAsync)
		{ NR(); DEVICE_EKG_APICALL; m_Inner->End(pAsync); }


	STDMETHOD_(void, ClearDepthStencilView)(THIS_ ID3D11DepthStencilView *pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
		{ NR(); DEVICE_EKG_APICALL; m_Inner->ClearDepthStencilView(pDepthStencilView, ClearFlags, Depth, Stencil); }

	STDMETHOD_(void, ClearRenderTargetView)(THIS_ ID3D11RenderTargetView *pRenderTargetView, const FLOAT ColorRGBA[4])
		{ NR(); DEVICE_EKG_APICALL; m_Inner->ClearRenderTargetView(pRenderTargetView, ColorRGBA); }

	STDMETHOD_(void, ClearState)(THIS)
		{ NR(); DEVICE_EKG_APICALL; m_Inner->ClearState(); }

	STDMETHOD_(void, ClearUnorderedAccessViewUint)(THIS_ ID3D11UnorderedAccessView *pUnorderedAccessView, const UINT Values[ 4 ])
		{ NR(); DEVICE_EKG_APICALL; m_Inner->ClearUnorderedAccessViewUint(pUnorderedAccessView, Values); }

	STDMETHOD_(void, ClearUnorderedAccessViewFloat)(THIS_ ID3D11UnorderedAccessView *pUnorderedAccessView, const FLOAT Values[ 4 ])
		{ NR(); DEVICE_EKG_APICALL; m_Inner->ClearUnorderedAccessViewFloat(pUnorderedAccessView, Values); }

	STDMETHOD_(void, CopyResource)(THIS_ ID3D11Resource *pDstResource, ID3D11Resource *pSrcResource)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CopyResource); m_Inner->CopyResource(pDstResource, pSrcResource); }

	STDMETHOD_(void, CopySubresourceRegion)(THIS_ ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CopySubResourceRegion); m_Inner->CopySubresourceRegion(pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox); }

#if __D3D11_1
	STDMETHOD_(void, CopySubresourceRegion1)(THIS_ ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox, UINT CopyFlags)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CopySubResourceRegion); m_Inner->CopySubresourceRegion1(pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox, CopyFlags); }
#endif // __D3D11_1

	STDMETHOD_(void, CopyStructureCount)(THIS_ ID3D11Buffer *pDstBuffer, UINT DstAlignedByteOffset, ID3D11UnorderedAccessView *pSrcView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CopyStructureCount); m_Inner->CopyStructureCount(pDstBuffer, DstAlignedByteOffset, pSrcView); }


	STDMETHOD_(void, CSGetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->CSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }

#if __D3D11_1
	STDMETHOD_(void, CSGetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->CSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants); }
#endif // __D3D11_1

	STDMETHOD_(void, CSGetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->CSGetSamplers(StartSlot, NumSamplers, ppSamplers); }

	STDMETHOD_(void, CSGetShader)(THIS_ ID3D11ComputeShader  **ppComputeShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->CSGetShader(ppComputeShader, ppClassInstances, pNumClassInstances); }

	STDMETHOD_(void, CSGetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->CSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews); }

	STDMETHOD_(void, CSGetUnorderedAccessViews)(THIS_ UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->CSGetUnorderedAccessViews(StartSlot, NumUAVs, ppUnorderedAccessViews); }

	STDMETHOD_(void, CSClearConstantBuffers)(THIS_)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CSSetConstantBuffers); m_DeviceState.m_CSConstBuffers.Reset(); m_Inner->CSSetConstantBuffers(0, MAX_CONSTANT_BUFFER, m_DeviceState.m_CSConstBuffers.ConstBuffers); }

	STDMETHOD_(void, CSSetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(CSSetConstantBuffers); m_Inner->CSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }
	#else
		{
			NR();
			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, ppConstantBuffers, m_DeviceState.m_CSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(CSSetConstantBuffers);
				m_Inner->CSSetConstantBuffers(StartSlot, NumBuffers, &(m_DeviceState.m_CSConstBuffers.ConstBuffers[StartSlot]));
			}
		}
	#endif

#if __D3D11_1
	STDMETHOD_(void, CSSetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *pConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(CSSetConstantBuffers); m_Inner->CSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants); }
	#else
		{
			NR();

			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, pConstantBuffers, &pFirstConstant, &pNumConstants, m_DeviceState.m_CSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(CSSetConstantBuffers);
				m_Inner->CSSetConstantBuffers1(StartSlot, NumBuffers, &(m_DeviceState.m_CSConstBuffers.ConstBuffers[StartSlot]), pFirstConstant, pNumConstants);
			}
		}
	#endif
#endif // __D3D11_1

	STDMETHOD_(void, CSSetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CSResourceSets); m_Inner->CSSetSamplers(StartSlot, NumSamplers, ppSamplers); }

	STDMETHOD_(void, CSSetShader)(THIS_ ID3D11ComputeShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CSResourceSets); m_Inner->CSSetShader(pShader, ppClassInstances, NumClassInstances); }

	STDMETHOD_(void, CSSetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView * const *ppShaderResourceViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CSResourceSets); m_Inner->CSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews); }

	STDMETHOD_(void, CSSetUnorderedAccessViews)(THIS_ UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView * const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CSResourceSets); m_Inner->CSSetUnorderedAccessViews(StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts); }


	STDMETHOD_(void, Dispatch)(THIS_ UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Dispatch); m_Inner->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ); }

	STDMETHOD_(void, DispatchIndirect)(THIS_ ID3D11Buffer *pBufferForArgs, UINT AlignedOffsetForArgs)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Dispatch); m_Inner->DispatchIndirect(pBufferForArgs, AlignedOffsetForArgs); }


	STDMETHOD_(void, Draw)(THIS_ UINT VertexCount, UINT StartVertexLocation)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Draw); m_Inner->Draw(VertexCount, StartVertexLocation); }

	STDMETHOD_(void, DrawAuto)(THIS)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Draw); m_Inner->DrawAuto(); }

	STDMETHOD_(void, DrawIndexed)(THIS_ UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Draw); m_Inner->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation); }

	STDMETHOD_(void, DrawIndexedInstanced)(THIS_ UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Draw); m_Inner->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation); }

	STDMETHOD_(void, DrawInstanced)(THIS_ UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Draw); m_Inner->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation); }

	STDMETHOD_(void, DrawInstancedIndirect)(THIS_ ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Draw); m_Inner->DrawInstancedIndirect(pBufferForArgs, AlignedByteOffsetForArgs); }

	STDMETHOD_(void, DrawIndexedInstancedIndirect)(THIS_ ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Draw); m_Inner->DrawIndexedInstancedIndirect(pBufferForArgs, AlignedByteOffsetForArgs); }


	STDMETHOD_(void, ExecuteCommandList)(THIS_ ID3D11CommandList *pCommandList, BOOL RestoreContextState)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CommandList); m_Inner->ExecuteCommandList(pCommandList, RestoreContextState); }

	STDMETHOD(FinishCommandList)(THIS_ BOOL RestoreDeferredContextState, ID3D11CommandList **ppCommandList)
		{ NR(); DEVICE_EKG_COUNTANDTIME(CommandList); return m_Inner->FinishCommandList(RestoreDeferredContextState, ppCommandList); }

	STDMETHOD_(void, Flush)(THIS)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Flush); m_Inner->Flush(); }


	STDMETHOD_(void, DSGetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->DSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }

#if __D3D11_1
	STDMETHOD_(void, DSGetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->DSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants); }
#endif // __D3D11_1

	STDMETHOD_(void, DSGetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->DSGetSamplers(StartSlot, NumSamplers, ppSamplers); }

	STDMETHOD_(void, DSGetShader)(THIS_ ID3D11DomainShader **ppDomainShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->DSGetShader(ppDomainShader, ppClassInstances, pNumClassInstances); }

	STDMETHOD_(void, DSGetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->DSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews); }

	STDMETHOD_(void, DSClearConstantBuffers)(THIS_)
		{ NR(); DEVICE_EKG_COUNTANDTIME(DSResourceSets); m_DeviceState.m_DSConstBuffers.Reset(); m_Inner->DSSetConstantBuffers(0, MAX_CONSTANT_BUFFER, m_DeviceState.m_DSConstBuffers.ConstBuffers); }

	STDMETHOD_(void, DSSetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(DSResourceSets); m_Inner->DSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }
	#else
		{
			NR();

			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, ppConstantBuffers, m_DeviceState.m_DSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(DSResourceSets); 
				m_Inner->DSSetConstantBuffers(StartSlot, NumBuffers, &(m_DeviceState.m_DSConstBuffers.ConstBuffers[StartSlot]));
			}
		}
	#endif

#if __D3D11_1
	STDMETHOD_(void, DSSetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *pConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(DSResourceSets); m_Inner->DSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants); }
	#else
		{
			NR();

			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, pConstantBuffers, &pFirstConstant, &pNumConstants, m_DeviceState.m_DSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(DSResourceSets);
				m_Inner->DSSetConstantBuffers1(StartSlot, NumBuffers, &(m_DeviceState.m_DSConstBuffers.ConstBuffers[StartSlot]), pFirstConstant, pNumConstants);
			}
		}
	#endif
#endif // __D3D11_1

	STDMETHOD_(void, DSSetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(DSResourceSets); m_Inner->DSSetSamplers(StartSlot, NumSamplers, ppSamplers); }

	STDMETHOD_(void, DSSetShader)(THIS_ ID3D11DomainShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(DSResourceSets); m_Inner->DSSetShader(pShader, ppClassInstances, NumClassInstances); }

	STDMETHOD_(void, DSSetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView * const *ppShaderResourceViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(DSResourceSets); m_Inner->DSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews); }


	STDMETHOD_(void, GenerateMips)(THIS_ ID3D11ShaderResourceView *pShaderResourceView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(MiscCreate); m_Inner->GenerateMips(pShaderResourceView); }

	STDMETHOD_(UINT, GetContextFlags)(THIS)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->GetContextFlags(); }

	STDMETHOD(GetData)(THIS_ ID3D11Asynchronous *pAsync, void *pData, UINT DataSize, UINT GetDataFlags)
		{ NR(); DEVICE_EKG_COUNTANDTIME(GetData); return m_Inner->GetData(pAsync, pData, DataSize, GetDataFlags); }

	STDMETHOD_(D3D11_DEVICE_CONTEXT_TYPE, GetType)(THIS)
		{ NR(); DEVICE_EKG_APICALL; return m_Inner->GetType(); }

	STDMETHOD_(void, GetPredication)(THIS_ ID3D11Predicate **ppPredicate, BOOL *pPredicateValue)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Predication);  m_Inner->GetPredication(ppPredicate, pPredicateValue); }

	STDMETHOD_(void, SetPredication)(THIS_ ID3D11Predicate *pPredicate, BOOL PredicateValue)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Predication);  m_Inner->SetPredication(pPredicate, PredicateValue); }

	STDMETHOD_(float, GetResourceMinLOD)(THIS_ ID3D11Resource *pResource)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); return m_Inner->GetResourceMinLOD(pResource); }

	STDMETHOD_(void, SetResourceMinLOD)(THIS_ ID3D11Resource *pResource, float MinLOD)
		{ NR(); DEVICE_EKG_APICALL; m_Inner->SetResourceMinLOD(pResource, MinLOD); }

	////STDMETHOD_(void, GetTextFilterSize)(THIS_ UINT *Width, UINT *Height)
	////	{ NR(); m_Inner->GetTextFilterSize(Width, Height); }

	////STDMETHOD_(void, SetTextFilterSize)(THIS_ UINT Width, UINT Height)
	////	{ NR(); m_Inner->SetTextFilterSize(Width, Height); }


	STDMETHOD_(void, GSGetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->GSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }

#if __D3D11_1
	STDMETHOD_(void, GSGetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->GSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants); }
#endif // __D3D11_1

	STDMETHOD_(void, GSGetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->GSGetSamplers(StartSlot, NumSamplers, ppSamplers); }

	STDMETHOD_(void, GSGetShader)(THIS_ ID3D11GeometryShader **ppGeometryShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->GSGetShader(ppGeometryShader, ppClassInstances, pNumClassInstances); }

	STDMETHOD_(void, GSGetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->GSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews); }

	STDMETHOD_(void, GSClearConstantBuffers)(THIS_)
		{ NR(); DEVICE_EKG_COUNTANDTIME(GSResourceSets); m_DeviceState.m_GSConstBuffers.Reset(); m_Inner->GSSetConstantBuffers(0, MAX_CONSTANT_BUFFER, m_DeviceState.m_GSConstBuffers.ConstBuffers); }

	STDMETHOD_(void, GSSetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(GSResourceSets); m_Inner->GSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }
	#else
		{
			NR();

			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, ppConstantBuffers, m_DeviceState.m_GSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(GSResourceSets);
				m_Inner->GSSetConstantBuffers(StartSlot, NumBuffers, &(m_DeviceState.m_GSConstBuffers.ConstBuffers[StartSlot]));
			}
		}
	#endif

#if __D3D11_1
	STDMETHOD_(void, GSSetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *pConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(GSResourceSets); m_Inner->GSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants); }
	#else
		{
			NR();

			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, pConstantBuffers, &pFirstConstant, &pNumConstants, m_DeviceState.m_GSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(GSResourceSets);
				m_Inner->GSSetConstantBuffers1(StartSlot, NumBuffers, &(m_DeviceState.m_GSConstBuffers.ConstBuffers[StartSlot]), pFirstConstant, pNumConstants);
			}
		}
	#endif
#endif // __D3D11_1

	STDMETHOD_(void, GSSetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(GSResourceSets); m_Inner->GSSetSamplers(StartSlot, NumSamplers, ppSamplers); }

	STDMETHOD_(void, GSSetShader)(THIS_ ID3D11GeometryShader *ppGeometryShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(GSResourceSets); m_Inner->GSSetShader(ppGeometryShader, ppClassInstances, NumClassInstances); }

	STDMETHOD_(void, GSSetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView * const *ppShaderResourceViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(GSResourceSets); m_Inner->GSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews); }
	

	STDMETHOD_(void, HSGetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->HSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }

#if __D3D11_1
	STDMETHOD_(void, HSGetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->HSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants); }
#endif // __D3D11_1

	STDMETHOD_(void, HSGetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->HSGetSamplers(StartSlot, NumSamplers, ppSamplers); }

	STDMETHOD_(void, HSGetShader)(THIS_ ID3D11HullShader **ppHullShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->HSGetShader(ppHullShader, ppClassInstances, pNumClassInstances); }

	STDMETHOD_(void, HSGetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->HSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews); }

	STDMETHOD_(void, HSClearConstantBuffers)(THIS_)
		{ NR(); DEVICE_EKG_COUNTANDTIME(HSResourceSets); m_DeviceState.m_HSConstBuffers.Reset(); m_Inner->HSSetConstantBuffers(0, MAX_CONSTANT_BUFFER, m_DeviceState.m_HSConstBuffers.ConstBuffers); }

	STDMETHOD_(void, HSSetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(HSResourceSets); m_Inner->HSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }
	#else
		{
			NR();

			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, ppConstantBuffers, m_DeviceState.m_HSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(HSResourceSets);
				m_Inner->HSSetConstantBuffers(StartSlot, NumBuffers, &(m_DeviceState.m_HSConstBuffers.ConstBuffers[StartSlot]));
			}
		}
	#endif

#if __D3D11_1
	STDMETHOD_(void, HSSetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *pConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(HSResourceSets); m_Inner->HSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants); }
	#else
		{
			NR();

			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, pConstantBuffers, &pFirstConstant, &pNumConstants, m_DeviceState.m_HSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(HSResourceSets);
				m_Inner->HSSetConstantBuffers1(StartSlot, NumBuffers, &(m_DeviceState.m_HSConstBuffers.ConstBuffers[StartSlot]), pFirstConstant, pNumConstants);
			}
		}
	#endif
#endif // __D3D11_1

	STDMETHOD_(void, HSSetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(HSResourceSets); m_Inner->HSSetSamplers(StartSlot, NumSamplers, ppSamplers); }

	STDMETHOD_(void, HSSetShader)(THIS_ ID3D11HullShader *pHullShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(HSResourceSets); m_Inner->HSSetShader(pHullShader, ppClassInstances, NumClassInstances); }

	STDMETHOD_(void, HSSetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView * const *ppShaderResourceViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(HSResourceSets); m_Inner->HSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews); }


	STDMETHOD_(void, IAGetIndexBuffer)(THIS_ ID3D11Buffer **pIndexBuffer, DXGI_FORMAT *Format, UINT *Offset)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->IAGetIndexBuffer(pIndexBuffer, Format, Offset); }

	STDMETHOD_(void, IAGetInputLayout)(THIS_ ID3D11InputLayout **ppInputLayout)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->IAGetInputLayout(ppInputLayout); }

	STDMETHOD_(void, IAGetPrimitiveTopology)(THIS_ D3D11_PRIMITIVE_TOPOLOGY *pTopology)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->IAGetPrimitiveTopology(pTopology); }

	STDMETHOD_(void, IAGetVertexBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppVertexBuffers, UINT *pStrides, UINT *pOffsets)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->IAGetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets); }

	STDMETHOD_(void, IASetIndexBuffer)(THIS_ ID3D11Buffer *pIndexBuffer, DXGI_FORMAT Format, UINT Offset)
#if !__D3D11_USE_STATE_CACHING
		{ NR(); DEVICE_EKG_COUNTANDTIME(IASetIndexBuffer); m_Inner->IASetIndexBuffer(pIndexBuffer, Format, Offset); }
#else
		{ 
			NR(); 
			if (m_DeviceState.m_pIndexBuffer != pIndexBuffer || Offset != m_DeviceState.m_IndexOffset)
			{
				DEVICE_EKG_COUNTANDTIME(IASetIndexBuffer); 
				m_Inner->IASetIndexBuffer(pIndexBuffer, Format, Offset);
				m_DeviceState.m_pIndexBuffer = pIndexBuffer;
				m_DeviceState.m_IndexOffset = Offset;
			}
		}
#endif // !__D3D11_USE_STATE_CACHING

	STDMETHOD_(void, IASetInputLayout)(THIS_ ID3D11InputLayout *pInputLayout)
#if !__D3D11_USE_STATE_CACHING
		{ NR(); DEVICE_EKG_COUNTANDTIME(IASetInputLayout); m_Inner->IASetInputLayout(pInputLayout); }
#else
	{
		NR();

		if(m_DeviceState.m_pInputLayout != pInputLayout)
		{
			DEVICE_EKG_COUNTANDTIME(IASetInputLayout);
			m_Inner->IASetInputLayout(pInputLayout);
			m_DeviceState.m_pInputLayout = pInputLayout;
		}
	}
#endif

	STDMETHOD_(void, IASetPrimitiveTopology)(THIS_ D3D11_PRIMITIVE_TOPOLOGY Topology)
#if !__D3D11_USE_STATE_CACHING
		{ NR(); DEVICE_EKG_COUNTANDTIME(IASetPrimitiveTopology); m_Inner->IASetPrimitiveTopology(Topology); }
#else
	{
		NR();

		if(m_DeviceState.m_Topology != (INT)Topology)
		{
			DEVICE_EKG_COUNTANDTIME(IASetPrimitiveTopology);
			m_Inner->IASetPrimitiveTopology(Topology);
			m_DeviceState.m_Topology = (INT)Topology;
		}
	}
#endif


	STDMETHOD_(void, IASetVertexBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer * const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets)
#if !__D3D11_USE_STATE_CACHING
		{ NR(); DEVICE_EKG_COUNTANDTIME(IASetVertexBuffers); m_Inner->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets); }
#else
	{
		NR();

		if (NumBuffers == 1)
		{	// Slightly fast-path, as NumBuffers is practically always 1
			if( m_DeviceState.m_pVertexBuffers[StartSlot] != *ppVertexBuffers
			||  m_DeviceState.m_Offsets[StartSlot] != *pOffsets
			||  m_DeviceState.m_Strides[StartSlot] != *pStrides )
			{
				{
 					DEVICE_EKG_COUNTANDTIME(IASetVertexBuffers);
					m_Inner->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
				}
				m_DeviceState.m_pVertexBuffers[StartSlot] = *ppVertexBuffers;
				m_DeviceState.m_Offsets[StartSlot] = *pOffsets;
				m_DeviceState.m_Strides[StartSlot] = *pStrides;
			}
		}
		else
		{	// Extra memcmp overhead seems to make lazy-test useless except for maybe larger numbers of vertex buffers.
			{
				DEVICE_EKG_COUNTANDTIME(IASetVertexBuffers);
				m_Inner->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
			}

			memcpy((void*)&(m_DeviceState.m_pVertexBuffers[StartSlot]),(void*)&(ppVertexBuffers),	sizeof(ID3D11Buffer*)*NumBuffers);
			memcpy((void*)&(m_DeviceState.m_Offsets[StartSlot]),		 (void*)pOffsets,			sizeof(UINT*)*NumBuffers);
			memcpy((void*)&(m_DeviceState.m_Strides[StartSlot]),		 (void*)pStrides,			sizeof(UINT*)*NumBuffers);
		}
	}
#endif


	STDMETHOD(Map)(THIS_ ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE *pMappedResource)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Map); return m_Inner->Map(pResource, Subresource, MapType, MapFlags, pMappedResource); }

	STDMETHOD_(void, Unmap)(THIS_ ID3D11Resource *pResource, UINT Subresource)
		{ NR(); DEVICE_EKG_COUNTANDTIME(UnMap); m_Inner->Unmap(pResource, Subresource); }


	STDMETHOD_(void, OMGetBlendState)(THIS_ ID3D11BlendState **ppBlendState, FLOAT BlendFactor[4], UINT *pSampleMask)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->OMGetBlendState(ppBlendState, BlendFactor, pSampleMask); }

	STDMETHOD_(void, OMGetDepthStencilState)(THIS_ ID3D11DepthStencilState **ppDepthStencilState, UINT *pStencilRef)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->OMGetDepthStencilState(ppDepthStencilState, pStencilRef); }

	STDMETHOD_(void, OMGetRenderTargets)(THIS_ UINT NumViews, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->OMGetRenderTargets(NumViews, ppRenderTargetViews, ppDepthStencilView); }

	STDMETHOD_(void, OMGetRenderTargetsAndUnorderedAccessViews)(THIS_ UINT NumRTVs, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView,
																	UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->OMGetRenderTargetsAndUnorderedAccessViews(NumRTVs, ppRenderTargetViews, ppDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews); }

	STDMETHOD_(void, OMSetBlendState)(THIS_ ID3D11BlendState *pBlendState, const FLOAT BlendFactor[4], UINT SampleMask)
#if !__D3D11_USE_STATE_CACHING
		{ NR(); DEVICE_EKG_COUNTANDTIME(OMSetBlendState); m_Inner->OMSetBlendState(pBlendState, BlendFactor, SampleMask); }
#else
	{
		NR(); 

#if RSG_PC
		// NULL is allowed.  Says the D3D11 ref page, "If you pass NULL, the runtime uses or stores a blend factor equal to { 1, 1, 1, 1 }."
		static FLOAT defaultBlendFactors[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		// But R* like to be explicit :).
		if (BlendFactor == NULL)
			BlendFactor = defaultBlendFactors;
#endif // RSG_PC

		if((m_DeviceState.m_pBlendState != pBlendState) || (m_DeviceState.m_BlendFactor[0] != BlendFactor[0]) || (m_DeviceState.m_BlendFactor[1] != BlendFactor[1]) || (m_DeviceState.m_BlendFactor[2] != BlendFactor[2]) || (m_DeviceState.m_BlendFactor[3] != BlendFactor[3]) || (m_DeviceState.m_SampleMask != SampleMask))
		{
			DEVICE_EKG_COUNTANDTIME(OMSetBlendState); 
			m_Inner->OMSetBlendState(pBlendState, BlendFactor, SampleMask);
			m_DeviceState.m_pBlendState = pBlendState;
			m_DeviceState.m_BlendFactor[0] = BlendFactor[0];
			m_DeviceState.m_BlendFactor[1] = BlendFactor[1];
			m_DeviceState.m_BlendFactor[2] = BlendFactor[2];
			m_DeviceState.m_BlendFactor[3] = BlendFactor[3];
			m_DeviceState.m_SampleMask = SampleMask;
		}
	}

#endif

	STDMETHOD_(void, OMSetDepthStencilState)(THIS_ ID3D11DepthStencilState *pDepthStencilState, UINT StencilRef)
#if !__D3D11_USE_STATE_CACHING
		{ NR(); DEVICE_EKG_COUNTANDTIME(OMSetDepthStencilState); m_Inner->OMSetDepthStencilState(pDepthStencilState, StencilRef); }
#else
	{
		NR(); 
		if((m_DeviceState.m_pDepthStencilState != pDepthStencilState) || (m_DeviceState.m_StencilRef != StencilRef))
		{
			DEVICE_EKG_COUNTANDTIME(OMSetDepthStencilState);
			m_Inner->OMSetDepthStencilState(pDepthStencilState, StencilRef);
			m_DeviceState.m_pDepthStencilState = pDepthStencilState;
			m_DeviceState.m_StencilRef = StencilRef;
		}
	}
#endif

	STDMETHOD_(void, OMSetRenderTargets)(THIS_ UINT NumViews, ID3D11RenderTargetView * const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView)
	{ 
		NR(); 
#if __D3D11_USE_STATE_CACHING
		m_DeviceState.ResetCachedShaderResources(); 
#endif // __D3D11_USE_STATE_CACHING
		DEVICE_EKG_COUNTANDTIME(OMSetRenderTargets); 
		m_Inner->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView); 
		D3D11_REPORT_RESOURCE_HAZARDS_ONLY(ReportHazardsOnSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView)); 
	}

	STDMETHOD_(void, OMSetRenderTargetsAndUnorderedAccessViews)(THIS_  UINT NumRTVs, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView,
																	UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessView, const UINT *pUAVInitialCounts)
	{ 
		NR(); 
#if __D3D11_USE_STATE_CACHING
		m_DeviceState.ResetCachedShaderResources(); 
#endif // __D3D11_USE_STATE_CACHING
		DEVICE_EKG_COUNTANDTIME(OMSetRenderTargetsAndUnorderedAccessViews); 
		m_Inner->OMSetRenderTargetsAndUnorderedAccessViews(NumRTVs, ppRenderTargetViews, pDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessView, pUAVInitialCounts); 
	}

	STDMETHOD_(void, ResolveSubresource)(THIS_ ID3D11Resource *pDstResource, UINT DstSubresource, ID3D11Resource *pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResolveSubresource); m_Inner->ResolveSubresource(pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format); }

	STDMETHOD_(void, UpdateSubresource)(THIS_ ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
		{ NR(); DEVICE_EKG_COUNTANDTIME(UpdateSubresource); m_Inner->UpdateSubresource(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch); }

#if __D3D11_1
	STDMETHOD_(void, UpdateSubresource1)(THIS_ ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, UINT CopyFlags)
		{ NR(); DEVICE_EKG_COUNTANDTIME(UpdateSubresource); m_Inner->UpdateSubresource1(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch, CopyFlags); }
#endif // __D3D11_1

	STDMETHOD_(void, PSGetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->PSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }

#if __D3D11_1
	STDMETHOD_(void, PSGetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->PSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants); }
#endif // __D3D11_1

	STDMETHOD_(void, PSGetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->PSGetSamplers(StartSlot, NumSamplers, ppSamplers); }

	STDMETHOD_(void, PSGetShader)(THIS_ ID3D11PixelShader **pPixelShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->PSGetShader(pPixelShader, ppClassInstances, pNumClassInstances); }

	STDMETHOD_(void, PSGetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->PSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews); }

	STDMETHOD_(void, PSClearConstantBuffers)(THIS_)
		{ NR(); DEVICE_EKG_COUNTANDTIME(PSSetConstantBuffers); m_DeviceState.m_PSConstBuffers.Reset(); m_Inner->PSSetConstantBuffers(0, MAX_CONSTANT_BUFFER, m_DeviceState.m_PSConstBuffers.ConstBuffers); }

	STDMETHOD_(void, PSSetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(PSSetConstantBuffers); m_Inner->PSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }
	#else
		{
			NR();

			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, ppConstantBuffers, m_DeviceState.m_PSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(PSSetConstantBuffers);
				m_Inner->PSSetConstantBuffers(StartSlot, NumBuffers, &(m_DeviceState.m_PSConstBuffers.ConstBuffers[StartSlot]));
			}
		}
	#endif

#if __D3D11_1
	STDMETHOD_(void, PSSetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *pConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(PSSetConstantBuffers); m_Inner->PSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants); }
	#else
		{
			NR();

			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, pConstantBuffers, &pFirstConstant, &pNumConstants, m_DeviceState.m_PSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(PSSetConstantBuffers1);
				m_Inner->PSSetConstantBuffers1(StartSlot, NumBuffers, &(m_DeviceState.m_PSConstBuffers.ConstBuffers[StartSlot]), pFirstConstant, pNumConstants);
			}
		}
	#endif
#endif // __D3D11_1

	STDMETHOD_(void, PSSetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
	#if !__D3D11_USE_STATE_CACHING
		{ NR(); DEVICE_EKG_COUNTANDTIME(PSSetSamplers); m_Inner->PSSetSamplers(StartSlot, NumSamplers, ppSamplers); }
	#else
		{
			NR();

			if(m_DeviceState.WillSamplerStatesChange(StartSlot, NumSamplers, ppSamplers, m_DeviceState.m_PSSamplers))
			{
				DEVICE_EKG_COUNTANDTIME(PSSetSamplers);
				m_Inner->PSSetSamplers(StartSlot, NumSamplers, &(m_DeviceState.m_PSSamplers.SamplerStates[StartSlot]));
			}
		}
	#endif

	STDMETHOD_(void, PSSetShader)(THIS_ ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(PSSetShader);m_Inner->PSSetShader(pPixelShader, ppClassInstances, NumClassInstances); }

	// Lazy-check code for this is now handled inside grcDevice. That means don't call this yourself :)
	STDMETHOD_(void, PSSetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView * const *ppShaderResourceViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(PSSetShaderResources); m_Inner->PSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews); D3D11_REPORT_RESOURCE_HAZARDS_ONLY(ReportHazardsOnShaderResourceBind(D3D11PhysicalUnitPS, StartSlot, NumViews, ppShaderResourceViews)); }

	STDMETHOD_(void, RSGetScissorRects)(THIS_ UINT *NumRects, D3D11_RECT *pRects)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->RSGetScissorRects(NumRects, pRects); }

	STDMETHOD_(void, RSGetState)(THIS_ ID3D11RasterizerState **ppRasterizerState)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->RSGetState(ppRasterizerState); }

	STDMETHOD_(void, RSGetViewports)(THIS_ UINT *NumViewports, D3D11_VIEWPORT *pViewports)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->RSGetViewports(NumViewports, pViewports); }

	STDMETHOD_(void, RSSetScissorRects)(THIS_ UINT NumRects, const D3D11_RECT *pRects)
		{ NR(); DEVICE_EKG_COUNTANDTIME(RSSets); m_Inner->RSSetScissorRects(NumRects, pRects); }

	STDMETHOD_(void, RSSetState)(THIS_ ID3D11RasterizerState *pRasterizerState)
	#if !__D3D11_USE_STATE_CACHING
		{ NR(); DEVICE_EKG_COUNTANDTIME(RSSets); m_Inner->RSSetState(pRasterizerState); }
	#else
		{
			NR();

			if(m_DeviceState.m_pRasterizerState != pRasterizerState)
			{
				DEVICE_EKG_COUNTANDTIME(RSSets); 
				m_Inner->RSSetState(pRasterizerState);
				m_DeviceState.m_pRasterizerState = pRasterizerState;
			}
		}
	#endif

	STDMETHOD_(void, RSSetViewports)(THIS_ UINT NumViewports, const D3D11_VIEWPORT *pViewports)
		{ NR(); DEVICE_EKG_COUNTANDTIME(RSSets); m_Inner->RSSetViewports(NumViewports, pViewports); }


	STDMETHOD_(void, SOGetTargets)(THIS_ UINT NumBuffers, ID3D11Buffer **ppSOTargets)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->SOGetTargets(NumBuffers, ppSOTargets); }

	STDMETHOD_(void, SOSetTargets)(THIS_ UINT NumBuffers, ID3D11Buffer * const *ppSOTargets, const UINT *pOffsets)
		{ NR(); DEVICE_EKG_APICALL; m_Inner->SOSetTargets(NumBuffers, ppSOTargets, pOffsets); }


	STDMETHOD_(void, VSGetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->VSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }

#if __D3D11_1
	STDMETHOD_(void, VSGetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->VSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants); }
#endif // __D3D11_1

	STDMETHOD_(void, VSGetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->VSGetSamplers(StartSlot, NumSamplers, ppSamplers); }

	STDMETHOD_(void, VSGetShader)(THIS_ ID3D11VertexShader **pVertexShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->VSGetShader(pVertexShader, ppClassInstances, pNumClassInstances); }

	STDMETHOD_(void, VSGetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
		{ NR(); DEVICE_EKG_COUNTANDTIME(ResourceGets); m_Inner->VSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews); }

	STDMETHOD_(void, VSClearConstantBuffers)(THIS_)
		{ NR(); DEVICE_EKG_COUNTANDTIME(VSSetConstantBuffers); m_DeviceState.m_VSConstBuffers.Reset(); m_Inner->VSSetConstantBuffers(0, MAX_CONSTANT_BUFFER, m_DeviceState.m_VSConstBuffers.ConstBuffers); }

	STDMETHOD_(void, VSSetConstantBuffers)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(VSSetConstantBuffers); m_Inner->VSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers); }
	#else
		{
			NR();

			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, ppConstantBuffers, m_DeviceState.m_VSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(VSSetConstantBuffers);
				m_Inner->VSSetConstantBuffers(StartSlot, NumBuffers, &(m_DeviceState.m_VSConstBuffers.ConstBuffers[StartSlot]));
			}
		}
	#endif	// !__D3D11_USE_STATE_CACHING

#if __D3D11_1
	STDMETHOD_(void, VSSetConstantBuffers1)(THIS_ UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *pConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
	#if !__D3D11_USE_STATE_CACHING || __D3D11_MONO_DRIVER
		{ NR(); DEVICE_EKG_COUNTANDTIME(VSSetConstantBuffers1); m_Inner->VSSetConstantBuffers1(StartSlot, NumBuffers, pConstantBuffers, pFirstConstant, pNumConstants); }
	#else
		{
			NR();

			if(m_DeviceState.WillConstantBuffersChange(StartSlot, NumBuffers, pConstantBuffers, &pFirstConstant, &pNumConstants, m_DeviceState.m_VSConstBuffers))
			{
				DEVICE_EKG_COUNTANDTIME(VSSetConstantBuffers1);
				m_Inner->VSSetConstantBuffers1(StartSlot, NumBuffers, &(m_DeviceState.m_VSConstBuffers.ConstBuffers[StartSlot]), pFirstConstant, pNumConstants);
			}
		}
	#endif // !__D3D11_USE_STATE_CACHING
#endif // __D3D11_1

	STDMETHOD_(void, VSSetSamplers)(THIS_ UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
	#if !__D3D11_USE_STATE_CACHING
		{ NR(); DEVICE_EKG_COUNTANDTIME(VSSetSamplers); m_Inner->VSSetSamplers(StartSlot, NumSamplers, ppSamplers); }
	#else
		{
			NR();

			if(m_DeviceState.WillSamplerStatesChange(StartSlot, NumSamplers, ppSamplers, m_DeviceState.m_VSSamplers))
			{
				DEVICE_EKG_COUNTANDTIME(VSSetSamplers);
				m_Inner->VSSetSamplers(StartSlot, NumSamplers, &(m_DeviceState.m_VSSamplers.SamplerStates[StartSlot]));
			}
		}
	#endif

#if __D3D11_MONO_DRIVER
	STDMETHOD_ (void, Suspend)(THIS_ UINT Flags)
	{
#if _XDK_VER >= 9586
		NR();
		static_cast<ID3D11DeviceContextX*>(m_Inner)->Suspend(Flags);
#endif // _XDK_VER >= 9586
	}

	STDMETHOD_ (void, Resume)(THIS_)
	{
#if _XDK_VER >= 9586
		NR();
		static_cast<ID3D11DeviceContextX*>(m_Inner)->Resume();
#endif //  _XDK_VER >= 9586
	}
#endif // __D3D11_MONO_DRIVER

	STDMETHOD_(void, VSSetShader)(THIS_ ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
		{ NR(); DEVICE_EKG_COUNTANDTIME(VSSetShader); m_Inner->VSSetShader(pVertexShader, ppClassInstances, NumClassInstances); }

	STDMETHOD_(void, VSSetShaderResources)(THIS_ UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView * const *ppShaderResourceViews)
	#if !__D3D11_USE_STATE_CACHING
		{ NR(); DEVICE_EKG_COUNTANDTIME(VSSetShaderResources); m_Inner->VSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews); }
	#else
		{
			NR();
			
			if(m_DeviceState.WillShaderResourcesChange(StartSlot, NumViews, ppShaderResourceViews, m_DeviceState.m_pVSShaderResources))
			{
				DEVICE_EKG_COUNTANDTIME(VSSetShaderResources);
				Assert(m_DeviceState.m_pVSShaderResources[StartSlot] != NULL);
				m_Inner->VSSetShaderResources(StartSlot, NumViews, &(m_DeviceState.m_pVSShaderResources[StartSlot]));
			}
		}
	#endif // !__D3D11_USE_STATE_CACHING

#if __D3D11_1
	STDMETHOD_(void, DiscardResource)(THIS_ ID3D11Resource *pResource)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Other); m_Inner->DiscardResource(pResource); }
        
	STDMETHOD_(void, ClearView)(THIS_ ID3D11View *pView, const FLOAT Color[ 4 ], const D3D11_RECT *pRect, UINT NumRects)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Other); m_Inner->ClearView(pView, Color, pRect, NumRects); }

	STDMETHOD_(void, DiscardView)(THIS_ ID3D11View *pResourceView)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Other); m_Inner->DiscardView(pResourceView); }

	STDMETHOD_(void, SwapDeviceContextState)(THIS_ ID3DDeviceContextState *pState, ID3DDeviceContextState **ppPreviousState)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Other); m_Inner->SwapDeviceContextState(pState, ppPreviousState); }

	STDMETHOD_(void, DiscardView1)(THIS_ ID3D11View *pResourceView, const D3D11_RECT *pRects, UINT NumRects)
		{ NR(); DEVICE_EKG_COUNTANDTIME(Other); m_Inner->DiscardView1(pResourceView, pRects, NumRects); }

	ID3D11DeviceContext1	*m_Inner;
#else
	ID3D11DeviceContext		*m_Inner;
#endif // __D3D11_1

	DWORD					*m_Next;
	size_t					 m_Remain;

#if __BANK && __D3D11_REPORT_RESOURCE_HAZARDS

	void ResetReportResourceHazards();
	void ReportHazardsOnShaderResourceBind( D3D11PhysicalUnitType PhysicalUnit, UINT StartSlot, UINT Count, ID3D11ShaderResourceView  * const *ppShaderResourceViews );
	void ReportHazardsOnSetRenderTargets( UINT NumViews, ID3D11RenderTargetView * const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView );

	s32 IsBoundAsRenderTarget( ID3D11Resource *pResource );
	s32 IsBoundAsAShaderResource( ID3D11Resource *pResource, D3D11PhysicalUnitType PhysicalUnit );

	ID3D11Resource *m_pRenderTargetResources[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	ID3D11Resource *m_pShaderResources[D3D11PhysicalUnitMax][D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11Resource *m_pDepthStencilResource;
	static char *s_PhysicalUnitNames[D3D11PhysicalUnitMax];

#endif //__BANK && __D3D11_REPORT_RESOURCE_HAZARDS


#if __D3D11_USE_STATE_CACHING

public:
	typedef struct DEVICE_STATE
	{
	public:
		typedef struct STAGE_CONSTANT_BUFFERS
		{
			STAGE_CONSTANT_BUFFERS()		{ Reset(); };

			ID3D11Buffer *	ConstBuffers[MAX_CONSTANT_BUFFER];
	#if __D3D11_1
			UINT			Offsets[MAX_CONSTANT_BUFFER];
			UINT			Sizes[MAX_CONSTANT_BUFFER];
	#endif // __D3D11_1

			void Reset()
			{
				for(UINT i=0; i<MAX_CONSTANT_BUFFER; i++)
				{
					ConstBuffers[i] = NULL;
	#if __D3D11_1
					Offsets[i] = 0;
					Sizes[i] = 4096;
	#endif // __D3D11_1
				}
			}

			void Print()
			{
				for(int i=0; i<MAX_CONSTANT_BUFFER; i++)
				{
					Printf("%d) %p\n", i, ConstBuffers[i]);
				}
			}
			bool operator==(struct STAGE_CONSTANT_BUFFERS &other)
			{
				for(UINT i=0; i<MAX_CONSTANT_BUFFER; i++)
				{
					if(ConstBuffers[i] != other.ConstBuffers[i])
					{
						return false;
					}
					return true;
				}
			}
		} STAGE_CONSTANT_BUFFERS;

		typedef struct STAGE_SAMPLERS
		{
			ID3D11SamplerState *SamplerStates[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];

			void Reset()
			{
				UINT i;

				for(i=0; i<D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; i++)
				{
					SamplerStates[i] = (ID3D11SamplerState *)0xdeadbeef;
				}
			}
		} STAGE_SAMPLERS;

		void ResetCachedStates()
		{
			// OMSetBlendState.
			m_pBlendState = NULL;
			m_BlendFactor[0] = -1.0f;
			m_BlendFactor[1] = -1.0f;
			m_BlendFactor[2] = -1.0f;
			m_BlendFactor[3] = -1.0f;
			m_SampleMask = 0x0;

			// OMsetDepthStencilState.
			m_pDepthStencilState = NULL;
			m_StencilRef = 0xffff;

			// RSSetState.
			m_pRasterizerState = NULL;

			// IASetPrimitiveTopology.
			m_Topology = -1;

			// IASetInputLayout.
			m_pInputLayout = NULL;
			for(int Slot=0; Slot<D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++Slot)
			{
				m_pVertexBuffers[Slot] = NULL;
				m_Strides[Slot] = 0;
				m_Offsets[Slot] = 0;
			}
			m_pIndexBuffer = NULL;
			m_IndexOffset = 0;

			// ##SetConstantBuffers.
			m_CSConstBuffers.Reset();
			m_DSConstBuffers.Reset();
			m_GSConstBuffers.Reset();
			m_HSConstBuffers.Reset();
			m_VSConstBuffers.Reset();
			m_PSConstBuffers.Reset();

			// PSSetSamplers.
			m_PSSamplers.Reset();
			m_VSSamplers.Reset();

			ResetCachedShaderResources();
		}

		void ResetCachedShaderResources()
		{
			// SetShaderResources.
			memset(m_pVSShaderResources, 0, sizeof(m_pVSShaderResources));
		}

		void Print()
		{
			Printf("Blend State:- %p, %f, %f, %f, %f, %d\n", m_pBlendState, m_BlendFactor[0], m_BlendFactor[1], m_BlendFactor[2], m_BlendFactor[3], m_SampleMask);
			Printf("Depth Stencil:- %p, %d\n", m_pDepthStencilState, m_StencilRef);
			Printf("RasterizerState:- %p\n", m_pRasterizerState);
			Printf("Topology:- %d\n", m_Topology);
			Printf("Input layput:- %p\n", m_pInputLayout);
			for(int i=0; i<D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; i++)
			{
				Printf("Vertex buffer %d) %p, %d, %d\n", i, m_pVertexBuffers[i], m_Strides[i], m_Offsets[i]);
			}
			Printf("Index buffer:- %p, %d\n", m_pIndexBuffer, m_IndexOffset);
			Printf("Vertex const buffers:-\n");
			m_VSConstBuffers.Print();
			Printf("Pixel const buffers:-\n");
			m_PSConstBuffers.Print();
		}

		bool operator==(DEVICE_STATE &other)
		{
			if((m_pBlendState != other.m_pBlendState) || (m_BlendFactor[0] != other.m_BlendFactor[0]) || (m_BlendFactor[1] != other.m_BlendFactor[1]) || (m_BlendFactor[2] != other.m_BlendFactor[2]) || (m_BlendFactor[3] != other.m_BlendFactor[3]) || (m_SampleMask != other.m_SampleMask))
			{
				Printf("Error in blend state!\n");
				return false;
			}
			if((m_pDepthStencilState != other.m_pDepthStencilState) || (m_StencilRef != m_StencilRef))
			{
				Printf("Error in depth stencil state!\n");
				return false;
			}
			if(m_pRasterizerState != other.m_pRasterizerState)
			{
				Printf("Error in rasterizer state!\n");
				return false;
			}
			if(m_Topology != other.m_Topology)
			{
				Printf("Error in Topology!\n");
				return false;
			}
			if(m_pInputLayout != other.m_pInputLayout)
			{
				Printf("Error in input layout!\n");
				return false;
			}
			for(int i=0; i<D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; i++)
			{
				if((m_pVertexBuffers[i] != other.m_pVertexBuffers[i]) || (m_Strides[i] != other.m_Strides[i]) || (m_Offsets[i] != other.m_Offsets[i]))
				{
					Printf("Error in vertex buffers %d\n", i);
					return false;
				}
			}
			if((m_pIndexBuffer != other.m_pIndexBuffer) || (m_IndexOffset != other.m_IndexOffset))
			{
				Printf("Error in index buffer!\n");
				return false;
			}
			if(!(m_VSConstBuffers == other.m_VSConstBuffers))
			{
				Printf("Error in vertex const buffers!n");
				return false;
			}
			if(!(m_PSConstBuffers == other.m_PSConstBuffers))
			{
				Printf("Error in pixel const buffers!n");
				return false;
			}
			return true;
		}

		// Returns true if the passed stages constant buffers need to be changed and updates contents.
		bool WillConstantBuffersChange(UINT & StartSlot, UINT & NumBuffers, ID3D11Buffer *const *ppConstantBuffers, STAGE_CONSTANT_BUFFERS &StageBuffers)
		{
			const UINT		ArrayStartSlot = StartSlot;
			const UINT		ArrayEndSlot = (StartSlot+NumBuffers)-1;
			ID3D11Buffer**	pShadowBuffer = &(StageBuffers.ConstBuffers[ArrayStartSlot]);
			UINT			EndSlot=0xffffffff;

			StartSlot = 0xffffffff;

			bool Changed=false;

			for(UINT Slot=ArrayStartSlot; Slot<=ArrayEndSlot; Slot++)
			{
				if (*ppConstantBuffers && (*pShadowBuffer != *ppConstantBuffers))
				{
					*pShadowBuffer = *ppConstantBuffers;
 					StartSlot = rage::Min(StartSlot, Slot);
 					EndSlot = Slot;
					Changed = true;
				}

				ppConstantBuffers++;
				pShadowBuffer++;
			}

 			NumBuffers = (EndSlot-StartSlot)+1;
			return Changed;
		}

	#if __D3D11_1
		bool RageDirect3DDeviceContext11::WillConstantBuffersChange(UINT & StartSlot, UINT & NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT ** ppOffsets, const UINT ** ppSizes, STAGE_CONSTANT_BUFFERS &StageBuffers)
		{
			const UINT		ArrayStartSlot = StartSlot;
			const UINT		ArrayEndSlot = (StartSlot+NumBuffers)-1;
			ID3D11Buffer**	pShadowBuffer = &(StageBuffers.ConstBuffers[ArrayStartSlot]);
			const UINT*		pOffsets = *ppOffsets;
			UINT*			pShadowOffset = &(StageBuffers.Offsets[ArrayStartSlot]);
			UINT			EndSlot=0xffffffff;
			StartSlot = 0xffffffff;

			bool Changed = false;
			for(UINT Slot=ArrayStartSlot; Slot<=ArrayEndSlot; Slot++)
			{
				if (*ppConstantBuffers && ( (*pShadowBuffer != *ppConstantBuffers) || (*pOffsets != *pShadowOffset) ) )
				{
					*pShadowBuffer = *ppConstantBuffers;
					*pShadowOffset = *pOffsets;
					StageBuffers.Sizes[Slot] = (*ppSizes)[Slot-ArrayStartSlot];

 					StartSlot = rage::Min(StartSlot, Slot);
 					EndSlot = Slot;
					Changed = true;
				}

				pShadowBuffer++;
				ppConstantBuffers++;
				pOffsets++;
				pShadowOffset++;
			}

			if (Changed)
			{
				*ppOffsets	= &(StageBuffers.Offsets[StartSlot]);
				*ppSizes	= &(StageBuffers.Sizes[StartSlot]);
				NumBuffers	= (EndSlot-StartSlot)+1;
			}
			return Changed;
		}
	#endif // __D3D11_1

		// Returns true if the passed stages sampler states need to be changed and updates contents.
		bool WillSamplerStatesChange(UINT & StartSlot, UINT & NumSamplers, ID3D11SamplerState *const *ppSamplers, STAGE_SAMPLERS &StageSamplerStates)
		{
			const UINT				ArrayStartSlot = StartSlot;
			const UINT				ArrayEndSlot = StartSlot + NumSamplers;
			ID3D11SamplerState **	ppShadowSampler = &(StageSamplerStates.SamplerStates[StartSlot]);
			UINT					EndSlot = 0xffffffff;

			StartSlot = 0xffffffff;

			for(UINT i=ArrayStartSlot; i<ArrayEndSlot; i++)
			{
				if(*ppSamplers && (*ppShadowSampler != *ppSamplers))
				{
					*ppShadowSampler = *ppSamplers;
					StartSlot = rage::Min(StartSlot,i);
					EndSlot = i;
				}
				*ppSamplers;
				*ppShadowSampler++;
			}

			NumSamplers = EndSlot - StartSlot+1;

			return (EndSlot != 0xffffffff);
		}


		// Returns true if the passed stages sampler states need to be changed and updates contents.
		bool RageDirect3DDeviceContext11::WillShaderResourcesChange(UINT & StartSlot, UINT & NumViews, ID3D11ShaderResourceView * const *ppShaderResourceViews, ID3D11ShaderResourceView ** ppShadowStates)
		{
			if (NumViews == 1)
			{	// This code-path now not hit for pixel shaders - so fast-path 1 item as that's all that ever gets called right now.
				Assert(ppShaderResourceViews[0] != NULL);
				if (ppShadowStates[StartSlot] != ppShaderResourceViews[0])
				{
					ppShadowStates[StartSlot] = ppShaderResourceViews[0];
					return true;
				}

				return false;
			}
			else
			{
				const UINT					ArrayStartSlot = StartSlot;
				const UINT					ArrayEndSlot = StartSlot + NumViews;
				ID3D11ShaderResourceView**	pShadowView = &(ppShadowStates[StartSlot]);
				UINT						EndSlot = 0xffffffff;	// Index of the last slot being changed.

				StartSlot = 0xffffffff;

				for(UINT Slot=ArrayStartSlot; Slot<ArrayEndSlot; ++Slot)
				{
					if (*ppShaderResourceViews && (*pShadowView != *ppShaderResourceViews))
					{
						*pShadowView = *ppShaderResourceViews;
						StartSlot = rage::Min(StartSlot, Slot);
						EndSlot = Slot;
					}
					pShadowView++;
					ppShaderResourceViews++;
				}

				NumViews = EndSlot-StartSlot+1;

				return (StartSlot != 0xffffffff);
			}
		}

	private:
		// OMSetBlendState.
		ID3D11BlendState *m_pBlendState;
		FLOAT m_BlendFactor[4];
		UINT m_SampleMask;

		// OMsetDepthStencilState.
		ID3D11DepthStencilState *m_pDepthStencilState;
		UINT m_StencilRef;

		// RSSetState.
		ID3D11RasterizerState *m_pRasterizerState;

		// IASetPrimitiveTopology.
		INT m_Topology;

		// IASetInputLayout.
		ID3D11InputLayout *m_pInputLayout;

		// IASetVertexBuffers
		ID3D11Buffer *	m_pVertexBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
		UINT			m_Strides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
		UINT			m_Offsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
		ID3D11Buffer *	m_pIndexBuffer;
		UINT			m_IndexOffset;

		// VS/PSSetShaderResources
		ID3D11ShaderResourceView *	m_pVSShaderResources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];

		// ##SetConstantBuffers.
		STAGE_CONSTANT_BUFFERS m_VSConstBuffers;
		STAGE_CONSTANT_BUFFERS m_PSConstBuffers;
		STAGE_CONSTANT_BUFFERS m_CSConstBuffers;
		STAGE_CONSTANT_BUFFERS m_DSConstBuffers;
		STAGE_CONSTANT_BUFFERS m_HSConstBuffers;
		STAGE_CONSTANT_BUFFERS m_GSConstBuffers;

		// PSSetSamplers.
		STAGE_SAMPLERS m_PSSamplers;
		STAGE_SAMPLERS m_VSSamplers;

		friend struct RageDirect3DDeviceContext11;
	};

	public:
		void ResetCachedStates()
		{
			m_DeviceState.ResetCachedStates();
		}
		DEVICE_STATE &GetDeviceState()
		{
			return m_DeviceState;
		}
	private:
		DEVICE_STATE m_DeviceState;

#endif //__D3D11_USE_STATE_CACHING

};
#else // DEVICE_USE_D3D_WRAPPER

#if __D3D11_MONO_DRIVER
typedef ID3D11DeviceX					RageDirect3DDevice11;
typedef ID3D11DeviceContextX			RageDirect3DDeviceContext11;
#elif __D3D11_1
struct ID3D11Device1;
struct ID3D11DeviceContext1;
typedef ID3D11Device1					RageDirect3DDevice11;
typedef ID3D11DeviceContext1			RageDirect3DDeviceContext11;
#elif __D3D11
typedef struct ID3D11Device				RageDirect3DDevice11;
typedef struct ID3D11DeviceContext		RageDirect3DDeviceContext11;
#endif // __D3D11_1

#endif // DEVICE_USE_D3D_WRAPPER

#undef NR


#endif // __D3D11

#endif	// GRCORE_WRAPPER_D3D_H

