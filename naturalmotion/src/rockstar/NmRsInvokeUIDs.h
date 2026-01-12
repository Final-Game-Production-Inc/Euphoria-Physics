/*
 * Copyright (c) 2005-2007 NaturalMotion Ltd. All rights reserved. 
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

#ifndef NM_RS_INVOKEUIDS_H
#define NM_RS_INVOKEUIDS_H

/**
 * This file contains a list of UIDs that can be passed to the directInvoke() function
 * of NmRsEngine - we need to share this list with RAGE / game-side
 */

enum
{
    nmInvoke_Start          = 0x1000,   // do not change

    // ------------ add new invoke UIDs here, "nmInvoke_<description>"

      nmInvoke_StopAllBehaviours,

    // ------------

    nmInvoke_End            = 0x2000    // do not change
};

#endif // NM_RS_INVOKEUIDS_H
