// 
// atl/simplecache.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_SIMPLECACHE_H 
#define ATL_SIMPLECACHE_H 

#if __SPU

#include "system/dma.h"

namespace rage {

	// #define dumpqw(x) Displayf(#x " = [%08x %08x %08x %08x]",si_to_uint(si_rotqbyi(x,0)),si_to_uint(si_rotqbyi(x,4)),si_to_uint(si_rotqbyi(x,8)),si_to_uint(si_rotqbyi(x,12)))


class atSimpleCache4_Base
{
public:
	__attribute__((noinline)) void *GetVoid(void *remotePtr,uint32_t elSize)
	{
		// bump splatted current age
		current = si_ai(current,1);

		qword remoteSplat = (qword)spu_splats((uint32_t)remotePtr);	// splat the address to all four channels
		qword match = si_ceq(remoteSplat,remoteAddrs);		// put 1's in the words that match (shouldn't be more than one)
		if (si_to_int(si_gb(match)))	// scored a hit?
		{
			mru = si_selb(mru,current,match);
			// Displayf("HIT matchIndex = %d",matchIndex);
			DRAWABLESPU_STATS_ONLY(++hits);
			return (void*)si_to_ptr(si_orx(si_and(ptrs,match)));
		}
		else
		{
			qword xyzw = mru;
			qword zwxy = si_rotqbyi(xyzw,8);
			qword min1 = si_selb(xyzw,zwxy,si_clgt(xyzw,zwxy));		//	min(x,z)	min(y,w)	min(z,x)	min(w,y)
			qword min2 = si_rotqbyi(min1,4);						//	min(y,w)	min(z,x)	min(w,y)	min(x,z)
			qword smallest = si_selb(min1,min2,si_clgt(min1,min2));	// min(x,z,y,w) min(y,w,z,x)min(z,x,w,y) min(w,y,x,z)
			qword match = si_ceq(smallest,mru);		// exactly one field is going to match
			void* result = (void*) si_to_ptr(si_orx(si_and(ptrs,match)));
			remoteAddrs = si_selb(remoteAddrs,remoteSplat,match);
			mru = si_selb(mru,current,match);
			sysDmaGetAndWait(result,(uint64_t)remotePtr,elSize,spuGetTag);
			// Displayf("MISS matchIndex = %d",matchIndex);
			DRAWABLESPU_STATS_ONLY(++misses);
			return result;
		}
	}
protected:
	qword remoteAddrs;
	qword mru;
	qword current;
	qword ptrs;
public:
	DRAWABLESPU_STATS_ONLY(uint32_t misses, hits, pad0, pad1);
};

// Generic cache, four entries, fully associative, perfect LRU eviction.  Minimal branches.
// SimpleCache4 and SimpleCache8 are special-cased and are *very* shorter on insn count than the SimpleCacheN general version.
template <class T> class atSimpleCache4: public atSimpleCache4_Base
{
public:
	void Init()
	{
		remoteAddrs = si_il(0);
		mru = (qword)(0,0,0,0, 0,0,0,1, 0,0,0,2, 0,0,0,3);
		// Note that mpya only does 16-bit multiplies but that's enough for our needs.
		ptrs = si_mpya(mru,si_from_uint(sizeof(T)),(qword)spu_splats((uint32_t)cache));
		current = si_il(3);
		DRAWABLESPU_STATS_ONLY(misses = hits = 0);
	}
	T* Get(T* remotePtr)
	{
		return (T*)GetVoid(remotePtr,sizeof(T));
	}
	T* GetSafe(T* remotePtr)
	{
		if ((uint32_t)remotePtr < 256*1024)
			return remotePtr;
		else
			return Get(remotePtr);
	}
private:
	T cache[4];
};

template <class T,int qwc> class atSimpleCacheN
{
public:
	void Init()
	{
		qword mruBase = (qword)(0,0,0,0, 0,0,0,1, 0,0,0,2, 0,0,0,3);
		for (int i=0; i<qwc; i++)
		{
			remoteAddrs[i] = si_il(0);
			mru[i] = mruBase;
			ptrs[i] = si_mpya(mruBase,si_from_uint(sizeof(T)),(qword)spu_splats((uint32_t)cache));
			mruBase = si_ai(mruBase,4);
		}
		current = si_il(qwc*4-1);
	}
	T* Get(T* remotePtr)
	{
		// bump splatted current age
		current = si_ai(current,1);

		qword remoteSplat = (qword)spu_splats((uint32_t)remotePtr);	// splat the address to all four channels
		qword hit = si_il(0);
		qword results = si_il(0);
		// Could hoist the first iteration out of the loop, skip the or, and start at 1.
		// Tried this for all loops except the one that computes min below, and N1 case was one insn
		// cheaper than before (still five more than special-case code), and N2 case was four insns
		// cheaper than before (still four more than special-case code) so it didn't seem worth
		// the additional complexity.
		for (int i=0; i<qwc; i++)
			hit = si_or(hit,si_ceq(remoteSplat,remoteAddrs[i]));
		if (si_to_int(si_gb(hit)))				// scored a hit? (only one of matchA or matchB will be nonzero)
		{
			for (int i=0; i<qwc; i++)
			{
				qword match = si_ceq(remoteSplat,remoteAddrs[i]);
				mru[i] = si_selb(mru[i],current,match);
				results = si_or(results, si_and(ptrs[i],match));
			}
			// Displayf("HIT matchIndex = %d",matchIndex);
			return (T*) si_to_ptr(si_orx(results));
		}
		else 
		{
			qword smallest = si_il(-1);
			for (int i=0; i<qwc; i++)
			{
				qword xyzw = mru[i];
				qword zwxy = si_rotqbyi(xyzw,8);
				qword min1 = si_selb(xyzw,zwxy,si_clgt(xyzw,zwxy));		//	min(x,z)	min(y,w)	min(z,x)	min(w,y)
				qword min2 = si_rotqbyi(min1,4);						//	min(y,w)	min(z,x)	min(w,y)	min(x,z)
				qword smaller = si_selb(min1,min2,si_clgt(min1,min2));	// min(x,z,y,w) min(y,w,z,x)min(z,x,w,y) min(w,y,x,z)
				smallest = si_selb(smallest,smaller,si_clgt(smallest,smaller));
			}
			for (int i=0; i<qwc; i++)
			{
				qword match = si_ceq(smallest,mru[i]);
				remoteAddrs[i] = si_selb(remoteAddrs[i],remoteSplat,match);
				mru[i] = si_selb(mru[i],current,match);
				results = si_or(results, si_and(ptrs[i],match));
			}
			T* result = (T*) si_to_ptr(si_orx(results));
			sysDmaGetAndWait(result,(uint64_t)remotePtr,sizeof(T),spuGetTag);
			// Displayf("MISS matchIndex = %d",matchIndex);
			return result;
		}
	}
	T* GetSafe(T* remotePtr)
	{
		if ((uint32_t)remotePtr < 256*1024)
			return remotePtr;
		else
			return Get(remotePtr);
	}
private:
	qword remoteAddrs[qwc];
	qword mru[qwc];
	qword current;
	qword ptrs[qwc];
	T cache[qwc*4];
};


} // namespace rage

#endif

#endif // ATL_SIMPLECACHE_H 
