/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(_SCE_GNMX_CUE_TO_LCUE_H)
#define _SCE_GNMX_CUE_TO_LCUE_H

/**
 * Constant-Update-Engine (CUE) to Lightweight-Constant-Update-Engine (LCUE) helper.
 *
 * Provides a new Gnmx::GfxContext that maps all of its methods to the Gnmx::LCUE::GraphicsContext. This should serve as
 * a quick first step to integrate and test your code with the LCUE.
 *
 * IMPORTANT NOTES / MUST-READ:
 * -------------------------------------------------------------------------------------------------------------------------
 * This header is NOT intended to be used in a shipping product, as performance will be very low
 * due to the need to generate structures used by the LCUE on-the-fly.
 * 
 * This header redefines Gnmx::GfxContext as Gnmx::GnmxGfxContext, and than declares its own class Gnmx::GfxContext. Thus,
 * depending on where you include this header or how you link your code, you are likely to find issues. To prevent issues, 
 * we recommend you:
 *	(a) Make sure you include <gnmx/cue_to_lcue.h> as the first line in any source file that uses Gnmx::GfxContext. An 
 *		easy way to achieve this is by including it as the first line inside <gnmx/gnmx.h>. Note that this has the drawback
 *		of affecting any other code that also includes <gnmx/gnmx.h>.
 *  (b) Make sure you add the "target\src\gnmx\lcue.cpp" source to your solution, and possibly 
 *		"target\src\gnmx\lcue_validation.cpp".
 *  (c) Make sure you rebuild your solution and all the libraries it uses. We have seen some cases in which the compiler
 *		 would incorrectly link code with previously built snippets of the original Gnmx::GfxContext.
 */

#define GfxContext GnmxGfxContext

#include "grcore/gnmx.h"
// This is in case we are including it from inside the gnmx.h itself
#include "grcore/gnmx/computecontext.h"
#include "grcore/gnmx/constantupdateengine.h"
#include "grcore/gnmx/fetchshaderhelper.h"
#include "grcore/gnmx/gfxcontext.h"
#include "grcore/gnmx/helpers.h"
#include "grcore/gnmx/shaderbinary.h"
#include "grcore/gnmx/computequeue.h"

#undef GfxContext

#include "grcore/gnmx/lwgfxconstantupdateengine.h"
#include "grcore/gnmx/lwcomputeconstantupdateengine.h"

namespace sce
{
	namespace Gnmx
	{
	
		class SCE_GNMX_EXPORT GfxContext : public LightweightConstantUpdateEngine::GraphicsContext
		{
		public:
			// Those methods are silently ignored as this doesn't apply to the LCUE
			SCE_GNM_FORCE_INLINE void setActiveResourceSlotCount(Gnm::ShaderStage stage, uint32_t count) { }
			SCE_GNM_FORCE_INLINE void setActiveRwResourceSlotCount(Gnm::ShaderStage stage, uint32_t count) { }
			SCE_GNM_FORCE_INLINE void setActiveSamplerSlotCount(Gnm::ShaderStage stage, uint32_t count) { }
			SCE_GNM_FORCE_INLINE void setActiveVertexBufferSlotCount(Gnm::ShaderStage stage, uint32_t count) { }
			SCE_GNM_FORCE_INLINE void setActiveConstantBufferSlotCount(Gnm::ShaderStage stage, uint32_t count) { }
			SCE_GNM_FORCE_INLINE void setActiveStreamoutBufferSlotCount(uint32_t count) { }

			// Dispatch Draw is not supported yet
			// ----------------------------------------------------------------------------------------------------------------------------------

			SCE_GNM_LCUE_NOT_SUPPORTED void initDispatchDrawCommandBuffer(void *acbBuffer, uint32_t acbSizeInBytes)
			{
				// Not implemented yet
			}


			SCE_GNM_LCUE_NOT_SUPPORTED void setDispatchDrawComputeQueue(ComputeQueue *pQueue)
			{
				// Not implemented yet
			}


			SCE_GNM_LCUE_NOT_SUPPORTED void setupDispatchDrawRingBuffers(void *pIndexRingBuffer, uint32_t sizeofIndexRingBufferAlign256B)
			{
				// Not implemented yet
			}


			SCE_GNM_LCUE_NOT_SUPPORTED void setupDispatchDrawScreenViewport(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
			{
				// Not implemented yet
			}


			SCE_GNM_LCUE_NOT_SUPPORTED void setupDispatchDrawClipCullSettings(Gnm::PrimitiveSetup primitiveSetup, Gnm::ClipControl clipControl)
			{
				// Not implemented yet
			}


			SCE_GNM_LCUE_NOT_SUPPORTED void setupDispatchDrawClipCullSettings(uint32_t dispatchDrawClipCullFlags)
			{
				// Not implemented yet
			}


			SCE_GNM_LCUE_NOT_SUPPORTED void dispatchDraw(Gnm::Buffer bufferInputData, uint32_t numBlocksTotal, uint32_t numPrimsPerVgt)
			{
				// Not implemented yet
			}
		

			SCE_GNM_LCUE_NOT_SUPPORTED void setCsVsShaders(const Gnmx::CsVsShader *csvsb, uint32_t shaderModifierVs, void *fetchShaderAddrVs, uint32_t shaderModifierCs, void *fetchShaderAddrCs)
			{
				// Not implemented yet
			}


			SCE_GNM_LCUE_NOT_SUPPORTED void setAsynchronousComputeShader(const Gnmx::CsShader *csb, uint32_t shaderModifierCs, void *fetchShaderAddrCs)
			{
				// Not implemented yet
			}

			// ----------------------------------------------------------------------------------------------------------------------------------

			SCE_GNM_LCUE_NOT_SUPPORTED void setBoolConstants(Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const uint32_t *bits)
			{
				// Not implemented yet
			}


			SCE_GNM_LCUE_NOT_SUPPORTED void setFloatConstants(Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const float *floats)
			{
				// Not implemented yet
			}

			// ----------------------------------------------------------------------------------------------------------------------------------
		
			SCE_GNM_FORCE_INLINE void init(void *cueCpRamShadowBuffer, void *cueHeapAddr, uint32_t numRingEntries,
						void *dcbBuffer, uint32_t dcbSizeInBytes, void *ccbBuffer, uint32_t ccbSizeInBytes)
			{
				uint32_t heapSizeInBytes = Gnmx::ConstantUpdateEngine::computeHeapSize(numRingEntries);
				GraphicsContext::init((uint32_t*)dcbBuffer, dcbSizeInBytes/4, (uint32_t*)cueHeapAddr, heapSizeInBytes/4, NULL, NULL, NULL);
			}


			SCE_GNM_FORCE_INLINE void setGlobalResourceTableAddr(void *addr)
			{
				GraphicsContext::setGlobalInternalResourceTable(addr);
			}


			SCE_GNM_FORCE_INLINE void setGlobalDescriptor(Gnm::ShaderGlobalResourceType resType, const Gnm::Buffer *res)
			{
				GraphicsContext::setGlobalInternalResource(resType, res);
			}


			SCE_GNM_FORCE_INLINE void setVsShader(const Gnmx::VsShader *vsb, uint32_t shaderModifier, void *fetchShaderAddr)
			{
				LightweightConstantUpdateEngine::generateInputResourceOffsetTable(&m_boundShaderResourceOffsets[Gnm::kShaderStageVs], Gnm::kShaderStageVs, vsb);
				GraphicsContext::setVsShader(vsb, shaderModifier, fetchShaderAddr, &m_boundShaderResourceOffsets[Gnm::kShaderStageVs]);
			}


			SCE_GNM_FORCE_INLINE void setPsShader(const Gnmx::PsShader *psb)
			{
				if (psb != NULL)
				{
					LightweightConstantUpdateEngine::generateInputResourceOffsetTable(&m_boundShaderResourceOffsets[Gnm::kShaderStagePs], Gnm::kShaderStagePs, psb);
					GraphicsContext::setPsShader(psb, &m_boundShaderResourceOffsets[Gnm::kShaderStagePs]);
				}
				else
				{
					memset(&m_boundShaderResourceOffsets[Gnm::kShaderStagePs], 0, sizeof(Gnmx::LightweightConstantUpdateEngine::InputResourceOffsets));
					GraphicsContext::setPsShader(psb, NULL);
				}
			}


			SCE_GNM_FORCE_INLINE void setCsShader(const Gnmx::CsShader *csb)
			{
				LightweightConstantUpdateEngine::generateInputResourceOffsetTable(&m_boundShaderResourceOffsets[Gnm::kShaderStageCs], Gnm::kShaderStageCs, csb);
				GraphicsContext::setCsShader(csb, &m_boundShaderResourceOffsets[Gnm::kShaderStageCs]);
			}


			SCE_GNM_FORCE_INLINE void setEsShader(const Gnmx::EsShader *esb, uint32_t shaderModifier, void *fetchShaderAddr)
			{
				LightweightConstantUpdateEngine::generateInputResourceOffsetTable(&m_boundShaderResourceOffsets[Gnm::kShaderStageEs], Gnm::kShaderStageEs, esb);
				GraphicsContext::setEsShader(esb, shaderModifier, fetchShaderAddr, &m_boundShaderResourceOffsets[Gnm::kShaderStageEs]);
			}


			SCE_GNM_FORCE_INLINE void setGsVsShaders(const Gnmx::GsShader *gsb)
			{
				LightweightConstantUpdateEngine::generateInputResourceOffsetTable(&m_boundShaderResourceOffsets[Gnm::kShaderStageGs], Gnm::kShaderStageGs, gsb);
				GraphicsContext::setGsVsShaders(gsb, &m_boundShaderResourceOffsets[Gnm::kShaderStageGs]);
			}


			SCE_GNM_API_REMOVED("GsMode is handled automatically by the LightweightConstantUpdateEngine, see GraphicsConstantUpdateEngine::setActiveShaderStages() for more details.")
			void setGsModeOff() 
			{
				// Nothing to be done here the LCUE GraphicsContext handles turning GsMode on/off for you
			}


			SCE_GNM_FORCE_INLINE void setLsHsShaders(Gnmx::LsShader *lsb, uint32_t shaderModifier, void *fetchShaderAddr, const Gnmx::HsShader *hsb, uint32_t numPatches)
			{
				SCE_GNM_VALIDATE((lsb && hsb) || (!lsb && !hsb), "lsb (0x%010llX) and hsb (0x%010llX) must either both be NULL, or both be non-NULL.", lsb, hsb);
				if (lsb && hsb)
				{
					// Update HW registers for local shader
					lsb->m_lsStageRegisters.updateLdsSize(&hsb->m_hullStateConstants, 
														  lsb->m_lsStride, numPatches);
				}
				
				LightweightConstantUpdateEngine::generateInputResourceOffsetTable(&m_boundShaderResourceOffsets[Gnm::kShaderStageLs], Gnm::kShaderStageLs, lsb);
				GraphicsContext::setLsShader(lsb, shaderModifier, fetchShaderAddr, &m_boundShaderResourceOffsets[Gnm::kShaderStageLs]);

				LightweightConstantUpdateEngine::generateInputResourceOffsetTable(&m_boundShaderResourceOffsets[Gnm::kShaderStageHs], Gnm::kShaderStageHs, hsb);
				GraphicsContext::setHsShader(hsb, &m_boundShaderResourceOffsets[Gnm::kShaderStageHs], numPatches);
			}


			SCE_GNM_FORCE_INLINE void setTessellationDataConstantBuffer(void *tcbAddr, Gnm::ShaderStage domainStage)
			{
				Gnm::Buffer tessellationCb;
				tessellationCb.initAsConstantBuffer(tcbAddr, sizeof(Gnm::TessellationDataConstantBuffer));
				setConstantBuffers(Gnm::kShaderStageHs, LightweightConstantUpdateEngine::kConstantBufferInternalApiSlotForTessellation, 1, &tessellationCb);
				setConstantBuffers(domainStage, LightweightConstantUpdateEngine::kConstantBufferInternalApiSlotForTessellation, 1, &tessellationCb);
			}


			SCE_GNM_FORCE_INLINE void setTessellationFactorBuffer(void *tessFactorMemoryBaseAddr)
			{
				Gnm::Buffer tessFactorBuffer;
				tessFactorBuffer.initAsTessellationFactorBuffer(tessFactorMemoryBaseAddr, Gnm::kTfRingSizeInBytes);
				GraphicsContext::setGlobalInternalResource(Gnm::kShaderGlobalResourceTessFactorBuffer, &tessFactorBuffer);
			}

			public:
				sce::Gnm::ConstantCommandBuffer m_ccb; // DUMMY! This is just here to allow code that touches this variable to compile

			private:
				LightweightConstantUpdateEngine::InputResourceOffsets m_boundShaderResourceOffsets[sce::Gnm::kShaderStageCount];
				LightweightConstantUpdateEngine::InputResourceOffsets m_SROCache[sce::Gnm::kShaderStageCount];
		};
	}
}

#endif // _SCE_GNMX_CUE_TO_LCUE_H
