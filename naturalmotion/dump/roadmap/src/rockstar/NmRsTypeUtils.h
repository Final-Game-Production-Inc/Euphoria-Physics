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

#ifndef NM_RS_TYPEUTILS_H
#define NM_RS_TYPEUTILS_H

namespace ART
{
  /**
   * Release a phBound, including recursing into all bounds owned
   * by a composite.
   */
  inline void recursiveDeleteBounds(rage::phBound *bound)
  {
    bound->Release(false);

    switch (bound->GetType())
    {
    case rage::phBound::COMPOSITE:
      {
        rage::phBoundComposite *composite = (rage::phBoundComposite *)bound;
        for (int i=0; i<composite->GetNumBounds(); i++)
        {
          rage::phBound* bound = composite->GetBound(i);
          composite->SetBound(i, NULL);
          recursiveDeleteBounds(bound);
        }
      }
      break;

    default:
      break;
    }
  }
}

#endif // NM_RS_TYPEUTILS_H

