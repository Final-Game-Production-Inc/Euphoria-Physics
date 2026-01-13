// 
// atl/binheap.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_BINHEAP_H
#define ATL_BINHEAP_H

#include "math/amath.h"
#include "system/new.h"

namespace rage {


// Set this to 1 to verify heap property after every operation.  
// This makes everything VERY SLOW -- all operations become O(n).
#define __QUALITY_ASSURANCE 0	

// Set this (and the above flag) to 1 to verify total sum of all keys remains valid.
// Floating point error will accumulate, so you can expect to see some small
// discrepency, especially in huge heaps with a wide range of values.
// Failing the checksum does not cause us to abort;  it just prints warnings.
#define __CHECKSUM 0	


//  PURPOSE: atBinHeap implements a binary heap -- a fast priority queue structure.  
//  The standard priority queue operations are implemented in Theta(log(n)) time (or better).
//
//  <TABLE>
//   Function              Time order      Functionality
//   -------------------   -------------   --------------------------------------
//   <c>FindMin</c>        O(1)            Return the minimum value in the queue.
//   <c>Insert</c>         O(log(n))       Add an element to the queue (and set up a pointer to it)
//   <c>ExtractMin</c>     O(log(n))       Remove the minimum element from the queue.
//   <c>DecreaseKey</c>    O(log(n))       Decrease the key value of a node in the queue.
//   <c>Delete</c>         O(log(n))       Remove an arbitrary node from the queue.
//  </TABLE>
//
//  In addition, Find (by Data) is implemented, but in O(n) time.  
//  Heaps are not intended to be searchable; the addition of Find was made 
//  for the sake of enhanced functionality, rather than efficiency.
// 
//	Union is not implemented.
//<FLAG Component>
template <class _Key, class _Data>
class atBinHeap 
{
public:
	
	class Node
	{
	public:
		_Key	Key;	// The value on which we maintain the heap property 
		_Data	Data;	// Other associated data.
		Node**	Handle;	// Our pointer to the client's pointer to us, so the client can find this node as it moves around.
	};

	// PURPOSE: Default constructor
	explicit atBinHeap(int maxSize) : MaxSize(maxSize), Size(0)
	{
		Nodes = rage_new Node[maxSize+1]; 
#if __QUALITY_ASSURANCE && __CHECKSUM
		CheckSum=0.0f;
#endif
	}

	// copy constructor
	atBinHeap(const atBinHeap &original)
	{
		MaxSize = original.MaxSize;
		Size = original.Size;
		Nodes = rage_new Node[original.MaxSize+1];
#if __QUALITY_ASSURANCE && __CHECKSUM
		CheckSum=0.0f;
#endif

		for (unsigned int n=0;n<Size;n++)
		{
			FastAssert((original.Nodes[n].Handle == &original.NodeDump) && "Node **Handle is evil so don't use it");
			Nodes[n]=original.Nodes[n];
		}
	}

	// PURPOSE: Destructor
	~atBinHeap() 
	{
		Kill();
	}

	// PURPOSE: Free all storage associated with the atBinHeap object
	void Kill() 
	{
		delete[] Nodes;
	}

	// PURPOSE: set the heap to be empty
	void Empty() 
	{
		Size=0;

#if __QUALITY_ASSURANCE && __CHECKSUM
		CheckSum=0.0f;
#endif
	}
	
	// PURPOSE:	Insert a new node (with key and data) into the heap. 
	// PARAMS:	key - Value of the key to insert
	//			data - User-defined data associated with the key
	//			node - VERY IMPORTANT!!!  You have two options:  
	//
	//			1)  IFF you do not need to keep a pointer to the Node you just
	//				inserted, simply pass in NULL for the "node" paramaeter.
	//
	//				[Note that NULL is the default value for that paramaeter.]
	//
	//			2)  IFF you want to keep a pointer to the Node you just inserted,
	//				pass in the address of a Node*.  This Node* MUST be allocated 
	//				by the caller, and MUST stay in scope for as long as this heap
	//				exists.  Here's why:
	//		
	//				The newly-inserted node is going to move around in the heap.
	//				If the client wants to keep a pointer to the Node, that
	//				pointer will have to be updated whenever the Node is 
	//				repositioned in the heap.
	//
	//				Therefore, the heap keeps a Node** Handle, which points back
	//				at the client's Node*.  Every time the Node moves, the heap
	//				uses the Node's Handle to inform the client of where it moved to.
	//				
	//				So, IF you pass in the /address/ of a client-allocated Node*
	//				that will ALWAYS remain in scope, the heap will ensure
	//				that you can always use that Node* to find the inserted Node.
	//
	//				IF THE PASSED-IN Node* GOES OUT OF SCOPE, THE HEAP
	//				WILL TRAMPLE ON ARBITRARY MEMORY.  Consider yourself warned!
	//				
						

	// RETURNS: pointer to the node we just added
	void Insert(_Key key, const _Data WIN32PC_ONLY(&) data, Node** node=NULL)	// WARNING:  read the explanation before passing in a non-NULL "node"!
	{
		Size++;
		FastAssert(Size<=MaxSize);
		Nodes[Size].Key			=key;
		Nodes[Size].Data		=data;
		Nodes[Size].Handle		=node ? node : &NodeDump;

		*(Nodes[Size].Handle)	=&(Nodes[Size]);
		unsigned int scan		=HeapifyUp(Size);
		*(Nodes[scan].Handle)	=&Nodes[scan];

#if __QUALITY_ASSURANCE
#if __CHECKSUM
		CheckSum+=key;
#endif
		VerifyHeap();
#endif
	}

	// PURPOSE:	Finds the node of minimum value 
	// RETURNS:	Pointer to the node of minimum value, or NULL if empty
	Node* FindMin() 
	{
		if (Size<=0) //lint !e775 non-negative quantity cannot be less than zero
			return NULL;

		return &Nodes[1];
	}

	// PURPOSE:	Finds the node of minimum value 
	// RETURNS:	Pointer to the node of minimum value, or NULL if empty
	const Node* FindMin() const
	{
		if (Size<=0) //lint !e775 non-negative quantity cannot be less than zero
			return NULL;

		return &Nodes[1];
	}

	// PURPOSE:	Informs interested parties of how many nodes we contain 
	// RETURNS:	NodeCount
	unsigned int GetNodeCount() const
	{
		return Size;
	}

	// PURPOSE:	Maximum number of nodes in the queue.  This is set by the constructor.
	// RETURNS:	MaxSize
	unsigned int GetMaxSize() const
	{
		return MaxSize;
	}

	// PURPOSE:	Merges another heap into this one.
	// RETURNS:	Pointer to associated data node, or NULL if not found
	// NOTES: The other heap will be emptied
	void Union(atBinHeap& /*otherHeap*/) 
	{
#if __QUALITY_ASSURANCE
		Quitf("atBinHeap: Union not implemented\n");
#endif
	}

	// PURPOSE:	Removes node of minimum value from heap, and returns its information. 
	//			The node itself is deallocated.
	// RETURNS:	Pointer to associated data node, or NULL if not found
	bool ExtractMin(_Key& key, _Data& data) 
	{
		if (Size<=0) //lint !e775 non-negative quantity cannot be less than zero
			return false;

		key		=Nodes[1].Key;
		data	=Nodes[1].Data;
		Swap(1, Size);
		Size--;
		HeapifyDown(1);
#if __QUALITY_ASSURANCE
#if __CHECKSUM
		CheckSum-=key;
#endif
		VerifyHeap();
#endif
		return true;
	}

	// PURPOSE:  Assigns a new, smaller key value to a node; 
	// PARAMS:  node: the node whose key we're decreasing.
	//			newKey: the value to decrease it to.
	// NOTE:  MUST BE A DECREASE!  We can't use this to increase a key's value
	void DecreaseKey(Node* node, _Key newKey) 
	{
		FastAssert(!(node->Key < newKey));
#if __QUALITY_ASSURANCE && __CHECKSUM
		CheckSum += (newKey - node->Key);
#endif
		node->Key = newKey;
		unsigned int index = (unsigned int)(node-Nodes);
		HeapifyUp(index);
#if __QUALITY_ASSURANCE
		VerifyHeap();
#endif
	}
			
	// PURPOSE:  Delete an arbitrary node from the heap. 
	// PARAMS:	node: the node to delete
	void Delete(Node* node) 
	{
		if (Size<=0) //lint !e775 non-negative quantity cannot be less than zero
			return;
		int which = (int) (node-Nodes);

#if __QUALITY_ASSURANCE && __CHECKSUM
		CheckSum -= node->Key;
#endif
		Swap(which, Size);
		Size--;
		HeapifyUp(which);
		HeapifyDown(which);
#if __QUALITY_ASSURANCE
		VerifyHeap();
#endif
	}

	// PURPOSE: Find a node by its Data
	// PARAMS:  reference to the _Data you're looking for.
	Node *FindNode(_Data& data)
	{
		for (unsigned i=1;i<=Size;i++)
			if (Nodes[i].Data == data)
				return &Nodes[i];

		return NULL;
	}
	// PURPOSE: Call a callback function for every node in the heap
	// PARAMS:  pFN is a function "bool ExampleFunc(_Data*data)"
	//			return false to stop iteration through nodes
	void ForAllNodes( bool(*pFN)(_Data*data) ) const
	{
		for (unsigned i=1;i<=Size;i++)
		{
			if(!pFN(&Nodes[i].Data))
				return;
		}
	}

private:
	unsigned int	LeftChild(unsigned int i) const		{return (i<<1);}
	unsigned int	RightChild(unsigned int i) const	{return (i<<1)+1;}
	unsigned int	Parent(unsigned int i) const		{return (i>>1);}

#if __QUALITY_ASSURANCE
#if __CHECKSUM
	_Key CheckSum;
#endif
	
	void VerifyHeap()
	{
		unsigned int i;

		for (i=2; i<=Size; i++)
		{
			FastAssert(Nodes[i].Key			>= Nodes[Parent(i)].Key);
		}

		for (i=1; i<=Size; i++)
		{
			FastAssert(	(*(Nodes[i].Handle)	== &(Nodes[i])) ||
					(Nodes[i].Handle == &NodeDump));
		}
		
#if __CHECKSUM
		if (Size > 0 && CheckSum)
		{
			_Key count = (_Key)(0.0f);
			for (i=1; i<=Size; i++)
				count += Nodes[i].Key;
			_Key diff = count - CheckSum;
			_Key ratio = diff / CheckSum;
			if(ratio > 0.01f || ratio < -0.01f)
				Warningf("Discrpenceny of %.4f (%.2f%%) in key total.  Heap size is %d.\n", diff, ratio*100, Size);
		}
#endif
	}
#endif

	// PURPOSE:  "Bubble down" changes to node i
	// PARAMS:	i: index of the node from which to "bubble down"
	void HeapifyDown(unsigned int i)
	{
		_Key key				=Nodes[i].Key;
		_Data data			=Nodes[i].Data;
		Node** Handle		=Nodes[i].Handle;

		unsigned int scan	=i;
		unsigned int child	=i;
		unsigned int left	=LeftChild(scan);
		unsigned int right	=RightChild(scan);
		Nodes[scan].Key		=key;
		if (left <= Size && Nodes[left].Key < Nodes[child].Key)
			child = left;
		if (right <= Size && Nodes[right].Key < Nodes[child].Key)
			child = right;

		while (child != scan)
		{
			Nodes[scan].Key			=Nodes[child].Key;
			Nodes[scan].Data		=Nodes[child].Data;
			Nodes[scan].Handle		=Nodes[child].Handle;
			*(Nodes[scan].Handle)	=&(Nodes[scan]);
			
			scan				=child;
			left				=LeftChild(scan);
			right				=RightChild(scan);
			Nodes[scan].Key		=key;

			if (left <= Size && Nodes[left].Key < Nodes[child].Key)
				child = left;
			if (right <= Size && Nodes[right].Key < Nodes[child].Key)
				child = right;
		}
		
		if (scan != i)
		{
			Nodes[scan].Data		=data;
			Nodes[scan].Handle		=Handle;
			*(Nodes[scan].Handle)	=&(Nodes[scan]);
		}
	}

	// PURPOSE:  "Bubble up" changes to node i
	// PARAMS:	i: index of the node from which to "bubble up"
	// RETURNS: the index where we stopped "bubbling".
	unsigned int HeapifyUp(unsigned int i)
	{
		_Key key		=Nodes[i].Key;
		_Data data		=Nodes[i].Data;
		Node** Handle	=Nodes[i].Handle;

		unsigned int scan	=i;
		unsigned int parent	=Parent(scan);
		while (scan>1 && Nodes[parent].Key > key)
		{
			Nodes[scan].Key			=Nodes[parent].Key;
			Nodes[scan].Data		=Nodes[parent].Data;
			Nodes[scan].Handle		=Nodes[parent].Handle;
			*(Nodes[scan].Handle)	=&(Nodes[scan]);
			scan					=parent;
			parent					=Parent(scan);
		}
		if (scan != i)
		{
			Nodes[scan].Key			=key;
			Nodes[scan].Data		=data;
			Nodes[scan].Handle		=Handle;
			*(Nodes[scan].Handle)	=&(Nodes[scan]);
		}
		return scan;
	}

	// PURPOSE:  Swap the info in one heap nodes with the info in another
	// PARAMS:	a, b:  the nodes to swap
	void Swap(unsigned int a, unsigned int b)
	{
		FastAssert(a<=Size && b<=Size);
		
		_Key		tempKey;
		_Data	tempData;
		Node**	tempHandle;

		Node& nodeA = Nodes[a];
		Node& nodeB = Nodes[b];

		tempKey			=nodeA.Key;
		tempData		=nodeA.Data;
		tempHandle		=nodeA.Handle;

		nodeA.Key		=nodeB.Key;
		nodeA.Data		=nodeB.Data;
		nodeA.Handle	=nodeB.Handle;

		nodeB.Key		=tempKey;	
		nodeB.Data		=tempData;
		nodeB.Handle	=tempHandle;

		*(nodeA.Handle)	=&nodeA;
		*(nodeB.Handle)	=&nodeB;
	}

private:
	unsigned int	MaxSize;	// The maximum number of nodes available in this heap.
	unsigned int	Size;		// How many nodes are currently used in the heap (use 1 thru Size, not 0 thru Size-1)
	Node*			Nodes;		// The array of heap nodes
	Node*			NodeDump;	// See the explanation for Insert.  Nodes who don't need handles update this instead!
};

}	// namespace rage

#endif
