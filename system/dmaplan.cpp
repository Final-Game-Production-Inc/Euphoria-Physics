// 
// system/dmaplan.cpp 
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
// 

#if __PS3

#include "dmaplan.h"

namespace rage {

bool g_DmaPlanAsList = true;

sysDmaPlan::ObjectInfo sysDmaPlan::sm_ObjectInfos[MAX_NUM_DMAS];

#if __SPU
void sysDmaPlan::Initialize(sysDmaPlan* ppuAddr)
{
	m_PpuAddr = ppuAddr;
	m_NumDmas = 0;
	m_NumFixups = 0;
	m_SpuAlloc = 0;

	if (m_ReadOnly)
	{
		m_ReadOnly->Reset();
	}
}
#else
void sysDmaPlan::Initialize()
{
	m_PpuAddr = this;
	m_NumDmas = 0;
	m_NumFixups = 0;
	m_SpuAlloc = 0;

	if (m_ReadOnly)
	{
		m_ReadOnly->Reset();
	}
}
#endif

u32 sysDmaPlan::FindSpuOffset(u32 ptr)
{
	for (int i = m_NumDmas - 1; i >= 0; --i)
	{
		const ObjectInfo& objectInfo = sm_ObjectInfos[i];
		if (ptr >= objectInfo.ppuPtr && ptr < objectInfo.ppuSize + objectInfo.ppuPtr)
		{
			return objectInfo.spuPtr + ptr - objectInfo.ppuPtr;
		}
	}

	return 0;
}

void sysDmaPlan::AddFixup(void** ptr)
{
	Assert(m_NumFixups < m_MaxFixups);
	Assertf(((u32)*ptr & 0xf) == 0, "Referent 0x%p is not 16 byte aligned", *ptr);

	Fixup& fixup = m_Fixups[m_NumFixups++];
	fixup.pointer = FindSpuOffset((u32)ptr);
	Assertf(fixup.pointer, "Fixup failed (0x%p->0x%p), the pointer (0x%p) is not in an area of memory added to the dma plan", ptr, *ptr, ptr);
	fixup.referent = FindSpuOffset((u32)*ptr);
	Assertf(fixup.referent, "Fixup failed (0x%p->0x%p), the referent (0x%p) is not in an area of memory added to the dma plan", ptr, *ptr, *ptr);
}

#if __SPU
void sysDmaPlan::AddFixup(void** lsPtr, void** mmPtr)
{
	Assert(m_NumFixups < m_MaxFixups);
	Assertf(((u32)*lsPtr & 0xf) == 0, "Referent 0x%p is not 16 byte aligned", *mmPtr);

	Fixup& fixup = m_Fixups[m_NumFixups++];
	fixup.pointer = FindSpuOffset((u32)mmPtr);
	Assertf(fixup.pointer, "Fixup failed (0x%p->0x%p), the pointer (0x%p) is not in an area of memory added to the dma plan", mmPtr, *lsPtr, mmPtr);
	fixup.referent = FindSpuOffset((u32)*lsPtr);
	Assertf(fixup.referent, "Fixup failed (0x%p->0x%p), the referent (0x%p) is not in an area of memory added to the dma plan", mmPtr, *lsPtr, *lsPtr);
}
#endif // __SPU

#if !__FINAL
void sysDmaPlan::PrintDmaList()
{
	const u32 numDmas = m_NumDmas;
#if SYS_DMA_VALIDATION
	Assert(numDmas < MAX_NUM_DMAS); // Something must have gone wrong
#endif
	for (u32 i = 0; i < numDmas; ++i)
	{
		if (m_ReadOnly)
		{
			Displayf("%d: addr 0x%x size 0x%x, %s", i, (u32)m_DmaList[i].eal, (u32)m_DmaList[i].size, m_ReadOnly->IsSet(i) ? "*" : "");
		}
		else
		{
			Displayf("%d: addr 0x%x size 0x%x", i, (u32)m_DmaList[i].eal, (u32)m_DmaList[i].size);
		}
	}
}
#endif


} // namespace rage

#endif
