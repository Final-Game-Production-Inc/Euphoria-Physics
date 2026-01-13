/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "grcore/gnmx/fetchshaderhelper.h"

using namespace sce::Gnm;
using namespace sce::Gnmx;

uint32_t sce::Gnmx::computeVsFetchShaderSize(const VsShader *vsb)
{
	Gnm::FetchShaderBuildState fb = {0};

	Gnm::generateVsFetchShaderBuildState(&fb, &vsb->m_vsStageRegisters, vsb->m_numInputSemantics, 0, vsb->getVertexOffsetUserRegister(), vsb->getInstanceOffsetUserRegister());

	return fb.m_fetchShaderBufferSize;
}


void sce::Gnmx::generateVsFetchShader(void *fs, uint32_t *shaderModifier, const VsShader *vsb, const FetchShaderInstancingMode *instancingData)
{
	Gnm::FetchShaderBuildState fb = {0};

	Gnm::generateVsFetchShaderBuildState(&fb, &vsb->m_vsStageRegisters, vsb->m_numInputSemantics, instancingData, vsb->getVertexOffsetUserRegister(), vsb->getInstanceOffsetUserRegister());

	fb.m_numInputSemantics	= vsb->m_numInputSemantics;
	fb.m_inputSemantics		= vsb->getInputSemanticTable();
	fb.m_numInputUsageSlots = vsb->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots	= vsb->getInputUsageSlotTable();

	Gnm::generateFetchShader(fs, &fb);
	*shaderModifier = fb.m_shaderModifier;
}

void sce::Gnmx::generateVsFetchShader(void *fs,
									  uint32_t *shaderModifier, 
									  const VsShader *vsb,
									  const FetchShaderInstancingMode *instancingData,
									  const void *semanticRemapTable, const uint32_t numElementsInRemapTable)
{
	Gnm::FetchShaderBuildState fb = {0};

	Gnm::generateVsFetchShaderBuildState(&fb, &vsb->m_vsStageRegisters, vsb->m_numInputSemantics, instancingData, vsb->getVertexOffsetUserRegister(), vsb->getInstanceOffsetUserRegister());

	fb.m_numInputSemantics		 = vsb->m_numInputSemantics;
	fb.m_inputSemantics			 = vsb->getInputSemanticTable();
	fb.m_numInputUsageSlots      = vsb->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots		 = vsb->getInputUsageSlotTable();
	fb.m_numElementsInRemapTable = numElementsInRemapTable;
	fb.m_semanticsRemapTable	 = static_cast<const uint32_t*>(semanticRemapTable);
	
	Gnm::generateFetchShader(fs, &fb);
	*shaderModifier = fb.m_shaderModifier;
}


uint32_t sce::Gnmx::computeLsFetchShaderSize(const LsShader *lsb)
{
	Gnm::FetchShaderBuildState fb = {0};

	Gnm::generateLsFetchShaderBuildState(&fb, &lsb->m_lsStageRegisters, lsb->m_numInputSemantics, 0, lsb->getVertexOffsetUserRegister(), lsb->getInstanceOffsetUserRegister());

	return fb.m_fetchShaderBufferSize;
}

void sce::Gnmx::generateLsFetchShader(void *fs, uint32_t *shaderModifier, const LsShader *lsb)
{
	generateLsFetchShader(fs,shaderModifier,lsb,0);
}

void sce::Gnmx::generateLsFetchShader(void *fs, uint32_t *shaderModifier, const LsShader *lsb, const Gnm::FetchShaderInstancingMode *instancingData)
{
	Gnm::FetchShaderBuildState fb = {0};

	Gnm::generateLsFetchShaderBuildState(&fb, &lsb->m_lsStageRegisters, lsb->m_numInputSemantics, instancingData, lsb->getVertexOffsetUserRegister(), lsb->getInstanceOffsetUserRegister());

	fb.m_numInputSemantics	= lsb->m_numInputSemantics;
	fb.m_inputSemantics		= lsb->getInputSemanticTable();
	fb.m_numInputUsageSlots = lsb->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots	= lsb->getInputUsageSlotTable();

	Gnm::generateFetchShader(fs, &fb);
	*shaderModifier = fb.m_shaderModifier;
}

void sce::Gnmx::generateLsFetchShader(void *fs,
									  uint32_t *shaderModifier,
									  const LsShader *lsb,
									  const void *semanticRemapTable, const uint32_t numElementsInRemapTable)
{
	Gnm::FetchShaderBuildState fb = {0};

	Gnm::generateLsFetchShaderBuildState(&fb, &lsb->m_lsStageRegisters, lsb->m_numInputSemantics, 0, lsb->getVertexOffsetUserRegister(), lsb->getInstanceOffsetUserRegister());

	fb.m_numInputSemantics		 = lsb->m_numInputSemantics;
	fb.m_inputSemantics			 = lsb->getInputSemanticTable();
	fb.m_numInputUsageSlots      = lsb->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots		 = lsb->getInputUsageSlotTable();
	fb.m_numElementsInRemapTable = numElementsInRemapTable;
	fb.m_semanticsRemapTable	 = static_cast<const uint32_t*>(semanticRemapTable);

	Gnm::generateFetchShader(fs, &fb);
	*shaderModifier = fb.m_shaderModifier;
}

void sce::Gnmx::generateLsFetchShader(void *fs,
									  uint32_t *shaderModifier,
									  const LsShader *lsb,
									  const FetchShaderInstancingMode *instancingData,
									  const void *semanticRemapTable, const uint32_t numElementsInRemapTable)
{
	Gnm::FetchShaderBuildState fb = {0};

	Gnm::generateLsFetchShaderBuildState(&fb, &lsb->m_lsStageRegisters, lsb->m_numInputSemantics, instancingData, lsb->getVertexOffsetUserRegister(), lsb->getInstanceOffsetUserRegister());

	fb.m_numInputSemantics		 = lsb->m_numInputSemantics;
	fb.m_inputSemantics			 = lsb->getInputSemanticTable();
	fb.m_numInputUsageSlots      = lsb->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots		 = lsb->getInputUsageSlotTable();
	fb.m_numElementsInRemapTable = numElementsInRemapTable;
	fb.m_semanticsRemapTable	 = static_cast<const uint32_t*>(semanticRemapTable);
	
	Gnm::generateFetchShader(fs, &fb);
	*shaderModifier = fb.m_shaderModifier;
}


uint32_t sce::Gnmx::computeEsFetchShaderSize(const EsShader *esb)
{
	Gnm::FetchShaderBuildState fb = {0};

	Gnm::generateEsFetchShaderBuildState(&fb, &esb->m_esStageRegisters, esb->m_numInputSemantics, 0, esb->getVertexOffsetUserRegister(), esb->getInstanceOffsetUserRegister());

	return fb.m_fetchShaderBufferSize;
}

void sce::Gnmx::generateEsFetchShader(void *fs, uint32_t *shaderModifier, const EsShader *esb, const FetchShaderInstancingMode *instancingData)
{
	Gnm::FetchShaderBuildState fb = {0};

	Gnm::generateEsFetchShaderBuildState(&fb, &esb->m_esStageRegisters, esb->m_numInputSemantics, instancingData, esb->getVertexOffsetUserRegister(), esb->getInstanceOffsetUserRegister());

	fb.m_numInputSemantics	= esb->m_numInputSemantics;
	fb.m_inputSemantics		= esb->getInputSemanticTable();
	fb.m_numInputUsageSlots = esb->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots	= esb->getInputUsageSlotTable();

	Gnm::generateFetchShader(fs, &fb);
	*shaderModifier = fb.m_shaderModifier;
}

void sce::Gnmx::generateEsFetchShader(void *fs,
									  uint32_t *shaderModifier,
									  const EsShader *esb,
									  const FetchShaderInstancingMode *instancingData,
									  const void *semanticRemapTable, const uint32_t numElementsInRemapTable)
{
	Gnm::FetchShaderBuildState fb = {0};

	Gnm::generateEsFetchShaderBuildState(&fb, &esb->m_esStageRegisters, esb->m_numInputSemantics, instancingData, esb->getVertexOffsetUserRegister(), esb->getInstanceOffsetUserRegister());

	fb.m_numInputSemantics		 = esb->m_numInputSemantics;
	fb.m_inputSemantics			 = esb->getInputSemanticTable();
	fb.m_numInputUsageSlots      = esb->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots		 = esb->getInputUsageSlotTable();
	fb.m_numElementsInRemapTable = numElementsInRemapTable;
	fb.m_semanticsRemapTable	 = static_cast<const uint32_t*>(semanticRemapTable);

	Gnm::generateFetchShader(fs, &fb);
	*shaderModifier = fb.m_shaderModifier;
}


uint32_t sce::Gnmx::computeVsFetchShaderSize(const CsVsShader *csvsb)
{
	Gnm::FetchShaderBuildState fb = {0};
	Gnm::generateVsFetchShaderBuildState(&fb, &csvsb->getVertexShader()->m_vsStageRegisters, csvsb->getVertexShader()->m_numInputSemantics, 0, 0, 0);

	return fb.m_fetchShaderBufferSize;
}

void sce::Gnmx::generateVsFetchShader(void *fs, uint32_t *shaderModifier, const CsVsShader *csvsb, const FetchShaderInstancingMode *instancingData)
{
	Gnm::FetchShaderBuildState fb = {0};
	Gnm::generateVsFetchShaderBuildState(&fb, &csvsb->getVertexShader()->m_vsStageRegisters, csvsb->getVertexShader()->m_numInputSemantics, instancingData, 0, 0);

	fb.m_numInputSemantics	= csvsb->getVertexShader()->m_numInputSemantics;
	fb.m_inputSemantics		= csvsb->getVertexShader()->getInputSemanticTable();
	fb.m_numInputUsageSlots = csvsb->getVertexShader()->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots	= csvsb->getVertexShader()->getInputUsageSlotTable();


	Gnm::generateFetchShader(fs, &fb);
	*shaderModifier = fb.m_shaderModifier;
}

void sce::Gnmx::generateVsFetchShader(void *fs,
	uint32_t *shaderModifier, 
	const CsVsShader *csvsb,
	const FetchShaderInstancingMode *instancingData,
	const void *semanticRemapTable, const uint32_t numElementsInRemapTable)
{
	Gnm::FetchShaderBuildState fb = {0};
	Gnm::generateVsFetchShaderBuildState(&fb, &csvsb->getVertexShader()->m_vsStageRegisters, csvsb->getVertexShader()->m_numInputSemantics, instancingData, 0, 0);

	fb.m_numInputSemantics		 = csvsb->getVertexShader()->m_numInputSemantics;
	fb.m_inputSemantics			 = csvsb->getVertexShader()->getInputSemanticTable();
	fb.m_numInputUsageSlots      = csvsb->getVertexShader()->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots		 = csvsb->getVertexShader()->getInputUsageSlotTable();
	fb.m_numElementsInRemapTable = numElementsInRemapTable;
	fb.m_semanticsRemapTable	 = static_cast<const uint32_t*>(semanticRemapTable);

	Gnm::generateFetchShader(fs, &fb);
	*shaderModifier = fb.m_shaderModifier;
}

uint32_t sce::Gnmx::computeCsFetchShaderSize(const CsVsShader *csvsb)
{
	Gnm::FetchShaderBuildState fb = {0};
	Gnm::generateCsFetchShaderBuildState(&fb, &csvsb->getComputeShader()->m_csStageRegisters, csvsb->getVertexShader()->m_gsModeOrNumInputSemanticsCs, 0, 0, 0);

	return fb.m_fetchShaderBufferSize;
}

void sce::Gnmx::generateCsFetchShader(void *fs, uint32_t *shaderModifier, const CsVsShader *csvsb, const FetchShaderInstancingMode *instancingData)
{
	Gnm::FetchShaderBuildState fb = {0};
	Gnm::generateCsFetchShaderBuildState(&fb, &csvsb->getComputeShader()->m_csStageRegisters, csvsb->getVertexShader()->m_gsModeOrNumInputSemanticsCs, instancingData, 0, 0);

	fb.m_numInputSemantics	= csvsb->getVertexShader()->m_gsModeOrNumInputSemanticsCs;
	fb.m_inputSemantics		= csvsb->getVertexShader()->getInputSemanticTable();
	fb.m_numInputUsageSlots = csvsb->getComputeShader()->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots	= csvsb->getComputeShader()->getInputUsageSlotTable();

	Gnm::generateFetchShader(fs, &fb);
	*shaderModifier = fb.m_shaderModifier;
}

void sce::Gnmx::generateCsFetchShader(void *fs,
	uint32_t *shaderModifier, 
	const CsVsShader *csvsb,
	const FetchShaderInstancingMode *instancingData,
	const void *semanticRemapTable, const uint32_t numElementsInRemapTable)
{
	Gnm::FetchShaderBuildState fb = {0};
	Gnm::generateCsFetchShaderBuildState(&fb, &csvsb->getComputeShader()->m_csStageRegisters, csvsb->getVertexShader()->m_gsModeOrNumInputSemanticsCs, instancingData, 0 ,0);

	fb.m_numInputSemantics	= csvsb->getVertexShader()->m_gsModeOrNumInputSemanticsCs;
	fb.m_inputSemantics		= csvsb->getVertexShader()->getInputSemanticTable();
	fb.m_numInputUsageSlots = csvsb->getComputeShader()->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots	= csvsb->getComputeShader()->getInputUsageSlotTable();
	fb.m_numElementsInRemapTable = numElementsInRemapTable;
	fb.m_semanticsRemapTable	 = static_cast<const uint32_t*>(semanticRemapTable);

	Gnm::generateFetchShader(fs, &fb);
	*shaderModifier = fb.m_shaderModifier;
}
