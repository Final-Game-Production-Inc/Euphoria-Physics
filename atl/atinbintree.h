// 
// atl/atinbintree.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_ATINBINTREE_H
#define ATL_ATINBINTREE_H

#include <stddef.h>		// for size_t

namespace rage
{

//PURPOSE
//  Predicate used to order items inserted into a tree.
//  This can be specialized for UDTs (user defined types).
template< typename T >
class atLess
{
public:

    bool operator()( const T& t0, const T& t1 ) const
    {
        return t0 < t1;
    }
};

//PURPOSE
//  atInBinTree provides an implementation of an intrusive binary tree.
//  Classes which will be placed in a bintree must embed an instance
//  of atInBinTreeLink in their class declarations.  This is the
//  /intrusive/ aspect of the tree.
//
//  A separate atInBinTreeLink embedding is required for each tree
//  an instance of a class will simultaneously occupy.
//
//  Note that atInBinTree is a subclass of atInBinTreeTemplate.
//  In atInBinTreeTemplate the type of the link is actually a template
//  parameter.  This means that any type can satisfy the requirements
//  of a link as long as it has members m_Left, m_Right, m_Parent, and m_Key.
//
//  atInBinTree simply specializes atInBinTreeTemplate to use an instance
//  of atInBinTreeLink as the link.
//
//  class Foo
//  {
//  public:
//
//      //Link for tree A
//      atInBinTreeLink< Foo > m_BTLinkA;
//
//      //Link for tree B
//      atInBinTreeLink< Foo > m_BTLinkB;
//  };
//
//  //Tree A
//  atInBinTree< Foo, &Foo::m_BTLinkA > btA;
//
//  //Tree B
//  atInBinTree< Foo, &Foo::m_BTLinkB > btB;
//
//  Foo foo;
//
//  btA.Append( &foo );
//  btB.Append( &foo );
//
//  Note regarding duplicate keys:
//  In as much as a binary tree "sorts" its items, newer items with keys
//  that are equal to existing keys are orderd to come after the existing items.
//
//  If the tree allows duplicate keys then it becomes similar to a STL
//  multimap.

//PURPOSE
//  A unique instance of atInBinTreeLink must be embedded in a class for each
//  bintree that instances of that class will occupy.
template< typename K, typename T >
struct atInBinTreeLink
{
    atInBinTreeLink()
        : m_Left( 0 )
        , m_Right( 0 )
        , m_Parent( 0 )
    {
    }

    //Copy ctor and assignment do nothing because it's invalid to assign
    //one link to another.

    atInBinTreeLink( const atInBinTreeLink< K, T >& )
        : m_Left( 0 )
        , m_Right( 0 )
        , m_Parent( 0 )
    {
        FastAssert( "Invalid copy ctor on atInBinTreeLink" && false );
    }

    atInBinTreeLink< K, T >& operator=( const atInBinTreeLink< K, T >& )
    {
        FastAssert( "Invalid assignment on atInBinTreeLink" && false );

        return *this;
    }

    T* m_Left;
    T* m_Right;
    T* m_Parent;
    K m_Key;
};

//PURPOSE
//  The binary tree.
template< typename K, typename T, typename L, L T::*LINK, typename COMPARE = atLess< K > >
class atInBinTreeTemplate
{
    //Because of the hack we use to store the red/black value
    //in the LSB of the parent address, we need to ensure that we can't
    //have odd-valued addresses.
    CompileTimeAssert( sizeof( T ) > sizeof( char ) );

public:

    //PURPOSE
    //  Non-const iterator.
    class Iterator
    {
    public:

        Iterator()
            : m_Node( 0 )
        {
        }

        explicit Iterator( T* node )
            : m_Node( node )
        {
        }

        bool IsValid() const
        {
            return !!m_Node;
        }

        void Next()
        {
            if( m_Node ) m_Node = atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Successor( m_Node );
        }

        void Prev()
        {
            if( m_Node ) m_Node = atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Predecessor( m_Node );
        }

        T* Item()
        {
            return m_Node;
        }

        const T* Item() const
        {
            return m_Node;
        }

        const K* Key() const
        {
            return m_Node ? &( m_Node->*LINK ).m_Key : 0;
        }

    private:

        T* m_Node;
    };

    //PURPOSE
    //  Const iterator.
    class ConstIterator
    {
    public:

        explicit ConstIterator( const T* node )
            : m_It( const_cast< T* >( node ) )
        {
        }

        explicit ConstIterator( const Iterator& it )
            : m_It( it )
        {
        }

        bool IsValid() const
        {
            return m_It.IsValid();
        }

        void Next()
        {
            m_It.Next();
        }

        void Prev()
        {
            m_It.Prev();
        }

        const T* Item() const
        {
            return m_It.Item();
        }

        const K* Key() const
        {
            return m_It.Key();
        }

    private:

        Iterator m_It;
    };

    //PURPOSE
    //  Contains the policies for constructing a tree.
    struct Policies
    {
        Policies()
            : m_IsBalanced( true )
            , m_AllowDuplicateKeys( false )
        {
        }

        //If true the tree will self balance.
        bool m_IsBalanced           : 1;
        //If true the tree may contain duplicate keys.
        //This is similar to a STL multimap.
        bool m_AllowDuplicateKeys   : 1;
    };

    atInBinTreeTemplate();

    ~atInBinTreeTemplate();

    //PURPOSE
    //  Sets the policies for this container.
    void SetPolicies( const Policies& policies );

    //PURPOSE
    //  Returns the policies for this container.
    const Policies& GetPolicies() const;

    //PURPOSE
    //  Inserts a new item into the tree.  If an item with the same key
    //  already exists the insertion will fail unless the Policies object
    //  allows duplicate keys.
    //RETURNS
    //  True if the item was inserted.
    bool Insert( const K& key, T* node );

    //PURPOSE
    //  Removes all items with the given key.
    void Remove( const K& key );

    //PURPOSE
    //  Removes the given item.
    void Remove( T* node );

    //PURPOSE
    //  Removes the given item.
    void Remove( Iterator& it );

    //PURPOSE
    //  Removes all items from the tree.
    void Clear();

    //PURPOSE
    //  Returns the first item with the given key.
    //  Returns 0 if the key is not in the tree.
    T* Find( const K& key );
    const T* Find( const K& key ) const;

    //PURPOSE
    //  Returns true if the tree contains an item with the given key.
    bool Contains( const K& key ) const;

    //PURPOSE
    //  Returns true if the tree contains the given item.
    bool Contains( const T* node ) const;

    //PURPOSE
    //  Returns the item in the tree with the minimum key.
    T* GetFirst();
    const T* GetFirst() const;

    //PURPOSE
    //  Returns the item in the tree with the maximum key.
    T* GetLast();
    const T* GetLast() const;

    T* GetNext( T* item );
    const T* GetNext( const T* item ) const;

    T* GetPrev( T* item );
    const T* GetPrev( const T* item ) const;

    //PURPOSE
    //  Returns the number of items in the tree.
    int GetCount() const;

    //PURPOSE
    //  Returns true if there are no items in the tree.
    bool IsEmpty() const;

#if __DEV
    //PURPOSE
    //  Returns the number of nodes traversed in the last call to Find().
    unsigned GetLastFindCost() const { return m_LastFindCost; }
#endif  //__DEV

protected:

    enum
    {
        RED     = 0x00,
        BLACK   = 0x01
    };

    static K& Key( T* node );
    static T*& Left( T* node );
    static T*& Right( T* node );
    //static T*& Parent( T* node ) { return ( node->*LINK ).m_Parent; }
    //static unsigned& Color( T* node ) { return ( node->*LINK ).m_Color; }

    static T* GetParent( T* node );
    static const T* GetParent( const T* node );
    static void SetParent( T* node, T* parent );

    static unsigned GetColor( T* node );
    static void SetColor( T* node, unsigned color );

    static T* Minimum( T* node );
    static T* Maximum( T* node );

    static T* Successor( T* node );
    static T* Predecessor( T* node );

    void RotateRight( T* node );
    void RotateLeft( T* node );

private:

    //Disallow copy construction and assignment
    atInBinTreeTemplate( const atInBinTreeTemplate< K, T, L, LINK, COMPARE >& );
    atInBinTreeTemplate< K, T, L, LINK, COMPARE >& operator=( const atInBinTreeTemplate< K, T, L, LINK, COMPARE >& );

    T* m_Root;

    int m_Count;

    COMPARE m_Comp;

    DEV_ONLY( unsigned m_LastFindCost; )

    Policies m_Policies;
};

//Specialization of InBinTreeTemplate for bin trees that use InBinTreeLink
//as the type of the LINK memeber.
template< typename K, typename T, atInBinTreeLink< K, T > T::*LINK, typename COMPARE = atLess< K > >
class atInBinTree : public atInBinTreeTemplate< K, T, atInBinTreeLink< K, T >, LINK, COMPARE >
{
public:
};

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::atInBinTreeTemplate()
    : m_Root( 0 )
    , m_Count( 0 )
{
    DEV_ONLY( m_LastFindCost = 0 );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::~atInBinTreeTemplate()
{
    this->Clear();
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
void
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::SetPolicies( const Policies& policies )
{
    m_Policies = policies;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
const typename atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Policies&
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetPolicies() const
{
    return m_Policies;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
bool
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Insert( const K& key, T* node )
{
    FastAssert( 0 == GetParent( node ) && 0 == Left( node ) && 0 == Right( node ) );
    //Check that addresses don't have bit 1 set.
    FastAssert( ( size_t( node ) & 0x01 ) == 0 );

    bool success = true;

    if( !m_Root )
    {
        m_Root = node;
    }
    else
    {
        T* parent = m_Root;

        while( parent )
        {
            T* next;

            if( m_Comp( key, Key( parent ) ) )
            {
                next = Left( parent );

                if( !next )
                {
                    Left( parent ) = node;
                    break;
                }
            }
            //If allowing duplicates, items with equal keys go right.
            else if( m_Comp( Key( parent ), key ) || m_Policies.m_AllowDuplicateKeys )
            {
                next = Right( parent );

                if( !next )
                {
                    Right( parent ) = node;
                    break;
                }
            }
            else
            {
                Warningf( "Item with key exists" );
                success = false;
                break;
            }

            parent = next;
        }

        if( success )
        {
            SetParent( node, parent );
        }
    }

    if( success )
    {
        Key( node ) = key;

        if( m_Policies.m_IsBalanced )
        {
            SetColor( node, RED );
            T* parent = GetParent( node );
            T* gp = parent ? GetParent( parent ) : 0;

            while( gp && RED == GetColor( parent ) )
            {
                if( Left( gp ) == parent )
                {
                    T* uncle = Right( gp );

                    if( uncle && RED == GetColor( uncle ) )
                    {
                        SetColor( parent, BLACK );
                        SetColor( uncle, BLACK );
                        SetColor( gp, RED );

                        node = gp;
                    }
                    else
                    {
                        if( Right( parent ) == node )
                        {
                            node = parent;

                            this->RotateLeft( node );

                            parent = GetParent( node );
                            gp = parent ? GetParent( node ) : 0;
                        }

                        FastAssert( RED == GetColor( parent ) );
                        FastAssert( Left( parent ) == node );

                        SetColor( parent, BLACK );

                        if( gp )
                        {
                            SetColor( gp, RED );
                            this->RotateRight( gp );
                        }
                    }
                }
                else
                {
                    T* uncle = Left( gp );

                    if( uncle && RED == GetColor( uncle ) )
                    {
                        SetColor( parent, BLACK );
                        SetColor( uncle, BLACK );
                        SetColor( gp, RED );

                        node = gp;
                    }
                    else
                    {
                        if( Left( parent ) == node )
                        {
                            node = parent;

                            this->RotateRight( node );

                            parent = GetParent( node );
                            gp = parent ? GetParent( node ) : 0;
                        }

                        FastAssert( RED == GetColor( parent ) );
                        FastAssert( Right( parent ) == node );

                        SetColor( parent, BLACK );

                        if( gp )
                        {
                            SetColor( gp, RED );
                            this->RotateLeft( gp );
                        }
                    }
                }

                parent = GetParent( node );
                gp = parent ? GetParent( parent ) : 0;
            }

            SetColor( m_Root, BLACK );

            FastAssert( !parent || RED != GetColor( parent ) );
        }   //m_Policies.m_IsBalanced

        ++m_Count;
    }

    return success;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
void
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Remove( const K& key )
{
    T* node = this->Find( key );

    while( node )
    {
        this->Remove( node );

        node = this->Find( key );
    }
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
void
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Remove( T* node )
{
    //FIXME (KB) - verify that the node comes from this tree.

    T* replacer = 0;
    T* tmp = 0;

    //Find a node in the tree that will be used to replace the
    //removed node.
    if( !Left( node ) || !Right( node ) )
    {
        replacer = node;
    }
    //Alternate taking the replacer from the left and right branches
    //to avoid skewing the tree.
    else if( m_Count & 0x01 )
    {
        replacer = Successor( node );
    }
    else
    {
        replacer = Predecessor( node );
    }

    //Remove the replacer from the tree.
    if( Left( replacer ) )
    {
        tmp = Left( replacer );
    }
    else
    {
        tmp = Right( replacer );
    }

    if( tmp )
    {
        SetParent( tmp, GetParent( replacer ) );
    }

    if( !GetParent( replacer ) )
    {
        m_Root = tmp;
    }
    else if( Left( GetParent( replacer ) ) == replacer )
    {
        Left( GetParent( replacer ) ) = tmp;
    }
    else
    {
        Right( GetParent( replacer ) ) = tmp;
    }

    const unsigned replacerColor = GetColor( replacer );

    //Replace the removed node with the replacer.
    if( replacer != node )
    {
        if( m_Root == node )
        {
            Left( replacer ) = Left( node );
            Right( replacer ) = Right( node );

            m_Root = replacer;
        }
        else if( Left( GetParent( node ) ) == node )
        {
            Left( GetParent( node ) ) = replacer;
        }
        else
        {
            Right( GetParent( node ) ) = replacer;
        }

        SetParent( replacer, GetParent( node ) );
        Left( replacer ) = Left( node );
        Right( replacer ) = Right( node );
        SetColor( replacer, GetColor( node ) );

        if( Left( node ) )
        {
            SetParent( Left( node ), replacer );
        }

        if( Right( node ) )
        {
            SetParent( Right( node ), replacer );
        }
    }

    if( m_Policies.m_IsBalanced )
    {
        if( tmp && BLACK == replacerColor )
        {
            T* cur = tmp;

            while( GetParent( cur ) && BLACK == GetColor( cur ) )
            {
                if( Left( GetParent( cur ) ) == cur )
                {
                    T* sib = Right( GetParent( cur ) );

                    if( sib && RED == GetColor( sib ) )
                    {
                        SetColor( sib, BLACK );
                        SetColor( GetParent( cur ), RED );

                        this->RotateLeft( GetParent( cur ) );

                        sib = Right( GetParent( cur ) );
                    }

                    if( !sib )
                    {
                        cur = GetParent( cur );
                        continue;
                    }

                    if( ( !Left( sib ) || GetColor( Left( sib ) ) == BLACK ) &&
                        ( !Right( sib ) || GetColor( Right( sib ) ) == BLACK ) )
                    {
                        SetColor( sib, RED );
                        cur = GetParent( cur );
                    }
                    else
                    {
                        if( !Right( sib ) || GetColor( Right( sib ) ) == BLACK )
                        {
                            if( Left( sib ) )
                            {
                                SetColor( Left( sib ), BLACK );
                            }

                            SetColor( sib, RED );

                            this->RotateRight( sib );

                            sib = Right( GetParent( cur ) );
                        }

                        SetColor( sib, GetColor( GetParent( cur ) ) );
                        SetColor( GetParent( cur ), BLACK );

                        if( Right( sib ) )
                        {
                            SetColor( Right( sib ), BLACK );
                        }

                        this->RotateLeft( GetParent( cur ) );

                        cur = m_Root;
                    }
                }
                else
                {
                    T* sib = Left( GetParent( cur ) );

                    if( sib && RED == GetColor( sib ) )
                    {
                        SetColor( sib, BLACK );
                        SetColor( GetParent( cur ), RED );

                        this->RotateRight( GetParent( cur ) );

                        sib = Left( GetParent( cur ) );
                    }

                    if( !sib )
                    {
                        cur = GetParent( cur );
                        continue;
                    }

                    if( ( !Right( sib ) || GetColor( Right( sib ) ) == BLACK ) &&
                        ( !Left( sib ) || GetColor( Left( sib ) ) == BLACK ) )
                    {
                        SetColor( sib, RED );
                        cur = GetParent( cur );
                    }
                    else
                    {
                        if( !Left( sib ) || GetColor( Left( sib ) ) == BLACK )
                        {
                            if( Right( sib ) )
                            {
                                SetColor( Right( sib ), BLACK );
                            }

                            SetColor( sib, RED );

                            this->RotateLeft( sib );

                            sib = Left( GetParent( cur ) );
                        }

                        SetColor( sib, GetColor( GetParent( cur ) ) );
                        SetColor( GetParent( cur ), BLACK );

                        if( Left( sib ) )
                        {
                            SetColor( Left( sib ), BLACK );
                        }

                        this->RotateRight( GetParent( cur ) );

                        cur = m_Root;
                    }
                }
            }
        }
    }   //m_Policies.m_IsBalanced

    SetParent( node, 0 );
    
    Left( node ) = Right( node ) = 0;

    --m_Count;

    FastAssert( m_Count >= 0 );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
void
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Remove( Iterator& it )
{
    this->Remove( it.Item() );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
void
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Clear()
{
    while( !this->IsEmpty() )
    {
        this->Remove( this->GetFirst() );
    }
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Find( const K& key )
{
    T* node = m_Root;

    DEV_ONLY( m_LastFindCost = 0 );

    while( node )
    {
        if( m_Comp( Key( node ), key ) )
        {
            node = Right( node );
        }
        else if( m_Comp( key, Key( node ) ) )
        {
            node = Left( node );
        }
        else
        {
            break;
        }

        DEV_ONLY( ++m_LastFindCost );
    }

    if( node && m_Policies.m_AllowDuplicateKeys )
    {
        T* pred = Predecessor( node );

        while( pred &&
               !m_Comp( Key( pred ), key ) &&
               !m_Comp( key, Key( pred ) ) )
        {
            node = pred;
            pred = Predecessor( node );

            DEV_ONLY( ++m_LastFindCost );
        }
    }

    return node;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
const T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Find( const K& key ) const
{
    return const_cast< atInBinTreeTemplate< K, T, L, LINK, COMPARE >* >( this )->Find( key );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
bool
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Contains( const K& key ) const
{
    return ( 0 != this->Find( key ) );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
bool
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Contains( const T* item ) const
{
    const T* root = item;
    const T* parent = GetParent( root );

    while( parent )
    {
        root = parent;
        parent = GetParent( root );
    }

    return ( root == m_Root );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetFirst()
{
    return m_Root ? Minimum( m_Root ) : 0;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
const T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetFirst() const
{
    return m_Root ? Minimum( m_Root ) : 0;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetLast()
{
    return m_Root ? Maximum( m_Root ) : 0;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
const T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetLast() const
{
    return m_Root ? Maximum( m_Root ) : 0;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetNext( T* item )
{
    return Successor( item );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
const T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetNext( const T* item ) const
{
    return Successor( item );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetPrev( T* item )
{
    return Predecessor( item );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
const T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetPrev( const T* item ) const
{
    return Predecessor( item );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
int
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetCount() const
{
    return m_Count;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
bool
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::IsEmpty() const
{
    FastAssert( m_Count >= 0 );

    return m_Count <= 0;
}

//protected:

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
K&
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Key( T* node )
{
    return ( node->*LINK ).m_Key;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*&
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Left( T* node )
{
    return ( node->*LINK ).m_Left;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*&
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Right( T* node )
{
    return ( node->*LINK ).m_Right;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetParent( T* node )
{
    return ( T* ) ( size_t( ( node->*LINK ).m_Parent ) & ~0x01L );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
const T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetParent( const T* node )
{
    return ( const T* ) ( size_t( ( node->*LINK ).m_Parent ) & ~0x01L );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
void
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::SetParent( T* node, T* parent )
{
    const size_t color = size_t( ( node->*LINK ).m_Parent ) & 0x01L;

    ( node->*LINK ).m_Parent = ( T* ) ( size_t( parent ) | color );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
unsigned
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::GetColor( T* node )
{
    return (unsigned) (size_t( ( node->*LINK ).m_Parent ) & 0x01);
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
void
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::SetColor( T* node, unsigned color )
{
    FastAssert( color <= 1U );

    size_t n = size_t( ( node->*LINK ).m_Parent ) & ~0x01UL;

    ( node->*LINK ).m_Parent = ( T* ) ( n | color );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Minimum( T* node )
{
    while( Left( node ) )
    {
        node = Left( node );
    }

    return node;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Maximum( T* node )
{
    while( Right( node ) )
    {
        node = Right( node );
    }

    return node;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Successor( T* node )
{
    T* successor = 0;

    if( Right( node ) )
    {
        successor = Minimum( Right( node ) );
    }
    else
    {
        successor = GetParent( node );

        while( successor && Right( successor ) == node )
        {
            node = successor;
            successor = GetParent( node );
        }
    }

    return successor;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
T*
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::Predecessor( T* node )
{
    T* predecessor = 0;

    if( Left( node ) )
    {
        predecessor = Maximum( Left( node ) );
    }
    else
    {
        predecessor = GetParent( node );

        while( predecessor && Left( predecessor ) == node )
        {
            node = predecessor;
            predecessor = GetParent( node );
        }
    }

    return predecessor;
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
void
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::RotateRight( T* node )
{
    T* tmp = Left( node );

    Left( node ) = Right( tmp );

    if( Right( tmp ) )
    {
        SetParent( Right( tmp ), node );
    }

    SetParent( tmp, GetParent( node ) );

    if( !GetParent( node ) )
    {
        m_Root = tmp;
    }
    else
    {
        if( Right( GetParent( node ) ) == node )
        {
            Right( GetParent( node ) ) = tmp;
        }
        else
        {
            Left( GetParent( node ) ) = tmp;
        }
    }

    Right( tmp ) = node;
    SetParent( node, tmp );
}

template< typename K, typename T, typename L, L T::*LINK, typename COMPARE >
void
atInBinTreeTemplate< K, T, L, LINK, COMPARE >::RotateLeft( T* node )
{
    T* tmp = Right( node );

    Right( node ) = Left( tmp );

    if( Left( tmp ) )
    {
        SetParent( Left( tmp ), node );
    }

    SetParent( tmp, GetParent( node ) );

    if( !GetParent( node ) )
    {
        m_Root = tmp;
    }
    else
    {
        if( Left( GetParent( node ) ) == node )
        {
            Left( GetParent( node ) ) = tmp;
        }
        else
        {
            Right( GetParent( node ) ) = tmp;
        }
    }

    Left( tmp ) = node;
    SetParent( node, tmp );
}

}   //namespace rage

#endif  //ATL_ATINBINTREE_H
