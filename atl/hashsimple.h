//
// atl/hashsimple.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_HASHSIMPLE_H
#define ATL_HASHSIMPLE_H

#include "atl/dlistsimple.h"

#include "math/simplemath.h"

namespace rage {

#if __WIN32
#pragma warning(push)
#pragma warning(disable: 4201)
#pragma warning(disable: 4324)
#endif

//0 is considered a power of two in this function...
inline bool IsPowOf2(u32 uVal)
{
	return(!(uVal & (uVal - 1)));
}

/*
PURPOSE:
This is a simple implementation of a fixed-size closed hash table.  You need to know about how many things go in the 
table from the start to use this class.  This hash is efficient(for one thing) because the hashing is done mod 
powers of two, instead of with primes.  The trick to making this work well is to ensure that your hashing function 
is a good randomizer.  A simple example hashing function is included in this class, but you may need your own if the 
distribution provided by the example hashing function is not very good.

The way this works is that you tell it how many keys you will need, and it allocates them on the heap(it would be 
trivial to make it take in a pointer instead - feel free to add that functionality if you wish).  The hash class owns 
the memory for the keys, but you provide the key class that has defined the functions GenerateHash and IsEqual.  The 
hash uses these to insert and remove elements.

There is no limit to the size of the hash(except available memory), and there is currently no way to iterate over the 
hash without the key.  You can feel free to add that functionality(and hopefully change this commment), but it would 
require a linear search over the hash, which will be pretty darn slow.

Testing indicates that this version is always faster than atmap - sometime by just a very little, and sometimes by 
quite a bit.  It also is a little less space efficient in key storage because it uses doubly-linked lists, which are 
not strictly required.
PARAMETERS:
	_TKey - This parameter is expected to contain enough data to generate a hash - everything else is up to the user.
	it must define the functions <c>GenerateHash()</c> and <c>IsEqual()</c>.
*/
template <class _TKey> class SimpleHash
{
private:
	struct KeyBase
	{
		_TKey					key;
		//DLinkSimple<KeyBase>	link;
		//DLinkSimple<KeyBase>	link;
		
		KeyBase					*pNext;
	};
	
	u32		uPow2NumKeys;
	u32		uNumKeysUsed;
	
	/*DLListSimple<KeyBase>	*paHashHeads; //each one is a linked list of hash keys...
	DLListSimple<KeyBase>	unusedHashHead; //all the unused ones go here...*/
	
	KeyBase					**ppHashHeads; //each one is a singly linked list of hash keys...
	//DLListSimple<KeyBase>	unusedHashHead; //all the unused ones go here...
	KeyBase					*pUnusedHashHead; //all the unused ones go here...
	
	KeyBase					*paHashHeadOrg;
	
	inline KeyBase *Search(const _TKey *pFindKey, u32 uIndex) const
	{
		KeyBase		*pSearchKey;
		
		for(pSearchKey = this->ppHashHeads[uIndex];
			pSearchKey;
			pSearchKey = pSearchKey->pNext)
		{
			if(pFindKey->IsEqual(&pSearchKey->key))
			{
				return pSearchKey;
			}
		}
		
		return 0;
	}
	
	inline KeyBase *Search(const _TKey *pFindKey, u32 uIndex, KeyBase *&rpPrevFoundKey) const
	{
		KeyBase		*pSearchKey;
		KeyBase		*pPrevFoundKey;
		
		for(pPrevFoundKey = 0, pSearchKey = this->ppHashHeads[uIndex];
			pSearchKey;
			pSearchKey = pSearchKey->pNext)
		{
			if(pFindKey->IsEqual(&pSearchKey->key))
			{
				rpPrevFoundKey = pPrevFoundKey;
				return pSearchKey;
			}
			
			pPrevFoundKey = pSearchKey;
		}
		
		return 0;
	}
	
public:
	//SimpleHash(){this->paHashHeads = 0;this->paHashHeadOrg = 0;this->uPow2NumKeys = 0;this->uNumKeysUsed = 0;};
	SimpleHash(){this->ppHashHeads = 0;this->paHashHeadOrg = 0;this->uPow2NumKeys = 0;this->uNumKeysUsed = 0;};
	~SimpleHash()
	{
		if(uPow2NumKeys)
		{
			delete[] this->ppHashHeads;
			delete[] this->paHashHeadOrg;
		}
	}
	
	void Init(u32 uMaxNumDataElements)
	{
		u32		uCount;
		//handle re-init...
		if(uPow2NumKeys)
		{
			delete[] this->ppHashHeads;
			delete[] this->paHashHeadOrg;
		}
		
		//if we're more than halfway to the next power of two, go for the next one after...
		this->uPow2NumKeys = GetNextPow2(uMaxNumDataElements + (uMaxNumDataElements >> 1));
		
		/*this->paHashHeads = rage_new DLListSimple<KeyBase>[this->uPow2NumKeys];
		this->paHashHeadOrg = rage_new KeyBase[uMaxNumDataElements];*/
		this->ppHashHeads = rage_new KeyBase *[this->uPow2NumKeys];
		this->pUnusedHashHead = this->paHashHeadOrg = rage_new KeyBase[uMaxNumDataElements];
		
		this->uNumKeysUsed = 0;
		
		//this->unusedHashHead.SetAndInitPool(this->paHashHeadOrg, uMaxNumDataElements);
		for(uCount = 0;uCount < uMaxNumDataElements - 1;uCount++)
		{
			this->paHashHeadOrg[uCount].pNext = &this->paHashHeadOrg[uCount + 1];
		}
		this->paHashHeadOrg[uCount].pNext = 0;

		memset(ppHashHeads, 0x00, sizeof(KeyBase *) * this->uPow2NumKeys);
	}
	
	static inline u32 SampleHash(u32 uKey)
	{
		uKey += (uKey << 12);
		uKey ^= (uKey >> 22);
		uKey += (uKey << 4);
		uKey ^= (uKey >> 9);
		uKey += (uKey << 10);
		uKey ^= (uKey >> 2);
		uKey += (uKey << 7);
		uKey ^= (uKey >> 12);
		uKey += !uKey; //Keep 0 from being a valid hash - this has some uses...
		return uKey;
	};
	
	//this will return the actual owned key, not a copy...
	//WARNING - YOU MUST NOT SCREW WITH THE PART OF THE KEY THAT IS USED FOR THE IsEqual FUNCTION!
	inline _TKey *Search(const _TKey *pFindKey) const
	{
		u32		uIndex;
		
		KeyBase		*pSearchKey;
		
		uIndex = pFindKey->GenerateHash();
		uIndex &= (this->uPow2NumKeys - 1);
		pSearchKey = this->Search(pFindKey, uIndex);
		
		return pSearchKey ? &pSearchKey->key : 0;
	}
	
	//this version is quick, but does not keep any kind of continuity among same-keyed elements...
	inline _TKey *Insert(const _TKey &pInsertKey)
	{
		u32		uIndex;
		
		KeyBase		*pKeyBase;
		
		//FastAssert(this->unusedHashHead.GetFirst());
		FastAssert(this->pUnusedHashHead);
		uIndex = pInsertKey.GenerateHash();
		uIndex &= (this->uPow2NumKeys - 1);
		
		/*pKeyBase = this->unusedHashHead.GetFirst();
		this->unusedHashHead.Remove(pKeyBase);
		this->paHashHeads[uIndex].AddToHead(pKeyBase);*/
		pKeyBase = this->pUnusedHashHead;
		this->pUnusedHashHead = pKeyBase->pNext;
		pKeyBase->pNext = this->ppHashHeads[uIndex];
		this->ppHashHeads[uIndex] = pKeyBase;
		
		pKeyBase->key = pInsertKey;
		
		this->uNumKeysUsed++;
		
		return &pKeyBase->key;
	}

	//this version is here for backward compatibility; don't use it
	inline _TKey *Insert(const _TKey *pInsertKey)
	{
		return Insert(*pInsertKey);
	}
	

#if 0	// This code does not compile because pFindKey is undefined!
	//this version is not as quick as Insert, but makes sure that same-keyed elements are contiguous(UNTESTED!)...
	inline _TKey *InsertContig(const _TKey *pInsertKey)
	{
		u32		uIndex;
		
		KeyBase		*pKeyBase;
		KeyBase		*pSearchKey;
		
		//FastAssert(this->unusedHashHead.GetFirst());
		FastAssert(this->pUnusedHashHead);
		uIndex = pInsertKey->GenerateHash();
		uIndex &= (this->uPow2NumKeys - 1);
		
		/*pKeyBase = this->unusedHashHead.GetFirst();
		pKeyBase->key = *pInsertKey;
		this->unusedHashHead.Remove(pKeyBase);*/
		pKeyBase = this->pUnusedHashHead;
		this->pUnusedHashHead = pKeyBase->pNext;
		
		//here's what slows us down a bit...
		pSearchKey = this->Search(pFindKey, uIndex);
		if(pSearchKey)
		{
			//this->paHashHeads[uIndex].AddAfter(pKeyBase, pSearchKey);
			pKeyBase->pNext = pSearchKey->pNext;
			pSearchKey->pNext = pKeyBase;
		}
		else
		{
			//this->paHashHeads[uIndex].AddToHead(pKeyBase);
			pKeyBase->pNext = this->ppHashHeads[uIndex];
			this->ppHashHeads[uIndex] = pKeyBase;
		}
		
		this->uNumKeysUsed++;
		
		return &pKeyBase->key;
	}
#endif
	
	//could make possibly faster by checking ::IsKeyOwned before searching - we may have been given the original...
	inline bool Delete(const _TKey *pDeleteKey)
	{
		u32		uIndex;
		
		KeyBase		*pDeleteKeyBase;
		KeyBase		*pPrevDeleteKeyBase;
		
		uIndex = pDeleteKey->GenerateHash();
		uIndex &= (this->uPow2NumKeys - 1);
		
		pDeleteKeyBase = this->Search(pDeleteKey, uIndex, pPrevDeleteKeyBase);
		if(pDeleteKeyBase)
		{
			if(pPrevDeleteKeyBase)
			{
				//we're in the middle somewhere...
				pPrevDeleteKeyBase->pNext = pDeleteKeyBase->pNext;
			}
			else
			{
				//we're the first entry(and possibly the last)...
				this->ppHashHeads[uIndex] = pDeleteKeyBase->pNext;
			}
			
			//add pDeleteKeyBase back to the unused list...
			pDeleteKeyBase->pNext = pUnusedHashHead;
			pUnusedHashHead = pDeleteKeyBase;
			
			/*this->paHashHeads[uIndex].Remove(pDeleteKeyBase);
			this->unusedHashHead.AddToHead(pDeleteKeyBase);*/
			
			FastAssert(this->uNumKeysUsed);
			this->uNumKeysUsed--;
			
			return true;
		}
		else
		{
			return false;
		}
	}
	
	//useful for debugging...
	inline bool IsKeyOwned(const _TKey *pDeleteKey) const
	{
		if(pDeleteKey < this->paHashHeadOrg)
		{
			return false;
		}
		else if(pDeleteKey >= (this->paHashHeadOrg + this->uPow2NumKeys))
		{
			return false;
		}
		return true;
	}
};

#if __WIN32
#pragma warning(pop)
#endif

}	// namespace rage

#endif
