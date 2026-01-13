// 
// system/distributor.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "distributor.h"
#include "system/criticalsection.h"

namespace rage {

sysDistributorNode* sysDistributorNode::sm_Nodes[sysDistributorNode::MaxQueue];

static sysCriticalSectionToken s_DistributorToken;

void sysDistributorNode::AddSelf(u32 queueId)
{
	Assert(queueId < MaxQueue);

	m_QueueId = queueId;
	m_WakeUp = 0;
	m_Added = 1;
	// Could have one per queue if global contention was a problem.
	SYS_CS_SYNC(s_DistributorToken);
	m_Next = sm_Nodes[m_QueueId];
	sm_Nodes[m_QueueId] = this;
}

void sysDistributorNode::RemoveSelf()
{
	if (m_Added) {
		SYS_CS_SYNC(s_DistributorToken);
		sysDistributorNode **i = &sm_Nodes[m_QueueId];
		while (*i != this)
			i = &(*i)->m_Next;
		*i = m_Next;
		m_QueueId = 0;
	}
}

void sysDistributorNode::Update(u32 timeDelta)
{
	SYS_CS_SYNC(s_DistributorToken);
	for (int i=0; i<MaxQueue; i++) {
		if (sm_Nodes[i]) {
			// Only have work to do if there is more than one node in this queue.
			if (sm_Nodes[i]->m_Next) {
				sysDistributorNode *newTail = sm_Nodes[i];
				sysDistributorNode *j = sm_Nodes[i] = sm_Nodes[i]->m_Next;
				while (j->m_Next) {
					j->UpdateTime(timeDelta);
					j = j->m_Next;
				}
				j->UpdateTime(timeDelta);
				j->m_Next = newTail;
				newTail->UpdateTime(timeDelta);
				newTail->m_Next = NULL;
			}
			else
				sm_Nodes[i]->UpdateTime(timeDelta);
		}
	}
}

}	// namespace rage