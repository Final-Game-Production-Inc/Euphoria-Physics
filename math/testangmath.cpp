//
// math/testangmath.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "angmath.h"
#include "system/main.h"

#include <stdio.h>

using namespace rage;

int Main()
{
	float angle = 0.0f;
	float min = -0.5f * PI;
	float max = 0.5f * PI;
	printf("%f pi \t= AngleClamp(%f pi, %f pi, %f pi)\n", AngleClamp(angle, min, max) / PI, angle / PI, min / PI, max / PI);
	angle = 0.2f * PI;
	printf("%f pi \t= AngleClamp(%f pi, %f pi, %f pi)\n", AngleClamp(angle, min, max) / PI, angle / PI, min / PI, max / PI);
	angle = 0.4f * PI;
	printf("%f pi \t= AngleClamp(%f pi, %f pi, %f pi)\n", AngleClamp(angle, min, max) / PI, angle / PI, min / PI, max / PI);
	angle = 0.6f * PI;
	printf("%f pi \t= AngleClamp(%f pi, %f pi, %f pi)\n", AngleClamp(angle, min, max) / PI, angle / PI, min / PI, max / PI);
	angle = 0.8f * PI;
	printf("%f pi \t= AngleClamp(%f pi, %f pi, %f pi)\n", AngleClamp(angle, min, max) / PI, angle / PI, min / PI, max / PI);
	angle = 1.0f * PI;
	printf("%f pi \t= AngleClamp(%f pi, %f pi, %f pi)\n", AngleClamp(angle, min, max) / PI, angle / PI, min / PI, max / PI);
	angle = 1.2f * PI;
	printf("%f pi \t= AngleClamp(%f pi, %f pi, %f pi)\n", AngleClamp(angle, min, max) / PI, angle / PI, min / PI, max / PI);
	angle = 1.4f * PI;
	printf("%f pi \t= AngleClamp(%f pi, %f pi, %f pi)\n", AngleClamp(angle, min, max) / PI, angle / PI, min / PI, max / PI);
	angle = 1.6f * PI;
	printf("%f pi \t= AngleClamp(%f pi, %f pi, %f pi)\n", AngleClamp(angle, min, max) / PI, angle / PI, min / PI, max / PI);
	angle = 1.8f * PI;
	printf("%f pi \t= AngleClamp(%f pi, %f pi, %f pi)\n", AngleClamp(angle, min, max) / PI, angle / PI, min / PI, max / PI);

	angle = -3.0f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = -2.5f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = -2.0f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = -1.5f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = -1.0f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = -0.5f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = -0.0f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = 0.5f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = 1.0f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = 1.5f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = 2.0f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = 2.5f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);
	angle = 3.0f * PI;
	printf("%f pi \t= CanonicalizeAngle(%f pi)\n", CanonicalizeAngle(angle) / PI, angle / PI);

	float angle2 = -PI;
	angle = -2.0f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.5f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.0f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -0.5f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.0f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.5f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.0f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.5f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);

	angle2 = 0.5f * PI;
	angle = -2.0f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.5f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.0f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -0.5f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.0f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.5f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.0f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.5f * PI;
	printf("%f pi \t= SubtractAngleShorter(%f pi, %f pi)\n", SubtractAngleShorter(angle, angle2) / PI, angle / PI, angle2 / PI);

	angle2 = -PI;
	angle = -2.0f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.5f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.0f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -0.5f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.0f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.5f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.0f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.5f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);

	angle2 = 0.5f * PI;
	angle = -2.0f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.5f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.0f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -0.5f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.0f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.5f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.0f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.5f * PI;
	printf("%f pi \t= SubtractAngleLonger(%f pi, %f pi)\n", SubtractAngleLonger(angle, angle2) / PI, angle / PI, angle2 / PI);

	angle2 = -PI;
	angle = -2.0f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.5f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.0f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -0.5f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.0f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.5f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.0f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.5f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);

	angle2 = 0.5f * PI;
	angle = -2.0f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.5f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.0f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -0.5f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.0f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.5f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.0f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.5f * PI;
	printf("%f pi \t= SubtractAngleNegative(%f pi, %f pi)\n", SubtractAngleNegative(angle, angle2) / PI, angle / PI, angle2 / PI);

	angle2 = -PI;
	angle = -2.0f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.5f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.0f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -0.5f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.0f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.5f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.0f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.5f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);

	angle2 = 0.5f * PI;
	angle = -2.0f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.5f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -1.0f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle = -0.5f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.0f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  0.5f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.0f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);
	angle =  1.5f * PI;
	printf("%f pi \t= SubtractAnglePositive(%f pi, %f pi)\n", SubtractAnglePositive(angle, angle2) / PI, angle / PI, angle2 / PI);

	angle = 0.0f * PI;
	angle2 = 0.75f * PI;
	float t = 0.0f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 0.25f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 0.5f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 0.75f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 1.0f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);

	angle = 0.0f * PI;
	angle2 = 1.25f * PI;
	t = 0.0f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 0.25f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 0.5f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 0.75f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 1.0f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);

	angle = -0.25f * PI;
	angle2 = 0.5f * PI;
	t = 0.0f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 0.25f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 0.5f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 0.75f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);
	t = 1.0f;
	printf("%f pi \t= InterpolateAngle(%f pi, %f pi, %f pi)\n", InterpolateAngle(t, angle, angle2) / PI, t / PI, angle / PI, angle2 / PI);

	float baseAngle = 0.0f;
	angle = 0.25f * PI;
	angle2 = 0.75f * PI;
	printf("%s \t= IsCloserToAngle(%f pi, %f pi, %f pi)\n", IsCloserToAngle(baseAngle, angle, angle2) ? "true" : "false",
		   baseAngle / PI, angle / PI, angle2 / PI);
	baseAngle = 0.1f * PI;
	printf("%s \t= IsCloserToAngle(%f pi, %f pi, %f pi)\n", IsCloserToAngle(baseAngle, angle, angle2) ? "true" : "false",
		   baseAngle / PI, angle / PI, angle2 / PI);
	baseAngle = 0.2f * PI;
	printf("%s \t= IsCloserToAngle(%f pi, %f pi, %f pi)\n", IsCloserToAngle(baseAngle, angle, angle2) ? "true" : "false",
		   baseAngle / PI, angle / PI, angle2 / PI);
	baseAngle = 0.3f * PI;
	printf("%s \t= IsCloserToAngle(%f pi, %f pi, %f pi)\n", IsCloserToAngle(baseAngle, angle, angle2) ? "true" : "false",
		   baseAngle / PI, angle / PI, angle2 / PI);
	baseAngle = 0.4f * PI;
	printf("%s \t= IsCloserToAngle(%f pi, %f pi, %f pi)\n", IsCloserToAngle(baseAngle, angle, angle2) ? "true" : "false",
		   baseAngle / PI, angle / PI, angle2 / PI);
	baseAngle = 0.5f * PI;
	printf("%s \t= IsCloserToAngle(%f pi, %f pi, %f pi)\n", IsCloserToAngle(baseAngle, angle, angle2) ? "true" : "false",
		   baseAngle / PI, angle / PI, angle2 / PI);
	baseAngle = 0.6f * PI;
	printf("%s \t= IsCloserToAngle(%f pi, %f pi, %f pi)\n", IsCloserToAngle(baseAngle, angle, angle2) ? "true" : "false",
		   baseAngle / PI, angle / PI, angle2 / PI);
	baseAngle = 0.7f * PI;
	printf("%s \t= IsCloserToAngle(%f pi, %f pi, %f pi)\n", IsCloserToAngle(baseAngle, angle, angle2) ? "true" : "false",
		   baseAngle / PI, angle / PI, angle2 / PI);
	baseAngle = 0.8f * PI;
	printf("%s \t= IsCloserToAngle(%f pi, %f pi, %f pi)\n", IsCloserToAngle(baseAngle, angle, angle2) ? "true" : "false",
		   baseAngle / PI, angle / PI, angle2 / PI);
	baseAngle = 0.9f * PI;
	printf("%s \t= IsCloserToAngle(%f pi, %f pi, %f pi)\n", IsCloserToAngle(baseAngle, angle, angle2) ? "true" : "false",
		   baseAngle / PI, angle / PI, angle2 / PI);
	baseAngle = -0.5f * PI;
	printf("%s \t= IsCloserToAngle(%f pi, %f pi, %f pi)\n", IsCloserToAngle(baseAngle, angle, angle2) ? "true" : "false",
		   baseAngle / PI, angle / PI, angle2 / PI);

	return 0;
}
