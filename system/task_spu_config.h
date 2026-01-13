// Only use a single spurs instance, but share the last SPU with the system.
#define SYSTEM_WORKLOAD			0

#define TASK_MAX_TASKS_SPU		255

#define DEBUG_TASK_COMPLETION	0

#if DEBUG_TASK_COMPLETION
#define DEBUG_TASK_COMPLETION_ONLY(x)		x
#else
#define DEBUG_TASK_COMPLETION_ONLY(x)
#endif
