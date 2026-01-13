// 
// system/distributor.h 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_DISTRIBUTOR_H 
#define SYSTEM_DISTRIBUTOR_H

namespace rage {

/*	
	Class which implements a really simple work distribution system.  Clients must declare
	a sysDistributorNode for each "task" they want to track globally.  Then they must
	call sysDistributorNode::AddSelf to register themselves with the system, using a queue
	ID smaller than sysDistributorNode::MaxQueue.  Once they do this, they can call IsMyTurn
	to see if they're ready to run.  If they want to wait a certain amount of time before
	running again, they can call Sleep to set a wakeup time at some point in the future.
	This wakeup time is not exact, but is a minimum bound on when they will be scheduled
	again; actual time depends on the number of other nodes in the same queue.  A sleep of
	zero means "as soon as possible".  Clients should call RemoveSelf when they no longer
	need notification; the dtor will do this automatically if AddSelf had been called.

	The notion of time in this system is independent of any actual time source; in fact, you
	are free to treat it as a frame count, a millisecond timer, or whatever you choose as
	long as all code treats time consistently.  Game code should call Update whenever it
	wants to advance all the ready queues, typically once per frame.
*/
class sysDistributorNode {
public:
	static const u32 MaxQueue = 8;

	sysDistributorNode() : m_Next(NULL), m_Added(0), m_QueueId(0), m_WakeUp(0) { }
	~sysDistributorNode() { if (m_Added) RemoveSelf(); }

	// PURPOSE: Returns true if this node is ready to run.
	// NOTES:	A node that has never had AddNode called on it will never pass the second test
	bool IsMyTurn() const { return m_WakeUp == 0 && sm_Nodes[m_QueueId] == this; }

	// PURPOSE: Put yourself to sleep for at least this long.  Zero means "wake me as soon as possible".
	//			The timer doesn't reset to its initial value; you need to do that yourself by
	//			calling Sleep again after IsMyTurn returns true for your object.
	// NOTES:	This is a minimum bound on when you will be woken up again.  If you miss your turn
	//			you will have to wait until it comes around again.  Assert is to catch overflow due
	//			to bitfield (typically a bogus input, like a negative number)
	void Sleep(u32 timeDelta) { m_WakeUp = timeDelta; FastAssert(m_WakeUp == timeDelta); }

	// PURPOSE: Add self to indicated schedule queue
	void AddSelf(u32 queueId);

	// PURPOSE:	Remove self from schedule queue.  Automatically called by dtor if necessary.
	void RemoveSelf();

	// PURPOSE:	Advance all ready queues, and advance all wakeup timers by this amount.
	static void Update(u32 timeDelta = 1);

private:
	void UpdateTime(u32 delta) { if (m_WakeUp <= delta) m_WakeUp = 0; else m_WakeUp -= delta; }

	sysDistributorNode *m_Next;
	u32 m_Added: 1, m_QueueId: 3, m_WakeUp: 28;

	static sysDistributorNode *sm_Nodes[MaxQueue];
};

}	// namespace rage

#endif // SYSTEM_DISTRIBUTOR_H 
