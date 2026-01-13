/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2012 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/


#define SCE_SHADER_BINARY_DLL_EXPORT

#include <shader/binary.h>
#include "grcore/gnmx/shader_parser.h"
#include "grcore/gnmx/shaderbinary.h"

using namespace sce;



void sce::Gnmx::parseShader(sce::Gnmx::ShaderInfo *shaderInfo, const void* data, sce::Gnmx::ShaderType shaderType)
{
	const Shader::Binary::Header		*binaryHeader = reinterpret_cast<const Shader::Binary::Header*>(data);
	const Gnmx::ShaderFileHeader		*header		  = reinterpret_cast<const Gnmx::ShaderFileHeader*>(binaryHeader + 1);
	const Gnmx::ShaderCommonData		*shaderCommon = reinterpret_cast<const Gnmx::ShaderCommonData*>(header + 1);
	const uint32_t						*sbAddress	  = reinterpret_cast<const uint32_t*>(shaderCommon + 1);

	SCE_GNM_UNUSED(shaderType);
#ifdef __ORBIS__ // host-side DLLs can't use SCE_GNM_VALIDATE, since it requires statically linking with Gnm
	SCE_GNM_VALIDATE(header->m_fileHeader == Gnmx::kShaderFileHeaderId, "The shader file header is invalid.");
	SCE_GNM_VALIDATE(shaderType != Gnmx::kComputeShader || header->m_majorVersion == 7, "The compute shader version doesn't match the runtime version, please rebuild your shader for v7.*");
	SCE_GNM_VALIDATE(shaderType == Gnmx::kComputeShader || header->m_majorVersion == 7 || header->m_majorVersion == 6, "The graphics shader version doesn't match the runtime version, please rebuild your shader for v6.* or v7.*");

	SCE_GNM_VALIDATE(header->m_type == shaderType, "The shader type doesn't match the input type.");
	SCE_GNM_VALIDATE(sbAddress[1] == 0xFFFFFFFF, "The shader has already been patched.");
#endif // __ORBIS__
	const uint32_t		sbSize		     = shaderCommon->m_shaderSize;
	const uint32_t		sbEmbeddedCbSize = shaderCommon->m_embeddedConstantBufferSizeInDQW*16;
	const uint32_t		sbOffset         = sbAddress[0];
	
	shaderInfo->m_shaderStruct	  = (void*)shaderCommon;
	shaderInfo->m_gpuShaderCode	  = (uint32_t*)((char*)shaderCommon + sbOffset);
	shaderInfo->m_gpuShaderCodeSize = sbSize + sbEmbeddedCbSize;
}

void sce::Gnmx::parseGsShader(sce::Gnmx::ShaderInfo* gsShaderInfo, sce::Gnmx::ShaderInfo* vsShaderInfo, const void* data)
{
	parseShader(gsShaderInfo, data, Gnmx::kGeometryShader);

	const Gnmx::VsShader*		vsbShader		  = gsShaderInfo->m_gsShader->getCopyShader();
	const uint32_t				vsbOffset         = vsbShader->m_vsStageRegisters.m_spiShaderPgmLoVs;
	const uint32_t				vsbSize           = vsbShader->m_common.m_shaderSize;
	const uint32_t				vsbEmbeddedCBSize = vsbShader->m_common.m_embeddedConstantBufferSizeInDQW*16;

	vsShaderInfo->m_vsShader			= (Gnmx::VsShader*)vsbShader;
	vsShaderInfo->m_gpuShaderCode		= (uint32_t*)((char*)gsShaderInfo->m_gsShader + vsbOffset);
	vsShaderInfo->m_gpuShaderCodeSize = vsbSize + vsbEmbeddedCBSize;
}

void sce::Gnmx::parseCsVsShader(sce::Gnmx::ShaderInfo* csvsShaderInfo, sce::Gnmx::ShaderInfo* csShaderInfo, const void* data)
{
	parseShader(csvsShaderInfo, data, Gnmx::kComputeVertexShader);

	const Gnmx::CsShader*		csbShader		  = csvsShaderInfo->m_csvsShader->getComputeShader();
	const uint32_t				csbOffset         = csbShader->m_csStageRegisters.m_computePgmLo;
	const uint32_t				csbSize           = csbShader->m_common.m_shaderSize;
	const uint32_t				csbEmbeddedCbSize = csbShader->m_common.m_embeddedConstantBufferSizeInDQW*16;

	csShaderInfo->m_csShader			= (Gnmx::CsShader*)csbShader;
	csShaderInfo->m_gpuShaderCode		= (uint32_t*)((char*)csvsShaderInfo->m_csvsShader + csbOffset);
	csShaderInfo->m_gpuShaderCodeSize = csbSize + csbEmbeddedCbSize;
}
