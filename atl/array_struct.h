// 
// atl/array_struct.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_ARRAY_STRUCT_H
#define ATL_ARRAY_STRUCT_H

#include "atl/array.h"
#include "data/struct.h"

#if __DECLARESTRUCT

namespace rage {

template <class _T,int _Align,class _CounterType>
inline void atArray<_T,_Align,_CounterType>::DeclareStruct(datTypeStruct &s) {
	STRUCT_BEGIN(atArray<_T>);
	STRUCT_DYNAMIC_ARRAY(m_Elements,m_Count);
	STRUCT_FIELD(m_Count);
	STRUCT_FIELD(m_Capacity);
	STRUCT_PADDING64(m_Padding);
	STRUCT_END();
}

template <class _T,int _MaxCount> 
inline void atFixedArray<_T,_MaxCount>::DeclareStruct(datTypeStruct &s) {
	// We cannot use STRUCT_BEGIN here because the preprocessor gets confused by the template instantiation
	STRUCT_BEGIN(atFixedArray);
	STRUCT_CONTAINED_ARRAY(m_Elements);
	STRUCT_FIELD(m_Count);
	STRUCT_END();
}

template <class _T,int _MaxCount> 
inline void atRangeArray<_T,_MaxCount>::DeclareStruct(datTypeStruct &s) {
	// We cannot use STRUCT_BEGIN here because the preprocessor gets confused by the template instantiation
	STRUCT_BEGIN(atRangeArray);
	STRUCT_CONTAINED_ARRAY(m_Elements);
	STRUCT_END();
}

template <class _T,int _Align,class _CounterType>
inline void atArray<_T, _Align, _CounterType>::DatSwapAllButContents() {
	datSwapper(m_Elements);
	datSwapper(m_Count);
	datSwapper(m_Capacity);
}

#if 0
/*!
 *  PURPOSE: Swap the magic array count cookie in an atArray. You must call this function inside DeclareStruct
 *  if and only if the atArray in question uses a type that has a destructor. If this array is an array of
 *  an array, you also need to call this function on each contained array (if those arrays use a type that has
 *  a destructor). You may also use the ATARRAY_SWAP_2D_ARRAY_COUNT macro for that purpose.
 */
template <class _T, class _CounterType, unsigned int _CounterMax>
inline void atArray<_T, _CounterType, _CounterMax>::SwapArrayCount()
{
#if !__FINAL
	if (GetCount() != GetCapacity())
	{
		Warningf("The atArray of an object that is being resourced has been created inefficiently and is wasting %d bytes.", (int)sizeof(_T) * (GetCapacity() - GetCount()));
	}
#endif // !__FINAL

	if (GetCapacity())
	{
		datSwapArrayCount(m_Elements, GetCapacity());
	}
}

/*!
 *  PURPOSE: Swap the magic cookie in an atArray that contains atArrays in itself. You must call this function
 *  inside DeclareStruct if and only if the inner atArray in question uses a type that has a destructor (that
 *  obviously applies to the outer atArray as well, since atArray has a destructor). This function will swap the
 *  magic array count in all sub-arrays and in the top-level array itself. If this is an atArray of an atArray of
 *  an atArray, the function will *not* swap the counts of the lowest-level arrays. On the other hand, if you
 *  have an atArray of an atArray of an atArray, there is a faint possibility that you might need to have
 *  your head examined.
 *
 *  PARAMS:
 *   array - The atArray that contains an atArray with elements that need to be swapped. It is legal to pass
 *   an empty array or an array that contains empty arrays.
 */
#define ATARRAY_SWAP_2D_ARRAY_COUNT(_array)		{		\
	_array.SwapArrayCount();							\
	for (int i=_array.GetCount()-1; i>=0; i--)			\
		_array[i].SwapArrayCount();						\
	}

#endif


}	// namespace rage

#endif	// __DECLARESTRUCT

#endif // ATL_ARRAY_STRUCT_H
