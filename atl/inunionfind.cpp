// 
// atl/atinbintree.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "inunionfind.h"
#include "qa/qa.h"

#if __QA

#include "math/random.h"

using namespace rage;

class qa_InUnionFindObject
{
public:

    unsigned m_Id;

    atInUnionFindLink<qa_InUnionFindObject> m_Link;

    void Print() const
    {
        Printf("%d ", m_Id);
    }
};

typedef atInUnionFind<qa_InUnionFindObject, &qa_InUnionFindObject::m_Link> qa_UnionFind;

class qa_atInUnionFindManual : public qaItem
{
public:

    void Init();
    void Shutdown();

    void Update(qaResult& result);

};

static void PrintClasses(qa_UnionFind& uf);

void
qa_atInUnionFindManual::Init()
{
}

void
qa_atInUnionFindManual::Shutdown()
{
}

void
qa_atInUnionFindManual::Update( qaResult& result )
{
    qa_UnionFind uf;

    int COUNT = 7;
    qa_InUnionFindObject* objs = rage_new qa_InUnionFindObject[ COUNT ];

	// Put the objects into the structure
    for (int index = 0; index < COUNT; ++index)
    {
        objs[index].m_Id = index;
        uf.Insert(&objs[index]);
    }

//    PrintClasses(uf);
    // 0       1       2       3       4       5       6

    if (uf.Find(&objs[0]) == uf.Find(&objs[2]))
    {
        TST_FAIL;
        goto fail;
    }

    if (uf.Find(&objs[3]) == uf.Find(&objs[6]))
    {
        TST_FAIL;
        goto fail;
    }

    // 0 <---> 1 <---> 2       3       4       5       6
    uf.Union(&objs[0], &objs[1]);
    uf.Union(&objs[1], &objs[2]);
//    PrintClasses(uf);

    if (uf.Find(&objs[0]) != uf.Find(&objs[2]))
    {
        TST_FAIL;
        goto fail;
    }

    // 0 <---> 1 <---> 2       3 <---> 4 <---> 5 <---> 6
    uf.Union(&objs[3], &objs[4]);
    uf.Union(&objs[4], &objs[5]);
    uf.Union(&objs[5], &objs[6]);
    PrintClasses(uf);

    if (uf.Find(&objs[3]) != uf.Find(&objs[6]))
    {
        TST_FAIL;
        goto fail;
    }

    if (uf.Find(&objs[1]) == uf.Find(&objs[5]))
    {
        TST_FAIL;
        goto fail;
    }

    // 0 <---> 1 <---> 2 <---> 3 <---> 4 <---> 5 <---> 6
    uf.Union(&objs[0], &objs[6]);
    //PrintClasses(uf);

    if (uf.Find(&objs[1]) != uf.Find(&objs[5]))
    {
        TST_FAIL;
        goto fail;
    }

	uf.Reset();
//	PrintClasses(uf);

	if (uf.Find(&objs[0]) == uf.Find(&objs[2]))
	{
		TST_FAIL;
		goto fail;
	}

	if (uf.Find(&objs[3]) == uf.Find(&objs[6]))
	{
		TST_FAIL;
		goto fail;
	}

    TST_PASS;

fail:

    for (int index = 0; index < COUNT; ++index)
    {
        uf.Remove(&objs[index]);
    }

    delete [] objs;
}

class qa_atInUnionFindAuto : public qaItem
{
public:
	qa_atInUnionFindAuto()
	{
	}

	qa_atInUnionFindAuto(qa_atInUnionFindAuto&)
	{
		AssertMsg(0, "Invalid copy ctor on qa_atInUnionFindAuto");
	}

	qa_atInUnionFindAuto operator=(const qa_atInUnionFindAuto&)
	{
		AssertMsg(0, "Invalid assignment on qa_atInUnionFindAuto");

		return *this;
	}

    void Init(int setSize, int randomUnions);
    void Shutdown();

    void Update( qaResult& result );

private:
    qa_InUnionFindObject* m_Objs;
    qa_UnionFind m_UF;
    int m_SetSize;
    int m_RandomUnions;
};

void
qa_atInUnionFindAuto::Init(int setSize, int randomUnions)
{
    m_SetSize = setSize;
    m_RandomUnions = randomUnions;

    m_Objs = rage_new qa_InUnionFindObject[ m_SetSize ];

    for (int index = 0; index < m_SetSize; ++index)
    {
        m_Objs[index].m_Id = index;
        m_UF.Insert(&m_Objs[index]);
    }

    //PrintClasses(m_UF);

    mthRandom rand;

    for (int u = 0; u < m_RandomUnions; ++u)
    {
        int a = rand.GetRanged(0, m_SetSize - 1);
        int b = rand.GetRanged(0, m_SetSize - 1);

        m_UF.Union(&m_Objs[a], &m_Objs[b]);    
    }

    //m_UF.PrintTree();
    //PrintClasses(m_UF);
}

void
qa_atInUnionFindAuto::Shutdown()
{
    qa_UnionFind::Iterator it(m_UF.GetFirstClass());
    qa_UnionFind::Iterator nextClass = it.GetNextClass();

#if 1
    while (it.Item())
    {
        //PrintClasses(m_UF);

        qa_InUnionFindObject* object = it.Item();

        it.Next();

        m_UF.Remove(object);
    }
#else
    m_UF.RemoveAll();
#endif

    delete [] m_Objs;
}

#define QA_ASSERT(X) if (!(X)) { TST_FAIL; return; }
void
qa_atInUnionFindAuto::Update( qaResult& result )
{
    qa_UnionFind::Iterator it(m_UF.GetFirstClass());

    int countObjects = 0;
    int countClasses = 1;
    int countObjectsInClass = 0;
    int objectsInClass = it.Item()->m_Link.m_Size;
    qa_UnionFind::Iterator nextClass = it.GetNextClass();
    while (it.Item())
    {
        ++countObjects;
        it.Next();

        if (it.Item() && it == nextClass)
        {
            nextClass = it.GetNextClass();
            ++countClasses;
            QA_ASSERT(objectsInClass == countObjectsInClass + 1); // Make sure m_Size reports correct count of objects
            objectsInClass = it.Item()->m_Link.m_Size;
            countObjectsInClass = 0;
        }
        else
        {
            ++countObjectsInClass;
        }
    }

    QA_ASSERT(countObjects == m_SetSize); // Make sure when we iterate we didn't lose any items
    QA_ASSERT(m_UF.GetObjectCount() == m_SetSize); // Make sure our object count isn't lying
    QA_ASSERT(m_UF.GetClassCount() == countClasses); // Make sure our class count agrees with our iteration results

    m_UF.Reset();
    //PrintClasses(m_UF);

    countClasses = 1;
    it = m_UF.GetFirstClass();
    nextClass = it.GetNextClass();
    while (it.Item())
    {
        QA_ASSERT(it.Item()->m_Link.m_Size == 1); // All equivalence classes shoud have size one

        it.Next();

        if (it.Item() && it == nextClass)
        {
            nextClass = it.GetNextClass();
            ++countClasses;
        }
    }

    QA_ASSERT(countClasses == m_SetSize); // Make sure after Reset we didn't lose any classes
    QA_ASSERT(m_UF.GetClassCount() == m_UF.GetObjectCount()); // Make sure after Reset every object is in its own class

    TST_PASS;
}

void
PrintClasses(qa_UnionFind& uf)
{
	qa_UnionFind::Iterator it(uf.GetFirstClass());
    qa_UnionFind::Iterator nextClass = it.GetNextClass();

	while (it.Item())
	{
        Printf("%d ", it.Item()->m_Id);

        it.Next();

        if (it == nextClass && it.Item())
        {
            nextClass = it.GetNextClass();
            Displayf("\n --- next class ---");
        }
	}

    Displayf("\n *** end ***");
}

QA_ITEM_FAMILY( qa_atInUnionFindManual, (), () );

QA_ITEM_FAST( qa_atInUnionFindManual, (), qaResult::FAIL_OR_TOTAL_TIME );

QA_ITEM_FAMILY( qa_atInUnionFindAuto, (int setSize, int randomUnions), (setSize, randomUnions) );

QA_ITEM_FAST( qa_atInUnionFindAuto, (10, 5), qaResult::FAIL_OR_TOTAL_TIME );
QA_ITEM_FAST( qa_atInUnionFindAuto, (10, 10), qaResult::FAIL_OR_TOTAL_TIME );
QA_ITEM_FAST( qa_atInUnionFindAuto, (10, 20), qaResult::FAIL_OR_TOTAL_TIME );

QA_ITEM_FAST( qa_atInUnionFindAuto, (100, 50), qaResult::FAIL_OR_TOTAL_TIME );
QA_ITEM_FAST( qa_atInUnionFindAuto, (100, 100), qaResult::FAIL_OR_TOTAL_TIME );
QA_ITEM_FAST( qa_atInUnionFindAuto, (100, 200), qaResult::FAIL_OR_TOTAL_TIME );

QA_ITEM_FAST( qa_atInUnionFindAuto, (1000, 500), qaResult::FAIL_OR_TOTAL_TIME );
QA_ITEM( qa_atInUnionFindAuto, (1000, 1000), qaResult::FAIL_OR_TOTAL_TIME );
QA_ITEM( qa_atInUnionFindAuto, (1000, 2000), qaResult::FAIL_OR_TOTAL_TIME );

QA_ITEM( qa_atInUnionFindAuto, (100000, 50000), qaResult::FAIL_OR_TOTAL_TIME );
QA_ITEM( qa_atInUnionFindAuto, (100000, 100000), qaResult::FAIL_OR_TOTAL_TIME );
QA_ITEM( qa_atInUnionFindAuto, (100000, 200000), qaResult::FAIL_OR_TOTAL_TIME );

#endif  //_QA
