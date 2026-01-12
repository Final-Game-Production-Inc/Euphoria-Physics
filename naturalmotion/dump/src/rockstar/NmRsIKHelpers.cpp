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

#include "NmRsInclude.h"
#include "NmRsIkHelpers.h"
#include "NmRsEffectors.h"
#include "NmRsEffectorInputWrapper.h"

namespace ART
{

  // compute child joint matrix 1 from parent joint matrix 1,
  // joint drive and part offsets.
  void fk(rage::Matrix34& parentMatrix1, rage::Matrix34& childMatrix1, const NmRsEffectorBase* parent, const NmRsEffectorBase* child, const NmRsEffectorInputWrapper* parentInput /*= 0*/)
  {
    rage::Matrix34 parentMatrix2;
    getEffectorMatrix2FromMatrix1AndDesiredDriveAngles(parentMatrix1, parent, parentMatrix2, parentInput);
    rage::Matrix34 partMatrix;
    getPartMatrixFromParentEffectorMatrix2(partMatrix, parent, parentMatrix2);
    getEffectorMatrix1FromParentPartTM(partMatrix, child, childMatrix1);
  }

  void getPartMatrixFromParentEffectorMatrix2(
    rage::Matrix34& partMatrix,
    const NmRsEffectorBase* parentEffector,
    rage::Matrix34& parentMatrix2)
  {
    rage::Matrix34 orientationChild;
    rage::phJoint* jt = parentEffector->getJoint();
    orientationChild = jt->GetOrientationChild();
    rage::Vector3 positionChild = jt->GetPositionChild();
    partMatrix = orientationChild;
    partMatrix.Dot3x3(parentMatrix2);
    partMatrix.Transform3x3(positionChild);
    partMatrix.d = parentMatrix2.d - positionChild;
  }

  void getEffectorMatrix1FromParentPartTM(
    rage::Matrix34& parentPartTM,
    const NmRsEffectorBase* effector,
    rage::Matrix34& matrix1)
  {
    rage::phJoint* jt = effector->getJoint();
    rage::Matrix34 orientationParent = jt->GetOrientationParent();//orientation of the joint's rotation axis in the parent's coordinates
    rage::Vector3 positionParent = jt->GetPositionParent();
    matrix1.Set(parentPartTM);
    matrix1.Transpose();//worldToPart
    matrix1.UnTransform3x3(positionParent);
    matrix1.Dot3x3(orientationParent);//worldToPart.partToJoint = worldToJoint
    matrix1.d += positionParent;
    matrix1.Transpose();
  }

  void getEffectorMatrix2FromChildPartTM(
    rage::Matrix34& childPartTM,
    const NmRsEffectorBase* effector,
    rage::Matrix34& matrix2)
  {
    rage::phJoint* jt = effector->getJoint();
    rage::Matrix34 oriChild;
    oriChild = jt->GetOrientationChild(); //orientation of the joint's rotation axis in the child's coordinates
    rage::Vector3 posChild = jt->GetPositionChild();
    matrix2.Set(childPartTM);
    matrix2.Transpose();
    matrix2.UnTransform3x3(posChild);
    matrix2.Dot3x3(oriChild);
    matrix2.d += posChild;
    matrix2.Transpose();
  }

  rage::Quaternion getDriveQuatFromMatrices(
    rage::Matrix34& matrix1,
    rage::Matrix34& matrix2)
  {
    rage::Matrix34 resultM;
    rage::Quaternion result;
    resultM.DotTranspose(matrix1, matrix2);
    result.FromMatrix34(resultM);
    return result;
  }

  void getEffectorMatrix2FromMatrix1AndDesiredDriveAngles(
    rage::Matrix34& matrix1,
    const NmRsEffectorBase* effector,
    rage::Matrix34& matrix2,
    const NmRsEffectorInputWrapper* input /*= 0*/)
  {
    rage::Quaternion driveQuat;

    // if we have an input, compute based on the drive angles in there, rather than those in the effector.
    if(input)
    {
      rage::Vector3 tss;
      if(effector->is3DofEffector())
      {
        const NmRs3DofEffectorInputWrapper* input3Dof = static_cast<const NmRs3DofEffectorInputWrapper*>(input);
        // check that we have valid data in all of the drive angles.
        if(input3Dof->m_data.m_flags & (NmRs3DofEffectorInputData::applyDesiredLean1 | NmRs3DofEffectorInputData::applyDesiredLean2 | NmRs3DofEffectorInputData::applyDesiredTwist))
        {
          tss.Set(input3Dof->getDesiredTwist(), input3Dof->getDesiredLean1(), input3Dof->getDesiredLean2());
          const NmRs3DofEffector* effector3Dof = static_cast<const NmRs3DofEffector*>(effector);
          effector3Dof->getRawTwistAndSwingFromTwistAndSwing(tss, tss);
          driveQuat = rsRageDriveTwistSwingToQuat(tss);
        }
        // otherwise use desired angles from effector.
        else
        {
          effector->getQuaternionFromDesiredRawAngles(driveQuat);
        }
      }
      else
      {
        const NmRs1DofEffectorInputWrapper* input1Dof = static_cast<const NmRs1DofEffectorInputWrapper*>(input);
        // check that we have valid data in all of the drive angles.
        if(input1Dof->m_data.m_flags & NmRs1DofEffectorInputData::applyDesiredAngle)
        {
          tss.Set(input1Dof->getDesiredAngle(), 0, 0);
          driveQuat = rsRageDriveTwistSwingToQuat(tss);
        }
        // otherwise use desired angles from effector.
        else
        {
          effector->getQuaternionFromDesiredRawAngles(driveQuat);
        }
      }
    }
    // otherwise use desired angles from effector.
    else
    {
      effector->getQuaternionFromDesiredRawAngles(driveQuat);
    }

    rage::Matrix34 driveMatrix;
    driveMatrix.Identity();
    driveMatrix.FromQuaternion(driveQuat);
    matrix2.Dot(driveMatrix, matrix1);
  }

} // namespace ART