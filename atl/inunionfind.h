// 
// atl/atinbintree.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_INUNIONFIND_H
#define ATL_INUNIONFIND_H

#include <stddef.h>		// for size_t

#include "delegate.h"
#include "dlistsimple.h"

namespace rage
{

// PURPOSE
//
// Template class for computing partitions on equivalent sets of objects. For
// example, if you have objects named 0, 1, 2, 3, and 4, and you assert that
// 0 == 1, 2 == 4 and 4 == 3, then we are interested in asking the questions
// "is 0 == 4?" (which should be false) and "is 2 == 3?" (which should be
// true).
//
// The operation (aka union) dealt with is an equivalence relation, which means
// it is all of these things:
//
//    * Transitive - a == b && b == c implies that a == c
//    * Symmetric  - a == b implies that b == a
//    * Reflexive  - a == a
//
// Using the concept of a "forest of trees" to represent the equivalence
// classes, combined with the optimizations of "union by rank" and "path
// compression", you achieve an amortized runtime complexity of O(a(n)) per
// query, the inverse of the Ackerman function, a very slow growing function.
// This was shown in 1989 by Fredman and Saks to be the lower bound on
// cost. For more background, see:
//
// http://en.wikipedia.org/wiki/Disjoint-set_data_structure
//
// The implementaiton is an "intrusive" data structure, which means the data
// structure atInUnionFindLink must be embedded into the instances to be
// organized. This saves a few pointers per instance, but means that every
// instance handled by the template must be the same concrete class.
//
// To use the system, you put the atInUnionFindLink into your class like this:
//
// class Foo
// {
// public:
//     unsigned m_Id;
//
//     atInUnionFindLink< Foo > m_Link;
// };
//
// then, create a union find object and insert all your objects into it
//
// Foo objs[5];
// atUnionFind<Foo, &Food:m_Link> unionFind;
//
// unionFind.Insert(&objs[0]);
// unionFind.Insert(&objs[1]);
// unionFind.Insert(&objs[2]);
// unionFind.Insert(&objs[3]);
// unionFind.Insert(&objs[4]);
// 
// Now, you can assert the equivalence of two items using the Union operation
//
// unionFind.Union(&objs[0], &objs[1]);
// unionFind.Union(&objs[1], &objs[2]);
//
// And you can test equivalence using the Find operation. Each call to Find
// returns the representative item of its class:
//
// if (unionFind.Find(&objs[0]) == unionFind(&objs[2]))
// {
//      // do something
// }
// 
// You can also iterate over all the items delimited by class:
//
// qa_UnionFind::Iterator it(&uf);
//
// while (it.Item())
// {
//     Printf("%d ", it.Item()->m_Id);
//
//     it.Next();
//
//     if (it.AtClassEnd())
//     {  
//         Displayf("\n --- next class ---");
//     }
// }
//

// PURPOSE
//
// Link embedded in the objects on which we want to determine equivalence
template< typename T >
struct atInUnionFindLink
{
    atInUnionFindLink()
        : m_Parent(NULL)
        , m_Rank(0)
        , m_Size(1)
    {
		m_Children[0] = m_Children[1] = m_Children[2] = NULL;
    }

    //Copy ctor and assignment do nothing because it's invalid to assign
    //one link to another.

    atInUnionFindLink(const atInUnionFindLink< T >&)
        : m_Parent(NULL)
        , m_Rank(0)
        , m_Size(1)
    {
		m_Children[0] = m_Children[1] = m_Children[2] = NULL;
        FastAssert("Invalid copy ctor on atInUnionFindLink" && false);
    }

    atInUnionFindLink< T >& operator=(const atInUnionFindLink< T >&)
    {
        FastAssert("Invalid assignment on atInUnionFindLink" && false);

        return *this;
    }

    DLinkSimple<atInUnionFindLink> m_SiblingLink; // Links children of the same node together
	void* m_Children[3]; // 
	T* m_Parent;
    int m_Rank;
    int m_Size;
};

//PURPOSE
//  The tree itself. You need one of these for every partition you want to create. In practice,
//  you will probably want to use the atInUnionFind, below, unless you want to make your own
//  link class.
template< typename T, typename L, L T::*LINK >
class atInUnionFindTemplate
{
	typedef DLListSimple<DLIST_SIMPLE_INIT(atInUnionFindLink<T>, m_SiblingLink)> ChildList;
	
public:

    //PURPOSE
    //  Non-const iterator.
    class Iterator
    {
    public:

        Iterator()
        {
        }

        explicit Iterator(atInUnionFindLink<T>* link)
			: m_It(link)
        {
        }

        Iterator(const Iterator& other)
            : m_It(other.m_It)
        {
        }

        const Iterator& operator= (const Iterator& other)
        {
            m_It = other.m_It;
            
            return *this;
        }

        // PURPOSE: Walk through the items contained by the union find, with all
        // equivalent items coming out in connected blocks.
        __forceinline void Next()
        {
            // We are done, so just return right away
            if (m_It.Item() == NULL)
            {
                return;
            }

            // Save off where we were, in case we run off the end of a sibling list
            typename ChildList::Iterator prevIt = m_It;

            // Look at our children
            ChildList* children = (ChildList*)(m_It.Item()->m_Children);
            if (children && children->GetFirst())
			{
                // If we have any children, descend into there
                m_It = typename ChildList::Iterator(children->GetFirst());
            }
            else
            {
                // Otherwise, go to our next sibling
			    m_It.Next();
            }

            // If we ran off the end of our sibling list, go to our parent's sibling
            // Stop if we run out of parents, that means we have popped all the way to
            // the last item
			while (m_It.Item() == NULL && prevIt.Item() != NULL && prevIt.Item()->m_Parent)
			{
                T* parent = prevIt.Item()->m_Parent;

				if (parent)
				{
                    // Go to our parent
                    m_It = typename ChildList::Iterator(&(parent->*LINK));

                    // Our parent might not have a sibling either, so be ready to go to
                    // our grandparent's sibling, etc.
                    prevIt = m_It;

                    // Go to our parent's (grandparent's, etc) sibling
                    m_It.Next();
				}
			}
        }

        // PURPOSE: Find the beginning of the next equivalence class.
		// RETURNS: An iterator pointing to the first element of the next class.
		// NOTES: To iterate over one class, iterate until class.GetNextClass
		__forceinline Iterator GetNextClass() const
        {
            if (m_It.Item() == NULL)
            {
                return *this;
            }

            Iterator it(*this);

            while (T* parent = it.m_It.Item()->m_Parent)
            {
                it.m_It = typename ChildList::Iterator(&(parent->*LINK));
            }

            it.m_It.Next();

            return it;
        }

		__forceinline bool operator==(const Iterator& other) const
		{
			return other.Item() == Item();
		}

		__forceinline bool operator!=(const Iterator& other) const
		{
			return other.Item() != Item();
		}

        // PURPOSE: Return the data pointed to by the iterator
        // RETURN: the item the iterator points to
        __forceinline T* Item() const
        {
            if (m_It.Item())
            {
                return GetObjFromLink(const_cast<atInUnionFindLink<T>* >(m_It.Item()));
            }
            else
            {
                return NULL;
            }
        }

    private:

		typename ChildList::Iterator m_It;
    };

    atInUnionFindTemplate();

    ~atInUnionFindTemplate();

    //PURPOSE
    //  Inserts a new item into the structure.
    void Insert(T* node);

    //PURPOSE
    //  Removes the given item.
    //NOTES
    // - This is the most expensive basic operation in the class! In the worst case, each call to Remove
    //   costs as many operations as there are elements in the class the node belongs to. So, if it is
    //   at all possible, Reset the list before removing items. Definitely don't clear out a built-up tree
    //   one node at a time by calling this function, because it will quite likely cost O(N*N) time.
	// - The worst case scenario is that the item that is chosen for removal is the representative item
	//   for a fully path-compressed tree, which means that every item in the class will have to be
	//   updated with a new parent.
	// - The worst worst case scenario is when the worst case scenario happens on every removal, because
	//   Iterator::Next will typically leave the iterator pointing at the new representative of the class
	//   after the Remove, triggering another O(N) operation.
    // - If a lot of one-at-a-time removals are necessary while the classes are built up, it would be
    //   possible to write a "removal list" that uses the m_ObjectLink member of the link class to link
    //   the objects together that need to be removed. Then, all the needed objects could be removed in one
    //   pass with a maximum cost equal to the total number of objects in the union find.
    void Remove(T* node);

     //PURPOSE
    //  Resets all equivalence class data, returning the tree to a pristine state
    void Reset();

   //PURPOSE
    //  Returns the representative element for a particular item
    //PARAMS
    //  node - The node to find the representative for
    //RETURNS
    //  the representative of the set that node belongs to
    //NOTES
    //   - Performs "path compression" as it operates, so the tree can be rearranged by this operation.
    //   - If two objects are in the same equivalence class, Find will return the same representative for them,
    //     in other words Find(a) == Find(b) will be true.
    T* Find(T* node);

    //PURPOSE
    //  Cause the two items to become equivalent, so they will appear in the same equivalence class. 
    //PARAMS
    //  nodeA - One of the nodes to become equivalent to the other
    //  nodeB - The other node
    //RETURNS
    //  The new representative node for the equivalence class uniting nodeA and nodeB
    //NOTES
    //   - If nodeA is already equivalent to nodeB, the operation has no effect, except that an internal
    //     call to Find could have caused the tree to reorganize because of path compression.
    T* Union(T* nodeA, T* nodeB);

    //PURPOSE
    //  Cause the two items to become equivalent, so they will appear in the same equivalence class. Must
    //      be called on two nodes that have just been returned by Find, otherwise the behavior is undefined.
    //PARAMS
    //  rootA - One of the nodes to become equivalent to the other, just returned by Find
    //  rootB - The other node, having just been returned by Find
    //RETURNS
    //  The new representative node for the equivalence class uniting nodeA and nodeB
    //NOTES
    //   - If nodeA is already equivalent to nodeB, the operation has no effect, except that an internal
    //     call to Find could have caused the tree to reorganize because of path compression.
    //   - Only call this function if you have the results of two Find operations to call it with. If
    //     you only have the two nodes (the normal case), use Union instead
    T* UnionFound(T* rootA, T* rootB);

    Iterator GetFirstClass();

    //PURPOSE
    //  Returns the item in the tree with the minimum key.
    T* GetFirst();
    const T* GetFirst() const;

    //PURPOSE
    //  Returns the number of equivalence classes in the union find.
    int GetClassCount() const;

    void PrintTree() const;
    void PrintChildren(const T* parent) const;

    static T* GetParent(T* node);
    static const T* GetParent(const T* node);
    void SetParent(T* node, T* parent);

    static int GetRank(const T* node);
    static void SetRank(T* node, int rank);

    static int GetSize(const T* node);
    static void SetSize(T* node, int size);

	static ChildList* GetChildren(T* node)
	{
		return (ChildList*)(node->*LINK).m_Children;
	}

    static const ChildList* GetChildren(const T* node)
    {
        return GetChildren(const_cast<T*>(node));
    }

protected:

    __forceinline static T* GetObjFromLink(atInUnionFindLink<T>* link)
    {
        FastAssert(link);
        return (T*)((size_t)link - reinterpret_cast<size_t>(&(reinterpret_cast<T*>(NULL)->*LINK)));
    }

    __forceinline static const T* GetObjFromLink(const atInUnionFindLink<T>* link)
    {
        return GetObjFromLink(const_cast<atInUnionFindLink<T>* >(link));
    }

private:

    //Disallow copy construction and assignment
    atInUnionFindTemplate(const atInUnionFindTemplate< T, L, LINK >&);
    atInUnionFindTemplate< T, L, LINK >& operator=(const atInUnionFindTemplate< T, L, LINK >&);

	ChildList m_EquivalenceClasses;
};

//Specialization of atInUnionFindTemplate for bin trees that use atInUnionFindLink
//as the type of the LINK memeber.
template< typename T, atInUnionFindLink< T > T::*LINK >
class atInUnionFind : public atInUnionFindTemplate< T, atInUnionFindLink< T >, LINK >
{
public:
};

template< typename T, typename L, L T::*LINK >
atInUnionFindTemplate< T, L, LINK >::atInUnionFindTemplate()
{
}

template< typename T, typename L, L T::*LINK >
atInUnionFindTemplate< T, L, LINK >::~atInUnionFindTemplate()
{
}

template< typename T, typename L, L T::*LINK >
void
atInUnionFindTemplate< T, L, LINK >::Insert(T* node)
{
    FastAssert(0 == GetParent(node));
    FastAssert(GetChildren(node)->IsEmpty());
    SetRank(node, 0);
    SetSize(node, 1);

    m_EquivalenceClasses.AddToTail(&(node->*LINK));
}

template< typename T, typename L, L T::*LINK >
void
atInUnionFindTemplate< T, L, LINK >::Remove(T* node)
{
    SetRank(node, 0);
    SetSize(node, 1);

    T* parent = GetParent(node);

    ChildList* children = GetChildren(node);
    FastAssert(children);
    if (!children->IsEmpty())
    {
        // if we have children, we have to put them somewhere where they're still connected
        if (parent)
        {
            // if we have a parent, then just hook our children into it
            typename ChildList::Iterator childIt(children->GetFirst());
            
            while (atInUnionFindLink<T>* childLink = childIt.Item())
            {
                childIt.Next();
                SetParent(GetObjFromLink(childLink), parent);
            }
        }
        else
        {
            // if we don't have a parent, then we have to pick a child, make it the parent of all
            // the other children, and put it into the equivalence classes
            atInUnionFindLink<T>* newParentLink = children->GetFirst();
            T* newParent = GetObjFromLink(newParentLink);
            SetParent(newParent, NULL);

            typename ChildList::Iterator childIt(children->GetFirst());

            while (atInUnionFindLink<T>* childLink = childIt.Item())
            {
                childIt.Next();
                SetParent(GetObjFromLink(childLink), newParent);
            }
        }
    }

    if (parent)
    {
        // Remove ourselves from the prior parent's children
        GetChildren(parent)->Remove(&(node->*LINK));
    }

    // Set our parent to NULL
    (node->*LINK).m_Parent = NULL;

    if (m_EquivalenceClasses.HasObj(&(node->*LINK)))
    {
        m_EquivalenceClasses.Remove(&(node->*LINK));
    }

    FastAssert(GetParent(node) == NULL);
    FastAssert(children->IsEmpty());
    FastAssert(0 == GetRank(node));
    FastAssert(1 == GetSize(node));
}

template< typename T, typename L, L T::*LINK >
void
atInUnionFindTemplate< T, L, LINK >::Reset()
{
	::new (this) atInUnionFindTemplate< T, L, LINK >;
}

template< typename T, typename L, L T::*LINK >
T*
atInUnionFindTemplate< T, L, LINK >::Find(T* node)
{
    T* representative = node;

    // Walk up the tree to find our representative
    while (T* parent = GetParent(representative))
    {
        representative = parent;
    }

    // Perform path compression, reassigning all nodes along the path to point directly to the representative
    while (node != representative)
    {
        T* parent = GetParent(node);
        SetParent(node, representative);
        node = parent;
    }

    return node;
}

template< typename T, typename L, L T::*LINK >
T*
atInUnionFindTemplate< T, L, LINK >::UnionFound(T* rootA, T* rootB)
{
    if (rootA == rootB)
    {
        // Already in the same class, don't do anything
        return rootA;
    }

    // Union by rank: Always attach a smaller tree to a larger tree. When two trees of equal size have to be
    // combined, the rank is increased by one.
    int rankA = GetRank(rootA);
    int rankB = GetRank(rootB);
    if (rankA > rankB)
    {
        // nodeA is the root of a taller tree, make it the parent to minimize depth
        SetParent(rootB, rootA);
        SetSize(rootA, GetSize(rootA) + GetSize(rootB));

        return rootA;
    }
    else if (rankA < rankB)
    {
        // nodeA is the root of a taller tree, make it the parent to minimize depth
        SetParent(rootA, rootB);
        SetSize(rootB, GetSize(rootA) + GetSize(rootB));

        return rootB;
    }
    else
    {
        // nodeA and nodeB are roots of trees of the same height, one of them has to get taller
        SetParent(rootB, rootA);
        SetRank(rootA, rankA + 1);
        SetSize(rootA, GetSize(rootA) + GetSize(rootB));

        return rootA;
    }
}

template< typename T, typename L, L T::*LINK >
T*
atInUnionFindTemplate< T, L, LINK >::Union(T* nodeA, T* nodeB)
{
    T* rootA = Find(nodeA);
    T* rootB = Find(nodeB);

    return UnionFound(rootA, rootB);
}

template< typename T, typename L, L T::*LINK >
typename atInUnionFindTemplate< T, L, LINK >::Iterator
atInUnionFindTemplate< T, L, LINK >::GetFirstClass()
{
    return Iterator(m_EquivalenceClasses.GetFirst());
}

template< typename T, typename L, L T::*LINK >
T*
atInUnionFindTemplate< T, L, LINK >::GetFirst()
{
    if (atInUnionFindLink<T>* link = m_EquivalenceClasses.GetFirst())
    {
	    return GetObjFromLink(link);
    }
    else
    {
        return NULL;
    }
}

template< typename T, typename L, L T::*LINK >
const T*
atInUnionFindTemplate< T, L, LINK >::GetFirst() const
{
	return GetFirst();
}

template< typename T, typename L, L T::*LINK >
int
atInUnionFindTemplate< T, L, LINK >::GetClassCount() const
{
    return m_EquivalenceClasses.GetCount();
}

template< typename T, typename L, L T::*LINK >
void
atInUnionFindTemplate< T, L, LINK >::PrintTree() const
{
    Printf("Class roots: ");

    const atInUnionFindLink<T>* childLink = m_EquivalenceClasses.GetFirst();
    
    while (childLink)
    {
        GetObjFromLink(childLink)->Print();
        childLink = m_EquivalenceClasses.GetNext(childLink);
    }

    Printf("\n");

    childLink = m_EquivalenceClasses.GetFirst();

    while (childLink)
    {
        Printf("Children of ");
        GetObjFromLink(childLink)->Print();
        Printf(": ");

        PrintChildren(GetObjFromLink(childLink));

        childLink = m_EquivalenceClasses.GetNext(childLink);
    }
}

template< typename T, typename L, L T::*LINK >
void
atInUnionFindTemplate< T, L, LINK >::PrintChildren(const T* parent) const
{
    const ChildList* children = GetChildren(parent);

    const atInUnionFindLink<T>* childLink = children->GetFirst();

    while (childLink)
    {
        GetObjFromLink(childLink)->Print();
        childLink = children->GetNext(childLink);
    }

    Printf("\n");

    childLink = children->GetFirst();

    while (childLink)
    {
        Printf("Children of ");
        GetObjFromLink(childLink)->Print();
        Printf(": ");

        PrintChildren(GetObjFromLink(childLink));

        childLink = children->GetNext(childLink);
    }
}

//protected:

template< typename T, typename L, L T::*LINK >
T*
atInUnionFindTemplate< T, L, LINK >::GetParent(T* node)
{
    return (node->*LINK).m_Parent;
}

template< typename T, typename L, L T::*LINK >
const T*
atInUnionFindTemplate< T, L, LINK >::GetParent(const T* node)
{
    return (node->*LINK).m_Parent;
}

template< typename T, typename L, L T::*LINK >
void
atInUnionFindTemplate< T, L, LINK >::SetParent(T* node, T* parent)
{
    if (GetParent(node) != parent)
    {
	    if (GetParent(node))
	    {
		    // Remove ourselves from the prior parent's children
		    GetChildren(GetParent(node))->Remove(&(node->*LINK));
	    }
	    else if (parent)
	    {
		    FastAssert(m_EquivalenceClasses.HasObj(&(node->*LINK)));
		    m_EquivalenceClasses.Remove(&(node->*LINK));
	    }

	    // Set our parent to our new parent
        (node->*LINK).m_Parent = parent;

	    if (parent)
	    {
		    // Add ourselves to the children of the new parent
		    GetChildren(parent)->AddToTail(&(node->*LINK));
	    }
	    else if (!m_EquivalenceClasses.HasObj(&(node->*LINK)))
	    {
            // Add to head so that we can iterate forward through
            // doing SetParent(x, NULL) to disconnect the sets
		    m_EquivalenceClasses.AddToHead(&(node->*LINK));
	    }
    }
}

template< typename T, typename L, L T::*LINK >
int
atInUnionFindTemplate< T, L, LINK >::GetRank(const T* node)
{
    return (node->*LINK).m_Rank;
}

template< typename T, typename L, L T::*LINK >
void
atInUnionFindTemplate< T, L, LINK >::SetRank(T* node, int rank)
{
    (node->*LINK).m_Rank = rank;
}

template< typename T, typename L, L T::*LINK >
int
atInUnionFindTemplate< T, L, LINK >::GetSize(const T* node)
{
    return (node->*LINK).m_Size;
}

template< typename T, typename L, L T::*LINK >
void
atInUnionFindTemplate< T, L, LINK >::SetSize(T* node, int size)
{
    (node->*LINK).m_Size = size;
}

}   //namespace rage

#endif  //ATL_INUNIONFIND_H
