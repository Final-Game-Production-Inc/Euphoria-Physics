
#include "systemallocator_system.h"

#if MEMORY_TRACKER
#include "system/xtl.h"
#include "system/memvisualize.h"

namespace rage
{
#if RSG_DURANGO || RSG_PC

#if RSG_DURANGO
#define SYSTEM_TRACKER_CRITICAL_SECTION sysCriticalSection cs(m_Token)
#endif

#if RSG_DURANGO
	sysMemSystemTracker::sysMemSystemTracker(const size_t size /*= (1 << 16)*/) : m_memoryUsed(0), m_totalAllocations(0), m_pool(size)
#elif RSG_PC
	sysMemSystemTracker::sysMemSystemTracker() : m_memoryUsed(0), m_totalAllocations(0)
#endif
	{
		sysMemSet(m_memoryUsedByID, 0, SYSTEM_ALLOCATOR_BUCKETS * sizeof(size_t));
		sysMemSet(m_totalAllocationsByID, 0, SYSTEM_ALLOCATOR_BUCKETS * sizeof(size_t));
	}

	size_t sysMemSystemTracker::GetSize() const
	{
#if RSG_DURANGO
		TITLEMEMORYSTATUS status;
		status.dwLength =  sizeof(TITLEMEMORYSTATUS);
		TitleMemoryStatus(&status);

		return status.ullTotalMem;
#else
		return sysMemTotalMemory();
#endif
	}

	size_t sysMemSystemTracker::GetUsed(int bucket /*= -1*/) const
	{
		if (bucket >= 0 && bucket < SYSTEM_ALLOCATOR_BUCKETS)
			return m_memoryUsedByID[bucket];
		else
			return m_memoryUsed;
	}

	size_t sysMemSystemTracker::GetAvailable() const
	{
#if RSG_DURANGO
		TITLEMEMORYSTATUS status;
		status.dwLength =  sizeof(TITLEMEMORYSTATUS);
		TitleMemoryStatus(&status);

		return status.ullAvailMem;
#else
		return sysMemTotalFreeMemory();
#endif
	}

	void sysMemSystemTracker::Allocate(void* ptr, size_t size, int id /*= 0*/) 
	{
#if RSG_DURANGO
		SYSTEM_TRACKER_CRITICAL_SECTION;

		//Assert(m_map.find(ptr) == m_map.end());
		if (m_map.find(ptr) != m_map.end())
			return;

		Node* node = m_pool.Construct();
		Assert(node);

		node->m_size = size;
		m_map.insert(ptr, node);
#endif
		m_memoryUsedByID[id] += size;
		m_memoryUsed += size;

		m_totalAllocationsByID[id]++;
		m_totalAllocations++;

#if RAGE_TRACKING
		if (::rage::diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
			::rage::diagTracker::GetCurrent()->Tally(ptr, size, 0);
#endif
	}

#if RSG_DURANGO
	void sysMemSystemTracker::Free(void* ptr, int id /*= 0*/) 
#else RSG_PC
	void sysMemSystemTracker::Free(void* ptr, size_t size, int id /*= 0*/) 
#endif
	{
#if RSG_DURANGO
		SYSTEM_TRACKER_CRITICAL_SECTION;

		NodeMap::iterator it = m_map.find(ptr);		
		//Assert(it != m_map.end());

		if (it == m_map.end())
			return;

		Node* node = it->second;
		const size_t size = node->m_size;
		Assert(size);
#endif
		m_memoryUsedByID[id] -= size;
		m_memoryUsed -= size;

		m_totalAllocationsByID[id]--;
		m_totalAllocations--;
#if RSG_DURANGO
		node->m_size = 0;

		m_map.erase(it);
		m_pool.Delete(node);
#endif

#if RAGE_TRACKING
		if (::rage::diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
			::rage::diagTracker::GetCurrent()->UnTally(ptr, size);
#endif
	}

	void sysMemSystemTracker::Print() const
	{
		Displayf("XTL Memory - Start");
		size_t total = 0;

		for(int i = 0; i < SYSTEM_ALLOCATOR_BUCKETS; ++i)
		{
			if (m_memoryUsedByID[i])
			{
				total += m_memoryUsedByID[i];
				Displayf("%8d: %" SIZETFMT "d, %" SIZETFMT "d KB, %2f MB", i, m_memoryUsedByID[i], m_memoryUsedByID[i] >> 10, (float) m_memoryUsedByID[i] / 1024.0 / 1024.0);
			}
		}

		Displayf("%8s: %" SIZETFMT "d, %" SIZETFMT "d KB, %2f MB", "Total", total, total >> 10, (float) total / 1024.0 / 1024.0);
		Displayf("XTL Memory - Finish");
	}

#elif __XENON
	const char* sysMemSystemTracker::GetMemBucketStringByType(int type)
	{
		switch (type)
		{
		case eXALLOCAllocatorId_D3D:			return "D3D";
		case eXALLOCAllocatorId_D3DX:			return "D3DX";
		case eXALLOCAllocatorId_XAUDIO:			return "XAUDIO";
		case eXALLOCAllocatorId_XAPI:			return "XAPI";
		case eXALLOCAllocatorId_XACT:			return "XACT";
		case eXALLOCAllocatorId_XBOXKERNEL:		return "XBOXKERNEL";
		case eXALLOCAllocatorId_XBDM:			return "XBDM";
		case eXALLOCAllocatorId_XGRAPHICS:		return "XGRAPHICS";
		case eXALLOCAllocatorId_XONLINE:		return "XONLINE";
		case eXALLOCAllocatorId_XVOICE:			return "XVOICE";
		case eXALLOCAllocatorId_XHV:			return "XHV";
		case eXALLOCAllocatorId_USB:			return "USB";
		case eXALLOCAllocatorId_XMV:			return "XMV";
		case eXALLOCAllocatorId_SHADERCOMPILER:	return "SHADERCOMPILER";
		case eXALLOCAllocatorId_XUI:			return "XUI";
		case eXALLOCAllocatorId_XASYNC:			return "XASYNC";
		case eXALLOCAllocatorId_XCAM:			return "XCAM";
		case eXALLOCAllocatorId_XVIS:			return "XVIS";
		case eXALLOCAllocatorId_XIME:			return "XIME";
		case eXALLOCAllocatorId_XFILECACHE:		return "XFILECACHE";
		case eXALLOCAllocatorId_XRN:			return "XRN";
		case eXALLOCAllocatorID_XMCORE:			return "XMCORE";
		case eXALLOCAllocatorId_XAUDIO2:		return "XAUDIO2";
		case eXALLOCAllocatorId_XAVATAR:		return "XAVATAR";
		case eXALLOCAllocatorId_XLSP:			return "XLSP";
		case eXALLOCAllocatorId_D3DAlloc:		return "D3DALLOC";
		case eXALLOCAllocatorID_NUISPEECH:		return "NUISPEECH";
		case eXALLOCAllocatorId_NuiApi:			return "NUIAPI";
		case eXALLOCAllocatorId_NuiIdentity:	return "NUIIDENTITY";
		case eXALLOCAllocatorId_XTweak:			return "XTWEAK";
		case eXALLOCAllocatorId_XAMCOMMON:		return "XAMCOMMON";
		case eXALLOCAllocatorId_NUIUI:			return "NUIUI";
		case eXALLOCAllocatorId_LUA:			return "LUA";
		case eXALLOCAllocatorId_DRM:			return "DRM";
		case eXALLOCAllocatorId_XAVPipeline:	return "XAVPIPELINE";
		case eXALLOCAllocatorId_XAVCodecs:		return "XAVCODECS";
		case eXALLOCAllocatorId_XAUTH:			return "XAUTH";
		default:								return "Default";	
		}
	}

	size_t sysMemSystemTracker::GetAvailable() const
	{
		MEMORYSTATUS status;
		GlobalMemoryStatus(&status);
		return status.dwAvailPhys;
	}

	sysMemSystemTracker::sysMemSystemTracker()
	{
		// Initialize everything to zero
		sysMemSet(m_memoryUsedByID, 0, XENON_ALLOCATOR_BUCKETS * sizeof(size_t));	

		// Fake allocation of OS reserved memory
		const int osMemory = 32 << 20;
		const int index = GetMemBucketIndex(eXALLOCAllocatorId_XBOXKERNEL);
		m_memoryUsedByID[index] += osMemory;
		m_memoryUsed = osMemory;
	}

	void* sysMemSystemTracker::Allocate(size_t UNUSED_PARAM(size), size_t UNUSED_PARAM(align), int UNUSED_PARAM(heapIndex))
	{
		AssertMsg(false, "sysMemSystemAllocator:Allocate not implemented");
		return 0;
	}

	void sysMemSystemTracker::Free(const void* UNUSED_PARAM(ptr))
	{
		AssertMsg(false, "sysMemSystemAllocator:Free not implemented");
	}

	void* sysMemSystemTracker::GetHeapBase() const
	{
		return NULL;
	}

	size_t sysMemSystemTracker::GetHeapSize() const
	{
		return m_memoryUsed + GetAvailable();
	}

	size_t sysMemSystemTracker::GetMemoryUsed(int bucket)
	{
		if (bucket >= 0 && bucket < XENON_ALLOCATOR_BUCKETS)
			return m_memoryUsedByID[bucket];
		else
			return m_memoryUsed;
	}

	size_t sysMemSystemTracker::GetMemoryAvailable()
	{
		return GetAvailable();
	}

	void sysMemSystemTracker::TrackMemAlloc(size_t size, int allocId) 
	{	
		const int index = GetMemBucketIndex(allocId);
		m_memoryUsedByID[index] += size;
		m_memoryUsed += size;
	}

	void sysMemSystemTracker::TrackMemFree(size_t size, int allocId) 
	{
		const int index = GetMemBucketIndex(allocId);	
		m_memoryUsedByID[index] -= size;
		m_memoryUsed -= size;
	}

	void sysMemSystemTracker::PrintMemBuckets() const
	{
		Displayf("XENON MEMORY BUCKETS - START");

		for(int i = 0; i < XENON_ALLOCATOR_BUCKETS; ++i)
			Displayf("%16s: %" SIZETFMT "d", GetMemBucketStringByIndex(i), m_memoryUsedByID[i]);

		Displayf("XENON MEMORY BUCKETS - END");
	}
#endif
} // namespace rage

#endif // MEMORY_TRACKER
