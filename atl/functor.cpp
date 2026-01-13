//
// atl/functor.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "atl/functor.h"

#include <string.h>		// for memcpy

using namespace rage;

FunctorBase::FunctorBase(const void *c,PFunc f,const void *mf,size_t sz)
{
	if(c)	//must be callee/memfunc
	{
		Callee = (void *)c;
		Assert(sz<=MEM_FUNC_SIZE);
		memcpy(MemFunc,mf,sz);

		if(sz<MEM_FUNC_SIZE)	//zero-out the rest, if any, so comparisons work
			memset(MemFunc+sz,0,MEM_FUNC_SIZE-sz);
	}
	else	//must be ptr-to-func
	{
		Callee = NULL;
		Func = f;
	}
}

namespace rage {

bool operator==(const FunctorBase &lhs,const FunctorBase &rhs)
{
	// must be just a pointer to function...
	if (lhs.Callee==NULL && rhs.Callee==NULL)
	{
		return lhs.Func==rhs.Func;
	}
	// ...or we're totally different (one's a pointer to member function, 
	// one's a pointer to a regular function)...
	else if (lhs.Callee!=rhs.Callee)
		return false;
	// ...if we get here, we're pointing to the same instance, so let's 
	// check if we're pointing to the same member function:
	else 
	{
		return memcmp(lhs.MemFunc,rhs.MemFunc,FunctorBase::MEM_FUNC_SIZE)==0;
	}
}

}
