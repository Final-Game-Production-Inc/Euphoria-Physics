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

#ifndef NM_RS_CBU_DBM_PELVISCONTROL_H
#define NM_RS_CBU_DBM_PELVISCONTROL_H

#include "NmRsCBU_DBMCommon.h"

namespace ART
{
  bool dbmPelvisControl(
    const NmRsCBUDynBal_ReadOnly& ro,
    const NmRsCBUDynBal_BodyPacket& body,
    NmRsCBUDynBal_PelvisState& pelvis);
}

namespace rage
{
  struct sysTaskParameters;
}

DECLARE_TASK_INTERFACE( NmRsCBU_DBMPelvisControl );
void RAGETask_dbmPelvisControl( rage::sysTaskParameters & );


#endif // NM_RS_CBU_DBM_PELVISCONTROL_H

