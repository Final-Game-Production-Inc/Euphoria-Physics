//
// atl/testqueue.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "queue.h"

#include "diag/output.h"
#include "system/main.h"

using namespace rage;


atQueue<int,5> queue;


void PrintQueue()
{
	Printf("queue = ");
	for (int i=0;i<queue.GetCount();i++)
		Printf("%d ",queue[i]);
	Printf("\n");
}


void PrintItem(int i)
{
	Displayf("queue[%d] = %d",i,queue[i]);
}

void FindTest(const int* testData)
{
	for (int index=-1; *testData>=0; testData++)
	{
		Printf("Find(%d) returned ", *testData);
		if (queue.Find(*testData, &index))
			Printf("slot %d -- which contains %d.\n", index, queue[index]);
		else
			Printf("<invalid slot>.\n");
	}
}


int Main()
{
	queue.Push(1);
	queue.Push(2);
	queue.Push(3);
	queue.Push(4);
	queue.Push(5);

	PrintQueue();						// 1 2 3 4 5

	const int findTestData[] = {4,3,1,9,5,4,2,-1};

	FindTest(findTestData);

	queue.Pop();
	queue.Pop();
	queue.Pop();

	PrintQueue();						// 4 5

	queue.Push(6);
	queue.Push(7);
	queue.Push(8);

	PrintQueue();						// 4 5 6 7 8

	PrintItem(queue.GetCount()-1);		// [4] = 8

	queue.Delete(queue.GetCount() - 1);

	PrintQueue();						// 4 5 6 7

	queue.Push(9);

	PrintQueue();						// 4 5 6 7 9

	PrintItem(2);						// [2] = 6

	queue.Delete(2);

	FindTest(findTestData);

	PrintQueue();						// 4 5 7 9

	queue.Delete(0);

	PrintQueue();						// 5 7 9

	queue.Insert(0, 4);

	PrintQueue();						// 4 5 7 9

	queue.Insert(2, 6);

	PrintQueue();						// 4 5 6 7 9

	queue.Pop();
	queue.Insert(3, 8);

	PrintQueue();						// 5 6 7 8 9

	return 0;
}
