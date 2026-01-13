
#include "threadtype.h"

#include "bank/bank.h"

namespace rage {

namespace sysThreadType
{


bool g_IsMainThreadInitted = false;
__THREAD int g_ThisThreadType;


void AddCurrentThreadType(int threadType)
{
	g_ThisThreadType |= threadType;
	if (threadType == THREAD_TYPE_PROCESS_MAIN)
	{
		g_IsMainThreadInitted = true;
	}
}

void ClearCurrentThreadType(int threadType)
{
	g_ThisThreadType &= ~threadType;
}



}	// namespace fwThreadId
}	// namespace rage
