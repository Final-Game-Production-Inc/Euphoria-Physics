//
// grcore/effect_d3d11.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "config.h"
#if __D3D11

#include "effect.h"
#include "effect.inl"

#include "channel.h"

#include "grcore/wrapper_d3d.h"
#include "profile/timebars.h"
#include "system/criticalsection.h"
#include "system/membarrier.h"
#include "system/xtl.h"

#define DX_ONLY_INCLUDE_ONCE
#if RSG_DURANGO
#	include <D3D11Shader_x.h>
#	include <D3DCompiler_x.h>
#else
#	include <D3D11Shader.h>
#	include <D3DCompiler.h>
#endif

namespace rage
{

#if RSG_DURANGO
extern DECLARE_MTR_THREAD ID3D11InputLayout *s_InputLayout;
#endif

sysCriticalSectionToken s_declSetupCs;

void grcVertexProgram::Bind(const grcVertexDeclaration *decl) const
{
	DeclSetup *ds = FirstDecl;
	while (ds)
		if (ds->FetchDecl == decl)
			break;
		else
			ds = ds->Next;

	if (!ds) {
		SYS_CS_SYNC(s_declSetupCs);

		// Check again after we grab the critsec in case somebody else tried and succeeded simultaneously
		ds = FirstDecl;
		while (ds)
			if (ds->FetchDecl == decl)
				break;
			else
				ds = ds->Next;

		if (!ds) {
			PF_PUSH_TIMEBAR("Create Vertex Declaration");

			ds = rage_new DeclSetup;
			ds->Next = FirstDecl;
			ds->FetchDecl = decl;
			decl->AddRef();

			ID3D11ShaderReflection* pShader = NULL;
			if (FAILED(D3DReflect( GetProgramData(), GetProgramSize(), IID_ID3D11ShaderReflection, (void**) &pShader) )) {
				Quitf(ERR_GFX_D3D_SHADER_1,"D3DReflect call failed");
			}

			// Copy original descriptors in
			D3D11_INPUT_ELEMENT_DESC temp[32];
			int tempCount = decl->elementCount;
			Assert( decl->elementCount < 32 );

			memcpy(temp,decl->desc,decl->elementCount * sizeof(D3D11_INPUT_ELEMENT_DESC));

			// Identify descriptors demanded by the shader that we don't provide, and wedge them in from stream 4
			D3D11_SHADER_DESC desc;
			CHECK_HRESULT(pShader->GetDesc(&desc));
			D3D11_SIGNATURE_PARAMETER_DESC shaderInputs[32];
			for (u32 i=0; i<desc.InputParameters; i++)
				CHECK_HRESULT(pShader->GetInputParameterDesc(i, &shaderInputs[i]));

			for (UINT i=0; i<desc.InputParameters; i++)
			{
				// Skip built-in attributes
				if (!strncmpi(shaderInputs[i].SemanticName, "SV_", 3))
				{
					break;
				}

				// Find this input in the vertex buffer descriptor.  If it's there, great.  If not, add it.
				bool bFound = false;
				for (int j=0; j<decl->elementCount; j++)
				{
					if (!strcmp(shaderInputs[i].SemanticName,temp[j].SemanticName) && shaderInputs[i].SemanticIndex == temp[j].SemanticIndex)
					{
						bFound = true;
						break;	// Found it
					}
				}

				if (!bFound)
				{
					D3D11_INPUT_ELEMENT_DESC &e = temp[tempCount++];
					e.AlignedByteOffset = 0;
					if (shaderInputs[i].ReadWriteMask == 0x3)
						e.Format = DXGI_FORMAT_R32G32_FLOAT;
					else if (shaderInputs[i].ReadWriteMask == 0x7)
						e.Format = DXGI_FORMAT_R32G32B32_FLOAT;
					else
						e.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					e.InputSlot = s_MissingInputsVertexBufferStream;
					e.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
					e.InstanceDataStepRate = 0;
					e.SemanticName = shaderInputs[i].SemanticName;
					e.SemanticIndex = shaderInputs[i].SemanticIndex;
				}
			}

			ASSERT_ONLY(static int inputLayoutsCreated = 0;)
			AssertMsg(inputLayoutsCreated < 4096, "Exceeded maximum number of InputLayouts possible in D3D11!");
			Assert(GetProgramData());
			Assert(GetProgramSize());
			ID3D11InputLayout *inputLayout;
			HRESULT hr = GRCDEVICE.GetCurrent()->CreateInputLayout(temp,tempCount,GetProgramData(),GetProgramSize(),&inputLayout);
			Assertf( hr == S_OK, "Error %x creating Input layout (%d items).", hr, tempCount );
			ds->InputLayout = inputLayout;

			if (FAILED(hr))
			{
				CheckDxHresultFatal(hr);
				Quitf(ERR_GFX_D3D_SHADER_2,"Failed calling CreateInputLayout.");
			}

			ASSERT_ONLY( inputLayoutsCreated++ );
			pShader->Release();

			// Only when completely initialized can we link the new DeclSetup
			// into the list, since other threads can scan the linked list
			// without locking the crit sec.
			sysMemWriteBarrier();
			FirstDecl = ds;

			PF_POP_TIMEBAR(); // Create Vertex Declaration
		}
	}

	g_grcCurrentContext->IASetInputLayout(ds->InputLayout);

#if RSG_DURANGO
	s_InputLayout = ds->InputLayout;
#endif
}

#if EFFECT_CACHE_PROGRAM
u32 grcProgram::ComputeFingerprint(const void* code, u32 length)
{
#if (RSG_PC || RSG_DURANGO) && 0
	ID3DBlob *blob;
	HRESULT hr = D3DGetBlobPart(code,length,D3D_BLOB_ALL_SIGNATURE_BLOB,0,&blob);
	if (hr == D3D_OK) {
		u32 key = atDataHash((const char*)blob->GetBufferPointer(),blob->GetBufferSize());
		blob->Release();
		return key;
	}
#endif
	return atDataHash((const char *)code,length);
}
#endif // EFFECT_CACHE_PROGRAM

grcShader* grcProgram::CreateShader(const char*
#if __BANK && EFFECT_CACHE_PROGRAM
									pszName
#endif
									, const u8* code, u32 length, ShaderStage eStage, u32 &uKey)
{
#if EFFECT_CACHE_PROGRAM
	uKey = ComputeFingerprint(code, length);
	grcShader* pShader = FindShader(uKey, length, eStage);
#else
	uKey = 0;
	grcShader* pShader = NULL;
#endif // EFFECT_CACHE_PROGRAM
	if (pShader == NULL)
	{
		switch(eStage)
		{
		case ssVertexStage:
			GRCDEVICE.CreateVertexShader((u8*)code,length,(grcVertexShader**)&pShader);
			break;
		case ssPixelStage:
			GRCDEVICE.CreatePixelShader((u8*)code,length,(grcPixelShader**)&pShader);
			break;
		case ssComputeStage:
			GRCDEVICE.CreateComputeShader((u8*)code, length,(grcComputeShader**)&pShader);
			break;
		case ssGeometryStage:
			GRCDEVICE.CreateGeometryShader((u8*)code, length,(grcGeometryShader**)&pShader);
			break;
		case ssHullStage:
			GRCDEVICE.CreateHullShader((u8*)code, length,(grcHullShader**)&pShader);
			break;
		case ssDomainStage:
			GRCDEVICE.CreateDomainShader((u8*)code, length,(grcDomainShader**)&pShader);
			break;
		default:
			Assertf(0, "Unknown/Unsupported Shader Stage %d", eStage);
			return NULL;
		}
#if EFFECT_CACHE_PROGRAM
		sm_ShaderCache[eStage][uKey] = pShader;
#if __BANK
		char* pszCacheName = rage_new char[strlen(pszName) + 2];
		formatf(pszCacheName, strlen(pszName) + 1, "%s", pszName);
		sm_ShaderNameCache[eStage][uKey] = pszCacheName;
#endif // __BANK
#endif // EFFECT_CACHE_PROGRAM
	}
#if EFFECT_CACHE_PROGRAM
	else
	{
		((IUnknown*)pShader)->AddRef();
#if __BANK && 0
		Assert(eStage < ssCount);
		CacheNameMap::iterator it = sm_ShaderNameCache[eStage].find(uKey);
		if (it != sm_ShaderNameCache[eStage].end())
		{
			Displayf("Program Type %d - %s using %s", eStage, pszName, it->second);
		}
#endif // __BANK
	}
#endif // EFFECT_CACHE_PROGRAM
	return pShader;
}

} // namespace rage

#endif // __D3D11
