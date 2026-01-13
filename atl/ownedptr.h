//
// atl/ownedptr.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_OWNEDPTR_H
#define ATL_OWNEDPTR_H

namespace rage {

/*
	atOwnedPtr is a simpler version of atPtr which does not
	assume any reference counting.  This class is designed to
	be used as a member variable of any class that owns dynamically
	allocated data.  You are specifically not allowed to copy or
	assign from atOwnedPtr's directly because that would assume
	more than one owner, leading to objects being deleted twice.

	This class will work for anything, even basic C++ types.
	For example, atOwnedPtr<char> should be identical to ConstString.

	This class also makes atArray and its variants more useful, because
	by declaring an atArray< atOwnedPtr<whatever> > you can make sure
	that the array does a non-shallow cleanup.  Just be careful with
	any resizing that may be going on.

	The class implements operator* and operator-> and operator _Type* so that 
	in many cases the syntax is totally transparent.

	This class is specifically designed NOT to be resourced.
*/
template <class _Type>
class atOwnedPtr {
public:
	// PURPOSE: Default constructor.
	atOwnedPtr() : ptr(0) { }

	// PURPOSE: Constructor
	atOwnedPtr(_Type* that) : ptr(that) { }

	// PURPOSE: Destructor; frees underlying object
	~atOwnedPtr() { delete ptr; }

	// PURPOSE: Assignment operator
	// PARAMS:	that - New value
	// RETURNS:	that
	// NOTES:	Frees any storage associated with the current object.
	//			Assigning to null pointer is the same as explicit free.
	_Type* operator=(_Type* that) {
		if (ptr) delete ptr;
		ptr = that;
		return ptr;
	}

	// PURPOSE:	Implements operator*
	_Type& operator*() const { return *ptr; }

	// PURPOSE:	Implements operator->
	_Type* operator->() const { return ptr; }

	// PURPOSE:	Implements implicit conversion
	operator _Type*() const { return ptr; }

	// PURPOSE: Copy an owned pointer, transferring ownership in the process.
	// NOTES:	This destroys the item being assigned from, which is probably
	//			not what you were expecting!  However, it does allow atArray::Delete
	//			to still work sensibly.
	void operator=(atOwnedPtr<_Type>& that) {
		ptr = that.ptr;
		that.ptr = NULL;
	}

private:
	// PURPOSE: Make it impossible to copy an atOwnedPtr object
	atOwnedPtr(const atOwnedPtr<_Type>& /*that*/) { }

	// The actual data being tracked
	_Type *ptr;
};

}	// namespace rage

#endif
