//
// data/base.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "base.h"

using namespace rage;

#if HAS_PADDED_POINTERS
datBaseBase::~datBaseBase() {
}
#else
datBase::~datBase() {
}
#endif
