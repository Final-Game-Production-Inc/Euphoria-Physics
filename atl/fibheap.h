//
// atl/fibheap.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_FIBHEAP_H
#define ATL_FIBHEAP_H

#include "math/amath.h"
#include "system/new.h"

namespace rage {

/*
PURPOSE:
	atFibHeap implements a Fibonacci heap -- a <B>very</B> fast priority queue structure.
	Fibonacci heaps were created specifically to optimize the speed of search algorithms,
	such as Dijkstra's single-source-shortest-path algorithm, or A* pathfinding.

	The primary motivation for Fibonacci heaps is speed.  When evaluated using amortized 
	analysis (which considers the aggregate cost over multiple calls to a function),
	they can be seen to perform considerably faster than a binary heap implementation.

	<TABLE>					     
	Operation			 Binary heap (worst-case)    Fibonacci heap (amortized)
	----------------     -------------------------   --------------------------
	<c>MakeHeap</c>      Theta(1)                    Theta(1)
	<c>Insert</c>        Theta(log n)                Theta(1)
	<c>FindMin</c>       Theta(1)                    Theta(1)
	<c>ExtractMin</c>    Theta(log n)                Theta(log n)
	<c>Union</c>         Theta(n)                    Theta(1)
	<c>DecreaseKey</c>   Theta(log n)                Theta(1)
	<c>Delete</c>        Theta(log n)                Theta(log n)
	</TABLE>

	However!  A binary heap has less constant-time overhead.
	For many applications, binary heaps will be faster.  Whee.

	The best available reference for more info on Fibonacci heaps is Chapter 21 of
	<i>Introduction to Algorithms</i> by Cormen, Lieserson, Rivest.

PARAMETERS:
	_Key - the type of the value that we're looking at to maintain the heap property of the <c>atFibHeap::Node</c>
	_Data - the type of data that a <c>atFibHeap::Node</c> will hold
NOTE:
	
	This code does dynamic, run-time allocation of memory.  As such, it is not 
	intended for use in game code.  Mostly, it's here to pave the way for a 
	spatially tighter version that does NOT do run-time allocation.  Of course, 
	it might be saner to just use a binary heap instead.

	Again, this should be used ONLY for development, not in game code!		
<FLAG Component>
*/
template <class _Key, class _Data>
class atFibHeap 
{
public:
	class Node 
	{
	public:
		Node(_Key key, _Data data) : Parent(0), Child(0), Degree(0), Mark(0), Key(key), Data(data) {Left=this; Right=this;}
	public:
		Node*	Left;		// My left neighbor in circular list
		Node*	Right;		// My right neighbor in circular list
		Node*	Parent;		// My parent (node above me)
		Node*	Child;		// My first child (any node below me)
		u16		Degree;		// How many direct children I have.
		u16		Mark;		// A boolean value for cascading cuts.  (nonzero means keep cascading; 0 means stop here)
		_Key		Key;		// The value we're looking at to maintain the heap property
		_Data	Data;		// The data held by this node -- (e.g., for Dijkstra, a pointer back to a vertex object)
	};

	// PURPOSE: Default constructor
	atFibHeap() : Min(0), NodeCount(0) {}

	// PURPOSE: Destructor
	~atFibHeap() 
	{
		Kill();
	}

	// PURPOSE: Free all storage associated with the atFibHeap object
	void Kill() 
	{
		// not exactly speedy
		_Key key;
		_Data data;
		while (ExtractMin(key, data));
		FastAssert(!Min && NodeCount==0);
	}

	// PURPOSE:	Insert a new node (with key and data) into the heap. 
	// PARAMS:	key - value of the key to insert
	// RETURNS: the array index of the node we just added
	Node* Insert(_Key key, _Data data) 
	{
		Node* newNode = rage_new Node(key, data);
		FastAssert(newNode);

		if (Min)
		{
			// concatenate into root list
			ConcatenateLists(Min, newNode);
			
			// update min
			if (key < Min->Key)
				Min=newNode;
		}
		else
		{
			// create min
			Min = newNode;
		}
		NodeCount++;
		return newNode;
	}

	// PURPOSE:	Finds the node of minimum value 
	// RETURNS:	Pointer to the node of minimum value, or NULL if empty
	Node* FindMin() 
	{
		return Min;
	}

	// PURPOSE:	Informs interested parties of how many nodes we contain 
	// RETURNS:	NodeCount
	unsigned int GetNodeCount()
	{
		return NodeCount;
	}

	// PURPOSE:	Merges another heap into this one.
	// RETURNS:	Pointer to associated data node, or NULL if not found
	// NOTES: The other heap will be emptied
	void Union(atFibHeap& otherHeap) 
	{	
		// If otherHeap is empty, skip this stuff.
		if (otherHeap.Min)
		{
			if (Min)
			{
				// Concatenate the other heap's root list with ours.
				Node& otherMin = otherHeap.Min;
				ConcatenateLists(Min, otherMin);

				// Update Min.
				if (otherMin.Key < Min->Key)
					Min = otherMin;
			}
			else
			{
				// If we have no min, steal it from the other heap
				Min = otherHeap.Min;
			}
			NodeCount += otherHeap.NodeCount;
		}

		// Let the other heap know that it is now empty.
		otherHeap.Min = NULL;
		otherHeap.NodeCount = 0;
		return;
	}

	// PURPOSE:	Removes node of minimum value from heap, and returns its information. 
	//			The node itself is deallocated.
	// RETURNS:	Pointer to associated data node, or NULL if not found
	bool ExtractMin(_Key& key, _Data& data) 
	{
		if (!Min)
			return false;

		// Move the children of Min to the root list
		Node* child = Min->Child;
		while (child)
			child = AddToRootList(child);	// remember, this returns a sibling

		FastAssert(Min->Child==NULL);			// Make sure we got all them pesky kids 

		Node* removedMin = Min;
		Min = RemoveNodeAndGetSibling(removedMin);	
		NodeCount--;
		if (Min)
			Consolidate();

		FastAssert(Min || NodeCount==0);
		key = removedMin->Key;
		data = removedMin->Data;
		
		delete removedMin;
		return true;
	}

	// PURPOSE:  Assigns a new, smaller key value to a node; 
	// PARAMS:  node: the node whose key we're decreasing.
	//			newKey: the value to decrease it to.
	// NOTE:  MUST BE A DECREASE!  We can't use this to increase a key's value
	void DecreaseKey(Node* node, _Key newKey) 
	{
		if(newKey > node->Key)
		{
			Errorf("atFibHeap::DecreaseKey -- attempted to INCREASE key!\n");
			return;
		}

		node->Key = newKey;
		Node* parent = node->Parent;
		if (parent && node->Key < parent->Key)
		{
			Cut(node, parent);
			CascadingCut(parent);
		}
		if (node->Key < Min->Key)
			Min = node;
	}
			
	// PURPOSE:  Delete an arbitrary node from the heap. 
	// PARAMS:	node: the node to delete
	void Delete(Node* node) 
	{
		DecreaseKey(node, (Min->Key - Min->Key));
		ExtractMin();
	}


private:

	// PURPOSE: join two lists of siblings together
	void ConcatenateLists(Node* a, Node* b)
	{
		Node* aEnd		=a->Left;
		Node* bEnd		=b->Left;

		a->Left			=bEnd;
		aEnd->Right		=b;

		b->Left			=aEnd;
		bEnd->Right		=a;
	}

	// PURPOSE: Add a node or list to the root list
	// RETURNS: The sibling, if any, of the node we just moved up to the root.
	Node* AddToRootList(Node* node)
	{
		Node* sibling = RemoveNodeAndGetSibling(node);
		if (Min)
			ConcatenateLists(Min, node);
		else
			Min = node;
		return sibling;
	}

	// PURPOSE: Remove a node from its circular list, and make it a singular list.
	// RETURNS: The sibling of the removed node -- or NULL if it already was a singular list.
	Node* RemoveNodeAndGetSibling(Node* node)
	{
		// Keep track of a sibling -- if I have one.
		Node* sibling = (node==node->Right) ? NULL : node->Right;

		// Fix up my parent's child pointer so it points to my sibling
		if (node->Parent)
		{
			FastAssert(node->Parent->Degree>0);
			if (node->Parent->Child == node)
				node->Parent->Child = sibling;
		}
		
		// Splice my neighboring siblings together 
		node->Right->Left = node->Left;
		node->Left->Right = node->Right;

		// Reset me to being a singular list
		node->Parent	=NULL;
		node->Left		=node;
		node->Right		=node;
		
		return sibling;
	}

	// PURPOSE:	Performs consolidation on the heap after its Min has been extracted. 
	void Consolidate() 
	{
		// The array cleverly named "Array" is used to ensure that there's only one root-level
		// node with degree N (again, degree == number of immediate children) for any N.
		
		// The upper bound on the maximum degree of a fibonacci heap node is log-base-phi
		// of NodeCount, where phi is the golden mean:  (1+sqrt(5))/2
		
		// Said another way: if we use some arbitrary size n, the maximum NodeCount we can 
		// handle is phi to the nth power.  For instance, phi^32 = 4870846.  
		// Do you suppose a maximum of 4.8 million nodes is enough?  :)

		FastAssert(NodeCount < 4870846);
		Node* Array[32];

		//memset(Array, 0, 32 * sizeof(Node*));
		Node** stopHere = &(Array[31]);	
		for (Node** zeroMe=Array; zeroMe<=stopHere; zeroMe++)
			*zeroMe = NULL;
			
		Node* x	= Min;
		Node* nextX;
		int degree = 0;

		// For each node in the heap's root list
		do 
		{
			degree	=x->Degree;
			nextX = RemoveNodeAndGetSibling(x);

			while (Array[degree] != NULL)
			{
				Node* y = Array[degree];
				if (x->Key > y->Key)
				{
					// Swap x and y
					Node* temp =x;
					x = y;
					y = temp;
				}
				FastAssert(x->Key <= y->Key);
					
				// Make y a child of x
				Link(y, x);
				Array[degree] = NULL;
				degree++;
			}

			Array[degree] = x;
			x = nextX;
		} while (x);

		// Find the new min from among the elements of Array	
		Min = NULL;
		for (int i=0; i<32; i++)
		{
			if (Array[i])
			{
				// Add Array[i] to the root list
				AddToRootList(Array[i]);
				if (!Min || Array[i]->Key < Min->Key)
					Min = Array[i];
			}
		}
		// Make sure we still have a min
		FastAssert(NodeCount==0 || Min);
	}

	// PURPOSE: Swap the locations of two nodes.
	void Exchange(Node* x, Node* y) 
	{
		// Swap x and y left pointers
		Node* temp		=x->Left;
		x->Left			=y->Left;
		y->Left			=temp;
						
		// Swap x and y right pointers
		temp			=x->Right;
		x->Right		=y->Right;	
		y->Right		=temp;

		// Fix up the neighbor's circular links	(still works even if x or y is singular).
		x->Left->Right	=x;
		x->Right->Left	=x;
		y->Left->Right	=y;
		y->Right->Left	=y;
	}

	// PURPOSE: Remove a node from its current position and make it a child of another node.
	void Link(Node* newChild, Node* newParent) 
	{
		// Remove child from its current position
		RemoveNodeAndGetSibling(newChild);
		
		// Also clear its "mark" value, for DecreaseKey.
		newChild->Mark = 0;
		
		// Increment the parent's degree and add the new child
		newParent->Degree++;
		if (newParent->Child)
			ConcatenateLists(newParent->Child, newChild);
		else
			newParent->Child = newChild;

		// Set the parent pointer of the child
		newChild->Parent = newParent;
	}

	// PURPOSE: Cut out a node and add it to its parent list.
	void Cut(Node* child, Node* parent)
	{
		AddToRootList(child);
		parent->Degree--;
		child->Mark = 0;
	}

	void CascadingCut(Node* node)
	{
		Node* parent = node->Parent;			
		if (!parent)
			return;
		
		if (node->Mark)
		{
			Cut(node, parent);
			CascadingCut(parent);
		}
		else
		{	
			node->Mark = 1;
		}
	}


private:
	Node*			Min;		// Pointer to the node with minimum key
	unsigned int	NodeCount;	// Count of the number of nodes in this heap.
};

}	// namespace rage

#endif
