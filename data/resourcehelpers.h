// 
// data/resourcehelpers.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_RESOURCEHELPERS_H
#define DATA_RESOURCEHELPERS_H

#include "data/resource.h"

namespace rage {

// PURPOSE: An object should call this function in its resource constructor
// on each member variable that points to an object this object owns. For example
// if a vehicle has a Wheel* pointer to a wheel that it owns, it should call
// ObjectFixup on that wheel.
template <class T> inline void ObjectFixup(datResource &rsc,T* &ptr) {
	if (ptr) {
		rsc.PointerFixup(ptr);
		::new ((void*)ptr) T(rsc);
	}
}

// PURPOSE: An object should call this function in its resource constructor
// on each member variable that's an array of pointers to owned objects. For example
// if a vehicle has an array of Wheel* pointers, it should call ObjectFixup on that
// array
template <class T> inline void ObjectFixup(datResource &rsc,T* &ptr,int count) {
	if (ptr) {
		rsc.PointerFixup(ptr);
		for (int i=0; i<count; i++)
			::new ((void*)(ptr+i)) T(rsc);
	}
}

// PURPOSE: An object should call this function in its resource constructor
// on each member variable that's a pointer to an array of pointers to owned objects. For example
// if a vehicle has a pointer to an array of Wheel* pointers, it should call ObjectFixup on that
// pointer
template <class T> inline void PtrArrayObjectFixup(datResource &rsc, T** &ptr,int count) {
	if (ptr) {
		rsc.PointerFixup(ptr);
		for (int i=0; i<count; i++) {
			rsc.PointerFixup(ptr[i]);
			::new ((void*)*(ptr+i)) T(rsc);
		}
	}
}

template <class T> inline void PtrArrayObjectUnPlace(T** ptr, int count = 1) {
	if (ptr) {
		for (int i=0; i<count; i++)
			ptr[i]->UnPlace();
	}
}

template <class T> inline void ObjectUnPlace(T* ptr,int count=1) {
	if (ptr) {
		for (int i=0; i<count; i++)
			(ptr + i)->UnPlace();
	}
}

// PURPOSE: An object should call this function in its resource constructor
// on each member variable that's a pointer where the pointed to object could
// be an unknown derived class of the pointer type. It's assumed that T::VirtualConstructFromPtr
// both exists and knows what to do.
template <class T> inline void VirtualConstructFromPtr(datResource &rsc,T* &ptr) {
	if (ptr) {
		rsc.PointerFixup(ptr);
		T::VirtualConstructFromPtr(rsc, ptr);
	}
}

// PURPOSE: An object should call this function in its resource constructor
// on each member variable that's an array of pointers, where the pointed to objects could
// be an unknown derived class of the pointer type. It's assumed that T::VirtualConstructFromPtr
// both exists and knows what to do.
template <class T> inline void VirtualConstructFromPtr(datResource &rsc,T* &ptr,int count) {
	if (ptr) {
		rsc.PointerFixup(ptr);
		for (int i=0; i<count; i++)
			T::VirtualConstructFromPtr(rsc, ptr + i);
	}
}

}	// namespace rage

#endif
