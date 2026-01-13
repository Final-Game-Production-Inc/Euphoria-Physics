/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
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



void ConstantUpdateEngine::setDwordMaskedInDispatchDrawData(uint8_t dispatchDrawDword, uint32_t data, uint32_t mask)
{
	SCE_GNM_UNUSED(dispatchDrawDword);
	SCE_GNM_UNUSED(data);
	SCE_GNM_UNUSED(mask);
	SCE_GNM_ERROR("Not yet implemented in CUE2!");
	SCE_GNM_STATIC_ASSERT(!CUE2_SHOW_UNIMPLEMENTED);
}


void ConstantUpdateEngine::setDwordInDispatchDrawData(uint8_t dispatchDrawDword, uint32_t data)
{
	SCE_GNM_UNUSED(dispatchDrawDword);
	SCE_GNM_UNUSED(data);
	SCE_GNM_ERROR("Not yet implemented in CUE2!");
	SCE_GNM_STATIC_ASSERT(!CUE2_SHOW_UNIMPLEMENTED);
}


void ConstantUpdateEngine::setTsharpInDispatchDrawData(uint8_t dispatchDrawDword, const Gnm::Texture *tex)
{
	SCE_GNM_UNUSED(dispatchDrawDword);
	SCE_GNM_UNUSED(tex);
	SCE_GNM_ERROR("Not yet implemented in CUE2!");
	SCE_GNM_STATIC_ASSERT(!CUE2_SHOW_UNIMPLEMENTED);
}


void ConstantUpdateEngine::setVsharpInDispatchDrawData(uint8_t dispatchDrawDword, const Gnm::Buffer *buffer)
{
	SCE_GNM_UNUSED(dispatchDrawDword);
	SCE_GNM_UNUSED(buffer);
	SCE_GNM_ERROR("Not yet implemented in CUE2!");
	SCE_GNM_STATIC_ASSERT(!CUE2_SHOW_UNIMPLEMENTED);
}


void ConstantUpdateEngine::setSsharpInDispatchDrawData(uint8_t dispatchDrawDword, const Gnm::Sampler *sampler)
{
	SCE_GNM_UNUSED(dispatchDrawDword);
	SCE_GNM_UNUSED(sampler);
	SCE_GNM_ERROR("Not yet implemented in CUE2!");
	SCE_GNM_STATIC_ASSERT(!CUE2_SHOW_UNIMPLEMENTED);
}


void ConstantUpdateEngine::preDispatchDraw(Gnm::DrawCommandBuffer *dcb, Gnm::DispatchCommandBuffer *acb, Gnm::ConstantCommandBuffer *ccb,
										   Gnm::DispatchOrderedAppendMode *pOut_orderedAppendMode, uint16_t *pOut_dispatchDrawIndexDeallocationMask, 
										   uint32_t *pOut_sgprKrbLoc, Gnm::DispatchDrawMode *pOut_dispatchDrawMode, uint32_t *pOut_sgprVrbLoc)
{
	SCE_GNM_UNUSED(dcb);
	SCE_GNM_UNUSED(acb);
	SCE_GNM_UNUSED(ccb);
	SCE_GNM_UNUSED(pOut_orderedAppendMode);
	SCE_GNM_UNUSED(pOut_dispatchDrawIndexDeallocationMask);
	SCE_GNM_UNUSED(pOut_sgprKrbLoc);
	SCE_GNM_UNUSED(pOut_dispatchDrawMode);
	SCE_GNM_UNUSED(pOut_sgprVrbLoc);
	SCE_GNM_ERROR("Not yet implemented in CUE2!");
	SCE_GNM_STATIC_ASSERT(!CUE2_SHOW_UNIMPLEMENTED);
}

//~void ConstantUpdateEngine::preDispatchDraw(Gnm::DrawCommandBuffer *dcb, Gnm::DispatchCommandBuffer *acb, Gnm::ConstantCommandBuffer *ccb,
//~										   Gnm::DispatchOrderedAppendMode *pOutOrderedAppendMode, uint16_t *pOutDispatchDrawIndexDeallocationMask, uint32_t *pOutSgprKrbLoc, uint32_t *pOutSgprInstanceCs,
//~										   Gnm::DispatchDrawMode *pOutDispatchDrawMode, uint32_t *pOutSgprVrbLoc, uint32_t *pOutSgprInstanceVs)
//~{
//~	SCE_GNM_UNUSED(dcb);
//~	SCE_GNM_UNUSED(acb);
//~	SCE_GNM_UNUSED(ccb);
//~	SCE_GNM_UNUSED(pOutOrderedAppendMode);
//~	SCE_GNM_UNUSED(pOutDispatchDrawIndexDeallocationMask);
//~	SCE_GNM_UNUSED(pOutSgprKrbLoc);
//~	SCE_GNM_UNUSED(pOutSgprInstanceCs);
//~	SCE_GNM_UNUSED(pOutDispatchDrawMode);
//~	SCE_GNM_UNUSED(pOutSgprVrbLoc);
//~	SCE_GNM_UNUSED(pOutSgprInstanceVs);
//~	SCE_GNM_ERROR("Not yet implemented in CUE2!");
//~	SCE_GNM_STATIC_ASSERT(!CUE2_SHOW_UNIMPLEMENTED);
//~}


void ConstantUpdateEngine::postDispatchDraw(Gnm::DrawCommandBuffer *dcb, Gnm::DispatchCommandBuffer *acb, Gnm::ConstantCommandBuffer *ccb)
{
	SCE_GNM_UNUSED(dcb);
	SCE_GNM_UNUSED(acb);
	SCE_GNM_UNUSED(ccb);
	SCE_GNM_ERROR("Not yet implemented in CUE2!");
	SCE_GNM_STATIC_ASSERT(!CUE2_SHOW_UNIMPLEMENTED);
}




#endif // CUE_V2
