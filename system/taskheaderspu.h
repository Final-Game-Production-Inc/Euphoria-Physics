// 
// system/taskheaderspu.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_TASKHEADERSPU_H
#define SYSTEM_TASKHEADERSPU_H

#if __SPU

#include <cell/spurs/job_chain.h>

namespace rage {

class sysTaskContext
{
public:
	sysTaskContext(CellSpursJobContext2* context, CellSpursJob256* job)
	:	m_jobContext(context)
	,	m_job(job)
	,	m_userPos(m_job->header.sizeDmaList + m_job->header.sizeCacheDmaList)
	,	m_inPos(0)
	,	m_outPos(0)
	,	m_scratchPos(0)
	{}

	u32		InputSize() const;
	u32		OutputSize() const;
	u32		ScratchSize() const;
	void*	GetInput(u32 size);
	void*	GetOutput(u32 size);
	void*	GetScratch(u32 size);
	void*	GetCacheable(u32 index) const;
	void*	GetUserData(u32 size, u32 align = 4);
	u32		DmaTag() const;

	template<class T> T* GetInputAs(u32 count = 1) {return (T*)GetInput(sizeof(T) * count);}
	template<class T> T* GetOutputAs(u32 count = 1) {return (T*)GetOutput(sizeof(T) * count);}
	template<class T> T* GetScratchAs(u32 count = 1) {return (T*)GetScratch(sizeof(T) * count);}
	template<class T> T* GetUserDataAs(u32 count = 1) {return (T*)GetUserData(sizeof(T) * count, __alignof(T));}

private:
	CellSpursJobContext2*	m_jobContext;
	CellSpursJob256*		m_job;
	u32						m_userPos;
	u32						m_inPos;
	u32						m_outPos;
	u32						m_scratchPos;
};

inline u32 sysTaskContext::InputSize() const
{
	return m_job->header.sizeInOrInOut;
}

inline u32 sysTaskContext::OutputSize() const
{
	return m_job->header.sizeOut;
}

inline u32 sysTaskContext::ScratchSize() const
{
	return m_job->header.sizeScratch<<4;
}

inline void* sysTaskContext::GetInput(u32 size)
{
	Assertf(m_inPos + size <= m_job->header.sizeInOrInOut, "m_inPos=%d size=%d m_job->header.sizeInOrInOut=%d", m_inPos, size, m_job->header.sizeInOrInOut);
	void* pData = (char*)m_jobContext->ioBuffer + m_inPos;
	m_inPos += size;
	return pData;
}

inline void* sysTaskContext::GetOutput(u32 size)
{
	Assertf(m_outPos + size <= m_job->header.sizeOut, "m_outPos=%d size=%d m_job->header.sizeOut=%d", m_outPos, size, m_job->header.sizeOut);
	void* pData = (char*)m_jobContext->oBuffer + m_outPos;
	m_outPos += size;
	return pData;
}

inline void* sysTaskContext::GetScratch(u32 size)
{
	size = (size + 15) >> 4;
	Assertf(m_scratchPos + size <= m_job->header.sizeScratch, "m_scratchPos=%d size=%d m_job->header.sizeScratch=%d", m_scratchPos, size, m_job->header.sizeScratch);
	void* pData = (qword*)m_jobContext->sBuffer + m_scratchPos;
	m_scratchPos += size;
	return pData;
}

inline void* sysTaskContext::GetCacheable(u32 index) const
{
	Assert(index < m_jobContext->numCacheBuffer);
	return m_jobContext->cacheBuffer[index];
}

inline void* sysTaskContext::GetUserData(u32 size, u32 align)
{
	m_userPos = (m_userPos + align - 1) & ~(align - 1);
	void* data = (char*)&m_job->workArea + m_userPos;
	m_userPos += size;
	Assert(m_userPos <= sizeof(m_job->workArea) - 16);
	return data;
}

inline u32 sysTaskContext::DmaTag() const
{
	return m_jobContext->dmaTag;
}

} // namespace rage

#endif // __SPU

#endif // SYSTEM_TASKHEADERSPU_H
