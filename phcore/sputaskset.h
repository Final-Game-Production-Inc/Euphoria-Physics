//
// phcore/sputaskset.h
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//

#ifndef PHCORE_SPUTASKSET_H
#define PHCORE_SPUTASKSET_H

#if __PS3

struct CellSpursTaskset;
struct CellSpursQueue;

namespace rage {

class phSpuTaskSet
{
public:
	static void InitClass();
	static void ShutdownClass();

	static CellSpursTaskset* GetSpursTaskSet() { return sm_SpursTaskSet; }
	static CellSpursQueue* GetSpursCompletionQueue() { return sm_SpursCompletionQueue; }

private:
	static CellSpursTaskset* sm_SpursTaskSet;
	static CellSpursQueue* sm_SpursCompletionQueue;
	static void* sm_SpursCompletionQueueBuffer;
};

} // namespace rage

#endif // __PS3

#endif // PHCORE_SPUTASKSET_H
