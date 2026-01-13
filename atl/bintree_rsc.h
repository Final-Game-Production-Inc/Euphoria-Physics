//
// atl/bintree_rsc.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_BINTREE_RSC_H
#define ATL_BINTREE_RSC_H

/* atl/bintree_rsc.h */

namespace rage {

/*
PURPOSE
	This file is intended to be included after 'data/resourcehelpers.h', i.e.
	at the end of an implementation file. It defines resource constructors
	for the atBinTree template and its associated classes.
*/

// PURPOSE: Fix up pointers when loading from a resource.
// NOTES:	Intended to be called by the resource constructor of atBinTree.
//			Please avoid the temptation of making this an actual constructor
//			instead of just a function, because that is going to invoke
//			a default constructor on the key and the data.
template<typename _Key, typename _Data> atBinTreeBase<_Key, _Data>::Node::Node(datResource &rsc)
	: m_Data(rsc)
	, m_Key(rsc)
	, m_ChildLeft(rsc)
	, m_ChildRight(rsc)
{
}


template<typename _Key, typename _Data> atBinTreeBase<_Key, _Data>::Node::Node(datResource &rsc,
		void (*fixupKeyFunc)(datResource &rsc, _Key &key),
		void (*fixupDataFunc)(datResource &rsc, _Data &data))
	: m_ChildLeft(rsc, fixupKeyFunc, fixupDataFunc)
	, m_ChildRight(rsc, fixupKeyFunc, fixupDataFunc)
{
	if(fixupKeyFunc)
		fixupKeyFunc(rsc, m_Key);

	if(fixupDataFunc)
		fixupDataFunc(rsc, m_Data);
}

/*
PURPOSE
	This constructor can be called to fix up the pointers of an atBinTreeBase
	when loading from a resource.
PARAMS
	rsc				- The datResource object.
	fixupKeyFunc	- A function that fixes up the keys of the map, or NULL.
	fixupDataFunc	- A function that fixes up the data in the map, or NULL.
*/
template <class _Key, class _Data> atBinTreeBase<_Key, _Data>::atBinTreeBase(datResource &rsc,
			void (*fixupKeyFunc)(datResource &rsc, _Key &key),
			void (*fixupDataFunc)(datResource &rsc, _Data &data))
	: m_Root(rsc, fixupKeyFunc, fixupDataFunc)
{
}

template <class _Key, class _Data> atBinTreeBase<_Key, _Data>::atBinTreeBase(datResource &rsc)
	: m_Root(rsc)
{
}

}	// namespace rage

#endif

/* End of file atl/bintree_rsc.h */
