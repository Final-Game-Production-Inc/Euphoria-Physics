// 
// atl/atinbintree.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "atinbintree.h"
#include "qa/qa.h"

#if __QA

#include "math/random.h"
#include <math.h>
#include <string.h>

using namespace rage;

class qa_AbtObject
{
public:

    unsigned id;
    unsigned priority;

    atInBinTreeLink< unsigned, qa_AbtObject > m_Link;
};

class qa_atInBinTree : public qaItem
{
public:

    qa_atInBinTree();

    void Init();
    void Shutdown();

    void Update( qaResult& result );

private:
};

qa_atInBinTree::qa_atInBinTree()
{
}

void
qa_atInBinTree::Init()
{
}

void
qa_atInBinTree::Shutdown()
{
}

#define RAND_SEED   sysTimer::GetSystemMsTime()
//#define RAND_SEED   370292541

//#define RAND_SEED   370292541

void
qa_atInBinTree::Update( qaResult& result )
{
    atInBinTree< unsigned, qa_AbtObject, &qa_AbtObject::m_Link, atLess< unsigned > > tree;

    const int COUNT = 1000;
    qa_AbtObject* objs = rage_new qa_AbtObject[ COUNT ];
    qa_AbtObject** pobjs = rage_new qa_AbtObject*[ COUNT ];
    unsigned* keys = rage_new unsigned[ COUNT ];

    const int randSeed = ( int ) RAND_SEED;

    QALog( "Using random seed:%d", randSeed );

    mthRandom r( randSeed );

    /*int count;
    for( count = 0; ; ++count )
    {
        if( 27058285 == r.GetInt() )
            break;
    }*/

    for( int i = 0; i < COUNT; ++i )
    {
        pobjs[ i ] = &objs[ i ];
    }

    int numItems = 0;

    QALog( "Inserting %d items with random keys...", COUNT );

    //Insert items with random keys
    for( ; numItems < COUNT; ++numItems )
    {
        unsigned key = r.GetInt();

        while( tree.Contains( key ) )
        {
            key = r.GetInt();
        }

        tree.Insert( key, pobjs[ numItems ] );
        keys[ numItems ] = key;
    }

    float accum = 0;

    QALog( "Verifying %d items are in tree...", numItems );

    //Verify the items are in the tree
    for( int i = 0; i < numItems; ++i )
    {
        if( !tree.Contains( keys[ i ] ) )
        {
            QALog( "Tree should contain key:%d but does not - fail", keys[ i ] );

            TST_FAIL;
            goto fail;
        }

#if __DEV
        accum += tree.GetLastFindCost();
#endif
    }

    QALog( "Average access cost per item:%f", accum / numItems );

    //Access cost should be less than 2 * log2( n )
    if( ( accum / numItems ) >= ( 2 * log( ( float ) numItems ) / log( 2.0f ) ) )
    {
        QALog( "Access cost was too large - fail" );
        TST_FAIL;
        goto fail;
    }

    QALog( "Randomly removing and inserting items..." );

    //Insert and remove randomly
    for( int i = 0; i < 1000000; ++i )
    {
        bool doinsert = !!( r.GetInt() & 0x01 );

        if( doinsert && numItems < COUNT )
        {
            unsigned key = r.GetInt();

            while( tree.Contains( key ) )
            {
                key = r.GetInt();
            }

            tree.Insert( key, pobjs[ numItems ] );
            keys[ numItems ] = key;

            ++numItems;
        }
        else if( numItems > 0 )
        {
            const int index = r.GetInt() % numItems;

            if( !tree.Contains( keys[ index ] ) )
            {
                QALog( "Tree should contain key:%d but does not - fail", keys[ index ] );

                TST_FAIL;
                goto fail;
            }

            tree.Remove( keys[ index ] );

            unsigned tmpkey = keys[ numItems - 1 ];
            qa_AbtObject* tmpobj = pobjs[ numItems - 1 ];
            keys[ numItems - 1 ] = keys[ index ];
            pobjs[ numItems - 1 ] = pobjs[ index ];
            keys[ index ] = tmpkey;
            pobjs[ index ] = tmpobj;

            --numItems;
        }
    }

    accum = 0;

    QALog( "Verifying %d items are in tree...", numItems );

    //Verify the items are in the tree
    for( int i = 0; i < numItems; ++i )
    {
        if( !tree.Contains( keys[ i ] ) )
        {
            QALog( "Tree should contain key:%d but does not - fail", keys[ i ] );

            TST_FAIL;
            goto fail;
        }

#if __DEV
        accum += tree.GetLastFindCost();
#endif
    }

    QALog( "Average access cost per item:%f", accum / numItems );

    //Access cost should be less than 2 * log2( n )
    if( ( accum / numItems ) >= ( 2 * log( ( float ) numItems ) / log( 2.0f ) ) )
    {
        QALog( "Access cost was too large - fail" );
        TST_FAIL;
        return;
    }

    QALog( "Removing %d items in random order...", numItems );

    for( ; numItems > 0 ; --numItems )
    {
        const int index = r.GetInt() % numItems;

        if( !tree.Contains( keys[ index ] ) )
        {
            QALog( "Tree should contain key:%d but does not - fail", keys[ index ] );

            TST_FAIL;
            return;
        }

        tree.Remove( keys[ index ] );

        keys[ index ] = keys[ numItems - 1 ];
    }

    if( tree.GetCount() != 0 )
    {
        TST_FAIL;
        goto fail;
    }

    TST_PASS;

fail:

    tree.Clear();

    delete [] objs;
    delete [] pobjs;
    delete [] keys;
}

class qa_atInBinTreeMultimap : public qaItem
{
public:

    qa_atInBinTreeMultimap();

    void Init();
    void Shutdown();

    void Update( qaResult& result );

private:
};

qa_atInBinTreeMultimap::qa_atInBinTreeMultimap()
{
}

void
qa_atInBinTreeMultimap::Init()
{
}

void
qa_atInBinTreeMultimap::Shutdown()
{
}

void
qa_atInBinTreeMultimap::Update( qaResult& result )
{
    const int NUM_PRIORITIES    = 10;
    const int NUM_OBJECTS       = 1000;

    typedef atInBinTree< unsigned, qa_AbtObject, &qa_AbtObject::m_Link > OTree;
    qa_AbtObject* objPool = rage_new qa_AbtObject[ NUM_OBJECTS ];
    qa_AbtObject** sortedObjects = rage_new qa_AbtObject*[ NUM_OBJECTS ];
    int priorityCounts[ NUM_PRIORITIES ];
    int priorityOffsets[ NUM_PRIORITIES ];

    const int randSeed = ( int ) sysTimer::GetSystemMsTime();

    QALog( "Using random seed:%d", randSeed );

    mthRandom r( randSeed );

    OTree::Policies treePolicies;

    treePolicies.m_AllowDuplicateKeys = true;

    OTree otree;

    otree.SetPolicies( treePolicies );

    ::memset( priorityCounts, 0, sizeof( priorityCounts ) );

    //Insert objects into the tree (keyed on priority) and count the
    //number of objects at each priority.
    for( int i = 0; i < NUM_OBJECTS; ++i )
    {
        objPool[ i ].id = i;
        objPool[ i ].priority = r.GetInt() % NUM_PRIORITIES;

        otree.Insert( objPool[ i ].priority, &objPool[ i ] );

        ++priorityCounts[ objPool[ i ].priority ];
    }

    //Build an array of sorted objects

    OTree::Iterator it( otree.GetFirst() );

    for( int i = 0; it.IsValid(); it.Next(), ++i )
    {
        sortedObjects[ i ] = it.Item();
    }

    //Compute array offsets to the beginning of each set of priorities

    int offset = 0;

    for( int i = 0; i < NUM_PRIORITIES; ++i )
    {
        priorityOffsets[ i ] = offset;
        offset += priorityCounts[ i ];
    }

    //Randomly remove and re-add objects from the tree.

    for( int i = 0; i < 10000; ++i )
    {
        const int pri = r.GetInt() % NUM_PRIORITIES;

        qa_AbtObject* obj = otree.Find( pri );

        otree.Remove( obj );
        otree.Insert( obj->priority, obj );

        const int count = priorityCounts[ pri ];
        int offset = priorityOffsets[ pri ];

        for( int j = 0; j < count - 1; ++j, ++offset )
        {
            sortedObjects[ offset ] = sortedObjects[ offset + 1 ];
        }

        sortedObjects[ offset ] = obj;
    }

    //Verify that the tree and the array contain objects in the same order

    for( int i = 0; i < NUM_PRIORITIES; ++i )
    {
        OTree::Iterator it( otree.Find( i ) );

        const int count = priorityCounts[ i ];
        int offset = priorityOffsets[ i ];

        for( int j = 0; it.IsValid() && j < count; ++j, ++offset, it.Next() )
        {
            qa_AbtObject* obj = sortedObjects[ offset ];

            if( obj != it.Item() )
            {
                QALog( "Item at array offset:%d (id:%d pri:%d) does not match item in tree",
                       offset,
                       obj->id,
                       obj->priority );

                TST_FAIL;

                goto fail;
            }
        }

        Assert( offset == priorityOffsets[ i ] + count );
    }

    TST_PASS;

fail:

    otree.Clear();

    delete [] objPool;
    delete [] sortedObjects;
}

QA_ITEM_FAMILY( qa_atInBinTree, (), () );
QA_ITEM_FAMILY( qa_atInBinTreeMultimap, (), () );

QA_ITEM( qa_atInBinTree, (), qaResult::PASS_OR_FAIL );
QA_ITEM( qa_atInBinTreeMultimap, (), qaResult::PASS_OR_FAIL );

#endif  //_QA
