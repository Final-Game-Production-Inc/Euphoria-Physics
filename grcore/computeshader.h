#ifndef __GRCORE_COMPUTESHADER_H_
#define __GRCORE_COMPUTESHADER_H_

#include "effect_config.h"
#include "effect.h"
#include "device.h"

#if __D3D11
#include "grcore/d3dwrapper.h"

namespace rage
{
	class grmShader;

	class BaseCS
	{
	public:
		BaseCS(grmShader* pShader = NULL);
		virtual ~BaseCS();

		virtual void Init() = 0;
		virtual void Shutdown() = 0;
		virtual void SetupResources(u8 programId = 0) const;
		virtual void Run(u16 xthread, u16 ythread, u16 zthread) const { Run("Unknown CS", xthread, ythread, zthread, 0); }
		virtual void CleanUpResources() const;

		//static HRESULT CreateTexture2D(u32 width, u32 height, DXGI_FORMAT Format, ID3D11Texture2D** ppTextureOut );
		//static HRESULT CreateStructuredBuffer( u32 uElementSize, u32 uCount, void* pInitData, ID3D11Buffer** ppBufOut, bool useAsIndirectDraw = false );
		//static HRESULT CreateRawBuffer( u32 uSize, VOID* pInitData, ID3D11Buffer** ppBufOut );
		//static HRESULT CreateBufferSRV( ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut );
		//static HRESULT CreateBufferUAV( ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** pUAVOut, bool append = false );

		//static HRESULT CreateTexture2DUAV( ID3D11Texture2D* pTexture, ID3D11UnorderedAccessView** pUAVOut );

	protected:
		void Run(const char pDescription[], u16 xthread, u16 ythread, u16 zthread, u8 programId) const;
		grmShader*					m_Shader;
	};

	class CSManagerBase
	{
	public:
		CSManagerBase();
		virtual ~CSManagerBase();

		virtual void Init()		= 0;
		virtual void Shutdown() = 0;

		void	RunComputeShader(u16 index, u16 xthread, u16 ythread, u16 zthread) const;

		BaseCS*	GetComputeShader(u16 index) const		{ return (index < m_ComputeShaders.GetCount()) ? m_ComputeShaders[index] : NULL; }
	protected:
		atArray<BaseCS*>			m_ComputeShaders;
	};
}
#endif

#endif
