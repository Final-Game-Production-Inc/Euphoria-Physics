//
// atl/pagedpool.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_PAGEDPOOL_H
#define ATL_PAGEDPOOL_H

#include <string.h>

#include "bitset.h"

#include "data/struct.h"
#include "data/safestruct.h"
#include "diag/tracker.h"
#include "math/amath.h"
#include "system/new.h"
#include "system/debugmemoryfill.h"


// Collect stats like peak usage
#define PAGED_POOL_TRACKING		(!__FINAL && !__PROFILE)

namespace rage {

/*
PURPOSE
    atPagedPool manages a pool of equally-sized elements grouped in pages that can grow and shrink
	on demand. Each page can have a "sub type" that is specified upon allocation of a single element.
	Pages are guaranteed to only contain elements of the same sub type.

	While pages are allocated on demand, the pool must be constructed with a maximum number of pages,
	and each possible page has a preallocated header.
<FLAG Component>
<COMBINE atPagedPool>
*/


/** PURPOSE: Header structure for a paged pool. atPagedPool keeps one of these for every possible
 *  page. The header contains a free list to point to the next available header, and a free list
 *  for elements within this page.
 */
struct atPagedPoolHeaderBase {

	enum {
		// Indicates the end of the linked list of free elements.
		NO_MORE_FREE = 0xffff,
#if __DEV
		// Sub-type value for a free page.
		PAGE_FREE = 0xfffe,
#endif // __DEV
	};


	/** PURPOSE: Return the sub-type of this page.
	 */
	u32 GetSubType() const {
		return m_SubType;
	}



	// Number of elements in this page that are currently used.
	u16 m_UsedCount;

	// Subtype for this page.
	// DEV-only: This is "PAGE_FREE" if the page is not used.
	u16 m_SubType;

	// If this page is used, but not full, this will point to the next page that is also used and not full and uses the same subtype.
	// if this element is not used, this will point to the next page that is also not used.
	u16 m_Next;

	// Points to the first element in m_Page that is currently not used. This element can be cast to a (u16 *), where it points to the next one
	// not used, or NO_MORE_FREE if there are no more free entries.
	// Likewise, this member will be NO_MORE_FREE if there are no free elements in this page.
	u16 m_FirstFree;

	// Points to the next page that is used for this particular subtype (full or not).
	u16 m_NextUsedPage;
};

/** PURPOSE: This is the iteration layer for a page - it's completely empty if the template parameter is false, and
 *  has a bitmask of used elements if true. Constructed with Init(), destroyed with Destroy(). The page needs to
 *  set/clear elements via SetAsUsed/SetAsUnused. The GetFirstUsedElement() and GetNextUsedElement() can be used
 *  to iterate through all used elements.
 */
template<bool _Iterable>
struct atPagedPoolHeaderIterableLayer
{
	void SetAsUsed(u32 /*index*/) {}
	void SetAsUnused(u32 /*index*/) {}
	void Init(u32 /*elementsPerPage*/) {}
	bool IsElementAllocated(u32 /*index*/) const { FastAssert(false); return false; }
	void Destroy() {}
};

template<>
struct atPagedPoolHeaderIterableLayer<false>
{
	void SetAsUsed(u32 /*index*/) {}
	void SetAsUnused(u32 /*index*/) {}
	void Init(u32 /*elementsPerPage*/) {}
	bool IsElementAllocated(u32 /*index*/) const { FastAssert(false); return false; }
	void Destroy() {}
};

template<>
struct atPagedPoolHeaderIterableLayer<true>
{
	atPagedPoolHeaderIterableLayer() {
		m_UsedMask = NULL;
	}

	void Init(u32 elementsPerPage) {
		FastAssert(!m_UsedMask);
		m_UsedMask = rage_new atBitSet(elementsPerPage);
	}

	void Destroy() {
		FastAssert(m_UsedMask);
		delete m_UsedMask;
		m_UsedMask = NULL;
	}

	void SetAsUsed(u32 index) {
		FastAssert(m_UsedMask);
		FastAssert(m_UsedMask->IsClear(index));
		m_UsedMask->Set(index);
	}

	void SetAsUnused(u32 index) {
		FastAssert(m_UsedMask);
		FastAssert(m_UsedMask->IsSet(index));
		m_UsedMask->Clear(index);
	}

	bool IsElementAllocated(u32 index) const {
		return m_UsedMask->IsSet(index);
	}

	u32 GetFirstUsedElement() {
		return (u32) m_UsedMask->GetFirstBitIndex();
	}

	u32 GetNextUsedElement(u32 lastUsedElement) {
		return (u32) m_UsedMask->GetNextBitIndex(lastUsedElement);
	}

	// Bits indicating which elements have been used, NULL if this not an iterable pool
	atBitSet *m_UsedMask;
};

template<typename _Type, bool _Iterable, typename _UserDataType = size_t>
struct atPagedPoolHeader : public atPagedPoolHeaderBase {

	/** PURPOSE: Set a user-defined value for this page.
	 */
	void SetUserData(const _UserDataType &userData) {
		m_UserData = userData;
	}
	
	/** PURPOSE: Get the user-defined value for this page.
	 */
	_UserDataType GetUserData() const {
		return m_UserData;
	}

	/** PURPOSE: Get the "next free element" index of a specific element. The element is
	 * expected to currently be available.
	 *
	 * PARAMS:
	 *  element - Index of the element within this page.
	 * RETURNS:
	 *  Index of the next free element in the page after this one. NO_MORE_FREE if this is the
	 *  last element in the free list.
	 */
	u32 GetNextFree(u32 element) {
		u16 *ptr = (u16 *) &m_Page[element];
		return *ptr;
	}

	/** PURPOSE: Set the "next free element" index of a specific element. The element is
	 * expected to currently be available.
	 *
	 * PARAMS:
	 *  element - Index of the element within this page.
	 *  next - index of the next free element in the page after this one. NO_MORE_FREE if this is the
	 *  last element in the free list.
	 */
	void SetNextFree(u32 element, u32 next) {
		u16 *ptr = (u16 *) &m_Page[element];
		*ptr = (u16) next;
	}

	/** PURPOSE: Returns true if this page has been allocated and contains elements, false
	 *  otherwise.
	 */
	bool IsAllocated() const {
		return m_Page != NULL;
	}

	/** PURPOSE: Returns true if a specific element within this page has been allocated.
	 *  Note that this can only be done on an iterable paged pool.
	 *
	 *  PARAMS:
	 *   element - Index of the element within this page to check.
	 */
	bool IsElementAllocated(u32 element) const {
		return m_IterationLayer.IsElementAllocated(element);
	}

	/** PURPOSE: Allocates an element in this page.
	 *  RETURNS: The index within this page of the element.
	 */
	u32 Allocate() {
		u32 result = m_FirstFree;
		m_FirstFree = (u16) GetNextFree(result);
		m_UsedCount++;

		m_IterationLayer.SetAsUsed(result);

		return result;
	}

	/** PURPOSE: Free up the given element.
	 *
	 * PARAMS:
	 *  element - Index of the element within this page.
	 */
	void Free(u32 index) {
		SetNextFree(index, m_FirstFree);
		m_FirstFree = (u16) index;

		m_IterationLayer.SetAsUnused(index);

		m_UsedCount--;
	}

	/** PURPOSE: Returns the actual element in the pool.
	 *
	 * PARAMS: 
	 *  element - Index of the element within this page.
	 */
	_Type *GetElement(u32 element) {
		return &m_Page[element];
	}

	const _Type *GetElement(u32 element) const {
		return &m_Page[element];
	}

	// Iteration manager (if we want iteration)
	atPagedPoolHeaderIterableLayer<_Iterable> m_IterationLayer;

	// Actual elements in this page.
	_Type *m_Page;


	// User-defined value for this page.
	_UserDataType m_UserData;
};

/** PURPOSE: An object of this type references a specific element in an atPagedPool. This object encodes
 *  both the page as well as the element within the page.
 */
struct atPagedPoolIndex {
	enum eInvalidType {
		INVALID_INDEX,

		INVALID_PAGE = 0xffff,
	};

	atPagedPoolIndex() {};

	atPagedPoolIndex(u32 page, u32 index)				{ m_PageAndIndex = (page << 16) | index; }

	atPagedPoolIndex(const atPagedPoolIndex &o)			{ m_PageAndIndex = o.m_PageAndIndex; }

	atPagedPoolIndex(eInvalidType)						{ m_PageAndIndex = (u32) INVALID_PAGE << 16; }

	bool IsInvalid() const								{ return GetPage() == INVALID_PAGE; }

	u32 GetPage() const									{ return m_PageAndIndex >> 16; }

	u32 GetElement() const								{ return m_PageAndIndex & 0xffff; }

	u32 GetRawValue() const								{ return m_PageAndIndex; }

	bool operator==(const atPagedPoolIndex &o) const	{ return m_PageAndIndex == o.m_PageAndIndex; }

	bool operator!=(const atPagedPoolIndex &o) const	{ return m_PageAndIndex != o.m_PageAndIndex; }

protected:
	u32 m_PageAndIndex;
};

/** The allocation policy for a paged pool. The pool will call Allocate() whenever a new page is allocated.
 *  DO NOT CONSTRUCT THE ELEMENTS - each element will have a free list index
 *  while it is not in use, so do not use the payload data until an individual element has been allocated.
 *
 *  It's okay to use this function to set the user data of the page.
 */
template<typename _Type, bool _Iterable>
struct atDefaultPagedPoolAllocator {

	/** PURPOSE: Called when a new page is allocated. This function is expected to allocate and return
	 *  a contiguous uninitialized array that is returned.
	 *
	 *  PARAMS:
	 *   header - Page header. May be accessed to set the user data.
	 *   elementCount - Numer of elements to allocate.
	 *   alignment - Alignment to use for the allocation.
	 */
	static _Type *Allocate(u32 /*page*/, atPagedPoolHeader<_Type, _Iterable> &/*header*/, u32 elementCount, u32 alignment, size_t *outMemoryUsage) {
		*outMemoryUsage = elementCount * sizeof(_Type);
		return reinterpret_cast<_Type *>(rage_aligned_new(alignment) char[elementCount * sizeof(_Type)]);
	}

	/** PURPOSE: Called when a page is freed.
	 *
	 *  PARAMS:
	 *   data - Pointer to the page data that had been returned by the previous call to Allocate.
	 *   header - Page header.
	 */
	static void Free(u32 /*page*/, u32 elementCount, _Type *data, atPagedPoolHeader<_Type, _Iterable> &/*header*/, size_t *outMemoryUsage) {
		*outMemoryUsage = elementCount * sizeof(_Type);
		delete[] (char *) data;
	}

	/** PURPOSE: Compute how much memory a page costs. This is supposed to include the cost for the payload, but not for the page header.
	 *
	 * PARAMS:
	 *   subType - Subtype of the page in question.
	 *   elementCount - Number of elements in this page.
	 */
	static size_t ComputePageMemUsage(u32 /*subType*/, u32 elementCount) {
		return sizeof(_Type) * elementCount;
	}
};


class atPagedPoolBase
{
public:
	enum {
		NO_MORE_USED = 0xffff,
	};

	typedef bool (*Callback)(void* item, void* data);

	struct SubtypeInfo {
		SubtypeInfo()
			: m_FirstUsedNonFullPage(NO_MORE_USED)
			, m_FirstUsedPage(NO_MORE_USED)
			, m_ElementsUsed(0)
#if PAGED_POOL_TRACKING
			, m_PeakElementUsage(0)
			, m_PagesUsed(0)
			, m_MemoryUsage(0)
#endif // PAGED_POOL_TRACKING
		{
		}

		// Number of elements that are allocated for this subtype.
		u32 m_ElementsUsed;

		// Number of elements per page for this subtype.
		u32 m_ElementsPerPage;

		// This is an index into m_PageHeaders for the first page that is used for this particular subtype but not full, or NO_MORE_USED if there aren't any.
		u16 m_FirstUsedNonFullPage;

		// This is an index into m_PageHeaders for the first page that is used for this particular subtype, full or not.
		u16 m_FirstUsedPage;

#if PAGED_POOL_TRACKING
		// Number of bytes used for this subtype - payload only, doesn't include headers or bitsets.
		size_t m_MemoryUsage;

		// Max number of elements used for this subtype
		u32 m_PeakElementUsage;

		// Number of pages used for this element
		u16 m_PagesUsed;
#endif // PAGED_POOL_TRACKING
	};

	atPagedPoolBase()
		: m_SubtypeInfo(NULL)
		, m_UsedCount(0)
		, m_MaxPages(0)
	{
	}

	/** PURPOSE: Specify how many elements will be allocated for a specific subtype.
	 *  This function may only be called when there are no pages allocated for this
	 *  subtype already.
	 *
	 *  PARAMS:
	 *   subType - Sub type to specify the element count for.
	 *
	 *   elementsPerPage - Number of elements for each page allocated for this subtype
	 *                     from now on. Cannot be higher than the number specified when
	 *                     initially creating this pool via Init().
	 */
	void SetSubtypeElementCount(u32 subType, u32 elementsPerPage) {
		FastAssert(subType < m_MaxSubTypes);
		Assert(m_SubtypeInfo[subType].m_ElementsUsed == 0);	// There are already pages allocated for this subtype
		Assert(elementsPerPage <= m_ElementsPerPage);		// Can't be larger than the master size

		m_SubtypeInfo[subType].m_ElementsPerPage = elementsPerPage;
	}

	/** PURPOSE: Return true if we can no longer allocate anything. This is normally not possible since
	 *  we're a freaking paged pool, but since there is a max number of pages, this CAN happen.
	 */
	bool IsFull(u32 subType = 0) {
		// No more free pages...
		if (m_FirstFree == atPagedPoolHeaderBase::NO_MORE_FREE) {
			// And nothing available for the current subtype?
			return m_SubtypeInfo[subType].m_FirstUsedNonFullPage == NO_MORE_USED;
		}

		// At least one entire free page - we're good.
		return false;
	}

	/** PURPOSE: Compress an atPagedPoolIndex into an integer that's as small as possible.
	 *  Useful when trying to put an atPagedPoolIndex into an integer that has fewer than
	 *  32 bits. The highest possible number for this value will be "max pages * elements per page".
	 */
	u32 CompressPagedPoolIndex(atPagedPoolIndex index) const {
		return index.GetPage() * m_ElementsPerPage + index.GetElement();
	}

	/** PURPOSE: Turn a value computed with CompressPagedPoolIndex() back into
	 *  a proper atPagedPoolIndex.
	 */
	atPagedPoolIndex DecompressPagedPoolIndex(u32 compressedIndex) const {
		return atPagedPoolIndex(compressedIndex / m_ElementsPerPage, compressedIndex % m_ElementsPerPage);
	}

	/** PURPOSE: Return the number of pages allocated for a given subtype.
	 */
	u32 GetElementsUsedForSubType(u32 subType) const {
		FastAssert(subType < m_MaxSubTypes);
		return m_SubtypeInfo[subType].m_ElementsUsed;
	}

	/** PURPOSE: Return the maximum number of used pages.
	 */
	u32 GetMaxPageCount() const {
		return m_MaxPages;
	}

	/** PURPOSE: Return the number of subtypes.
	 */
	u32 GetMaxSubTypes() const {
		return m_MaxSubTypes;
	}

	/** PURPOSE: Returns the max number of elements per page for all subtypes.
	 */
	u32 GetMaxElementsPerPage() const {
		return m_ElementsPerPage;
	}

	/** PURPOSE: Returns the number of elements per page for a specific subtype.
	 */
	u32 GetElementsPerPage(u32 subType) const {
		return m_SubtypeInfo[subType].m_ElementsPerPage;
	}

#if PAGED_POOL_TRACKING
	/** PURPOSE: Returns the number of pages used for a specific subtype.
	 */
	u32 GetPagesUsedForSubType(u32 subType) const {
		return m_SubtypeInfo[subType].m_PagesUsed;
	}

	/** PURPOSE: Returns the peak number of elements allocated for a specific subtype.
	 */
	u32 GetPeakElementUsageForSubType(u32 subType) const {
		return m_SubtypeInfo[subType].m_PeakElementUsage;
	}

	/** PURPOSE: Return the amount of memory taken up by the payload of a specific subtype of this pool.
	 */
	size_t GetMemoryUsage(u32 subType) const {
		return m_SubtypeInfo[subType].m_MemoryUsage;
	}
#endif // PAGED_POOL_TRACKING

	/** PURPOSE: Returns the first used page for a specific subtype. NO_MORE_USED
	 *  if there are no elements at all for this subtype.
	 */
	u32 GetFirstUsedPage(u32 subType) const {
		return m_SubtypeInfo[subType].m_FirstUsedPage;
	}

	// Compatibility with atPool - get the number of used elements.
	size_t GetCount() const {
		return (size_t) m_UsedCount;
	}

	// Compatibility with atPool - this is a hack. We do technically have a free count, but it's non-trivial to assess.
	size_t GetFreeCount() const {
		return 0x7fff;
	}



protected:
	// Array of information structures for each subtype
	SubtypeInfo *m_SubtypeInfo;

	// Number of elements per page.
	u32 m_ElementsPerPage;

	// Total number of pages that can be allocated.
	u32 m_MaxPages;

	// Alignment to use for the actual page data.
	u32 m_Alignment;

	// Number of possible subtypes.
	u32 m_MaxSubTypes;

	// Total number of elements allocated across all subtypes.
	u32 m_UsedCount;

	// Index into m_PageHeaders for the first page that is currently free, or _Type::NO_MORE_FREE if there aren't any.
	u16 m_FirstFree;
};

template<typename _Type, bool _Iterable, typename _Allocator>
class atPagedPool;

template<typename _Type, bool _Iterable, typename _Allocator >
void atPagedPool_ForAll(atPagedPool<_Type, _Iterable, _Allocator> &pool, atPagedPoolBase::Callback cb, void* data);

template<typename _Type, bool _Iterable = false, typename _Allocator = atDefaultPagedPoolAllocator<_Type, _Iterable> >
class atPagedPool : public atPagedPoolBase
{
public:
	typedef atPagedPoolHeader<_Type, _Iterable> Header;
	typedef void (*PoolFullCB)(void* item);


	atPagedPool()
		: m_PageHeaders(NULL)
#if __BANK
		, m_poolFullCB(NULL)
#endif // __BANK
	{
	}

	// PURPOSE: Compatibility with atPool
	atPagedPool(size_t size, u32 pageSize = 512)
		: m_PageHeaders(NULL)
#if __BANK
		, m_poolFullCB(NULL)
#endif // __BANK
	{
		// Don't waste memory
		size += (pageSize - (size % pageSize)) % pageSize;
//		Assertf((size % pageSize) == 0, "Wasting %" SIZETFMT "d bytes on our last page! Increase the atPagedPool size by %" SIZETFMT "d elements!", (size % pageSize) * sizeof(_Type), pageSize - (size % pageSize));

		Init((u32) (size + (pageSize - 1)) / pageSize, pageSize, 1, __alignof(_Type));
	}

	// PURPOSE: Compatibility with atPool
	atPagedPool(s32 size, const char* /*pName*/, s32 /*redZone*/=0, s32 ASSERT_ONLY(storageSize)=sizeof(_Type), u32 pageSize = 512)
		: m_PageHeaders(NULL)
#if __BANK
		, m_poolFullCB(NULL)
#endif // __BANK
	{
		Assert(storageSize == sizeof(_Type));

		// Don't waste memory
		size += (pageSize - (size % pageSize)) % pageSize;
//		Assertf((size % pageSize) == 0, "Wasting %" SIZETFMT "d bytes on our last page! Increase the atPagedPool size by %d elements!", (size % pageSize) * sizeof(_Type), pageSize - (size % pageSize));

		Init((u32) (size + (pageSize - 1)) / pageSize, pageSize, 1, __alignof(_Type));
	}


	~atPagedPool() {
		Shutdown();
	}

	/** PURPOSE: Initializes this paged pool. This will create page headers for every
	 *  possible page and set up the free lists.
	 *
	 *  PARAMS:
	 *   maxPages - Total number of pages that can possibly be allocated. Note that there
	 *              will be a small but persistent memory overhead for each page, even unallocated ones.
	 *
	 *   elementsPerPage - Default number of elements per page. This number can be overridden per
	 *                     subType, but it can never be higher than this value.
	 *
	 *   maxSubTypes - Number of possible subtypes.
	 *
	 *   alignment - Memory alignment when allocating data for a page.
	 */
	void Init(u32 maxPages, u32 elementsPerPage, u32 maxSubTypes = 1, u32 alignment = 0) {
		// Initialized twice?
		Assert(!m_PageHeaders);

		// Allocate the page headers.
		m_PageHeaders = rage_new Header[maxPages];

		// Initialize every page header.
		for (u32 x=0; x<maxPages; x++) {
			Header &phs = m_PageHeaders[x];

#if __DEV
			phs.m_SubType = atPagedPoolHeaderBase::PAGE_FREE;
#endif // __DEV
			phs.m_Page = NULL;
			phs.m_UsedCount = 0;
			phs.m_FirstFree = 0;
			phs.m_UserData = 0;
			phs.m_Next = (u16) x+1;
		}

		m_PageHeaders[maxPages-1].m_Next = atPagedPoolHeaderBase::NO_MORE_FREE;

		m_SubtypeInfo = rage_new SubtypeInfo[maxSubTypes];

		for (u32 x=0; x<maxSubTypes; x++) {
			m_SubtypeInfo[x].m_ElementsPerPage = elementsPerPage;
		}

		m_MaxPages = maxPages;
		m_Alignment = alignment;
		m_ElementsPerPage = elementsPerPage;
		m_MaxSubTypes = maxSubTypes;
		m_FirstFree = 0;
	}

	void Shutdown() {
		// TODO: How about a used list for pages?
		for (u32 x=0; x<m_MaxPages; x++) {
			if (m_PageHeaders[x].m_Page) {
				FreePage(x);
			}
		}

		delete[] reinterpret_cast<Header *>(m_PageHeaders);
		delete[] m_SubtypeInfo;
		m_PageHeaders = NULL;
		m_SubtypeInfo = NULL;
		m_MaxPages = 0;
	}

	/** PURPOSE: Allocate a single element. If there are no free pages (or no pages
	 *  that use this particular subtype), a new page will be allocated.
	 *
	 *  NOTE that this object will not be constructed! The memory being returned
	 *  is uninitialized.
	 *
	 *  PARAMS:
	 *   subType - Subtype of the element to be allocated. The returned element is
	 *             guaranteed to be in a page that only has elements of this subtype.
	 *
	 *  RETURNS:
	 *   atPagedPoolIndex reference to the allocated element.
	 */
	atPagedPoolIndex Allocate(u32 subType = 0) {
		FastAssert((u32) subType < (u32) m_MaxSubTypes);

		SubtypeInfo &subtypeInfo = m_SubtypeInfo[subType];

		// Do we already have a page we can draw from?
		u32 pageIndex = subtypeInfo.m_FirstUsedNonFullPage;

		if (pageIndex == NO_MORE_USED) {
			// No - allocate a new one.
			pageIndex = AllocatePage(subType);
		}

		// Let's grab the next free element here.
		Header *header = &m_PageHeaders[pageIndex];

		// Shouldn't be on the use list if there's nothing free.
		FastAssert(header->m_FirstFree != atPagedPoolHeaderBase::NO_MORE_FREE);

		// Allocate the element.
		u32 element = header->Allocate();

		subtypeInfo.m_ElementsUsed++;
#if PAGED_POOL_TRACKING
		subtypeInfo.m_PeakElementUsage = Max(subtypeInfo.m_PeakElementUsage, subtypeInfo.m_ElementsUsed);
#endif // PAGED_POOL_TRACKING

		if (header->m_UsedCount == subtypeInfo.m_ElementsPerPage) {
			// If this page is full, we need to remove it from the "used" list.
			subtypeInfo.m_FirstUsedNonFullPage = header->m_Next;
		}

		m_UsedCount++;

		return atPagedPoolIndex(pageIndex, element);
	}

	/** PURPOSE: Free a single element. If this frees up the last element of the page,
	 *  the page will be deallocated and returned to the pool of free pages.
	 *
	 *  NOTE that the element will not be destroyed! The memory will just be returned to
	 *  the free list, although the data will be slightly defaced to maintain the free
	 *  pool, so the state of the memory after being reallocated is undefined.
	 *  The destructor will NOT be called.
	 *  
	 *  PARAMS:
	 *   index - atPagedPoolIndex reference to the element being deleted.
	 */
	void Free(atPagedPoolIndex index) {

		// Get the page.
		u32 pageIndex = index.GetPage();
		Header &header = m_PageHeaders[pageIndex];

		FastAssert(header.m_UsedCount > 0);

		u32 subType = header.m_SubType;
		SubtypeInfo &subtypeInfo = m_SubtypeInfo[subType];

		subtypeInfo.m_ElementsUsed--;

		// Free the actual element.
		header.Free(index.GetElement());

		if (header.m_UsedCount == subtypeInfo.m_ElementsPerPage - 1) {
			// If we're available all a sudden, put us back on the used list.
			header.m_Next = subtypeInfo.m_FirstUsedNonFullPage;
			subtypeInfo.m_FirstUsedNonFullPage = (u16) pageIndex;
		}

		// Free the page if that was the last element.
		if (header.m_UsedCount == 0) {
			// Remove the page from the sub type use linked list.
			u16 *nextPtr = &subtypeInfo.m_FirstUsedNonFullPage;

			while (*nextPtr != pageIndex) {
				FastAssert(*nextPtr != NO_MORE_USED);	// This page is not in the used list?!
				nextPtr = &m_PageHeaders[*nextPtr].m_Next;
			}

			*nextPtr = header.m_Next;

			// Free the page itself
			FreePage(pageIndex);
		}

		m_UsedCount--;
	}

	/** PURPOSE: Free a single element. See the Free() for more information.
	 *  NOTE that this function is not O(1). Using an atPagedPoolIndex is recommended
	 *  if it is available. This function mainly exists for compatibility with atPool.
	 */
	void Delete(_Type* ptr) {
		Free(GetPagedPoolIndex(ptr));
	}

	/** PURPOSE: Return true if this index points to a valid element that has been
	 *  allocated, false otherwise. This function can only be used on iterable pools.
	 */
	bool IsAllocated(atPagedPoolIndex index) const {
		if (index.IsInvalid()) {
			return false;
		}

		const Header &header = GetPageHeader(index);

		if (!header.IsAllocated()) {
			return false;
		}

		return header.IsElementAllocated(index.GetElement());
	}

	/** PURPOSE: Returns the index of a page header.
	 */
	u32 GetPageHeaderIndex(const atPagedPoolHeaderBase &header) {
		return ptrdiff_t_to_int(&header - m_PageHeaders);
	}

	/** PURPOSE: Return an element referenced by an atPagedPoolIndex.
	 *  PARAMS:
	 *   index - The atPagedPoolIndex that references a specific element.
	 */
	_Type *Get(atPagedPoolIndex index) {
		Header &header = m_PageHeaders[index.GetPage()];

#if __DEV
		Assert(header.GetSubType() != atPagedPoolHeaderBase::PAGE_FREE);
#endif // __DEV

		return header.GetElement(index.GetElement());
	}

	const _Type *Get(atPagedPoolIndex index) const {
		const Header &header = m_PageHeaders[index.GetPage()];

#if __DEV
		Assert(header.GetSubType() != atPagedPoolHeaderBase::PAGE_FREE);
#endif // __DEV

		return header.GetElement(index.GetElement());
	}

	/** PURPOSE: Get the paged pool index from an element that has been allocated
	 *  in this pool. Returns an invalid index if the element is not inside the pool.
	 *  This function will work for elements that are currently not allocated
	 *  but inside an allocated page.
	 *
	 *  NOTE that this function is not O(1), so it is recommended to hold on to the
	 *  atPagedPoolIndex of an element.
	 *
	 *  If you know the subtype, you'll make things a lot faster if you pass it in.
	 */
	atPagedPoolIndex GetPagedPoolIndex(_Type* ptr, u32 subType) {
		SubtypeInfo &subtypeInfo = m_SubtypeInfo[subType];
		u16 firstPage = subtypeInfo.m_FirstUsedPage;
		u32 elementsPerPage = subtypeInfo.m_ElementsPerPage;

		while (firstPage != NO_MORE_USED) {
			Header &header = m_PageHeaders[firstPage];

			_Type *base = header.m_Page;

			if (base && ptr >= base && ptr < &base[elementsPerPage]) {
				// We have the page, let's get the index.
				u32 element = ptrdiff_t_to_int(ptr - base);
				return atPagedPoolIndex(firstPage, element);
			}

			firstPage = header.m_NextUsedPage;
		}

		// Not inside the pool.
		return atPagedPoolIndex(atPagedPoolIndex::INVALID_INDEX);
	}

	atPagedPoolIndex GetPagedPoolIndex(_Type* ptr) {
		// Go fishing in all subtypes.
		for (u32 subType=0; subType < m_MaxSubTypes; subType++) {
			atPagedPoolIndex result = GetPagedPoolIndex(ptr, subType);

			if (!result.IsInvalid()) {
				return result;
			}
		}

		return atPagedPoolIndex(atPagedPoolIndex::INVALID_INDEX);
	}

	/** PURPOSE: Return a specific page header.
	 */
	Header &GetPageHeader(atPagedPoolIndex index) const {
		FastAssert(index.GetPage() < m_MaxPages);
		return m_PageHeaders[index.GetPage()];
	}

	/** PURPOSE: Return a specific page header.
	 */
	Header &GetPageHeader(u32 index) const {
		FastAssert(index < m_MaxPages);
		return m_PageHeaders[index];
	}

	/** PURPOSE: Compatibility with atPool - allocate a new element without constructing it.
	 *  THIS FUNCTION IS DEPRECATED - you'll want to use Allocate() so you have an atPagedPoolIndex to
	 *  hold on to. Freeing an element by pointer is not O(1).
	 */
	_Type *New(u32 subType = 0) {
		atPagedPoolIndex index = Allocate(subType);
		return Get(index);
	}

	// Compatibility with atPool - get the number of used elements.
	size_t GetStorageSize() const {
		return sizeof(_Type);
	}

	size_t GetNoOfUsedSpaces(u32 subType = 0) const {
		return m_SubtypeInfo[subType].m_ElementsUsed;
	}

	size_t GetSize(u32 subType = 0) const {
		return m_SubtypeInfo[subType].m_ElementsPerPage * m_MaxPages;
	}

	bool IsValidPtr(const void *ptr) const {
		for (int x=0; x<m_MaxSubTypes; x++) {
			u16 page = m_SubtypeInfo[x].m_FirstUsedPage;
			size_t payloadSize = m_SubtypeInfo[x].m_ElementsPerPage * sizeof(_Type);

			while (page != atPagedPoolBase::NO_MORE_USED) {
				const char *basePtr = (const char *) m_PageHeaders[page].m_Page;

				if (basePtr <= ptr && basePtr + payloadSize >= ptr) {
					return true;
				}

				page = m_PageHeaders[page].m_NextUsedPage;
			}
		}

		return false;
	}

	_Type* GetSlot(s32 index)
	{
		FastAssert(m_MaxSubTypes == 1);

		u32 elementsPerPage = m_SubtypeInfo[0].m_ElementsPerPage;
		u32 pageIndex = (u32) index / elementsPerPage;

		FastAssert(pageIndex < m_MaxPages);

		if (!m_PageHeaders[pageIndex].IsAllocated()) {
			return NULL;
		}

		u32 indexInPage = (u32) index % elementsPerPage;

		if (!m_PageHeaders[pageIndex].IsElementAllocated(indexInPage)) {
			return NULL;
		}

		return m_PageHeaders[pageIndex].GetElement(indexInPage);
	}

	bool GetIsUsed(s32 index) const
	{
		return !GetIsFree(index);
	}

	bool GetIsFree(s32 index) const {
		FastAssert(m_MaxSubTypes == 1);

		u32 elementsPerPage = m_SubtypeInfo[0].m_ElementsPerPage;
		u32 pageIndex = (u32) index / elementsPerPage;

		FastAssert(pageIndex < m_MaxPages);

		if (!m_PageHeaders[pageIndex].IsAllocated()) {
			return true;
		}

		return !m_PageHeaders[pageIndex].IsElementAllocated((u32) index % elementsPerPage);
	}

	void ForAll(Callback cb, void* data) {
		atPagedPool_ForAll(*this, cb, data);
	}




#if __BANK
	void DumpUsage() {
		u32 pageCount = 0;
		u32 elementCount = 0;
		u32 *subTypePageCount = Alloca(u32, m_MaxSubTypes);
		u32 *subTypeElementCount = Alloca(u32, m_MaxSubTypes);

		sysMemSet(subTypePageCount, 0, sizeof(u32) * m_MaxSubTypes);
		sysMemSet(subTypeElementCount, 0, sizeof(u32) * m_MaxSubTypes);

		for (u32 x=0; x<m_MaxPages; x++) {
			Header &header = m_PageHeaders[x];

			if (header.IsAllocated()) {
				u32 subType = header.GetSubType();

				pageCount++;
				subTypePageCount[subType]++;

				elementCount += header.m_UsedCount;
				subTypeElementCount[subType] += header.m_UsedCount;
			}
		}

		u32 totalMemUsage = 0;

		for (u32 x=0; x<m_MaxSubTypes; x++) {
			u32 pages = subTypePageCount[x];
			u32 elementsPerPage = m_SubtypeInfo[x].m_ElementsPerPage;
			u32 memUsage = (pages == 0) ? 0 : (u32) (_Allocator::ComputePageMemUsage((u32) x, elementsPerPage) * pages);
			totalMemUsage += memUsage;
			Displayf("Subtype %2d: %u pages (%u elements each), %u elements, %u free elements, %uKB", x, pages, elementsPerPage,
				subTypeElementCount[x], pages * elementsPerPage - subTypeElementCount[x], memUsage / 1024);
		}

		Displayf("Total count: %u/%u pages, %u elements, %uKB", pageCount, m_MaxPages, elementCount, totalMemUsage / 1024);
	}

	void RegisterPoolCallback(PoolFullCB callback) { m_poolFullCB = callback; }

#endif // __BANK



private:
	/** PURPOSE: Take a new page from the pool of free pages and allocate it for
	 *  a given subtype. This will call the allocator's Allocate() function.
	 *
	 *  PARAMS:
	 *   subType - Sub type for the page.
	 */
	u32 AllocatePage(u32 subType) {
		FastAssert(subType < m_MaxSubTypes);

		SubtypeInfo &subtypeInfo = m_SubtypeInfo[subType];

		// Grab a page from the free list.
		u32 pageIndex = m_FirstFree;

		if (pageIndex == Header::NO_MORE_FREE) {
			// We're out of elements.
#if __BANK
			DumpUsage();

			if (m_poolFullCB)
			{
				m_poolFullCB(NULL);
			}
#endif // __BANK
			Quitf(ERR_GEN_POOL_1,"Paged pool - out of pages when trying to allocate type %u.", subType);
			return (u32) atPagedPoolIndex::INVALID_INDEX;
		}

#if PAGED_POOL_TRACKING
		subtypeInfo.m_PagesUsed++;
#endif // PAGED_POOL_TRACKING

		// Get the page and update the free list.
		u32 elementsPerPage = subtypeInfo.m_ElementsPerPage;
		Header *header = &m_PageHeaders[pageIndex];
		m_FirstFree = header->m_Next;

		header->m_NextUsedPage = subtypeInfo.m_FirstUsedPage;
		subtypeInfo.m_FirstUsedPage = (u16) pageIndex;

#if __DEV
		FastAssert(header->m_SubType == atPagedPoolHeaderBase::PAGE_FREE);
#endif // __DEV

		// Initialize the page header.
		header->m_SubType = (u16) subType;

		header->m_Next = subtypeInfo.m_FirstUsedNonFullPage;

		if (_Iterable) {
			header->m_IterationLayer.Init(elementsPerPage);
		}

		// Use the allocator to actually allocate the page memory.
		size_t memUsage = 0;
		header->m_Page = _Allocator::Allocate(pageIndex, *header, elementsPerPage, m_Alignment, &memUsage);

#if PAGED_POOL_TRACKING
		subtypeInfo.m_MemoryUsage += memUsage;
#endif // PAGED_POOL_TRACKING

		if (!header->m_Page) {
#if __BANK
			// Undo this page first, or else we'll break the debug dump.
#if PAGED_POOL_TRACKING
			subtypeInfo.m_PagesUsed--;
#endif // PAGED_POOL_TRACKING

			u16 *prevNextPtr = &subtypeInfo.m_FirstUsedPage;

			while (*prevNextPtr != pageIndex) {
				prevNextPtr = &m_PageHeaders[*prevNextPtr].m_NextUsedPage;
				FastAssert(*prevNextPtr != NO_MORE_USED);
			}

			*prevNextPtr = header->m_NextUsedPage;
			header->m_NextUsedPage = NO_MORE_USED;

			DumpUsage();

			if (m_poolFullCB)
			{
				m_poolFullCB(NULL);
			}
#endif // __BANK
			Quitf(ERR_GEN_POOL_2,"Out of memory trying to allocate data for paged pool with type %u", subType);
		}

		// Set up the linked list for used pages for this subtype.
		subtypeInfo.m_FirstUsedNonFullPage = (u16) pageIndex;

		// Set up the free list for the page.
		for (u32 x=0; x<elementsPerPage; x++) {
			header->SetNextFree(x, x+1);
		}

		header->SetNextFree(elementsPerPage-1, atPagedPoolHeaderBase::NO_MORE_FREE);
		header->m_FirstFree = 0;

		return pageIndex;
	}

	/** PURPOSE: Free a page. This will free up the memory taken up by it by calling
	 *  Free() of the allocator.
	 *
	 *  PARAMS:
	 *   pageIndex - index of the page to delete.
	 */
	void FreePage(u32 pageIndex) {

		// Get the page header.
		Header *header = &m_PageHeaders[pageIndex];
		u32 subType = header->GetSubType();
#if __DEV
		FastAssert(subType != atPagedPoolHeaderBase::PAGE_FREE);
		FastAssert(header->m_UsedCount == 0);
#endif // __DEV

		if (_Iterable) {
			header->m_IterationLayer.Destroy();
		}

		SubtypeInfo &subtypeInfo = m_SubtypeInfo[subType];

		// Call Free() to free up the actual data.
		size_t memUsage = 0;
		_Allocator::Free(pageIndex, subtypeInfo.m_ElementsPerPage, header->GetElement(0), *header, &memUsage);

		header->m_Page = NULL;

#if PAGED_POOL_TRACKING
		Assert(subtypeInfo.m_MemoryUsage >= memUsage);
		subtypeInfo.m_MemoryUsage -= memUsage;
#endif // PAGED_POOL_TRACKING

#if PAGED_POOL_TRACKING
		subtypeInfo.m_PagesUsed--;
#endif // PAGED_POOL_TRACKING

		// Unlink from the list of used pages (TODO: Maybe we need to use a double linked list?)
		u16 *prevNextPtr = &subtypeInfo.m_FirstUsedPage;

		while (*prevNextPtr != pageIndex) {
			prevNextPtr = &m_PageHeaders[*prevNextPtr].m_NextUsedPage;
			FastAssert(*prevNextPtr != NO_MORE_USED);
		}

		*prevNextPtr = header->m_NextUsedPage;
		header->m_NextUsedPage = NO_MORE_USED;

#if __DEV
		header->m_SubType = atPagedPoolHeaderBase::PAGE_FREE;
#endif // __DEV

		header->m_Next = m_FirstFree;
		m_FirstFree = (u16) pageIndex;
	}

	// Array of paged headers, the size of this array is specified in m_MaxPages.
	Header *m_PageHeaders;

#if __BANK
	PoolFullCB m_poolFullCB;
#endif //!__BANK
};


/** PURPOSE: This is an iterator to walk through every element in the pool. NOTE that the order in which the iterator operates
 *  may not be what you might expect due to the nature of atBitSet - Within a page, it may start at element 31, then go back down to element 0,
 *  then go up to element 63, slowly down to element 32, etc.
 *
 *  Note that a pagedPoolIterator can only be created for a paged pool that has its _Iterable argument set to true.
 *
 *  To create an iterator, construct it with the pool as an argument, like this:
 *
 *  atPagedPool<Type, true> pool;
 *
 *  atPagedPoolIterator<Type> it(pool);
 *  while (!it.IsAtEnd()) {
 *     atPagedPoolIndex index = it.GetPagedPoolIndex();			// Access current element via pool index
 *     it->foo();												// Access current element via -> operator
 *     ++it;													// Do not use the postfix operator!
 *  }
 */
template<typename _Type, typename _Allocator = atDefaultPagedPoolAllocator<_Type, true> >
struct atPagedPoolSubtypeIterator {

	/** PURPOSE: Construct the iterator and set it to the beginning.
	 */
	atPagedPoolSubtypeIterator(atPagedPool<_Type, true, _Allocator> &pool, u32 subType) {
		m_Pool = &pool;
		Reset(subType);
	}

	/** PURPOSE: Go back to the initial position.
	 */
	void Reset(u32 subType) {

		u32 firstPage = m_Pool->GetFirstUsedPage(subType);

		if (firstPage == atPagedPool<_Type, true, _Allocator>::NO_MORE_USED) {
			// No elements.
			m_CurrentPosition = atPagedPoolIndex(firstPage, 0);
		} else {
			u32 firstElement = m_Pool->GetPageHeader(firstPage).m_IterationLayer.GetFirstUsedElement();
			m_CurrentPosition = atPagedPoolIndex(firstPage, firstElement);
		}
	}

	/** PURPOSE: Return the current position as an atPagedPoolIndex.
	 */
	atPagedPoolIndex GetPagedPoolIndex() const {
		return m_CurrentPosition;
	}

	/** PURPOSE: Access the current element.
	 */
	_Type *operator ->() {
		if (IsAtEnd()) {
			return NULL;
		}

		return m_Pool->Get(m_CurrentPosition);
	}

	_Type &operator *() {
		return *(m_Pool->Get(m_CurrentPosition));
	}

	/** PURPOSE: Move to the next element. This may not be called if the iterator is already at the end,
	 *  which can be checked with IsAtEnd().
	 */
	void operator++() {
		FastAssert(!IsAtEnd());
		u32 page = m_CurrentPosition.GetPage();
		u32 element = m_CurrentPosition.GetElement();

		element = m_Pool->GetPageHeader((u32) page).m_IterationLayer.GetNextUsedElement(element);

		if (element == (u32) ~0U) {
			page = m_Pool->GetPageHeader(page).m_NextUsedPage;

			if (page != atPagedPoolBase::NO_MORE_USED) {
				element = m_Pool->GetPageHeader(page).m_IterationLayer.GetFirstUsedElement();
			}
		}

		m_CurrentPosition = atPagedPoolIndex(page, element);
	}

	/** PURPOSE: True if the iterator is at the end. If this is the case, the current position may not
	 *  be accessed, and the ++ operator may not be invoked.
	 */
	bool IsAtEnd() const {
		return m_CurrentPosition.GetPage() == atPagedPool<_Type, true, _Allocator>::NO_MORE_USED;
	}


protected:
	// This is private - postfix on an iterator is a bad idea.
	void operator++(int) {};


	atPagedPool<_Type, true, _Allocator> *m_Pool;

	atPagedPoolIndex m_CurrentPosition;
};


template<typename _Type, typename _Allocator = atDefaultPagedPoolAllocator<_Type, true> >
struct atPagedPoolIterator : public atPagedPoolSubtypeIterator<_Type, _Allocator> {

	typedef atPagedPoolSubtypeIterator<_Type, _Allocator> base;

	/** PURPOSE: Construct the iterator and set it to the beginning.
	 */
	atPagedPoolIterator(atPagedPool<_Type, true, _Allocator> &pool)
		: atPagedPoolSubtypeIterator<_Type, _Allocator>(pool, 0)
	{
		Reset();
	}

	void Reset() {
		// Keep trying until we find a subtype that has elements.
		u32 subType = 0;
		u32 highestSubtype = base::m_Pool->GetMaxSubTypes() - 1;
		while (base::IsAtEnd() && subType < highestSubtype) {
			base::Reset(++subType);
		}

		m_CurrentSubtype = subType;
	}

	void operator++() {
		FastAssert(!base::IsAtEnd());

		// Try going to the next element.
		base::operator++();

		// If we're not at the end, that's fine.
		while (base::IsAtEnd()) {
			// If we are, try the next subtype.
			if (++m_CurrentSubtype == base::m_Pool->GetMaxSubTypes()) {
				// We've reached the end.
				return;
			}

			base::Reset(m_CurrentSubtype);
		}
	}

private:
	u32 m_CurrentSubtype;
};

template<typename _Type, typename _Allocator>
void atPagedPool_ForAll(atPagedPool<_Type, true, _Allocator> &pool, atPagedPoolBase::Callback cb, void* data) {
	atPagedPoolIterator<_Type, _Allocator> it(pool);

	while (!it.IsAtEnd()) {
		if (!cb(&(*it), data)) {
			return;
		}

		++it;
	}
}



}   // namespace rage

#endif
