// 
// grcore/wrapper_d3d.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#if __WIN32PC || RSG_DURANGO

#include "wrapper_d3d.h"

#if DEVICE_EKG
#include "profile/element.h"
#include "profile/group.h"
#include "profile/page.h"

#define DEVICE_EKG_MONITOR_MAJOR(name)		PF_TIMER(APITimer_##name, d3dAPICallGroupTimingMajor); PF_COUNTER(APICount_##name, d3dAPICallGroupCountMajor);
#define DEVICE_EKG_MONITOR_MINOR(name)		PF_TIMER(APITimer_##name, d3dAPICallGroupTimingMinor); PF_COUNTER(APICount_##name, d3dAPICallGroupCountMinor);

PF_PAGE(d3dAPICallPageMajor, "D3D_API_Major");
PF_GROUP(d3dAPICallGroupCountMajor);
PF_GROUP(d3dAPICallGroupTimingMajor);
PF_LINK(d3dAPICallPageMajor, d3dAPICallGroupCountMajor);
PF_LINK(d3dAPICallPageMajor, d3dAPICallGroupTimingMajor);

PF_PAGE(d3dAPICallPageMinor, "D3D_API_Minor");
PF_GROUP(d3dAPICallGroupCountMinor);
PF_GROUP(d3dAPICallGroupTimingMinor);
PF_LINK(d3dAPICallPageMinor, d3dAPICallGroupCountMinor);
PF_LINK(d3dAPICallPageMinor, d3dAPICallGroupTimingMinor);


DEVICE_EKG_MONITOR_MAJOR(AllCalls);
DEVICE_EKG_MONITOR_MINOR(Other);
DEVICE_EKG_MONITOR_MAJOR(Draw);
DEVICE_EKG_MONITOR_MAJOR(Map);
DEVICE_EKG_MONITOR_MAJOR(UnMap);
DEVICE_EKG_MONITOR_MAJOR(MapCBuffer);
DEVICE_EKG_MONITOR_MAJOR(UnMapCBuffer);
DEVICE_EKG_MONITOR_MINOR(IASetVertexBuffers);
DEVICE_EKG_MONITOR_MINOR(IASetVertexBuffersTotal);
DEVICE_EKG_MONITOR_MINOR(IASetVertexBuffersSingleCount);
DEVICE_EKG_MONITOR_MINOR(CreateBuffer);
DEVICE_EKG_MONITOR_MINOR(CreateTexture);
DEVICE_EKG_MONITOR_MINOR(PrivateData);

DEVICE_EKG_MONITOR_MINOR(CPUFlush);
DEVICE_EKG_MONITOR_MINOR(GPUFlush);

// DeviceContext funcs
DEVICE_EKG_MONITOR_MINOR(CopyResource);
DEVICE_EKG_MONITOR_MINOR(CopySubResourceRegion);
DEVICE_EKG_MONITOR_MINOR(CopyStructureCount);
DEVICE_EKG_MONITOR_MINOR(IASetIndexBuffer);
DEVICE_EKG_MONITOR_MINOR(IASetPrimitiveTopology);
DEVICE_EKG_MONITOR_MINOR(IASetInputLayout);
DEVICE_EKG_MONITOR_MINOR(PSSetConstantBuffers);
DEVICE_EKG_MONITOR_MINOR(PSSetConstantBuffers1);
DEVICE_EKG_MONITOR_MINOR(PSSetPlacementConstantBuffer);
DEVICE_EKG_MONITOR_MINOR(PSSetSamplers);
DEVICE_EKG_MONITOR_MINOR(PSSetShader);
DEVICE_EKG_MONITOR_MINOR(PSSetShaderResources);
DEVICE_EKG_MONITOR_MINOR(PSSetFastShaderResource);
DEVICE_EKG_MONITOR_MINOR(RSSets);
DEVICE_EKG_MONITOR_MINOR(VSSetConstantBuffers);
DEVICE_EKG_MONITOR_MINOR(VSSetConstantBuffers1);
DEVICE_EKG_MONITOR_MINOR(VSSetPlacementConstantBuffer);
DEVICE_EKG_MONITOR_MINOR(VSSetSamplers);
DEVICE_EKG_MONITOR_MINOR(VSSetShader);
DEVICE_EKG_MONITOR_MINOR(VSSetShaderResources);
DEVICE_EKG_MONITOR_MINOR(OMSetBlendState);
DEVICE_EKG_MONITOR_MINOR(OMSetDepthStencilState);
DEVICE_EKG_MONITOR_MINOR(OMSetRenderTargets);
DEVICE_EKG_MONITOR_MINOR(OMSetRenderTargetsAndUnorderedAccessViews);

DEVICE_EKG_MONITOR_MINOR(ResolveSubresource);
DEVICE_EKG_MONITOR_MINOR(UpdateSubresource);

DEVICE_EKG_MONITOR_MINOR(ResourceGets);

DEVICE_EKG_MONITOR_MINOR(CSResourceSets);
DEVICE_EKG_MONITOR_MINOR(CSSetConstantBuffers);
DEVICE_EKG_MONITOR_MINOR(Dispatch);
DEVICE_EKG_MONITOR_MINOR(DSResourceSets);
DEVICE_EKG_MONITOR_MINOR(GSResourceSets);
DEVICE_EKG_MONITOR_MINOR(HSResourceSets);
DEVICE_EKG_MONITOR_MINOR(CommandList);
DEVICE_EKG_MONITOR_MINOR(FinishCommandList);
DEVICE_EKG_MONITOR_MINOR(GetData);
DEVICE_EKG_MONITOR_MINOR(MiscCreate);
DEVICE_EKG_MONITOR_MINOR(Predication);

DEVICE_EKG_MONITOR_MINOR(Flush);
DEVICE_EKG_MONITOR_MINOR(Present);

#endif // DEVICE_EKG

#if CHECKING_DX_HRESULTS
HRESULT CheckDxHresult(HRESULT hr)
{
#	if __ASSERT
#		if RSG_PC
#			if __D3D11_1
#				if USE_UNICODE
					if (!SUCCEEDED(hr))
					{
						const int iStringSize = 256;
						wchar_t wErrDesc[iStringSize];
						const wchar_t* pwErrString;
						DXGetErrorDescription(hr, wErrDesc, iStringSize);
						pwErrString = DXGetErrorString(hr);
						char errString[iStringSize];
						char errDesc[iStringSize];
						wcstombs(errString, pwErrString, iStringSize);
						wcstombs(errDesc, wErrDesc, iStringSize);
						Assertf(SUCCEEDED(hr), "%s: %s", errString, errDesc);
					}
#				else // !USE_UNICODE
					const int iStringSize = 256;
					static __THREAD char wErrDesc[iStringSize];
					Assertf(SUCCEEDED(hr), "%s: %s", DXGetErrorString(hr), DXGetErrorDescription(hr, wErrDesc, iStringSize - 1));
#				endif // USE_UNICODE
#			else // __D3D11_1
				Assertf(SUCCEEDED(hr), "%s: %s", DXGetErrorString(hr), DXGetErrorDescription(hr));
#			endif // __D3D11_1
#		else // RSG_PC
			Assertf(SUCCEEDED(hr), "0x%08x", hr);
#		endif // RSG_PC
#	endif // __ASSERT
	return hr;
}
#endif // CHECKING_DX_HRESULTS


#if RSG_PC
FORCEINLINE static void QuitfErrGfxD3dInit(const char *pDetailedReason)
{
	Quitf(rage::ERR_GFX_D3D_INIT, "Device Remove - Unable to recover (%s)", pDetailedReason);
}
#endif // RSG_PC...

void CheckDxHresultFatal(HRESULT WIN32PC_ONLY(hr))
{
#if RSG_PC
	switch (hr)
	{
	case D3DERR_DRIVERINTERNALERROR:
		QuitfErrGfxD3dInit("D3DERR_DRIVERINTERNALERROR");		break;
	case D3DERR_NOTAVAILABLE:
		QuitfErrGfxD3dInit("D3DERR_NOTAVAILABLE");				break;
	case D3DERR_INVALIDDEVICE:
		QuitfErrGfxD3dInit("D3DERR_INVALIDDEVICE");				break;
	case D3DERR_DRIVERINVALIDCALL:
		QuitfErrGfxD3dInit("D3DERR_DRIVERINVALIDCALL");			break;
	case DXGI_ERROR_DEVICE_REMOVED:
		QuitfErrGfxD3dInit("DXGI_ERROR_DEVICE_REMOVED");		break;
	case DXGI_ERROR_DEVICE_HUNG:
		QuitfErrGfxD3dInit("DXGI_ERROR_DEVICE_HUNG");			break;
	case DXGI_ERROR_DEVICE_RESET:
		QuitfErrGfxD3dInit("DXGI_ERROR_DEVICE_RESET");			break;
	case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
		QuitfErrGfxD3dInit("DXGI_ERROR_DRIVER_INTERNAL_ERROR");	break;
	}
#endif // RSG_PC
}

#if __D3D9
RageDirect3DDevice9::RageDirect3DDevice9()
{
	ClearCachedState();
}

HRESULT RageDirect3DDevice9::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	ClearCachedState();
	return CHECK_HRESULT(m_Inner->Reset(pPresentationParameters));
}

void RageDirect3DDevice9::ClearCachedState()
{
	unsigned int uIndex;
	for (uIndex = 0; uIndex < 256; uIndex++)
	{
		m_aiRenderStates[uIndex] = 0xFFFFFFFE;
		m_aiFlushedRenderStates[uIndex] = 0xFFFFFFFE;
	}
	for (uIndex = 0; uIndex < (D3DDMAPSAMPLER + 16); uIndex++)
	{
		m_aiTextures[uIndex] = NULL;
	}
	for (uIndex = 0; uIndex < (D3DDMAPSAMPLER + 16); uIndex++)
	{
		for (unsigned int uStates = 0; uStates < 16; uStates++)
			m_aaiSamplerStates[uIndex][uStates] = 0xFFFFFFFE;
	}
	for (uIndex = 0; uIndex < 256; uIndex++)
	{
		memset(m_aVertexFloatConstants, 0, sizeof(m_aVertexFloatConstants));
		memset(m_aFlushedVertexFloatConstants, 0, sizeof(m_aFlushedVertexFloatConstants));
		memset(m_aPixelFloatConstants, 0, sizeof(m_aPixelFloatConstants));
		memset(m_aFlushedPixelFloatConstants, 0, sizeof(m_aFlushedPixelFloatConstants));
	}
	m_aiRenderStateTouched = 0;
	m_aiVertexFloatTouched = 0;
	m_aiPixelFloatTouched = 0;
	m_LazyTouched = false;
}

HRESULT RageDirect3DDevice9::ExternSetRenderState(D3DRENDERSTATETYPE State,DWORD Value)
{
	Assert(State < 256);

	if (m_aiRenderStates[State] == Value)
		return D3D_OK;

	m_aiRenderStates[State] = Value;
	m_aiRenderStateTouched |= (1ULL<<(State>>3));
	m_LazyTouched = true;
	return D3D_OK;
}

void RageDirect3DDevice9::ExternLazyFlush()
{
	if (m_aiRenderStateTouched)
	{
		unsigned touched = m_aiRenderStateTouched, mask = 1;
		m_aiRenderStateTouched = 0;
		for (unsigned i=0; touched; i+=8, mask<<=1)
		{
			if (touched & mask)
			{
				touched &= ~mask;
				for (unsigned j=i; j<i+8; j++)
				{
					if (m_aiRenderStates[j] != m_aiFlushedRenderStates[j]) 
						m_Inner->SetRenderState((D3DRENDERSTATETYPE)j,m_aiFlushedRenderStates[j] = m_aiRenderStates[j]);
				}
			}
		}
	}
#if 0	// Use this version for sanity checking if you don't trust the fancy minimal update version.
#define LazyUpdate(type) \
	if (m_ai##type##FloatTouched) \
	{ \
		m_Inner->Set##type##ShaderConstantF(0, m_a##type##FloatConstants[0], 224); \
		m_ai##type##FloatTouched = 0; \
	}
#else
#define LazyUpdate(type) \
	if (m_ai##type##FloatTouched) \
	{ \
		unsigned base = 0; \
		unsigned __int64 mask = 1ULL; \
		unsigned __int64 touched = m_ai##type##FloatTouched; \
		m_ai##type##FloatTouched = 0; \
		while (touched) \
		{ \
			if ((touched & mask) && ((touched &= ~mask), memcmp(m_a##type##FloatConstants[base], m_aFlushed##type##FloatConstants[base], 64))) \
			{ \
				unsigned stop = base + 4; \
				mask <<= 1; \
				while ((touched & mask) && ((touched &= ~mask), memcmp(m_a##type##FloatConstants[stop], m_aFlushed##type##FloatConstants[stop], 64))) \
				{ \
					stop += 4; \
					mask <<= 1; \
				} \
				memcpy(m_aFlushed##type##FloatConstants[base], m_a##type##FloatConstants[base], (stop - base) * 16); \
				m_Inner->Set##type##ShaderConstantF(base, m_aFlushed##type##FloatConstants[base], stop - base); \
				base = stop + 4; \
				mask <<= 1; \
			} \
			else \
			{ \
				base += 4; \
				mask <<= 1; \
			} \
		} \
	}
#endif

	LazyUpdate(Vertex)
	LazyUpdate(Pixel)

	m_LazyTouched = false;
}

HRESULT RageDirect3DDevice9::ExternSetTexture(DWORD Stage,IDirect3DBaseTexture9* pTexture)
{
	Assert(Stage < (D3DDMAPSAMPLER + 16));
	if (m_aiTextures[Stage] == pTexture)
		return D3D_OK;

	m_aiTextures[Stage] = pTexture;
	return CHECK_HRESULT(m_Inner->SetTexture(Stage,pTexture));
}

HRESULT RageDirect3DDevice9::ExternSetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value)
{
	Assert(Sampler < (D3DDMAPSAMPLER + 16));
	Assert(Type < 16);

#if __WIN32PC
	// TODO: Clamp this at runtime based on supported texture filter capabilities
	if (Type == D3DSAMP_MAGFILTER && Value >= D3DTEXF_ANISOTROPIC)
		Value = D3DTEXF_LINEAR;
#endif

	if (m_aaiSamplerStates[Sampler][Type] == Value)
		return D3D_OK;

	m_aaiSamplerStates[Sampler][Type] = Value;
	return CHECK_HRESULT(m_Inner->SetSamplerState(Sampler,Type,Value));
}

#define LazySet(type) \
	Assert(StartRegister < 256); \
	Assert((StartRegister + Vector4fCount) < 256); \
	if (!memcmp(pConstantData,&m_a##type##FloatConstants[StartRegister], Vector4fCount * 16)) \
	{ \
		return D3D_OK; \
	} \
	memcpy(&m_a##type##FloatConstants[StartRegister], pConstantData, Vector4fCount * 16); \
	UINT first = StartRegister >> 2, last = (StartRegister + Vector4fCount - 1) >> 2; \
	unsigned __int64 mask = (1ULL << first); \
	while (first <= last) \
	{ \
		m_ai##type##FloatTouched |= mask; \
		++first; \
		mask<<=1; \
	} \
	m_LazyTouched = true; \
	return D3D_OK;


HRESULT RageDirect3DDevice9::ExternSetPixelShaderConstantF( UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount )
{
	LazySet(Pixel)
}

HRESULT RageDirect3DDevice9::ExternSetPixelShaderConstantI( UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount )
{
	Assert(StartRegister < 256);
	Assert((StartRegister + Vector4iCount) < 256);
	if (!memcmp(pConstantData,&m_aPixelIntegerConstants[StartRegister], Vector4iCount * sizeof(UINT)))
	{
		return D3D_OK;
	}
	memcpy(&m_aPixelIntegerConstants[StartRegister], pConstantData, Vector4iCount * sizeof(UINT));
	return CHECK_HRESULT(m_Inner->SetPixelShaderConstantI(StartRegister,pConstantData,Vector4iCount));
}

HRESULT RageDirect3DDevice9::ExternSetPixelShaderConstantB( UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount )
{
	Assert(StartRegister < 256);
	Assert((StartRegister + BoolCount) < 256);
	if (!memcmp(pConstantData,&m_aPixelBooleanConstants[StartRegister], BoolCount * sizeof(UINT)))
	{
		return D3D_OK;
	}
	memcpy(&m_aPixelBooleanConstants[StartRegister], pConstantData, BoolCount * sizeof(UINT));
	return CHECK_HRESULT(m_Inner->SetPixelShaderConstantB(StartRegister,pConstantData,BoolCount));
}

HRESULT RageDirect3DDevice9::ExternSetVertexShaderConstantF( UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount )
{
	LazySet(Vertex)
}

HRESULT RageDirect3DDevice9::ExternSetVertexShaderConstantI( UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount )
{
	Assert(StartRegister < 256);
	Assert((StartRegister + Vector4iCount) < 256);
	if (!memcmp(pConstantData,&m_aVertexIntegerConstants[StartRegister], Vector4iCount * sizeof(UINT)))
	{
		return D3D_OK;
	}
	memcpy(&m_aVertexIntegerConstants[StartRegister], pConstantData, Vector4iCount * sizeof(UINT));
	return CHECK_HRESULT(m_Inner->SetVertexShaderConstantI(StartRegister,pConstantData,Vector4iCount));
}

HRESULT RageDirect3DDevice9::ExternSetVertexShaderConstantB( UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount )
{
	Assert(StartRegister < 256);
	Assert((StartRegister + BoolCount) < 256);
	if (!memcmp(pConstantData,&m_aVertexBooleanConstants[StartRegister], BoolCount * sizeof(UINT)))
	{
		return D3D_OK;
	}
	memcpy(&m_aVertexBooleanConstants[StartRegister], pConstantData, BoolCount * sizeof(UINT));
	return CHECK_HRESULT(m_Inner->SetVertexShaderConstantB(StartRegister,pConstantData,BoolCount));
}
#endif // __D3D9

#if __D3D11

#if DEVICE_USE_D3D_WRAPPER && __BANK && __D3D11_REPORT_RESOURCE_HAZARDS

#include "grprofile/pix.h"

char *RageDirect3DDeviceContext11::s_PhysicalUnitNames[D3D11PhysicalUnitMax] = 
{
	"Compute Shader",
	"Domain Shader",
	"Geometry Shader",
	"Hull Shader",
	"Pixel Shader",
	"Vertex Shader"
};


void RageDirect3DDeviceContext11::ResetReportResourceHazards()
{
	u32 i;

	for( i=0; i<D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++ )
	{
		m_pRenderTargetResources[i] = NULL;
	}

	for( i=0; i<D3D11PhysicalUnitMax; i++ )
	{
		u32 j;

		for( j=0; j<D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; j++ )
		{
			m_pShaderResources[i][j] = NULL;
		}
	}
	m_pDepthStencilResource = NULL;
}


void RageDirect3DDeviceContext11::ReportHazardsOnShaderResourceBind( D3D11PhysicalUnitType PhysicalUnit, UINT StartSlot, UINT Count, ID3D11ShaderResourceView  * const *ppShaderResourceViews )
{
	u32 i;

	for( i=0; i<Count; i++ )
	{
		ID3D11Resource *pResource = NULL;

		if( ppShaderResourceViews[i] )
		{
			ppShaderResourceViews[i]->GetResource( &pResource );

			if( pResource != NULL )
			{
				char Data[256];
				UINT DataSize = 256;
				s32 Slot = IsBoundAsRenderTarget( pResource );

				if(( Slot != -1 ) || ( pResource == m_pDepthStencilResource ))
				{
					CHECK_HRESULT(pResource->GetPrivateData( WKPDID_D3DDebugObjectName, &DataSize, Data ));
				}

				if( Slot != -1 )
				{
				#if ENABLE_PIX_TAGGING
					PIXBeginN( 0x1, "Error:- %s(%d):%s (bound to RT %d)", s_PhysicalUnitNames[PhysicalUnit], i + StartSlot, Data, Slot );
					PIXEnd();
				#endif// ENABLE_PIX_TAGGING

					grcWarningf( "Attempting to bind resource \"%s\" in slot %d of unit %s when it`s already bound as the %d-th render target.", Data, i + StartSlot, s_PhysicalUnitNames[PhysicalUnit], Slot );
				}
				if( pResource == m_pDepthStencilResource )
				{
				#if ENABLE_PIX_TAGGING
					PIXBeginN( 0x1, "Error:- %s(%d):%s (depthbuffer)", s_PhysicalUnitNames[PhysicalUnit], i + StartSlot, Data );
					PIXEnd();
				#endif //ENABLE_PIX_TAGGING

					grcWarningf( "Attempting to bind resource \"%s\" in slot %d of unit %s when it`s already bound as the depth buffer.", Data, i + StartSlot, s_PhysicalUnitNames[PhysicalUnit], Slot );
				}
			}
		}
		m_pShaderResources[PhysicalUnit][i + StartSlot] = pResource;
	}
}

void RageDirect3DDeviceContext11::ReportHazardsOnSetRenderTargets( UINT NumViews, ID3D11RenderTargetView * const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView )
{
	u32 i;

	for( i=0; i<D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++ )
	{
		m_pRenderTargetResources[i] = NULL;
	}

	for( i=0; i<NumViews; i++)
	{
		ID3D11Resource *pResource = NULL;

		if( ppRenderTargetViews[i] )
		{
			ppRenderTargetViews[i]->GetResource( &pResource );
		}
		m_pRenderTargetResources[i] = pResource;
	}

	ID3D11Resource *pDepthResource = NULL;

	if( pDepthStencilView )
		pDepthStencilView->GetResource( &pDepthResource );

	m_pDepthStencilResource = pDepthResource;
}


s32 RageDirect3DDeviceContext11::IsBoundAsRenderTarget( ID3D11Resource *pResource )
{
	u32 i;

	for( i=0; i<D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++ )
	{
		if( pResource == m_pRenderTargetResources[i] )
		{
			return i;
		}
	}
	return -1;
}


s32 RageDirect3DDeviceContext11::IsBoundAsAShaderResource( ID3D11Resource *pResource, D3D11PhysicalUnitType PhysicalUnit )
{
	(void)PhysicalUnit;
	(void)pResource;
	return -1;
}

#endif //DEVICE_USE_D3D_WRAPPER && __BANK && __D3D11_REPORT_RESOURCE_HAZARDS

#endif //__D3D11

#endif
