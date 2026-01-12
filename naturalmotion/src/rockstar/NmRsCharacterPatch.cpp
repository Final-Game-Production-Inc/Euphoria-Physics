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
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsCBU_TaskManager.h"
#include "NmRsBodyLayout.h"

namespace ART
{
#if NM_EA
  //0 EO_UseGameGeometry,  (i.e. use the actual game intance/bounds geometry type)
  //	1 EO_Point,
  //	2 EO_Line,
  //	3	EO_Corner,
  //	4	EO_Edge,
  //	5	EO_Plane, e.g. Walls/Table tops
  //	6	EO_Disc, e.g. Table tops
  //	7	EO_Capsule,
  //	8	EO_Sphere,
  //	9	EO_Box. E.g for railings/blocks (closed edge)

  //----------------------------------------------------------------------------------------------------------------------
  void NmRsCharacter::Patch::Init()
  {
    corner.Zero();

    for (size_t i = 0 ; i < sizeof(faceNormals)/sizeof(faceNormals[0]) ; i++)
    {
      faceNormals[i].Zero();
    }

    knownContactPoint.Zero();

    for (size_t i = 0 ; i < sizeof(edgeLengths)/sizeof(edgeLengths[0]) ; ++i)
    {
      edgeLengths[i] = 0.0f;
    }
    numKnownEdgeLengths = 0;

    for (size_t i = 0 ; i < sizeof(knownContactPointFromCollision)/sizeof(knownContactPointFromCollision[0]) ; ++i)
    {
      knownContactPointFromCollision[i].local.Zero();
    }

    for (size_t i = 0 ; i < sizeof(knownContactNormalFromCollision)/sizeof(knownContactNormalFromCollision[0]) ; ++i)
    {
      knownContactNormalFromCollision[i].local.Zero();
    }

    radius = 0.f;
    type = EO_None;
    inputIsLocal = false;
  }  

  void NmRsCharacter::Patch::nearestPoint_Disc(const rage::Vector3 &pointLocal, rage::Vector3 &nearestPoint) const
  {
    ////convert pointWorld to local
    //rage::Vector3 point = pointWorld;
    //if (instLevelIndex > -1)
    //	boundToLocalSpace(false, &point, pointWorld, instLevelIndex, boundIndex);

    rage::Vector3 point = pointLocal;
    rage::Vector3 centre = corner.local;
    // find nearest point on this shape
    rage::Vector3 centre2Point = point - centre;
    float dot= 0;

    // 1. check if a face has the closest point
    {
      //rage::Vector3 cross;
      //cross.Cross(centre2Point, faceNormals[0].local);
      dot = centre2Point.Dot(faceNormals[0].local);
      if (dot >= 0.f)
      {
        nearestPoint = point - faceNormals[0].local*dot; // we'd like this first early-out to be the most common case
      }

      rage::Vector3 centre2NearestPoint = nearestPoint - centre;
      //if underside of disc or outside disc return an edge - mmmmmreturn egde aswell anyway?
      if (dot < 0.f || centre2NearestPoint.Mag() > edgeLengths[0])
      {
        centre2NearestPoint.Normalize();
        centre2NearestPoint *= edgeLengths[0];
        nearestPoint = centre + centre2NearestPoint;
      }
    }
  }

  void NmRsCharacter::Patch::getNearestPointOnLineSegment(const rage::Vector3 &point, rage::Vector3 &p0, rage::Vector3 &p1, float* mag2, rage::Vector3 *nearestPoint) const
  {
    rage::Vector3 p0_2_p1 = p1 - p0;
    p0_2_p1.Normalize();
    rage::Vector3 p0_2_point = point - p0;
    rage::Vector3 p1_2_point = point - p1;
    rage::Vector3 closestPoint;
    float dot0 = p0_2_p1.Dot(p0_2_point);
    float dot1 = p0_2_p1.Dot(p1_2_point);
    if (dot0 > 0.f && dot1 < 0.f)//on line
      closestPoint = p0 + dot0*p0_2_p1;
    else if (dot0 > 0.f && dot1 > 0.f)
      closestPoint = p1;
    else if (dot0 < 0.f && dot1 < 0.f)
      closestPoint = p0;
    //else if (dot0 < 0.f && dot1 > 0.f)//can't happen?
    float newMag2 = (point-closestPoint).Mag2();
    if ((point-closestPoint).Mag2() < *mag2)
    {
      *nearestPoint = closestPoint;
      *mag2 = newMag2;
    }
  }


  void NmRsCharacter::Patch::nearestPoint_Plane(const rage::Vector3 &pointLocal, rage::Vector3 &nearestPoint) const
  {
    // find nearest point on this shape
    float mag2 = 10000.f;
    //find closest points on the lines and choose them if closer or if dot < 0.f i.e. below plane
    rage::Vector3 v0 = corner.local;
    rage::Vector3 v1 = v0 + faceNormals[1].local*edgeLengths[0];
    rage::Vector3 v2 = v0 + faceNormals[2].local*edgeLengths[1];
    rage::Vector3 v3 = v1 + faceNormals[2].local*edgeLengths[1];
    getNearestPointOnLineSegment(pointLocal, v0, v1, &mag2, &nearestPoint);
    getNearestPointOnLineSegment(pointLocal, v0, v2, &mag2, &nearestPoint);
    getNearestPointOnLineSegment(pointLocal, v1, v3, &mag2, &nearestPoint);
    getNearestPointOnLineSegment(pointLocal, v2, v3, &mag2, &nearestPoint);
    // 1. check if a face has the closest point
    rage::Vector3 corner2Point = pointLocal - corner.local;
    float dot = corner2Point.Dot(faceNormals[0].local);
    if (dot >= 0.f)
    {
      rage::Vector3 closestPoint = pointLocal - faceNormals[0].local*dot; // point on plane
      rage::Vector3 point2NearestPoint = closestPoint - pointLocal;
      rage::Vector3 v0_2_closestPoint = closestPoint - v0;
      if (v0_2_closestPoint.Dot(faceNormals[1].local) > 0.f 
        && v0_2_closestPoint.Dot(faceNormals[2].local) > 0.f)
      {
        float newMag2 = point2NearestPoint.Mag2();
        if (newMag2 < mag2)
        {
          nearestPoint = closestPoint;
        }
      }
    }
  }

  void NmRsCharacter::Patch::nearestPoint_Line(const rage::Vector3 &pointLocal, rage::Vector3 &nearestPoint) const
  {
    float mag2 = FLT_MAX;
    rage::Vector3 startPoint = corner.local;
    rage::Vector3 endPoint =	endPoint = corner.local + edgeLengths[0]*faceNormals[2].local;
    getNearestPointOnLineSegment(pointLocal, startPoint, endPoint, &mag2, &nearestPoint);
  }

  void NmRsCharacter::Patch::nearestPoint(const rage::Vector3 &pointLocal, rage::Vector3 &nearestPoint) const
  {
    switch (type)
    {
    case NmRsCharacter::Patch::EO_Plane:
      {
        nearestPoint_Plane(pointLocal, nearestPoint);
      }
      break;
    case NmRsCharacter::Patch::EO_Disc:
      {
        nearestPoint_Disc(pointLocal, nearestPoint);
      }
      break;
    case NmRsCharacter::Patch::EO_Point:
      {
        nearestPoint = corner.local;
      }
      break;
    case NmRsCharacter::Patch::EO_Line:
      {
        nearestPoint_Line(pointLocal, nearestPoint);
      }
      break;
    }
  }


#if NM_EA_TEST_FROM_IMPACTS
  //Add from a contact
  void NmRsCharacter::Patch_Add(rage::phInst* inst, int boundIndex, bool localInput, const rage::Vector3 &pos, const rage::Vector3 &normal)
  {
    static bool allow = false;
    if (!allow)
      return;
    if (inst == getFirstInstance())
      return;
    rage::phBound* bound = inst->GetArchetype()->GetBound();
    rage::phBound::BoundType boundType = (rage::phBound::BoundType)bound->GetType();
    int instLevelIndex = inst->GetLevelIndex();
    if (boundType == rage::phBound::COMPOSITE)
    {
      rage::phBoundComposite* composite = static_cast<rage::phBoundComposite*>(bound);
      int boundCount = composite->GetNumBounds();
      if (boundIndex < composite->GetNumBounds())
      {
        rage::phBound* subBound = composite->GetBound(boundIndex);
        if (subBound)
        {
          bound = subBound;
        }
      }
    }
    boundType = (rage::phBound::BoundType)bound->GetType();
    if (boundType == rage::phBound::CAPSULE)
    {
      //mmmmtodo add early out
      bool newPatch = true;
      for (int i = 0; i<NUM_OF_PATCHES; i++)
      {
        if (m_patches[i].instLevelIndex == instLevelIndex 
          && m_patches[i].boundIndex == boundIndex)
        {
          newPatch = false;
          //add some new contact data
        }
      }

      if (newPatch)
      {
        m_currentPatch++;
        if (m_currentPatch >= NUM_OF_PATCHES)
          m_currentPatch = 0;
        m_patches[m_currentPatch].Init();
        m_patches[m_currentPatch].instLevelIndex = instLevelIndex;
        m_patches[m_currentPatch].boundIndex = boundIndex;
        rage::Vector3 posLocal = pos;
        rage::Vector3 normalLocal = normal;
        if (!localInput && instLevelIndex > -1)
        {
          //convert the input to local co-ordinates
          boundToLocalSpace(false, &posLocal, pos, instLevelIndex, boundIndex);
          boundToLocalSpace(true, &normalLocal, normal, instLevelIndex, boundIndex);
        }
        m_patches[m_currentPatch].knownContactPointFromCollision[0].local.Set(posLocal);
        m_patches[m_currentPatch].knownContactNormalFromCollision[0].local.Set(-normalLocal);
        m_patches[m_currentPatch].type = Patch::EO_Capsule;
      }
    }
  }
#endif //#if NM_EA_TEST_FROM_IMPACTS

  //Add from a message
  void NmRsCharacter::Patch_Add(int geomType, 
    unsigned int /*action*/, 
    int instanceIndex, 
    int boundIndex, 
    const rage::Vector3 &corner, 
    const rage::Vector3 &faceNormal0,
    const rage::Vector3 &faceNormal1,
    const rage::Vector3 &faceNormal2,
    const rage::Vector3 &edgeLengths,
    float /*edgeRadius*/,
    bool localVectors)
  {
    //Don't allow the character to reference itself?
    //if (instanceIndex == getFirstInstance()->GetLevelIndex())
    //	return;
    //check inst exists
    int patchType = Patch::EO_None;
    if (instanceIndex > -1)
    {
      rage::phInst* inst = getEngine()->getLevel()->GetInstance(instanceIndex);
      if (!inst)
        return;

      //check bound exists
      if (boundIndex > 0)
      {
        rage::phBound* bound = inst->GetArchetype()->GetBound();
        rage::phBound::BoundType boundType = (rage::phBound::BoundType)bound->GetType();
        if (boundType == rage::phBound::COMPOSITE)
        {
          rage::phBoundComposite* composite = static_cast<rage::phBoundComposite*>(bound);
          if (boundIndex < composite->GetNumBounds())
          {
            rage::phBound* subBound = composite->GetBound(boundIndex);
            if (!subBound)
              return;//bound doesn't exist

#if NM_EA_DEVEL
            rage::phBound::BoundType boundType = (rage::phBound::BoundType)subBound->GetType();
            if (geomType == Patch::EO_UseGameGeometry)
            {
              if (boundType == rage::phBound::CAPSULE)
              {
                patchType = Patch::EO_Capsule;
              }
              else
                return;//Instance is unknown geometry type
            }
#endif//#if NM_EA_DEVEL
          }
          else
            return;//boundIndex > num of bounds
        }
        else
          return;//bound doesn't exist
      }
#if NM_EA_DEVEL
      else if (geomType == Patch::EO_UseGameGeometry)
      {
        rage::phBound* bound = inst->GetArchetype()->GetBound();
        rage::phBound::BoundType boundType = (rage::phBound::BoundType)bound->GetType();
        if (boundType == rage::phBound::CAPSULE)
        {
          patchType = Patch::EO_Capsule;
        }
        else
          return;//Instance is unknown geometry type
      }
#endif //#if NM_EA_DEVEL
    }

    m_currentPatch++;
    if (m_currentPatch >= NUM_OF_PATCHES)
      m_currentPatch = 0;
    m_patches[m_currentPatch].Init();
    m_patches[m_currentPatch].instLevelIndex = instanceIndex;
    m_patches[m_currentPatch].boundIndex = boundIndex;
    if (patchType != Patch::EO_None)
    {//Convert GameGeometry to patch
      //corner = ;
      //faceNormal0 = ;
      //faceNormal1 = ;
      //faceNormal2 = ;
      //edgeLengths = ;
      //edgeRadius = ;
      m_patches[m_currentPatch].UseGameGeometry = true;
      geomType = patchType;
      localVectors = true;
    }
    else
      m_patches[m_currentPatch].UseGameGeometry = false;

    m_patches[m_currentPatch].type = geomType;

    //conversion to global here as don't know when message comes in
    if (!localVectors && instanceIndex > -1)
    {
      m_patches[m_currentPatch].corner.global = corner;
      m_patches[m_currentPatch].faceNormals[0].global = faceNormal0;
      m_patches[m_currentPatch].faceNormals[1].global = faceNormal1;
      if (m_patches[m_currentPatch].type == Patch::EO_Plane)
        m_patches[m_currentPatch].faceNormals[2].global.Cross(faceNormal0,faceNormal1);
      else
        m_patches[m_currentPatch].faceNormals[2].global = faceNormal2;
      //Convert to local vectors
      boundToLocalSpace(false, &m_patches[m_currentPatch].corner.local, corner, instanceIndex, boundIndex);
      boundToLocalSpace(true, &m_patches[m_currentPatch].faceNormals[0].local, faceNormal0, instanceIndex, boundIndex);
      boundToLocalSpace(true, &m_patches[m_currentPatch].faceNormals[1].local, faceNormal1, instanceIndex, boundIndex);
      boundToLocalSpace(true, &m_patches[m_currentPatch].faceNormals[2].local, faceNormal2, instanceIndex, boundIndex);
    }
    else if (instanceIndex > -1)//&& localVector
    {
      m_patches[m_currentPatch].corner.local = corner;
      m_patches[m_currentPatch].faceNormals[0].local = faceNormal0;
      m_patches[m_currentPatch].faceNormals[1].local = faceNormal1;
      if (m_patches[m_currentPatch].type == Patch::EO_Plane)
        m_patches[m_currentPatch].faceNormals[2].local.Cross(faceNormal0,faceNormal1);
      else
        m_patches[m_currentPatch].faceNormals[2].local = faceNormal2;
      //Convert to global vectors
      boundToWorldSpace(&m_patches[m_currentPatch].corner.global, corner, instanceIndex, boundIndex);
      rotateBoundToWorldSpace(&m_patches[m_currentPatch].faceNormals[0].global, faceNormal0, instanceIndex, boundIndex);
      rotateBoundToWorldSpace(&m_patches[m_currentPatch].faceNormals[1].global, faceNormal1, instanceIndex, boundIndex);
      rotateBoundToWorldSpace(&m_patches[m_currentPatch].faceNormals[2].global, faceNormal2, instanceIndex, boundIndex);

    }
    else if (instanceIndex == -1) 
    {
      m_patches[m_currentPatch].corner.global = corner;
      m_patches[m_currentPatch].faceNormals[0].global = faceNormal0;
      m_patches[m_currentPatch].faceNormals[1].global = faceNormal1;
      if (m_patches[m_currentPatch].type == Patch::EO_Plane)
        m_patches[m_currentPatch].faceNormals[2].global.Cross(faceNormal0,faceNormal1);
      else
        m_patches[m_currentPatch].faceNormals[2].global = faceNormal2;
      m_patches[m_currentPatch].corner.local = corner;
      m_patches[m_currentPatch].faceNormals[0].local = faceNormal0;
      m_patches[m_currentPatch].faceNormals[1].local = faceNormal1;
      m_patches[m_currentPatch].faceNormals[2].local = m_patches[m_currentPatch].faceNormals[2].global;
    }
    //todo edgeRadius
    m_patches[m_currentPatch].edgeLengths[0] = edgeLengths.x;
    m_patches[m_currentPatch].edgeLengths[1] = edgeLengths.y;
    m_patches[m_currentPatch].edgeLengths[2] = edgeLengths.z;

    //Just for quick geom test
    m_patches[m_currentPatch].knownContactPointFromCollision[0].local.Set(m_patches[m_currentPatch].corner.local);
    m_patches[m_currentPatch].knownContactNormalFromCollision[0].local.Set(m_patches[m_currentPatch].faceNormals[0].local);
    //mmmremove conv for test
    m_patches[m_currentPatch].knownContactPointFromCollision[0].global.Set(m_patches[m_currentPatch].corner.global);
    m_patches[m_currentPatch].knownContactNormalFromCollision[0].global.Set(m_patches[m_currentPatch].faceNormals[0].global);
  }

  //----------------------------------------------------------------------------------------------------------------------
  // delete the patch if the inst or boundIndex has disappeared
  void NmRsCharacter::Patch_Cull(int i)
  {
    if (m_patches[i].instLevelIndex == -1)
      return;

    if (m_patches[i].type != Patch::EO_None && (!IsInstValid_NoGenIDCheck(m_patches[i].instLevelIndex)))
    {
        m_patches[i].type = Patch::EO_None;
    }
          //mmmmtodo check to see if bound still exists
    //else if !bound m_patches[i].type = Patch::EO_None;
  }

#if NM_EA_DEVEL
  /*extern*/ int cNumberOfFacesOrCapsuleEdges[NmRsCharacter::Patch::EO_Max];
  int cNumberOfFaces[NmRsCharacter::Patch::EO_Max] =  {-1, -1, -1, 3, 2, 1, 1, 0, 0, 5};
  int cNumberOfEdges [NmRsCharacter::Patch::EO_Max] = {-1,  0,  1, 3, 1, 0, 1, 0, 0, 8};

  //----------------------------------------------------------------------------------------------------------------------
  // returns the nearest point in the aabb represented by just the vector extents
  inline bool nearestPointOnBox(rage::Vector3 &vecLocal, const rage::Vector3 &extents)
  {
    rage::Vector3 vec;
    vec.x = (vecLocal.x > extents.x) ? extents.x : ( (vecLocal.x < -extents.x) ? -extents.x : vecLocal.x );
    vec.y = (vecLocal.y > extents.y) ? extents.y : ( (vecLocal.y < -extents.y) ? -extents.y : vecLocal.y );
    vec.z = (vecLocal.z > extents.z) ? extents.z : ( (vecLocal.z < -extents.z) ? -extents.z : vecLocal.z );

    bool outside = (vecLocal.x != vec.x) || (vecLocal.y != vec.y) || (vecLocal.z != vec.z);
    vecLocal = vec;
    return outside;
  }

  //----------------------------------------------------------------------------------------------------------------------
  void NmRsCharacter::Patch::getEdgeTangents(rage::Vector3 *edgeTangents) const
  {
    for (int i = 0; i<3; i++) // TDL edge tangents need to be initialised to zero
    {
      edgeTangents[i].Zero();
    }

    if (type == EO_Capsule)
    {
      // for compactness we store the capsule 'to end' vector in the faceNormal for this special case
      edgeTangents[0] = faceNormals[0].local;
    }
    else
    {
      // create the edge tangents from the face normals
      int numEdges = cNumberOfEdges[type];
      int j = cNumberOfFaces[type]-1;

      for (int i = 0; i<numEdges; j=i, i++)
        edgeTangents[i].Cross(faceNormals[i].local, faceNormals[j].local);
    }
  }

  ////----------------------------------------------------------------------------------------------------------------------
  //// returns 0 if already inside the shape, -1 if clamps to box, 1 if usual
  //// if getSurfacePoint is true it returns the point on the surface, otherwise the nearest point inside the surface
  //int32_t NmRsCharacter::Patch::nearestPoint(const rage::Vector3 &pointWorld, rage::Vector3 &nearestPoint, bool getSurfacePoint) const
  //{
  //	// first get the point on the patch geometry
  //	rage::Vector3 edgeTangents[3];
  //	getEdgeTangents(edgeTangents);
  //	bool outsidePatch = nearestPointInternal(pointWorld, nearestPoint, edgeTangents, getSurfacePoint);
  //	nearestPoint -= state.boxCentre;

  //	// now check whether it is outside the box, simultaneously get the nearest point on the box
  //	bool outsideBox = nearestPointOnBox(nearestPoint, state.extents);
  //	nearestPoint += state.boxCentre;
  //	float dist = 1e10f;

  //	// now inflate this point if the geometry has a radius (e.g. a capsule)
  //	if (radius)
  //	{
  //		rage::Vector3 toPoint = pointWorld - nearestPoint;
  //		//dist = toPoint.normaliseGetLength();
  //		dist = toPoint.Mag();
  //		toPoint.Normalize();
  //		nearestPoint += toPoint*radius; // shift by radius
  //	}
  //	if ((!outsidePatch && !outsideBox) || dist < radius)
  //	{
  //		if (!getSurfacePoint)
  //			nearestPoint = pointWorld;
  //		return 0;
  //	}
  //	return outsideBox ? -1 : 1;
  //}

  //----------------------------------------------------------------------------------------------------------------------
  // returns false if already inside the shape
  // sets nearestPoint to either the nearest point in the shape volume or the nearest point on the surface depending on the getSurfacePoint bool
  bool NmRsCharacter::Patch::nearestPointInternal(const rage::Vector3 &point, rage::Vector3 &nearestPoint, rage::Vector3 *edgeTangents, bool getSurfacePoint) const
  {
    // find nearest point on this shape made of several faces
    rage::Vector3 pos = point - corner.local; // do everything local to the shape
    float dots[3] = {0,0,0};//mmmmmtodo 5 zeros for box

    // 1. check if a face has the closest point
    int numFaces = cNumberOfFaces[type];
    int largestI = 0;
    for (int i = 0; i<numFaces; i++) 
    {
      rage::Vector3 cross;
      cross.Cross(pos, faceNormals[i].local);
      dots[i] = pos.Dot(faceNormals[i].local);
      if (dots[i] >= 0.f && cross.Dot(edgeTangents[i]) >= 0.f && cross.Dot(edgeTangents[(i+1)%numFaces]) <= 0.f)
      {
        nearestPoint = point - faceNormals[i].local*dots[i]; // we'd like this first early-out to be the most common case
        return true;
      }
      if (dots[i] > dots[largestI])
        largestI = i;
    }
    // and return false if inside these planes
    if (numFaces && dots[largestI]<0.f)
    {
      nearestPoint = point;
      if (getSurfacePoint)
        nearestPoint -= faceNormals[largestI].local*dots[largestI]; 
      return false; // we're inside the shape
    }

    // 2. check if an edge has the closest point
    float biggestLength = 0.f;
    int biggestI = 0;
    int numEdges = cNumberOfEdges[type];
    for (int i = 0; i<numEdges; i++)
    {
      float length = pos.Dot(edgeTangents[i]);
      if (length > biggestLength)
      {
        biggestLength = length;
        biggestI = i;
      }
    }
    float edgeSqr = edgeTangents[biggestI].Mag2();
    if (type == EO_Capsule && biggestLength > edgeSqr) // this line clamps capsules to the far end length, relies on edgeTangents[0] being vector to far side
      biggestLength = edgeSqr;
    nearestPoint = corner.local + biggestLength * (edgeTangents[biggestI]/(edgeSqr + 1e-10f)); // note, the corner case happens naturally if no edge has a t>0

    return true;
  }

  //----------------------------------------------------------------------------------------------------------------------
  // this updates the position, velocity and orientation of the patch based on its known angular vel and acceleration
  void NmRsCharacter::Patch::update(float timeStep)
  {
    knownContactPoint.local -= state.position;
    corner.local -= state.position;
    state.velocity += state.acceleration * timeStep;
    state.position += state.velocity * timeStep;
    state.boxCentre += state.velocity * timeStep; // we don't know what will happen to the bounding box so we have to assume it'll move with the position

    // now update from the angular velocity
    //rage::Quaternion rotation(state.angularVelocity * timeStep, false);
    rage::Quaternion rotation;
    rotation.FromRotation(state.angularVelocity * timeStep);
    rotation.Normalize();
    rage::Matrix34 rot;
    rot.FromQuaternion(rotation);

    //knownContactPoint.Dot3x3(rot);
    knownContactPoint.local.Dot3x3(rot);
    corner.local.Dot3x3(rot);
    knownContactPoint.local += state.position;
    corner.local += state.position;
    for (int i = 0; i<cNumberOfFacesOrCapsuleEdges[type]; i++)
      faceNormals[i].local.Dot3x3(rot);
  }

#endif//#if NM_EA_DEVEL

#if ART_ENABLE_BSPY
  void NmRsCharacter::PatchSendUpdate(int i, NmRsSpy& /*spy*/)
  {
    Patch_Cull(i);
    if (getBSpyID() == INVALID_AGENTID)
      return;
    rage::Vector3 posGlob;
    rage::Vector3 normalGlob;

    switch (m_patches[i].type)
    {
#if NM_EA_TEST_FROM_IMPACTS
    case Patch::EO_Capsule:
      {
        rage::phInst* const pInst = getEngine()->getLevel()->GetInstance(m_patches[i].instLevelIndex);

        if (pInst)
        {
          rage::Matrix34 instTm = RCC_MATRIX34(pInst->GetMatrix());
          rage::Matrix34 partWorld;
          rage::phBound* pBound = pInst->GetArchetype() ? pInst->GetArchetype()->GetBound() : NULL;//RageMP3
          if (pBound)
          {
            rage::phBound::BoundType boundType = (rage::phBound::BoundType)pBound->GetType();
            if (boundType == rage::phBound::COMPOSITE)
            {
              rage::phBoundComposite* const pBoundComposite = static_cast<rage::phBoundComposite*>(pBound);
              int dum = pBoundComposite->GetNumBounds();
              dum = dum;
              if (m_patches[i].boundIndex < pBoundComposite->GetNumBounds()) 
              {
                pBound = pBoundComposite->GetBound(m_patches[i].boundIndex);
                if (pBound)
                {
                  const rage::Matrix34 &subTm = RCC_MATRIX34(pBoundComposite->GetCurrentMatrix(m_patches[i].boundIndex));
                  partWorld.Dot(subTm, instTm);
                }
              }
              else
                pBound = NULL;
            }//composite
            else
              partWorld = instTm;

            if (pBound)
            {
              rage::Matrix34 bTm;
              partWorld.Transform(VEC3V_TO_VECTOR3(pBound->GetCentroidOffset()), bTm.d);
              bTm.Set3x3(partWorld);

              DynamicCollisionShapePacket dcs((bs_uint32)m_patches[i].instLevelIndex, (bs_uint32)atDataHash((const char*)&pBound, sizeof(pBound)), (bs_int16) getBSpyID(), (bs_int32)pInst->GetClassType());

              if (PHLEVEL->IsFixed(m_patches[i].instLevelIndex))
                dcs.m_flags |= DynamicCollisionShapePacket::bSpyDO_Fixed;
              if (PHLEVEL->IsInactive(m_patches[i].instLevelIndex))
                dcs.m_flags |= DynamicCollisionShapePacket::bSpyDO_Inactive;
              dcs.m_flags |= DynamicCollisionShapePacket::bSpyDO_EPatch;
              ////add offset so I can see it in a different place
              //bTm.d.y += 1.f;
              dcs.m_tm = bSpyMat34fromMatrix34(bTm);

              phBoundToShapePrimitive(pBound, dcs.m_shape);
              bspySendPacket(dcs);

              if (m_patches[i].instLevelIndex > -1)
              {
                //convert the input to global co-ordinates
                boundToWorldSpace(&posGlob, m_patches[i].knownContactPointFromCollision[0].local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
                rotateBoundToWorldSpace(&normalGlob, m_patches[i].knownContactNormalFromCollision[0].local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
              }
              bspyDrawLine(posGlob,posGlob+normalGlob,rage::Vector3(1.f,1.f,1.f));
            }//bound exists
          }//bound exists
        }//inst exists
      }
      break;
#endif //#if NM_EA_TEST_FROM_IMPACTS

    case Patch::EO_Disc:                  
      //mmmmtodo add as bSpy::bSpyShapePrimitive::eShapeMeshRef
      {
        if (m_patches[i].instLevelIndex > -1)
        {
          //convert the input to global co-ordinates
          boundToWorldSpace(&posGlob, m_patches[i].knownContactPointFromCollision[0].local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
          rotateBoundToWorldSpace(&normalGlob, m_patches[i].knownContactNormalFromCollision[0].local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
        }
        bspyDrawLine(posGlob,posGlob+normalGlob,rage::Vector3(0.2f,1.f,0.f));
        bspyDrawLine(m_patches[i].corner.global,m_patches[i].corner.global+m_patches[i].faceNormals[0].global,rage::Vector3(0.f,1.f,0.f));
        bspyDrawCircle(m_patches[i].corner.global, m_patches[i].faceNormals[0].global,m_patches[i].edgeLengths[0],rage::Vector3(0.f,1.f,0.f),15);
      }
      break;
    case Patch::EO_Plane:                  
      //mmmmtodo add as bSpy::bSpyShapePrimitive::eShapeMeshRef
      {
        posGlob = m_patches[i].corner.global;
        normalGlob = m_patches[i].faceNormals[0].global;
        rage::Vector3 edge1 = m_patches[i].faceNormals[1].global;;
        rage::Vector3 edge2 = m_patches[i].faceNormals[2].global;;
        if (m_patches[i].instLevelIndex > -1)
        {
          //convert the input to global co-ordinates
          boundToWorldSpace(&posGlob, m_patches[i].corner.local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
          rotateBoundToWorldSpace(&normalGlob, m_patches[i].faceNormals[0].local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
          rotateBoundToWorldSpace(&edge1, m_patches[i].faceNormals[1].local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
          rotateBoundToWorldSpace(&edge2, m_patches[i].faceNormals[2].local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
        }
        bspyDrawLine(posGlob,posGlob+normalGlob,rage::Vector3(02.f,1.f,0.f));

        //edge2.Cross(normalGlob,edge1);
        bspyDrawLine(posGlob,posGlob+edge1*m_patches[i].edgeLengths[0],rage::Vector3(0.f,1.f,0.f));
        bspyDrawLine(posGlob,posGlob+edge2*m_patches[i].edgeLengths[1],rage::Vector3(0.f,1.f,0.f));
        bspyDrawLine(posGlob+edge1*m_patches[i].edgeLengths[0],posGlob+edge1*m_patches[i].edgeLengths[0]+edge2*m_patches[i].edgeLengths[1], rage::Vector3(0.f,1.f,0.f));
        bspyDrawLine(posGlob+edge2*m_patches[i].edgeLengths[1],posGlob+edge1*m_patches[i].edgeLengths[0]+edge2*m_patches[i].edgeLengths[1], rage::Vector3(0.f,1.f,0.f));
      }
      break;
    case Patch::EO_Line:                  
      {
        posGlob = m_patches[i].corner.global;
        normalGlob = m_patches[i].faceNormals[0].global;
        rage::Vector3 edge1 = m_patches[i].faceNormals[2].global;;
        if (m_patches[i].instLevelIndex > -1)
        {
          //convert the input to global co-ordinates
          boundToWorldSpace(&posGlob, m_patches[i].corner.local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
          rotateBoundToWorldSpace(&normalGlob, m_patches[i].faceNormals[0].local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
          rotateBoundToWorldSpace(&edge1, m_patches[i].faceNormals[2].local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
        }
        bspyDrawLine(posGlob,posGlob+normalGlob,rage::Vector3(0.2f,1.f,0.f));
        bspyDrawLine(posGlob,posGlob+edge1*m_patches[i].edgeLengths[0],rage::Vector3(0.f,1.f,0.f));
      }
      break;
    case Patch::EO_Point:                  
      //mmmmtodo add as bSpy::bSpyShapePrimitive::eShapeMeshRef
      {
        posGlob = m_patches[i].corner.global;
        normalGlob = m_patches[i].faceNormals[0].global;
        if (m_patches[i].instLevelIndex > -1)
        {
          //convert the input to global co-ordinates
          boundToWorldSpace(&posGlob, m_patches[i].corner.local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
          rotateBoundToWorldSpace(&normalGlob, m_patches[i].faceNormals[0].local, m_patches[i].instLevelIndex, m_patches[i].boundIndex);
        }
        bspyDrawPoint(posGlob, 0.2f, rage::Vector3(0.2f,1.f,0.f));
        bspyDrawLine(posGlob,posGlob+normalGlob,rage::Vector3(0.f,1.f,0.f));
      }
      break;
    default:             
      break;
    }
    bspyScratchpad(getBSpyID(),"Patches", i);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].instLevelIndex);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].boundIndex);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].type);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].corner.global);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].faceNormals[0].global);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].faceNormals[1].global);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].faceNormals[2].global);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].edgeLengths[0]);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].edgeLengths[1]);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].edgeLengths[2]);
    bspyScratchpad(getBSpyID(),"Patches", posGlob);
    bspyScratchpad(getBSpyID(),"Patches", normalGlob);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].knownContactPointFromCollision[0].global);
    bspyScratchpad(getBSpyID(),"Patches", m_patches[i].knownContactNormalFromCollision[0].global);

  }
#endif
#endif
} // namespace ART
