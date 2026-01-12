#if __SPU

#include "grcore/fvf.cpp"
#include "grcore/vertexbuffereditor.cpp"
#include "grcore/indexbuffer.h"
#include "grmodel/geometry.h"
#include "vector/geometry.cpp"
#include "cloth/clothcontroller.h"

using namespace rage;

const Matrix34 rage::M34_IDENTITY;

namespace rage {


#define TRAP_VIRTUAL_CALLS (!__FINAL)
//#define dprintf(...) //Printf(__VA_ARGS__)

#if TRAP_VIRTUAL_CALLS
	void VirtualCall(void* pClass)
	{
		Quitf("Virtual function called on class instance %p", pClass);
	}
	void (*g_pVTable[32])(void*);
#endif


	bool sysLsCheckPointer(const void* start, const void* end, bool forWrite);
	template<class T> __forceinline void sysValidateWrite(T* addr, u32 count = 1)
	{
		if (!sysLsCheckPointer(addr, addr + count, true)) 
			__debugbreak();
	}
	template<> __forceinline void sysValidateWrite(void* addr, u32 count)
	{
		sysValidateWrite((u8*)addr, count);
	}
	template<class T> __forceinline void sysValidateRead(const T* addr, u32 count = 1)
	{
		if (!sysLsCheckPointer(addr, addr + count, false)) 
			__debugbreak();
	}
	template<> __forceinline void sysValidateRead(const void* addr, u32 count)
	{
		sysValidateRead((u8*)addr, count);
	}

	template <typename _T>
	struct atSpuArray : public atArray<_T>
	{
		atSpuArray(int count, _T* elements)
		{
			atArray<_T>::m_Count = count;
			atArray<_T>::m_Capacity = count;
			atArray<_T>::m_Elements = elements;
		}
	};

} // namespace rage


u8*     g_instLvlIdxToMtxAddrMM = NULL;
Mat34V* g_instLastMtxAddrMM = NULL;


#endif // __SPU
