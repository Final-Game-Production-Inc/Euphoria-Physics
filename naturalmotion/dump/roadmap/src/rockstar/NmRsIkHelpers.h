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

#ifndef IK_HELPERS_H
#define IK_HELPERS_H

namespace ART
{

  class NmRsEffectorBase;
  class NmRsEffectorInputWrapper;

  void fk(
    rage::Matrix34& parentMatrix1,
    rage::Matrix34& childMatrix1,
    const NmRsEffectorBase* parent,
    const NmRsEffectorBase* child,
    const NmRsEffectorInputWrapper* parentInput = 0);

  // helper functions for fk
  void getEffectorMatrix1FromParentPartTM(
    rage::Matrix34& parentPartTM,
    const NmRsEffectorBase* effector,
    rage::Matrix34& matrix1);
  void getPartMatrixFromParentEffectorMatrix2(
    rage::Matrix34& partMatrix,
    const NmRsEffectorBase* parentEffector,
    rage::Matrix34& parentMatrix2);
  void getEffectorMatrix2FromChildPartTM(
    rage::Matrix34& childPartTM,
    const NmRsEffectorBase* effector,
    rage::Matrix34& matrix2);
  rage::Quaternion getDriveQuatFromMatrices(
    rage::Matrix34& matrix1,
    rage::Matrix34& matrix2);

  void getEffectorMatrix2FromMatrix1AndDesiredDriveAngles(
    rage::Matrix34& matrix1,
    const NmRsEffectorBase* effector,
    rage::Matrix34& matrix2,
    const NmRsEffectorInputWrapper* input = 0);

} // namespace ART

#endif // IK_HELPERS_H
