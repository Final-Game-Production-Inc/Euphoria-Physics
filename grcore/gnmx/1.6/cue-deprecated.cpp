/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2014 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "grcore/gnmx/common.h"
#ifdef CUE_V2

#include "grcore/gnmx/cue.h"

using namespace sce::Gnm;
using namespace sce::Gnmx;


void ConstantUpdateEngine::init(void *cpramShadowBuffer, void *heapAddr, uint32_t numRingEntries) // DEPRECATED -- for compatibility
{
	SCE_GNM_UNUSED(cpramShadowBuffer);
	return init(heapAddr, numRingEntries);
}

void ConstantUpdateEngine::preDraw(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb)
{
	SCE_GNM_VALIDATE(m_dcb && m_ccb, "No dcb nor ccb are linked to this object -- Please call: bindCommandBuffers() API.");
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(ccb == m_ccb, "Invalid ccb pointer -- the ccb pointer must match the one linked to this object.");

	preDraw();
}


void ConstantUpdateEngine::postDraw(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb)
{
	SCE_GNM_VALIDATE(m_dcb && m_ccb, "No dcb nor ccb are linked to this object -- Please call: bindCommandBuffers() API.");
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(ccb == m_ccb, "Invalid ccb pointer -- the ccb pointer must match the one linked to this object.");

	postDraw();
}


void ConstantUpdateEngine::preDispatch(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb)
{
	SCE_GNM_VALIDATE(m_dcb && m_ccb, "No dcb nor ccb are linked to this object -- Please call: bindCommandBuffers() API.");
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(ccb == m_ccb, "Invalid ccb pointer -- the ccb pointer must match the one linked to this object.");

	preDispatch();
}


void ConstantUpdateEngine::postDispatch(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb)
{
	SCE_GNM_VALIDATE(m_dcb && m_ccb, "No dcb nor ccb are linked to this object -- Please call: bindCommandBuffers() API.");
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(ccb == m_ccb, "Invalid ccb pointer -- the ccb pointer must match the one linked to this object.");

	postDispatch();
}

void ConstantUpdateEngine::setVertexAndInstanceOffset(Gnm::DrawCommandBuffer *dcb, uint32_t vertexOffset, uint32_t instanceOffset)
{
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	setVertexAndInstanceOffset(vertexOffset, instanceOffset);
}

void ConstantUpdateEngine::setActiveResourceSlotCount(Gnm::ShaderStage stage, uint32_t count)
{
	SCE_GNM_UNUSED(stage);
	SCE_GNM_UNUSED(count);
}


void ConstantUpdateEngine::setActiveRwResourceSlotCount(Gnm::ShaderStage stage, uint32_t count)
{
	SCE_GNM_UNUSED(stage);
	SCE_GNM_UNUSED(count);
}


void ConstantUpdateEngine::setActiveSamplerSlotCount(Gnm::ShaderStage stage, uint32_t count)
{
	SCE_GNM_UNUSED(stage);
	SCE_GNM_UNUSED(count);
}


void ConstantUpdateEngine::setActiveVertexBufferSlotCount(Gnm::ShaderStage stage, uint32_t count)
{
	SCE_GNM_UNUSED(stage);
	SCE_GNM_UNUSED(count);
}


void ConstantUpdateEngine::setActiveConstantBufferSlotCount(Gnm::ShaderStage stage, uint32_t count)
{
	SCE_GNM_UNUSED(stage);
	SCE_GNM_UNUSED(count);
}


void ConstantUpdateEngine::setActiveStreamoutBufferSlotCount(uint32_t count)
{
	SCE_GNM_UNUSED(count);
}


#endif // CUE_V2
