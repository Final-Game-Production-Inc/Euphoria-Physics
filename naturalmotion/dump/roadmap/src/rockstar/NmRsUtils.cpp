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

/**
 * -------------------------- IMPORTANT --------------------------
 * This file contains general computational utilities and body helper functions.
 * It must be compilable on SPU, so no external references outside of the RAGE
 * vector/math libraries and our own math macros.
 */

#include "NmRsCommon.h"
#include "NmRsMinimalInclude.h"
#include "NmRsUtils.h"
#if !__SPU & ART_ENABLE_BSPY
#include "NmRsSpy.h"
#include "bspy/NmRsSpyPackets.h"
#endif // ART_ENABLE_BSPY

namespace ART
{

    /**
    * quaternion -> twist/swing with twist on z axis i.e. how the drive treats twist/swing
     */
    rage::Vector3 rsQuatToRageDriveTwistSwing(const rage::Quaternion& quat)
    {
      rage::QuatV q = RCC_QUATV(quat);
      rage::Vec3V v = rage::geomVectors::QuatVToLeanAndTwist(q);

      rage::Vector3 result(v.GetZf(), v.GetXf(), v.GetYf()); // re-order to put twist first per NM spec
      return result;
    }

    /**
    * twist/swing -> quaternion with twist on z axis i.e. how the drive treats twist/swing
    */
    rage::Quaternion rsRageDriveTwistSwingToQuat(const rage::Vector3& ts)
    {
      rage::Vec3V v = RCC_VEC3V(ts);
      rage::QuatV q = rage::geomVectors::LeanAndTwistToQuatV(v.GetY(), v.GetZ(), v.GetX()); // re-order to put twist last per RAGE spec

      rage::Quaternion quat = RCC_QUATERNION(q);
      return quat;
    }

    /**
    * Converts quat to a normal lean twist drive.  Twist is *separate* here,
    * which is how (I think) the 3dof limits are implemented, but *not* how
    * the drive works.  *Do not* use this function unless you know what you're
    * doing.
    */
    rage::Vector3 rsQuatToRageLimitTwistSwing(const rage::Quaternion& quat)
    {
      rage::Vector3 result;
      float tw, s1, s2;
      // reorder - swap z/x
      rage::Quaternion rsQuat;
      rsQuat.Set(quat.z, quat.y, quat.x, quat.w);

      if ( rage::SqrtfSafe(rage::square(rsQuat.x) + rage::square(rsQuat.y) + rage::square(rsQuat.z)) > 0 )
      {
        float chs2 = rage::square(rsQuat.w) + rage::square(rsQuat.x);

        if (chs2 > (1.2e-7f)) 
        {
          float chs = rage::SqrtfSafe(chs2);
          float mul = 1.0f / (chs * (chs + 1.0f));

#ifdef NM_HAS_FSEL_INTRINSIC
          // this operates very slightly differently than the original
          // because fsel uses >= 0 and the original is >0...
          // should make no visual difference
          tw = (float)__fsel(rsQuat.w, 
            rsQuat.x / (rsQuat.w+chs), 
            rsQuat.x / (rsQuat.w-chs) );
#else
          tw = rsQuat.w > 0 ? rsQuat.x / (rsQuat.w+chs) : rsQuat.x / (rsQuat.w-chs);
#endif // NM_PLATFORM_X360

          s1 = mul * (rsQuat.x*rsQuat.y - rsQuat.w*rsQuat.z);
          s2 = mul * (rsQuat.w*rsQuat.y + rsQuat.x*rsQuat.z);
        }
        else
        {
          float rshs = 1.0f / (rage::SqrtfSafe(1.0f - chs2));
          tw = 0;
          s1 = -rshs * rsQuat.z;
          s2 = rshs * rsQuat.y;
        }

        float a = rage::SqrtfSafe(rage::square(s1) + rage::square(s2));
        float b = 4.0f * atanf(a);
        // FastAssert(a > 1e-10f); // TDL failing on occasion so changed to an if
        if (a > 1e-10f)
          result.Set(4.0f * atanf(tw), s2 * b / a, s1 * b / a);
        else
          result.Zero();
      }
      else
        result.Zero();

      return result;
    }

    /**
    * Converts a normal lean twist drive to quat.  Twist is *separate* here,
    * which is how (I think) the 3dof limits are implemented, but *not* how
    * the drive works.  *Do not* use this function unless you know what you're
    * doing.
    */
    rage::Quaternion rsRageLimitTwistSwingToQuat(const rage::Vector3& ts)
    {
      // hdd: changed this from Vector3 as we were constantly accessing
      //      the subcomponents, making storing it in a vector register 
      //      pointless and inefficient
      float ts2[3];
      rage::Quaternion q;

      if (ts.Mag() > 0.0f)
      {
        float tw = ts.x, s1 = ts.y, s2 = ts.z;
        float a = rage::SqrtfSafe( rage::square(s1) + rage::square(s2) );

        float b = rage::Tanf(a * 0.25f);

#ifdef NM_PLATFORM_X360

        float _a = 1.0f / (a + NM_RS_FLOATEPS);

        float ts2_x = rage::Tanf(tw * 0.25f);
        float ts2_y = s2 * b * _a;
        float ts2_z = s1 * b * _a;
        float ts2_0 = 0.0f;

        ts2[0] = ts2_x;
        ts2[1] = (float)__fsel(a, ts2_y, ts2_0);
        ts2[2] = (float)__fsel(a, ts2_z, ts2_0);

#else

        ts2[0] = rage::Tanf(tw * 0.25f);
        if (a > 0.0f)
        {
          ts2[1] = s2 * b / a;
          ts2[2] = s1 * b / a;
        }
        else
        {
          ts2[1] = 0;
          ts2[2] = 0;
        }

#endif

        // NMQuaternionFromTwistAndSwing
        {
          float b = 2.0f / (1.0f  + ts2[1] * ts2[1] + ts2[2] * ts2[2]);
          float c = 2.0f / (1.0f  + ts2[0] * ts2[0]);

          q.w = (b - 1.0f) * (c - 1.0f);
          q.z = ts2[0] * (b - 1.0f) * c;
          q.y = b * (c * ts2[0] * ts2[1] + (c-1) * ts2[2]);
          q.x = b * (c * ts2[0] * ts2[2] - (c-1) * ts2[1]);
        }
      }
      else
        q.Identity();

      return q;
    }

    void separateFeet(rage::Vector3& leftFoot, rage::Vector3& rightFoot, float length)
    {
      rage::Vector3 leftToRight(rightFoot - leftFoot);
      float dist = leftToRight.Mag();
      leftToRight.Normalize();

      if (dist < length)
      {
        leftFoot.AddScaled(leftToRight, (dist - length) * 0.5f);
        rightFoot.AddScaled(leftToRight, (length - dist) * 0.5f);
      }
    }

    void constrainFeet(rage::Vector3& leftFoot, rage::Vector3& rightFoot, float length, rage::Vector3& progCom)
    {
#define NM_CONSTRAIN_TO_BALANCE 1
#if NM_CONSTRAIN_TO_BALANCE
      //Move feet closer to the balance point
      rage::Vector3 leftToRight(rightFoot - leftFoot);
      float dist = leftToRight.Mag();
      leftToRight.Normalize();

      progCom.z = leftFoot.z;//bad calc for projecting pogCom+down with plane defined by foot and foot normal
      rage::Vector3 leftToCOM(progCom - leftFoot);
      float distL = leftToCOM.Mag();
      distL = rage::Max(distL, 0.002f);
      leftToCOM.Normalize();
      progCom.z = rightFoot.z;////bad calc for projecting pogCom+down with plane defined by foot and foot normal
      rage::Vector3 rightToCOM(progCom - rightFoot);
      float distR = rightToCOM.Mag();
      rightToCOM.Normalize();
      distR = rage::Max(distR, 0.002f);

      if (dist > length)
      {
        leftFoot.AddScaled(leftToCOM, (dist - length)*distL/(distL+distR));
        rightFoot.AddScaled(rightToCOM, (dist - length)*distR/(distL+distR));
      }
#else
      (void) progCom;
      //Move feet towards the feet centre
      rage::Vector3 leftToRight(rightFoot - leftFoot);
      float dist = leftToRight.Mag();
      leftToRight.Normalize();
      if (dist > length)
      {
        leftFoot.AddScaled(leftToRight, (dist - length) * 0.5f);
        rightFoot.AddScaled(leftToRight, (length - dist) * 0.5f);
      }
#endif//NM_CONSTRAIN_TO_BALANCE
    }

    // Ported from NMGeomUtils
    //----------------------------------------------------------------------------------------------------------------------
    // Be aware that if the point input is inside the ellipse then there are three extrema in the
    // cost function, and this algorithm may find the furthest point rather than the closest.  However,
    // the algorithm is robust to points that are on the ellipse boundary or just inside.
    //
    // Taking our cost function for point (x,y) and current closest point estimate (cx, cy) to be
    //   f = (x - cx)^2 + (y - cy)^2,
    // with derivatives with respect to angle theta where cx = Acos(theta) and cy = Bsin(theta) of
    //   df/dtheta   = 2((A/B)(x - cx)cy - (B/A)(y - cy)cx
    //   d2f/dtheta2 = 2((cx(B/A))^2 + (cy(A/B))^2 + (x - cx)cx + (y - cy)cy)
    // Then, in order to avoid calling any trigonometric functions in the code, we convert to the
    // parameter t where t = tan(theta/2) and
    //   dtheta/dt        = 1 + cx/A
    //   d2theta/dthetadt = -cy/B
    // making relevant substitutions and using half-angle formulae where appropriate.
    //
    // With this in mind we can do Newton iteration on the parameter t to minimise f, using as our
    // initial value the intersection with the ellipse of the line through the origin from (x,y).
    void closestPointOnEllipse(float& x, float& y, float A, float B)
    {
      Assert(A > 0);
      Assert(B > 0);

      const unsigned int maxIterations = 100;  // Should always be enough

      // By forcing the point into the x-positive half of the space we can avoid our t variable
      // having to cross a singularity.  xswitch keeps track of whether we flipped the x-axis or
      // not.
      float xswitch = 1;
      if (x < 0)
      {
        xswitch = -1;
        x = -x;
      }
      float yswitch = 1;
      if (y < 0)
      {
        yswitch = -1;
        y = -y;
      }

      // Get some values up front
      float AoverB = A/B;
      float BoverA = B/A;
      float AAoverBB = AoverB*AoverB;
      float BBoverAA = BoverA*BoverA;

      // Now clamp to independent limits, which is effectively clamping to a rectangle fitting
      // around the ellipse.  This improves our first guess and ensures convergence to the
      // minimum rather than a maximum.
      float xClamped = x;
      float yClamped = y;
      if (xClamped > A)
        xClamped = A;
      if (yClamped > B)
        yClamped = B;
      float xClampedNormMag = rage::SqrtfSafe(xClamped*xClamped/(A*A) + yClamped*yClamped/(B*B));

      // Get our initial guess for our closest point, as the intersection with the ellipse boundary
      // of a line through the origin.
      float cx = xClamped/xClampedNormMag;
      float cy = yClamped/xClampedNormMag;

      // Calculate t = tan(theta/2) using half angle formulae.
      float cosTheta = cx/A; // Always >= 0
      float t = rage::SqrtfSafe((1 - cosTheta)/(1 + cosTheta));

      // Newton iteration loop
      unsigned int iter = 0;
      float deltat;
      do
      {
        ++iter;

        // Get some more values up front
        float xsubcx = x - cx;
        float ysubcy = y - cy;
        float dthetaBydt = 1 + cx/A;
        float d2thetaBydthetadt = -cy/B;

        // Calculate cost function derivative fdash and second derivative fddash
        float fdash = 2.f*(AoverB*xsubcx*cy - BoverA*ysubcy*cx);
        float fddash = 2.f*(BBoverAA*cx*cx + AAoverBB*cy*cy + xsubcx*cx + ysubcy*cy);

        // Modify fddash to account for the parameter change theta -> t.  We would normally
        // have to modify both derivatives, but one dtheta/dt term cancels out later.
        fddash = fddash*dthetaBydt + fdash*d2thetaBydthetadt;

        // Do the update
        deltat = fdash/fddash;
        Assert(rage::FPIsFinite(deltat)); // Shouldn't get any divide-by-zero issues in theory
        t -= deltat;
        if (t > 1) t = 1;
        if (t < 0) t = 0;

        // Update our closest point
        float t2 = t*t;
        float t2plus1 = 1 + t2;
        cosTheta = (1 - t2)/t2plus1;
        float sinTheta = 2*t/t2plus1;
        cx = A*cosTheta;
        cy = B*sinTheta;
        Assert(rage::FPIsFinite(cx));
        Assert(rage::FPIsFinite(cy));
      }
      while (fabs(deltat) > 1e-7f && iter < maxIterations);

      // Copy result out
      x = cx*xswitch;
      y = cy*yswitch;
    }

    float rotateFromVel(
      float dir, 
      const rage::Matrix34& pelvisMat, 
      const rage::Vector3& vel, 
      rage::Vector3::Vector3Param comRotVel, 
      bool dualFacing, 
      bool incremental)
    {
      float twistAmount = 0.025f;

      rage::Vector3 right(pelvisMat.b), forward(pelvisMat.c), newVec;
      rage::Vector3 rot(comRotVel);
      rage::Quaternion q;

      rot *= -0.2f;

      q.FromRotation(rot);
      if (incremental)
        q.Transform(vel, newVec);
      else
        newVec = vel;

      float dotX = -right.Dot(newVec);
      float dotZ = forward.Dot(newVec);
      float angle = rage::Atan2f(-dotZ, dir * dotX);

      if (dualFacing)
      {        
#ifdef NM_HAS_FSEL_INTRINSIC
        float _gt = angle - (0.5f * PI);
        float _lt = angle + (0.5f * PI);
        angle = (float)__fsel(angle, _gt, _lt);
#else
        if (angle > 0)
          angle -= 0.5f*PI;
        else
          angle += 0.5f*PI;
#endif // NM_PLATFORM_X360
      }
      else
      {
        angle = angle - 0.5f*PI;
        if (angle < -PI)
          angle = angle + 2.0f * PI;
        if ((angle > PI * 0.8f) || (angle < PI * -0.8))
          angle = 0.0f;
      }
      if (incremental)
      {
        angle = angle * rage::Min(vel.Mag(), 1.0f);
        return angle * twistAmount;
      }
      else
        return angle;
    }

    float pointInsideSupport(const rage::Vector3 &point, const rage::Matrix34 *leftMat, const rage::Matrix34 *rightMat, float footWidth, float footLength, const rage::Vector3 &up, rage::Vector3 *nearestPoint)  
    {
      if (!leftMat && !rightMat)
      {
        if (nearestPoint)
          *nearestPoint = point; 
        return 0.f;
      }
      else if (!leftMat)
        leftMat = rightMat;
      else if (!rightMat)
        rightMat = leftMat;
      rage::Vector3 backLeft = leftMat->d; 
      backLeft.AddScaled(leftMat->a, -footWidth);
      backLeft.AddScaled(leftMat->c,  footLength);
      rage::Vector3 frontLeft = leftMat->d;
      frontLeft.AddScaled(leftMat->a, -footWidth);
      frontLeft.AddScaled(leftMat->c, -footLength);
      rage::Vector3 frontRight = rightMat->d;
      frontRight.AddScaled(rightMat->a,  footWidth);
      frontRight.AddScaled(rightMat->c, -footLength);
      rage::Vector3 backRight = rightMat->d;
      backRight.AddScaled(rightMat->a,  footWidth);
      backRight.AddScaled(rightMat->c,  footLength); 

      rage::Vector3 forwards = frontRight - frontLeft;
      rage::Vector3 right = backRight - frontRight;
      rage::Vector3 backwards = backLeft - backRight;
      rage::Vector3 left = frontLeft - backLeft;
      forwards.Cross(up); forwards.Normalize();
      right.Cross(up); right.Normalize();
      backwards.Cross(up); backwards.Normalize();
      left.Cross(up); left.Normalize();
      float insideForwards = (point - frontRight).Dot(forwards);
      float insideRight = (point - frontRight).Dot(right);
      float insideBackwards = (point - backLeft).Dot(backwards);
      float insideLeft = (point - backLeft).Dot(left);
      float insideness = rage::Min(insideForwards, rage::Min(insideRight, rage::Min(insideBackwards, insideLeft)));
      if (nearestPoint)
      {
        rage::Vector3 middle = (leftMat->d + rightMat->d)*0.5f;
        *nearestPoint = point + up*(middle-point).Dot(up);
        if (insideness < 0.f)
        {
          rage::Vector3 start, end;
          if (insideLeft < 0.f && insideLeft < insideForwards && insideLeft < insideBackwards)
            end = frontLeft, start = backLeft;
          else if (insideRight < 0.f && insideRight < insideForwards && insideRight < insideBackwards)
            end = frontRight, start = backRight;
          else if (insideForwards < 0.f)
            end = frontRight, start = frontLeft;
          else if (insideBackwards < 0.f)
            end = backRight, start = backLeft;
          rage::Vector3 dir = end-start;
          float t = (*nearestPoint - start).Dot(dir) / dir.Mag2();
          *nearestPoint = start + dir*rage::Clamp(t, 0.f, 1.f);
        }
      }
      return insideness;
    }   

    void clampTarget(
      rage::Vector3& target,
      const rage::Vector3& upVector,
      const rage::Matrix34& pelvisMat,
      float lean1Extent,
      float lean2Extent)
    {
      lean1Extent *= 2.0f * 0.75f;
      lean2Extent *= 2.0f * 1.25f;
      float legLength = 0.4f;

      rage::Vector3 centre = pelvisMat.d;
      rage::Vector3 forward = pelvisMat.c;
      rage::Vector3 right = pelvisMat.b;

      float pitch = upVector.Dot(forward);
      float roll = upVector.Dot(right);

      forward.AddScaled(forward, upVector, (-pitch));
      forward.Normalize();
      right.Cross(forward, upVector);

      // reposition ellipse to account for hip rotation affecting limits of reach
      centre.AddScaled(forward, pitch * legLength);
      centre.AddScaled(right, roll * legLength);

      // clampTargetToEllipse
      {
        right.Cross(forward, upVector);

        float vh = upVector.Dot(target);

        float dot = upVector.Dot(centre);
        centre.AddScaled(centre, upVector, (vh - dot));

        target -= centre;

        float f = target.Dot(forward);
        float r = target.Dot(right);
        float scale = rage::SqrtfSafe(((f/lean1Extent) * (f/lean1Extent)) + ((r/lean2Extent) * (r/lean2Extent)));

        scale = rage::Min(1.0f / (scale + NM_RS_FLOATEPS), 1.0f);

        target.Set(centre);
        target.AddScaled(forward, scale * f);
        target.AddScaled(right, scale * r);
      }
    }

    rage::Vector3 m_corners[32];              /* (Position) */
    int m_numCorners;
    struct GrahamSortVertex 
    {
      rage::Vector3 v;
      float        a;
    };

    // adjust the component of rVec that represents the height (defined by up)
    inline void levelVector(rage::Vector3& rVec, const rage::Vector3 &up, float height = 0.0f) /*const*/
    {
      float dot = up.Dot(rVec);
      rVec.AddScaled(rVec, up, (height - dot));
    }

    float pointInsideSupportNew(const rage::Vector3 &point, const rage::Matrix34 *leftMat, const rage::Matrix34 *rightMat, const rage::Matrix34 *leftHand, bool leftHandCollided, const rage::Matrix34 *rightHand, bool rightHandCollided, float footWidth, float footLength, const rage::Vector3 &up, rage::Vector3 *nearestPoint)  
    {
      int numPoints = 8;
      rage::Vector3 supportPoints[10];
      if (!leftMat && !rightMat)
      {
        if (nearestPoint)
          *nearestPoint = point; 
        return 0.f;
      }
      else if (!leftMat)
      {
        leftMat = rightMat;
        numPoints = 4;
      }
      else if (!rightMat)
      {
        rightMat = leftMat;
        numPoints = 4;
      }

      supportPoints[0] = leftMat->d;
      supportPoints[0].AddScaled(leftMat->a, -footWidth);
      supportPoints[0].AddScaled(leftMat->c, footLength);
      supportPoints[1] = leftMat->d;
      supportPoints[1].AddScaled(leftMat->a, -footWidth);
      supportPoints[1].AddScaled(leftMat->c, -footLength);
      supportPoints[2] = leftMat->d;
      supportPoints[2].AddScaled(leftMat->a, footWidth);
      supportPoints[2].AddScaled(leftMat->c, footLength);
      supportPoints[3] = leftMat->d;
      supportPoints[3].AddScaled(leftMat->a, footWidth);
      supportPoints[3].AddScaled(leftMat->c, -footLength);

      if (numPoints == 8)
      {
        supportPoints[4] = rightMat->d;
        supportPoints[4].AddScaled(rightMat->a, -footWidth);
        supportPoints[4].AddScaled(rightMat->c, footLength);
        supportPoints[5] = rightMat->d;
        supportPoints[5].AddScaled(rightMat->a, -footWidth);
        supportPoints[5].AddScaled(rightMat->c, -footLength);
        supportPoints[6] = rightMat->d;
        supportPoints[6].AddScaled(rightMat->a, footWidth);
        supportPoints[6].AddScaled(rightMat->c, footLength);
        supportPoints[7] = rightMat->d;
        supportPoints[7].AddScaled(rightMat->a, footWidth);
        supportPoints[7].AddScaled(rightMat->c, -footLength);
      }

      if (leftHandCollided && rightHandCollided)
      {
        numPoints += 2;
        supportPoints[numPoints-2] = leftHand->d;
        supportPoints[numPoints-1] = rightHand->d;
      }
      else
      {
        if (leftHandCollided)
        {
          numPoints += 1;
          supportPoints[numPoints-1] = leftHand->d;
        }
        else if (rightHandCollided)
        {
          numPoints += 1;
          supportPoints[numPoints-1] = rightHand->d;
        }

      }
#if !__SPU & ART_ENABLE_BSPY
      float footHeight = up.Dot(supportPoints[0]);
#if !NM_TEST_NEW_INSIDESUPPORT
      for (int i=0; i<numPoints; i++)
      {
        levelVector(supportPoints[i],up,footHeight);//Only works for 2d at moment
      }
#endif
      //Draw rectangles around the foot projections
      BSPY_DRAW_LINE(supportPoints[0],supportPoints[1],rage::Vector3(0,1,1));
      BSPY_DRAW_LINE(supportPoints[0],supportPoints[2],rage::Vector3(0,1,1));
      BSPY_DRAW_LINE(supportPoints[2],supportPoints[3],rage::Vector3(0,1,1));
      BSPY_DRAW_LINE(supportPoints[1],supportPoints[3],rage::Vector3(0,1,1));
      if (numPoints >= 8)
      {
        BSPY_DRAW_LINE(supportPoints[4],supportPoints[5],rage::Vector3(0,1,1));
        BSPY_DRAW_LINE(supportPoints[4],supportPoints[6],rage::Vector3(0,1,1));
        BSPY_DRAW_LINE(supportPoints[6],supportPoints[7],rage::Vector3(0,1,1));
        BSPY_DRAW_LINE(supportPoints[5],supportPoints[7],rage::Vector3(0,1,1));
      }
#endif


      //This is done inside buildConvexHull2D
      //for(int i=0;i<numPoints;i++)
      //{ 
      //  levelVector(supportPoints[i],up);
      //}
      buildConvexHull2D(supportPoints, numPoints, rage::Vector3(0,0,1));
      rage::Vector3 upCopy(up),pointCopy(point);
      levelVector(pointCopy,upCopy);//mmmmtodo nearestPoint may need to be calculated in 3d remember (see comment below)
      rage::Vector3 nrstPoint;
      float insideResult = getDistanceToPoint(pointCopy, upCopy/*, radius*/, &nrstPoint); //when a member function make const;
      if (nearestPoint)
        nearestPoint = &nrstPoint;
      //mmmmtodo nearestPoint returned above has not been check to see if it returns the same as below
      //when this is done we can use this routine in Gravity compensation
      //if (nearestPoint)
      //{
      //  rage::Vector3 middle = (leftMat->d + rightMat->d)*0.5f;
      //  *nearestPoint = point + up*(middle-point).Dot(up);
      //  if (insideness < 0.f)
      //  {
      //    rage::Vector3 start, end;
      //    if (insideLeft < 0.f && insideLeft < insideForwards && insideLeft < insideBackwards)
      //      end = frontLeft, start = backLeft;
      //    else if (insideRight < 0.f && insideRight < insideForwards && insideRight < insideBackwards)
      //      end = frontRight, start = backRight;
      //    else if (insideForwards < 0.f)
      //      end = frontRight, start = frontLeft;
      //    else if (insideBackwards < 0.f)
      //      end = backRight, start = backLeft;
      //    rage::Vector3 dir = end-start;
      //    float t = (*nearestPoint - start).Dot(dir) / dir.Mag2();
      //    *nearestPoint = start + dir*rage::Clamp(t, 0.f, 1.f);
      //  }
      //}
#if !__SPU & ART_ENABLE_BSPY
      rage::Vector3 col(1,0,0);
      if (insideResult < 0.f)
        col.Set(1,1,1);
      pointCopy.Set(point);
      levelVector(pointCopy,up,footHeight);
      BSPY_DRAW_POINT(pointCopy,0.3f,col);
      bspyScratchpad(bspyLastSeenAgent, "foot", insideResult);
      drawConvexHull(col);
#endif


      return insideResult;
    }  

    //----------------------------------------------------------------------------------------------------------------------
    float getDistanceToPoint(
      const rage::Vector3 &position, 
      const rage::Vector3 &normal, 
      //const float radius, 
      rage::Vector3 *closestPointInPolygon) //when a member function make const
    {
      // We assume points are in a clockwise order viewed along normal
      //rage::Vector3 centre(0,0,0);//mmmmmUNUSED centre
      //for (int i = 0; i<m_numCorners; i++)
      //  centre += m_corners[i] / (float)m_numCorners;

      *closestPointInPolygon = position; // default for if it is inside the polygon
      float maxDistance = 0.f;
      rage::Vector3 toClosest(0,0,0);
      if (m_numCorners > 1)
      {
        for (int i = 0; i<m_numCorners; i++)
        {
          int j = (i+1)%m_numCorners;
          rage::Vector3 outwards;
          outwards.Cross(m_corners[j] - m_corners[i], normal);
          outwards.Normalize();
          float distance = (position - m_corners[i]).Dot(outwards);
          if (distance > 0.f) // is potentially outside the polygon
          {
            rage::Vector3 start = m_corners[i] + normal * (position - m_corners[i]).Dot(normal);
            rage::Vector3 end = m_corners[j] + normal * (position - m_corners[j]).Dot(normal);
            rage::Vector3 toEnd = end - start;
            float t = rage::Clamp((position - start).Dot(toEnd)/(toEnd.Mag2() + 1e-10f), 0.f, 1.f);
            rage::Vector3 clampedPos; 
            clampedPos = start + toEnd*t;
            toClosest = start + toEnd*t - position;
            distance = toClosest.Mag();
            toClosest.Normalize();
#if NM_TEST_NEW_INSIDESUPPORT
            if (distance > 0 && distance > maxDistance)
            {
              start = m_corners[i];
              end = m_corners[j];
              toEnd = end - start;
              *closestPointInPolygon = clampedPos = start + toEnd*t;
            }
#endif
          }
          /*distance -= radius;*/
          if (distance > maxDistance || i==0)
          {
            maxDistance = distance;
#if !NM_TEST_NEW_INSIDESUPPORT
            if (distance > 0)
              *closestPointInPolygon = position + toClosest*distance;
#endif
          }
        }
      }
      else if (m_numCorners == 1)
      {
        toClosest = m_corners[0] + normal * (position - m_corners[0]).Dot(normal) - position;
        maxDistance = toClosest.Mag()/* - radius*/;
        toClosest.Normalize();
        if (maxDistance > 0)
          *closestPointInPolygon = position + toClosest*maxDistance;
      }
      else
      {
        maxDistance = 999999.0f;
      }
      return maxDistance;
    }
    
    //avoid stepping into car functions:
	  //----------------------------------------------------------------------------------------------------------------------
    void setCorner(int index, const rage::Vector3 &corner)
	  {
        m_corners[index] = corner;
	  }
	  void setNumOfCorners(int numOfCorners)
	  {
        m_numCorners = numOfCorners;
	  }
    //Replace progCom with a point on the car if original position would mean stepping into the car
    //Make a line passing through fromPosition and position
    //Find the 2 intersections of this line with the polygon 
    //If fromPosition outside polygon: pointOnPolygon = closest intersection or position to fromPosition.  I.e don't step through polygon even if position outside
    //if fromPosition inside polygon:
    //  pointOnPolygon = position if position outside polygon
    //  pointOnPolygon = closest intersection or position to fromPosition
	  void getIntersectionPointOnPolygon(
	    const rage::Vector3 &position, 
	    //const rage::Vector3 &up, 
	    const rage::Vector3 &fromPosition, 
	    //const float radius, 
	    rage::Vector3 *pointOnPolygon) //when a member function make const
	  {
	    // We assume points are in a clockwise order viewed along up
	    rage::Vector3 pointOnPoly = position;// default
	    //todo if is line ie m_numCorners == 2
	    if (m_numCorners > 2)
	    {
		    float ub[2];//distance from fromPosition to intersection
		    rage::Vector3 intersection[2];

		    //We are looking for 2 intersections of 
		    // the infinite line defined by fromPostion_to_position
		    // with each edge segment.
		    //Todo check:  There is a possibility that a corner (or 2) could be returned giving 4 intersections
		    int intersectionsFound = 0;
		    for (int i = 0; i<m_numCorners; i++)
		    {
		      int j = (i+1)%m_numCorners;
		      //get intersection of line fromPosition_to_position and the current edge
		      float x1 = m_corners[i].x;
		      float y1 = m_corners[i].y;
		      float x2 = m_corners[j].x;
		      float y2 = m_corners[j].y;//lineA

		      float x3 = fromPosition.x;
		      float y3 = fromPosition.y;
		      float x4 = position.x;
		      float y4 = position.y;//lineB

		      float denom = (y4-y3)*(x2-x1) - (x4-x3)*(y2-y1);
		      if (rage::Abs(denom) > NM_RS_FLOATEPS)//not parallel
		      {
			      float ua = ((x4-x3)*(y1-y3) - (y4-y3)*(x1-x3))/denom;//p1/p2 line
			      if (ua > 0.0f && ua <= 1.0f)//intersection must be on edge segment
			      {
			        //This is the intersection with the polygon
			        intersection[intersectionsFound].x = x1 + ua*(x2-x1);
			        intersection[intersectionsFound].y = y1 + ua*(y2-y1);
			        intersection[intersectionsFound].z = position.z;
			        ub[intersectionsFound] = ((x2-x1)*(y1-y3) - (y2-y1)*(x1-x3))/denom;//ub - p3/p4 line
			        intersectionsFound++;
			      }
		      }
		      if (intersectionsFound == 2)
			    break;
		    }
		    if (intersectionsFound == 2)
		    {
    #if !__SPU & ART_ENABLE_BSPY
		      rage::Vector3 col(1,0,0);
		      BSPY_DRAW_POINT(intersection[0], 0.1f, col);
		      BSPY_DRAW_POINT(intersection[1], 0.1f, col);
    #endif

		      if (ub[0]>0.0f && ub[1]>0.0f)//fromPosition outside polygon (before polygon)
		      {
			      //return the closest intersection or position from fromPosition
			      if (ub[0]<1.0f || ub[1]<1.0f)
			      {
			        if (ub[0]<=ub[1])
				      pointOnPoly = intersection[0];
			        else
				      pointOnPoly = intersection[1];
			      }
			      // else pointOnPoly = position
		      }
		      else if (!(ub[0]<0.0f && ub[1]<0.0f))//if not (fromPosition outside polygon (after polygon), pointOnPoly = position)
		      {
			      //fromPosition inside polygon
			      int i = 0;
			      int j = 1;
			      if (rage::Abs(ub[0]) > rage::Abs(ub[1]))
			      {
			        i = 1;
			        j = 0;
			      }
			      if (!(ub[i] < 1.0f && ub[i] > ub[j]))
			      {
			        pointOnPoly = intersection[i];
			      }
			      //else //position is further on from closest intersection to polygon i.e. outside it
			      //pointOnPoly = position;
		      }
		    }//if (intersectionsFound == 2)
	    }//if (m_numCorners > 2)
  #if !__SPU & ART_ENABLE_BSPY
	    rage::Vector3 col(1,1,0);
	    BSPY_DRAW_POINT(pointOnPoly, 0.9f, col);
	    col.Set(1.0f,0.0f,0.0f);
	    BSPY_DRAW_POINT(position, 0.1f, col);
  #endif

	    *pointOnPolygon = pointOnPoly;
	  }

    //----------------------------------------------------------------------------------------------------------------------
    //Points are leveled to 0 in the up direction
    void buildConvexHull2D(/*const*/ rage::Vector3 *points, int nPoints, const rage::Vector3 &up)
    {
      rage::Vector3 side(VEC3V_TO_VECTOR3(nmGetOrthogonal(VECTOR3_TO_VEC3V(up))));
      Assert(nPoints > 2);
      //if (nPoints == 0)
      //{
      //  m_numCorners = 0;
      //  return;
      //}

      //if (nPoints <= 2)
      //{
      //  m_numCorners = nPoints;
      //  for (int i = 0 ; i < nPoints ; ++i)
      //  {
      //    m_corners[i] = points[i];
      //  }
      //  return;
      //}

      const int maxNumCorners = sizeof(m_corners) / sizeof(m_corners[0]);
      Assert(nPoints <= maxNumCorners);
      GrahamSortVertex vSort[maxNumCorners];

#if !__SPU & ART_ENABLE_BSPY
      float footHeight = up.Dot(points[0]);
#endif
      // go find the lowest point in the set
      int lowIndex = 0, i, j;
#if !NM_TEST_NEW_INSIDESUPPORT
      levelVector(points[0],up);//Only works for 2d at moment
#endif
      for (i=1; i<nPoints; i++)
      {
#if !NM_TEST_NEW_INSIDESUPPORT
        levelVector(points[i],up);//Only works for 2d at moment
#endif
        //float height = points[i].Dot(up);
        //float lowHeight = points[lowIndex].Dot(up);
        //if (height < lowHeight || 
        //  (height == lowHeight && points[i].Dot(side) < points[lowIndex].Dot(side)))
        //{
        //  lowIndex = i;
        //}
        //find the point with the least x component
        if (points[i].Dot(side) < points[lowIndex].Dot(side))
        {
          lowIndex = i;
        }
      }
#if !__SPU & ART_ENABLE_BSPY
#if !NM_TEST_NEW_INSIDESUPPORT
      for (i=0; i<nPoints; i++)
      {
        levelVector(points[i],up,footHeight);//Only works for 2d at moment
      }
#endif
#endif

      // create an array to sort; copy across the input vertices, calculate
      // their polar angles against the lowest point while we do so
      vSort[0].v = points[lowIndex];
      for (i=0, j=1; i<nPoints; i++)
      {
        if (lowIndex != i)
        {
          vSort[j].v = points[i] - vSort[0].v;
          vSort[j].a = polarAngle(vSort[j].v, up, side);
          ++j;
        }
      }

      // sort the vertices based on the angles; we then have an ordered fan of vertices, the base
      // of the fan being the lowest point found
      qsort(&vSort[1], nPoints - 1, sizeof(GrahamSortVertex), zoGrahamSortAnglesFn);

      for (i = 1; i < nPoints; i++)
      {
        vSort[i].v += vSort[0].v;
      }

      m_corners[0] = vSort[0].v;
      m_corners[1] = vSort[1].v;

      // iterate around the fan of vertices, only accepting the next iteration if it is a "left turn" from
      // one vertex to another.
      int vOutIdx = 2;
      i = 2;
      while (i < nPoints)
      {
        if (vOutIdx > 1)
        {
          if (isPointLeftOfLine(m_corners[vOutIdx - 2], m_corners[vOutIdx - 1], vSort[i].v, up) > 0.0f)
          {
            m_corners[vOutIdx] = vSort[i++].v;
            ++ vOutIdx;
          }
          else
          {
            -- vOutIdx;
          }
        }
        else
        {
          m_corners[vOutIdx].Set(vSort[i++].v);
          vOutIdx ++;
        }
      }

      m_numCorners = vOutIdx;
    }

#if !__SPU & ART_ENABLE_BSPY
    //--------------------------------------------------------------------------------------------------
    // Only call this after buildConvexHull2D or after setting corners manually into a convexHull
    void drawConvexHull(const rage::Vector3 &col)
    {
      for(int i=1;i<m_numCorners;i++)
      { 
        BSPY_DRAW_LINE(m_corners[i-1],m_corners[i],col);
      }
      BSPY_DRAW_LINE(m_corners[0],m_corners[m_numCorners-1],col);
      }
#endif

    //--------------------------------------------------------------------------------------------------
    float polarAngle(const rage::Vector3 &v, const rage::Vector3 &up, const rage::Vector3 &side)
    {
      if (v.Mag2() == 0.0f)
      {
        return 0.0f;
      }

      rage::Vector3 fwd;
      fwd.Cross(side, up); // assume they are orthonormal
      float res = atan2f(v.Dot(side), v.Dot(fwd));
      return res;
    }

    //----------------------------------------------------------------------------------------------------------------------
    int zoGrahamSortAnglesFn(const void *a, const void *b)
    {
      const GrahamSortVertex *aVert = (GrahamSortVertex*)a;
      const GrahamSortVertex *bVert = (GrahamSortVertex*)b;

      // first criteria is sorting by polar angle
      if (aVert->a < bVert->a) 
      {
        return -1;
      }
      if (aVert->a > bVert->a) 
      {
        return 1;
      }

      // second is distance from the lowest point (shifted to 0)
      // only happens when points have equal angles, possible when
      // building with grid-snapping
      float distSqA = aVert->v.Mag2();
      float distSqB = bVert->v.Mag2();

      if (distSqA < distSqB) 
      {
        return -1;
      }
      if (distSqA > distSqB) 
      {
        return 1;
      }
      return 0;
    }

    //----------------------------------------------------------------------------------------------------------------------
    // returns:
    // >0 if p2 is left of the line through p0 and p1 (ignoring y)
    //  0 if p2 is on the line
    // >0 if p2 is to the right
    float isPointLeftOfLine(const rage::Vector3 &p0, const rage::Vector3 &p1, const rage::Vector3 &p2, const rage::Vector3 &normal)
    {
      rage::Vector3 cross;
      cross.Cross(normal, p2-p1);
      float res = cross.Dot(p0-p1);

      return res;
    }

} // ART

namespace rage
{
  Vec3V_Out nmGetOrthogonal (Vec3V_In u)
  {
    // ORIGINAL CODE:
    //if ( u.x > 0.5f || u.x < -0.5f || u.y > 0.5f || u.y < -0.5f )
    //{
    // v.Set ( u.y, -u.x, 0.0f );
    //}
    //else
    //{
    // v.Set ( 0.0f, u.z, -u.y);
    //}
    //v.Normalize();
    Vec::Vector_4V v_u = u.GetIntrin128();
    Vec::Vector_4V v_neg_u = Vec::V4Negate( v_u );
    // We want to compare (a > b) with:
    // ---------------------------
    // a = | u.x | -0.5 | u.y | -0.5 |
    // ---------------------------
    // with:
    // ---------------------------
    // b = | 0.5 | u.x | 0.5 | u.y |
    // ---------------------------
    Vec::Vector_4V v_zero = Vec::V4VConstant(V_ZERO);
    Vec::Vector_4V v_half = Vec::V4VConstant(V_HALF);
    Vec::Vector_4V v_negHalf = Vec::V4VConstant(V_NEGHALF);
    Vec::Vector_4V a = Vec::V4MergeXY( v_u, v_negHalf );
    Vec::Vector_4V b = Vec::V4MergeXY( v_half, v_u );
    Vec::Vector_4V isGt = Vec::V4IsGreaterThanV( a, b );
    // Combine the results.
    Vec::Vector_4V resX = Vec::V4SplatX( isGt );
    Vec::Vector_4V resY = Vec::V4SplatY( isGt );
    Vec::Vector_4V resZ = Vec::V4SplatZ( isGt );
    Vec::Vector_4V resW = Vec::V4SplatW( isGt );
    Vec::Vector_4V res = Vec::V4Or( Vec::V4Or( resX, resY ), Vec::V4Or( resZ, resW ) );
    // Generate result A to select from.
    Vec::Vector_4V v_uy = Vec::V4SplatY( v_u );
    Vec::Vector_4V v_uy_0_uy_0 = Vec::V4MergeXY( v_uy, v_zero );
    Vec::Vector_4V resultA = Vec::V4MergeXY( v_uy_0_uy_0, v_neg_u );
    // Generate result B to select from.
    Vec::Vector_4V v_uz = Vec::V4SplatZ( v_u );
    Vec::Vector_4V v_0_nux_0_nuy = Vec::V4MergeXY( v_zero, v_neg_u );
    Vec::Vector_4V resultB = Vec::V4MergeZW( v_0_nux_0_nuy, v_uz );
    // Select the result.
    Vec::Vector_4V result = Vec::V4SelectFT( res, resultB, resultA );
    // Normalize and return.
    Vec::Vector_4V normalizedResult = Vec::V3Normalize( result );
    return Vec3V( normalizedResult );
  }
}

