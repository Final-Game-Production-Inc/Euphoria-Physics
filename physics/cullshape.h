// 
// physics/cullshape.h 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_CULL_SHAPE_H 
#define PHYSICS_CULL_SHAPE_H 

#include "grprofile/drawmanager.h"
#include "vector/geometry.h"
#include "vector/matrix34.h"
#include "vector/vector3.h"

namespace rage {

////////////////////////////////////////////

// Class used to define a culling shape.
class phCullShape
{
public:
    enum phCullType
    {
        PHCULLTYPE_UNSPECIFIED = -1,
        PHCULLTYPE_SPHERE,
        PHCULLTYPE_LINESEGMENT,
        PHCULLTYPE_XZCIRCLE,
        PHCULLTYPE_CAPSULE,
        PHCULLTYPE_BOX,
		PHCULLTYPE_POINT,
		PHCULLTYPE_AABB,
        PHCULLTYPE_ALL,						// Useful for when you care only about the object's state.
    };

	enum
	{
		DATA_BUFFER_SIZE = 80
	};

	class phCullData_Sphere
	{
	public:
		Vec3V m_Center;
		ScalarV m_Radius;
	};

	class phCullData_LineSegment
	{
	public:
		Vec3V m_P0, m_P1;
	};

	class phCullData_XZCircle
	{
	public:
		Vec3V m_Center;
		ScalarV m_Radius;
	};

	class phCullData_Capsule
	{
	public:
		Vec3V m_P0, m_ShaftAxis;
		ScalarV m_ShaftLength, m_Radius;
	};

	class phCullData_Box
	{
	public:
		Mat34V m_BoxAxes;
		Vec3V m_BoxHalfSize;
	};

	class phCullData_Point
	{
	public:
		Vec3V m_vecPoint;
	};

	class phCullData_AABB
	{
	public:
		Vec3V m_BoxCenter;
		Vec3V m_BoxHalfSize;
	};

	phCullShape();
	phCullShape(const phCullShape& other);

    ~phCullShape();

	__forceinline phCullShape &operator =(const phCullShape &otherCullShape)	// Should this be __forceinline'd or not?  I'm not sure.
	{
		CompileTimeAssert(sizeof(phCullShape) == 96);
		//sysMemCpy(this, &otherCullShape, sizeof(phCullShape));
		Vec3V *thisAsVec3V = reinterpret_cast<Vec3V *>(this);
		const Vec3V *otherAsVec3V = reinterpret_cast<const Vec3V *>(&otherCullShape);
		thisAsVec3V[0] = otherAsVec3V[0];
		thisAsVec3V[1] = otherAsVec3V[1];
		thisAsVec3V[2] = otherAsVec3V[2];
		thisAsVec3V[3] = otherAsVec3V[3];
		thisAsVec3V[4] = otherAsVec3V[4];
		thisAsVec3V[5] = otherAsVec3V[5];
		return *this;
	}

    phCullType GetCullType() const;

	void InitCull_Sphere(Vec3V_In center, ScalarV_In radius);
	void InitCull_LineSegment(Vec3V_In p0, Vec3V_In p1);
	void InitCull_XZCircle(Vec3V_In center, ScalarV_In radius);
	void InitCull_Capsule(Vec3V_In p0, Vec3V_In shaftAxis, ScalarV_In shaftLength, ScalarV_In radius);
	void InitCull_Box(Mat34V_In boxAxes, Vec3V_In boxHalfSize);
	void InitCull_Point(Vec3V_In point);
	void InitCull_AABB(Vec3V_In boxCenter, Vec3V_In boxHalfSize);
	void InitCull_All();

	// In the future, in order to avoid needing these functions (and to avoid exposing each of the phCullData_XXX classes) we should abstract the functionality
	//   necessary to cull against BVH nodes - that is, provide a -vs-quantized AABB culling function that can then get plugged into the BVH culling functionality.
	const phCullData_Sphere &GetSphereData() const;
	const phCullData_LineSegment &GetLineSegmentData() const;
	const phCullData_XZCircle &GetXZCircleData() const;
	const phCullData_Capsule &GetCapsuleData() const;
	const phCullData_Box &GetBoxData() const;
	const phCullData_Point &GetPointData() const;
	const phCullData_AABB &GetAABBData() const;

	void Localize(phCullShape &localizedCullShape, Mat34V_In matrix) const;

	bool CheckSphere(Vec3V_In sphereCenter, ScalarV_In sphereRadius) const;
	bool CheckAABB(Vec3V_In AABBCenter, Vec3V_In AABBHalfSize) const;

#if __PFDRAW
	void Draw() const;
#endif


private:

    ALIGNAS(16) u8 m_CullShapeData[DATA_BUFFER_SIZE] ;				// Buffer to hold one of the phCullData objects.
    phCullType m_CullDataType;
};

inline phCullShape::phCullType phCullShape::GetCullType() const
{
    return m_CullDataType; 
}

inline void phCullShape::InitCull_Sphere(Vec3V_In center, ScalarV_In radius)
{
	CompileTimeAssert(sizeof(phCullData_Sphere) <= DATA_BUFFER_SIZE);	// To ensure that phCullData_Sphere will fit in m_CullShapeData.
	phCullData_Sphere* data = (phCullData_Sphere*)(m_CullShapeData);
	data->m_Center = center;
	data->m_Radius = radius;

	m_CullDataType = PHCULLTYPE_SPHERE;
};

inline void phCullShape::InitCull_LineSegment(Vec3V_In p0, Vec3V_In p1)
{
	CompileTimeAssert(sizeof(phCullData_LineSegment) <= DATA_BUFFER_SIZE);	// To ensure that phCullData_LineSegment will fit in m_CullShapeData.
	phCullData_LineSegment* data = (phCullData_LineSegment*)(m_CullShapeData);
	data->m_P0 = p0;
	data->m_P1 = p1;

	m_CullDataType = PHCULLTYPE_LINESEGMENT;
}

inline void phCullShape::InitCull_XZCircle(Vec3V_In center, ScalarV_In radius)
{
	CompileTimeAssert(sizeof(phCullData_XZCircle) <= DATA_BUFFER_SIZE);	// To ensure that phCullData_XZCircle will fit in m_CullShapeData.
	phCullData_XZCircle* data = (phCullData_XZCircle*)(m_CullShapeData);
	data->m_Center = center;
	data->m_Radius = radius;

	m_CullDataType = PHCULLTYPE_XZCIRCLE;
};

inline void phCullShape::InitCull_Capsule(Vec3V_In p0, Vec3V_In shaftAxis, ScalarV_In shaftLength, ScalarV_In radius)
{
	CompileTimeAssert(sizeof(phCullData_Capsule) <= DATA_BUFFER_SIZE);	// To ensure that phCullData_Capsule will fit in m_CullShapeData.
	phCullData_Capsule* data = (phCullData_Capsule*)(m_CullShapeData);
	data->m_P0 = p0;
	data->m_ShaftAxis = SelectFT(IsEqual(shaftLength, ScalarV(V_ZERO)), Normalize(shaftAxis), Vec3V(V_ZERO));
	data->m_ShaftLength = shaftLength;
	data->m_Radius = radius;

	m_CullDataType = PHCULLTYPE_CAPSULE;
}

inline void phCullShape::InitCull_Box (Mat34V_In boxAxes, Vec3V_In boxHalfSize)
{
	CompileTimeAssert(sizeof(phCullData_Box) <= DATA_BUFFER_SIZE);	// To ensure that phCullData_Box will fit in m_CullShapeData.
	phCullData_Box* data = (phCullData_Box*)(m_CullShapeData);
	data->m_BoxAxes = boxAxes;
	data->m_BoxHalfSize = boxHalfSize;

	m_CullDataType = PHCULLTYPE_BOX;
}

inline void phCullShape::InitCull_Point(Vec3V_In point)
{
	CompileTimeAssert(sizeof(phCullData_Point) <= DATA_BUFFER_SIZE);	// To ensure that phCullData_Point will fit in m_CullShapeData.
	phCullData_Point* data = (phCullData_Point*)(m_CullShapeData);
	data->m_vecPoint = point;

	m_CullDataType = PHCULLTYPE_POINT;
}

inline void phCullShape::InitCull_AABB(Vec3V_In boxCenter, Vec3V_In boxHalfSize)
{
	CompileTimeAssert(sizeof(phCullData_AABB) <= DATA_BUFFER_SIZE);	// To ensure that phCullData_AABB will fit in m_CullShapeData.
	phCullData_AABB* data = (phCullData_AABB*)(m_CullShapeData);
	data->m_BoxCenter = boxCenter;
	data->m_BoxHalfSize = boxHalfSize;

	m_CullDataType = PHCULLTYPE_AABB;
}

inline void phCullShape::InitCull_All()
{
    m_CullDataType = PHCULLTYPE_ALL;
}


inline phCullShape::phCullShape() : 
m_CullDataType(PHCULLTYPE_UNSPECIFIED)
{
}

inline phCullShape::phCullShape(const phCullShape& other)
{
	*this = other;
}

inline phCullShape::~phCullShape()
{
}

__forceinline const phCullShape::phCullData_Sphere &phCullShape::GetSphereData() const
{
	FastAssert(GetCullType() == PHCULLTYPE_SPHERE);
	return *(reinterpret_cast<const phCullData_Sphere *>(m_CullShapeData));
}

__forceinline const phCullShape::phCullData_LineSegment &phCullShape::GetLineSegmentData() const
{
	FastAssert(GetCullType() == PHCULLTYPE_LINESEGMENT);
	return *(reinterpret_cast<const phCullData_LineSegment *>(m_CullShapeData));
}

__forceinline const phCullShape::phCullData_XZCircle &phCullShape::GetXZCircleData() const
{
	FastAssert(GetCullType() == PHCULLTYPE_XZCIRCLE);
	return *(reinterpret_cast<const phCullData_XZCircle *>(m_CullShapeData));
}

__forceinline const phCullShape::phCullData_Capsule &phCullShape::GetCapsuleData() const
{
	FastAssert(GetCullType() == PHCULLTYPE_CAPSULE);
	return *(reinterpret_cast<const phCullData_Capsule *>(m_CullShapeData));
}

__forceinline const phCullShape::phCullData_Box &phCullShape::GetBoxData() const
{
	FastAssert(GetCullType() == PHCULLTYPE_BOX);
	return *(reinterpret_cast<const phCullData_Box *>(m_CullShapeData));
}

__forceinline const phCullShape::phCullData_Point &phCullShape::GetPointData() const
{
	FastAssert(GetCullType() == PHCULLTYPE_POINT);
	return *(reinterpret_cast<const phCullData_Point *>(m_CullShapeData));
}


__forceinline const phCullShape::phCullData_AABB &phCullShape::GetAABBData() const
{
	FastAssert(GetCullType() == PHCULLTYPE_AABB);
	return *(reinterpret_cast<const phCullData_AABB *>(m_CullShapeData));
}

} // namespace rage

#endif // PHYSICS_CULL_SHAPE_H 
