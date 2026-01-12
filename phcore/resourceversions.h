//
// phcore/resourceversions.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHCORE_RESOURCEVERSIONS_H
#define PHCORE_RESOURCEVERSIONS_H

#include "constants.h"

namespace rage {

// Change this whenever ph* structures change
// Higher level code that resources physics objects should
// add this value to its own private resource version number.
const int phResourceBaseVersionConstant = 30;
const int phResourceBaseVersion = phResourceBaseVersionConstant + 7 * POLYGON_INDEX_IS_U32 + 3 * POLY_MAX_VERTICES;

}

#endif
