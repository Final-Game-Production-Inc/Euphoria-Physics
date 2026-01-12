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

#ifndef NM_RS_MINIMALINC_H
#define NM_RS_MINIMALINC_H

/**
 * This file is included by the Dynamic Balancer modules, and any component that
 * we wish to be compiled separate from the main code (eg. for execution on SPU)
 *
 * Thus we bring in only the very minimal set of includes that are required to 
 * get those modules compiling. Please check with Harry before changing this file.
 */


#if __SPU
#include "vector/vector3_consts_spu.cpp"
#endif

#include "vector/matrix34.h"
#include "vector/quaternion.h"
#include "system/taskheader.h"
#include "math/amath.h"

#include "NMutils/NMtypes.h"

// Ensures bSpy debug draw functionality when character is not available
#if ART_ENABLE_BSPY
#define BSPY_DRAW_COORDINATEFRAME(_tm_, _size_) { \
  BSPY_DRAW_LINE(_tm_.d, (_tm_.d+(_tm_.a*_size_)), rage::Vector3(1,0,0)) \
  BSPY_DRAW_LINE(_tm_.d, (_tm_.d+(_tm_.b*_size_)), rage::Vector3(0,1,0)) \
  BSPY_DRAW_LINE(_tm_.d, (_tm_.d+(_tm_.c*_size_)), rage::Vector3(0,0,1)) \
}
#define BSPY_DRAW_LINE(_start_, _end_, _color_) if (ART::bSpyServer::inst()->shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_DebugDraw)) { \
  bSpy::DebugLinePacket dlp(-1); \
  dlp.m_start = bSpyVec3fromVector3(_start_); \
  dlp.m_end = bSpyVec3fromVector3(_end_); \
  dlp.m_colour = bSpyVec3fromVector3(_color_); \
  bspySendPacket(dlp); \
}
#define BSPY_DRAW_POINT(_pos_, _size_, _color) { \
  BSPY_DRAW_LINE((_pos_+rage::Vector3(_size_,0,0)), (_pos_-rage::Vector3(_size_,0,0)), _color) \
  BSPY_DRAW_LINE((_pos_+rage::Vector3(0,_size_,0)), (_pos_-rage::Vector3(0,_size_,0)), _color) \
  BSPY_DRAW_LINE((_pos_+rage::Vector3(0,0,_size_)), (_pos_-rage::Vector3(0,0,_size_)), _color) \
}
#else
#define BSPY_DRAW_COORDINATEFRAME(...)
#define BSPY_DRAW_COORDINATEFRAME2(...)
#define BSPY_DRAW_LINE(...)
#define BSPY_DRAW_POINT(...)
#endif

#endif // NM_RS_MINIMALINC_H
