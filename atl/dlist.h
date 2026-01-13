//
// atl/dlist.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_DLIST_H
#define ATL_DLIST_H


////////////////////////////////////////////////////////////////
// external defines

#include "data/resource.h"
#include "data/struct.h"

namespace rage {

	class datTypeStruct;

// PURPOSE:
//	The sole purpose of this class is to provide an empty base class for atDNode.
//
class atDNodeEmptyClass
{
};

////////////////////////////////////////////////////////////////
// atDNode

// PURPOSE: An atDNode is a templated node of an atDList.  An atDList is a templated
//          doubly-linked list of atDNodes.
// PARAMETERS:
//	_Type - the type that this node holds
//	_Base - the base class of this node.  If you subclass this template and you need a 
// virtual destructor, set this to datBase or some other class that has a virtual destructor.
// 
//<FLAG Component>
template <class _Type, class _Base = atDNodeEmptyClass > class atDNode : public _Base
{
public:

	// PURPOSE: Constructor
	atDNode<_Type,_Base> ()													{Next=NULL; Prev=NULL;}

	// PURPOSE: Destructor
	~atDNode<_Type,_Base> ()												{}

	// PURPOSE: Overloaded Constructor
	// PARAMS: d - data that wish to store at this node
	atDNode<_Type,_Base> (const _Type & d)										{Data=d; Next=NULL; Prev=NULL;}

	// PURPOSE: No-op resource constructor that you can use to avoid invoking the default constructor.
	// NOTES:	This doesn't fix up the actual pointers. Normally this is done automatically for you
	//			through atDList::atDList<_Type,_Base>(datResource&).
	atDNode<_Type,_Base> (datResource &/*rsc*/)								{	}

	// PURPOSE: Get the next node from the list, usable with const data
	// RETURNS: the next node from the current node in const 
	const atDNode<_Type,_Base> * GetNext () const								{return Next;}

	// PURPOSE: Get the next node from the list, usable with non const data
	// RETURNS: the next node from the current node
	atDNode<_Type,_Base> * GetNext ()											{return Next;}

	// PURPOSE: Set the current node's next pointer to the given address
	// PARAMS:	next - data that wish to store at this node's next pointer
	void SetNext (atDNode<_Type,_Base> * next)								{Next = next;}

	// PURPOSE: Get the previous node from the list, usable with const data
	// RETURNS: the previous node from the current node in const 
	const atDNode<_Type,_Base> * GetPrev () const								{return Prev;}

	// PURPOSE: Get the previous node from the list, usable with non const data
	// RETURNS: the previous node from the current node  
	atDNode<_Type,_Base> * GetPrev ()											{return Prev;}

	// PURPOSE: Set the current node's previous pointer to the given address
	// PARAMS:	prev - data that wish to store at this node's prev pointer
	void SetPrev (atDNode<_Type,_Base> * prev)								{Prev = prev;}

	// PURPOSE: Insert 'node' before this
	// PARAMS:	node - 'node' to insert in to the list
	void Insert (atDNode<_Type,_Base> & node);

	// PURPOSE: Append 'node' after this
	// PARAMS:	node - 'node' to append in to the list
	void Append (atDNode<_Type,_Base> & node);

	// PURPOSE: Detach this node from the list it is in
	// NOTES:	Please be very careful with this function - normally
	//			atDList<_Type,_Base>::PopNode() should be used instead if you want
	//			to remove a node from a list, because Detach() doesn't
	//			know what list to remove the node from and will mess up
	//			the list if the node happens to be the first or the last one.
	void Detach ();

	// PURPOSE: Fix up pointers when loading from a resource.
	// NOTES:	Intended to be called by the resource constructor of atDList.
	inline void PointerFixup(datResource &rsc)
	{	rsc.PointerFixup(Next);
		rsc.PointerFixup(Prev);
	}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s)
	{
		STRUCT_BEGIN(atDNode);
		STRUCT_FIELD_VP(Next);
		STRUCT_FIELD_VP(Prev);
		STRUCT_END();
	}
#endif // __DECLARESTRUCT

public:
	// data stored at this node
	_Type Data;															

protected:
	// next node in list
	atDNode<_Type,_Base> * Next;												
	// previous node in list
	atDNode<_Type,_Base> * Prev;												
};


template <class _Type,class _Base> void atDNode<_Type,_Base>::Insert (atDNode<_Type,_Base> & node)
{
	node.SetNext(this);
	node.SetPrev(Prev);
	if (Prev)
	{
		Prev->SetNext(&node);
	}
	Prev = &node;
}


template <class _Type,class _Base> void atDNode<_Type,_Base>::Append (atDNode<_Type,_Base> & node)
{
	node.SetNext(Next);
	node.SetPrev(this);
	if (Next)
	{
		Next->SetPrev(&node);
	}
	Next = &node;
}


template <class _Type,class _Base> void atDNode<_Type,_Base>::Detach ()
{
	if (Next)
	{
		Next->SetPrev(Prev);
	}
	if (Prev)
	{
		Prev->SetNext(Next);
	}
	Next = NULL;
	Prev = NULL;
}


////////////////////////////////////////////////////////////////
// atDList

// PURPOSE: An atDList is a templated doubly-linked list of atDNodes.
// PARAMETERS:
//	_Type - the type that this node holds
//	_Base - the base class of this node.  If you subclass this template and you need a 
// virtual destructor, set this to datBase or some other class that has a virtual destructor.
//<FLAG Component>
template <class _Type, class _Base = atDNodeEmptyClass > class atDList
{
public:

	// PURPOSE: Constructor
	atDList<_Type,_Base> ()													{Head=NULL; Tail=NULL;}

	// PURPOSE: Resource constructor
	inline atDList<_Type,_Base>(datResource &rsc);

	// PURPOSE: Resource constructor that constructs data as well, flag must be true.
	inline atDList<_Type,_Base>(datResource &rsc, bool flag);

	// PURPOSE: Force to empty the list by set Head and Tail to NULL
	void Empty ()													{Head=NULL; Tail=NULL;}

	// PURPOSE: Remove all the nodes in the list quickly
	void RemoveAll ();

	// PURPOSE: Delete all the nodes in the list in reverse order (only use if the list owns all the nodes!)
	void DeleteAll ();

	// PURPOSE: Get the data address from the const head pointer
	// RETURN :	the constant address from head pointer 
	const atDNode<_Type,_Base> * GetHead () const								{return Head;}

	// PURPOSE: Get the data address from the head pointer
	// RETURN :	the address from the head pointer
	atDNode<_Type,_Base> * GetHead ()											{return Head;}

	// PURPOSE: Get the constant address from the Tail pointer 
	// RETURN :	the constant address from the tail pointer
	const atDNode<_Type,_Base> * GetTail () const								{return Tail;}

	// PURPOSE: Get the address from the Tail pointer 
	// RETURN :	the address from the tail pointer
	atDNode<_Type,_Base> * GetTail ()											{return Tail;}

	/////////////////////////////// O(1) time manipulators //////////////////////

	// PURPOSE: prepend 'node' to the front of the list
	// PARAMS :	node - the node to be prepend
	void Prepend (atDNode<_Type,_Base> & node);		

	// PURPOSE: append 'node' to the back of the list
	// PARAMS :	node - the node to append
	void Append (atDNode<_Type,_Base> & node);

	// PURPOSE: prepend 'list' to the front of the list
	// PARAMS :	other - the list to be added
	void Prepend (atDList<_Type,_Base> & other);

	// PURPOSE: append 'list' to the back of the list
	// PARAMS :	other - the list to be added
	void Append (atDList<_Type,_Base> & other);

	// PURPOSE: insert 'node' after 'insertPoint'
	// PARAMS :	insertPoint - node to give location in list where to insert the new node
	//			node - the node to insert
	void InsertAfter (atDNode<_Type,_Base> & insertPoint, atDNode<_Type,_Base> & node);

	// PURPOSE: insert 'node' before 'insertPoint'
	// PARAMS :	insertPoint - node to give location in list where to insert the new node
	//			node - the node to insert
	void InsertBefore (atDNode<_Type,_Base> & insertPoint, atDNode<_Type,_Base> & node);

	// PURPOSE: remove and return the head of the list
	// RETURN :	node pointer to the head
	atDNode<_Type,_Base> * PopHead ();										

	// PURPOSE: remove and return the tail of the list
	// RETURN :	node pointer to the tail
	atDNode<_Type,_Base> * PopTail ();			

	// PURPOSE: remove 'node' from the list ('node' must be in this list)
	// PARAMS :	node - the node to remove from the list
	void PopNode (atDNode<_Type,_Base> & node);								

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

private:
	// beginning of the list
	atDNode<_Type,_Base> * Head;
	
	// end of the list
	atDNode<_Type,_Base> * Tail;
};


template <class _Type,class _Base> atDList<_Type,_Base>::atDList(datResource &rsc)
{
	rsc.PointerFixup(Head);
	rsc.PointerFixup(Tail);

	// Can't use placement new here because 'new' may be #defined
	// and we can't do '#undef new' either.
	atDNode<_Type,_Base> *node;
	for(node = Head; node; node = node->GetNext())
		node->PointerFixup(rsc);
}

template <class _Type,class _Base> atDList<_Type,_Base>::atDList(datResource &rsc, bool ASSERT_ONLY(flag))
{
	FastAssert(flag);

	rsc.PointerFixup(Head);
	rsc.PointerFixup(Tail);

	atDNode<_Type,_Base> *node;
	for(node = Head; node; node = node->GetNext())
	{
		node->PointerFixup(rsc);
		node->Data.Place(&node->Data,rsc);
	}
}

template <class _Type,class _Base> void atDList<_Type,_Base>::DeleteAll ()
{
	while (Tail)
	{
		atDNode<_Type,_Base> * node = PopTail();
		delete node;
	}
}

template <class _Type,class _Base> void atDList<_Type,_Base>::RemoveAll ()
{
	for (atDNode<_Type,_Base> *pNext, *pNode = GetHead(); pNode; pNode = pNext)
	{
		pNext = pNode->GetNext();
		pNode->SetNext(NULL);
		pNode->SetPrev(NULL);
	}

	Empty();
}

template <class _Type,class _Base> void atDList<_Type,_Base>::Prepend (atDList<_Type,_Base> & other)
{
	if (!other.Head)
		return;						//Nothing to do

	if (Head)
	{
		other.Tail->SetNext(Head);	//Link other's tail to head
		Head->SetPrev(other.Tail);		
		Head = other.Head;			//Set new tail pointer
	}
	else
	{
		*this = other;				//Copy
	}

	other.Empty();						//Reset other list
}

template <class _Type,class _Base> void atDList<_Type,_Base>::Append (atDList<_Type,_Base> & other)
{
	if (!other.Head)
		return;			//Nothing to do

	if (Head)
	{
		other.Head->SetPrev(Tail);	//Link other's tail to head
		Tail->SetNext(other.Head);		
		Tail = other.Tail;			//Set new tail pointer
	}
	else
	{
		*this = other;				//Copy
	}

	other.Empty();						//Reset other list
}

template <class _Type,class _Base> void atDList<_Type,_Base>::Prepend (atDNode<_Type,_Base> & node)
{
	FastAssert(node.GetNext()==NULL && node.GetPrev()==NULL);

	if (Head)
	{
		Head->Insert(node);
		Head = &node;
	}
	else
	{
		Head = &node;
		Tail = &node;
	}
}


template <class _Type,class _Base> void atDList<_Type,_Base>::Append (atDNode<_Type,_Base> & node)
{
	FastAssert(node.GetNext()==NULL && node.GetPrev()==NULL);

	if (Tail)
	{
		Tail->Append(node);
		Tail = &node;
	}
	else
	{
		Head = &node;
		Tail = &node;
	}
}


template <class _Type,class _Base> void atDList<_Type,_Base>::InsertAfter (atDNode<_Type,_Base> & insertPoint, atDNode<_Type,_Base> & node)
{
	FastAssert(node.GetNext()==NULL && node.GetPrev()==NULL);

	insertPoint.Append(node);

	if (&insertPoint == Tail)
	{
		Tail = &node;
	}
}


template <class _Type,class _Base> void atDList<_Type,_Base>::InsertBefore (atDNode<_Type,_Base> & insertPoint, atDNode<_Type,_Base> & node)
{
	FastAssert(node.GetNext()==NULL && node.GetPrev()==NULL);

	insertPoint.Insert(node);

	if (&insertPoint == Head)
	{
		Head = &node;
	}
}


template <class _Type,class _Base> atDNode<_Type,_Base> * atDList<_Type,_Base>::PopHead ()
{
	atDNode<_Type,_Base> * oldHead = Head;

	if (Head)
	{
		Head = Head->GetNext();

		if (Head==NULL)
		{
			Tail = NULL;
		}

		oldHead->Detach();
	}

	return oldHead;
}


template <class _Type,class _Base> atDNode<_Type,_Base> * atDList<_Type,_Base>::PopTail ()
{
	atDNode<_Type,_Base> * oldTail = Tail;

	if (Tail)
	{
		Tail = Tail->GetPrev();

		if (Tail==NULL)
		{
			Head = NULL;
		}

		oldTail->Detach();

	}

	return oldTail;
}


template <class _Type,class _Base> void atDList<_Type,_Base>::PopNode (atDNode<_Type,_Base> & node)
{
	if (&node==Head)
	{
		Head = node.GetNext();
	}
	if (&node==Tail)
	{
		Tail = node.GetPrev();
	}
	node.Detach();
}


#if __DECLARESTRUCT

template <class _Type,class _Base> void atDList<_Type,_Base>::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(atDList);
	STRUCT_FIELD_VP(Head);
	STRUCT_FIELD_VP(Tail);
	STRUCT_END();
}

#endif //!__FINAL


}	// namespace rage


#endif // ndef ATL_DLIST_H
