/*
 * Copyright (c) 2005-2012 NaturalMotion Ltd. All rights reserved. 
 *
 * Not to be copied, adapted, modified, used, distributed, sold,
 * licensed or commercially exploited in any manner without the
 * written consent of NaturalMotion. 
 *
 * All non public elements of this software are the confidential
 * information of NaturalMotion and may not be disclosed to any
 * person nor used for any purpose not expressly approved by
 * NaturalMotion in writing.
 *
 */

#ifndef NM_RS_INCLUDE_H
#define NM_RS_INCLUDE_H

#include "NmRsCommon.h"

#include "physics/levelnew.h"
#include "physics/simulator.h"
#include "physics/archetype.h"
#include "physics/contact.h"
#include "physics/collider.h"
#include "physics/sleep.h"
#include "physics/constraintdistance.h"
#include "physics/constraintfixed.h"
#include "phbound/bound.h"
#include "phbound/boundbox.h"
#include "phbound/boundcapsule.h"
#include "phbound/boundcylinder.h"
#include "phbound/boundsphere.h"
#include "phbound/boundgeom.h"
#include "phbound/boundcomposite.h"
#include "phcore/material.h"
#include "phcore/materialmgr.h"
#include "phcore/materialmgrimpl.h"
#include "pharticulated/articulatedbody.h"
#include "pharticulated/joint1dof.h"
#include "pharticulated/joint3dof.h"
#include "pharticulated/articulatedcollider.h"
#include "diag/output.h"
#include "system/param.h"
#include "system/task.h"
#include "vector/vec.h"
#include "math/random.h"
#include "math/amath.h"
#include "mathext/noise.h"
#include "physics/shapetest.h"


#include "art/ARTInternal.h"
#include "art/ARTFeedback.h"
#include "NMmath/OldMath.h"

#if ART_ENABLE_BSPY
#include "NmRsSpy.h"
#include "bspy/NmRsSpyPackets.h"
#endif // ART_ENABLE_BSPY 

// Switches to remove limbs debug overhead for bSpy profiling.
#if ART_ENABLE_BSPY && 1
#define ART_ENABLE_BSPY_LIMBS 1
#define ART_ENABLE_BSPY_EFFECTOR_SETBY 1
#define DEBUG_LIMBS_PARAMETER(_param) , _param
#else
#define ART_ENABLE_BSPY_LIMBS 0
#define ART_ENABLE_BSPY_EFFECTOR_SETBY 0
#define DEBUG_LIMBS_PARAMETER(_param)
#endif

#ifdef NM_HAS_FSEL_INTRINSIC
  #define nm_fsel_max_float(a, b) (float)__fsel((a)-(b), a, b)
  #define nm_fsel_min_float(a, b) (float)__fsel((a)-(b), b, a)
#else
  #define nm_fsel_max_float(a, b) (float)(a > b ? a : b)
  #define nm_fsel_min_float(a, b) (float)(a > b ? b : a)
#endif // NM_HAS_FSEL_INTRINSIC


#define SMALLEST_TIMESTEP 0.0000001f  // 10million fps

#define CODE_REVISION 102849           // Taken from the svn checkin

namespace ART
{
  class NmRsEngine;
  extern NmRsEngine* gRockstarARTInstance;
}

#endif // NM_RS_INCLUDE_H
