#ifndef GRCORE_BUFFER_DURANGO_H
#define GRCORE_BUFFER_DURANGO_H

#include "grcore/effect_mrt_config.h"
#include "grcore/orbisdurangoresourcebase.h"

#if RSG_DURANGO || (RSG_PC && __64BIT && __RESOURCECOMPILER)

struct XG_BUFFER_DESC;
struct XG_RESOURCE_LAYOUT;
struct ID3D11Buffer;

#if RSG_DURANGO || __RESOURCECOMPILER || __TOOL
enum XG_BIND_FLAG;
#endif // RSG_DURANGO || __RESOURCECOMPILER || __TOOL

namespace rage
{

/*======================================================================================================================================*/
/* grcBufferDurango class.																												*/
/*======================================================================================================================================*/

#define GRC_BUFFER_DURANGO_OWNS_GRAPHICS_MEM			0x1
#define GRC_BUFFER_DURANGO_GPU_FLUSH_BEFORE_USE			0x4
#define GRC_BUFFER_DURANGO_HAS_LOCK_INFO				0x8
#define GRC_BUFFER_DURANGO_IS_CONSTANT_BUFFER			0x10
#define GRC_BUFFER_DURANGO_WHOLE_BUFFER_LOCKED			0x20

class grcBufferDurango : public grcOrbisDurangoBufferBase
{
private:
#if RSG_DURANGO
	struct LOCK_INFO
	{
		LOCK_INFO();
		~LOCK_INFO();
	#if RSG_DURANGO
		void *operator new(size_t size RAGE_NEW_EXTRA_ARGS_UNUSED);
		void operator delete(void *buf);
	#endif // RSG_DURANGO

	#if __ASSERT
		void OnGetD3DBuffer();
		void OnLock(int lockFlags);
	#endif //__ASSERT

		ID3D11Buffer *m_pD3DBuffer;
		size_t m_FlushOffsets_CPU[2];
		size_t m_FlushOffsets_GPU[2];
	#if __ASSERT
		int m_ThreadIdx;
		u32 m_FrameThreadIdxIsValidOn;
		u32 m_FrameLastLockDiscardedOn;
		u32 m_FrameLastRendereredFrom;
	#endif //__ASSERT

		static atPool < LOCK_INFO > s_Pool;
	};
#endif // RSG_DURANGO

public:
	grcBufferDurango();
	grcBufferDurango(size_t size, size_t stride, u32 bindType, void *pPreAllocatedMemory);
	grcBufferDurango(class datResource& rsc, u32 bindType);
	~grcBufferDurango();
	void Initialise(size_t size, size_t stride, u32 bindType, void *pPreAllocatedMemory, u32 miscFlags = 0, bool flushBeforeUse = true);
	void CleanUp();
	void CreateD3DResources(u32 bindType, u32 stride, u32 miscFlags, bool flushBeforeUse);
	void ReleaseD3DBufferHack();

public:
	void *Lock(u32 flags, u32 offset, u32 size, void **ppLockBase) const;
	void Unlock(u32 flags) const;
	void Unlock(u32 flags, u32 offset, u32 size) const;
	void *LockAll();
	void UnlockAll();
private:
	void UpdateFlushRange(u32 flags, u32 offset, u32 size) const;
	void PerformCPUFlush(u32 flags) const;
public:
	ID3D11Buffer *GetD3DBuffer() const;
	void Flush();
	size_t GetSize();
public:
	bool IsValid() const;
private:
#if RSG_DURANGO
	LOCK_INFO *GetLockInfo() const;
	ID3D11Buffer *FreeLockInfo();
#endif //RSG_DURANGO

public:
#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

#if RSG_DURANGO
	struct USER_MEMORY
	{
		union
		{
			ID3D11Buffer *m_pD3DBuffer;
			mutable LOCK_INFO *m_pLockInfo;
		};
	};

	USER_MEMORY *GetUserMemory() { return (USER_MEMORY *)&m_UserMemory[0]; };
	USER_MEMORY *GetUserMemory() const { return (USER_MEMORY *)&m_UserMemory[0]; };
#endif // RSG_DURANGO

private:

	friend class grcVertexBufferDurango;
	friend class grcIndexBufferDurango;
	friend class grcInstanceBufferBasic;
};

/*======================================================================================================================================*/
/* grcBufferDurangoResource class.																										*/
/*======================================================================================================================================*/

#if RSG_DURANGO

class grcBufferResourceDurango : public grcBufferDurango
{
public:
	grcBufferResourceDurango();
	~grcBufferResourceDurango();
	void Initialise(u32 Count, u32 Stride, u32 BindType, void *pPreAllocated, u32 miscFlags = 0, u32 UAVBufferFlags = 0, bool flushBeforeUse = true);
	void CleanUp();

	grcDeviceView* GetShaderResourceView()  const { return m_SRView; }
	grcDeviceView* GetUnorderedAccessView() const { return m_UAView; }

private:

	grcDeviceView *m_SRView;
	grcDeviceView *m_UAView;
};

#endif // RSG_DURANGO

}; // namespace rage

#endif // RSG_DURANGO || (RSG_PC && __64BIT && __RESOURCECOMPILER)

#endif // GRCORE_BUFFER_DURANGO_H
