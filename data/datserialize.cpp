//
// data/serialize.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "serialize.h"

using namespace rage;

datSerialize & rage::datNewLine( datSerialize &s ) {
	 // This is trivial to do by hand, but wanted to provide an example of how to use
	 //	a manipulator
	 if ( s.IsBinary() == false && s.IsRead() == false ) {
		s.Put("\r\n");
	}
		 
	 return s;
}
 
