/*
 * Copyright (c) 2005-2012 NaturalMotion Ltd. All rights reserved. 
 *
 * Not to be copied, adapted, modified, used, distributed, sold,
 * licensed or commercially exploited in any manner without the
 * written consent of NaturalMotion. 
 *
 * All non public elements of this software are the confidential
 * information of NaturalMotion and may not be disclosed to any
 * person nor used for any purpose not expressly approved by
 * NaturalMotion in writing.
 *
 */

#ifndef NMRSHASHTABLE
#define NMRSHASHTABLE
#endif //NMRSHASHTABLE

using namespace rage;

namespace ART
{

/**
 * Given the input value, this function will return the next biggest prime number 
 * in the chosen sequence. Each entry is approximately half way between the two nearest 
 * power-of-two values.
 */
inline u32 nextPrime(u32 nv) 
{
  if (nv < 11) return 11;
#define NMHS_SIZE(_v) else if (nv < _v) return _v
    NMHS_SIZE(17);
    NMHS_SIZE(53);
    NMHS_SIZE(97);
    NMHS_SIZE(193);
    NMHS_SIZE(389);
    NMHS_SIZE(769);
    NMHS_SIZE(1543);
    NMHS_SIZE(3079);
    NMHS_SIZE(6151);
    NMHS_SIZE(12289);
    NMHS_SIZE(24593);
    NMHS_SIZE(49157);
    NMHS_SIZE(98317);
#undef NMHS_SIZE
  DebugAssert(0);
  return 0;
}

namespace hfn
{
  inline u32 hashUint32(u32 key)
  {
    key = (key ^ 0xe995) ^ (key >> 16);
    key += (key << 3);
    key ^= (key >> 4);
    key *= 0x27d4eb2d;
    key ^= (key >> 15);
    return key;
  }
}

template<typename T> struct HashType { typedef T CoerceTo; };
template<> struct HashType<const char*> { typedef const char* CoerceTo; };
template<typename T> struct HashType<T*> { typedef u32 CoerceTo; };

template <typename _Type> 
struct CompareContents
{
  inline static bool equal(const _Type &lhs, const _Type &rhs)
  { 
    return (lhs == rhs);
  }
};

template <typename KeyType>
struct HashFunction : CompareContents<KeyType> {};

template <>
struct HashFunction<u32> : CompareContents<u32>
{
  static inline u32 hash(const u32 key) 
  {
    return hfn::hashUint32(key);
  }
};



//#ifdef _MSC_VER
//# pragma warning(push)
//// 4127: temporarily disable warning about comparisons with constants (when comparing with default template arguments)
//// 4324: don't whine about padding when stored types require declspec(align())
//# pragma warning(disable:4127 4324)
//#endif // _MSC_VER

/**
 * NmRsHashtable
 *
 * Description:
 *
 *  An implementation of an open-addressing hashtable container with linear probing for collision resolution. It 
 *  offers very fast insertion, searching & deletion, with a significant performance spike if an insertion causes
 *  an expansion of the table. All the core API operations are O(1) in the average case.
 *
 *  Memory allocation only occurs during construction or re-hashing. The table is allocated to a given 
 *  first-estimate size and grown dynamically when the number of entries in the table exceeds a load-factor of about 80%. 
 *  
 */
template <
  typename KeyType,           ///< key-type, ensure there is a hashFunction that can turn it into a uint32 key value
  typename ValueType          ///< POD value type to store at key index
  >
class NmRsHashtable
{
public:

  typedef NmRsHashtable<KeyType, ValueType> _ThisType;

  typedef HashType<KeyType>                             THashType;
  typedef HashFunction<typename THashType::CoerceTo>    THashFunction;


 
  inline NmRsHashtable(u32 initialSize, ART::MemoryManager *mmgr);
  inline ~NmRsHashtable();

  
  // core API
  inline bool insert(const KeyType& key, const ValueType& value);        ///< insert item into the table; returns true if value already inserted
  inline bool find(const KeyType& key, ValueType* value = NULL) const;   ///< passing NULL for value turns find() into exists(); returns false if key not found
  inline bool erase(const KeyType& key);                                 ///< returns true if the key was found and removed
  inline bool replace(const KeyType& key, const ValueType& value);       ///< replace the value at given key with new value; returns false if key not found
  inline void clear();                                                   ///< clear the table without resizing, ready for re-use

  // comparison
  inline bool operator==(const _ThisType &rhs) const;

  // returns number of items currently inserted in the table
  inline u32 getNumUsedSlots() const { return m_numUsedSlots; }

  /**
   * call after potential fragmentation after erase()ing to re-hash the table (without expanding it)
   * which will strip out unused entries, reduce probe distances, etc. 
   */
  inline void optimize() { rehashInternal(m_hashSize); }

  /**
   * call to manually expand the table by one level and re-hash; useful if you are about to 
   * add a great deal of new entries to an already well populated table
   */
  inline void expand() { rehashInternal(m_hashSize + 1); }

  /**
   * ensure we have a table of at least 'requestedSize' - asking for less than m_hashSize bails out early
   */
  inline void reserve(u32 requestedSize) { rehashInternal(requestedSize); }


protected:

  struct KVPair
  {
    KeyType         m_key;
    ValueType       m_value;
    u32             m_probeDistance;
  };

  // common table search logic, used in find, erase, etc.
  inline bool doFind(const KeyType& key, u32 &outTableIndex, KVPair** outPair) const;

  // handles copying data from given other instance of table, used in copy ctors
  inline void copyFrom(const _ThisType &rhs);

  // core worker function that turns a key into a table index
  inline void hashToTableIndex(u32 &result, const KeyType& key, u32 hashSize) const
  {
    result = THashFunction::hash(THashType::CoerceTo(key));
    result %= hashSize;
  }

  // number of slots that can be occupied until the table is re-hashed;
  // this is a factor smaller than the total table size, to try and avoid the
  // performance penalties associated with high load factors on closed hash tables
  inline u32 scaleByLoadFactor(u32 hashSize) const
  {
    return (u32)((double)hashSize * 0.75);
  }

  // rebuild the hash table, either with a new, larger index or simply rehash the 
  // current contents. if requestedSize == m_hashSize, the function returns without doing anything.
  // if requestedSize < m_hashSize, the table is re-indexed but not re-sized (doesn't support table shrinking)
  inline void rehashInternal(u32 requestedSize);

  // returns true if the slot at index 'i' in the table is in use
  inline bool isSlotOccupied(u32 i) const
  {
    return ( m_occupiedSlots[i>>5] & (1<<(i & 31)) ) != 0;
  }


  KVPair                 *m_pairs;              // contiguous key/value storage
  u32                    *m_occupiedSlots,      // bitfield, one bit per pair, indicating slot usage
                          m_hashSize,           // size of current hash table
                          m_insertsTillRehash,  // inserts until we have to rehash
                          m_numUsedSlots;       // slots in use (tracking insert/erase/clear)

  //-----------------------------------------------------------------

private:

  ART::MemoryManager* m_allocator;

  NmRsHashtable(const _ThisType &rhs);
  NmRsHashtable& operator=(const _ThisType &rhs);
};

template <typename KeyType, typename ValueType>
NmRsHashtable<KeyType, ValueType>::NmRsHashtable(u32 initialSize,ART::MemoryManager *mmgr) :
    m_hashSize(0),
    m_numUsedSlots(0)
{
  m_allocator = mmgr;

  m_hashSize = nextPrime(initialSize);
  m_insertsTillRehash = scaleByLoadFactor(m_hashSize);

  m_pairs = reinterpret_cast<KVPair*>(m_allocator->callocate(sizeof(KVPair) * m_hashSize, NM_MEMORY_TRACKING_ARGS));
  m_occupiedSlots = reinterpret_cast<u32*>( m_allocator->callocate(sizeof(u32) * ((m_hashSize+31)>>5), NM_MEMORY_TRACKING_ARGS) );
}


template <typename KeyType, typename ValueType>
NmRsHashtable<KeyType, ValueType>::~NmRsHashtable()
{
  m_allocator->deallocate(m_occupiedSlots, NM_MEMORY_TRACKING_ARGS);
  m_allocator->deallocate(m_pairs, NM_MEMORY_TRACKING_ARGS);

  m_pairs = 0;
  m_occupiedSlots = 0;

  m_hashSize = 0;
  m_numUsedSlots = 0;
}


template <typename KeyType, typename ValueType>
bool NmRsHashtable<KeyType, ValueType>::insert(const KeyType& key, const ValueType& value)
{
  if (m_insertsTillRehash == 0)
    expand();

  u32 tableIndex, probeCount = 0;

  hashToTableIndex(tableIndex, key, m_hashSize);

  // store first-hit slot where the probeDistance will be stored
  KVPair* chunk = &m_pairs[tableIndex];

  // linear probe search, walk until we find an empty slot
  while (isSlotOccupied(tableIndex))
  {
    // check if the value already exists in this slot
    if (THashFunction::equal(THashType::CoerceTo(m_pairs[tableIndex].m_key), THashType::CoerceTo(key)))
    {
      return false;
    }

    // next slot, wrap-around at the end
    tableIndex ++;
    if (tableIndex >= m_hashSize)
      tableIndex = 0;

    // keep note on probe distance
    probeCount ++;

    // should never happen, the table will be expanded
    // automatically before completely full
    Assert(probeCount < m_hashSize);
  }

  m_insertsTillRehash --;
  m_numUsedSlots ++;

  // mark slot as occupied
  m_occupiedSlots[tableIndex>>5] |= (1<<(tableIndex & 31));

  // store maximum probe distance for all keys that hit the original slot
  if (probeCount > chunk->m_probeDistance)
    chunk->m_probeDistance = probeCount;

  // store data into slot
  chunk = &m_pairs[tableIndex];
  chunk->m_key = key;
  chunk->m_value = value;

  return true;
}

template <typename KeyType, typename ValueType>
bool NmRsHashtable<KeyType, ValueType>::doFind(const KeyType& key, u32 &outTableIndex, KVPair** outPair) const
{
  u32 tableIndex, probeDist, i;
  hashToTableIndex(tableIndex, key, m_hashSize);

  probeDist = m_pairs[tableIndex].m_probeDistance;
  for (i=0; i<=probeDist; i++)
  {
    if (isSlotOccupied(tableIndex))
    {
      if (THashFunction::equal(THashType::CoerceTo(m_pairs[tableIndex].m_key), THashType::CoerceTo(key)))
      {
        *outPair = &m_pairs[tableIndex];
        outTableIndex = tableIndex;
        return true;
      }
    }

    tableIndex ++;
    if (tableIndex >= m_hashSize)
      tableIndex = 0;
  }

  return false;
}

template <typename KeyType, typename ValueType>
bool NmRsHashtable<KeyType, ValueType>::find(const KeyType& key, ValueType* value) const
{
  u32 tableIndex;
  KVPair *tPair;
  if (doFind(key, tableIndex, &tPair))
  {
    if (value != NULL)
      *value = tPair->m_value;

    return true;
  }

  return false;
}

template <typename KeyType, typename ValueType>
bool NmRsHashtable<KeyType, ValueType>::erase(const KeyType& key)
{
  u32 tableIndex;
  KVPair *tPair;
  if (doFind(key, tableIndex, &tPair))
  {
    m_occupiedSlots[tableIndex>>5] &= ~(1<<(tableIndex & 31)); 
    m_numUsedSlots --;
    return true;
  }

  return false;
}

template <typename KeyType, typename ValueType>
bool NmRsHashtable<KeyType, ValueType>::replace(const KeyType& key, const ValueType& value)
{
  u32 tableIndex;
  KVPair *tPair;
  if (doFind(key, tableIndex, &tPair))
  {
    tPair->m_value = value;
    return true;
  }

  return false;
}

template <typename KeyType, typename ValueType>
void NmRsHashtable<KeyType, ValueType>::clear()
{
  m_insertsTillRehash = scaleByLoadFactor(m_hashSize);

  // not strictly necessary
  memset(m_pairs, 0, sizeof(KVPair) * m_hashSize);

  size_t occupiedSlotsSz = sizeof(u32) * ((m_hashSize+31)>>5);
  memset(m_occupiedSlots, 0, occupiedSlotsSz);
  m_numUsedSlots = 0;
}

template <typename KeyType, typename ValueType>
inline bool NmRsHashtable<KeyType, ValueType>::operator==(const _ThisType &rhs) const
{
  if (m_hashSize != rhs.m_hashSize)
  {
    return false;
  }

  if (m_numUsedSlots != rhs.m_numUsedSlots)
  {
    return false;
  }

  return memcmp(m_pairs, rhs.m_pairs, sizeof(KVPair) * m_hashSize) == 0;
}

template <typename KeyType, typename ValueType>
void NmRsHashtable<KeyType, ValueType>::rehashInternal(u32 requestedSize)
{
  u32 newHashSize = (requestedSize == m_hashSize)?m_hashSize:nextPrime(requestedSize);

  // we cannot arbitrarily shrink the table
  if (newHashSize < m_hashSize)
  {
    return;
  }

  KVPair* newPairs = reinterpret_cast<KVPair*>(m_allocator->callocate(sizeof(KVPair) * newHashSize, NM_MEMORY_TRACKING_ARGS));
  u32* newBitfield = reinterpret_cast<u32*>( m_allocator->callocate(sizeof(u32) * ((newHashSize+31)>>5), NM_MEMORY_TRACKING_ARGS) );

  m_insertsTillRehash = scaleByLoadFactor(newHashSize);
  m_numUsedSlots = 0;

  u32 tableIndex, probeCount;

  for (u32 i=0; i<m_hashSize; i++)
  {
    if (!isSlotOccupied(i))
      continue;

    // hash existing entry with new hash table size to find it's new position
    hashToTableIndex(tableIndex, m_pairs[i].m_key, newHashSize);

    // store first-hit slot where the probeDistance will be stored
    probeCount = 0;
    KVPair* originalSlot = &newPairs[tableIndex];

    while ((newBitfield[tableIndex>>5] & (1<<(tableIndex & 31))) != 0)
    {
      tableIndex ++;
      if (tableIndex >= newHashSize)
        tableIndex = 0;

      probeCount ++;
    }

    m_insertsTillRehash --;
    m_numUsedSlots ++;

    if (probeCount > originalSlot->m_probeDistance)
      originalSlot->m_probeDistance = probeCount;

    newBitfield[tableIndex>>5] |= (1<<(tableIndex & 31));
    newPairs[tableIndex] = m_pairs[i];
  }


  m_hashSize = newHashSize;

  m_allocator->deallocate(m_pairs, NM_MEMORY_TRACKING_ARGS);
  m_pairs = newPairs;

  m_allocator->deallocate(m_occupiedSlots, NM_MEMORY_TRACKING_ARGS);
  m_occupiedSlots = newBitfield;
}

//#ifdef _MSC_VER
//# pragma warning(pop)
//#endif // _MSC_VER

} // namespace ART


