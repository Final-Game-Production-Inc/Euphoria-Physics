// 
// atl/bucket_struct.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_BUCKET_STRUCT_H
#define ATL_BUCKET_STRUCT_H

#include "atl/bucket.h"
#include "data/struct.h"

namespace rage {

/* THIS NEEDS TO BE INPLEMENTED CORRECTLY
template <class _T,int _AllocStep> 
inline void atBucket<_T,_AllocStep>::DeclareStruct(datTypeStruct &s) {
	STRUCT_BEGIN(atBucket<_T>);
	STRUCT_DYNAMIC_ARRAY(m_Elements, m_ChunkAlloc);
	STRUCT_FIELD(m_ChunkSize);
	STRUCT_FIELD(m_ChunkMask);
	STRUCT_FIELD(m_ChunkShift);
	STRUCT_FIELD(m_Count);
	STRUCT_FIELD(m_ChunkAlloc);
	STRUCT_END();
}
*/

}	// namespace rage

#endif // ATL_BUCKET_STRUCT_H
