// 
// atl/atsafecontainer.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "atsafecontainer.h"
#include "qa/qa.h"
#include "system/nelem.h"

#if __QA

using namespace rage;

struct Node
{
    int i;

    DLinkSimple< Node > m_ListLink;
    atInBinTreeLink< const Node*, Node > m_TreeLink;
};

typedef atSafeDLListSimple< Node, &Node::m_ListLink > NodeList;
typedef atSafeInBinTree< const Node*, Node, &Node::m_TreeLink > NodeTree;

void Func0( NodeList* list, const int index )
{
    NodeList::Iterator it;

    list->BeginIteration( &it );

    for( int i = 0; i < index; ++i )
    {
        it.Next();
    }

    if( it.IsValid() )
    {
        Printf( "Removing:%d\n", it.Item()->i );
    }

    list->Remove( it );
}

void Func1( NodeTree* tree, const int index )
{
    NodeTree::Iterator it;

    tree->BeginIteration( &it );

    for( int i = 0; i < index; ++i )
    {
        it.Next();
    }

    if( it.IsValid() )
    {
        Printf( "Removing:%d\n", it.Item()->i );
    }

    tree->Remove( it );
}

class atSafeContainerQA : public qaItem
{
public:

    atSafeContainerQA();

    void Init();
    void Shutdown();

    void Update( qaResult& result );

private:
};


atSafeContainerQA::atSafeContainerQA()
{
}

void
atSafeContainerQA::Init()
{
}

void
atSafeContainerQA::Shutdown()
{
}

void
atSafeContainerQA::Update( qaResult& result )
{
    NodeList::Iterator itList1, itList2;
    NodeTree::Iterator itTree1, itTree2;

    {
        NodeList list;
        NodeTree tree;

        Node nodes[ 10 ];

        //Add items to the containers
        for( int i = 0; i < NELEM( nodes ); ++i )
        {
            nodes[ i ].i = i;
            list.AddToTail( &nodes[ i ] );
            tree.Insert( &nodes[ i ], &nodes[ i ] );
        }

        NodeList::Iterator itList0;
        NodeTree::Iterator itTree0;

        list.BeginIteration( &itList0 );
        list.BeginIteration( &itList1 );
        list.BeginIteration( &itList2 );

        tree.BeginIteration( &itTree0 );
        tree.BeginIteration( &itTree1 );
        tree.BeginIteration( &itTree2 );

        //Make sure our iterators are valid.
        if( !itList0.IsValid() ||
            !itList1.IsValid() ||
            !itList2.IsValid() ||
            !itTree0.IsValid() ||
            !itTree1.IsValid() ||
            !itTree2.IsValid() )
        {
            TST_FAIL;
            goto fail;
        }

        int count = list.GetCount();

        //Test removal of items while iterating.

        int numRemoved = 0;

        for( int i = 0; itList0.IsValid(); itList0.Next(), itTree0.Next(), ++i )
        {
            if( itList0.Item()->i != itTree0.Item()->i )
            {
                TST_FAIL;
                goto fail;
            }

            if( !( i % 3 ) )
            {
                //Remove an item
                Func0( &list, i );
                Func1( &tree, i );

                ++numRemoved;
            }
        }

        count -= numRemoved;

        if( list.GetCount() != count || tree.GetCount() != count )
        {
            TST_FAIL;
            goto fail;
        }

        //Make sure iterators 0 are invalid and the rest are valid.
        if( itList0.IsValid() ||
            !itList1.IsValid() ||
            !itList2.IsValid() ||
            itTree0.IsValid() ||
            !itTree1.IsValid() ||
            !itTree2.IsValid() )
        {
            TST_FAIL;
            goto fail;
        }

        //Test removal of items while iterating over a different iterator.

        numRemoved = 0;

        for( int i = 0; itList1.IsValid(); itList1.Next(), itTree1.Next(), ++i )
        {
            if( itList1.Item()->i != itTree1.Item()->i )
            {
                TST_FAIL;
                goto fail;
            }

            if( !( i % 3 ) )
            {
                //Remove an item
                Func0( &list, i );
                Func1( &tree, i );
                ++numRemoved;
            }
        }

        count -= numRemoved;

        if( list.GetCount() != count || tree.GetCount() != count )
        {
            TST_FAIL;
            goto fail;
        }

        //Make sure iterators 0 and 1 are invalid and the rest are valid.
        if( itList0.IsValid() ||
            itList1.IsValid() ||
            !itList2.IsValid() ||
            itTree0.IsValid() ||
            itTree1.IsValid() ||
            !itTree2.IsValid() )
        {
            TST_FAIL;
            goto fail;
        }
    }

    //Iterator 2 should no longer be valid because the safe containers
    //have been destroyed.
    if( itList2.IsValid() || itTree2.IsValid() )
    {
        TST_FAIL;
        goto fail;
    }

    TST_PASS;

fail:;
}

QA_ITEM_FAMILY( atSafeContainerQA, (), () );

QA_ITEM( atSafeContainerQA, (), qaResult::PASS_OR_FAIL );

#endif  //_QA
