// 
// atl/inmap.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "inmap.h"

///////////////////////////////////////////////////////////////////////////////
// Unit tests
///////////////////////////////////////////////////////////////////////////////

#define QA_ASSERT_ON_FAIL   0
#include "qa/qa.h"

#if __QA

#include <map>
#include <algorithm>
#include <math.h>

#include "math/random.h"

//#define QA_MAKE_RAND_SEED   781916531
#define QA_MAKE_RAND_SEED   int( sysTimer::GetSystemMsTime() )

using namespace rage;

class qa_IMap : public qaItem
{
public:

    static const int NUM_ITEMS          = 1024;
    static const int MAX_UNIQUE_KEYS    = 100;

    void Init()
    {
        static bool s_IsInitialized = false;
        if( !s_IsInitialized )
        {
            const int seed = QA_MAKE_RAND_SEED;
            QALog( "qa_IMap: Using random seed:%u", seed );

            sm_Rand.Reset( seed );

            s_IsInitialized = true;
        }

        sm_DupKeys = false;
    }

    void Shutdown()
    {
    }

    struct MItem
    {
        int m_Key;
        int m_Index;

        inmap_node< int, MItem > m_link;
    };

    typedef inmultimap< int, MItem, &MItem::m_link > IMMap;
    typedef std::multimap< int, MItem > SMMap;

	void Update( qaResult& result )
    {
        inmap< int, MItem, &MItem::m_link > _imap;
        std::map< int, MItem > _smap;
        inmultimap< int, MItem, &MItem::m_link > _immap;
        std::multimap< int, MItem > _smmap;

        sm_DupKeys = false;

        QALog( "\n  Testing map with unique keys" );
        Test( _imap, _smap, result );
        QALog( "\n  Testing multimap with unique keys" );
        Test( _immap, _smmap, result );

        sm_DupKeys = true;

        QALog( "\n  Testing map with duplicate keys" );
        Test( _imap, _smap, result );
        QALog( "\n  Testing multimap with duplicate keys" );
        Test( _immap, _smmap, result );
    }

    template< typename MAP0, typename MAP1 >
    void Test( MAP0& map0,
               MAP1& map1,
               qaResult& result );

    template< typename IMAP, typename SMAP >
    static void initMaps( MItem* items,
                          MItem** pitems,
                          const int numMItems,
                          IMAP& _imap,
                          SMAP& _smap );

    template< typename MAP0, typename MAP1 >
    static void testInsertEraseAndFind( MAP0& map0,
                                        MAP1& map1,
                                        qaResult& result );

    template< typename MAP0, typename MAP1 >
    static void testIterator( MAP0& map0,
                              MAP1& map1,
                              qaResult& result );

    template< typename MAP0, typename MAP1 >
    static void testReverseIterator( MAP0& map0,
                                     MAP1& map1,
                                     qaResult& result );

    template< typename MAP0, typename MAP1 >
    static void testLowerBound( MAP0& map0,
                                MAP1& map1,
                                qaResult& result );

    template< typename MAP0, typename MAP1 >
    static void testUpperBound( MAP0& map0,
                                MAP1& map1,
                                qaResult& result );

    template< typename MAP0, typename MAP1 >
    static void testEqualRange( MAP0& map0,
                                MAP1& map1,
                                qaResult& result );

    template< typename MAP0, typename MAP1, typename IT0, typename IT1 >
    static void testIteratorHelper( MAP0& map0, MAP1& map1, qaResult& result );

    template< typename MAP0, typename MAP1, typename IT0, typename IT1 >
    static void testReverseIteratorHelper( MAP0& map0, MAP1& map1, qaResult& result );

    static mthRandom sm_Rand;

    static bool sm_DupKeys;
};

struct RandOp
{
    intptr_t operator()( const intptr_t n )
    {
        return qa_IMap::sm_Rand.GetInt() % n;
    }
};

mthRandom qa_IMap::sm_Rand;
bool qa_IMap::sm_DupKeys;

template< typename MAP0, typename MAP1 >
void
qa_IMap::Test( MAP0& map0, MAP1& map1, qaResult& result )
{
    result.Reset();
    testInsertEraseAndFind( map0, map1, result );
    QA_CHECK( qaResult::PASS == result.GetCondition() );
    Assert( map0.empty() && map1.empty() );

    result.Reset();
    testIterator( map0, map1, result );
    QA_CHECK( qaResult::PASS == result.GetCondition() );
    Assert( map0.empty() && map1.empty() );

    result.Reset();
    testReverseIterator( map0, map1, result );
    QA_CHECK( qaResult::PASS == result.GetCondition() );
    Assert( map0.empty() && map1.empty() );

    result.Reset();
    testLowerBound( map0, map1, result );
    QA_CHECK( qaResult::PASS == result.GetCondition() );
    Assert( map0.empty() && map1.empty() );

    result.Reset();
    testUpperBound( map0, map1, result );
    QA_CHECK( qaResult::PASS == result.GetCondition() );
    Assert( map0.empty() && map1.empty() );

    result.Reset();
    testEqualRange( map0, map1, result );
    QA_CHECK( qaResult::PASS == result.GetCondition() );
    Assert( map0.empty() && map1.empty() );
}

template< typename IMAP, typename SMAP >
void
qa_IMap::initMaps( MItem* items,
                   MItem** pitems,
                   const int numMItems,
                   IMAP& _imap,
                   SMAP& _smap )
{
    _imap.clear();
    _smap.clear();

    int dupKeyCounts[ MAX_UNIQUE_KEYS ] = { 0 };

    for( int i = 0; i < NUM_ITEMS; ++i )
    {
        bool unique = false;

        while( !unique )
        {
            int key = sm_Rand.GetInt();

            if( !sm_DupKeys )
            {
                unique = true;

                //Make sure the key is unique.
                for( int j = 0; j < i; ++j )
                {
                    if( key == items[ i ].m_Key )
                    {
                        unique = false;
                        break;
                    }
                }
            }
            else
            {
                key %= MAX_UNIQUE_KEYS;
                unique = true;

                ++dupKeyCounts[ key ];
            }

            if( unique )
            {
                items[ i ].m_Key = key;
                items[ i ].m_Index = i;
                pitems[ i ] = &items[ i ];
            }
        }
    }

    int keyCount = 0;

    if( sm_DupKeys )
    {
        for( int i = 0; i < MAX_UNIQUE_KEYS; ++i )
        {
            keyCount += !!dupKeyCounts[ i ];
        }
    }

    RandOp randOp;

    std::random_shuffle( &pitems[ 0 ], &pitems[ numMItems ], randOp );

    for( int i = 0; i < numMItems; ++i )
    {
        MItem* item = pitems[ i ];
        _imap.insert( typename IMAP::value_type( item->m_Key, item ) );
        _smap.insert( typename SMAP::value_type( item->m_Key, *item ) );
    }

    Assert( _imap.size() == _smap.size() );

    //Either our map contains NUM_ITEMS items, or we're not allowing
    //duplicate keys.  If we're not allowing dup keys our map will contain
    //less than NUM_ITEMS, but not more than MAX_UNIQUE_KEYS, items.
    Assert( NUM_ITEMS == ( int ) _imap.size()
            || ( !IMAP::ALLOW_DUP_KEYS && keyCount == ( int ) _imap.size() ) );
}

template< typename MAP0, typename MAP1 >
void
qa_IMap::testInsertEraseAndFind( MAP0& map0, MAP1& map1, qaResult& result )
{
    MItem items[ NUM_ITEMS ];
    MItem* pitems[ NUM_ITEMS ];
    float cost = 0;

    initMaps( items, pitems, NUM_ITEMS, map0, map1 );

    RandOp randOp;

    std::random_shuffle( &pitems[ 0 ], &pitems[ NUM_ITEMS ], randOp );

    for( int i = 0; i < NUM_ITEMS; ++i )
    {
        size_t curCost;
        typename MAP0::iterator it = map0.find( pitems[ i ]->m_Key, &curCost );
        QA_CHECK( map0.end() != it );
        QA_CHECK( it->second->m_Key == pitems[ i ]->m_Key );

        cost += curCost;
    }

    QALog( "  Average find cost for %d items:%f (should be ~%f)",
           map0.size(),
           cost / ( float ) NUM_ITEMS,
           ::log( ( float ) map0.size() ) / ::log( 2.0f ) );

    std::random_shuffle( &pitems[ 0 ], &pitems[ NUM_ITEMS ], randOp );

    for( int i = 0; i < NUM_ITEMS; ++i )
    {
        typename MAP0::iterator it = map0.find( pitems[ i ]->m_Key );
        QA_CHECK( map0.end() == it
                  || it->second->m_Key == pitems[ i ]->m_Key );

        if( ( i & 0x01 ) && map0.end() != it )
        {
            //Erase by value
            map0.erase( it->second );
            map1.erase( map1.find( pitems[ i ]->m_Key ) );

            QA_CHECK( map0.count( pitems[ i ]->m_Key ) == map1.count( pitems[ i ]->m_Key ) );
        }
        else
        {
            //Erase by key
            const size_t count = map0.erase( pitems[ i ]->m_Key );

            QA_CHECK( map1.erase( pitems[ i ]->m_Key ) == count );
            QA_CHECK( map0.end() == map0.find( pitems[ i ]->m_Key ) );
        }

        QA_CHECK( map0.size() == map1.size() );
    }

    QA_CHECK( map0.empty() );

    map0.clear();
    map1.clear();

    TST_PASS;
}

template< typename MAP0, typename MAP1 >
void
qa_IMap::testIterator( MAP0& map0, MAP1& map1, qaResult& result )
{
    MItem items[ NUM_ITEMS ];
    MItem* pitems[ NUM_ITEMS ];

    result.Reset();
    initMaps( items, pitems, NUM_ITEMS, map0, map1 );
    testIteratorHelper< MAP0, MAP1, typename MAP0::iterator, typename MAP1::iterator >( map0, map1, result );
    QA_CHECK( qaResult::PASS == result.GetCondition() );

    result.Reset();
    initMaps( items, pitems, NUM_ITEMS, map0, map1 );
    testIteratorHelper< const MAP0, const MAP1, typename MAP0::const_iterator, typename MAP1::const_iterator >( map0, map1, result );
    QA_CHECK( qaResult::PASS == result.GetCondition() );

    map0.clear();
    map1.clear();
}

template< typename MAP0, typename MAP1 >
void
qa_IMap::testReverseIterator( MAP0& map0, MAP1& map1, qaResult& result )
{
    MItem items[ NUM_ITEMS ];
    MItem* pitems[ NUM_ITEMS ];

    result.Reset();
    initMaps( items, pitems, NUM_ITEMS, map0, map1 );
    testReverseIteratorHelper< MAP0, MAP1, typename MAP0::reverse_iterator, typename MAP1::reverse_iterator >( map0, map1, result );
    QA_CHECK( qaResult::PASS == result.GetCondition() );

    result.Reset();
    initMaps( items, pitems, NUM_ITEMS, map0, map1 );
    testReverseIteratorHelper< const MAP0, const MAP1, typename MAP0::const_reverse_iterator, typename MAP1::const_reverse_iterator >( map0, map1, result );
    QA_CHECK( qaResult::PASS == result.GetCondition() );

    map0.clear();
    map1.clear();
}

template< typename MAP0, typename MAP1, typename IT0, typename IT1 >
void
qa_IMap::testIteratorHelper( MAP0& map0, MAP1& map1, qaResult& result )
{
    //float cost = 0;

    IT0 it0 = map0.begin();
    IT1 it1 = map1.begin();
    unsigned count;

    typename MAP0::const_iterator kit = map0.begin();
    Assert( map0.end() != kit );

    //Check that incrementing from one prior to begin and
    //decrementing from one past the end are consistent.
    QA_CHECK( ++( --map0.begin() ) == map0.begin() );
    QA_CHECK( ++( --map0.end() ) == map0.end() );

    //Pre-increment
    for( count = 0; map0.end() != it0; ++it0, ++it1, ++count )
    {
        QA_CHECK( it0->second->m_Key == it1->second.m_Key );
    }

    QA_CHECK( map0.end() == it0 );
    QA_CHECK( map1.end() == it1 );
    QA_CHECK( map0.size() == count );
    QA_CHECK( map1.size() == count );

    it0 = map0.begin();
    it1 = map1.begin();

    //Post-increment
    for( count = 0; map0.end() != it0; it0++, it1++, count++ )
    {
        QA_CHECK( it0->second->m_Key == it1->second.m_Key );
    }

    QA_CHECK( map0.end() == it0 );
    QA_CHECK( map1.end() == it1 );
    QA_CHECK( map0.size() == count );
    QA_CHECK( map1.size() == count );

    it0 = map0.begin();
    it1 = map1.begin();

    for( unsigned i = 0; i < map0.size(); ++it0, ++it1, ++i )
    {
    }

    //Pre-decrement
    for( count = 0, --it0, --it1; map0.begin() != it0; --it0, --it1, ++count )
    {
        QA_CHECK( it0->second->m_Key == it1->second.m_Key );
    }

    QA_CHECK( map0.begin() == it0 );
    QA_CHECK( map1.begin() == it1 );
    QA_CHECK( map0.size() == count + 1 );
    QA_CHECK( map1.size() == count + 1 );

    it0 = map0.begin();
    it1 = map1.begin();

    for( unsigned i = 0; i < map0.size(); it0++, it1++, i++ )
    {
    }

    //Post-decrement
    for( count = 0, it0--, it1--; map0.begin() != it0; it0--, it1--, count++ )
    {
        QA_CHECK( it0->second->m_Key == it1->second.m_Key );
    }

    QA_CHECK( map0.begin() == it0 );
    QA_CHECK( map1.begin() == it1 );
    QA_CHECK( map0.size() == count + 1 );
    QA_CHECK( map1.size() == count + 1 );

    TST_PASS;
}

template< typename MAP0, typename MAP1, typename IT0, typename IT1 >
void
qa_IMap::testReverseIteratorHelper( MAP0& map0, MAP1& map1, qaResult& result )
{
    //float cost = 0;

    IT0 it0 = map0.rbegin();
    IT1 it1 = map1.rbegin();
    unsigned count;

    //Check that incrementing from one prior to begin and
    //decrementing from one past the end are consistent.
    //QA_CHECK( ++( --map0.rbegin() ) == map0.rbegin() );
    //QA_CHECK( ++( --map0.rend() ) == map0.rend() );

    //Pre-increment
    for( count = 0; map0.rend() != it0; ++it0, ++it1, ++count )
    {
        QA_CHECK( it0->second->m_Key == it1->second.m_Key );
    }

    QA_CHECK( map0.rend() == it0 );
    QA_CHECK( map1.rend() == it1 );
    QA_CHECK( map0.size() == count );
    QA_CHECK( map1.size() == count );

    it0 = map0.rbegin();
    it1 = map1.rbegin();

    //Post-increment
    for( count = 0; map0.rend() != it0; it0++, it1++, count++ )
    {
        QA_CHECK( it0->second->m_Key == it1->second.m_Key );
    }

    QA_CHECK( map0.rend() == it0 );
    QA_CHECK( map1.rend() == it1 );
    QA_CHECK( map0.size() == count );
    QA_CHECK( map1.size() == count );

    it0 = map0.rbegin();
    it1 = map1.rbegin();

    for( unsigned i = 0; i < map0.size(); ++it0, ++it1, ++i )
    {
    }

    //Pre-decrement
    for( count = 0, --it0, --it1; map0.rbegin() != it0; --it0, --it1, ++count )
    {
        QA_CHECK( it0->second->m_Key == it1->second.m_Key );
    }

    QA_CHECK( map0.rbegin() == it0 );
    QA_CHECK( map1.rbegin() == it1 );
    QA_CHECK( map0.size() == count + 1 );
    QA_CHECK( map1.size() == count + 1 );

    it0 = map0.rbegin();
    it1 = map1.rbegin();

    for( unsigned i = 0; i < map0.size(); it0++, it1++, i++ )
    {
    }

    //Post-decrement
    for( count = 0, it0--, it1--; map0.rbegin() != it0; it0--, it1--, count++ )
    {
        QA_CHECK( it0->second->m_Key == it1->second.m_Key );
    }

    QA_CHECK( map0.rbegin() == it0 );
    QA_CHECK( map1.rbegin() == it1 );
    QA_CHECK( map0.size() == count + 1 );
    QA_CHECK( map1.size() == count + 1 );

    TST_PASS;
}

template< typename MAP0, typename MAP1 >
void
qa_IMap::testLowerBound( MAP0& map0, MAP1& map1, qaResult& result )
{
    MItem items[ NUM_ITEMS ];
    MItem* pitems[ NUM_ITEMS ];

    initMaps( items, pitems, NUM_ITEMS, map0, map1 );

    for( int i = 0; i < NUM_ITEMS; ++i )
    {
        const int n = sm_Rand.GetInt();
        typename MAP0::iterator it0 = map0.lower_bound( n );
        typename MAP1::iterator it1 = map1.lower_bound( n );

        if( it0 == map0.end() )
        {
            QA_CHECK( it1 == map1.end() );
        }
        else
        {
            QA_CHECK( it1 != map1.end() );
            QA_CHECK( it0->second->m_Key == it1->second.m_Key );
        }
    }

    map0.clear();
    map1.clear();

    TST_PASS;
}

template< typename MAP0, typename MAP1 >
void
qa_IMap::testUpperBound( MAP0& map0, MAP1& map1, qaResult& result )
{
    MItem items[ NUM_ITEMS ];
    MItem* pitems[ NUM_ITEMS ];

    initMaps( items, pitems, NUM_ITEMS, map0, map1 );

    for( int i = 0; i < NUM_ITEMS; ++i )
    {
        const int n = sm_Rand.GetInt();
        typename MAP0::iterator it0 = map0.upper_bound( n );
        typename MAP1::iterator it1 = map1.upper_bound( n );

        if( it0 == map0.end() )
        {
            QA_CHECK( it1 == map1.end() );
        }
        else
        {
            QA_CHECK( it1 != map1.end() );
            QA_CHECK( it0->second->m_Key == it1->second.m_Key );
        }
    }

    map0.clear();
    map1.clear();

    TST_PASS;
}

template< typename MAP0, typename MAP1 >
void
qa_IMap::testEqualRange( MAP0& map0, MAP1& map1, qaResult& result )
{
    MItem items[ NUM_ITEMS ];
    MItem* pitems[ NUM_ITEMS ];

    initMaps( items, pitems, NUM_ITEMS, map0, map1 );

    typename MAP0::iterator it = map0.begin();

    for( int i = 0; i < ( int ) map0.size(); ++i, ++it )
    {
        //const int n = sm_Rand.GetInt();
        const int n = it->second->m_Key;
        std::pair< typename MAP0::iterator, typename MAP0::iterator > p0 = map0.equal_range( n );
        std::pair< typename MAP1::iterator, typename MAP1::iterator > p1 = map1.equal_range( n );

        if( map0.end() == p0.first )
        {
            if( map1.end() != p1.first )
            {
                Displayf( "%d", p1.first->second.m_Key );
            }

            QA_CHECK( map1.end() == p1.first );
        }
        else
        {
            for( ; p0.first != p0.second; ++p0.first, ++p1.first )
            {
                QA_CHECK( p1.first != p1.second );
                QA_CHECK( p0.first->second->m_Key == p1.first->second.m_Key );
            }
        }
    }

    map0.clear();
    map1.clear();

    TST_PASS;
}

QA_ITEM_FAMILY( qa_IMap, (), () );

QA_ITEM( qa_IMap, (), qaResult::PASS_OR_FAIL );

#endif  //__QA
