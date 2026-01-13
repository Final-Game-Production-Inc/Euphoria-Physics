//
// atl/slist.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_SLIST_H
#define ATL_SLIST_H


////////////////////////////////////////////////////////////////
// external defines

#include "data/resource.h"

namespace rage {


////////////////////////////////////////////////////////////////
// atSNode

//
// PURPOSE:
// The linked element in a atSList.
//
template <class _Type> class atSNode
{
public:
	// PURPOSE: Constructor
	atSNode<_Type> ()													{m_Next=NULL;}
	
	// PURPOSE: Overload Construcor
	atSNode<_Type> (const _Type & d)										{Data=d; m_Next=NULL;}

	// PURPOSE: No-op resource constructor that you can use to avoid invoking the default constructor.
	// NOTES:	This doesn't fix up the actual pointers. Normally this is done automatically for you
	//			through atSList::atSList<_Type>(datResource&).
	atSNode<_Type> (datResource &/*rsc*/)								{	}

	// PURPOSE: Get the next node in constant mode
	// RETURN : the const pointer to the next node
	const atSNode<_Type> * GetNext () const								{return m_Next;}

	// PURPOSE: Get the next node 
	// RETURN : the pointer to the next node
	atSNode<_Type> * GetNext ()											{return m_Next;}

	// PURPOSE: Set the next pointer
	// PARAMS : next - the node to be set as the next node
	void SetNext (atSNode<_Type> * next)								{m_Next = next;}

	// PURPOSE: Fix up the pointer when loading from a resource.
	// NOTES:	Intended to be called by the resource constructor of atSList.
	inline void PointerFixup(datResource &rsc)
	{	rsc.PointerFixup(m_Next);	}

public:
	// data stored at this node
	_Type Data;

protected:
	// next node in list
	atSNode<_Type> * m_Next;
};


////////////////////////////////////////////////////////////////
// atSList

//
// PURPOSE:
//	An atSList is a singly linked list of
//	atSNode's.
// <FLAG Component>
template <class _Type> class atSList
{
public:
	// PURPOSE:	Constructor
	atSList<_Type> ()													{m_Head=NULL; m_Tail=NULL; m_NumItems=0;}

	// PURPOSE: Resource constructor
	inline atSList<_Type>(datResource &rsc);

	// PURPOSE: Delete all the nodes in the list (only use if the list owns all the nodes!)
	void DeleteAll ();

	//////////// accessors ////////////////

	// PURPOSE:	get the constant pointer to the head in the list
	// RETURN :	The const pointer to the first node in the list
	const atSNode<_Type> * GetHead () const								{return m_Head;}

	// PURPOSE:	get the pointer to the head in the list
	// RETURN :	The pointer to the first node in the list
	atSNode<_Type> * GetHead ()											{return m_Head;}

	// PURPOSE:	get the constant pointer to the tail in the list
	// RETURN :	The const pointer to the last node in the list
	const atSNode<_Type> * GetTail () const								{return m_Tail;}

	// PURPOSE:	get the pointer to the tail in the list
	// RETURN :	The pointer to the last node in the list
	atSNode<_Type> * GetTail ()											{return m_Tail;}

	// PURPOSE:	get the number of items in the list
	// RETURN :	The number of items in the list
	// 
	int GetNumItems () const										{return m_NumItems;}

	/////////////////// O(1) time manipulators ///////////////////////////////////

	// PURPOSE:	prepend 'node' to the front of the list
	// PARAMS : node -  the node to insert in to the list
	void Prepend (atSNode<_Type> & node);

	// PURPOSE:	append 'node' to the back of the list
	// PARAMS : node -  the node to insert in to the list
	void Append (atSNode<_Type> & node);
	
	// PURPOSE:	insert 'node' after 'insertPoint'
	// PARAMS :	insertPoint - node that act as indication where the new node should be placed
	//			node - the new node to be added in to the list
	void InsertAfter (atSNode<_Type> & insertPoint, atSNode<_Type> & node);	

	// PURPOSE: remove the head of the list
	// RETURN :	the pointer to the head, which is the first node on the list
	atSNode<_Type> * PopHead ();
	
	// PURPOSE:	remove prev->Next from the list
	// PARAMS : prev - the previous node 
	// RETURN : the removed node
	atSNode<_Type> * RemoveNext (atSNode<_Type> & prev);

	// PURPOSE:	remove prev->Next from the list
	// PARAMS : prev - the previous node.  if prev is NULL, then the first node in the list will be removed
	// RETURN : the removed node
	atSNode<_Type> * RemoveNext (atSNode<_Type> * prev);

	// PURPOSE:	remove the node specified
	// PARAMS : node - the node to be removed
	// RETURN : true if the node was successfully found and removed
	bool Remove(atSNode<_Type>& node);

	// PURPOSE:	access const item in list linearly
	// PARAMS : index - 0 is the first item in the list, m_NumItems-1 is the last
	// RETURN : the node being accessed
	// NOTE : this performs a linear traversal, so use it sparingly
	const _Type& operator[](int index) const;

	// PURPOSE:	access item in list linearly
	// PARAMS : index - 0 is the first item in the list, m_NumItems-1 is the last
	// RETURN : the node being accessed
	// NOTE : this performs a linear traversal, so use it sparingly
	_Type& operator[](int index);

	void Reset() { m_Head=NULL; m_Tail=NULL; m_NumItems=0; }
protected:

	// PURPOSE:	have the tow list exchange pointers to the head and tail, as well as item count
	// PARAMS : list that is a being swapped
	void	SwapList(atSList<_Type>& list);

private:
	// beginning of the list
	atSNode<_Type> * m_Head;

	// end of the list
	atSNode<_Type> * m_Tail;

	// number of items in list
	int m_NumItems;
};



template <class _Type> atSList<_Type>::atSList(datResource &rsc)
{
	rsc.PointerFixup(m_Head);
	rsc.PointerFixup(m_Tail);

	// Can't use placement new here because 'new' may be #defined
	// and we can't do '#undef new' either.
	atSNode<_Type> *node;
	for(node = m_Head; node; node = node->GetNext())
		node->PointerFixup(rsc);
}



template <class _Type> void atSList<_Type>::DeleteAll ()
{
	while (m_Head)
	{
		atSNode<_Type> * node = PopHead();
		delete node;
	}
}



template <class _Type> void atSList<_Type>::Prepend (atSNode<_Type> & node)
{
	FastAssert(node.GetNext()==NULL);

	atSNode<_Type> * oldHead = m_Head;
	m_Head = &node;

	m_Head->SetNext(oldHead);

	if (m_Tail==NULL)
	{
		m_Tail = &node;
	}

	m_NumItems++;
}


template <class _Type> void atSList<_Type>::Append (atSNode<_Type> & node)
{
	FastAssert(node.GetNext()==NULL);

	atSNode<_Type> * oldTail = m_Tail;
	m_Tail = &node;

	if (oldTail)
	{
		oldTail->SetNext(&node);
	}

	if (m_Head==NULL)
	{
		m_Head = &node;
	}

	m_NumItems++;
}


template <class _Type> void atSList<_Type>::InsertAfter (atSNode<_Type> & insertPoint, atSNode<_Type> & node)
{
	FastAssert(node.GetNext()==NULL);

	if (&insertPoint==m_Tail)
	{
		m_Tail = &node;
	}

	node.SetNext(insertPoint.GetNext());
	insertPoint.SetNext(&node);
	m_NumItems++;
}


template <class _Type> atSNode<_Type> * atSList<_Type>::PopHead ()
{
	atSNode<_Type> * oldHead = m_Head;

	if (oldHead)
	{
		m_Head = oldHead->GetNext();
		oldHead->SetNext(NULL);

		if (m_Tail==oldHead)
		{
			m_Tail=NULL;
		}
		m_NumItems--;
	}
	return oldHead;
}


template <class _Type> atSNode<_Type> * atSList<_Type>::RemoveNext (atSNode<_Type> & prev)
{
	atSNode<_Type> *deader = prev.GetNext();
	if (deader)
	{
		prev.SetNext(deader->GetNext());
		deader->SetNext(NULL);
		if (prev.GetNext() == NULL)
			m_Tail = &prev;
		m_NumItems--;
	}
	return deader;
}


template <class _Type> atSNode<_Type> * atSList<_Type>::RemoveNext (atSNode<_Type> * prev)
{
	if (prev == NULL)
		return PopHead();

	atSNode<_Type> *deader = prev->GetNext();
	if (deader)
	{
		prev->SetNext(deader->GetNext());
		deader->SetNext(NULL);
		if (prev->GetNext() == NULL)
			m_Tail = prev;
		m_NumItems--;
	}
	return deader;
}

template <class _Type> bool atSList<_Type>::Remove(atSNode<_Type>& node)
{
	bool retVal = false;

	atSNode<_Type>* current = GetHead();
	atSNode<_Type>* prev = NULL;
	while(current)
	{
		if(current == &node)
		{
			if(prev)
				current = RemoveNext(*prev);
			else
				current = PopHead();
			Assert(current == &node);
			retVal = true;
			break;
		}
		prev = current;
		current = current->GetNext();
	}
	return retVal;
}


template <class _Type> _Type & atSList<_Type>::operator[] (int index)
{
	FastAssert(index >=0 && index < m_NumItems);
	atSNode<_Type> *current = m_Head;
	while (index)
	{
		current = current->GetNext();
		index--;
	}
	return current->Data;
}


template <class _Type> const _Type & atSList<_Type>::operator[] (int index) const
{
	FastAssert(index >=0 && index < m_NumItems);
	const atSNode<_Type> *current = m_Head;
	while (index)
	{
		current = current->GetNext();
		index--;
	}
	return current->Data;
}

template <class _Type> void atSList<_Type>::SwapList(atSList<_Type>& list)
{
	atSList<_Type> tempList;
	tempList.m_Head = m_Head;
	tempList.m_Tail = m_Tail;
	tempList.m_NumItems = m_NumItems;

	m_Head = list.m_Head;
	m_Tail = list.m_Tail;
	m_NumItems = list.m_NumItems;

	list.m_Head = tempList.m_Head;
	list.m_Tail = tempList.m_Tail;
	list.m_NumItems = tempList.m_NumItems;

	tempList.m_Head = NULL;
	tempList.m_Tail = NULL;
	tempList.m_NumItems = 0;
}


////////////////////////////////////////////////////////////////
// atSListPreAlloc

// PURPOSE:
//	Pre-allocates and manages the list nodes.
template <class _Type> class atSListPreAlloc : public atSList<_Type>
{
public:
	
	// PURPOSE:	Constructor
	atSListPreAlloc<_Type>() : m_NodeHeap(NULL), m_FirstNode(NULL), m_NumNodes(0), m_MaxNodes(0)	{}

	// PURPOSE:	Destructor, clean up memory
	~atSListPreAlloc<_Type>()					{delete[] m_NodeHeap;}

	// PURPOSE:	allocate numbers of node for the list 
	// PARAMS : maxNodes - the maximum number of nodes in this list
	void AllocateNodes(int maxNodes);

	// PURPOSE: kill the nodes and free up the memory
	void DeallocateNodes();

	// PURPOSE:	get the number of nodes exist in the list
	// RETURNS:	the number of nodes in the list
	int GetNumNodes() const					{return m_NumNodes;}

	// PURPOSE:	get the maximum number of nodes in the list
	// RETURNS: maximum number of nodes in the list
	int GetMaxNodes() const					{return m_MaxNodes;}

	// PURPOSE:	release a node from the list if there is still node available
	// RETURNS:	pointer to the released node from the list
	atSNode<_Type> *RequestNode();

	// PURPOSE:	put the node back in to the list for reuse
	// PARAMS : the node for recycling..
	void RecycleNode(atSNode<_Type> *node);

protected:
	atSNode<_Type> *m_NodeHeap;
	atSNode<_Type> *m_FirstNode;
	int m_NumNodes;
	int m_MaxNodes;
};


template <class _Type> void atSListPreAlloc<_Type>::AllocateNodes(int maxNodes)
{
	m_NumNodes = 0;
	m_MaxNodes = maxNodes;

	m_NodeHeap = rage_new atSNode<_Type>[maxNodes];
	m_FirstNode = m_NodeHeap;
	for (int i = maxNodes-2; i >= 0; i--)
		m_NodeHeap[i].SetNext(&m_NodeHeap[i+1]);
}


template <class _Type> void atSListPreAlloc<_Type>::DeallocateNodes()
{
	m_NumNodes = 0;
	m_MaxNodes = 0;
	delete[] m_NodeHeap;
	m_FirstNode = m_NodeHeap = NULL;
}


template <class _Type> atSNode<_Type> *atSListPreAlloc<_Type>::RequestNode()
{
	FastAssert(m_NumNodes < m_MaxNodes);
	atSNode<_Type> *node = m_FirstNode;
	m_FirstNode = m_FirstNode->GetNext();
	node->SetNext(NULL);
	m_NumNodes++;

	return node;
}


template <class _Type> void atSListPreAlloc<_Type>::RecycleNode(atSNode<_Type> *node)
{
	FastAssert(m_NumNodes > 0 && node >= m_NodeHeap && node < m_NodeHeap+m_MaxNodes);
	node->SetNext(m_FirstNode);
	m_FirstNode = node;
	m_NumNodes--;
}

}	// namespace rage

#endif // ndef ATL_SLIST_H
