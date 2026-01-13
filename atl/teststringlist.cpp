//
// atl/teststringlist.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

// Simple tester to demonstrate both ASSET.EnumFiles and arrays of strings.
//
// A list is constructed of all the files in the current directory,
// excluding directories.  The list is then printed, then destroyed
// one node at a time.

#include "string.h"

#include "slist.h"

#include "file/asset.h"
#include "file/device.h"	// for fiFindData
#include "system/main.h"
#include "system/timer.h"

using namespace rage;

atSList<atString> stringList;


void callback(const fiFindData& data,void*)
{
	if (data.m_Attributes & FILE_ATTRIBUTE_DIRECTORY)
		Displayf("folder '%s' skipped",data.m_Name);
	else
	{
		atSNode<atString> *node=rage_new atSNode<atString>(atString(data.m_Name));
		stringList.Append(*node);
		Displayf("  file '%s' added",data.m_Name);
	}
}


int Main()
{
	Displayf("\nCreating list of files:\n");
	ASSET.EnumFiles(".",callback,0);

	Displayf("\nList contains:\n");
	atSNode<atString> *node=stringList.GetHead();
	while (node)
	{
		atString string=node->Data;
		Displayf("string '%s'",(const char*)string);
		node=node->GetNext();
	}

	Displayf("\nPopping from list:\n");
	while ((node=stringList.PopHead()) != NULL)
	{
		Displayf("Popped %s off list",(const char*)node->Data);
		delete node;
	}

	Displayf("\nList is %s.\n",stringList.GetHead()?"not empty":"empty");

	return 0;
}
