// 
// system/stack_collector.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_STACK_COLLECTOR_H
#define SYSTEM_STACK_COLLECTOR_H

#include "system/stack.h"
#include "atl/map.h"

namespace rage {

/*
PURPOSE
This class allows the programmer to collect frequency data about the stack traces occurring
at a certain method. For example, let's say you have a method like:

void MyClass::MyMethod(...)
{
	stackCollector.CollectStack( m_tag );
	
	// your stuff...
}

Later, you can print all the stack traces whose last symbol is MyClass::MyMethod along
with frequency information.

Every stack can be associated with an arbitrary tag (i.e. a number) to ease collection
of statistical data.
*/

class sysStackCollector
{
#if !__FINAL

public:

	// PURPOSE:	Clears every information contained in this stack collector (tags and stack data).
	void Clear();

	// PURPOSE:	Registers a tag.
	// PARAMS:	tag - the tag to register
	void RegisterTag(u32 tag);

	// PURPOSE: Unregisters a tag, freeing all the resources associated with it.
	// PARAMS:	tag - the tag to unregister
	void UnregisterTag(u32 tag);

	// PURPOSE:	Check whether a tag is registered or not.
	// PARAMS:	tag - the tag to check
	// RETURNS:	true if the tag is registered, false otherwise
	bool IsTagUsed(u32 tag);

	// PURPOSE:	Helper method to get a tag from a string through hashing. Useful if you
	//			don't want to maintain a set of tags on your own.
	// PARAMS:	str - the string to process
	// RETURNS:	a tag for the given string.
	static u32 GetTagFromString(const char* str);

	// PURPOSE:	Collects a stack with the given tag. The stack should be captured through the sysStack::CaptureStackTrace method.
	// PARAMS:	tag - the tag to associate this stack trace with
	//			stack - the stack data
	//			stackSize - the number of entries in the stack
	//			ignoreBottomLevels - number of stack entries to ignore at the bottom (i.e. near the calling method)
	//			ignoreTopLevels - number of stack entries to ignore at the top (i.e. those above the main() method)
	void CollectStack(u32 tag, const size_t* stack, u32 stackSize, u32 ignoreBottomLevels = 0, u32 ignoreTopLevels = 0);

	// PURPOSE:	Collects a stack with the given tag. The stack is captured through the sysStack::CaptureStackTrace method.
	// PARAMS:	tag - the tag to associate this stack trace with
	//			ignoreBottomLevels - number of stack entries to ignore at the bottom (i.e. near the calling method)
	//			ignoreTopLevels - number of stack entries to ignore at the top (i.e. those above the main() method)
	void CollectStack(u32 tag, u32 ignoreBottomLevels = 0, u32 ignoreTopLevels = 0);

	// PURPOSE:	Default printing function for frequency data pertaining a certain stack trace.
	// PARAMS:	tagCount - the total number of stack traces collected for this tag
	//			stackCount - the total number this particular stack trace has been collected
	//			stack - stack data
	//			stackSize - number of entries in the stack
	static void DefaultPrintStackInfo(u32 tagCount, u32 stackCount, const size_t* stack, u32 stackSize);

	// PURPOSE:	Print frequency data for every stack trace collected for a certain tag.
	// PARAMS:	tag - the tag whose informations are to be printed
	//			PrintStackInfoFunc - callback to print frequency information
	void PrintInfoForTag(u32 tag, void (*PrintStackInfoFunc)(u32 tagCount, u32 stackCount, const size_t* stack, u32 stackSize) = DefaultPrintStackInfo);

private:

	struct TagEntry
	{
		u32					m_count;
		atMap<u32,u32>		m_stackList;
	};

	struct StackEntry
	{
		size_t				m_stack[32];
		u32					m_stackSize;
		u32					m_count;
	};

	atMap<u32,TagEntry>		m_tags;
	atMap<u32,StackEntry>	m_stacks;

#endif // !__FINAL
}; // class sysStackCollector

} // namespace rage

#endif // SYSTEM_STACK_COLLECTOR_H
