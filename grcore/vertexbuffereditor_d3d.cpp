// 
// grcore/vertexbuffereditor_d3d.cpp
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/vertexbuffereditor.h"

#if __WIN32 && !RSG_DURANGO

#include "system/xtl.h"
#include "grcore/resourcecache.h"
#include "grcore/device.h"
#include "profile/timebars.h"
#include "wrapper_d3d.h"

#pragma warning(disable:4062)
#include "system/d3d9.h"
#pragma warning(error:4062)

#if !__XENON 
#include "xenon_resource.h"
#if __D3D11
#include "system/d3d11.h"
#endif
#else
#include "system/xgraphics.h"
#include "xdk.h"
#endif // !__XENON 

namespace rage
{

	void grcVertexBufferEditor::MakeDynamic()
	{
		// Reorder the vertex buffer with padding for optimal memory access on the cpu
		// Position - float4
		// Normal - float4
		// Tangent0 - float4
		// Tangent1 - float4
		// Tangent2 - float4
		// Tangent3 - float4
		// Tangent4 - float4
		// Tangent5 - float4
		// Tangent6 - float4
		// Tangent7 - float4
		// TexCoord0 - float4
		// TexCoord1 - float4
		// TexCoord2 - float4
		// TexCoord3 - float4
		// TexCoord4 - float4
		// TexCoord5 - float4
		// TexCoord6 - float4
		// TexCoord7 - float4
		// Weights - float4
		// Binding - u32
		// Diffuse - u32
		// Specular - u32
		// UnusedPadding - u32

		// Create a new Fvf
		const grcFvf* fvf = m_VertexBuffer->GetFvf();
		grcFvf* pNewFvf = rage_new grcFvf(*fvf, true);

		// Replace old buffer with new buffer
		SetFvf(pNewFvf);
		delete fvf;

		// um, let's not try to access a deleted pointer below...
		fvf = m_VertexBuffer->GetFvf();

		u32 nStride = m_VertexBuffer->GetVertexStride();

		// Allocate new vertex buffer
		d3dVertexBuffer* pNewBuffer = 0;
#if __D3D9
		u32 nUsage = 0; // m_VertexBuffer->GetUsage();
#endif
		void* pLockPtr = 0;

#if __WIN32PC && __PAGING
		if( sysMemAllocator::GetCurrent().IsBuildingResource() ) 
		{
			pLockPtr = RESOURCE_ALLOCATOR((nStride * m_VertexBuffer->GetVertexCount()), RAGE_VERTEXBUFFER_ALIGNMENT) ;
		}
		else
#endif // __WIN32PC && __PAGING
		{
#if __XENON
			nUsage |= D3DUSAGE_CPU_CACHED_MEMORY;
			if( sysMemAllocator::GetCurrent().IsBuildingResource() )
			{
				pNewBuffer = rage_new D3DVertexBuffer;
				u32 Length = nStride * m_VertexBuffer->GetVertexCount();
				u8* baseLock = (u8*) physical_new(Length, RAGE_VERTEXBUFFER_ALIGNMENT);
				XGSetVertexBufferHeader(Length, D3DUSAGE_CPU_CACHED_MEMORY, D3DPOOL_MANAGED, 0, pNewBuffer);

				pNewBuffer->Common |= D3DCOMMON_CPU_CACHED_MEMORY;

				XGOffsetResourceAddress(pNewBuffer, baseLock);
			}
			else
#endif // __XENON
			{	
#if __D3D9
				{
					CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateVertexBuffer(nStride * m_VertexBuffer->GetVertexCount(), nUsage, 0, D3DPOOL_MANAGED, &pNewBuffer, 0));
					AssertVerify((pNewBuffer)->Lock(0, 0, &pLockPtr, 0) == D3D_OK);
				}
#elif __D3D11
				{
					PF_AUTO_PUSH_TIMEBAR_BUDGETED("MakeDynamic", 0.1f);
					D3D11_BUFFER_DESC oDesc = {0};
					oDesc.ByteWidth = nStride * m_VertexBuffer->GetVertexCount();
					oDesc.BindFlags = grcBindVertexBuffer;
					oDesc.MiscFlags = 0;
					oDesc.CPUAccessFlags = grcCPUWrite;
					oDesc.Usage = static_cast<D3D11_USAGE>(grcUsageDynamic);

					ASSERT_ONLY(HRESULT hRes = )GRCDEVICE.GetCurrent()->CreateBuffer(&oDesc, NULL, &pNewBuffer);
					AssertMsg(SUCCEEDED(hRes), ("Failed to Create Buffer %x", hRes));

					D3D11_MAPPED_SUBRESOURCE lock;
					ASSERT_ONLY(hRes = )g_grcCurrentContext->Map(pNewBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&lock);
					pLockPtr = lock.pData;
					AssertMsg(SUCCEEDED(hRes), ("Failed to Map Buffer %x", hRes));
				}
#endif
			}
		}

		// Copy data into new buffer
		{
			grcVertexBuffer::LockHelper helper(m_VertexBuffer);

			for( int i = 0; i < m_VertexBuffer->GetVertexCount(); i++ )
			{
				u8* pVertPtr = (u8*)pLockPtr + (nStride * i);
				if( fvf->GetPosChannel() )
				{
					Vector3 vPos = GetPosition(i);
					sysMemCpy(pVertPtr, &vPos, sizeof(Vector3));
					pVertPtr += 16;
				}
				if( fvf->GetNormalChannel() )
				{
					Vector3 vNrm = GetNormal(i);
					sysMemCpy(pVertPtr, &vNrm, sizeof(Vector3));
					pVertPtr += 16;
				}
				for( int j = 0; j < grcTanBiCount; j++ )
				{
					if( fvf->GetTangentChannel(j) )
					{
						Vector4 vTan = GetTangent(i, j);
						sysMemCpy(pVertPtr, &vTan, sizeof(Vector4));
						pVertPtr += 16;
					}
				}
				for( int j = 0; j < grcTexStageCount; j++ )
				{
					if( fvf->GetTextureChannel(j) )
					{
						Vector2 vUV = GetUV(i, j);
						vUV.y = 1.0f - vUV.y;
						sysMemCpy(pVertPtr, &vUV, sizeof(Vector2));
						pVertPtr += 16;
					}
				}
				if( fvf->GetBlendWeightChannel() )
				{
					Vector4 vWeights = GetBlendWeights(i);
					sysMemCpy(pVertPtr, &vWeights, sizeof(Vector4));
					pVertPtr += 16;
				}
				if( fvf->GetBindingsChannel() )
				{
					Color32 bindings = GetBlendIndices(i);
					sysMemCpy(pVertPtr, &bindings, sizeof(Color32));
					pVertPtr += 4;
				}
				if( fvf->GetDiffuseChannel() )
				{
					Color32 diffuse = GetDiffuse(i);
					sysMemCpy(pVertPtr, &diffuse, sizeof(Color32));
					pVertPtr += 4;
				}
				if( fvf->GetSpecularChannel() )
				{
					Color32 spec = GetSpecular(i);
					sysMemCpy(pVertPtr, &spec, sizeof(Color32));
					pVertPtr += 4;
				}
			}
		}

#if __WIN32PC && __PAGING
		if( sysMemAllocator::GetCurrent().IsBuildingResource() ) 
		{
			// No unlock necessary
		}
		else
#endif // __WIN32PC && __PAGING
		{
#if __WIN32PC && __D3D9
				AssertVerify((pNewBuffer)->Unlock() == D3D_OK);
#elif __D3D11
				g_grcCurrentContext->Unmap(pNewBuffer,0);
#endif
		}

#if __WIN32PC && __PAGING
		if( sysMemAllocator::GetCurrent().IsBuildingResource() ) 
		{
			// Replace the vertex data
			delete[] m_VertexBuffer->GetVertexData();
			SetVertexData(reinterpret_cast<u8*>(pLockPtr));
		}
		else
#endif // __WIN32PC && __PAGING
		{
		#if !__D3D11
			grcVertexBufferD3D* vertexBuffer = static_cast<grcVertexBufferD3D*>(m_VertexBuffer);
			d3dVertexBuffer* pOldBuffer = vertexBuffer->GetD3DBuffer();
		#else // !__D3D11
			grcVertexBufferD3D11* vertexBuffer = static_cast<grcVertexBufferD3D11*>(m_VertexBuffer);
			d3dVertexBuffer* pOldBuffer = vertexBuffer->GetD3DBuffer();
		#endif // !__D3D11

		#if !__D3D11
			vertexBuffer->SetD3DBuffer(pNewBuffer);
		#endif //!__D3D11

#if __XENON
			if( sysMemAllocator::GetCurrent().IsBuildingResource() )
				delete pOldBuffer;
			else
#endif // __XENON
			{
				if( pOldBuffer )
					SAFE_RELEASE_RESOURCE(pOldBuffer);
			}
		}	

		SetDynamic();
	}

} // namespace rage

#endif // __WIN32 && !RSG_DURANGO


