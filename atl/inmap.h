// 
// atl/inmap.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_INMAP_H
#define ATL_INMAP_H

#include <iterator>
#include <algorithm>
#include <functional>
#include <stddef.h>		// for ptrdiff_t when not using STLport

namespace rage
{

///////////////////////////////////////////////////////////////////////////////
//  inmap
//
//  inmap is a template class implementing an intrusive binary search tree
//  and is intended to be a near drop-in replacement for std::map.
//
//  An intrusive map is a map in which the list link nodes are embedded
//  in the items that are contained in the map.  Unlike std::map, inmap
//  operations do not allocate memory.
//
//  Conversely, it must be known up front how many maps in which an item
//  will be contained simultaneously.  For each map there must be a unique
//  inmap_node embedded in the the item.
//
//  The API for inmap is a subset of that for std::map.  The missing
//  functions cannot be straightforwardly implemented for an intrusive map.
//
//  Below is an example declaring a struct whose instances will be
//  contained in two maps, followed by declarations of the maps that
//  will contain them:
//
//  struct Foo
//  {
//      inmap_node< int, Foo > m_MapA;
//      inmap_node< int, Foo > m_MapB;
//  };
//
//  typedef inmap< int, Foo, &Foo::m_MapA > InMapA;
//  typedef inmap< int, Foo, &Foo::m_MapB > InMapB;
//
//  An analogous std::map declaration would look like this:
//
//  typedef std::map< int, Foo* > StdMap;
//
//  In otherwords, the items of an inmap are contained as pointers.
//
//  Note that the reference type of an inmap is not a true C++ reference.
//

//PURPOSE
//  Embed within the target item a unique instance of inmap_node for each
//  inmap a target item will occupy.
template< typename K, typename T >
class inmap_node
{
public:

    inmap_node()
        : m_right( 0 )
        , m_left( 0 )
        , m_parent( 0 )
    {
        ASSERT_ONLY(m_owner = 0);
    }

    //Copy constructor intentionally left empty.
    inmap_node( const inmap_node< K, T >& )
        : m_right( 0 )
        , m_left( 0 )
        , m_parent( 0 )
    {
        ASSERT_ONLY(m_owner = 0);
    }

#if __ASSERT
    ~inmap_node()
    {
        Assert(!m_right);
        Assert(!m_left);
        Assert(!m_parent);
        Assert(!m_owner);
    }
#endif  //__ASSERT

    //Assigment operator intentionally left empty.
    inmap_node< K, T >& operator=( const inmap_node< K, T >& )
    {
        return *this;
    }

    // WARNING!!!! Be careful when changing the size of inmap_nodes
    // inmap_nodes are used in resourced objects, though they themselves don't
    // get fixed up.
    // Changing the size will likely mean having to rebuild resources.

    T* m_right;
    T* m_left;
    T* m_parent;
    K m_key;

    ASSERT_ONLY(void* m_owner;) // Only used in assert builds, but here all the time to keep resource sizes constant
};

template< typename K, typename T, inmap_node< K, T > T::*NODE >
struct inmap_detail
{
    //Store the node color as the LSB of one of the pointers.
    //Use TProxy and CProxy (color proxy) to proxy a T* that has might
    //have its LSB set.

    struct TProxy
    {
        explicit TProxy( T*& t )
            : m_T( t )
        {
        }

        operator T*()
        {
            return ( T* ) ( size_t( m_T ) & ~size_t( 0x01 ) );
        }

        TProxy& operator=( T* t )
        {
            FastAssert( !( size_t( t ) & 0x01 ) );
            m_T = ( T* ) ( size_t( t ) | ( size_t( m_T ) & size_t( 0x01 ) ) );
            return *this;
        }

        TProxy& operator=( TProxy& that )
        {
            if( this != &that )
            {
                m_T = that.m_T;
            }
            return *this;
        }

        T*& m_T;

    private:
    };

    struct CProxy
    {
        explicit CProxy( T*& t )
            : m_T( t )
        {
        }

        operator bool()
        {
            return bool( size_t( m_T ) & 0x01 );
        }

        CProxy& operator=( const bool color )
        {
            m_T = ( T* ) ( size_t( color ) | ( size_t( m_T ) & ~size_t( 0x01 ) ) );
            return *this;
        }

        CProxy& operator=( CProxy& that )
        {
            if( this != &that )
            {
                m_T = that.m_T;
            }
            return *this;
        }

        T*& m_T;

    private:
    };

    static TProxy right( T* t ) { return TProxy( ( t->*NODE ).m_right ); }
    //static T*& right( T* t ) { return ( t->*NODE ).m_right; }
    //static const T* right( const T* t ) { return ( t->*NODE ).m_right; }
    static T*& left( T* t ) { return ( t->*NODE ).m_left; }
    //static const T* left( const T* t ) { return ( t->*NODE ).m_left; }
    static T*& parent( T* t ) { return ( t->*NODE ).m_parent; }
    //static const T* parent( const T* t ) { return ( t->*NODE ).m_parent; }
    static T*& grandparent( T* t ) { return parent( parent( t ) ); }
    //static const T* grandparent( const T* t ) { return parent( parent( t ) ); }
    static T*& uncle( T* t )
    {
        T* p = parent( t );
        T* gp = parent( p );
        T* lu = left( gp );
        return p == lu ? right( gp ) : lu;
    }
    //static const T* uncle( const T* t ) { return uncle( const_cast< T* >( t ) ); }
    static CProxy color( T* t ) { return CProxy( ( t->*NODE ).m_right ); }
    //static bool& color( T* t ) { return ( t->*NODE ).m_color; }
    //static bool color( const T* t ) { return ( t->*NODE ).m_color; }

    static K& key( T* t ) { return ( t->*NODE ).m_key; }
    static K key( const T* t ) { return ( t->*NODE ).m_key; }

    static T* minimum( T* t )
    {
        for( T* l = left( t ); l; t = l, l = left( t ) )
        {
        }

        return t;
    }
    static const T* minimum( const T* t ) { return minimum( const_cast< T* >( t ) ); }

    static T* maximum( T* t )
    {
        for( T* r = right( t ); r; t = r, r = right( t ) )
        {
        }

        return t;
    }
    static const T* maximum( const T* t ) { return maximum( const_cast< T* >( t ) ); }

    static T* predecessor( T* t )
    {
        T* p = left( t );

        if( p )
        {
            p = maximum( p );
        }
        else
        {
            T* s = t;
            p = parent( s );

            while( p && s == left( p ) )
            {
                s = p;
                p = parent( p );
            }
        }

        return p;
    }
    static const T* predecessor( const T* t ) { return predecessor( const_cast< T* >( t ) ); }

    static T* successor( T* t )
    {
        T* s = right( t );

        if( s )
        {
            s = minimum( s );
        }
        else
        {
            T* p = t;
            s = parent( p );

            while( s && p == right( s ) )
            {
                p = s;
                s = parent( s );
            }
        }

        return s;
    }
    static const T* successor( const T* t ) { return successor( const_cast< T* >( t ) ); }

    //PURPOSE
    //  Template we'll use to create reverse_iterators from iterators
    template< typename ITER >
    class reverse_iterator
    {
    public:

        typedef typename ITER::iterator_category iterator_category;
        typedef typename ITER::value_type value_type;
        typedef typename ITER::difference_type difference_type;
        typedef typename ITER::pointer pointer;
        typedef typename ITER::reference reference;

        reverse_iterator()
        {
        }

        explicit reverse_iterator( const ITER& it )
            : m_It( it )
        {
        }

        template< typename OTHER >
        reverse_iterator( const reverse_iterator< OTHER >& that )
            : m_It( that.base() )
        {
        }

        template< typename OTHER >
        reverse_iterator< ITER >& operator=( const reverse_iterator< OTHER >& that )
        {
            m_It = that.base();
            return *this;
        }

        ITER base() const
        {
            return m_It;
        }

        reverse_iterator operator++()
        {
            --m_It;
            return *this;
        }

        reverse_iterator operator--()
        {
            ++m_It;
            return *this;
        }

        reverse_iterator operator++( const int )
        {
            reverse_iterator tmp = *this;
            --m_It;
            return tmp;
        }

        reverse_iterator operator--( const int )
        {
            reverse_iterator tmp = *this;
            ++m_It;
            return tmp;
        }

        pointer operator->() const
        {
            ITER tmp = m_It;
            return ( --tmp ).operator->();
        }

        reference operator*() const
        {
            ITER tmp = m_It;
            return ( --tmp ).operator*();
        }

        bool operator==( const reverse_iterator& rhs ) const
        {
            return m_It == rhs.m_It;
        }

        bool operator!=( const reverse_iterator& rhs ) const
        {
            return m_It != rhs.m_It;
        }

    private:

        ITER m_It;
    };
};  //struct inmap_detail

template< typename K, typename T, inmap_node< K, T > T::*NODE, typename P >
class inmap_base
{
    typedef inmap_base< K, T, NODE, P > MapType;

    typedef inmap_detail< K, T, NODE > detail;

public:

    enum
    {
        ALLOW_DUP_KEYS  = P::ALLOW_DUP_KEYS
    };

    class const_iterator;

    class iterator
    {
        friend class inmap_base< K, T, NODE, P >;
        friend class const_iterator;

        bool before_begin() const
		{
			const size_t one = 0x1;
            return ( ( ( size_t ) m_map ) & ~one )
                   && ( ( ( size_t ) m_map ) & one );
        }

        void set_before_begin()
		{
			const size_t one = 0x1;
            m_map = ( MapType* ) ( ( ( size_t ) m_map ) | one );
        }

        void clear_before_begin()
		{
			const size_t one = 0x1;
            m_map = ( MapType* ) ( ( ( size_t ) m_map ) & ~one );
        }

        MapType* map()
        {
			const size_t one = 0x1;
            return ( MapType* ) ( ( ( size_t ) m_map ) & ~one );
        }

        MapType* map() const
		{
			const size_t one = 0x1;
            return ( MapType* ) ( ( ( size_t ) m_map ) & ~one );
        }

    public:

        typedef std::bidirectional_iterator_tag iterator_category;
        typedef std::pair< K, T* > value_type;
        typedef ptrdiff_t difference_type;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        //Can't make this a true reference - the intrusive nature
        //of the list prevents us from changing the value of a contained item.
        typedef value_type reference;

        iterator()
            : m_map( 0 )
        {
            m_Value.second = 0;
        }

        iterator operator++()
        {
            if( m_Value.second )
            {
                m_Value.second = detail::successor( m_Value.second );
            }
            else if( this->before_begin() )
            {
                //We're one before begin - go to begin
                this->clear_before_begin();
                m_Value.second = detail::minimum( m_map->m_root );
            }

            if( m_Value.second )
            {
                m_Value.first = detail::key( m_Value.second );
            }

            return *this;
        }

        iterator operator--()
        {
            if( m_Value.second )
            {
                m_Value.second = detail::predecessor( m_Value.second );

                if( !m_Value.second )
                {
                    //Went past begin.
                    this->set_before_begin();
                }
            }
            else if( m_map && !this->before_begin() )
            {
                //We're at the end - go to one before the end
                m_Value.second = detail::maximum( m_map->m_root );
            }

            if( m_Value.second )
            {
                m_Value.first = detail::key( m_Value.second );
            }

            return *this;
        }

        iterator operator++( const int )
        {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }

        iterator operator--( const int )
        {
            iterator tmp = *this;
            --*this;
            return tmp;
        }

        pointer operator->()
        {
            return &m_Value;
        }

        const_pointer operator->() const
        {
            return &m_Value;
        }

        reference operator*() const
        {
            FastAssert( m_Value.second );
            return m_Value;
        }

        bool operator==( const iterator& rhs ) const
        {
            return rhs.m_Value.second == m_Value.second;
        }

        bool operator!=( const iterator& rhs ) const
        {
            return rhs.m_Value.second != m_Value.second;
        }

        bool operator==( const const_iterator& rhs ) const
        {
            return rhs == *this;
        }

        bool operator!=( const const_iterator& rhs ) const
        {
            return rhs != *this;
        }

    private:

        iterator( T* t, MapType* m )
            : m_map( m )
        {
            if( t )
            {
                m_Value.first = detail::key( t );
                m_Value.second = t;
            }
            else
            {
                m_Value.second = 0;
            }
        }

        value_type m_Value;
        MapType* m_map;
    };

    class const_iterator
    {
        friend class inmap_base< K, T, NODE, P >;

        bool before_begin() const
        {
            return ( ( ( size_t ) m_map ) & ~0x01u )
                && ( ( ( size_t ) m_map ) & 0x01u );
        }

        void set_before_begin()
        {
            m_map = ( MapType* ) ( ( ( size_t ) m_map ) | 0x01u );
        }

        void clear_before_begin()
        {
            m_map = ( MapType* ) ( ( ( size_t ) m_map ) & ~0x01u );
        }

        MapType* map()
        {
            return ( MapType* ) ( ( ( size_t ) m_map ) & ~0x01u );
        }

        MapType* map() const
        {
            return ( MapType* ) ( ( ( size_t ) m_map ) & ~0x01u );
        }

    public:

        typedef std::bidirectional_iterator_tag iterator_category;
        typedef std::pair< K, const T* > value_type;
        typedef ptrdiff_t difference_type;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        //Can't make this a true reference - the intrusive nature
        //of the list prevents us from changing the value of a contained item.
        typedef value_type reference;

        const_iterator()
            : m_map( 0 )
        {
            m_Value.second = 0;
        }

        const_iterator( const iterator& it )
            : m_map( it.m_map )
        {
            this->m_Value.first = it.m_Value.first;
            this->m_Value.second = it.m_Value.second;
        }

        const_iterator operator++()
        {
            if( m_Value.second )
            {
                m_Value.second = detail::successor( m_Value.second );
            }
            else if( this->before_begin() )
            {
                //We're one before begin - go to begin
                this->clear_before_begin();
                m_Value.second = detail::minimum( m_map->m_root );
            }

            if( m_Value.second )
            {
                m_Value.first = detail::key( m_Value.second );
            }

            return *this;
        }

        const_iterator operator--()
        {
            if( m_Value.second )
            {
                m_Value.second = detail::predecessor( m_Value.second );

                if( !m_Value.second )
                {
                    //Went past begin.
                    this->set_before_begin();
                }
            }
            else if( m_map && !this->before_begin() )
            {
                //We're at the end - go to one before the end
                m_Value.second = detail::maximum( m_map->m_root );
            }

            if( m_Value.second )
            {
                m_Value.first = detail::key( m_Value.second );
            }

            return *this;
        }

        const_iterator operator++( const int )
        {
            const_iterator tmp = *this;
            ++*this;
            return tmp;
        }

        const_iterator operator--( const int )
        {
            const_iterator tmp = *this;
            --*this;
            return tmp;
        }

        pointer operator->()
        {
            return &m_Value;
        }

        const_pointer operator->() const
        {
            return &m_Value;
        }

        reference operator*() const
        {
            FastAssert( m_Value.second );
            return m_Value;
        }

        bool operator==( const const_iterator& rhs ) const
        {
            return rhs.m_Value.second == m_Value.second;
        }

        bool operator!=( const const_iterator& rhs ) const
        {
            return rhs.m_Value.second != m_Value.second;
        }

    private:

        const_iterator( T* t, const MapType* m )
            : m_map( m )
        {
            if( t )
            {
                m_Value.first = detail::key( t );
                m_Value.second = t;
            }
            else
            {
                m_Value.second = 0;
            }
        }

        value_type m_Value;
        const MapType* m_map;
    };

    typedef typename detail::template reverse_iterator< iterator > reverse_iterator;
    typedef typename detail::template reverse_iterator< const_iterator > const_reverse_iterator;

    typedef std::pair< K, T* > value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef T* mapped_type;
    typedef K key_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef typename P::Predicate key_compare;

    struct value_compare
    {
        bool operator()( const value_type& v0, const value_type& v1 )
        {
            P pred;
            return pred( v0.first, v1.first );
        }
    };

    inmap_base()
        : m_root( 0 )
        , m_size( 0 )
    {
    }

    iterator begin()
    {
        return iterator( m_root ? minimum( m_root ) : 0, this );
    }

    const_iterator begin() const
    {
        return const_iterator( m_root ? minimum( m_root ) : 0, this );
    }

    iterator end()
    {
        return iterator( 0, this );
    }

    const_iterator end() const
    {
        return const_iterator( 0, this );
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator( this->end() );
    }

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator( this->end() );
    }

    reverse_iterator rend()
    {
        return reverse_iterator( this->begin() );
    }

    const_reverse_iterator rend() const
    {
        return const_reverse_iterator( this->begin() );
    }

    void clear()
    {
        while( !this->empty() )
        {
            this->erase( this->begin() );
        }
    }

    size_type count( const K& key ) const
    {
        const_iterator it = this->find( key );
        const_iterator stop = this->end();

        int n = 0;

        for( ; stop != it && key == it->first; ++it )
        {
            ++n;
        }

        return n;
    }

    bool empty() const
    {
        return 0 == m_size;
    }

    std::pair< iterator, bool > insert( const value_type& item )
    {
        return this->insert( item.first, item.second );
    }

    std::pair< iterator, bool > insert( const K& k, T* item )
    {
        FastAssert( 0 == owner(item) );
        FastAssert( !left( item ) && !right( item ) && !parent( item ) );

        T* n = 0;

        if( !m_root )
        {
            n = m_root = item;
        }
        else
        {
            T* cur = m_root;

            while( cur )
            {
                T* next;

                if( m_pred( k, key( cur ) ) )
                {
                    next = left( cur );

                    if( !next )
                    {
                        left( cur ) = item;
                        parent( item ) = cur;
                        n = item;
                    }
                }
                //If allowing duplicates, items with equal keys go right.
                else if( m_pred( key( cur ), k ) || ALLOW_DUP_KEYS )
                {
                    next = right( cur );

                    if( !next )
                    {
                        right( cur ) = item;
                        parent( item ) = cur;
                        n = item;
                    }
                }
                else
                {
                    break;
                }

                cur = next;
            }
        }

        if( n )
        {
            FastAssert( n == item );

            color( item ) = true;
            insertFixup( item );

            key( item ) = k;

            ASSERT_ONLY( owner(item) = this );

            ++m_size;
        }

        return std::pair< iterator, bool >( iterator( n, this ), 0 != n );
    }

    key_compare key_comp() const
    {
        return m_pred;
    }

    iterator find( const K& k, size_t* cost = 0 )
    {
        iterator it = this->lower_bound( k, cost );

        return ( this->end() == it || m_pred( k, it->first ) ) ? this->end() : it;
    }

    const_iterator find( const K& k, size_t* cost = 0 ) const
    {
        const_iterator it = this->lower_bound( k, cost );

        return ( this->end() == it || m_pred( k, it->first ) ) ? this->end() : it;
    }

    iterator erase( T* item )
    {
        FastAssert( this == owner(item) );
        FastAssert( left( item ) || right( item ) || parent( item ) || m_root == item );
        FastAssert( m_size > 0 );

        iterator next( item, this );
        ++next;

        T* y;

        if( !left( item ) || !right( item ) )
        {
            y = item;
        }
        else if( m_size & 0x01 )
        {
            y = right( item );
            while( left( y ) ) y = left( y );
        }
        else
        {
            y = left( item );
            while( right( y ) ) y = right( y );
        }

        T* x;

        if( left( y ) )
        {
            x = left( y );
        }
        else
        {
            x = right( y );
        }

        if( x )
        {
            parent( x ) = parent( y );
        }

        if( !parent( y ) )
        {
            m_root = x;
        }
        else if( left( parent( y ) ) == y )
        {
            left( parent( y ) ) = x;
        }
        else
        {
            right( parent( y ) ) = x;
        }

        if( item != y )
        {
            if( item == m_root )
            {
                m_root = y;
            }
            else if( left( parent( item ) ) == item )
            {
                left( parent( item ) ) = y;
            }
            else
            {
                right( parent( item ) ) = y;
            }

            parent( y ) = parent( item );
            left( y ) = left( item );
            right( y ) = right( item );
            color( y ) = color( item );

            if( left( item ) )
            {
                parent( left( item ) ) = y;
            }

            if( right( item ) )
            {
                parent( right( item ) ) = y;
            }

            FastAssert( !left( y )
                || m_pred( key( left( y ) ), key( y ) )
                || ( ALLOW_DUP_KEYS && !m_pred( key( y ), key( left( y ) ) ) ) );
            FastAssert( !right( y )
                || m_pred( key( y ), key( right( y ) ) )
                || !m_pred( key( right( y ) ), key( y ) ) );
        }

        if( x && color( y ) == false )
        {
            this->eraseFixup( x );
        }

        //Use the raw pointer here, not the accessors.
        //Using the accessors can leave the bit zero set to 1, which
        //will compare not equal to NULL, and we don't want that.
        ( item->*NODE ).m_parent =
            ( item->*NODE ).m_left =
            ( item->*NODE ).m_right = 0;

        ASSERT_ONLY( ( item->*NODE ).m_owner = 0 );

        --m_size;

        FastAssert( m_root || !m_size );

        return next;
    }

    iterator erase( iterator where )
    {
        FastAssert( this == where.map() );
        return where->second ? this->erase( where->second ) : this->end();
    }

    iterator erase( iterator first, iterator last )
    {
        if( this->begin() == first && this->end() == last )
        {
            this->clear();
            first = this->end();
        }
        else
        {
            while( first != last )
            {
                first = this->erase( first );
            }
        }

        return first;
    }

    size_type erase( const K& k )
    {
        size_t count = 0;
        iterator it = this->find( k );
        const_iterator stop = this->end();

        while( stop != it && k == it->first )
        {
            it = this->erase( it );
            ++count;
        }

        return count;
    }

    std::pair< iterator, iterator > equal_range( const K& k )
    {
        return std::pair< iterator, iterator >( this->lower_bound( k ), this->upper_bound( k ) );
    }

    std::pair< const_iterator, const_iterator > equal_range( const K& k ) const
    {
        return std::pair< const_iterator, const_iterator >( this->lower_bound( k ), this->upper_bound( k ) );
    }

    iterator lower_bound( const K& k, size_t* cost = 0 )
    {
        T* cur = m_root;
        T* n = 0;
        size_t findCost = 0;

        while( cur )
        {
            if( m_pred( key( cur ), k ) )
            {
                cur = right( cur );
            }
            else
            {
                n = cur;
                cur = left( cur );
            }

            ++findCost;
        }

        if( ALLOW_DUP_KEYS && n )
        {
            cur = predecessor( n );

            while( cur
                && !m_pred( key( cur ), k )
                && !m_pred( k, key( cur ) ) )
            {
                n = cur;
                cur = predecessor( cur );

                ++findCost;
            }
        }

        if(cost) *cost = findCost;

        return iterator( n, this );
    }

    const_iterator lower_bound( const K& k, size_t* cost = 0 ) const
    {
        return const_cast< MapType* >( this )->lower_bound( k, cost );
    }

    iterator upper_bound( const K& k, size_t* cost = 0 )
    {
        T* cur = m_root;
        T* n = 0;
        size_t findCost = 0;

        while( cur )
        {
            if( m_pred( k, key( cur ) ) )
            {
                n = cur;
                cur = left( cur );
            }
            else
            {
                cur = right( cur );
            }

            ++findCost;
        }

        if(cost) *cost = findCost;

        return iterator( n, this );
    }

    const_iterator upper_bound( const K& k, size_t* cost = 0 ) const
    {
        return const_cast< MapType* >( this )->upper_bound( k, cost );
    }

    size_type max_size() const
    {
        const size_type maxsize = size_type( -1 ) >> 1;
        return ( 0 < maxsize ? maxsize : 1 );
    }

    size_type size() const
    {
        return m_size;
    }

    value_compare value_comp() const
    {
        value_compare vc;
        return vc;
    }

private:

    void insertFixup( T* x )
    {
        /* check Red-Black properties */
        while( x != m_root && color( parent( x ) ) == true )
        {
            /* we have a violation */
            if( parent( x ) == left( grandparent( x ) ) )
            {
                T* y = right( grandparent( x ) );
                if( y && color( y ) == true )
                {
                    /* uncle is RED */
                    color( parent( x ) ) = false;
                    color( y ) = false;
                    color( grandparent( x ) ) = true;
                    x = grandparent( x );
                }
                else
                {
                    /* uncle is BLACK */
                    if( x == right( parent( x ) ) )
                    {
                        /* make x a left child */
                        x = parent( x );
                        rotl(x);
                    }

                    /* recolor and rotate */
                    color( parent( x ) ) = false;
                    color( grandparent( x ) ) = true;
                    rotr( grandparent( x ) );
                }
            }
            else
            {
                /* mirror image of above code */
                T* y = left( grandparent( x ) );
                if( y && color( y ) == true )
                {
                    /* uncle is RED */
                    color( parent( x ) ) = false;
                    color( y ) = false;
                    color( grandparent( x ) ) = true;
                    x = grandparent( x );
                }
                else
                {
                    /* uncle is BLACK */
                    if( x == left( parent( x ) ) )
                    {
                        x = parent( x );
                        rotr( x );
                    }
                    color( parent( x ) ) = false;
                    color( grandparent( x ) ) = true;
                    rotl( grandparent( x ) );
                }
            }
        }

        color( m_root ) = false;
    }

    void eraseFixup( T* x )
    {
        while (x != m_root && color( x ) == false )
        {
            if( x == left( parent( x ) ) )
            {
                T* w = right( parent( x ) );
                if( w && color( w ) == true )
                {
                    color( w ) = false;
                    color( parent( x ) ) = true;
                    rotl( parent( x ) );
                    w = right( parent( x ) );
                }

                if( !w )
                {
                    x = parent( x );
                    continue;
                }

                if( ( !left( w ) || color( left( w ) ) == false )
                    && ( !right( w ) || color( right( w ) ) ==  false ) )
                {
                    color( w ) = true;
                    x = parent( x );
                }
                else
                {
                    if( !right( w ) || color( right( w ) ) == false )
                    {
                        color( left( w ) ) = false;
                        color( w ) = true;
                        rotr( w );
                        w = right( parent( x ) );
                    }

                    color( w ) = color( parent( x ) );
                    color( parent( x ) ) = false;
                    color( right( w ) ) = false;
                    rotl( parent( x ) );
                    x = m_root;
                }
            }
            else
            {
                T* w = left( parent( x ) );
                if( w && color( w ) == true )
                {
                    color( w ) = false;
                    color( parent( x ) ) = true;
                    rotr( parent( x ) );
                    w = left( parent( x ) );
                }

                if( !w )
                {
                    x = parent( x );
                    continue;
                }

                if( ( !right( w ) || color( right( w ) ) == false )
                    && ( !left( w ) || color( left( w ) ) == false ) )
                {
                    color( w ) = true;
                    x = parent( x );
                }
                else
                {
                    if( !left( w ) || color( left( w ) ) == false )
                    {
                        color( right( w ) ) = false;
                        color( w ) = true;
                        rotl( w );
                        w = left( parent( x ) );
                    }

                    color( w ) = color( parent( x ) );
                    color( parent( x ) ) = false;
                    color( left( w ) ) = false;
                    rotr( parent( x ) );
                    x = m_root;
                }
            }
        }

        color( x ) = false;
    }

    void rotl( T* t )
    {
        T* n = right( t );
        right( t ) = left( n );

        if( left( n ) )
        {
            parent( left( n ) ) = t;
        }

        parent( n ) = parent( t );

        if( t == m_root )
        {
            m_root = n;
        }
        else if( t == left( parent( t ) ) )
        {
            left( parent( t ) ) = n;
        }
        else
        {
            right( parent( t ) ) = n;
        }

        left( n ) = t;
        parent( t ) = n;
    }

    void rotr( T* t )
    {
        T* n = left( t );
        left( t ) = right( n );

        if( right( n ) )
        {
            parent( right( n ) ) = t;
        }

        parent( n ) = parent( t );

        if( t == m_root )
        {
            m_root = n;
        }
        else if( t == right( parent( t ) ) )
        {
            right( parent( t ) ) = n;
        }
        else
        {
            left( parent( t ) ) = n;
        }

        right( n ) = t;
        parent( t ) = n;
    }

    static typename detail::TProxy right( T* t ) { return detail::right( t ); }
    static T*& left( T* t ) { return detail::left( t ); }
    static T*& parent( T* t ) { return detail::parent( t ); }
    static T*& grandparent( T* t ) { return detail::grandparent( t ); }
    static T*& uncle( T* t ) { return detail::uncle( t ); }
    static K& key( T* t ) { return detail::key( t ); }
    static typename detail::CProxy color( T* t ) { return detail::color( t ); }
    static T* minimum( T* t ) { return detail::minimum( t ); }
    static T* maximum( T* t ) { return detail::maximum( t ); }
    static T* predecessor( T* t ) { return detail::predecessor( t ); }
    static T* successor( T* t ) { return detail::successor( t ); }
#if __ASSERT
    static void*& owner( T* t ) { return ( t->*NODE ).m_owner; }
#endif  //__ASSERT

    T* m_root;
    size_type m_size;
    key_compare m_pred;

    //Non-copyable
    inmap_base( const inmap_base< K, T, NODE, P >& );
    inmap_base& operator=( const inmap_base< K, T, NODE, P >& );
};

template< typename PRED >
struct inmap_policies
{
    typedef PRED Predicate;
    enum
    {
        ALLOW_DUP_KEYS      = false
    };
};

template< typename PRED >
struct inmultimap_policies
{
    typedef PRED Predicate;
    enum
    {
        ALLOW_DUP_KEYS      = true
    };
};

template< typename K, typename T, inmap_node< K, T > T::*NODE, typename P = std::less< K > >
class inmap : public inmap_base< K, T, NODE, inmap_policies< P > >
{
public:

    inmap()
    {
    }

private:

    //Non-copyable
    inmap( const inmap< K, T, NODE, P >& );
    inmap& operator=( const inmap< K, T, NODE, P >& );
};

template< typename K, typename T, inmap_node< K, T > T::*NODE, typename P = std::less< K > >
class inmultimap : public inmap_base< K, T, NODE, inmultimap_policies< P > >
{
public:

    inmultimap()
    {
    }

private:

    //Non-copyable
    inmultimap( const inmultimap< K, T, NODE, P >& );
    inmultimap& operator=( const inmultimap< K, T, NODE, P >& );
};

}   //namespace rage

#endif  //ATL_INMAP_H
