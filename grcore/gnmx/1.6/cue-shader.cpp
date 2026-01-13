/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.071
* Copyright (C) 2014 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifdef __ORBIS__
#include <x86intrin.h>
#endif // __ORBIS__
#include <gnm.h>

#include "grcore/gnmx/common.h"

#ifdef CUE_V2
#include "grcore/gnmx/cue.h"
#include "grcore/gnmx/cue-helper.h"

using namespace sce::Gnm;
using namespace sce::Gnmx;



void ConstantUpdateEngine::setVsShader(const Gnmx::VsShader *vsb, uint32_t shaderModifier, void *fetchShaderAddr)
{
	StageInfo *vsStage = m_stageInfo+Gnm::kShaderStageVs;

	m_currentVSB  = vsb;
	m_dirtyVsOrPs = true;
	m_psInputs	  = NULL;	// new VS or PS -> new PS inputs table (unless the user passes one in with setPsInputTable)

	if ( !vsb )
	{
		vsStage->shaderBaseAddr256   = 0;
		vsStage->inputUsageTable	 = 0;
		vsStage->inputUsageTableSize = 0;
		return;
	}

	vsStage->shaderBaseAddr256	   = vsb->m_vsStageRegisters.m_spiShaderPgmLoVs;
	vsStage->shaderCodeSizeInBytes = vsb->m_common.m_shaderSize;

	m_dcb->setVsShader(&vsb->m_vsStageRegisters, shaderModifier);
	if ( fetchShaderAddr )
	{
		m_dcb->setPointerInUserData(Gnm::kShaderStageVs, 0, fetchShaderAddr);
		// TODO: Need a warning, not an assert.
		//SCE_GNM_VALIDATE(vsb->m_numInputSemantics &&
		//				 vsb->getInputUsageSlotTable()[0].m_usageType == Gnm::kShaderInputUsageSubPtrFetchShader,
		//				 "VsShader vsb [0x%p] doesn't expect a fetch shader, but got one passed in fetchShaderAddr==[0x%p]",
		//				 vsb, fetchShaderAddr);
	}
	else
	{
		SCE_GNM_VALIDATE(!vsb->m_numInputSemantics ||
						 vsb->getInputUsageSlotTable()[0].m_usageType != Gnm::kShaderInputUsageSubPtrFetchShader,
						 "VsShader vsb [0x%p] expects a fetch shader, but fetchShaderAddr==0", vsb);
	}

	vsStage->inputUsageTable	 = vsb->getInputUsageSlotTable();
	vsStage->inputUsageTableSize = vsb->m_common.m_numInputUsageSlots;

	m_shaderUsesSrt &= ~(1 << kShaderStageVs);
	m_shaderUsesSrt |= vsb->m_common.m_shaderIsUsingSrt << kShaderStageVs;

	// EUD is automatically dirtied
	m_shaderDirtyEud |= (1 << kShaderStageVs);
	vsStage->eudSizeInDWord = ConstantUpdateEngineHelper::calculateEudSizeInDWord(vsStage->inputUsageTable, vsStage->inputUsageTableSize);

	// Handle embedded constant buffer
	if ( vsb->m_common.m_embeddedConstantBufferSizeInDQW )
	{
		// Set up the internal constants:
		Gnm::Buffer vsEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)vsb->m_vsStageRegisters.m_spiShaderPgmHiVs << 32) + vsb->m_vsStageRegisters.m_spiShaderPgmLoVs );
		vsEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + vsb->m_common.m_shaderSize ),
												   vsb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageVs, 15, 1, &vsEmbeddedConstBuffer);
	}
	else
	{
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageVs, 15, 1, NULL);
	}
}


void ConstantUpdateEngine::setPsShader(const Gnmx::PsShader *psb)
{
	StageInfo *psStage = m_stageInfo+Gnm::kShaderStagePs;

	m_currentPSB  = psb;	// note: PS is allowed to be NULL, unlike most stages
	m_dirtyVsOrPs = true;
	m_psInputs	  = NULL;	// new VS or PS -> new PS inputs table (unless the user passes one in with setPsInputTable)

	if ( !psb )
	{
		m_dcb->setPsShader(0);
		psStage->shaderBaseAddr256   = 0;
		psStage->inputUsageTable	 = 0;
		psStage->inputUsageTableSize = 0;
		return;
	}

	psStage->shaderBaseAddr256	   = psb->m_psStageRegisters.m_spiShaderPgmLoPs;
	psStage->shaderCodeSizeInBytes = psb->m_common.m_shaderSize;

	m_dcb->setPsShader(&psb->m_psStageRegisters);
	psStage->inputUsageTable	 = psb->getInputUsageSlotTable();
	psStage->inputUsageTableSize = psb->m_common.m_numInputUsageSlots;

	m_shaderUsesSrt &= ~(1 << kShaderStagePs);
	m_shaderUsesSrt |= psb->m_common.m_shaderIsUsingSrt << kShaderStagePs;

	// EUD is automatically dirtied
	m_shaderDirtyEud |= (1 << kShaderStagePs);
	psStage->eudSizeInDWord = ConstantUpdateEngineHelper::calculateEudSizeInDWord(psStage->inputUsageTable, psStage->inputUsageTableSize);

	if ( psb->m_common.m_embeddedConstantBufferSizeInDQW )
	{
		// Setup the internal constants
		Gnm::Buffer psEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)psb->m_psStageRegisters.m_spiShaderPgmHiPs << 32) + psb->m_psStageRegisters.m_spiShaderPgmLoPs );
		psEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + psb->m_common.m_shaderSize ),
												   psb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStagePs, 15, 1, &psEmbeddedConstBuffer);
	}
	else
	{
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStagePs, 15, 1, NULL);
	}
}


void ConstantUpdateEngine::setCsShader(const Gnmx::CsShader *csb)
{
	StageInfo *csStage = m_stageInfo+Gnm::kShaderStageCs;

	if ( !csb )
	{
		csStage->shaderBaseAddr256   = 0;
		csStage->inputUsageTable	 = 0;
		csStage->inputUsageTableSize = 0;
		return;
	}

	csStage->shaderBaseAddr256	   = csb->m_csStageRegisters.m_computePgmLo;
	csStage->shaderCodeSizeInBytes = csb->m_common.m_shaderSize;

	m_dcb->setCsShader(&csb->m_csStageRegisters);
	csStage->inputUsageTable	 = csb->getInputUsageSlotTable();
	csStage->inputUsageTableSize = csb->m_common.m_numInputUsageSlots;

	m_shaderUsesSrt &= ~(1 << kShaderStageCs);
	m_shaderUsesSrt |= csb->m_common.m_shaderIsUsingSrt << kShaderStageCs;

	// EUD is automatically dirtied
	m_shaderDirtyEud |= (1 << kShaderStageCs);
	csStage->eudSizeInDWord = ConstantUpdateEngineHelper::calculateEudSizeInDWord(csStage->inputUsageTable, csStage->inputUsageTableSize);

	if (csb->m_common.m_embeddedConstantBufferSizeInDQW)
	{
		// Setup the internal constants
		Gnm::Buffer csEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)csb->m_csStageRegisters.m_computePgmHi << 32) + csb->m_csStageRegisters.m_computePgmLo );
		csEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + csb->m_common.m_shaderSize ),
												   csb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageCs, 15, 1, &csEmbeddedConstBuffer);
	}
	else
	{
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageCs, 15, 1, NULL);
	}
}


void ConstantUpdateEngine::setLsShader(const Gnmx::LsShader *lsb, uint32_t shaderModifier, void *fetchShaderAddr)
{
	StageInfo *lsStage = m_stageInfo+Gnm::kShaderStageLs;

	m_currentLSB  = lsb;

	if ( !lsb )
	{
		lsStage->shaderBaseAddr256   = 0;
		lsStage->inputUsageTable	 = 0;
		lsStage->inputUsageTableSize = 0;
		return;
	}

	lsStage->shaderBaseAddr256	   = lsb->m_lsStageRegisters.m_spiShaderPgmLoLs;
	lsStage->shaderCodeSizeInBytes = lsb->m_common.m_shaderSize;

	m_dcb->setLsShader(&lsb->m_lsStageRegisters, shaderModifier);
	if ( fetchShaderAddr )
	{
		m_dcb->setPointerInUserData(Gnm::kShaderStageLs, 0, fetchShaderAddr);
		// TODO: Need a warning, not an assert.
		//SCE_GNM_VALIDATE(lsb->m_numInputSemantics &&
		//				 lsb->getInputUsageSlotTable()[0].m_usageType == Gnm::kShaderInputUsageSubPtrFetchShader,
		//				 "LsShader lsb [0x%p] doesn't expect a fetch shader, but got one passed in fetchShaderAddr==[0x%p]",
		//				 lsb, fetchShaderAddr);
	}
	else
	{
		SCE_GNM_VALIDATE(!lsb->m_numInputSemantics ||
						 lsb->getInputUsageSlotTable()[0].m_usageType != Gnm::kShaderInputUsageSubPtrFetchShader,
						 "LsShader lsb [0x%p] expects a fetch shader, but fetchShaderAddr==0", lsb);
	}

	lsStage->inputUsageTable	 = lsb->getInputUsageSlotTable();
	lsStage->inputUsageTableSize = lsb->m_common.m_numInputUsageSlots;

	m_shaderUsesSrt &= ~(1 << kShaderStageLs);
	m_shaderUsesSrt |= lsb->m_common.m_shaderIsUsingSrt << kShaderStageLs;

	// EUD is automatically dirtied
	m_shaderDirtyEud |= (1 << kShaderStageLs);
	lsStage->eudSizeInDWord = ConstantUpdateEngineHelper::calculateEudSizeInDWord(lsStage->inputUsageTable, lsStage->inputUsageTableSize);

	// Handle embedded constant buffer
	if ( lsb->m_common.m_embeddedConstantBufferSizeInDQW )
	{
		// Set up the internal constants:
		Gnm::Buffer lsEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)lsb->m_lsStageRegisters.m_spiShaderPgmHiLs << 32) + lsb->m_lsStageRegisters.m_spiShaderPgmLoLs );
		lsEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + lsb->m_common.m_shaderSize ),
												   lsb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageLs, 15, 1, &lsEmbeddedConstBuffer);
	}
	else
	{
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageLs, 15, 1, NULL);
	}
}


void ConstantUpdateEngine::setHsShader(const Gnmx::HsShader *hsb, const Gnm::TessellationRegisters *tessRegs)
{
	StageInfo *hsStage = m_stageInfo+Gnm::kShaderStageHs;

	if ( !hsb )
	{
		hsStage->shaderBaseAddr256   = 0;
		hsStage->inputUsageTable	 = 0;
		hsStage->inputUsageTableSize = 0;
		return;
	}

	hsStage->shaderBaseAddr256	   = hsb->m_hsStageRegisters.m_spiShaderPgmLoHs;
	hsStage->shaderCodeSizeInBytes = hsb->m_common.m_shaderSize;

	m_dcb->setHsShader(&hsb->m_hsStageRegisters, tessRegs);
	hsStage->inputUsageTable	 = hsb->getInputUsageSlotTable();
	hsStage->inputUsageTableSize = hsb->m_common.m_numInputUsageSlots;

	m_shaderUsesSrt &= ~(1 << kShaderStageHs);
	m_shaderUsesSrt |= hsb->m_common.m_shaderIsUsingSrt << kShaderStageHs;

	// EUD is automatically dirtied
	m_shaderDirtyEud |= (1 << kShaderStageHs);
	hsStage->eudSizeInDWord = ConstantUpdateEngineHelper::calculateEudSizeInDWord(hsStage->inputUsageTable, hsStage->inputUsageTableSize);

	if (hsb->m_common.m_embeddedConstantBufferSizeInDQW)
	{
		// Setup the internal constants
		Gnm::Buffer hsEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)hsb->m_hsStageRegisters.m_spiShaderPgmHiHs << 32) + hsb->m_hsStageRegisters.m_spiShaderPgmLoHs );
		hsEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + hsb->m_common.m_shaderSize ),
												   hsb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageHs, 15, 1, &hsEmbeddedConstBuffer);
	}
	else
	{
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageHs, 15, 1, NULL);
	}
}


void ConstantUpdateEngine::setEsShader(const Gnmx::EsShader *esb, uint32_t shaderModifier, void *fetchShaderAddr)
{
	StageInfo *esStage = m_stageInfo+Gnm::kShaderStageEs;

	m_currentESB  = esb;

	if ( !esb )
	{
		esStage->shaderBaseAddr256   = 0;
		esStage->inputUsageTable	 = 0;
		esStage->inputUsageTableSize = 0;
		return;
	}

	esStage->shaderBaseAddr256	   = esb->m_esStageRegisters.m_spiShaderPgmLoEs;
	esStage->shaderCodeSizeInBytes = esb->m_common.m_shaderSize;

	m_dcb->setEsShader(&esb->m_esStageRegisters, shaderModifier);
	if ( fetchShaderAddr )
	{
		m_dcb->setPointerInUserData(Gnm::kShaderStageEs, 0, fetchShaderAddr);
		// TODO: Need a warning, not an assert.
		//SCE_GNM_VALIDATE(esb->m_numInputSemantics &&
		//				 esb->getInputUsageSlotTable()[0].m_usageType == Gnm::kShaderInputUsageSubPtrFetchShader,
		//				 "EsShader esb [0x%p] doesn't expect a fetch shader, but got one passed in fetchShaderAddr==[0x%p]",
		//				 esb, fetchShaderAddr);
	}
	else
	{
		SCE_GNM_VALIDATE(!esb->m_numInputSemantics ||
						 esb->getInputUsageSlotTable()[0].m_usageType != Gnm::kShaderInputUsageSubPtrFetchShader,
						 "EsShader esb [0x%p] expects a fetch shader, but fetchShaderAddr==0", esb);
	}

	esStage->inputUsageTable	 = esb->getInputUsageSlotTable();
	esStage->inputUsageTableSize = esb->m_common.m_numInputUsageSlots;

	m_shaderUsesSrt &= ~(1 << kShaderStageEs);
	m_shaderUsesSrt |= esb->m_common.m_shaderIsUsingSrt << kShaderStageEs;

	// EUD is automatically dirtied
	m_shaderDirtyEud |= (1 << kShaderStageEs);
	esStage->eudSizeInDWord = ConstantUpdateEngineHelper::calculateEudSizeInDWord(esStage->inputUsageTable, esStage->inputUsageTableSize);

	// Handle embedded constant buffer
	if ( esb->m_common.m_embeddedConstantBufferSizeInDQW )
	{
		// Set up the internal constants:
		Gnm::Buffer esEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)esb->m_esStageRegisters.m_spiShaderPgmHiEs << 32) + esb->m_esStageRegisters.m_spiShaderPgmLoEs );
		esEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + esb->m_common.m_shaderSize ),
												   esb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageEs, 15, 1, &esEmbeddedConstBuffer);
	}
	else
	{
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageEs, 15, 1, NULL);
	}
}


void ConstantUpdateEngine::setGsVsShaders(const GsShader *gsb)
{
	StageInfo *gsStage = m_stageInfo+Gnm::kShaderStageGs;

	if ( !gsb )
	{
		gsStage->shaderBaseAddr256   = 0;
		gsStage->inputUsageTable	 = 0;
		gsStage->inputUsageTableSize = 0;
		return;
	}

	gsStage->shaderBaseAddr256	   = gsb->m_gsStageRegisters.m_spiShaderPgmLoGs;
	gsStage->shaderCodeSizeInBytes = gsb->m_common.m_shaderSize;

	m_dcb->setGsShader(&gsb->m_gsStageRegisters);
	gsStage->inputUsageTable	 = gsb->getInputUsageSlotTable();
	gsStage->inputUsageTableSize = gsb->m_common.m_numInputUsageSlots;

	m_shaderUsesSrt &= ~(1 << kShaderStageGs);
	m_shaderUsesSrt |= gsb->m_common.m_shaderIsUsingSrt << kShaderStageGs;

	// EUD is automatically dirtied
	m_shaderDirtyEud |= (1 << kShaderStageGs);
	gsStage->eudSizeInDWord = ConstantUpdateEngineHelper::calculateEudSizeInDWord(gsStage->inputUsageTable, gsStage->inputUsageTableSize);

	if ( gsb->m_common.m_embeddedConstantBufferSizeInDQW )
	{
		// Setup the internal constants
		Gnm::Buffer gsEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)gsb->m_gsStageRegisters.m_spiShaderPgmHiGs << 32) + gsb->m_gsStageRegisters.m_spiShaderPgmLoGs );
		gsEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + gsb->m_common.m_shaderSize ),
												   gsb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageGs, 15, 1, &gsEmbeddedConstBuffer);
	}
	else
	{
		// TODO: Add a special case function for this buffer
		setConstantBuffers(kShaderStageGs, 15, 1, NULL);
	}

	setVsShader(gsb->getCopyShader(), 0, (void*)0); // Setting the GS also set the VS copy shader
}

void ConstantUpdateEngine::setCsVsShaders(const Gnmx::CsVsShader *csvsb, uint32_t shaderModifierVs, void *fetchShaderAddrVs, uint32_t shaderModifierCs, void *fetchShaderAddrCs)
{
	if ( !csvsb ) return;
	SCE_GNM_ERROR("Not yet implemented in CUE2!");
	SCE_GNM_STATIC_ASSERT(!CUE2_SHOW_UNIMPLEMENTED);
}


void ConstantUpdateEngine::setAsynchronousComputeShader(const Gnmx::CsShader *csb, uint32_t shaderModifierCs, void *fetchShaderAddrCs)
{
	if ( !csb ) return;
	SCE_GNM_ERROR("Not yet implemented in CUE2!");
	SCE_GNM_STATIC_ASSERT(!CUE2_SHOW_UNIMPLEMENTED);
}


//-----------------------------------------------------------------------------


void ConstantUpdateEngine::setVertexAndInstanceOffset(uint32_t vertexOffset, uint32_t instanceOffset)
{
	ShaderStage fetchShaderStage	   = Gnm::kShaderStageCount;		// initialize with an invalid value
	uint32_t	vertexOffsetUserSlot   = 0;
	uint32_t	instanceOffsetUserSlot = 0;

	// find where the fetch shader is
	switch(m_activeShaderStages)
	{
		case Gnm::kActiveShaderStagesVsPs:
			SCE_GNM_ASSERT(m_currentVSB);
			fetchShaderStage	   = Gnm::kShaderStageVs;
			vertexOffsetUserSlot   = m_currentVSB->getVertexOffsetUserRegister();
			instanceOffsetUserSlot = m_currentVSB->getInstanceOffsetUserRegister();
			break;
		case Gnm::kActiveShaderStagesEsGsVsPs:
			SCE_GNM_ASSERT(m_currentESB);
			fetchShaderStage	   = Gnm::kShaderStageEs;
			vertexOffsetUserSlot   = m_currentESB->getVertexOffsetUserRegister();
			instanceOffsetUserSlot = m_currentESB->getInstanceOffsetUserRegister();
			break;
		case Gnm::kActiveShaderStagesLsHsEsGsVsPs:
		case Gnm::kActiveShaderStagesLsHsVsPs:
			SCE_GNM_ASSERT(m_currentLSB);
			fetchShaderStage	   = Gnm::kShaderStageLs;
			vertexOffsetUserSlot   = m_currentLSB->getVertexOffsetUserRegister();
			instanceOffsetUserSlot = m_currentLSB->getInstanceOffsetUserRegister();
			break;
		case kActiveShaderStagesDispatchDrawVsPs:
			SCE_GNM_ERROR("Current shader configuration does not support vertex/instance offset");
	}
	if (fetchShaderStage != kShaderStageCount)
	{
		if( vertexOffsetUserSlot )
			m_dcb->setUserData(fetchShaderStage,vertexOffsetUserSlot,vertexOffset);
		if( instanceOffsetUserSlot )
			m_dcb->setUserData(fetchShaderStage,instanceOffsetUserSlot, instanceOffset);
	}
}


bool ConstantUpdateEngine::isVertexOrInstanceOffsetEnabled() const
{
	uint32_t	vertexOffsetUserSlot   = 0;
	uint32_t	instanceOffsetUserSlot = 0;

	// find where the fetch shader is
	switch(m_activeShaderStages)
	{
	 case Gnm::kActiveShaderStagesVsPs:
		SCE_GNM_ASSERT(m_currentVSB);
		vertexOffsetUserSlot   = m_currentVSB->getVertexOffsetUserRegister();
		instanceOffsetUserSlot = m_currentVSB->getInstanceOffsetUserRegister();
		break;
	 case Gnm::kActiveShaderStagesEsGsVsPs:
		SCE_GNM_ASSERT(m_currentESB);
		vertexOffsetUserSlot   = m_currentESB->getVertexOffsetUserRegister();
		instanceOffsetUserSlot = m_currentESB->getInstanceOffsetUserRegister();
		break;
	 case Gnm::kActiveShaderStagesLsHsEsGsVsPs:
	 case Gnm::kActiveShaderStagesLsHsVsPs:
		SCE_GNM_ASSERT(m_currentLSB);
		vertexOffsetUserSlot   = m_currentLSB->getVertexOffsetUserRegister();
		instanceOffsetUserSlot = m_currentLSB->getInstanceOffsetUserRegister();
		break;
	 case kActiveShaderStagesDispatchDrawVsPs:
		return false;
	}
	return vertexOffsetUserSlot || instanceOffsetUserSlot;
}




#endif // CUE_V2
