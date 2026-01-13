// 
// atl/atsafedlistsimple.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_ATSAFEDLLISTSIMPLE_H
#define ATL_ATSAFEDLLISTSIMPLE_H

#include "atl/dlistsimple.h"
#include "atl/atinbintree.h"

namespace rage
{

//PURPOSE
//  This class permits safely removing items from a list while maintaining
//  the integrity of iterations that are proceeding at the time of removal.
//
//  Each new iteration must be started by calling BeginIteration().
//
//  The container maintains a list of all current iterations.  Whenever
//  an item is removed all current iterators are validated.  If any
//  iterators reference the removed item they are advanced to the next item
//  in the collection.
//
//  This class is useful in cases where removals can occur in the midst
//  of an iteration.  For example, if contained items have a method with
//  a potential side effect of removing items from the container, and that
//  method is called during an iteration, the integrity of the iteration will
//  be maintained.  One example of this is an event dispatcher where each
//  item in the collection is an event handler that, when invoked, might remove
//  itself from the collection.

template< typename T, typename UnsafeContainer >
class atSafeContainer
{
    typedef atSafeContainer< T, UnsafeContainer > Container;

public:

    //PURPOSE
    //  Used to iterate over items in a safe container.
    class Iterator
    {
        friend class atSafeContainer< T, UnsafeContainer >;

    public:

        Iterator()
            : m_Container( 0 )
            , m_GotoNext( true )
        {
        }

        ~Iterator()
        {
            //Automatically remove ourselves from the list of current
            //iterations.
            if( m_Container )
            {
                this->Reset( 0, 0 );
            }
        }

        //PURPOSE
        //  Returns true if the iterator contains a valid item.
        bool IsValid() const
        {
            return m_It.IsValid();
        }

        //PURPOSE
        //  Advances to the next item in the container.
        void Next()
        {
            if( m_GotoNext )
            {
                m_It.Next();
            }
            else
            {
                m_GotoNext = true;
            }

            if( !m_It.IsValid() && m_Container )
            {
                this->Reset( 0, 0 );
            }
        }

        //PURPOSE
        //  Advances to the previous item in the container.
        void Prev()
        {
            m_It.Prev();

            if( !m_It.IsValid() && m_Container )
            {
                this->Reset( 0, 0 );
            }

            m_GotoNext = true;
        }

        //PURPOSE
        //  Returns a pointer to the contained item.
        T* Item()
        {
            return m_It.Item();
        }

        //PURPOSE
        //  Returns a const pointer to the contained item.
        const T* Item() const
        {
            return m_It.Item();
        }

    protected:

        //PURPOSE
        //  Resets the iterator to a new iteration.
        void Reset( Container* container, T* item )
        {
            if( container && item )
            {
                //Make sure the iterator is not currently used.
                FastAssert( 0 == m_Container );

                m_It = typename UnsafeContainer::Iterator( item );

                m_Container = container;
                m_GotoNext = true;

                //Add ourselves to the container's managed set of
                //iterators.
                m_Container->m_CurrentIterators.AddToTail( this );
            }
            else
            {
                //Remove ourselves to the container's managed set of
                //iterators.
                if( m_Container )
                {
                    m_Container->m_CurrentIterators.Remove( this );
                }

                m_It = typename UnsafeContainer::Iterator();
                m_Container = 0;
                m_GotoNext = true;
            }
        }

        //The unsafe iterator.
        typename UnsafeContainer::Iterator m_It;

        //The container over which we're iterating.
        Container* m_Container;

        //List link for maintaining a list of current iterators.
        DLinkSimple< Iterator > m_Link;

        bool m_GotoNext     : 1;
    };

    friend class Iterator;

protected:

    atSafeContainer()
    {
    }

    atSafeContainer( const atSafeContainer< T, UnsafeContainer >& rhs )
    {
        *this = rhs;
    }

    virtual ~atSafeContainer()
    {
        while( !m_CurrentIterators.IsEmpty() )
        {
            this->EndIteration( m_CurrentIterators.GetFirst() );
        }
    }

    atSafeContainer< T, UnsafeContainer >& operator=( const atSafeContainer< T, UnsafeContainer >& rhs )
    {
        if( this != &rhs )
        {
            m_UnsafeContainer = rhs.m_UnsafeContainer;
        }

        return *this;
    }

public:

    //PURPOSE
    //  Begins a new iteration.
    //  Places the iterator in a set of managed iterators that will
    //  be validated whenever an item is removed from the collection.
    void BeginIteration( Iterator* it )
    {
        it->Reset( this, m_UnsafeContainer.GetFirst() );
    }

    //PURPOSE
    //  Ends the given iteration.
    //  Removes the iterator from the set of managed iterators.
    void EndIteration( Iterator* it )
    {
        FastAssert( 0 == it->m_Container || this == it->m_Container );

        it->Reset( 0, 0 );
    }

    //PURPOSE
    //  Removes an item from the collection and maintains the integrity of
    //  all current iterators.
    void Remove( T* item )
    {
        typename ItList::Iterator itCur( m_CurrentIterators.GetFirst() );

        for( ; itCur.IsValid(); itCur.Next() )
        {
            if( itCur.Item()->Item() == item )
            {
                Iterator* it = itCur.Item();

                it->m_It.Next();
                it->m_GotoNext = false;
            }
        }

        m_UnsafeContainer.Remove( item );
    }

    //PURPOSE
    //  Removes an item from the collection and maintains the integrity of
    //  all current iterators.
    void Remove( Iterator& it )
    {
        this->Remove( it.Item() );
    }

    //PURPOSE
    //  Returns true if the container is empty.
    bool IsEmpty() const
    {
        return m_UnsafeContainer.IsEmpty();
    }

    //PURPOSE
    //  Returns the number of items in the container.
    int GetCount() const
    {
        return m_UnsafeContainer.GetCount();
    }

    //PURPOSE
    //  Resets the container to its initial state by removing
    //  all items while maintaining the integrity of all current iterators.
    void Reset()
    {
        Iterator it;
        
        this->BeginIteration( &it );

        while( it.IsValid() )
        {
            this->Remove( it );
        }
    }

    //PURPOSE
    //  Resets the container to its initial state by removing
    //  all items while maintaining the integrity of all current iterators.
    void Clear()
    {
        this->Reset();
    }

protected:

    void BeginIteration( Iterator* it, T* item )
    {
        it->Reset( this, item );
    }

    UnsafeContainer m_UnsafeContainer;

private:

    typedef DLListSimple< Iterator, &Iterator::m_Link > ItList;

    ItList m_CurrentIterators;
};

//PURPOSE
//  A safe version of DLListSimple.  Maintains the integrity of all current
//  iterations when an item is removed from the collection.
template< typename T, DLinkSimple< T > T::*LINK >
class atSafeDLListSimple : public atSafeContainer< T, DLListSimple< T, LINK > >
{
    typedef atSafeContainer< T, DLListSimple< T, LINK > > SafeContainer;
    typedef atSafeDLListSimple< T, LINK > Container;

public:

    typedef typename SafeContainer::Iterator Iterator;

    atSafeDLListSimple()
    {
    }

    atSafeDLListSimple( const atSafeDLListSimple< T, LINK >& rhs )
    {
        *this = rhs;
    }

    ~atSafeDLListSimple()
    {
    }

    atSafeDLListSimple< T, LINK >& operator=( const atSafeDLListSimple< T, LINK >& rhs )
    {
        if( this != &rhs )
        {
            this->SafeContainer::operator=( rhs );
        }

        return *this;
    }

    //PURPOSE
    //  Adds an item to the head of the list.
    void AddToHead( T* item )
    {
        this->SafeContainer::m_UnsafeContainer.AddToHead( item );
    }

    //PURPOSE
    //  Adds an item to the tail of the list.
    void AddToTail( T* item )
    {
        this->SafeContainer::m_UnsafeContainer.AddToTail( item );
    }

    //PURPOSE
    //  Returns true if the list contains the item.
    bool Contains( const T* t ) const
    {
        return this->SafeContainer::m_UnsafeContainer.Contains( t );
    }
};

//PURPOSE
//  A safe version of atInBinTree.  Maintains the integrity of all current
//  iterations when an item is removed from the collection.
template< typename K, typename T, atInBinTreeLink< K, T > T::*LINK, typename COMPARE = atLess< K >, bool BALANCED = true >
class atSafeInBinTree : public atSafeContainer< T, atInBinTree< K, T, LINK, COMPARE > >
{
    typedef atSafeContainer< T, atInBinTree< K, T, LINK, COMPARE > > SafeContainer;
    typedef atSafeInBinTree< K, T, LINK, COMPARE > Container;

public:

    class Iterator : public SafeContainer::Iterator
    {
    public:

        const K* Key() const
        {
            return SafeContainer::Iterator::m_It.Key();
        }
    };

    typedef typename atInBinTree< K, T, LINK, COMPARE >::Policies Policies;

    atSafeInBinTree()
    {
    }

    atSafeInBinTree( const atSafeInBinTree< K, T, LINK, COMPARE >& rhs )
    {
        *this = rhs;
    }

    ~atSafeInBinTree()
    {
    }

    atSafeInBinTree< K, T, LINK, COMPARE >& operator=( const atSafeInBinTree< K, T, LINK, COMPARE >& rhs )
    {
        if( this != &rhs )
        {
            this->SafeContainer::operator=( rhs );
        }

        return *this;
    }

    void SetPolicies( const Policies& policies )
    {
        this->SafeContainer::m_UnsafeContainer.SetPolicies( policies );
    }

    const Policies& GetPolicies() const
    {
        return this->SafeContainer::m_UnsafeContainer.GetPolicies();
    }

    //PURPOSE
    //  Inserts a new item into the tree.  If an item with the same key
    //  already exists the insertion will fail.  Use Replace() instead.
    //RETURNS
    //  True if the item was inserted.
    bool Insert( const K& key, T* node )
    {
        return this->SafeContainer::m_UnsafeContainer.Insert( key, node );
    }

    //PURPOSE
    //  Removes the item with the given key.
    void Remove( const K& key )
    {
        T* item = this->SafeContainer::m_UnsafeContainer.Find( key );

        if( item )
        {
            this->Remove( item );
        }
    }

    //PURPOSE
    //  Provide access to the base class's Remove().
    using SafeContainer::Remove;

    //PURPOSE
    //  Returns the item with the given key.
    //  Returns 0 if the key is not in the tree.
    void Find( const K& key, Iterator* it )
    {
        T* t = this->SafeContainer::m_UnsafeContainer.Find( key );

        this->BeginIteration( it, t );
    }

    //PURPOSE
    //  Returns true if the tree contains an item with the given key.
    bool Contains( const K& key ) const
    {
        return this->SafeContainer::m_UnsafeContainer.Contains( key );
    }

    //PURPOSE
    //  Returns true if the tree contains the given item.
    bool Contains( const T* item ) const
    {
        return this->SafeContainer::m_UnsafeContainer.Contains( item );
    }
};

}   //namespace rage

#endif  //ATL_ATSAFEDLLISTSIMPLE_H
