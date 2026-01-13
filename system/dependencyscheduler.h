// 
// system/dependencyscheduler.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_DEPENDENCYSCHEDULER_H
#define SYSTEM_DEPENDENCYSCHEDULER_H

#include "diag/stats.h"

#define SYS_DEPENDENCY_PROFILE (__STATS && __CONSOLE)

namespace rage
{

struct sysDependency;

////////////////////////////////////////////////////////////////////////////////

// PURPOSE: Dependency manager
// Manages dependency chain(s) and worker threads
class sysDependencyScheduler
{
public:
	// PURPOSE: Initializer
	static void InitClass();

	// PURPOSE: Shutdown
	static void ShutdownClass();

	// PURPOSE: Insert a ready dependency
	static void Insert(sysDependency* dep);

	// PURPOSE: Output the debug state
#if !__FINAL
	static void OutputDebug();
#endif

	// PURPOSE: Register a profile tag
#if SYS_DEPENDENCY_PROFILE
	static void RegisterTag(class pfGroup& group, u32 tag, const char* name);
#endif

private:

	// PURPOSE: Insert a new dependency
	static void InsertInternal(sysDependency* dep);

	// PURPOSE: Worker thread loop
	static void DependencyWorkerThread(void* param);

	// PURPOSE: Local thread loop
	static void DependencyLocalThread();
};

////////////////////////////////////////////////////////////////////////////////

} // namespace rage

#endif  // SYSTEM_DEPENDENCYSCHEDULER_H
