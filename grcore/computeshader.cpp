
#include "computeshader.h"
#include "grmodel/shader.h"
#include "grcore/resourcecache.h"

#if __D3D11
#include "grcore/wrapper_d3d.h"

namespace rage
{
	BaseCS::BaseCS(grmShader* pShader /*=NULL*/) :
		  m_Shader(pShader)
	{
	}
	BaseCS::~BaseCS()
	{
		m_Shader = NULL;

	}
#if 0
	//--------------------------------------------------------------------------------------
	// Create Structured Buffer
	HRESULT BaseCS::CreateStructuredBuffer( u32 uElementSize, u32 uCount, void* pInitData, ID3D11Buffer** ppBufOut, bool useAsIndirectDraw )
	{
		*ppBufOut = NULL;

		D3D11_BUFFER_DESC desc;
		ZeroMemory( &desc, sizeof(desc) );
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		desc.ByteWidth = uElementSize * uCount;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.MiscFlags |= useAsIndirectDraw ? D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS : 0;
		desc.StructureByteStride = uElementSize;

		if ( pInitData )
		{
			D3D11_SUBRESOURCE_DATA InitData;
			InitData.pSysMem = pInitData;
			return grcResourceCache::GetInstance().CreateBuffer( &desc, &InitData, ppBufOut );
		} 

		return static_cast<RageDirect3DDevice11*>(GRCDEVICE.GetCurrent())->m_Inner->CreateBuffer(&desc, NULL, ppBufOut);
		//return grcResourceCache::GetInstance().CreateBuffer( &desc, NULL, ppBufOut );
	}

	//--------------------------------------------------------------------------------------
	// Create Texture2D
	HRESULT BaseCS::CreateTexture2D(u32 width, u32 height, DXGI_FORMAT Format, ID3D11Texture2D** ppTextureOut )
	{
		D3D11_TEXTURE2D_DESC Desc;
		ZeroMemory( &Desc, sizeof( Desc ) );

		Desc.Width = width;
		Desc.Height = height;
		Desc.MipLevels = 1;
		Desc.ArraySize = 1;
		Desc.Format = Format;
		Desc.SampleDesc.Count = 1;
		Desc.Usage = D3D11_USAGE_DEFAULT;
		Desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

		return grcResourceCache::GetInstance().CreateTexture2D(&Desc, NULL, ppTextureOut);
	}

	//--------------------------------------------------------------------------------------
	// Create Raw Buffer
	HRESULT BaseCS::CreateRawBuffer( u32 uSize, void* pInitData, ID3D11Buffer** ppBufOut )
	{
		*ppBufOut = NULL;

		D3D11_BUFFER_DESC desc;
		ZeroMemory( &desc, sizeof(desc) );
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_INDEX_BUFFER | D3D11_BIND_VERTEX_BUFFER;
		desc.ByteWidth = uSize;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

		if ( pInitData )
		{
			D3D11_SUBRESOURCE_DATA InitData;
			InitData.pSysMem = pInitData;
			return grcResourceCache::GetInstance().CreateBuffer( &desc, &InitData, ppBufOut );
		} else
			return grcResourceCache::GetInstance().CreateBuffer( &desc, NULL, ppBufOut );
	}
	//--------------------------------------------------------------------------------------
	// Create Shader Resource View for Structured or Raw Buffers
	HRESULT BaseCS::CreateBufferSRV( ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut )
	{
		D3D11_BUFFER_DESC descBuf;
		ZeroMemory( &descBuf, sizeof(descBuf) );
		pBuffer->GetDesc( &descBuf );

		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		ZeroMemory( &desc, sizeof(desc) );
		desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		desc.BufferEx.FirstElement = 0;

		if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS )
		{
			// This is a Raw Buffer

			desc.Format = DXGI_FORMAT_R32_TYPELESS;
			desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
			desc.BufferEx.NumElements = descBuf.ByteWidth / 4;
		} 
		else
		{
			if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
			{
				// This is a Structured Buffer

				desc.Format = DXGI_FORMAT_UNKNOWN;
				desc.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
			} 
			else
			{
				return E_INVALIDARG;
			}
		}

			return GRCDEVICE.GetCurrent()->CreateShaderResourceView( pBuffer, &desc, ppSRVOut );
	}

	//--------------------------------------------------------------------------------------
	// Create Unordered Access View for Structured or Raw Buffers
	HRESULT BaseCS::CreateBufferUAV( ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** ppUAVOut, bool append )
	{
		D3D11_BUFFER_DESC descBuf;
		ZeroMemory( &descBuf, sizeof(descBuf) );
		pBuffer->GetDesc( &descBuf );

		D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
		ZeroMemory( &desc, sizeof(desc) );
		desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		desc.Buffer.FirstElement = 0;

		if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS )
		{
			// This is a Raw Buffer

			desc.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
			desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
			desc.Buffer.NumElements = descBuf.ByteWidth / 4; 
		} 
		else
		{
			if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
			{
				// This is a Structured Buffer
				desc.Buffer.Flags = append ? D3D11_BUFFER_UAV_FLAG_APPEND : 0;
				desc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
				desc.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride; 
			} 
			else
			{
				return E_INVALIDARG;
			}
		}

		return GRCDEVICE.GetCurrent()->CreateUnorderedAccessView( pBuffer, &desc, ppUAVOut );
	}

	//--------------------------------------------------------------------------------------
	// Create Unordered Access View for Texture2D
	HRESULT BaseCS::CreateTexture2DUAV( ID3D11Texture2D* pTexture, ID3D11UnorderedAccessView** ppUAVOut)
	{
		if ( *ppUAVOut ) 
		{
			SAFE_RELEASE_RESOURCE(*ppUAVOut);
		}
		HRESULT hr = D3D_OK;
		D3D11_TEXTURE2D_DESC Desc;
		ZeroMemory( &Desc, sizeof( Desc ) );
		pTexture->GetDesc( &Desc );

		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		ZeroMemory( &UAVDesc, sizeof( D3D11_UNORDERED_ACCESS_VIEW_DESC ) );
		UAVDesc.Format = Desc.Format;
		UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Buffer.FirstElement = 0;
		UAVDesc.Buffer.NumElements = Desc.Width * Desc.Height;
		hr = GRCDEVICE.GetCurrent()->CreateUnorderedAccessView( pTexture, &UAVDesc, ppUAVOut );
		Assert( D3D_OK == hr );
		return hr;
	}
#endif
	void BaseCS::SetupResources(u8) const
	{
		/*grcComputeProgram* pComputeProgram = m_Shader->GetComputeProgram(pass+1); // (+1) is moved here from the old GetComputeProgram() implementation
		pComputeProgram->Bind(m_Shader->GetInstanceData(), m_Shader->GetInstanceData().GetBasis().GetLocalVar());
		pComputeProgram->SetConstantBuffer(CS_TYPE);
		pComputeProgram->SetTextureResource(CS_TYPE);*/
	}
	void BaseCS::CleanUpResources() const
	{
		/*ID3D11DeviceContext* pd3dImmediateContext = GRCDEVICE.GetCurrentContextEx();
		pd3dImmediateContext->CSSetShader( NULL, NULL, 0 );*/
	}
	//-------------------------------------------------------------------------------------------
	// run the compute shader using its resources 
	void BaseCS::Run( const char pDescription[], u16 xthread, u16 ythread, u16 zthread, u8 programId) const
	{
		//ID3D11DeviceContext* pd3dImmediateContext = GRCDEVICE.GetCurrent();

		SetupResources(programId);

		GRCDEVICE.RunComputation( pDescription, *m_Shader, programId,
			(xthread != static_cast<u16>(-1)) ? xthread : 1, 
			(ythread != static_cast<u16>(-1)) ? ythread : 1,
			(zthread != static_cast<u16>(-1)) ? zthread : 1 );

		/*g_grcCurrentContext->Dispatch( (xthread != static_cast<u16>(-1)) ? xthread : 1, 
										(ythread != static_cast<u16>(-1)) ? ythread : 1,
										(zthread != static_cast<u16>(-1)) ? zthread : 1 );*/

		CleanUpResources();
	}

	//--------------------------------------------------------------------------------------
	CSManagerBase::CSManagerBase()
	{

	}
	CSManagerBase::~CSManagerBase()
	{
		for (int  i = 0; i < m_ComputeShaders.GetCount(); ++i)
		{
			delete m_ComputeShaders[i];
		}
	}

	void CSManagerBase::RunComputeShader( u16 index, u16 xthread, u16 ythread, u16 zthread) const
	{
		m_ComputeShaders[index]->Run(xthread, ythread, zthread);
	}
}
#endif
