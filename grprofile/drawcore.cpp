//
// profile/drawcore.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "grprofile/drawcore.h"
#include "math/simplemath.h"

#if __PFDRAW

void rage::pfDrawArrow (const Vector3& start, const Vector3& end)
{
	Vector3 line(end);
	line.Subtract(start);
	float lineMag2 = line.Mag2();
	int numPoints = (lineMag2>0.01f ? 10 : 2);
	grcBegin(drawLines,numPoints);

	grcLighting(false);

	grcVertex3fv(&start[0]);
	grcVertex3fv(&end[0]);

	if (numPoints>2)
	{
		Vector3 c0;
		switch (MinimumIndex(fabsf(line.x),fabsf(line.y),fabsf(line.z)))
		{
		case 0:
			{
				c0.Set(XAXIS);
				break;
			}
		case 1:
			{
				c0.Set(YAXIS);
				break;
			}
		case 2:
			{
				c0.Set(ZAXIS);
				break;
			}
		}

		Vector3 c1(line);
		c1.Cross(c0);
		c1.Scale(0.05f*sqrtf(lineMag2/c1.Mag2()));

		Vector3 v0(start);
		v0.AddScaled(line,0.9f);
		Vector3 v(v0);
		v.Add(c1);
		grcVertex3fv(&v[0]);
		grcVertex3fv(&end[0]);

		v.Subtract(v0,c1);
		grcVertex3fv(&v[0]);
		grcVertex3fv(&end[0]);

		v.Cross(line,c1);
		v.Scale(0.05f*sqrtf(lineMag2/v.Mag2()));
		c1.Set(v);

		v.Add(v0,c1);
		grcVertex3fv(&v[0]);
		grcVertex3fv(&end[0]);

		v.Subtract(v0,c1);
		grcVertex3fv(&v[0]);
		grcVertex3fv(&end[0]);
	}

	grcEnd();
}

#endif // __PFDRAW

// <eof> profile/drawcore.cpp
