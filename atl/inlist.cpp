// 
// atl/inlist.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "inlist.h"

///////////////////////////////////////////////////////////////////////////////
// Unit tests
///////////////////////////////////////////////////////////////////////////////

#define QA_ASSERT_ON_FAIL   0
#include "qa/qa.h"

#if __QA

#include <list>

#include "math/random.h"
#include "system/nelem.h"

//#define QA_MAKE_RAND_SEED   588089026
#define QA_MAKE_RAND_SEED   int( sysTimer::GetSystemMsTime() )

using namespace rage;

struct LItem
{
    bool operator==( const LItem& that ) const
    {
        return m_Value == that.m_Value;
    }

    bool operator!=( const LItem& that ) const
    {
        return m_Value != that.m_Value;
    }

    //Used in merge()
    bool operator<( const LItem& f ) const { return m_Value < f.m_Value; }

    int m_Value;
    inlist_node< LItem > m_link;
};

typedef inlist< LItem, &LItem::m_link > IList;
typedef std::list< LItem* > SList;

class qa_IList : public qaItem
{
public:

    void Init()
    {
        const int seed = QA_MAKE_RAND_SEED;
        QALog( "qa_IList: Using random seed:%u", seed );

        sm_Rand.Reset( seed );
    }

    void Shutdown()
    {
    }

	void Update( qaResult& result )
    {
        result.Reset();
        testIterator( result );
        QA_CHECK( qaResult::PASS == result.GetCondition() );

        result.Reset();
        testReverseIterator( result );
        QA_CHECK( qaResult::PASS == result.GetCondition() );

        //result.Reset();
        //testInsert( result );
        //QA_CHECK( qaResult::PASS == result.GetCondition() );

        result.Reset();
        testMerge( result );
        QA_CHECK( qaResult::PASS == result.GetCondition() );

        result.Reset();
        testSplice( result );
        QA_CHECK( qaResult::PASS == result.GetCondition() );

        result.Reset();
        testSwap( result );
        QA_CHECK( qaResult::PASS == result.GetCondition() );
    }

    static void testIterator( qaResult& result );
    static void testReverseIterator( qaResult& result );
    //static void testInsert( qaResult& result );
    static void testMerge( qaResult& result );
    static void testSplice( qaResult& result );
    static void testSwap( qaResult& result );

    static mthRandom sm_Rand;
};

mthRandom qa_IList::sm_Rand;

void
qa_IList::testIterator( qaResult& result )
{
    LItem items[ 10 ];
    IList _ilist;
    SList _slist;

    for( int i = 0; i < NELEM( items ); ++i )
    {
        items[ i ].m_Value = sm_Rand.GetInt();
        _ilist.push_back( &items[ i ] );
        _slist.push_back( &items[ i ] );
    }

    //Check that incrementing from one prior to begin and
    //decrementing from one past the end are consistent.
    QA_CHECK( ++( --_ilist.begin() ) == _ilist.begin() );
    QA_CHECK( ++( --_ilist.end() ) == _ilist.end() );
    QA_CHECK( *--_ilist.end() == _ilist.back() );

    IList::iterator i_it = _ilist.begin();
    IList::const_iterator i_cit = _ilist.begin();
    SList::iterator it = _slist.begin();
    SList::const_iterator cit = _slist.begin();

    unsigned count;;

    //Pre-increment
    for( count = 0; i_it != _ilist.end(); ++i_it, ++i_cit, ++it, ++cit, ++count )
    {
        QA_CHECK( *i_it == *it );
        QA_CHECK( *i_cit == *cit );
    }

    QA_CHECK( _ilist.end() == i_it );
    QA_CHECK( _ilist.end() == i_cit );
    QA_CHECK( _slist.end() == it );
    QA_CHECK( SList::const_iterator( _slist.end() ) == cit );
    QA_CHECK( _ilist.size() == count );
    QA_CHECK( _slist.size() == count );

    i_it = _ilist.begin();
    i_cit = _ilist.begin();
    it = _slist.begin();
    cit = _slist.begin();

    //Post-increment
    for( count = 0; i_it != _ilist.end(); i_it++, i_cit++, it++, cit++, count++ )
    {
        QA_CHECK( *i_it == *it );
        QA_CHECK( *i_cit == *cit );
    }

    QA_CHECK( _ilist.end() == i_it );
    QA_CHECK( _ilist.end() == i_cit );
    QA_CHECK( _slist.end() == it );
    QA_CHECK( SList::const_iterator( _slist.end() ) == cit );
    QA_CHECK( _ilist.size() == count );
    QA_CHECK( _slist.size() == count );

    i_it = _ilist.begin();
    i_cit = _ilist.begin();
    it = _slist.begin();
    cit = _slist.begin();

    for( int i = 0; i < NELEM( items ) - 1; ++i, ++i_it, ++i_cit, ++it, ++cit )
    {
    }

    //Pre-decrement
    for( count = 0; i_it != _ilist.begin(); --i_it, --i_cit, --it, --cit, ++count )
    {
        QA_CHECK( *i_it == *it );
        QA_CHECK( *i_cit == *cit );
    }

    QA_CHECK( _ilist.begin() == i_it );
    QA_CHECK( _ilist.begin() == i_cit );
    QA_CHECK( _slist.begin() == it );
    QA_CHECK( SList::const_iterator( _slist.begin() ) == cit );
    QA_CHECK( _ilist.size() == count + 1 );
    QA_CHECK( _slist.size() == count + 1 );

    i_it = _ilist.begin();
    i_cit = _ilist.begin();
    it = _slist.begin();
    cit = _slist.begin();

    for( int i = 0; i < NELEM( items ) - 1; i++, i_it++, i_cit++, it++, cit++ )
    {
    }

    //Post-decrement
    for( count = 0; i_it != _ilist.begin(); i_it--, i_cit--, it--, cit--, count++ )
    {
        QA_CHECK( *i_it == *it );
        QA_CHECK( *i_cit == *cit );
    }

    QA_CHECK( _ilist.begin() == i_it );
    QA_CHECK( _ilist.begin() == i_cit );
    QA_CHECK( _slist.begin() == it );
    QA_CHECK( SList::const_iterator( _slist.begin() ) == cit );
    QA_CHECK( _ilist.size() == count + 1 );
    QA_CHECK( _slist.size() == count + 1 );

    _ilist.clear();
    _slist.clear();
    QA_CHECK( _ilist.empty() );
    QA_CHECK( _ilist.size() == 0 );

    TST_PASS;
}

void
qa_IList::testReverseIterator( qaResult& result )
{
    LItem items[ 10 ];
    IList _ilist;
    SList _slist;

    for( int i = 0; i < NELEM( items ); ++i )
    {
        items[ i ].m_Value = sm_Rand.GetInt();
        _ilist.push_back( &items[ i ] );
        _slist.push_back( &items[ i ] );
    }

    //Check that incrementing from one prior to begin and
    //decrementing from one past the end are consistent.
    //QA_CHECK( ++( --_ilist.rbegin() ) == _ilist.rbegin() );
    //QA_CHECK( ++( --_ilist.rend() ) == _ilist.rend() );
    //QA_CHECK( *--_ilist.rend() == _ilist.front() );

    IList::reverse_iterator i_it = _ilist.rbegin();
    IList::const_reverse_iterator i_cit = _ilist.rbegin();
    SList::reverse_iterator it = _slist.rbegin();
    SList::const_reverse_iterator cit = _slist.rbegin();

    unsigned count;

    //Pre-increment
    for( count = 0; i_it != _ilist.rend(); ++i_it, ++i_cit, ++it, ++cit, ++count )
    {
        QA_CHECK( *i_it == *it );
        QA_CHECK( *i_cit == *cit );
    }

    QA_CHECK( _ilist.rend() == i_it );
    QA_CHECK( _slist.rend() == it );
    QA_CHECK( SList::const_reverse_iterator( _slist.rend() ) == cit );
    QA_CHECK( _ilist.size() == count );
    QA_CHECK( _slist.size() == count );

    i_it = _ilist.rbegin();
    i_cit = _ilist.rbegin();
    it = _slist.rbegin();
    cit = _slist.rbegin();

    //Post-increment
    for( count = 0; i_it != _ilist.rend(); i_it++, i_cit++, it++, cit++, count++ )
    {
        QA_CHECK( *i_it == *it );
        QA_CHECK( *i_cit == *cit );
    }

    QA_CHECK( _ilist.rend() == i_it );
    QA_CHECK( _slist.rend() == it );
    QA_CHECK( SList::const_reverse_iterator( _slist.rend() ) == cit );
    QA_CHECK( _ilist.size() == count );
    QA_CHECK( _slist.size() == count );

    i_it = _ilist.rbegin();
    i_cit = _ilist.rbegin();
    it = _slist.rbegin();
    cit = _slist.rbegin();

    for( int i = 0; i < NELEM( items ) - 1; ++i, ++i_it, ++i_cit, ++it, ++cit )
    {
    }

    //Pre-decrement
    for( count = 0; i_it != _ilist.rbegin(); --i_it, --i_cit, --it, --cit, ++count )
    {
        QA_CHECK( *i_it == *it );
        QA_CHECK( *i_cit == *cit );
    }

    QA_CHECK( _ilist.rbegin() == i_it );
    QA_CHECK( _slist.rbegin() == it );
    QA_CHECK( SList::const_reverse_iterator( _slist.rbegin() ) == cit );
    QA_CHECK( _ilist.size() == count + 1 );
    QA_CHECK( _slist.size() == count + 1 );

    i_it = _ilist.rbegin();
    i_cit = _ilist.rbegin();
    it = _slist.rbegin();
    cit = _slist.rbegin();

    for( int i = 0; i < NELEM( items ) - 1; i++, i_it++, i_cit++, it++, cit++ )
    {
    }

    //Post-decrement
    for( count = 0; i_it != _ilist.rbegin(); i_it--, i_cit--, it--, cit--, count++ )
    {
        QA_CHECK( *i_it == *it );
        QA_CHECK( *i_cit == *cit );
    }

    QA_CHECK( _ilist.rbegin() == i_it );
    QA_CHECK( _slist.rbegin() == it );
    QA_CHECK( SList::const_reverse_iterator( _slist.rbegin() ) == cit );
    QA_CHECK( _ilist.size() == count + 1 );
    QA_CHECK( _slist.size() == count + 1 );

    _ilist.clear();
    _slist.clear();
    QA_CHECK( _ilist.empty() );
    QA_CHECK( _ilist.size() == 0 );

    TST_PASS;
}

void
qa_IList::testMerge( qaResult& result )
{
    LItem items[ 20 ];
    IList _ilistA, _ilistB;;
    SList _slistA, _slistB;

    int i;

    for( i = 0; i < NELEM( items ) / 2; ++i )
    {
        //items[ i ].m_Value = i + NELEM( items ) / 2;
        items[ i ].m_Value = sm_Rand.GetInt();
        _ilistA.push_back( &items[ i ] );
        _slistA.push_back( &items[ i ] );
    }

    for( ; i < NELEM( items ); ++i )
    {
        //items[ i ].m_Value = i - NELEM( items ) / 2;
        items[ i ].m_Value = sm_Rand.GetInt();
        _ilistB.push_back( &items[ i ] );
        _slistB.push_back( &items[ i ] );
    }

    _ilistA.merge( _ilistB );
    _slistA.merge( _slistB );

    IList::iterator i_it = _ilistA.begin();
    SList::iterator it = _slistA.begin();

    for( ; i_it != _ilistA.end(); ++i_it, ++it )
    {
        QA_CHECK( *i_it == *it );
        //printf( "%d %d\n", i_it->m_Value, it->m_Value );
    }

    QA_CHECK( _ilistA.end() == i_it );
    QA_CHECK( SList::const_iterator( _slistA.end() ) == it  );

    QA_CHECK( _ilistA.size() == _slistA.size() );
    QA_CHECK( _ilistB.size() == _slistB.size() );

    TST_PASS;
}

void
qa_IList::testSplice( qaResult& result )
{
    LItem items[ 20 ];
    IList _ilistA, _ilistB;;
    SList _slistA, _slistB;

    int i;

    for( i = 0; i < NELEM( items ) / 2; ++i )
    {
        //items[ i ].m_Value = i + NELEM( items ) / 2;
        items[ i ].m_Value = sm_Rand.GetInt();
        _ilistA.push_back( &items[ i ] );
        _slistA.push_back( &items[ i ] );
    }

    for( ; i < NELEM( items ); ++i )
    {
        //items[ i ].m_Value = i - NELEM( items ) / 2;
        items[ i ].m_Value = sm_Rand.GetInt();
        _ilistB.push_back( &items[ i ] );
        _slistB.push_back( &items[ i ] );
    }

    _ilistA.splice( _ilistA.begin(), _ilistB );
    _slistA.splice( _slistA.begin(), _slistB );

    IList::iterator i_it = _ilistA.begin();
    SList::iterator it = _slistA.begin();

    for( ; i_it != _ilistA.end(); ++i_it, ++it )
    {
        QA_CHECK( *i_it == *it );
        //printf( "%d %d\n", i_it->m_Value, it->m_Value );
    }

    QA_CHECK( _ilistA.end() == i_it );
    QA_CHECK( SList::const_iterator( _slistA.end() ) == it  );

    QA_CHECK( _ilistA.size() == _slistA.size() );
    QA_CHECK( _ilistB.size() == _slistB.size() );

    TST_PASS;
}

void
qa_IList::testSwap( qaResult& result )
{
    LItem items[ 20 ];
    IList _ilistA, _ilistB;;
    SList _slistA, _slistB;

    int i;

    for( i = 0; i < NELEM( items ) / 2; ++i )
    {
        //items[ i ].m_Value = i + NELEM( items ) / 2;
        items[ i ].m_Value = sm_Rand.GetInt();
        _ilistA.push_back( &items[ i ] );
        _slistA.push_back( &items[ i ] );
    }

    for( ; i < NELEM( items ); ++i )
    {
        //items[ i ].m_Value = i - NELEM( items ) / 2;
        items[ i ].m_Value = sm_Rand.GetInt();
        _ilistB.push_back( &items[ i ] );
        _slistB.push_back( &items[ i ] );
    }

    _ilistA.swap( _ilistB );
    _slistA.swap( _slistB );

    IList::iterator i_it = _ilistA.begin();
    SList::iterator it = _slistA.begin();

    for( ; i_it != _ilistA.end(); ++i_it, ++it )
    {
        QA_CHECK( *i_it == *it );
        //printf( "%d %d\n", i_it->m_Value, it->m_Value );
    }

    QA_CHECK( i_it == _ilistA.end() );
    QA_CHECK( it == _slistA.end() );

    int ii = 0, iii = 1;
    std::swap( ii, iii );
    std::swap( _slistA, _slistB );

    QA_CHECK( _ilistA.size() == _slistA.size() );
    QA_CHECK( _ilistB.size() == _slistB.size() );

    TST_PASS;
}

QA_ITEM_FAMILY( qa_IList, (), () );

QA_ITEM( qa_IList, (), qaResult::PASS_OR_FAIL );

#endif  //__QA
