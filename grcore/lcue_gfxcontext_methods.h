#undef _SCE_GNMX_GFXCONTEXT_METHODS_H
#include <gnmx/gfxcontext_methods.h>


inline void setRenderTarget(uint32_t rtSlot, sce::Gnm::RenderTarget const *target)
{
	if (target == NULL)
		return m_dcb.setRenderTarget(rtSlot, NULL);

	if (target->getCmaskFastClearEnable() && target->getCmaskAddress256ByteBlocks() != 0 && 
		target->getFmaskAddress256ByteBlocks() == 0)
	{
		sce::Gnm::RenderTarget rtCopy = *target;
		rtCopy.disableFmaskCompressionForMrtWithCmask();
		target = &rtCopy;
	}
	m_dcb.setRenderTarget(rtSlot, target);
}


inline void reset()
{
	m_dcb.resetBuffer();
	swapBuffers();
}


inline void submit()
{
	void* commandBuffer1GpuAddress = m_dcb.m_beginptr;
	uint32_t commandBuffer1Size = (m_dcb.m_cmdptr - m_dcb.m_beginptr) * sizeof(uint32_t);
	Gnm::submitCommandBuffers(1, &commandBuffer1GpuAddress, &commandBuffer1Size, 0, 0);
}


inline void submitAndFlip(uint32_t videoOutHandler, uint32_t rtIndex, uint32_t flipMode, int64_t flipArg)
{
	void* commandBuffer1GpuAddress = m_dcb.m_beginptr;
	uint32_t commandBuffer1Size = (m_dcb.m_cmdptr - m_dcb.m_beginptr) * sizeof(uint32_t);
	Gnm::submitAndFlipCommandBuffers(1, &commandBuffer1GpuAddress, &commandBuffer1Size, 0, 0, videoOutHandler, rtIndex, flipMode, flipArg);
}


inline int validate()
{
	void* commandBuffer1GpuAddress = m_dcb.m_beginptr;
	uint32_t commandBuffer1Size = (m_dcb.m_cmdptr - m_dcb.m_beginptr) * sizeof(uint32_t);
	return Gnm::validateCommandBuffers(1, &commandBuffer1GpuAddress, &commandBuffer1Size, 0, 0);
}
