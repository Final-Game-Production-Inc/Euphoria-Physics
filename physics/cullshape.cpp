// 
// physics/cullshape.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "cullshape.h"

#include "vectormath/classes.h"

namespace rage {

EXT_PFD_DECLARE_ITEM(PhysicsLevelCullShape);


void phCullShape::Localize(phCullShape &localizedCullShape, Mat34V_In matrix) const
{
	Assert(m_CullDataType != PHCULLTYPE_UNSPECIFIED);
	localizedCullShape.m_CullDataType = m_CullDataType;

	switch( m_CullDataType )
	{

	case PHCULLTYPE_SPHERE :
		{
			const phCullData_Sphere* RESTRICT data = (const phCullData_Sphere *)(m_CullShapeData);
			const Vec3V localizedCenter = UnTransformOrtho(matrix, data->m_Center);
			const ScalarV radius = data->m_Radius;

			phCullData_Sphere* RESTRICT localizedData = (phCullData_Sphere *)(localizedCullShape.m_CullShapeData);
			localizedData->m_Center = localizedCenter;
			localizedData->m_Radius = radius;
			break;
		}

	case PHCULLTYPE_LINESEGMENT :
		{
			// Locally declaring the parameters for TestSphereToSeg avoids a vector size assert failure with VECTORIZED && __WIN32.
			const phCullData_LineSegment* RESTRICT data = (const phCullData_LineSegment *)(m_CullShapeData);
			const Vec3V localizedP0 = UnTransformOrtho(matrix, data->m_P0);
			const Vec3V localizedP1 = UnTransformOrtho(matrix, data->m_P1);

			phCullData_LineSegment* RESTRICT localizedData = (phCullData_LineSegment *)(localizedCullShape.m_CullShapeData);
			localizedData->m_P0 = localizedP0;
			localizedData->m_P1 = localizedP1;
			break;
		}

	case PHCULLTYPE_XZCIRCLE :
		{
			const phCullData_XZCircle* RESTRICT data = (const phCullData_XZCircle *)(m_CullShapeData);
			const Vec3V localizedCenter = UnTransformOrtho(matrix, data->m_Center);
			const ScalarV radius = data->m_Radius;

			phCullData_XZCircle* RESTRICT localizedData = (phCullData_XZCircle *)(localizedCullShape.m_CullShapeData);
			localizedData->m_Center = localizedCenter;
			localizedData->m_Radius = radius;
			break;
		}

	case PHCULLTYPE_CAPSULE :
		{
			const phCullData_Capsule* RESTRICT data = (const phCullData_Capsule *)(m_CullShapeData);
			const Vec3V localizedP0 = UnTransformOrtho(matrix, data->m_P0);
			const Vec3V localizedShaftAxis = UnTransform3x3Ortho(matrix, data->m_ShaftAxis);
			const ScalarV radius = data->m_Radius;
			const ScalarV shaftLength = data->m_ShaftLength;

			phCullData_Capsule* RESTRICT localizedData = (phCullData_Capsule *)(localizedCullShape.m_CullShapeData);
			localizedData->m_P0 = localizedP0;
			localizedData->m_ShaftAxis = localizedShaftAxis;
			localizedData->m_Radius = radius;
			localizedData->m_ShaftLength = shaftLength;
			break;
		}

	case PHCULLTYPE_BOX :
		{
			const phCullData_Box* RESTRICT data = (const phCullData_Box *)(m_CullShapeData);
			Mat34V localizedBoxAxes;
			UnTransformOrtho(localizedBoxAxes, matrix, data->m_BoxAxes);
			const Vec3V boxHalfSize = data->m_BoxHalfSize;

			// TODO: Is this writing to a location on the stack and then reading from there or is it writing directly to the destination in the other cull shape?
			phCullData_Box* RESTRICT localizedData = (phCullData_Box *)(localizedCullShape.m_CullShapeData);
			localizedData->m_BoxAxes = localizedBoxAxes;
			localizedData->m_BoxHalfSize = boxHalfSize;
			break;
		}

	case PHCULLTYPE_POINT :
		{
			const phCullData_Point* RESTRICT data = (const phCullData_Point *)(m_CullShapeData);
			const Vec3V localizedCenter = UnTransformOrtho(matrix, data->m_vecPoint);

			phCullData_Point* RESTRICT localizedData = (phCullData_Point *)(localizedCullShape.m_CullShapeData);
			localizedData->m_vecPoint = localizedCenter;
			break;
		}

	case PHCULLTYPE_AABB :
		{
			const phCullData_AABB* RESTRICT data = (const phCullData_AABB *)(m_CullShapeData);
			const Vec3V localizedCenter = UnTransformOrtho(matrix, data->m_BoxCenter);
			Mat33V inverseRotation;
			InvertOrtho(inverseRotation,matrix.GetMat33ConstRef());
			const Vec3V localizedHalfSize = geomBoxes::ComputeAABBExtentsFromOBB(inverseRotation,data->m_BoxHalfSize);

			phCullData_AABB* RESTRICT localizedData = (phCullData_AABB *)(localizedCullShape.m_CullShapeData);
			localizedData->m_BoxCenter = localizedCenter;
			localizedData->m_BoxHalfSize = localizedHalfSize;
			break;
		}

	default :
		{
			Assert(m_CullDataType == PHCULLTYPE_ALL);
			break;
		}
	}
}


bool phCullShape::CheckSphere(Vec3V_In sphereCenter, ScalarV_In sphereRadius) const
{
	Assert(m_CullDataType != PHCULLTYPE_UNSPECIFIED);

	switch( m_CullDataType )
	{

	case PHCULLTYPE_SPHERE :
		{
			const phCullData_Sphere* RESTRICT data = (const phCullData_Sphere *)(m_CullShapeData);
			const ScalarV testRadiusV = Add( data->m_Radius, sphereRadius );
			const Vec3V testCenterV = Subtract( data->m_Center, sphereCenter );
			return (IsLessThanAll( Dot(testCenterV, testCenterV), Scale(testRadiusV, testRadiusV) ) != 0);
		}

	case PHCULLTYPE_LINESEGMENT :
		{
			// Locally declaring the parameters for TestSphereToSeg avoids a vector size assert failure with VECTORIZED && __WIN32.
			const phCullData_LineSegment* RESTRICT data = (const phCullData_LineSegment *)(m_CullShapeData);
			/// ScalarV radius2 = Scale(sphereRadius, sphereRadius);
			Vec3V start(data->m_P0);
			Vec3V end(data->m_P1);
			return (geomSpheres::TestSphereToSeg(sphereCenter, sphereRadius, start, end));
		}

	case PHCULLTYPE_XZCIRCLE :
		{
			const phCullData_XZCircle* RESTRICT data = (const phCullData_XZCircle *)(m_CullShapeData);
			ScalarV testRadius = Add(data->m_Radius, sphereRadius);
			Vec2V centerFlatDifference = Subtract(data->m_Center, sphereCenter).Get<Vec::X, Vec::Z>();
			return (IsLessThanAll(MagSquared(centerFlatDifference), Scale(testRadius, testRadius)) != 0);
		}

	case PHCULLTYPE_CAPSULE :
		{
			const phCullData_Capsule* RESTRICT data = (const phCullData_Capsule *)(m_CullShapeData);
			return geomSpheres::TestSphereToSeg(sphereCenter, Add(data->m_Radius, sphereRadius), data->m_P0, AddScaled(data->m_P0, data->m_ShaftAxis, data->m_ShaftLength));
		}

	case PHCULLTYPE_BOX :
		{
			const phCullData_Box* RESTRICT data = (const phCullData_Box *)(m_CullShapeData);
			Vec3V boxMax(data->m_BoxHalfSize);
			Vec3V boxMin(boxMax);
			boxMin = Negate(boxMin);
			return geomBoxes::TestSphereToBox(sphereCenter, sphereRadius, boxMin, boxMax, RCC_MATRIX34(data->m_BoxAxes));
		}

	case PHCULLTYPE_POINT :
		{
			const phCullData_Point* RESTRICT data = (const phCullData_Point *)(m_CullShapeData);
			return (IsLessThanAll( MagSquared( Subtract(data->m_vecPoint, sphereCenter) ), Scale( sphereRadius, sphereRadius ) ) != 0);
		}

	case PHCULLTYPE_AABB :
		{
			const phCullData_AABB* RESTRICT data = (const phCullData_AABB *)(m_CullShapeData);
			return geomBoxes::TestSphereToAABB_CenterHalfSize(sphereCenter,sphereRadius,data->m_BoxCenter,data->m_BoxHalfSize).Getb();
		}

	default :
		{
			Assert(m_CullDataType == PHCULLTYPE_ALL);
			return true;
		}
	}
}


bool phCullShape::CheckAABB(Vec3V_In AABBCenter, Vec3V_In AABBHalfSize) const
{
	AssertMsg(m_CullDataType == PHCULLTYPE_AABB, "Only AABBs are supported with 'cull against instance AABBs'.");

	const phCullData_AABB* RESTRICT data = (const phCullData_AABB *)(m_CullShapeData);
	return geomBoxes::TestAABBtoAABB_CenterHalfSize(AABBCenter,AABBHalfSize,data->m_BoxCenter,data->m_BoxHalfSize).Getb();
}


#if __PFDRAW
void phCullShape::Draw() const
{
	switch (m_CullDataType)
	{
		case PHCULLTYPE_SPHERE:
		{
			const phCullData_Sphere* data = (const phCullData_Sphere *)(m_CullShapeData);
			PFD_PhysicsLevelCullShape.DrawSphere(data->m_Radius.Getf(),RCC_VECTOR3(data->m_Center));
			break;
		}

		case PHCULLTYPE_LINESEGMENT:
		{
			const phCullData_LineSegment* data = (const phCullData_LineSegment *)(m_CullShapeData);
			PFD_PhysicsLevelCullShape.DrawLine(RCC_VECTOR3(data->m_P0),RCC_VECTOR3(data->m_P1));
			break;
		}

		case PHCULLTYPE_XZCIRCLE:
		{
			const phCullData_XZCircle* data = (const phCullData_XZCircle *)(m_CullShapeData);
			Vec3V low(data->m_Center);
			low = SubtractScaled(low, Vec3V(g_UnitUp), ScalarVFromF32(20.0f));
			Vec3V high(data->m_Center);
			high = AddScaled(high, Vec3V(g_UnitUp), ScalarVFromF32(20.0f));
			PFD_PhysicsLevelCullShape.DrawCapsule(RCC_VECTOR3(low), RCC_VECTOR3(high), data->m_Radius.Getf());
			break;
		}

		case PHCULLTYPE_CAPSULE:
		{
			const phCullData_Capsule* data = (const phCullData_Capsule *)(m_CullShapeData);
			Vec3V shaftEnd(data->m_P0);
			shaftEnd = AddScaled(shaftEnd, data->m_ShaftAxis, data->m_ShaftLength);
			PFD_PhysicsLevelCullShape.DrawCapsule(RCC_VECTOR3(data->m_P0), RCC_VECTOR3(shaftEnd), data->m_Radius.Getf());
			break;
		}

		case PHCULLTYPE_BOX:
		{
			const phCullData_Box* data = (const phCullData_Box *)(m_CullShapeData);
			PFD_PhysicsLevelCullShape.DrawBox(RCC_MATRIX34(data->m_BoxAxes), RCC_VECTOR3(data->m_BoxHalfSize));
			break;
		}

		case PHCULLTYPE_POINT:
		{
			const phCullData_Point* data = (const phCullData_Point *)(m_CullShapeData);
			PFD_PhysicsLevelCullShape.DrawTick(RCC_VECTOR3(data->m_vecPoint),0.1f);
			break;
		}

		case PHCULLTYPE_AABB:
		{
			const phCullData_AABB* data = (const phCullData_AABB *)(m_CullShapeData);
			Mat34V matrix(Mat33V(V_IDENTITY), data->m_BoxCenter);
			PFD_PhysicsLevelCullShape.DrawBox(RCC_MATRIX34(matrix), RCC_VECTOR3(data->m_BoxHalfSize));
			break;
		}

		case PHCULLTYPE_ALL:
		case PHCULLTYPE_UNSPECIFIED:
		{
			break;
		}
	}
}
#endif

} // namespace rage
