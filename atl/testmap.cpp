//
// atl/testmap.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "map.h"

#include "bitset.h"
#include "string.h"

#include "diag/output.h"
#include "file/stream.h"
#include "file/token.h"
#include "system/main.h"
#include "system/param.h"

using namespace rage;


FPARAM(1,filename,"name of file to do word counts on");

/*
void computeAllSmallPrimes() {
	atFixedBitSet<65536> sieve;
	int i;
	for (i=2; i<=65535; i++) {
		for (int j=i+i; j<=65535; j+=i)
			sieve.Set(j,true);
	}
	int last = -100;
	int interval = 10;
	for (i=11; i<=65535; i++)
		if (!sieve.IsSet(i) && i - last > interval) {
			last = i;
			interval = (interval * 17) / 10;
			Printf("\t%d,\n",i);
		}
}
*/

int Main() {
	// computeAllSmallPrimes();

	atMap<atString,int> m;

	const char *filename = NULL;
	PARAM_filename.Get(filename);
	AssertMsg(filename,"Usage: filename.txt");
	fiStream *S = fiStream::Open(filename);
	Assert(S);
	fiTokenizer T(filename,S);
	char buf[128];

	while (T.GetToken(buf,sizeof(buf))) {
		int *count = m.Access(buf);
		if (!count)
			m.Insert(atString(buf),1);
		else
			++*count;
	}

	S->Close();

	atMap<atString,int> m2(m);

	for (int i=0; i<m2.GetNumSlots(); i++) {
		atMap<atString,int>::Entry *e = m2.GetEntry(i);
		while (e) {
			Displayf("Word [%s] appears %d times",(const char*)e->key,e->data);
			e = e->next;
		}
	}
	
	return 0;
}
