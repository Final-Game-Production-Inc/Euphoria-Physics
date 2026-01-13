//
// atl/bitset.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_BITSET_H
#define ATL_BITSET_H

#include "data/serialize.h"
#include "data/struct.h"
#include "data/resource.h"
#include "math/amath.h"
#include "math/intrinsics.h"
#include "system/bit.h"
#include "system/endian.h"
#include "system/interlocked.h"
#include "system/memory.h"

#include <type_traits>


namespace rage {

	namespace FixedBitSet{
		// Precomputed bit count table, used to quickly bit count..
		extern const u8 BitCountTable[];
	};

template <int _T, typename _SU> class atFixedBitSet;

// broken out so that it can be selectively disabled separately if it's a time sink
#ifndef BitSetAssert
#if !__FINAL
#define BitSetAssert(x) FastAssert(x)
# else
# define BitSetAssert(x)
# endif
#endif

/*
 PURPOSE
	atBitSet maintains an array of bit flags on your behalf with range checking.
	Memory is dynamically allocated.
<FLAG Component>
*/
template<typename _CounterType>
class atBitSetCore {
public:
	// PURPOSE: Reset all bits in bitset to the specified value
	void Reset(const int value = 0) {	
		sysMemSet( m_Bits, value, m_Size*4 );
	}

	// PURPOSE: Initialize bitset to specified size
	// PARAMS: maxBits - Maximum number of bits in the bitset
	// NOTES: Bitset is zeroed out after initialization
	void Init(int maxBits) {
		delete[] m_Bits;

		// Alert users if we won't be able to make the
		// bitset that big. /FF/MAK
		// Note that some compilers will complain if we do 1 << 32 on ints, so this test is a u64.
		BitSetAssert((u64) maxBits <= (u64) ((1ULL << (8 * sizeof(_CounterType))) - 1)); 

		m_BitSize = (_CounterType) maxBits;
		m_Size = (_CounterType) ((maxBits+31)>>5);
		m_Bits = rage_new unsigned[m_Size];
		Reset();
	}

#if __SPU
	void SetBits(unsigned *bits)
	{
		m_Bits = bits;
	}

	// RETURNS: The number of 32-bit words in this bit mask.
	// NOTE: This is just an public accessor for GetNumBitWords() for when SIMPLIFIED_ATL is enabled.
	_CounterType GetSizeInWords() const	{ return GetNumBitWords(); }

	unsigned *GetBitsPtr() const	{ return GetRawBitMask(); }
#endif

private:
	// PURPOSE: Internal copy helper.  Allocates array of same size and copies elements.
	// PARAMS: that - BitSet to copy
	void CopyFrom(const atBitSetCore<_CounterType> &that) {
		m_Bits = rage_new unsigned[m_Size = that.m_Size]; //lint !e423 creation of memory leak in assignment
		m_BitSize = that.m_BitSize;
		
		sysMemCpy( m_Bits, that.m_Bits, m_Size*4 );
	}

public:
	// PURPOSE: Reclaim memory associated with bitset
	void Kill() 
	{ 
		delete[] m_Bits; 
		m_Bits = 0; 
		m_BitSize = 0;
		m_Size = 0;
	}

	// PURPOSE: Default constructor, produces empty bitset
	atBitSetCore() { m_Bits = 0; m_BitSize = m_Size = 0; }

	// PURPOSE: Resource constructor
	explicit atBitSetCore(class datResource &rsc) {rsc.PointerFixup(m_Bits);}

	// PURPOSE: Construct bitset with specied size
	// PARAMS: maxBits - Maximum number of bits in the bitset
	atBitSetCore(int maxBits) { m_Bits=0; Init(maxBits); } 

	// PURPOSE: Construct an atBitSet from another atBitSet.  Original atBitSet is unchanged.
	// PARAMS: that - atBitSet to copy initial data from
	atBitSetCore(const atBitSetCore<_CounterType>& that) { CopyFrom(that); }

	// PURPOSE: Destructor
	~atBitSetCore() { Kill(); }

	// PURPOSE: Copy an atBitSet from another atBitSet.  Other atBitSet is unchanged, this atBitSet has its contents replaced.
	// PARAMS: that - atBitSet to copy data from
	atBitSetCore& operator=(const atBitSetCore<_CounterType>& that) { if (this != &that) { Kill(); CopyFrom(that); } return *this; }

	// PURPOSE: Returns true if bitsets are same (non-zero) length and have same bits set.
	// PARAMS: that - atBitSet to compare to
	bool operator==(const atBitSetCore<_CounterType>& that) const
	{ 
		if((m_BitSize != that.m_BitSize)
		   || !m_BitSize
		   || !that.m_BitSize)
		{
			return false;
		}

		return !memcmp(m_Bits, that.m_Bits, m_Size * sizeof(u32));
	}

	// PURPOSE: Turn on specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to set
	__forceinline atBitSetCore& Set(unsigned int i) { 
		BitSetAssert(i<m_BitSize); 
		m_Bits[i>>5] |= (1<<(i & 31)); 
		return *this;
	}

	// PURPOSE: Turn off specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to clear
	__forceinline atBitSetCore& Clear(unsigned int i) { 
		BitSetAssert(i<m_BitSize); 
		m_Bits[i>>5] &= ~(1<<(i & 31)); 
		return *this;
	}

	// PURPOSE: Toggle specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to clear
	__forceinline void Toggle(unsigned int i) { 
		BitSetAssert(i<m_BitSize); 
		m_Bits[i>>5] ^= (1<<(i & 31)); 
	}

	// PURPOSE: Set or clear specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to set or clear
	//			val - true to set bit, or false to clear it
	__forceinline void Set(unsigned int i, bool val) {
		BitSetAssert(i<m_BitSize);
		m_Bits[i>>5] ^= ((val ? 0xffffffff : 0) ^ m_Bits[i>>5]) & (1<<(i & 31));
	}

	// PURPOSE: Test specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to test
	// RETURNS: True if bit is set, else false
	__forceinline bool IsSet(unsigned int i) const { 
		BitSetAssert(i<m_BitSize); 
		return (m_Bits[i>>5] & (1<<(i&31))) != 0; 
	}

	// PURPOSE: Test specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to test
	// RETURNS: True if bit is clear, else false
	__forceinline bool IsClear(unsigned int i) const { 
		BitSetAssert(i<m_BitSize); 
		return (m_Bits[i>>5] & (1<<(i&31))) == 0; 
	}

	// PURPOSE: Set a bit and return its previous value (with range checking)
	// PARAMS: i - Zero-based index of bit to test and set
	// RETURNS: True if bit was previously set else false
	__forceinline bool GetAndSet(unsigned int i) {
		BitSetAssert(i<m_BitSize);
		bool value = ((m_Bits[i>>5] & (1<<(i&31))) != 0);
		m_Bits[i>>5] |= (1<<(i & 31)); 
		return value;
	}

	// PURPOSE: Clear a bit and return its previous value (with range checking)
	// PARAMS: i - Zero-based index of bit to test and clear
	// RETURNS: True if bit was previously set else false
	__forceinline bool GetAndClear(unsigned int i) {
		BitSetAssert(i<m_BitSize);
		bool value = ((m_Bits[i>>5] & (1<<(i&31))) != 0);
		m_Bits[i>>5] &= ~(1<<(i & 31));
		return value;
	}

	// PURPOSE: Determine if there is at least one bit set
	// PARAMS: None
	// RETURNS: True if any of the bits in the bitset are set
	bool AreAnySet() const {
		// no range checking because out of range bits are zero
		for (unsigned i=0; i<m_Size; i++) {
			if (m_Bits[i]) return true;
		}
		return false;
	}

	// PURPOSE: Boolean intersection operation: this = intersection of this and a (a result bit is set iff a.IsSet(bit) && this.IsSet(bit))
	// PARAMS: a - Bitset to intersect with
	// RETURNS: None
	void Intersect(const atBitSetCore<_CounterType>& a) {
		// Not using min here because including amath.h pulls in a bunch of other files
		_CounterType minSize = bsMin(m_Size, a.m_Size);
		for(int i = 0; i < minSize; ++i) {
			m_Bits[i] &= a.m_Bits[i];
		}
	}

	// PURPOSE: Boolean intersection operation: this = intersection of a and b (a result bit is set iff a.IsSet(bit) && b.IsSet(bit))
	// PARAMS: a - First bitset to intersect with
	//			b - Second bitset to intersect with
	// RETURNS: None
	void Intersect(const atBitSetCore<_CounterType>& a, const atBitSetCore<_CounterType>& b) {
		_CounterType minSize = bsMin(m_Size, a.m_Size);
		for(int i = 0; i < minSize; ++i) {
			m_Bits[i] = a.m_Bits[i] & b.m_Bits[i];
		}
	}

	// PURPOSE: Boolean intersection operation: this = intersection of this and a (a result bit is set iff a.IsSet(bit) && this.IsSet(bit))
	// PARAMS: a - Bitset to intersect with
	// RETURNS: None
	template <int _ArraySize>
	void Intersect(const atFixedBitSet<_ArraySize, u32>& a) {
		_CounterType minSize = bsMin(m_Size, (_CounterType)(((_ArraySize)+31)>>5));
		for(int i = 0; i < minSize; ++i) {
			m_Bits[i] &= a.GetWord(i);
		}
	}

	// PURPOSE: Boolean union operation: this = union of this and a (a result bit is set iff a.IsSet(bit) || this.IsSet(bit))
	// PARAMS: a - Bitset to intersect with
	// RETURNS: None
	void Union(const atBitSetCore<_CounterType>& a) {
		_CounterType minSize = bsMin(m_Size, a.m_Size);
		for(int i = 0; i < minSize; ++i) {
			m_Bits[i] |= a.m_Bits[i];
		}
	}

	// PURPOSE: Boolean union operation: this = union of a and b (a result bit is set iff a.IsSet(bit) || b.IsSet(bit))
	// PARAMS: a - First bitset to intersect with
	//			b - Second bitset to intersect with
	// RETURNS: None
	void Union(const atBitSetCore<_CounterType>& a, const atBitSetCore<_CounterType>& b) {
		_CounterType minSize = bsMin(a.m_Size, b.m_Size);
		for(int i = 0; i < minSize; ++i) {
			m_Bits[i] = a.m_Bits[i] | b.m_Bits[i];
		}
	}

	// PURPOSE: Boolean union operation: this = union of this and a (a result bit is set iff a.IsSet(bit) || this.IsSet(bit))
	// PARAMS: a - Bitset to intersect with
	// RETURNS: None
	template <int _ArraySize>
	void Union(const atFixedBitSet<_ArraySize, u32>& a) {
		_CounterType minSize = bsMin(m_Size, (_CounterType)(((_ArraySize)+31)>>5));
		for(int i = 0; i < minSize; ++i) {
			m_Bits[i] |= a.GetWord(i);
		}
	}

	// PURPOSE: Returns number of bits in the bitset
	// RETURNS: Returns number of bits in the bitset
	int GetNumBits() const { return m_BitSize; }

	// PURPOSE: Counts the number of bits of in the given state
	// PARAMS: val - bit state to count for
	// RETURNS: number of bits set to the given state, returns 0 if none are set
	int CountBits(bool val=true) const {
		int count = CountOnBits();

		if (!val)
		{
			return GetNumBits()-count;
		}

		return count;
	}  

	// PURPOSE: Counts the number of bits on
	// RETURNS: number of bits set to the on state, returns 0 if none are set
	int CountOnBits() const {
		int count = 0;
		// no range checking because out of range bits are zero
		for(unsigned int i = 0; i < m_Size; ++i) {
			count += ::rage::CountOnBits(m_Bits[i]);
		}
		return count;
	}  

	// PURPOSE: Gets the nth on bit index, return -1 of it can't find one.
	// PARAMS: nth - the nth on bit whose index we want
	// RETURNS: the index of the nth on bit, returns -1 if it doesn't exit.
	int GetNthBitIndex(unsigned int nth) const {
		BitSetAssert(nth<m_BitSize);
		unsigned int count = 0;
		// no range checking because out of range bits are zero
		for(unsigned int i = 0; i < m_Size; ++i) {
			u8 * rawPtr = (u8 *) &(m_Bits[i]);

			if (*((u32*)rawPtr)==0)
			{
				continue;
			}

			for (int j = (__BE ? 3  : 0); j != (__BE ? -1  : 4); j+=(__BE ? -1 : 1))
			{
				count += FixedBitSet::BitCountTable[rawPtr[j]]; 
				if (count>nth)
				{
					int subCount = count-nth;

					for (int bit = 7; bit >= 0; bit--)
					{
						if (rawPtr[j]&(1<<bit))
						{
							subCount--;
						}
						if (subCount==0)
						{
							return (i*32)+bit+((__BE ? 3-j : j)*8);
						}
					}
				}
			}
		}
		return -1;
	}

	/** PURPOSE: Returns the next set bit in this bitset. You can use GetFirstBitIndex to get the index of the
	 *           first bit in the set that's set.
	 *           NOTE that the numbers will not necessarily be sequential. If all bits are set, for example, GetFirstBitIndex()
	 *           will return 31, and the first call to this function will return 30, then 29, until it gets to 0, and then
	 *           move on to 63, 62, 61...
	 *  PARAMS: lastBitSet - this is the index of a bit in this bitset. The returned value will be the next higher index
	 *                       of a set bit.
	 *  RETURNS: The next bit that is currently set and higher than lastBitSet, or -1 if there are no more set bits.
	 */
	int GetNextBitIndex(int lastBitSet) const {

		u32 count = m_Size;
		u32 curIdx = lastBitSet / 32;
		// Get the word where the last occurrence was.
		u32 bit = m_Bits[curIdx];

		// Mask out the previous bits.
		// Okay, let's go through the logic here. Let's say our last bit set was 1. That means we have to mask out bit 0 and 1,
		// i.e. mask out 0x3. 
		u32 bitIndex = lastBitSet & 31;
		u32 bitMask = (1U << bitIndex) - 1;

		bit &= bitMask;

		// If we exhausted all the bits in this word, get the next one.
		while (!bit) {
			if (++curIdx == count) {
				// No more bits.
				return -1;
			}

			bit = m_Bits[curIdx];
		}

		// Okay, we have a non-zero word. Let's get the next set bit.
		int index = ::_FirstBitIndex(bit);
		FastAssert(index >= 0);		// We KNOW bit is non-zero.

		int result = index + curIdx*(sizeof(m_Bits[0])<<3);

		return (result < (int) m_BitSize) ? result : -1;
	}

	int GetFirstBitIndex(bool val = true) const {
		if (val)
		{
			for (_CounterType i = 0; i < m_Size; ++i)
			{
				int index = ::_FirstBitIndex(m_Bits[i]);
				if (index >= 0)
				{
					int ret = index + i * (sizeof( m_Bits[0] ) << 3);
					if( (_CounterType)ret < m_BitSize )
					{
						return ret;
					}
					else
					{
						return -1;
					}
				}
			}
		}
		else
		{
			for (_CounterType i = 0; i < m_Size; ++i)
			{
				int index = ::_FirstBitIndex(~m_Bits[i]);
				if (index >= 0)
				{
					int ret = index + i*(sizeof(m_Bits[0])<<3);
					if( (_CounterType)ret < m_BitSize )
					{
						return ret;
					}
					else
					{
						return -1;
					}
				}
			}
		}

		return -1;
	}

	int GetLastBitIndex(bool val = true) const {
		if (val)
		{
			for (int i = m_Size - 1; i >= 0; --i)
			{
				int index = ::_LastBitIndex(m_Bits[i]);
				if (index >= 0)
				{
					return index + i*(sizeof(m_Bits[0])<<3);
				}
			}
		}
		else
		{
			for (int i = m_Size - 1; i >= 0; --i)
			{
				int index = ::_LastBitIndex(~m_Bits[i]);
				if (index >= 0)
				{
					return index + i*(sizeof(m_Bits[0])<<3);
				}
			}
		}

		return -1;
	}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s)
	{
		STRUCT_BEGIN(atBitSetCore);
		if(m_Bits)
		{
			for (int i=0; i<m_Size; i++) 
				datSwapper(m_Bits[i]); 
			s.AddField((size_t)&m_Bits-(size_t)this,sizeof(m_Bits));
			datSwapper((void*&)m_Bits);
		}
		STRUCT_FIELD(m_Size);
		STRUCT_FIELD(m_BitSize);
		STRUCT_END();
	}
#endif

	// Not using amath's min here because #including that pulls in a bunch of other stuff.
	template <class Type> inline Type bsMin(Type a,Type b)
	{
		return a<b?a:b;
	}

protected:
	// RETURNS: The raw memory area with all bits.
	unsigned *GetRawBitMask() const	{ return m_Bits; }

	// RETURNS: The number of 32-bit words in this bit mask.
	_CounterType GetNumBitWords() const	{ return m_Size; }

	unsigned *m_Bits;			// Storage for the bitset (assumes unsigned is a 32-bit integer)
	_CounterType m_Size,		// Current size of the bitset, in 32-bit words
				m_BitSize;		// Current size of the bitset in bits.  Used for exact range checking.

	// Serialize method -- use the constructor that allows automatic array allocations 
	// friend inline datSerialize & operator<< ( datSerialize &ser, atBitSet &bit );

	// The fixed bit set needs access to the raw data, so we'll let it.
	template<int, typename>
		friend class atFixedBitSet;
	// so does the parser
	friend class parMemberBitset;
	
};

class atBitSet : public atBitSetCore<u16>
{
public:
	atBitSet() : atBitSetCore<u16>() {}
	explicit atBitSet(class datResource &rsc) : atBitSetCore<u16>(rsc) {}
	atBitSet(int maxBits) : atBitSetCore<u16>(maxBits) {}
	atBitSet(const atBitSet& that) : atBitSetCore<u16>(that) {}
};

class atBitSet32 : public atBitSetCore<u32>
{
public:
	atBitSet32() : atBitSetCore<u32>() {}
	explicit atBitSet32(class datResource &rsc) : atBitSetCore<u32>(rsc) {}
	atBitSet32(int maxBits) : atBitSetCore<u32>(maxBits) {}
	atBitSet32(const atBitSet32& that) : atBitSetCore<u32>(that) {}
};

/* datSerialize & operator<< ( datSerialize &ser, atBitSet &bit ) 
{
	ser << bit.m_BitSize << datArray<unsigned, unsigned short>(&bit.m_Bits, &bit.m_Size);
	return ser;
} */


// PURPOSE: This class is identical to atBitSet, however, the user is responsible
// for the creation and deletion of the array that contains all bits.
// NOTE: Calling certain base-level functions such as Reset() is obviously
// pretty dumb since those functions will delete the user-provided array.
class atUserBitSet : public atBitSet
{
public:

	atUserBitSet() { }

	atUserBitSet(unsigned *bits, int maxBits) {
		Init( bits, maxBits );
	}

	~atUserBitSet() {
		// Clears the m_Bits pointer so that the base destructor
		// will not try to delete it.
		m_Bits = 0;
	}

	void Init(unsigned *bits, int maxBits) {
		m_Bits = bits;
		m_BitSize = (unsigned short) maxBits;
		m_Size = (unsigned short) ((maxBits + 31) >> 5);
	}
};


template<int, typename> class atFixedBitSetIterator;


/*
PURPOSE
atFixedBitSet maintains an array of bit flags on your behalf with range checking.
	Memory is part of the object, and so is more efficient if the size of the bitset
	can be known at compile time.
<FLAG Component>
*/
template <int _ArraySize, typename _BlockType=u32> class atFixedBitSetBase {
	template<int X, typename Y> friend class atFixedBitSetIterator;
protected:
	static const int _BlockSize = sizeof(_BlockType);
	static const int _BitsPerBlock = _BlockSize*8;
	static const int _BlockBitShift = CompileTimeLog2Floor<_BitsPerBlock>::value;
	static const int _NumBlocks = (((_ArraySize)+(_BitsPerBlock-1))>>_BlockBitShift);

public:

	//Compile-time access to _ArraySize
	enum
	{
		NUM_BITS        = _ArraySize,
		NUM_BLOCKS		= _NumBlocks,
		BLOCK_SIZE		= _BlockSize
	};

	// PURPOSE: Default constructor
	atFixedBitSetBase() { 
#if !__SPU
		if (!datResource_sm_Current) 
#endif
			Reset(); 
	}

	// PURPOSE: Resource constructor
	explicit atFixedBitSetBase(datResource&) {}

	explicit atFixedBitSetBase(bool isSet)
	{
		isSet ? SetAll() : Reset();
	}

	typedef atFixedBitSetBase<_ArraySize, _BlockType> myType;

	static void Place(void* that, datResource& rsc)
	{
		rage_placement_new(that) atFixedBitSetBase<_ArraySize, _BlockType>(rsc);
	}

	// PURPOSE: Clear all bits in the bitset
	void Reset() {
		memset(m_Bits, 0, sizeof(m_Bits));
	}

	void SetAll() {
		memset(m_Bits, 0xff, sizeof(m_Bits));
		ClearUndefinedBits();
	}

	// PURPOSE: Returns the size of the bitset
	// RETURNS: size of the bitset.
	int GetNumBits() const {
		return _ArraySize;
	}

	// PURPOSE: Turn on specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to set
	__forceinline myType& Set(unsigned int i) {
		BitSetAssert(i<_ArraySize);
		Block(i) |= BitInBlock(i);
		return *this;
	}

	// PURPOSE: Turn off specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to clear
	__forceinline myType& Clear(unsigned int i) {
		BitSetAssert(i<_ArraySize);
		Block(i) &= ~BitInBlock(i);
		return *this;
	}

	// PURPOSE: Atomically turn on specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to set
	__forceinline myType& AtomicSet(unsigned int i) {
		BitSetAssert(i<_ArraySize);
		sysInterlockedOr(&Block(i), BitInBlock(i));
		return *this;
	}

	// PURPOSE: Atomically turn off specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to clear
	__forceinline myType& AtomicClear(unsigned int i) {
		BitSetAssert(i<_ArraySize);
		sysInterlockedAnd(&Block(i), ~BitInBlock(i));
		return *this;
	}

	// PURPOSE: Set or clear specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to set or clear
	//			val - true to set bit, or false to clear it
	__forceinline void Set(unsigned int i, bool val) {
		BitSetAssert(i<_ArraySize);
		Block(i) ^= (-(int)val ^ Block(i)) & BitInBlock(i);
	}

	// PURPOSE: Test specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to test
	// RETURNS: True if bit is set, else false
	__forceinline bool IsSet(unsigned int i) const {
		BitSetAssert(i<_ArraySize);
		return (Block(i) & BitInBlock(i)) != 0;
	}
	
	// PURPOSE: Test specified bit, with range checking
	// PARAMS: i - Zero-based index of bit to test
	// RETURNS: True if bit is clear, else false
	__forceinline bool IsClear(unsigned int i) const {
		BitSetAssert(i<_ArraySize);
		return (Block(i) & BitInBlock(i)) == 0;
	}

	// PURPOSE: Set a bit and return its previous value (with range checking)
	// PARAMS: i - Zero-based index of bit to test and set
	// RETURNS: True if bit was previously set else false
	__forceinline bool GetAndSet(unsigned int i) {
		BitSetAssert(i<_ArraySize);
		bool value = ((Block(i) & BitInBlock(i)) != 0);
		Block(i) |= BitInBlock(i);
		return value;
	}

	// PURPOSE: Clear a bit and return its previous value (with range checking)
	// PARAMS: i - Zero-based index of bit to test and clear
	// RETURNS: True if bit was previously set else false
	__forceinline bool GetAndClear(unsigned int i) {
		BitSetAssert(i<_ArraySize);
		bool value = ((Block(i) & BitInBlock(i)) != 0);
		Block(i) &= ~BitInBlock(i);
		return value;
	}

	// PURPOSE: Copy the data from another fixed-size array with identical bit count.
	myType &operator =(const myType &src) {
		for (int i=0; i<_NumBlocks; i++)
			m_Bits[i] = src.m_Bits[i];
		return *this;
	}

	bool operator ==(const myType &src) const {
		for (int i=0; i<_NumBlocks; i++)
			if(m_Bits[i] != src.m_Bits[i]) return false;
		return true;
	}

	bool operator !=(const myType &src) const {
		return !operator==(src);
	}

	// PURPOSE: Determine if there is at least one bit set
	// PARAMS: None
	// RETURNS: True if any of the bits in the bitset are set
	bool AreAnySet() const {
		// no range checking because out of range bits are zero
		for(int i = 0; i < _NumBlocks; ++i) {
			if (m_Bits[i]) return true;
		}
		return false;
	}
	
	// PURPOSE: checks if both sets are equal
	// PARAMS: a - Bitset to test with
	// RETURNS: true if both sets are equal, false if they are not
	bool IsEqual(const myType& a) const {
		return operator==(a);
	}

	// PURPOSE: checks if either set overlaps any bits
	// PARAMS: a - Bitset to test with
	// RETURNS: true if both sets are equal, false if they are not
	bool DoesOverlap(const myType& a) const {
		// no range checking because out of range bits are zero
		for(int i = 0; i < _NumBlocks; ++i) {
			if (m_Bits[i] & a.m_Bits[i])
			{
				return true;
			}
		}
		return false;
	}

	// PURPOSE: checks if this bitset is a subset of the tested bitset
	// PARAMS: a - Bitset to test with
	// RETURNS: true if this bitset is a subset of the given bitset, false if its not
	bool IsSubsetOf(const myType& a) const {
		// no range checking because out of range bits are zero
		for(int i = 0; i < _NumBlocks; ++i) {
			if ((m_Bits[i] & a.m_Bits[i]) != m_Bits[i])
			{
				return false;
			}
		}
		return true;
	}

	// PURPOSE: checks if this bitset is a superset of the tested bitset
	// PARAMS: a - Bitset to test with
	// RETURNS:  true if this bitset is a superset of the given bitset, false if its not
	bool IsSuperSetOf(const myType& a) const {
		// no range checking because out of range bits are zero
		for(int i = 0; i < _NumBlocks; ++i) {
			if ((a.m_Bits[i] & m_Bits[i]) != a.m_Bits[i])
			{
				return false;
			}
		}
		return true;
	}

	// PURPOSE: Boolean intersection operation: this = intersection of this and negate a (a result bit is set iff ~(a.IsSet(bit)) && this.IsSet(bit))
	// PARAMS: a - Bitset to intersect with, is negated
	// RETURNS: None
	void IntersectNegate(const myType& a) {
		for(int i = 0; i < _NumBlocks; ++i) {
			m_Bits[i] &= ~(a.m_Bits[i]);
		}
	}

	// PURPOSE: Boolean intersection operation: this = intersection of a and negate b (a result bit is set iff a.IsSet(bit) && ~(b.IsSet(bit)))
	// PARAMS:  a - First bitset to intersect with
	//			b - Second bitset to intersect with, is negated
	// RETURNS: None
	void IntersectNegate(const myType& a, const myType& b) {
		for(int i = 0; i < _NumBlocks; ++i) {
			m_Bits[i] = a.m_Bits[i] & ~(b.m_Bits[i]);
		}
	}

	// PURPOSE: Boolean intersection operation: this = intersection of this and a (a result bit is set iff a.IsSet(bit) && this.IsSet(bit))
	// PARAMS: a - Bitset to intersect with
	// RETURNS: None
	void Intersect(const myType& a) {
		for(int i = 0; i < _NumBlocks; ++i) {
			m_Bits[i] &= a.m_Bits[i];
		}
	}

	// PURPOSE: Boolean intersection operation: this = intersection of a and b (a result bit is set iff a.IsSet(bit) && b.IsSet(bit))
	// PARAMS: a - First bitset to intersect with
	//			b - Second bitset to intersect with
	// RETURNS: None
	void Intersect(const myType& a, const myType& b) {
		for(int i = 0; i < _NumBlocks; ++i) {
			m_Bits[i] = a.m_Bits[i] & b.m_Bits[i];
		}
	}

	// PURPOSE: Boolean union operation: this = union of this and a (a result bit is set iff a.IsSet(bit) || this.IsSet(bit))
	// PARAMS: a - Bitset to intersect with
	// RETURNS: None
	void Union(const myType& a) {
		for(int i = 0; i < _NumBlocks; ++i) {
			m_Bits[i] |= a.m_Bits[i];
		}
	}

	// PURPOSE: Boolean union operation: this = union of a and b (a result bit is set iff a.IsSet(bit) || b.IsSet(bit))
	// PARAMS: a - First bitset to intersect with
	//			b - Second bitset to intersect with
	// RETURNS: None
	void Union(const myType& a, const myType& b) {
		for(int i = 0; i < _NumBlocks; ++i) {
			m_Bits[i] = a.m_Bits[i] | b.m_Bits[i];
		}
	}

	// PURPOSE: this = ~this
	// PARAMS: a - Bitset to get the negate of this.
	// RETURNS: None
	void Negate() {
		for(int i = 0; i < _NumBlocks; ++i) {
			m_Bits[i] = ~m_Bits[i];
		}
	}

	// PURPOSE: this = ~a
	// PARAMS: a - Bitset to get the negate of this.
	// RETURNS: None
	void Negate(const myType& a) {
		for(int i = 0; i < _NumBlocks; ++i) {
			m_Bits[i] = ~a.m_Bits[i];
		}
	}

	// PURPOSE: Sets the bits to _NumBlocks copies of the specified value (most useful when _NumBlocks == 1)
	// PARAMS: val - the value to set all the blocks to
	// RETURNS: None
	void SetBits(_BlockType val)
	{
		for(int i = 0; i < _NumBlocks; ++i)
		{
			m_Bits[i] = val;
		}
		ClearUndefinedBits();
	}

	// PURPOSE: Allows external code to access the raw data array used for serialization
	// PARAMS:  data - ptr to ptr returning the array
	//			size - ptr to size element of the array
	// RETURNS: true if succeeds, false if it fails.
	bool    GetRaw(_BlockType** data, u32* size)
	{
		if (data==NULL || size==NULL)
		{
			return false;
		}

		*data       = m_Bits;
		*size       = _NumBlocks;

		return true;
	}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s)
	{
		STRUCT_BEGIN(atFixedBitSet);
		STRUCT_CONTAINED_ARRAY(m_Bits);
		STRUCT_END();
	}
#endif

	// PURPOSE: Allows external code to access the raw data array used for serialization
	// PARAMS:  data - ptr to ptr returning the array
	//			size - ptr to size element of the array
	// RETURNS: true if succeeds, false if it fails.
	bool    GetRaw(const _BlockType** data, u32* size) const
	{
		if (data==NULL || size==NULL)
		{
			return false;
		}

		*data       = m_Bits;
		*size       = _NumBlocks;

		return true;
	}

	_BlockType GetRawBlock(u32 blockIndex) const
	{
		FastAssert(blockIndex < _NumBlocks);
		return m_Bits[blockIndex];
	}

	// PURPOSE: Allows external code to directly set the raw data array
	// PARAMS:  data - the ptr to the block of data to copy over.
	//			size - ptr to size element of the array
	// RETURNS: true if succeeds, false if it fails.
	bool SetRawBitMask(const _BlockType* data, u32 size)
	{
		if (data==NULL || size==_NumBlocks)
		{
			return false;
		}

		sysMemCpy( m_Bits, data, _NumBlocks*4 );

		return true;
	}

	void SetRawBlock(u32 blockIndex, _BlockType val)
	{
		FastAssert(blockIndex < _NumBlocks);
		m_Bits[blockIndex] = val;
	}


	// RETURNS: The raw memory area with all bits.
	_BlockType* GetRawBitMask() { return m_Bits; }
	const _BlockType* GetRawBitMask() const { return m_Bits; }

	int GetFirstBitIndex(bool val = true) const
	{
		if (val)
		{
			for (int i = 0; i < _NumBlocks; ++i)
			{
				_BlockType bits = m_Bits[i];
				if (bits)
				{
					_BlockType b = bits & (_BlockType)-(SIGNED_TYPE(_BlockType))bits;
					return Log2Floor(b) + i*_BitsPerBlock;
				}
			}
		}
		else
		{
			for (int i = 0; i < _NumBlocks; ++i)
			{
				_BlockType bits = m_Bits[i];
				if (bits != ~(_BlockType)0)
				{
					_BlockType b = ~bits & (bits+1);
					return Log2Floor(b) + i*_BitsPerBlock;
				}
			}
		}

		return -1;
	}

	int GetLastBitIndex(bool val = true) const
	{
		if (val)
		{
			for (int i = _NumBlocks - 1; i >= 0; --i)
			{
				_BlockType bits = m_Bits[i];
				if (bits)
				{
					return Log2Floor(bits) + i*_BitsPerBlock;
				}
			}
		}
		else
		{
			for (int i = _NumBlocks - 1; i >= 0; --i)
			{
				_BlockType bits = m_Bits[i];
				if (bits != ~(_BlockType)0)
				{
					return Log2Floor(~bits) + i*_BitsPerBlock;
				}
			}
		}

		return -1;
	}

	int AtomicFindSetBitIndexAndClear()
	{
		for (int i = 0; i < _NumBlocks; ++i)
		{
			for (;;)
			{
				// No bits set, break out loop to the next m_Bits value
				_BlockType bits = const_cast<volatile _BlockType*>(m_Bits)[i];
				if (!bits)
				{
					break;
				}

				// Try to clear the bit
				_BlockType mask = bits & (_BlockType)-(SIGNED_TYPE(_BlockType))bits;
				_BlockType orig = sysInterlockedAnd(m_Bits+i, ~mask);
				if (orig & mask)
				{
					// Successfully cleared the bit
					return Log2Floor(mask) + i*_BitsPerBlock;
				}

				// Someone else just cleared the bit, try to find another bit in
				// m_Bits[i] before checking m_Bits[i+1].
			}
		}

		// Not able to find a set bit
		return -1;
	}

	int AtomicFindClearBitIndexAndSet()
	{
		for (int i = 0; i < _NumBlocks; ++i)
		{
			for (;;)
			{
				// No bits clear, break out loop to the next m_Bits value
				_BlockType bits = const_cast<volatile _BlockType*>(m_Bits)[i];
				if (bits == ~(_BlockType)0)
				{
					break;
				}

				// Try to set the bit
				_BlockType mask = ~bits & (bits+1);
				_BlockType orig = sysInterlockedOr(m_Bits+i, mask);
				if (~orig & mask)
				{
					// Successfully set the bit
					return Log2Floor(mask) + i*_BitsPerBlock;
				}

				// Someone else just set the bit, try to find another bit in
				// m_Bits[i] before checking m_Bits[i+1].
			}
		}

		// Not able to find a clear bit
		return -1;
	}

	atFixedBitSetIterator<_ArraySize, _BlockType> Begin() const;

	int End() const
	{
		return -1;
	}

protected:
	_BlockType m_Bits[_NumBlocks];		// Storage for the bitset

	__forceinline void ClearUndefinedBits()
	{
		// zero out any bits off the end
		// numOnes is the number of 1s we want in the mask
		const int numOnes = ((_ArraySize-1) % _BitsPerBlock) + 1;		// numOnes is 1..32
		const _BlockType ones = (_BlockType)~(_BlockType)0x0u;;			// ones = 0xFFFFFFFF
		const _BlockType mask = ones >> (_BitsPerBlock - numOnes);		// Shift by (32 - (1..32)) = (31..0)
		m_Bits[_NumBlocks-1] &= mask;
	}

	// gets the block containing bit i
	__forceinline _BlockType& Block(unsigned int i) {return m_Bits[i >> _BlockBitShift];}
	__forceinline const _BlockType& Block(unsigned int i) const {return m_Bits[i >> _BlockBitShift];}

	// returns which (shifted) bit in Block(i) corresponds to i
	__forceinline _BlockType BitInBlock(unsigned int i) const {return (_BlockType)1 << (i & (_BitsPerBlock-1));}

};

// PURPOSE: The standard (non-specialized) implementation of atFixedBitSet
template <int _ArraySize, typename _BlockType=u32> 
class atFixedBitSet : public atFixedBitSetBase<_ArraySize, _BlockType>
{
public:
	// PURPOSE: Default constructor
	atFixedBitSet() : atFixedBitSetBase<_ArraySize, _BlockType>() {}

	// PURPOSE: Resource constructor
	explicit atFixedBitSet(datResource& rsc) : atFixedBitSetBase<_ArraySize, _BlockType>(rsc) {}

	explicit atFixedBitSet(bool isSet) : atFixedBitSetBase<_ArraySize, _BlockType>(isSet) {}

	typedef atFixedBitSetBase<_ArraySize, _BlockType> myBaseType;
	typedef atFixedBitSet<_ArraySize, _BlockType> myType;

	__forceinline myType& Set(unsigned int i) {
		BitSetAssert(i<_ArraySize);
		myBaseType::Block(i) |= myBaseType::BitInBlock(i);
		return *this;
	}

	__forceinline void Set(unsigned int i, bool val) {
		BitSetAssert(i<_ArraySize);
		myBaseType::Block(i) ^= (-(int)val ^ myBaseType::Block(i)) & myBaseType::BitInBlock(i);
	}

	__forceinline myType& Clear(unsigned int i) {
		BitSetAssert(i<_ArraySize);
		myBaseType::Block(i) &= ~myBaseType::BitInBlock(i);
		return *this;
	}

	myType& operator= (const myType& other)
	{
		static_cast<myBaseType*>(this)->operator =(static_cast<const myBaseType&>(other));
		return *this;
	}

	atFixedBitSet(const myType& other)
	{
		*this = other;
	}
};

#define bsWordSize                              (((_ArraySize)+31)>>5)

// PURPOSE: The specialization of atFixedBitSet using 32-bit blocks.
// Any operations on 32-bit only bit sets (or operations that are _faster_ for 32-bit only bitsets) go here.
template<int _ArraySize> class atFixedBitSet<_ArraySize, u32> : public atFixedBitSetBase<_ArraySize, u32>
{
public:

	// PURPOSE: Default constructor
	atFixedBitSet() : atFixedBitSetBase<_ArraySize, u32>() {}

	// PURPOSE: Resource constructor
	explicit atFixedBitSet(datResource& rsc) : atFixedBitSetBase<_ArraySize, u32>(rsc) {}

	// PURPOSE: Initialize this bitset from an atBitSet.
	// NOTE: It is legal to initialize from an array that is smaller than this array.
	// The state of the bits beyond the range of source array will be undefined.
	// It is NOT legal to initialize from an array that exceeds this array's
	// size.
	explicit atFixedBitSet<_ArraySize, u32>(const atBitSet &bitSet) 
	{
		InitFrom(bitSet);
	}

	explicit atFixedBitSet(bool isSet) : atFixedBitSetBase<_ArraySize, u32>(0)
	{
		isSet ? SetAll() : Reset();
	}

	typedef atFixedBitSetBase<_ArraySize, u32> myBaseType;
	typedef atFixedBitSet<_ArraySize, u32> myType;

	myType& operator= (const myType& other)
	{
		static_cast<myBaseType*>(this)->operator =(static_cast<const myBaseType&>(other));
		return *this;
	}

	atFixedBitSet(const myType& other)
	{
		*this = other;
	}

	bool operator== (const myType& other) const
	{
		return static_cast<const myBaseType*>(this)->operator ==(static_cast<const myBaseType&>(other));
	}

	__forceinline myType& Set(unsigned int i) {
		BitSetAssert(i<_ArraySize);
		myBaseType::Block(i) |= myBaseType::BitInBlock(i);
		return *this;
	}

	__forceinline void Set(unsigned int i, bool val) {
		BitSetAssert(i<_ArraySize);
		myBaseType::Block(i) ^= (-(int)val ^ myBaseType::Block(i)) & myBaseType::BitInBlock(i);
	}

	__forceinline myType& Clear(unsigned int i) {
		BitSetAssert(i<_ArraySize);
		myBaseType::Block(i) &= ~myBaseType::BitInBlock(i);
		return *this;
	}

	// PURPOSE: Clear all bits in the bitset
	void Reset() {
		sysMemZeroWords<myBaseType::_NumBlocks>(myBaseType::m_Bits);
	}

	// PURPOSE: Clear at least the first size bits in the bitset
	void Reset(int size) {
		FastAssert(size <= _ArraySize);
		size = (((size)+31)>>5);
		sysMemSet( myBaseType::m_Bits, 0, (size)*4 );
	}

	// PURPOSE: Set all bits in the bitset
	void SetAll() {
		if (_ArraySize & 31)
		{
			sysMemSet( myBaseType::m_Bits, 0xFF, (bsWordSize - 1)*4 );
			myBaseType::m_Bits[bsWordSize - 1] = u32(1 << (_ArraySize & 31)) - 1;
		}
		else
		{
			sysMemSet( myBaseType::m_Bits, 0xFF, (bsWordSize)*4 );
		}
	}

	// PURPOSE: Initialize this bitset from an atBitSet.
	// See the constructor for more information.
	void InitFrom(const atBitSet &bitSet) {
		FastAssert(bitSet.GetNumBits() <= _ArraySize);

		u32 *srcBits = (u32 *) bitSet.GetRawBitMask();
		int words = bitSet.GetNumBitWords();
		for (int i=0; i<words; i++)
			myBaseType::m_Bits[i] = srcBits[i];
	}

	// PURPOSE: Copy this bitmask into an atBitSet. The atBitSet is assumed
	// to be pre-initialized with the proper size.
	// NOTE: It is legal to copy into an array that is bigger than this one -
	// the bits past the ones in this array (in 32-bit groups) will not be
	// touched. It is NOT legal to copy into an array that is smaller than
	// this one.
	void CopyIntoFast(atBitSet &dest) const {
		FastAssert(dest.GetNumBitWords() >= bsWordSize);

		u32 *dstBits = (u32 *) dest.GetRawBitMask();
		for (int i=0; i<bsWordSize; i++)
			dstBits[i] = myBaseType::m_Bits[i];
	}

	// PURPOSE: Return an entire word out of the bitset
	// PARAMS: i - Zero-based index of word to return
	// RETURNS: Entire word from bit set
	__forceinline u32 GetWord(unsigned int i) const
	{
		BitSetAssert(i<bsWordSize);
		return myBaseType::m_Bits[i];
	}

	// PURPOSE: Counts the number of bits of in the given state
	// PARAMS: val - bit state to count for
	// RETURNS: number of bits set to the given state, returns 0 if none are set
	int CountBits(bool val=true) const {
		int count = CountOnBits();

		if (!val)
		{
			return myBaseType::GetNumBits()-count;
		}

		return count;
	}  

	// PURPOSE: Counts the number of bits on
	// RETURNS: number of bits set to the on state, returns 0 if none are set
	int CountOnBits() const {
		int count = 0;
		// no range checking because out of range bits are zero
		for(int i = 0; i < bsWordSize; ++i) {
			unsigned int const w = myBaseType::m_Bits[i] - ((myBaseType::m_Bits[i] >> 1) & 0x55555555);     
			unsigned int const x = (w & 0x33333333) + ((w >> 2) & 0x33333333);      
			unsigned int const c = ((x + (x >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;  
			count += c;
		}
		return count;
	}  

	// PURPOSE: Gets the nth on bit index, return -1 of it can't find one.
	// PARAMS: nth - the nth on bit whose index we want
	// RETURNS: the index of the nth on bit, returns -1 if it doesn't exit.
	int GetNthBitIndex(unsigned int nth) const {
		BitSetAssert(nth<_ArraySize);
		unsigned int count = 0;
		// no range checking because out of range bits are zero
		for(unsigned int i = 0; i < bsWordSize; ++i) {
			u8 * rawPtr = (u8 *) &(myBaseType::m_Bits[i]);

			if (*((u32*)rawPtr)==0)
			{
				continue;
			}

			for (int j = (__BE ? 3  : 0); j != (__BE ? -1  : 4); j+=(__BE ? -1 : 1))
			{
				count += FixedBitSet::BitCountTable[rawPtr[j]]; 
				if (count>nth)
				{
					int subCount = count-nth;

					for (int bit = 7; bit >= 0; bit--)
					{
						if (rawPtr[j]&(1<<bit))
						{
							subCount--;
						}
						if (subCount==0)
						{
							return (i*32)+bit+((__BE ? 3-j : j)*8);
						}
					}
				}
			}
		}
		return -1;
	}

protected:
	// Serialize method -- we don't want allocations in this array
	friend inline datSerialize & operator<< ( datSerialize &ser, myType &bit ) {
		ser << datArray<u32>(bit.m_Bits, bsWordSize);
		return ser;
	}
};

typedef atFixedBitSet<8, u8> atFixedBitSet8;
typedef atFixedBitSet<16, u16> atFixedBitSet16;
typedef atFixedBitSet<32, u32> atFixedBitSet32;

namespace sysEndian
{
	template<int size, typename T> void SwapMe(atFixedBitSet<size, T>& data)
	{
		T* blocks = NULL;
		u32 numBlocks;
		data.GetRaw(&blocks, &numBlocks);
		for(u32 i = 0; i < numBlocks; i++)
		{
			SwapMe(blocks[i]);
		}
	}
}

/*
PURPOSE
atFixedBitSetIterator simple iterator class which wraps the iteration of an atFixedBitSet.
<FLAG Component>
*/
template <int _ArraySize, typename _BlockType=u32> class atFixedBitSetIterator
{
public:

	// PURPOSE: Ctor for bitset iterator
	// PARAMS:  set - the bit set the iterator will iterate over
	explicit atFixedBitSetIterator(const atFixedBitSetBase<_ArraySize, _BlockType>& set)
		: m_Set(set)
		, m_Index(0)
	{
		if (set.IsClear(0))
		{
			Next();
		}
	}

	// PURPOSE: Convenient operator overload to iterate to the next element
	// RETURNS: this
	atFixedBitSetIterator& operator++()
	{
		Next();
		return *this;
	}

	// PURPOSE: Convenient operator overload to do automatic conversion from iterator -> int
	// RETURNS: m_Index
	operator int () const
	{
		return m_Index;
	}

	// PURPOSE: Convenient operator overload to do inequality comparison between val and m_Index
	// RETURNS: true if m_Index != val
	bool operator!=(int val) const
	{
		return m_Index != val;
	}

	// PURPOSE: Gets m_Index, the current index for the current m_Bit 
	// RETURNS: m_Index, which corresponds to the bit index for the current m_Bit
	int Get()
	{
		return m_Index;
	}

	// PURPOSE: Finds the next valid of m_Bit+1
	// RETURNS: m_Index
	int Next()
	{
		static const int _BitsPerBlock  = atFixedBitSet<_ArraySize, _BlockType>::_BitsPerBlock;
		static const int _BlockBitShift = atFixedBitSet<_ArraySize, _BlockType>::_BlockBitShift;

		int bitIdx = m_Index;
		Assert(bitIdx >= 0);

		int lastBlockIdx = (_ArraySize-1)>>_BlockBitShift;

		_BlockType blockMask = -((SIGNED_TYPE(_BlockType))m_Set.BitInBlock(bitIdx))<<1;
		_BlockType blockBits;

		// Loop over blocks up until, but not including the very last block
		int blockIdx = bitIdx>>_BlockBitShift;
		while (blockIdx < lastBlockIdx)
		{
			blockBits = m_Set.GetRawBlock(blockIdx) & blockMask;
			blockMask = ~(_BlockType)0;
			if (blockBits)
			{
				_BlockType bit = blockBits & (_BlockType)-(SIGNED_TYPE(_BlockType))blockBits;
				bitIdx = Log2Floor(bit) + (blockIdx<<_BlockBitShift);
				m_Index = bitIdx;
				return bitIdx;
			}
			++blockIdx;
		}

		// Last block
		_BlockType lastBlockMask = ~(_BlockType)0 >> ((-_ArraySize)&(_BitsPerBlock-1));
		blockMask &= lastBlockMask;
		blockBits = m_Set.GetRawBlock(blockIdx) & blockMask;
		if (blockBits)
		{
			_BlockType bit = blockBits & (_BlockType)-(SIGNED_TYPE(_BlockType))blockBits;
			bitIdx = Log2Floor(bit) + (blockIdx<<_BlockBitShift);
			m_Index = bitIdx;
			return bitIdx;
		}

		m_Index = -1;
		return -1;
	}

private:

	const atFixedBitSetBase<_ArraySize, _BlockType> &m_Set;
	int m_Index;
};


template<int _ArraySize, typename _BlockType>
atFixedBitSetIterator<_ArraySize, _BlockType> atFixedBitSetBase<_ArraySize, _BlockType>::Begin() const
{
	return atFixedBitSetIterator<_ArraySize, _BlockType>(*this);
}


#undef bsWordSize

}	// namespace rage

// Helper macro for class enums that are used for flags
// class enums don't define the & and | operators because they're treated as proper classes with proper scoping and casting (not assumed to be an int),
// so we'll need to define these functions ourselves.
// WARNING: Rage & Framework can’t use C++11 features yet – it conflicts with some old tools!
#define DEFINE_CLASS_ENUM_FLAG_FUNCTIONS(flagType) \
	inline bool IsAnyFlagSet(flagType flags, flagType mask) { return (static_cast<std::underlying_type<flagType>::type>(flags) & static_cast<std::underlying_type<flagType>::type>(mask)) != 0;} \
	inline bool AreAllFlagsSet(flagType flags, flagType mask) { return (static_cast<std::underlying_type<flagType>::type>(flags) & static_cast<std::underlying_type<flagType>::type>(mask)) == static_cast<std::underlying_type<flagType>::type>(mask);} \
	inline flagType operator&(flagType l, flagType r) { return static_cast<flagType>(static_cast<std::underlying_type<flagType>::type>(l) & static_cast<std::underlying_type<flagType>::type>(r)); } \
	inline flagType operator|(flagType l, flagType r) { return static_cast<flagType>(static_cast<std::underlying_type<flagType>::type>(l) | static_cast<std::underlying_type<flagType>::type>(r)); } \
	inline flagType operator&=(flagType& l, flagType r) { return l = (l & r); } \
	inline flagType operator|=(flagType& l, flagType r) { return l = (l | r); }

#endif
