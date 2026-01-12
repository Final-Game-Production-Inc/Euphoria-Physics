//
// vector/colorvector3.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_COLORVECTOR3_H
#define VECTOR_COLORVECTOR3_H

#include "vector3.h"

namespace rage {

//=============================================================================
// ColorVector3
// PURPOSE
//   ColorVector3 represents a three-dimensional color without alpha transparency.
// NOTES
//   Currently this class does not track the color space (e.g. RGB, HSV, etc) 
//   that is being represented by the contained data.
// <FLAG Component>
//
class ColorVector3 : public Vector3
{
public:
	//=========================================================================

	// PURPOSE: Default constructor.
	// NOTES: Sets the color to RGB white.
	ColorVector3();

	// PURPOSE: Converts this color from RGB space to HSV space.
	// NOTES: Currently it is left to the user to ensure that the color was represented in RGB.
	void RGBtoHSV();

	// PURPOSE: Converts this color from HSV space to RGB space.
	// NOTES: Currently it is left to the user to ensure that the color was represented in HSV.
	void HSVtoRGB();

	// PURPOSE: Determines if this color (represented in RGB) is a "hot" NTSC color.
	// RETURNS: TODO
	// NOTES: A "hot" NTSC color is one that is outside of the NTSC color space.
	float IsNtscSafeColor() const;

	// PURPOSE: Converts this color to the closest non-"hot" NTSC color.
	// NOTES: A "hot" NTSC color is one that is outside of the NTSC color space.
	void TransformToNtscSafeColor();
};


//=============================================================================
// Implementations

inline ColorVector3::ColorVector3()
{
	x = 1.0f;
	y = 1.0f;
	z = 1.0f;
}


}	// namespace rage

#endif // VECTOR_VECTOR3_H
