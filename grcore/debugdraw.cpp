#include "debugdraw.h"

// Rage headers
#include "file/asset.h"
#include "file/stream.h"
#include "grcore/im.h"
#include "grcore/effect.h"
#include "grcore/font.h"
#include "grcore/gfxcontext.h"
#include "grcore/light.h"
#include "grcore/quads.h"
#include "grcore/setup.h"
#include "grcore/viewport.h"
#include "grcore/texture.h"
#include "input/keyboard.h"
#include "input/keys.h"
#include "input/mapper.h"
#include "math/amath.h"
#include "system/bit.h"
#include "system/criticalsection.h"
#include "system/interlocked.h"
#include "system/messagequeue.h"
#include "system/nelem.h"
#include "system/param.h"
#include "system/timemgr.h"
#include "vector/colors.h"
#if __PS3
#include "grcore/wrapper_gcm.h"
#endif // __PS3
#if __D3D
#include "system/xtl.h"
#include "system/d3d9.h"
#endif


namespace rage {

#if __IM_BATCHER

PARAM(nodebugtxt, "[debug] Don't display debug text on screen");
PARAM(fixedheightdebugfont, "[debug] Use the same height for all debug text, proportional and fixed-width");
PARAM(debugtextfixedwidth, "[debug] Use fixed-width font for EVERYTHING");
PARAM(nowarnonexcessivedraw, "[debug] Don't display on-screen message if there is too much debug drawing");

#define __USE_RAGE_TEXT
#define DEBUG_CHAR_WIDTH	9


// The way debug drawing "works", doesn't really sit well with multi-threaded
// rendering.  Any subrender thread can add debug drawing to a batcher, then
// this is drawn near the end of the frame.  But, "near the end of the frame" is
// from the GPU's point of view.  It is possible for the subrender threads to
// run in a different order.
//
// The kludge to attempt to fix this is to stall the thread that renders the
// debug drawing until all previous subrender threads have completed.  We don't
// want this stall to occur all the time, so instead it is only done if there is
// debug drawing from a subrender thread to be done.  But that cannot be
// properly known without the stall.  To reduce this race to just a single frame
// problem, once there is debug drawing, the stall will occur for the next few
// frames.
//
#define WANT_SUB_RENDER_THREAD_DEBUG_DRAW_COUNT     4

#if __XENON
#define DEBUG_OUTPUT_XSTART	7
#define DEBUG_OUTPUT_YSTART	3
#elif __PPU
#define DEBUG_OUTPUT_XSTART	5
#define DEBUG_OUTPUT_YSTART	3
#else
#define DEBUG_OUTPUT_XSTART	6
#define DEBUG_OUTPUT_YSTART	1
#endif

// Minimum number of frames to display the "too much debug draw" message
#define TOO_MUCH_DEBUG_DRAW_FRAMECOUNT (80)


class grdStockDebugAllocation : public grcDebugDraw::MemoryAllocationInterface
{
public:
	virtual void Init() {};
	virtual void Shutdown() {};
	virtual void *Alloc(size_t size)    { return rage_aligned_new(16) char[size]; }
	virtual void Free(void *mem)        { delete[] (char*)mem; }
};

static grdStockDebugAllocation s_StockAllocator;
static grcDebugDraw::MemoryAllocationInterface *s_MemoryAllocationInterface = &s_StockAllocator;

enum { BUFFER_SIZE = 0x10000 };

static bool BufferAllocCallback(void **alloc, size_t *sizeBytes)
{
	void *const ptr = s_MemoryAllocationInterface->Alloc(BUFFER_SIZE);
	*alloc = ptr;
	*sizeBytes = BUFFER_SIZE;
	return ptr != NULL;
}

static void BufferFreeCallback(void *alloc)
{
	s_MemoryAllocationInterface->Free(alloc);
}




class BatcherLock;


// The synchronization between threads generating batched debug draw commands,
// and the render thread that renders them, relies primarily on ring buffers.
// Threads acquire, or are assigned, batchers from this ring buffer.  This then
// sets a fixed order of batcher rendering for consumption by the render thread.
//
// Generally there is some form of higher level synchronization that garuntees
// the batchers will have been filled before the rendering gets to them.  This
// is the case for most threads.  But some worker threads may be unsynchronized
// with the render threads.  Batcher allocations work with handles rather than a
// direct pointer to the batcher in the ring to handle this unsynchronized case.
// Each access to a batcher must lock it, and the handle is used to check if the
// lock is stale, requiring a new batcher allocation.
//
class BatcherRing
{
public:

	typedef u32 Handle;     // 0 indicates invalid handle

	explicit BatcherRing(unsigned numBatchers);
	~BatcherRing();
	Handle GetAndIncNextWrite();
	grcBatcher *GetNextRead();
	void IncNextRead();
	unsigned GetNumPendingReads() const;

	grcBatcher *LockForWrite(Handle batcherHandle);
	void UnlockForWrite(Handle batcherHandle);

private:
	unsigned m_NumBatchers;
	grcBatcher *m_Ring;
	BatcherLock *m_Locks;
	volatile Handle m_RingNextReadStart;
	volatile Handle m_RingNextReadDone;
	volatile Handle m_RingNextWrite;

	static inline Handle Next(Handle i);
	typedef s32 HandleDiff;
	static inline HandleDiff Diff(Handle a, Handle b);

	static inline void CompileTimeAssertFunction()
	{
		CompileTimeAssert(sizeof(Handle) == sizeof(grcDebugDraw::BatcherHandle));
		CompileTimeAssert(sizeof(Handle) == sizeof(HandleDiff));
	}
};

// Rather than have a mutex per BatcherRing entry (which could be a very large
// number), BatcherLock and BatcherLockMutexes work together to share a smallish
// number of mutexes.  Locking a BatcherLock will allocate a mutex from
// BatcherLockMutexes if it doesn't already have one.  Unlocking a BatcherLock
// will free the mutex if there are no other threads waiting to lock it.
class BatcherLock
{
public:

	BatcherLock();
	~BatcherLock();
	bool Lock();
	void Unlock();

private:

	// Least significant 16-bits are index into BatcherLockMutexes
	// Most  significant 16-bits are count of threads waiting (not including any current lock owner)
	volatile u32 m_Lock;

	// Magic value for m_Lock indicating no lock allocated
	enum { NO_LOCK = 0x0000ffff };
};

class BatcherLockMutexes
{
public:

	BatcherLockMutexes();
	~BatcherLockMutexes();
	u16 AllocateLock();
	void FreeLock(u16 idx);
	void Lock(u16 idx);
	void Unlock(u16 idx);

private:

	enum { NUM_LOCKS = 16 };
	sysCriticalSectionToken *m_Locks;
	sysMessageQueue<u16, NUM_LOCKS> m_FreeLocks;
};



static BatcherLockMutexes *s_BatcherLockMutexes;

BatcherLockMutexes::BatcherLockMutexes()
{
	m_Locks = rage_new sysCriticalSectionToken[NUM_LOCKS];
	for (u16 i=0; i<NUM_LOCKS; ++i)
	{
		m_FreeLocks.Push(i);
	}
}

BatcherLockMutexes::~BatcherLockMutexes()
{
	delete [] m_Locks;
}

u16 BatcherLockMutexes::AllocateLock()
{
	return m_FreeLocks.Pop();
}

void BatcherLockMutexes::FreeLock(u16 idx)
{
	Assert(idx < NUM_LOCKS);
	ASSERT_ONLY(u16 *succeeded=) m_FreeLocks.Push(idx);
	Assert(succeeded);
}

void BatcherLockMutexes::Lock(u16 idx)
{
	Assert(idx < NUM_LOCKS);
	m_Locks[idx].Lock();
}

void BatcherLockMutexes::Unlock(u16 idx)
{
	Assert(idx < NUM_LOCKS);
	m_Locks[idx].Unlock();
}

BatcherLock::BatcherLock()
	: m_Lock(NO_LOCK)
{
}

BatcherLock::~BatcherLock()
{
	Assert(m_Lock == NO_LOCK);
}

bool BatcherLock::Lock()
{
	u32 val = m_Lock;
	for (;;)
	{
		u32 mem;

		if (val == NO_LOCK)
		{
			const u32 lock = s_BatcherLockMutexes->AllocateLock();
			s_BatcherLockMutexes->Lock((u16)lock);
			mem = sysInterlockedCompareExchange(&m_Lock, lock, val);
			if (val == mem)
			{
				return true;
			}
			val = mem;
			s_BatcherLockMutexes->Unlock((u16)lock);
			s_BatcherLockMutexes->FreeLock((u16)lock);
		}

		mem = sysInterlockedCompareExchange(&m_Lock, val+0x10000, val);
		if (val == mem)
		{
			const u32 lock = val & 0xffff;
			s_BatcherLockMutexes->Lock((u16)lock);
			sysInterlockedAdd(&m_Lock, -0x10000);
			return true;
		}
		val = mem;
	}
}

void BatcherLock::Unlock()
{
	u32 val = m_Lock;

	// No other threads waiting ?
	if ((val & 0xffff0000) == 0)
	{
		if (sysInterlockedCompareExchange(&m_Lock, NO_LOCK, val) == val)
		{
			s_BatcherLockMutexes->Unlock((u16)val);
			s_BatcherLockMutexes->FreeLock((u16)val);
			return;
		}
	}

	s_BatcherLockMutexes->Unlock(val&0xffff);
}




BatcherRing::BatcherRing(unsigned numBatchers)
{
	Assert(((numBatchers) & (numBatchers-1)) == 0);    // numBatchers must be a power of two
	m_NumBatchers = numBatchers;
	m_Ring = rage_new grcBatcher[numBatchers];
	for (unsigned i=0; i<numBatchers; ++i)
	{
		m_Ring[i].Init(BufferAllocCallback, BufferFreeCallback);
	}
	m_Locks = rage_new BatcherLock[numBatchers];

	// Indices must always have MSB set (to ensure 0, ie invalid handle, is
	// never returned).  Initialize close to wrap-around so that this case is
	// properly tested.
	m_RingNextReadStart = m_RingNextReadDone = m_RingNextWrite = 0xfffffff0u;
}

BatcherRing::~BatcherRing()
{
	delete [] m_Ring;
	delete [] m_Locks;
}

/*static*/ inline u32 BatcherRing::Next(u32 i)
{
	// Always set most significant bit so that zero can never be returned.
	return (i+1)|0x80000000u;
}

/*static*/ inline BatcherRing::HandleDiff BatcherRing::Diff(Handle a, Handle b)
{
	// Relies on arithmetic shift right of signed values to copy second most significant bit to most significant bit
	const HandleDiff d = ((HandleDiff)(a-b)<<1)>>1;
	Assert(d == (HandleDiff)(((Handle)(a-b)&0x7fffffff) | (((Handle)(a-b)&0x40000000)<<1)));
	return d;
}

BatcherRing::Handle BatcherRing::GetAndIncNextWrite()
{
	const unsigned numBatchers = m_NumBatchers;
	Handle w = m_RingNextWrite;
	for (;;)
	{
		const Handle n = Next(w);
		const Handle unread = (Handle)Diff(n, m_RingNextReadDone);
		if (unread >= numBatchers)
		{
			Assert(unread == numBatchers);
			return 0;
		}
		const u32 mem = sysInterlockedCompareExchange(&m_RingNextWrite, n, w);
		if (mem == w)
		{
			Assert(!m_Ring[w&(numBatchers-1)].IsClosed());
			Assert(w);
			return w;
		}
		w = mem;
	}
}

grcBatcher *BatcherRing::GetNextRead()
{
	const u32 r = m_RingNextReadStart;
	const u32 w = m_RingNextWrite;
	if (r == w)
	{
		return NULL;
	}
	Assert(Diff(w,r) > 0);
	const unsigned idx = r&(m_NumBatchers-1);
	grcBatcher *const batcher = m_Ring+idx;
	// Lame
	while (batcher->GetRenderMustWait())
	{
		sysIpcSleep(1);
	}
	m_Locks[idx].Lock();
	m_RingNextReadStart = Next(r);
	return batcher;
}

void BatcherRing::IncNextRead()
{
	const u32 r = m_RingNextReadDone;
	Assert(r != m_RingNextWrite);
	Assert(Next(r) == m_RingNextReadStart);
	m_RingNextReadDone = Next(r);       // only one thread should increment this per frame, so atomic not required
	m_Locks[r&(m_NumBatchers-1)].Unlock();
}

unsigned BatcherRing::GetNumPendingReads() const
{
	return Diff(m_RingNextWrite, m_RingNextReadStart);
}

grcBatcher *BatcherRing::LockForWrite(Handle batcherHandle)
{
	const u32 numBatchers = m_NumBatchers;
	if ((Handle)Diff(batcherHandle, m_RingNextReadStart) < numBatchers)
	{
		const u32 idx = batcherHandle&(numBatchers-1);
		if (m_Locks[idx].Lock())
		{
			if ((Handle)Diff(batcherHandle, m_RingNextReadStart) < numBatchers)
			{
				return m_Ring+idx;
			}
			m_Locks[idx].Unlock();
		}
	}
	return NULL;
}

void BatcherRing::UnlockForWrite(Handle batcherHandle)
{
	Assert(batcherHandle);
	m_Locks[batcherHandle&(m_NumBatchers-1)].Unlock();
}


// There is a separate ring buffer for 2D and 3D batchers.
static BatcherRing *s_BatcherRing[grcDebugDraw::MAX_LINGER_BUFFERS];

// Lingering commands are copied to a double buffered batcher.
static grcBatcher *s_LingerBatcher[grcDebugDraw::MAX_LINGER_BUFFERS][2];
static unsigned s_LingerBatcherFrame[grcDebugDraw::MAX_LINGER_BUFFERS];

class PerThreadState
{
public:

	PerThreadState();
	void Init();

	BatcherRing::Handle batcher[grcDebugDraw::MAX_LINGER_BUFFERS];

	int framesToLive;

	bool doDebugZTest;
	bool drawAdditively;
	bool disableCulling;

	const grcFont* fontStack[8];
	unsigned       fontStackIndex;

	int columnizedTop;              // Top line (in characters) where this columnized debug output begins
	int rawColumnizedTop;           // Top line (in pixels) where this columnized debug output begins
	int columnizedBottom;           // Bottom line (in characters) where this columnized debug output ends
	int rawColumnizedBottom;        // Bottom line (in pixels) where this columnized debug output ends
	int columnizedMaxLines;         // Total number of lines each column may have (-1 = unlimited)
	int columnizedLineCount;        // Total number of lines in the current column
	int columnizedCharOffset;       // Number of characters to move right by when auto-moving to a new column
	int currentColumn;              // 0-based index of the current column
	int debugOutputX;
	int debugOutputY;
	int rawDebugOutputY;
};

PerThreadState::PerThreadState()
{
	Init();
}

void PerThreadState::Init()
{
	sysMemSet(this, 0, sizeof(*this));

	framesToLive = 1;

	doDebugZTest = true;
	columnizedTop = -1;

	debugOutputX = DEBUG_OUTPUT_XSTART;
	debugOutputY = DEBUG_OUTPUT_YSTART;
}

static __THREAD PerThreadState *s_PerThreadState/*=NULL*/;

static inline PerThreadState *GetPerThreadState()
{
	PerThreadState *pts = s_PerThreadState;
	if (Likely(pts))
	{
		return pts;
	}

#	if __ASSERT
		// There is no thread cleanup, so if debug drawing is used from transient threads, this will leak.
		// Assert to make sure no one is doing that.
		static volatile u32 s_NumPerThreadStateStructs/*=0*/;
		const u32 numPerThreadStateStructs = sysInterlockedIncrement(&s_NumPerThreadStateStructs);
		Assert(numPerThreadStateStructs < 16);
#	endif

	pts = rage_new PerThreadState;
	s_PerThreadState = pts;
	return pts;
}


// PTSC_PRIVATE_FLAGS
template<unsigned X> struct LeastSignificantBit  { enum { value = X&-(signed)X }; };
template<unsigned X> struct MostSignificantBit   { enum { value = 1<<CompileTimeLog2Floor<X>::value }; };
template<unsigned X> struct AreSetBitsContiguous { enum { value = X+LeastSignificantBit<X>::value == MostSignificantBit<X>::value<<1 }; };
enum
{
	RELOCATABLE_STRING  = LeastSignificantBit<grcDebugDraw::PTSC_PRIVATE_FLAGS>::value<<0,
	SCROLLABLE_STRING   = LeastSignificantBit<grcDebugDraw::PTSC_PRIVATE_FLAGS>::value<<1,
};

enum CustomStringCallbackMode
{
	CSCM_INVALID,
	CSCM_COLLATE,
	CSCM_RENDER,
};

static CustomStringCallbackMode s_CustomStringCallbackMode = CSCM_INVALID;
static grcBatcher *s_CustomStringBatcher[2];
static unsigned s_CustomStringBatcherDst;
static u32 s_CustomStringLastY, s_CustomStringLastRawY;
static u32 s_CustomStringOffsetY, s_CustomStringOffsetRawY;
static s32 s_CustomStringScrollOffset;
static s32 s_CustomStringScrollOffsetRaw;
static s32 s_CustomStringScrollLine;


namespace Vec 
{
	__forceinline Vector_4V_Out _V3FindMinAbsAxis(Vector_4V_In v) // nice vector-friendly implementation
	{
		const Vector_4V xyz           = V4Abs               (v);
		const Vector_4V yzx           = V4Permute<Y,Z,X,W>  (xyz);
		const Vector_4V zxy           = V4Permute<Z,X,Y,W>  (xyz);
		const Vector_4V xyz_CmpLT_yzx = V4IsLessThanV       (xyz, yzx);
		const Vector_4V xyz_CmpLE_zxy = V4IsLessThanOrEqualV(xyz, zxy);
		const Vector_4V a             = V4And               (xyz_CmpLT_yzx, xyz_CmpLE_zxy);
		const Vector_4V ax            = V4SplatX            (a);
		const Vector_4V ay            = V4SplatY            (a);
		const Vector_4V az            = V4SplatZ            (a);
		const Vector_4V axy           = V4Or                (ax, ay);
		const Vector_4V axyz          = V4Or                (axy, az); // could use si_orx(a) on SPU
		const Vector_4V mask          = V4Andc              (V4VConstant(V_MASKX), axyz); // 0xffffffff in x component iff a.xyz = 0,0,0
		const Vector_4V b             = V4Or                (a, mask);
		const Vector_4V result        = V4And               (b, V4VConstant(V_ONE));

		return result;
	}
} // namespace Vec

__forceinline Vec3V_Out FindMinAbsAxis(Vec3V_In v)
{
	return Vec3V(Vec::_V3FindMinAbsAxis(v.GetIntrin128ConstRef()));
}

inline void GetOrthoBasisFromDirectionZ(Vec3V_In directionZ, Vec3V_InOut basisX, Vec3V_InOut basisY, Vec3V_InOut basisZ)
{
	basisZ = Normalize(directionZ);
	basisX = Normalize(Cross(basisZ, FindMinAbsAxis(basisZ)));
	basisY = Normalize(Cross(basisX, basisZ));
}

static Vec3V s_CameraPosition;
static const grcViewport *s_CullingViewport;

static bool s_FrozenMode = false;
static u32 s_TooManyBatchersTimestamp = 0xffff0000;
static bool im3DDisplayUsage = false;
static fiStream *s_DebugTextStream = NULL;		// If not NULL, we'll save all debug text to a file for a frame

static u32 imUsageLast[grcDebugDraw::MAX_LINGER_BUFFERS] = { 0 };
static u32 imUsageMin[grcDebugDraw::MAX_LINGER_BUFFERS];
static u32 imUsageMax[grcDebugDraw::MAX_LINGER_BUFFERS] = { 0 } ;
static u32 imUsageAcc[grcDebugDraw::MAX_LINGER_BUFFERS] = { 0 };
static u32 im3DUsageHit = 0;

static u32 wantSubRenderThreadDebugDraw[grcDebugDraw::MAX_LINGER_BUFFERS] = {0};
static bool expectingSubRenderThreadDebugDraw[grcDebugDraw::MAX_LINGER_BUFFERS] = {false};

#if __PS3 || __XENON
static float im3DLineWidth = 1.75f;
#endif // __PS3 || __XENON

static bool im2DDisplayUsage = false;
static u32 im2DUsageHit = 0;

#if __PS3 || __XENON
static float im2DLineWidth = 1.75f;
#endif // __PS3 || __XENON

float grcDebugDraw::g_textDist3DNear = 0.0f;
float grcDebugDraw::g_textDist3DFar = 8192.0f;

Vector2 grcDebugDraw::g_aspect2D;

bool  grcDebugDraw::g_allowScriptDebugDraw = true;

// Global bank toggles that combine with the per thread state.
bool grcDebugDraw::g_doDebugZTest = true;
static bool g_drawAdditively = false;

// Standard implementation
struct RageTextRenderer : public grcDebugDraw::TextRenderer
{
	RageTextRenderer() : m_iPropCharHeight(16), m_iFixedCharHeight(16) { }
	virtual void Begin();
	virtual void RenderText(Color32 color, int xPos, int yPos, const char *string, bool proportional, bool drawBGQuad, bool rawCoords);
	virtual void End();
	virtual int GetTextHeight(bool proportional) const;

private:
	grcViewport *m_oldVP;
	float m_scaleX, m_scaleY;
	int m_iPropCharHeight;
	int m_iFixedCharHeight;
};

static RageTextRenderer g_standardTextRenderer;
grcDebugDraw::TextRenderer *grcDebugDraw::g_textRenderer = &g_standardTextRenderer;

int grcDebugDraw::g_PushRasterizerStateCalls   = 0;
int grcDebugDraw::g_PushBlendStateCalls        = 0;
int grcDebugDraw::g_PushDepthStencilStateCalls = 0;
int grcDebugDraw::g_PushLightingStateCalls     = 0;

const Color32 grcDebugDraw::g_debugCols[grcDebugDraw::NUM_DEBUG_COLORS] = {
	Color32(1.0f, 0.0f, 0.0f, 1.0f), Color32(0.0f, 1.0f, 0.0f, 1.0f), Color32(0.0f, 0.0f, 1.0f, 1.0f), 
	Color32(0.7f, 0.7f, 0.0f, 1.0f), Color32(0.0f, 0.7f, 0.7f, 1.0f), Color32(0.7f, 0.0f, 0.7f, 1.0f), 
	Color32(0.8f, 0.5f, 0.3f, 1.0f), Color32(0.0f, 0.8f, 0.5f, 1.0f), Color32(0.5f, 0.0f, 0.8f, 1.0f), 
	Color32(0.8f, 0.0f, 0.5f, 1.0f), Color32(0.5f, 0.8f, 0.0f, 1.0f), Color32(0.0f, 0.5f, 0.8f, 1.0f), 
	Color32(0.7f, 0.5f, 0.5f, 1.0f), Color32(0.5f, 0.7f, 0.5f, 1.0f), Color32(0.5f, 0.5f, 0.7f, 1.0f), 
	Color32(0.8f, 0.2f, 0.5f, 1.0f), Color32(0.5f, 0.8f, 0.2f, 1.0f), Color32(0.2f, 0.5f, 0.8f, 1.0f), 
	Color32(0.8f, 0.6f, 0.5f, 1.0f), Color32(0.5f, 0.8f, 0.6f, 1.0f), Color32(0.6f, 0.5f, 0.8f, 1.0f), 
	Color32(0.2f, 0.6f, 0.5f, 1.0f), Color32(0.5f, 0.2f, 0.6f, 1.0f), Color32(0.6f, 0.5f, 0.2f, 1.0f), 
	Color32(1.0f, 0.7f, 0.0f, 1.0f), Color32(0.0f, 1.0f, 0.7f, 1.0f), Color32(0.7f, 0.0f, 1.0f, 1.0f), 
	Color32(1.0f, 0.6f, 0.3f, 1.0f), Color32(0.3f, 1.0f, 0.6f, 1.0f), Color32(0.6f, 0.3f, 1.0f, 1.0f), 
	Color32(0.9f, 0.6f, 0.6f, 1.0f), Color32(0.6f, 0.9f, 0.6f, 1.0f), 
	Color32(0.5f, 0.0f, 0.0f, 1.0f), Color32(0.0f, 0.5f, 0.0f, 1.0f), Color32(0.0f, 0.0f, 0.5f, 1.0f), 
	Color32(0.4f, 0.4f, 0.0f, 1.0f), Color32(0.0f, 0.4f, 0.4f, 1.0f), Color32(0.4f, 0.0f, 0.4f, 1.0f), 
	Color32(0.4f, 0.3f, 0.3f, 1.0f), Color32(0.0f, 0.4f, 0.3f, 1.0f), Color32(0.3f, 0.0f, 0.4f, 1.0f), 
	Color32(0.4f, 0.0f, 0.3f, 1.0f), Color32(0.3f, 0.4f, 0.0f, 1.0f), Color32(0.0f, 0.3f, 0.4f, 1.0f), 
	Color32(0.4f, 0.3f, 0.3f, 1.0f), Color32(0.3f, 0.4f, 0.3f, 1.0f), Color32(0.3f, 0.3f, 0.4f, 1.0f), 
	Color32(0.4f, 0.1f, 0.3f, 1.0f), Color32(0.3f, 0.4f, 0.1f, 1.0f), Color32(0.1f, 0.3f, 0.4f, 1.0f), 
	Color32(0.4f, 0.3f, 0.2f, 1.0f), Color32(0.2f, 0.4f, 0.3f, 1.0f), Color32(0.3f, 0.2f, 0.4f, 1.0f), 
	Color32(0.1f, 0.3f, 0.3f, 1.0f), Color32(0.3f, 0.1f, 0.3f, 1.0f), Color32(0.3f, 0.3f, 0.1f, 1.0f), 
	Color32(0.5f, 0.4f, 0.0f, 1.0f), Color32(0.0f, 0.5f, 0.4f, 1.0f), Color32(0.4f, 0.0f, 0.5f, 1.0f), 
	Color32(0.5f, 0.3f, 0.2f, 1.0f), Color32(0.2f, 0.5f, 0.3f, 1.0f), Color32(0.3f, 0.2f, 0.5f, 1.0f), 
	Color32(0.5f, 0.3f, 0.3f, 1.0f), Color32(0.3f, 0.5f, 0.3f, 1.0f), 
};

static bool sm_displayDebugText;
static bool sm_initialized = false;
static grcDepthStencilStateHandle sm_DepthTestOnDS;
static grcDepthStencilStateHandle sm_DepthTestOffDS;
static grcDepthStencilStateHandle sm_DepthTestOnStencilOnDS;
static grcDepthStencilStateHandle sm_DepthTestReverseOnStencilOnDS;

static grcBlendStateHandle sm_AdditiveBS;
static grcBlendStateHandle sm_NormalBS;


/** PURPOSE: Write a line of debug text to the debug output stream, if there is one.
 */
static void WriteToDebugStream(const char *debugString)
{
	if (s_DebugTextStream)
	{
		s_DebugTextStream->Write(debugString, (int)strlen(debugString));
		s_DebugTextStream->Write("\r\n", 2);
	}
}

/** PURPOSE: Returns true if this line is visible in the primary viewport, false otherwise.
 *  The culling will be done using the viewport provided with SetCullingViewport(). if that function
 *  had never been called (or most recently be called with NULL), this function will always return
 *  true.
 */
static bool IsLineVisible(Vec3V_In p1, Vec3V_In p2)
{
	// If no culling viewport was specified, we cannot perform any frustum culling.
	if (!s_CullingViewport)
	{
		return true;
	}

	const Vec3V minP = Min(p1, p2);
	const Vec3V maxP = Max(p1, p2);

	return grcViewport::IsAABBVisible(VEC3V_TO_VECTOR3(minP), VEC3V_TO_VECTOR3(maxP), s_CullingViewport->GetCullLocalLRTB()) != 0;
}

void grcDebugDraw::SetMemoryAllocationInterface(MemoryAllocationInterface *memoryAllocationInterface)
{
	s_MemoryAllocationInterface = memoryAllocationInterface;
}


//////////////////////////////////////////////////////////////////////////////////
// SYSTEM		 
//////////////////////////////////////////////////////////////////////////////////

void grcDebugDraw::Init(grcDepthStencilStateHandle depthTestOnOverride)
{
	Assertf(s_MemoryAllocationInterface, "Designate an allocation interface before calling Init()");
	s_MemoryAllocationInterface->Init();

	for (int x=0; x<MAX_LINGER_BUFFERS; x++) {
		imUsageMin[x] = 0xffffffff;
	}

	g_aspect2D.x = 1.0f;
	g_aspect2D.y = ((float)GRCDEVICE.GetWidth()) / ((float)GRCDEVICE.GetHeight());

	sm_displayDebugText = false;

	if(PARAM_nodebugtxt.Get())
	{
		sm_displayDebugText = false;
	}

	grcDepthStencilStateDesc DSDesc;

	sm_DepthTestOnDS = depthTestOnOverride;
	if( sm_DepthTestOnDS == grcStateBlock::DSS_Invalid )
	{
		DSDesc.DepthFunc = rage::FixupDepthDirection(grcRSV::CMP_LESSEQUAL);
		DSDesc.DepthWriteMask = true;
		DSDesc.DepthEnable = true;
		sm_DepthTestOnDS = grcStateBlock::CreateDepthStencilState(DSDesc);
	}

	DSDesc.DepthFunc = rage::FixupDepthDirection(grcRSV::CMP_LESSEQUAL);
	DSDesc.DepthWriteMask = true;
	DSDesc.DepthEnable = true;
	DSDesc.StencilEnable = true;
	DSDesc.StencilReadMask = 0xFF;
	DSDesc.StencilWriteMask = 0xFF;
	DSDesc.FrontFace.StencilFunc = grcRSV::CMP_NOTEQUAL;
	DSDesc.FrontFace.StencilPassOp = grcRSV::STENCILOP_REPLACE;
	DSDesc.BackFace = DSDesc.FrontFace;
	sm_DepthTestOnStencilOnDS = grcStateBlock::CreateDepthStencilState(DSDesc);

	DSDesc.DepthFunc = rage::FixupDepthDirection(grcRSV::CMP_GREATEREQUAL);
	DSDesc.DepthWriteMask = true;
	DSDesc.DepthEnable = true;
	DSDesc.StencilEnable = true;
	DSDesc.StencilReadMask = 0xFF;
	DSDesc.StencilWriteMask = 0x00;
	DSDesc.FrontFace.StencilFunc = grcRSV::CMP_NOTEQUAL;
	DSDesc.FrontFace.StencilPassOp = grcRSV::STENCILOP_REPLACE;
	DSDesc.BackFace = DSDesc.FrontFace;
	sm_DepthTestReverseOnStencilOnDS = grcStateBlock::CreateDepthStencilState(DSDesc);
	
	DSDesc.DepthFunc = grcRSV::CMP_LESSEQUAL;
	DSDesc.DepthWriteMask = false;
	DSDesc.DepthEnable = false;
	sm_DepthTestOffDS = grcStateBlock::CreateDepthStencilState(DSDesc);
	
	
	grcBlendStateDesc BSDesc;
	BSDesc.BlendRTDesc[0].BlendEnable = 1;
	
	BSDesc.BlendRTDesc[0].DestBlend = grcRSV::BLEND_ONE;
	BSDesc.BlendRTDesc[0].SrcBlend = grcRSV::BLEND_ONE;
	BSDesc.BlendRTDesc[0].BlendOp = grcRSV::BLENDOP_ADD;
	
	sm_AdditiveBS = grcStateBlock::CreateBlendState(BSDesc);

	BSDesc.BlendRTDesc[0].DestBlend = grcRSV::BLEND_INVSRCALPHA;
	BSDesc.BlendRTDesc[0].SrcBlend = grcRSV::BLEND_SRCALPHA;
	BSDesc.BlendRTDesc[0].BlendOp = grcRSV::BLENDOP_ADD;
	
	sm_NormalBS = grcStateBlock::CreateBlendState(BSDesc);

	s_BatcherLockMutexes = rage_new BatcherLockMutexes;
	for (unsigned i=0; i<MAX_LINGER_BUFFERS; ++i)
	{
		s_BatcherRing[i] = rage_new BatcherRing(256);
		for (unsigned j=0; j<NELEM(s_LingerBatcher[i]); ++j)
		{
			grcBatcher *const batcher = rage_new grcBatcher;
			s_LingerBatcher[i][j] = batcher;
			batcher->Init(BufferAllocCallback, BufferFreeCallback);
		}
	}

	for (unsigned i=0; i<NELEM(s_CustomStringBatcher); ++i)
	{
		grcBatcher *const batcher = rage_new grcBatcher;
		s_CustomStringBatcher[i] = batcher;
		batcher->Init(BufferAllocCallback, BufferFreeCallback);
	}

	sm_initialized = true;
}

void grcDebugDraw::Shutdown()
{
	sm_initialized = false;

	for (unsigned i=0; i<MAX_LINGER_BUFFERS; ++i)
	{
		delete s_BatcherRing[i];
		s_BatcherRing[i] = NULL;
		for (unsigned j=0; j<NELEM(s_LingerBatcher[i]); ++j)
		{
			delete s_LingerBatcher[i][j];
			s_LingerBatcher[i][j] = NULL;
		}
	}
	delete s_BatcherLockMutexes;
	s_BatcherLockMutexes = NULL;

	for (unsigned i=0; i<NELEM(s_CustomStringBatcher); ++i)
	{
		delete s_CustomStringBatcher[i];
		s_CustomStringBatcher[i] = NULL;
	}
}

void grcDebugDraw::SetFrozenMode(bool frozenMode)
{
	s_FrozenMode = frozenMode;
}

/** PURPOSE: This must be called at the beginning of the game update as it
 *  clears the bathed rendering calls.  we do this instead of clearing
 *  them at render time as the render may occur at varying points in the
 *  game update which then causes some some batched render calls to
 *  flicker or double up.
 */
void grcDebugDraw::Update()
{
	for (unsigned i=0; i<MAX_LINGER_BUFFERS; ++i)
	{
		u32 want = wantSubRenderThreadDebugDraw[i];
		if (want)
		{
			expectingSubRenderThreadDebugDraw[i] = true;
			wantSubRenderThreadDebugDraw[i] = want-1;
		}
		else
		{
			expectingSubRenderThreadDebugDraw[i] = false;
		}
	}

	if(ioMapper::DebugKeyPressed(KEY_B))
	{
		SetDisplayDebugText(!GetDisplayDebugText());
	}

	// move up and down a page
	const int debugCharHeight = Max(g_textRenderer->GetTextHeight(false), g_textRenderer->GetTextHeight(true));
	const int screenHeight = GRCDEVICE.GetHeight();
	const s32 linesPerPage = (screenHeight / debugCharHeight) - (DEBUG_OUTPUT_XSTART + 3);	// -3 to make space for page number and debug bar at bottom of screen 
	if (ioMapper::DebugKeyPressed(KEY_ADD))
	{
		s_CustomStringScrollLine += linesPerPage;
	}
	if (ioMapper::DebugKeyPressed(KEY_SUBTRACT))
	{
		s_CustomStringScrollLine = Max(s_CustomStringScrollLine - linesPerPage, 0);
	}
}

const u32 *grcDebugDraw::GetCurrentBatcherCursor()
{
	FastAssert(grcBatcher::GetCurrent());
	return grcBatcher::GetCurrent()->GetCursor();
}

static inline int ReduceFramesToLive(int framesToLive)
{
	if (framesToLive > 1)
	{
		return framesToLive - 1;
	}
	// Negative framesToLive is unchanged when frozen, otherwise it counts upwards towards zero.
	else if (framesToLive < 0)
	{
		if (s_FrozenMode)
		{
			return framesToLive;
		}
		else if (framesToLive < -1)
		{
			return framesToLive+1;
		}
	}

	return 0;
}

// Currently there really only needs to be one s_CopyToBatcherWhileRendering, as
// Render2D and Render3D are called from the same drawlist.  But relying on that
// is fragile, and this is just debug code anyways, so index per subrender
// thread.
static grcBatcher *s_CopyToBatcherWhileRendering[NUMBER_OF_RENDER_THREADS];

struct SetFramesToLiveData
{
	int framesToLive;
};

static void SetFramesToLiveCallback(const void *data_, size_t size, grcBatcher *batcher);

static void AddSetFramesToLiveCallback(grcBatcher *batcher, int framesToLive)
{
	SetFramesToLiveData *const data = batcher->AddRenderCallback<SetFramesToLiveData>(SetFramesToLiveCallback, grcBatcher::ARC_NEVER_COPY);
	if (data)
	{
		data->framesToLive = framesToLive;
	}
}

static void SetFramesToLiveCallback(const void *data_, size_t size, grcBatcher *batcher)
{
	const SetFramesToLiveData *const data = (SetFramesToLiveData*)data_;
	(void)size;

	const int framesToLive = ReduceFramesToLive(data->framesToLive);
	if (framesToLive)
	{
		grcBatcher *const dst = s_CopyToBatcherWhileRendering[g_RenderThreadIndex];
		AddSetFramesToLiveCallback(dst, framesToLive);
		batcher->CopyToBatcherWhileRendering(dst);
	}
	else
	{
		batcher->CopyToBatcherWhileRendering(NULL);
	}
}

/** PURPOSE: This will mark the current position in the IM batcher for the
 *  given mode (as defined in the buffer parameter).
 *  It will also set the batcher for that particular mode as the current batcher.
 *  This information will be used in EndRecording to record all draw commands
 *  that had been issued between StartRecording and EndRecording, if need be.
 *
 *  Note that these calls cannot be nested. EndRecording must be called before
 *  StartRecording can be called again.
 *
 * PARAMS:
 *   buffer - The type of IM batcher to record.
 *   framesToLive - Number of frames that everything that had been called since StartRecording
 *                  is supposed to stick around for. A value of 1 will
 *                  cause this function to not record anything, a negative number means that
 *                  it will be visible for that (positive) number of frames, but will remain
 *                  visible while frozen mode is enabled.
 */
bool grcDebugDraw::StartRecording(LingerBuffer buffer, int framesToLive)
{
	Assertf(grcBatcher::GetCurrent() == NULL, "Nested batcher recordings are not supported.");

	PerThreadState *const pts = GetPerThreadState();
	BatcherRing::Handle batcherHandle = pts->batcher[buffer];
	grcBatcher *batcher;

	// (Sub-)render threads use a slightly different batcher allocation scheme.
	// The batcher handle should have been pre-allocated by the main thread when
	// building the drawlists.  Locking the handle should never fail, since the
	// subrender thread that is doing the actual rendering of the batched debug
	// drawing should wait.
	if (g_IsSubRenderThread)
	{
		wantSubRenderThreadDebugDraw[buffer] = WANT_SUB_RENDER_THREAD_DEBUG_DRAW_COUNT;
		if (!batcherHandle)
		{
			Assert(!expectingSubRenderThreadDebugDraw[buffer]);
			return false;
		}
		Assert(expectingSubRenderThreadDebugDraw[buffer]);
		batcher = s_BatcherRing[buffer]->LockForWrite(batcherHandle);
		Assert(batcher);
	}
	else
	{
		for (;;)
		{
			// Allocate a batcher handle if we don't have one already.
			if (!batcherHandle)
			{
				// If there is no batchers available, silently handle this for
				// now.  Maybe want to assert?
				batcherHandle = s_BatcherRing[buffer]->GetAndIncNextWrite();
				if (!batcherHandle)
				{
					return false;
				}
			}

			// Attempt to lock the batcher.  This can fail if the batcher has
			// already been rendered.  For most threads this is not an issue,
			// but a thread that is not synchronized with the render threads may
			// encounter this.
			batcher = s_BatcherRing[buffer]->LockForWrite(batcherHandle);
			if (batcher)
			{
				pts->batcher[buffer] = batcherHandle;
				break;
			}
			else
			{
				// Batcher lock failed, that means the batcher handle is stale.
				// Clear it so that we allocate a new one.
				batcherHandle = 0;
			}
		}
	}

	Assert(batcher);
	grcBatcher::SetCurrent(batcher);

	if (pts->framesToLive != framesToLive)
	{
		pts->framesToLive = framesToLive;
		AddSetFramesToLiveCallback(batcher, framesToLive);
	}

	return true;
}

/** PURPOSE: This is the matching call for StartRecording. It will set the
 *  current batcher back to NULL, and if framesToLive is greater than 1, every
 *  draw command that has been issued since StartRecording will be stored in a
 *  buffer and reissued during the next frame.
 *
 * PARAMS:
 *   buffer - The type of IM batcher to record. This must match what was passed into
 *            StartRecording earlier.
 */
void grcDebugDraw::EndRecording(LingerBuffer buffer)
{
	const BatcherRing::Handle batcherHandle = GetPerThreadState()->batcher[buffer];
	Assert(batcherHandle);

	s_BatcherRing[buffer]->UnlockForWrite(batcherHandle);

	grcBatcher::SetCurrent(NULL);
}

/** PURPOSE: Add all the RAG widgets for the debug draw system.
 */
void grcDebugDraw::AddWidgets(bkBank &bank)
{
	bank.PushGroup("Rendering");

	bank.AddToggle("Allow script debug draw", &g_allowScriptDebugDraw);
	bank.AddToggle("Do Debug Z Test", &g_doDebugZTest);
	bank.AddToggle("Draw Additively", &g_drawAdditively);

#if __PS3 || __XENON
	bank.AddSlider("3D Line width", &im3DLineWidth, 1.0f/8.0f, 511.0f/8.0f, 1.0f/8.0f);
#endif // __PS3 || __XENON
	bank.AddToggle("Display im3DBatcher usage", &im3DDisplayUsage);
#if __PS3 || __XENON
	bank.AddSlider("2D Line width", &im2DLineWidth, 1.0f/8.0f, 511.0f/8.0f, 1.0f/8.0f);
#endif // __PS3 || __XENON
	bank.AddToggle("Display im2DBatcher usage", &im2DDisplayUsage);

	bank.AddSlider("3d text dist Near", &g_textDist3DNear, 0.0f, 9000.0f, 1.0f );
	bank.AddSlider("3d text dist Far", &g_textDist3DFar, 0.0f, 20000.0f, 1.0f );

	bank.AddToggle("Debug Text", &sm_displayDebugText);

	bank.PopGroup();
}

struct PrimitiveStats
{
	u32 line2d;
	u32 quad2d;
	u32 poly2d;
	u32 circle2d;
	u32 arc2d;
	u32 text2d;
	u32 line3d;
	u32 quad3d;
	u32 poly3d;
	u32 circle3d;
	u32 arc3d;
	u32 sphere3d;
	u32 ellipsoidal3d;
	u32 box3d;
	u32 axis3d;
	u32 arrow3d;
	u32 capsule3d;
};

// Similarly to s_CopyToBatcherWhileRendering, having s_CollectedStats per
// subrender thread is not strictly necissary at the moment, but does make code
// less fragile to future changes.
static PrimitiveStats s_CollectedStats[NUMBER_OF_RENDER_THREADS];

struct IncStatData
{
	u32 offset;
};

static void IncStatCallback(const void *data_, size_t size, grcBatcher *batcher)
{
	const IncStatData *const data = (IncStatData*)data_;
	(void)size;
	(void)batcher;
	++(*(u32*)((uptr)(s_CollectedStats+g_RenderThreadIndex) + data->offset));
}

static void IncStat(size_t offset)
{
	grcBatcher *const batcher = grcBatcher::GetCurrent();
	Assert(batcher);
	IncStatData *const data = batcher->AddRenderCallback<IncStatData>(IncStatCallback);
	if (data)
	{
		data->offset = (u32)offset;
	}
}

#define INC_STAT(FIELD)     IncStat(offsetof(PrimitiveStats, FIELD))


//////////////////////////////////////////////////////////////////////////
// DEBUG DRAW 2D
//////////////////////////////////////////////////////////////////////////

/** PURPOSE: This function will batch up a 2D line to be drawn.
 *  You'll typically want to call this function from within the main thread.
 *
 * PARAMS:
 *   vec1 - Starting position of the line (-1.0 indicates the top/left, +1.0 the right/bottom).
 *   vec2 - Ending position of the line (-1.0 indicates the top/left, +1.0 the right/bottom).
 *   col - Color of the line.
 *   framesToLive - If this value is greater than one, the line will stick around for the
 *                  specified number of frames.
 */
void grcDebugDraw::Line(Vec2V_In vec1, Vec2V_In vec2, Color32 col, int framesToLive)
{
	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}

	const Vec3V v0(vec1, ScalarV(V_ZERO));
	const Vec3V v1(vec2, ScalarV(V_ZERO));

	grcDrawLine(v0, v1, col);
	// Note: For grcDrawLine we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_2D);
}

void grcDebugDraw::Line(Vec2V_In vec1, Vec2V_In vec2, Color32 color1, Color32 color2, int framesToLive)
{
	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}

	INC_STAT(line2d);

	const Vec3V v0(vec1, ScalarV(V_ZERO));
	const Vec3V v1(vec2, ScalarV(V_ZERO));

	grcDrawLine(VEC3V_TO_VECTOR3(v0), VEC3V_TO_VECTOR3(v1), color1, color2);
	// Note: For Lines we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_2D);
}

void grcDebugDraw::Quad(Vec2V_In vec1, Vec2V_In vec2, Vec2V_In vec3, Vec2V_In vec4, Color32 col, bool solid, int framesToLive)
{
	if (solid)
	{
		Poly(vec1, vec2, vec3, col, true, framesToLive);
		Poly(vec1, vec3, vec4, col, true, framesToLive);
		return;
	}

	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}

	INC_STAT(quad2d);

	const Vec3V v0(vec1, ScalarV(V_ZERO));
	const Vec3V v1(vec2, ScalarV(V_ZERO));
	const Vec3V v2(vec3, ScalarV(V_ZERO));
	const Vec3V v3(vec4, ScalarV(V_ZERO));

	grcDraw2dQuad(VEC3V_TO_VECTOR3(v0), VEC3V_TO_VECTOR3(v1), VEC3V_TO_VECTOR3(v2), VEC3V_TO_VECTOR3(v3), col);
	// Note: For Lines we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_2D);
}


void grcDebugDraw::Poly(Vec2V_In p1, Vec2V_In p2, Vec2V_In p3, const Color32 col, bool solid, int framesToLive)
{
	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}

	INC_STAT(poly2d);

	Vec3V points[3];
	points[0] = Vec3V(p1, ScalarV(V_ZERO));
	points[1] = Vec3V(p2, ScalarV(V_ZERO));
	points[2] = Vec3V(p3, ScalarV(V_ZERO));

	grcDrawPolygon((Vector3 *) points, 3, NULL, solid, col);
	// Note: For grcDrawPolygon we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_2D);
}


void grcDebugDraw::Poly(Vec2V_In v1, Vec2V_In v2, Vec2V_In v3, const Color32 color1, const Color32 color2, const Color32 color3, bool solid, int framesToLive)
{
	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}

	INC_STAT(poly2d);

	Vec3V points[3];
	points[0] = Vec3V(v1, ScalarV(V_ZERO));
	points[1] = Vec3V(v2, ScalarV(V_ZERO));
	points[2] = Vec3V(v3, ScalarV(V_ZERO));

	Color32 color[3];
	color[0] = color1;
	color[1] = color2;
	color[2] = color3;

	grcDrawPolygon((Vector3 *) points, 3, NULL, solid, color);
	// Note: For grcDrawPolygon we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.

	EndRecording(LINGER_2D);
}


void grcDebugDraw::Cross(Vec2V_In pos, const float &size, const Color32 col, int framesToLive)
{
	const ScalarV sizeV = ScalarVFromF32(size);
	const Vec2V vXDir = Vec2V(V_X_AXIS_WZERO)*sizeV;
	const Vec2V vYDir = Vec2V(V_Y_AXIS_WZERO)*sizeV;

	Line(pos - vYDir, pos + vYDir, col, framesToLive);
	Line(pos - vXDir, pos + vXDir, col, framesToLive);
}


// When correctAspect is true the circle is drawn as a narrowed ellipse in
// screen 0 to 1 space, but at the original screen 0 to 1 space center.  This
// causes the figure to appear as a perfect circle.
void grcDebugDraw::Circle(Vec2V_In center, float radius, Color32 col, bool solid, int numSides, bool correctAspect, int framesToLive)
{
	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}

	const Vec3V centre3(center, ScalarV(V_ZERO));
	const Vec3V xAxis((correctAspect)?(1.0f / g_aspect2D.y):1.0f, 0.0f, 0.0f);
	const Vec3V yAxis(V_Y_AXIS_WZERO);

	INC_STAT(circle2d);

	grcColor(col);
	grcDrawCircle(radius, VEC3V_TO_VECTOR3(centre3), VEC3V_TO_VECTOR3(xAxis), VEC3V_TO_VECTOR3(yAxis), numSides, false, solid);
	// Note: For grcDrawCircle we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_2D);
}

// Draws an arc
void grcDebugDraw::Arc(Vec2V_In center, float radius, const Color32 col, float fBeginAngle, float fEndAngle, bool solid, int numSides, bool correctAspect, int framesToLive)
{
	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}

	const Vec3V centre3(center, ScalarV(V_ZERO));
	const Vec3V xAxis((correctAspect)?(1.0f / g_aspect2D.y):1.0f, 0.0f, 0.0f);
	const Vec3V yAxis(V_Y_AXIS_WZERO);

	INC_STAT(arc2d);

	grcColor(col);
	grcDrawArc(radius, VEC3V_TO_VECTOR3(centre3), VEC3V_TO_VECTOR3(xAxis), VEC3V_TO_VECTOR3(yAxis), fBeginAngle, fEndAngle, numSides, solid);

	// Note: For grcDrawArc we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_2D);
}

// Draws a rectangle without rotation based on a given max and min
// which describe the two opposite corners.
void grcDebugDraw::RectAxisAligned(Vec2V_In min, Vec2V_In max, Color32 col, bool solid, int framesToLive)
{
	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}

	INC_STAT(poly2d);

	// Get the other two corners
	const Vec2V maxXminY = GetFromTwo<Vec::X2, Vec::Y1>(min, max);
	const Vec2V minXmaxY = GetFromTwo<Vec::X1, Vec::Y2>(min, max);

	Vec3V points[4];
	points[0] = Vec3V(min, ScalarV(V_ZERO));
	points[1] = Vec3V(maxXminY, ScalarV(V_ZERO));
	points[2] = Vec3V(max, ScalarV(V_ZERO));
	points[3] = Vec3V(minXmaxY, ScalarV(V_ZERO));

	grcDrawPolygon((Vector3 *) points, 4, NULL, solid, col);
	// Note: For grcDrawPolygon we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_2D);
}


// Draws a rectangle rotated to a given angle.  Can be given either a 
// rotation and a translation or an x basis vector, y basis vector, and
// a translation.
void grcDebugDraw::RectOriented(Vec2V_In localMin, Vec2V_In localMax, const float &rotRadians, Vec2V_In  translation, Color32 col, bool solid, int framesToLive)
{
	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}

	INC_STAT(poly2d);

	Vec2V maxXminY;
	Vec2V minXmaxY;	
	Vec2V newMin;
	Vec2V newMax;

	// Rotate all the points.
	ScalarV sinRot;
	ScalarV cosRot;
	SinAndCos(sinRot, cosRot, ScalarVFromF32(rotRadians));

	newMin.SetX((cosRot * localMin.GetX()) - (sinRot * localMin.GetY()));
	newMin.SetY((sinRot * localMin.GetX()) + (cosRot * localMin.GetY()));
	newMax.SetX((cosRot * localMax.GetX()) - (sinRot * localMax.GetY()));
	newMax.SetY((sinRot * localMax.GetX()) + (cosRot * localMax.GetY()));
	maxXminY.SetX((cosRot * localMax.GetX()) - (sinRot * localMin.GetY()));
	maxXminY.SetY((sinRot * localMax.GetX()) + (cosRot * localMin.GetY()));
	minXmaxY.SetX((cosRot * localMin.GetX()) - (sinRot * localMax.GetY()));
	minXmaxY.SetY((sinRot * localMin.GetX()) + (cosRot * localMax.GetY()));

	// Apply the translation.
	newMin = newMin + translation;
	newMax = newMax + translation;
	maxXminY = maxXminY + translation;
	minXmaxY = minXmaxY + translation;

	Vec3V points[4];
	points[0] = Vec3V(newMin, ScalarV(V_ZERO));
	points[1] = Vec3V(maxXminY, ScalarV(V_ZERO));
	points[2] = Vec3V(newMax, ScalarV(V_ZERO));
	points[3] = Vec3V(minXmaxY, ScalarV(V_ZERO));

	grcDrawPolygon((Vector3 *) points, 4, NULL, solid, col);
	// Note: For grcDrawPolygon we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_2D);
}


void grcDebugDraw::RectOriented(Vec2V_In localMin, Vec2V_In localMax, Vec2V_In xBasis, Vec2V_In yBasis, Vec2V_In translation, Color32 col, bool solid, int framesToLive)
{
	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}

	INC_STAT(poly2d);

	Vec2V maxXminY;
	Vec2V minXmaxY;
	Vec2V newMin;
	Vec2V newMax;

	// Apply the basis vectors.
	newMin.SetX((xBasis.GetX() * localMin.GetX()) + (xBasis.GetY() * localMin.GetY()));
	newMin.SetY((yBasis.GetX() * localMin.GetX()) + (yBasis.GetY() * localMin.GetY()));
	newMax.SetX((xBasis.GetX() * localMax.GetX()) + (xBasis.GetY() * localMax.GetY()));
	newMax.SetY((yBasis.GetX() * localMax.GetX()) + (yBasis.GetY() * localMax.GetY()));
	maxXminY.SetX((xBasis.GetX() * localMax.GetX()) + (xBasis.GetY() * localMin.GetY()));
	maxXminY.SetY((yBasis.GetX() * localMax.GetX()) + (yBasis.GetY() * localMin.GetY()));
	minXmaxY.SetX((xBasis.GetX() * localMin.GetX()) + (xBasis.GetY() * localMax.GetY()));
	minXmaxY.SetY((yBasis.GetX() * localMin.GetX()) + (yBasis.GetY() * localMax.GetY()));

	// Apply the  translation.
	newMin = newMin + translation;
	newMax = newMax + translation;
	maxXminY = maxXminY + translation;
	minXmaxY = minXmaxY + translation;

	Vec3V points[4];
	points[0] = Vec3V(newMin, ScalarV(V_ZERO));
	points[1] = Vec3V(maxXminY, ScalarV(V_ZERO));
	points[2] = Vec3V(newMax, ScalarV(V_ZERO));
	points[3] = Vec3V(minXmaxY, ScalarV(V_ZERO));

	grcDrawPolygon((Vector3 *) points, 4, NULL, solid, col);
	// Note: For grcDrawPolygon we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_2D);
}


// 2D version of Axis which simply draws an axis on a given orientation.
// Can be given either a rotation and a translation or an x basis
// vector, y basis vector, and a translation.
void grcDebugDraw::Axis(const float &rotRadians, Vec2V_In translation, const float &scale, int framesToLive)
{
	Vec2V xaxis;
	Vec2V yaxis;

	// Apply the rotation and scaling.
	ScalarV scaleV = ScalarVFromF32(scale);
	ScalarV sinRot, cosRot;
	SinAndCos(sinRot, cosRot, ScalarVFromF32(rotRadians));

	xaxis.SetX(cosRot * scaleV);
	xaxis.SetY(sinRot * scaleV);
	yaxis.SetX(-(sinRot * scaleV));
	yaxis.SetY(cosRot * scaleV);

	// Apply the translation.
	xaxis = xaxis + translation;
	yaxis = yaxis + translation;

	// Draw the x axis as red and the y axis as green.
	Line(translation, xaxis, Color32(255, 0, 0), framesToLive);
	Line(translation, yaxis, Color32(0, 255, 0), framesToLive);
}


void grcDebugDraw::Axis(Vec2V_In xBasis, Vec2V_In yBasis, Vec2V_In translation, const float &scale, int framesToLive)
{
	// Start with x and y axis of the given size.
	Vec2V xaxis;
	Vec2V yaxis;
	ScalarV scaleV = ScalarVFromF32(scale);

	// Apply the basis vectors and scaling.
	xaxis.SetX(xBasis.GetX() * scaleV);
	xaxis.SetY(yBasis.GetX() * scaleV);
	yaxis.SetX(xBasis.GetY() * scaleV);
	yaxis.SetY(yBasis.GetY() * scaleV);

	// Apply the translation.
	xaxis = xaxis + translation;
	yaxis = yaxis + translation;

	// Draw the x axis as red and the y axis as green.
	Line(translation, xaxis, Color32(255, 0, 0), framesToLive);
	Line(translation, yaxis, Color32(0, 255, 0), framesToLive);
}


void grcDebugDraw::Meter(Vec2V_In axisPositionStart, Vec2V_In axisDir, const float &axisLength, const float &endPointWidth, const Color32 axisColor, const char* axisName, int framesToLive)
{
	const Vec2V min = axisPositionStart;
	const Vec2V max = min + (axisDir* ScalarVFromF32(axisLength));
	ScalarV endPointWidthV = ScalarVFromF32(endPointWidth);

	Vec2V axisDirPerp = axisDir;
	axisDirPerp = Rotate(axisDirPerp, ScalarV(V_PI_OVER_TWO));

	Line(min, max, axisColor, axisColor);
	Line(min - (axisDirPerp*endPointWidthV), min+ (axisDirPerp*endPointWidthV), axisColor, axisColor, framesToLive);
	Line(max - (axisDirPerp*endPointWidthV), max+ (axisDirPerp*endPointWidthV), axisColor, axisColor, framesToLive);

	Vec2V vLabelPos = axisPositionStart;
	vLabelPos -= axisDir*ScalarVFromF32(0.05f);

	Text(vLabelPos, axisColor, axisName, true, 1.0f, 1.0f, framesToLive);
}


void grcDebugDraw::MeterValue(Vec2V_In axisPositionStart, Vec2V_In axisDir, const float &axisLength, const float &val, const float &valWidth, const Color32 valColor, bool bFull, int framesToLive)
{
	ScalarV valV = ScalarVFromF32(val);
	ScalarV valWidthV = ScalarVFromF32(valWidth);
	const Vec2V min = axisPositionStart;
	const Vec2V max = min + (axisDir*ScalarVFromF32(axisLength));
	Vec2V axisDirPerp = axisDir;
	axisDirPerp = Rotate(axisDirPerp, ScalarV(V_PI_OVER_TWO));

	Vec2V vValue;
	if (bFull)
	{
		vValue = (max - min) * (valV) + min;
	}
	else
	{
		vValue = (max - min) * (ScalarV(V_HALF)*valV + ScalarV(V_HALF)) + min;
	}

	Line(vValue - axisDirPerp*valWidthV, vValue+ axisDirPerp*valWidthV, valColor, valColor, framesToLive);
}

static void PrintToScreenCoorsInternal(grcBatcher *batcher, const char *pString, const grcFont* font, s32 x, s32 y, Color32 col, u32 flags, int framesToLive);

void grcDebugDraw::Render2D(u32 numBatchers)
{
	if (grcDebugDraw::IsExpectingSubRenderThreadDebugDraw(grcDebugDraw::LINGER_2D))
	{
		grcDebugDraw::ClearSubRenderBatcher(grcDebugDraw::LINGER_2D);
	}

	u32 usageLast = imUsageLast[LINGER_2D];
	imUsageMin[LINGER_2D] = Min(imUsageLast[LINGER_2D], imUsageMin[LINGER_2D]);
	imUsageMax[LINGER_2D] = Max(imUsageLast[LINGER_2D], imUsageMax[LINGER_2D]);
	imUsageAcc[LINGER_2D] += imUsageLast[LINGER_2D];
	imUsageLast[LINGER_2D] = 0;
	im2DUsageHit++;

	grcStateBlock::SetDepthStencilState(sm_DepthTestOffDS);
	grcStateBlock::SetBlendState(g_drawAdditively ? sm_AdditiveBS : sm_NormalBS);

	// Reset the world matrix to make sure the following draw calls start where we expect them to.
	grcWorldIdentity();

#if __PS3
	GCM_STATE(SetLineSmoothEnable, CELL_GCM_TRUE);
	GCM_STATE(SetLineWidth, (u32)(im2DLineWidth*8.0f)); // 6.3 fixed point
#elif __XENON
	GRCDEVICE.GetCurrent()->SetRenderState(D3DRS_HISTENCILENABLE, FALSE);
	GRCDEVICE.GetCurrent()->SetRenderState(D3DRS_HIZENABLE, FALSE);
	GRCDEVICE.GetCurrent()->SetRenderState(D3DRS_LINEWIDTH, *(const u32*)&im2DLineWidth);
#endif // __XENON

	const unsigned rti = g_RenderThreadIndex;
	sysMemSet(s_CollectedStats+rti, 0, sizeof(*s_CollectedStats));

	const bool flushAfterRender = true;

	// Ping-pong buffered strings from last frame, removing any that have
	// expired.  s_CustomStringCallbackMode is then left as CSCM_COLLATE to
	// collate any newly added strings this frame.
	s_CustomStringCallbackMode = CSCM_COLLATE;
	s_CustomStringLastY = s_CustomStringLastRawY = 0;
	s_CustomStringOffsetY = s_CustomStringOffsetRawY = 0;
	const unsigned customStringBatcherDst = s_CustomStringBatcherDst;
	const unsigned customStringBatcherSrc = customStringBatcherDst^1;
	s_CustomStringBatcher[customStringBatcherSrc]->Render(flushAfterRender);
	s_CustomStringOffsetY = s_CustomStringLastY;
	s_CustomStringOffsetRawY = s_CustomStringLastRawY;

	// Render any lingering 2D commands from last frame (does not include text
	// though).
	const unsigned lbf = s_LingerBatcherFrame[LINGER_2D];
	const unsigned nextLbf = lbf^1;
	s_CopyToBatcherWhileRendering[rti] = s_LingerBatcher[LINGER_2D][nextLbf];
	s_LingerBatcher[LINGER_2D][lbf]->Render(flushAfterRender);
	s_LingerBatcherFrame[LINGER_2D] = nextLbf;

	// Render all the standard batchers.
	for (u32 i=0; i<numBatchers; ++i)
	{
		grcBatcher *const batcher = s_BatcherRing[LINGER_2D]->GetNextRead();
		Assert(batcher);
		batcher->Render(flushAfterRender);
		s_BatcherRing[LINGER_2D]->IncNextRead();
		s_CustomStringOffsetY = s_CustomStringLastY;
		s_CustomStringOffsetRawY = s_CustomStringLastRawY;
	}

	// Ensure that copying is disabled.
	s_CopyToBatcherWhileRendering[rti] = NULL;

	// This is a nasty special case.  To get these stats to actually render,
	// need to shove these directly into
	// s_CustomStringBatcher[customStringBatcherDst].
	if (im2DDisplayUsage && GetDisplayDebugText())
	{
		grcBatcher *const batcher = s_CustomStringBatcher[customStringBatcherDst];
		const grcFont *const font = NULL;
		PerThreadState *const pts = GetPerThreadState();
		const s32 x = pts->debugOutputX;
		s32 y = s_CustomStringOffsetY;
		const Color32 color(0.90f,0.90f,0.90f);
		const u32 flags = SCROLLABLE_STRING;
		const int framesToLive = 0;
		char str[128];

		formatf(str, "Im2d Usage %d, Pages:%d", usageLast, numBatchers);
		PrintToScreenCoorsInternal(batcher, str, font, x, y++, color, flags, framesToLive);
		formatf(str, "Min:%d - Max:%d - Avg:%d", imUsageMin[LINGER_2D], imUsageMax[LINGER_2D],
		 (int)((float)imUsageAcc[LINGER_2D]/(float)im2DUsageHit));
		PrintToScreenCoorsInternal(batcher, str, font, x, y++, color, flags, framesToLive);
		formatf(str, "Lines: %d", s_CollectedStats[rti].line2d);
		PrintToScreenCoorsInternal(batcher, str, font, x, y++, color, flags, framesToLive);
		formatf(str, "Quads: %d", s_CollectedStats[rti].quad2d);
		PrintToScreenCoorsInternal(batcher, str, font, x, y++, color, flags, framesToLive);
		formatf(str, "Polys: %d", s_CollectedStats[rti].poly2d);
		PrintToScreenCoorsInternal(batcher, str, font, x, y++, color, flags, framesToLive);
		formatf(str, "Circles: %d", s_CollectedStats[rti].circle2d);
		PrintToScreenCoorsInternal(batcher, str, font, x, y++, color, flags, framesToLive);
		formatf(str, "Arcs: %d", s_CollectedStats[rti].arc2d);
		PrintToScreenCoorsInternal(batcher, str, font, x, y++, color, flags, framesToLive);
		formatf(str, "Text: %d", s_CollectedStats[rti].text2d);
		PrintToScreenCoorsInternal(batcher, str, font, x, y++, color, flags, framesToLive);

		s_CustomStringLastY = y;
	}

	// Render any 2D text.
	g_textRenderer->Begin();
	s_CustomStringCallbackMode = CSCM_RENDER;
	s_CustomStringScrollLine = Min<s32>(s_CustomStringScrollLine, Max<s32>(s_CustomStringLastY-1, 0));
	s_CustomStringScrollOffset = -s_CustomStringScrollLine;
	s_CustomStringScrollOffsetRaw = s_CustomStringScrollOffset * GetScreenSpaceTextHeight();
	s_CustomStringBatcher[customStringBatcherDst]->Render(!flushAfterRender);
	g_textRenderer->End();

	s_CustomStringCallbackMode = CSCM_INVALID;
	s_CustomStringBatcherDst = customStringBatcherSrc;

#if __PS3
	GCM_STATE(SetLineSmoothEnable, CELL_GCM_FALSE);
#endif // __PS3

	grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_Default);
	grcStateBlock::SetBlendState(grcStateBlock::BS_Default);
	grcStateBlock::SetRasterizerState(grcStateBlock::RS_Default);
	
	if (!PARAM_nowarnonexcessivedraw.Get() && TIME.GetFrameCount() - s_TooManyBatchersTimestamp < TOO_MUCH_DEBUG_DRAW_FRAMECOUNT)
	{
		int x, y;
		x = GRCDEVICE.GetWidth() / 10;
		y = GRCDEVICE.GetHeight() / 3;

		g_textRenderer->Begin();
		g_textRenderer->RenderText(Color32(0xffaaaaff), x, y, "Too much debug drawing enabled. Some debug visualization will be missing.", false, true, true);
		g_textRenderer->End();
	}
}


//////////////////////////////////////////////////////////////////////////
// DEBUG DRAW 3D
//////////////////////////////////////////////////////////////////////////

void grcDebugDraw::Line(Vec3V_In vec1, Vec3V_In vec2, const Color32 color1, int framesToLive)
{
	// Frustum culling
	if (!GetPerThreadState()->disableCulling && !IsLineVisible(vec1, vec2))
	{
		return;
	}

	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(line3d);

	grcDrawLine(vec1, vec2, color1);
	// Note: For grcDrawLine we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_3D);
}


void grcDebugDraw::Line(Vec3V_In vec1, Vec3V_In vec2, const Color32 color1, const Color32 color2, int framesToLive)
{
	// Frustum culling
	if (!GetPerThreadState()->disableCulling && !IsLineVisible(vec1, vec2))
	{
		return;
	}

	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(line3d);

	grcDrawLine(VEC3V_TO_VECTOR3(vec1), VEC3V_TO_VECTOR3(vec2), color1, color2);
	// Note: For grcDrawLine we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_3D);
}

void grcDebugDraw::Quad(Vec3V_In vec1, Vec3V_In vec2, Vec3V_In vec3, Vec3V_In vec4, Color32 col, bool drawDoubleSided, bool solid, int framesToLive)
{
	if (solid)
	{
		Poly(vec1, vec2, vec3, col, drawDoubleSided, solid, framesToLive);
		Poly(vec1, vec3, vec4, col, drawDoubleSided, solid, framesToLive);
		return;
	}

	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(quad3d);

	const Vec3V points[4] = {vec1, vec2, vec3, vec4};
	grcDrawPolygon((const Vector3 *)points, 4, NULL, solid, col);
	// Note: For grcDrawPolygon we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_3D);
}

void grcDebugDraw::Poly(Vec3V_In v1, Vec3V_In v2, Vec3V_In v3, const Color32 col, bool drawDoubleSided, bool solid, int framesToLive)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(line3d);

	Vec3V points[3];
	points[0] = v1;
	points[1] = v2;
	points[2] = v3;

	grcBatcher *batcher = grcBatcher::GetCurrent();

	if(drawDoubleSided)	
	{
		batcher->SetRasterizerState(grcStateBlock::RS_NoBackfaceCull);
	}
	grcDrawPolygon((Vector3 *) points, 3, NULL, solid, col);
	if(drawDoubleSided)	
	{
		batcher->SetRasterizerState(grcStateBlock::RS_Default);
	}
	// Note: For grcDrawPolygon we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_3D);
}


void grcDebugDraw::Poly(Vec3V_In v1, Vec3V_In v2, Vec3V_In v3, const Color32 color1, const Color32 color2, const Color32 color3, bool drawDoubleSided, bool solid, int framesToLive)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(poly3d);

	Vec3V points[3];
	points[0] = v1;
	points[1] = v2;
	points[2] = v3;

	Color32 color[3];
	color[0] = color1;
	color[1] = color2;
	color[2] = color3;

	grcBatcher *batcher = grcBatcher::GetCurrent();

	if(drawDoubleSided)	
	{
		batcher->SetRasterizerState(grcStateBlock::RS_NoBackfaceCull);
	}
	grcDrawPolygon((Vector3 *) points, 3, NULL, solid, color);
	if(drawDoubleSided)	
	{
		batcher->SetRasterizerState(grcStateBlock::RS_Default);
	}
	// Note: For grcDrawPolygon we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_3D);
}


void grcDebugDraw::Cross(Vec3V_In pos, float size, const Color32 col, int framesToLive, bool bZAxis)
{
	const ScalarV sizeV = ScalarVFromF32(size);
	const Vec3V vXDir = Vec3V(V_X_AXIS_WZERO)*sizeV;
	const Vec3V vYDir = Vec3V(V_Y_AXIS_WZERO)*sizeV;

	Line(pos - vYDir, pos + vYDir, col, framesToLive);
	Line(pos - vXDir, pos + vXDir, col, framesToLive);

	if(bZAxis)
	{
		const Vec3V vZDir = Vec3V(V_Z_AXIS_WZERO)*sizeV;
		Line(pos - vZDir, pos + vZDir, col, framesToLive);
	}
}

void grcDebugDraw::Circle(Vec3V_In center, float r, const Color32 col, Vec3V_In vXAxis, Vec3V_In vYAxis, bool dashed, bool solid, int framesToLive, int numsides)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(circle3d);

	grcColor(col);
	grcDrawCircle(r, VEC3V_TO_VECTOR3(center), VEC3V_TO_VECTOR3(vXAxis), VEC3V_TO_VECTOR3(vYAxis), numsides, dashed, solid);

	// Reset the world matrix as the grcDraw___ call immediately above sets it.
	grcWorldIdentity();
	EndRecording(LINGER_3D);
}

void grcDebugDraw::Arc(Vec3V_In center, float r, const Color32 col, Vec3V_In vXAxis, Vec3V_In vYAxis, float fBeginAngle, float fEndAngle, bool solid, int framesToLive )
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(arc3d);

	grcColor(col);
	grcDrawArc(r, VEC3V_TO_VECTOR3(center), VEC3V_TO_VECTOR3(vXAxis), VEC3V_TO_VECTOR3(vYAxis), fBeginAngle, fEndAngle, 20, solid);

	// Reset the world matrix as the grcDraw___ call immediately above sets it.
	grcWorldIdentity();
	EndRecording(LINGER_3D);
}

void grcDebugDraw::Sphere(Vec3V_In center, float r, const Color32 col, bool solid, int framesToLive, int steps, bool longitudinalCircles)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(sphere3d);

	grcColor(col);
	grcDrawSphere(r, center, steps, longitudinalCircles, solid);// Note: This version of grcDrawSphere uses DrawEllipsoid, which sets the world matrix...

	// Reset the world matrix as the grcDraw___ call immediately above sets it.
	grcWorldIdentity();
	EndRecording(LINGER_3D);
}

void grcDebugDraw::Ellipsoidal(Mat34V_In mat, Vec3V_In size, const Color32 col, bool solid, int framesToLive, int steps, bool longitudinalCircles)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(ellipsoidal3d);

	grcColor(col);
	grcDrawEllipsoid(size, mat, steps, longitudinalCircles, solid);

	// Reset the world matrix as the grcDraw___ call immediately above sets it.
	grcWorldIdentity();
	EndRecording(LINGER_3D);
}

void grcDebugDraw::PartialSphere(Vec3V_In center, float r, Vec3V_In direction, float angle, const Color32 col, bool solid, int framesToLive, int steps, bool longitudinalCircles)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(sphere3d);

	grcColor(col);
	grcDrawPartialSphere(center, r, direction, angle, steps, longitudinalCircles, solid);// Note: This version of grcDrawSphere uses DrawEllipsoid, which sets the world matrix...

	// Reset the world matrix as the grcDraw___ call immediately above sets it.
	grcWorldIdentity();
	EndRecording(LINGER_3D);
}


void grcDebugDraw::BoxAxisAligned(Vec3V_In min, Vec3V_In max, const Color32 col, bool solid, int framesToLive)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(box3d);

	if(solid)
	{
		grcDrawSolidBox(VEC3V_TO_VECTOR3(min), VEC3V_TO_VECTOR3(max), col);
	}
	else
	{
		grcDrawBox(VEC3V_TO_VECTOR3(min), VEC3V_TO_VECTOR3(max), col);
	}

	// Reset the world matrix as the grcDraw___ call immediately above sets it.
	grcWorldIdentity();
	EndRecording(LINGER_3D);
}


void grcDebugDraw::BoxOriented(Vec3V_In min, Vec3V_In max, Mat34V_In mat, const Color32 col, bool solid, int framesToLive)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(box3d);

	const Vec3V size(max - min);

	// Shift matrix pos to center of box
	Mat34V matCentred = mat;
	Vec3V vCentrePos = max + min;
	vCentrePos *= ScalarV(V_HALF);
	vCentrePos = Transform(mat, vCentrePos);
	matCentred.SetCol3(vCentrePos);

	if(solid)
	{
		grcDrawSolidBox(VEC3V_TO_VECTOR3(size), MAT34V_TO_MATRIX34(matCentred), col);
	}
	else
	{
		grcDrawBox(VEC3V_TO_VECTOR3(size), MAT34V_TO_MATRIX34(matCentred), col);
	}

	// Reset the world matrix as the grcDraw___ call immediately above sets it.
	grcWorldIdentity();

	EndRecording(LINGER_3D);
}

void grcDebugDraw::CubeOriented(Vec3V_In center, float size, Mat34V_In mat, const Color32 col, bool solid, int framesToLive)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(box3d);

	Mat34V matCentred = mat;
	matCentred.SetCol3(center);

	if(solid)
	{
		grcDrawSolidBox(Vector3(size, size, size), MAT34V_TO_MATRIX34(matCentred), col);
	}
	else
	{
		grcDrawBox(Vector3(size, size, size), MAT34V_TO_MATRIX34(matCentred), col);
	}

	// Reset the world matrix as the grcDraw___ call immediately above sets it.
	grcWorldIdentity();

	EndRecording(LINGER_3D);
}


void grcDebugDraw::Cylinder(Vec3V_In start, Vec3V_In end, float r, const Color32 col, bool cap, bool solid, int numSides, int framesToLive)
{
	Vec3V basisX, basisY, basisZ;
	GetOrthoBasisFromDirectionZ(end - start, basisX, basisY, basisZ);

	Cylinder(start, end, basisX, basisY, basisZ, r, col, cap, solid, numSides, framesToLive);
}


void grcDebugDraw::Cylinder(Vec3V_In start, Vec3V_In end, float r, const Color32 color1, const Color32 color2, bool cap, bool solid, int numSides, int framesToLive)
{
	// Get the directions of the basis vectors.
	Vec3V basisX, basisY, basisZ;
	GetOrthoBasisFromDirectionZ(end - start, basisX, basisY, basisZ);
	ScalarV length = Mag(end - start); 

	// Scale the basis vector according to the length and radius.
	basisX *= ScalarVFromF32(r);
	basisY *= ScalarVFromF32(r);
	basisZ *= length;

	for (int sideIndex = 0; sideIndex < numSides; sideIndex++)
	{
		const float angle0	= (PI * 2.0f) * static_cast<float>(sideIndex) / static_cast<float>(numSides);
		const ScalarV ang0Cos	= ScalarVFromF32(rage::Cosf(angle0));
		const ScalarV ang0Sin	= ScalarVFromF32(rage::Sinf(angle0));
		const Vec3V offsetP0((basisX * ang0Cos) + (basisY * ang0Sin));

		const float angle1	= (PI * 2.0f) * (sideIndex + 1) / numSides;
		const ScalarV ang1Cos	= ScalarVFromF32(rage::Cosf(angle1));
		const ScalarV ang1Sin	= ScalarVFromF32(rage::Sinf(angle1));
		const Vec3V offsetP1((basisX * ang1Cos) + (basisY * ang1Sin));

		const Vec3V bottomP0(start + offsetP0);
		const Vec3V bottomP1(start + offsetP1);

		const Vec3V topP1(end + offsetP1);
		const Vec3V topP0(end + offsetP0);

		if(solid)
		{
			if(cap)
			{
				// Top and bottom pie wedges.
				Poly(start, bottomP0, bottomP1, color1, true, true, framesToLive);
				Poly(end, topP0, topP1, color2, true, true, framesToLive);
			}

			// Side panel.
			Poly(bottomP0, bottomP1, topP1, color1, color1, color2, true, true, framesToLive);
			Poly(bottomP0, topP1, topP0, color1, color2, color2, true, true, framesToLive);
		}
		else
		{
			// Bottom and top edges.
			Line(bottomP0, bottomP1, color1, framesToLive);
			Line(topP0, topP1, color2, framesToLive);

			// Side edge 0 and edge 1.
			Line(bottomP0, topP0, color1, color2, framesToLive);
			Line(bottomP1, topP1, color1, color2, framesToLive);
		}
	}
}

void grcDebugDraw::Cylinder(Vec3V_In start, Vec3V_In end, Vec3V_In basisX, Vec3V_In basisY, Vec3V_In basisZ, float r, const Color32 col, bool cap, bool solid, int numSides, int framesToLive)
{
	ScalarV length = Mag(end - start); 
	Vec3V bX(basisX);
	Vec3V bY(basisY);
	Vec3V bZ(basisZ);
	
	// Scale the basis vector according to the length and radius.
	bX *= ScalarVFromF32(r);
	bY *= ScalarVFromF32(r);
	bZ *= length;

	for (int sideIndex = 0; sideIndex < numSides; sideIndex++)
	{
		const float angle0	= (PI * 2.0f) * static_cast<float>(sideIndex) / static_cast<float>(numSides);
		const ScalarV ang0Cos	= ScalarVFromF32(rage::Cosf(angle0));
		const ScalarV ang0Sin	= ScalarVFromF32(rage::Sinf(angle0));
		const Vec3V offsetP0((bX * ang0Cos) + (bY * ang0Sin));

		const float angle1	= (PI * 2.0f) * (sideIndex + 1) / numSides;
		const ScalarV ang1Cos	= ScalarVFromF32(rage::Cosf(angle1));
		const ScalarV ang1Sin	= ScalarVFromF32(rage::Sinf(angle1));
		const Vec3V offsetP1((bX * ang1Cos) + (bY * ang1Sin));

		const Vec3V bottomP0(start + offsetP0);
		const Vec3V bottomP1(start + offsetP1);

		const Vec3V topP1(end + offsetP1);
		const Vec3V topP0(end + offsetP0);

		if(solid)
		{
			if(cap)
			{
				// Top and bottom pie wedges.
				Poly(start, bottomP0, bottomP1, col, true, true, framesToLive);
				Poly(end, topP0, topP1, col, true, true, framesToLive);
			}

			// Side panel.
			Poly(bottomP0, bottomP1, topP1, col, true, true, framesToLive);
			Poly(bottomP0, topP1, topP0, col, true, true, framesToLive);
		}
		else
		{
			// Bottom and top edges.
			Line(bottomP0, bottomP1, col, framesToLive);
			Line(topP0, topP1, col, framesToLive);

			// Side edge 0 and edge 1.
			Line(bottomP0, topP0, col, framesToLive);
			Line(bottomP1, topP1, col, framesToLive);
		}
	}
}


void grcDebugDraw::Cone(Vec3V_In start, Vec3V_In end, float r, const Color32 col, bool cap, bool solid, int numSides, int framesToLive)
{
	Vec3V basisX, basisY, basisZ;
	GetOrthoBasisFromDirectionZ(end - start, basisX, basisY, basisZ);

	Cone(start, end, basisX, basisY, basisZ, r, col, cap, solid, numSides, framesToLive);
}

void grcDebugDraw::Cone(Vec3V_In start, Vec3V_In end, Vec3V_In basisX, Vec3V_In basisY, Vec3V_In basisZ, float r, const Color32 col, bool cap, bool solid, int numSides, int framesToLive)
{
	ScalarV length = Mag(end - start); 
	Vec3V bX(basisX);
	Vec3V bY(basisY);
	Vec3V bZ(basisZ);

	// Scale the basis vector according to the length and radius.
	bX *= ScalarVFromF32(r);
	bY *= ScalarVFromF32(r);
	bZ *= length;

	for (int sideIndex = 0; sideIndex < numSides; sideIndex++)
	{
		const float angle0	= (PI * 2.0f) * static_cast<float>(sideIndex) / static_cast<float>(numSides);
		const ScalarV ang0Cos	= ScalarVFromF32(rage::Cosf(angle0));
		const ScalarV ang0Sin	= ScalarVFromF32(rage::Sinf(angle0));
		const Vec3V offsetP0((bX * ang0Cos) + (bY * ang0Sin));

		const float angle1	= (PI * 2.0f) * (sideIndex + 1) / numSides;
		const ScalarV ang1Cos	= ScalarVFromF32(rage::Cosf(angle1));
		const ScalarV ang1Sin	= ScalarVFromF32(rage::Sinf(angle1));
		const Vec3V offsetP1((bX * ang1Cos) + (bY * ang1Sin));

		const Vec3V bottomP0(start/* + offsetP0*/);

		const Vec3V topP1(end + offsetP1);
		const Vec3V topP0(end + offsetP0);

		if(solid)
		{
			if(cap)
			{
				// Top pie wedge.
				Poly(end, topP0, topP1, col, true, true, framesToLive);
			}

			// Side panel.
			Poly(bottomP0, topP1, topP0, col, true, true, framesToLive);
		}
		else
		{
			// Top edge.
			Line(topP0, topP1, col, framesToLive);

			// Side edge 0 and edge 1.
			Line(bottomP0, topP0, col, framesToLive);
			Line(bottomP0, topP1, col, framesToLive);
		}
	}
}

void grcDebugDraw::Capsule(Vec3V_In start, Vec3V_In end, float r, const Color32 col, bool solid, int framesToLive)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(capsule3d);

	grcBatcher* batcher = grcBatcher::GetCurrent();
	if (batcher)
	{
		batcher->SaveLightingState();
		batcher->SetLighting(false);
	}
	grcColor(col);
	Vec3V axis;
	axis = Subtract(end,start);
	ScalarV length = Mag(axis);
	ScalarV inverseLength = InvertSafe(length,ScalarV(V_ZERO));
	axis *= inverseLength;	// Normalize
	Mat34V m1(V_IDENTITY);
	m1.SetCol3(AddScaled(start,axis,ScalarV(V_HALF)*length));
	if (IsGreaterThanAll(length,ScalarV(V_ZERO)))
	{
		QuatV direction = QuatVFromVectors(m1.GetCol1(), axis);
		Mat34VFromQuatV(m1, direction, m1.GetCol3());
	}
	grcDrawCapsule(length.Getf(), r, MAT34V_TO_MATRIX34(m1), 6, solid);
	if (batcher)
	{
		batcher->RestoreLightingState();
	}

	// Reset the world matrix as the grcDraw___ call immediately above sets it.
	grcWorldIdentity();
	EndRecording(LINGER_3D);
}

void grcDebugDraw::Axis(Mat34V_In mtx, float scale, bool drawArrows, int framesToLive)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(axis3d);

	grcDrawAxis(scale, MAT34V_TO_MATRIX34(mtx), drawArrows);

	// Reset the world matrix as the grcDraw___ call immediately above sets it.
	grcWorldIdentity();
	EndRecording(LINGER_3D);
}

void grcDebugDraw::Arrow(Vec3V_In vec1, Vec3V_In vec2, float fArrowHeadSize, const Color32 color1, int framesToLive)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	INC_STAT(arrow3d);

	grcDrawLine(vec1, vec2, color1);

	Vec3V vDirection = vec2 - vec1;
	float fDirection = rage::Atan2f(-vDirection.GetXf(), vDirection.GetYf());

	/// Vec3V vFacing = RotateAboutZAxis(Vec3V(V_Y_AXIS_WZERO), ScalarVFromF32(fDirection));

	static float s_fArrowAngle = 3.0f * (PI / 4.0f); 

	Vec3V vArrowHeadL(0.0f,fArrowHeadSize,0.0f);
	vArrowHeadL = RotateAboutZAxis(vArrowHeadL, ScalarVFromF32(fDirection + s_fArrowAngle));

	Vec3V vArrowHeadR(0.0f,fArrowHeadSize,0.0f);
	vArrowHeadR = RotateAboutZAxis(vArrowHeadR, ScalarVFromF32(fDirection - s_fArrowAngle));

	grcDrawLine(vec2,vec2 + vArrowHeadL, color1);
	grcDrawLine(vec2,vec2 + vArrowHeadR, color1);
	// Note: For grcDrawLine we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_3D);
}

void grcDebugDraw::GizmoPosition(Mat34V_In mtx, float scale, int selection)
{
	GizmoPosition_Internal(mtx, scale, false, selection);

	PushDepthStencilState(sm_DepthTestOnStencilOnDS);
	GizmoPosition_Internal(mtx, scale, false, selection);
	PopDepthStencilState();

	PushDepthStencilState(sm_DepthTestReverseOnStencilOnDS);
	GizmoPosition_Internal(mtx, scale, true, selection);
	PopDepthStencilState();

	PushDepthStencilState(sm_DepthTestOnDS);
	PopDepthStencilState();
}

void grcDebugDraw::GizmoRotation(Mat34V_In mtx, float scale, int selection)
{
	GizmoRotation_Internal(mtx, scale, false, selection);

	PushDepthStencilState(sm_DepthTestOnStencilOnDS);
	GizmoRotation_Internal(mtx, scale, false, selection);
	PopDepthStencilState();

	PushDepthStencilState(sm_DepthTestReverseOnStencilOnDS);
	GizmoRotation_Internal(mtx, scale, true, selection);
	PopDepthStencilState();

	PushDepthStencilState(sm_DepthTestOnDS);
	PopDepthStencilState();
}

void grcDebugDraw::GizmoScale(Mat34V_In mtx, float scale, int selection)
{
	GizmoScale_Internal(mtx, scale, false, selection);

	PushDepthStencilState(sm_DepthTestOnStencilOnDS);
	GizmoScale_Internal(mtx, scale, false, selection);
	PopDepthStencilState();

	PushDepthStencilState(sm_DepthTestReverseOnStencilOnDS);
	GizmoScale_Internal(mtx, scale, true, selection);
	PopDepthStencilState();

	PushDepthStencilState(sm_DepthTestOnDS);
	PopDepthStencilState();
}

void grcDebugDraw::GizmoPosition_Internal(Mat34V_In mtx, float scale, bool faded, int selection)
{
	Color32 red(faded?0.33f:1.0f, 0.0f, 0.0f);
	Color32 green(0.0f, faded?0.33f:1.0f, 0.0f);
	Color32 blue(0.0f, 0.0f, faded?0.33f:1.0f);
	Color32 yellow(faded?0.33f:1.0f, faded?0.33f:1.0f, 0.0f);

	ScalarV size(scale);
	Vec3V origin(mtx.d());
	Vec3V right(mtx.a()*size);
	Vec3V front(mtx.b()*size);
	Vec3V up(mtx.c()*size);

	ScalarV third(0.33f);
	ScalarV ninth(0.9f);
	float radius = 0.025f*scale;
	Cylinder(origin, origin + ninth*right, mtx.b(), mtx.c(), mtx.a(), radius,			selection==X_AXIS ? yellow:red);
	Cylinder(origin, origin + ninth*front, mtx.c(), mtx.a(), mtx.b(), radius,			selection==Y_AXIS ? yellow:green);
	Cylinder(origin, origin + ninth*up, mtx.a(), mtx.b(), mtx.c(), radius,				selection==Z_AXIS ? yellow:blue);
	Cone(origin + right, origin + ninth*right, mtx.b(), mtx.c(), mtx.a(), 2.0f*radius,	selection==X_AXIS ? yellow:red);
	Cone(origin + front, origin + ninth*front, mtx.c(), mtx.a(), mtx.b(), 2.0f*radius,	selection==Y_AXIS ? yellow:green);
	Cone(origin + up, origin + ninth*up, mtx.a(), mtx.b(), mtx.c(), 2.0f*radius,		selection==Z_AXIS ? yellow:blue);
	Line(origin + third*right, origin + third*right + third*front,						selection==XY_PLANE ? yellow:red);
	Line(origin + third*right, origin + third*right + third*up,							selection==XZ_PLANE ? yellow:red);
	Line(origin + third*front, origin + third*front + third*right,						selection==XY_PLANE ? yellow:green);
	Line(origin + third*front, origin + third*front + third*up,							selection==ZY_PLANE ? yellow:green);
	Line(origin + third*up, origin + third*up + third*right,							selection==XZ_PLANE ? yellow:blue);
	Line(origin + third*up, origin + third*up + third*front,							selection==ZY_PLANE ? yellow:blue);
}

void grcDebugDraw::GizmoRotation_Internal(Mat34V_In mtx, float scale, bool faded, int selection)
{
	Color32 red(faded?0.33f:1.0f, 0.0f, 0.0f);
	Color32 green(0.0f, faded?0.33f:1.0f, 0.0f);
	Color32 blue(0.0f, 0.0f, faded?0.33f:1.0f);
	Color32 yellow(faded?0.33f:1.0f, faded?0.33f:1.0f, 0.0f);

	ScalarV size(scale);
	Vec3V origin(mtx.d());
	Vec3V right(mtx.a()*size);
	Vec3V front(mtx.b()*size);
	Vec3V up(mtx.c()*size);

	ScalarV width(0.05f);

	Cylinder(origin - width*right, origin + width*right, mtx.b(), mtx.c(), mtx.a(), scale,			selection==ROT_X_PLANE ? yellow:red, false, true, 30);
	Cylinder(origin - width*front, origin + width*front, mtx.c(), mtx.a(), mtx.b(), 0.98f*scale,	selection==ROT_Y_PLANE ? yellow:green, false, true, 30);
	Cylinder(origin - width*up, origin + width*up, mtx.a(), mtx.b(), mtx.c(), 1.02f*scale,			selection==ROT_Z_PLANE ? yellow:blue, false, true, 30);
}

void grcDebugDraw::GizmoScale_Internal(Mat34V_In mtx, float scale, bool faded, int selection)
{
	Color32 red(faded?0.33f:1.0f, 0.0f, 0.0f);
	Color32 green(0.0f, faded?0.33f:1.0f, 0.0f);
	Color32 blue(0.0f, 0.0f, faded?0.33f:1.0f);
	Color32 yellow(faded?0.33f:1.0f, faded?0.33f:1.0f, 0.0f);

	ScalarV size(scale);
	Vec3V origin(mtx.d());
	Vec3V right(mtx.a()*size);
	Vec3V front(mtx.b()*size);
	Vec3V up(mtx.c()*size);

	ScalarV third(0.33f);
	ScalarV ninth(0.9f);
	float radius = 0.025f*scale;
	Cylinder(origin, origin + ninth*right, radius,					selection==X_AXIS ? yellow:red);
	Cylinder(origin, origin + ninth*front, radius,					selection==Y_AXIS ? yellow:green);
	Cylinder(origin, origin + ninth*up, radius,						selection==Z_AXIS ? yellow:blue);
	CubeOriented(origin + ninth*right, 4.0f*radius, mtx,			selection==X_AXIS ? yellow:red);
	CubeOriented(origin + ninth*front, 4.0f*radius, mtx,			selection==Y_AXIS ? yellow:green);
	CubeOriented(origin + ninth*up, 4.0f*radius, mtx,				selection==Z_AXIS ? yellow:blue);
	Line(origin + third*right, origin + third*right + third*front,	selection==XY_PLANE ? yellow:red);
	Line(origin + third*right, origin + third*right + third*up,		selection==XZ_PLANE ? yellow:red);
	Line(origin + third*front, origin + third*front + third*right,	selection==XY_PLANE ? yellow:green);
	Line(origin + third*front, origin + third*front + third*up,		selection==ZY_PLANE ? yellow:green);
	Line(origin + third*up, origin + third*up + third*right,		selection==XZ_PLANE ? yellow:blue);
	Line(origin + third*up, origin + third*up + third*front,		selection==ZY_PLANE ? yellow:blue);
}

void grcDebugDraw::Proxy(const grcDrawProxy& proxy, int framesToLive)
{
	if (!StartRecording(LINGER_3D, framesToLive)) {
		return;
	}

	grcDraw(proxy);

	EndRecording(LINGER_3D);
}

void grcDebugDraw::SetCameraPosition(Vec3V_In camPos)
{
	s_CameraPosition = camPos;
}

void grcDebugDraw::SetCullingViewport(const grcViewport *viewport)
{
	s_CullingViewport = viewport;
}


void grcDebugDraw::SetDefaultDebugRenderStates()
{
	grcWorldIdentity();

	const PerThreadState *const pts = GetPerThreadState();
	grcLightState::SetEnabled(false);
	grcStateBlock::SetBlendState(g_drawAdditively || pts->drawAdditively ? sm_AdditiveBS : sm_NormalBS);
	grcStateBlock::SetDepthStencilState( g_doDebugZTest && pts->doDebugZTest ? sm_DepthTestOnDS : sm_DepthTestOffDS);
	grcStateBlock::SetRasterizerState(grcStateBlock::RS_Default);

#if __PS3
	GCM_STATE(SetLineSmoothEnable, CELL_GCM_TRUE);
	GCM_STATE(SetLineWidth, (u32)(im3DLineWidth*8.0f)); // 6.3 fixed point
#elif __XENON
	GRCDEVICE.GetCurrent()->SetRenderState(D3DRS_HISTENCILENABLE, FALSE);
	GRCDEVICE.GetCurrent()->SetRenderState(D3DRS_HIZENABLE, FALSE);
	GRCDEVICE.GetCurrent()->SetRenderState(D3DRS_LINEWIDTH, *(const u32*)&im3DLineWidth);
#endif // __XENON
}

/** PURPOSE: Call this function from the render thread to discard all the buffers
 *  that were accumulated during the last frame. This will clean out all buffers
 *  like Render2D() and Render3D() would do, without actually rendering anything.
 */
void grcDebugDraw::DiscardRenderBuffers()
{
	// During initial boot, this is called before grcDebugDraw::Init, and
	// changing that would be messy.  Simplest to just safely early out here.
	if (!s_BatcherLockMutexes)
	{
		return;
	}

	for (unsigned i=0; i<MAX_LINGER_BUFFERS; ++i)
	{
		grcBatcher *batcher;
		while ((batcher = s_BatcherRing[i]->GetNextRead()) != NULL)
		{
			batcher->Flush();
			s_BatcherRing[i]->IncNextRead();
		}
		for (unsigned j=0; j<NELEM(s_LingerBatcher[i]); ++j)
		{
			s_LingerBatcher[i][j]->Flush();
		}
	}
}

void grcDebugDraw::CloseBatcher(LingerBuffer buffer)
{
	PerThreadState *const pts = GetPerThreadState();
	const BatcherRing::Handle batcherHandle = pts->batcher[buffer];
	if (batcherHandle)
	{
		grcBatcher *const batcher = s_BatcherRing[buffer]->LockForWrite(batcherHandle);
		if (batcher)
		{
			batcher->Close();
			s_BatcherRing[buffer]->UnlockForWrite(batcherHandle);
		}
		pts->batcher[buffer] = 0;
	}
}

u32 grcDebugDraw::GetNumBatchers(LingerBuffer buffer)
{
	return s_BatcherRing[buffer]->GetNumPendingReads();
}

bool grcDebugDraw::IsExpectingSubRenderThreadDebugDraw(LingerBuffer buffer)
{
	return expectingSubRenderThreadDebugDraw[buffer];
}

grcDebugDraw::BatcherHandle grcDebugDraw::AllocateSubRenderBatcher(LingerBuffer buffer)
{
	Assert(!g_IsSubRenderThread);
	const BatcherRing::Handle batcherHandle = s_BatcherRing[buffer]->GetAndIncNextWrite();
	Assert(batcherHandle);
	grcBatcher *const batcher = s_BatcherRing[buffer]->LockForWrite(batcherHandle);
	batcher->SetRenderMustWait(true);
	s_BatcherRing[buffer]->UnlockForWrite(batcherHandle);
	return (BatcherHandle)batcherHandle;
}

void grcDebugDraw::SetSubRenderBatcher(LingerBuffer buffer, BatcherHandle batcherHandle)
{
	Assert(g_IsSubRenderThread);
	Assert(batcherHandle);
	GetPerThreadState()->batcher[buffer] = (BatcherRing::Handle)batcherHandle;
}

void grcDebugDraw::ClearSubRenderBatcher2(LingerBuffer buffer, BatcherHandle batcherHandle)
{
	grcBatcher *const batcher = s_BatcherRing[buffer]->LockForWrite(batcherHandle);
	Assert(batcher);
	batcher->SetRenderMustWait(false);
	batcher->Close();
	s_BatcherRing[buffer]->UnlockForWrite(batcherHandle);
}

void grcDebugDraw::ClearSubRenderBatcher(LingerBuffer buffer)
{
	Assert(g_IsSubRenderThread);
	PerThreadState *const pts = GetPerThreadState();
	const BatcherRing::Handle batcherHandle = pts->batcher[buffer];
	// In the case of the subrender thread that actually renders the batched
	// debug drawing, the batcher will already have been cleared.
	if (batcherHandle)
	{
		ClearSubRenderBatcher2(buffer, batcherHandle);
		pts->batcher[buffer] = 0;
	}
}

void grcDebugDraw::ClearRenderState()
{
	GetPerThreadState()->Init();
}

void grcDebugDraw::Render3D(u32 numBatchers)
{
	if (grcDebugDraw::IsExpectingSubRenderThreadDebugDraw(grcDebugDraw::LINGER_3D))
	{
		grcDebugDraw::ClearSubRenderBatcher(grcDebugDraw::LINGER_3D);
	}

	u32 usageLast = imUsageLast[LINGER_3D];
	imUsageMin[LINGER_3D] = Min(imUsageLast[LINGER_3D], imUsageMin[LINGER_3D]);
	imUsageMax[LINGER_3D] = Max(imUsageLast[LINGER_3D], imUsageMax[LINGER_3D]);
	imUsageAcc[LINGER_3D] += imUsageLast[LINGER_3D];
	imUsageLast[LINGER_3D] = 0;
	im3DUsageHit++;

	grcBindTexture(NULL);

	// Reset the world matrix to make sure the following draw calls start where we expect them to.
	SetDefaultDebugRenderStates();

	const unsigned rti = g_RenderThreadIndex;
	sysMemSet(s_CollectedStats+rti, 0, sizeof(*s_CollectedStats));

	const bool flushAfterRender = true;

	// Render any lingering 3D commands from last frame.
	const unsigned lbf = s_LingerBatcherFrame[LINGER_3D];
	const unsigned nextLbf = lbf^1;
	s_CopyToBatcherWhileRendering[rti] = s_LingerBatcher[LINGER_3D][nextLbf];
	s_LingerBatcher[LINGER_3D][lbf]->Render(flushAfterRender);
	s_LingerBatcherFrame[LINGER_3D] = nextLbf;

	// Render any standard batchers.
	for (u32 i=0; i<numBatchers; ++i)
	{
		grcBatcher *const batcher = s_BatcherRing[LINGER_3D]->GetNextRead();
		Assert(batcher);
		batcher->Render(flushAfterRender);
		s_BatcherRing[LINGER_3D]->IncNextRead();
	}

	s_CopyToBatcherWhileRendering[rti] = NULL;

#if __PS3
	GCM_STATE(SetLineSmoothEnable, CELL_GCM_FALSE);
#endif // __PS3

	grcWorldIdentity();
	grcBatcher::SetCurrent(NULL);

	grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_Default);
	grcStateBlock::SetBlendState(grcStateBlock::BS_Default);
	grcStateBlock::SetRasterizerState(grcStateBlock::RS_Default);

	// Add stats as text for Render2D.
	if( im3DDisplayUsage )
	{
		AddDebugOutputEx(true, "Im3D Usage %d, Pages: %d", usageLast, numBatchers);
		AddDebugOutputEx(true, "Min:%d - Max:%d - Avg:%d", imUsageMin[LINGER_3D], imUsageMax[LINGER_3D], (int)((float)imUsageAcc[LINGER_3D]/(float)im3DUsageHit));

		AddDebugOutputEx(true, "Lines: %d", s_CollectedStats[rti].line3d);
		AddDebugOutputEx(true, "Quads: %d", s_CollectedStats[rti].quad3d);
		AddDebugOutputEx(true, "Polys: %d", s_CollectedStats[rti].poly3d);
		AddDebugOutputEx(true, "Circles: %d", s_CollectedStats[rti].circle3d);
		AddDebugOutputEx(true, "Arcs: %d", s_CollectedStats[rti].arc3d);
		AddDebugOutputEx(true, "Spheres: %d", s_CollectedStats[rti].sphere3d);
		AddDebugOutputEx(true, "Ellipsoidals: %d", s_CollectedStats[rti].ellipsoidal3d);
		AddDebugOutputEx(true, "Boxes: %d", s_CollectedStats[rti].box3d);
		AddDebugOutputEx(true, "Axes: %d", s_CollectedStats[rti].axis3d);
		AddDebugOutputEx(true, "Arrows: %d", s_CollectedStats[rti].arrow3d);
		AddDebugOutputEx(true, "Capsules: %d", s_CollectedStats[rti].capsule3d);
	}
}


//////////////////////////////////////////////////////////////////////////
// DEBUG_DRAW_TEXT
//////////////////////////////////////////////////////////////////////////

struct CustomStringPacket
{
	const grcFont* font;
	int framesToLive;
	s32 x;
	s32 y;
	Color32 color;
	u32 flags;
	bool proportionalFlag;
	bool rawCoordFlag;
	bool drawBGQuadFlag;

	// struct is followed by a '\0' terminated ascii string
};


static void CustomStringCallback(const void *data, size_t size, grcBatcher *batcher)
{
	const CustomStringPacket *const packet = (CustomStringPacket*)data;
	(void)batcher;

	switch (s_CustomStringCallbackMode)
	{
		case CSCM_INVALID:
		{
			Assertf(0, "CustomStringCallback unexpected call");
			break;
		}

		case CSCM_COLLATE:
		{
			if (packet->framesToLive)
			{
				grcBatcher *const dstBatcher = s_CustomStringBatcher[s_CustomStringBatcherDst];
				CustomStringPacket *const dstPacket = (CustomStringPacket*)(dstBatcher->AddRenderCallback(CustomStringCallback, size));
				if (dstPacket)
				{
					sysMemCpy(dstPacket, packet, size);
					dstPacket->framesToLive = ReduceFramesToLive(packet->framesToLive);
					if ((packet->flags & RELOCATABLE_STRING) != 0)
					{
						const bool proportional = ((packet->flags & grcDebugDraw::FIXED_WIDTH) == 0);
						const int textHeight = grcDebugDraw::g_textRenderer->GetTextHeight(proportional);
						if ((packet->flags & grcDebugDraw::RAW_COORDS) != 0)
						{
							dstPacket->y += s_CustomStringOffsetRawY;
							s_CustomStringLastRawY = dstPacket->y + textHeight;
							s_CustomStringLastY = (s_CustomStringLastRawY + textHeight - 1) / textHeight;
						}
						else
						{
							dstPacket->y += s_CustomStringOffsetY;
							s_CustomStringLastY = dstPacket->y + 1;
							s_CustomStringLastRawY = s_CustomStringLastY * textHeight;
						}
						dstPacket->flags &= ~RELOCATABLE_STRING;
					}
				}
			}
			break;
		}

		case CSCM_RENDER:
		{
			const char *const str = (char*)(packet+1);
			const bool proportional = ((packet->flags & grcDebugDraw::FIXED_WIDTH)  == 0);
			const bool drawBGQuad   = ((packet->flags & grcDebugDraw::NO_BG_QUAD)   == 0);
			const bool rawCoords    = ((packet->flags & grcDebugDraw::RAW_COORDS)   != 0);
			const bool scrollable   = ((packet->flags & SCROLLABLE_STRING)          != 0);
			const s32 x = packet->x;
			s32 y = packet->y;
			if (scrollable)
			{
				y += rawCoords ? s_CustomStringScrollOffsetRaw : s_CustomStringScrollOffset;
			}
			grcDebugDraw::g_textRenderer->RenderText(packet->color, x, y, str, proportional, drawBGQuad, rawCoords);
			break;
		}
	}
}

bool grcDebugDraw::GetDisplayDebugText()
{
	return sm_displayDebugText;
}

void grcDebugDraw::SetDisplayDebugText(bool display)
{
	sm_displayDebugText=display;
}

bool grcDebugDraw::GetIsInitialized()
{
	return sm_initialized;
}

bool grcDebugDraw::GetDisableCulling()
{
	return GetPerThreadState()->disableCulling;
}

void grcDebugDraw::SetDisableCulling(bool bDisableCulling)
{
	GetPerThreadState()->disableCulling = bDisableCulling;
}

bool grcDebugDraw::GetDoDebugZTest()
{
	return GetPerThreadState()->doDebugZTest;
}

void grcDebugDraw::SetDoDebugZTest(bool value)
{
	GetPerThreadState()->doDebugZTest = value;
}

bool grcDebugDraw::GetGlobalDoDebugZTest()
{
	return g_doDebugZTest;
}

void grcDebugDraw::SetGlobalDoDebugZTest(bool value)
{
	g_doDebugZTest = value;
}

void grcDebugDraw::SetDebugDrawAdditively(bool value)
{
	GetPerThreadState()->drawAdditively = value;
}

static s32 s_iHeight = 10;

s32 grcDebugDraw::GetScreenSpaceTextHeight()
{
	return s_iHeight;
}

void grcDebugDraw::SetScreenSpaceTextHeight(s32 iTextHeight)
{
	s_iHeight = iTextHeight;
}

static float s_fDrawLabelScaleX = 1.0f;
static float s_fDrawLabelScaleY = 1.0f;

float grcDebugDraw::GetDrawLabelScaleX()
{
	return s_fDrawLabelScaleX;
}

float grcDebugDraw::GetDrawLabelScaleY()
{
	return s_fDrawLabelScaleY;
}

void grcDebugDraw::SetDrawLabelScaleX(float xScale)
{
	s_fDrawLabelScaleX = xScale;
}

void grcDebugDraw::SetDrawLabelScaleY(float yScale)
{
	s_fDrawLabelScaleY = yScale;
}

void grcDebugDraw::TextFontPush(const grcFont* font)
{
	PerThreadState *const pts = GetPerThreadState();
	const unsigned idx = pts->fontStackIndex;
	if (Verifyf(idx < NELEM(pts->fontStack), "overflow"))
	{
		pts->fontStack[idx] = font;
		pts->fontStackIndex = idx+1;
	}
}

void grcDebugDraw::TextFontPop()
{
	PerThreadState *const pts = GetPerThreadState();
	const unsigned idx = pts->fontStackIndex;
	if (Verifyf(idx, "underflow"))
	{
		pts->fontStackIndex = idx-1;
	}
}

const grcFont* grcDebugDraw::TextFontGet()
{
	PerThreadState *const pts = GetPerThreadState();
	const unsigned idx = pts->fontStackIndex;
	if (idx > 0)
	{
		return pts->fontStack[idx-1];
	}
	return NULL;
}

void grcDebugDraw::Text(float x, float y, const Color32 col, const char* pText, bool drawBGQuad, float scaleX, float scaleY, int framesToLive)
{
	const grcFont* font = TextFontGet();

	const s32 width = grcDevice::GetWidth();
	const s32 height = grcDevice::GetHeight();

	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}
	
	INC_STAT(text2d);

	grcColor(col);
	grcDraw2dText(x*(float)width, y*(float)height, pText, drawBGQuad, scaleX, scaleY, font);
	// Note: For grcDraw2dText we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_2D);
}

void grcDebugDraw::Text(float x, float y, DD_ePixelCoordSpace coordSpace, const Color32 col, const char* pText, bool drawBGQuad, float scaleX, float scaleY, int framesToLive)
{
	const grcFont* font = TextFontGet();

	if (coordSpace == DD_ePCS_Normalised)
	{
		x = (x + 1.0f)/2.0f; // [0..1]
		y = (y + 1.0f)/2.0f; // [0..1]
		x *= (float)grcDevice::GetWidth();
		y *= (float)grcDevice::GetHeight();
	}
	else if (coordSpace == DD_ePCS_NormalisedZeroToOne)
	{
		x *= (float)grcDevice::GetWidth();
		y *= (float)grcDevice::GetHeight();
	}
	else if (coordSpace == DD_ePCS_Pixels)
	{
		// no change
	}
	else
	{
		return;
	}

	if (!StartRecording(LINGER_2D, framesToLive)) {
		return;
	}

	INC_STAT(text2d);

	grcColor(col);
	grcDraw2dText(x, y, pText, drawBGQuad, scaleX, scaleY, font);
	// Note: For grcDraw2dText we don't call grcWorldIdentity(); afterwards as they do
	// not set the world matrix like some primitives.
	EndRecording(LINGER_2D);
}

void grcDebugDraw::PrintToScreenCoors(const char *pString, s32 x, s32 y, Color32 col, u32 flags, int framesToLive)
{
	PrintToScreenCoors(pString, NULL, x, y, col, flags, framesToLive);
}

static void PrintToScreenCoorsInternal(grcBatcher *batcher, const char *pString, const grcFont* font, s32 x, s32 y, Color32 col, u32 flags, int framesToLive)
{
	const size_t stringLen  = strlen(pString);
	const size_t packetSize = sizeof(CustomStringPacket) + stringLen + 1;
	CustomStringPacket *const packet = (CustomStringPacket*)(batcher->AddRenderCallback(CustomStringCallback, packetSize));
	if (packet)
	{
		packet->font            = font;
		packet->framesToLive    = framesToLive;
		packet->x               = x;
		packet->y               = y;
		packet->color           = col;
		packet->flags           = flags;
		sysMemCpy(packet+1, pString, stringLen+1);
	}
}

// Name			:	PrintToScreenCoors
// Purpose		:	Stores a string to be printed to a specific screen coordinate.
// Parameters	:	string
//					x & y coordinates
//					Color
//					proportional (or fixed-width if false)
//					drawBGQuad - render a dimmed background quad behind the text
//					rawCoords - x & y are actual screen coordinates in pixels, rather than text coordinates
// Returns		:	Nothing
void grcDebugDraw::PrintToScreenCoors(const char *pString, const grcFont* font, s32 x, s32 y, Color32 col, u32 flags, int framesToLive)
{
	if (!GetDisplayDebugText())
	{
		return;
	}

	// Notice always passing framesToLive=1 to StartRecording here.  The real
	// value is stored inside the CustomStringPacket.
	if (!StartRecording(LINGER_2D, 1))
	{
		return;
	}

	PrintToScreenCoorsInternal(grcBatcher::GetCurrent(), pString, font, x, y, col, flags, framesToLive);

	EndRecording(LINGER_2D);
}

//
// name:		AddDebugOutput
// description:	Add debug output down the screen
void grcDebugDraw::AddDebugOutputExV(bool proportional, const char *fmt, va_list args)
{
	AddDebugOutputExV(proportional, Color32(0.9f, 0.9f, 0.9f), fmt, args);
}

void grcDebugDraw::AddDebugOutputEx(bool proportional, const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	AddDebugOutputExV(proportional, fmt, argptr);
	va_end(argptr);
}

//
// name:		AddDebugOutput
// description:	Add debug output down the screen
void grcDebugDraw::AddDebugOutputExV(bool proportional, Color32 col, const char *fmt, va_list args)
{
	char output[256];
	vformatf(output, 256, fmt, args);
	AddDebugOutputExRaw(proportional, col, output);
}

void grcDebugDraw::AddDebugOutputExRaw(bool proportional, Color32 col, const char *output)
{
	const int debugCharHeight = Max(g_textRenderer->GetTextHeight(false), g_textRenderer->GetTextHeight(true));
	const int screenHeight = GRCDEVICE.GetHeight();
	s32 linesPerPage = (screenHeight / debugCharHeight) - (DEBUG_OUTPUT_YSTART + 3);	// -3 to make space for page number and debug bar at bottom of screen

	PerThreadState *const pts = GetPerThreadState();

	// Get the current font.  The font stack if non-empty overrides grcFont::GetCurrent.
	const grcFont *font = NULL;
	if (grcFont::HasCurrent())
	{
		font = &grcFont::GetCurrent();
	}
	const unsigned fsi = pts->fontStackIndex;
	if (fsi)
	{
		font = pts->fontStack[fsi-1];
	}

	if (PARAM_fixedheightdebugfont.Get())
	{
		if(pts->debugOutputY > 0 && pts->debugOutputY < linesPerPage)
		{
			PrintToScreenCoors(output, font, pts->debugOutputX, pts->debugOutputY, col, (proportional?0:FIXED_WIDTH)|RELOCATABLE_STRING|SCROLLABLE_STRING);
		}
		else
		{
			WriteToDebugStream(output);
		}
	}
	else
	{
		if (pts->debugOutputY > 0 && pts->debugOutputY < screenHeight * 85 / 100)
		{
			const int fontCharWidth = /*font ? font->GetWidth() :*/ DEBUG_CHAR_WIDTH;
			PrintToScreenCoors(output, font, pts->debugOutputX * fontCharWidth, pts->rawDebugOutputY, col, (proportional?0:FIXED_WIDTH)|RAW_COORDS|RELOCATABLE_STRING|SCROLLABLE_STRING);
			pts->rawDebugOutputY += g_textRenderer->GetTextHeight(proportional);
		}
		else
		{
			WriteToDebugStream(output);
		}
	}

	pts->debugOutputY++;

	if (pts->columnizedTop != -1)
	{
		if (++(pts->columnizedLineCount) == pts->columnizedMaxLines)
		{
			NewDebugOutputColumn(pts->columnizedCharOffset);
		}
	}
}

void grcDebugDraw::AddDebugOutputEx(bool proportional, Color32 col, const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	AddDebugOutputExV(proportional, col, fmt, argptr);
	va_end(argptr);
}

void grcDebugDraw::AddDebugOutput(const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	AddDebugOutputExV(true, fmt, argptr);
	va_end(argptr);
}

void grcDebugDraw::AddDebugOutput(Color32 col, const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	AddDebugOutputExV(true, col, fmt, argptr);
	va_end(argptr);
}

void grcDebugDraw::AddDebugOutputSeparator(s32 pixelHeight)
{
	if (!PARAM_fixedheightdebugfont.Get())
	{
		PerThreadState *const pts = GetPerThreadState();
		pts->rawDebugOutputY += pixelHeight;
	}
}

void grcDebugDraw::BeginDebugOutputColumnized(s32 maxLines, s32 colCharOffset)
{
	PerThreadState *const pts = GetPerThreadState();

	Assertf(pts->columnizedTop == -1, "Calls to BeginDebugOutputColumnized() cannot be nested");

	pts->columnizedTop = pts->columnizedBottom = pts->debugOutputY;
	pts->rawColumnizedTop = pts->rawColumnizedBottom = pts->rawDebugOutputY;
	pts->columnizedLineCount = 0;

	pts->columnizedMaxLines = maxLines;
	pts->columnizedCharOffset = colCharOffset;
	pts->currentColumn = 0;
}

void grcDebugDraw::EndDebugOutputColumnized()
{
	PerThreadState *const pts = GetPerThreadState();

	Assertf(pts->columnizedTop != -1, "Calling EndDebugOutputColumnized() without prior BeginDebugDrawColumnized()");

	// Continue at the lowest line that we rendered in any column.
	pts->rawDebugOutputY = pts->rawColumnizedBottom;
	pts->debugOutputY = pts->columnizedBottom;
	pts->debugOutputX = DEBUG_OUTPUT_XSTART;

	pts->columnizedTop = -1;
}

void grcDebugDraw::NewDebugOutputColumn(s32 charOffset)
{
	PerThreadState *const pts = GetPerThreadState();

	Assertf(pts->columnizedTop != -1, "Calling NewDebugOutputColumn() without prior BeginDebugDrawColumnized()");

	// Find the new low watermark for the height.
	pts->columnizedBottom = Max(pts->columnizedBottom, pts->debugOutputY);
	pts->rawColumnizedBottom = Max(pts->rawColumnizedBottom, pts->rawDebugOutputY);
	pts->columnizedLineCount = 0;
	pts->currentColumn++;

	// Go back to the top.
	pts->rawDebugOutputY = pts->rawColumnizedTop;
	pts->debugOutputY = pts->columnizedTop;
	pts->debugOutputX += charOffset;
}

int grcDebugDraw::GetDebugOutputColumCount()
{
	PerThreadState *const pts = GetPerThreadState();
	Assertf(pts->columnizedTop != -1, "Calling GetDebugOutputColumCount() without prior BeginDebugDrawColumnized()");
	return pts->currentColumn + 1;
}

bool grcDebugDraw::Text(Vec3V_In pos, const Color32 col, const char *pText, bool drawBGQuad, int framesToLive)
{
	bool res = false;
	const Vec3V viewPos = pos - s_CameraPosition;
	float Dist = viewPos.GetXf() * viewPos.GetXf() + viewPos.GetYf() * viewPos.GetYf();
	if ((Dist < g_textDist3DFar*g_textDist3DFar) && (Dist >= g_textDist3DNear*g_textDist3DNear))
	{
		if (!StartRecording(LINGER_3D, framesToLive)) {
			return false;
		}

		grcColor(col);
		grcDrawLabel(pos, pText, drawBGQuad);
		// Note: For grcDrawLabel we don't call grcWorldIdentity(); afterwards as they do
		// not set the world matrix like some primitives.
		EndRecording(LINGER_3D);

		res = true;
	}

	return res;
}

bool grcDebugDraw::Text(Vec3V_In pos, const Color32 col, const s32 iXOffset, const s32 iYOffset, const char *pText, bool drawBGQuad, int framesToLive)
{
	bool res = false;
	const Vec3V viewPos = pos - s_CameraPosition;
	float Dist = viewPos.GetXf() * viewPos.GetXf() + viewPos.GetYf() * viewPos.GetYf();
	if ((Dist < g_textDist3DFar*g_textDist3DFar) && (Dist >= g_textDist3DNear*g_textDist3DNear))
	{
		if (!StartRecording(LINGER_3D, framesToLive)) {
			return false;
		}

		grcColor(col);
		grcDrawLabel(VEC3V_TO_VECTOR3(pos), iXOffset, iYOffset, pText, drawBGQuad, s_fDrawLabelScaleX, s_fDrawLabelScaleY);
		// Note: For grcDrawLabel we don't call grcWorldIdentity(); afterwards as they do
		// not set the world matrix like some primitives.
		EndRecording(LINGER_3D);

		res = true;
	}

	return res;
}

void RageTextRenderer::Begin()
{
#if __XENON 
	m_scaleX = 0.75f;
	m_scaleY = 1.0f;
#else
	m_scaleX = 1.0f;
	m_scaleY = 1.0f;
#endif

	m_iFixedCharHeight= grcSetup::GetFixedWidthFont()->GetHeight() + 2;

	if (PARAM_debugtextfixedwidth.Get())
	{
		m_iPropCharHeight = (int) grcSetup::GetProportionalFont()->GetHeight();
	}
	else
	{
		float rawFontHeight = (float) grcSetup::GetProportionalFont()->GetHeight();
		m_iPropCharHeight = (int) (rawFontHeight * (GRCDEVICE.GetHiDef() ? 0.9f : 1.05f));
	}
	

	m_oldVP = grcViewport::GetCurrent();
	if (m_oldVP != grcViewport::GetDefaultScreen()) {
		grcViewport::SetCurrent(grcViewport::GetDefaultScreen());
	}
}

void RageTextRenderer::RenderText(Color32 color, int xPos, int yPos, const char *string, bool proportional, bool drawBGQuad, bool rawCoords)
{
	// Note: grcFont::DrawScaled() asserts for strings longer than k=(grcBeginMax/6)
	// PSN: grcBeginMax is differently defined in WR294
	const s32 maxStrLen = __PPU?(1024/6):(grcBeginMax/6);
	char str[maxStrLen+1];
	int tallestHeight = Max(m_iPropCharHeight, m_iFixedCharHeight);
	strncpy(str, string, maxStrLen);

	if (!rawCoords) {
		yPos *= tallestHeight;
		xPos *= DEBUG_CHAR_WIDTH;
	}

	grcFont &oldFont = grcFont::GetCurrent();

	/* float oldScale = grcFont::GetCurrent().GetInternalScale();
	if (proportional && !PARAM_debugtextfixedwidth.Get()) 
	{
		float newScale = !GRCDEVICE.GetHiDef() ? 1.05f : 0.9f;
		grcFont::GetCurrent().SetInternalScale(newScale);
	} */

	Vector2 pos = Vector2(float(xPos), float(yPos));

	if (drawBGQuad) 
	{
		int barHeight = PARAM_fixedheightdebugfont.Get() ? tallestHeight : GetTextHeight(proportional);
		grcDrawSingleQuadf(pos.x, pos.y, pos.x + grcFont::GetCurrent().GetStringWidth(string, (int)strlen(string)), pos.y+barHeight, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, Color32(0, 0, 0, 140));
	}

	grcStateBlock::SetBlendState(sm_AdditiveBS);
	grcFont::GetCurrent().Draw(pos.x, pos.y, color, string);
	grcStateBlock::SetBlendState(sm_NormalBS);

	// grcFont::GetCurrent().SetInternalScale(oldScale);
	grcFont::SetCurrent(&oldFont);
}

void RageTextRenderer::End()
{
	if (m_oldVP != grcViewport::GetDefaultScreen()) {
		grcViewport::SetCurrent(m_oldVP);
	}
}

int RageTextRenderer::GetTextHeight(bool proportional) const
{
	return (proportional) ? m_iPropCharHeight : m_iFixedCharHeight;
}

void grcDebugDraw::SetTextRenderer(TextRenderer &textRenderer)
{
	g_textRenderer = &textRenderer;
}


void grcDebugDraw::PushRasterizerState(grcRasterizerStateHandle RasterizerStateHandle)
{
	Assertf(g_PushRasterizerStateCalls == 0, "Nested render states are not supported, use at your own risk !");
	++g_PushRasterizerStateCalls;

	if (!StartRecording(LINGER_3D, 1)) {
		return;
	}

	grcBatcher* batcher = grcBatcher::GetCurrent();

	if (batcher)
	{
		batcher->SaveRasterizerState();
		batcher->SetRasterizerState(RasterizerStateHandle);
	}

	EndRecording(LINGER_3D);
}

void grcDebugDraw::PushBlendState(grcBlendStateHandle BlendStateHandle)
{
	Assertf(g_PushBlendStateCalls == 0, "Nested render states are not supported, use at your own risk !");
	++g_PushBlendStateCalls;

	if (!StartRecording(LINGER_3D, 1)) {
		return;
	}

	grcBatcher* batcher = grcBatcher::GetCurrent();

	if (batcher)
	{
		batcher->SaveBlendState();
		batcher->SetBlendState(BlendStateHandle);
	}

	EndRecording(LINGER_3D);
}

void grcDebugDraw::PushDepthStencilState(grcDepthStencilStateHandle depthStencilHandle)
{
	Assertf(g_PushDepthStencilStateCalls == 0, "Nested render states are not supported, use at your own risk !");
	++g_PushDepthStencilStateCalls;

	if (!StartRecording(LINGER_3D, 1)) {
		return;
	}

	grcBatcher* batcher = grcBatcher::GetCurrent();

	if (batcher)
	{
		batcher->SaveDepthStencilState();
		batcher->SetDepthStencilState(depthStencilHandle);
	}

	EndRecording(LINGER_3D);
}

bool grcDebugDraw::PushLightingState(bool lightingState, int framesToLive/*=1*/)
{
	Assertf(g_PushLightingStateCalls == 0, "Nested render states are not supported, use at your own risk !");
	++g_PushLightingStateCalls;

	if (!StartRecording(LINGER_3D, framesToLive)) {
		return false;
	}

	grcBatcher* batcher = grcBatcher::GetCurrent();

	if (batcher)
	{
		batcher->SaveLightingState();
		batcher->SetLighting(lightingState);
	}

	EndRecording(LINGER_3D);

	return true;
}

void grcDebugDraw::PopRasterizerState()
{
	Assertf(g_PushRasterizerStateCalls >= 0, "More Pops than Pushes! Don't do that !");
	--g_PushRasterizerStateCalls;

	if (!StartRecording(LINGER_3D, 1)) {
		return;
	}

	grcBatcher* batcher = grcBatcher::GetCurrent();

	if (batcher)
		batcher->RestoreRasterizerState();

	EndRecording(LINGER_3D);
}

void grcDebugDraw::PopBlendState()
{
	Assertf(g_PushBlendStateCalls >= 0, "More Pops than Pushes! Don't do that !");
	--g_PushBlendStateCalls;

	if (!StartRecording(LINGER_3D, 1)) {
		return;
	}

	grcBatcher* batcher = grcBatcher::GetCurrent();

	if (batcher)
		batcher->RestoreBlendState();

	EndRecording(LINGER_3D);
}

void grcDebugDraw::PopDepthStencilState()
{
	Assertf(g_PushDepthStencilStateCalls >= 0, "More Pops than Pushes! Don't do that !");
	--g_PushDepthStencilStateCalls;

	if (!StartRecording(LINGER_3D, 1)) {
		return;
	}

	grcBatcher* batcher = grcBatcher::GetCurrent();

	if (batcher)
		batcher->RestoreDepthStencilState();

	EndRecording(LINGER_3D);
}

void grcDebugDraw::PopLightingState()
{
	Assertf(g_PushLightingStateCalls >= 0, "More Pops than Pushes! Don't do that !");
	--g_PushLightingStateCalls;

	if (!StartRecording(LINGER_3D, 1)) {
		return;
	}

	grcBatcher* batcher = grcBatcher::GetCurrent();

	if (batcher)
		batcher->RestoreLightingState();

	EndRecording(LINGER_3D);
}

#endif // __IM_BATCHER


} // namespace rage
