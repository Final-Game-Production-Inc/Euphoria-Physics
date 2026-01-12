// 
// data/base.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_BASE_H
#define DATA_BASE_H

#include "struct.h"		// for HAS_PADDED_POINTERS

namespace rage {

/*
PURPOSE
	Defines a simple base class "Base" containing a single virtual destructor. 
	Any C++ class which wants to have its member functions called directly by 
	a <c>datCallback</c> needs to derive from this class. You can always 
	write a simple static stub function to do the same thing rather than
	having your class derive from the <c>datBase</c> class.
<FLAG Component>
*/
#if HAS_PADDED_POINTERS
class datBaseBase {
public:
	virtual ~datBaseBase();
};

class datBase: public datBaseBase {
	unsigned vptrPadded;
};
#else
class datBase {
public:
	virtual ~datBase();
};
#endif

template <class T> void SafeRelease(T *&ptrRef) {
	if (ptrRef) {
		ptrRef->Release();
		ptrRef = NULL;
	}
}

#if __ASSERT
#define LastSafeRelease(p)	do { FastAssert(!(p) || !(p)->Release()); (p) = NULL; } while (0)
#else
#define LastSafeRelease(p)	::rage::SafeRelease(p)
#endif

#if __PROFILE || __FINAL
#define ValidateRefCount(x)
#elif __SPU
#define ValidateRefCount(x)		__builtin_spu_hcmpgt(0,(int)x)		// halt if 0 > (int)x
#elif __XENON
#define ValidateRefCount(x)		do { if (int(x)<0) *(int*)0xD1E = int(x); } while (0)
#else
#define ValidateRefCount(x)		do { if (int(x)<0) __builtin_trap(); } while (0)
#endif

}	// namespace

#endif
