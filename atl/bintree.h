//
// atl/bintree.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_BINTREE_H
#define ATL_BINTREE_H


////////////////////////////////////////////////////////////////
// external defines

#include "pool.h"

#include "data/resource.h"
#include "data/struct.h"
#include "string/string.h"

namespace rage {

////////////////////////////////////////////////////////////////
// order functions

inline bool atOrderLT (int x, int y)									{ return x < y; }
inline bool atOrderLT (u32 x, u32 y)									{ return x < y; }
inline bool atOrderLT (float x, float y)								{ return x < y; }
inline bool atOrderLT (u64 x, u64 y)									{ return x < y; }
extern bool atOrderLT (const char * x, const char * y);					// string ordering function
inline bool atOrderLT (const ConstString & x, const ConstString & y)	{ return atOrderLT(x.m_String,y.m_String); }


// PURPOSE: Implements a binary tree.
//
// PARAMS:
//	_Key - the type of the tree key
//	_Data - the type of the data to insert
//
// NOTES:
// This class can be used as a map with no memory deletion
// as data is added to it.  The primary interface is simply
// <c>Insert(key,data)</c>, <c>Access(key)</c>, and <c>Delete(key)</c>.
//
// For maps with <c>ConstString</c> as the key, special accessors have
// been added for <c>const char*</c> keys so you can use the map
// without memory allocation. Unfortunately this means that
// the template can't be used with <c>const char*</c> as the actual
// keys because of function ambiguity. This problem is the
// same for <c>atMap</c>.
//<FLAG Component>
template<typename _Key, typename _Data> class atBinTreeBase
{
public:
	////////////////////////////////////////////////////////////
	// the tree node
	class Node
	{
		friend class atBinTreeBase;

	public:
		// PURPOSE: Constructor
		Node () : m_Parent(NULL), m_ChildLeft(NULL), m_ChildRight(NULL) { }

		// PURPOSE: Overloaded Constructor
		// PARAMS: k - key for this node, d - data to store at this node
		Node (const _Key & k, const _Data & d) : m_Data(d), m_Key(k), m_Parent(NULL), m_ChildLeft(NULL), m_ChildRight(NULL) { }

		// PURPOSE: Fix up pointers when loading from a resource.
		// NOTES:	Intended to be called by the resource constructor of atBinTreeBase.
		Node(datResource &rsc);
		Node(datResource &rsc,
			void (*fixupKeyFunc)(datResource &rsc, _Key &key),
			void (*fixupDataFunc)(datResource &rsc, _Data &data));

#if __DECLARESTRUCT
		void DeclareStruct(datTypeStruct &s)
		{
			STRUCT_BEGIN(Node);

			STRUCT_FIELD(m_Data);
			STRUCT_FIELD(m_Key);
			STRUCT_FIELD(m_Parent);
			STRUCT_FIELD(m_ChildLeft);
			STRUCT_FIELD(m_ChildRight);

			STRUCT_END();
		}
#endif // __DECLARESTRUCT

		static void Place(void *that,datResource& rsc)
		{
			::new (that) Node(rsc);
		}

		static void Place(void *that, datResource& rsc,
			void (*fixupKeyFunc)(datResource &rsc, _Key &key),
			void (*fixupDataFunc)(datResource &rsc, _Data &data))
		{
			::new (that) Node(rsc, fixupKeyFunc, fixupDataFunc);
		}

		// PURPOSE: Get the parent or left/right child of this node (const and non-const versions)
		// RETURNS: the next node from the current node in const 
		Node * GetParent () const									{ return m_Parent; }
		Node * GetParent ()											{ return m_Parent; }

		const Node * GetChildLeft () const							{ return m_ChildLeft; }
		Node * GetChildLeft ()										{ return m_ChildLeft; }

		const Node * GetChildRight () const							{ return m_ChildRight; }
		Node * GetChildRight ()										{ return m_ChildRight; }

		// PURPOSE: Return true if node is not attached to a tree
		bool IsAttached () const									{ return m_Parent || m_ChildLeft || m_ChildRight; }

		const _Key & GetKey () const									{ return m_Key; }
		void SetKey (const _Key & key)									{ FastAssert(!m_Parent && !m_ChildRight && !m_ChildLeft); m_Key = key; }

		const _Data & GetData () const									{ return m_Data; }
		void SetData (const _Data & data)								{ m_Data = data; }

		bool IsLeftChild () const									{ return m_Parent && m_Parent->GetChildLeft()==this; }
		bool IsRightChild () const									{ return m_Parent && m_Parent->GetChildRight()==this; }

	protected:
		// PURPOSE: Set the current node's left/right/parent pointers to the given address.
		//          To be used by the atBinTreeBase friend class.
		void SetParent (Node * node)								{ FastAssert(!m_Parent || !node); m_Parent = node; }
		void SetChildLeft (Node * node)								{ FastAssert(!m_ChildLeft || !node); m_ChildLeft = node; }
		void SetChildRight (Node * node)							{ FastAssert(!m_ChildRight || !node); m_ChildRight = node; }
		void ReplaceChild (Node * oldChild, Node * newChild)		{ if (m_ChildLeft==oldChild) { SetChildLeft(NULL); SetChildLeft(newChild); } else if (m_ChildRight==oldChild) { SetChildRight(NULL); SetChildRight(newChild); } else AssertMsg(0 , "atBinTreeBase:ReplaceChild - not a child"); }

	public:
		_Data m_Data;													// data stored at this node

	protected:
		_Key m_Key;													// key for this node

		datRef<Node> m_Parent;										// parent of this node
		datOwner<Node> m_ChildLeft;									// left child of this node
		datOwner<Node> m_ChildRight;								// right child of this node
	};

public:
	typedef atPool<Node> NodePool;

	// PURPOSE: Class to encapsulate linear iteration through all inserted items in map
	class Iterator
	{
	public:
		// PURPOSE: Reset the iterator to the first entry of the map
		void Start()
		{	m_CurrentNode = m_Tree->FindMinimumNode();	}

		// PURPOSE: Advance the iterator to the next entry in the map
		void Next()
		{	if(!m_CurrentNode)
				return;
			m_CurrentNode = m_Tree->FindSuccessorNode(*m_CurrentNode);
		}

		// PURPOSE: Check if the iteration is complete
		// RETURNS: True if there are no more entries for the iterator to advance to
		bool AtEnd() const		{	return m_CurrentNode == NULL;		}

		// PURPOSE: Access the key and data of the current map entry
		// RETURNS: Reference to the key or the data the iterator is currently at
		const _Key& GetKey() const	{	FastAssert(m_CurrentNode); return m_CurrentNode->GetKey();	}
		_Data& GetData() const		{	FastAssert(m_CurrentNode); return m_CurrentNode->m_Data;	}
		_Data* GetDataPtr() const	{	FastAssert(m_CurrentNode); return &m_CurrentNode->m_Data;	}

		// PURPOSE: Operators to use the iterator as if it were a pointer to the type of data mapped
		// RETURNS: Reference or pointer to template type "_Data"
		_Data& operator*() const	{	return GetData();		}

		// PURPOSE: Advance the iterator to the next entry in the map
		Iterator& operator++()	{	Next(); return *this;	}

		// PURPOSE: Check if the iterator points to a valid entry
		// RETURNS: True if the iterator points to a valid entry, false if iteration is complete
		operator bool() const	{	return !AtEnd();		}

	protected:
		friend class atBinTreeBase<_Key, _Data>;

		Iterator(atBinTreeBase<_Key, _Data> &m) : m_CurrentNode(NULL), m_Tree(&m)
		{	Start();	}

		Node*				    m_CurrentNode;
		atBinTreeBase<_Key, _Data>*	m_Tree;
	};

	class ConstIterator : public Iterator
	{
	public:
		friend class atBinTreeBase<_Key, _Data>;

		const _Data& GetData() const	{	return Iterator::GetData();	}
		const _Data* GetDataPtr() const	{	return Iterator::GetDataPtr();	}
		const _Data& operator*() const	{	return Iterator::operator*(); }

	protected:
		ConstIterator(const atBinTreeBase<_Key, _Data> &m) : Iterator(const_cast<atBinTreeBase<_Key, _Data> &>(m))
		{ }
	};

public:
	// PURPOSE: constructor
	atBinTreeBase () : m_Root(NULL), m_NumNodes(0), m_NodePool(NULL)
	{	}

	// PURPOSE: constructor, preallocates node pool
	atBinTreeBase (int nodePoolSize) : m_Root(NULL), m_NumNodes(0), m_NodePool(rage_new NodePool(nodePoolSize))
	{	}

	// PURPOSE: Resource constructor.
	// NOTES: Inlined below.
	inline atBinTreeBase(datResource &rsc);

	inline atBinTreeBase(datResource &rsc,
			void (*fixupKeyFunc)(datResource &rsc, _Key &key),
			void (*fixupDataFunc)(datResource &rsc, _Data &data));

	void Place(datResource& rsc,
			   void (*fixupKeyFunc)(datResource &rsc, _Key &key) = NULL,
		       void (*fixupDataFunc)(datResource &rsc, _Data &data) = NULL)
	{
		if (fixupKeyFunc || fixupDataFunc)
		{
			::new (this) atBinTreeBase<_Key, _Data>(rsc, fixupKeyFunc, fixupDataFunc);
		}
		else
		{
			::new (this) atBinTreeBase<_Key, _Data>(rsc);
		}
	}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s)
	{
		typedef atBinTreeBase<_Key, _Data> atBinTreeType;
		STRUCT_BEGIN(atBinTreeType);

		STRUCT_FIELD(m_Root);
		STRUCT_FIELD(m_NumNodes);
		STRUCT_FIELD(m_NodePool);

		STRUCT_END();
	}
#endif // __DECLARESTRUCT

	// PURPOSE: destructor
	~atBinTreeBase ();

	void AllocatePool (int poolSize);

	bool IsEmpty () const
	{	FastAssert(m_NumNodes==0 || m_Root); return !m_Root;	}

	// PURPOSE: Get the root of the tree
	const Node * GetRoot () const
	{ return m_Root; }

	Node * GetRoot ()
	{ return m_Root; }

	// PURPOSE: Get the number of nodes in the tree
	int GetNumNodes () const
	{ return m_NumNodes; }

protected:
	// inlined add function for internal use
	void AddNodeUnderParent (Node & node, Node & parent)
	{
		AssertMsg((atOrderLT(node.m_Key,parent.m_Key) || atOrderLT(parent.m_Key,node.m_Key)) , "atBinTreeBase:AddNodeUnderParent - parent key same as child");
		if (atOrderLT(node.m_Key,parent.m_Key))
		{
			parent.SetChildLeft(&node); m_NumNodes++;
			node.SetParent(&parent);
		}
		else
		{
			parent.SetChildRight(&node); m_NumNodes++;
			node.SetParent(&parent);
		}
	}

public:
	////////////////////////////////////////////////////////////
	// manipulators, O(tree height) time

	////////////////////////////////////////////////////////////
	// These functions allow the user to manipulate nodes in the tree.
	// Functions that don't know about nodes are below (for inlining).

	// PURPOSE: add 'node' into the tree
	// RETURNS: false if a node with the same key is already in the tree
	bool AddNode (Node & node);

	// PURPOSE: remove 'node' from the tree
	// RETURNS: true if node is in the tree, false otherwise
	void RemoveNode (Node & node);

	// PURPOSE: remove and delete all nodes from the tree
	void DeleteAll ();

	void CopyTo(atBinTreeBase<_Key, _Data> &me) const;

	// PURPOSE: find the node with 'key' if it exists in the tree
	// RETURNS: found node, NULL if not found;
	//          'parentReturn' is optionally parent of the found node, or the parent of where it should have been
	const Node * FindNode (const _Key & key, Node ** parentReturn=NULL) const;
	Node * FindNode (const _Key & key, Node ** parentReturn=NULL);

	// PURPOSE: find the node with minimum/maximum key in the subtree under 'node'
	Node * FindMinimumNode (const Node & subtree);					// 
	Node * FindMinimumNode ()										{ return m_Root ? FindMinimumNode(*m_Root) : NULL; }
	Node * FindMaximumNode (const Node & subtree);					// 
	Node * FindMaximumNode ()										{ return m_Root ? FindMaximumNode(*m_Root) : NULL; }

	// PURPOSE: find the node with (minimum/maximum) key (greater than/less than) 'node''s key
	Node * FindPredecessorNode (const Node & node);
	Node * FindSuccessorNode (const Node & node);

	void Validate (const Node * node=NULL);							// validate the subtree under node, NULL starts at the root

	////////////////////////////////////////////////////////////
	// These functions access the keys and data only, and do not 
	// require the user to know about how data is stored in nodes in the
	// tree.  These functions are sufficient to use the tree as a map.

	// PURPOSE: create a new node with 'key' and 'data' and insert into the tree
	// RETURNS: false if a node with the same key is already in the tree
	bool Insert (const _Key & key, const _Data & data)
	{
		Node * parent;
		const Node * foundNode = FindNode(key,&parent);
		if (foundNode) return false;
		Node * newNode = (m_NodePool ? ::new (m_NodePool->New()) Node(key,data) : rage_new Node(key,data));
		FastAssert(newNode);
		if (parent) AddNodeUnderParent(*newNode,*parent);
		else AddNode(*newNode);
		return true;
	}

	// PURPOSE: delete a node with 'key' if it exists in the tree
	// RETURNS: false if a node with 'key' doesn't exist
	bool Delete (const _Key & key)
	{
		Node * foundNode = FindNode(key);
		if (foundNode) RemoveNode(*foundNode);
		else return false;
		if (m_NodePool) m_NodePool->Delete(foundNode);
		else delete foundNode;
		return true;
	}

	// PURPOSE: find the data associated with 'key' if it exists in the tree
	const _Data * Access (const _Key & key) const
	{
		const Node * foundNode = FindNode(key);
		return (foundNode ? &foundNode->m_Data : NULL);
	}
	_Data * Access (const _Key & key)
	{
		Node * foundNode = FindNode(key);
		return (foundNode ? &foundNode->m_Data : NULL);
	}

	// PURPOSE: Create an iterator to search through the map linearly
	// RETURNS: An Iterator object pointing at the first entry in the map
	Iterator CreateIterator()				{	return Iterator(*this);	}
	ConstIterator CreateIterator() const	{	return ConstIterator(*this);	}

protected:
	datOwner<Node> m_Root;											// root of the tree
	int m_NumNodes;													// number of nodes in the tree
	datRef<NodePool> m_NodePool;									// pool of preallocated nodes
	// NOTE: m_NodePool should be a datOwner, but atPool doesn't yet have resourcing ability,
	//       my reasoning is that mostly resourced trees won't use the node pool
};

////////////////////////////////////////////////////////////////
// implementations


//////////////////////////////////
// atBinTreeBase

template<typename _Key, typename _Data> atBinTreeBase<_Key,_Data>::~atBinTreeBase ()
{
	// The user is responsible for removing the contents of the tree, possibly
	// by calling DeleteAll(). It is done this way because the functions of the
	// template allow for nodes to be either internally or externally created.
	FastAssert(!m_Root);

	delete m_NodePool;
}


template<typename _Key, typename _Data> void atBinTreeBase<_Key,_Data>::DeleteAll ()
{
	// not the fastest implementation
	Node * node, * nextNode;
	node = FindMinimumNode();
	while (node)
	{
		nextNode = FindSuccessorNode(*node);
		RemoveNode(*node);
		if (m_NodePool)
		{
			node->~Node();
			m_NodePool->Delete(node);
		}
		else
			delete node;
		node = nextNode;
	}
	FastAssert(!m_Root);
}

template<typename _Key, typename _Data> void atBinTreeBase<_Key,_Data>::CopyTo(atBinTreeBase<_Key, _Data> &me) const
{
	// not the fastest implementation
	ConstIterator iter = CreateIterator();
	iter.Start();
	while(!iter.AtEnd())
	{
		//copy the node
		me.Insert(iter.GetKey(), iter.GetData());

		//move on
		iter.Next();
	}	
}

template<typename _Key, typename _Data> void atBinTreeBase<_Key,_Data>::AllocatePool (int poolSize)
{
	FastAssert(m_NodePool==NULL);
	FastAssert(IsEmpty());

	m_NodePool = rage_new NodePool(poolSize);
}


template<typename _Key, typename _Data> typename atBinTreeBase<_Key,_Data>::Node * atBinTreeBase<_Key,_Data>::FindMinimumNode (const Node & startNode)
{
	Node * node = (Node*)&startNode;
	Node * rv = node;
	while ((node=node->GetChildLeft())!=NULL)
	{
		rv = node;
	}
	return rv;
}


template<typename _Key, typename _Data> typename atBinTreeBase<_Key,_Data>::Node * atBinTreeBase<_Key,_Data>::FindMaximumNode (const Node & startNode)
{
	Node * node = (Node*)&startNode;
	Node * rv = node;
	while ((node=node->GetChildRight())!=NULL)
	{
		rv = node;
	}
	return rv;
}


template<typename _Key, typename _Data> typename atBinTreeBase<_Key,_Data>::Node * atBinTreeBase<_Key,_Data>::FindNode (const _Key & key, Node ** parentReturn)
{
	Node * node = m_Root;
	Node * parent = NULL;

	while (node)
	{
		if (atOrderLT(key,node->m_Key))
		{
			parent = node;
			node = node->GetChildLeft();
		}
		else if (atOrderLT(node->m_Key,key))
		{
			parent = node;
			node = node->GetChildRight();
		}
		else
		{
			if (parentReturn)
			{
				*parentReturn = parent;
			}
			return node;
		}
	}

	if (parentReturn)
	{
		*parentReturn = parent;
	}

	return node;
}


template<typename _Key, typename _Data> const typename atBinTreeBase<_Key,_Data>::Node * atBinTreeBase<_Key,_Data>::FindNode (const _Key & key, Node ** parentReturn) const
{
	Node * node = m_Root;
	Node * parent = NULL;

	while (node)
	{
		if (atOrderLT(key,node->m_Key))
		{
			parent = node;
			node = node->GetChildLeft();
		}
		else if (atOrderLT(node->m_Key,key))
		{
			parent = node;
			node = node->GetChildRight();
		}
		else
		{
			if (parentReturn)
			{
				*parentReturn = parent;
			}
			return node;
		}
	}

	if (parentReturn)
	{
		*parentReturn = parent;
	}

	return node;
}


template<typename _Key, typename _Data> typename atBinTreeBase<_Key,_Data>::Node * atBinTreeBase<_Key,_Data>::FindPredecessorNode (const Node & startNode)
{
	Node * node = (Node*)&startNode;

	if (node->GetChildLeft())
	{
		return FindMaximumNode(*node->GetChildLeft());
	}

	do
	{
		if (node->IsRightChild())
		{
			return node->GetParent();
		}
		node = node->GetParent();
	}
	while (node);

	return NULL;
}


template<typename _Key,typename _Data> typename atBinTreeBase<_Key,_Data>::Node * atBinTreeBase<_Key,_Data>::FindSuccessorNode (const Node & startNode)
{
	Node * node = (Node*)&startNode;

	if (node->GetChildRight())
	{
		return FindMinimumNode(*node->GetChildRight());
	}

	do
	{
		if (node->IsLeftChild())
		{
			return node->GetParent();
		}
		node = node->GetParent();
	}
	while (node);

	return NULL;
}


template<typename _Key, typename _Data> bool atBinTreeBase<_Key,_Data>::AddNode (Node & node)
{
	FastAssert(!node.IsAttached());

	Node * insertParent = NULL;
	Node * foundNode = FindNode(node.m_Key,&insertParent);

	if (foundNode)
	{
		return false;
	}
	else if (insertParent)
	{
		AddNodeUnderParent(node,*insertParent);
	}
	else
	{
		FastAssert(!m_Root);
		m_Root = &node;
		FastAssert(m_NumNodes==0);
		m_NumNodes = 1;
	}

	return true;
}


template<typename _Key, typename _Data> void atBinTreeBase<_Key,_Data>::RemoveNode (Node & node)
{
	FastAssert(&node==FindNode(node.m_Key));

	Node * replacement = NULL;

	if (!node.GetChildLeft() && !node.GetChildRight())
	{
		replacement = NULL;
	}
	else if (!node.GetChildLeft())
	{
		replacement = node.GetChildRight();
	}
	else if (!node.GetChildRight())
	{
		replacement = node.GetChildLeft();
	}
	else
	{
		// find the successor (and successor child)
		Node * successor = FindSuccessorNode(node);
		FastAssert(successor);
		FastAssert(successor->GetParent());
		FastAssert(!successor->GetChildLeft() || !successor->GetChildRight());
		Node * successorChild = successor->GetChildLeft();
		if (!successorChild)
		{
			successorChild = successor->GetChildRight();
		}

		// splice out the successor
		if (successorChild)
		{
			successorChild->SetParent(NULL);
			successorChild->SetParent(successor->GetParent());
		}

		successor->GetParent()->ReplaceChild(successor,successorChild);

		// and set the successor's children to the removed nodes children
		successor->SetChildLeft(NULL);
		successor->SetChildLeft(node.GetChildLeft());
		if (successor->GetChildLeft())
		{
			successor->GetChildLeft()->SetParent(NULL);
			successor->GetChildLeft()->SetParent(successor);
		}
		successor->SetChildRight(NULL);
		successor->SetChildRight(node.GetChildRight());
		if (successor->GetChildRight())
		{
			successor->GetChildRight()->SetParent(NULL);
			successor->GetChildRight()->SetParent(successor);
		}

		replacement = successor;
	}

	// set the parent for the replacement
	if (replacement)
	{
		replacement->SetParent(NULL);
		replacement->SetParent(node.GetParent());
	}

	// replace the appropriate node's parent's child
	if (node.GetParent())
	{
		node.GetParent()->ReplaceChild(&node,replacement);
	}

	// fixup the root as necessary
	if (m_Root==&node)
	{
		m_Root = replacement;
	}

	// clear out the removed node
	node.SetParent(NULL);
	node.SetChildLeft(NULL);
	node.SetChildRight(NULL);

	m_NumNodes--;
}


template<typename _Key, typename _Data> void atBinTreeBase<_Key,_Data>::Validate (const Node * node)
{
	// NOTE: This function does not use function-call recursion, so it 
	// is safe for large trees.

	if (!node)
	{
		node = m_Root;
	}

	const Node * prevNode = NULL;
	const Node * nextNode = NULL;

	while (node)
	{
		// visit left child (recurse), then right child (recurse), then parent (recurse)
		nextNode = node->GetParent();
		if (prevNode==node->GetParent())
		{
			// go to left, right, or parent
			if (node->GetChildLeft())
			{
				nextNode = node->GetChildLeft();
			}
			else if (node->GetChildRight())
			{
				nextNode = node->GetChildRight();
			}
		}
		else if (prevNode==node->GetChildLeft())
		{
			// go to right or parent
			if (node->GetChildRight())
			{
				nextNode = node->GetChildRight();
			}
		}

		// validate all links before leaving
		if (node->GetParent())
		{
			FastAssert(node->IsLeftChild() || node->IsRightChild());
			if (node->IsLeftChild())
			{
				FastAssert(atOrderLT(node->GetKey(),node->GetParent()->GetKey()));
			}
			else
			{
				FastAssert(atOrderLT(node->GetParent()->GetKey(),node->GetKey()));
			}
		}
		if (node->GetChildLeft())
		{
			FastAssert(node->GetChildLeft()->GetParent()==node);
			FastAssert(atOrderLT(node->GetChildLeft()->GetKey(),node->GetKey()));
		}
		if (node->GetChildRight())
		{
			FastAssert(node->GetChildRight()->GetParent()==node);
			FastAssert(atOrderLT(node->GetKey(),node->GetChildRight()->GetKey()));
		}

		prevNode = node;
		node = nextNode;
	}
}

template<typename _Key, typename _Data> 
class atBinTree : public atBinTreeBase<_Key, _Data> {
public:
	atBinTree()
	{	}

	atBinTree (int nodePoolSize) : atBinTreeBase<_Key, _Data>(nodePoolSize)
	{	}

	inline atBinTree(datResource &rsc) : atBinTreeBase<_Key, _Data>()
	{	}

	inline atBinTree(	datResource &rsc,
		void (*fixupKeyFunc)(datResource &rsc, _Key &key),
		void (*fixupDataFunc)(datResource &rsc, _Data &data)
		)
		: atBinTreeBase<_Key, _Data>(rsc, fixupKeyFunc, fixupDataFunc)
	{	}
};

//Partial specialization
template<typename _Data> 
class atBinTree<ConstString, _Data> : public atBinTreeBase<ConstString, _Data> {
public:
	// PURPOSE: find the data associated with 'key' if it exists in the tree
	// NOTES:	this version is here to avoid ConstString objects from being
	//			implicitly constructed (allocating memory) when atBinTreeBase
	//			is used with ConstStrings as keys.
	const _Data * Access (const char * key) const
	{
		ConstString wrapper;
		wrapper.m_String = key;
		const _Data * r = atBinTreeBase<ConstString, _Data>::Access(wrapper);
		wrapper.m_String = NULL;
		return r;
	}
	_Data * Access (const char * key)
	{
		ConstString wrapper;
		wrapper.m_String = key;
		_Data * r = atBinTreeBase<ConstString, _Data>::Access(wrapper);
		wrapper.m_String = NULL;
		return r;
	}

	// PURPOSE: create a new node with 'key' and 'data' and insert into the tree
	// RETURNS: false if a node with the same key is already in the tree
	// NOTES:	this version is here to avoid ConstString objects from being
	//			implicitly constructed (allocating memory) when atBinTreeBase
	//			is used with ConstStrings as keys.
	bool Insert (const char * key, const _Data & data)
	{
		ConstString wrapper;
		wrapper.m_String = key;
		bool r = atBinTreeBase<ConstString, _Data>::Insert(wrapper, data);
		wrapper.m_String = NULL;
		return r;
	}

	// PURPOSE: delete a node with 'key' if it exists in the tree
	// RETURNS: false if a node with 'key' doesn't exist
	// NOTES:	this version is here to avoid ConstString objects from being
	//			implicitly constructed (allocating memory) when atBinTreeBase
	//			is used with ConstStrings as keys.
	bool Delete (const char * key)
	{
		ConstString wrapper;
		wrapper.m_String = key;
		bool r = atBinTreeBase<ConstString, _Data>::Delete(wrapper);
		wrapper.m_String = NULL;
		return r;
	}

	atBinTree()
	{	}

	// PURPOSE: constructor, preallocates node pool
	atBinTree (int nodePoolSize) : atBinTreeBase<ConstString, _Data>(nodePoolSize)
	{	}

	// PURPOSE: Resource constructor.
	inline atBinTree(datResource &rsc) : atBinTreeBase<ConstString, _Data>(rsc)
	{	}

	inline atBinTree(	datResource &rsc,
						void (*fixupKeyFunc)(datResource &rsc, ConstString &key),
						void (*fixupDataFunc)(datResource &rsc, _Data &data)
		)
		: atBinTreeBase<ConstString, _Data>(rsc, fixupKeyFunc, fixupDataFunc)
	{	}

};

}	// namespace rage

#endif // ndef ATL_BINTREE_H
