// 
// system/new.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#include "new.h"

// This is for rage applications that don't use our memory system.

#if RAGE_ENABLE_RAGE_NEW && !defined(__UNITYBUILD)

	void* operator new(size_t size,const char*,int) {
		return ::operator new(size);
	}

	void* operator new[](size_t size,const char*,int) {
		return ::operator new(size);
	}

	void* operator new(size_t size,size_t /*align*/,const char*,int) {
		return ::operator new(size);
	}

	void* operator new[](size_t size,size_t /*align*/,const char*,int) {
		return ::operator new(size);
	}

#if __PS3 // special version for gcc when the rage_aligned_new is called on a class that is already ALIGNED
	void* operator new(size_t size,size_t /*alignType*/,size_t /*alignParam*/,const char*,int) {
		return ::operator new(size);
	}

	void* operator new[](size_t size,size_t /*alignType*/,size_t /*alignParam*/,const char*,int) {
		return ::operator new(size);
	}
#endif

#endif
