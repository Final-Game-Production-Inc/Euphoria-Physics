// 
// atl/testatl.cpp
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "atl/slist.h"
#include "atl/dlist.h"
#include "diag/output.h"
#include "system/main.h"

using namespace rage;


////////////////////////////////////////////////////////////////

int Main()
{
	int i;

	const int nElements = 10;

#if 0
	int elements[nElements];
	//bool found[nElements];

	for (i=0; i<nElements; i++)
	{
		elements[i] = i;
	}
#endif

	////////////////////////////////////////////////////////////
	// test atSList

	atSList<int> slistA;
	atSList<int> slistB;
	atSNode<int> snodes[nElements];
	atSNode<int> * snode;

	// insert into slistA
	for (i=0; i<nElements; i++)
	{
		snodes[i].Data = i;
		slistA.Prepend(snodes[i]);
	}

	// output slistA
	for (snode=slistA.GetHead(); snode; snode=snode->GetNext())
	{
		Displayf("A: snode %p Data %d",snode,snode->Data);
	}

	// move every one to slistB
	while ((snode=slistA.PopHead())!=NULL)
	{
		slistB.Append(*snode);
	}

	// output slistB
	for (snode=slistB.GetHead(); snode; snode=snode->GetNext())
	{
		Displayf("B: snode %p Data %d",snode,snode->Data);
	}

	/*
	void InsertAfter (atSNode<T> & insertPoint, atSNode<T> & node);	// insert <node> after <insertPoint>
	*/

	
	////////////////////////////////////////////////////////////
	// test atDList

	atDList<int> dlistA;
	atDList<int> dlistB;
	atDNode<int> dnodes[nElements];
	atDNode<int> * dnode;

	// insert into dlistA
	for (i=0; i<nElements; i++)
	{
		dnodes[i].Data = i;
		dlistA.Prepend(dnodes[i]);
	}

	// output dlistA
	for (dnode=dlistA.GetHead(); dnode; dnode=dnode->GetNext())
	{
		Displayf("C: dnode %p Data %d",dnode,dnode->Data);
	}

	// move every one to dlistB
	while ((dnode=dlistA.PopHead())!=NULL)
	{
		dlistB.Append(*dnode);
	}

	// output dlistB
	for (dnode=dlistB.GetHead(); dnode; dnode=dnode->GetNext())
	{
		Displayf("D: dnode %p Data %d",dnode,dnode->Data);
	}

	// move every one to dlistA
	while ((dnode=dlistB.PopTail())!=NULL)
	{
		dlistA.Append(*dnode);
	}

	// output dlistA
	for (dnode=dlistA.GetHead(); dnode; dnode=dnode->GetNext())
	{
		Displayf("E: dnode %p Data %d",dnode,dnode->Data);
	}

	return 0;
}
