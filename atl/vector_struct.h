// 
// atl/vector_struct.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_VECTOR_STRUCT_H
#define ATL_VECTOR_STRUCT_H

#include "atl/array.h"
#include "data/struct.h"

#if __DECLARESTRUCT

namespace rage {

	template <class _T,int _Align,class _CounterType>
	inline void atVector<_T,_Align,_CounterType>::DeclareStruct(datTypeStruct &s) {
		STRUCT_BEGIN(atVector<_T>);
		STRUCT_DYNAMIC_ARRAY(m_Elements,m_Count);
		STRUCT_FIELD(m_Count);
		STRUCT_FIELD(m_Capacity);
		STRUCT_END();
	}

	template <class _T,int _MaxCount> 
	inline void atFixedVector<_T,_MaxCount>::DeclareStruct(datTypeStruct &s) {
		// We cannot use STRUCT_BEGIN here because the preprocessor gets confused by the template instantiation
		STRUCT_BEGIN(atFixedVector);
		STRUCT_CONTAINED_ARRAY(m_Elements);
		STRUCT_FIELD(m_Count);
		STRUCT_END();
	}

} // namespace rage

#endif // ATL_VECTOR_STRUCT_H