// 
// system/stack_collector.cpp
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved.
//

#include "diag/channel.h"
#include "string/stringhash.h"
#include "system/stack_collector.h"
#include "system/memops.h"

using namespace rage;

#if !__FINAL

void sysStackCollector::Clear()
{
	for ( atMap<u32,TagEntry>::Iterator it = m_tags.CreateIterator(); !it.AtEnd(); it.Next() )
		it.GetData().m_stackList.Kill();
	m_tags.Kill();

	m_stacks.Kill();
}

void sysStackCollector::RegisterTag(u32 tag)
{
	if ( IsTagUsed( tag ) )
	{
		Warningf( "sysStackCollector: attempting to register tag %d that is already registered.", tag );
		return;
	}
	
	m_tags[ tag ].m_count = 0;
	m_tags[ tag ].m_stackList.Kill();
}

void sysStackCollector::UnregisterTag(u32 tag)
{
	if ( !IsTagUsed(tag) )
		return;

	for ( atMap<u32,u32>::Iterator it = m_tags[ tag ].m_stackList.CreateIterator(); !it.AtEnd(); it.Next() )
		m_stacks.Delete( it.GetKey() );
	m_tags[ tag ].m_stackList.Kill();

	m_tags.Delete( tag );
}

bool sysStackCollector::IsTagUsed(u32 tag) {
	return m_tags.Access( tag ) != NULL;
}

u32 sysStackCollector::GetTagFromString(const char* str) {
	return atStringHash( str );
}

void sysStackCollector::CollectStack(u32 tag, const size_t* stack, u32 stackSize, u32 ignoreBottomLevels, u32 ignoreTopLevels)
{
	size_t	trimStack[32];
	u32		trimStackSize;
	size_t	hashData[33];
	u32		hash;

	if (!IsTagUsed(tag))
		RegisterTag(tag);

	// Prepare trim stack

	memset( trimStack, 0, sizeof(trimStack) );
	trimStackSize = stackSize - ignoreBottomLevels - ignoreTopLevels;
	sysMemCpy( trimStack, stack + ignoreBottomLevels, sizeof(size_t) * trimStackSize );

	// Compute the hash

	memset( hashData, 0, sizeof(hashData) );
	hashData[0] = tag;
	sysMemCpy( (hashData + 1), trimStack, trimStackSize );
	hash = atDataHash( (const char*) hashData, sizeof(hashData) );

	// If there isn't a stack entry for this stack, allocate one

	if ( m_stacks.Access( hash ) == NULL )
	{
		StackEntry		stackEntry;

		memset( &stackEntry, 0, sizeof(StackEntry) );
		stackEntry.m_count = 0;
		sysMemCpy( stackEntry.m_stack, trimStack, sizeof(size_t) * trimStackSize );
		stackEntry.m_stackSize = trimStackSize;
		
		m_stacks[ hash ] = stackEntry;
	}

	// Fetch the stack entry and increment its counter

	m_stacks[ hash ].m_count++;

	// Add this stack to tag's stack list; increment tag counter

	m_tags[ tag ].m_count++;
	m_tags[ tag ].m_stackList[ hash ] = 0;
}

void sysStackCollector::CollectStack(u32 tag, u32 ignoreBottomLevels, u32 ignoreTopLevels)
{
	size_t	stack[32];
	u32		stack_size = 0;

	sysStack::CaptureStackTrace( stack, 32, 0 );
	while ( stack[stack_size] != 0 )
		stack_size++;

	CollectStack( tag, stack, stack_size, ignoreBottomLevels, ignoreTopLevels );
}

void sysStackCollector::DefaultPrintStackInfo(u32 OUTPUT_ONLY(tagCount), u32 OUTPUT_ONLY(stackCount), const size_t* stack, u32 stackSize)
{
	OUTPUT_ONLY(float	percent = (float)( stackCount * 100 ) / (float)( tagCount ));
	diagLoggedPrintf( "%f%% (%d/%d)\n", percent, stackCount, tagCount );
	sysStack::PrintCapturedStackTrace( stack, stackSize );
}

void sysStackCollector::PrintInfoForTag(u32 tag, void (*PrintStackInfoFunc)(u32 tagCount, u32 stackCount, const size_t* stack, u32 stackSize))
{
	if ( !IsTagUsed( tag ) )
	{
		Warningf( "sysStackCollector: attempting to print info for unexistant tag %d.", tag );
		return;
	}

	TagEntry&	tagEntry = m_tags[ tag ];
	
	for ( atMap<u32,u32>::Iterator it = tagEntry.m_stackList.CreateIterator(); !it.AtEnd(); it.Next() )
	{
		StackEntry&		stackEntry = m_stacks[ it.GetKey() ];
		PrintStackInfoFunc( tagEntry.m_count, stackEntry.m_count, stackEntry.m_stack, stackEntry.m_stackSize );
	}
}

#endif // !__FINAL
