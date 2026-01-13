//
// atl/teststring.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "string.h"

#include "diag/output.h"
#include "system/main.h"

using namespace rage;


#define TEST(x)	atParsePathElements(atString(x),path,basename,ext); Displayf("[%s] : path=[%s] basename=[%s] ext=[%s]",x,(const char*)path,(const char*)basename,(const char*)ext)

#define DO(x)	Displayf("[%s] = %s",#x,x? "true":"false")

int Main() {
	atString path, basename, ext;

	TEST("c:\\one/two\\three.txt");
	TEST("");
	TEST("a");
	TEST("a.b");
	TEST("\\foo");
	TEST("c:\\a\\.txt");
	TEST("\\a.b.c.d");
	TEST("foo.backup\\baz");

	atString a("test"), b("blob"), c("test");
	DO(a == b);
	DO(a == "test");
	DO(b != "test");
	DO(a == c);
	DO(a != b);
	DO(a != c);

	return 0;
}
