// 
// atl/binmap_struct.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_BINMAP_STRUCT_H 
#define ATL_BINMAP_STRUCT_H 

#include "atl/binmap.h"
#include "data/struct.h"

namespace rage {

template <class _T, class _KeyType, class _HashedKeyType, class _HashFnObj>
template <typename _Placer>
inline atHashedMap<_T, _KeyType, _HashedKeyType, _HashFnObj>::atHashedMap(datResource& rsc, _Placer p)
: atBinaryMap<_T, _HashedKeyType>(rsc, p)
{
}

template <class _T> 
template <typename _Placer>
inline atStringMap<_T>::atStringMap(datResource& rsc, _Placer p)
: atHashedMap<_T, const char*, u32, atStringHashObj>(rsc, p)
{
}

template <class _T> 
template <typename _Placer>
inline atCaseSensitiveStringMap<_T>::atCaseSensitiveStringMap(datResource& rsc, _Placer p)
: atHashedMap<_T, const char*, u32, atCaseSensitiveStringHashObj>(rsc, p)
{
}

#ifdef _CPPRTTI
template <class _T> 
template <typename _Placer>
inline atTypeMap<_T>::atTypeMap(datResource& rsc, _Placer p)
: atHashedMap<_T, type_info, u32, atTypeHashObj>(rsc, p)
{	
}
#endif


#if __DECLARESTRUCT

template <class _T, class _KeyType>
inline void atBinaryMap<_T, _KeyType>::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(atBinaryMap);
		STRUCT_FIELD(Sorted);
		STRUCT_CONTAINED_ARRAY(Pad);
		STRUCT_FIELD(Data);
	STRUCT_END();
}

template<class _T, class _KeyType>
inline void atBinaryMap<_T, _KeyType>::DataPair::DeclareStruct(datTypeStruct& s)
{
	STRUCT_BEGIN(atBinaryMap::DataPair);
		STRUCT_FIELD(key);
		STRUCT_FIELD(data);
	STRUCT_END();
}

#endif // __DECLARESTRUCT

} // namespace rage

#endif // ATL_BINMAP_STRUCT_H 
