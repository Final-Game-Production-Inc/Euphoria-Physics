//
// atl/ilist.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_ILIST_H
#define ATL_ILIST_H

////////////////////////////////////////////////////////////////
// external defines


namespace rage {


////////////////////////////////////////////////////////////////
// atINodeBase
//

// PURPOSE: This base class exists so the INVALID enum can be defined before the
// template is introduced, so it can be scoped by external code. (i.e. atINodeBase::INVALID)
//
class atINodeBase
{
public:
	// invalid index, similar to NULL for pointers
	enum {INVALID=0xFFFF};						
	
protected:
	// index of the element following this
	u16 Next;														
	// index of the element preceeding this
	u16 Prev;														
};


////////////////////////////////////////////////////////////////
// atINode

// 
// These functions are similar to those in atLNode, but sometimes
// require a pointer to the base array so the indices can be 
// resolved into pointers.
//

//
// An atINode is an object that can be used
// in a doubly linked list that is "indexed".
// This means that the standard "next" and 
// "prev" pointers in a linked list are replaced
// with next/prev indices that index into the
// base array that holds all nodes that might
// be in the list.  The only reason for using
// an atINode over an atLNode is to save the
// 4 bytes per object of u16 indices versus
// pointers.
//
template <class _Type> class atINode : public atINodeBase
{
public:
	
	// PURPOSE:	Constructor
	atINode<_Type> ()													{Next=atINodeBase::INVALID; Prev=atINodeBase::INVALID;}

	////// accessors /////////

	// PURPOSE:	return the next index
	// RETURN : the next index
	u16 GetNextIndex ()												{return Next;}

	// PURPOSE:	return the previous index
	// RETURN : the previous index
	u16 GetPrevIndex ()												{return Prev;}

	// PURPOSE:	get the index by node
	// PARAMS :	master - the array that hold the index 
	// RETURN :	the index based on the node input
	u16 GetIndex (atINode<_Type> * master)								{return u16(this-master);}

	// PURPOSE:	get the next node base on the given node
	// PARAMS :	master - the array that hold the index 
	// RETURN :	the pointer to the next node, if next is an Invalid node
	//			NULL will be returned
	atINode<_Type> * GetNext (atINode<_Type> * master)						{return (Next!=atINodeBase::INVALID)?master+Next:NULL;}
	
	// PURPOSE:	get the previous node base on the given node
	// PARAMS :	master - the array that hold the index 
	// RETURN :	the pointer to the previous node, if prev is an Invalid node
	//			NULL will be returned
	atINode<_Type> * GetPrev (atINode<_Type> * master)						{return (Prev!=atINodeBase::INVALID)?master+Prev:NULL;}

	// PURPOSE:	get the next node base on the given node in constant mode
	// PARAMS :	master - the array that hold the index 
	// RETURN :	the constant pointer to the next node, if next is an Invalid node
	//			NULL will be returned
	const atINode<_Type> * GetNext (atINode<_Type> * master) const			{return (Next!=atINodeBase::INVALID)?master+Next:NULL;}

	// PURPOSE:	get the previous node base on the given node in constant mode
	// PARAMS :	master - the array that hold the index 
	// RETURN :	the constant pointer to the previous node, if prev is an Invalid node
	//			NULL will be returned
	const atINode<_Type> * GetPrev (atINode<_Type> * master) const			{return (Prev!=atINodeBase::INVALID)?master+Prev:NULL;}

	// PURPOSE:	append 'node' after 'this', both from 'master' array
	// PARAMS :	node - the node to insert
	//			master - the array that hold the index
	inline void Append (atINode<_Type> * node, atINode<_Type> * master);	

	// PURPOSE:	insert 'node' before 'this', both from 'master' array
	// PARAMS :	node - the node to insert
	//			master - the array that hold the index
	inline void Insert (atINode<_Type> * node, atINode<_Type> * master);	

	// PURPOSE:	remove 'this' from its list, this from 'master' array
	// PARAMS : master - the array that hold the index
	inline void Detach (atINode<_Type> * master);						

	// PURPOSE:	Sort a doubly-linked indexed list by the index numbers.
	// PARAMS : head - pointer to the first node in the list
	//			master - the array that hold the index
	// RETURN : This returns the new list head, so it should be used in the form head=SortList(head,master);
	// NOTE	  : from http://www.chiark.greenend.org.uk/~sgtatham/algorithms/listsort.html
	atINode<_Type> * SortList (atINode<_Type> * head, atINode<_Type> * master);

	// PURPOSE:	Check 'this' is in the list
	// RETURN : return true if both prev and next is not INVALID, else false
	bool IsInList ()												{return (Next==atINodeBase::INVALID && Prev==atINodeBase::INVALID) ? false : true;}

	//////////////////// debug functions //////////////////////////////
	#if __DEV

	// PURPOSE:	traverse list and validate data structs
	// PARAMS :	master - the array that hold the index
	// RETURN : the valid node count
	int ValidateList (const atINode<_Type> * master);					
	#endif

public:
	// the actual data for the class
	_Type Data;
};

// PURPOSE: this template contains head and tail indices for the linked list comprised of atInNode instances.
//<FLAG Component>
template<class _Type> class atIList
{
public:
	atIList<_Type>()
	{	Head = Tail = INVALID;	}

	// Accessors.
	const atINode<_Type> *GetHead(atINode<_Type> *master) const
	{	return (Head == INVALID) ? NULL : master + Head;	}
	atINode<_Type> *GetHead(atINode<_Type> *master)
	{	return (Head == INVALID) ? NULL : master + Head;	}
	const atINode<_Type> *GetTail(atINode<_Type> *master) const
	{	return (Tail == INVALID) ? NULL : master + Tail;	}
	atINode<_Type> *GetTail(atINode<_Type> *master)
	{	return (Tail == INVALID) ? NULL : master + Tail;	}

	// O(1) time manipulators

	// Prepend 'node' to the front of the list.
	void Prepend(atINode<_Type> &node, atINode<_Type> *master);

	// Append 'node' to the back of the list.
	void Append(atINode<_Type> &node, atINode<_Type> *master);

	// Insert 'node' after <insertPoint>.
	void InsertAfter(atINode<_Type> &insertPoint, atINode<_Type> &node,
		atINode<_Type> *master);

	// Insert 'node' before <insertPoint>.
	void InsertBefore(atINode<_Type> &insertPoint, atINode<_Type> &node,
		atINode<_Type> *master);

	// Remove the head of the list.
	atINode<_Type> *PopHead(atINode<_Type> *master);

	// Remove the tail of the list.
	atINode<_Type> *PopTail(atINode<_Type> *master);

	// Remove 'node' from the list ('node' must be in this list).
	void PopNode(atINode<_Type> &node, atINode<_Type> *master);

private:
	enum
	{	INVALID = atINodeBase::INVALID	};

	u16 Head;														// beginning of the list
	u16 Tail;														// end of the list
};


template <class _Type> void atINode<_Type>::Append (atINode<_Type> * node, atINode<_Type> * master)
{
	node->Next = Next;
	node->Prev = (u16)(this - master);
	if (Next != atINodeBase::INVALID)
	{
		master[Next].Prev = (u16)(node - master);
	}
	Next = (u16)(node - master);
}


template <class _Type> void atINode<_Type>::Insert (atINode<_Type> * node, atINode<_Type> * master)
{
	node->Next = (u16)(this - master);
	node->Prev = Prev;
	if (Prev != atINodeBase::INVALID)
	{
		master[Prev].Next = (u16)(node - master);
	}
	Prev = (u16)(node - master);
}


template <class _Type> void atINode<_Type>::Detach (atINode<_Type> * master)
{
	if (Next != atINodeBase::INVALID)
	{
		master[Next].Prev = Prev;
	}
	if (Prev != atINodeBase::INVALID)
	{
		master[Prev].Next = Next;
	}
	Next = atINodeBase::INVALID;
	Prev = atINodeBase::INVALID;
}


// Sort a doubly-linked indexed list by the index numbers.
// from http://www.chiark.greenend.org.uk/~sgtatham/algorithms/listsort.html
// This returns the new list head, so it should be used in the form head=SortList(head,master);
template <class _Type> atINode<_Type> * atINode<_Type>::SortList (atINode<_Type> * head, atINode<_Type> * master)
{
	atINode<_Type> *p, *q, *e, *tail;
	int inSize,numMerges,pSize,qSize,index;
	inSize=1;
	while(1)
	{
		p=head;
		head=NULL;
		tail=NULL;
		numMerges=0;		// the number of merges done in this pass

		while(p)
		{
			numMerges++;	// another merge will be done
			q=p;
			pSize=0;
			for(index=0;index<inSize;index++)
			{
				pSize++;
				q=q->GetNext(master);
				if(!q)
				{
					break;
				}
			}
			qSize=inSize;
			while(pSize>0 || (qSize>0 && q))
			{
	            // decide whether next element of merge comes from p or q
                if(pSize==0)
				{
				    // p is empty; e must come from q
				    e=q;
					q=q->GetNext(master);
					qSize--;
				}
				else if(qSize==0 || !q)
				{
				    // q is empty; e must come from p
				    e=p;
					p=p->GetNext(master);
					pSize--;
				}
				else if (p->GetIndex(master)<=q->GetIndex(master))
				{
				    // p is lower (or same); e must come from p
				    e=p;
					p=p->GetNext(master);
					pSize--;
				}
				else
				{
				    // q is lower; e must come from q.
				    e=q;
					q=q->GetNext(master);
					qSize--;
				}

                // add the next element to the merged list
				if(tail)
				{
					tail->Next=e->GetIndex(master);
					// maintain the previous index numbers
					e->Prev=tail->GetIndex(master);
				}
				else
				{
					e->Prev=atINodeBase::INVALID;
					e->Next=atINodeBase::INVALID;
				    head=e;
				}
				// make e the tail
				tail=e;
			}
            // now p has stepped inSize places along, and q has too
            p=q;
		}
		if(tail)
		{
		    tail->Next=atINodeBase::INVALID;
		}

        // if there was only one merge on this round then the list is sorted
        if(numMerges<=1)
		{
            return head;
		}

        // otherwise repeat, merging lists twice the size
        inSize *= 2;
	}
}


#if __DEV
template <class _Type> int atINode<_Type>::ValidateList (const atINode<_Type> * master)
{
	FastAssert(master!=NULL);

	int count;
	const atINode<_Type> * index;
	for (index=this, count=0; index!=NULL; )
	{
		count++;
		FastAssert (index->Next != (index-master));
		FastAssert (index->Prev != (index-master));
		if (index->Prev != atINodeBase::INVALID)
		{
			FastAssert (master[index->Prev].Next == (index-master));
		}
		if (index->Next != atINodeBase::INVALID)
		{
			FastAssert (master[index->Next].Prev == (index-master));
			index = master + index->Next;
		}
		else
		{
			index = NULL;
		}
	}

	return count;
}
#endif // __DEV



template<class _Type> void atIList<_Type>::Prepend(atINode<_Type> &node,
										   atINode<_Type> *master)
{
	FastAssert(node.GetNext(master) == NULL && node.GetPrev(master) == NULL);

	u16 nodeIndx = (u16)(&node - master);
	if(Head != INVALID)
	{	GetHead(master)->Insert(&node, master);
	Head = nodeIndx;
	}
	else
	{	Head = nodeIndx;
	Tail = nodeIndx;
	}
}


template<class _Type> void atIList<_Type>::Append(atINode<_Type> &node,
										  atINode<_Type> *master)
{
	FastAssert(node.GetNext(master) == NULL && node.GetPrev(master) == NULL);

	u16 nodeIndx = (u16)(&node - master);
	if(Tail != INVALID)
	{	GetTail(master)->Append(&node, master);
	Tail = nodeIndx;
	}
	else
	{	Head = nodeIndx;
	Tail = nodeIndx;
	}
}


template<class _Type> void atIList<_Type>::InsertAfter(atINode<_Type> &insertPoint,
											   atINode<_Type> &node, atINode<_Type> *master)
{
	FastAssert(node.GetNext()==NULL && node.GetPrev()==NULL);

	insertPoint.Append(node, master);

	u16 insertIndx = (u16)(&insertPoint - master);
	if(insertIndx == Tail)
		Tail = (u16)(&node - master);
}


template<class _Type> void atIList<_Type>::InsertBefore(atINode<_Type> &insertPoint,
												atINode<_Type> &node, atINode<_Type> *master)
{
	FastAssert(node.GetNext(master)==NULL && node.GetPrev(master)==NULL);

	insertPoint.Insert(node, master);

	u16 insertIndx = (u16)(&insertPoint - master);
	if(insertIndx == Head)
		Head = (u16)(&node - master);
}


template<class _Type> atINode<_Type> *atIList<_Type>::PopHead(atINode<_Type> *master)
{
	atINode<_Type> *oldHead = GetHead(master);

	if(Head != INVALID)
	{
		Head = oldHead->GetNextIndex();
		if(Head == INVALID)
			Tail = INVALID;
		oldHead->Detach(master);
	}

	return oldHead;
}


template<class _Type> atINode<_Type> *atIList<_Type>::PopTail(atINode<_Type> *master)
{
	atINode<_Type> *oldTail = GetTail(master);

	if(Tail != INVALID)
	{
		Tail = oldTail->GetPrevIndex();

		if(Tail == INVALID)
			Head = INVALID;

		oldTail->Detach(master);

	}

	return oldTail;
}


template<class _Type> void atIList<_Type>::PopNode(atINode<_Type> &node,
										   atINode<_Type> *master)
{
	u16 nodeIndex = (u16)(&node - master);
	if(nodeIndex == Head)
		Head = node.GetNextIndex();
	if(nodeIndex == Tail)
		Tail = node.GetPrevIndex();
	node.Detach(master);
}

}	// namespace rage

#endif // ndef ATL_ILIST_H
