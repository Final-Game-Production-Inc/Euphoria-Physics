// 
// pheffects/cloth_verlet_col_cloth.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "cloth_verlet.h"

#include "phbound/boundcapsule.h"
#include "phbound/boundcomposite.h"
#include "phbound/boundgeom.h"
#include "phbound/boundsphere.h"
#include "phbound/boundbox.h"
#include "phbound/boundtaperedcapsule.h"
#include "phbound/boundbvh.h"
#include "phbullet/TriangleShape.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "vectormath/classes.h"
#include "cloth_verlet_spu.h"


#if __WIN32
#pragma warning(disable:4100)
#endif

//#pragma optimize("", off)

namespace rage {

EXT_PFD_DECLARE_ITEM(VerletCollision);

extern int QuickRejectList(int nVertCount, Vector3::Vector3Param vRadius, Vector3::Vector3Param vCenter, const Vector3* RESTRICT pVerts, int* RESTRICT pOutValues, int nPin, const Vector3& offset /*= Vector3(Vector3::ZeroType)*/ );
extern bool PointOnTriangle(Vec3V_In p, Vec3V_In _a, Vec3V_In _b, Vec3V_In _c);


}  // namespace rage
