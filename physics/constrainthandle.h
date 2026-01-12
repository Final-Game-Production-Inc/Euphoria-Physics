//
// physics/constrainthandle.h
//
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_HANDLE_H
#define PHYSICS_CONSTRAINT_HANDLE_H

namespace rage {

// PURPOSE: A safe mechanism for the user to refer to constraints
struct phConstraintHandle
{
	// Indexes into the virtual constraint record array
	u16 index;

	// Matched against the generation in the virtual constraint record to make sure the constraint is the one we're looking for
	u16 generation;

	phConstraintHandle() : index(0), generation(0) { }

	// Generation zero is never a valid generation, and it's the default, so a default constructed Id is automatically invalid.
	bool IsValid() const
	{
		return generation != 0;
	}

	void Reset()
	{
		index = 0;
		generation = 0;
	}
};


}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_HANDLE_H
