/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2014 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "grcore/gnmx/common.h"
#ifdef SCE_GNMX_ENABLE_CUE_V2

#include "grcore/gnmx/cue.h"

using namespace sce::Gnm;
using namespace sce::Gnmx;


void ConstantUpdateEngine::init(void *cpramShadowBuffer, void *heapAddr, uint32_t numRingEntries) // DEPRECATED -- for compatibility
{
	SCE_GNM_UNUSED(cpramShadowBuffer);
	return init(heapAddr, numRingEntries);
}

void ConstantUpdateEngine::preDraw(sce::Gnm::DrawCommandBuffer *dcb, sce::Gnm::ConstantCommandBuffer *ccb)
{
	SCE_GNM_VALIDATE(m_dcb && m_ccb, "No dcb nor ccb are linked to this object -- Please call: bindCommandBuffers() API.");
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(ccb == m_ccb, "Invalid ccb pointer -- the ccb pointer must match the one linked to this object.");

	preDraw();
}


void ConstantUpdateEngine::postDraw(sce::Gnm::DrawCommandBuffer *dcb, sce::Gnm::ConstantCommandBuffer *ccb)
{
	SCE_GNM_VALIDATE(m_dcb && m_ccb, "No dcb nor ccb are linked to this object -- Please call: bindCommandBuffers() API.");
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(ccb == m_ccb, "Invalid ccb pointer -- the ccb pointer must match the one linked to this object.");

	postDraw();
}


void ConstantUpdateEngine::preDispatch(sce::Gnm::DrawCommandBuffer *dcb, sce::Gnm::ConstantCommandBuffer *ccb)
{
	SCE_GNM_VALIDATE(m_dcb && m_ccb, "No dcb nor ccb are linked to this object -- Please call: bindCommandBuffers() API.");
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(ccb == m_ccb, "Invalid ccb pointer -- the ccb pointer must match the one linked to this object.");

	preDispatch();
}


void ConstantUpdateEngine::postDispatch(sce::Gnm::DrawCommandBuffer *dcb, sce::Gnm::ConstantCommandBuffer *ccb)
{
	SCE_GNM_VALIDATE(m_dcb && m_ccb, "No dcb nor ccb are linked to this object -- Please call: bindCommandBuffers() API.");
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(ccb == m_ccb, "Invalid ccb pointer -- the ccb pointer must match the one linked to this object.");

	postDispatch();
}

void ConstantUpdateEngine::preDispatchDraw(sce::Gnm::DrawCommandBuffer *dcb, sce::Gnm::DispatchCommandBuffer *acb, sce::Gnm::ConstantCommandBuffer *ccb,
	sce::Gnm::DispatchOrderedAppendMode *pOutOrderedAppendMode, uint32_t *pOutDispatchDrawIndexDeallocationMask, uint32_t *pOutSgprKrbLoc, uint32_t *pOutSgprInstanceCs,
	sce::Gnm::DispatchDrawMode *pOutDispatchDrawMode, uint32_t *pOutSgprVrbLoc, uint32_t *pOutSgprInstanceVs)
{
	SCE_GNM_VALIDATE(m_dcb && m_ccb && m_acb, "No dcb, ccb, or acb are linked to this object -- Please call: bindCommandBuffers() API.");
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(ccb == m_ccb, "Invalid ccb pointer -- the ccb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(acb == m_acb, "Invalid acb pointer -- the acb pointer must match the one linked to this object.");

	preDispatchDraw(pOutOrderedAppendMode, pOutDispatchDrawIndexDeallocationMask, pOutSgprKrbLoc, pOutSgprInstanceCs, pOutDispatchDrawMode, pOutSgprVrbLoc, pOutSgprInstanceVs);
}
void ConstantUpdateEngine::postDispatchDraw(sce::Gnm::DrawCommandBuffer *dcb, sce::Gnm::DispatchCommandBuffer *acb, sce::Gnm::ConstantCommandBuffer *ccb)
{
	SCE_GNM_VALIDATE(m_dcb && m_ccb && m_acb, "No dcb, ccb, or acb are linked to this object -- Please call: bindCommandBuffers() API.");
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(ccb == m_ccb, "Invalid ccb pointer -- the ccb pointer must match the one linked to this object.");
	SCE_GNM_VALIDATE(acb == m_acb, "Invalid acb pointer -- the acb pointer must match the one linked to this object.");

	postDispatchDraw();
}

void ConstantUpdateEngine::setVertexAndInstanceOffset(sce::Gnm::DrawCommandBuffer *dcb, uint32_t vertexOffset, uint32_t instanceOffset)
{
	SCE_GNM_VALIDATE(dcb == m_dcb, "Invalid dcb pointer -- the dcb pointer must match the one linked to this object.");
	setVertexAndInstanceOffset(vertexOffset, instanceOffset);
}

void ConstantUpdateEngine::setActiveResourceSlotCount(sce::Gnm::ShaderStage stage, uint32_t count)
{
	SCE_GNM_UNUSED(stage);
	SCE_GNM_UNUSED(count);
}


void ConstantUpdateEngine::setActiveRwResourceSlotCount(sce::Gnm::ShaderStage stage, uint32_t count)
{
	SCE_GNM_UNUSED(stage);
	SCE_GNM_UNUSED(count);
}


void ConstantUpdateEngine::setActiveSamplerSlotCount(sce::Gnm::ShaderStage stage, uint32_t count)
{
	SCE_GNM_UNUSED(stage);
	SCE_GNM_UNUSED(count);
}


void ConstantUpdateEngine::setActiveVertexBufferSlotCount(sce::Gnm::ShaderStage stage, uint32_t count)
{
	SCE_GNM_UNUSED(stage);
	SCE_GNM_UNUSED(count);
}


void ConstantUpdateEngine::setActiveConstantBufferSlotCount(sce::Gnm::ShaderStage stage, uint32_t count)
{
	SCE_GNM_UNUSED(stage);
	SCE_GNM_UNUSED(count);
}


void ConstantUpdateEngine::setActiveStreamoutBufferSlotCount(uint32_t count)
{
	SCE_GNM_UNUSED(count);
}


#endif // SCE_GNMX_ENABLE_CUE_V2
