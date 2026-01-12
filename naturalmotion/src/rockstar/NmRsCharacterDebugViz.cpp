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

#define NM_DEBUGVIZ_VERBOSE 0

#if ART_ENABLE_BSPY

#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsUtils.h"
#include "NmRsGenericPart.h"

namespace ART
{
  bool NmRsCharacter::bspyDebugDrawIsInhibited()
  {
    return ( 
      (getBSpyID() == INVALID_AGENTID) ||
      !ART::bSpyServer::inst()->shouldTransmit(bSpy::TransmissionControlPacket::bSpyTF_DebugDraw)
      );
  }

  /**
  * draw a classic 'elongated diamond' bone shape using bspyDraw calls, between points a and b
  */
  void NmRsCharacter::drawBone(const rage::Vector3& a, const rage::Vector3& b, const rage::Vector3& axis, const rage::Vector3& col, float finScale)
  {
    rage::Vector3 midPt, finUp, finLeft, finRight, finDown;

    // create checkerboard colours to draw bone diamonds with
    rage::Vector3 colA(col), colB(col);
    colB *= 0.75f;

    // get direction of bone, store length for later
    rage::Vector3 dir = (b - a);
    float length = dir.Mag();
    dir.Normalize();

    // form a matrix looking down bone direction
    rage::Matrix34 mat;
    mat.LookDown(dir, axis);
    mat.d = a;

    // midPoint is shifted down the bone by 1/4 from top
    midPt.Scale(mat.c, length * 0.25f);

    // form fin points away from bone
    finLeft = mat.a;
    finLeft *= finScale;
    finLeft += midPt;
    finLeft += mat.d;

    finUp = mat.b;
    finUp *= finScale;
    finUp += midPt;
    finUp += mat.d;

    finRight = mat.a;
    finRight *= -finScale;
    finRight += midPt;
    finRight += mat.d;

    finDown = mat.b;
    finDown *= -finScale;
    finDown += midPt;
    finDown += mat.d;

    bspyDrawLine(a, finUp, col);
    bspyDrawLine(a, finDown, col);
    bspyDrawLine(a, finLeft, col);
    bspyDrawLine(a, finRight, col);

    bspyDrawLine(b, finUp, col);
    bspyDrawLine(b, finDown, col);
    bspyDrawLine(b, finLeft, col);
    bspyDrawLine(b, finRight, col);
  }

  void NmRsCharacter::drawSkeleton(bool desiredSkeleton)
  {
    if (!getArticulatedWrapper() || bspyDebugDrawIsInhibited())
      return;

    rage::Vector3 offset = RCC_VECTOR3(getArticulatedWrapper()->getArticulatedCollider()->GetMatrix().GetCol3ConstRef());
    rage::Vector3 boneCol;

    if (desiredSkeleton)
      boneCol.Set(0.0f, 0.3f, 1.0f);
    else
      boneCol.Set(1.0f, 0.3f, 0.0f);

    Assert(getNumberOfParts() < 28); // otherwise up the hard coded number below
    rage::Matrix34 partMatrixList[28];
    rage::Vector3 jointPosList[28];

    rage::phArticulatedBody *body = getArticulatedBody();
    partMatrixList[0] = MAT34V_TO_MATRIX34(body->GetLink(0).GetMatrix());
    partMatrixList[0].Transpose();
    partMatrixList[0].d += offset;

    // we're also going to work out the shift in COM.
    float totalMass = body->GetMass(0).Getf();
    rage::Vector3 COMshift(0,0,0);

    rage::Vector3 COM = m_COM;
    rage::Vector3 rotShift(0,0,0);
    float totalMoment = (partMatrixList[0].d - COM).Mag2() * totalMass; // moment from root part

    rage::Matrix34 mat;
    for (int i = 0; i<getNumberOfEffectors(); i++)
    {
      const NmRsEffectorBase *effector = getConstEffector(i);
      rage::Matrix34 orientationParent, orientationChild;
      rage::Vector3 positionParent, positionChild;
      rage::Vector3 twistSwing;

      if (effector->is3DofEffector())
      {
        NmRs3DofEffector *e3dof = (NmRs3DofEffector *)effector;
        rage::phJoint3Dof *threeDof = e3dof->get3DofJoint();
        orientationParent = threeDof->GetOrientationParent();
        orientationChild = threeDof->GetOrientationChild();
        positionParent = threeDof->GetPositionParent();
        positionChild = threeDof->GetPositionChild();
        float lean1 = (desiredSkeleton ? e3dof->getDesiredLean1() : e3dof->getActualLean1()) - e3dof->getMidLean1();
        if (e3dof->getInfo().reverseFirstLeanMotor)
          lean1 = -lean1;
        float lean2 = (desiredSkeleton ? e3dof->getDesiredLean2() : e3dof->getActualLean2()) - e3dof->getMidLean2();
        if (e3dof->getInfo().reverseSecondLeanMotor)
          lean2 = -lean2;
        float twist = (desiredSkeleton ? e3dof->getDesiredTwist() : e3dof->getActualTwist())- e3dof->getMidTwist();
        if (e3dof->getInfo().reverseTwistMotor)
          twist = -twist;
        twistSwing.Set(twist, lean1, lean2); 
      }
      else
      {
        NmRs1DofEffector *e1dof = (NmRs1DofEffector *)effector;
        rage::phJoint1Dof *oneDof = e1dof->get1DofJoint();
        orientationParent = oneDof->GetOrientationParent();
        orientationChild = oneDof->GetOrientationChild();
        positionParent = oneDof->GetPositionParent();
        positionChild = oneDof->GetPositionChild();
        twistSwing.Set(desiredSkeleton ? e1dof->getDesiredAngle() : e1dof->getActualAngle(),0,0); 
      }

      // find jointMatrix1 from current partMatrix of the parent
      rage::Matrix34 jointMat1 = partMatrixList[effector->getParentIndex()];
      rage::Vector3 pos;
      jointMat1.Transpose();
      jointMat1.UnTransform3x3( positionParent, pos );
      jointMat1.Dot3x3(orientationParent);
      jointMat1.d += pos;
      jointMat1.Transpose();
      // instance->addAxis(rage::NMDRAW_JOINT, 0.1f, jointMat1);
      // find jointMatrix2 from the specified swing/twist values
      rage::Quaternion quat = rsRageDriveTwistSwingToQuat(twistSwing);
      rage::Matrix34 rot, jointMat2;
      rot.FromQuaternion(quat);
      jointMat2.Dot3x3(rot, jointMat1);
      jointMat2.d = jointMat1.d;
      // instance->addAxis(rage::NMDRAW_JOINT, 0.1f, jointMat2);
      // find child's part matrix from the jointMatrix2
      rage::Matrix34 childPartMat = orientationChild;
      childPartMat.Dot3x3(jointMat2);
      childPartMat.Transform3x3( positionChild, pos );
      childPartMat.d = jointMat2.d - pos;
      // add into list
      partMatrixList[i+1] = childPartMat;
      jointPosList[i] = jointMat1.d;

      // accumulate the shift in COM
      float mass = body->GetMass(i+1).Getf();
      totalMass += mass;
      rage::Vector3 partPos = body->GetLink(i+1).GetPosition() + offset;
      rage::Vector3 shift = partMatrixList[i+1].d - partPos;
      COMshift += shift;
      // accumulate rotational change for conservation of angular momentum
      rage::Vector3 rotVec;
      rotVec.Cross(shift, partPos - COM);
      totalMoment += (partPos - COM).Mag2()*mass;
      rotShift += rotVec*mass;
    }
    COMshift /= totalMass; // conservation of momentum
    rotShift /= totalMoment;
    rage::Matrix34 rot; // conservation of angular momentum
    float angle = rotShift.Mag();
    rotShift.Normalize();
    rot.MakeRotate(rotShift, angle);
    rot.MakeTranslate(0,0,0);

    // determine the transform between the desired
    // and actual position of the desired root joint
    rage::Matrix34 desiredRootTMSkeletonInv, desiredRootTMActual;
    if(m_skeletonVizRoot != 0)
    {
      desiredRootTMSkeletonInv.Set(partMatrixList[m_skeletonVizRoot]);
      desiredRootTMSkeletonInv.d -= COM;
      desiredRootTMSkeletonInv.Dot(rot);
      desiredRootTMSkeletonInv.d += COM - COMshift;
#if 0
      bspyDrawCoordinateFrame(0.1f, desiredRootTMSkeletonInv);
#endif
      desiredRootTMSkeletonInv.Inverse();

      getGenericPartByIndex(m_skeletonVizRoot)->getMatrix(desiredRootTMActual);
#if 0
      bspyDrawCoordinateFrame(0.1f, desiredRootTMActual);
#endif
    }

    // draw part matrices
    for (int i = 0; i<getNumberOfParts(); i++)
    {
      // put in world frame
      partMatrixList[i].d -= COM;
      partMatrixList[i].Dot(rot);
      partMatrixList[i].d += COM - COMshift;

      if(m_skeletonVizRoot != 0)
      {
        //re-root
        partMatrixList[i].Dot(desiredRootTMSkeletonInv);
        partMatrixList[i].Dot(desiredRootTMActual);
      }

      if(isPartInMask(m_skeletonVizMask, i))
      {

#if NM_DEBUGVIZ_VERBOSE
        bspyDrawCoordinateFrame(0.025f, partMatrixList[i]);
        rage::Matrix34 temp;
        getGenericPartByIndex(i)->getMatrix(temp);
        bspyDrawCoordinateFrame(0.025f, temp);
#endif
      }

    }
    // draw joints
    for (int i = 0; i<getNumberOfEffectors(); i++)
    {
      // put in world frame
      jointPosList[i] -= COM;
      jointPosList[i].Dot3x3(rot);
      jointPosList[i] += COM - COMshift;

      if(m_skeletonVizRoot != 0)
      {
        //re-root
        desiredRootTMSkeletonInv.Transform(jointPosList[i]);
        desiredRootTMActual.Transform(jointPosList[i]);
      }

      const NmRsEffectorBase *effector = getConstEffector(i);
      int parentIndex = effector->getParentIndex();        
      rage::Vector3 axis = partMatrixList[parentIndex].a;

      if(isEffectorInMask(m_skeletonVizMask, parentIndex-1) && isEffectorInMask(m_skeletonVizMask, parentIndex))
      {
        if (parentIndex>0)
        {
          float oppose = rage::Clamp(getConstEffector(parentIndex-1)->getOpposeGravity(), 0.f, 2.f)/2.f;
          rage::Vector3 newBoneCol(1,1,1);
          newBoneCol = boneCol + (newBoneCol-boneCol)*oppose;
          drawBone(jointPosList[parentIndex-1], jointPosList[i], axis, newBoneCol, 0.03f);
        }
        else
        {
          if(m_bodyIdent == rdrCowboy || m_bodyIdent == rdrCowgirl)//spine0 is root body
          {        
            float oppose = rage::Clamp(getConstEffector(rdrJtSpine_0)->getOpposeGravity(), 0.f, 2.f)/2.f;
            rage::Vector3 newBoneCol(1,1,1);
            newBoneCol = boneCol + (newBoneCol-boneCol)*oppose;
            drawBone(jointPosList[rdrJtSpine_0], jointPosList[i], axis, newBoneCol, 0.03f);
          }
          else if(isBiped())//all other bipeds have pelvis as root body
          {
            float oppose = rage::Clamp(getConstEffector(gtaJtSpine_0)->getOpposeGravity(), 0.f, 2.f)/2.f;
            rage::Vector3 newBoneCol(1,1,1);
            newBoneCol = boneCol + (newBoneCol-boneCol)*oppose;
            drawBone(jointPosList[gtaJtSpine_0], jointPosList[i], axis, newBoneCol, 0.03f);
          }
        }

        rage::Vector3 temp;
        // draw the leaves (draw to opposite side of leaf part than the joint)
        if (i+1==getNumberOfParts()-1 || getConstEffector(i+1)->getParentIndex()!=i+1)
        {
          temp = (partMatrixList[i+1].d-jointPosList[i]);
          temp *= 1.5f;
          temp.Add(jointPosList[i]);

          float oppose = rage::Clamp(effector->getOpposeGravity(), 0.f, 2.f)/2.f;
          rage::Vector3 newBoneCol(1,1,1);
          newBoneCol = boneCol + (newBoneCol-boneCol)*oppose;
          drawBone(jointPosList[i], temp, partMatrixList[i+1].a, newBoneCol, 0.03f);
        }
      }
    }
  }

  void NmRsCharacter::bspyDrawLine(const rage::Vector3& start, const rage::Vector3& end, const rage::Vector3& colour)
  {
    if (bspyDebugDrawIsInhibited())
      return;
    bSpy::DebugLinePacket dlp((bSpy::bs_int8)getBSpyID());
    dlp.m_start = bSpyVec3fromVector3(start);
    dlp.m_end = bSpyVec3fromVector3(end);
    dlp.m_colour = bSpyVec3fromVector3(colour);
    bspySendPacket(dlp);
  }

  void NmRsCharacter::bspyDrawPoint(const rage::Vector3& position, float size, const rage::Vector3& colour)
  {
    if (bspyDebugDrawIsInhibited())
      return;
    bspyDrawLine(position-rage::Vector3(size,0.f,0.f), position+rage::Vector3(size,0.f,0.f), colour);
    bspyDrawLine(position-rage::Vector3(0.f,size,0.f), position+rage::Vector3(0.f,size,0.f), colour);
    bspyDrawLine(position-rage::Vector3(0.f,0.f,size), position+rage::Vector3(0.f,0.f,size), colour);
  }

  void NmRsCharacter::bspyDrawCoordinateFrame(float size, const rage::Matrix34& tm)
  {
    if (bspyDebugDrawIsInhibited())
      return;
    bspyDrawLine(tm.d, tm.d+tm.a*size, rage::Vector3(1.f,0.f,0.f));
    bspyDrawLine(tm.d, tm.d+tm.b*size, rage::Vector3(0.f,1.f,0.f));
    bspyDrawLine(tm.d, tm.d+tm.c*size, rage::Vector3(0.f,0.f,1.f));
  }

  void NmRsCharacter::bspyDrawTorque(float scale, const rage::Vector3& position, const rage::Vector3& torque, const rage::Vector3& colour)
  {
    if (bspyDebugDrawIsInhibited())
      return;
    rage::Vector3 unit(torque);
    unit.Normalize();
    rage::Vector3 radius;
    radius.Set(unit.y,-unit.x,unit.z); // get an orthogonal vector. this may have problems...

    // scale based on torque magnitude
    float size = scale * torque.Mag();

    // draw the arc
    const int steps = 8;
    rage::Quaternion q;
    q.FromRotation(unit, 2.f*PI/(float)steps);
    rage::Vector3 start, end, tan;
    start.Set(position + radius * size);
    int i;
    for(i = 0; i < steps-1; ++i)
    {
      q.Transform(radius);
      end.Set(position + radius * size);
      bspyDrawLine(start, end, colour);
      tan.Set(start-end);
      start.Set(end);
    }

    // draw the arrow cap
    q.Transform(tan);
    bspyDrawLine(end, end+tan, colour);
    radius.Set(start-end);
    q.Inverse();
    q.Transform(tan);
    q.Transform(tan);
    bspyDrawLine(end, end+tan, colour);
  }

  // debug draw for constraint distance.
  // not the most efficient way to do this, but... meh.
  // get an arbitrary orthogonal vector.
  rage::Vector3 getOrthogonal(const rage::Vector3& unit)
  {
    rage::Vector3 result;
    rage::Vector3 v1(1, 0, 0);
    float v1DotUnit = v1.Dot(unit);
    if(v1DotUnit > 0.5f || v1DotUnit < -0.5)
      v1.Set(0, 1, 0);
    result.Cross(v1, unit);
    result.Normalize();
    return result;
  }
  // draw a circle around pos.
  void NmRsCharacter::bspyDrawCircle(const rage::Vector3& pos, const rage::Vector3& axis, float radius, const rage::Vector3& colour, unsigned int segments /* = 8 */)
  {
    if (bspyDebugDrawIsInhibited() || segments == 0)
      return;
    float step = ( 2.f * PI ) / segments;
    rage::Vector3 p1(getOrthogonal(axis));
    rage::Vector3 p2;
    rage::Quaternion q;
    p1.Scale(radius);
    q.FromRotation(axis, step);
    unsigned int i;
    for(i = 0; i < segments; ++i)
    {
      q.Transform(p1, p2);
      bspyDrawLine(pos + p1, pos + p2, colour);
      p1.Set(p2);
    }
  }
#if NM_UNUSED_CODE
  void NmRsCharacter::bspyDrawArc(const rage::Vector3& pos, const rage::Vector3& axis, const rage::Vector3& center, const float length, const rage::Vector3& colour, unsigned int segments /* = 8 */)
  {
    if (bspyDebugDrawIsInhibited() || segments == 0)
      return;
    float step = length / segments;
    rage::Quaternion q;
    rage::Vector3 p1(center);
    q.FromRotation(axis, length / -2.f);
    q.Transform(p1);
    rage::Vector3 p2;
    q.FromRotation(axis, step);
    bspyDrawLine(pos, pos + p1, colour);
    unsigned int i;
    for(i = 0; i < segments; ++i)
    {
      q.Transform(p1, p2);
      bspyDrawLine(pos + p1, pos + p2, colour);
      p1.Set(p2);
    }
    bspyDrawLine(pos, pos + p1, colour);
  }
  // draw three circles around pos to (poorly) indicate a sphere.
  void NmRsCharacter::bspyDrawSphere(const rage::Vector3& pos, float radius, const rage::Vector3& colour, unsigned int segments /* = 8 */)
  {
    if (bspyDebugDrawIsInhibited() || segments == 0)
      return;
    bspyDrawCircle(pos, rage::Vector3(1,0,0), radius, colour, segments);
    bspyDrawCircle(pos, rage::Vector3(0,1,0), radius, colour, segments);
    bspyDrawCircle(pos, rage::Vector3(0,0,1), radius, colour, segments);
  }
#endif
}
#endif
