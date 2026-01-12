/*
* Copyright (c) 2005-2010 NaturalMotion Ltd. All rights reserved. 
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

#include "NmRsInclude.h"
#include "NmRsCBU_Shot.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"

#include "NmRsCBU_TaskManager.h"
#include "NmRsCBU_PointGun.h"
#include "NmRsCBU_DynamicBalancer.h"

namespace ART
{
  //----------------POINT-GUN---------------------------------------
  bool NmRsCBUShot::pointGun_entryCondition()
  {
    return m_parameters.pointGun && !m_falling;
  }

  void NmRsCBUShot::pointGun_entry()
  {
    NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)m_cbuParent->m_tasks[bvid_pointGun];
    Assert(pointGunTask);
    if(!pointGunTask->isActive())
    {
      // at the moment, the game starts point gun externally.
      // allowing this to continue for the convenience of not
      // having to pass through a pile of parameters.
      pointGunTask->updateBehaviourMessage(NULL);
      // put custom params / pass-through here...
      pointGunTask->activate();
    }
  }

  void NmRsCBUShot::pointGun_tick(float /* timeStep */)
  {
    NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)m_cbuParent->m_tasks[bvid_pointGun];
    Assert(pointGunTask);

    // disable point gun altogether if balancer is nearing failure
    // conditions.
    const float minHeight = 0.7f;
    const float minBackwardsness = 0.7f;
    NmRsCBUDynamicBalancer *dynamicBalancerTask = (NmRsCBUDynamicBalancer *)m_cbuParent->m_tasks[bvid_dynamicBalancer];
    Assert(dynamicBalancerTask);
    bool falling = dynamicBalancerTask->m_height < minHeight;
    bool fallingBackwards = false;
    if(falling)
    {
      // are we falling backwards?
      // measure backwardsness around pelvis up.
      rage::Matrix34 pelvisTM;
      rage::Vector3 COMvelSplat;
      // project com vel on yz plane
      getSpine()->getPelvisPart()->getMatrix(pelvisTM);
      pelvisTM.UnTransform3x3(m_character->m_COMvel, COMvelSplat);
      COMvelSplat.x = 0.f;
      // dot with z axis
      fallingBackwards = COMvelSplat.Dot(m_character->m_gUp) > minBackwardsness;
#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), "shot.point", fallingBackwards);
#endif
    }

    if(falling && !fallingBackwards)
    {
      // disable point gun.
      if(pointGunTask->isActive())
        pointGunTask->deactivate();
    }
//#if 0 // this may be causing issues when point gun was not previously running.
    else
    {
      // re-enable point gun.
      if(!pointGunTask->isActive())
        pointGunTask->activate();
    }
//#endif

    // disable point gun supporting hand if a reach for wound is desired
    //mmmmmtodo remove shotPointGun: move below to reachForWound and have a deactivate on falling backwards in shot/pointGun/catchFall/etc?
    if(m_reachLeftEnabled && (m_parameters.rfwWithPistol))
      pointGunTask->enableLeftArm(false);
    else
      pointGunTask->enableLeftArm(true);
  }

  bool NmRsCBUShot::pointGun_exitCondition()
  {
    return !m_parameters.pointGun || m_falling;
  }

  void NmRsCBUShot::pointGun_exit()
  {
#if !NM_FALL2KNEESPOINTGUN
    NmRsCBUPointGun* pointGunTask = (NmRsCBUPointGun*)m_cbuParent->m_tasks[bvid_pointGun];
    Assert(pointGunTask);
    if(pointGunTask->isActive())
    {
      pointGunTask->deactivate();
    }
#endif
  }
}

