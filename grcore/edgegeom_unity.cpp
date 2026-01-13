//
// grcore/edgegeom_unity.cpp
//
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved.
//

#if __SPU
#include "system/dma.h"

#if __ASSERT
#define EDGE_GEOM_DEBUG
#endif // __ASSERT

#include "edge/geom/edgegeom_masks.cpp"
#include "edge/geom/edgegeom.cpp"
#include "edge/geom/edgegeom_c.cpp"
#include "edge/geom/edgegeom_compress.cpp"
#include "edge/geom/edgegeom_compress_c.cpp"
#include "edge/geom/edgegeom_cull.cpp"
#include "edge/geom/edgegeom_cull_c.cpp"
#include "edge/geom/edgegeom_decompress.cpp"
#include "edge/geom/edgegeom_decompress_c.cpp"
#include "edge/geom/edgegeom_occlusion.cpp"
#include "edge/geom/edgegeom_occlusion_c.cpp"
#include "edge/geom/edgegeom_occlusion_main.cpp"
#include "edge/geom/edgegeom_skin.cpp"
#include "edge/geom/edgegeom_skin_c.cpp"
#include "edge/geom/edgegeom_transform.cpp"
#include "edge/geom/edgegeom_transform_c.cpp"

#endif // __SPU