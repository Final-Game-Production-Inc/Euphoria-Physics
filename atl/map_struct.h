// 
// atl/map_struct.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_MAP_STRUCT_H
#define ATL_MAP_STRUCT_H

#include "atl/map.h"
#include "data/struct.h"

namespace rage {

#if __DECLARESTRUCT
template <class _Key, class _Data>
void atMapEntry<_Key, _Data>::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(atMapEntry);

	STRUCT_FIELD(key);
	STRUCT_POTENTIAL_PADDING(key, data);
	STRUCT_FIELD(data);

	if (next)
	{
		datTypeStruct ts;
		next->DeclareStruct(ts);
	}

	STRUCT_POTENTIAL_PADDING(data, next);
	STRUCT_FIELD_VP(next);

	STRUCT_END();
}


template <class _Key, class _Data, class _Hash, class _Equals, class _MemoryPolicy>
void atMap<_Key, _Data, _Hash, _Equals, _MemoryPolicy>::DeclareStruct(datTypeStruct &s)
{

	STRUCT_BEGIN(atMap);

	if (m_Hash)
	{
		for (int slot=0; slot < m_Slots; slot++)
		{
			if (m_Hash[slot])
			{
				datTypeStruct ts;

				m_Hash[slot]->DeclareStruct(ts);
				datSwapper((void *&) m_Hash[slot]);
			}
		}
	}

	STRUCT_FIELD_VP(m_Hash);
	STRUCT_FIELD(m_Slots);
	STRUCT_FIELD(m_Used);
	STRUCT_SKIP(m_HashFn, 1);
	STRUCT_SKIP(m_Equals, 1);
	STRUCT_SKIP(m_Memory, 1);
	STRUCT_FIELD(m_AllowReCompute);

	STRUCT_END();
}

#endif // !__FINAL


}	// namespace rage

#endif // ATL_ARRAY_STRUCT_H
