//
// vector/vector2.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "vector3.h"

#include "data/struct.h"

using namespace rage;

//=============================================================================
// Vector2

Vector2::Vector2(const Vector3 & v3d, eVector3Axes axes)
{
	switch(axes)
	{
		case kXY:
			x = v3d.x;
			y = v3d.y;
			break;
		case kXZ:
			x = v3d.x;
			y = v3d.z;
			break;
		case kYZ:
			x = v3d.y;
			y = v3d.z;
			break;
		default:
			mthErrorf("Invalid axis selector %d", axes);
	}
}


float Vector2::ComputeSide(const Vector2 &p1, const Vector2 &p2) const
{
	return (p2.x - p1.x)*(y - p1.y) - (x - p1.x)*(p2.y - p1.y);
}

void Vector2::Print(bool newline) const
{
	Printf("%f,%f",x,y);
	if (newline)
	{
		Printf("\n");
	}
}


void Vector2::Print(const char *OUTPUT_ONLY(label), bool newline) const
{
	Printf("%s: %f,%f",label,x,y);
	if (newline)
	{
		Printf("\n");
	}
}

#if __DECLARESTRUCT
void Vector2::DeclareStruct(datTypeStruct &s) {
	STRUCT_BEGIN(Vector2);
	STRUCT_FIELD(x);
	STRUCT_FIELD(y);
	STRUCT_END();
}
#endif
