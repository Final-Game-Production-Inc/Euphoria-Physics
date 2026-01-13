//
// grcore/allocscope.h
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_ALLOCSCOPE_H
#define GRCORE_ALLOCSCOPE_H


#define GRCCONTEXT_ALLOC_SCOPES_SUPPORTED       (RSG_DURANGO || RSG_ORBIS)

#if GRCCONTEXT_ALLOC_SCOPES_SUPPORTED
#	define GRCCONTEXT_ALLOC_SCOPES_SUPPORTED_ONLY(...)      __VA_ARGS__
#else
#	define GRCCONTEXT_ALLOC_SCOPES_SUPPORTED_ONLY(...)
#endif

#if !GRCCONTEXT_ALLOC_SCOPES_SUPPORTED
#	define GRC_ALLOC_SCOPE_DECLARE_GLOBAL(STATIC, NAME)
#	define GRC_ALLOC_SCOPE_DECLARE_FUNCTION_STATIC(NAME)
#	define GRC_ALLOC_SCOPE_PUSH(NAME)
#	define GRC_ALLOC_SCOPE_POP(NAME)
#	define GRC_ALLOC_SCOPE_LOCK(NAME)
#	define GRC_ALLOC_SCOPE_UNLOCK(NAME)
#	define GRC_ALLOC_SCOPE_SINGLE_THREADED_DECLARE(STATIC, NAME)
#	define GRC_ALLOC_SCOPE_SINGLE_THREADED_PUSH(NAME)
#	define GRC_ALLOC_SCOPE_SINGLE_THREADED_POP(NAME)
#	define GRC_ALLOC_SCOPE_SINGLE_THREADED_LOCK(NAME)
#	define GRC_ALLOC_SCOPE_SINGLE_THREADED_UNLOCK(NAME)
#	define GRC_ALLOC_SCOPE_AUTO_PUSH_POP()
#else

#	include "effect.h"
#	include "atl/bitset.h"
#	include "system/membarrier.h"
#	include "system/ipc.h"


	// Enables grcContextAllocScope locking.  By default only enabled in assert
	// builds, but error checking will still be performed if enabled in other
	// non-assert builds.
#	define GRCCONTEXT_ALLOC_SCOPE_LOCKS            (1 && __ASSERT)

	// Maximum number of command buffer segments that are supported.
#	define GRCCONTEXT_MAX_NUM_SEGMENTS             (256)


	namespace rage
	{

	class grcGfxContext;

	// The active grcContextAllocScope for a context records all allocations
	// from the command buffers, then releases them upon popScope or releaseAll.
	class grcContextAllocScope
	{
	private:

		friend grcGfxContext;

		static volatile u32                             sm_AllocScopeId;

		grcContextAllocScope                           *m_ParentScope;
		atFixedBitSet<GRCCONTEXT_MAX_NUM_SEGMENTS,u64>  m_AllocedSegs;
		u32                                             m_Id;
#		if GRCCONTEXT_ALLOC_SCOPE_LOCKS
			u32                                             m_Locked;
#		endif
#		if __ASSERT
			u32                                             m_SizeAlloced;  // useful for post mortem debugging
#		endif

		void generateId();

	public:

		enum {
			ID_INVALID,         // not a valid scope id
			ID_INFINITE,        // fake scope id for a pseudo infinite, always valid scope
			NUM_SPECIAL_IDS
		};

		grcContextAllocScope();

#		if __ASSERT
			bool isDefaultState() const;
#		endif

#		if GRCCONTEXT_ALLOC_SCOPE_LOCKS
			void assertNotLocked() const;
#		else
			inline void assertNotLocked() const {}
#		endif

		void releaseAll();

		void pushScope();
		void popScope();

		// TODO: When grcComputeContext (or whatever it ends up being called) is
		// supported, add pushScope and popScope overloads that take a
		// grcComputeContext* as input.  The default, no-arg overload should
		// remain the same and use the current grcGfxContext.
		//
		// Will need to add the corresponding GRC_ALLOC_SCOPE_... macros too.
		//

		const grcContextAllocScope *parent() const;
		u32 id() const;

		void lock();
		void unlock();
		bool isLocked() const;
	};

	inline grcContextAllocScope::grcContextAllocScope()
		: m_ParentScope(NULL)
		, m_AllocedSegs(false)
		, m_Id(ID_INVALID)
#		if GRCCONTEXT_ALLOC_SCOPE_LOCKS
			, m_Locked(0)
#		endif
#		if __ASSERT
			, m_SizeAlloced(0)
#		endif
	{
	}

	inline const grcContextAllocScope *grcContextAllocScope::parent() const {
		return m_ParentScope;
	}

	inline u32 grcContextAllocScope::id() const {
		return m_Id;
	}

	inline void grcContextAllocScope::lock() {
#		if GRCCONTEXT_ALLOC_SCOPE_LOCKS
			++m_Locked;
#		endif
	}

	inline void grcContextAllocScope::unlock() {
#		if GRCCONTEXT_ALLOC_SCOPE_LOCKS
			--m_Locked;
#		endif
	}

	inline bool grcContextAllocScope::isLocked() const {
#		if GRCCONTEXT_ALLOC_SCOPE_LOCKS
			return !!m_Locked;
#		else
			return false;
#		endif
	}

	// RAII wrapper around grcContextAllocScope
	class grcContextAutoAllocScope : public grcContextAllocScope
	{
	public:
		grcContextAutoAllocScope()  { pushScope(); }
		~grcContextAutoAllocScope() { popScope();  }
	};

	// Cross context allocations that live outside of an alloc scope.  These can
	// be shared between contexts.  The standard scoped allocations are lower
	// overhead, and can be recycled quicker.  Cross context allocations should
	// only be required when memory needs to be shared between multiple deferred
	// contexts (which is hopefully somewhat uncommon).
	class grcCrossContextAlloc {
	public:

		// Object should generally be created on the update thread.  This object
		// should then be accessed by the render threads.  The actual allocation
		// won't occur until the first call to getPtr().
		inline grcCrossContextAlloc(u32 size, u32 alignBytes)
			: m_Ptr(NULL)
			, m_Size(size)
			, m_AlignBytes(alignBytes)
		{
		}

		// Default ctor leaves object in undefined state.
		// Must call init() before use.
		inline grcCrossContextAlloc() {
		}

		inline void init(u32 size, u32 alignBytes) {
			m_Ptr = NULL;
			m_Size = size;
			m_AlignBytes = alignBytes;
		}

#		if __ASSERT
			// Object lifetime is generally too long to do RAII style freeing
			// here.  But we can assert to make sure that if getPtr() was ever
			// called, then free() was also called.
			inline ~grcCrossContextAlloc() {
				Assert(!m_Ptr);
			}
#		endif

		// Access the allocated memory.  Must be called from (sub) render
		// threads only.  The first call will perform the allocation from a
		// segment in the current context.
		//
		// TODO: In the future when we also support grcComputeContext (or
		// whatever it ends up being called), we should probably pass the
		// context in as an argument, rather than just always allocating from
		// the current thread's grcGfxContext.
		//
		void *getPtr();

		// If it can be garunteed that there is no contention, then
		// getPtrSingleThreaded() is a little faster than getPtr().  But that
		// does NOT mean this function can be held while a mutex is locked.  If
		// the context is currently throttled that could cause a deadlock.
		// Basically the only way to ensure no contention, is to only use in a
		// single subrender job.
		void *getPtrSingleThreaded();

		// Free the allocated memory.  Call after the last use in a render
		// thread (in the order the GPU will see the uses).
		void free();

	private:
		void *volatile  m_Ptr;
		u32             m_Size;
		u32             m_AlignBytes;
	};

	template<class T>
	class grcCrossContextAllocTyped {
	public:
		typedef T value_type;
		typedef T* pointer;

		grcCrossContextAllocTyped() : m_Count(0) {}
		explicit grcCrossContextAllocTyped(u32 count, u32 alignBytes = __alignof(T)) : m_CCA(sizeof(T) * count, alignBytes), m_Count(count) {}

		inline void init(u32 count, u32 alignBytes = __alignof(T)) {
			m_CCA.init(sizeof(T) * count, alignBytes);
			m_Count = count;
		}

		pointer getPtr()				{ return reinterpret_cast<pointer>(m_CCA.getPtr()); }
		pointer getPtrSingleThreaded()	{ return reinterpret_cast<pointer>(m_CCA.getPtrSingleThreaded()); }
		void free()						{ m_CCA.free(); }

		u32 GetCount()					{ return m_Count; }

	private:
		grcCrossContextAlloc m_CCA;
		u32 m_Count;
	};

	}
	// namespace rage

#define GRC_ALLOC_SCOPE_DECLARE_GLOBAL(STATIC, NAME)                           \
	STATIC grcContextAllocScope NAME[NUMBER_OF_RENDER_THREADS];                \
	/* Dummy function to prevent accidental usage of this macro at function */ \
	/* scope.  Will cause a compile error because nested functions are      */ \
	/* illegal.  Function name chosen so that the compiler's error message  */ \
	/* will give a hint as to what is wrong.                                */ \
	void NAME##__GRC_ALLOC_SCOPE_DECLARE_GLOBAL__not_allowed_at_function_scope() {}

#define GRC_ALLOC_SCOPE_DECLARE_FUNCTION_STATIC(NAME)                          \
	THREAD_SAFE_FUNCTION_SCOPE_STATIC_ARRAY(/*non-const*/, grcContextAllocScope, NAME, NUMBER_OF_RENDER_THREADS);

#	define GRC_ALLOC_SCOPE_PUSH(NAME)                              NAME[g_RenderThreadIndex].pushScope();
#	define GRC_ALLOC_SCOPE_POP(NAME)                               NAME[g_RenderThreadIndex].popScope();
#	define GRC_ALLOC_SCOPE_LOCK(NAME)                              NAME[g_RenderThreadIndex].lock();
#	define GRC_ALLOC_SCOPE_UNLOCK(NAME)                            NAME[g_RenderThreadIndex].unlock();
#	define GRC_ALLOC_SCOPE_SINGLE_THREADED_DECLARE(STATIC, NAME)   STATIC grcContextAllocScope NAME;
#	define GRC_ALLOC_SCOPE_SINGLE_THREADED_PUSH(NAME)              NAME.pushScope();
#	define GRC_ALLOC_SCOPE_SINGLE_THREADED_POP(NAME)               NAME.popScope();
#	define GRC_ALLOC_SCOPE_SINGLE_THREADED_LOCK(NAME)              NAME.lock();
#	define GRC_ALLOC_SCOPE_SINGLE_THREADED_UNLOCK(NAME)            NAME.unlock();
#	define GRC_ALLOC_SCOPE_AUTO_PUSH_POP()                         grcContextAutoAllocScope MacroJoin(grcContextAutoAllocScope_,__LINE__);
#endif

#endif // GRCORE_ALLOCSCOPE_H
